/*
 * tcp_client_async.cpp
 *
 *  Created on: 2021年4月14日
 *      Author: jianzhao
 *
 *  功能：分别使用单独线程进行数据的发送和接收
 *  目的：客户端视角，计算发送+接收的吞吐量
 */

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iomanip>
#include <map>
#include <vector>
#include <algorithm>
#include <numeric>
#include <math.h>

#include "helper.h"
#include "packet.h"

using namespace std;


/********************************************
 ** 		       全局变量			       **
 ********************************************/
int gSendBeginTime 	= 0; // 开始发送时间
int gSize 			= 0; // 每次发送数据包大小，单位：字节
long gCount 		= 0; // request-response次数，总消息量 = count * 2


/********************************************
 ** 		       常量宏定义			       **
 ********************************************/
/* 服务端监听端口 */
#define SERVER_PORT 8080


/**
 * 采样点记录
 */
typedef struct _SSamplePoint {
	int64 time;  // 单位：微秒
	int64 count; // time时间内的消息数
} SSamplePointT;

/**
 * 读线程，负责数据读取
 */
void *Read(void *arg) {
	int cfd = *(int*)arg;

	int msgLen = MSG_LEN(gSize);
	char *buf = (char*) malloc(msgLen);
	if (buf == NULL) {
		perror("malloc");
		return NULL;
	}
	memset(buf, 0, msgLen);

	/* 接收并统计信息 */
	//	SSamplePointT *report = (SSamplePointT *) malloc(count * sizeof(SSamplePointT));
	//	int64 begTs = STime_GetMicrosecondsTime();
	//	int64 recvCount = 0;

	for (int i = 0; i < gCount; i++) {
		int len = 0;
		for (int j = 0; j < gSize;) {
			len = read(cfd, buf, msgLen - j);
			if (len == -1) {
				perror("read");
				return NULL;
			}
			j += len;
		}

		/**
		 * TODO
		 * 增加每条统计明细
		 * 	- 配合发送端明细，统计系统每秒的吞吐量，便于计算吞吐量的分位数及稳定性
		 */


		if (debug > 2) {
			printf(">>>> recv(%d), [%s]\n", i, buf+HEADER_LEN);
		}
	}

	/* 吞吐量统计 */
	int span = STime_GetMicrosecondsTime() - gSendBeginTime;
    printf("average throughput: %li msg/s\n", (gCount * 1000000) / span);
    printf("average throughput: %li Mb/s\n",
           (((gCount * 1000000) / span) * gSize * 8) / 1000000);

	/* 资源释放 */
    return NULL;
}

/**
 * 写线程，负责数据发送
 */
void *Write(void *arg)
{
	int cfd = *(int*)arg;

	/* 构建发送消息 */
	int msgLen = 0;
	char *pPkgBuf = Pkt_BuildRequest(gSize, &msgLen);
	if (pPkgBuf == NULL) {
		cout << "Pkt_BuildRequest failed" << endl;
		return NULL;
	}

	if (debug > 2) {
    	char *p = pPkgBuf;

#ifdef PKT_WITH_HEADER
    	int64 a = 0;
    	memcpy(&a, pPkgBuf, HEADER_LEN_SEND_US);
    	printf("%lld\n", a);
#endif

    	p += HEADER_LEN;
    	printf("%s\n", p);
	}

	/* 记录开始发送时间 */
	gSendBeginTime = STime_GetMicrosecondsTime();
	int sendTs = 0;

	/* 发送消息 */
	for (int i = 0; i < gCount; i++) {
		if (debug > 2) {
			sendTs = STime_GetMicrosecondsTime();
		}

		if (write(cfd, pPkgBuf, msgLen) != msgLen) {
			perror("wirte");
			return NULL;
		}

		if (debug > 2) {
			int64 rrTs = STime_GetMicrosecondsTime() - sendTs;
			printf("<<<< send(%d), [%s]\n", i,  pPkgBuf+HEADER_LEN);
			printf("current latency(rr): %lld(us)\n", rrTs);
		}
	}

	/* 释放资源 */
	Pkt_FreeRequest(pPkgBuf);

	return NULL;
}

int main(int argc, char *argv[]) {
	if (argc < 6) {
		printf("usage: ./tcp_client [local ip] [remote ip] [remote port] [msg-size] [rt-count]\n"
			   "eg: ./tcp_client 192.168.0.11 192.168.0.21 8080 64 10000\n");
		return -1;
	}

	char *localHost 	= argv[1]; 				// 指定发送IP
	char *remoteHost 	= argv[2];				// 指定远端IP
	int remotePort 		= atoi(argv[3]); 		// 指定远端端口
	gSize 				= atoi(argv[4]); 		// 每次发送数据包大小，单位：字节
	gCount 				= atol(argv[5]); 		// 往返次数，总消息量 = count * 2
	printf("\n from: [%s], to: [%s:%d], size: [%d], round-trip count: [%ld]\n",
			localHost, remoteHost, remotePort, gSize, gCount);

	int cfd = -1;
	pthread_t rid = NULL, wid = NULL;
	do {
		/* 初始化客户端套接字 */
		cfd = socket(AF_INET /* IPV4 */, SOCK_STREAM /* TCP */, 0 /* IP */);
		if (cfd < 0) {
			perror("socket creation");
			return -1;
		}

		/* 绑定地址到套接字，指定发送IP */
		struct sockaddr_in addr;
		int addrlen = sizeof(addr);
		memset(&addr, 0, addrlen);
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(localHost); // 指定发送IP
		if (bind(cfd, (struct sockaddr*) &addr, addrlen) < 0) {
			perror("bind failed");
			break;
		}

		/* 设置服务端信息 */
		struct sockaddr_in saddr;
		saddr.sin_family = AF_INET;
		saddr.sin_port = htons(remotePort);
		if (inet_pton(AF_INET, remoteHost, &saddr.sin_addr) <= 0) {
			perror("inet_pton");
			break;
		}

		/* 连接服务器 */
		if (connect(cfd, (struct sockaddr*) &saddr, sizeof(saddr)) < 0) {
			perror("connect");
			break;
		}

		/* 启动发送、接收线程 */
		if(pthread_create(&rid, NULL, Read, &cfd)) {
			perror("pthread_create(read)");
			break;
		}
		if(pthread_create(&wid, NULL, Write, &cfd)) {
			perror("pthread_create(write)");
			break;
		}
	} while(0);

	/* 释放资源 */
	pthread_join(rid, NULL);
	pthread_join(wid, NULL);

	if (cfd > 0 && close(cfd) == -1) {
		perror("close");
		return -1;
	}

	return 0;
}

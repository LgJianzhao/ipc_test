/*
 * udp_server.cpp
 *
 *  Created on: 2021年4月18日
 *      Author: jianzhao
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <algorithm>
#include <numeric>

#include "helper.h"
#include "packet.h"

using namespace std;

/**
 *  服务运行方式，是否回射数据
 *  0: 不回射，默认值
 *  1: 回射，同步方式
 *  通过启动参数可修改运行方式
 */
int gRunWithEcho = 0;

typedef struct _SReportT{
	int lat; 	// 延迟，单位：微妙
	int loss; 	// 丢包数
	int ooo; 	// TODO 乱序 out-of-order
} SReportT;

bool Comp(const SReportT &a, const SReportT &b){
	return a.lat < b.lat;
}

class Add {
 public:
	int operator()(int a, const SReportT& b) {
		return (a + b.lat);
	}
};

/**
 * 打印性能报告
 */
int
PrintPerfReport(SReportT *pReport, int size, int count, int elapsed /* us */) {

	vector<SReportT> vec(pReport, pReport + count);
	sort(vec.begin(), vec.end(), Comp);

	if (debug > 2) {
		printf("vec size: %lu\n", vec.size());
		for (size_t idx = 0; idx < vec.size(); ++idx) {
			printf("vec[%lu] == %d\n", idx, vec[idx].lat);
		}
	}
	if (vec.size() == 0) {
		printf("empty report\n");
		return 0;
	}

	/*
	 * 计算 延迟指标（最值、分位数、抖动）
	 * 计算 吞吐量指标（trans/s Mbits/s）
	 */
	int sum = accumulate(vec.begin(), vec.end(), 0, Add());
	int min = vec.front().lat;
	int max = vec.back().lat;
	int avg = sum / count;
	int p99 = vec[vec.size() * 0.99].lat;
	int p95 = vec[vec.size() * 0.95].lat;
	int p90 = vec[vec.size() * 0.90].lat;
	int p80 = vec[vec.size() * 0.80].lat;
	int p50 = vec[vec.size() * 0.50].lat;

	/* 吞吐量 带宽维度 Mbits/s */
	double elapseds = elapsed * 1.00 / 1000000;
	double thr =  size * 8.00 /* bits */ / elapseds / 1000000 /* 10^6bits */ * count;

	/* 吞吐量 消息数量维度（TPS）*/
	double trans = count*1.00 / elapseds;

	/* 抖动，标准差 */
	int s = 0;
	for (size_t i = 0; i < vec.size(); i++) {
		s += pow(vec[i].lat - avg, 2);
	}
	int sz = vec.size();
	double sdev = sqrt(s / sz);

	/* 统计信息，格式如下 */
	if (debug > 0) {
		printf("\n"
				"RR         Send     Elapsed  Trans     Lat   Lat   Lat   Lat   Lat   Lat   Lat   Lat   Jitter 	ThroughPut		\n"
				"Test       Message  Time     Rate      Avg   Max   Min   P99   P95   P90   P80   P50   StdDev 					\n"
				"count      bytes    secs.    per/s     us.   us.   us.   us.   us.   us.   us.   us.   us.    	10^6bits/sec	\n");
	}

	printf("%-11d%-9d%-9.2f%-10.2f%-6d%-6d%-6d%-6d%-6d%-6d%-6d%-6d%-8.2f%-13.2f\n",
			count, size, elapseds, trans, avg, max, min, p99, p95, p90, p80, p50, sdev, thr);

	return 1;
}

int main(int argc, char *argv[]) {
	if (argc < 4) {
		printf("usage: ./udp_server [listen port] [msg-size] [rt-count] [echo](optional)\n"
			   "eg: \n"
			   "run without [echo]: ./udp_server 8080 64 10000\n"
			   "run with [echo]: ./udp_server 8080 64 10000 1\n");
		return -1;
	}

	int port 	= atoi(argv[1]); // 监听端口
	int size 	= atoi(argv[2]); // 每次发送数据包大小，单位：字节
	long count 	= atol(argv[3]); // 消息发送/接收次数，总消息量 = count * 2
	if (argc > 4) {
		gRunWithEcho = atoi(argv[4]);
	}

	if (debug > 0) {
		printf("listen port: [%d], message size: [%d], rt-count: [%ld], echo: [%d]\n",
				port, size, count, gRunWithEcho);
	}

	/* 初始化服务端套接字 */
	int sfd = socket(AF_INET /* IPV4 */, SOCK_DGRAM /* UDP */, 0 /* IP */);
	if (sfd < 0) {
		perror("socket creation");
		return -1;
	}

	/* 设置套接字属性*/
	int opt = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		perror("setsockopt");
		return -1;
	}

	/* 绑定地址到套接字 */
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY; // 任意网卡数据
	addr.sin_port = htons(port);
	if (bind(sfd, (struct sockaddr*) &addr, addrlen) < 0) {
		perror("bind failed");
		return -1;
	}

	/* 接收数据并回射，记录 */
	int msgLen = MSG_LEN(size);
	char *pPktBuf = (char*) malloc(msgLen);
	if (pPktBuf == NULL) {
		perror("malloc");
		return 1;
	}

	/* 发送、接收并统计信息 */
	int64 startTs = STime_GetMicrosecondsTime();
	int64 perTs = 0;
	SReportT *pReport = (SReportT *) malloc(count * sizeof(SReportT));

	struct sockaddr_in caddr;
	socklen_t addrLen = sizeof(caddr);
	for (int i = 0; i < count; i++) {
		/* 接收数据 */
		int rLen = 0;
		for (rLen = 0; rLen < msgLen;) {
			int cLen = recvfrom(sfd, pPktBuf + rLen, msgLen - rLen, 0, (struct sockaddr *)&caddr, &addrLen);
			if (cLen == -1) {
				perror("read");
				return -1;
			}
			if (cLen == 0) {
				break;
			}
			rLen += cLen;
		}

		/* 打印接收数据详情 */
		if (debug > 2) {
#ifdef PKT_WITH_HEADER
			int64 sendTs = 0;
			memcpy(&sendTs, pPktBuf, HEADER_LEN_SEND_US);
			printf("send ts: %d\n", sendTs);
#endif
			printf("<<<< recv msg len: %d, [%s]\n", rLen, pPktBuf + HEADER_LEN);
		}

		/* 回射数据 */
		if (gRunWithEcho) {
			if (sendto(sfd, pPktBuf, msgLen, 0, (struct sockaddr *)&caddr, sizeof(caddr)) != msgLen) {
				perror("write");
				return 1;
			}
		}

		pReport[i].lat = STime_GetMicrosecondsTime() - perTs;
		pReport[i].loss = msgLen - rLen;
	}

	/* 关闭socket */
	if (close(sfd) == -1) {
		perror("close");
		return -1;
	}

	if (debug > 2) {
		int64 end_time_us = STime_GetMicrosecondsTime();
		printf("udp server exit, ctime(ns): %lld, time elapsed(us): %lld\n", end_time_us, end_time_us - startTs);
	}
	return 0;
}

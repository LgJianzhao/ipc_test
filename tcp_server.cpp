/*
 * ipc_perf_socket_tcp.cpp
 *
 *  Created on: 2021年4月13日
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

int main(int argc, char *argv[]) {
	if (argc < 4) {
		printf("usage: ./tcp_server [listen port] [msg-size] [rt-count] [echo](optional)\n"
			   "eg: \n"
			   "run without [echo]: ./tcp_server 8080 64 10000\n"
			   "run with [echo]: ./tcp_server 8080 64 10000 1");
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
	int sfd = socket(AF_INET /* IPV4 */, SOCK_STREAM /* TCP */, 0 /* IP */);
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

	/* 监听 */
	if (listen(sfd, 128) < 0) {
		perror("listen");
		return -1;
	}

	/* 接收连接请求 */
	int nfd = accept(sfd, (struct sockaddr*) &addr, (socklen_t*) &addrlen);
	if (nfd < 0) {
		perror("accept");
		return -1;
	}

	/* 接收数据并回射，记录 */
	int msgLen = MSG_LEN(size);
	char *buf = (char*) malloc(msgLen);
	if (buf == NULL) {
		perror("malloc");
		return 1;
	}

	int64 startTs = STime_GetMicrosecondsTime();
	for (int i = 0; i < count; i++) {
		/* 接收数据 */
		int rLen = 0;
		for (rLen = 0; rLen < msgLen;) {
			int cLen = read(nfd, buf + rLen, msgLen - rLen);
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
			memcpy(&sendTs, buf, HEADER_LEN_SEND_US);
			printf("send ts: %d\n", sendTs);
#endif
			printf("<<<< recv msg len: %d, [%s]\n", rLen, buf + HEADER_LEN);
		}

		/**
		 *  TODO
		 *  打上接收时间标签并写回，用于统计单向性能指标
		 */

		if (gRunWithEcho) {
			if (write(nfd, buf, msgLen) != msgLen) {
				perror("write");
				return 1;
			}
		}
	}

	/* 关闭socket */
	if (close(sfd) == -1) {
		perror("close");
		return -1;
	}

	if (debug > 2) {
		int64 end_time_us = STime_GetMicrosecondsTime();
		printf("tcp server exit, ctime(ns): %lld, time elapsed(us): %lld\n", end_time_us, end_time_us - startTs);
	}
	return 0;
}

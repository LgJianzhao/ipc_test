/*
 * udp_client.cpp
 *
 *  Created on: 2021年4月18日
 *      Author: jianzhao
 *
 *  功能：发送并接受数据
 *  目的：客户端视角，计算RR（request-response）模式下的网络延迟
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


/**
 *  服务运行方式，是否接收对端回射数据
 *  0: 不接收，默认值
 *  1: 接收，udp_server同时开启echo，否则阻塞
 *  通过启动参数可修改运行方式
 */
int gRecvEcho = 0;

typedef struct _SReportT{
	int lat; 	// 延迟，单位：微妙
	int loss; 	// 丢包数
	int ooo; 	// TODO 乱序 out-of-order
} SReportT;

bool comp(const SReportT &a, const SReportT &b){
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
	sort(vec.begin(), vec.end(), comp);

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

	/* 计算 */
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
	if (argc < 6) {
		printf("usage: ./udp_client [local ip] [remote ip] [remote port] [msg-size] [rt-count] [recv](optional)\n"
			 "eg: \n"
			 "run without [recv]: ./tcp_client 192.168.0.11 192.168.0.21 8080 64 10000\n"
			 "run with [recv]: ./tcp_client 192.168.0.11 192.168.0.21 8080 64 10000 1");
		return -1;
	}

	char *localHost 	= argv[1]; 				// 指定发送IP
	char *remoteHost 	= argv[2];				// 指定远端IP
	int remotePort 		= atoi(argv[3]); 		// 指定远端端口
	int size 			= atoi(argv[4]); 		// 每次发送数据包大小，单位：字节
	long count 			= atol(argv[5]); 		// 往返次数，总消息量 = count * 2
	if (argc > 6) {
		gRecvEcho = atoi(argv[6]);
	}
	if (debug > 0) {
		printf("\nfrom: [%s], to: [%s:%d], size: [%d], round-trip count: [%ld]\n",
			localHost, remoteHost, remotePort, size, count);
	}

	/* 初始化客户端套接字 */
	int cfd = socket(AF_INET /* IPV4 */, SOCK_DGRAM /* TCP */, 0 /* IP */);
	if (cfd < 0) {
		perror("socket creation");
		return -1;
	}

	/**
	 *  设置socket属性
	 *  SO_REUSEADDR:
	 *  - 可以立即使用在time_wait状态下的端口
	 *  - 疑问：根据测试来看，Mac下不指定也能立即使用，但在Linux下则必须指定，待确认
	 */
	int opt = 1;
	if (setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		perror("setsockopt");
		return -1;
	}

	/* 绑定地址到套接字，指定发送IP */
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);
	// printf("in.sin_port:%d", addr.sin_port);
	memset(&addr, 0, addrlen);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(localHost);
	if (bind(cfd, (struct sockaddr*) &addr, addrlen) < 0) {
		perror("bind failed");
		return -1;
	}

	/* 设置服务端信息 */
	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(remotePort);
	if (inet_pton(AF_INET, remoteHost, &saddr.sin_addr) <= 0) {
		perror("inet_pton");
		return -1;
	}

	/* 构建发送包 */
	int pktLen = 0;
	char *pPktbuf = Pkt_BuildRequest(size, &pktLen);
	if (pPktbuf == NULL) {
		printf("Pkt_BuildRequest failed\n");
		return -1;
	}

	if (debug > 2) {
    	char *p = pPktbuf;

#ifdef PKT_WITH_HEADER
    	int64 a = 0;
    	memcpy(&a, pPktbuf, HEADER_LEN_SEND_US);
    	printf("send time: %lld\n", a);
#endif

    	p += HEADER_LEN;
    	printf("pkt: %s\n", p);
	}

	/* 发送、接收并统计信息 */
	int64 perTs = 0;
	int64 startTs = STime_GetMicrosecondsTime();
	SReportT *pReport = (SReportT *) malloc(count * sizeof(SReportT));

	for (int i = 0; i < count; i++) {

#ifdef PKT_WITH_HEADER
		pPktbuf = Pkt_SetSendTime(pPktbuf, sendTs);
		pPktbuf += HEADER_LEN_SEND_US;
#endif

		/* 记录发送时间戳 */
		perTs = STime_GetMicrosecondsTime();

		if (sendto(cfd, pPktbuf, pktLen, 0, (struct sockaddr *)&saddr, sizeof(saddr)) != pktLen) {
			perror("wirte");
			goto END;
		}

		if (gRecvEcho) {
			int cLen = 0;
			for (int sofar = 0; sofar < size;) {
				cLen = read(cfd, pPktbuf + sofar, pktLen - sofar);
				if (cLen == -1) {
					perror("read");
					goto END;
				}

				if (cLen == 0) {
					break;
				}
				sofar += cLen;
			}
		}

		pReport[i].lat = STime_GetMicrosecondsTime() - perTs;

		if (debug > 2) {
			printf("rr-msg (%d), [%s] \n current latency(rr): %d \n", i, pPktbuf+HEADER_LEN, pReport[i].lat);
		}
	}

END:
	/**
	 * 输出性能报告
	 */
	PrintPerfReport(pReport, size, count, STime_GetMicrosecondsTime()-startTs);

	/* 释放资源 */
	Pkt_FreeRequest(pPktbuf);
	SAFE_FREE(pReport);
	if (close(cfd) == -1) {
		perror("close");
		return -1;
	}

	return 0;
}

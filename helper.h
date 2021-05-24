/*
 * tools.h
 *
 *  Created on: 2021年4月13日
 *      Author: jianzhao
 */

#ifndef TIMES_H_
#define TIMES_H_

#include <string>
#include <sys/time.h>
#include <vector>
#include <algorithm>
#include <numeric>

using namespace std;

#define int64 long long

/**
 * debug日志等级
 * 0：仅输出结果，不包含头
 * 1：输出结果及头
 * 3：输出详细收发信息
 */
int debug = 0;

/**
 * 返回毫秒级的当前时间
 *
 * @return  相对与UTC 1970年1月1日零时的毫秒数
 */
static inline int64
STime_GetMillisecondsTime() {
	struct timeval tv = {0, 0};

	gettimeofday(&tv, NULL);
    return (int64) tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/**
 *
 * @return  相对与UTC 1970年1月1日零时的微妙数
 */
static inline int64
STime_GetMicrosecondsTime() {
	struct timeval tv = {0, 0};

	gettimeofday(&tv, NULL);
    return (int64) tv.tv_sec * 1000000 + tv.tv_usec;
}

/**
 *
 * @return  相对与UTC 1970年1月1日零时的纳秒数
 */
static inline int64
STime_GetNanosecondsTime() {
	timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
    return (int64) ts.tv_sec * 1000000000 + ts.tv_nsec;
}


/**
 * 打印性能报告
 */
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
	int operator()(int a, const SReportT &b) {
		return (a + b.lat);
	}
};

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
			"RR         Send     Elapsed  Trans     Lat   Lat   Lat   Lat   Lat   Lat   Lat   Lat   Jitter 	ThroughPut	\n"
			"Test       Message  Time     Rate      Avg   Max   Min   P99   P95   P90   P80   P50   StdDev 			\n"
			"count      bytes    secs.    per/s     us.   us.   us.   us.   us.   us.   us.   us.   us.    	10^6bits/sec	\n");
	}

	printf("%-11d%-9d%-9.2f%-10.2f%-6d%-6d%-6d%-6d%-6d%-6d%-6d%-6d%-8.2f%-13.2f\n",
			count, size, elapseds, trans, avg, max, min, p99, p95, p90, p80, p50, sdev, thr);

	return 1;
}

/**
 * 打印性能报告 V1
 */
int
PrintPerfReport_V1(int *pReport, int size, int count, int elapsed /* us */) {

	vector<int> vec(pReport, pReport + count);
	sort(vec.begin(), vec.end());

	if (debug > 2) {
		printf("vec size: %lu\n", vec.size());
		for (size_t idx = 0; idx < vec.size(); ++idx) {
			printf("vec[%lu] == %d\n", idx, vec[idx]);
		}
	}
	if (vec.size() == 0) {
		printf("empty report\n");
		return 0;
	}

	/* 计算 */
	int sum = accumulate(vec.begin(), vec.end(), 0);
	int min = vec.front();
	int max = vec.back();
	int avg = sum / count;
	int p99 = vec[vec.size() * 0.99];
	int p95 = vec[vec.size() * 0.95];
	int p90 = vec[vec.size() * 0.90];
	int p80 = vec[vec.size() * 0.80];
	int p50 = vec[vec.size() * 0.50];

	/* 吞吐量 带宽维度 Mbits/s */
	double elapseds = elapsed * 1.00 / 1000000;
	double thr =  size * 8.00 /* bits */ / elapseds / 1000000 /* 10^6bits */ * count;

	/* 吞吐量 消息数量维度（TPS）*/
	double trans = count*1.00 / elapseds;

	/* 抖动，标准差 */
	int s = 0;
	for (size_t i = 0; i < vec.size(); i++) {
		s += pow(vec[i] - avg, 2);
	}
	int sz = vec.size();
	double sdev = sqrt(s / sz);

	/* 统计信息，格式如下 */
	if (debug > 0) {
		printf("\n"
			"RR         Send     Elapsed  Trans     Lat   Lat   Lat   Lat   Lat   Lat   Lat   Lat   Jitter 	ThroughPut	\n"
			"Test       Message  Time     Rate      Avg   Max   Min   P99   P95   P90   P80   P50   StdDev 			\n"
			"count      bytes    secs.    per/s     us.   us.   us.   us.   us.   us.   us.   us.   us.    	10^6bits/sec	\n");
	}

	printf("%-11d%-9d%-9.2f%-10.2f%-6d%-6d%-6d%-6d%-6d%-6d%-6d%-6d%-8.2f%-13.2f\n",
			count, size, elapseds, trans, avg, max, min, p99, p95, p90, p80, p50, sdev, thr);

	return 1;
}


#endif /* TIMES_H_ */

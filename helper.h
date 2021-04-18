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

#endif /* TIMES_H_ */

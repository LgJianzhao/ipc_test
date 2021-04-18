/*
 * packet.h
 *
 *  Created on: 2021年4月13日
 *      Author: jianzhao
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <string>
#include <string.h>
#include "helper.h"

/**
 * 控制发送消息体是否有header信息
 * 之前考虑复杂了，目前性能指标计算在client计算，暂不需要增加时间戳信息
 * 如果需要在服务端计算相关性能指标可能会需要该部分信息
 */
//#define PKT_WITH_HEADER

/**
 * 应用层消息报文格式
 * | -----------------  16 字节 header -------------- |
 * |   8字节(发送端微妙级时间戳) | 8字节(接收端微妙级时间戳) |
 * | ----------------------------------------------- |
 * |                      payload                    |
 * | ----------------------------------------------- |
 * TODO 增加宏定义，区分是否增加8字节的接收时间戳，即是否需要计算《单向》性能指标
 * TODO 增加序列号，统计UDP乱序
 */

typedef struct _SMsgHeader {
	int64 sendTimeNs;
	int64 recvTimeNs;
} SMsgHeaderT;

typedef struct _STcpMsg {

#ifdef PKT_WITH_HEADER
	SMsgHeaderT header;
#endif

	char 		*pPayload;
} STcpMsgT;

typedef STcpMsgT SUdpMsgT;


/* 包头发送、接收时间戳长度 */
#define HEADER_LEN_SEND_US ( sizeof(int64) )
#define HEADER_LEN_RECV_US ( HEADER_LEN_SEND_US )

/* 包头、包体长度 */
#ifdef PKT_WITH_HEADER
	#define MSG_LEN(bodyLen) ( HEADER_LEN + bodyLen )
	#define HEADER_LEN ( sizeof(SMsgHeaderT) )
#else
	#define MSG_LEN(bodyLen) ( bodyLen )
	#define HEADER_LEN (0)
#endif


/* 释放内存 */
#define SAFE_FREE(ptr) \
	do { \
		if (ptr != NULL) { \
			free((void *)ptr); \
			ptr = NULL; \
		} \
	} while(0)

/**************************************************
 *				请求包体方法						  *
 **************************************************/

/**
 * 解析头部信息
 * @param		msg		待解析消息
 * @param[out]	sendTs	发送时间戳，单位：ns
 * @param[out]	recvTs	接收时间戳，单位：ns
 * @return 		大于等于0, 成功；小于0, 失败
 */
static inline int
Pkt_ParseHeader(const char *pMsg, int & sendTs, int & recvTs) {

	return 1;
}

/**
 * 设置发送时间戳
 * @param		msg		消息
 * @return 		msg 	消息
 */
 static inline char *
 Pkt_SetSendTime(char *pMsg, int64 ctime) {
	 memcpy(pMsg, &ctime, HEADER_LEN_SEND_US);
	 return pMsg;
}

/**
 * 设置接收时间戳
 * @param[in/out]		msg		消息
 * @return 		大于等于0, 成功；小于0, 失败
 */
 static inline int
 Pkt_SetRecvTime(char **pMsg, int recvTs) {

	return 1;
}

/**
 * 构建发送包体（在size字节大小包体的基础上，额外增加SMsgHeaderT长度包头）
 * @param		size		payload字节数
 * @param[out]	effective	最终生成的有效字节数(指定包体长度加固定header长度)
 * @return	指向包体的指针
 */
static inline char *
Pkt_BuildRequest(int size, int *pEffective, int64 sendTs = 0) {
	if (size < 0) {
		return NULL;
	}

	int msgLen = size;
#ifdef PKT_WITH_HEADER
	msgLen = size + HEADER_LEN;
#endif
	*pEffective = msgLen;

	char *pBuf = (char*) malloc(msgLen);
	if (pBuf == NULL) {
		perror("malloc");
		return NULL;
	}

	char *p = pBuf;

#ifdef PKT_WITH_HEADER
	sendTs = (sendTs <= 0) ? STime_GetNanosecondsTime() : sizeTs;
	memcpy(buf, &sendTs, HEADER_LEN_SEND_US);
	buf += HEADER_LEN;
#endif

	string str = string(size, 'z');
	strncpy(pBuf, str.c_str(), size);
	p[size + HEADER_LEN] = '\0';

	return p;
}

/**
 * 释放发送包体内存
 * @param	指向待释放内存的指针
 * @return	无
 */
static inline void
Pkt_FreeRequest(const char *pBuf) {
	SAFE_FREE(pBuf);
}

#endif /* PACKET_H_ */

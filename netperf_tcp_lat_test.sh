#! /bin/sh

# 千兆网卡
gigabit=192.168.0.11

# 低延迟网卡（万兆）
lowLatency=192.168.10.11


# 测试用例1，不同发送消息大小对延迟的影响
sendSizes="1 32 64 128 256 512 1024 2048 4096 10240" # 发送消息大小，单位字节，需要跟服务端保持一致

echo "case1 测试不通发送消息对延迟的影响，基于千兆网卡，默认测试时间是10s"
for tsize in ${sendSizes}
do
	echo "netperf -H ${gigabit} -t omni -- -T tcp -d rr -r ${tsize},${tsize} -O "MEAN_LATENCY, MAX_LATENCY, MIN_LATENCY, P90_LATENCY, P50_LATENCY, STDDEV_LATENCY, THROUGHPUT, THROUGHPUT_UNITS""
	netperf -H ${gigabit} -t omni -- -T tcp -d rr -r ${tsize},${tsize} -O "MEAN_LATENCY, MAX_LATENCY, MIN_LATENCY, P90_LATENCY, P50_LATENCY, STDDEV_LATENCY, THROUGHPUT, THROUGHPUT_UNITS"
done


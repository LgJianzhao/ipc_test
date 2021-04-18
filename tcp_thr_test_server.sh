#! /bin/sh

server=tcp_server

runTimes=10			# 测试次数		
port=""				# 监听端口
sendSize=10000		# 发送消息大小，单位字节，*跟客户端保持一致
rrCount=0			# round-trip 次数，*跟客户端保持一致
withEcho=1 			# 是否开启回射


# 测试用例1，不同发送消息大小对延迟的影响
sendSizes="1 32 64 128 256 512 1024 2048 4096 10240" # 发送消息大小，单位字节，需要跟客户端保持一致


# 测试用例2，不通网卡对延迟的影响
hosts="127.0.0.1 127.0.0.1"

while getopts "p:s:c:m:" arg
do
    case $arg in
        p)
			port="$OPTARG"
			;;
        s)
			sendSize="$OPTARG"
			;;			
        c)
			rrCount="$OPTARG"
			;;
        m)
			withEcho="$OPTARG"
			;;			
        ?)
			echo "unkonw argument"
			exit 1
		;;
    esac
done


# 运行前的一些校验
if [ ! -x ${server} ]; 		then echo "${server} not found."; 	exit 1; fi
if [[ -z ${port} ]]; 		then echo "没有指定本地端口"; 			exit 1; fi
if [[ ${rrCount} -eq 0 ]]; 	then echo "没有指定往返次数"; 			exit 1; fi


# 开始运行
echo "********************************************************"
echo "case-1 消息大小对延迟的影响"
echo "********************************************************"
for tsize in ${sendSizes}
do
	echo ""
	echo "========================================================"
	echo "./${server} ${port} ${tsize} ${rrCount} ${withEcho}"
	echo "--------------------------------------------------------"
	for i in $(seq 1 ${runTimes}) 
	do
		# ./tcp_server 8080 131072 1000000
		./${server} ${port} ${tsize} ${rrCount} ${withEcho}
	done
done

echo ""
echo "********************************************************"
echo "case-1 网卡环境对延迟的影响"
echo "********************************************************"
for host in ${hosts}
do
	echo ""
	echo "========================================================"
	echo "./${server} ${port} ${sendSize} ${rrCount} ${withEcho}"
	echo "--------------------------------------------------------"
	for i in $(seq 1 ${runTimes}) 
	do
		# ./tcp_server 8080 131072 1000000
		./${server} ${port} ${sendSize} ${rrCount} ${withEcho}
	done
done


echo "test over"
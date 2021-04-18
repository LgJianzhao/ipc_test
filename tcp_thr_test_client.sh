#! /bin/sh

# 测试相关参数
client=tcp_client						# 进程名
localHost=""							# 指定发送IP
#remoteHost=""							# 对端IP
remotePort=""							# 对端端口
sendSize=10000							# 发送消息大小，单位字节
rrCount=0								# round-trip 次数
recvEnable=0							# 是否接收回射数据包，需要跟服务端保持一致，延迟测试需要打开该参数
runTimes=10								# 测试次数，每个用例跑10次，最终结果取10次均值
logFile=$0_`date '+%Y%m%d.%H%M%S'`.csv 	# 结果Excel文件

# 测试用例1，不同发送消息大小对延迟的影响
sendSizes="1 32 64 128 256 512 1024 2048 4096 10240" # 发送消息大小，单位字节，需要跟服务端保持一致

# 测试用例2，不通网卡对延迟的影响
remoteHosts="127.0.0.1 127.0.0.1"

while getopts "l:h:p:s:c:m:" arg
do
    case $arg in
        l)
			localHost="$OPTARG"
			;;
        h)
			remoteHost="$OPTARG"
			;;
        p)
			remotePort="$OPTARG"
			;;
        s)
			sendSize="$OPTARG"
			;;			
        c)
			rrCount="$OPTARG"
			;;
        m)
			recvEnable"=$OPTARG"
			;;			
        ?)
			echo "unkonw argument"
			exit 1
		;;
    esac
done

# 运行前的一些校验
if [ ! -x ${client} ]; 		then echo "${client} not found."; 	exit 1; fi
if [[ -z ${localHost} ]]; 	then echo "没有指定本地IP"; 			exit 1; fi
if [[ -z ${remotePort} ]]; 	then echo "没有指定远端端口"; 			exit 1; fi
if [ ${rrCount} -eq 0 ]; 	then echo "没有指定往返次数"; 			exit 1; fi


#if [ $# -ne 4 ]
#then
#	echo "usage: [localhost] [remotehost] [remoteport] [send size] [round-trip count]
#	eg: ./$0 127.0.0.1 127.0.0.1 8080 1000000"
#	exit 1
#fi

# case-1
echo "********************************************************"
echo "case-1 消息大小对延迟的影响"
echo "********************************************************"
if [[ -z ${remoteHost} ]]; 	then echo "没有指定远端IP"; 			exit 1; fi
for tsize in ${sendSizes}
do
	echo ""
	echo "========================================================"
	echo "./${client} ${localHost} ${remoteHost} ${remotePort} ${tsize} ${rrCount} ${recvEnable}"
	echo "./${client} ${localHost} ${remoteHost} ${remotePort} ${tsize} ${rrCount} ${recvEnable}" >> ${logFile}
	echo "--------------------------------------------------------"
	for i in $(seq 1 ${runTimes}) 
	do
		# ./tcp_client 0.0.0.0 0.0.0.0 8080 32 1000000 1
		rslt=`./${client} ${localHost} ${remoteHost} ${remotePort} ${tsize} ${rrCount} ${recvEnable}`
		echo $rslt
		echo $rslt | sed 's/ /,/g' >> ${logFile}
		sleep 0.5 # 等服务端启动，待优化
	done
done

# case-2
echo ""
echo "********************************************************"
echo "case-2 网卡环境对延迟的影响"
echo "********************************************************"
for remoteHost in ${remoteHosts}
do
	echo ""
	echo "========================================================"
	echo "./${client} ${localHost} ${remoteHost} ${remotePort} ${sendSize} ${rrCount} ${recvEnable}"
	echo "./${client} ${localHost} ${remoteHost} ${remotePort} ${sendSize} ${rrCount} ${recvEnable}" >> ${logFile}
	echo "--------------------------------------------------------"
	for i in $(seq 1 ${runTimes}) 
	do
		# ./tcp_client 0.0.0.0 0.0.0.0 8080 32 1000000 1
		rslt=`./${client} ${localHost} ${remoteHost} ${remotePort} ${sendSize} ${rrCount} ${recvEnable}`
		echo $rslt
		echo $rslt | sed 's/ /,/g' >> ${logFile}
		sleep 0.5 # 等服务端启动，待优化
	done
done

echo "test over"

#! /bin/sh

# 测试相关参数
client=udp_client                                               # 进程名
runTimes=10                                                             # 测试次数，每个用例跑10次，最终结果取10次均值
logFile=$0_`date '+%Y%m%d.%H%M%S'`.csv  # 结果Excel文件

localHost=""                                                    # 指定发送IP
remotePort=""                                                   # 对端端口
sendSize=10000                                                  # 发送消息大小，单位字节
rrCount=0                                                               # round-trip 次数
recvEcho=1                                                    # 是否接收回射数据包，需要跟服务端保持一致，延迟测试需要打开该参数

usage() {
    echo -e "usage:\n sh $0 -l 'local ip' -h 'remote ip' -p 'remote port' -c 'rr count'"
    echo -e "eg:\n sh $0 -l 192.168.0.11 -h 192.168.0.21 -p 8080 -c 10000"
    exit 1
}

if [[ $# -lt 4 ]]; then
    usage
fi

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
                        recvEcho="$OPTARG"
                        ;;
        ?)
                        echo "unkonw argument"
                        exit 1
                ;;
    esac
done

# 运行前的一些校验
if [ ! -x ${client} ];          then echo "${client} not found.";       exit 1; fi
if [[ -z ${localHost} ]];       then echo "没有指定本地IP";                     exit 1; fi # case1使用
if [[ -z ${remoteHost} ]];      then echo "没有指定远端IP";                     exit 1; fi # case1使用
if [[ -z ${remotePort} ]];      then echo "没有指定远端端口";                   exit 1; fi
if [ ${rrCount} -eq 0 ];        then echo "没有指定往返次数";                   exit 1; fi

run() {
    onload=$1           # 是否onload模式
    sendSizeArr=$2      # 发送数据大小
    echo ${sendSizeArr}

    for tsize in ${sendSizeArr}
    do
        cmd="./${client} ${localHost} ${remoteHost} ${remotePort} ${tsize} ${rrCount} ${recvEcho}"
        if [[ ${onload} -eq 1 ]];then
            cmd="onload --profile=latency ./${client} ${localHost} ${remoteHost} ${remotePort} ${tsize} ${rrCount} ${recvEcho} ${time}"
        fi

        echo -e "\n========================================================"
        time=`date '+%Y%m%d.%H%M%S'`
        #echo ${cmd} | tee ${logFile}
        echo -e "--------------------------------------------------------\n"

        # 执行指定次数,最终测试数据取其均值
        for ((i=0; i < ${runTimes}; i++))
        do
            echo  ${cmd}
            rslt=`$cmd`
            echo $rslt
            echo $rslt | sed 's/ /,/g' >> ${logFile}

            sleep 0.5 # 等服务端启动，待优化
        done
    done
}

# case-1
echo "********************************************************"
echo -e "\ncase-1 消息大小对延迟的影响" 
echo -e "\ncase-1 消息大小对延迟的影响" >> ${logFile}
echo "********************************************************"

# 测试用例1，不同发送消息大小对延迟的影响
sendSizes="1 2 4 8 16 32 64 128 256 512 1024 2048 4096 10240" # 发送消息大小，单位字节，需要跟服务端保持一致
#run 0 "${sendSizes[*]}"

# case-2
# 测试用例2，网卡对延迟的影响
sendSizes="10240"
echo -e "\n********************************************************"
echo -e "\ncase-2 网卡环境对延迟的影响" 
echo -e "\ncase-2 网卡环境对延迟的影响" >> ${logFile}
echo "********************************************************"

# 千兆网卡发送测试, 千兆《=》千兆 千兆《=》万兆
localHost="192.168.0.11"
remoteHosts="192.168.0.21 192.168.10.21"
for remoteHost in ${remoteHosts}
do
    run 0  "${sendSizes[*]}"
done

# 万兆网卡发送测试, 万兆《=》万兆 万兆《=》onload
localHost="192.168.10.11"
remoteHosts="192.168.10.21 192.168.10.21"
for remoteHost in ${remoteHosts}
do
    echo -e "\n万兆网卡发送测试["${localHost}"]<==>["${remoteHost}"]"
    run 0 "${sendSizes[*]}"
done

# onload发送测试, onload《=》onload
localHost="192.168.10.11"
remoteHosts="192.168.10.21"
for remoteHost in ${remoteHosts}
do
    echo -e "\n万兆网卡onload发送测试["${localHost}"]<==>["${remoteHost}"]"
    run 1  "${sendSizes[*]}"
done

echo "test over"

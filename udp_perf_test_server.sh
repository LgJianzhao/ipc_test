#! /bin/sh

server=udp_server

runTimes=10                     # 测试次数
port=""                         # 监听端口
rrCount=0                       # round-trip 次数，*跟客户端保持一致
withEcho=1                      # 是否开启回射, 延迟测试必须开启

usage() {
    echo -e "usage:\n sh $0 -p 'listen port'  -c 'rr count'"
    echo -e "eg:\n sh $0 -p 8080 -c 10000"
    exit 1
}

if [[ $# -lt 2 ]]; then
    usage
fi

while getopts "p:c:m:" arg
do
    case $arg in
        p)
                        port="$OPTARG"
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
if [ ! -x ${server} ];          then echo "${server} not found.";       exit 1; fi
if [[ -z ${port} ]];            then echo "没有指定本地端口";                   exit 1; fi
if [[ ${rrCount} -eq 0 ]];      then echo "没有指定往返次数";                   exit 1; fi

run() {
    onload=$1           # 是否onload模式
    sendSizeArr=$2      # 发送数据大小
    echo ${sendSizeArr}

    for tsize in ${sendSizeArr}
    do
    # 执行命令
    cmd="./${server} ${port} ${tsize} ${rrCount} ${withEcho}"
    if [[ ${onload} -eq 1 ]];then
        cmd="onload --profile=latency ./${server} ${port} ${tsize} ${rrCount} ${withEcho} ${time}"
    fi

    # 输出测试信息
    echo -e "\n========================================================"
    time=`date '+%Y%m%d.%H%M%S'`
    #echo ${cmd}
    echo "--------------------------------------------------------"

    # 执行指定次数,最终测试数据取其均值
    for ((i=0; i < ${runTimes}; i++))
    do
        echo ${cmd}
        $cmd
    done

    done
}

echo "********************************************************"
echo "case-1 消息大小对延迟的影响"
echo "********************************************************"

# 测试用例1，不同发送消息大小对延迟的影响
sendSizes="1 2 4 8 16 32 64 128 256 512 1024 2048 4096 10240" # 发送消息大小，单位字节，需要跟客户端保持一致
run 0 "${sendSizes[*]}"

echo ""
echo "********************************************************"
echo "case-2 网卡环境对延迟的影响"
echo "********************************************************"
sendSizes="1 2048 10240"

# 千兆网卡运行次数
hosts="192.168.0.21"
for host in ${hosts}
do
    echo -e "\n千兆网卡接收测试 ["$host"]"
    run 0 "${sendSizes[*]}"
done

# 万兆低延迟网卡运行次数
hosts="192.168.10.21 192.168.10.21"
for host in ${hosts}
do
    echo -e "\n万兆网卡接收测试 ["$host"]"
    run 0 "${sendSizes[*]}"
done

# onload运行次数
hosts="192.168.10.21"
for host in ${hosts}
do
    echo -e "\n万兆网卡onload接收测试 ["$host"]"
    run 1 "${sendSizes[*]}"
done

echo "test over"
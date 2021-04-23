# IPC 性能测试

## 使用：
### TCP
1. 登陆服务器 && 启动服务端 
sh tcp_perf_test_server.sh -p 10097 -c 50000

2. 启动客户端
sh tcp_perf_test_client.sh -l 192.168.0.11 -h 192.168.0.21 -p 10097 -c 50000

3. 拷贝测试报告tcp_perf_test_client.sh_${date}.csv 到本地,这里分析
scp zhaoj@192.168.0.11:/home/zhaoj/code/ipc_perf_test/tcp_perf_test_client.sh_20210416.111446.csv .


### UDP
1. 登陆服务器 && 启动服务端 
sh udp_perf_test_server.sh -p 10097 -c 50000

2. 启动客户端
sh udp_perf_test_client.sh -l 192.168.0.11 -h 192.168.0.21 -p 10097 -c 50000

3. 拷贝测试报告udp_perf_test_client.sh_${date}.csv 到本地,这里分析
scp zhaoj@192.168.0.11:/home/zhaoj/code/ipc_perf_test/udp_perf_test_client.sh_20210416.111446.csv .


## 参考： 
1. https://www.geeksforgeeks.org/socket-programming-cc/
2. https://cloud.google.com/blog/products/networking/using-netperf-and-ping-to-measure-network-latency

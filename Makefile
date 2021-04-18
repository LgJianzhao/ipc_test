CC := g++
CFLAGS := -g -Wall -lpthread
#TARGET := tcp_client tcp_client_async tcp_server udp_client udp_server
TARGET := tcp_client tcp_client_async tcp_server udp_client udp_server


######################## tcp/udp ##########################
.PHONY: all
all: $(TARGET)


######################### tcp #############################
tcp_client: tcp_client.cpp helper.h packet.h
	$(CC) $(CFLAGS) -o $@ $<
# $(CC) $(CFLAGS) tcp_client.cpp -o tcp_client
#tcp_client:  times.h tcp_client.cpp
#	$(CC) $(CFLAGS) -o $@ $<

tcp_client_async: tcp_client_async.cpp helper.h packet.h 
	$(CC) $(CFLAGS) -o $@ $<

tcp_server: tcp_server.cpp helper.h packet.h
	$(CC) $(CFLAGS) -o $@ $<
	
	
######################### udp #############################
udp_client: udp_client.cpp helper.h packet.h
	$(CC) $(CFLAGS) -o $@ $<
	
udp_server: udp_server.cpp helper.h packet.h
	$(CC) $(CFLAGS) -o $@ $<
	
clean:
	rm -rf tcp_client tcp_client_async tcp_server udp_client udp_server

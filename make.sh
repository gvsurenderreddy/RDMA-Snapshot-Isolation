g++ -c -Wno-write-strings src/util/utils.cpp

g++ -c -Wno-write-strings src/auxilary/lock.cpp

g++ -c -Wno-write-strings src/util/RDMACommon.cpp

g++ -c -Wno-write-strings src/rdma-SI/rdma-server.cpp
g++ utils.o lock.o RDMACommon.o rdma-server.o -libverbs -lpthread -o rdma-server
#g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/traditional-server.cpp
#g++ utils.o lock.o RDMACommon.o traditional-server.o -libverbs -lpthread -o traditional-server

g++ -c -Wno-write-strings src/rdma-SI/rdma-client.cpp
g++ utils.o lock.o RDMACommon.o rdma-client.o -libverbs -lpthread -o rdma-client
#g++ -c -Wno-write-strings src/traditional-SI/traditional-client.cpp
#g++ utils.o lock.o RDMACommon.o traditional-client.o -libverbs -lpthread -o traditional-client

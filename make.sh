g++ -c -Wno-write-strings src/util/utils.cpp


g++ -c -Wno-write-strings src/rdma/RDMACommon.cpp

g++ -c -Wno-write-strings src/rdma/server.cpp
g++ utils.o RDMACommon.o server.o -libverbs -lpthread -o server

g++ -c -Wno-write-strings src/rdma/client.cpp
g++ utils.o RDMACommon.o client.o -libverbs -lpthread -o client

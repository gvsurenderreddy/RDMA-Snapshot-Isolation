g++ -c -Wno-write-strings src/util/utils.cpp

g++ -c -Wno-write-strings src/auxilary/lock.cpp

g++ -c -Wno-write-strings src/util/RDMACommon.cpp

g++ -c -Wno-write-strings -std=c++11 src/util/BaseContext.cpp


#g++ -c -Wno-write-strings -std=c++11 src/timestamp/timestamp-server.cpp
#g++ utils.o RDMACommon.o timestamp-server.o -libverbs -lpthread -o timestamp-server

g++ -c -Wno-write-strings -std=c++11 src/rdma-SI/RDMAServerContext.cpp
g++ -c -Wno-write-strings -std=c++11 src/rdma-SI/RDMAClientContext.cpp

g++ -c -Wno-write-strings -std=c++11 src/rdma-SI/RDMAServer.cpp
g++ utils.o lock.o RDMACommon.o BaseContext.o RDMAServerContext.o RDMAServer.o -libverbs -lpthread -o RDMAServer

g++ -c -Wno-write-strings -std=c++11 src/rdma-SI/RDMAClient.cpp
g++ utils.o lock.o RDMACommon.o BaseContext.o RDMAClientContext.o RDMAClient.o -libverbs -lpthread -o RDMAClient


g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradClientContext.cpp
g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradResManagerContext.cpp
g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradTrxManagerContext.cpp

g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradTrxManager.cpp
g++ utils.o lock.o RDMACommon.o BaseContext.o TradTrxManagerContext.o TradClientContext.o TradResManagerContext.o TradTrxManager.o -libverbs -lpthread -o TradTrxManager

g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradResManager.cpp
g++ utils.o lock.o RDMACommon.o BaseContext.o TradResManagerContext.o TradResManager.o -libverbs -lpthread -o TradResManager

g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradClient.cpp
g++ utils.o lock.o RDMACommon.o BaseContext.o TradClientContext.o TradClient.o -libverbs -lpthread -o TradClient





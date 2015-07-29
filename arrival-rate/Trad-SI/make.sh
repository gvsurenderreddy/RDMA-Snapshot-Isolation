mkdir -p ./obj/RDMA-SI
mkdir -p ./obj/Trad-SI


mkdir -p ./exe/RDMA-SI
mkdir -p ./exe/Trad-SI
mkdir -p ./exe/cache-effect


mkdir -p ./exe/cache-effect

g++ -Wall -c -Wno-write-strings src/util/utils.cpp	-o obj/utils.o

g++ -Wall -c -Wno-write-strings src/auxilary/lock.cpp	-o obj/lock.o

g++ -Wall -c -Wno-write-strings src/util/RDMACommon.cpp	-o obj/RDMACommon.o

g++ -Wall -c -Wno-write-strings src/util/zipf.cpp	-o obj/zipf.o

g++ -Wall  -c -Wno-write-strings -std=c++11 src/util/BaseContext.cpp -o obj/BaseContext.o


# g++ -Wall  -c -Wno-write-strings -std=c++11 src/micro-benchmarks/cache-effect/local_and_rdma_access/ServerContext.cpp	-o obj/ServerContext.o
# g++ -Wall  -c -Wno-write-strings -std=c++11 src/micro-benchmarks/cache-effect/local_and_rdma_access/ClientContext.cpp	-o obj/ClientContext.o


# g++ -Wall -I/usr/local/include -O0  -c -Wno-write-strings -std=c++11 src/micro-benchmarks/cache-effect/local_and_rdma_access/Server.cpp -o obj/Server.o
# g++ -Wall -I/usr/local/include -O0 obj/utils.o obj/RDMACommon.o obj/BaseContext.o obj/ServerContext.o obj/Server.o /usr/local/lib/libpapi.a -libverbs -lpthread -o exe/cache-effect/Server
#
# g++ -Wall  -I/usr/local/include -O0 -c -Wno-write-strings -std=c++11 src/micro-benchmarks/cache-effect/local_and_rdma_access/Client.cpp -o obj/Client.o
# g++ -Wall  -I/usr/local/include -O0 obj/utils.o obj/RDMACommon.o obj/BaseContext.o obj/ClientContext.o obj/Client.o /usr/local/lib/libpapi.a -libverbs -lpthread -o exe/cache-effect/Client


# g++ -Wall -O0  -c -Wno-write-strings -std=c++11 src/micro-benchmarks/cache-effect/local_and_rdma_access/Server.cpp -o obj/Server.o
# g++ -Wall -O0 obj/utils.o obj/RDMACommon.o obj/BaseContext.o obj/ServerContext.o obj/Server.o -libverbs -lpthread -o exe/cache-effect/Server
#
# g++ -Wall -O0 -c -Wno-write-strings -std=c++11 src/micro-benchmarks/cache-effect/local_and_rdma_access/Client.cpp -o obj/Client.o
# g++ -Wall -O0 obj/utils.o obj/RDMACommon.o obj/BaseContext.o obj/ClientContext.o obj/Client.o -libverbs -lpthread -o exe/cache-effect/Client




# g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/2PC/IPoIB/coordinator/IPCoordinatorContext.cpp
# g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/2PC/IPoIB/cohort/IPCohortContext.cpp
#
# g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/2PC/IPoIB/coordinator/IPCoordinator.cpp
# g++ utils.o RDMACommon.o BaseContext.o IPCoordinatorContext.o IPCoordinator.o -libverbs -lpthread -o IPCoordinator
#
# g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/2PC/IPoIB/cohort/IPCohort.cpp
# g++ utils.o RDMACommon.o BaseContext.o IPCohortContext.o IPCohort.o -libverbs -lpthread -o IPCohort



# g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/2PC/Send-Recv/coordinator/CoordinatorContext.cpp
# g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/2PC/Send-Recv/cohort/CohortContext.cpp
#
# g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/2PC/Send-Recv/coordinator/Coordinator.cpp
# g++ utils.o RDMACommon.o BaseContext.o CoordinatorContext.o Coordinator.o -libverbs -lpthread -o Coordinator
#
# g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/2PC/Send-Recv/cohort/Cohort.cpp
# g++ utils.o RDMACommon.o BaseContext.o CohortContext.o Cohort.o -libverbs -lpthread -o Cohort




# g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/simple-verbs/ServerContext.cpp
# g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/simple-verbs/ClientContext.cpp
#
# g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/simple-verbs/BenchmarkServerRDMA.cpp
# g++ utils.o RDMACommon.o BaseContext.o ServerContext.o BenchmarkServerRDMA.o -libverbs -lpthread -o BenchmarkServerRDMA
#
# g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/simple-verbs/BenchmarkClientRDMA.cpp
# g++ utils.o RDMACommon.o BaseContext.o ClientContext.o BenchmarkClientRDMA.o -libverbs -lpthread -o BenchmarkClientRDMA



#g++ -c -Wno-write-strings -std=c++11 src/timestamp/timestamp-server.cpp
#g++ utils.o RDMACommon.o timestamp-server.o -libverbs -lpthread -o timestamp-server




# g++ -Wall -c -Wno-write-strings -std=c++11 src/rdma-SI/RDMAServerContext.cpp -o obj/RDMA-SI/RDMAServerContext.o
# g++ -Wall -c -Wno-write-strings -std=c++11 src/rdma-SI/RDMAClientContext.cpp -o obj/RDMA-SI/RDMAClientContext.o
#
# g++ -Wall -c -Wno-write-strings -std=c++11 src/rdma-SI/RDMAServer.cpp -o obj/RDMA-SI/RDMAServer.o
# g++ -Wall obj/utils.o obj/lock.o obj/RDMACommon.o obj/BaseContext.o obj/RDMA-SI/RDMAServerContext.o obj/RDMA-SI/RDMAServer.o -libverbs -lpthread -o exe/RDMA-SI/RDMAServer
#
# g++ -Wall -c -Wno-write-strings -std=c++11 src/rdma-SI/RDMAClient.cpp -o obj/RDMA-SI/RDMAClient.o
# g++ -Wall obj/utils.o obj/lock.o obj/zipf.o obj/RDMACommon.o obj/BaseContext.o obj/RDMA-SI/RDMAClientContext.o obj/RDMA-SI/RDMAClient.o -libverbs -lpthread -o exe/RDMA-SI/RDMAClient


g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradClientContext.cpp -o obj/Trad-SI/TradClientContext.o
g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradResManagerContext.cpp -o obj/Trad-SI/TradResManagerContext.o
g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradTrxManagerContext.cpp -o obj/Trad-SI/TradTrxManagerContext.o

g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradTrxManager.cpp  -o obj/Trad-SI/TradTrxManager.o
g++ obj/utils.o obj/lock.o obj/RDMACommon.o obj/BaseContext.o obj/Trad-SI/TradTrxManagerContext.o obj/Trad-SI/TradClientContext.o obj/Trad-SI/TradResManagerContext.o obj/Trad-SI/TradTrxManager.o -libverbs -lpthread -o exe/Trad-SI/TradTrxManager

g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradResManager.cpp  -o obj/Trad-SI/TradResManager.o
g++ obj/utils.o obj/lock.o obj/RDMACommon.o obj/BaseContext.o obj/Trad-SI/TradResManagerContext.o obj/Trad-SI/TradResManager.o -libverbs -lpthread -o exe/Trad-SI/TradResManager

g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradClient.cpp  -o obj/Trad-SI/TradClient.o
g++ obj/utils.o obj/lock.o obj/zipf.o obj/RDMACommon.o obj/BaseContext.o obj/Trad-SI/TradClientContext.o obj/Trad-SI/TradClient.o -libverbs -lpthread -o exe/Trad-SI/TradClient



g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/BaseTradResManager.cpp -o obj/Trad-SI/BaseTradResManager.o
g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/BaseTradClient.cpp -o obj/Trad-SI/BaseTradClient.o
g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/BaseTradTrxManager.cpp -o obj/Trad-SI/BaseTradTrxManager.o


g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/IPoIB/IPTradResManagerContext.cpp -o obj/Trad-SI/IPTradResManagerContext.o
g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/IPoIB/IPTradClientContext.cpp -o obj/Trad-SI/IPTradClientContext.o
g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/IPoIB/IPTradTrxManagerContext.cpp -o obj/Trad-SI/IPTradTrxManagerContext.o


g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/IPoIB/IPTradTrxManager.cpp -o obj/Trad-SI/IPTradTrxManager.o
g++ obj/utils.o obj/lock.o obj/RDMACommon.o obj/BaseContext.o obj/Trad-SI/BaseTradTrxManager.o obj/Trad-SI/IPTradClientContext.o obj/Trad-SI/IPTradResManagerContext.o obj/Trad-SI/IPTradTrxManagerContext.o obj/Trad-SI/IPTradTrxManager.o -libverbs -lpthread -o exe/Trad-SI/IPTradTrxManager

g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/IPoIB/IPTradResManager.cpp -o obj/Trad-SI/IPTradResManager.o
g++ obj/utils.o obj/lock.o obj/RDMACommon.o obj/BaseContext.o obj/Trad-SI/BaseTradResManager.o obj/Trad-SI/IPTradResManagerContext.o obj/Trad-SI/IPTradResManager.o -libverbs -lpthread -o exe/Trad-SI/IPTradResManager

g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/IPoIB/IPTradClient.cpp -o obj/Trad-SI/IPTradClient.o
g++ obj/utils.o obj/lock.o obj/RDMACommon.o obj/BaseContext.o obj/zipf.o obj/Trad-SI/BaseTradClient.o obj/Trad-SI/IPTradClientContext.o obj/Trad-SI/IPTradClient.o -libverbs -lpthread -o exe/Trad-SI/IPTradClient

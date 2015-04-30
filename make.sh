g++ -c -Wno-write-strings src/util/utils.cpp

g++ -c -Wno-write-strings src/auxilary/lock.cpp

g++ -c -Wno-write-strings src/util/RDMACommon.cpp

g++ -c -Wno-write-strings -std=c++11 src/util/BaseContext.cpp





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






g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/simple-verbs/ServerContext.cpp
g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/simple-verbs/ClientContext.cpp

g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/simple-verbs/BenchmarkServerRDMA.cpp
g++ utils.o RDMACommon.o BaseContext.o ServerContext.o BenchmarkServerRDMA.o -libverbs -lpthread -o BenchmarkServerRDMA

g++ -c -Wno-write-strings -std=c++11 src/micro-benchmarks/simple-verbs/BenchmarkClientRDMA.cpp
g++ utils.o RDMACommon.o BaseContext.o ClientContext.o BenchmarkClientRDMA.o -libverbs -lpthread -o BenchmarkClientRDMA





#g++ -c -Wno-write-strings -std=c++11 src/timestamp/timestamp-server.cpp
#g++ utils.o RDMACommon.o timestamp-server.o -libverbs -lpthread -o timestamp-server




# g++ -c -Wno-write-strings -std=c++11 src/rdma-SI/RDMAServerContext.cpp
# g++ -c -Wno-write-strings -std=c++11 src/rdma-SI/RDMAClientContext.cpp
#
# g++ -c -Wno-write-strings -std=c++11 src/rdma-SI/RDMAServer.cpp
# g++ utils.o lock.o RDMACommon.o BaseContext.o RDMAServerContext.o RDMAServer.o -libverbs -lpthread -o RDMAServer
#
# g++ -c -Wno-write-strings -std=c++11 src/rdma-SI/RDMAClient.cpp
# g++ utils.o lock.o RDMACommon.o BaseContext.o RDMAClientContext.o RDMAClient.o -libverbs -lpthread -o RDMAClient
#
#
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradClientContext.cpp
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradResManagerContext.cpp
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradTrxManagerContext.cpp
#
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradTrxManager.cpp
# g++ utils.o lock.o RDMACommon.o BaseContext.o TradTrxManagerContext.o TradClientContext.o TradResManagerContext.o TradTrxManager.o -libverbs -lpthread -o TradTrxManager
#
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradResManager.cpp
# g++ utils.o lock.o RDMACommon.o BaseContext.o TradResManagerContext.o TradResManager.o -libverbs -lpthread -o TradResManager
#
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/TradClient.cpp
# g++ utils.o lock.o RDMACommon.o BaseContext.o TradClientContext.o TradClient.o -libverbs -lpthread -o TradClient
#
#
#
#
#
#
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/BaseTradResManager.cpp
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/BaseTradClient.cpp
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/BaseTradTrxManager.cpp
#
#
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/IPoIB/IPTradResManagerContext.cpp
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/IPoIB/IPTradClientContext.cpp
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/IPoIB/IPTradTrxManagerContext.cpp
#
#
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/IPoIB/IPTradTrxManager.cpp
# g++ utils.o lock.o RDMACommon.o BaseContext.o BaseTradTrxManager.o IPTradClientContext.o IPTradResManagerContext.o IPTradTrxManagerContext.o IPTradTrxManager.o -libverbs -lpthread -o IPTradTrxManager
#
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/IPoIB/IPTradResManager.cpp
# g++ utils.o lock.o RDMACommon.o BaseContext.o BaseTradResManager.o IPTradResManagerContext.o IPTradResManager.o -libverbs -lpthread -o IPTradResManager
#
# g++ -c -Wno-write-strings -std=c++11 src/traditional-SI/IPoIB/IPTradClient.cpp
# g++ utils.o lock.o RDMACommon.o BaseContext.o BaseTradClient.o IPTradClientContext.o IPTradClient.o -libverbs -lpthread -o IPTradClient

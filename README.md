# Notes on compiling and running the source code


- To compile the source code:***make***
- To delete the compiled files: ***make clean***


Instructions to execute the binaries for #c clients and #s servers :

1. First, run the timestamp oracle:
  - ***./exe/test.exe timestamp -n #c***
2. Second, run the memory server #i (0 <= i < #s):
  - ***./exe/test.exe server i -n #c***
3. Finally, run the clients by either:
  - one by one (this command has to be executed #c times):
    - ***./exe/test.exe client***  
  - all at once:
    - ***./scripts/run_clients.sh RDMA #c*** 


Note that the config file (located at *ROOT/config.hpp*) has to conform to the execution and the deployment parameters.
More specifically, the number of memory servers (*SERVER_CNT*) and their IP addresses (*SERVER_ADDR*, *TCP_PORT* and *IB_PORT*),
in addition to the IP address of the timestamp oracle (*TIMESTAMP_SERVER_ADDR*, *TIMESTAMP_SERVER_PORT* and *TIMESTAMP_SERVER_IB_PORT*) must match what is specified in the deployment.

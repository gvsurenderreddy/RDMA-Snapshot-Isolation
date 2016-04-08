# Notes on compiling and running the source code


- To compile the source code:***make***
- To delete the compiled files: ***make clean***


Instructions to execute the binaries for #c clients and #s servers :

1. First, run the timestamp oracle:  
```./exe/executor.exe oracle -n #c```
2. Second, run the memory server #i (0 <= i < #s):  
```./exe/executor.exe server i -b #BENCHMARK_NAME -n #c```
3. Finally, run the clients by either:
  - one by one (this command has to be executed #c times):  
  ```./exe/executor.exe client -b #BENCHMARK_NAME -p #PORT_NUM -i #INSTANCE_NUM"```
  - many clients at once:  
  ```./scripts/run_clients.sh -b #BENCHMARK_NAME -n #c -i #INSTANCE_NUM -o #NUMBER_OF_PORTS```

More details about the arguments:
  - Currently, **#BENCHMARK_NAME** could only be **TPCC**.
  - **#PORT_NUM** must be the number of a valid IB port (which can be queried using *ibstat* linux command).
  - **#INSTANCE_NUM** determines the deployment configuration of clients and servers. For the moment, please use 0.
  - **#NUMBER_OF_PORTS#** should be set to the number of IB ports to be used by the clients. In the best case, it should be set to total number of IB ports in the system, resulting in higher bandwidth.  

Note that the config file (located at *ROOT/config.hpp*) has to conform to the execution and the deployment parameters.
More specifically, the number of memory servers (*SERVER_CNT*) and their IP addresses (*SERVER_ADDR*, *TCP_PORT* and *IB_PORT*),
in addition to the IP address of the timestamp oracle (*TIMESTAMP_SERVER_ADDR*, *TIMESTAMP_SERVER_PORT* and *TIMESTAMP_SERVER_IB_PORT*) must match what is used in the deployment.

Should you have any questions, please contact me at erfanz \_at\_ cs \_dot\_ brown \_dot\_ edu.

#!/bin/bash

function usage {
	printf "Usage:\n"
	printf "$0 -b <TPCC | RDMA | Trad | IPTrad | Benchmark> <#threads_per_client>\n\n"
	printf "Example: for TPCC:\n"
	printf "$0 -b TPCC -n #clients_cnt -i #instance_number\n\n"
	printf "Example: for Micro Benchmarking:\n"
	printf "$0 Benchmark 40 -v READ -m SHARED\n"
}

if [ $# -lt 2 ] || [ $2 != "RDMA" -a $2 != "TPCC" -a  $2 != "Trad" -a $2 != "IPTrad" -a $2 != "Benchmark" ] ; then
	usage $0
	exit 0
fi

clients_num=$4
instance_number=$6

for x in `seq 1 $clients_num`;
do
	#echo "Launching transaction $x at $(date +%s%N | cut -b1-13)..." 
    if [ $2 = "RDMA" ]; then
  	  echo "Launching RDMA client -i 0 $x ..."
  	  ./exe/executor.exe client -i 0 &
    elif [ $2 = "TPCC" ]; then
  	  echo "Launching TPCC client $x on port $(( $x % 2 )) on instance $instance_number ..."
  	  ./exe/executor.exe client -b TPCC -p $(( $x % 2 + 1)) -i $instance_number &
    elif [ $2 = "Trad" ]; then
  	  echo "Launching Traditional client $x ..."
  	  ./exe/Trad-SI/TradClient &
    elif [ $2 = "IPTrad" ]; then
  	  echo "Launching IP Traditional client $x ..."
  	  ./exe/IPTrad/IPTradClient &
    elif [ $2 = "Benchmark" ]; then
  	  echo "Launching Benchmark client $x ..."
  	  ./exe/executor.exe client $3 $4 $5 $6 &
    fi
    #x=$(( $x + 1 ))
    sleep 0.00001
done
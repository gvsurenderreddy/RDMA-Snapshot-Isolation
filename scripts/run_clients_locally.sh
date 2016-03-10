#!/bin/bash

if [ $# -lt 2 ]; then
	echo "Usage: $0 <RDMA | TPCC | Trad | IPTrad | Benchmark> <#threads_per_client>"
	echo '\n'
	echo "For TPCC:"
	echo "$0 TPCC -n #clients_cnt -i #instance_number"
	echo '\n'
	echo "For Benchmarking:"
	echo "$0 Benchmark 40 -v READ -m SHARED"
	exit 0
fi

if [[ $1 != "RDMA" && $1 != "TPCC" &&  $1 != "Trad" && $1 != "IPTrad" && $1 != "Benchmark" ]]; then
	echo "Usage: $0 <RDMA | TPCC | Trad | IPTrad | Benchmark> <#threads_per_client>"
	exit 0
fi

threads_per_client=$3
instance_number=$5


for x in `seq 1 $threads_per_client`;
do
	#echo "Launching transaction $x at $(date +%s%N | cut -b1-13)..." 
    if [ $1 == "RDMA" ]; then
  	  echo "Launching RDMA client -i 0 $x ..."
  	  ./exe/test.exe client -i 0 &
    elif [ $1 == "TPCC" ]; then
  	  echo "Launching TPCC client $x on port $(( $x % 2 )) on instance $instance_number ..."
  	  ./exe/test.exe client -p $(( $x % 2 + 1)) -i $instance_number &
    elif [ $1 == "Trad" ]; then
  	  echo "Launching Traditional client $x ..."
  	  ./exe/Trad-SI/TradClient &
    elif [ $1 == "IPTrad" ]; then
  	  echo "Launching IP Traditional client $x ..."
  	  ./exe/IPTrad/IPTradClient &
    else [ $1 == "Benchmark" ]
  	  echo "Launching Benchmark client $x ..."
  	  ./exe/test.exe client $3 $4 $5 $6 &
    fi
    #x=$(( $x + 1 ))
    sleep 0.00001
done
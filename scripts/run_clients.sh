#!/bin/bash

if [ $# -lt 2 ]; then
	echo "Usage: $0 <RDMA | Trad | IPTrad | Benchmark> <#threads_per_client>"
	echo '\n'
	echo "For Benchmarking:"
	echo "./scripts/run_clients.sh Benchmark 40 -v READ -m SHARED"
	exit 0
fi

if [[ $1 != "RDMA" && $1 != "Trad" && $1 != "IPTrad" && $1 != "Benchmark" ]]; then
	echo "Usage: $0 <RDMA | Trad | IPTrad | Benchmark> <#threads_per_client>"
	exit 0
fi

threads_per_client=$2
x=1

while [ $x -le $threads_per_client ]
do
  #echo "Launching transaction $x at $(date +%s%N | cut -b1-13)..." 
  if [ $1 == "RDMA" ]; then
	  echo "Launching RDMA client $x ..."
	  ./exe/test.exe client &
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
  x=$(( $x + 1 ))
  sleep 0.00001
done
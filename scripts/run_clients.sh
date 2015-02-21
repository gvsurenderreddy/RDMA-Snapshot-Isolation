#!/bin/bash

if [ $# -ne 2 ]; then
	echo "Usage: $0 <RDMA | Trad> <#threads_per_client>"
	exit 0
fi

if [[ $1 != "RDMA" && $1 != "Trad" ]]; then
	echo "Usage: $0 <RDMA | Trad> <#threads_per_client>"
	exit 0
fi

threads_per_client=$2

x=1
while [ $x -le $threads_per_client ]
do
  #echo "Launching transaction $x at $(date +%s%N | cut -b1-13)..." 
  if [ $1 == "RDMA" ]; then
	  echo "Launching RDMA client $x ..."
	  ./RDMAClient & 
  else
	  echo "Launching Traditional client $x ..."
	  ./TradClient & 
  fi
  x=$(( $x + 1 ))
  sleep 0.1
done

#!/bin/bash
#ip="192.168.0.1"

if [ $# -ne 1 ]; then
	echo "Usage: $0 <#threads_per_client>"
	exit 0
fi

threads_per_client=$1

x=1
while [ $x -le $threads_per_client ]
do
  #echo "Launching transaction $x at $(date +%s%N | cut -b1-13)..." 
  echo "Launching client $x ..."
  ./rdma-client & 
  x=$(( $x + 1 ))
  sleep 0.1
done
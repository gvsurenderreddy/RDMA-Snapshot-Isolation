#!/bin/bash
#ip="192.168.0.1"

if [ $# -eq 0 ]; then
	echo "Usage: $0 <server-ip> <#threads_per_client>"
	exit 0
fi

ip=$1
threads_per_client=$2

x=1
while [ $x -le $threads_per_client ]
do
  #echo "Launching transaction $x at $(date +%s%N | cut -b1-13)..." 
  echo "Launching transaction $x ..."
  ./rdma-client $ip & 
  x=$(( $x + 1 ))
  sleep 0.1
done

#!/bin/bash
x=1
threads_per_client=$1
ip="192.168.1.1"

while [ $x -le $threads_per_client ]
do
  #echo "Launching transaction $x at $(date +%s%N | cut -b1-13)..." 
  echo "Launching transaction $x ..."
  ./client $ip & 
  x=$(( $x + 1 ))
  sleep 0.1
done

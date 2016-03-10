#!/bin/bash

usage ()
{ 
	echo "Usage: $0 [-f] <#client_threads_per_machine>"
	echo ''
	echo '-f forces recompile on remote machines before running the clients'
	exit 1
}

if [ $# -lt 1 ]; then
	usage
fi

username='erfanz'
machines=('bsn03.cs.brown.edu' 'bsn04.cs.brown.edu' 'bsn05.cs.brown.edu' 'bsn06.cs.brown.edu' 'bsn07.cs.brown.edu')
clients_cnt=$1
per_machine=$(( $clients_cnt / ${#machines[@]} ))
echo $per_machine

for m in "${machines[@]}"
do
	for m in "${machines[@]}"
	do
		ssh $username@$m '/data/erfanz/rdma/RDMA-Snapshot-Isolation/exe/test.exe '
	done
done


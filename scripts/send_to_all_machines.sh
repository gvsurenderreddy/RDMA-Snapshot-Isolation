#!/bin/bash

if [ $# -eq 0 ]; then
	echo "Needs at least one argument"
	exit 0
fi

file=$1
dest=""

if [ $# -eq 2 ]; then
	dest=$2
fi

echo "sending file(s) to /data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest"

scp -r  $file erfanz@bsn00.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest
scp -r  $file erfanz@bsn01.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest
scp -r  $file erfanz@bsn02.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest
scp -r  $file erfanz@bsn03.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest
scp -r  $file erfanz@bsn04.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest
scp -r  $file erfanz@bsn06.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest
scp -r  $file erfanz@bsn07.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest

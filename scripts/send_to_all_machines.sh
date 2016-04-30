#!/bin/bash

###################################################################################################
#	Sends single or multiple file(s) to all servers.
#	It sends only those files which are older on the receiver
#
#	Use:
#	./send_to_all_machines.sh \*	// sends the root folder and all subdirectories to servers
#	./send_to_all_machines.sh path/to/sourceFile path/to/destFolder	// sends a particular file to all servers	
##################################################################################################
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

#rsync -rhazP  $file erfanz@bsn00.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest
rsync -rhazP  $file erfanz@bsn01.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest
#rsync -rhazP  $file erfanz@bsn02.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest
#rsync -rvhazP  $file erfanz@bsn03.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest
#rsync -rvhazP  $file erfanz@bsn04.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest
#rsync -rvhazP  $file erfanz@bsn05.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest
#rsync -rvhazP  $file erfanz@bsn06.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest
#rsync -rvhazP  $file erfanz@bsn07.cs.brown.edu:/data/erfanz/rdma/RDMA-Snapshot-Isolation/$dest
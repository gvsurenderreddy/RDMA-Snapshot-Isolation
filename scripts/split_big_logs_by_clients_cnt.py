#!/usr/bin/python

import sys

filename = sys.argv[1]
output_file_prefix = "output_"
target = open(output_file_prefix + "0" , 'w')

with open(filename, "r") as ins:
    for line in ins:
        if "./exe/executor.exe oracle -n" in line:
            target.close()
            clientCnt = int(line.split()[4])
            target = open(output_file_prefix + str(clientCnt) + '.log' , 'w')
            target.write(line)
        else:
            target.write(line)
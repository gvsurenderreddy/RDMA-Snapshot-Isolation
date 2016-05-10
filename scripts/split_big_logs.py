#!/usr/bin/python

import sys

filename = sys.argv[1]
output_file_prefix = "output_"
target = open(output_file_prefix + "0" , 'w')

with open(filename, "r") as ins:
    for line in ins:
        if "export CLIENT_MACHINES" in line:
            target.close()
            machineCnt = int(line.split('=')[1])
            target = open(output_file_prefix + str(machineCnt) + '.log' , 'w')
            target.write(line)
            #target.write("\n")
        else:
            target.write(line)
            #target.write("\n")
    
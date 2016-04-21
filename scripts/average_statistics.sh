#!/bin/bash

output_file=$1

cat $output_file | grep  'Avg elapsed time in execution' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "NewOrder avg execution:\t" sum / n; }'
cat $output_file | grep  'Avg elapsed time in index' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print  "NewOrder avg index:\t" sum / n; }'
cat $output_file | grep  'Avg elapsed time in version' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "NewOrder avg version checking:\t" sum / n; }'
cat $output_file | grep  'Avg elapsed time in locking' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print  "NewOrder avg locking:\t" sum / n; }' 
cat $output_file | grep  'Avg elapsed time in updating' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "NewOrder avg updating:\t" sum / n; }'
cat $output_file | grep  'Avg elapsed time in committing' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "NewOrder avg commit:\t" sum / n; }'    

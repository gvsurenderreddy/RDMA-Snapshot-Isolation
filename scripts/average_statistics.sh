#!/bin/bash

output_file=$1

# cat $output_file | grep  'Avg elapsed time in execution' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "NewOrder avg execution:\t" sum / n; }'
# cat $output_file | grep  'Avg elapsed time in index' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print  "NewOrder avg index:\t" sum / n; }'
# cat $output_file | grep  'Avg elapsed time in version' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "NewOrder avg version checking:\t" sum / n; }'
# cat $output_file | grep  'Avg elapsed time in locking' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print  "NewOrder avg locking:\t" sum / n; }'
# cat $output_file | grep  'Avg elapsed time in updating' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "NewOrder avg updating:\t" sum / n; }'
# cat $output_file | grep  'Avg elapsed time in committing' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "NewOrder avg commit:\t" sum / n; }'    

cat $output_file | grep  '(Trx: New Order) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "NewO abort rate:\t" sum / n; }'
cat $output_file | grep  '(Trx: New Order) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print  "NewO Trx/sec:\t" sum / n; }'
cat $output_file | grep  '(Trx: Payment) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "Paym abort:\t" sum / n; }'
cat $output_file | grep  '(Trx: Payment) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print  "Paym trx/sec:\t" sum / n; }' 
cat $output_file | grep  '(Trx: OrderStatus) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "OrdeS abort:\t" sum / n; }'
cat $output_file | grep  '(Trx: OrderStatus) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "OrderS trx/sec:\t" sum / n; }'
cat $output_file | grep  '(Trx: Delivery) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "Deliv abort:\t" sum / n; }'
cat $output_file | grep  '(Trx: Delivery) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print  "Deliv trx/sec:\t" sum / n; }' 
cat $output_file | grep  '(Trx: StockLevel) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "StockL abort:\t" sum / n; }'
cat $output_file | grep  '(Trx: StockLevel) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "StockL trx/sec:\t" sum / n; }'
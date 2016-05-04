#!/bin/bash

output_file=$1

# cat $output_file | grep  'Avg elapsed time in execution' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "NewOrder avg execution:\t" sum / n; }'
# cat $output_file | grep  'Avg elapsed time in index' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print  "NewOrder avg index:\t" sum / n; }'
# cat $output_file | grep  'Avg elapsed time in version' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "NewOrder avg version checking:\t" sum / n; }'
# cat $output_file | grep  'Avg elapsed time in locking' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print  "NewOrder avg locking:\t" sum / n; }'
# cat $output_file | grep  'Avg elapsed time in updating' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "NewOrder avg updating:\t" sum / n; }'
# cat $output_file | grep  'Avg elapsed time in committing' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) print "NewOrder avg commit:\t" sum / n; }'    

cat $output_file | grep  '(Trx: New Order) committed:' | awk -F $'\t' '{ n++ } END { if (n > 0) printf "Total Number of Client logs:\t%.3d\n", n; }'

cat $output_file | grep  '(Trx: New Order) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "NewOrder abort:\t%.3f\n", sum / n; }'
cat $output_file | grep  '(Trx: New Order) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf  "NewOrder Trx/sec:\t%.1f\n", sum / n; }'
cat $output_file | grep  '(Trx: Payment) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "Payment abort:\t%.3f\n", sum / n; }'
cat $output_file | grep  '(Trx: Payment) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf  "Payment trx/sec:\t%.1f\n", sum / n; }' 
cat $output_file | grep  '(Trx: OrderStatus) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "OrdeStatus abort:\t%.3f\n", sum / n; }'
cat $output_file | grep  '(Trx: OrderStatus) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "OrderStatus trx/sec:\t%.1f\n", sum / n; }'
cat $output_file | grep  '(Trx: Delivery) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "Delivery abort:\t%.3f\n", sum / n; }'
cat $output_file | grep  '(Trx: Delivery) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf  "Delivery trx/sec:\t%.1f\n", sum / n; }' 
cat $output_file | grep  '(Trx: StockLevel) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "StockLevel abort:\t%.3f\n", sum / n; }'
cat $output_file | grep  '(Trx: StockLevel) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "StockLevel trx/sec:\t%.1f\n", sum / n; }'
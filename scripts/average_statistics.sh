#!/bin/bash

output_file=$1 

cat $output_file | grep  '[Stat] Total elapsed time (sec):' | awk -F $'\t' '{ n++ } END { if (n > 0) printf "Total Number of client logs:\t%.3d\n", n; }'
cat $output_file | grep  '[Stat] Total elapsed time (sec):' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "Total benchmark time (sec):\t%.1d\n", n; }'

cat $output_file | grep  '(Trx: New Order) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "NewOrder abort:\t%.3f\n", sum / n; }'
cat $output_file | grep  '(Trx: New Order) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf  "NewOrder Trx/sec:\t%.1f\n", sum / n; }'
cat $output_file | grep  '(Trx: New Order) Avg elapsed time in execution' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf  "NewOrder latency execution:\t%.1f\n", sum / n; }'
cat $output_file | grep  '(Trx: New Order) Avg elapsed time in index' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf  "NewOrder latency index:\t%.1f\n", sum / n; }'
cat $output_file | grep  '(Trx: New Order) Avg elapsed time in version check' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf  "NewOrder latency version check:\t%.1f\n", sum / n; }'
cat $output_file | grep  '(Trx: New Order) Avg elapsed time in locking' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf  "NewOrder latency locking:\t%.1f\n", sum / n; }'
cat $output_file | grep  '(Trx: New Order) Avg elapsed time in updating records' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf  "NewOrder latency updating records:\t%.1f\n", sum / n; }'
cat $output_file | grep  '(Trx: New Order) Avg elapsed time in committing snapshot' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf  "NewOrder latency commit snapshot:\t%.1f\n", sum / n; }'

cat $output_file | grep  '(Trx: Payment) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "Payment abort:\t%.3f\n", sum / n; }'
cat $output_file | grep  '(Trx: Payment) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf  "Payment trx/sec:\t%.1f\n", sum / n; }' 

cat $output_file | grep  '(Trx: OrderStatus) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "OrdeStatus abort:\t%.3f\n", sum / n; }'
cat $output_file | grep  '(Trx: OrderStatus) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "OrderStatus trx/sec:\t%.1f\n", sum / n; }'

cat $output_file | grep  '(Trx: Delivery) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "Delivery abort:\t%.3f\n", sum / n; }'
cat $output_file | grep  '(Trx: Delivery) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf  "Delivery trx/sec:\t%.1f\n", sum / n; }' 

cat $output_file | grep  '(Trx: StockLevel) committed:' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "StockLevel abort:\t%.3f\n", sum / n; }'
cat $output_file | grep  '(Trx: StockLevel) Committed Transactions/sec' | awk -F $'\t' '{ sum += $2; n++ } END { if (n > 0) printf "StockLevel trx/sec:\t%.1f\n", sum / n; }'

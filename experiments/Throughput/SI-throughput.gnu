set style fill solid 1.00 border 0
set style line 1 linewidth 4 linetype 1
set style line 2 linewidth 4 linetype 2
set style line 3 linewidth 4 linetype 3 


#set xtics rotate by -90
set xtics 0,10,50
set xrange [1:50]
set xtics font "Arial, 22"
set xlabel "# Clients" font "Arial, 22"


set ytics font "Arial, 22"
set ylabel "Transaction per second" offset -2 font "Arial, 22"
set grid ytics
set logscale y 


# Show human-readable Y-axis. E.g. "100 k" instead of 100000.
set format y '%.1s %c'

set key Left reverse
set key at 20, 7500000 font "Arial, 22"
set key spacing 1.7 samplen 2

plot	"RSI (RDMA).dat" using 1:2 title 'RSI (RDMA)' ls 1 linecolor rgb "red" with linespoints, \
        "Trad-SI (IPoIB).dat" using 1:2 title 'Trad-SI (IPoIB)' ls 1 linecolor rgb "purple" with linespoints, \
        "Trad-SI (IPoEth).dat" using 1:2 title 'Trad-SI (IPoEth)' ls 1 linecolor rgb "green" with linespoints
		
		  
set terminal postscript enhanced font "Arial, 16" color # monochrome dashed 8

set terminal postscript
set output "SI-throughput.eps"
replot
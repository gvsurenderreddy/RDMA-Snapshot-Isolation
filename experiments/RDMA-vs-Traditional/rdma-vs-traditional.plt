
set style fill solid 1.00 border 0
set style line 1 linewidth 4 linetype 1
set style line 2 linewidth 4 linetype 2
set style line 3 linewidth 4 linetype 3 

#set xtics rotate by -90
set xtics 0,10,40
set xrange [1:40]


set ytics 0,500000,2000000
set yrange [0:2000000]
set xlabel "# Clients"  font "Helvetica, 14"
set ylabel "Transaction per second" font "Helvetica, 14"
set grid ytics

# Show human-readable Y-axis. E.g. "100 k" instead of 100000.
set format y '%.1s %c'

set key top left font "Helvetica, 14"



plot	"rdma.dat" using 1:2 title 'RDMA SI' ls 1 linecolor rgb "red" with linespoints, \
        "traditional.dat" using 1:2 title 'Traditional SI' ls 1 linecolor rgb "blue" with linespoints
		  
set term postscript enhanced "Helvetica" 20 color # monochrome dashed 8
		  
		      
#set size 0.6,0.5
set terminal postscript 
#set terminal pdf
set output "rdmaSI-vs-traditionalSI.eps"
replot



#"exp1a_heuristic.dat" using 1:2 title 'heuristic' ls 1 linecolor rgb "orange" with linespoints, \
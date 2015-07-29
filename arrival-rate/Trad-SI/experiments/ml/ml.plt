set style data histogram
set style histogram cluster gap 1
set style fill solid 1.00 border 0
set grid ytics


set key spacing 1.7 samplen 2
set key right top font "Arial, 14"

set xtics ("Ethernet" 0, "IPoIB" 1) font "Arial, 16"
set autoscale x

set ylabel "Time (s)" font "Arial, 16"
set ytics font "Arial, 16"
set yrange [0:200]

plot	"classic.dat" u 2 title 'Classic' lt -1 linecolor rgb "#408736", \
        "communication-avoiding.dat" u 2 title 'Communication-avoiding' lt -1 linecolor rgb "#DE3E3E"

set size 0.6,0.5
set term postscript 
set output "ml.eps"
replot
set style data histograms
set style histogram rowstacked
set boxwidth 0.5
set style fill solid 1.0 border 0

	###########################
	#	IMPORTANT:
	#	Windows Gnuplot has a bug that doesn't show stakced histogram files with header properly.
	#	In fact, if your file has headers (for example, name of the columns),
	#	you should remove them before plotting.
	###########################


set xtics font "Arial, 24"
set xlabel "# Clients"  font "Arial, 24"

set ylabel "Latency (micro sec)" offset -3 font "Arial, 24"
set ytics 10 nomirror font "Arial, 24"
set grid ytics
set yrange [0:40]

set key Left reverse


#Grey	#908F96
#Green	#408736
#Yellow	#DBD82C
#Red		#DE3E3E
#Blue	#1781EB

################################################
# set key at 1.3, 39 font "Arial, 24"
# set key spacing 1.7 samplen 2
#
# plot 'RSI (RDMA) - latency.dat' \
# 	using 2:xticlabels(1) title 'acquire read ts' lc rgb "#908F96",\
# 	'' using 3 title 'read phase' lc rgb "#1781EB",\
# 	'' using 4 title 'acquire commit ts' lc rgb "#DBD82C",\
# 	'' using 5 title '2PC-1st' lc rgb "#DE3E3E",\
# 	'' using 6 title '2PC-2nd' lc rgb "#408736"
#
# set terminal postscript
# set output "RSI-latency-breakdown.eps"
# replot


################################################
# set yrange [0:50]
# set key at 1.3, 49 font "Times-Roman, 28"
# set key spacing 1.7 samplen 2
#
# plot 'Trad-SI (2-sided verbs) - latency.dat' \
# 	using 2:xticlabels(1) title 'fetch items info' lc rgb "#1781EB",\
# 	'' using 3 title 'acquire commit ts' lc rgb "#DE3E3E",\
# 	'' using 4 title '2PC-1st' lc rgb "#DBD82C",\
# 	'' using 5 title '2PC-2nd' lc rgb "#408736",\
# 	'' using 6 title 'respond' lc rgb "#908F96"
#
# set terminal postscript
# set output "Trad-SI (2-sided verbs) - latency-breakdown.eps"
# replot



################################################
set yrange [0:3000]
set ytics 750 nomirror font "Arial, 24"

set key at 1.3, 2900 font "Arial, 24"
set key spacing 1.7 samplen 2

plot 'Trad-SI (IPoIB) - latency.dat' \
	using 2:xticlabels(1) title 'read phase' lc rgb "#1781EB",\
	'' using 3 title 'acquire commit ts' lc rgb "#DBD82C",\
	'' using 4 title '2PC-1st' lc rgb "#DE3E3E",\
	'' using 5 title '2PC-2nd' lc rgb "#408736",\
	'' using 6 title 'respond' lc rgb "#908F96"

set terminal postscript
set output "TradSI-latency-breakdown.eps"
replot



################################################
# set yrange [0:3000]
# set ytics 750 nomirror font "Times-Roman, 22"
#
# set key at 1.3, 2900 font "Times-Roman, 28"
# set key spacing 1.7 samplen 2
#
# plot 'Trad-SI (IPoEth) - latency.dat' \
# 	using 2:xticlabels(1) title 'fetch items info' lc rgb "#1781EB",\
# 	'' using 3 title 'acquire commit ts' lc rgb "#DE3E3E",\
# 	'' using 4 title '2PC-1st' lc rgb "#DBD82C",\
# 	'' using 5 title '2PC-2st' lc rgb "#408736",\
# 	'' using 6 title 'respond' lc rgb "#908F96"
#
# set terminal postscript
# set output "Trad-SI (IPoEth) - latency-breakdown.eps"
# replot
file1=$1
file2=$2
x=$3
y=$4
file3=$5

echo "use: <file1> <file2> <x> <y> [file3]"
sleep 2

plotline=`echo "\"$file1\" with lines, \"$file2\" with lines"`

if [ "$file3" != "" ]; then
    plotline=`echo "\"$file1\" with lines, \"$file2\" with lines, \"$file3\" with lines"`
fi

gnuplot -persist << EOF
set term png large
set out "/tmp/graph.png"
##set title "Binary PSO on BSP problem"
set xlabel "$x"
set ylabel "$y"
set key out vert
set key bot center
#set key box
set xrange[0:10]
#set yrange[0.000000e+00:1]
unset key
unset xlabel
unset ylabel

plot $plotline
pause -1
EOF

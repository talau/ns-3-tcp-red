file=$1
x=$2
y=$3

echo "use: <file> <x> <y>"
sleep 2

gnuplot -persist << EOF
##set term png large
##set out "$BASE_DIR/graph.png"
##set title "Binary PSO on BSP problem"
set xlabel "$x"
set ylabel "$y"
set key out vert
set key bot center
#set key box
#set xrange[1:1000]
set xrange[0:10]
#set yrange[0.000000e+00:1]

plot "$file" with lines
pause -1
EOF

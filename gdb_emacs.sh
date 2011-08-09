NS_PATH="~/src/ns-3-tcp"
cd $NS_PATH
$NS_PATH/waf --run "scratch/$1" --command-template='/usr/bin/gdb --annotate=3 %s'

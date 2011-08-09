#!/bin/sh

# keep minutes param in the simulation

use()
{
    echo "$0 <simul> <type> [extra_param]"
    echo "i.e. $0 scrach/ngwa-test-2 new-reno"
    exit -1
}

# ./run_test.sh scrach/tcp-ngwa-test-2 1

simul="$1"
type="$2"

if [ "$3" != "" ]; then
    extra_param="$3"
fi

if [ "$simul" = "" ]; then
    use
fi
if [ "$type" = "" ]; then
    use
fi

rm out/* -v 

case $type in
    793)
	echo "Test 793"
        ./waf --run "$simul --mode=0 --ngwa-mode=0 --tcpmodel-normal=ns3::TcpRfc793 --trace-throughput=1 --trace-allowedWnd=1 $extra_param"
	;;
    new-reno)
	echo "Test NewReno"
        ./waf --run "$simul --mode=0 --ngwa-mode=0 --tcpmodel-normal=ns3::TcpNewReno --trace-throughput=1 --trace-cwnd=1 --trace-allowedWnd=1 $extra_param"
	;;
    1)
	echo "Test #1"
        ./waf --run "$simul --mode=1 --ngwa-mode=1 --trace-throughput=1 --trace-cwnd=1 --trace-allowedWnd=1"
	;;
    2)
	echo "Test #2"
        ./waf --run "$simul --mode=1 --ngwa-mode=2 --trace-throughput=1 --trace-cwnd=1 --trace-allowedWnd=1"
	;;
    3)
	echo "Test #3"
        ./waf --run "$simul --mode=1 --ngwa-mode=3 --trace-throughput=1 --trace-allowedWnd=1"
	;;
    4)
	echo "Test #4"
        ./waf --run "$simul --mode=1 --ngwa-mode=4 --trace-throughput=1 --trace-allowedWnd=1"
	;;
    5)
	echo "Test #5"
        ./waf --run "$simul --mode=1 --ngwa-mode=5 --trace-throughput=1 --trace-cwnd=1 --trace-allowedWnd=1 $extra_param"
	;;
    6)
	echo "Test #6"
        ./waf --run "$simul --mode=1 --ngwa-mode=6 --trace-throughput=1 --trace-cwnd=1 --trace-allowedWnd=1 $extra_param"
	;;
    7)
	echo "Test #7"
        ./waf --run "$simul --mode=1 --ngwa-mode=7 --trace-throughput=1 --trace-allowedWnd=1 $extra_param"
	;;
    8)
	echo "Test #8"
        ./waf --run "$simul --mode=1 --ngwa-mode=8 --trace-throughput=1 --trace-allowedWnd=1 $extra_param"
	;;
    *)
	echo "Invalid test."
	exit -1
	;;
esac

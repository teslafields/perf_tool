#!/bin/bash

function error () {
    echo $1
    exit
}

function usage () {
    echo "Usage: ./perf_run [options] program_bin"
    echo "  -A    Specify list of the program's argv: -A 10,tcp,0"
    echo "  -e    Select others events to measure: -e kmem:kmalloc,probe_user:probe1,branches"
    echo "  -ls   Select Leader Sampling method: -ls cache-misses,cycles"
    echo "  -a    Measure events in system-wide mode"
    echo "  -C    Select which CPU to measure: -C 1"
    echo "  -m    Select to measure multiple functions calls within the selected fuction"
    echo "  -t    Specify the number of threads whether the program is multithread"
    echo "  -p    Specify different program binary path. Default is out/"
    echo "  -r    Specify number of measurements to repeat."
    exit
}

loops=1
parser_args=""

TEMP=`getopt -o e:m:t:p:l:C:A:ah --long ls: -- "$@"`
eval set -- "$TEMP"
echo $TEMP
while true ; do
    case "$1" in
        -h) usage ;;
        -e) events=$2 ; parserargs="$parserargs $1 $2" ; shift 2 ;;
        -m) mevents=$2 ; parserargs="$parserargs $1 $2" ; shift 2 ;;
        -t) threads=$2 ; parserargs="$parserargs $1 $2" ; shift 2 ;;
        -p) path=$2 ; shift 2 ;;
        -r) loops=$2 ; shift 2 ;;
        --ls) lsevents=$2 ;
              parserargs="$parserargs $1 $2" ;
              shift 2 ;;
        -a) systemwide=$1 ; shift ;;
        -A) program_args=$2 ; shift 2 ;;
        -C) cpu="$1 $2" ; shift 2 ;;
        --) shift ; program_bin=$1 ; break ;;
        *) error "Unknown option: $1";
    esac
done
echo $TEMP

echo "-e $events"
echo "-m $mevents"
echo "--ls $lsevents"
echo "-p $path"
echo "-r $loops"
echo "-A $program_args"
echo "$cpu"
echo "-t $threads"
echo "$program_bin"
exit

program_path=out/$program_bin
if ! [ -z "$path" ]; then
    program_path=$path/$program_bin
fi

events_arr=(${events//,/ })
lsevents_arr=(${lsevents//,/ })
mevents_arr=(${mevents//,/ })
program_args=${program_args//,/ }

if [ ! -z $lsevents ]; then
    for ev in ${events_arr[@]}
    do
        cmd_events=''$cmd_events' -e "{'$ev','$lsevents'}:S"'
    done
    for ev in ${mevents_arr[@]}
    do
        cmd_events=''$cmd_events' -e "{probe_'$filename':'$ev'in,'$lsevents'}:S"'
        cmd_events=''$cmd_events' -e "{probe_'$filename':'$ev'out,'$lsevents'}:S"'
    done
    cmd_events=''$cmd_events' -e "{probe_'$filename':in,'$lsevents'}:S"'
    cmd_events=''$cmd_events' -e "{probe_'$filename':out,'$lsevents'}:S"'

else
    for ev in ${events_arr[@]}
    do
        cmd_events="$cmd_events -e $ev"
    done
    for ev in ${mevents_arr[@]}
    do
        cmd_events=''$cmd_events' -e probe_'$filename':'$ev'in'
        cmd_events=''$cmd_events' -e probe_'$filename':'$ev'out'
    done
    cmd_events="$cmd_events -e probe_$filename:$probein -e probe_$filename:$probeout*"
fi

echo "The current CPU frequency is:"
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
echo "Continue? [y/n]"
read option
if [ "$option" == "n" ]; then
    exit
fi

probes=$( sudo perf probe -l | grep $filename )
if [ -z "$probes" ]; then
    ( sudo perf probe -x $filepath -F ) || error "Error: Listing functions"
    echo "Type the function's name to measure:"
    read taskname

    if [ -z $taskname ]; then
        taskname="${filename}_task"
    fi

    ( sudo perf probe -x $filepath -L $taskname ) || error "Error: Listing function lines"
    echo "Insert probe's lines:"
    read probe1 probe2

    if [ -z $probe1 ]; then
        ( sudo perf probe -x $filepath --add "in=$taskname" ) || error "Error: Adding probe in"
    else
        ( sudo perf probe -x $filepath --add "in=$taskname:$probe1" ) || error "Error: Adding probe in"
    fi

    if [ -z $probe2 ]; then
        ( sudo perf probe -x $filepath --add "out=$taskname%return" ) || error "Error: Adding probe out"
    else
        ( sudo perf probe -x $filepath --add "out=$taskname:$probe2" ) || error "Error: Adding probe out"
    fi
else
    echo "Probes in $filename already exists! Continue? [y/n]"
    read option
    if [ $option == "n" ]; then
        error "Delete probes!"
    fi
fi

record_cmd="sudo perf record $cmd_events $sys_wide $set_cpu $call_graph $filepath $program_args"
echo "$record_cmd"
echo "ENTER to run or Ctrl+C to exit:"
read

counter=0
while [ $counter -lt $loops ]; do
    ( $record_cmd ) || echo "Error: perf record"
    ( sudo perf script > perf_text.data ) || error "Error: perf script"
    echo "python3 $parser $parser_args"
    # sudo python3 $parser $parser_args
    # timenow=$( date "+%FT%T" )
    # sudo cp out'/'$filename'_events.csv' 'out/csv_tables/'$timenow'_'$filename'_events.csv'
    let counter=counter+1
    read
done

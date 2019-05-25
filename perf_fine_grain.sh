#!/bin/bash
loops=10

function error () {
    echo $1
    exit
}

cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
read

args=("$@")
for var in ${args[@]}
do
    count=$[ $count+1 ]
    if [ "$var" == "-ls" ]; then
        lsevents=${args[$count]}
    elif [ "$var" == "-fg" ]; then
        fgevents=${args[$count]}
    elif [ "$var" == "-e" ]; then
        events=${args[$count]}
    elif [ "$var" == "-p" ]; then
        path=${args[$count]}
    elif [ "$var" == "-A" ]; then
        program_args="${args[$count]}"
    elif [ "$var" == "-f" ]; then
        filename=${args[$count]}
    fi
done

if [ -z "$path" -o -z "$filename" ]; then
    error "Provide at least a path and a file"
fi
events_parser=${events}
lsevents_parser=${lsevents}
events_arr=(${events//,/ })
lsevents_arr=(${lsevents//,/ })
fgevents_arr=${fgevents//,/ }
program_args=${program_args//,/ }
if [ ! -z $lsevents ]; then
    for ev in ${events_arr[@]}
    do
        cmd_events=''$cmd_events' -e "{'$ev','$lsevents'}:S"'
    done
    if [ ! -z $fgevents ]; then
        for ev in ${fgevents_arr[@]}
        do
            cmd_events=''$cmd_events' -e "{probe_'$filename':'$ev'in,'$lsevents'}:S"'
            cmd_events=''$cmd_events' -e "{probe_'$filename':'$ev'out,'$lsevents'}:S"'
        done
    else
        cmd_events=''$cmd_events' -e "{probe_'$filename':in,'$lsevents'}:S"'
        cmd_events=''$cmd_events' -e "{probe_'$filename':out,'$lsevents'}:S"'
    fi
else
    for ev in ${events_arr[@]}
    do
        cmd_events="$cmd_events -e $ev"
    done
    if [ ! -z $fgevents ]; then
        for ev in ${fgevents_arr[@]}
        do
            cmd_events=''$cmd_events' -e probe_'$filename':'$ev'in'
            cmd_events=''$cmd_events' -e probe_'$filename':'$ev'out'
        done
    else
        cmd_events="$cmd_events -e probe_$filename:in"
        cmd_events="$cmd_events -e probe_$filename:out"
    fi
fi

if [ ! -z $events ]; then
    event_flag=1
else
    event_flag=0
fi
# record_cmd="sudo perf record -e sched:sched_switch,irq:softirq_entry,probe_netio:netin,probe_netio:netout,probe_netio:cryptin,probe_netio:cryptout,probe_netio:afilein,probe_netio:afileout -a -C 2 ./netio"
record_cmd="sudo perf record $cmd_events ./$filename $program_args"

echo "$record_cmd"
echo "ENTER to run or Ctrl+C to exit:"
read

COUNTER=1
while [  $COUNTER -lt $loops ]; do
    echo The counter is $COUNTER
    #    ( ./hash & ) || error "hash error"
    #    ( sudo dd if=/dev/urandom of=/dev/null & ) || error "DD"
    ( $record_cmd ) || echo "E: perf record"
    ( sudo perf script > results/fine_grain.data ) || error "E: perf script --ns"
    #    killdd="sudo killall -9 dd"
    #    ( $killdd ) || error "kill dd"
    echo "Starting parser"
    python3 parsers/parser_fg_perf_data.py $filename $fgevents $event_flag $COUNTER $lsevents
    let COUNTER=COUNTER+1
    if pgrep hash > /dev/null; then
        sudo pkill hash
    fi
    read
done

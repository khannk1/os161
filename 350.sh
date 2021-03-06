#!/bin/bash

BASE="$HOME/cs350-os161"
CODE="$BASE/os161-1.99"
BUILD="$BASE/root"
LOGS="$BASE/logs"

function make {
	cd $CODE
	./configure --ostree=$HOME/cs350-os161/root --toolprefix=cs350-
	cd kern/conf
	./config ASST$ano
	cd ../compile/ASST$ano
	bmake depend
	bmake
	bmake install	
}

function debug-attach {
	clear
	printf "dir ../os161-1.99/kern/compile/ASST$ano\ntarget remote unix:.sockets/gdb" > /tmp/350gdbcommands
	cd ~/cs350-os161/root
	cs350-gdb $kernel_file --command=/tmp/350gdbcommands -tui
}

function debug {
	tmux new-session -d -s debug -n run
	tmux send-keys -t debug:run "350 debug-wait" Enter
	sleep 2s
	tmux split-window -t debug -h
	tmux rename-window debugger
	tmux send-keys -t debug:debugger "350 debug-attach" Enter
	tmux attach -t debug:debugger
}

function resume {
	tmux attach -t debug
}

function end {
	tmux kill-server
}

function run {
	cd $BUILD
	if [ ! -f $kernel_file ]; then
		echo "Kernel file not found. Did you make?"
		exit 1
	fi
	sys161 $kernel_file $1
}

function debug-wait {
	cd $BUILD
	if [ ! -f $kernel_file ]; then
		echo "Kernel file not found. Did you make?"
		exit 1
	fi
	sys161 -w $kernel_file
}


function loop {
	cd $BUILD
	if [ ! -f $kernel_file ]; then
		echo "Kernel file not found. Did you make?"
		exit 1
	fi
	vi commands
	counter=0
	log_file="$LOGS/loop_$(date +%s)_$1.log"
	while [ $counter -lt $1 ]; do
		echo -ne "Iteration $((counter+1)) \r"
		echo ">>>>>>> ITERATION $((counter+1)) START <<<<<<<<" >> $log_file
		sys161 $kernel_file "$(cat commands)" >> $log_file 2>&1
		echo ">>>>>>> ITERATION $((counter+1)) ENDED <<<<<<<<" >> $log_file
		echo >> $log_file
		let counter=counter+1
	done
	echo "Completed $1 Iterations"
	echo "Logged output to $log_file"
	vi $log_file
}

function submit {
	cd $BASE
	if [ -f os161kern.tgz ]; then
		rm os161kern.tgz
		echo "Removing previous submission"
	fi
	echo "Submitting Assignment $ano"
	log_file="$LOGS/submit_$ano_$(date +%s).log"
	echo "Logging to $log_file"
	/u/cs350/bin/cs350_submit $ano > $log_file 2>&1
	vi $log_file
}

function set_ano {
	cd $BASE
	printf "$1" > .ano
	echo "Set Assignment Number to $1"
	exit 0
}

function try_get_ano {
	cd $BASE
	if [ ! -f .ano ]; then
		echo "No Assignment Number set!"
		exit 1
	fi
	ano=$(cat .ano)
	kernel_file="$BUILD/kernel-ASST$ano"
}


if [ $# -lt 1 ]; then
	echo "usage : 350 <command>"
	exit 1
fi

if [ $1 = "set" ]; then
	if [ $# -ne 2 ]; then
		echo "Missing assignment number?"
		exit 1
	fi
	set_ano $2
fi

try_get_ano

if [ $1 = "resume" ]; then
	resume
elif [ $1 = "ano" ]; then
	echo "Assignment Number : $ano"
elif [ $1 = "debug-attach" ]; then
	debug-attach
elif [ $1 = "make" ]; then
	make
elif [ $1 = "run" ]; then
	run $2
elif [ $1 = "debug-wait" ]; then
	debug-wait
elif [ $1 = "debug" ]; then
	debug
elif [ $1 = "submit" ]; then
	submit
elif [ $1 = "loop" ]; then
	if [ $# -ne 2 ]; then
		echo "Missing loop counter?"
		exit 1
	fi
	loop $2
else 
	echo "Unknown command"
	exit 1
fi

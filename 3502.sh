#!/bin/bash

BASE="$HOME/cs350-os161"
CODE="$BASE/os161-1.99"
BUILD="$BASE/root"
LOGS="$BASE/logs"
LAST_LOG="$LOGS/.last_log"
ANO_FILE="$BASE/.ano"
LOOP_FILE="$BASE/.loop"
DO_FILE="$BASE/.do"

function set-last-log {
	touch $LAST_LOG
	ln -sf $1 $LAST_LOG
}

function make-kernel {
	log_file="$LOGS/make_kernel_$(date +%s).log"
	echo "Logging to $log_file"
	echo -ne "Making Kernel...\r"
	cd $CODE/kern/conf
	./config ASST$ano >> $log_file
	cd ../compile/ASST$ano
	bmake depend >> $log_file
	bmake >> $log_file
	bmake install >> $log_file
	
	set-last-log $log_file
	cat $log_file | tail -20 | grep 'Stop.' &>/dev/null
	if [ $? -eq 0 ]; then
		echo
		echo "Make failed"
		exit 1;
	else
		echo "Make successful!"
	fi
}

function make-user {
	log_file="$LOGS/make_user_$(date +%s).log"
	echo "Logging to $log_file"
	echo -ne "Making User Programs...\r"
	cd $CODE
	bmake >> $log_file
	bmake install >> $log_file

	set-last-log $log_file
	cat $log_file | tail -20 | grep 'Stop.' &>/dev/null
	if [ $? -eq 0 ]; then
		echo
		echo "Make failed"
		exit 1;
	else
		echo "Make successful!\t"
	fi
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
	sys161 $1 $kernel_file
}

function set-do {
	vi $DO_FILE
}

function run-do {
	if [ ! -f $DO_FILE ]; then
		echo "No Do File found!"
		exit 1
	fi
	cd $BUILD
	sys161 $kernel_file "$(cat $DO_FILE)"
}

function loop {
	log_file="$LOGS/loop_$(date +%s)_$1.log"
	echo "Logging to $log_file"
	cd $BUILD
	if [ ! -f $kernel_file ]; then
		echo "Kernel file not found. Did you make?"
		exit 1
	elif [ ! -f $LOOP_FILE ]; then
		echo "No Loop File found!"
		exit 1
	fi
	counter=0
	loop_commands=$(cat $LOOP_FILE)
	while [ $counter -lt $1 ]; do
		echo -ne "Iteration $((counter+1)) \r"
		echo ">>>>>>> ITERATION $((counter+1)) START <<<<<<<<" >> $log_file
		sys161 $kernel_file "$loop_commands" >> $log_file 2>&1
		echo ">>>>>>> ITERATION $((counter+1)) ENDED <<<<<<<<" >> $log_file
		echo >> $log_file
		let counter=counter+1
	done
	echo "Completed $1 Iterations"
	vi $log_file
	set-last-log $log_file
}

function set-loop {
	vi $LOOP_FILE
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
	set-last-log $log_file
}

function set-ano {
	printf $1 > $ANO_FILE
	echo "Set Assignment Number to $(cat $ANO_FILE)"
}

function try-get-ano {
	if [ ! -f $ANO_FILE ]; then
		echo "No Assignment Number set!"
		exit 1
	fi
	ano=$(cat $ANO_FILE)
	kernel_file="$BUILD/kernel-ASST$ano"
}

function find {
	cd $CODE/kern
	egrep -Inir $1
}

function help {
	echo "usage : 350 <command>"
	echo
	echo "Command List"
	echo -e "help\t\t\tShow this page"
	echo -e "set ano <num> \t\tSet Assignment Number to <num>"
	echo -e "set do\t\t\tSet commands that will run when using \"350 do\""
	echo -e "set loop\t\tSet commands that will run when using \"350 loop <count>\""
	echo -e "ano\t\t\tShow Assignment Number"
	echo -e "loop <count>\t\tLoop by <count> times"
	echo -e "run\t\t\tRun last compiled kernel"
	echo -e "make\t\t\tCompile kernel"
	echo -e "find <phrase>\t\tSearch all code for the phrase specified"
	echo -e "do\t\t\tSimilar to run. Will automatically execute commands given via \"350 set do\"."
	echo -e "make run\t\tMake + Run"
	echo -e "make do\t\t\tMake + Do"
	echo -e "debug\t\t\tRuns kernel and attaches debugger automatically (uses tmux)"
	echo -e "resume\t\t\tResumes a debug session in case of a disconnect (from SSH)"
	echo -e "submit\t\t\tSubmit Assignment"
	echo -e "log\t\t\tVim into last log created"
	echo -e "log -o\t\t\tOutput last log created (useful for piping into grep)"
}

if [ $# -lt 1 ]; then
	help
	exit 1
fi

if [ $1 = "help" ]; then
	help
	exit 0
fi

if [ $1 = "set" ]; then
	if [ $# -lt 2 ]; then
		echo "Set what?"
		exit 1
	elif [ $2 = "ano" ]; then
		if [ $# -lt 3 ]; then
			echo "Missing Assignment Number"
			exit 1
		fi
		set-ano $3
	elif [ $2 = "do" ]; then
		set-do
	elif [ $2 = "loop" ]; then
		set-loop
	fi
	exit 0
fi

try-get-ano

if [ $1 = "resume" ]; then
	resume
elif [ $1 = "ano" ]; then
	echo "Assignment Number : $ano"
elif [ $1 = "debug-attach" ]; then
	debug-attach
elif [ $1 = "make" ]; then
	make-kernel
	if [ "$2" = "run" ]; then
		echo -ne "\n"
		run
	elif [ "$2" = "do" ]; then
		echo -ne "\n"
		run-do
	fi
elif [ $1 = "run" ]; then
	run
elif [ $1 = "debug-wait" ]; then
	run -w
elif [ $1 = "debug" ]; then
	debug
elif [ $1 = "submit" ]; then
	submit
elif [ $1 = "do" ]; then
	run-do
elif [ $1 = "loop" ]; then
	if [ $# -ne 2 ]; then
		echo "Missing loop counter?"
		exit 1
	fi
	loop $2
elif [ $1 = "log" ]; then
	if [ ! -f $LAST_LOG ]; then
		echo "No Last Log found!"
		exit 1
	fi
	if [ "$2" = "-o" ]; then
		cat $LAST_LOG	
	else 
		vi $LAST_LOG
	fi
elif [ $1 = "find" ]; then
	if [ $# -le 1 ]; then
		echo "Find what?"
		exit 1
	fi
	find $2
else 
	echo "Unknown command"
	help
	exit 1
fi

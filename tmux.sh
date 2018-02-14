#!/bin/bash

cd ~/cs350-os161/root

# set up a tmux session with SYS/161 in one pane, and GDB in another
tmux kill-session -t os161 || true # kill old tmux session, if present
tmux new-session -d -s os161 # start a new tmux session, but don't attach to it just yet
tmux split-window -v -t os161:0 # split the tmux window in half
tmux send-keys -t os161:0.0 'sys161 -w kernel "p uw-testbin/onefork"' C-m # start SYS/161 and wait for GDB to connect
tmux send-keys -t os161:0.1 'cs350-gdb kernel -tui' C-m # start GDB
sleep 0.5 # wait a little bit for SYS/161 and GDB to start
tmux send-keys -t os161:0.1 "dir ~/cs350-os161/os161-1.99/kern/compile/ASST${1}" C-m # in GDB, switch to the kernel dir
tmux send-keys -t os161:0.1 'target remote unix:.sockets/gdb' C-m # in GDB, connect to SYS/161
tmux send-keys -t os161:0.1 '' # in GDB, fill in the continue command automatically so the user can just press Enter to continue
tmux attach-session -t os161 # attach to the tmux session



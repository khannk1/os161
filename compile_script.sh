#! /bin/bash
if [ $1 -ge 0 ] && [ $1 -le 10 ]
then
	DIRECTORY=os161-1.99/kern/compile/ASST$1
	if [ -d "$DIRECTORY" ]; 
	then
		set -e
		cd "$DIRECTORY"
		echo "I'm in "
		pwd
		ls
		bmake depend
		bmake
		bmake install
		cd ../../../
		echo "Now I'm in"
		pwd
		bmake
		bmake install
		cd ../root
		echo "Now I would have run sys161 command in "
		sys161 kernel-ASST$1
		pwd
	else
		echo "You haven't reached that assignment yet big boy. Hold your horses."
	fi
else
	echo Hey that\'s not a valid assignment number.
fi
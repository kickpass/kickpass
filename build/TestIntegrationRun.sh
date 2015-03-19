#!/bin/sh
set -e

TEST_NAME=$1
TEST=$2

[ -d $HOME ] || mkdir $HOME
[ ! -d $HOME/.kickpass ] || rm -r $HOME/.kickpass

echo "Running test $TEST_NAME"
echo "HOME=$HOME KP=$KP EDITOR=$EDITOR EXPECT=$EXPECT SRC=$SRC $0 $TEST_NAME $TEST"

function abort {
	echo $1
	return 1
}

. $TEST

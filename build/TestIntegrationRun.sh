#!/bin/sh
set -e

export KP=$1
TEST_NAME=$2
TEST=$3

[ -d $HOME ] || mkdir $HOME
[ ! -d $HOME/.kickpass ] || rm -r $HOME/.kickpass

echo "Running test $TEST_NAME"
echo "VALGRIND_COMMAND=$VALGRIND_COMMAND VALGRIND_OPTIONS=$VALGRIND_OPTIONS HOME=$HOME KP=$KP EDITOR_PATH=$EDITOR_PATH EXPECT=$EXPECT SRC=$SRC $0 $TEST_NAME $TEST"

do_test() {
	$VALGRIND_COMMAND $VALGRIND_OPTIONS "$@"
}

abort() {
	echo $1
	return 1
}

. $TEST

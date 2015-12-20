#!/bin/sh
set -e

# Given

# When
do_test $KP init

# Then
[ -d $HOME/.kickpass ] || abort "missing kickpass workspace directory"

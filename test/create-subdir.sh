#!/bin/sh
set -e

# Given
$KP init

# When
$EXPECT $SRC/create.expect create subdir/test

# Then
[ -f $HOME/.kickpass/subdir/test ] || abort "missing kickpass safe"

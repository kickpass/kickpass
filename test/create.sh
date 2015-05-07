#!/bin/sh
set -e

# Given
$KP init

# When
$EXPECT $SRC/create.expect create test

# Then
[ -f $HOME/.kickpass/test ] || abort "missing kickpass safe"

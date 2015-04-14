#!/bin/sh
set -e

# Given
$KP init

# When
$EXPECT $SRC/create.expect

# Then
[ -f $HOME/.kickpass/test ] || abort "missing kickpass safe"

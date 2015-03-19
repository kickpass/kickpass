#!/bin/sh
set -e

# Given
$KP init

# When
$EXPECT -d $SRC/create.expect

# Then
[ -f $HOME/.kickpass/test ] || abort "missing kickpass safe"
head -n 1 $HOME/.kickpass/test | grep "^-----BEGIN PGP MESSAGE-----$" || abort "invalid kickpass safe header"

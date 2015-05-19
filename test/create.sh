#!/bin/sh
set -e

# Given
export EDITOR="$EDITOR_PATH/TestIntegrationEditorDate.sh"
$KP init

# When
do_test $EXPECT $SRC/password-confirm.expect create test

# Then
[ -f $HOME/.kickpass/test ] || abort "missing kickpass safe"
size=`wc -c $HOME/.kickpass/test|awk '{print $1}'`
[ $size -gt 20 ] || abort "invalid kickpass safe size"

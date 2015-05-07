#!/bin/sh
set -e

# Given
export EDITOR="$EDITOR_PATH/TestIntegrationEditorDate.sh"
$KP init

# When
$EXPECT $SRC/password-confirm.expect create subdir/test

# Then
[ -f $HOME/.kickpass/subdir/test ] || abort "missing kickpass safe"
size=`du -b $HOME/.kickpass/subdir/test|cut -f 1`
[ $size -gt 20 ] || abort "invalid kickpass safe size"

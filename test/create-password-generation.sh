#!/bin/sh
set -e

# Given
export EDITOR="$EDITOR_PATH/TestIntegrationEditorSave.sh"
$KP init

# When
$EXPECT $SRC/password-confirm.expect create -g -l 42 test

# Then
[ -f $HOME/.kickpass/test ] || abort "missing kickpass safe"
pwlen=`cat $HOME/editor-save.txt | head -n 1 | wc -c`
# pwlen should be 42 + \n = 43
[ $pwlen -eq 43 ] || abort "invalid password length"

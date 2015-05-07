#!/bin/sh
set -e

# Given
export EDITOR="$EDITOR_PATH/TestIntegrationEditorDate.sh"
$KP init
$EXPECT $SRC/password-confirm.expect create test
export EDITOR="$EDITOR_PATH/TestIntegrationEditorSave.sh"

# When
$EXPECT $SRC/password.expect edit test

# Then
[ -f $HOME/.kickpass/test ] || abort "missing kickpass safe"
pwlen=`cat $HOME/editor-save.txt | head -n 1 | wc -c`
# pwlen should be 10 + \n = 11
[ $pwlen -eq 11 ] || abort "invalid password length"

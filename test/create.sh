#!/bin/sh
set -e

# Given
export EDITOR="$EDITOR_PATH/TestIntegrationEditorDate.sh"
$KP init

# When
$EXPECT $SRC/create.expect create test

# Then
[ -f $HOME/.kickpass/test ] || abort "missing kickpass safe"

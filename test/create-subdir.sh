#!/bin/sh
set -e

# Given
export EDITOR="$EDITOR_PATH/TestIntegrationEditorDate.sh"
$KP init

# When
$EXPECT $SRC/create.expect create subdir/test

# Then
[ -f $HOME/.kickpass/subdir/test ] || abort "missing kickpass safe"

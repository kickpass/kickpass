#!/bin/bash
set -e

# Given

# When
$KP init

# Then
[ -d $HOME/.kickpass ] || abort "missing kickpass workspace directory"

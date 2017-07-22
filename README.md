[![Build Status](https://travis-ci.org/kickpass/kickpass.svg?branch=develop)](https://travis-ci.org/paulfariello/kickpass)

Kickpass
========

Kickpass is a stupid simple password safe. It keep each password in a specific
safe, protected with modern cryptography. Its main user interface is command
line.

Full documentation is available throught manual page ``kickpass(1)``:
```bash
man 1 kickpass
```

Quick help looks like:
```bash
usage: kickpass [-hv] <command> [<cmd_opts>] [<args>]

options:
    -h, --help     Print this help
    -v, --version  Print kickpass version

commands:
    help <command>               Print help for given command
    init                         Initialize a new password safe directory. Default to ~/.kickpass
    create [-hgl] <safe>         Create a new password safe
    open <safe>                  Open a password safe and print its content on stdout
    edit [-pm] <safe>            Edit a password safe with $EDIT
    copy <safe>                  Copy a password (first line of safe) into X clipboard
    list                         List available safes
    delete <safe>                Delete a password safe after password confirmation
    rename <old_safe> <new_safe> Rename a password safe
```

Features
--------

 * One password to rule them all
 * One password to find them
 * One password to bring them all
 * Integrated password generator
 * Full text metadata with your favorite editor
 * Strong encryption: AEAD with chacha20 and poly1305
 * Direct copy to X selection and clipboard

Examples
--------

```bash
$ kickpass create -g www/github.com
[kickpass] master password:

$ kickpass cat www/github.com
url: https://www.github.com
username: paulfariello

$ kickpass copy www/github.com
$
```

Technical overview
------------------

Kickpass is built around a shared library named libkickpass.

libkickpass leverage [libsodium](https://libsodium.org/) to create safes.

Safes are created using authenticated encryption with associated data. As of
now libkickpass use chacha20 along with poly1305 to encrypt and authenticate
the safe.

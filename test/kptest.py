#
# Copyright (c) 2015 Paul Fariello <paul@fariello.eu>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
import collections
import sys
import os
import pexpect
import subprocess
import shutil
import unittest

class KPTestCase(unittest.TestCase):
    EDITORS = {
            'env': 'TestIntegrationEditorEnv.sh',
            'date': 'TestIntegrationEditorDate.sh',
            'save': 'TestIntegrationEditorSave.sh',
        }

    def __init__(self, *args, **kwargs):
        super(KPTestCase, self).__init__(*args, **kwargs)
        # Get env vars
        self.kp = os.environ['KP']
        self.editor_path = os.environ['EDITOR_PATH']
        self.kp_ws = os.path.join(os.environ['HOME'], '.kickpass')
        self.clear_text = os.path.join(os.environ['HOME'], 'editor-save.txt')

    def setUp(self):
        shutil.rmtree(self.kp_ws, ignore_errors=True)

    # Env
    def editor(self, editor, env=None):
        os.environ['EDITOR'] = os.path.join(self.editor_path, KPTestCase.EDITORS[editor])
        if env:
            os.environ['EDITOR_ENV'] = env

    # Assertions
    def assertStdoutEquals(self, *refs):
        self.assertEqual(self.stdout.splitlines(), list(refs))

    def assertStdoutContains(self, *refs):
        self.assertEqual(collections.Counter(self.stdout.splitlines()), collections.Counter(list(refs)))

    def assertSafeExists(self, safe):
        self.assertTrue(os.path.isfile(os.path.join(self.kp_ws, safe)))

    def assertSafeIsBigEnough(self, safe, size=20):
        self.assertGreater(os.path.getsize(os.path.join(self.kp_ws, safe)), size)

    def assertClearTextExists(self):
        self.assertTrue(os.path.isfile(self.clear_text))

    def assertClearTextPasswordSizeEquals(self, size):
        with open(self.clear_text) as f:
                clear = f.read().splitlines()
        self.assertEqual(len(clear[0]), size)

    def assertClearTextEquals(self, *refs):
        with open(self.clear_text) as f:
            self.assertEqual(f.read().splitlines(), list(refs))

    def assertWsExists(self):
        self.assertTrue(os.path.isdir(self.kp_ws))

    # Run commands
    def cmd(self, args, password=None, confirm=False):
        self.stdout = ""
        self.child = pexpect.spawn(self.kp, args)

        if password:
            self.child.expect('password:')
            self.child.sendline(password)
            if confirm:
                self.child.expect('confirm:')
                self.child.sendline(password)

        for line in self.child:
            self.stdout = self.stdout + line

    def init(self):
        self.cmd(['init'])

    def create(self, name, options=None, password="test password"):
        cmd = ['create']
        if options:
            cmd = cmd + options
        self.cmd(cmd + [name], password=password, confirm=True)

    def edit(self, name, options=None, password="test password"):
        cmd = ['edit']
        if options:
            cmd = cmd + options
        self.cmd(cmd + [name], password=password, confirm=False)

    def open(self, name, options=None, password="test password"):
        cmd = ['open']
        if options:
            cmd = cmd + options
        self.cmd(cmd + [name], password=password, confirm=False)

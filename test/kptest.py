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
import os
import pexpect
import shlex
import shutil
import subprocess
import logging
import sys
import unittest

class KPAgent(subprocess.Popen):
    def __init__(self, kp):
        super(KPAgent, self).__init__([kp, 'agent', '-d'], stdout=subprocess.PIPE, universal_newlines=True)
        env, value = self.stdout.readline().strip().split('=')
        self.env = {env: value}
        os.environ.update(self.env)


def with_agent(f):
    def wrapper(self):
        self.start_agent()
        f(self)
        self.stop_agent()

    return wrapper


class KPTestCase(unittest.TestCase):
    EDITORS = {
            'env': 'TestFunctionalEditorEnv.sh',
            'date': 'TestFunctionalEditorDate.sh',
            'save': 'TestFunctionalEditorSave.sh',
        }

    def __init__(self, *args, **kwargs):
        super(KPTestCase, self).__init__(*args, **kwargs)
        # Get env vars
        self.kp = os.environ['KP']
        self.editor_path = os.environ['EDITOR_PATH']
        self.kp_ws = os.path.join(os.environ['HOME'], '.kickpass')
        self.clear_text = os.path.join(os.environ['HOME'], 'editor-save.txt')
        self.agent = None

    @classmethod
    def setUpClass(cls):
        logging.getLogger().setLevel(logging.INFO)

    def setUp(self):
        shutil.rmtree(self.kp_ws, ignore_errors=True)
        self.init()

    # Env
    def editor(self, editor, env=None):
        os.environ['EDITOR'] = os.path.join(self.editor_path, KPTestCase.EDITORS[editor])
        if env:
            os.environ['EDITOR_ENV'] = env

    # Assertions
    def assertStdoutEquals(self, *refs):
        lines = [l for l in self.stdout.splitlines() if len(l.strip()) > 0]
        self.assertEqual(lines, list(refs))

    def assertStdoutContains(self, *refs):
        lines = [l for l in self.stdout.splitlines() if len(l.strip()) > 0]
        self.assertEqual(collections.Counter(lines), collections.Counter(list(refs)))

    def assertSafeExists(self, safe):
        self.assertTrue(os.path.isfile(os.path.join(self.kp_ws, safe)))

    def assertSafeDoesntExists(self, safe):
        self.assertFalse(os.path.isfile(os.path.join(self.kp_ws, safe)))

    def assertSafeIsBigEnough(self, safe, size=20):
        self.assertGreater(os.path.getsize(os.path.join(self.kp_ws, safe)), size)

    def assertClearTextExists(self):
        self.assertTrue(os.path.isfile(self.clear_text))

    def assertClearTextEquals(self, *refs):
        with open(self.clear_text) as f:
            self.assertEqual(f.read().splitlines(), list(refs))

    def assertWsExists(self):
        self.assertTrue(os.path.isdir(self.kp_ws))

    # Run commands
    def cmd(self, args, master=None, confirm_master=False, password=None, confirm_password=False, yesno=None):
        options = {"master":master, "confirm_master":confirm_master, "password":password, "confirm_password":confirm_password, "yesno":yesno}
        logging.info(" ".join([self.kp]+args) + " [" + ", ".join(["{}={}".format(k, v) for k, v in options.items()]) + "]")
        self.stdout = ""
        cmd = [self.kp] + args
        if "VALGRIND_COMMAND" in os.environ:
            cmd = [os.environ["VALGRIND_COMMAND"], "--log-file=valgrind-"+self.id()+".log"] + shlex.split(os.environ.get("VALGRIND_OPTIONS", "")) + cmd
        self.child = pexpect.spawn(cmd[0], cmd[1:])

        if master is not None:
            self.child.expect('password:')
            self.child.sendline(master)
            if confirm_master:
                self.child.expect('confirm:')
                self.child.sendline(master)

        if password is not None:
            self.child.expect('password:')
            self.child.sendline(password)
            if confirm_password:
                self.child.expect('confirm:')
                self.child.sendline(password)

        if yesno is not None:
            self.child.expect('(y/n)')
            self.child.sendline(yesno)

        for line in self.child:
            self.stdout = self.stdout + (line.decode(sys.stdin.encoding) if sys.stdin.encoding is not None else line)

        self.child.wait()
        self.assertEqual(self.child.exitstatus, 0)

    def start_agent(self):
        self.agent = KPAgent(self.kp)

    def stop_agent(self):
        res = self.agent.poll()
        self.assertIsNone(res)
        self.agent.terminate()
        res = self.agent.wait()
        self.agent.stdout.close()
        self.assertIsNotNone(res)
        for env in self.agent.env:
            del os.environ[env]
        self.agent = None

    def init(self):
        self.cmd(['init', '--memlimit', '16777216', '--opslimit', '32768'], master="test master password", confirm_master=True)

    def create(self, name, options=None, master="test master password", password="test password"):
        cmd = ['create']
        if options:
            cmd = cmd + options
        self.cmd(cmd + [name], master=master, confirm_master=False, password=password, confirm_password=True)

    def edit(self, name, options=None, master="test master password", password="test password", yesno=None):
        cmd = ['edit']
        if options:
            cmd = cmd + options
        self.cmd(cmd + [name], master=master, confirm_master=False, password=password, confirm_password=True, yesno=yesno)

    def rename(self, old, new, options=None, master="test master password"):
        cmd = ['rename']
        if options:
            cmd = cmd + options
        self.cmd(cmd + [old, new], master=master, confirm_master=False)

    def cat(self, name, options=None, master="test master password"):
        cmd = ['cat']
        if options:
            cmd = cmd + options
        self.cmd(cmd + [name], master=master, confirm_master=False)

    def delete(self, name, options=None, master="test master password"):
        cmd = ['delete']
        if options:
            cmd = cmd + options
        self.cmd(cmd + [name], master=master, confirm_master=False)

    def open(self, name, options=None, master="test master password"):
        cmd = ['open']
        if options:
            cmd = cmd + options
        self.cmd(cmd + [name], master=master, confirm_master=False)

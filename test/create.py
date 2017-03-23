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
import unittest
import kptest

class TestCreateCommand(kptest.KPTestCase):

    def test_create_is_successful(self):
        # Given
        self.editor('date')

        # When
        self.create("test")

        # Then
        self.assertSafeExists("test")
        self.assertSafeIsBigEnough("test")

    def test_create_with_subdir_is_successful(self):
        # Given
        self.editor('date')

        # When
        self.create("subdir/test")

        # Then
        self.assertSafeExists("subdir/test")
        self.assertSafeIsBigEnough("subdir/test")

    def test_create_with_password_generation_is_successful(self):
        # Given
        self.editor('save')

        # When
        self.create("test", options=["-g", "-l", "42"], password=None)

        # Then
        self.cat("test", options=["-p"])
        passwd = self.stdout.splitlines()[1]
        self.assertEqual(len(passwd), 42)

    @kptest.with_agent
    def test_create_with_agent_is_successful(self):
        # Given
        self.editor('env', env="Watch out for turtles. They'll bite you if you put your fingers in their mouths.")
        self.create("test")

        # When
        # cat should ask for password, thus master param is not set to None
        self.cat("test")

        # Then
        self.assertStdoutEquals("Watch out for turtles. They'll bite you if you put your fingers in their mouths.")

    @kptest.with_agent
    def test_create_open_with_agent_is_successful(self):
        # Given
        self.editor('env', env="Watch out for turtles. They'll bite you if you put your fingers in their mouths.")
        self.create("test", options=["-o"])

        # When
        self.cat("test", master=None)

        # Then
        self.assertStdoutEquals("Watch out for turtles. They'll bite you if you put your fingers in their mouths.")

if __name__ == '__main__':
        unittest.main()

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

class TestEditCommand(kptest.KPTestCase):

    def test_edit_is_successful(self):
        # Given
        self.init()
        self.editor('env', env="But a RocknRolla, oh, he's different. Why? Because a real RocknRolla wants the fucking lot.")
        self.create("test")
        self.editor('save')

        # When
        self.edit("test")

        # Then
        self.assertSafeExists("test")
        self.assertSafeIsBigEnough("test")
        self.assertClearTextExists()
        self.assertClearTextEquals("But a RocknRolla, oh, he's different. Why? Because a real RocknRolla wants the fucking lot.")

    def test_edit_only_password_is_successful(self):
        # Given
        self.init()
        self.editor('date')
        self.create("test")

        # When
        self.edit("test", password="RocknRolla", options=["-p"])

        # Then
        self.open("test", options=["-p"])
        self.assertStdoutEquals("RocknRolla")

    def test_edit_only_metadata_is_successful(self):
        # Given
        self.init()
        self.editor('date')
        self.create("test")
        self.editor('env', env="Oh, you are something special, Mr. Johnny Quid.")

        # When
        self.edit("test", password=None, options=["-m"])

        # Then
        self.open("test")
        self.assertStdoutEquals("Oh, you are something special, Mr. Johnny Quid.")

    def test_edit_with_empty_password(self):
        # Given
        self.init()
        self.editor('date')
        self.create("test", password="RocknRolla")

        # When
        self.edit("test", password="", options=["-p"], yesno="n")

        # Then
        self.open("test", options=["-p"])
        self.assertStdoutEquals("RocknRolla")

    def test_edit_with_empty_password_erased(self):
        # Given
        self.init()
        self.editor('date')
        self.create("test", password="RocknRolla")

        # When
        self.edit("test", password="", options=["-p"], yesno="y")

        # Then
        self.open("test", options=["-p"])
        self.assertStdoutEquals()

    def test_edit_with_password_generation_is_successful(self):
        # Given
        self.init()
        self.editor('save')
        self.create("test", password="RocknRolla")

        # When
        self.edit("test", options=["-g", "-l", "42"], password=None)

        # Then
        self.cat("test", options=["-p"])
        passwd = self.stdout.splitlines()[1]
        self.assertNotEqual(passwd, "RocknRolla")
        self.assertEqual(len(passwd), 42)

if __name__ == '__main__':
        unittest.main()

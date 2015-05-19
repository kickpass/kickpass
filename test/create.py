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
        self.init()
        self.editor('date')

        # When
        self.create("test")

        # Then
        self.assertSafeExists("test")
        self.assertSafeIsBigEnough("test")

    def test_create_with_subdir_is_successful(self):
        # Given
        self.init()
        self.editor('date')

        # When
        self.create("subdir/test")

        # Then
        self.assertSafeExists("subdir/test")
        self.assertSafeIsBigEnough("subdir/test")

    def test_create_with_password_generation_is_successful(self):
        # Given
        self.init()
        self.editor('save')

        # When
        self.create("test", options=["-g", "-l", "42"])

        # Then
        self.assertClearTextPasswordSizeEquals(42)

if __name__ == '__main__':
        unittest.main()

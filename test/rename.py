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

class TestRenameCommand(kptest.KPTestCase):

    def test_rename_is_successful(self):
        # Given
        self.editor('date')
        self.create("old")

        # When
        self.rename("old", "new")

        # Then
        self.assertSafeDoesntExists("old")
        self.assertSafeExists("new")
        self.assertSafeIsBigEnough("new")

    def test_rename_on_nonexistent_directory_is_successful(self):
        # Given
        self.init()
        self.editor('date')
        self.create("old")

        # When
        self.rename("old", "nonexistent/new")

        # Then
        self.assertSafeDoesntExists("old")
        self.assertSafeExists("nonexistent/new")
        self.assertSafeIsBigEnough("nonexistent/new")

if __name__ == '__main__':
        unittest.main()

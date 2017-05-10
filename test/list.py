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

class TestListCommand(kptest.KPTestCase):

    def test_list_is_complete(self):
        # Given
        self.editor('date')
        self.create("subdir/test")
        self.create("subdir/other")

        # When
        self.cmd(["ls"])

        # Then
        self.assertStdoutContains("subdir/other", "subdir/test")

    def test_list_is_sorted(self):
        # Given
        self.editor('date')
        self.create("subdir/test")
        self.create("subdir/other")

        # When
        self.cmd(["ls"])

        # Then
        self.assertStdoutEquals("subdir/other", "subdir/test")

    def test_list_subpath(self):
        # Given
        self.editor('date')
        self.create("withoutdir")
        self.create("subdir/test")
        self.create("subdir/other")
        self.create("important/doh")
        self.create("important/lastone")
        self.create("boringdir/stuff")
        self.create("boringdir/things")

        # When
        self.cmd(["ls", "subdir", "important"])

        # Then
        self.assertStdoutEquals("subdir/",
                                "  other",
                                "  test",
                                "important/",
                                "  doh",
                                "  lastone")

if __name__ == '__main__':
        unittest.main()

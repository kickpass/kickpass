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

class TestDeleteCommand(kptest.KPTestCase):

    def test_delete_is_successful(self):
        # Given
        self.init()
        self.editor('date')
        self.create("test", master="MyT0pS3cr3tP4ssw0rd")

        # When
        self.delete("test", master="MyT0pS3cr3tP4ssw0rd")

        # Then
        self.assertSafeDoesntExists("test")

    def test_delete_refuse_to_delete_without_valid_password(self):
        # Given
        self.init()
        self.editor('date')
        self.create("test", master="MyT0pS3cr3tP4ssw0rd")

        # When
        self.delete("test", master="Il0v3ST20")

        # Then
        self.assertSafeExists("test")

    def test_delete_is_successful_with_force_and_invalid_password(self):
        # Given
        self.init()
        self.editor('date')
        self.create("test", master="MyT0pS3cr3tP4ssw0rd")

        # When
        self.delete("test", options=['-f'], master=None)

        # Then
        self.assertSafeDoesntExists("test")

if __name__ == '__main__':
        unittest.main()

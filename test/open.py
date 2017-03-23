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
import time

class TestOpenCommand(kptest.KPTestCase):

    @kptest.with_agent
    def test_open_timeout_remove_password_from_agent(self):
        # Given
        self.editor('env', env="Watch out for turtles. They'll bite you if you put your fingers in their mouths.")
        self.create("test")
        self.open("test", options=['-t', '1'])

        # When
        time.sleep(2)
        # cat should ask for password, thus master param is not set to None
        self.cat("test")

        # Then
        self.assertStdoutEquals("Watch out for turtles. They'll bite you if you put your fingers in their mouths.")

if __name__ == '__main__':
        unittest.main()

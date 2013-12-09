polld
=====

**This is dead code and there is probably no good use for it.**

[![Build Status](https://travis-ci.org/nijel/polld.png?branch=master)](https://travis-ci.org/nijel/polld)
[![Coverage Status](https://coveralls.io/repos/nijel/polld/badge.png)](https://coveralls.io/r/nijel/polld)

This is simple daemon which peridically opens files (defined in /etc/polld).
I use this for scanning partitions in card reader, which does not report card
insertion/removal. With polld, they appear in 10 seconds after insertion and
udev will then create appripriate device nodes.

Program homepage is at <http://cihar.com/software/polld>.

Please report bugs to <https://github.com/nijel/polld/issues>.

Git is available at GitHub: <https://github.com/nijel/polld>

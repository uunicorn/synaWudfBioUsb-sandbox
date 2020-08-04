#!/bin/sh

export WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/7.3-win32
export WINEPREFIX=~/.wine-testbed
export WINEDEBUG=trace+rsaenh,trace+bcrypt,trace+crypt

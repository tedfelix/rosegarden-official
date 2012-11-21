#!/bin/sh

# This script creates the configure script using the autoconf tools.

# A more modern and complete way of doing this:
#
#   autoreconf --force --install
#
# We might want to switch to this at some point in the future.

aclocal -I . && autoconf

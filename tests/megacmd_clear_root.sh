#!/bin/sh

set -e

mega-login $MEGACMD_TEST_USER $MEGACMD_TEST_PASS

mega-rm -r -f /*

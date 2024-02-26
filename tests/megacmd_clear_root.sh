#!/bin/sh

set -e

mega-login $MEGA_EMAIL $MEGA_PWD

LS_OUTPUT=`mega-ls "/" | xargs echo -n`
if ["$LS_OUTPUT" = ""]; then
    echo "Account cloud storage already empty, nothing to clear."
else
    mega-rm -rf "/*"
fi

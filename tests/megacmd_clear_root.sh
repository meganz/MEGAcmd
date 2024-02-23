#!/bin/sh

set -e

mega-login $MEGA_EMAIL $MEGA_PWD

mega-rm -r -f /*

#!/bin/bash

##
 # @file examples/megacmd/build/generate_changelog_entry.sh
 # @brief Processes the input file and prints DEB ChangeLog entry
 #
 # (c) 2013-2014 by Mega Limited, Auckland, New Zealand
 #
 # This file is part of the MEGA SDK - Client Access Engine.
 #
 # Applications using the MEGA API must present a valid application key
 # and comply with the the rules set forth in the Terms of Service.
 #
 # The MEGA SDK is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 #
 # @copyright Simplified (2-clause) BSD License.
 #
 # You should have received a copy of the license along with this
 # program.
##

if [ "$#" -ne 2 ]; then
    echo " $0 [version] [input file path]"
    exit 1
fi

in_file="$2"
out1=$(awk -NF '"$| "|""\\\\n' ' /megacmdchangelog/ {flag=1;next} /;/{flag=0} flag { print "  * "$2 }' $in_file)


# print ChangeLog entry
NOW=$(LANG=C date -R)
echo "megacmd ($1) stable; urgency=low"
echo ""
echo "$out1" | sed 's#\\"#"#g'
echo ""
echo " -- MEGA Team <linux@mega.co.nz>  $NOW"
echo ""

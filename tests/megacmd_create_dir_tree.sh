#!/bin/sh

set -e

mega-login $MEGACMD_TEST_USER $MEGACMD_TEST_PASS

TMP=$(mktemp --directory)
cleanup() {
    ARG=$?
    echo "Removing $TMP"
    rm -rf $TMP
    exit $ARG
}
trap cleanup EXIT

mkdir -p $TMP/testReadingFolder01

for i in 01 02;
do
    mkdir -p $TMP/testReadingFolder01/folder$i
    for n in 01 02 03;
    do
	echo $TMP/testReadingFolder01/file$n.txt > $TMP/testReadingFolder01/file$n.txt
	echo $TMP/testReadingFolder01/folder$i/file$n.txt > $TMP/testReadingFolder01/folder$i/file$n.txt

	mkdir -p $TMP/testReadingFolder01/folder$i/subfolder$n
	for n2 in 01 02 03;
	do
	    echo $TMP/testReadingFolder01/folder$i/subfolder$n/file$n2.txt > $TMP/testReadingFolder01/folder$i/subfolder$n/file$n2.txt
	done
    done
done

mega-put $TMP/testReadingFolder01 /

#!/bin/bash
#better run in an empty folder

GET=mega-get
RM=mega-rm
CD=mega-cd
LCD=mega-lcd
EXPORT=mega-export
MKDIR=mega-mkdir
IMPORT=mega-import
ABSPWD=`pwd`

if [ "x$VERBOSE" == "x" ]; then
VERBOSE=0
fi

if [ "x$MEGACMDSHELL" != "x" ]; then
GET="executeinMEGASHELL get"
CMDSHELL=1
fi


function executeinMEGASHELL()
{
	command=$1
	shift;
	echo lcd "$PWD" > /tmp/shellin
	echo $command "$@" >> /tmp/shellin
	#~ echo $command "$@" > /tmp/shellin
	
	$MEGACMDSHELL < /tmp/shellin  | sed "s#^.*\[K##g" | grep $MEGA_EMAIL -A 1000 | grep -v $MEGA_EMAIL
}


function clean_all() { 
	if [[ $(mega-whoami) != *"$MEGA_EMAIL" ]]; then
	mega-logout || :
	mega-login $MEGA_EMAIL $MEGA_PWD || exit -1
	fi
	$RM -rf "*" > /dev/null
	$RM -rf "//bin/*" > /dev/null
	if [ -e origin ]; then rm -r origin; fi
	if [ -e megaDls ]; then rm -r megaDls; fi
	if [ -e localDls ]; then rm -r localDls; fi
}

function clear_dls() {
	rm -r megaDls/* 2>/dev/null
	rm -r localDls/* 2>/dev/null
}

currentTest=1

function compare_and_clear() {
if [ "$VERBOSE" == "1" ]; then
	echo "test $currentTest"
	echo "megaDls:"
	find megaDls
	echo
	echo "localDls:"
	find localDls
	echo
fi

if diff megaDls localDls 2>/dev/null >/dev/null; then
	echo "test $currentTest succesful!"
else
	echo "test $currentTest failed!"
	cd $ABSPWD
	exit 1
fi

clear_dls
currentTest=$((currentTest+1))
$CD /
}

function check_failed_and_clear() {
if [ "$?" != "0" ]; then
	echo "test $currentTest succesful!"
else
	echo "test $currentTest failed!"
	cd $ABSPWD
	exit 1
fi

clear_dls
currentTest=$((currentTest+1))
$CD /
}

function initialize_contents() {

if [[ $(mega-whoami) != *"$MEGA_EMAIL_AUX" ]]; then
mega-logout || :
mega-login $MEGA_EMAIL_AUX $MEGA_PWD_AUX || exit -1
fi


if [ "$(ls -A .)" ]; then
	echo "initialization folder not empty!"
	cd $ABSPWD
	exit 1
fi
mkdir -p cloud0{1,2}/c0{1,2}s0{1,2}
mkdir -p foreign/sub0{1,2}
mkdir -p bin0{1,2}/c0{1,2}s0{1,2}
for i in `find *`; do echo $i >  $i/fileat`basename $i`.txt; done

mega-put foreign /
mega-share foreign -a --with=$MEGA_EMAIL
URIFOREIGNEXPORTEDFOLDER=`$EXPORT foreign/sub01 -a | awk '{print $NF}'`
URIFOREIGNEXPORTEDFILE=`$EXPORT foreign/sub02/fileatsub02.txt -a | awk '{print $NF}'`

mega-logout

mega-login $MEGA_EMAIL $MEGA_PWD
mega-ipc -a $MEGA_EMAIL_AUX >/dev/null

mega-put cloud0* /
mega-put bin0* //bin
URIEXPORTEDFOLDER=`$EXPORT cloud01/c01s01 -a | awk '{print $NF}'`
URIEXPORTEDFILE=`$EXPORT cloud02/fileatcloud02.txt -a | awk '{print $NF}'`

}

if [ "$MEGA_EMAIL" == "" ] || [ "$MEGA_PWD" == "" ] || [ "$MEGA_EMAIL_AUX" == "" ] || [ "$MEGA_PWD_AUX" == "" ]; then
	echo "You must define variables MEGA_EMAIL MEGA_PWD MEGA_EMAIL_AUX MEGA_PWD_AUX. WARNING: Use an empty account for $MEGA_EMAIL"
	cd $ABSPWD
	exit 1
fi

#INITIALIZATION
clean_all
mkdir origin
pushd origin > /dev/null
initialize_contents
popd > /dev/null

mkdir -p megaDls
mkdir -p localDls

ABSMEGADLFOLDER=$PWD/megaDls

URIEXPORTEDFOLDER=`mega-export cloud01/c01s01 -a | awk '{print $NF}'`
URIEXPORTEDFILE=`mega-export cloud02/fileatcloud02.txt -a | awk '{print $NF}'`

clear_dls

#Test 01
$GET /cloud01/fileatcloud01.txt $ABSMEGADLFOLDER/
cp origin/cloud01/fileatcloud01.txt localDls/
compare_and_clear

#Test 02
$GET //bin/bin01/fileatbin01.txt $ABSMEGADLFOLDER
cp origin/bin01/fileatbin01.txt localDls/
compare_and_clear

#Test 03
$GET //bin/bin01/fileatbin01.txt $ABSMEGADLFOLDER/
cp origin/bin01/fileatbin01.txt localDls/
compare_and_clear

#Test 04
$GET $MEGA_EMAIL_AUX:foreign/fileatforeign.txt $ABSMEGADLFOLDER/
cp origin/foreign/fileatforeign.txt localDls/
compare_and_clear

#Test 05
$GET $MEGA_EMAIL_AUX:foreign/fileatforeign.txt $ABSMEGADLFOLDER/
cp origin/foreign/fileatforeign.txt localDls/
compare_and_clear

#Test 06
$CD cloud01
$GET "*.txt" $ABSMEGADLFOLDER
cp origin/cloud01/*.txt localDls/
compare_and_clear

#Test 07
$CD //bin/bin01
$GET "*.txt" $ABSMEGADLFOLDER
cp origin/bin01/*.txt localDls/
compare_and_clear

#Test 08
$CD $MEGA_EMAIL_AUX:foreign
$GET "*.txt" $ABSMEGADLFOLDER
cp origin/foreign/*.txt localDls/
compare_and_clear

#Test 09
$GET cloud01/c01s01 $ABSMEGADLFOLDER
cp -r origin/cloud01/c01s01 localDls/
compare_and_clear

#Test 10
$GET cloud01/c01s01 $ABSMEGADLFOLDER/
cp -r origin/cloud01/c01s01 localDls/
compare_and_clear

#Test 11
$GET cloud01/c01s01 $ABSMEGADLFOLDER -m
rsync -r origin/cloud01/c01s01/ localDls/
compare_and_clear

#Test 12
$GET cloud01/c01s01 $ABSMEGADLFOLDER/ -m
rsync -r origin/cloud01/c01s01/ localDls/
compare_and_clear

#Test 13
$GET "$URIEXPORTEDFOLDER" $ABSMEGADLFOLDER
cp -r origin/cloud01/c01s01 localDls/
compare_and_clear

#Test 14
$GET "$URIEXPORTEDFILE" $ABSMEGADLFOLDER
cp origin/cloud02/fileatcloud02.txt localDls/
compare_and_clear

#Test 15
$GET "$URIEXPORTEDFOLDER" $ABSMEGADLFOLDER -m
rsync -r  origin/cloud01/c01s01/ localDls/
compare_and_clear

#Test 16
$CD /cloud01/c01s01
$GET . $ABSMEGADLFOLDER
cp -r origin/cloud01/c01s01 localDls/
compare_and_clear

#Test 17
$CD /cloud01/c01s01
$GET . $ABSMEGADLFOLDER -m
rsync -r origin/cloud01/c01s01/ localDls/
compare_and_clear

#Test 18
$CD /cloud01/c01s01
$GET ./ $ABSMEGADLFOLDER
cp -r origin/cloud01/c01s01 localDls/
compare_and_clear

#Test 19
$CD /cloud01/c01s01
$GET ./ $ABSMEGADLFOLDER -m
rsync -r origin/cloud01/c01s01/ localDls/
compare_and_clear

#Test 20
$CD /cloud01/c01s01
$GET .. $ABSMEGADLFOLDER -m
rsync -r origin/cloud01/ localDls/
compare_and_clear

#Test 21
$CD /cloud01/c01s01
$GET ../ $ABSMEGADLFOLDER
cp -r origin/cloud01 localDls/
compare_and_clear

#Test 22
echo "existing" > $ABSMEGADLFOLDER/existing
$GET /cloud01/fileatcloud01.txt $ABSMEGADLFOLDER/existing
cp origin/cloud01/fileatcloud01.txt "localDls/existing (1)"
echo "existing" > localDls/existing
compare_and_clear

#Test 23
echo "existing" > megaDls/existing
$GET /cloud01/fileatcloud01.txt megaDls/existing
cp origin/cloud01/fileatcloud01.txt "localDls/existing (1)"
echo "existing" > localDls/existing
compare_and_clear

#Test 24
$GET cloud01/c01s01 megaDls
cp -r origin/cloud01/c01s01 localDls/
compare_and_clear

#Test 25
$GET cloud01/fileatcloud01.txt megaDls
cp -r origin/cloud01/fileatcloud01.txt localDls/
compare_and_clear

if [ "x$CMDSHELL" != "x1" ]; then #TODO: currently there is no way to know last CMSHELL status code
	#Test 26
	$GET cloud01/fileatcloud01.txt /no/where > /dev/null
	check_failed_and_clear

	#Test 27
	$GET /cloud01/cloud01/fileatcloud01.txt /no/where > /dev/null
	check_failed_and_clear
fi

currentTest=28

#Test 28
$GET /cloud01/fileatcloud01.txt $ABSMEGADLFOLDER/newfile
cp origin/cloud01/fileatcloud01.txt localDls/newfile
compare_and_clear

#Test 29
pushd $ABSMEGADLFOLDER > /dev/null
$GET /cloud01/fileatcloud01.txt .
popd > /dev/null
cp origin/cloud01/fileatcloud01.txt localDls/
compare_and_clear

#Test 30
pushd $ABSMEGADLFOLDER > /dev/null
$GET /cloud01/fileatcloud01.txt ./
popd > /dev/null
cp origin/cloud01/fileatcloud01.txt localDls/
compare_and_clear

#Test 31
mkdir $ABSMEGADLFOLDER/newfol
pushd $ABSMEGADLFOLDER/newfol > /dev/null
$GET /cloud01/fileatcloud01.txt ..
popd > /dev/null
mkdir localDls/newfol
cp origin/cloud01/fileatcloud01.txt localDls/
compare_and_clear

#Test 32
mkdir $ABSMEGADLFOLDER/newfol
pushd $ABSMEGADLFOLDER/newfol > /dev/null
$GET /cloud01/fileatcloud01.txt ../
popd > /dev/null
mkdir localDls/newfol
cp origin/cloud01/fileatcloud01.txt localDls/
compare_and_clear

if [ "x$CMDSHELL" != "x1" ]; then #TODO: currently there is no way to know last CMSHELL status code
#Test 33
$GET path/to/nowhere $ABSMEGADLFOLDER > /dev/null
check_failed_and_clear

#Test 34
$GET /path/to/nowhere $ABSMEGADLFOLDER > /dev/null
check_failed_and_clear
fi

currentTest=35

#Test 35
pushd $ABSMEGADLFOLDER > /dev/null
$GET /cloud01/fileatcloud01.txt
popd > /dev/null
cp origin/cloud01/fileatcloud01.txt localDls/
compare_and_clear

currentTest=36

#Test 36 # imported stuff (to test import folder)
$RM -rf /imported 2>&1 >/dev/null || :
$MKDIR -p /imported
$IMPORT $URIFOREIGNEXPORTEDFOLDER /imported > /dev/null
$GET /imported/* $ABSMEGADLFOLDER
cp -r origin/foreign/sub01 localDls/
compare_and_clear

#Test 37 # imported stuff (to test import file)
$RM -rf /imported 2>&1 >/dev/null || :
$MKDIR -p /imported
$IMPORT $URIFOREIGNEXPORTEDFILE /imported/ > /dev/null
$GET /imported/fileatsub02.txt $ABSMEGADLFOLDER
cp origin/foreign/sub02/fileatsub02.txt localDls/
compare_and_clear

# Clean all
clean_all

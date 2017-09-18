#!/bin/bash

GET=mega-get
PUT=mega-put
RM=mega-rm
CD=mega-cd
LCD=mega-lcd
MKDIR=mega-mkdir
EXPORT=mega-export
FIND=mega-find


if [ "x$VERBOSE" == "x" ]; then
VERBOSE=0
fi

if [ "x$MEGACMDSHELL" != "x" ]; then
RM="executeinMEGASHELL rm"
CMDSHELL=1
fi


function executeinMEGASHELL()
{
	command=$1
	shift;
	echo $command "$@" > /tmp/shellin
	
	$MEGACMDSHELL < /tmp/shellin  | sed "s#^.*\[K##g" | grep $MEGA_EMAIL -A 1000 | grep -v $MEGA_EMAIL
}

ABSPWD=`pwd`

function clean_all() { 
	
	if [[ $(mega-whoami) != *"$MEGA_EMAIL" ]]; then
	mega-logout || :
	mega-login $MEGA_EMAIL $MEGA_PWD || exit -1
	fi
	
	$RM -rf "/*" > /dev/null
	$RM -rf "//bin/*" > /dev/null

	if [ -e localUPs ]; then rm -r localUPs; fi
	if [ -e localtmp ]; then rm -r localtmp; fi
	
	if [ -e megafind.txt ]; then rm megafind.txt; fi
	if [ -e localfind.txt ]; then rm localfind.txt; fi
}

function clear_local_and_remote() {
	rm -r localUPs/* 2>/dev/null
	$RM -rf "/*" 2>/dev/null || :
	initialize_contents
}

currentTest=1

function compare_and_clear() {
	if [ "$VERBOSE" == "1" ]; then
		echo "test $currentTest"
	fi
		$FIND | sort > megafind.txt
		(cd localUPs; find . | sed "s#\./##g" | sort) > localfind.txt

	if diff --side-by-side megafind.txt localfind.txt 2>/dev/null >/dev/null; then
		if [ "$VERBOSE" == "1" ]; then
			echo "diff megafind vs localfind:"
			diff --side-by-side megafind.txt localfind.txt
		fi
		echo "test $currentTest succesful!"
	else
		echo "test $currentTest failed!"
		echo "diff megafind vs localfind:"
		diff --side-by-side megafind.txt localfind.txt
		cd $ABSPWD
		exit 1
	fi

	clear_local_and_remote
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

	clear_local_and_remote
	currentTest=$((currentTest+1))
	$CD /
}

function initialize () {

	if [[ $(mega-whoami) != *"$MEGA_EMAIL" ]]; then
	mega-logout || :
	mega-login $MEGA_EMAIL $MEGA_PWD || exit -1
	fi

	if [ "$(ls -A .)" ]; then
		echo "initialization folder not empty!"
		cd $ABSPWD
		exit 1
	fi

	if [ $($FIND / | wc  -l) != 1 ]; then 
		echo "REMOTE Not empty, please clear it before starting!"
		cd $ABSPWD
		exit 1
	fi

	#initialize localtmp estructure:
	mkdir -p localtmp
	touch localtmp/file01.txt
	echo file01contents > localtmp/file01nonempty.txt
	#local empty folders structure
	mkdir -p localtmp/le01/{les01/less01,les02/less0{1,2}}
	#local filled folders structure
	mkdir -p localtmp/lf01/{lfs01/lfss01,lfs02/lfss0{1,2}}
	touch localtmp/lf01/{lfs01/lfss01,lfs02/lfss0{1,2}}/commonfile.txt
	#spaced structure
	mkdir -p localtmp/ls\ 01/{ls\ s01/ls\ ss01,ls\ s02/ls\ ss0{1,2}}
	touch localtmp/ls\ 01/{ls\ s01/ls\ ss01,ls\ s02/ls\ ss0{1,2}}/common\ file.txt

	# localtmp/
	# ├── file01nonempty.txt
	# ├── file01.txt
	# ├── le01
	# │   ├── les01
	# │   │   └── less01
	# │   └── les02
	# │       ├── less01
	# │       └── less02
	# ├── lf01
	# │   ├── lfs01
	# │   │   └── lfss01
	# │   │       └── commonfile.txt
	# │   └── lfs02
	# │       ├── lfss01
	# │       │   └── commonfile.txt
	# │       └── lfss02
	# │           └── commonfile.txt
	# └── ls 01
		# ├── ls s01
		# │   └── ls ss01
		# │       └── common file.txt
		# └── ls s02
			# ├── ls ss01
			# │   └── common file.txt
			# └── ls ss02
				# └── common file.txt


	#initialize dynamic contents:
	clear_local_and_remote
}

function initialize_contents() {
	$PUT localtmp/* /
	mkdir -p localUPs
	cp -r localtmp/* localUPs/
}

if [ "$MEGA_EMAIL" == "" ] || [ "$MEGA_PWD" == "" ]; then
	echo "You must define variables MEGA_EMAIL MEGA_PWD. WARNING: Use an empty account for $MEGA_EMAIL"
	cd $ABSPWD
	exit 1
fi

#INITIALIZATION
clean_all
initialize

ABSMEGADLFOLDER=$PWD/megaDls

clear_local_and_remote

#Test 01 #clean comparison
compare_and_clear

#Test 02 #destiny empty file
$RM file01.txt
rm localUPs/file01.txt
compare_and_clear

#Test 03 #/ destiny empty file upload
$PUT localtmp/file01.txt /
cp localtmp/file01.txt localUPs/
compare_and_clear

#Test 04 #no destiny nont empty file upload
$RM file01nonempty.txt
rm localUPs/file01nonempty.txt
compare_and_clear

#Test 05 #empty folder
$RM -rf le01/les01/less01
rm -r localUPs/le01/les01/less01
compare_and_clear

#Test 06 #1 file folder
$RM -rf lf01/lfs01/lfss01
rm -r localUPs/lf01/lfs01/lfss01
compare_and_clear

#Test 07 #entire empty folders structure
$RM -rf le01
rm -r localUPs/le01
compare_and_clear

#Test 08 #entire non empty folders structure
$RM -rf lf01
rm -r localUPs/lf01
compare_and_clear


#Test 09 #multiple
$RM -rf lf01 le01/les01
rm -r localUPs/{lf01,le01/les01}
compare_and_clear

#Test 10 #.
$CD le01
$RM -rf .
$CD /
rm -r localUPs/le01
compare_and_clear

#Test 11 #..
$CD le01/les01
$RM -rf ..
$CD /
rm -r localUPs/le01
compare_and_clear

#Test 12 #../XX
$CD le01/les01
$RM -rf ../les01
$CD /
rm -r localUPs/le01/les01
compare_and_clear

#Test 13 #spaced stuff
$RM -rf "ls\ 01"
rm -r localUPs/ls\ 01
compare_and_clear

#Test 14 #complex stuff
$RM -rf "ls\ 01/../le01/les01" "lf01/../ls*/ls\ s02"
rm -r localUPs/{ls\ 01/../le01/les01,lf01/../ls*/ls\ s02}
compare_and_clear

#Test 15 #complex stuff with PCRE exp
$RM -rf --use-pcre "ls\ 01/../le01/les0[12]" "lf01/../ls.*/ls\ s0[12]"
rm -r localUPs/{ls\ 01/../le01/les0[12],lf01/../ls*/ls\ s0[12]}
compare_and_clear

###TODO: do stuff in shared folders...

########

#~ #Test XX #regexp #yet unsupported
#~ $RM -rf "le01/les*"
#~ rm -r localUPs/le01/les*
#~ compare_and_clear


# Clean all
if [ "$VERBOSE" != "1" ]; then
clean_all
fi

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
FIND="executeinMEGASHELL find"
CMDSHELL=1
fi


function executeinMEGASHELL()
{
	command=$1
	shift;
	echo $command "$@" > /tmp/shellin
	
	$MEGACMDSHELL < /tmp/shellin  | sed "s#^.*\[K##g" | grep $MEGA_EMAIL -A 1000 | grep -v $MEGA_EMAIL
}



function clean_all() { 
	if [[ $(mega-whoami) != *"$MEGA_EMAIL" ]]; then
	mega-logout || :
	mega-login $MEGA_EMAIL $MEGA_PWD || exit -1
	fi
	
	rm pipe > /dev/null 2>/dev/null || :
	$RM -rf "*" > /dev/null
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

ABSPWD=`pwd`

currentTest=1

function compare_and_clear() {
	if [ "$VERBOSE" == "1" ]; then
		echo "test $currentTest"
	fi
		$FIND | sort > megafind.txt
		(cd localUPs; find | sed "s#\./##g" | sort) > localfind.txt
		
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

	if [ $($FIND / | grep -v "^$" |wc  -l) != 1 ]; then
		echo "REMOTE Not empty, please clear it before starting!"
		#~ $FIND /
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

function compare_find(){
	if [ "$VERBOSE" == "1" ]; then
		echo "test $currentTest"
	fi
	
	$FIND "$@"  | sort > $ABSPWD/megafind.txt
	(cd localUPs 2>/dev/null; find "$@" | sed "s#\./##g" | sort) > $ABSPWD/localfind.txt
	
	
		
	if diff -B --side-by-side $ABSPWD/megafind.txt $ABSPWD/localfind.txt 2>/dev/null >/dev/null; then
		if [ "$VERBOSE" == "1" ]; then
			echo "diff megafind vs localfind:"
			diff -B --side-by-side $ABSPWD/megafind.txt $ABSPWD/localfind.txt
		fi
		echo "test $currentTest succesful!"
	else
		echo "test $currentTest failed!"
		echo "diff megafind vs localfind:"
		diff -B --side-by-side $ABSPWD/megafind.txt $ABSPWD/localfind.txt
		cd $ABSPWD
		exit 1
	fi
	
	currentTest=$((currentTest+1))
}


function find_remote(){
	if [ "$VERBOSE" == "1" ]; then
		echo "test $currentTest"
	fi

	$FIND "$@"  | sort > $ABSPWD/megafind.txt
}

function find_local(){
  if [ "Darwin" == `uname` ]; then
    (cd localUPs 2>/dev/null; find "$@" | sed "s#\.///#/#g"  | sed "s#\.//#/#g" | sort) > $ABSPWD/localfind.txt
  else
    (cd localUPs 2>/dev/null; find "$@" | sed "s#\./##g" | sort) > $ABSPWD/localfind.txt
  fi
}

function find_local_append(){
	(cd localUPs 2>/dev/null; find "$@" | sed "s#\./##g" | sort) >> $ABSPWD/localfind.txt
}

function compare_remote_local(){
	
		if diff -B --side-by-side $ABSPWD/megafind.txt $ABSPWD/localfind.txt 2>/dev/null >/dev/null; then
		if [ "$VERBOSE" == "1" ]; then
			echo "diff megafind vs localfind:"
			diff -B --side-by-side $ABSPWD/megafind.txt $ABSPWD/localfind.txt
		fi
		echo "test $currentTest succesful!"
	else
		echo "test $currentTest failed!"
		echo "diff megafind vs localfind:"
		diff -B --side-by-side $ABSPWD/megafind.txt $ABSPWD/localfind.txt
		cd $ABSPWD
		exit 1
	fi
	
	currentTest=$((currentTest+1))
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

#Test 01 #destiny empty file
compare_find file01.txt

#Test 02  #entire empty folders
compare_find le01/les01/less01

#Test 03 #1 file folder
compare_find lf01/lfs01/lfss01

#Test 04 #entire non empty folders structure
compare_find lf01

#Test 05 #multiple
compare_find lf01 le01/les01

#Test 06 #.
$CD le01
pushd localUPs/le01 > /dev/null
compare_find .
$CD /
popd > /dev/null

#Test 07 #. global
compare_find .

#Test 08 #. global
find_remote "ls\ 01"
find_local ls\ 01
compare_remote_local

#Test 09 #XX/..
find_remote "ls\ 01/.."
find_local .//
compare_remote_local

#Test 10 #..
$CD le01
find_remote ..
$CD /
find_local .//
compare_remote_local

#Test 11 #complex stuff
#$RM -rf ls\ 01/../le01/les01 lf01/../ls*/ls\ s02 #This one fails since it is PCRE expresion TODO: if ever supported PCRE enabling/disabling, consider that
find_remote "ls\ 01/../le01/les01" "lf01/../ls\ *01/ls\ s02"
find_local .//le01/les01
find_local_append .//ls\ 01/ls\ s02
compare_remote_local


#Test 12 #folder/
find_remote le01/
find_local le01
compare_remote_local

if [ "x$CMDSHELL" != "x1" ]; then #TODO: currently there is no way to know last CMSHELL status code

	#Test 13 #file01.txt/non-existent
	$FIND file01.txt/non-existent >/dev/null 2>/dev/null
	if [ $? == 0 ]; then echo "test $currentTest failed!"; cd $ABSPWD; exit 1;
	else echo "test $currentTest succesful!"; fi

fi

currentTest=14

#Test 14 #/
find_remote /
find_local .//
compare_remote_local
###TODO: do stuff in shared folders...

########

# Clean all
if [ "$VERBOSE" != "1" ]; then
clean_all
fi

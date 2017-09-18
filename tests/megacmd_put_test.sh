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
PUT="executeinMEGASHELL put"
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

ABSPWD=`pwd`

function clean_all() { 
	if [[ $(mega-whoami) != *"$MEGA_EMAIL" ]]; then
	mega-logout || :
	mega-login $MEGA_EMAIL $MEGA_PWD || exit -1
	fi
	$RM -rf "*" > /dev/null
	$RM -rf "//bin/*" > /dev/null

	if [ -e localUPs ]; then rm -r localUPs; fi
	if [ -e localtmp ]; then rm -r localtmp; fi
	
	if [ -e megafind.txt ]; then rm megafind.txt; fi
	if [ -e localfind.txt ]; then rm localfind.txt; fi
}

function clear_local_and_remote() {
	rm -r localUPs/* 2>/dev/null
	$RM -rf /01/../* >/dev/null 2>/dev/null || :
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
		exit 1
	fi

	if [ $($FIND / | wc  -l) != 1 ]; then 
		echo "REMOTE Not empty, please clear it before starting!"
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
	$MKDIR -p 01/{s01/ss01,s02/ss0{1,2}}
	mkdir -p localUPs/01/{s01/ss01,s02/ss0{1,2}}
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

#Test 02 #no destiny empty file upload
$PUT localtmp/file01.txt
cp localtmp/file01.txt localUPs/
compare_and_clear

#Test 03 #/ destiny empty file upload
$PUT localtmp/file01.txt /
cp localtmp/file01.txt localUPs/
compare_and_clear

#Test 04 #no destiny nont empty file upload
$PUT localtmp/file01nonempty.txt
cp localtmp/file01nonempty.txt localUPs/
compare_and_clear

#Test 05 #empty folder
$PUT localtmp/le01/les01/less01
cp -r localtmp/le01/les01/less01 localUPs/
compare_and_clear

#Test 06 #1 file folder
$PUT localtmp/lf01/lfs01/lfss01
cp -r localtmp/lf01/lfs01/lfss01 localUPs/
compare_and_clear

#Test 07 #entire empty folders structure
$PUT localtmp/le01
cp -r localtmp/le01 localUPs/
compare_and_clear

#Test 08 #entire non empty folders structure
$PUT localtmp/lf01
cp -r localtmp/lf01 localUPs/
compare_and_clear

#Test 09 #copy structure into subfolder
$PUT localtmp/le01 /01/s01
cp -r localtmp/le01 localUPs/01/s01
compare_and_clear

#~ #Test 10 #copy exact structure
mkdir aux
cp -pr localUPs/01 aux
$PUT aux/01/s01 /01/s01
cp -r aux/01/s01 localUPs/01/s01
rm -r aux
compare_and_clear

#Test 11 #merge increased structure
mkdir aux
cp -pr localUPs/01 aux
touch aux/01/s01/another.txt
$PUT aux/01/s01 /01/
rsync -aLp aux/01/s01/ localUPs/01/s01/
rm -r aux
compare_and_clear

#Test 12 #multiple upload
$PUT localtmp/le01 localtmp/lf01 /01/s01
cp -r localtmp/le01 localtmp/lf01 localUPs/01/s01
compare_and_clear

#Test 13 #local regexp
$PUT localtmp/*txt /01/s01
cp -r localtmp/*txt localUPs/01/s01
compare_and_clear

#Test 14 #../
$CD 01
$PUT localtmp/le01 ../01/s01
$CD /
cp -r localtmp/le01 localUPs/01/s01
compare_and_clear

currentTest=15

#Test 15 #spaced stuff
if [ "x$CMDSHELL" != "x1" ]; then #TODO: think about this again
$PUT localtmp/ls\ 01
else
$PUT "localtmp/ls\ 01"
fi

cp -r localtmp/ls\ 01 localUPs/ls\ 01
compare_and_clear


###TODO: do stuff in shared folders...

########

##Test XX #merge structure with file updated
##This test fails, because it creates a remote copy of the updated file.
##That's expected. If ever provided a way to create a real merge (e.g.: put -m ...)  to do BACKUPs, reuse this test.
#mkdir aux
#cp -pr localUPs/01 aux
#touch aux/01/s01/another.txt
#$PUT aux/01/s01 /01/
#rsync -aLp aux/01/s01/ localUPs/01/s01/
#echo "newcontents" > aux/01/s01/another.txt 
#$PUT aux/01/s01 /01/
#rsync -aLp aux/01/s01/ localUPs/01/s01/
#rm -r aux
#compare_and_clear


# Clean all
if [ "$VERBOSE" != "1" ]; then
clean_all
fi

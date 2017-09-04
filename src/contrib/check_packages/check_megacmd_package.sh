#!/bin/bash

##
 # @file examples/megacmd/contrib/check_packages/check_megacmd_package.sh
 # @brief checks the correctness of a package using a virtual machine
 #
 # (c) 2013-2016 by Mega Limited, Auckland, New Zealand
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

display_help() {
    local app=$(basename "$0")
    echo ""
    echo "Usage:"
    echo " $app [-c] [-i] [-k] [-p pass] VMNAME URL_REPO"
    echo ""
    echo "This script will check the correctness of a package using a virtual machine."
    echo " It sill receive the machine name and the repository that will be used to download" 
    echo " megacmd packages."
    echo ""
    echo "If the repo has a newly generated package, we can use -c to only validate a VM"
    echo " in case the version of megacmd package has changed. This will fail if the VM already"
    echo " contained the latests megacmd package" 
    echo " In case we dont know the state of the VM, we can safely run the test using -c and -i"
    echo ""
    echo "This script generates 2 files: "
    echo " - Either ${VMNAME}_OK (in case of success) or ${VMNAME}_FAIL (in case of failure)"
    echo " - result_${VMNAME}.log: this file sumarizes the problems encountered while testing."
    echo ""
    echo "Options:"
    echo " -c : check megacmd package has changed (updated or newly installed)"
    echo " -i : install anew (removes previous megacmd package)"
    echo " -k : keep VM running after completion"
    echo " -p pass : password for VM (both user mega & root)"
    echo ""
}

remove_megacmd=0
quit_machine=1
require_change=0

while getopts ":ikcp:" opt; do
  case $opt in
    i)
		remove_megacmd=1
      ;;
    n)
		nogpgchecksSUSE=--no-gpg-checks
		nogpgchecksYUM=--nogpgcheck
      ;;
	c)
		require_change=1
	;;
	p)
		arg_passwd="-p $OPTARG"
		sshpasscommand="sshpass $arg_passwd"
	;;	
    k)
		quit_machine=0
      ;;
    \?)
		echo "Invalid option: -$OPTARG" >&2
		display_help $0
		exit
	;;
	*)
		display_help $0
		exit
	;;
  esac
done

shift $(($OPTIND-1))

if [ "$#" -ne 2 ]; then
    echo "Illegal number of parameters"
    display_help $0
    exit
fi

VMNAME=$1
REPO=$2


echo "" > result_$VMNAME.log

logLastComandResult() {
	local RESULT=$?
	logOperationResult "$1" $RESULT
}

logOperationResult(){
	local RESULT=$2
	if [ $RESULT -eq 0 ]; then
		RESULT="OK"
	else
		RESULT="FAIL($RESULT)"
	fi
	
	printf "%-50s %s\n"  "$1" "$RESULT" >> result_$VMNAME.log
}

logSth(){
	printf "%-50s %s\n"  "$1" "$2" >> result_$VMNAME.log
}



echo " running machine $VMNAME ..."
sudo virsh create /etc/libvirt/qemu/$VMNAME.xml 

#sudo virsh domiflist $VMNAME 
IP_GUEST=$( sudo arp -n | grep `sudo virsh domiflist $VMNAME  | grep vnet | awk '{print $NF}'` | awk '{print $1}' )
while [ -z $IP_GUEST ]; do
echo " could not get guest IP. retrying in 2 sec ..."
sleep 2
IP_GUEST=$( sudo arp -n | grep `sudo virsh domiflist $VMNAME  | grep vnet | awk '{print $NF}'` | awk '{print $1}' )
done
echo " obtained IP guest = $IP_GUEST"

echo " sshing to save host into the known hosts ...."
#sshpass would do that automatically neither ask for it, we need to force it the first time
while ! $sshpasscommand ssh  -oStrictHostKeyChecking=no root@$IP_GUEST hostname ; do 
echo " could ssh into GUEST. retrying in 2 sec ..."
sleep 2
IP_GUEST=$( sudo arp -n | grep `sudo virsh domiflist $VMNAME  | grep vnet | awk '{print $NF}'` | awk '{print $1}' )
while [ -z $IP_GUEST ]; do
echo " could not get new guest IP. retrying in 2 sec ..."
sleep 2
IP_GUEST=$( sudo arp -n | grep `sudo virsh domiflist $VMNAME  | grep vnet | awk '{print $NF}'` | awk '{print $1}' )
done
echo " obtained new IP guest = $IP_GUEST"
done
 
echo "quering VM info:"
$sshpasscommand ssh root@$IP_GUEST cat /etc/issue
$sshpasscommand ssh root@$IP_GUEST uname -a
echo -n " Inotify max watchers initial: "
$sshpasscommand ssh root@$IP_GUEST sysctl fs.inotify.max_user_watches


echo " deleting testing file ..."
$sshpasscommand ssh root@$IP_GUEST rm /home/mega/testFile.txt #Notice: filename comes from the shared file
echo " checking existing mega-cmd running ..."
$sshpasscommand ssh root@$IP_GUEST ps aux | grep megacmd
echo " killing mega-cmd ..."
$sshpasscommand ssh root@$IP_GUEST killall mega-cmd


#DEPENDENT ON SYSTEM
if [ $VMNAME == "OPENSUSE_12.3" ]; then
	#this particular VM had a problem with missing DNS servers.
	$sshpasscommand ssh root@$IP_GUEST netconfig update -f #TODO: take this out and configure the machine accordingly
fi

if [[ $VMNAME == *"OPENSUSE"* ]]; then
	#stop packagekit service to prevent zypper from freezing
	$sshpasscommand ssh root@$IP_GUEST service packagekit stop
	sleep 1

	$sshpasscommand ssh root@$IP_GUEST rpm --rebuilddb #TODO: test without this in opensuses > 12.2

	if [ $remove_megacmd -eq 1 ]; then
		echo " removing megacmd ..."
		attempts=10
		$sshpasscommand ssh root@$IP_GUEST zypper --non-interactive remove megacmd
		resultREMOVE=$?
		while [[ $attempts -ge 0 && $resultREMOVE -ne 0 ]]; do
			sleep $((5*(10-$attempts)))
			echo " removing megacmd ... attempts left="$attempts
			$sshpasscommand ssh root@$IP_GUEST zypper --non-interactive remove megacmd
			resultREMOVE=$?
			attempts=$(($attempts - 1))
		done
		logLastComandResult "removing megacmd ..." $resultREMOVE
	fi
	
	echo " modifying repos ..."
	$sshpasscommand ssh root@$IP_GUEST "cat > /etc/zypp/repos.d/megasync.repo" <<-EOF
	[MEGAsync]
	name=MEGAsync
	type=rpm-md
	baseurl=$REPO/
	gpgkey=$REPO/repodata/repomd.xml.key
	gpgcheck=1
	autorefresh=1
	enabled=1
	EOF
	$sshpasscommand ssh root@$IP_GUEST zypper --non-interactive $nogpgchecksSUSE refresh 2> tmp$VMNAME
	resultMODREPO=$?
	#notice: zypper will report 0 as status even though it "failed", we do stderr checking
	if cat tmp$VMNAME | grep $REPO; then
	 resultMODREPO=$(expr 1000 + 0$resultMODREPO); cat tmp$VMNAME; 
	else
	 resultMODREPO=0 #we discard any other failure
	fi; rm tmp$VMNAME;	
	# if [ -s tmp$VMNAME ]; then resultMODREPO=$(expr 1000 + $resultMODREPO); cat tmp$VMNAME; fi; rm tmp$VMNAME;
	logOperationResult "modifying repos ..." $resultMODREPO
	$sshpasscommand ssh root@$IP_GUEST cat /etc/zypp/repos.d/megasync.repo
	
	echo " reinstalling/updating megacmd ..."
	BEFOREINSTALL=`$sshpasscommand ssh root@$IP_GUEST rpm -q megacmd`
	attempts=10
	$sshpasscommand ssh root@$IP_GUEST zypper --non-interactive $nogpgchecksSUSE install -f megacmd 
	resultINSTALL=$?
	while [[ $attempts -ge 0 && $resultINSTALL -ne 0 ]]; do
		sleep $((5*(10-$attempts)))
		echo " reinstalling/updating megacmd ... attempts left="$attempts
		$sshpasscommand ssh root@$IP_GUEST zypper --non-interactive $nogpgchecksSUSE install -f megacmd 
		resultINSTALL=$?
		attempts=$(($attempts - 1))
	done
	#TODO: zypper might fail and still say "IT IS OK!"
	#Doing stderr checking will give false FAILS, since zypper outputs non failure stuff in stderr
	#if [ -s tmp$VMNAME ]; then resultINSTALL=$(expr 1000 + $resultINSTALL); cat tmp$VMNAME; fi; rm tmp$VMNAME;	
	AFTERINSTALL=`$sshpasscommand ssh root@$IP_GUEST rpm -q megacmd`
	resultINSTALL=$(expr $? + 0$resultINSTALL)
	echo "BEFOREINSTALL = $BEFOREINSTALL"
	echo "AFTERINSTALL = $AFTERINSTALL"
	if [ $require_change -eq 1 ]; then
		if [ "$BEFOREINSTALL" == "$AFTERINSTALL" ]; then resultINSTALL=$(expr 1000 + 0$resultINSTALL); fi
	fi
	logOperationResult "reinstalling/updating megacmd ..." $resultINSTALL
	VERSIONINSTALLEDAFTER=`echo $AFTERINSTALL	| grep megacmd | awk '{for(i=1;i<=NF;i++){ if(match($i,/[0-9].[0-9].[0-9]/)){print $i} } }'`
	logSth "installed megacmd ..." "$VERSIONINSTALLEDAFTER"
		
elif [[ $VMNAME == *"DEBIAN"* ]] || [[ $VMNAME == *"UBUNTU"* ]] || [[ $VMNAME == *"LINUXMINT"* ]]; then

	if [ $remove_megacmd -eq 1 ]; then
		echo " removing megacmd ..."
		attempts=10
		$sshpasscommand ssh root@$IP_GUEST DEBIAN_FRONTEND=noninteractive apt-get -y remove megacmd
		resultREMOVE=$?
		while [[ $attempts -ge 0 && $resultREMOVE -ne 0 && $resultREMOVE -ne 100 ]]; do
			sleep $((5*(10-$attempts)))
			echo " removing megacmd ... attempts left="$attempts
			$sshpasscommand ssh root@$IP_GUEST DEBIAN_FRONTEND=noninteractive apt-get -y remove megacmd
			resultREMOVE=$?
			attempts=$(($attempts - 1))
		done
		logOperationResult "removing megacmd ..." $resultREMOVE
	fi
	sleep 1

	echo " modifying repos ..."
	$sshpasscommand ssh root@$IP_GUEST "cat > /etc/apt/sources.list.d/megasync.list" <<-EOF
	deb $REPO/ ./
	EOF
	
	$sshpasscommand ssh root@$IP_GUEST DEBIAN_FRONTEND=noninteractive apt-get -y update 2> tmp$VMNAME
	resultMODREPO=$?
	attempts=10
	while [[ $attempts -ge 0 && $resultMODREPO -ne 0 ]]; do
		sleep $((5*(10-$attempts)))
		echo " modifing repos ... attempts left="$attempts
		$sshpasscommand ssh root@$IP_GUEST DEBIAN_FRONTEND=noninteractive apt-get -y update 2> tmp$VMNAME
		resultMODREPO=$?
		attempts=$(($attempts - 1))
	done
	
	#notice: zypper will report 0 as status even though it "failed", we do stderr checking
	if cat tmp$VMNAME | grep $REPO; then
	 resultMODREPO=$(expr 1000 + 0$resultMODREPO); cat tmp$VMNAME; 
	#else
	 #resultMODREPO=0 #we discard any other failure
	fi; rm tmp$VMNAME;	
	logOperationResult "modifying repos ..." $resultMODREPO
	$sshpasscommand ssh root@$IP_GUEST "cat /etc/apt/sources.list.d/megasync.list"

	echo " reinstalling/updating megacmd ..."
	BEFOREINSTALL=`$sshpasscommand ssh root@$IP_GUEST dpkg -l megacmd`
	$sshpasscommand ssh root@$IP_GUEST DEBIAN_FRONTEND=noninteractive apt-get -y install megacmd
	resultINSTALL=$?
	AFTERINSTALL=`$sshpasscommand ssh root@$IP_GUEST dpkg -l megacmd`
	resultINSTALL=$(expr $? + 0$resultINSTALL)
	echo "BEFOREINSTALL = $BEFOREINSTALL"
	echo "AFTERINSTALL = $AFTERINSTALL"
	if [ $require_change -eq 1 ]; then
		if [ "$BEFOREINSTALL" == "$AFTERINSTALL" ]; then resultINSTALL=$(expr 1000 + 0$resultINSTALL); fi
	fi
	attempts=10
	while [[ $attempts -ge 0 && $resultINSTALL -ne 0 ]]; do
		sleep $((5*(10-$attempts)))
		echo " reinstalling/updating megacmd ... attempts left="$attempts
		BEFOREINSTALL=`$sshpasscommand ssh root@$IP_GUEST dpkg -l megacmd`
		$sshpasscommand ssh root@$IP_GUEST DEBIAN_FRONTEND=noninteractive apt-get -y install megacmd
		resultINSTALL=$?
		AFTERINSTALL=`$sshpasscommand ssh root@$IP_GUEST dpkg -l megacmd`
		resultINSTALL=$(expr $? + 0$resultINSTALL)
		echo "BEFOREINSTALL = $BEFOREINSTALL"
		echo "AFTERINSTALL = $AFTERINSTALL"
		if [ $require_change -eq 1 ]; then
			if [ "$BEFOREINSTALL" == "$AFTERINSTALL" ]; then resultINSTALL=$(expr 1000 + 0$resultINSTALL); fi
		fi
		attempts=$(($attempts - 1))
	done
	logOperationResult "reinstalling/updating megacmd ..." $resultINSTALL	
	VERSIONINSTALLEDAFTER=`echo $AFTERINSTALL	| grep megacmd | awk '{for(i=1;i<=NF;i++){ if(match($i,/[0-9].[0-9].[0-9]/)){print $i} } }'`
	logSth "installed megacmd ..." "$VERSIONINSTALLEDAFTER"

elif [[ $VMNAME == *"ARCHLINUX"* ]]; then

	if [ $remove_megacmd -eq 1 ]; then
		echo " removing megacmd ..."
		$sshpasscommand ssh root@$IP_GUEST pacman -Rc --noconfirm megacmd
		logLastComandResult "removing megacmd ..."
	fi
	sleep 1

	echo " modifying repos ..."
	
	#remove repo if existing
	$sshpasscommand ssh root@$IP_GUEST sed -n "'1h;1!H;\${g;s/###REPO for MEGA###\n.*###END REPO for MEGA###//;p;}'" -i /etc/pacman.conf 

	#include repo
	$sshpasscommand ssh root@$IP_GUEST "cat >> /etc/pacman.conf" <<-EOF
###REPO for MEGA###
[DEB_Arch_Extra]
SigLevel = Optional TrustAll
Server = $REPO/\$arch
###END REPO for MEGA###
EOF
	
	$sshpasscommand ssh root@$IP_GUEST pacman -Syy --noconfirm 2> tmp$VMNAME
	resultMODREPO=$?
	#notice: in case pacman reports 0 as status even though it "failed", we do stderr checking
	if cat tmp$VMNAME | grep "$REPO\|error"; then
	 resultMODREPO=$(expr 1000 + 0$resultMODREPO); cat tmp$VMNAME; 
	#else
	 #resultMODREPO=0 #we discard any other failure
	fi; rm tmp$VMNAME;	
	logOperationResult "modifying repos ..." $resultMODREPO
	$sshpasscommand ssh root@$IP_GUEST "cat /etc/pacman.conf | grep 'REPO for MEGA' -A 5"
	
	
	echo " reinstalling/updating megacmd ..."
	BEFOREINSTALL=`$sshpasscommand ssh root@$IP_GUEST pacman -Q megacmd`
	$sshpasscommand ssh root@$IP_GUEST pacman -S --noconfirm megacmd
	resultINSTALL=$?
	
	AFTERINSTALL=`$sshpasscommand ssh root@$IP_GUEST pacman -Q megacmd`
	resultINSTALL=$(expr $? + 0$resultINSTALL)
	echo "BEFOREINSTALL = $BEFOREINSTALL"
	echo "AFTERINSTALL = $AFTERINSTALL"
	if [ $require_change -eq 1 ]; then
		if [ "$BEFOREINSTALL" == "$AFTERINSTALL" ]; then resultINSTALL=$(expr 1000 + 0$resultINSTALL); fi
	fi
	
	attempts=10
	while [[ $attempts -ge 0 && $resultINSTALL -ne 0 ]]; do
		sleep $((5*(10-$attempts)))
		echo " reinstalling/updating megacmd ... attempts left="$attempts
		BEFOREINSTALL=`$sshpasscommand ssh root@$IP_GUEST pacman -Q megacmd`
		$sshpasscommand ssh root@$IP_GUEST pacman -S --noconfirm megacmd
		resultINSTALL=$?
		
		AFTERINSTALL=`$sshpasscommand ssh root@$IP_GUEST pacman -Q megacmd`
		resultINSTALL=$(expr $? + 0$resultINSTALL)
		echo "BEFOREINSTALL = $BEFOREINSTALL"
		echo "AFTERINSTALL = $AFTERINSTALL"
		if [ $require_change -eq 1 ]; then
			if [ "$BEFOREINSTALL" == "$AFTERINSTALL" ]; then resultINSTALL=$(expr 1000 + 0$resultINSTALL); fi
		fi
		attempts=$(($attempts - 1))
	done
	
	logOperationResult "reinstalling/updating megacmd ..." $resultINSTALL
	VERSIONINSTALLEDAFTER=`echo $AFTERINSTALL	| grep megacmd | awk '{for(i=1;i<=NF;i++){ if(match($i,/[0-9].[0-9].[0-9]/)){print $i} } }'`
	logSth "installed megacmd ..." "$VERSIONINSTALLEDAFTER"


else
	#stop packagekit service to prevent yum from freezing
		if [[ $VMNAME == *"FEDORA"* ]]; then
		$sshpasscommand ssh root@$IP_GUEST service packagekit stop
	fi
	sleep 1
	
	YUM=yum
	if $sshpasscommand ssh root@$IP_GUEST which dnf; then
		YUM="dnf --best"
	fi

	if [ $remove_megacmd -eq 1 ]; then
		echo " removing megacmd ..."
		attempts=10
		$sshpasscommand ssh root@$IP_GUEST $YUM -y --disableplugin=refresh-packagekit remove megacmd
		resultREMOVE=$?
		while [[ $attempts -ge 0 && $resultREMOVE -ne 0 && $resultREMOVE -ne 1 ]]; do
			sleep $((5*(10-$attempts)))
			echo " removing megacmd ... attempts left="$attempts
		$sshpasscommand ssh root@$IP_GUEST $YUM -y --disableplugin=refresh-packagekit remove megacmd
			resultREMOVE=$?
			attempts=$(($attempts - 1))
		done
		logOperationResult "removing megacmd ..." $resultREMOVE
	fi
	sleep 1
	

	echo " modifying repos ..."
	$sshpasscommand ssh root@$IP_GUEST "cat > /etc/yum.repos.d/megasync.repo" <<-EOF
	[MEGAsync]
	name=MEGAsync
	baseurl=$REPO/
	gpgkey=$REPO/repodata/repomd.xml.key
	gpgcheck=1
	enabled=1
	EOF
	$sshpasscommand ssh root@$IP_GUEST $YUM -y --disableplugin=refresh-packagekit clean all 2> tmp$VMNAME
	resultMODREPO=$?
	#fix inconclusive problem in Fedora24:
	cat tmp$VMNAME | grep -v "FutureWarning: split() requires a non-empty pattern match\|return _compile(pattern, flags).split(stri" > tmp$VMNAME
	if [ -s tmp$VMNAME ]; then resultMODREPO=$(expr 1000 + $resultMODREPO); cat tmp$VMNAME; fi; rm tmp$VMNAME; 
	logOperationResult "modifying repos ..." $resultMODREPO
	$sshpasscommand ssh root@$IP_GUEST "cat /etc/yum.repos.d/megasync.repo"
	sleep 1
	
	
	echo " reinstalling/updating megacmd ..."
	BEFOREINSTALL=`$sshpasscommand ssh root@$IP_GUEST rpm -q megacmd`
	$sshpasscommand ssh root@$IP_GUEST $YUM -y --disableplugin=refresh-packagekit $nogpgchecksYUM install megacmd  2> tmp$VMNAME
	resultINSTALL=$? #TODO: yum might fail and still say "IT IS OK!"
	#Doing simple stderr checking will give false FAILS, since yum outputs non failure stuff in stderr
	if cat tmp$VMNAME | grep $REPO; then
	 resultINSTALL=$(expr 1000 + 0$resultINSTALL); cat tmp$VMNAME; 
	fi; rm tmp$VMNAME;	
	
	AFTERINSTALL=`$sshpasscommand ssh root@$IP_GUEST rpm -q megacmd`
	resultINSTALL=$(expr $? + 0$resultINSTALL)
	echo "BEFOREINSTALL = $BEFOREINSTALL"
	echo "AFTERINSTALL = $AFTERINSTALL"
	if [ $require_change -eq 1 ]; then
		if [ "$BEFOREINSTALL" == "$AFTERINSTALL" ]; then resultINSTALL=$(expr 1000 + 0$resultINSTALL); fi
	fi
	
	attempts=10
	while [[ $attempts -ge 0 && $resultINSTALL -ne 0 ]]; do
		sleep $((5*(10-$attempts)))
		echo " reinstalling/updating megacmd ... attempts left="$attempts
		BEFOREINSTALL=`$sshpasscommand ssh root@$IP_GUEST rpm -q megacmd`
		$sshpasscommand ssh root@$IP_GUEST $YUM -y --disableplugin=refresh-packagekit $nogpgchecksYUM install megacmd  2> tmp$VMNAME
		resultINSTALL=$(($? + 0$resultINSTALL)) #TODO: yum might fail and still say "IT IS OK!"
		#Doing simple stderr checking will give false FAILS, since yum outputs non failure stuff in stderr
		if cat tmp$VMNAME | grep $REPO; then
		 resultINSTALL=$(expr 1000 + 0$resultINSTALL); cat tmp$VMNAME; 
		fi; rm tmp$VMNAME;	
		
		AFTERINSTALL=`$sshpasscommand ssh root@$IP_GUEST rpm -q megacmd`
		resultINSTALL=$(expr $? + 0$resultINSTALL)
		echo "BEFOREINSTALL = $BEFOREINSTALL"
		echo "AFTERINSTALL = $AFTERINSTALL"
		if [ $require_change -eq 1 ]; then
			if [ "$BEFOREINSTALL" == "$AFTERINSTALL" ]; then resultINSTALL=$(expr 1000 + 0$resultINSTALL); fi
		fi
		attempts=$(($attempts - 1))
	done
	
	logOperationResult "reinstalling/updating megacmd ..." $resultINSTALL
	VERSIONINSTALLEDAFTER=`echo $AFTERINSTALL	| grep megacmd | awk '{for(i=1;i<=NF;i++){ if(match($i,/[0-9].[0-9].[0-9]/)){print $i} } }'`
	logSth "installed megacmd ..." "$VERSIONINSTALLEDAFTER"
fi
#END OF DEPENDENT OF SYSTEM

theDisplay="DISPLAY=:0.0"

echo " relaunching megacmd as user ..."

$sshpasscommand ssh -oStrictHostKeyChecking=no  mega@$IP_GUEST $theDisplay mega-cmd &

sleep 5 #TODO: sleep longer?

echo " checking new megacmd running ..."
$sshpasscommand ssh root@$IP_GUEST ps aux | grep mega-cmd
resultRunning=$?
logOperationResult "checking new megacmd running ..." $resultRunning

VERSIONMEGACMDREMOTERUNNING=`$sshpasscommand ssh -oStrictHostKeyChecking=no  mega@$IP_GUEST $theDisplay mega-version`
logSth "running megacmd ..." "$VERSIONMEGACMDREMOTERUNNING"

echo " forcing dl test file ..."
$sshpasscommand ssh mega@$IP_GUEST "mega-get 'https://mega.nz/#!FQ5miCCB!WkMOvzgPWhBtvE7tYQQv8urhwuYmuS74C3HnhboDE-I' /home/mega/ -vvv" 
resultGET=$?
logOperationResult "mega-get worked ..." $resultGET

$sshpasscommand ssh root@$IP_GUEST cat /home/mega/testFile.txt >/dev/null  #TODO: do hash file comparation
resultDL=$?

logOperationResult "check file dl correctly ..." $resultDL



echo " checking repo set ok ..."
$sshpasscommand ssh root@$IP_GUEST  "cat /usr/share/doc/megasync/{distro,version}"

## CHECKING REPO SET OK ##
if [[ $VMNAME == *"OPENSUSE"* ]]; then
	distroDir="openSUSE"
	ver=$($sshpasscommand ssh root@$IP_GUEST lsb_release -rs)
	if [ x$ver == "x20160920" ]; then ver="Tumbleweed"; fi
	if [[ x$ver == "x42"* ]]; then ver="Leap_$ver"; fi
	expected="baseurl=https://mega.nz/linux/MEGAsync/${distroDir}_$ver"
	resultRepoConfiguredOk=0
	if ! $sshpasscommand ssh root@$IP_GUEST cat /etc/zypp/repos.d/megasync.repo | grep "$expected" > /dev/null; then
		echo "WRONG repo configured. Read: <$($sshpasscommand ssh root@$IP_GUEST "cat /etc/zypp/repos.d/megasync.repo" | grep baseurl)>"
		echo "Expected: <$expected>"
		resultRepoConfiguredOk=1
	fi
	logOperationResult "check repo configured correctly ..." $resultRepoConfiguredOk

elif [[ $VMNAME == *"DEBIAN"* ]] || [[ $VMNAME == *"UBUNTU"* ]] || [[ $VMNAME == *"LINUXMINT"* ]]; then
	distro=$($sshpasscommand ssh root@$IP_GUEST lsb_release -ds)
	distroDir=$distro
	if [[ $distroDir == "Ubuntu"* ]]; then distroDir="xUbuntu"; fi
	ver=$($sshpasscommand ssh root@$IP_GUEST lsb_release -rs)
	if [[ $distroDir == "Debian"* ]]; then 
		distroDir="Debian" 
		if [[ x$ver == "x8"* ]]; then ver="8.0"; fi
		if [[ x$ver == "x7"* ]]; then ver="7.0"; fi
		if [[ x$ver == "x9"* ]]; then ver="9.0"; fi
		if [[ x$ver == "xtesting"* ]]; then ver="9.0"; fi
	fi
	
	if [[ $distroDir == "Linux Mint 17"* ]]; then distroDir="xUbuntu"; ver="14.04"; fi
	if [[ $distroDir == "Linux Mint 18"* ]]; then distroDir="xUbuntu"; ver="16.04"; fi
	
	resultRepoConfiguredOk=0
	expected="deb https://mega.nz/linux/MEGAsync/${distroDir}_$ver/ ./"
	if [ "x$expected" != "x$($sshpasscommand ssh root@$IP_GUEST "cat /etc/apt/sources.list.d/megasync.list")" ]; then
		echo "WRONG repo configured. Read: <$($sshpasscommand ssh root@$IP_GUEST "cat /etc/apt/sources.list.d/megasync.list")>"
		echo "Expected: <$expected>"
		resultRepoConfiguredOk=1
	fi
	
	logOperationResult "check repo configured correctly ..." $resultRepoConfiguredOk

elif [[ $VMNAME == *"ARCHLINUX"* ]]; then
	resultRepoConfiguredOk=0
	distroDir="Arch_Extra"
	ver=""
	expected="^Server = https://mega.nz/linux/MEGAsync/Arch_Extra/\$arch"
	
	resultRepoConfiguredOk=0
	if ! $sshpasscommand ssh root@$IP_GUEST cat /etc/pacman.conf | grep "$expected" > /dev/null; then
		echo "WRONG repo configured. Read: <$($sshpasscommand ssh root@$IP_GUEST "cat /etc/pacman.conf" | grep baseurl)>"
		echo "Expected: <$expected>"
		resultRepoConfiguredOk=1
	fi
	logOperationResult "check repo configured correctly ..." $resultRepoConfiguredOk
else #FEDORA | CENTOS...
	distroDir=$($sshpasscommand ssh root@$IP_GUEST cat /etc/system-release | awk '{print $1}')
	if [[ $distroDir == "Scientific"* ]]; then distroDir="ScientificLinux"; fi
	ver=$($sshpasscommand ssh root@$IP_GUEST cat /etc/system-release | awk -F"release "  '{print $2}' | awk '{print $1}')
	if [[ x$ver == "x7"* ]]; then ver="7"; fi #centos7

	expected="baseurl=https://mega.nz/linux/MEGAsync/${distroDir}_$ver"
	resultRepoConfiguredOk=0
	if ! $sshpasscommand ssh root@$IP_GUEST cat /etc/yum.repos.d/megasync.repo | grep "$expected" > /dev/null; then
		echo "WRONG repo configured. Read: <$($sshpasscommand ssh root@$IP_GUEST "cat /etc/yum.repos.d/megasync.repo" | grep baseurl)>"
		echo "Expected: <$expected>"
		resultRepoConfiguredOk=1
	fi
	logOperationResult "check repo configured correctly ..." $resultRepoConfiguredOk
fi

echo -n " Inotify max watchers final: "
$sshpasscommand ssh root@$IP_GUEST sysctl fs.inotify.max_user_watches


if [ $resultDL -eq 0 ] && [ $resultGET -eq 0 ] && [ $resultRunning -eq 0 ] \
&& [ $resultMODREPO -eq 0 ] && [ $resultINSTALL -eq 0 ] \
&& [ $resultRepoConfiguredOk -eq 0 ]; then
	echo " megacmd working smoothly" 
	touch ${VMNAME}_OK
else
	echo "MEGACMD FAILED: $resultDL $resultRunning $resultMODREPO $resultINSTALL $resultRepoConfiguredOk"
	#cat result_$VMNAME.log
	touch ${VMNAME}_FAIL
fi


	
if [ $quit_machine -eq 1 ]; then
	$sshpasscommand ssh root@$IP_GUEST shutdown -h now & #unfurtonately this might (though rarely) hang if vm destroyed
	#$sshpasscommand ssh root@$IP_GUEST sleep 20 #unfurtonately this might hang if vm destroyed
	sleep 8
	sudo virsh destroy $VMNAME
fi

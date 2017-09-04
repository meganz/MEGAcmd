#!/bin/bash

##
 # @file examples/megacmd/contrib/check_packages/check_megacmd_package.sh
 # @brief checks the correctness of all packages using virtual machines
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
    echo " $app [-c] [-i] [-k] [-p pass] BASEREPOURL"
    echo ""
    echo "This script will check the correctness of all packages using virtual machines."
    echo "It will choose the specific repo corresponding to all the configured VM, using"
    echo " BASEREPOURL passed as argument. e.g: $app https://mega.nz/linux/MEGAsync/"
    echo ""
    echo "If all repos have a newly generated package, we can use -c to only validate a VM"
    echo " in case the version of megasync package has changed. This will fail if the VM already"
    echo " contained the latests megasync package"
    echo " In case we dont know the state of the VM, we can safely run the test using -c and -i"
    echo ""
    echo "This script generates 3 files per VM: "
    echo " - Either ${VMNAME}_OK (in case of success) or ${VMNAME}_FAIL (in case of failure)"
    echo " - result_${VMNAME}.log: this file sumarizes the problems encountered while testing."
    echo " - output_check_megacmd_package_${VMNAME}.log: stdout of check_megacmd_package.sh"
    echo ""
    echo "Options:"
    echo " -c : check megasync packages have changed (updated or newly installed)"
    echo " -i : install anew (removes previous megasync package)"
    echo " -k : keep VMs running after completion"
    echo " -p pass : password for VMs (both user mega & root)"
    echo ""
}


remove_megasync=0
quit_machine=1


while getopts ":ikcnp:" opt; do
  case $opt in
    i)
		remove_megasync=1
		flag_remove_megasync="-$opt"
      ;;
	n)
		flag_nogpgcheck="-$opt"
	;;
	c)
		flag_require_change="-$opt"
	;;	
	p)
		arg_passwd="-$opt $OPTARG"
	;;     		
    k)
		quit_machine=0
		flag_quit_machine="-$opt"
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


echo -n " Repository packages checking beginning at "
sudo date

# notice: use http://linux.deve.... instead of https://linu...
BASEURL=$1
BASEURLDEB=$BASEURL
BASEURLRPM=$BASEURL
BASEURLARCH=$BASEURL
if [ -z $BASEURL ]; then
 BASEURL=http://192.168.122.1:8000
 BASEURLDEB=$BASEURL/DEB
 BASEURLRPM=$BASEURL/RPM
 BASEURLARCH=$BASEURL/DEB  #they belong to the same OBS project named 'DEB'
fi

PAIRSVMNAMEREPOURL=""
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL ARCHLINUX;$BASEURLARCH/Arch_Extra"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL CENTOS_7;$BASEURLRPM/CentOS_7"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL SCIENTIFICLINUX_7;$BASEURLRPM/ScientificLinux_7"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL FEDORA_19;$BASEURLRPM/Fedora_19"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL FEDORA_20;$BASEURLRPM/Fedora_20"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL FEDORA_21;$BASEURLRPM/Fedora_21"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL FEDORA_22;$BASEURLRPM/Fedora_22"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL FEDORA_23;$BASEURLRPM/Fedora_23"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL FEDORA_24;$BASEURLRPM/Fedora_24"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL FEDORA_25;$BASEURLRPM/Fedora_25"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL FEDORA_25_i386;$BASEURLRPM/Fedora_25"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL FEDORA_26;$BASEURLARCH/Fedora_26"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL FEDORA_26_i386;$BASEURLARCH/Fedora_26"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL OPENSUSE_12.2;$BASEURLRPM/openSUSE_12.2"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL OPENSUSE_12.3;$BASEURLRPM/openSUSE_12.3"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL OPENSUSE_13.2;$BASEURLRPM/openSUSE_13.2"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL OPENSUSE_LEAP_42.1;$BASEURLRPM/openSUSE_Leap_42.1"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL OPENSUSE_LEAP_42.2;$BASEURLRPM/openSUSE_Leap_42.2"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL OPENSUSE_LEAP_42.3;$BASEURLRPM/openSUSE_Leap_42.3"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL OPENSUSE_TUMBLEWEED;$BASEURLRPM/openSUSE_Tumbleweed"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL UBUNTU_12.04;$BASEURLDEB/xUbuntu_12.04"	
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL UBUNTU_13.10;$BASEURLDEB/xUbuntu_13.10"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL UBUNTU_14.04;$BASEURLDEB/xUbuntu_14.04"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL UBUNTU_15.04;$BASEURLDEB/xUbuntu_15.04"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL UBUNTU_15.10;$BASEURLDEB/xUbuntu_15.10"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL UBUNTU_16.04;$BASEURLDEB/xUbuntu_16.04"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL UBUNTU_16.04_i386;$BASEURLDEB/xUbuntu_16.04"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL UBUNTU_16.10;$BASEURLDEB/xUbuntu_16.10"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL UBUNTU_17.04;$BASEURLDEB/xUbuntu_17.04"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL LINUXMINT_17.3;$BASEURLDEB/xUbuntu_14.04"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL LINUXMINT_18;$BASEURLDEB/xUbuntu_16.04"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL DEBIAN_7.8.0;$BASEURLDEB/Debian_7.0"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL DEBIAN_8;$BASEURLDEB/Debian_8.0"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL DEBIAN_8.6;$BASEURLDEB/Debian_8.0"
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL DEBIAN_9_CLEAN_testing;$BASEURLDEB/Debian_9.0" #NOTICE: using other repo
PAIRSVMNAMEREPOURL="$PAIRSVMNAMEREPOURL DEBIAN_9_i386_T;$BASEURLDEB/Debian_9.0" #NOTICE: using other repo

#existing,but failing VMs
##Opensuse 13.1 -> no network interface!!
##Ubuntu 12.10 -> cannot install curl (support ended, official repos fail)
##Debian6 -> cannot install curl (support ended feb 2016, official repos fail)

#Untested but offered repos 
#~ RHEL_7/ #NO VM
#~ xUbuntu_12.10/ #VM FAILS
#~ xUbuntu_13.04/ #NO VM
#~ xUbuntu_14.10/ #NO VM
# ALL i386 (but Debian9)


count=0
for i in `shuf -e $PAIRSVMNAMEREPOURL`; do

    RUNNING=`ps aux | grep check_megacmd_package.sh | grep -v check_all_ | grep -v grep | wc -l`
	echo "$RUNNING processes running..."
    
    while [ $RUNNING -ge 7 ]; do
	 echo "$RUNNING processes running. Waiting for one to finish ..."
	 sleep 5;
	 RUNNING=`ps aux | grep check_megacmd_package.sh | grep -v check_all_ | grep -v grep | wc -l`
	done
    

	VMNAME=`echo $i | cut -d";" -f1`;
	REPO=`echo $i | cut -d";" -f2`;
	
	DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
	
	rm ${VMNAME}_{OK,FAIL} 2> /dev/null
	echo $DIR/check_megacmd_package.sh $arg_passwd $flag_require_change $flag_remove_megasync $flag_quit_machine $flag_nogpgcheck $VMNAME $REPO 
	( $DIR/check_megacmd_package.sh $arg_passwd $flag_require_change $flag_remove_megasync $flag_quit_machine $flag_nogpgcheck $VMNAME $REPO 2>&1 ) > output_check_megacmd_package_${VMNAME}.log &
	
	sleep 1
	
	
done

#!/bin/bash

##
 # @file build/getlatestmegacmdversionAndTriggerBuildAllRepos.sh
 # @brief Gets the project cloning git project and creates tarball, then
 #     Triggers OBS compilation for configured repositories          
 #     It stores the project files at                                
 #         $DATAFOLDER/building/local_project/megacmd_$THEDATE   
 #     and the stuff for repos building at                           
 #         $DATAFOLDER/building/osc_projects/megacmd/$THEDATE    
 #
 # (c) 2013-2016 by Mega Limited, Auckland, New Zealand
 #
 # This file is part of the MEGAcmd.
 #
 # MEGAcmd is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 #
 # @copyright Simplified (2-clause) BSD License.
 #
 # You should have received a copy of the license along with this
 # program.
##


function printusage {
	echo "$0 [--home] [user@remoteobsserver] [-t branch] [PROJECTPATH [OSCFOLDER]]"
	echo "This scripts gets latest version of the git project and creates tarball, then triggers OBS compilation for configured repositories"
	echo " An alternative branch/commit can be specified with -t BRANCH "
	echo " Use --home to only update home:Admin project. Otherwise DEB and RPM projects will be updated" 
}

if [[ $1 == "--help" ]]; then
	printusage
	exit 1
fi


if [[ $1 == "--home" ]]; then
	onlyhomeproject=$1;
	shift
fi

sshpasscommand=""
if [[ $1 == *@* ]]; then
remote=$1
shift
echo -n $remote Password: 
read -s password
sshpasscommand="sshpass -p $password ssh $remote"
echo
fi

while getopts ":t:" opt; do
  case $opt in
    t)
		tagtodl="--branch $OPTARG"
      ;;
  esac
done

shift $(($OPTIND-1))

THEDATE=`date +%Y%m%d%H%M%S`
PROJECT_PATH=$1
NEWOSCFOLDER_PATH=$2
if [ -z "$PROJECT_PATH" ]; then
	PROJECT_PATH=$DATAFOLDER/building/local_project/megacmd_$THEDATE
	echo "using default PROJECT_PATH: $PROJECT_PATH"
fi


function copy
{
	if [ "$sshpasscommand" ]; then
		sshpass -p $password scp $1 $remote:$2
	else #linking will be enough
		ln -sf $1 $2
	fi
}

#checkout master project and submodules
echo git clone $tagtodl --depth 1 --recursive https://github.com/meganz/megacmd $PROJECT_PATH
if ! git clone $tagtodl --depth 1 --recursive https://github.com/meganz/megacmd $PROJECT_PATH; then exit 1;fi
pushd $PROJECT_PATH
pushd build

./create_tarball.sh
ver=`cat version`
copy ./megacmd/debian.changelog /assets/changelogs/megacmd/megacmd_${ver}.changelog

popd
popd

# trigger build commiting new changes into OBS projects
if [ -z "$NEWOSCFOLDER_PATH" ]; then
	NEWOSCFOLDER_PATH=$DATAFOLDER/building/osc_projects/megacmd/$THEDATE
	echo "using default NEWOSCFOLDER_PATH: $NEWOSCFOLDER_PATH"
fi


DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REMOTEP=$password $DIR/triggermegacmdBuild.sh $onlyhomeproject $remote $PROJECT_PATH $NEWOSCFOLDER_PATH

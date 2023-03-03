#!/usr/bin/python2.7
# -*- coding: utf-8 -*-

import sys, os, subprocess, shutil
from megacmd_tests_common import *

GET="mega-get"
PUT="mega-put"
RM="mega-rm"
CD="mega-cd"
LCD="mega-lcd"
MKDIR="mega-mkdir"
EXPORT="mega-export -f"
FIND="mega-find"
WHOAMI="mega-whoami"
LOGOUT="mega-logout"
LOGIN="mega-login"
ABSPWD=os.getcwd()
currentTest=1


try:
    os.environ['VERBOSE']
    VERBOSE=True
except:
    VERBOSE=False

#VERBOSE=True


try:
    MEGA_EMAIL=os.environ["MEGA_EMAIL"]
    MEGA_PWD=os.environ["MEGA_PWD"]
except:
    print >>sys.stderr, "You must define variables MEGA_EMAIL MEGA_PWD. WARNING: Use an empty account for $MEGA_EMAIL"
    exit(1)


try:
    MEGACMDSHELL=os.environ['MEGACMDSHELL']
    CMDSHELL=True
    #~ FIND="executeinMEGASHELL find" #TODO

except:
    CMDSHELL=False

def initialize_contents():
    contents=" ".join(['"localtmp/'+x+'"' for x in os.listdir('localtmp/')])
    cmd_ef(PUT+" "+contents+" /")
    shutil.copytree('localtmp', 'localUPs')

def clean_all(): 
    
    if cmd_es(WHOAMI) != osvar("MEGA_EMAIL"):
        cmd_ef(LOGOUT)
        cmd_ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))
        
    #~ rm pipe > /dev/null 2>/dev/null || :

    cmd_ec(RM+' -rf "*"')
    cmd_ec(RM+' -rf "//bin/*"')
    
    rmfolderifexisting("localUPs")
    rmfolderifexisting("localtmp")
    
    rmfileifexisting("megafind.txt")
    rmfileifexisting("localfind.txt")



def clear_local_and_remote():
    rmfolderifexisting("localUPs")
    cmd_ec(RM+' -rf "/*"')
    initialize_contents()

def compare_and_clear() :
    global currentTest
    if VERBOSE:
        print "test "+str(currentTest)
    
    megafind=sort(cmd_ef(FIND))
    localfind=sort(find('localUPs'))
    
    #~ if diff --side-by-side megafind.txt localfind.txt 2>/dev/null >/dev/null; then
    if (megafind == localfind):
        if VERBOSE:
            print "diff megafind vs localfind:"
            #diff --side-by-side megafind.txt localfind.txt#TODO: do this
            print "MEGAFIND:"
            print megafind
            print "LOCALFIND"
            print localfind
        print "test "+str(currentTest)+" succesful!"     
    else:
        print "test "+str(currentTest)+" failed!"
        print "diff megafind vs localfind:"
        #~ diff --side-by-side megafind.txt localfind.txt #TODO: do this
        print "MEGAFIND:"
        print megafind
        print "LOCALFIND"
        print localfind
        
        #cd $ABSPWD #TODO: consider this
        exit(1)

    clear_local_and_remote()
    currentTest+=1
    cmd_ef(CD+" /")

def initialize():
     
    if cmd_es(WHOAMI) != osvar("MEGA_EMAIL"):
        cmd_es(LOGOUT)
        cmd_ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))

    if len(os.listdir(".")):
        print >>sys.stderr, "initialization folder not empty!"
        #~ cd $ABSPWD
        exit(1)

    if cmd_es(FIND+" /") != "/":
        print >>sys.stderr, "REMOTE Not empty, please clear it before starting!"
        #~ cd $ABSPWD
        exit(1)


    #initialize localtmp estructure:
    makedir("localtmp")
    touch("localtmp/file01.txt")
    out('file01contents', 'localtmp/file01nonempty.txt')
    #local empty folders structure
    for f in ['localtmp/le01/'+a for a in ['les01/less01']+ ['les02/less0'+z for z in ['1','2']] ]: makedir(f)
    #local filled folders structure
    for f in ['localtmp/lf01/'+a for a in ['lfs01/lfss01']+ ['lfs02/lfss0'+z for z in ['1','2']] ]: makedir(f)
    for f in ['localtmp/lf01/'+a for a in ['lfs01/lfss01']+ ['lfs02/lfss0'+z for z in ['1','2']] ]: touch(f+"/commonfile.txt")
    #spaced structure
    for f in ['localtmp/ls 01/'+a for a in ['ls s01/ls ss01']+ ['ls s02/ls ss0'+z for z in ['1','2']] ]: makedir(f)
    for f in ['localtmp/ls 01/'+a for a in ['ls s01/ls ss01']+ ['ls s02/ls ss0'+z for z in ['1','2']] ]: touch(f+"/common file.txt")

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
    clear_local_and_remote()


def compare_find(what, localFindPrefix='localUPs'):
    global currentTest
    if VERBOSE:
        print "test "+str(currentTest)

    if not isinstance(what, list):
        what = [what]
    megafind=""
    localfind=""
    for w in what:
        megafind+=cmd_ef(FIND+" "+w)+"\n"
        localfind+=find(localFindPrefix+'/'+w,w)+"\n"
    
    megafind=sort(megafind).strip()
    localfind=sort(localfind).strip()
    
    #~ megafind=$FIND "$@"  | sort > $ABSPWD/megafind.txt
    #~ (cd localUPs 2>/dev/null; find "$@" | sed "s#\./##g" | sort) > $ABSPWD/localfind.txt
    if (megafind == localfind):
        if VERBOSE:
            print "diff megafind vs localfind:"
            #diff --side-by-side megafind.txt localfind.txt#TODO: do this
            print "MEGAFIND:"
            print megafind
            print "LOCALFIND"
            print localfind
        print "test "+str(currentTest)+" succesful!"     
    else:
        print "test "+str(currentTest)+" failed!"
        print "diff megafind vs localfind:"
        #~ diff --side-by-side megafind.txt localfind.txt #TODO: do this
        print "MEGAFIND:"
        print megafind
        print "LOCALFIND"
        print localfind
        
        #cd $ABSPWD #TODO: consider this
        exit(1)
    
    currentTest+=1

def compare_remote_local(megafind, localfind):
    global currentTest
    if (megafind == localfind):
        if VERBOSE:
            print "diff megafind vs localfind:"
            #diff --side-by-side megafind.txt localfind.txt#TODO: do this
            print "MEGAFIND:"
            print megafind
            print "LOCALFIND"
            print localfind
        print "test "+str(currentTest)+" succesful!"     
    else:
        print "test "+str(currentTest)+" failed!"
        print "diff megafind vs localfind:"
        #~ diff --side-by-side megafind.txt localfind.txt #TODO: do this
        print "MEGAFIND:"
        print megafind
        print "LOCALFIND"
        print localfind
        
        exit(1)
       
    currentTest+=1


if VERBOSE: print "STARTING..."

#INITIALIZATION
clean_all()
initialize()

#Test 01 #destiny empty file
compare_find('file01.txt')

#Test 02  #entire empty folders
compare_find('le01/les01/less01')

#Test 03 #1 file folder
compare_find('lf01/lfs01/lfss01')

#Test 04 #entire non empty folders structure
compare_find('lf01')

#Test 05 #multiple
compare_find(['lf01','le01/les01'])

#Test 06 #.
cmd_ef(CD+" le01")
compare_find('.','localUPs/le01')
cmd_ef(CD+" /")

#Test 07 #. global
compare_find('.')

#Test 08 #spaced
megafind=sort(cmd_ef(FIND+" "+"ls\ 01"))
localfind=sort(find('localUPs/ls 01',"ls 01"))
compare_remote_local(megafind,localfind)

#Test 09 #XX/..
currentTest=9
megafind=sort(cmd_ef(FIND+" "+"ls\ 01/.."))
localfind=sort(find('localUPs/',"/"))
compare_remote_local(megafind,localfind)

#Test 10 #..
cmd_ef(CD+' le01')
megafind=sort(cmd_ef(FIND+" "+".."))
cmd_ef(CD+' /')
localfind=sort(find('localUPs/',"/"))
compare_remote_local(megafind,localfind)

#Test 11 #complex stuff
megafind=sort(cmd_ef(FIND+" "+"ls\ 01/../le01/les01" +" " +"lf01/../ls\ *01/ls\ s02"))
localfind=sort(find('localUPs/le01/les01',"/le01/les01"))
localfind+="\n"+sort(find('localUPs/ls 01/ls s02',"/ls 01/ls s02"))
compare_remote_local(megafind,localfind)

#Test 12 #folder/
megafind=sort(cmd_ef(FIND+" "+"le01/"))
localfind=sort(find('localUPs/le01',"le01"))
compare_remote_local(megafind,localfind)

if not CMDSHELL: #TODO: currently there is no way to know last CMSHELL status code

    #Test 13 #file01.txt/non-existent
    megafind,status=cmd_ec(FIND+" "+"file01.txt/non-existent")
    if status == 0: 
        print "test "+str(currentTest)+" failed!"
        exit(1)
    else:
        print "test "+str(currentTest)+" succesful!"

currentTest=14

#Test 14 #/
compare_find('/')

###TODO: do stuff in shared folders...

###################

# Clean all
if not VERBOSE:
    clean_all()

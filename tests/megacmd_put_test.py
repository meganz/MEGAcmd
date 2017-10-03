#!/usr/bin/python
# -*- coding: utf-8 -*-
#better run in an empty folder

import sys, os, subprocess, shutil, distutils, distutils.dir_util
from megacmd_tests_common import *

GET="mega-get"
PUT="mega-put"
RM="mega-rm"
CD="mega-cd"
LCD="mega-lcd"
MKDIR="mega-mkdir"
EXPORT="mega-export"
SHARE="mega-share"
FIND="mega-find"
WHOAMI="mega-whoami"
LOGOUT="mega-logout"
LOGIN="mega-login"
IPC="mega-ipc"
IMPORT="mega-import"

ABSPWD=os.getcwd()
currentTest=1

try:
    MEGA_EMAIL=os.environ["MEGA_EMAIL"]
    MEGA_PWD=os.environ["MEGA_PWD"]
    MEGA_EMAIL_AUX=os.environ["MEGA_EMAIL_AUX"]
    MEGA_PWD_AUX=os.environ["MEGA_PWD_AUX"]
except:
    print >>sys.stderr, "You must define variables MEGA_EMAIL MEGA_PWD MEGA_EMAIL_AUX MEGA_PWD_AUX. WARNING: Use an empty account for $MEGA_EMAIL"
    exit(1)

try:
    os.environ['VERBOSE']
    VERBOSE=True
except:
    VERBOSE=False

#VERBOSE=True

try:
    MEGACMDSHELL=os.environ['MEGACMDSHELL']
    CMDSHELL=True
    #~ FIND="executeinMEGASHELL find" #TODO
except:
    CMDSHELL=False


def clean_all():

    if es(WHOAMI) != osvar("MEGA_EMAIL"):
        ef(LOGOUT)
        ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))
    
    ec(RM+' -rf "*"')
    ec(RM+' -rf "//bin/*"')
    
    rmfolderifexisting("localUPs")
    rmfolderifexisting("localtmp")
    
    rmfileifexisting("megafind.txt")
    rmfileifexisting("localfind.txt")


def clear_local_and_remote():
    rmfolderifexisting("localUPs")
    ec(RM+' -rf "/*"')
    initialize_contents()

currentTest=1

def compare_and_clear() :
    global currentTest
    if VERBOSE:
        print "test $currentTest"
    
    megafind=sort(ef(FIND))
    localfind=sort(find('localUPs','.'))
    
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
    ef(CD+" /")

def check_failed_and_clear(o,status):
    global currentTest

    if status == 0: 
        print "test "+str(currentTest)+" failed!"
        print o
        exit(1)
    else:
        print "test "+str(currentTest)+" succesful!"

    clear_dls()
    currentTest+=1
    ef(CD+" /")



def check_failed_and_clear(o,status):
    global currentTest

    if status == 0: 
        print "test "+str(currentTest)+" failed!"
        print o
        exit(1)
    else:
        print "test "+str(currentTest)+" succesful!"

    clear_local_and_remote()
    currentTest+=1
    ef(CD+" /")

def initialize():
    if es(WHOAMI) != osvar("MEGA_EMAIL"):
        ef(LOGOUT)
        ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))
        

    if len(os.listdir(".")):
        print >>sys.stderr, "initialization folder not empty!"
        #~ cd $ABSPWD
        exit(1)

    if es(FIND+" /") != "/":
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

def initialize_contents():
    remotefolders=['01/'+a for a in ['s01/ss01']+ ['s02/ss0'+z for z in ['1','2']] ]
    ef(MKDIR+" -p "+" ".join(remotefolders))
    for f in ['localUPs/01/'+a for a in ['s01/ss01']+ ['s02/ss0'+z for z in ['1','2']] ]: makedir(f)

#INITIALIZATION
clean_all()
initialize()

ABSMEGADLFOLDER=ABSPWD+'/megaDls'

clear_local_and_remote()

#Test 01 #clean comparison
compare_and_clear()

#Test 02 #no destiny empty file upload
ef(PUT+' '+'localtmp/file01.txt')
shutil.copy2('localtmp/file01.txt','localUPs/')
compare_and_clear()

#Test 03 #/ destiny empty file upload
ef(PUT+' '+'localtmp/file01.txt /')
shutil.copy2('localtmp/file01.txt','localUPs/')
compare_and_clear()

#Test 04 #no destiny nont empty file upload
ef(PUT+' '+'localtmp/file01nonempty.txt')
shutil.copy2('localtmp/file01nonempty.txt','localUPs/')
compare_and_clear()

#Test 05 #empty folder
ef(PUT+' '+'localtmp/le01/les01/less01')
copyfolder('localtmp/le01/les01/less01','localUPs/')
compare_and_clear()

#Test 06 #1 file folder
ef(PUT+' '+'localtmp/lf01/lfs01/lfss01')
copyfolder('localtmp/lf01/lfs01/lfss01','localUPs/')
compare_and_clear()

#Test 07 #entire empty folders structure
ef(PUT+' '+'localtmp/le01')
copyfolder('localtmp/le01','localUPs/')
compare_and_clear()

#Test 08 #entire non empty folders structure
ef(PUT+' '+'localtmp/lf01')
copyfolder('localtmp/lf01','localUPs/')
compare_and_clear()

#Test 09 #copy structure into subfolder
ef(PUT+' '+'localtmp/le01 /01/s01')
copyfolder('localtmp/le01','localUPs/01/s01')
compare_and_clear()

#~ #Test 10 #copy exact structure
makedir('aux')
copyfolder('localUPs/01','aux')
ef(PUT+' '+'aux/01/s01 /01/s01')
copyfolder('aux/01/s01','localUPs/01/s01')
rmfolderifexisting("aux")
compare_and_clear()

#~ #Test 11 #merge increased structure 
makedir('aux')
copyfolder('localUPs/01','aux')
touch('aux/01/s01/another.txt')
ef(PUT+' '+'aux/01/s01 /01/')
shutil.copy2('aux/01/s01/another.txt','localUPs/01/s01')
compare_and_clear()
rmfolderifexisting("aux")

#Test 12 #multiple upload
ef(PUT+' '+'localtmp/le01 localtmp/lf01 /01/s01')
copyfolder('localtmp/le01','localUPs/01/s01')
copyfolder('localtmp/lf01','localUPs/01/s01')
compare_and_clear()

currentTest=13
#Test 13 #local regexp
ef(PUT+' '+'localtmp/*txt /01/s01')
copybyfilepattern('localtmp/','*.txt','localUPs/01/s01')
compare_and_clear()

#Test 14 #../
ef(CD+' 01')
ef(PUT+' '+'localtmp/le01 ../01/s01')
ef(CD+' /')
copyfolder('localtmp/le01','localUPs/01/s01')
compare_and_clear()

currentTest=15

#Test 15 #spaced stuff
if CMDSHELL: #TODO: think about this again
    ef(PUT+' '+'localtmp/ls\ 01')
else:
    ef(PUT+' '+'"localtmp/ls 01"')

copyfolder('localtmp/ls 01','localUPs')
compare_and_clear()


###TODO: do stuff in shared folders...

########

##Test XX #merge structure with file updated
##This test fails, because it creates a remote copy of the updated file.
##That's expected. If ever provided a way to create a real merge (e.g.: put -m ...)  to do BACKUPs, reuse this test.
#mkdir aux
#shutil.copy2('-pr localUPs/01','aux')
#touch aux/01/s01/another.txt
#ef(PUT+' '+'aux/01/s01 /01/')
#rsync -aLp aux/01/s01/ localUPs/01/s01/
#echo "newcontents" > aux/01/s01/another.txt 
#ef(PUT+' '+'aux/01/s01 /01/')
#rsync -aLp aux/01/s01/ localUPs/01/s01/
#rm -r aux
#compare_and_clear()


# Clean all
if not VERBOSE:
    clean_all()

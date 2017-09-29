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

def initialize_contents():
    contents=" localtmp/"+" localtmp/".join(['"'+x+'"' for x in os.listdir('localtmp/')])
    ef(PUT+" "+contents+" /")
    shutil.copytree('localtmp', 'localUPs')

def clean_all(): 
    
    if es(WHOAMI) != osvar("MEGA_EMAIL"):
        ef(LOGOUT)
        ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))
        
    ec(RM+' -rf "*"')
    ec(RM+' -rf "//bin/*"')
    
    rmfolderifexisting("localUPs")
    rmfolderifexisting("localtmp")
    rmfolderifexisting("origin")
    rmfolderifexisting("megaDls")
    rmfolderifexisting("localDls")
    
    rmfileifexisting("megafind.txt")
    rmfileifexisting("localfind.txt")

def clear_dls():
    rmcontentsifexisting("megaDls")
    rmcontentsifexisting("localDls")

currentTest=1

def compare_and_clear():
    global currentTest
        
    megaDls=sort(find('megaDls'))
    localDls=sort(find('localDls'))
    
    #~ if diff megaDls localDls 2>/dev/null >/dev/null; then
    if (megaDls == localDls):
        print "test "+str(currentTest)+" succesful!"     
        if VERBOSE:
            print "test "+str(currentTest)
            print "megaDls:"
            print megaDls
            print
            print "localDls:"
            print localDls
            print
    else:
        print "test "+str(currentTest)+" failed!"

        print "megaDls:"
        print megaDls
        print
        print "localDls:"
        print localDls
        print
        exit(1)

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

    clear_dls()
    currentTest+=1
    ef(CD+" /")


def initialize_contents():
    global URIFOREIGNEXPORTEDFOLDER
    global URIFOREIGNEXPORTEDFILE
    global URIEXPORTEDFOLDER
    global URIEXPORTEDFILE
    
    if es(WHOAMI) != osvar("MEGA_EMAIL_AUX"):
        ef(LOGOUT)
        ef(LOGIN+" " +osvar("MEGA_EMAIL_AUX")+" "+osvar("MEGA_PWD_AUX"))

    if len(os.listdir(".")):
        print >>sys.stderr, "initialization folder not empty!"
        exit(1)
        
    #initialize localtmp estructure:
    makedir('foreign')

    for f in ['cloud0'+a for a in ['1','2']] + \
     ['bin0'+a for a in ['1','2']] + \
     ['cloud0'+a+'/c0'+b+'s0'+c for a in ['1','2'] for b in ['1','2'] for c in ['1','2']] + \
     ['foreign'] + \
     ['foreign/sub0'+a for a in ['1','2']] + \
     ['bin0'+a+'/c0'+b+'s0'+c for a in ['1','2'] for b in ['1','2'] for c in ['1','2']]:
        makedir(f)
        out(f,f+'/fileat'+f.split('/')[-1]+'.txt')

    ef(PUT+' foreign /')
    ef(SHARE+' foreign -a --with='+osvar("MEGA_EMAIL"))
    URIFOREIGNEXPORTEDFOLDER=ef(EXPORT+' foreign/sub01 -a').split(' ')[-1]
    URIFOREIGNEXPORTEDFILE=ef(EXPORT+' foreign/sub02/fileatsub02.txt -a').split(' ')[-1]
    
    if VERBOSE:
        print "URIFOREIGNEXPORTEDFOLDER=",URIFOREIGNEXPORTEDFILE
        print "URIFOREIGNEXPORTEDFILE=",URIFOREIGNEXPORTEDFILE
    

    ef(LOGOUT)
    ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))
    ec(IPC+" -a "+osvar("MEGA_EMAIL_AUX"))

    ef(PUT+' foreign /')
    
    #~ mega-put cloud0* /
    ef(PUT+" cloud01 cloud02 /")

    #~ mega-put bin0* //bin
    ef(PUT+" bin01 bin02 //bin")

    URIEXPORTEDFOLDER=ef(EXPORT+' cloud01/c01s01 -a').split(' ')[-1]
    URIEXPORTEDFILE=ef(EXPORT+' cloud02/fileatcloud02.txt -a').split(' ')[-1]
    
    if VERBOSE:
        print "URIEXPORTEDFOLDER=",URIEXPORTEDFOLDER
        print "URIEXPORTEDFILE=",URIEXPORTEDFILE
    
if VERBOSE: print "STARTING..."

#INITIALIZATION
clean_all
makedir('origin')
os.chdir('origin')
initialize_contents()
os.chdir(ABSPWD)

makedir('megaDls')
makedir('localDls')

ABSMEGADLFOLDER=ABSPWD+'/megaDls'

URIEXPORTEDFOLDER=ef(EXPORT+' cloud01/c01s01 -a').split(' ')[-1]
URIEXPORTEDFILE=ef(EXPORT+' cloud02/fileatcloud02.txt -a').split(' ')[-1]


clear_dls()

#Test 01
ef(GET+' /cloud01/fileatcloud01.txt '+ABSMEGADLFOLDER+'/')
shutil.copy2('origin/cloud01/fileatcloud01.txt','localDls/')
compare_and_clear()

#Test 02
ef(GET+' //bin/bin01/fileatbin01.txt '+ABSMEGADLFOLDER)
shutil.copy2('origin/bin01/fileatbin01.txt','localDls/')
compare_and_clear()

#Test 03
ef(GET+' //bin/bin01/fileatbin01.txt '+ABSMEGADLFOLDER+'/')
shutil.copy2('origin/bin01/fileatbin01.txt','localDls/')
compare_and_clear()

#Test 04
ef(GET+' '+osvar('MEGA_EMAIL_AUX')+':foreign/fileatforeign.txt '+ABSMEGADLFOLDER+'/')
shutil.copy2('origin/foreign/fileatforeign.txt','localDls/')
compare_and_clear()

#Test 05
ef(GET+' '+osvar('MEGA_EMAIL_AUX')+':foreign/fileatforeign.txt '+ABSMEGADLFOLDER+'/')
shutil.copy2('origin/foreign/fileatforeign.txt','localDls/')
compare_and_clear()

#~ #Test 06
ef(CD+' cloud01')
ef(GET+' "*.txt" '+ABSMEGADLFOLDER+'')
copybyfilepattern('origin/cloud01/','*.txt','localDls/')
compare_and_clear()

#Test 07
ef(CD+' //bin/bin01')
ef(GET+' "*.txt" '+ABSMEGADLFOLDER+'')
copybyfilepattern('origin/bin01/','*.txt','localDls/')
compare_and_clear()

#Test 08
ef(CD+' '+osvar('MEGA_EMAIL_AUX')+':foreign')
ef(GET+' "*.txt" '+ABSMEGADLFOLDER+'')
copybyfilepattern('origin/foreign/','*.txt','localDls/')
compare_and_clear()

#Test 09
ef(GET+' cloud01/c01s01 '+ABSMEGADLFOLDER+'')
copyfolder('origin/cloud01/c01s01','localDls/')
compare_and_clear()

#Test 10
ef(GET+' cloud01/c01s01 '+ABSMEGADLFOLDER+'/')
copyfolder('origin/cloud01/c01s01','localDls/')
compare_and_clear()

#~ #Test 11
ef(GET+' cloud01/c01s01 '+ABSMEGADLFOLDER+' -m')
distutils.dir_util.copy_tree('origin/cloud01/c01s01/', 'localDls/')
compare_and_clear()

#Test 12
ef(GET+' cloud01/c01s01 '+ABSMEGADLFOLDER+'/ -m')
distutils.dir_util.copy_tree('origin/cloud01/c01s01/', 'localDls/')
compare_and_clear()

#Test 13
ef(GET+' "'+URIEXPORTEDFOLDER+'" '+ABSMEGADLFOLDER+'')
copyfolder('origin/cloud01/c01s01','localDls/')
compare_and_clear()

#Test 14
ef(GET+' "'+URIEXPORTEDFILE+'" '+ABSMEGADLFOLDER+'')
shutil.copy2('origin/cloud02/fileatcloud02.txt','localDls/')
compare_and_clear()

#Test 15
ef(GET+' "'+URIEXPORTEDFOLDER+'" '+ABSMEGADLFOLDER+' -m')
distutils.dir_util.copy_tree('origin/cloud01/c01s01/', 'localDls/')
compare_and_clear()

#Test 16
ef(CD+' /cloud01/c01s01')
ef(GET+' . '+ABSMEGADLFOLDER+'')
copyfolder('origin/cloud01/c01s01','localDls/')
compare_and_clear()

#Test 17
ef(CD+' /cloud01/c01s01')
ef(GET+' . '+ABSMEGADLFOLDER+' -m')
distutils.dir_util.copy_tree('origin/cloud01/c01s01/', 'localDls/')
compare_and_clear()

#Test 18
ef(CD+' /cloud01/c01s01')
ef(GET+' ./ '+ABSMEGADLFOLDER+'')
copyfolder('origin/cloud01/c01s01','localDls/')
compare_and_clear()

#Test 19
ef(CD+' /cloud01/c01s01')
ef(GET+' ./ '+ABSMEGADLFOLDER+' -m')
distutils.dir_util.copy_tree('origin/cloud01/c01s01/', 'localDls/')
compare_and_clear()

#Test 20
ef(CD+' /cloud01/c01s01')
ef(GET+' .. '+ABSMEGADLFOLDER+' -m')
distutils.dir_util.copy_tree('origin/cloud01/', 'localDls/')
compare_and_clear()

#Test 21
ef(CD+' /cloud01/c01s01')
ef(GET+' ../ '+ABSMEGADLFOLDER+'')
copyfolder('origin/cloud01','localDls/')
compare_and_clear()

#Test 22
out("existing",ABSMEGADLFOLDER+'/existing')
ef(GET+' /cloud01/fileatcloud01.txt '+ABSMEGADLFOLDER+'/existing')
shutil.copy2('origin/cloud01/fileatcloud01.txt','localDls/existing (1)')
out("existing",'localDls/existing')
compare_and_clear()

#Test 23
out("existing",'megaDls/existing')
ef(GET+' /cloud01/fileatcloud01.txt megaDls/existing')
shutil.copy2('origin/cloud01/fileatcloud01.txt','localDls/existing (1)')
out("existing",'localDls/existing')
compare_and_clear()

#Test 24
ef(GET+' cloud01/c01s01 megaDls')
copyfolder('origin/cloud01/c01s01','localDls/')
compare_and_clear()

currentTest=25

#Test 25
ef(GET+' cloud01/fileatcloud01.txt megaDls')
shutil.copy2('origin/cloud01/fileatcloud01.txt','localDls/')
compare_and_clear()

if not CMDSHELL: #TODO: currently there is no way to know last CMSHELL status code
    #Test 26
    o,status=ec(GET+' cloud01/fileatcloud01.txt /no/where')
    check_failed_and_clear(o,status)

    #Test 27
    o,status=ec(GET+' /cloud01/cloud01/fileatcloud01.txt /no/where')
    check_failed_and_clear(o,status)

currentTest=28

#Test 28
ef(GET+' /cloud01/fileatcloud01.txt '+ABSMEGADLFOLDER+'/newfile')
shutil.copy2('origin/cloud01/fileatcloud01.txt','localDls/newfile')
compare_and_clear()

#Test 29
os.chdir(ABSMEGADLFOLDER)
ef(GET+' /cloud01/fileatcloud01.txt .')
os.chdir(ABSPWD)
shutil.copy2('origin/cloud01/fileatcloud01.txt','localDls/')
compare_and_clear()

#Test 30
os.chdir(ABSMEGADLFOLDER)
ef(GET+' /cloud01/fileatcloud01.txt ./')
os.chdir(ABSPWD)
shutil.copy2('origin/cloud01/fileatcloud01.txt','localDls/')
compare_and_clear()

#Test 31
makedir(ABSMEGADLFOLDER+'/newfol')
os.chdir(ABSMEGADLFOLDER+'/newfol')
ef(GET+' /cloud01/fileatcloud01.txt ..')
os.chdir(ABSPWD)
makedir('localDls/newfol')
shutil.copy2('origin/cloud01/fileatcloud01.txt','localDls/')
compare_and_clear()

#Test 32
makedir(ABSMEGADLFOLDER+'/newfol')
os.chdir(ABSMEGADLFOLDER+'/newfol')
ef(GET+' /cloud01/fileatcloud01.txt ../')
os.chdir(ABSPWD)
makedir('localDls/newfol')
shutil.copy2('origin/cloud01/fileatcloud01.txt','localDls/')
compare_and_clear()

if not CMDSHELL: #TODO: currently there is no way to know last CMSHELL status code
    #Test 33
    o,status=ec(GET+' path/to/nowhere '+ABSMEGADLFOLDER+' > /dev/null')
    check_failed_and_clear(o,status)

    #Test 34
    o,status=ec(GET+' /path/to/nowhere '+ABSMEGADLFOLDER+' > /dev/null')
    check_failed_and_clear(o,status)

currentTest=35

#Test 35
os.chdir(ABSMEGADLFOLDER)
ef(GET+' /cloud01/fileatcloud01.txt')
os.chdir(ABSPWD)
shutil.copy2('origin/cloud01/fileatcloud01.txt','localDls/')
compare_and_clear()

currentTest=36

#Test 36 # imported stuff (to test import folder)
ex(RM+' -rf /imported')
ef(MKDIR+' -p /imported')
ef(IMPORT+' '+URIFOREIGNEXPORTEDFOLDER+' /imported')
ef(GET+' /imported/* '+ABSMEGADLFOLDER+'')
copyfolder('origin/foreign/sub01','localDls/')
compare_and_clear()

#Test 37 # imported stuff (to test import file)
ex(RM+' -rf /imported')
ef(MKDIR+' -p /imported')
ef(IMPORT+' '+URIFOREIGNEXPORTEDFILE+' /imported')
ef(GET+' /imported/fileatsub02.txt '+ABSMEGADLFOLDER+'')
shutil.copy2('origin/foreign/sub02/fileatsub02.txt','localDls/')
compare_and_clear()

# Clean all
clean_all()

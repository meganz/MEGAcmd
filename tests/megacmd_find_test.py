#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys, os, subprocess, shutil


GET="mega-get"
PUT="mega-put"
RM="mega-rm"
CD="mega-cd"
LCD="mega-lcd"
MKDIR="mega-mkdir"
EXPORT="mega-export"
FIND="mega-find"
WHOAMI="mega-whoami"
LOGOUT="mega-logout"
LOGIN="mega-login"
ABSPWD=os.getcwd()
currentTest=1


try:
    MEGA_EMAIL=os.environ["MEGA_EMAIL"]
    MEGA_PWD=os.environ["MEGA_PWD"]
except:
    print >>sys.stderr, "You must define variables MEGA_EMAIL MEGA_PWD. WARNING: Use an empty account for $MEGA_EMAIL"
    exit(1)


try:
    os.environ['VERBOSE']
    VERBOSE=True
except:
    VERBOSE=False

try:
    MEGACMDSHELL=os.environ['MEGACMDSHELL']
    CMDSHELL=True
    #~ FIND="executeinMEGASHELL find" #TODO

except:
    CMDSHELL=False
    
#execute
def ec(what):
    if VERBOSE:
        print "Executing "+what
    process = subprocess.Popen(what, shell=True, stdout=subprocess.PIPE)
    stdoutdata, stderrdata = process.communicate()
    if VERBOSE:
        print stdoutdata.strip()
    return stdoutdata,process.returncode

#execute and return only stdout contents
def ex(what):
    return ec(what)[0]
    #return subprocess.Popen(what, shell=True, stdout=subprocess.PIPE).stdout.read()

#Execute and strip, return only stdout
def es(what):
    return ec(what)[0].strip()

#Execute and strip with status code
def esc(what):
    ret=ec(what)
    return ret[0].strip(),ret[1]
    
#exit if failed
def ef(what):
    out,code=esc(what)
    if code != 0:
        print >>sys.stderr, "FALLO en "+str(what) #TODO: stderr?
        print >>sys.stderr, out #TODO: stderr?
        
        exit(code)
    return out

#~ def executeinMEGASHELL()
#~ {
    #~ command=$1
    #~ shift;
    #~ echo $command "$@" > /tmp/shellin

    #~ $MEGACMDSHELL < /tmp/shellin  | sed "s#^.*\[K##g" | grep $MEGA_EMAIL -A 1000 | grep -v $MEGA_EMAIL
    
    
    #~ from subprocess import Popen, PIPE, STDOUT

    #~ p = Popen(['grep', 'f'], stdout=PIPE, stdin=PIPE, stderr=STDOUT)    
    #~ grep_stdout = p.communicate(input=b'one\ntwo\nthree\nfour\nfive\nsix\n')[0]
    #~ print(grep_stdout.decode())


#~ }


def rmfolderifexisting(what):
    if os.path.exists(what):
        shutil.rmtree(what)

def rmfileifexisting(what):
    if os.path.exists(what):
        os.remove(what)
        
def makedir(what):
    if (not os.path.exists(what)):
        os.makedirs(what)

def osvar(what):
    try:
        return os.environ[what]
    except:
        return ""

def sort(what):
    return "\n".join(sorted(what.split("\n")))

def initialize_contents():
    ef(PUT+" localtmp/* /")
    makedir("localUPs") #TODO: remove all mkdirs by python
    ef("cp -r localtmp/* localUPs/") #do this in python

def clean_all(): 
    
    if es(WHOAMI) != osvar("MEGA_EMAIL"):
        ef(LOGOUT)
        ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))
        
    #~ rm pipe > /dev/null 2>/dev/null || :

    ec(RM+' -rf "*"')
    ec(RM+' -rf "//bin/*"')
    
    rmfolderifexisting("localUPs")
    rmfolderifexisting("localtmp")
    
    rmfileifexisting("megafind.txt")
    rmfileifexisting("localfind.txt")

 
def findR(where, prefix=""):
    toret=""
    #~ print "e:",where
    for f in os.listdir(where):
        #~ print "f:",where+"/"+f
        toret+=prefix+f+"\n"
        if (os.path.isdir(where+"/"+f)):
            #~ print "entering ",prefix+f
            toret=toret+findR(where+"/"+f,prefix+f+"/")
    return toret

def find(where, prefix=""):
    if not os.path.exists(where):
        if VERBOSE: print "file not found in find:", where, os.getcwd()

        return ""
    
    if (not os.path.isdir(where)):
        return prefix        

    if (prefix == ""): toret ="."
    else: toret=prefix
    
    if (prefix == "."):
        toret+="\n"+findR(where).strip()
    else:
        if prefix.endswith('/'):
            toret+="\n"+findR(where, prefix).strip()
        else:
            toret+="\n"+findR(where, prefix+"/").strip()
    return toret.strip()

def touch(what, times=None):
    with open(what, 'a'):
        os.utime(what, times)

def out(what, where):
    #~ print(what, file=where)
    with open(where, 'w') as f:
        f.write(what)

def clear_local_and_remote():
    rmfolderifexisting("localUPs")
    ec(RM+' -rf "/*"')
    initialize_contents()

def compare_and_clear() :
    global currentTest
    if VERBOSE:
        print "test $currentTest"
    
    megafind=sort(ef(FIND))
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
    ef("/")

def initialize():
     
    if es(WHOAMI) != osvar("MEGA_EMAIL"):
        es(LOGOUT)
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


def compare_find(what, localFindPrefix='localUPs'):
    global currentTest
    if VERBOSE:
        print "test "+str(currentTest)

    if not isinstance(what, list):
        what = [what]
    megafind=""
    localfind=""
    for w in what:
        megafind+=ef(FIND+" "+w)+"\n"
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
ef(CD+" le01")
compare_find('.','localUPs/le01')
ef(CD+" /")

#Test 07 #. global
compare_find('.')

#Test 08 #spaced
megafind=sort(ef(FIND+" "+"ls\ 01"))
localfind=sort(find('localUPs/ls 01',"ls 01"))
compare_remote_local(megafind,localfind)

#Test 09 #XX/..
currentTest=9
megafind=sort(ef(FIND+" "+"ls\ 01/.."))
localfind=sort(find('localUPs/',"/"))
compare_remote_local(megafind,localfind)

#Test 10 #..
ef(CD+' le01')
megafind=sort(ef(FIND+" "+".."))
ef(CD+' /')
localfind=sort(find('localUPs/',"/"))
compare_remote_local(megafind,localfind)

#Test 11 #complex stuff
megafind=sort(ef(FIND+" "+"ls\ 01/../le01/les01" +" " +"lf01/../ls\ *01/ls\ s02"))
localfind=sort(find('localUPs/le01/les01',"/le01/les01"))
localfind+="\n"+sort(find('localUPs/ls 01/ls s02',"/ls 01/ls s02"))
compare_remote_local(megafind,localfind)

#Test 12 #folder/
megafind=sort(ef(FIND+" "+"le01/"))
localfind=sort(find('localUPs/le01',"le01"))
compare_remote_local(megafind,localfind)

if not CMDSHELL: #TODO: currently there is no way to know last CMSHELL status code

    #Test 13 #file01.txt/non-existent
    megafind,status=ec(FIND+" "+"file01.txt/non-existent")
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

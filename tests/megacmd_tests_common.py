#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys, os, subprocess, shutil
import fnmatch

try:
    os.environ['VERBOSE']
    VERBOSE=True
except:
    VERBOSE=False


#execute
def ec(what):
    if VERBOSE:
        print "Executing "+what
    process = subprocess.Popen(what, shell=True, stdout=subprocess.PIPE)
    stdoutdata, stderrdata = process.communicate()

    stdoutdata=stdoutdata.replace('\r\n','\n')
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
        
def rmcontentsifexisting(what):
    if os.path.exists(what) and os.path.isdir(what):
        shutil.rmtree(what)
        os.makedirs(what)
        
def copybyfilepattern(origin,pattern,destiny):
    for f in fnmatch.filter(os.listdir(origin),pattern):
        shutil.copy2(origin+'/'+f,destiny)
        
def copyfolder(origin,destiny):
    shutil.copytree(origin,destiny+'/'+origin.split('/')[-1])

def copybypattern(origin,pattern,destiny):
    for f in fnmatch.filter(os.listdir(origin),pattern):
        if (os.path.isdir(origin+'/'+f)):
            copyfolder(origin+'/'+f,destiny)
        else:
            shutil.copy2(origin+'/'+f,destiny)
        
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

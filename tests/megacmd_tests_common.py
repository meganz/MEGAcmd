#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys, os, subprocess, shutil, re
import fnmatch

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

#execute command
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
        print >>sys.stderr, "FAILED trying ", what
        print >>sys.stderr, out #TODO: stderr?
        
        exit(code)
    return out    

def cmdshell_ec(what):
    what=re.sub("^mega-","",what)
    if VERBOSE:
        print "Executing in cmdshell: "+what
    towrite="lcd "+os.getcwd()+"\n"+what
    out(towrite+"\n",'/tmp/shellin')
    with open('/tmp/shellin') as shellin:
        if VERBOSE:
            print "Launching in cmdshell ... " + MEGACMDSHELL
        process = subprocess.Popen(MEGACMDSHELL, shell=True, stdin=shellin, stdout=subprocess.PIPE)
        stdoutdata, stderrdata = process.communicate()
        realout =[]
        equallines=0
        afterwelcomemsg=False
        afterorder=False
        for l in stdoutdata.split('\n'):
            l=re.sub(".*\x1b\[K","",l) #replace non printable stuff(erase line controls)
            l=re.sub(".*\r","",l) #replace non printable stuff
            if afterorder:
                if "Exiting ..." in l: break
                realout+=[l]
            elif afterwelcomemsg:
                if what in l: afterorder = True
            elif "="*20 in l:
                equallines+=1
                if equallines==2: afterwelcomemsg = True
        
        realout="\n".join(realout)
        if VERBOSE:
            print realout.strip()

        return realout,process.returncode

#execute and return only stdout contents
def cmdshell_ex(what):
    return cmdshell_ec(what)[0]
    #return subprocess.Popen(what, shell=True, stdout=subprocess.PIPE).stdout.read()

#Execute and strip, return only stdout
def cmdshell_es(what):
    return cmdshell_ec(what)[0].strip()

#Execute and strip with status code
def cmdshell_esc(what):
    ret=cmdshell_ec(what)
    return ret[0].strip(),ret[1]
    
#exit if failed
def cmdshell_ef(what):
    out,code=cmdshell_ec(what)
    if code != 0:
        print >>sys.stderr, "FALLO en "+str(what) #TODO: stderr?
        print >>sys.stderr, out #TODO: stderr?
        
        exit(code)
    return out
    
def cmd_ec(what):
    if CMDSHELL: return cmdshell_ec(what)
    else: return ec(what)
#execute and return only stdout contents
def cmd_ex(what):
    if CMDSHELL: return cmdshell_ex(what)
    else: return ex(what)
#Execute and strip, return only stdout
def cmd_es(what):
    if CMDSHELL: return cmdshell_es(what)
    else: return es(what)
#Execute and strip with status code
def cmd_esc(what):
    if CMDSHELL: return cmdshell_esc(what)
    else: return esc(what)
#exit if failed
def cmd_ef(what):
    if CMDSHELL: return cmdshell_ef(what)
    else: return ef(what)

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

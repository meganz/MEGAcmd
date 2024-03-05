#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys, os, subprocess, shutil, logging
import ftplib
from megacmd_tests_common import *

GET="mega-get"
PUT="mega-put"
RM="mega-rm"
MV="mega-mv"
CP="mega-cp"
CD="mega-cd"
LCD="mega-lcd"
MKDIR="mega-mkdir"
EXPORT="mega-export -f"
FIND="mega-find"
INVITE="mega-invite"
IPC="mega-ipc"
FTP="mega-ftp"
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
 #   MEGA_EMAIL_AUX=os.environ["MEGA_EMAIL_AUX"]
 #   MEGA_PWD_AUX=os.environ["MEGA_PWD_AUX"]
except:
    #print >>sys.stderr, "You must define variables MEGA_EMAIL MEGA_PWD MEGA_EMAIL_AUX MEGA_PWD_AUX. WARNING: Use an empty account for $MEGA_EMAIL"
    logging.error("You must define variables MEGA_EMAIL MEGA_PWD. WARNING: Use an empty account for $MEGA_EMAIL")

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

    cmd_ec(RM+' -rf "*"')
    cmd_ec(RM+' -rf "//bin/*"')

    rmfolderifexisting("localUPs")
    rmfolderifexisting("localtmp")
    rmfolderifexisting("megaDls")
    rmfolderifexisting("localDls")

    rmfileifexisting("megafind.txt")
    rmfileifexisting("localfind.txt")

def clear_local_and_remote():
    rmfolderifexisting("localUPs")
    cmd_ec(RM+' -rf "/*"')
    clear_dls()
    initialize_contents()

def clear_dls():
    rmcontentsifexisting("megaDls")
    rmcontentsifexisting("localDls")

def compare_and_clear() :
    global currentTest

    megaDls=sort(find('megaDls'))
    localDls=sort(find('localDls'))

    if (megaDls == localDls):
        print("test "+str(currentTest)+" succesful!")
        if VERBOSE:
            print("test "+str(currentTest))
            print("megaDls:")
            print(megaDls)
            print()
            print("localDls:")
            print(localDls)
            print()
    else:
        print("test "+str(currentTest)+" failed!")

        print("megaDls:")
        print(megaDls)
        print()
        print("localDls:")
        print(localDls)
        print()
        exit(1)

    clear_dls()
    currentTest+=1
    cmd_ef(CD+" /")

def check_failed_and_clear(o,status):
    global currentTest

    if status == 0:
        print("test "+str(currentTest)+" failed!")
        print(o)
        exit(1)
    else:
        print("test "+str(currentTest)+" succesful!")

    clear_dls()
    currentTest+=1
    cmd_ef(CD+" /")

opts='";X()[]{}<>|`\''
if os.name == 'nt':
    opts=";()[]{}`'"

def initialize():

    if cmd_es(WHOAMI) != osvar("MEGA_EMAIL"):
        cmd_es(LOGOUT)
        cmd_ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))

    if len(os.listdir(".")):
        logging.error("initialization folder not empty!")
        #~ cd $ABSPWD
        exit(1)

    if cmd_es(FIND+" /") != b"/":
        logging.error("REMOTE Not empty, please clear it before starting!")
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
    #weird chars
    makedir("localtmp/odd")
    for f in ['localtmp/odd/'+a for a in ['file'+z for z in opts+"X"]]: touch(f+'01.txt')

    #~ localtmp/
    #~ ├── file01nonempty.txt
    #~ ├── file01.txt
    #~ ├── le01
    #~ │   ├── les01
    #~ │   │   └── less01
    #~ │   └── les02
    #~ │       ├── less01
    #~ │       └── less02
    #~ ├── lf01
    #~ │   ├── lfs01
    #~ │   │   └── lfss01
    #~ │   │       └── commonfile.txt
    #~ │   └── lfs02
    #~ │       ├── lfss01
    #~ │       │   └── commonfile.txt
    #~ │       └── lfss02
    #~ │           └── commonfile.txt
    #~ ├── ls 01
    #~ │   ├── ls s01
    #~ │   │   └── ls ss01
    #~ │   │       └── common file.txt
    #~ │   └── ls s02
    #~ │       ├── ls ss01
    #~ │       │   └── common file.txt
    #~ │       └── ls ss02
    #~ │           └── common file.txt
    #~ └── odd
        #~ ├── file;.txt
        #~ ├── ....

    #initialize dynamic contents:
    clear_local_and_remote()
    makedir('megaDls')
    makedir('localDls')

ABSMEGADLFOLDER=ABSPWD+'/megaDls'

def compare_find(what, localFindPrefix='localUPs'):
    global currentTest
    if VERBOSE:
        print("test "+str(currentTest))

    if not isinstance(what, list):
        what = [what]
    megafind=b""
    localfind=""
    for w in what:
        megafind+=cmd_ef(FIND+" "+w)+b"\n"
        localfind+=find(localFindPrefix+'/'+w,w)+"\n"

    megafind=sort(megafind).strip()
    localfind=sort(localfind).strip()

    #~ megafind=$FIND "$@"  | sort > $ABSPWD/megafind.txt
    #~ (cd localUPs 2>/dev/null; find "$@" | sed "s#\./##g" | sort) > $ABSPWD/localfind.txt
    if (megafind == localfind):
        if VERBOSE:
            print("diff megafind vs localfind:")
            #diff --side-by-side megafind.txt localfind.txt#TODO: do this
            print("MEGAFIND:")
            print(megafind)
            print("LOCALFIND")
            print(localfind)
        print("test "+str(currentTest)+" succesful!")
    else:
        print("test "+str(currentTest)+" failed!")
        print("diff megafind vs localfind:")
        #~ diff --side-by-side megafind.txt localfind.txt #TODO: do this
        print("MEGAFIND:")
        print(megafind)
        print("LOCALFIND")
        print(localfind)

        #cd $ABSPWD #TODO: consider this
        exit(1)

    currentTest+=1

def compare_remote_local(megafind, localfind):
    global currentTest
    if (isinstance(megafind, bytes)):
        megafind = megafind.decode()
    if (isinstance(localfind, bytes)):
        localfind = localfind.decode()
    if (megafind == localfind):
        if VERBOSE:
            print("diff megafind vs localfind:")
            #diff --side-by-side megafind.txt localfind.txt#TODO: do this
            print("MEGAFIND:")
            print(megafind)
            print("LOCALFIND")
            print(localfind)
        print("test "+str(currentTest)+" succesful!")
    else:
        print("test "+str(currentTest)+" failed!")
        print("diff megafind vs localfind:")
        #~ diff --side-by-side megafind.txt localfind.txt #TODO: do this
        print("MEGAFIND:")
        print(megafind)
        print("LOCALFIND")
        print(localfind)

        exit(1)

    currentTest+=1


def getFile(ftp, filename, destiny):
    try:
        ftp.retrbinary("RETR " + filename ,open(destiny, 'wb').write)
    except Exception as ex:
        print("Error: "+str(ex))
        exit(1)

def upload(ftp, source, destination):
    ext = os.path.splitext(source)[1]
    if ext in (".txt", ".htm", ".html"):
        ftp.storlines("STOR " + destination, open(source))
    else:
        ftp.storbinary("STOR " + destination, open(source, "rb"), 1024)

def lsftp():
    data=[]
    ftp.dir(data.append)
    toret=""
    for l in data:
        toret+=l[49:]+"\n"
    return toret

if VERBOSE: print("STARTING...")

#INITIALIZATION
clean_all()

initialize()

#ftp initialize connection
cmd_ec(FTP+' -d --all')
url=cmd_ef(FTP+' /').split(b' ')[-1]
server=url.split(b"//")[1].split(b":")[0]
port=url.split(b"//")[1].split(b":")[1].split(b"/")[0]
subpath=b"/"+b"/".join(url.split(b"//")[1].split(b":")[1].split(b"/")[1:])
subpath=subpath.replace(b'%20',b' ')
if VERBOSE:
    print(" connecting ... to "+server.decode()+" port="+port.decode()+" path="+subpath.decode())
ftp=ftplib.FTP()
ftp.connect(server.decode(),int(port.decode()), timeout=30)
ftp.login("anonymous", "nomatter")
ftp.cwd(subpath.decode())

currentTest=1

def executeTests():
    #Test 1 #/
    compare_remote_local(ftp.pwd(),subpath)

    #Test 2 #mkdir
    ftp.mkd('newfolder')
    makedir('localUPs/newfolder')
    compare_remote_local(sort(lsftp().strip()),sort(ls('localUPs').strip()))

    #Test 3
    getFile(ftp, 'file01nonempty.txt', 'megaDls/file01nonempty.txt')
    shutil.copy2('localUPs/file01nonempty.txt','localDls/')
    compare_and_clear()

    #Test 4 upload txt file
    upload(ftp, 'localUPs/file01nonempty.txt', 'newfile.txt')
    shutil.copy2('localUPs/file01nonempty.txt','localUPs/newfile.txt')
    compare_remote_local(sort(lsftp().strip()),sort(ls('localUPs').strip()))

    #Test 5 upload non txt file
    shutil.copy2('localUPs/file01nonempty.txt','localUPs/newfile')
    upload(ftp, 'localUPs/newfile', 'newfile')
    compare_remote_local(sort(lsftp().strip()),sort(ls('localUPs').strip()))

    #Test 6 rename
    shutil.move('localUPs/lf01/lfs01/lfss01/commonfile.txt','localUPs/lf01/lfs01/lfss01/renamed.txt')
    ftp.rename('lf01/lfs01/lfss01/commonfile.txt', 'lf01/lfs01/lfss01/renamed.txt')
    compare_remote_local(sort(lsftp().strip()),sort(ls('localUPs').strip()))

    #Test 7 rename folder
    shutil.move('localUPs/lf01/lfs01/lfss01','localUPs/lf01/lfs01/lfss01_renamed')
    ftp.rename('lf01/lfs01/lfss01', 'lf01/lfs01/lfss01_renamed')
    compare_remote_local(sort(lsftp().strip()),sort(ls('localUPs').strip()))

    #Test 8 remove file
    os.remove('localUPs/lf01/lfs02/lfss01/commonfile.txt')
    ftp.delete('lf01/lfs02/lfss01/commonfile.txt')
    compare_remote_local(sort(lsftp().strip()),sort(ls('localUPs').strip()))

    #Test 9 remove folder
    shutil.rmtree('localUPs/lf01/lfs02/lfss02')
    ftp.rmd('lf01/lfs02/lfss02')
    compare_remote_local(sort(lsftp().strip()),sort(ls('localUPs').strip()))

executeTests()

#FTPS
#~ if ftp != None: ftp.close()
#~ cmd_ec(FTP+' -d --all')
#~ url=cmd_ef(FTP+' / --tls --certificate=/assets/others/certs/selfsignedSDK/pepitopalotes.pem --key=/assets/others/certs/selfsignedSDK/pepitopalotes.key --port=21  ').split(' ')[-1]
#~ server=url.split("//")[1].split(":")[0]
#~ #server="pepitopalotes"
#~ port=url.split("//")[1].split(":")[1].split("/")[0]
#~ subpath="/"+"/".join(url.split("//")[1].split(":")[1].split("/")[1:])
#~ subpath=subpath.replace('%20',' ')

#~ if VERBOSE:
    #~ print " connecting ... to "+server+" port="+port+" path="+subpath
#~ ftp=ftplib.FTP_TLS()
#~ ftp.connect(server,port, timeout=5)
#~ ftp.login("anonymous", "nomatter") #this fails!
#~ ftp.cwd(subpath)

#~ currentTest=100
#~ executeTests()

ftp.close()
###################

# Clean all
if not VERBOSE:
    clean_all()

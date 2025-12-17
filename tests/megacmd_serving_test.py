#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys, os, shutil
import ftplib
import unittest
import xmlrunner
from megacmd_tests_common import *

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

def check_failed_and_clear(self, o,status):
    self.assertNotEqual(status, 0)
    if status == 0:
        print(o)

    clear_dls()
    cmd_ef(CD+" /")

opts='";X()[]{}<>|`\''
if os.name == 'nt':
    opts=";()[]{}`'"

def initialize():
    if cmd_es(WHOAMI) != osvar("MEGA_EMAIL"):
        cmd_es(LOGOUT)
        cmd_ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))

    if len(os.listdir(".")):
        print("initialization folder not empty!", file=sys.stderr)
        #~ cd $ABSPWD
        exit(1)

    if cmd_es(FIND+" /") != b"/":
        print("REMOTE Not empty, please clear it before starting!", file=sys.stderr)
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

#ABSMEGADLFOLDER=ABSPWD+'/megaDls'


class MEGAcmdServingTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if VERBOSE: print("STARTING...")

        clean_all()
        initialize()

        # Initialize FTP connection
        cmd_ec(FTP+' -d --all')
        url=cmd_ef(FTP+' /').split(b' ')[-1]
        server=url.split(b"//")[1].split(b":")[0]
        port=url.split(b"//")[1].split(b":")[1].split(b"/")[0]
        cls.subpath=b"/"+b"/".join(url.split(b"//")[1].split(b":")[1].split(b"/")[1:])
        cls.subpath=cls.subpath.replace(b'%20',b' ')

        if VERBOSE:
            print(" connecting ... to "+server.decode()+" port="+port.decode()+" path="+cls.subpath.decode())
        cls.ftp=ftplib.FTP()
        cls.ftp.connect(server.decode(),int(port.decode()), timeout=30)
        cls.ftp.login("anonymous", "nomatter")
        cls.ftp.cwd(cls.subpath.decode())

    @classmethod
    def tearDownClass(cls):
        cls.ftp.close()
        if not VERBOSE:
            clean_all()

    def setUp(self):
        print(f"\n=== Running test: {self._testMethodName} ===")

    def compare_find(self, what, localFindPrefix='localUPs'):
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
        self.assertEqual(megafind, localfind)
        if (megafind == localfind):
            if VERBOSE:
                print("diff megafind vs localfind:")
                #diff --side-by-side megafind.txt localfind.txt#TODO: do this
                print("MEGAFIND:")
                print(megafind)
                print("LOCALFIND")
                print(localfind)
        else:
            print("diff megafind vs localfind:")
            #~ diff --side-by-side megafind.txt localfind.txt #TODO: do this
            print("MEGAFIND:")
            print(megafind)
            print("LOCALFIND")
            print(localfind)

            #cd $ABSPWD #TODO: consider this

    def compare_remote_local(self, megafind, localfind):
        if (isinstance(megafind, bytes)):
            megafind = megafind.decode()
        if (isinstance(localfind, bytes)):
            localfind = localfind.decode()

        self.assertEqual(megafind, localfind)
        if (megafind == localfind):
            if VERBOSE:
                print("diff megafind vs localfind:")
                #diff --side-by-side megafind.txt localfind.txt#TODO: do this
                print("MEGAFIND:")
                print(megafind)
                print("LOCALFIND")
                print(localfind)
        else:
            print("diff megafind vs localfind:")
            #~ diff --side-by-side megafind.txt localfind.txt #TODO: do this
            print("MEGAFIND:")
            print(megafind)
            print("LOCALFIND")
            print(localfind)

    def compare_and_clear(self):
        megaDls=sort(find('megaDls'))
        localDls=sort(find('localDls'))

        self.assertEqual(megaDls, localDls)
        if (megaDls == localDls):
            if VERBOSE:
                print("megaDls:")
                print(megaDls)
                print()
                print("localDls:")
                print(localDls)
                print()
        else:
            print("megaDls:")
            print(megaDls)
            print()
            print("localDls:")
            print(localDls)
            print()
            exit(1)

        clear_dls()
        cmd_ef(CD+" /")

    def getFile(self, filename, destiny):
        try:
            self.ftp.retrbinary("RETR " + filename ,open(destiny, 'wb').write)
        except Exception as ex:
            print("Error: "+str(ex))
            exit(1)

    def upload(self, source, destination):
        ext = os.path.splitext(source)[1]
        if ext in (".txt", ".htm", ".html"):
            self.ftp.storlines("STOR " + destination, open(source, "rb", 1024))
        else:
            self.ftp.storbinary("STOR " + destination, open(source, "rb"), 1024)

    def lsftp(self):
        data=[]
        self.ftp.dir(data.append)
        toret=""
        for l in data:
            toret+=l[49:]+"\n"
        return toret

    def test_01_compare_root(self):
        self.compare_remote_local(self.ftp.pwd(), self.subpath)

    def test_02_mkdir(self):
        self.ftp.mkd('newfolder')
        makedir('localUPs/newfolder')
        self.compare_remote_local(sort(self.lsftp().strip()),sort(ls('localUPs').strip()))

    def test_03_download_txt_file(self):
        self.getFile('file01nonempty.txt', 'megaDls/file01nonempty.txt')
        shutil.copy2('localUPs/file01nonempty.txt','localDls/')
        self.compare_and_clear()

    def test_04_upload_txt_file(self):
        self.upload('localUPs/file01nonempty.txt', 'newfile.txt')
        shutil.copy2('localUPs/file01nonempty.txt','localUPs/newfile.txt')
        self.compare_remote_local(sort(self.lsftp().strip()),sort(ls('localUPs').strip()))

    def test_05_upload_non_txt_file(self):
        shutil.copy2('localUPs/file01nonempty.txt','localUPs/newfile')
        self.upload('localUPs/newfile', 'newfile')
        self.compare_remote_local(sort(self.lsftp().strip()),sort(ls('localUPs').strip()))

    def test_06_rename(self):
        shutil.move('localUPs/lf01/lfs01/lfss01/commonfile.txt','localUPs/lf01/lfs01/lfss01/renamed.txt')
        self.ftp.rename('lf01/lfs01/lfss01/commonfile.txt', 'lf01/lfs01/lfss01/renamed.txt')
        self.compare_remote_local(sort(self.lsftp().strip()),sort(ls('localUPs').strip()))

    def test_07_rename_folder(self):
        shutil.move('localUPs/lf01/lfs01/lfss01','localUPs/lf01/lfs01/lfss01_renamed')
        self.ftp.rename('lf01/lfs01/lfss01', 'lf01/lfs01/lfss01_renamed')
        self.compare_remote_local(sort(self.lsftp().strip()),sort(ls('localUPs').strip()))

    def test_08_remove_file(self):
        os.remove('localUPs/lf01/lfs02/lfss01/commonfile.txt')
        self.ftp.delete('lf01/lfs02/lfss01/commonfile.txt')
        self.compare_remote_local(sort(self.lsftp().strip()),sort(ls('localUPs').strip()))

    def test_09_remove_folder(self):
        shutil.rmtree('localUPs/lf01/lfs02/lfss02')
        self.ftp.rmd('lf01/lfs02/lfss02')
        self.compare_remote_local(sort(self.lsftp().strip()),sort(ls('localUPs').strip()))

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

#~ executeTests()

###################

if __name__ == '__main__':
    if "OUT_DIR_JUNIT_XML" in os.environ:
        unittest.main(testRunner=xmlrunner.XMLTestRunner(output=os.environ["OUT_DIR_JUNIT_XML"]), failfast=False, buffer=False, catchbreak=False)
    else:
        unittest.main()


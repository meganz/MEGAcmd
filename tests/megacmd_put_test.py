#!/usr/bin/python3
# -*- coding: utf-8 -*-
#better run in an empty folder

import os, shutil, platform
import unittest
import xmlrunner
from megacmd_tests_common import *

def setUpModule():
    global ABSPWD
    global ABSMEGADLFOLDER

    ABSPWD = os.getcwd()
    ABSMEGADLFOLDER = ABSPWD+'/megaDls'

def clean_all():
    if not clean_root_confirmed_by_user():
        raise Exception("Tests need to be run with YES_I_KNOW_THIS_WILL_CLEAR_MY_MEGA_ACCOUNT=1")

    if cmd_es(WHOAMI) != osvar("MEGA_EMAIL"):
        cmd_ef(LOGOUT)
        cmd_ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))

    cmd_ec(RM+' -rf "/*"')
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

currentTest=1

def check_failed_and_clear(o,status):
    global currentTest

    if status == 0:
        print("test "+str(currentTest)+" failed!")
        print(o)
        exit(1)
    else:
        print("test "+str(currentTest)+" succesful!")

    clear_local_and_remote()
    currentTest+=1
    cmd_ef(CD+" /")

def initialize():
    if cmd_es(WHOAMI) != osvar("MEGA_EMAIL"):
        cmd_ef(LOGOUT)
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
    cmd_ef(MKDIR+" -p "+" ".join(remotefolders))
    for f in ['localUPs/01/'+a for a in ['s01/ss01']+ ['s02/ss0'+z for z in ['1','2']] ]: makedir(f)

class MEGAcmdPutTests(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        clean_all()
        initialize()
        clear_local_and_remote()

    @classmethod
    def tearDownClass(cls):
        if not VERBOSE:
            clean_all()

    def setUp(self):
        print(f"\n=== Running test: {self._testMethodName} ===")

    def compare_and_clear(self) :
        def cleanup():
            clear_local_and_remote()
            cmd_ef(CD+ " /")

        megafind=sort(cmd_ef(FIND))
        localfind=sort(find('localUPs','.'))
        self.addCleanup(cleanup)
        self.assertEqual(megafind, localfind)

    def test_clean_comparison(self):
        #Test 01 #clean comparison
        self.compare_and_clear()

    def test_no_dest_empty_file_upload(self):
        #Test 02 #no destiny empty file upload
        cmd_ef(PUT+' '+'localtmp/file01.txt')
        shutil.copy2('localtmp/file01.txt','localUPs/')
        self.compare_and_clear()

    def test_dest_empty_file_upload(self):
        #Test 03 #/ destiny empty file upload
        cmd_ef(PUT+' '+'localtmp/file01.txt /')
        shutil.copy2('localtmp/file01.txt','localUPs/')
        self.compare_and_clear()

    def test_no_dest_non_empty_file_upload(self):
        #Test 04 #no destiny non empty file upload
        cmd_ef(PUT+' '+'localtmp/file01nonempty.txt')
        shutil.copy2('localtmp/file01nonempty.txt','localUPs/')
        self.compare_and_clear()

    def test_update_non_empty_file_upload(self):
        #Test 05 #update non empty file upload
        out('newfile01contents', 'localtmp/file01nonempty.txt')
        cmd_ef(PUT+' '+'localtmp/file01nonempty.txt')
        shutil.copy2('localtmp/file01nonempty.txt','localUPs/file01nonempty.txt')
        self.compare_and_clear()

    def test_empty_folder(self):
        #Test 06 #empty folder
        cmd_ef(PUT+' '+'localtmp/le01/les01/less01')
        copyfolder('localtmp/le01/les01/less01','localUPs/')
        self.compare_and_clear()

    def test_1_file_folder(self):
        #Test 07 #1 file folder
        cmd_ef(PUT+' '+'localtmp/lf01/lfs01/lfss01')
        copyfolder('localtmp/lf01/lfs01/lfss01','localUPs/')
        self.compare_and_clear()

    def test_entire_empty_folder_structure(self):
        #Test 08 #entire empty folders structure
        cmd_ef(PUT+' '+'localtmp/le01')
        copyfolder('localtmp/le01','localUPs/')
        self.compare_and_clear()

    def test_entire_non_empty_folder_structure(self):
        #Test 09 #entire non empty folders structure
        cmd_ef(PUT+' '+'localtmp/lf01')
        copyfolder('localtmp/lf01','localUPs/')
        self.compare_and_clear()

    def test_copy_structure_into_subfolder(self):
        #Test 10 #copy structure into subfolder
        cmd_ef(PUT+' '+'localtmp/le01 /01/s01')
        copyfolder('localtmp/le01','localUPs/01/s01')
        self.compare_and_clear()

    def test_copy_exact_structure(self):
        #~ #Test 11 #copy exact structure
        makedir('auxx')
        copyfolder('localUPs/01','auxx')
        cmd_ef(PUT+' '+'auxx/01/s01 /01/s01')
        copyfolder('auxx/01/s01','localUPs/01/s01')
        rmfolderifexisting("auxx")
        self.compare_and_clear()

    def test_merge_increased_structure(self):
        #~ #Test 12 #merge increased structure
        makedir('auxx')
        copyfolder('localUPs/01','auxx')
        touch('auxx/01/s01/another.txt')
        cmd_ef(PUT+' '+'auxx/01/s01 /01/')
        shutil.copy2('auxx/01/s01/another.txt','localUPs/01/s01')
        self.compare_and_clear()
        rmfolderifexisting("auxx")

    def test_multiple_upload(self):
        #Test 13 #multiple upload
        cmd_ef(PUT+' '+'localtmp/le01 localtmp/lf01 /01/s01')
        copyfolder('localtmp/le01','localUPs/01/s01')
        copyfolder('localtmp/lf01','localUPs/01/s01')
        self.compare_and_clear()

    @unittest.skipIf(platform.system() == "Windows" or CMDSHELL, "skipping test")
    def test_local_regexp(self):
        #Test 14 #local regexp
        cmd_ef(PUT+' '+'localtmp/*txt /01/s01')
        copybyfilepattern('localtmp/','*.txt','localUPs/01/s01')
        self.compare_and_clear()

    def test_prev_dir(self):
        """../"""
        #Test 15 #../
        cmd_ef(CD+' 01')
        cmd_ef(PUT+' '+'localtmp/le01 ../01/s01')
        cmd_ef(CD+' /')
        copyfolder('localtmp/le01','localUPs/01/s01')
        self.compare_and_clear()


    def test_spaces(self):
        #Test 16 #spaced stuff
        if CMDSHELL: #TODO: think about this again
            cmd_ef(PUT+' '+'localtmp/ls\\ 01')
        else:
            cmd_ef(PUT+' '+'"localtmp/ls 01"')
        copyfolder('localtmp/ls 01','localUPs')
        self.compare_and_clear()

if __name__ == '__main__':
    if "OUT_DIR_JUNIT_XML" in os.environ:
        unittest.main(testRunner=xmlrunner.XMLTestRunner(output=os.environ["OUT_DIR_JUNIT_XML"]), failfast=False, buffer=False, catchbreak=False)
    else:
        unittest.main()

###TODO: do stuff in shared folders...

########

##Test XX #merge structure with file updated
##This test fails, because it creates a remote copy of the updated file.
##That's expected. If ever provided a way to create a real merge (e.g.: put -m ...)  to do BACKUPs, reuse this test.
#mkdir aux
#shutil.copy2('-pr localUPs/01','aux')
#touch aux/01/s01/another.txt
#cmd_ef(PUT+' '+'aux/01/s01 /01/')
#rsync -aLp aux/01/s01/ localUPs/01/s01/
#echo "newcontents" > aux/01/s01/another.txt
#cmd_ef(PUT+' '+'aux/01/s01 /01/')
#rsync -aLp aux/01/s01/ localUPs/01/s01/
#rm -r aux
#compare_and_clear()


# Clean all

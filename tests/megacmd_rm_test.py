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
    contents=" ".join(['"localtmp/'+x+'"' for x in os.listdir('localtmp/')])
    cmd_ef(PUT+" "+contents+" /")
    makedir('localUPs')
    copybypattern('localtmp/','*','localUPs')

class MEGAcmdRmTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        #INITIALIZATION
        clean_all()
        initialize()
        clear_local_and_remote()

    @classmethod
    def tearDownClass(cls):
        if not VERBOSE:
            clean_all()

    def setUp(self):
        print(f"\n=== Running test: {self._testMethodName} ===")

    def compare_and_clear(self):
        def cleanup():
            clear_local_and_remote()
            cmd_ef(CD+" /")

        megafind=sort(cmd_ef(FIND))
        localfind=sort(find('localUPs','.'))
        self.addCleanup(cleanup)
        self.assertEqual(megafind, localfind)
        print(f"megafind: {megafind}, localfind: {localfind}")

    def test_01_clean_comparison(self):
        #Test 01 #clean comparison
        self.compare_and_clear()

    def test_02_dest_empty_file(self):
        #Test 02 #destiny empty file
        cmd_ef(RM+' '+'file01.txt')
        rmfileifexisting('localUPs/file01.txt')
        self.compare_and_clear()

    def test_03_dest_empty_file_upload(self):
        #Test 03 #/ destiny empty file upload
        cmd_ef(PUT+' '+'localtmp/file01.txt /')
        shutil.copy2('localtmp/file01.txt','localUPs')
        self.compare_and_clear()

    def test_04_no_dest_non_empty_file_upload(self):
        #Test 04 #no destiny nont empty file upload
        cmd_ef(RM+' '+'file01nonempty.txt')
        rmfileifexisting('localUPs/file01nonempty.txt')
        self.compare_and_clear()

    def test_05_empty_folder(self):
        #Test 05 #empty folder
        cmd_ef(RM+' '+'-rf le01/les01/less01')
        rmfolderifexisting('localUPs/le01/les01/less01')
        self.compare_and_clear()

    def test_06_1_file_folder(self):
        #Test 06 #1 file folder
        cmd_ef(RM+' '+'-rf lf01/lfs01/lfss01')
        rmfolderifexisting('localUPs/lf01/lfs01/lfss01')
        self.compare_and_clear()

    def test_07_entire_empty_folders_structure(self):
        #Test 07 #entire empty folders structure
        cmd_ef(RM+' '+'-rf le01')
        rmfolderifexisting('localUPs/le01')
        self.compare_and_clear()

    def test_08_entire_non_empty_folders_structure(self):
        #Test 08 #entire non empty folders structure
        cmd_ef(RM+' '+'-rf lf01')
        rmfolderifexisting('localUPs/lf01')
        self.compare_and_clear()

    def test_09_multiple(self):
        #Test 09 #multiple
        cmd_ef(RM+' '+'-rf lf01 le01/les01')
        rmfolderifexisting('localUPs/lf01')
        rmfolderifexisting('localUPs/le01/les01')
        self.compare_and_clear()

    def test_10_current_wd(self):
        #Test 10 #.
        cmd_ef(CD+' '+'le01')
        cmd_ef(RM+' '+'-rf .')
        cmd_ef(CD+' '+'/')
        rmfolderifexisting('localUPs/le01')
        self.compare_and_clear()

    def test_11_prev_dir(self):
        #Test 11 #..
        cmd_ef(CD+' '+'le01/les01')
        cmd_ef(RM+' '+'-rf ..')
        cmd_ef(CD+' '+'/')
        rmfolderifexisting('localUPs/le01')
        self.compare_and_clear()

    def test_12_prev_dir_2(self):
        """../XX"""
        #Test 12 #../XX
        cmd_ef(CD+' '+'le01/les01')
        cmd_ef(RM+' '+'-rf ../les01')
        cmd_ef(CD+' '+'/')
        rmfolderifexisting('localUPs/le01/les01')
        self.compare_and_clear()


    def test_13_spaces(self):
        #Test 13 #spaced stuff
        cmd_ef(RM+' '+'-rf "ls 01"')
        rmfolderifexisting('localUPs/ls 01')
        self.compare_and_clear()

    def test_14_complex(self):
        #Test 14 #complex stuff
        cmd_ef(RM+' '+'-rf "ls 01/../le01/les01" "lf01/../ls*/ls s02"')
        rmfolderifexisting('localUPs/ls 01/../le01/les01')
        [rmfolderifexisting('localUPs/lf01/../'+f+'/ls s02') for f in os.listdir('localUPs/lf01/..') if f.startswith('ls')]
        self.compare_and_clear()

    @unittest.skipIf(platform.system() == "Windows", "skipping for Windows systems")
    def test_15_complex_pcre_expr(self):
        #Test 15 #complex stuff with PCRE exp
        cmd_ef(RM+' '+'-rf --use-pcre "ls 01/../le01/les0[12]" "lf01/../ls.*/ls s0[12]"')
        rmfolderifexisting('localUPs/ls 01/../le01/les01')
        rmfolderifexisting('localUPs/ls 01/../le01/les02')
        [rmfolderifexisting('localUPs/lf01/../'+f+'/ls s01') for f in os.listdir('localUPs/lf01/..') if f.startswith('ls')]
        [rmfolderifexisting('localUPs/lf01/../'+f+'/ls s02') for f in os.listdir('localUPs/lf01/..') if f.startswith('ls')]
        self.compare_and_clear()

    def test_16_spaces_2(self):
        #Test 16 #spaced stuff2
        cmd_ef(RM+' '+'-rf ls\\ 01')
        rmfolderifexisting('localUPs/ls 01')
        self.compare_and_clear()

if __name__ == '__main__':
    if "OUT_DIR_JUNIT_XML" in os.environ:
        unittest.main(testRunner=xmlrunner.XMLTestRunner(output=os.environ["OUT_DIR_JUNIT_XML"]), failfast=False, buffer=False, catchbreak=False)
    else:
        unittest.main()

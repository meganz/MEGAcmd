#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os, shutil
import unittest
import xmlrunner
from megacmd_tests_common import *

def setUpModule():
    global ABSPWD
    ABSPWD = os.getcwd()

def initialize_contents():
    contents=" ".join(['"localtmp/'+x+'"' for x in os.listdir('localtmp/')])
    cmd_ef(PUT+" "+contents+" /")
    shutil.copytree('localtmp', 'localUPs')

def clean_all():
    if not clean_root_confirmed_by_user():
        raise Exception("Tests need to be run with YES_I_KNOW_THIS_WILL_CLEAR_MY_MEGA_ACCOUNT=1")

    if cmd_es(WHOAMI) != osvar("MEGA_EMAIL"):
        cmd_ef(LOGOUT)
        cmd_ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))

    #~ rm pipe > /dev/null 2>/dev/null || :

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

def initialize():
    if cmd_es(WHOAMI) != osvar("MEGA_EMAIL"):
        cmd_es(LOGOUT)
        cmd_ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))

    if len(os.listdir(".")):
        raise Exception("initialization folder not empty!")
        #~ cd $ABSPWD

    if cmd_es(FIND+" /") != b"/":
        raise Exception("REMOTE Not empty, please clear it before starting!")


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

class MEGAcmdFindTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        #INITIALIZATION
        clean_all()
        initialize()

    @classmethod
    def tearDownClass(cls):
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

    def test_empty_file(self):
        self.compare_find('file01.txt')

    def test_entire_empty_folders(self):
        self.compare_find('le01/les01/less01')

    def test_1_file_folder(self):
        self.compare_find('lf01/lfs01/lfss01')

    def test_entire_non_empty_folders_structure(self):
        self.compare_find('lf01')

    def test_multiple_names(self):
        self.compare_find(['lf01','le01/les01'])

    def test_current_wd(self):
        """."""
        cmd_ef(CD+" le01")
        self.compare_find('.','localUPs/le01')
        cmd_ef(CD+" /")

    def test_current_wd_global(self):
        """. global"""
        self.compare_find('.')

    def test_spaced(self):
        megafind=sort(cmd_ef(FIND+" "+"ls\\ 01"))
        localfind=sort(find('localUPs/ls 01',"ls 01"))
        self.assertEqual(megafind,localfind)

    def test_multiple(self):
        """XX/.."""
        megafind=sort(cmd_ef(FIND+" "+"ls\\ 01/.."))
        localfind=sort(find('localUPs/',"/"))
        self.assertEqual(megafind,localfind)

    def test_multiple_2(self):
        cmd_ef(CD+' le01')
        megafind=sort(cmd_ef(FIND+" "+".."))
        cmd_ef(CD+' /')
        localfind=sort(find('localUPs/',"/"))
        self.assertEqual(megafind,localfind)

    def test_complex(self):
        megafind=sort(cmd_ef(FIND+" "+"ls\\ 01/../le01/les01" +" " +"lf01/../ls\\ *01/ls\\ s02"))
        localfind=sort(find('localUPs/le01/les01',"/le01/les01"))
        localfind+="\n"+sort(find('localUPs/ls 01/ls s02',"/ls 01/ls s02"))
        self.assertEqual(megafind,localfind)

    def test_inside_folder(self):
        megafind=sort(cmd_ef(FIND+" "+"le01/"))
        localfind=sort(find('localUPs/le01',"le01"))
        self.assertEqual(megafind,localfind)

    def test_non_existent(self):
        file = 'file01.txt/non-existent'
        out, status, err = cmd_ec(f'{FIND} {file}')
        if not CMDSHELL:
            self.assertNotEqual(status, 0)
        self.assertIn(f'{file}: no such file or directory', err.decode().lower())

    def test_root(self):
        self.compare_find('/')

###TODO: do stuff in shared folders...

if __name__ == '__main__':
    if "OUT_DIR_JUNIT_XML" in os.environ:
        unittest.main(testRunner=xmlrunner.XMLTestRunner(output=os.environ["OUT_DIR_JUNIT_XML"]), failfast=False, buffer=False, catchbreak=False)
    else:
        unittest.main()

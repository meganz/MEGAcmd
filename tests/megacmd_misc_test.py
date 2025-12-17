#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys, os, shutil, filecmp
import unittest
import xmlrunner
from megacmd_tests_common import *

def setUpModule():
    global ABSPWD
    global ABSMEGADLFOLDER
    ABSPWD = os.getcwd()
    ABSMEGADLFOLDER=ABSPWD+'/megaDls'

def initialize_contents():
    cmd_ec(INVITE+" "+osvar("MEGA_EMAIL_AUX"))
    cmd_ef(LOGOUT)
    cmd_ef(LOGIN+" " +osvar("MEGA_EMAIL_AUX")+" "+osvar("MEGA_PWD_AUX"))
    cmd_ec(IPC+" -a "+osvar("MEGA_EMAIL"))
    cmd_ef(LOGOUT)
    cmd_ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))

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
    rmfileifexisting("thumbnail.jpg")



def clear_local_and_remote():
    rmfolderifexisting("localUPs")
    cmd_ec(RM+' -rf "/*"')
    initialize_contents()

opts='";X()[]{}<>|`\''
if os.name == 'nt':
    opts=";()[]{}`'"

def initialize():

    if cmd_es(WHOAMI) != osvar("MEGA_EMAIL"):
        cmd_es(LOGOUT)
        cmd_ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))

    if len(os.listdir(".")) and ( len(os.listdir(".")) != 1 and os.listdir(".")[0] != 'images'):
        print("initialization folder not empty!", "\n",os.listdir("."), file=sys.stderr)
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

if VERBOSE: print("STARTING...")

class MEGAcmdMiscTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bar = ""

        if not CMDSHELL:
            cls.bar="\\"
        if os.name == 'nt':
            cls.bar=""

        cls.imagesUrl = os.environ.get('MEGACMD_TESTS_IMAGES_URL', "https://mega.nz/folder/bxomFKwL#3V1dUJFzL98t1GqXX29IXg")
        cls.pdfsURL = os.environ.get('MEGACMD_TESTS_PDFS_URL', "https://mega.nz/folder/D0w0nYiY#egvjqP5R-anbBdsJg8QRVg")

        clean_all()
        initialize()

    @classmethod
    def tearDownClass(cls):
        if not VERBOSE:
            clean_all()

    def setUp(self):
        print(f"\n=== Running test: {self._testMethodName} ===")

    def compare_remote_local(self, megafind, localfind):
        self.assertEqual(megafind, localfind)
        print(f"megafind: {megafind}, localfind: {localfind}")

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
        print(f"megafind: {megafind}, localfind: {localfind}")

    def check_failed_and_clear(self, o,status):
        self.addCleanup(lambda : cmd_ef(CD+" /"))
        self.assertNotEqual(status, 0, o)

    def test_01_special_characters(self):
        for c in opts:
            with self.subTest(c=c):
                megafind=sort(cmd_ef(FIND+" "+'odd/file'+self.bar+c+'01.txt'))
                self.compare_remote_local(megafind,'odd/file'+c+'01.txt')

    @unittest.skipIf(os.name == "nt", "only for non NT environments")
    def test_02_unix_special_characters(self):
        def cleanup():
            #revert original situation
            cmd_ef(MV+" "+'odd/file\\\\01.txt odd/fileX01.txt')
            shutil.move('localUPs/odd/file\\01.txt', 'localUPs/odd/fileX01.txt')

        #Test 15 # very special \ character
        c='\\'
        cmd_ef(MV+" "+'odd/fileX01.txt odd/file'+self.bar+'\\01.txt') #upload of file containing "\\" fails (SDK issue)
        shutil.move('localUPs/odd/fileX01.txt', 'localUPs/odd/file\\01.txt')
        megafind=sort(cmd_ef(FIND+" "+'odd/file'+self.bar+c+'01.txt'))
        self.addCleanup(cleanup)
        self.compare_remote_local(megafind,'odd/file'+c+'01.txt')

    def test_03_move_and_rename(self):
        def cleanup():
            #revert original situation
            cmd_ef(MV+' /le01/moved.txt'+" file01nonempty.txt ")
            shutil.move("localUPs/le01/moved.txt", "localUPs/file01nonempty.txt")

        #Test 16 #move & rename
        cmd_ef(MV+" file01nonempty.txt "+'/le01/moved.txt')
        shutil.move("localUPs/file01nonempty.txt", "localUPs/le01/moved.txt")
        self.addCleanup(cleanup)
        self.compare_find('/')

    def test_04_new_empty_file(self):
        #Test 17
        touch("localUPs/newemptyfile.txt")
        cmd_ef(PUT+" "+"localUPs/newemptyfile.txt"+" "+"/")
        #shutil.copy2('auxx/01/s01/another.txt','localUPs/01/s01')
        self.compare_find('/')

    @unittest.skipIf(os.name == "nt", "only for non NT environments")
    def test_05_unicode(self):
        for fname in UNICODE_NAME_LIST:
            touch("localUPs/"+fname)
            cmd_ef(PUT+" "+"localUPs/"+fname+" "+"/")
        self.compare_find('/')

    def test_06_copy_and_rename_2(self):
        #Test 19 #copy & rename
        cmd_ef(CP+" file01nonempty.txt "+'/le01/copied')
        copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/le01/copied")
        self.compare_find('/')

    def test_07_multicopy(self):
        #Test 20 #multicopy
        cmd_ef(CP+" *.txt "+'/le01')
        copybyfilepattern("localUPs/","*.txt", "localUPs/le01/")
        self.compare_find('/')

    def test_08_multicopy_with_trailing_slash(self):
        #Test 21 #multicopy with trailing /
        cmd_ef(CP+" *.txt "+'/le01/les01/')
        copybyfilepattern("localUPs/","*.txt", "localUPs/le01/les01/")
        self.compare_find('/')

    # ~ #Test 22 #multisend
    # ~ cmd_ef(CP+" *.txt "+MEGA_EMAIL_AUX+':')
    # ~ print "test "+str(currentTest)+" succesful!"
    # ~ currentTest+=1

    def test_09_copy_folder(self):
        #Test 23 #copy folder
        cmd_ef(CP+" le01 "+'lf01')
        copyfolder("localUPs/le01", "localUPs/lf01/")
        self.compare_find('/')

    def test_10_send_to_non_contact(self):
        o, status, e = cmd_ec(f'{CP} *.txt badContact{MEGA_EMAIL_AUX}:')
        if not CMDSHELL:
            self.check_failed_and_clear(o, status)
        self.assertIn('failed to send file to user: not found', e.decode().lower())

    def test_11_multicopy_into_file(self):
        bad_folder = '/le01/file01nonempty.txt'
        o, status, e = cmd_ec(f'{CP} *.txt {bad_folder}')
        if not CMDSHELL:
            self.check_failed_and_clear(o, status)
        self.assertIn(f'{bad_folder} must be a valid folder', e.decode().lower())

    def test_12_copy_into_existing_file(self):
        #Test 25 #copy into existing file
        cmd_ef(CP+" -vvv file01nonempty.txt "+'copied2')
        cmd_ef(CP+" -vvv file01nonempty.txt "+'copied2')
        copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/copied2")
        self.compare_find('/')

    def test_13_move_into_non_existent_file(self):
        #Test 26 #move into non existing file
        cmd_ef(CP+" -vvv file01nonempty.txt "+'copied3')
        cmd_ef(MV+" -vvv copied3 "+'moved3')
        copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/moved3")
        self.compare_find('/')

    def test_14_move_into_existing_different_file(self):
        #Test 28 #move into existing (different) file
        cmd_ef(CP+" -vvv file01nonempty.txt "+'copied4')
        cmd_ef(CP+" -vvv file01.txt "+'moved4')
        cmd_ef(MV+" -vvv copied4 "+'moved4')
        copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/moved4")
        self.compare_find('/')

    def test_15_move_into_existing_equal_file(self):
        #Test 29 #move into existing equal file
        cmd_ef(CP+" -vvv file01nonempty.txt "+'copied5')
        cmd_ef(CP+" -vvv copied5 "+'moved5')
        cmd_ef(MV+" -vvv copied5 "+'moved5')
        copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/moved5")
        self.compare_find('/')

    def test_16_move_into_other_path(self):
        #Test 30 #move into other path
        cmd_ef(CP+" -vvv file01nonempty.txt "+'copied6')
        cmd_ef(MV+" -vvv copied6 "+'/le01/moved6')
        copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/le01/moved6")
        self.compare_find('/')

    def test_17_move_existing_other_path_different_file(self):
        #Test 31 #move existing other path (different file)
        cmd_ef(CP+" -vvv file01nonempty.txt "+'copied7')
        cmd_ef(CP+" -vvv file01.txt "+'/le01/moved7')
        cmd_ef(MV+" -vvv copied7 "+'/le01/moved7')
        copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/le01/moved7")
        self.compare_find('/')

    def test_18_move_existing_other_path(self):
        #Test 32 #move existing other path
        cmd_ef(CP+" -vvv file01nonempty.txt "+'copied7')
        cmd_ef(CP+" -vvv file01nonempty.txt "+'/le01/moved7')
        cmd_ef(MV+" -vvv copied7 "+'/le01/moved7')
        copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/le01/moved7")
        self.compare_find('/')

    def test_19_thumbnails(self):
        #Test 33 #ensure thumnail generation
        #1st get images selection
        print(f"using images URL: {self.imagesUrl}")
        cmd_ef(GET+" "+self.imagesUrl+" localtmp/")
        #2nd, upload folder
        cmd_ef(PUT+" localtmp/images")
        #3rd, for each file, download thumbnail
        folder="/images"
        o,status,e=cmd_ec(FIND+" "+folder+"/*")
        fullout=""
        fullStatus=1

        for f in o.split():
            if b"." not in f:
                continue # Ignore folders
            rmfileifexisting("thumbnail.jpg")
            o,status,e=cmd_ec(THUMB+" "+f.decode()+" thumbnail.jpg")
            ext=f.decode().split(".")[-1].lower().strip()
            allowedFailure=["ai","ani","cur","eps","exe","gif","heic","html","idx","j2c","jpm","md","mj2","pdf","psd","sgi","svg","txt","webp","xmp", "pnm","ppm", "tiff", "tif", "x3f"]
            if not ext in allowedFailure and b"saved in" not in o: #note: output code is not trustworthy: check for "saved in"
                fullout=fullout+str("missing thumbnail for:"+str(f)+"\n")
                fullStatus=0
                print (status, ext," missing thumbnail:",f,"\n",o,
                       self.check_failed_and_clear(fullout,fullStatus))

    @unittest.skipIf('SKIP_PDF_THUMBNAIL_TESTS' in os.environ, "only for systems where pdfium is enabled")
    def test_20_pdf_thumbnail(self):
        print(f"using pdfsURL: {self.pdfsURL}")
        cmd_ef(GET+" "+self.pdfsURL+" localtmp/")

        #2nd, upload folder
        cmd_ef(PUT+" localtmp/pdfs")
        #3rd, for each file, download thumbnail
        folder="/pdfs"
        o,status,e=cmd_ec(FIND+" "+folder+"/*")
        fullout=""
        fullStatus=1

        print(f"output = {o}")
        split = o.split(b"\n")
        print(f"split output = {split}")
        for f in split:
            if not len(f): continue
            rmfileifexisting("thumbnail.jpg")
            o,status,e=cmd_ec(THUMB+" '"+f.decode()+"' thumbnail.jpg")
            allowedFailure=["very big sheet size", "protected", "non-pdf-file","with-password","_TFG"]

            if not True in [x.encode() in f for x in allowedFailure] and b"saved in" not in o: #note: output code is not trustworthy: check for "saved in"
                fullout=fullout+str("missing thumbnail for:"+str(f)+"\n")
                fullStatus=0
                print(f'{status} missing thumbnail: {f}')
                print(o)
                self.check_failed_and_clear(fullout,fullStatus)

    @unittest.skipIf(platform.system() != 'Windows' or CMDSHELL, "The `-o` argument in `cat` is only available on Windows and in non-interactive mode")
    def test_21_ascii_cat_to_file(self):
        ascii_string_list = [
            'Hello world',
            'The quick brown fox jumps over the lazy dog.',
            '0123456789',
            '!@#$%^&*()_+-=[]{}|;:\'",.<>?/',
        ]
        for i, file_contents in enumerate(ascii_string_list, start=1):
            file_name = f'unicode_{i}.txt'
            file_path = f'localUPs/{file_name}'
            with open(file_path, mode='w', encoding='ascii') as f:
                f.write(file_contents)
            cmd_ef(f'{PUT} {file_path} /')

            cat_contents = cmd_es(f'{CAT} /{file_name}')
            self.assertEqual(cat_contents.decode('ascii'), file_contents)

            cmd_ef(f'{CAT} /{file_name} -o localUPs/cat_output.txt')
            self.assertTrue(filecmp.cmp('localUPs/cat_output.txt', file_path, shallow=False))

    @unittest.skipIf(platform.system() != 'Windows' or CMDSHELL, "The `-o` argument in `cat` is only available on Windows and in non-interactive mode")
    def test_22_unicode_cat_to_file(self):
        for i, file_contents in enumerate(UNICODE_NAME_LIST, start=1):
            file_name = f'unicode_{i}.txt'
            file_path = f'localUPs/{file_name}'
            with open(file_path, mode='w', encoding='utf-8') as f:
                f.write(file_contents)
            cmd_ef(f'{PUT} {file_path} /')

            cat_contents = cmd_es(f'{CAT} /{file_name}')
            self.assertEqual(cat_contents.decode('utf-8'), file_contents)

            cmd_ef(f'{CAT} /{file_name} -o localUPs/cat_output.txt')
            self.assertTrue(filecmp.cmp('localUPs/cat_output.txt', file_path, shallow=False))

if __name__ == '__main__':
    if "OUT_DIR_JUNIT_XML" in os.environ:
        unittest.main(testRunner=xmlrunner.XMLTestRunner(output=os.environ["OUT_DIR_JUNIT_XML"]), failfast=False, buffer=False, catchbreak=False)
    else:
        unittest.main()

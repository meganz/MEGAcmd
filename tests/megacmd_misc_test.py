#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys, os, subprocess, shutil
from megacmd_tests_common import *

GET="mega-get"
PUT="mega-put"
RM="mega-rm"
MV="mega-mv"
CP="mega-cp"
CD="mega-cd"
THUMB="mega-thumbnail"
LCD="mega-lcd"
MKDIR="mega-mkdir"
EXPORT="mega-export -f"
FIND="mega-find"
INVITE="mega-invite"
IPC="mega-ipc"
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
    MEGA_EMAIL_AUX=os.environ["MEGA_EMAIL_AUX"]
    MEGA_PWD_AUX=os.environ["MEGA_PWD_AUX"]
except:
    print >>sys.stderr, "You must define variables MEGA_EMAIL MEGA_PWD MEGA_EMAIL_AUX MEGA_PWD_AUX. WARNING: Use an empty account for $MEGA_EMAIL"
    exit(1)


try:
    MEGACMDSHELL=os.environ['MEGACMDSHELL']
    CMDSHELL=True
    #~ FIND="executeinMEGASHELL find" #TODO

except:
    CMDSHELL=False

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
    
    if cmd_es(WHOAMI) != osvar("MEGA_EMAIL"):
        cmd_ef(LOGOUT)
        cmd_ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))
        
    #~ rm pipe > /dev/null 2>/dev/null || :

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

def compare_and_clear() :
    global currentTest
    if VERBOSE:
        print "test $currentTest"
    
    megafind=sort(cmd_ef(FIND))
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
    cmd_ef(CD+" /")

def check_failed_and_clear(o,status):
    global currentTest

    if status == 0:
        print "test "+str(currentTest)+" failed!"
        print o
        exit(1)
    else:
        print "test "+str(currentTest)+" succesful!"

    currentTest+=1
    cmd_ef(CD+" /")

opts='";X()[]{}<>|`\''
if os.name == 'nt':
    opts=";()[]{}`'"

def initialize():
     
    if cmd_es(WHOAMI) != osvar("MEGA_EMAIL"):
        cmd_es(LOGOUT)
        cmd_ef(LOGIN+" " +osvar("MEGA_EMAIL")+" "+osvar("MEGA_PWD"))

    if len(os.listdir(".")) and ( len(os.listdir(".")) != 1 and os.listdir(".")[0] != 'images'):
        print >>sys.stderr, "initialization folder not empty!", "\n",os.listdir(".")
        #~ cd $ABSPWD
        exit(1)

    if cmd_es(FIND+" /") != "/":
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


def compare_find(what, localFindPrefix='localUPs'):
    global currentTest
    if VERBOSE:
        print "test "+str(currentTest)

    if not isinstance(what, list):
        what = [what]
    megafind=""
    localfind=""
    for w in what:
        megafind+=cmd_ef(FIND+" "+w)+"\n"
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

#Test 1 #/

currentTest=1

bar=""
if not CMDSHELL: bar="\\"
if os.name == 'nt': bar=""
#Tests 1-14 # handle special characters
for c in opts:
    megafind=sort(cmd_ef(FIND+" "+'odd/file'+bar+c+'01.txt'))
    compare_remote_local(megafind,'odd/file'+c+'01.txt')

currentTest=15

if os.name != 'nt':
    #Test 15 # very special \ character
    c='\\'
    cmd_ef(MV+" "+'odd/fileX01.txt odd/file'+bar+'\\01.txt') #upload of file containing "\\" fails (SDK issue)
    shutil.move('localUPs/odd/fileX01.txt', 'localUPs/odd/file\\01.txt')
    megafind=sort(cmd_ef(FIND+" "+'odd/file'+bar+c+'01.txt'))
    compare_remote_local(megafind,'odd/file'+c+'01.txt')
    #revert original situation
    cmd_ef(MV+" "+'odd/file\\\\01.txt odd/fileX01.txt')
    shutil.move('localUPs/odd/file\\01.txt', 'localUPs/odd/fileX01.txt')

currentTest=16

#Test 16 #move & rename
cmd_ef(MV+" file01nonempty.txt "+'/le01/moved.txt')
shutil.move("localUPs/file01nonempty.txt", "localUPs/le01/moved.txt")
compare_find('/')
#revert original situation
cmd_ef(MV+' /le01/moved.txt'+" file01nonempty.txt ")
shutil.move("localUPs/le01/moved.txt", "localUPs/file01nonempty.txt")

#Test 17
touch("localUPs/newemptyfile.txt")
cmd_ef(PUT+" "+"localUPs/newemptyfile.txt"+" "+"/")
#shutil.copy2('auxx/01/s01/another.txt','localUPs/01/s01')
compare_find('/')

currentTest=18

#Test 18 # Some unicode characters
# us=''.join(tuple(chr(l) for l in range(1, 0x10ffff) if chr(l).isprintable())) #requires python 3.5
# for r in range(10):
    # fname=""
    # for x in range(10):
        # fname+=unicodedata.normalize('NFC',random.choice(us)) # this could give characters that might need escaping! e.g: "("

if os.name != 'nt':
    i=0
    for fname in [
    u'\U000213e4\ucc90\u6361\U0002569b\U00028106\u1575\u416d\u267c\u11f3\ua091\u38b2\U000201b0\u594d\U00029839\u3f46\u481c\u65b8\u25cc\U00026f3b\u26ff\u292c\U0001d6d1\u7b2b\uca6a\uc23b\U000202e5\u90d7\u7200\u6c2c\U00021cf2\u63e6\U00023cfd\U0001d7f5\U00010305\u4a06\u7716\U000247b9\ufdbb\ua841\U00021965\U0001e8d4\U00026d38\U00023a82\U0001d118\u1fc7\u114d\U000112e2\U0002a465\u90a9',
    u'\U0002bfb4\ua320\U0002682f\U00028d67\U0002c09e\U0002aaa3\uc703\u937a\U00028fe6\U0002c3e2\u5886\U000278ff\U000266de\U0002928c\u6c62\U0002166c\u15cc\u4fda\U00029cbb\u3308\U0002a5fd\U00029059\u4144\U0001f3d7\U00024d9d\u6117\U00029823\uc063\U0002be26\u254c\u866d\U00024837\u8e36\U0002bc25\U000205c4\U00025083\u606a\U0002323b\u1a0e\u388f\U0002b5c2\ubff5\U0002b26f\U000144f5\u4b17\u872f\ub5f4\ua2d5\U0002863a\U0002abc3',
    u'\u2978\U00024714\ua343\U0001d954\uff51\U00028694\u1da0\u63a1\u68c4\U0002c23f\u8315\uad85\U0002024a\ucbcb\uba86\u7cb1\u3658\u68a0\ucb6e\u7262\u021e\u98a3\U0002177d\U00025a63\U0001f47e\U000293a1\U000270f0\U00022477\u3701\U0001085a\U00022c60\U00024344\u34a9\ua2b8\u7af8\U0002b288\U00023acc\u08fa\U00013058\U00021e39\U00026f4a\U000275a1\u100b\U0002236a\ucd7b\U0002805b\U00011221\u3cd8\U0002ab7b\U0001e8cd',
    u'\u5c63\u1684\u8988\U0002ca91\U0002ce04\U000295a9\U00010034\u63a8\ub021\u7ac2\u99df\U0002b983\U00027c91\u32ed\U00023677\U00011156\U00025f85\u8e9b\u092f\U000293d1\u4c84\u6f2f\U00024eb7\U0001f705\u9ecd\U000217d8\U00029064\ufc23\U000222f3\U00026bc9\u581c\u95c1\ua56c\u56aa\u1266\u2f6b\U00022ee0\u5c2b\U00024506\U00022112\u387f\U0001ee2b\U00022ced\ua028\U0002566d\u1c3c\U00021489\U0002ae1a\U00028972\U0002356e',
    u'\U0002c6f7\u8396\U00024ec9\U0002cab5\U0002389c\U00029663\ub237\U000213fe\u3079\u664e\u1ce6\u527a\u6696\u9df5\u2de0\u6a04\U000282b9\U0002bea5\u6a06\U00020575\ub734\U0002cc4c\U00022487\U00023409\ubb28\U00025a22\U00022ad3\U000274e6\ub86a\U00021d31\u998b\U0002c3d6\U000120a0\U00028188\u26b9\U0002740f\U0002cb90\U00023559\U0002b3b9\uff19\ufe08\ufc4e\u0a71\U00020524\u4304\ua395\u3421\U0001d802\U0002be22\U0002ba22',
    u'\U00013146\U00029f94\U00025fb9\U000243fe\u58eb\ua42d\u9b0c\ud4c7\U0002afd9\U00020ecd\U0002b10c\U00027562\U0002295e\U00023fd2\U0001f701\U0002c5e9\U0002b12b\u3bd0\u25a1\ud170\U000277e2\U0002828c\u3f27\u949a\u5d40\u5f08\U000268bf\u9851\U000238a7\U0002a997\U00023659\U00012180\U00020e3c\u45ab\U0001f70e\U00010634\U0002421f\U0002374e\u59f9\u044c\u3b42\u3901\U00020015\U00027b6f\u6765\U00029c00\U0002a047\U0002b07d\u23dc\U0001f453',
    u'\U000102a0\U00029697\U00026514\ua344\U00028bcc\u7c1c\U000293a1\u45ad\U0002ae08\u19f8\U00020114\U0002b2d8\U0002432d\u1387\U000245c4\U00024710\U00020528\U000225a2\U00028f2c\u765f\U0002286b\u4715\U00027e51\U000270d4\u99fb\U00021aeb\U0002aa25\u449a\ub86c\U00022672\ud4af\U000296ea\u511a\u7889\u9cfe\U00024d16\u893b\U0002a362\u3cb4\U00026383\ud3cc\u8750\U0001e859\U00028aa5\u0af9\u056f\u4404\U0002a82e\U00020524\u55c9',
    u'\U0002151f\U000269cb\u8040\U0002842b\U0001d11d\U0002a212\U00025584\u142c\u2415\U00021cec\U000268e3\U00027392\u5e02\u1b6a\U000233a1\u9a28\U00022dee\u97bd\U000109eb\U0002762e\uce08\U00026d94\u2ba8\u3d7c\u9219\u2bac\U00029398\U00023b98\U00025124\U00024cad\u463b\u8b37\u5265\U00026451\u61ce\ud62c\U0001d7bd\U0001d342\u37bf\U00028105\U00025a01\u7699\u3def\U000276d6\U00011483\ucdbf\u15bc\U000286da\u8dca\ubb06',
    u'\u9004\U00027165\U000252e6\U00020bee\U00023f5c\uc17c\U00025a8f\U00021c40\U00021e77\U0002ae0c\U0002538a\u0b92\U0002874e\U000233ed\U0001f573\U00027be4\U0001170c\U00020755\u8b8e\u2e06\U0002525d\U0002ab05\U00010666\ub337\u3c91\ucac1\u2ab1\U000212d0\U00020e29\u5081\u18e5\u37de\U00021de9\u0786\U00023de9\u7061\u0625\u399e\U0002043e\u828a\U0002c208\u05ab\U00028859\u0100\ufcb7\U00016861\u4762\ua5e4\U000268d0\u234b',
    u'\U00023911\U00024084\ua5f7\U0002aaa5\u1798\uc82c\ucfc0\U0002b4fe\U00014456\U000286bb\U0002897e\U000217c5\U0002a0c0\U0002a160\ub5e8\U00022abe\U000285be\U0002cd8d\U0002c36a\U0002c040\ub183\ub7c7\U0002b082\ud724\u7dd5\u0986\U00029507\uabaa\U00021920\u811c\u29c5\u7d3f\U00027831\U000227c2\u1f31\U00021153\u79a1\U0002b233\u8d1d\U000100b9\u8a5a\u9f16\u3e9a\U0002bd58\ua738\U00021110\U00025739\U0002cd05\U0002c7bf\U00024547',
    u'\U0002afa6\u3eed\u399c\U0002ca82\U0001f3ed\ua8a2\U0002c898\U0001ee72\U000298ef\u24a5\U0002b2c2\u5f30\ub387\u44b5\u7280\ua040\U0001d7f4\U0002270e\u2650\U0002ad54\uc07f\u889d\U0002caf7\U0002a77c\U000132a3\U0002353e\U00026115\u9c5d\uc4a5\U0002ccb1\u6d9c\u15b6\U0002a1ce\U00022dc4\u134a\u3aba\U00010020\U000259e8\u194f\u134d\u6865\U0002b139\U00021928\U000253d0\U00028a77\U0002783d\U00022b62\u36f7\u954f\U0002b6b3',
    u'\u1d61\U0002be0b\U00029142\U0002b41a\U0002cc5c\U00025583\u512c\u0d91\u668a\ud36e\U00022b9a\u6766\u1ec7\u8d0d\U000218c4\U0002c073\u53de\U00029bd6\U00027727\U0002c87b\u90e5\u967c\ucef8\U00027830\u725e\u445f\u71b6\U0002676e\ua740\uca76\U00028aa8\U00029761\U000280ee\u1bde\U00028c03\ud534\U00013022\U0002b023\ufc50\U0002b5f6\U000262a1\U0002ba31\U0002844c\U0002230f\uc4c0\U00029187\U00027b98\U00026446\U00023cd3\u0942',
    u'\uc288\u799e\ufe86\U00020b7d\ub663\u3e50\U000115a7\ufeb2\U000222af\U00012351\U000203ff\U000131be\u15b9\U000248b2\U00023206\u3460\U00028076\U0002800d\U0002896f\ua842\U0002ce7d\U0002bad7\u4288\u0489\ufb65\u238d\U00020ffe\U000100d5\U0002c100\u58f5\ucdbd\U000252fa\uc0b6\u7e29\u1186\u8e45\U00029acf\U000271d5\U000133e9\U00021991\U00027e04\U00026111\U00029842\u7724\u26d0\u45cb\U00024507\U00020567\ud006\U000245e5',
    u'\U0002cbba\u9b3e\U00025a9e\ud6a7\u7be3\ub389\U000203c2\ub501\U0001087d\u5bd9\U0002250d\uacb0\u8082\U00012071\ub2af\U00022b2f\u7c4f\u0cc2\u7670\U00029653\uff1d\U00021736\ud422\uc308\ud46b\U0002a3fa\u2d4e\u7981\ub73e\u5ec3\U00020a56\U000209db\U000240c1\u04a5\U00025d84\U00029cda\u8b52\U000249af\U0002c3d7\U000210ac\U00027944\U00021488\U0001211f\U00021235\ubb61\u6453\U0002651d\u7490\u6160\U00028827',
    u'\U00028f6f\u62e2\U0002362b\U00028c10\U0002b62a\u99b8\u837e\u5299\U000261e5\ubf41\u7d4a\U00027de5\U00028a26\uc9e5\u561e\u948c\U0002a4e0\U00028c25\U000260a9\U0002b48e\U00028132\U00020fa0\U00028a9e\U0002b09c\U00025a63\uae03\U0002c87a\U00020e63\u8471\U00029b91\U000246f3\U0002ae88\U000264bf\U00021ae2\U0002b3cd\U0001d0b3\uadf6\U0002a488\U0002701a\U00022dd6\u38c7\uacba\U0002152a\u718b\u37bb\U00024d74\u893c\u16ab\U000280fe\U00013404',
    u'\U00027436\ua44f\uc908\u217f\U00027425\U0002ba8c\uc30a\U00023dc5\ud325\u70c8\U00024bca\U0002914f\U0002b522\u4651\U00023606\U000169dd\u6e8d\u2b81\u3bfc\U00028217\ua1d0\u37c7\U00025caa\u7d9f\U000e0138\U0002682a\U0002543f\U00028e37\ufb79\U00020924\U00023089\u5673\u425e\ua30a\uce45\U000275cd\u4033\U00025d88\u7eeb\u7589\u1e9b\U00027992\uc74e\U00028bd4\U00022670\u926e\U0002ba1a\U0002a0b8\U00022f27\u3a0f'
    ]:
        i+=1
        touch("localUPs/"+fname)
        cmd_ef(PUT+" "+"localUPs/"+fname.encode('utf-8')+" "+"/")
    compare_find('/')

currentTest=19
#Test 19 #copy & rename
cmd_ef(CP+" file01nonempty.txt "+'/le01/copied')
copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/le01/copied")
compare_find('/')

#Test 18 #multicopy
cmd_ef(CP+" *.txt "+'/le01')
copybyfilepattern("localUPs/","*.txt", "localUPs/le01/")
compare_find('/')

#Test 19 #multicopy with trailing /
cmd_ef(CP+" *.txt "+'/le01/les01/')
copybyfilepattern("localUPs/","*.txt", "localUPs/le01/les01/")
compare_find('/')

#Test 20 #multisend
cmd_ef(CP+" *.txt "+MEGA_EMAIL_AUX+':')
print "test "+str(currentTest)+" succesful!"
currentTest+=1

#Test 21 #copy folder
cmd_ef(CP+" le01 "+'lf01')
copyfolder("localUPs/le01", "localUPs/lf01/")
compare_find('/')

if not CMDSHELL: #TODO: currently there is no way to know last CMSHELL status code
    #Test 24 #send to non contact
    o,status=cmd_ec(CP+" *.txt badContact"+MEGA_EMAIL_AUX+':')
    check_failed_and_clear(o,status)

    #Test 25 #multicopy into file
    o,status=cmd_ec(CP+" *.txt /le01/file01nonempty.txt")
    check_failed_and_clear(o,status)

currentTest=26

#Test 25 #copy into existing file
cmd_ef(CP+" -vvv file01nonempty.txt "+'copied2')
cmd_ef(CP+" -vvv file01nonempty.txt "+'copied2')
copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/copied2")
compare_find('/')

currentTest=27
#Test 26 #move into non existing file
cmd_ef(CP+" -vvv file01nonempty.txt "+'copied3')
cmd_ef(MV+" -vvv copied3 "+'moved3')
copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/moved3")
compare_find('/')

currentTest=28
#Test 28 #move into existing (different) file
cmd_ef(CP+" -vvv file01nonempty.txt "+'copied4')
cmd_ef(CP+" -vvv file01.txt "+'moved4')
cmd_ef(MV+" -vvv copied4 "+'moved4')
copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/moved4")
compare_find('/')

#Test 29 #move into existing equal file
cmd_ef(CP+" -vvv file01nonempty.txt "+'copied5')
cmd_ef(CP+" -vvv copied5 "+'moved5')
cmd_ef(MV+" -vvv copied5 "+'moved5')
copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/moved5")
compare_find('/')

#Test 30 #move into other path
cmd_ef(CP+" -vvv file01nonempty.txt "+'copied6')
cmd_ef(MV+" -vvv copied6 "+'/le01/moved6')
copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/le01/moved6")
compare_find('/')

#Test 31 #move existing other path (different file)
cmd_ef(CP+" -vvv file01nonempty.txt "+'copied7')
cmd_ef(CP+" -vvv file01.txt "+'/le01/moved7')
cmd_ef(MV+" -vvv copied7 "+'/le01/moved7')
copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/le01/moved7")
compare_find('/')

#Test 32 #move existing other path
cmd_ef(CP+" -vvv file01nonempty.txt "+'copied7')
cmd_ef(CP+" -vvv file01nonempty.txt "+'/le01/moved7')
cmd_ef(MV+" -vvv copied7 "+'/le01/moved7')
copybyfilepattern("localUPs/","file01nonempty.txt", "localUPs/le01/moved7")
compare_find('/')

currentTest=33 #thumbnails
#Test 33 #ensure thumnail generation
#1st get images selection
try:
    imagesUrl=os.environ['MEGACMD_TESTS_IMAGES_URL']
except:
    imagesUrl="https://mega.nz/folder/bxomFKwL#3V1dUJFzL98t1GqXX29IXg"
if VERBOSE:
    print "Using this imagesUrl: ",imagesUrl
cmd_ef(GET+" "+imagesUrl+" localtmp/")
#2nd, upload folder
cmd_ef(PUT+" localtmp/images")
#3rd, for each file, download thumbnail
folder="/images"
o,status=cmd_ec(FIND+" "+folder+"/*")
fullout=""
fullStatus=1
for f in o.split():
    rmfileifexisting("thumbnail.jpg")
    o,status=cmd_ec(THUMB+" "+f+" thumbnail.jpg")
    ext=f.split(".")[-1].lower().strip()
    allowedFailure=["ai","ani","cur","eps","exe","gif","heic","html","idx","j2c","jpm","md","mj2","pdf","psd","sgi","svg","txt","webp","xmp", "pnm","ppm"]    
    if not ext in allowedFailure and "saved in" not in o: #note: output code is not trustworthy: check for "saved in"
        fullout=fullout+str("missing thumbnail for:"+str(f)+"\n")
        fullStatus=0
        print status, ext," missing thumbnail:",f,"\n",o,
check_failed_and_clear(fullout,fullStatus)


currentTest=34 #pdf thumbnails
#Test 33 #ensure thumnail generation
#1st get pdfs selection
try:
    pdfsURL=os.environ['MEGACMD_TESTS_PDFS_URL']
except:
    pdfsURL="https://mega.nz/folder/D0w0nYiY#egvjqP5R-anbBdsJg8QRVg"
if VERBOSE:
    print "Using this pdfsURL: ",pdfsURL
cmd_ef(GET+" "+pdfsURL+" localtmp/")

#2nd, upload folder
cmd_ef(PUT+" localtmp/pdfs")
#3rd, for each file, download thumbnail
folder="/pdfs"
o,status=cmd_ec(FIND+" "+folder+"/*")
fullout=""
fullStatus=1

print o
print 'splited=', o.split("\n")
for f in o.split("\n"):
    if not len(f): continue
    rmfileifexisting("thumbnail.jpg")
    o,status=cmd_ec(THUMB+" '"+f+"' thumbnail.jpg")
    allowedFailure=["protected", "non-pdf-file","with-password","_TFG"]
    if not True in [x in f for x in allowedFailure] and "saved in" not in o: #note: output code is not trustworthy: check for "saved in"
        fullout=fullout+str("missing thumbnail for:"+str(f)+"\n")
        fullStatus=0
        print status, " missing thumbnail:",f,"\n",o,
check_failed_and_clear(fullout,fullStatus)
###################

# Clean all
if not VERBOSE:
    clean_all()

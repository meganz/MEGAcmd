#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "mega/types.h"
#include "mega/crypto/cryptopp.h"
#include "mega.h"

#define KEY_LENGTH 4096
#define SIGNATURE_LENGTH 512

using namespace mega;
using namespace std;

const char SERVER_BASE_URL_WIN[] = "http://g.static.mega.co.nz/upd/wcmd/";
const char SERVER_BASE_URL_OSX[] = "http://g.static.mega.co.nz/upd/mcmd/MEGAcmd.app/";

const char *TARGET_PATHS_WIN[] = {
    "api-ms-win-core-console-l1-1-0.dll",
    "api-ms-win-core-datetime-l1-1-0.dll",
    "api-ms-win-core-debug-l1-1-0.dll",
    "api-ms-win-core-errorhandling-l1-1-0.dll",
    "api-ms-win-core-file-l1-1-0.dll",
    "api-ms-win-core-file-l1-2-0.dll",
    "api-ms-win-core-file-l2-1-0.dll",
    "api-ms-win-core-handle-l1-1-0.dll",
    "api-ms-win-core-heap-l1-1-0.dll",
    "api-ms-win-core-interlocked-l1-1-0.dll",
    "api-ms-win-core-libraryloader-l1-1-0.dll",
    "api-ms-win-core-localization-l1-2-0.dll",
    "api-ms-win-core-memory-l1-1-0.dll",
    "api-ms-win-core-namedpipe-l1-1-0.dll",
    "api-ms-win-core-processenvironment-l1-1-0.dll",
    "api-ms-win-core-processthreads-l1-1-0.dll",
    "api-ms-win-core-processthreads-l1-1-1.dll",
    "api-ms-win-core-profile-l1-1-0.dll",
    "api-ms-win-core-rtlsupport-l1-1-0.dll",
    "api-ms-win-core-string-l1-1-0.dll",
    "api-ms-win-core-synch-l1-1-0.dll",
    "api-ms-win-core-synch-l1-2-0.dll",
    "api-ms-win-core-sysinfo-l1-1-0.dll",
    "api-ms-win-core-timezone-l1-1-0.dll",
    "api-ms-win-core-util-l1-1-0.dll",
    "api-ms-win-crt-conio-l1-1-0.dll",
    "api-ms-win-crt-convert-l1-1-0.dll",
    "api-ms-win-crt-environment-l1-1-0.dll",
    "api-ms-win-crt-filesystem-l1-1-0.dll",
    "api-ms-win-crt-heap-l1-1-0.dll",
    "api-ms-win-crt-locale-l1-1-0.dll",
    "api-ms-win-crt-math-l1-1-0.dll",
    "api-ms-win-crt-multibyte-l1-1-0.dll",
    "api-ms-win-crt-private-l1-1-0.dll",
    "api-ms-win-crt-process-l1-1-0.dll",
    "api-ms-win-crt-runtime-l1-1-0.dll",
    "api-ms-win-crt-stdio-l1-1-0.dll",
    "api-ms-win-crt-string-l1-1-0.dll",
    "api-ms-win-crt-time-l1-1-0.dll",
    "api-ms-win-crt-utility-l1-1-0.dll",
    "avcodec-57.dll",
    "avformat-57.dll",
    "avutil-55.dll",
    "cares.dll",
    "concrt140.dll",
    "FreeImage.dll",
    "libcurl.dll",
    "libeay32.dll",
    "libsodium.dll",
    "MEGA Website.url",
    "mega-attr.bat",
    "mega-backup.bat",
    "mega-cancel.bat",
    "mega-cd.bat",
    "mega-confirm.bat",
    "mega-confirmcancel.bat",
    "mega-cp.bat",
    "mega-debug.bat",
    "mega-deleteversions.bat",
    "mega-du.bat",
    "mega-errorcode.bat",
    "mega-exclude.bat",
    "mega-export.bat",
    "mega-find.bat",
    "mega-ftp.bat",
    "mega-get.bat",
    "mega-graphics.bat",
    "mega-help.bat",
    "mega-https.bat",
    "mega-import.bat",
    "mega-invite.bat",
    "mega-ipc.bat",
    "mega-killsession.bat",
    "mega-lcd.bat",
    "mega-log.bat",
    "mega-login.bat",
    "mega-logout.bat",
    "mega-lpwd.bat",
    "mega-ls.bat",
    "mega-mkdir.bat",
    "mega-mount.bat",
    "mega-mv.bat",
    "mega-passwd.bat",
    "mega-preview.bat",
    "mega-put.bat",
    "mega-pwd.bat",
    "mega-quit.bat",
    "mega-reload.bat",
    "mega-rm.bat",
    "mega-session.bat",
    "mega-share.bat",
    "mega-showpcr.bat",
    "mega-signup.bat",
    "mega-speedlimit.bat",
    "mega-sync.bat",
    "mega-thumbnail.bat",
    "mega-transfers.bat",
    "mega-userattr.bat",
    "mega-users.bat",
    "mega-version.bat",
    "mega-webdav.bat",
    "mega-whoami.bat",
    "MEGAclient.exe",
    "MEGAcmdServer.exe",
    "MEGAcmdShell.exe",
    "msvcp140.dll",
    "ssleay32.dll",
    "swresample-2.dll",
    "swscale-4.dll",
    "ucrtbase.dll",
    "uninst.exe",
    "vccorlib140.dll",
    "vcruntime140.dll"
};

const char *UPDATE_FILES_WIN[] = {
    "api-ms-win-core-console-l1-1-0.dll",
    "api-ms-win-core-datetime-l1-1-0.dll",
    "api-ms-win-core-debug-l1-1-0.dll",
    "api-ms-win-core-errorhandling-l1-1-0.dll",
    "api-ms-win-core-file-l1-1-0.dll",
    "api-ms-win-core-file-l1-2-0.dll",
    "api-ms-win-core-file-l2-1-0.dll",
    "api-ms-win-core-handle-l1-1-0.dll",
    "api-ms-win-core-heap-l1-1-0.dll",
    "api-ms-win-core-interlocked-l1-1-0.dll",
    "api-ms-win-core-libraryloader-l1-1-0.dll",
    "api-ms-win-core-localization-l1-2-0.dll",
    "api-ms-win-core-memory-l1-1-0.dll",
    "api-ms-win-core-namedpipe-l1-1-0.dll",
    "api-ms-win-core-processenvironment-l1-1-0.dll",
    "api-ms-win-core-processthreads-l1-1-0.dll",
    "api-ms-win-core-processthreads-l1-1-1.dll",
    "api-ms-win-core-profile-l1-1-0.dll",
    "api-ms-win-core-rtlsupport-l1-1-0.dll",
    "api-ms-win-core-string-l1-1-0.dll",
    "api-ms-win-core-synch-l1-1-0.dll",
    "api-ms-win-core-synch-l1-2-0.dll",
    "api-ms-win-core-sysinfo-l1-1-0.dll",
    "api-ms-win-core-timezone-l1-1-0.dll",
    "api-ms-win-core-util-l1-1-0.dll",
    "api-ms-win-crt-conio-l1-1-0.dll",
    "api-ms-win-crt-convert-l1-1-0.dll",
    "api-ms-win-crt-environment-l1-1-0.dll",
    "api-ms-win-crt-filesystem-l1-1-0.dll",
    "api-ms-win-crt-heap-l1-1-0.dll",
    "api-ms-win-crt-locale-l1-1-0.dll",
    "api-ms-win-crt-math-l1-1-0.dll",
    "api-ms-win-crt-multibyte-l1-1-0.dll",
    "api-ms-win-crt-private-l1-1-0.dll",
    "api-ms-win-crt-process-l1-1-0.dll",
    "api-ms-win-crt-runtime-l1-1-0.dll",
    "api-ms-win-crt-stdio-l1-1-0.dll",
    "api-ms-win-crt-string-l1-1-0.dll",
    "api-ms-win-crt-time-l1-1-0.dll",
    "api-ms-win-crt-utility-l1-1-0.dll",
    "avcodec-57.dll",
    "avformat-57.dll",
    "avutil-55.dll",
    "cares.dll",
    "concrt140.dll",
    "FreeImage.dll",
    "libcurl.dll",
    "libeay32.dll",
    "libsodium.dll",
    "MEGA Website.url",
    "mega-attr.bat",
    "mega-backup.bat",
    "mega-cancel.bat",
    "mega-cd.bat",
    "mega-confirm.bat",
    "mega-confirmcancel.bat",
    "mega-cp.bat",
    "mega-debug.bat",
    "mega-deleteversions.bat",
    "mega-du.bat",
    "mega-errorcode.bat",
    "mega-exclude.bat",
    "mega-export.bat",
    "mega-find.bat",
    "mega-ftp.bat",
    "mega-get.bat",
    "mega-graphics.bat",
    "mega-help.bat",
    "mega-https.bat",
    "mega-import.bat",
    "mega-invite.bat",
    "mega-ipc.bat",
    "mega-killsession.bat",
    "mega-lcd.bat",
    "mega-log.bat",
    "mega-login.bat",
    "mega-logout.bat",
    "mega-lpwd.bat",
    "mega-ls.bat",
    "mega-mkdir.bat",
    "mega-mount.bat",
    "mega-mv.bat",
    "mega-passwd.bat",
    "mega-preview.bat",
    "mega-put.bat",
    "mega-pwd.bat",
    "mega-quit.bat",
    "mega-reload.bat",
    "mega-rm.bat",
    "mega-session.bat",
    "mega-share.bat",
    "mega-showpcr.bat",
    "mega-signup.bat",
    "mega-speedlimit.bat",
    "mega-sync.bat",
    "mega-thumbnail.bat",
    "mega-transfers.bat",
    "mega-userattr.bat",
    "mega-users.bat",
    "mega-version.bat",
    "mega-webdav.bat",
    "mega-whoami.bat",
    "MEGAclient.exe",
    "MEGAcmdServer.exe",
    "MEGAcmdShell.exe",
    "msvcp140.dll",
    "ssleay32.dll",
    "swresample-2.dll",
    "swscale-4.dll",
    "ucrtbase.dll",
    "uninst.exe",
    "vccorlib140.dll",
    "vcruntime140.dll"
};

const char *TARGET_PATHS_OSX[] = {
    "Contents/_CodeSignature/CodeResources",
    "Contents/_CodeSignature/CodeDirectory",
    "Contents/_CodeSignature/CodeRequirements-1",
    "Contents/_CodeSignature/CodeSignature",
    "Contents/_CodeSignature/CodeRequirements",
    "Contents/MacOS/mega-lcd",
    "Contents/MacOS/mega-deleteversions",
    "Contents/MacOS/mega-cancel",
    "Contents/MacOS/mega-exec",
    "Contents/MacOS/mega-log",
    "Contents/MacOS/mega-ipc",
    "Contents/MacOS/mega-lpwd",
    "Contents/MacOS/mega-killsession",
    "Contents/MacOS/mega-attr",
    "Contents/MacOS/mega-speedlimit",
    "Contents/MacOS/mega-mv",
    "Contents/MacOS/MEGAcmdShell",
    "Contents/MacOS/mega-login",
    "Contents/MacOS/mega-whoami",
    "Contents/MacOS/mega-share",
    "Contents/MacOS/mega-debug",
    "Contents/MacOS/mega-signup",
    "Contents/MacOS/mega-cp",
    "Contents/MacOS/mega-ls",
    "Contents/MacOS/mega-help",
    "Contents/MacOS/mega-errorcode",
    "Contents/MacOS/mega-exclude",
    "Contents/MacOS/mega-cd",
    "Contents/MacOS/mega-logout",
    "Contents/MacOS/mega-permissions",
    "Contents/MacOS/mega-sync",
    "Contents/MacOS/mega-confirm",
    "Contents/MacOS/mega-users",
    "Contents/MacOS/mega-showpcr",
    "Contents/MacOS/mega-rm",
    "Contents/MacOS/mega-mount",
    "Contents/MacOS/mega-mkdir",
    "Contents/MacOS/mega-find",
    "Contents/MacOS/mega-transfers",
    "Contents/MacOS/mega-invite",
    "Contents/MacOS/mega-reload",
    "Contents/MacOS/mega-version",
    "Contents/MacOS/mega-pwd",
    "Contents/MacOS/mega-userattr",
    "Contents/MacOS/mega-ftp",
    "Contents/MacOS/MEGAcmd",
    "Contents/MacOS/mega-cmd",
    "Contents/MacOS/mega-confirmcancel",
    "Contents/MacOS/mega-webdav",
    "Contents/MacOS/mega-du",
    "Contents/MacOS/mega-get",
    "Contents/MacOS/mega-preview",
    "Contents/MacOS/megacmd_completion.sh",
    "Contents/MacOS/mega-https",
    "Contents/MacOS/mega-import",
    "Contents/MacOS/mega-backup",
    "Contents/MacOS/mega-quit",
    "Contents/MacOS/mega-export",
    "Contents/MacOS/mega-graphics",
    "Contents/MacOS/mega-put",
    "Contents/MacOS/mega-thumbnail",
    "Contents/MacOS/mega-session",
    "Contents/MacOS/MEGAcmdLoader",
    "Contents/MacOS/mega-passwd",
    "Contents/Resources/empty.lproj",
    "Contents/Resources/app.icns",
    "Contents/Frameworks/libavutil.55.dylib",
    "Contents/Frameworks/libavcodec.57.dylib",
    "Contents/Frameworks/libswscale.4.dylib",
    "Contents/Frameworks/libavformat.57.dylib",
    "Contents/Info.plist",
    "Contents/PkgInfo"
};

const char *UPDATE_FILES_OSX[] = {
    "Contents/_CodeSignature/CodeResources",
    "Contents/_CodeSignature/CodeDirectory",
    "Contents/_CodeSignature/CodeRequirements-1",
    "Contents/_CodeSignature/CodeSignature",
    "Contents/_CodeSignature/CodeRequirements",
    "Contents/MacOS/mega-lcd",
    "Contents/MacOS/mega-deleteversions",
    "Contents/MacOS/mega-cancel",
    "Contents/MacOS/mega-exec",
    "Contents/MacOS/mega-log",
    "Contents/MacOS/mega-ipc",
    "Contents/MacOS/mega-lpwd",
    "Contents/MacOS/mega-killsession",
    "Contents/MacOS/mega-attr",
    "Contents/MacOS/mega-speedlimit",
    "Contents/MacOS/mega-mv",
    "Contents/MacOS/MEGAcmdShell",
    "Contents/MacOS/mega-login",
    "Contents/MacOS/mega-whoami",
    "Contents/MacOS/mega-share",
    "Contents/MacOS/mega-debug",
    "Contents/MacOS/mega-signup",
    "Contents/MacOS/mega-cp",
    "Contents/MacOS/mega-ls",
    "Contents/MacOS/mega-help",
    "Contents/MacOS/mega-errorcode",
    "Contents/MacOS/mega-exclude",
    "Contents/MacOS/mega-cd",
    "Contents/MacOS/mega-logout",
    "Contents/MacOS/mega-permissions",
    "Contents/MacOS/mega-sync",
    "Contents/MacOS/mega-confirm",
    "Contents/MacOS/mega-users",
    "Contents/MacOS/mega-showpcr",
    "Contents/MacOS/mega-rm",
    "Contents/MacOS/mega-mount",
    "Contents/MacOS/mega-mkdir",
    "Contents/MacOS/mega-find",
    "Contents/MacOS/mega-transfers",
    "Contents/MacOS/mega-invite",
    "Contents/MacOS/mega-reload",
    "Contents/MacOS/mega-version",
    "Contents/MacOS/mega-pwd",
    "Contents/MacOS/mega-userattr",
    "Contents/MacOS/mega-ftp",
    "Contents/MacOS/MEGAcmd",
    "Contents/MacOS/mega-cmd",
    "Contents/MacOS/mega-confirmcancel",
    "Contents/MacOS/mega-webdav",
    "Contents/MacOS/mega-du",
    "Contents/MacOS/mega-get",
    "Contents/MacOS/mega-preview",
    "Contents/MacOS/megacmd_completion.sh",
    "Contents/MacOS/mega-https",
    "Contents/MacOS/mega-import",
    "Contents/MacOS/mega-backup",
    "Contents/MacOS/mega-quit",
    "Contents/MacOS/mega-export",
    "Contents/MacOS/mega-graphics",
    "Contents/MacOS/mega-put",
    "Contents/MacOS/mega-thumbnail",
    "Contents/MacOS/mega-session",
    "Contents/MacOS/MEGAcmdLoader",
    "Contents/MacOS/mega-passwd",
    "Contents/Resources/empty.lproj",
    "Contents/Resources/app.icns",
    "Contents/Frameworks/libavutil.55.dylib",
    "Contents/Frameworks/libavcodec.57.dylib",
    "Contents/Frameworks/libswscale.4.dylib",
    "Contents/Frameworks/libavformat.57.dylib",
    "Contents/Info.plist",
    "Contents/PkgInfo"
};

template <typename T, std::size_t N>
char (&static_sizeof_array( T(&)[N] ))[N];
#define SIZEOF_ARRAY( x ) sizeof(static_sizeof_array(x))

void printUsage(const char* appname)
{
    cerr << "Usage: " << endl;
    cerr << "Generate a keypair" << endl;
    cerr << "    " << appname << " -g" << endl;
    cerr << "Sign an update:" << endl;
    cerr << "    " << appname << " -s <win|osx> <update folder> <keyfile> <version_code>" << endl;
    cerr << "  or:" << endl;
    cerr << "    " << appname << " <update folder> <keyfile> <version_code> --file <contentsfile> --base-url <base_url>" << endl;
    cerr << "    e.g:" << endl;
    cerr << "        " << appname << " /tmp/updatefiles /tmp/key.pem 100050 --file /tmp/files.txt --base-url http://g.static.mega.co.nz/upd/wcmd/" << endl;
}

unsigned signFile(const char * filePath, AsymmCipher* key, byte* signature, unsigned signbuflen)
{
    HashSignature signatureGenerator(new Hash());
    char buffer[1024];

    ifstream input(filePath, std::ios::in | std::ios::binary);
    if (input.fail())
    {
        return 0;
    }

    while (input.good())
    {
        input.read(buffer, sizeof(buffer));
        signatureGenerator.add((byte *)buffer, (unsigned)input.gcount());
    }

    if (input.bad())
    {
        return 0;
    }

    unsigned signatureSize = signatureGenerator.get(key, signature, signbuflen);
    if (signatureSize < signbuflen)
    {
        int padding = signbuflen - signatureSize;
        for (int i = signbuflen - 1; i >= 0; i--)
        {
            if (i >= padding)
            {
                signature[i] = signature[i - padding];
            }
            else
            {
                signature[i] = 0;
            }
        }
        signatureSize = signbuflen;
    }
    return signatureSize;
}

bool extractarg(vector<const char*>& args, const char *what)
{
    for (int i = int(args.size()); i--; )
    {
        if (!strcmp(args[i], what))
        {
            args.erase(args.begin() + i);
            return true;
        }
    }
    return false;
}

bool extractargparam(vector<const char*>& args, const char *what, std::string& param)
{
    for (int i = int(args.size()) - 1; --i >= 0; )
    {
        if (!strcmp(args[i], what) && args.size() > i)
        {
            param = args[i + 1];
            args.erase(args.begin() + i, args.begin() + i + 2);
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[])
{
    vector<const char*> args(argv + 1, argv + argc);

    string fileInput;
    bool externalfile = extractargparam(args, "--file", fileInput);
    bool generate = extractarg(args, "-g");
    string os;
    bool bos = extractargparam(args, "-s", os);
    string fileUrl;
    bool bUrl = extractargparam(args, "--base-url", fileUrl);


    HashSignature signatureGenerator(new Hash());
    AsymmCipher aprivk;
    vector<string> downloadURLs;
    vector<string> signatures;
    byte signature[SIGNATURE_LENGTH];
    unsigned signatureSize;
    string pubk;
    string privk;
    bool win = true;

    if (generate)
    {
        //Generate a keypair
        CryptoPP::Integer pubk[AsymmCipher::PUBKEY];
        string pubks;
        string privks;

        AsymmCipher asymkey;
        asymkey.genkeypair(asymkey.key,pubk,KEY_LENGTH);
        AsymmCipher::serializeintarray(pubk,AsymmCipher::PUBKEY,&pubks);
        AsymmCipher::serializeintarray(asymkey.key,AsymmCipher::PRIVKEY,&privks);

        int len = pubks.size();
        char* pubkstr = new char[len*4/3+4];
        Base64::btoa((const byte *)pubks.data(),len,pubkstr);

        len = privks.size();
        char *privkstr = new char[len*4/3+4];
        Base64::btoa((const byte *)privks.data(),len,privkstr);

        cout << pubkstr << endl;
        cout << privkstr << endl;

        delete [] pubkstr;
        delete [] privkstr;
        return 0;
    }
    else if ((args.size() == 3)  && ((bos && (os == "win" || os == "osx")) || (bUrl && externalfile)))
    {
        //Sign an update
        win = os == "win";

        if (!bUrl)
        {
            fileUrl = string ((win ? SERVER_BASE_URL_WIN : SERVER_BASE_URL_OSX));
        }

        //Prepare the update folder path
        string updateFolder(args.at(0));
        if (updateFolder[updateFolder.size()-1] != '/')
        {
            updateFolder.append("/");
        }

        //Read keys
        ifstream keyFile(args.at(1), std::ios::in);
        if (keyFile.bad())
        {
            printUsage(argv[0]);
            return 2;
        }
        getline(keyFile, pubk);
        getline(keyFile, privk);
        if (!pubk.size() || !privk.size())
        {
            cerr << "Invalid key file" << endl;
            keyFile.close();
            return 3;
        }
        keyFile.close();

        long versionCode = strtol (args.at(2), NULL, 10);
        if (!versionCode)
        {
            cerr << "Invalid version code" << endl;
            return 5;
        }

        //Initialize AsymmCypher
        string privks;
        privks.resize(privk.size()/4*3+3);
        privks.resize(Base64::atob(privk.data(), (byte *)privks.data(), privks.size()));
        aprivk.setkey(AsymmCipher::PRIVKEY,(byte*)privks.data(), privks.size());

        //Generate update file signature
        signatureGenerator.add((const byte *)args.at(2), strlen(args.at(2)));
        vector<string> filesVector;
        vector<string> targetPathsVector;

        if (externalfile)
        {
            filesVector.clear();
            targetPathsVector.clear();
            ifstream infile(fileInput.c_str());
            string line;
            while (getline(infile, line))
            {
                if (line.length() > 0 && line[0] != '#')
                {
                    string fileToDl, targetpah;
                    size_t pos = line.find(";");
                    fileToDl = line.substr(0, pos);
                    if (pos != string::npos && ((pos + 1) < line.size()))
                    {
                        targetpah = line.substr(pos + 1);
                    }
                    else
                    {
                        targetpah = fileToDl;
                    }
                    filesVector.push_back(fileToDl.c_str());
                    targetPathsVector.push_back(targetpah.c_str());
                }
            }
        }
        else
        {
            unsigned int numFiles;
            if (win)
            {
                numFiles = SIZEOF_ARRAY(UPDATE_FILES_WIN);
            }
            else
            {
                numFiles = SIZEOF_ARRAY(UPDATE_FILES_OSX);
            }
            for (unsigned int i = 0; i < numFiles; i++)
            {
                filesVector.push_back( (win ? UPDATE_FILES_WIN : UPDATE_FILES_OSX)[i]);
                targetPathsVector.push_back( (win ? TARGET_PATHS_WIN : TARGET_PATHS_OSX)[i]);
            }
        }


        for (unsigned int i = 0; i < filesVector.size(); i++)
        {
            string filePath = updateFolder + filesVector.at(i);
            signatureSize = signFile(filePath.data(), &aprivk, signature, sizeof(signature));
            if (!signatureSize)
            {
                cerr << "Error signing file: " << filePath << endl;
                return 4;
            }

            string s;
            s.resize((signatureSize*4)/3+4);
            s.resize(Base64::btoa((byte *)signature, signatureSize, (char *)s.data()));
            signatures.push_back(s);

            fileUrl.append(filesVector.at(i));
            downloadURLs.push_back(fileUrl);

            signatureGenerator.add((const byte*)fileUrl.data(), fileUrl.size());
            signatureGenerator.add((const byte*)targetPathsVector.at(i).data(),
                                   targetPathsVector.at(i).size());
            signatureGenerator.add((const byte*)s.data(), s.length());
        }

        signatureSize = signatureGenerator.get(&aprivk, signature, sizeof(signature));
        if (!signatureSize)
        {
            cerr << "Error signing the update file" << endl;
            return 6;
        }

        if (signatureSize < sizeof(signature))
        {
            int padding = sizeof(signature) - signatureSize;
            for (int i = sizeof(signature) - 1; i >= 0; i--)
            {
                if (i >= padding)
                {
                    signature[i] = signature[i - padding];
                }
                else
                {
                    signature[i] = 0;
                }
            }
            signatureSize = sizeof(signature);
        }

        string updateFileSignature;
        updateFileSignature.resize((signatureSize*4)/3+4);
        updateFileSignature.resize(Base64::btoa((byte *)signature, signatureSize, (char *)updateFileSignature.data()));

        //Print update file
        cout << args.at(2) << endl;
        cout << updateFileSignature << endl;
        for (unsigned int i = 0; i < targetPathsVector.size(); i++)
        {
            cout << downloadURLs[i] << endl;
            cout << targetPathsVector.at(i) << endl;
            cout << signatures[i] << endl;
        }

        return 0;
    }

    printUsage(argv[0]);
    return 1;
}

/**
 * @file examples/megacmd/megacmdexecuter.h
 * @brief MEGAcmd: Executer of the commands
 *
 * (c) 2013-2016 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGA SDK - Client Access Engine.
 *
 * Applications using the MEGA API must present a valid application key
 * and comply with the the rules set forth in the Terms of Service.
 *
 * The MEGA SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @copyright Simplified (2-clause) BSD License.
 *
 * You should have received a copy of the license along with this
 * program.
 */

#ifndef MEGACMDEXECUTER_H
#define MEGACMDEXECUTER_H

#include "megacmdlogger.h"
#include "megacmdsandbox.h"
#include "listeners.h"

class MegaCmdExecuter
{
private:
    mega::MegaApi *api;
    mega::handle cwd;
    char *session;
    mega::MegaFileSystemAccess *fsAccessCMD;
    MegaCMDLogger *loggerCMD;
    MegaCmdSandbox *sandboxCMD;
    MegaCmdGlobalTransferListener *globalTransferListener;
    mega::MegaMutex mtxSyncMap;

    // login/signup e-mail address
    std::string login;

    // signup name
    std::string name;

    // link to confirm
    std::string link;

    //delete confirmation
    std::vector<mega::MegaNode *> nodesToConfirmDelete;


    void updateprompt(mega::MegaApi *api, mega::MegaHandle handle);

public:
    bool signingup;
    bool confirming;

    MegaCmdExecuter(mega::MegaApi *api, MegaCMDLogger *loggerCMD, MegaCmdSandbox *sandboxCMD);
    ~MegaCmdExecuter();

    // nodes browsing
    void listtrees();
    static bool includeIfIsExported(mega::MegaApi* api, mega::MegaNode * n, void *arg);
    static bool includeIfIsShared(mega::MegaApi* api, mega::MegaNode * n, void *arg);
    static bool includeIfIsPendingOutShare(mega::MegaApi* api, mega::MegaNode * n, void *arg);
    static bool includeIfIsSharedOrPendingOutShare(mega::MegaApi* api, mega::MegaNode * n, void *arg);
    static bool includeIfMatchesPattern(mega::MegaApi* api, mega::MegaNode * n, void *arg);
    bool processTree(mega::MegaNode * n, bool(mega::MegaApi *, mega::MegaNode *, void *), void *( arg ));
    mega::MegaNode* nodebypath(const char* ptr, std::string* user = NULL, std::string* namepart = NULL);
    void getPathsMatching(mega::MegaNode *parentNode, std::deque<std::string> pathParts, std::vector<std::string> *pathsMatching, bool usepcre, std::string pathPrefix = "");
    void getNodesMatching(mega::MegaNode *parentNode, std::queue<std::string> pathParts, std::vector<mega::MegaNode *> *nodesMatching, bool usepcre);
    mega::MegaNode * getRootNodeByPath(const char *ptr, std::string* user = NULL);
    std::vector <mega::MegaNode*> * nodesbypath(const char* ptr, bool usepcre, std::string* user = NULL);
    std::vector <std::string> * nodesPathsbypath(const char* ptr, bool usepcre, std::string* user = NULL, std::string* namepart = NULL);
    void dumpNode(mega::MegaNode* n, int extended_info, int depth = 0, const char* title = NULL);
    void dumptree(mega::MegaNode* n, int recurse, int extended_info, int depth = 0, std::string pathRelativeTo = "NULL");
    mega::MegaContactRequest * getPcrByContact(std::string contactEmail);
    bool TestCanWriteOnContainingFolder(std::string *path);
    std::string getDisplayPath(std::string givenPath, mega::MegaNode* n);
    int dumpListOfExported(mega::MegaNode* n, std::string givenPath);
    void listnodeshares(mega::MegaNode* n, std::string name);
    void dumpListOfShared(mega::MegaNode* n, std::string givenPath);
    void dumpListOfAllShared(mega::MegaNode* n, std::string givenPath);
    void dumpListOfPendingShares(mega::MegaNode* n, std::string givenPath);
    std::string getCurrentPath();

    //acting
    void loginWithPassword(char *password);
    void changePassword(const char *oldpassword, const char *newpassword);
    void actUponGetExtendedAccountDetails(mega::SynchronousRequestListener  *srl, int timeout = -1);
    bool actUponFetchNodes(mega::MegaApi * api, mega::SynchronousRequestListener  *srl, int timeout = -1);
    void actUponLogin(mega::SynchronousRequestListener  *srl, int timeout = -1);
    void actUponLogout(mega::SynchronousRequestListener  *srl, bool deletedSession, int timeout = 0);
    int actUponCreateFolder(mega::SynchronousRequestListener  *srl, int timeout = 0);
    void deleteNode(mega::MegaNode *nodeToDelete, mega::MegaApi* api, int recursive, int force = 0);
    void downloadNode(std::string localPath, mega::MegaApi* api, mega::MegaNode *node, bool background, bool ignorequotawar, int clientID);
    void uploadNode(std::string localPath, mega::MegaApi* api, mega::MegaNode *node, std::string newname, bool background, bool ignorequotawarn, int clientID);
    void exportNode(mega::MegaNode *n, int expireTime);
    void disableExport(mega::MegaNode *n);
    void shareNode(mega::MegaNode *n, std::string with, int level = mega::MegaShare::ACCESS_READ);
    void disableShare(mega::MegaNode *n, std::string with);
    std::vector<std::string> listpaths(bool usepcre, std::string askedPath = "", bool discardFiles = false);
    std::vector<std::string> getlistusers();
    std::vector<std::string> getNodeAttrs(std::string nodePath);
    std::vector<std::string> getUserAttrs();
    std::vector<std::string> getsessions();

    void executecommand(std::vector<std::string> words, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions);

    bool checkNoErrors(mega::MegaError *error, std::string message = "");

    //doomedtodie
    void syncstat(mega::Sync* sync);
//    const char* treestatename(treestate_t ts);
    bool is_syncable(const char* name);
    int loadfile(std::string* name, std::string* data);
    void signup(std::string name, std::string passwd, std::string email);
    void signupWithPassword(std::string passwd);
    void confirm(std::string passwd, std::string email, std::string link);
    void confirmWithPassword(std::string passwd);

    int makedir(std::string remotepath, bool recursive, mega::MegaNode *parentnode = NULL);
    bool IsFolder(std::string path);
    void doDeleteNode(mega::MegaNode *nodeToDelete, mega::MegaApi* api);

    void confirmDelete();
    void discardDelete();

    void printTransfersHeader(const unsigned int PATHSIZE, bool printstate=true);
    void printTransfer(mega::MegaTransfer *transfer, const unsigned int PATHSIZE, bool printstate=true);
    void doFind(mega::MegaNode* nodeBase, std::string word, int printfileinfo, std::string pattern, bool usepcre);

    void move(mega::MegaNode *n, std::string destiny);
    std::string getLPWD();
    bool isValidFolder(std::string destiny);
};

#endif // MEGACMDEXECUTER_H

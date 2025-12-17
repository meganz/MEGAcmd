/**
 * @file src/megacmdexecuter.h
 * @brief MEGAcmd: Executer of the commands
 *
 * (c) 2013 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGAcmd.
 *
 * MEGAcmd is distributed in the hope that it will be useful,
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
#include "deferred_single_trigger.h"
#include "sync_issues.h"

namespace megacmd {
class MegaCmdGlobalTransferListener;
class MegaCmdMultiTransferListener;
class MegaCmdSandbox;

class MegaCmdExecuter
{
private:
    mega::MegaApi *api;
    mega::handle cwd;
    std::unique_ptr<char[]> session;
    std::unique_ptr<mega::FileSystemAccess> mFsAccessCMD;
    MegaCmdLogger *loggerCMD;
    MegaCmdSandbox *sandboxCMD;
    MegaCmdGlobalTransferListener *globalTransferListener;
    std::mutex mtxSyncMap;
    std::mutex mtxWebDavLocations;
    std::mutex mtxFtpLocations;

    DeferredSingleTrigger mDeferredSharedFoldersVerifier;
    SyncIssuesManager mSyncIssuesManager;

    std::recursive_mutex mtxBackupsMap;

    // login/signup e-mail address
    std::string login;

    // signup name
    std::string name;

    // delete confirmation
    std::vector<std::unique_ptr<mega::MegaNode>> mNodesToConfirmDelete;

    std::string getNodePathString(mega::MegaNode *n);

    void cancelOngoingVerification(mega::MegaApi* api, bool start_new_verification);

    /**
     * @name forEachFileInNode
     * @brief Traverses through all files in the given node with the provided callback.
     * @param n - the node object to traverse
     * @param recurse - whether to recursively traverse directories in this node
     * @param callback - the callback which receives child entries. Should be of the form `void
     * (MegaNode *)`
     */
    template <typename Cb>
    void forEachFileInNode(mega::MegaNode &n, bool recurse, Cb &&callback)
    {
        auto children = std::unique_ptr<mega::MegaNodeList>(api->getChildren(&n));
        if (children != nullptr)
        {
            for (int i = 0; i < children->size(); i++)
            {
                auto child = children->get(i);
                if (child->getType() == mega::MegaNode::TYPE_FILE)
                {
                    callback(children->get(i));
                }
                else if (recurse)
                {
                    forEachFileInNode(*child, true, std::forward<Cb>(callback));
                }
            }
        }
    }

public:
    bool signingup;
    bool confirming;
    bool confirmingcancel;
    // link to confirm
    std::string link;

    MegaCmdExecuter(mega::MegaApi *api, MegaCmdLogger *loggerCMD, MegaCmdSandbox *sandboxCMD);
    ~MegaCmdExecuter();

    void updateprompt(mega::MegaApi *api = nullptr);

    // nodes browsing
    void listtrees();
    static bool includeIfIsExported(mega::MegaApi* api, mega::MegaNode * n, void *arg);
    static bool includeIfIsShared(mega::MegaApi* api, mega::MegaNode * n, void *arg);
    static bool includeIfIsPendingOutShare(mega::MegaApi* api, mega::MegaNode * n, void *arg);
    static bool includeIfIsSharedOrPendingOutShare(mega::MegaApi* api, mega::MegaNode * n, void *arg);
    static bool includeIfMatchesPattern(mega::MegaApi* api, mega::MegaNode * n, void *arg);
    static bool includeIfMatchesCriteria(mega::MegaApi* api, mega::MegaNode * n, void *arg);

    bool processTree(mega::MegaNode * n, bool(mega::MegaApi *, mega::MegaNode *, void *), void *( arg ));

    std::unique_ptr<mega::MegaNode> nodebypath(const char* ptr, std::string* user = nullptr, std::string* namepart = nullptr);
    std::vector<std::unique_ptr<mega::MegaNode>> nodesbypath(const char* ptr, bool usepcre, std::string* user = nullptr);
    void getNodesMatching(mega::MegaNode* parentNode, std::deque<std::string> pathParts, std::vector<std::unique_ptr<mega::MegaNode>>& nodesMatching, bool usepcre);

    std::vector <std::string> * nodesPathsbypath(const char* ptr, bool usepcre, std::string* user = NULL, std::string* namepart = NULL);
    void getPathsMatching(mega::MegaNode *parentNode, std::deque<std::string> pathParts, std::vector<std::string> *pathsMatching, bool usepcre, std::string pathPrefix = "");

    void printTreeSuffix(int depth, std::vector<bool> &lastleaf);
    void dumpNode(mega::MegaNode* n, const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, int extended_info, bool showversions = false, int depth = 0, const char* title = NULL);
    void dumptree(mega::MegaNode* n, bool treelike, std::vector<bool> &lastleaf, const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, int recurse, int extended_info, bool showversions = false, int depth = 0, std::string pathRelativeTo = "NULL");
    void dumpNodeSummaryHeader(const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions);
    void dumpNodeSummary(mega::MegaNode* n, const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, bool humanreadable = false, const char* title = NULL);
    void dumpTreeSummary(mega::MegaNode* n, const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, int recurse, bool show_versions, int depth = 0, bool humanreadable = false, std::string pathRelativeTo = "NULL");
    std::unique_ptr<mega::MegaContactRequest> getPcrByContact(std::string contactEmail);
    bool TestCanWriteOnContainingFolder(std::string path);
    std::string getDisplayPath(std::string givenPath, mega::MegaNode* n);
    int dumpListOfExported(mega::MegaNode* n, const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, std::string givenPath);
    void listnodeshares(mega::MegaNode* n, std::string name, bool listPending, bool onlyPending);
    void dumpListOfShared(mega::MegaNode* n, std::string givenPath);
    void dumpListOfAllShared(mega::MegaNode* n, std::string givenPath);
    void dumpListOfPendingShares(mega::MegaNode* n, std::string givenPath);
    std::string getCurrentPath();
    long long getVersionsSize(mega::MegaNode* n);

    //acting
    void verifySharedFolders(mega::MegaApi * api); //verifies unverified shares and broadcasts warning accordingly
    void loginWithPassword(const char *password);
    void changePassword(const char *newpassword, std::string pin2fa = "");
    void actUponGetExtendedAccountDetails(std::unique_ptr<mega::MegaAccountDetails> storageDetails, std::unique_ptr<mega::MegaAccountDetails> extAccountDetails);
    bool actUponFetchNodes(mega::MegaApi * api, mega::SynchronousRequestListener  *srl, int timeout = -1);
    int actUponLogin(mega::SynchronousRequestListener *srl, int timeout = -1);
    void actUponLogout(mega::MegaApi& api, mega::MegaError* e, bool keptSession);
    void actUponLogout(mega::SynchronousRequestListener *srl, bool keptSession, int timeout = 0);
    int actUponCreateFolder(mega::SynchronousRequestListener *srl, int timeout = 0);
    int deleteNode(const std::unique_ptr<mega::MegaNode>& nodeToDelete, mega::MegaApi* api, int recursive, int force = 0);
    int deleteNodeVersions(const std::unique_ptr<mega::MegaNode>& nodeToDelete, mega::MegaApi* api, int force = 0);
    void downloadNode(std::string source, std::string localPath, mega::MegaApi* api, mega::MegaNode *node, bool background, bool ignorequotawar, int clientID, std::shared_ptr<MegaCmdMultiTransferListener> listener);
    void uploadNode(const std::map<std::string, int> &clflags, const std::map<std::string, std::string> &cloptions, const std::string &receivedPath, mega::MegaApi* api, mega::MegaNode *node, const std::string &newname, MegaCmdMultiTransferListener *multiTransferListener = NULL);
    void exportNode(mega::MegaNode *n, int64_t expireTime, const std::optional<std::string>& password = {},
                    std::map<std::string, int> *clflags = nullptr, std::map<std::string, std::string> *cloptions = nullptr);
    void disableExport(mega::MegaNode *n);
    std::pair<bool, bool> isSharePendingAndVerified(mega::MegaNode *n, const char *email) const;
    void shareNode(mega::MegaNode *n, std::string with, int level = mega::MegaShare::ACCESS_READ);
    void disableShare(mega::MegaNode *n, std::string with);
    void createOrModifyBackup(std::string local, std::string remote, std::string speriod, int numBackups);
    std::vector<std::string> listpaths(bool usepcre, std::string askedPath = "", bool discardFiles = false);
    std::vector<std::string> listLocalPathsStartingBy(std::string askedPath, bool discardFiles);
    std::vector<std::string> getlistusers();
    std::vector<std::string> getNodeAttrs(std::string nodePath);
    std::vector<std::string> getUserAttrs();
    std::vector<std::string> getsessions();
    std::vector<std::string> getlistfilesfolders(std::string location);

    void executecommand(std::vector<std::string> words, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions);

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
    bool pathExists(const std::string &path);
    void doDeleteNode(const std::unique_ptr<mega::MegaNode>& nodeToDelete, mega::MegaApi* api);

    void confirmDelete();
    void discardDelete();
    void confirmDeleteAll();
    void discardDeleteAll();

    void printTransfersHeader(const unsigned int PATHSIZE, bool printstate=true);
    void printTransfer(mega::MegaTransfer *transfer, const unsigned int PATHSIZE, bool printstate=true);
    void printTransferColumnDisplayer(ColumnDisplayer *cd, mega::MegaTransfer *transfer, bool printstate=true);

    void printBackupHeader(const unsigned int PATHSIZE);
    void printBackupSummary(int tag, const char *localfolder, const char *remoteparentfolder, std::string status, const unsigned int PATHSIZE);
    void printBackupHistory(mega::MegaScheduledCopy *backup, const char *timeFormat, mega::MegaNode *parentnode, const unsigned int PATHSIZE);
    void printBackupDetails(mega::MegaScheduledCopy *backup, const char *timeFormat);
    void printBackup(int tag, mega::MegaScheduledCopy *backup, const char *timeFormat, const unsigned int PATHSIZE, bool extendedinfo = false, bool showhistory = false, mega::MegaNode *parentnode = NULL);
    void printBackup(backup_struct *backupstruct, const char *timeFormat, const unsigned int PATHSIZE, bool extendedinfo = false, bool showhistory = false);

    void doFind(mega::MegaNode* nodeBase, const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, std::string word, int printfileinfo, std::string pattern, bool usepcre, mega::m_time_t minTime, mega::m_time_t maxTime, int64_t minSize, int64_t maxSize);

    void moveToDestination(const std::unique_ptr<mega::MegaNode>& n, std::string destiny);
    void copyNode(mega::MegaNode *n, std::string destiny, mega::MegaNode *tn, std::string &targetuser, std::string &newname);
    std::string getLPWD();
    bool isValidFolder(std::string destiny);
    bool establishBackup(std::string local, mega::MegaNode *n, int64_t period, std::string periodstring, int numBackups);
    mega::MegaNode *getBaseNode(std::string thepath, std::string &rest, bool *isrelative = NULL);
    void getPathParts(std::string path, std::deque<std::string> *c);

    // decrypt a link if it's encrypted. Returns false in case of error
    bool decryptLinkIfEncrypted(mega::MegaApi *api, std::string &publicLink, std::map<std::string, std::string> *cloptions);

    bool checkAndInformPSA(CmdPetition *inf, bool enforce = false);

    // Provide a helpful error message for the provided error, setting the current error code in case of an error.
    std::string formatErrorAndMaySetErrorCode(const mega::MegaError &error);
    bool checkNoErrors(int errorCode, const std::string &message = "");
    bool checkNoErrors(mega::MegaError *error, const std::string &message = "", mega::SyncError syncError = mega::SyncError::NO_SYNC_ERROR);
    bool checkNoErrors(::mega::SynchronousRequestListener *listener, const std::string &message = "", mega::SyncError syncError = mega::SyncError::NO_SYNC_ERROR);

    void confirmCancel(const char* confirmlink, const char* pass);
    bool amIPro();

    void processPath(std::string path, bool usepcre, bool& firstone, void (*nodeprocessor)(MegaCmdExecuter *, mega::MegaNode *, bool), MegaCmdExecuter *context = NULL);
    void catFile(mega::MegaNode *n);
    void printInfoFile(mega::MegaNode *n, bool &firstone, int PATHSIZE);


#ifdef HAVE_LIBUV
    void removeWebdavLocation(mega::MegaNode *n, bool firstone, std::string name = std::string());
    void addWebdavLocation(mega::MegaNode *n, bool firstone, std::string name = std::string());
    void removeFtpLocation(mega::MegaNode *n, bool firstone, std::string name = std::string());
    void addFtpLocation(mega::MegaNode *n, bool firstone, std::string name = std::string());
#endif
    bool printUserAttribute(int a, std::string user, bool onlylist = false);
    bool setProxy(const std::string &url, const std::string &username, const std::string &password, int proxyType);
    void fetchNodes(mega::MegaApi *api = nullptr, int clientID = -27);

    void mayExecutePendingStuffInWorkerThread();
};

}//end namespace
#endif // MEGACMDEXECUTER_H

/**
 * @file src/megacmdexecuter.cpp
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

#include "megacmdexecuter.h"
#include "megaapi.h"
#include "megacmd.h"

#include "megacmdutils.h"
#include "megacmdcommonutils.h"
#include "configurationmanager.h"
#include "megacmdlogger.h"
#include "comunicationsmanager.h"
#include "listeners.h"
#include "megacmdversion.h"
#include "sync_command.h"
#include "sync_ignore.h"
#include "megacmd_fuse.h"

#include <iomanip>
#include <limits>
#include <string>
#include <ctime>
#include <set>

#include <signal.h>

#ifdef MEGACMD_TESTING_CODE
    #include "../tests/common/Instruments.h"
    using TI = TestInstruments;
#endif

using namespace mega;
using namespace std;

namespace megacmd {
static const char* rootnodenames[] = { "ROOT", "INBOX", "RUBBISH" };
static const char* rootnodepaths[] = { "/", "//in", "//bin" };

#define SSTR( x ) static_cast< const std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()


#ifdef HAVE_GLOB_H
std::vector<std::string> resolvewildcard(const std::string& pattern) {

    vector<std::string> filenames;
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));

    if (!glob(pattern.c_str(), GLOB_TILDE, NULL, &glob_result))
    {
        for(size_t i = 0; i < glob_result.gl_pathc; ++i) {
            filenames.push_back(std::string(glob_result.gl_pathv[i]));
        }
    }

    globfree(&glob_result);
    return filenames;
}
#endif

#ifdef WITH_FUSE
std::optional<FuseCommand::ConfigDelta> loadFuseConfigDelta(const std::map<std::string, std::string>& cloptions)
{
    FuseCommand::ConfigDelta configDelta;

    std::string enableAtStartupStr = getOption(&cloptions, "enable-at-startup", "");
    if (!enableAtStartupStr.empty())
    {
        if (enableAtStartupStr != "yes" && enableAtStartupStr != "no")
        {
            LOG_err << "Flag \"enable-at-startup\" can only be \"yes\" or \"no\"";
            return {};
        }
        configDelta.mEnableAtStartup = (enableAtStartupStr == "yes");
    }

    std::string persistentStr = getOption(&cloptions, "persistent", "");
    if (!persistentStr.empty())
    {
        if (persistentStr != "yes" && persistentStr != "no")
        {
            LOG_err << "Flag \"persistent\" can only be \"yes\" or \"no\"";
            return {};
        }
        configDelta.mPersistent = (persistentStr == "yes");
    }

    std::string readOnlyStr = getOption(&cloptions, "read-only", "");
    if (!readOnlyStr.empty())
    {
        if (readOnlyStr != "yes" && readOnlyStr != "no")
        {
            LOG_err << "Flag \"read-only\" can only be \"yes\" or \"no\"";
            return {};
        }
        configDelta.mReadOnly = (readOnlyStr == "yes");
    }

    configDelta.mName = getOptionAsOptional(cloptions, "name");

    if (!configDelta.isAnyFlagSet())
    {
        LOG_err << "At least one flag must be set";
        return {};
    }

    if (configDelta.isPersistentStartupInvalid())
    {
        LOG_err << "A non-persistent mount cannot be enabled at startup";
        return {};
    }

    return configDelta;
}
#endif

/**
 * @brief updateprompt updates prompt with the current user/location
 * @param api
 * @param handle
 */
void MegaCmdExecuter::updateprompt(MegaApi *api)
{
    if (!api) api = this->api;
    MegaHandle handle = cwd;

    string newprompt;

    MegaNode *n = api->getNodeByHandle(handle);

    MegaUser *u = api->getMyUser();
    if (u)
    {
        const char *email = u->getEmail();
        newprompt.append(email);
        delete u;
    }

    if (n)
    {
        char *np = api->getNodePath(n);

        if (!newprompt.empty())
        {
            newprompt.append(":");
        }

        if (np)
        {
            newprompt.append(np);
        }

        delete n;
        delete []np;
    }

    if (getBlocked())
    {
        newprompt.append("[BLOCKED]");
    }

    if (newprompt.empty()) //i.e. !u && !n
    {
        newprompt = prompts[0];
    }
    else
    {
        newprompt.append("$ ");
    }

    changeprompt(newprompt.c_str());
}

MegaCmdExecuter::MegaCmdExecuter(MegaApi *api, MegaCmdLogger *loggerCMD, MegaCmdSandbox *sandboxCMD) :
    // Give a few seconds in order for key sharing to happen
    mFsAccessCMD(::mega::createFSA()),
    mDeferredSharedFoldersVerifier(std::chrono::seconds(5)),
    mSyncIssuesManager(api)
{
    signingup = false;
    confirming = false;

    this->api = api;
    this->loggerCMD = loggerCMD;
    this->sandboxCMD = sandboxCMD;
    this->globalTransferListener = new MegaCmdGlobalTransferListener(api, sandboxCMD);
    api->addTransferListener(globalTransferListener);
    api->addGlobalListener(mSyncIssuesManager.getGlobalListener());
    cwd = UNDEF;
    session = NULL;

}

MegaCmdExecuter::~MegaCmdExecuter()
{
    delete globalTransferListener;
}

// list available top-level nodes and contacts/incoming shares
void MegaCmdExecuter::listtrees()
{
    for (int i = 0; i < (int)( sizeof rootnodenames / sizeof *rootnodenames ); i++)
    {
        OUTSTREAM << rootnodenames[i] << " on " << rootnodepaths[i] << endl;
        if (!api->isLoggedIn())
        {
            break;                     //only show /root
        }
    }

    MegaShareList * msl = api->getInSharesList();
    for (int i = 0; i < msl->size(); i++)
    {
        MegaShare *share = msl->get(i);
        MegaNode *n = api->getNodeByHandle(share->getNodeHandle());

        OUTSTREAM << "INSHARE on //from/" << share->getUser() << ":" << n->getName() << " (" << getAccessLevelStr(share->getAccess()) << ")"
                  << (!share->isVerified() ? " [UNVERIFIED]" : "") << endl;
        delete n;
    }

    delete ( msl );
}

bool MegaCmdExecuter::includeIfIsExported(MegaApi *api, MegaNode * n, void *arg)
{
    if (n->isExported())
    {
        (( vector<MegaNode*> * )arg )->push_back(n->copy());
        return true;
    }
    return false;
}

bool MegaCmdExecuter::includeIfIsShared(MegaApi *api, MegaNode * n, void *arg)
{
    if (n->isShared())
    {
        (( vector<MegaNode*> * )arg )->push_back(n->copy());
        return true;
    }
    return false;
}

bool MegaCmdExecuter::includeIfIsPendingOutShare(MegaApi *api, MegaNode * n, void *arg)
{
    MegaShareList* pendingoutShares = api->getPendingOutShares(n);
    if (pendingoutShares && pendingoutShares->size())
    {
        (( vector<MegaNode*> * )arg )->push_back(n->copy());
        return true;
    }
    if (pendingoutShares)
    {
        delete pendingoutShares;
    }
    return false;
}


bool MegaCmdExecuter::includeIfIsSharedOrPendingOutShare(MegaApi *api, MegaNode * n, void *arg)
{
    if (n->isShared())
    {
        (( vector<MegaNode*> * )arg )->push_back(n->copy());
        return true;
    }
    MegaShareList* pendingoutShares = api->getPendingOutShares(n);
    if (pendingoutShares && pendingoutShares->size())
    {
        (( vector<MegaNode*> * )arg )->push_back(n->copy());
        return true;
    }
    if (pendingoutShares)
    {
        delete pendingoutShares;
    }
    return false;
}

struct patternNodeVector
{
    string pattern;
    bool usepcre;
    vector<MegaNode*> *nodesMatching;
};

struct criteriaNodeVector
{
    string pattern;
    bool usepcre;
    m_time_t minTime;
    m_time_t maxTime;

    int64_t maxSize;
    int64_t minSize;

    int mType = MegaNode::TYPE_UNKNOWN;

    vector<MegaNode*> *nodesMatching;
};

bool MegaCmdExecuter::includeIfMatchesPattern(MegaApi *api, MegaNode * n, void *arg)
{
    struct patternNodeVector *pnv = (struct patternNodeVector*)arg;
    if (patternMatches(n->getName(), pnv->pattern.c_str(), pnv->usepcre))
    {
        pnv->nodesMatching->push_back(n->copy());
        return true;
    }
    return false;
}


bool MegaCmdExecuter::includeIfMatchesCriteria(MegaApi *api, MegaNode * n, void *arg)
{
    struct criteriaNodeVector *pnv = (struct criteriaNodeVector*)arg;

    if (pnv->mType != MegaNode::TYPE_UNKNOWN && n->getType() != pnv->mType )
    {
        return false;
    }

    if ( pnv->maxTime != -1 && (n->getModificationTime() >= pnv->maxTime) )
    {
        return false;
    }
    if ( pnv->minTime != -1 && (n->getModificationTime() <= pnv->minTime) )
    {
        return false;
    }

    if ( pnv->maxSize != -1 && (n->getType() != MegaNode::TYPE_FILE || (n->getSize() > pnv->maxSize) ) )
    {
        return false;
    }

    if ( pnv->minSize != -1 && (n->getType() != MegaNode::TYPE_FILE || (n->getSize() < pnv->minSize) ) )
    {
        return false;
    }

    if (!patternMatches(n->getName(), pnv->pattern.c_str(), pnv->usepcre))
    {
        return false;
    }

    pnv->nodesMatching->push_back(n->copy());
    return true;
}

bool MegaCmdExecuter::processTree(MegaNode *n, bool processor(MegaApi *, MegaNode *, void *), void *( arg ))
{
    if (!n)
    {
        return false;
    }
    bool toret = true;
    MegaNodeList *children = api->getChildren(n);
    if (children)
    {
        for (int i = 0; i < children->size(); i++)
        {
            bool childret = processTree(children->get(i), processor, arg);
            toret = toret && childret;
        }

        delete children;
    }

    bool currentret = processor(api, n, arg);
    return toret && currentret;
}


// returns node pointer determined by path relative to cwd
// path naming conventions:
// * path is relative to cwd
// * /path is relative to ROOT
// * //in is in INBOX
// * //bin is in RUBBISH
// * X: is user X's INBOX
// * X:SHARE is share SHARE from user X
// * H:HANDLE is node with handle HANDLE
// * : and / filename components, as well as the \, must be escaped by \.
// (correct UTF-8 encoding is assumed)
// returns nullptr if path malformed or not found
std::unique_ptr<MegaNode> MegaCmdExecuter::nodebypath(const char* ptr, string* user, string* namepart)
{
    if (ptr && ptr[0] == 'H' && ptr[1] == ':')
    {
        std::unique_ptr<MegaNode> n(api->getNodeByHandle(api->base64ToHandle(ptr+2)));
        if (n)
        {
            return n;
        }
    }

    string rest;
    std::unique_ptr<MegaNode> baseNode(getBaseNode(ptr, rest));

    if (baseNode && !rest.size())
    {
        return baseNode;
    }

    if (!rest.size() && !baseNode)
    {
        string path(ptr);
        if (path.size() && path.find("@") != string::npos && path.find_last_of(":") == (path.size() - 1))
        {
            if (user)
            {
                *user = path.substr(0,path.size()-1);
            }
        }
    }

    while (baseNode)
    {
        size_t possep = rest.find('/');
        string curName = rest.substr(0,possep);

        if (curName != ".")
        {
            std::unique_ptr<MegaNode> nextNode;
            if (curName == "..")
            {
                nextNode.reset(api->getParentNode(baseNode.get()));
            }
            else
            {
                replaceAll(curName, "\\\\", "\\"); //unescape '\\'
                replaceAll(curName, "\\ ", " "); //unescape '\ '
                bool isversion = nodeNameIsVersion(curName);
                if (isversion)
                {
                    std::unique_ptr<MegaNode> childNode(api->getChildNode(baseNode.get(), curName.substr(0,curName.size()-11).c_str()));
                    if (childNode)
                    {
                        std::unique_ptr<MegaNodeList> versionNodes(api->getVersions(childNode.get()));
                        if (versionNodes)
                        {
                            for (int i = 0; i < versionNodes->size(); i++)
                            {
                                MegaNode* versionNode = versionNodes->get(i);
                                if (curName.substr(curName.size()-10) == SSTR(versionNode->getModificationTime()))
                                {
                                    nextNode.reset(versionNode->copy());
                                    break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    nextNode.reset(api->getChildNode(baseNode.get(), curName.c_str()));
                }
            }

            // mv command target? return name part of not found
            if (namepart && !nextNode && (possep == string::npos)) //if this is the last part, we will pass that one, so that a mv command know the name to give the new node
            {
                *namepart = rest;
                return baseNode;
            }

            baseNode = std::move(nextNode);
        }

        if (possep != string::npos && possep != (rest.size() - 1))
        {
            rest = rest.substr(possep+1);
        }
        else
        {
            return baseNode;
        }
    }

    return nullptr;
}

/**
 * @brief MegaCmdExecuter::getPathsMatching Gets paths of nodes matching a pattern given its path parts and a parent node
 *
 * @param parentNode node for reference for relative paths
 * @param pathParts path pattern (separated in strings)
 * @param pathsMatching for the returned paths
 * @param usepcre use PCRE expressions if available
 * @param pathPrefix prefix to append to paths
 */
void MegaCmdExecuter::getPathsMatching(MegaNode *parentNode, deque<string> pathParts, vector<string> *pathsMatching, bool usepcre, string pathPrefix)
{
    if (!pathParts.size())
    {
        return;
    }

    string currentPart = pathParts.front();
    pathParts.pop_front();

    if (currentPart == "." || currentPart == "")
    {
        if (pathParts.size() == 0  /*&& currentPart == "."*/) //last leave.  // for consistency we also take parent when ended in / even if it's not a folder
         {
             pathsMatching->push_back(pathPrefix+currentPart);
         }

        //ignore this part
        return getPathsMatching(parentNode, pathParts, pathsMatching, usepcre, pathPrefix+"./");
    }
    if (currentPart == "..")
    {
        if (parentNode->getParentHandle())
        {
            if (!pathParts.size())
            {
                pathsMatching->push_back(pathPrefix+"..");
            }

            unique_ptr<MegaNode> p(api->getNodeByHandle(parentNode->getParentHandle()));
            return getPathsMatching(p.get(), pathParts, pathsMatching, usepcre, pathPrefix+"../");
        }
        else
        {
            return; //trying to access beyond root node
        }
    }

    MegaNodeList* children = api->getChildren(parentNode);
    if (children)
    {
        bool isversion = nodeNameIsVersion(currentPart);

        for (int i = 0; i < children->size(); i++)
        {
            MegaNode *childNode = children->get(i);
            // get childname from its path: alternative: childNode->getName()
            char *childNodePath = api->getNodePath(childNode);
            char *aux;
            aux = childNodePath+strlen(childNodePath);
            while (aux>childNodePath){
                if (*aux=='/' && *(aux-1) != '\\')  break;
                aux--;
            }
            if (*aux=='/') aux++;
            string childname(aux);
            delete []childNodePath;

            if (isversion)
            {

                if (childNode && patternMatches(childname.c_str(), currentPart.substr(0,currentPart.size()-11).c_str(), usepcre))
                {
                    MegaNodeList *versionNodes = api->getVersions(childNode);
                    if (versionNodes)
                    {
                        for (int i = 0; i < versionNodes->size(); i++)
                        {
                            MegaNode *versionNode = versionNodes->get(i);
                            if ( currentPart.substr(currentPart.size()-10) == SSTR(versionNode->getModificationTime()) )
                            {
                                if (pathParts.size() == 0) //last leave
                                {
                                    pathsMatching->push_back(pathPrefix+childname+"#"+SSTR(versionNode->getModificationTime())); //TODO: def version separator elswhere
                                }
                                else
                                {
                                    getPathsMatching(versionNode, pathParts, pathsMatching, usepcre,pathPrefix+childname+"#"+SSTR(versionNode->getModificationTime())+"/");
                                }

                                break;
                            }
                        }
                        delete versionNodes;
                    }
                }
            }
            else
            {
                if (patternMatches(childname.c_str(), currentPart.c_str(), usepcre))
                {
                    if (pathParts.size() == 0) //last leave
                    {
                        pathsMatching->push_back(pathPrefix+childname);
                    }
                    else
                    {
                        getPathsMatching(childNode, pathParts, pathsMatching, usepcre,pathPrefix+childname+"/");
                    }
                }


            }

        }

        delete children;
    }
}

/**
 * @brief MegaCmdExecuter::nodesPathsbypath returns paths of nodes that match a determined by path pattern
 * path naming conventions:
 * path is relative to cwd
 * /path is relative to ROOT
 * //in is in INBOX
 * //bin is in RUBBISH
 * X: is user X's INBOX
 * X:SHARE is share SHARE from user X
 * H:HANDLE is node with handle HANDLE
 * : and / filename components, as well as the \, must be escaped by \.
 * (correct UTF-8 encoding is assumed)
 *
 * You take the ownership of the returned value
 * @param ptr
 * @param usepcre use PCRE expressions if available
 * @param user
 * @param namepart
 * @return
 */
vector <string> * MegaCmdExecuter::nodesPathsbypath(const char* ptr, bool usepcre, string* user, string* namepart)
{
    vector<string> *pathsMatching = new vector<string> ();

    if (ptr && ptr[0] == 'H' && ptr[1] == ':')
    {
        MegaNode * n = api->getNodeByHandle(api->base64ToHandle(ptr+2));
        if (n)
        {
            char * nodepath = api->getNodePath(n);
            pathsMatching->push_back(nodepath);
            delete []nodepath;
            return pathsMatching;
        }
    }

    string rest;
    bool isrelative;
    MegaNode *baseNode = getBaseNode(ptr, rest, &isrelative);

    if (baseNode)
    {
        string pathPrefix;
        if (!isrelative)
        {
            char * nodepath = api->getNodePath(baseNode);
            pathPrefix=nodepath;
            if (pathPrefix.size() && pathPrefix.at(pathPrefix.size()-1)!='/')
                pathPrefix+="/";
            delete []nodepath;
        }

        if (string(ptr).find("//from/") == 0)
        {
            pathPrefix.insert(0,"//from/");
        }

        deque<string> c;
        getPathParts(rest, &c);

        if (!c.size())
        {
            char * nodepath = api->getNodePath(baseNode);
            pathsMatching->push_back(nodepath);
            delete []nodepath;
        }
        else
        {
            getPathsMatching((MegaNode *)baseNode, c, (vector<string> *)pathsMatching, usepcre, pathPrefix);
        }
        delete baseNode;
    }
    else if (!strncmp(ptr,"//from/",max(3,min((int)strlen(ptr)-1,7)))) //pattern trying to match inshares
    {

        string matching = ptr;
        unescapeifRequired(matching);
        unique_ptr<MegaShareList> inShares(api->getInSharesList());
        if (inShares)
        {
            for (int i = 0; i < inShares->size(); i++)
            {
                unique_ptr<MegaNode> n(api->getNodeByHandle(inShares->get(i)->getNodeHandle()));
                string tomatch = string("//from/")+inShares->get(i)->getUser() + ":"+n->getName();

                if (patternMatches(tomatch.c_str(), matching.c_str(), false))
                {
                    pathsMatching->push_back(tomatch);
                }
            }
        }
    }

    return pathsMatching;
}

/**
 *  You take the ownership of the nodes added in nodesMatching
 * @brief getNodesMatching
 * @param parentNode
 * @param c
 * @param nodesMatching
 */
void MegaCmdExecuter::getNodesMatching(MegaNode *parentNode, deque<string> pathParts, vector<std::unique_ptr<MegaNode>>& nodesMatching, bool usepcre)
{
    if (!pathParts.size())
    {
        return;
    }

    string currentPart = pathParts.front();
    pathParts.pop_front();

    if (currentPart == "." || currentPart == "")
    {
        if (pathParts.size() == 0  /*&& currentPart == "."*/) //last leave.  // for consistency we also take parent when ended in / even if it's not a folder
        {
            if (parentNode)
            {
                nodesMatching.emplace_back(parentNode->copy());
                return;
            }
        }
        else
        {
            //ignore this part
            return getNodesMatching(parentNode, pathParts, nodesMatching, usepcre);
        }
    }
    if (currentPart == "..")
    {
        if (parentNode->getParentHandle())
        {
            MegaNode *newparentNode = api->getNodeByHandle(parentNode->getParentHandle());
            if (!pathParts.size()) //last leave
            {
                if (newparentNode)
                {
                    nodesMatching.emplace_back(newparentNode);
                }
                return;
            }
            else
            {
                getNodesMatching(newparentNode, pathParts, nodesMatching, usepcre);
                delete newparentNode;
                return;
            }

        }
        else
        {
            return; //trying to access beyond root node
        }

    }

    MegaNodeList* children = api->getChildren(parentNode);
    if (children)
    {
        bool isversion = nodeNameIsVersion(currentPart);

        for (int i = 0; i < children->size(); i++)
        {
            MegaNode *childNode = children->get(i);
            if (isversion)
            {

                if (childNode && patternMatches(childNode->getName(), currentPart.substr(0,currentPart.size()-11).c_str(), usepcre))
                {
                    MegaNodeList *versionNodes = api->getVersions(childNode);
                    if (versionNodes)
                    {
                        for (int i = 0; i < versionNodes->size(); i++)
                        {
                            MegaNode *versionNode = versionNodes->get(i);
                            if ( currentPart.substr(currentPart.size()-10) == SSTR(versionNode->getModificationTime()) )
                            {
                                if (pathParts.size() == 0) //last leave
                                {
                                    nodesMatching.emplace_back(versionNode->copy());
                                }
                                else
                                {
                                    getNodesMatching(versionNode, pathParts, nodesMatching, usepcre);
                                }

                                break;
                            }
                        }
                        delete versionNodes;
                    }
                }
            }
            else
            {

                if (patternMatches(childNode->getName(), currentPart.c_str(), usepcre))
                {
                    if (pathParts.size() == 0) //last leave
                    {
                        nodesMatching.emplace_back(childNode->copy());
                    }
                    else
                    {
                        getNodesMatching(childNode, pathParts, nodesMatching, usepcre);
                    }

                }
            }
        }

        delete children;
    }
}

// TODO: docs
MegaNode * MegaCmdExecuter::getBaseNode(string thepath, string &rest, bool *isrelative)
{
    if (isrelative != NULL)
    {
        *isrelative = false;
    }
    MegaNode *baseNode = NULL;
    rest = string();
    if (thepath == "//bin")
    {
        baseNode = api->getRubbishNode();
        rest = "";
    }
    else if (thepath == "//in")
    {
        baseNode = api->getVaultNode();
        rest = "";
    }
    else if (thepath == "/")
    {
        baseNode = api->getRootNode();
        rest = "";
    }
    else if (thepath.find("//bin/") == 0 )
    {
        baseNode = api->getRubbishNode();
        rest = thepath.substr(6);
    }
    else if (thepath.find("//in/") == 0 )
    {
        baseNode = api->getVaultNode();
        rest = thepath.substr(5);
    }
    else if (thepath.find("/") == 0 && !(thepath.find("//from/") == 0 ))
    {
        if ( thepath.find("//f") == 0 && string("//from/").find(thepath.substr(0,thepath.find("*"))) == 0)
        {
            return NULL;
        }
        baseNode = api->getRootNode();
        rest = thepath.substr(1);
    }
    else if ( thepath == "//from/*" )
    {
        return NULL;
    }
    else
    {
        bool from = false;
        if  (thepath.find("//from/") == 0 && thepath != "//from/*" )
        {
            thepath = thepath.substr(7);
            from = true;
        }
        size_t possep = thepath.find('/');
        string base = thepath.substr(0,possep);
        size_t possepcol = base.find(":");
        size_t possepat = base.find("@");

        if ( possepcol != string::npos && possepat != string::npos  && possepat < possepcol && possepcol < (base.size() + 1) )
        {
            string userName = base.substr(0,possepcol);
            string inshareName = base.substr(possepcol + 1);
            unescapeifRequired(inshareName);

            MegaUserList * usersList = api->getContacts();
            MegaUser *u = NULL;
            for (int i = 0; i < usersList->size(); i++)
            {
                if (usersList->get(i)->getEmail() == userName)
                {
                    u = usersList->get(i);
                    break;
                }
            }
            if (u)
            {
                MegaNodeList* inshares = api->getInShares(u);
                for (int i = 0; i < inshares->size(); i++)
                {
                    if (inshares->get(i)->getName() == inshareName)
                    {
                        baseNode = inshares->get(i)->copy();
                        break;
                    }
                }

                delete inshares;
            }
            delete usersList;

            if (possep != string::npos && possep != (thepath.size() - 1) )
            {
                rest = thepath.substr(possep+1);
            }
        }
        else if (!from)
        {
            baseNode = api->getNodeByHandle(cwd);
            rest = thepath;
            if (isrelative != NULL)
            {
                *isrelative = true;
            }
        }
    }

    return baseNode;
}

void MegaCmdExecuter::getPathParts(string path, deque<string> *c)
{
    size_t possep = path.find('/');
    do
    {
        string curName = path.substr(0,possep);
        replaceAll(curName, "\\\\", "\\"); //unescape '\\'
        replaceAll(curName, "\\ ", " "); //unescape '\ '
        c->push_back(curName);
        if (possep != string::npos && possep < (path.size()+1))
        {
            path = path.substr(possep+1);
        }
        else
        {
            break;
        }
        possep = path.find('/');
        if (possep == string::npos ||  !(possep < (path.size()+1)))
        {
            string curName = path.substr(0,possep);
            replaceAll(curName, "\\\\", "\\"); //unescape '\\'
            replaceAll(curName, "\\ ", " "); //unescape '\ '
            c->push_back(curName);

            break;
        }
    } while (path.size());
}

bool MegaCmdExecuter::decryptLinkIfEncrypted(MegaApi *api, std::string &publicLink, std::map<string, string> *cloptions)
{
    if (isEncryptedLink(publicLink))
    {
        string linkPass = getOption(cloptions, "password", "");
        if (!linkPass.size())
        {
            linkPass = askforUserResponse("Enter password: ");
        }

        if (linkPass.size())
        {
            std::unique_ptr<MegaCmdListener>megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
            api->decryptPasswordProtectedLink(publicLink.c_str(), linkPass.c_str(), megaCmdListener.get());
            megaCmdListener->wait();
            if (checkNoErrors(megaCmdListener->getError(), "decrypt password protected link"))
            {
                publicLink = megaCmdListener->getRequest()->getText();
            }
            else
            {
                setCurrentThreadOutCode(MCMD_NOTPERMITTED);
                LOG_err << "Invalid password";
                return false;
            }
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Need a password to decrypt provided link (--password=PASSWORD)";
            return false;
        }
    }
    return true;
}

bool MegaCmdExecuter::checkAndInformPSA(CmdPetition *inf, bool enforce)
{
    bool toret = false;
    m_time_t now = m_time();
    if ( enforce || (now - sandboxCMD->timeOfPSACheck > 86400) )
    {
        MegaUser *u = api->getMyUser();
        if (!u)
        {
            LOG_debug << "No PSA request (not logged into an account)";
            return toret; //Not logged in, no reason to get PSA
        }
        delete u;

        sandboxCMD->timeOfPSACheck = now;

        LOG_verbose << "Getting PSA";
        MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);
        api->getPSA(megaCmdListener);
        megaCmdListener->wait();
        if (megaCmdListener->getError()->getErrorCode() == MegaError::API_ENOENT)
        {
            LOG_verbose << "No new PSA available";
            sandboxCMD->lastPSAnumreceived = max(0,sandboxCMD->lastPSAnumreceived);
        }
        else if(checkNoErrors(megaCmdListener->getError(), "get PSA"))
        {
            int number = static_cast<int>(megaCmdListener->getRequest()->getNumber());
            std::string name = megaCmdListener->getRequest()->getName();
            std::string text = megaCmdListener->getRequest()->getText();

            sandboxCMD->lastPSAnumreceived = number;

            LOG_debug << "Informing PSA #" << number << ": " << name;

            stringstream oss;

            oss << "<" << name << ">";
            oss << text;

            string action = megaCmdListener->getRequest()->getPassword();
            string link = megaCmdListener->getRequest()->getLink();
            if (action.empty())
            {
                action = "read more: ";
            }

            if (link.size())
            {
                oss << endl << action << ": " << link;
            }

            oss << endl << " Execute \"psa --discard\" to stop seeing this message";

            if (inf)
            {
                informStateListener(oss.str(), inf->clientID);
            }
            else
            {
                broadcastMessage(oss.str());
            }
            toret = true;
        }
        delete megaCmdListener;
    }
    return toret;
}

std::string MegaCmdExecuter::formatErrorAndMaySetErrorCode(const MegaError &error)
{
    auto code = error.getErrorCode();
    if (code == MegaError::API_OK)
    {
        return std::string();
    }

    setCurrentThreadOutCode(code);
    if (code == MegaError::API_EBLOCKED)
    {
        auto reason = sandboxCMD->getReasonblocked();
        auto reasonStr = std::string("Account blocked.");
        if (!reason.empty())
        {
            reasonStr.append("Reason: ").append(reason);
        }
        return reasonStr;
    }
    else if (code == MegaError::API_EPAYWALL || (code == MegaError::API_EOVERQUOTA && sandboxCMD->storageStatus == MegaApi::STORAGE_STATE_RED))
    {
        return "Reached storage quota. You can change your account plan to increase your quota limit. See \"help --upgrade\" for further details";
    }

    return error.getErrorString();
}

bool MegaCmdExecuter::checkNoErrors(int errorCode, const string &message)
{
    MegaErrorPrivate e(errorCode);
    return checkNoErrors(&e, message);
}

bool MegaCmdExecuter::checkNoErrors(MegaError *error, const string &message, SyncError syncError)
{
    if (!error)
    {
        LOG_fatal << "No MegaError at request: " << message;
        assert(false);
        return false;
    }
    if (error->getErrorCode() == MegaError::API_OK)
    {
        if (syncError)
        {
            std::unique_ptr<const char[]> megaSyncErrorCode(MegaSync::getMegaSyncErrorCode(syncError));
            LOG_info << "Able to " << message << ", but received syncError: " << megaSyncErrorCode.get();
        }
        return true;
    }

    auto logErrMessage = std::string("Failed to ").append(message).append(": ").append(formatErrorAndMaySetErrorCode(*error));
    if (syncError)
    {
        std::unique_ptr<const char[]> megaSyncErrorCode(MegaSync::getMegaSyncErrorCode(syncError));
        logErrMessage.append(". ").append(megaSyncErrorCode.get());
    }
    LOG_err << logErrMessage;
    return false;
}

bool MegaCmdExecuter::checkNoErrors(::mega::SynchronousRequestListener *listener, const std::string &message, ::mega::SyncError syncError)
{
    assert(listener);
    listener->wait();
    assert(listener->getError());
    return checkNoErrors(listener->getError(), message, syncError);
}

/**
 * @brief MegaCmdExecuter::nodesbypath
 * returns nodes determined by path pattern
 * path naming conventions:
 * path is relative to cwd
 * /path is relative to ROOT
 * //in is in INBOX
 * //bin is in RUBBISH
 * X: is user X's INBOX
 * X:SHARE is share SHARE from user X
 * H:HANDLE is node with handle HANDLE
 * : and / filename components, as well as the \, must be escaped by \.
 * (correct UTF-8 encoding is assumed)
 * @param ptr
 * @param usepcre use PCRE expressions if available
 * @param user
 * @return List of std::unique_ptr<MegaNode>. All are guaranteed to be non-null.
 * 		   The list is empty if the path is malformed or not found.
 */
vector<std::unique_ptr<MegaNode>> MegaCmdExecuter::nodesbypath(const char* ptr, bool usepcre, string* user)
{
    vector<std::unique_ptr<MegaNode>> nodesMatching;

    if (ptr && ptr[0] == 'H' && ptr[1] == ':')
    {
        MegaNode* n = api->getNodeByHandle(api->base64ToHandle(ptr+2));
        if (n)
        {
            nodesMatching.emplace_back(n);
            return nodesMatching;
        }
    }

    string rest;

    std::unique_ptr<MegaNode> baseNode(getBaseNode(ptr, rest));
    if (baseNode)
    {
        if (rest.empty())
        {
            nodesMatching.emplace_back(std::move(baseNode));
            return nodesMatching;
        }

        deque<string> c;
        getPathParts(rest, &c);

        if (c.empty())
        {
            nodesMatching.emplace_back(std::move(baseNode));
        }
        else
        {
            getNodesMatching(baseNode.get(), c, nodesMatching, usepcre);
        }
    }
    else if (!strncmp(ptr, "//from/", max(3, min(static_cast<int>(strlen(ptr)-1), 7)))) //pattern trying to match inshares
    {
        unique_ptr<MegaShareList> inShares(api->getInSharesList());
        if (inShares)
        {
            string matching = ptr;
            unescapeifRequired(matching);

            for (int i = 0; i < inShares->size(); i++)
            {
                std::unique_ptr<MegaNode> n(api->getNodeByHandle(inShares->get(i)->getNodeHandle()));
                if (!n) continue;

                string tomatch = string("//from/") + inShares->get(i)->getUser() + ":" + n->getName();
                if (patternMatches(tomatch.c_str(), matching.c_str(), false))
                {
                    nodesMatching.emplace_back(std::move(n));
                }
            }
        }
    }

    return nodesMatching;
}

void MegaCmdExecuter::dumpNode(MegaNode* n, const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, int extended_info, bool showversions, int depth, const char* title)
{
    if (!title && !( title = n->getName()))
    {
        title = "CRYPTO_ERROR";
    }

    if (depth)
    {
        for (int i = depth - 1; i--; )
        {
            OUTSTREAM << "\t";
        }
    }

    OUTSTREAM << title;

    if (getFlag(clflags, "show-handles"))
    {
        OUTSTREAM << " <H:" << handleToBase64(n->getHandle()) << ">";
    }

    if (extended_info)
    {
        OUTSTREAM << " (";
        switch (n->getType())
        {
            case MegaNode::TYPE_FILE:
                OUTSTREAM << sizeToText(n->getSize(), false);
                if (INVALID_HANDLE != n->getPublicHandle())
//            if (n->isExported())
                {
                    OUTSTREAM << ", shared as exported";
                    if (n->getExpirationTime())
                    {
                        OUTSTREAM << " temporal";
                    }
                    else
                    {
                        OUTSTREAM << " permanent";
                    }
                    OUTSTREAM << " file link";
                    if (extended_info > 1)
                    {
                        char * publicLink = n->getPublicLink();
                        OUTSTREAM << ": " << publicLink;
                        if (n->getExpirationTime())
                        {
                            if (n->isExpired())
                            {
                                OUTSTREAM << " expired at ";
                            }
                            else
                            {
                                OUTSTREAM << " expires at ";
                            }

                            OUTSTREAM << getReadableTime(n->getExpirationTime(), timeFormat);
                        }

                        if (n->getWritableLinkAuthKey())
                        {
                            string authKey(n->getWritableLinkAuthKey());
                            if (authKey.size())
                            {
                                OUTSTREAM << " AuthKey="<< authKey;
                            }
                        }

                        delete []publicLink;
                    }
                }
                break;

            case MegaNode::TYPE_FOLDER:
            {
                OUTSTREAM << "folder";
                MegaShareList* outShares = api->getOutShares(n);
                if (outShares)
                {
                    for (int i = 0; i < outShares->size(); i++)
                    {
                        auto outShare = outShares->get(i);
                        if (outShare->getNodeHandle() == n->getHandle() && !outShare->isPending()/*shall be listed via getPendingOutShares*/)
                        {
                            OUTSTREAM << ", shared with " << outShare->getUser() << ", access "
                                      << getAccessLevelStr(outShare->getAccess())
                                      << (outShare->isVerified() ? "" : "[UNVERIFIED]");
                        }
                    }

                    MegaShareList* pendingoutShares = api->getPendingOutShares(n);
                    if (pendingoutShares)
                    {
                        for (int i = 0; i < pendingoutShares->size(); i++)
                        {
                            auto pendingoutShare = pendingoutShares->get(i);
                            if (pendingoutShare->getNodeHandle() == n->getHandle())
                            {
                                OUTSTREAM << ", shared (still pending)";
                                if (pendingoutShare->getUser())
                                {
                                    OUTSTREAM << " with " << pendingoutShare->getUser();
                                }
                                OUTSTREAM << " access " << getAccessLevelStr(pendingoutShare->getAccess())
                                              << (pendingoutShare->isVerified() ? "" : "[UNVERIFIED]");
                            }
                        }

                        delete pendingoutShares;
                    }

                    if (UNDEF != n->getPublicHandle())
                    {
                        OUTSTREAM << ", shared as exported";
                        if (n->getExpirationTime())
                        {
                            OUTSTREAM << " temporal";
                        }
                        else
                        {
                            OUTSTREAM << " permanent";
                        }
                        OUTSTREAM << " folder link";
                        if (extended_info > 1)
                        {
                            std::unique_ptr<char[]> publicLink(n->getPublicLink());
                            OUTSTREAM << ": " << publicLink.get();

                            if (n->getWritableLinkAuthKey())
                            {
                                constexpr const char *prefix = "https://mega.nz/folder/";
                                string authKey(n->getWritableLinkAuthKey());
                                string nodeLink(publicLink.get());
                                if (authKey.size() && nodeLink.rfind(prefix, 0) == 0)
                                {
                                    string authToken = nodeLink.substr(strlen(prefix)).append(":").append(authKey);
                                    OUTSTREAM << " AuthToken="<< authToken;
                                }
                            }
                        }
                    }
                    delete outShares;
                }

                if (n->isInShare())
                {
                    OUTSTREAM << ", inbound " << getAccessLevelStr(api->getAccess(n)) << " share";
                }
                break;
            }
            case MegaNode::TYPE_ROOT:
            {
                OUTSTREAM << "root node";
                break;
            }
            case MegaNode::TYPE_INCOMING:
            {
                OUTSTREAM << "inbox";
                break;
            }
            case MegaNode::TYPE_RUBBISH:
            {
                OUTSTREAM << "rubbish";
                break;
            }
            default:
                OUTSTREAM << "unsupported type: " <<  n->getType() <<" , please upgrade";
        }
        OUTSTREAM << ")" << ( n->isRemoved() ? " (DELETED)" : "" );
    }

    OUTSTREAM << endl;

    if (showversions && n->getType() == MegaNode::TYPE_FILE)
    {
        std::unique_ptr<MegaNodeList> versionNodes(api->getVersions(n));
        if (versionNodes)
        {
            for (int i = 0; i < versionNodes->size(); i++)
            {
                MegaNode *versionNode = versionNodes->get(i);

                if (versionNode->getHandle() != n->getHandle())
                {
                    string fullname(n->getName()?n->getName():"NO_NAME");
                    fullname += "#";
                    fullname += SSTR(versionNode->getModificationTime());
                    OUTSTREAM << "  " << fullname;
                    if (versionNode->getName() && !strcmp(versionNode->getName(),n->getName()) )
                    {
                        OUTSTREAM << "[" << (versionNode->getName()?versionNode->getName():"NO_NAME") << "]";
                    }
                    OUTSTREAM << " (" << getReadableTime(versionNode->getModificationTime(), timeFormat) << ")";
                    if (extended_info)
                    {
                        OUTSTREAM << " (" << sizeToText(versionNode->getSize(), false) << ")";
                    }


                    if (getFlag(clflags, "show-handles"))
                    {
                        OUTSTREAM << " <H:" << handleToBase64(versionNode->getHandle()) << ">";
                    }

                    OUTSTREAM << endl;
                }
            }
        }
    }
}

// 12 is the length of "999000000000" i.e 999 GiB.
static constexpr size_t MAX_SIZE_LEN = 12;
static unsigned int DUMPNODE_SIZE_WIDTH = static_cast<unsigned>((MAX_SIZE_LEN > strlen("SIZE  ")) ? MAX_SIZE_LEN : strlen("SIZE  "));

void MegaCmdExecuter::dumpNodeSummaryHeader(const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions)
{
    int datelength = int(getReadableTime(m_time(), timeFormat).size());

    OUTSTREAM << "FLAGS";
    OUTSTREAM << " ";
    OUTSTREAM << getFixLengthString("VERS", 4);
    OUTSTREAM << " ";
    OUTSTREAM << getFixLengthString("SIZE  ", DUMPNODE_SIZE_WIDTH - 1 /*-1 to compensate FLAGS header*/, ' ', true);
    OUTSTREAM << " ";
    OUTSTREAM << getFixLengthString("DATE      ", datelength+1, ' ', true);
    if (getFlag(clflags, "show-handles"))
    {
        OUTSTREAM << " ";
        OUTSTREAM << "   HANDLE";
    }
    OUTSTREAM << " ";
    OUTSTREAM << "NAME";
    OUTSTREAM << endl;
}

void MegaCmdExecuter::dumpNodeSummary(MegaNode *n, const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, bool humanreadable, const char *title)
{
    if (!title && !( title = n->getName()))
    {
        title = "CRYPTO_ERROR";
    }

    switch (n->getType())
    {
    case MegaNode::TYPE_FILE:
        OUTSTREAM << "-";
        break;
    case MegaNode::TYPE_FOLDER:
        OUTSTREAM << "d";
        break;
    case MegaNode::TYPE_ROOT:
        OUTSTREAM << "r";
        break;
    case MegaNode::TYPE_INCOMING:
        OUTSTREAM << "i";
        break;
    case MegaNode::TYPE_RUBBISH:
        OUTSTREAM << "b";
        break;
    default:
        OUTSTREAM << "x";
        break;
    }

    if (UNDEF != n->getPublicHandle())
    {
        OUTSTREAM << "e";
        if (n->getExpirationTime())
        {
            OUTSTREAM << "t";
        }
        else
        {
            OUTSTREAM << "p";
        }
    }
    else
    {
        OUTSTREAM << "--";
    }

    if (n->isShared())
    {
        OUTSTREAM << "s";
    }
    else if (n->isInShare())
    {
        OUTSTREAM << "i";
    }
    else
    {
        OUTSTREAM << "-";
    }

    OUTSTREAM << " ";

    if (n->isFile())
    {
        MegaNodeList *versionNodes = api->getVersions(n);
        int nversions = versionNodes ? versionNodes->size() : 0;
        if (nversions > 999)
        {
            OUTSTREAM << getFixLengthString(">999", 4, ' ', true);
        }
        else
        {
            OUTSTREAM << getFixLengthString(SSTR(nversions), 4, ' ', true);
        }

        delete versionNodes;
    }
    else
    {
        OUTSTREAM << getFixLengthString("-", 4, ' ', true);
    }

    OUTSTREAM << " ";

    if (n->isFile())
    {
        if (humanreadable)
        {
            OUTSTREAM << getFixLengthString(sizeToText(n->getSize()), DUMPNODE_SIZE_WIDTH, ' ', true);
        }
        else
        {
            OUTSTREAM << getFixLengthString(SSTR(n->getSize()), DUMPNODE_SIZE_WIDTH, ' ', true);
        }
    }
    else
    {
        OUTSTREAM << getFixLengthString(
            "-", (unsigned int)std::max(MAX_SIZE_LEN, strlen("SIZE  ")), ' ', true);
    }

    if (n->isFile() && !getFlag(clflags, "show-creation-time"))
    {
        OUTSTREAM << " " << getReadableTime(n->getModificationTime(), timeFormat);
    }
    else
    {
        OUTSTREAM << " " << getReadableTime(n->getCreationTime(), timeFormat);
    }

    if (getFlag(clflags, "show-handles"))
    {
        OUTSTREAM << " H:" << handleToBase64(n->getHandle());
    }

    OUTSTREAM << " " << title;
    OUTSTREAM << endl;
}

void MegaCmdExecuter::createOrModifyBackup(string local, string remote, string speriod, int numBackups)
{
    LocalPath locallocal = LocalPath::fromAbsolutePath(local);
    std::unique_ptr<FileAccess> fa = mFsAccessCMD->newfileaccess();
    if (!fa->isfolder(locallocal))
    {
        setCurrentThreadOutCode(MCMD_NOTFOUND);
        LOG_err << "Local path must be an existing folder: " << local;
        return;
    }


    int64_t period = -1;

    if (!speriod.size())
    {
        MegaScheduledCopy *backup = api->getScheduledCopyByPath(local.c_str());
        if (!backup)
        {
            backup = api->getScheduledCopyByTag(toInteger(local, -1));
        }
        if (backup)
        {
            speriod = backup->getPeriodString();
            if (!speriod.size())
            {
                period = backup->getPeriod();
            }
            delete backup;
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("backup");
            return;
        }
    }
    if (speriod.find(" ") == string::npos && period == -1)
    {
        period = 10 * getTimeStampAfter(0,speriod);
        speriod = "";
    }

    if (numBackups == -1)
    {
        MegaScheduledCopy *backup = api->getScheduledCopyByPath(local.c_str());
        if (!backup)
        {
            backup = api->getScheduledCopyByTag(toInteger(local, -1));
        }
        if (backup)
        {
            numBackups = backup->getMaxBackups();
            delete backup;
        }
    }

    if (numBackups == -1)
    {
        setCurrentThreadOutCode(MCMD_EARGS);
        LOG_err << "      " << getUsageStr("backup");
        return;
    }

    std::unique_ptr<MegaNode> n;
    if (remote.size())
    {
        n = nodebypath(remote.c_str());
    }
    else
    {
        std::unique_ptr<MegaScheduledCopy> backup(api->getScheduledCopyByPath(local.c_str()));
        if (!backup)
        {
            backup.reset(api->getScheduledCopyByTag(toInteger(local, -1)));
        }

        if (backup)
        {
            n.reset(api->getNodeByHandle(backup->getMegaHandle()));
        }
    }

    if (n)
    {
        if (n->getType() != MegaNode::TYPE_FOLDER)
        {
            setCurrentThreadOutCode(MCMD_INVALIDTYPE);
            LOG_err << remote << " must be a valid folder";
        }
        else
        {
            if (establishBackup(local, n.get(), period, speriod, numBackups))
            {
                {
                    std::lock_guard g(mtxBackupsMap);
                    ConfigurationManager::saveBackups(&ConfigurationManager::configuredBackups);
                }

                OUTSTREAM << "Backup established: " << local << " into " << remote << " period="
                          << ((period != -1)?getReadablePeriod(period/10):"\""+speriod+"\"")
                          << " Number-of-Backups=" << numBackups << endl;
            }
        }
    }
    else
    {
        setCurrentThreadOutCode(MCMD_NOTFOUND);
        LOG_err << remote << " not found";
    }
}

void MegaCmdExecuter::printTreeSuffix(int depth, vector<bool> &lastleaf)
{
#ifdef _WIN32
    const wchar_t *c0 = L" ";
    const wchar_t *c1 = L"\u2502";
    const wchar_t *c2 = L"\u2514";
    const wchar_t *c3 = L"\u251c";
    const wchar_t *c4 = L"\u2500\u2500";
#else
    const char *c0 = " ";
    const char *c1 = "\u2502";
    const char *c2 = "\u2514";
    const char *c3 = "\u251c";
    const char *c4 = "\u2500\u2500";
#endif
    for (int i = 0; i < depth-1; i++)
    {
        OUTSTREAM << (lastleaf.at(i)?c0:c1) << "   ";
    }
    if (lastleaf.size())
    {
        OUTSTREAM << (lastleaf.back()?c2:c3) << c4 << " ";
    }
}

void MegaCmdExecuter::dumptree(MegaNode* n, bool treelike, vector<bool> &lastleaf, const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, int recurse, int extended_info, bool showversions, int depth, string pathRelativeTo)
{
    if (depth || ( n->getType() == MegaNode::TYPE_FILE ))
    {
        if (treelike) printTreeSuffix(depth, lastleaf);

        if (pathRelativeTo != "NULL")
        {
            if (!n->getName())
            {
                dumpNode(n, timeFormat, clflags, cloptions, extended_info, showversions, treelike?0:depth, "CRYPTO_ERROR");
            }
            else
            {
                char * nodepath = api->getNodePath(n);

                char *pathToShow = NULL;
                if (pathRelativeTo != "")
                {
                    pathToShow = strstr(nodepath, pathRelativeTo.c_str());
                }

                if (pathToShow == nodepath)     //found at beginning
                {
                    pathToShow += pathRelativeTo.size();
                    if (( *pathToShow == '/' ) && ( pathRelativeTo != "/" ))
                    {
                        pathToShow++;
                    }
                }
                else
                {
                    pathToShow = nodepath;
                }

                dumpNode(n, timeFormat, clflags, cloptions, extended_info, showversions, treelike?0:depth, pathToShow);

                delete []nodepath;
            }
        }
        else
        {
                dumpNode(n, timeFormat, clflags, cloptions, extended_info, showversions, treelike?0:depth);
        }

        if (!recurse && depth)
        {
            return;
        }
    }

    if (n->getType() != MegaNode::TYPE_FILE)
    {
        MegaNodeList* children = api->getChildren(n);
        if (children)
        {
            for (int i = 0; i < children->size(); i++)
            {
                vector<bool> lfs = lastleaf;
                lfs.push_back(i==(children->size()-1));
                dumptree(children->get(i), treelike, lfs, timeFormat, clflags, cloptions, recurse, extended_info, showversions, depth + 1);
            }

            delete children;
        }
    }
}

void MegaCmdExecuter::dumpTreeSummary(MegaNode *n, const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, int recurse, bool show_versions, int depth, bool humanreadable, string pathRelativeTo)
{
    char * nodepath = api->getNodePath(n);

    string scryptoerror = "CRYPTO_ERROR";

    char *pathToShow = NULL;
    if (pathRelativeTo != "")
    {
        pathToShow = strstr(nodepath, pathRelativeTo.c_str());
    }

    if (pathToShow == nodepath) //found at beginning
    {
        pathToShow += pathRelativeTo.size();
        if (( *pathToShow == '/' ) && ( pathRelativeTo != "/" ))
        {
            pathToShow++;
        }
    }
    else
    {
        pathToShow = nodepath;
    }

    if (!pathToShow && !( pathToShow = (char *)n->getName()))
    {
        pathToShow = (char *)scryptoerror.c_str();
    }

    if (n->getType() != MegaNode::TYPE_FILE)
    {
        MegaNodeList* children = api->getChildren(n);
        if (children)
        {
            if (depth)
            {
                OUTSTREAM << endl;
            }

            if (recurse)
            {
                OUTSTREAM << pathToShow << ":" << endl;
            }

            for (int i = 0; i < children->size(); i++)
            {
                dumpNodeSummary(children->get(i), timeFormat, clflags, cloptions, humanreadable);
            }

            if (show_versions)
            {
                for (int i = 0; i < children->size(); i++)
                {
                    MegaNode *c = children->get(i);

                    MegaNodeList *vers = api->getVersions(c);
                    if (vers &&  vers->size() > 1)
                    {
                        OUTSTREAM << endl << "Versions of " << pathToShow << "/" << c->getName() << ":" << endl;

                        for (int i = 0; i < vers->size(); i++)
                        {
                            dumpNodeSummary(vers->get(i), timeFormat, clflags, cloptions, humanreadable);
                        }
                    }
                    delete vers;
                }
            }

            if (recurse)
            {
                for (int i = 0; i < children->size(); i++)
                {
                    MegaNode *c = children->get(i);
                    dumpTreeSummary(c, timeFormat, clflags, cloptions, recurse, show_versions, depth + 1, humanreadable);
                }
            }
            delete children;
        }
    }
    else // file
    {
        if (!depth)
        {

            dumpNodeSummary(n, timeFormat, clflags, cloptions, humanreadable);

            if (show_versions)
            {
                MegaNodeList *vers = api->getVersions(n);
                if (vers &&  vers->size() > 1)
                {
                    OUTSTREAM << endl << "Versions of " << pathToShow << ":" << endl;

                    for (int i = 0; i < vers->size(); i++)
                    {
                        string nametoshow = n->getName()+string("#")+SSTR(vers->get(i)->getModificationTime());
                        dumpNodeSummary(vers->get(i), timeFormat, clflags, cloptions, humanreadable, nametoshow.c_str());
                    }
                }
                delete vers;
            }
        }

    }
    delete []nodepath;
}


/**
 * @brief Tests if a path can be created
 * @param path
 * @return
 */
bool MegaCmdExecuter::TestCanWriteOnContainingFolder(string path)
{
#ifdef _WIN32
    replaceAll(path, "/", "\\");
#endif

    auto containingFolder = LocalPath::fromAbsolutePath(path);
    if (!containingFolder.isRootPath())
    {
        containingFolder = containingFolder.parentPath();
    }

    std::unique_ptr<FileAccess> fa = mFsAccessCMD->newfileaccess();
    if (!fa->isfolder(containingFolder))
    {
        setCurrentThreadOutCode(MCMD_INVALIDTYPE);
        LOG_err << containingFolder.toPath(false) << " is not a valid Download Folder";
        return false;
    }

    if (!canWrite(containingFolder.platformEncoded()))
    {
        setCurrentThreadOutCode(MCMD_NOTPERMITTED);
        LOG_err << "Write not allowed in " << containingFolder.toPath(false);
        return false;
    }

    return true;
}

std::unique_ptr<MegaContactRequest> MegaCmdExecuter::getPcrByContact(string contactEmail)
{
    unique_ptr<MegaContactRequestList> icrl(api->getIncomingContactRequests());
    if (icrl)
    {
        for (int i = 0; i < icrl->size(); i++)
        {
            if (icrl->get(i)->getSourceEmail() == contactEmail)
            {
                return std::unique_ptr<MegaContactRequest>(icrl->get(i)->copy());
            }
        }
    }
    return NULL;
}

string MegaCmdExecuter::getDisplayPath(string givenPath, MegaNode* n_param)
{
    char * pathToNode = api->getNodePath(n_param);
    if (!pathToNode)
    {
        LOG_err << " GetNodePath failed for: " << givenPath;
        return givenPath;
    }

    char * pathToShow = pathToNode;

    string pathRelativeTo = "NULL";
    string cwpath = getCurrentPath();
    string toret="";


    if (givenPath.find('/') == 0 )
    {
        pathRelativeTo = "";
    }
    else if(givenPath.find("../") == 0 || givenPath.find("./") == 0 )
    {
        pathRelativeTo = "";
        MegaNode *n = api->getNodeByHandle(cwd);
        while(true)
        {
            if(givenPath.find("./") == 0)
            {
                givenPath=givenPath.substr(2);
                toret+="./";
                if (n)
                {
                    char *npath = api->getNodePath(n);
                    pathRelativeTo = string(npath);
                    delete []npath;
                }
                return toret;

            }
            else if(givenPath.find("../") == 0)
            {
                givenPath=givenPath.substr(3);
                toret+="../";
                MegaNode *aux = n;
                if (n)
                {
                    n=api->getNodeByHandle(n->getParentHandle());
                }
                delete aux;
                if (n)
                {
                    char *npath = api->getNodePath(n);
                    pathRelativeTo = string(npath);
                    delete []npath;
                }
            }
            else
            {
                break;
            }
        }
        delete n;
    }
    else
    {
        if (cwpath == "/") //TODO: //bin /X:share ...
        {
            pathRelativeTo = cwpath;
        }
        else
        {
            pathRelativeTo = cwpath + "/";
        }
    }

    if (( "" == givenPath ) && !strcmp(pathToNode, cwpath.c_str()))
    {
        assert(strlen(pathToNode)>0);
        pathToNode[0] = '.';
        pathToNode[1] = '\0';
    }

    if (pathRelativeTo != "")
    {
        pathToShow = strstr(pathToNode, pathRelativeTo.c_str());
    }

    if (pathToShow == pathToNode)     //found at beginning
    {
        if (strcmp(pathToNode, "/"))
        {
            pathToShow += pathRelativeTo.size();
        }
    }
    else
    {
        pathToShow = pathToNode;
    }

    toret+=pathToShow;
    delete []pathToNode;
    return toret;
}

int MegaCmdExecuter::dumpListOfExported(MegaNode* n_param, const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, string givenPath)
{
    int toret = 0;
    vector<MegaNode *> listOfExported;
    processTree(n_param, includeIfIsExported, (void*)&listOfExported);
    for (std::vector< MegaNode * >::iterator it = listOfExported.begin(); it != listOfExported.end(); ++it)
    {
        MegaNode * n = *it;
        if (n)
        {
            string pathToShow = getDisplayPath(givenPath, n);
            dumpNode(n, timeFormat, clflags, cloptions, 2, 1, false, pathToShow.c_str());

            delete n;
        }
    }
    toret = int(listOfExported.size());
    listOfExported.clear();
    return toret;
}

void printOutShareInfo(const char *pathOrName, const char *email, int accessLevel, bool pending, bool verified)
{
    OUTSTREAM << pathOrName;
    OUTSTREAM << ",";
    if (email)
    {
        OUTSTREAM << " shared";
    }
    if (pending)
    {
        OUTSTREAM << " (still pending)";
    }
    if (email)
    {
        OUTSTREAM << " with " << email;
    }
    OUTSTREAM << " (" << getAccessLevelStr(accessLevel) << ")";
    if (!verified && !pending /*Do not indicate lack of verification for pending out shares*/)
    {
         OUTSTREAM << "[UNVERIFIED]";
    }
    OUTSTREAM << endl;
}

/**
 * @brief listnodeshares For a node, it prints all the shares it has
 * @param n
 * @param name
 */
void MegaCmdExecuter::listnodeshares(MegaNode* n, string name, bool listPending = false, bool onlyPending = false)
{
    assert(listPending || !onlyPending);

    auto printOutShares = [n, CONST_CAPTURE(name)](MegaShareList * outShares, bool skipPending = false)
    {
        if (outShares)
        {
            for (int i = 0; i < outShares->size(); i++)
            {
                auto outShare = outShares->get(i);
                if (outShare && (!skipPending || !outShare->isPending()))
                {
                    printOutShareInfo(name.empty() ? n->getName() : name.c_str(), outShare->getUser(),
                                      outShare->getAccess(), outShare->isPending(), outShare->isVerified());
                }
            }
        }
    };

    if (!onlyPending)
    {
        std::unique_ptr<MegaShareList> allOutShares (api->getOutShares(n));
        printOutShares(allOutShares.get(), true /*to not list them twice*/);
    }

    if (listPending)
    {
        std::unique_ptr<MegaShareList> pendingOutShares (api->getPendingOutShares(n));
        printOutShares(pendingOutShares.get());
    }
}

void MegaCmdExecuter::dumpListOfShared(MegaNode* n_param, string givenPath)
{
    vector<MegaNode *> listOfShared;
    processTree(n_param, includeIfIsShared, (void*)&listOfShared);
    if (!listOfShared.size())
    {
        setCurrentThreadOutCode(MCMD_NOTFOUND);
        LOG_err << "No shared found for given path: " << givenPath;
    }
    for (std::vector< MegaNode * >::iterator it = listOfShared.begin(); it != listOfShared.end(); ++it)
    {
        MegaNode * n = *it;
        if (n)
        {
            string pathToShow = getDisplayPath(givenPath, n);
            listnodeshares(n, pathToShow);

            delete n;
        }
    }

    listOfShared.clear();
}

//includes pending and normal shares
void MegaCmdExecuter::dumpListOfAllShared(MegaNode* n_param, string givenPath)
{
    vector<MegaNode *> listOfShared;
    processTree(n_param, includeIfIsSharedOrPendingOutShare, (void*)&listOfShared);
    for (std::vector< MegaNode * >::iterator it = listOfShared.begin(); it != listOfShared.end(); ++it)
    {
        MegaNode * n = *it;
        if (n)
        {
            string pathToShow = getDisplayPath(givenPath, n);
            listnodeshares(n, pathToShow, true);
            delete n;
        }
    }

    listOfShared.clear();
}

void MegaCmdExecuter::dumpListOfPendingShares(MegaNode* n_param, string givenPath)
{
    vector<MegaNode *> listOfShared;
    processTree(n_param, includeIfIsPendingOutShare, (void*)&listOfShared);

    for (std::vector< MegaNode * >::iterator it = listOfShared.begin(); it != listOfShared.end(); ++it)
    {
        MegaNode * n = *it;
        if (n)
        {
            string pathToShow = getDisplayPath(givenPath, n);
            listnodeshares(n, pathToShow, true, true);
            delete n;
        }
    }

    listOfShared.clear();
}


void MegaCmdExecuter::loginWithPassword(const char *password)
{
    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
    sandboxCMD->resetSandBox();
    api->login(login.c_str(), password, megaCmdListener);
    actUponLogin(megaCmdListener);
    delete megaCmdListener;
}

void MegaCmdExecuter::changePassword(const char *newpassword, string pin2fa)
{
    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
    api->changePassword(NULL, newpassword, megaCmdListener);
    megaCmdListener->wait();

    if (megaCmdListener->getError()->getErrorCode() == MegaError::API_EMFAREQUIRED)
    {
        MegaCmdListener *megaCmdListener2 = new MegaCmdListener(NULL);
        if (!pin2fa.size())
        {
            pin2fa = askforUserResponse("Enter the code generated by your authentication app: ");
        }
        LOG_verbose << " Using confirmation pin: " << pin2fa;
        api->multiFactorAuthChangePassword(NULL, newpassword, pin2fa.c_str(), megaCmdListener2);
        megaCmdListener2->wait();
        if (megaCmdListener2->getError()->getErrorCode() == MegaError::API_EFAILED)
        {
            setCurrentThreadOutCode(megaCmdListener2->getError()->getErrorCode());
            LOG_err << "Password unchanged: invalid authentication code";
        }
        else if (checkNoErrors(megaCmdListener2->getError(), "change password with auth code"))
        {
            OUTSTREAM << "Password changed successfully" << endl;
        }

    }
    else if (!checkNoErrors(megaCmdListener->getError(), "change password"))
    {
        LOG_err << "Please, ensure you enter the old password correctly";
    }
    else
    {
        OUTSTREAM << "Password changed successfully" << endl;
    }
    delete megaCmdListener;
}

void str_localtime(char s[32], ::mega::m_time_t t)
{
    struct tm tms;
    strftime(s, 32, "%c", m_localtime(t, &tms));
}

void MegaCmdExecuter::actUponGetExtendedAccountDetails(std::unique_ptr<::mega::MegaAccountDetails> storageDetails,
                                                       std::unique_ptr<::mega::MegaAccountDetails> extAccountDetails)
{
    char timebuf[32], timebuf2[32];

    LOG_verbose << "actUponGetExtendedAccountDetails ok";

    if (storageDetails)
    {
        OUTSTREAM << "    Available storage: " << getFixLengthString(sizeToText(storageDetails->getStorageMax()), 9, ' ', true) << "ytes" << endl;
        std::unique_ptr<MegaNode> n(api->getRootNode());
        if (n)
        {
            OUTSTREAM << "        In ROOT:      " << getFixLengthString(sizeToText(storageDetails->getStorageUsed(n->getHandle())), 9, ' ', true) << "ytes in "
                      << getFixLengthString(SSTR(storageDetails->getNumFiles(n->getHandle())), 5, ' ', true) << " file(s) and "
                      << getFixLengthString(SSTR(storageDetails->getNumFolders(n->getHandle())), 5, ' ', true) << " folder(s)" << endl;
        }

        n = std::unique_ptr<MegaNode>(api->getVaultNode());
        if (n)
        {
            OUTSTREAM << "        In INBOX:     " << getFixLengthString(sizeToText(storageDetails->getStorageUsed(n->getHandle())), 9, ' ', true) << "ytes in "
                      << getFixLengthString(SSTR(storageDetails->getNumFiles(n->getHandle())), 5, ' ', true) << " file(s) and "
                      << getFixLengthString(SSTR(storageDetails->getNumFolders(n->getHandle())), 5, ' ', true) << " folder(s)" << endl;
        }

        n = std::unique_ptr<MegaNode>(api->getRubbishNode());
        if (n)
        {
            OUTSTREAM << "        In RUBBISH:   " << getFixLengthString(sizeToText(storageDetails->getStorageUsed(n->getHandle())), 9, ' ', true) << "ytes in "
                      << getFixLengthString(SSTR(storageDetails->getNumFiles(n->getHandle())), 5, ' ', true) << " file(s) and "
                      << getFixLengthString(SSTR(storageDetails->getNumFolders(n->getHandle())), 5, ' ', true) << " folder(s)" << endl;
        }

        long long usedinVersions = storageDetails->getVersionStorageUsed();

        OUTSTREAM << "        Total size taken up by file versions: " << getFixLengthString(sizeToText(usedinVersions), 12, ' ', true) << "ytes" << endl;

        std::unique_ptr<MegaNodeList> inshares(api->getInShares());
        if (inshares)
        {
            for (int i = 0; i < inshares->size(); i++)
            {
                auto node = inshares->get(i);
                OUTSTREAM << "        In INSHARE " << node->getName() << ": " << getFixLengthString(sizeToText(storageDetails->getStorageUsed(node->getHandle())), 9, ' ', true)
                          << "ytes in " << getFixLengthString(SSTR(storageDetails->getNumFiles(node->getHandle())), 5, ' ', true) << " file(s) and "
                          << getFixLengthString(SSTR(storageDetails->getNumFolders(node->getHandle())), 5, ' ', true) << " folder(s)" << endl;
            }
        }
        OUTSTREAM << "    Pro level: " << storageDetails->getProLevel() << endl;
        if (storageDetails->getProLevel())
        {
            if (storageDetails->getProExpiration())
            {
                str_localtime(timebuf, storageDetails->getProExpiration());
                OUTSTREAM << "        "
                          << "Pro expiration date: " << timebuf << endl;
            }
        }
    }
    if (extAccountDetails)
    {
        std::unique_ptr<char[]> subscriptionMethod(extAccountDetails->getSubscriptionMethod());
        OUTSTREAM << "    Subscription type: " << subscriptionMethod.get() << endl;
        OUTSTREAM << "    Account balance:" << endl;
        for (int i = 0; i < extAccountDetails->getNumBalances(); i++)
        {
            std::unique_ptr<MegaAccountBalance> balance(extAccountDetails->getBalance(i));
            std::unique_ptr<char[]> currency(balance->getCurrency());
            char sbalance[50];

            sprintf(sbalance, "    Balance: %.3s %.02f", currency.get(), balance->getAmount());
            OUTSTREAM << "    "
                      << "Balance: " << sbalance << endl;
        }

        if (extAccountDetails->getNumPurchases())
        {
            OUTSTREAM << "Purchase history:" << endl;
            for (int i = 0; i < extAccountDetails->getNumPurchases(); i++)
            {
                std::unique_ptr<MegaAccountPurchase> purchase(extAccountDetails->getPurchase(i));
                std::unique_ptr<char[]> currency(purchase->getCurrency());
                std::unique_ptr<char[]> handle(purchase->getHandle());
                char spurchase[150];

                str_localtime(timebuf, purchase->getTimestamp());
                sprintf(spurchase, "ID: %.11s Time: %s Amount: %.3s %.02f Payment method: %d\n", handle.get(), timebuf, currency.get(),
                        purchase->getAmount(), purchase->getMethod());
                OUTSTREAM << "    " << spurchase << endl;
            }
        }

        if (extAccountDetails->getNumTransactions())
        {
            OUTSTREAM << "Transaction history:" << endl;
            for (int i = 0; i < extAccountDetails->getNumTransactions(); i++)
            {
                std::unique_ptr<MegaAccountTransaction> transaction(extAccountDetails->getTransaction(i));
                std::unique_ptr<char[]> currency(transaction->getCurrency());
                char stransaction[100];

                str_localtime(timebuf, transaction->getTimestamp());
                sprintf(stransaction, "ID: %.11s Time: %s Amount: %.3s %.02f\n", transaction->getHandle(), timebuf, currency.get(), transaction->getAmount());
                OUTSTREAM << "    " << stransaction << endl;
            }
        }

        int alive_sessions = 0;
        OUTSTREAM << "Current Active Sessions:" << endl;
        char sdetails[500];
        for (int i = 0; i < extAccountDetails->getNumSessions(); i++)
        {
            std::unique_ptr<MegaAccountSession> session(extAccountDetails->getSession(i));
            if (session->isAlive())
            {
                str_localtime(timebuf, session->getCreationTimestamp());
                str_localtime(timebuf2, session->getMostRecentUsage());

                std::unique_ptr<char[]> sid(api->userHandleToBase64(session->getHandle()));

                const char *current_session = "";
                if (session->isCurrent())
                {
                    current_session = "    * Current Session\n";
                }

                std::unique_ptr<char[]> userAgent(session->getUserAgent());
                std::unique_ptr<char[]> country(session->getCountry());
                std::unique_ptr<char[]> ip(session->getIP());

                sprintf(sdetails, "%s    Session ID: %s\n    Session start: %s\n    Most recent activity: %s\n    IP: %s\n    Country: %.2s\n    User-Agent: %s\n    -----\n",
                        current_session, sid.get(), timebuf, timebuf2, ip.get(), country.get(), userAgent.get());
                OUTSTREAM << sdetails;
                alive_sessions++;
            }
        }

        if (alive_sessions)
        {
            OUTSTREAM << alive_sessions << " active sessions opened" << endl;
        }
    }
}

void MegaCmdExecuter::verifySharedFolders(MegaApi *api)
{
    auto getInstructionsNeedsVerification = [](const char *intro)
    {
        std::stringstream ss;
        ss << intro << ".\n";
        ss << "You need to verify your contacts. \nUse ";
        ss << getCommandPrefixBasedOnMode();
        ss << "users --help-verify to get instructions";
        return ss.str();
    };

    auto getInstructionsNeedsLogin = [](const char *intro, const std::set<std::string> &emails)
    {
        std::stringstream ss;
        ss << intro << ".";
        for (auto &email : emails)
        {
            ss << "\nYour contact " << email << " may need to resume using his MEGA application.";
        }
        return ss.str();
    };

    {
        std::unique_ptr<MegaShareList> shares(api->getUnverifiedInShares());
        if (shares && shares->size())
        {
            broadcastMessage(getInstructionsNeedsVerification("Some not verified contact is sharing a folder with you"), true);
            return;
        }

        std::set<std::string> usersWithNonDecryptable;

        std::unique_ptr<MegaShareList> inSharesByAllUsers (api->getInSharesList());// this one should not return unverified ones
        if (inSharesByAllUsers)
        {
            for (int i = 0, total = inSharesByAllUsers->size(); i < total; i++)
            {
                auto share = inSharesByAllUsers->get(i);
                if (!share->isVerified()) // Just in case
                {
                    broadcastMessage(getInstructionsNeedsVerification("Found some not verified share"), true);
                    return;
                }

                std::unique_ptr<MegaNode> n (api->getNodeByHandle(share->getNodeHandle()));
                if (n && !n->isNodeKeyDecrypted() && share->getUser())
                {
                    usersWithNonDecryptable.insert(share->getUser());
                }
            }
        }

        if (!usersWithNonDecryptable.empty())
        {
            std::string title("Found some inaccessible (in)share");
            if (usersWithNonDecryptable.size() > 1)
            {
                title.append("s");
            }
            broadcastMessage(getInstructionsNeedsLogin(title.c_str(), usersWithNonDecryptable), true);
            return;
        }
    }
    {
        std::unique_ptr<MegaShareList> shares(api->getUnverifiedOutShares());

        for (int i = 0; shares && i < shares->size(); i++)
        {
            if (!shares->get(i)->isPending())
            {
                broadcastMessage(getInstructionsNeedsVerification("You are sharing a folder with some unverified contact"), true);
                return;
            }
        }
    }
}

bool MegaCmdExecuter::actUponFetchNodes(MegaApi *api, SynchronousRequestListener *srl, int timeout)
{
    if (timeout == -1)
    {
        srl->wait();
    }
    else
    {
        int trywaitout = srl->trywait(timeout);
        if (trywaitout)
        {
            LOG_err << "Fetch nodes took too long, it may have failed. No further actions performed";
            return false;
        }
    }

    if (srl->getError()->getErrorCode() == MegaError::API_EBLOCKED)
    {
        LOG_verbose << " EBLOCKED after fetch nodes. querying for reason...";

        MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);
        api->whyAmIBlocked(megaCmdListener); //This shall cause event that sets reasonblocked
        megaCmdListener->wait();
        auto reason = sandboxCMD->getReasonblocked();
        LOG_warn << "Failed to fetch nodes. Account blocked." <<( reason.empty()?"":" Reason: "+reason);
    }
    else if (checkNoErrors(srl->getError(), "fetch nodes"))
    {
        // Let's save the session for future resumptions
        // Note: when logged into folders,
        // folder session depends on node handle,
        // which requires fetch nodes to be complete
        // i.e. dumpSession won't be valid after login
        session = std::unique_ptr<char[]>(srl->getApi()->dumpSession());
        ConfigurationManager::saveSession(session.get());

        LOG_verbose << "ActUponFetchNodes ok. Let's wait for nodes current:";

        auto futureNodesCurrent = sandboxCMD->mNodesCurrentPromise.getFuture();
        bool discardGet = false;

        LOG_debug << "Waiting for nodes current ...";

        if (futureNodesCurrent.wait_for(std::chrono::seconds(30)) == std::future_status::timeout)
        {
            LOG_warn << "Getting up to date with last changes in your account is taking long ...";
            if (futureNodesCurrent.wait_for(std::chrono::seconds(120)) == std::future_status::timeout)
            {
                LOG_err << "Getting up to date with last changes in your account is taking more than expected. MEGAcmd will continue. "
                           "Caveat: you may be interacting with an out-dated version of your account.";
                sendEvent(StatsManager::MegacmdEvent::WAITED_TOO_LONG_FOR_NODES_CURRENT, api, false);

                discardGet = true;
            }
        }

        if (!discardGet)
        {
            auto eventCurrentArrivedOk = futureNodesCurrent.get();
            assert(eventCurrentArrivedOk);
            LOG_debug << "Waited for nodes current ... " << eventCurrentArrivedOk;
        }

        std::string sessionString(session ? session.get() : "");
        if (!sessionString.empty())
        {
            // Verify shared folders to brodcast caveat messages if required
            mDeferredSharedFoldersVerifier.triggerDeferredSingleShot([this, api] { verifySharedFolders(api); });
        }

        return true;
    }
    else
    {
        sandboxCMD->mNodesCurrentPromise.reset();
    }
    return false;
}

int MegaCmdExecuter::actUponLogin(SynchronousRequestListener *srl, int timeout)
{
    if (timeout == -1)
    {
        srl->wait();
    }
    else
    {
        int trywaitout = srl->trywait(timeout);
        if (trywaitout)
        {
            LOG_err << "Login took too long, it may have failed. No further actions performed";
            return MegaError::API_EAGAIN;
        }
    }

    LOG_debug << "actUponLogin login";
    setCurrentThreadOutCode(srl->getError()->getErrorCode());

    if (srl->getRequest()->getEmail())
    {
        LOG_debug << "actUponLogin login email: " << srl->getRequest()->getEmail();
    }


    if (srl->getError()->getErrorCode() == MegaError::API_EMFAREQUIRED) // failed to login
    {
        return srl->getError()->getErrorCode();
    }


    if (srl->getError()->getErrorCode() == MegaError::API_ENOENT) // failed to login
    {
        LOG_err << "Login failed: invalid email or password";
    }
    else if (srl->getError()->getErrorCode() == MegaError::API_EFAILED)
    {
        LOG_err << "Login failed: incorrect authentication";
    }
    else if (srl->getError()->getErrorCode() == MegaError::API_EINCOMPLETE)
    {
        LOG_err << "Login failed: unconfirmed account. Please confirm your account";
    }
    else if (checkNoErrors(srl->getError(), "Login")) //login success:
    {
        LOG_debug << "Login correct ... " << (srl->getRequest()->getEmail()?srl->getRequest()->getEmail():"");

        if (srl->getRequest()->getEmail()) // login with email
        {
            // If login with email, here we will have a valid new session
            // otherwise, we can asume that session is already stored.
            //
            // Note, if logging into a folder (no email),
            // the dumpSession reported by the SDK at this point is not valid:
            // folder session depends on node handle, which requires fetching nodes
            session = std::unique_ptr<char[]>(srl->getApi()->dumpSession());
            ConfigurationManager::saveSession(session.get());
        }

        /* Restoring configured values */
        mtxSyncMap.lock();
        ConfigurationManager::loadsyncs();
        mtxSyncMap.unlock();

        mtxBackupsMap.lock();
        ConfigurationManager::loadbackups();
        mtxBackupsMap.unlock();

        ConfigurationManager::transitionLegacyExclusionRules(*api);

        long long maxspeeddownload = ConfigurationManager::getConfigurationValue("maxspeeddownload", -1);
        if (maxspeeddownload != -1) api->setMaxDownloadSpeed(maxspeeddownload);
        long long maxspeedupload = ConfigurationManager::getConfigurationValue("maxspeedupload", -1);
        if (maxspeedupload != -1) api->setMaxUploadSpeed(maxspeedupload);

        for (bool up :{true, false})
        {
            auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
            auto value = ConfigurationManager::getConfigurationValue(up ? "maxuploadconnections" : "maxdownloadconnections", -1);
            if (value != -1)
            {
                api->setMaxConnections(up ? 1 : 0, value, megaCmdListener.get());
                megaCmdListener->wait();
                if (megaCmdListener->getError()->getErrorCode() != MegaError::API_OK)
                {
                    LOG_err << "Failed to change max " << (up ? "upload" : "download") << " connections: "
                            << megaCmdListener->getError()->getErrorString();
                }
            }
        }

        for (auto &vc : Instance<ConfiguratorMegaApiHelper>::Get().getConfigurators())
        {
            auto name = vc.mKey.c_str();
            auto newValueOpt = vc.mGetter(api, name);
            if (newValueOpt)
            {
                if (vc.mValidator && !vc.mValidator.value()(newValueOpt->data()))
                {
                    LOG_err << "Failed to change " << vc.mKey << " (" << vc.mDescription << ") after login. Invalid value in configuration";
                    continue;
                }

                auto previousErrorCode = getCurrentThreadOutCode();
                if (!vc.mSetter(api, std::string(name), newValueOpt->data()))
                {
                    LOG_err << "Failed to change " << vc.mKey << " (" << vc.mDescription << ") after login. Setting failed";
                    setCurrentThreadOutCode(previousErrorCode); //Do not consider this a a failure on login ...
                }
            }
        }

        api->useHttpsOnly(ConfigurationManager::getConfigurationValue("https", false));
        api->disableGfxFeatures(!ConfigurationManager::getConfigurationValue("graphics", true));

#ifndef _WIN32
        string permissionsFiles = ConfigurationManager::getConfigurationSValue("permissionsFiles");
        if (permissionsFiles.size())
        {
            int perms = permissionsFromReadable(permissionsFiles);
            if (perms != -1)
            {
                api->setDefaultFilePermissions(perms);
            }
        }
        string permissionsFolders = ConfigurationManager::getConfigurationSValue("permissionsFolders");
        if (permissionsFolders.size())
        {
            int perms = permissionsFromReadable(permissionsFolders);
            if (perms != -1)
            {
                api->setDefaultFolderPermissions(perms);
            }
        }
#endif

        ConfigurationManager::migrateSyncConfig(api);

        LOG_info << "Fetching nodes ... ";
        MegaApi *api = srl->getApi();
        int clientID = static_cast<MegaCmdListener*>(srl)->clientID;

        fetchNodes(api, clientID);
    }

#if defined(_WIN32) || defined(__APPLE__)

    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
    srl->getApi()->getLastAvailableVersion("BdARkQSQ",megaCmdListener);
    megaCmdListener->wait();

    if (!megaCmdListener->getError())
    {
        LOG_fatal << "No MegaError at getLastAvailableVersion: ";
    }
    else if (megaCmdListener->getError()->getErrorCode() != MegaError::API_OK)
    {
        LOG_debug << "Couldn't get latests available version: " << megaCmdListener->getError()->getErrorString();
    }
    else
    {
        if (megaCmdListener->getRequest()->getNumber() != MEGACMD_CODE_VERSION)
        {

            OUTSTREAM << "---------------------------------------------------------------------" << endl;
            OUTSTREAM << "--        There is a new version available of megacmd: " << getLeftAlignedStr(megaCmdListener->getRequest()->getName(),12) << "--" << endl;
            OUTSTREAM << "--        Please, update this one: See \"update --help\".          --" << endl;
            OUTSTREAM << "--        Or download the latest from https://mega.nz/cmd          --" << endl;
            OUTSTREAM << "---------------------------------------------------------------------" << endl;
        }
    }
    delete megaCmdListener;

    //this goes here in case server is launched directly and thus we ensure that's shown at the beginning
    int autoupdate = ConfigurationManager::getConfigurationValue("autoupdate", -1);
    bool enabledupdaterhere = false;
    if (autoupdate == -1 || autoupdate == 2)
    {
        OUTSTREAM << "ENABLING AUTOUPDATE BY DEFAULT. You can disable it with \"update --auto=off\"" << endl;
        autoupdate = 1;
        enabledupdaterhere = true;
    }

    if (autoupdate >= 1)
    {
        startcheckingForUpdates();
    }
    if (enabledupdaterhere)
    {
        ConfigurationManager::savePropertyValue("autoupdate", 2); //save to special value to indicate first listener that it is enabled
    }

#endif
    return srl->getError()->getErrorCode();
}

void MegaCmdExecuter::fetchNodes(MegaApi *api, int clientID)
{
    if (!api) api = this->api;

    sandboxCMD->mNodesCurrentPromise.initiatePromise();
    auto megaCmdListener = std::make_unique<MegaCmdListener>(api, nullptr, clientID);
    api->fetchNodes(megaCmdListener.get());

    if (!actUponFetchNodes(api, megaCmdListener.get()))
    {
        //Ideally we should enter an state that indicates that we are not fully logged.
        //Specially when the account is blocked
        return;
    }

    // This is the actual acting upon fetch nodes ended correctly:

    //automatic now:
    //api->enableTransferResumption();

    auto cwdNode = (cwd == UNDEF) ? nullptr : std::unique_ptr<MegaNode>(api->getNodeByHandle(cwd));
    if (cwd == UNDEF || !cwdNode)
    {
        auto rootNode = std::unique_ptr<MegaNode>(api->getRootNode());
        if (rootNode)
        {
            cwd = rootNode->getHandle();
        }
        else
        {
            LOG_err << "Root node was not found after fetching nodes";
            sendEvent(StatsManager::MegacmdEvent::ROOT_NODE_NOT_FOUND_AFTER_FETCHING, api);
        }
    }

    setloginInAtStartup(false); //to enable all commands before giving clients the green light!
    informStateListeners("loged:"); // tell the clients login ended, before providing them the first prompt
    updateprompt(api);
    LOG_debug << " Fetch nodes correctly";

    MegaUser *u = api->getMyUser();
    if (u)
    {
        LOG_info << "Login complete as " << u->getEmail();
        delete u;
    }

    if (ConfigurationManager::getConfigurationValue("ask4storage", true))
    {
        ConfigurationManager::savePropertyValue("ask4storage",false);
        auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
        api->getAccountDetails(megaCmdListener.get());
        megaCmdListener->wait();
        // we don't call getAccountDetails on startup always: we ask on first login (no "ask4storage") or previous state was STATE_RED | STATE_ORANGE
        // if we were green, don't need to ask: if there are changes they will be received via action packet indicating STATE_CHANGE
    }

    checkAndInformPSA(NULL); // this needs broacasting in case there's another Shell running.
    // no need to enforce, because time since last check should has been restored

    mtxBackupsMap.lock();
    if (ConfigurationManager::configuredBackups.size())
    {
        LOG_info << "Restablishing backups ... ";
        unsigned int i=0;
        for (map<string, backup_struct *>::iterator itr = ConfigurationManager::configuredBackups.begin();
             itr != ConfigurationManager::configuredBackups.end(); ++itr, i++)
        {
            backup_struct *thebackup = itr->second;

            MegaNode * node = api->getNodeByHandle(thebackup->handle);
            if (establishBackup(thebackup->localpath, node, thebackup->period, thebackup->speriod, thebackup->numBackups))
            {
                thebackup->failed = false;
                const char *nodepath = api->getNodePath(node);
                LOG_debug << "Successfully resumed backup: " << thebackup->localpath << " to " << nodepath;
                delete []nodepath;
            }
            else
            {
                thebackup->failed = true;
                char *nodepath = api->getNodePath(node);
                LOG_err << "Failed to resume backup: " << thebackup->localpath << " to " << nodepath;
                delete []nodepath;
            }

            delete node;
        }

        ConfigurationManager::saveBackups(&ConfigurationManager::configuredBackups);
    }
    mtxBackupsMap.unlock();

#ifdef HAVE_LIBUV
    // restart webdav
    int port = ConfigurationManager::getConfigurationValue("webdav_port", -1);
    if (port != -1)
    {
        bool localonly = ConfigurationManager::getConfigurationValue("webdav_localonly", -1);
        bool tls = ConfigurationManager::getConfigurationValue("webdav_tls", false);
        string pathtocert, pathtokey;
        pathtocert = ConfigurationManager::getConfigurationSValue("webdav_cert");
        pathtokey = ConfigurationManager::getConfigurationSValue("webdav_key");

        api->httpServerEnableFolderServer(true);
        if (api->httpServerStart(localonly, port, tls, pathtocert.c_str(), pathtokey.c_str()))
        {
            list<string> servedpaths = ConfigurationManager::getConfigurationValueList("webdav_served_locations");
            bool modified = false;

            for ( std::list<string>::iterator it = servedpaths.begin(); it != servedpaths.end(); ++it)
            {
                string pathToServe = *it;
                if (pathToServe.size())
                {
                    std::unique_ptr<MegaNode> n = nodebypath(pathToServe.c_str());
                    if (n)
                    {
                        char* l = api->httpServerGetLocalWebDavLink(n.get());
                        char* actualNodePath = api->getNodePath(n.get());
                        LOG_debug << "Serving via webdav: " << actualNodePath << ": " << l;

                        if (pathToServe != actualNodePath)
                        {
                            it = servedpaths.erase(it);
                            servedpaths.insert(it,string(actualNodePath));
                            modified = true;
                        }
                        delete []l;
                        delete []actualNodePath;
                    }
                    else
                    {
                        LOG_warn << "Could no find location to server via webdav: " << pathToServe;
                    }
                }
            }
            if (modified)
            {
                ConfigurationManager::savePropertyValueList("webdav_served_locations", servedpaths);
            }

            LOG_info << "Webdav server restored due to saved configuration";
        }
        else
        {
            LOG_err << "Failed to initialize WEBDAV server. Ensure the port is free.";
        }
    }

    //ftp
    // restart ftp
    int portftp = ConfigurationManager::getConfigurationValue("ftp_port", -1);

    if (portftp != -1)
    {
        bool localonly = ConfigurationManager::getConfigurationValue("ftp_localonly", -1);
        bool tls = ConfigurationManager::getConfigurationValue("ftp_tls", false);
        string pathtocert, pathtokey;
        pathtocert = ConfigurationManager::getConfigurationSValue("ftp_cert");
        pathtokey = ConfigurationManager::getConfigurationSValue("ftp_key");
        int dataPortRangeBegin = ConfigurationManager::getConfigurationValue("ftp_port_data_begin", 1500);
        int dataPortRangeEnd = ConfigurationManager::getConfigurationValue("ftp_port_data_end", 1500+100);

        if (api->ftpServerStart(localonly, portftp, dataPortRangeBegin, dataPortRangeEnd, tls, pathtocert.c_str(), pathtokey.c_str()))
        {
            list<string> servedpaths = ConfigurationManager::getConfigurationValueList("ftp_served_locations");
            bool modified = false;

            for ( std::list<string>::iterator it = servedpaths.begin(); it != servedpaths.end(); ++it)
            {
                string pathToServe = *it;
                if (pathToServe.size())
                {
                    std::unique_ptr<MegaNode> n = nodebypath(pathToServe.c_str());
                    if (n)
                    {
                        char* l = api->ftpServerGetLocalLink(n.get());
                        char* actualNodePath = api->getNodePath(n.get());
                        LOG_debug << "Serving via ftp: " << pathToServe << ": " << l << ". Data Channel Port Range: " << dataPortRangeBegin << "-" << dataPortRangeEnd;

                        if (pathToServe != actualNodePath)
                        {
                            it = servedpaths.erase(it);
                            servedpaths.insert(it,string(actualNodePath));
                            modified = true;
                        }
                        delete []l;
                        delete []actualNodePath;
                    }
                    else
                    {
                        LOG_warn << "Could no find location to server via ftp: " << pathToServe;
                    }
                }
            }
            if (modified)
            {
                ConfigurationManager::savePropertyValueList("ftp_served_locations", servedpaths);
            }

            LOG_info << "FTP server restored due to saved configuration";
        }
        else
        {
            LOG_err << "Failed to initialize FTP server. Ensure the port is free.";
        }
    }
#endif
}

void MegaCmdExecuter::actUponLogout(MegaApi& api, MegaError* e, bool keptSession)
{
    if (e->getErrorCode() == MegaError::API_ESID || checkNoErrors(e, "logout"))
    {
        LOG_verbose << "actUponLogout logout ok";
        cwd = UNDEF;
        session.reset();
        mtxSyncMap.lock();
        ConfigurationManager::unloadConfiguration();
        if (!keptSession)
        {
            ConfigurationManager::saveSession("");
            ConfigurationManager::saveBackups(&ConfigurationManager::configuredBackups);
            ConfigurationManager::saveSyncs(&ConfigurationManager::oldConfiguredSyncs);
        }
        ConfigurationManager::clearConfigurationFile();

        mtxSyncMap.unlock();

        // clear greetings (asuming they are account-related)
        clearGreetingStatusAllListener();
        clearGreetingStatusFirstListener();
    }
    updateprompt(&api);
}

void MegaCmdExecuter::actUponLogout(SynchronousRequestListener *srl, bool keptSession, int timeout)
{
    if (!timeout)
    {
        srl->wait();
    }
    else
    {
        int trywaitout = srl->trywait(timeout);
        if (trywaitout)
        {
            LOG_err << "Logout took too long, it may have failed. No further actions performed";
            return;
        }
    }
    actUponLogout(*api, srl->getError(), keptSession);
}

int MegaCmdExecuter::actUponCreateFolder(SynchronousRequestListener *srl, int timeout)
{
    if (!timeout)
    {
        srl->wait();
    }
    else
    {
        int trywaitout = srl->trywait(timeout);
        if (trywaitout)
        {
            LOG_err << "actUponCreateFolder took too long, it may have failed. No further actions performed";
            return 1;
        }
    }
    if (checkNoErrors(srl->getError(), "create folder"))
    {
        LOG_verbose << "actUponCreateFolder Create Folder ok";
        return 0;
    }

    return 2;
}

void MegaCmdExecuter::confirmDelete()
{
    if (mNodesToConfirmDelete.size())
    {
        std::unique_ptr<MegaNode> nodeToConfirmDelete = std::move(mNodesToConfirmDelete.front());
        mNodesToConfirmDelete.erase(mNodesToConfirmDelete.begin());
        doDeleteNode(nodeToConfirmDelete, api);
    }


    if (mNodesToConfirmDelete.size())
    {
        string newprompt("Are you sure to delete ");
        newprompt += mNodesToConfirmDelete.front()->getName();
        newprompt += " ? (Yes/No/All/None): ";
        setprompt(AREYOUSURETODELETE,newprompt);
    }
    else
    {
        setprompt(COMMAND);
    }

}

void MegaCmdExecuter::discardDelete()
{
    if (mNodesToConfirmDelete.size())
    {
        mNodesToConfirmDelete.erase(mNodesToConfirmDelete.begin());
    }
    if (mNodesToConfirmDelete.size())
    {
        string newprompt("Are you sure to delete ");
        newprompt += mNodesToConfirmDelete.front()->getName();
        newprompt += " ? (Yes/No/All/None): ";
        setprompt(AREYOUSURETODELETE,newprompt);
    }
    else
    {
        setprompt(COMMAND);
    }
}


void MegaCmdExecuter::confirmDeleteAll()
{
    while (mNodesToConfirmDelete.size())
    {
        std::unique_ptr<MegaNode> nodeToConfirmDelete = std::move(mNodesToConfirmDelete.front());
        mNodesToConfirmDelete.erase(mNodesToConfirmDelete.begin());
        doDeleteNode(nodeToConfirmDelete, api);
    }

    setprompt(COMMAND);
}

void MegaCmdExecuter::discardDeleteAll()
{
    mNodesToConfirmDelete.clear();
    setprompt(COMMAND);
}


void MegaCmdExecuter::doDeleteNode(const std::unique_ptr<MegaNode>& nodeToDelete, MegaApi* api)
{
    char* nodePath = api->getNodePath(nodeToDelete.get());
    if (nodePath)
    {
        LOG_verbose << "Deleting: "<< nodePath;
    }
    else
    {
        LOG_warn << "Deleting node whose path could not be found " << nodeToDelete->getName();
    }

    MegaCmdListener* megaCmdListener = new MegaCmdListener(api, nullptr);
    std::unique_ptr<MegaNode> parent(api->getParentNode(nodeToDelete.get()));
    if (parent && parent->getType() == MegaNode::TYPE_FILE)
    {
        api->removeVersion(nodeToDelete.get(), megaCmdListener);
    }
    else
    {
        api->remove(nodeToDelete.get(), megaCmdListener);
    }
    megaCmdListener->wait();

    string msj = "delete node ";
    if (nodePath)
    {
        msj += nodePath;
    }
    else
    {
        msj += nodeToDelete->getName();
    }
    checkNoErrors(megaCmdListener->getError(), msj);
    delete megaCmdListener;
    delete []nodePath;
}

int MegaCmdExecuter::deleteNodeVersions(const std::unique_ptr<MegaNode>& nodeToDelete, MegaApi* api, int force)
{
    if (nodeToDelete->getType() == MegaNode::TYPE_FILE && api->getNumVersions(nodeToDelete.get()) < 2)
    {
        if (!force)
        {
            LOG_err << "No versions found for " << nodeToDelete->getName();
        }
        return MCMDCONFIRM_YES; //nothing to do, no sense asking
    }

    int confirmationResponse;

    if (nodeToDelete->getType() != MegaNode::TYPE_FILE)
    {
        string confirmationQuery("Are you sure todelete the version histories of files within ");
        confirmationQuery += nodeToDelete->getName();
        confirmationQuery += "? (Yes/No): ";

        confirmationResponse = force?MCMDCONFIRM_ALL:askforConfirmation(confirmationQuery);

        if (confirmationResponse == MCMDCONFIRM_YES || confirmationResponse == MCMDCONFIRM_ALL)
        {
            auto children = std::unique_ptr<MegaNodeList>(api->getChildren(nodeToDelete.get()));
            if (children)
            {
                for (int i = 0; i < children->size(); i++)
                {
                    auto child = std::unique_ptr<MegaNode>(children->get(i)); // wrap the pointer into the expected type by deleteNodeVersion
                    deleteNodeVersions(child, api, true);
                    child.release(); // the MegaNodeList owns the child, we don't want to double free it
                }
            }
        }
    }
    else
    {

        string confirmationQuery("Are you sure todelete the version histories of ");
        confirmationQuery += nodeToDelete->getName();
        confirmationQuery += "? (Yes/No): ";
        confirmationResponse = force?MCMDCONFIRM_ALL:askforConfirmation(confirmationQuery);

        if (confirmationResponse == MCMDCONFIRM_YES || confirmationResponse == MCMDCONFIRM_ALL)
        {

            MegaNodeList* versionsToDelete = api->getVersions(nodeToDelete.get());
            if (versionsToDelete)
            {
                for (int i = 0; i < versionsToDelete->size(); i++)
                {
                    MegaNode *versionNode = versionsToDelete->get(i);

                    if (versionNode->getHandle() != nodeToDelete->getHandle())
                    {
                        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                        api->removeVersion(versionNode,megaCmdListener);
                        megaCmdListener->wait();
                        string fullname(versionNode->getName()?versionNode->getName():"NO_NAME");
                        fullname += "#";
                        fullname += SSTR(versionNode->getModificationTime());
                        if (checkNoErrors(megaCmdListener->getError(), "remove version: "+fullname))
                        {
                            LOG_verbose << " Removed " << fullname << " (" << getReadableTime(versionNode->getModificationTime()) << ")";
                        }
                        delete megaCmdListener;
                    }
                }
                delete versionsToDelete;
            }
        }
    }
    return confirmationResponse;
}

/**
 * @brief MegaCmdExecuter::deleteNode
 * @param nodeToDelete this function will delete this accordingly
 * @param api
 * @param recursive
 * @param force
 * @return confirmation code
 */
int MegaCmdExecuter::deleteNode(const std::unique_ptr<MegaNode>& nodeToDelete, MegaApi* api, int recursive, int force)
{
    if (nodeToDelete->getType() != MegaNode::TYPE_FILE && !recursive)
    {
        char* nodePath = api->getNodePath(nodeToDelete.get());
        setCurrentThreadOutCode(MCMD_INVALIDTYPE);
        LOG_err << "Unable to delete folder: " << nodePath << ". Use -r to delete a folder recursively";
        delete []nodePath;
    }
    else
    {
        if (!isCurrentThreadCmdShell() && isCurrentThreadInteractive() && !force && nodeToDelete->getType() != MegaNode::TYPE_FILE)
        {
            bool alreadythere = false;
            for (const auto& node : mNodesToConfirmDelete)
            {
                if (node->getHandle() == nodeToDelete->getHandle())
                {
                    alreadythere= true;
                }
            }
            if (!alreadythere)
            {
                if (getprompt() != AREYOUSURETODELETE)
                {
                    string newprompt("Are you sure to delete ");
                    newprompt += nodeToDelete->getName();
                    newprompt += " ? (Yes/No/All/None): ";
                    setprompt(AREYOUSURETODELETE, newprompt);
                }
                mNodesToConfirmDelete.emplace_back(nodeToDelete->copy());
            }

            return MCMDCONFIRM_NO; //default return
        }
        else if (!force && nodeToDelete->getType() != MegaNode::TYPE_FILE)
        {
            string confirmationQuery("Are you sure to delete ");
            confirmationQuery += nodeToDelete->getName();
            confirmationQuery += " ? (Yes/No/All/None): ";

            int confirmationResponse = askforConfirmation(confirmationQuery);

            if (confirmationResponse == MCMDCONFIRM_YES || confirmationResponse == MCMDCONFIRM_ALL)
            {
                LOG_debug << "confirmation received";
                doDeleteNode(nodeToDelete, api);
            }
            else
            {
                LOG_debug << "confirmation denied";
            }
            return confirmationResponse;
        }
        else //force
        {
            doDeleteNode(nodeToDelete, api);
            return MCMDCONFIRM_ALL;
        }
    }

    return MCMDCONFIRM_NO; //default return
}

void MegaCmdExecuter::downloadNode(string source, string path, MegaApi* api, MegaNode *node, bool background, bool ignorequotawarn,
                                   int clientID, std::shared_ptr<MegaCmdMultiTransferListener> multiTransferListener)
{


    if (sandboxCMD->isOverquota() && !ignorequotawarn)
    {
        m_time_t ts = m_time();
        // in order to speedup and not flood the server we only ask for the details every 1 minute or after account changes
        if (!sandboxCMD->temporalbandwidth || (ts - sandboxCMD->lastQuerytemporalBandwith ) > 60 )
        {
            LOG_verbose << " Updating temporal bandwidth ";
            sandboxCMD->lastQuerytemporalBandwith = ts;

            MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);
            api->getExtendedAccountDetails(false, false, false, megaCmdListener);
            megaCmdListener->wait();

            if (checkNoErrors(megaCmdListener->getError(), "get account details"))
            {
                MegaAccountDetails *details = megaCmdListener->getRequest()->getMegaAccountDetails();
                sandboxCMD->istemporalbandwidthvalid = details->isTemporalBandwidthValid();
                if (details && details->isTemporalBandwidthValid())
                {
                    sandboxCMD->temporalbandwidth = details->getTemporalBandwidth();
                    sandboxCMD->temporalbandwithinterval = details->getTemporalBandwidthInterval();
                }
            }
            delete megaCmdListener;
        }

        OUTSTREAM << "Transfer not started. " << endl;
        if (sandboxCMD->istemporalbandwidthvalid)
        {
            OUTSTREAM << "You have utilized " << sizeToText(sandboxCMD->temporalbandwidth) << " of data transfer in the last "
                      << sandboxCMD->temporalbandwithinterval << " hours, "
                      "which took you over our current limit";
        }
        else
        {
            OUTSTREAM << "You have reached your bandwidth quota";
        }
        OUTSTREAM << ". To circumvent this limit, "
                  "you can upgrade to Pro, which will give you your own bandwidth "
                  "package and also ample extra storage space. "
                     "Alternatively, you can try again in " << secondsToText(sandboxCMD->secondsOverQuota-(ts-sandboxCMD->timeOfOverquota)) <<
                     "." << endl << "See \"help --upgrade\" for further details" << endl;
        OUTSTREAM << "Use --ignore-quota-warn to initiate nevertheless" << endl;
        return;
    }

    if (!ignorequotawarn)
    {
        MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);
        api->queryTransferQuota(node->getSize(),megaCmdListener);
        megaCmdListener->wait();
        if (checkNoErrors(megaCmdListener->getError(), "query transfer quota"))
        {
            if (megaCmdListener->getRequest() && megaCmdListener->getRequest()->getFlag() )
            {
                OUTSTREAM << "Transfer not started: proceeding will exceed transfer quota. "
                             "Use --ignore-quota-warn to initiate nevertheless" << endl;
                return;
            }
        }
        delete megaCmdListener;
    }

    multiTransferListener->onNewTransfer();

#ifdef _WIN32
    replaceAll(path,"/","\\");
#endif
    LOG_debug << "Starting download: " << node->getName() << " to : " << path;

    api->startDownload(
                node, //MegaNode* node,
                path.c_str(), // const char* localPath,
                nullptr, // const char *customName,
                nullptr, // const char *appData,
                false, // bool startFirst,
                nullptr, // MegaCancelToken *cancelToken,
                MegaTransfer::COLLISION_CHECK_FINGERPRINT, // int collisionCheck,
                MegaTransfer::COLLISION_RESOLUTION_NEW_WITH_N, // int collisionResolution,
                false, // bool undelete
                new ATransferListener(multiTransferListener, source) // MegaTransferListener *listener
     );
}

void MegaCmdExecuter::uploadNode(string path, MegaApi* api, MegaNode *node, string newname,
                                 bool background, bool ignorequotawarn, int clientID, MegaCmdMultiTransferListener *multiTransferListener)
{
    if (!ignorequotawarn)
    { //TODO: reenable this if ever queryBandwidthQuota applies to uploads as well
//        MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);
//        api->queryBandwidthQuota(node->getSize(),megaCmdListener);
//        megaCmdListener->wait();
//        if (checkNoErrors(megaCmdListener->getError(), "query bandwidth quota"))
//        {
//            if (megaCmdListener->getRequest() && megaCmdListener->getRequest()->getFlag() )
//            {
//                OUTSTREAM << "Transfer not started: proceding will exceed bandwith quota. "
//                             "Use --ignore-quota-warn to initiate nevertheless" << endl;
//                return;
//            }
//        }
    }
    unescapeifRequired(path);

    if (!pathExists(path))
    {
        setCurrentThreadOutCode(MCMD_NOTFOUND);
        LOG_err << "Unable to open local path: " << path;
        return;
    }

    MegaTransferListener *thelistener = nullptr;
    std::unique_ptr<MegaCmdTransferListener> singleNonBackgroundTransferListener;
    if (!background)
    {
        if (!multiTransferListener)
        {
            singleNonBackgroundTransferListener.reset(new MegaCmdTransferListener(api, sandboxCMD, nullptr, clientID));
            thelistener = singleNonBackgroundTransferListener.get();
        }
        else
        {
            multiTransferListener->onNewTransfer();
            thelistener = multiTransferListener;
        }
    }
    else
    {
        thelistener = nullptr;
    }

#ifdef _WIN32
    replaceAll(path,"/","\\");
#endif

    LOG_debug << "Starting upload: " << path << " to : " << node->getName() << (newname.size()?"/":"") << newname;

    api->startUpload(
                removeTrailingSeparators(path).c_str(),//const char *localPath,
                node,//MegaNode *parent,
                 newname.size() ? newname.c_str() : nullptr,//const char *fileName,
                MegaApi::INVALID_CUSTOM_MOD_TIME,//int64_t mtime,
                nullptr,//const char *appData,
                false, //bool isSourceTemporary,
                false, //bool startFirst,
                nullptr,//MegaCancelToken *cancelToken,
                thelistener);

    if (singleNonBackgroundTransferListener)
    {
        assert(!background);
        singleNonBackgroundTransferListener->wait();
#ifdef _WIN32
            Sleep(100); //give a while to print end of transfer
#endif
        if (singleNonBackgroundTransferListener->getError()->getErrorCode() == API_EREAD)
        {
            setCurrentThreadOutCode(MCMD_NOTFOUND);
            LOG_err << "Could not find local path: " << path;
        }
        else if (checkNoErrors(singleNonBackgroundTransferListener->getError(), "Upload node"))
        {
            char * destinyPath = api->getNodePath(node);
            LOG_info << "Upload complete: " << path << " to " << destinyPath << newname;
            delete []destinyPath;
        }
    }
}


bool MegaCmdExecuter::amIPro()
{
    int prolevel = -1;

#ifdef MEGACMD_TESTING_CODE
    auto proLevelOpt = TI::Instance().testValue(TI::TestValue::AMIPRO_LEVEL);
    if (proLevelOpt)
    {
        prolevel = static_cast<int>(std::get<int64_t>(*proLevelOpt));
        LOG_debug << "Enforced test value for Pro Level: " << prolevel;
        return prolevel;
    }
#endif

    auto megaCmdListener = std::make_unique<MegaCmdListener>(api, nullptr);
    api->getAccountDetails(megaCmdListener.get());
    megaCmdListener->wait();

    if (checkNoErrors(megaCmdListener->getError(), "get account details"))
    {
        std::unique_ptr<MegaAccountDetails> details(megaCmdListener->getRequest()->getMegaAccountDetails());
        prolevel = details->getProLevel();
    }

    return prolevel > 0;
}

void MegaCmdExecuter::exportNode(MegaNode *n, int64_t expireTime, const std::optional<std::string>& password,
                                 std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions)
{
    const bool force = getFlag(clflags,"f");
    const bool writable = getFlag(clflags,"writable");
    const bool megaHosted = getFlag(clflags,"mega-hosted");

    bool alreadyAcceptedBefore = false;
    bool copyrightAccepted = force ||
            [&alreadyAcceptedBefore]() { return alreadyAcceptedBefore = ConfigurationManager::getConfigurationValue("copyrightAccepted", false); }();

    if (!copyrightAccepted)
    {
        auto publicLinks = std::unique_ptr<MegaNodeList>(api->getPublicLinks());

        // Implicit acceptance (the user already has public links)
        copyrightAccepted = (publicLinks && publicLinks->size());
    }

    if (!copyrightAccepted)
    {
        string confirmationQuery("MEGA respects the copyrights of others and requires that users of the MEGA cloud service comply with the laws of copyright.\n"
                                 "You are strictly prohibited from using the MEGA cloud service to infringe copyright.\n"
                                 "You may not upload, download, store, share, display, stream, distribute, email, link to, "
                                 "transmit or otherwise make available any files, data or content that infringes any copyright "
                                 "or other proprietary rights of any person or entity.\n");

        confirmationQuery += "Do you accept these terms? (Yes/No): ";

        const int confirmationResponse = askforConfirmation(confirmationQuery);
        if (confirmationResponse != MCMDCONFIRM_YES && confirmationResponse != MCMDCONFIRM_ALL)
        {
            return;
        }
    }

    if (!alreadyAcceptedBefore)
    {
        // Save as accepted regardless of the source of acceptance
        ConfigurationManager::savePropertyValue("copyrightAccepted", true);
    }

    auto megaCmdListener = std::make_unique<MegaCmdListener>(api, nullptr);
    api->exportNode(n, expireTime, writable, megaHosted, megaCmdListener.get());
    megaCmdListener->wait();

    auto error = megaCmdListener->getError();
    assert(error != nullptr);

    if (error->getErrorCode() != MegaError::API_OK)
    {
        auto path = std::unique_ptr<char[]>(api->getNodePath(n));
        std::string msg = "Failed to export node";

        if (path != nullptr)
        {
            msg.append(" ").append(path.get());
        }

        if (expireTime != 0 && !amIPro())
        {
            msg.append(": Only PRO users can set an expiry time for links");
        }
        else if (path != nullptr && strcmp(path.get(), "/") == 0)
        {
            msg.append(": The root folder cannot be exported");
        }
        else
        {
            msg.append(": ").append(formatErrorAndMaySetErrorCode(*error));
        }
        LOG_err << msg;

        return;
    }

    auto nexported = std::unique_ptr<MegaNode>(api->getNodeByHandle(megaCmdListener->getRequest()->getNodeHandle()));
    if (!nexported)
    {
        setCurrentThreadOutCode(MCMD_NOTFOUND);
        LOG_err << "Exported node not found";
        return;
    }

    auto publicLink = std::unique_ptr<char[]>(nexported->getPublicLink());
    if (!publicLink)
    {
        setCurrentThreadOutCode(MCMD_NOTFOUND);
        LOG_err << "Public link for exported node not found";
        return;
    }

    auto nodepath = std::unique_ptr<char[]>(api->getNodePath(nexported.get()));
    if (!nodepath)
    {
        setCurrentThreadOutCode(MCMD_NOTFOUND);
        LOG_err << "Path for exported node not found";
        return;
    }

    string publicPassProtectedLink;
    if (password)
    {
        // Encrypting links with passwords is a client-side operation that will be done regardless
        // of PRO status of the account. So we need to manually check for it before calling
        // `encryptLinkWithPassword`; the function itself will not fail check this.
        if (amIPro())
        {
            megaCmdListener.reset(new MegaCmdListener(api, nullptr));
            api->encryptLinkWithPassword(publicLink.get(), password->c_str(), megaCmdListener.get());
            megaCmdListener->wait();

            if (checkNoErrors(megaCmdListener->getError(), "protect public link with password"))
            {
                publicPassProtectedLink = megaCmdListener->getRequest()->getText();
            }
        }
        else
        {
            LOG_err << "Only PRO users can protect links with passwords. Showing UNPROTECTED link";
        }
    }

    const int64_t actualExpireTime = nexported->getExpirationTime();
    if (expireTime != 0 && !actualExpireTime)
    {
        setCurrentThreadOutCode(MCMD_INVALIDSTATE);
        LOG_err << "Could not add expiration date to exported node";
    }

    const string authKey(nexported->getWritableLinkAuthKey() ? nexported->getWritableLinkAuthKey() : "");
    if (writable && authKey.empty())
    {
        setCurrentThreadOutCode(MCMD_INVALIDSTATE);
        LOG_err << "Failed to generate writable folder: missing auth key. Showing read-only link";
    }

    const string nodeLink(publicPassProtectedLink.size() ? publicPassProtectedLink : publicLink.get());
    OUTSTREAM << "Exported " << nodepath.get() << ": " << nodeLink;

    constexpr const char* prefix = "https://mega.nz/folder/";
    if (authKey.size() && nodeLink.rfind(prefix, 0) == 0)
    {
        string authToken = nodeLink.substr(strlen(prefix)).append(":").append(authKey);
        OUTSTREAM << "\n          AuthToken = " << authToken;
        if (megaHosted && megaCmdListener->getRequest()->getPassword())
        {
            OUTSTREAM << "\n          Share key encryption key = " << megaCmdListener->getRequest()->getPassword();
        }
    }

    if (actualExpireTime)
    {
        OUTSTREAM << " expires at " << getReadableTime(nexported->getExpirationTime());
    }

    OUTSTREAM << endl;
}

void MegaCmdExecuter::disableExport(MegaNode *n)
{
    if (!n->isExported())
    {
        setCurrentThreadOutCode(MCMD_INVALIDSTATE);
        LOG_err << "Could not disable export: node not exported.";
        return;
    }
    MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);

    api->disableExport(n, megaCmdListener);
    megaCmdListener->wait();
    if (checkNoErrors(megaCmdListener->getError(), "disable export"))
    {
        MegaNode *nexported = api->getNodeByHandle(megaCmdListener->getRequest()->getNodeHandle());
        if (nexported)
        {
            char *nodepath = api->getNodePath(nexported);
            OUTSTREAM << "Disabled export: " << nodepath << endl;
            delete[] nodepath;
            delete nexported;
        }
        else
        {
            setCurrentThreadOutCode(MCMD_NOTFOUND);
            LOG_err << "Exported node not found!";
        }
    }

    delete megaCmdListener;
}

std::pair<bool/*pending*/, bool /*verified*/> MegaCmdExecuter::isSharePendingAndVerified(MegaNode* n, const char *email) const
{
    if (!email)
    {
        return std::make_pair(false, false);
    }
    std::unique_ptr<MegaShareList> outShares (api->getOutShares(n));
    if (outShares)
    {
        for (int i = 0; i < outShares->size(); i++)
        {
            auto outShare = outShares->get(i);
            auto shareEmail = outShare->getUser();
            if (shareEmail && !strcmp(email, shareEmail))
            {
                return std::make_pair(outShare->isPending(), outShare->isVerified());
            }
        }
    }

    // just in case: loop pending ones too:
    std::unique_ptr<MegaShareList> pendingoutShares (api->getPendingOutShares(n));
    if (pendingoutShares)
    {
        for (int i = 0; i < pendingoutShares->size(); i++)
        {
            auto pendingoutShare = pendingoutShares->get(i);
            auto shareEmail = pendingoutShare->getUser();
            if (shareEmail && !strcmp(email, shareEmail))
            {
                return std::make_pair(true, pendingoutShare->isVerified());
            }
        }
    }
    return std::make_pair(false, false);
}

void MegaCmdExecuter::shareNode(MegaNode *n, string with, int level)
{
    std::unique_ptr<MegaCmdListener> megaCmdListener(new MegaCmdListener(api));
    api->openShareDialog(n, megaCmdListener.get());
    megaCmdListener->wait();
    if (megaCmdListener->getError()->getErrorCode() == MegaError::API_EINCOMPLETE)
    {
        setCurrentThreadOutCode(MCMD_NOTPERMITTED);
        LOG_err << "Unable to share folder. Your account security may need upgrading. Type \"" <<getCommandPrefixBasedOnMode() << "confirm --security\"";
        return;
    }
    else if (!checkNoErrors(megaCmdListener->getError(), "prepare sharing"))
    {
        return;
    }


    megaCmdListener.reset(new MegaCmdListener(api));
    api->share(n, with.c_str(), level, megaCmdListener.get());
    megaCmdListener->wait();

    if (megaCmdListener->getError()->getErrorCode() == MegaError::API_EINCOMPLETE)
    {
        setCurrentThreadOutCode(MCMD_NOTPERMITTED);
        LOG_err << "Unable to share folder. Your account security may need upgrading. Type \"" <<getCommandPrefixBasedOnMode() << "confirm --security\"";
        return;
    }
    else if (checkNoErrors(megaCmdListener->getError(), ( level != MegaShare::ACCESS_UNKNOWN ) ? "share node" : "disable share"))
    {
        MegaNode *nshared = api->getNodeByHandle(megaCmdListener->getRequest()->getNodeHandle());
        if (nshared)
        {
            char *nodepath = api->getNodePath(nshared);
            if (megaCmdListener->getRequest()->getAccess() == MegaShare::ACCESS_UNKNOWN)
            {
                OUTSTREAM << "Stopped sharing " << nodepath << " with " << megaCmdListener->getRequest()->getEmail() << endl;
            }
            else
            {
                auto pendingAndVerified = isSharePendingAndVerified(n, megaCmdListener->getRequest()->getEmail());
                auto pending = pendingAndVerified.first;
                auto verified = pendingAndVerified.second;
                OUTSTREAM << "New share: ";
                printOutShareInfo(nodepath, megaCmdListener->getRequest()->getEmail(),
                                  megaCmdListener->getRequest()->getAccess(), pending, verified);
            }
            delete[] nodepath;
            delete nshared;
        }
        else
        {
            setCurrentThreadOutCode(MCMD_NOTFOUND);
            LOG_err << "Shared node not found!";
        }
    }

}

void MegaCmdExecuter::disableShare(MegaNode *n, string with)
{
    shareNode(n, with, MegaShare::ACCESS_UNKNOWN);
}

int MegaCmdExecuter::makedir(string remotepath, bool recursive, MegaNode *parentnode)
{
    MegaNode *currentnode;
    if (parentnode)
    {
        currentnode = parentnode;
    }
    else
    {
        currentnode = api->getNodeByHandle(cwd);
    }
    if (currentnode)
    {
        string rest = remotepath;
        while (rest.length())
        {
            bool lastleave = false;
            size_t possep = rest.find_first_of("/");
            if (possep == string::npos)
            {
                possep = rest.length();
                lastleave = true;
            }

            string newfoldername = rest.substr(0, possep);
            if (!rest.length())
            {
                break;
            }
            if (newfoldername.length())
            {
                MegaNode *existing_node = api->getChildNode(currentnode, newfoldername.c_str());
                if (!existing_node)
                {
                    if (!recursive && !lastleave)
                    {
                        LOG_err << "Use -p to create folders recursively";
                        if (currentnode != parentnode)
                            delete currentnode;
                        return MCMD_EARGS;
                    }
                    LOG_verbose << "Creating (sub)folder: " << newfoldername;
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                    api->createFolder(newfoldername.c_str(), currentnode, megaCmdListener);
                    actUponCreateFolder(megaCmdListener);
                    delete megaCmdListener;
                    MegaNode *prevcurrentNode = currentnode;
                    currentnode = api->getChildNode(currentnode, newfoldername.c_str());
                    if (prevcurrentNode != parentnode)
                        delete prevcurrentNode;
                    if (!currentnode)
                    {
                        LOG_err << "Couldn't get node for created subfolder: " << newfoldername;
                        if (currentnode != parentnode)
                            delete currentnode;
                        return MCMD_INVALIDSTATE;
                    }
                }
                else
                {
                    if (currentnode != parentnode)
                        delete currentnode;
                    currentnode = existing_node;
                }

                if (lastleave && existing_node)
                {
                    LOG_err << ((existing_node->getType() == MegaNode::TYPE_FILE)?"File":"Folder") << " already exists: " << remotepath;
                    if (currentnode != parentnode)
                        delete currentnode;
                    return MCMD_INVALIDSTATE;
                }
            }

            //string rest = rest.substr(possep+1,rest.length()-possep-1);
            if (!lastleave)
            {
                rest = rest.substr(possep + 1, rest.length());
            }
            else
            {
                break;
            }
        }
        if (currentnode != parentnode)
            delete currentnode;
    }
    else
    {
        return MCMD_EARGS;
    }
    return MCMD_OK;

}


string MegaCmdExecuter::getCurrentPath()
{
    string toret;
    MegaNode *ncwd = api->getNodeByHandle(cwd);
    if (ncwd)
    {
        char *currentPath = api->getNodePath(ncwd);
        toret = string(currentPath);
        delete []currentPath;
        delete ncwd;
    }
    return toret;
}

long long MegaCmdExecuter::getVersionsSize(MegaNode *n)
{
    long long toret = 0;

    MegaNodeList *versionNodes = api->getVersions(n);
    if (versionNodes)
    {
        for (int i = 0; i < versionNodes->size(); i++)
        {
            MegaNode *versionNode = versionNodes->get(i);
            toret += api->getSize(versionNode);
        }
        delete versionNodes;
    }

    MegaNodeList *children = api->getChildren(n);
    if (children)
    {
        for (int i = 0; i < children->size(); i++)
        {
            MegaNode *child = children->get(i);
            toret += getVersionsSize(child);
        }
        delete children;
    }
    return toret;
}

vector<string> MegaCmdExecuter::listpaths(bool usepcre, string askedPath, bool discardFiles)
{
    vector<string> paths;
    if ((int)askedPath.size())
    {
        vector<string> *pathsToList = nodesPathsbypath(askedPath.c_str(), usepcre);
        if (pathsToList)
        {
            for (std::vector< string >::iterator it = pathsToList->begin(); it != pathsToList->end(); ++it)
            {
                string nodepath= *it;
                MegaNode *ncwd = api->getNodeByHandle(cwd);
                if (ncwd)
                {
                    std::unique_ptr<MegaNode> n = nodebypath(nodepath.c_str());
                    if (n)
                    {
                        if (n->getType() != MegaNode::TYPE_FILE)
                        {
                            nodepath += "/";
                        }
                        if (!(discardFiles && (n->getType() == MegaNode::TYPE_FILE)))
                        {
                            paths.push_back(nodepath);
                        }
                    }
                    else
                    {
                        LOG_debug << "Unexpected: matching path has no associated node: " << nodepath << ". Could have been deleted in the process";
                    }
                    delete ncwd;
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_INVALIDSTATE);
                    LOG_err << "Couldn't find woking folder (it might been deleted)";
                }
            }
            pathsToList->clear();
            delete pathsToList;
        }
    }

    return paths;
}

#ifdef _WIN32
//TODO: try to use these functions from somewhere else
static std::wstring toUtf16String(const std::string& s, UINT codepage = CP_UTF8)
{
    std::wstring ws;
    ws.resize(s.size() + 1);
    int nwchars = MultiByteToWideChar(codepage, 0, s.data(), int(s.size()), (LPWSTR)ws.data(), int(ws.size()));
    ws.resize(nwchars);
    return ws;
}

std::string toUtf8String(const std::wstring& ws, UINT codepage = CP_UTF8)
{
    std::string s;
    s.resize((ws.size() + 1) * 4);
    int nchars = WideCharToMultiByte(codepage, 0, ws.data(), int(ws.size()), (LPSTR)s.data(), int(s.size()), NULL, NULL);
    s.resize(nchars);
    return s;
}


bool replaceW(std::wstring& str, const std::wstring& from, const std::wstring& to)
{
    size_t start_pos = str.find(from);
    if (start_pos == std::wstring::npos)
    {
        return false;
    }
    str.replace(start_pos, from.length(), to);
    return true;
}
#endif

vector<string> MegaCmdExecuter::listLocalPathsStartingBy(string askedPath, bool discardFiles)
{
    vector<string> paths;

#ifdef WIN32
    string actualaskedPath = fs::u8path(askedPath).u8string();
    char sep = (!askedPath.empty() && askedPath.find('/') != string::npos ) ?'/':'\\';
    size_t postlastsep = actualaskedPath.find_last_of("/\\");
#else
    string actualaskedPath = askedPath;
    char sep = '/';
    size_t postlastsep = actualaskedPath.find_last_of(sep);
#endif

    if (postlastsep == 0) postlastsep++; // absolute paths
    string containingfolder = postlastsep == string::npos ? string() : actualaskedPath.substr(0, postlastsep);

    bool removeprefix = false;
    bool requiresseparatorafterunit = false;
    if (!containingfolder.size())
    {
        containingfolder = ".";
        removeprefix= true;
    }
#ifdef WIN32
    else if (containingfolder.find(":") == 1 && (containingfolder.size() < 3 || ( containingfolder.at(2) != '/' && containingfolder.at(2) != '\\')))
    {
        requiresseparatorafterunit = true;
    }
#endif

    std::error_code ec;
    fs::directory_iterator dirIt(fs::u8path(containingfolder), ec);
    if (ec)
    {
        // We need to check the error directly because the iterator
        // might not be end() in certain underlying OS errors
        return paths;
    }

    for (const auto& dirEntry : dirIt)
    {
        if (discardFiles && !dirEntry.is_directory(ec))
        {
            continue;
        }

#ifdef _WIN32
        wstring path = dirEntry.path().wstring();
#else
        string path = dirEntry.path().string();
#endif
        if (removeprefix)
        {
            path = path.substr(2);
        }
        if (requiresseparatorafterunit)
        {
            path.insert(2, 1, sep);
        }
        if (dirEntry.is_directory(ec))
        {
            path.append(1, sep);
        }

#ifdef _WIN32
        // try to mimic the exact startup of the asked path to allow mix of '\' & '/'
        fs::path paskedpath = fs::u8path(askedPath);
        paskedpath.make_preferred();
        wstring toreplace = paskedpath.wstring();
        if (path.find(toreplace) == 0)
        {
            replaceW(path, toreplace, toUtf16String(askedPath));
        }
        paths.push_back(toUtf8String(path));
#else
        paths.push_back(path);
#endif
    }

    return paths;
}

vector<string> MegaCmdExecuter::getlistusers()
{
    vector<string> users;

    MegaUserList* usersList = api->getContacts();
    if (usersList)
    {
        for (int i = 0; i < usersList->size(); i++)
        {
            users.push_back(usersList->get(i)->getEmail());
        }

        delete usersList;
    }
    return users;
}

vector<string> MegaCmdExecuter::getNodeAttrs(string nodePath)
{
    vector<string> attrs;

    std::unique_ptr<MegaNode> n = nodebypath(nodePath.c_str());
    if (n)
    {
        //List node custom attributes
        MegaStringList *attrlist = n->getCustomAttrNames();
        if (attrlist)
        {
            for (int a = 0; a < attrlist->size(); a++)
            {
                attrs.push_back(attrlist->get(a));
            }

            delete attrlist;
        }
    }
    return attrs;
}

vector<string> MegaCmdExecuter::getUserAttrs()
{
    vector<string> attrs;
    int i = 0;
    do
    {
        const char *catrn = api->userAttributeToString(i);
        if (strlen(catrn))
        {
            attrs.push_back(catrn);
        }
        else
        {
            delete [] catrn;
            break;
        }
        delete [] catrn;
        i++;
    } while (true);

    return attrs;
}

vector<string> MegaCmdExecuter::getsessions()
{
    vector<string> sessions;
    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
    api->getExtendedAccountDetails(true, true, true, megaCmdListener);
    int trywaitout = megaCmdListener->trywait(3000);
    if (trywaitout)
    {
        return sessions;
    }

    if (checkNoErrors(megaCmdListener->getError(), "get sessions"))
    {
        MegaAccountDetails *details = megaCmdListener->getRequest()->getMegaAccountDetails();
        if (details)
        {
            int numSessions = details->getNumSessions();
            for (int i = 0; i < numSessions; i++)
            {
                MegaAccountSession * session = details->getSession(i);
                if (session)
                {
                    if (session->isAlive())
                    {
                        std::unique_ptr<char []> handle {api->userHandleToBase64(session->getHandle())};
                        sessions.push_back(handle.get());
                    }
                    delete session;
                }
            }

            delete details;
        }
    }
    delete megaCmdListener;
    return sessions;
}

vector<string> MegaCmdExecuter::getlistfilesfolders(string location)
{
    vector<string> toret;
    for (fs::directory_iterator iter(fs::u8path(location)); iter != fs::directory_iterator(); ++iter)
    {
        toret.push_back(iter->path().filename().u8string());
    }
    return toret;
}

void MegaCmdExecuter::signup(string name, string passwd, string email)
{
    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);

    size_t spos = name.find(" ");
    string firstname = name.substr(0, spos);
    string lastname;
    if (spos != string::npos && ((spos + 1) < name.length()))
    {
        lastname = name.substr(spos+1);
    }

    if (lastname.empty())
    {
        setCurrentThreadOutCode(MCMD_INVALIDSTATE);
        LOG_err << "Please provide a valid name (with name and surname separated by \" \")";
        return;
    }

    OUTSTREAM << "Singing up. name=" << firstname << ". surname=" << lastname<< endl;

    api->createAccount(email.c_str(), passwd.c_str(), firstname.c_str(), lastname.c_str(), megaCmdListener);
    megaCmdListener->wait();
    if (checkNoErrors(megaCmdListener->getError(), "create account <" + email + ">"))
    {
        OUTSTREAM << "Account <" << email << "> created successfully. You will receive a confirmation link. Use \"confirm\" with the provided link to confirm that account" << endl;
    }

    delete megaCmdListener;

    MegaCmdListener *megaCmdListener2 = new MegaCmdListener(NULL);
    api->localLogout(megaCmdListener2);
    megaCmdListener2->wait();
    checkNoErrors(megaCmdListener2->getError(), "logging out from ephemeral account");
    delete megaCmdListener2;
}

void MegaCmdExecuter::signupWithPassword(string passwd)
{
    return signup(name, passwd, login);
}

void MegaCmdExecuter::confirm(string passwd, string email, string link)
{
    MegaCmdListener *megaCmdListener2 = new MegaCmdListener(NULL);
    api->confirmAccount(link.c_str(), passwd.c_str(), megaCmdListener2);
    megaCmdListener2->wait();
    if (megaCmdListener2->getError()->getErrorCode() == MegaError::API_ENOENT)
    {
        LOG_err << "Invalid password";
    }
    else if (checkNoErrors(megaCmdListener2->getError(), "confirm account"))
    {
        OUTSTREAM << "Account " << email << " confirmed successfully. You can login with it now" << endl;
    }

    delete megaCmdListener2;
}

void MegaCmdExecuter::confirmWithPassword(string passwd)
{
    return confirm(passwd, login, link);
}

bool MegaCmdExecuter::pathExists(const std::string &path)
{
    LocalPath localPath = LocalPath::fromAbsolutePath(path);
    std::unique_ptr<FileAccess> fa(mFsAccessCMD->newfileaccess());
    return fa->isfolder(localPath) || fa->isfile(localPath);
}

bool MegaCmdExecuter::IsFolder(string path)
{
#ifdef _WIN32
    replaceAll(path,"/","\\");
#endif
    LocalPath localpath = LocalPath::fromAbsolutePath(path);
    std::unique_ptr<FileAccess> fa = mFsAccessCMD->newfileaccess();
    return fa->isfolder(localpath);
}

void MegaCmdExecuter::printTransfersHeader(const unsigned int PATHSIZE, bool printstate)
{
    OUTSTREAM << "TYPE     TAG  " << getFixLengthString("SOURCEPATH ",PATHSIZE) << getFixLengthString("DESTINYPATH ",PATHSIZE)
              << "  " << getFixLengthString("    PROGRESS",21);
    if (printstate)
    {
        OUTSTREAM  << "  " << "STATE";
    }
    OUTSTREAM << endl;
}

void MegaCmdExecuter::printTransfer(MegaTransfer *transfer, const unsigned int PATHSIZE, bool printstate)
{
    //Direction
#ifdef _WIN32
    OUTSTREAM << " " << ((transfer->getType() == MegaTransfer::TYPE_DOWNLOAD)?"D":"U") << " ";
#else
    OUTSTREAM << " " << ((transfer->getType() == MegaTransfer::TYPE_DOWNLOAD)?"\u21d3":"\u21d1") << " ";
#endif
    //TODO: handle TYPE_LOCAL_TCP_DOWNLOAD

    //type (transfer/normal)
    if (transfer->isSyncTransfer())
    {
#ifdef _WIN32
        OUTSTREAM << "S";
#else
        OUTSTREAM << "\u21f5";
#endif
    }
    else if (transfer->isBackupTransfer())
    {
#ifdef _WIN32
        OUTSTREAM << "B";
#else
        OUTSTREAM << "\u23eb";
#endif
    }
    else
    {
        OUTSTREAM << " " ;
    }

    OUTSTREAM << " " ;

    //tag
    OUTSTREAM << getRightAlignedString(SSTR(transfer->getTag()),7) << " ";

    if (transfer->getType() == MegaTransfer::TYPE_DOWNLOAD)
    {
        // source
        MegaNode * node = api->getNodeByHandle(transfer->getNodeHandle());
        if (node)
        {
            char * nodepath = api->getNodePath(node);
            OUTSTREAM << getFixLengthString(nodepath,PATHSIZE);
            delete []nodepath;

            delete node;
        }
        else
        {
            globalTransferListener->completedTransfersMutex.lock();
            OUTSTREAM << getFixLengthString(globalTransferListener->completedPathsByHandle[transfer->getNodeHandle()],PATHSIZE);
            globalTransferListener->completedTransfersMutex.unlock();
        }

        OUTSTREAM << " ";

        //destination
        string dest = transfer->getParentPath() ? transfer->getParentPath() : "";
        dest.append(transfer->getFileName());
        OUTSTREAM << getFixLengthString(dest,PATHSIZE);
    }
    else
    {

        //source
        string source(transfer->getParentPath()?transfer->getParentPath():"");
        source.append(transfer->getFileName());

        OUTSTREAM << getFixLengthString(source, PATHSIZE);
        OUTSTREAM << " ";

        //destination
        MegaNode * parentNode = api->getNodeByHandle(transfer->getParentHandle());
        if (parentNode)
        {
            char * parentnodepath = api->getNodePath(parentNode);
            OUTSTREAM << getFixLengthString(parentnodepath ,PATHSIZE);
            delete []parentnodepath;

            delete parentNode;
        }
        else
        {
            OUTSTREAM << getFixLengthString("",PATHSIZE,'-');
            LOG_warn << "Could not find destination (parent handle "<< ((transfer->getParentHandle()==INVALID_HANDLE)?" invalid":" valid")
                     <<" ) for upload transfer. Source=" << transfer->getParentPath() << transfer->getFileName();
        }
    }

    //progress
    float percent;
    if (transfer->getTotalBytes() == 0)
    {
        percent = 0;
    }
    else
    {
        percent = float(transfer->getTransferredBytes()*1.0/transfer->getTotalBytes());
    }
    OUTSTREAM << "  " << getFixLengthString(percentageToText(percent),7,' ',true)
              << " of " << getFixLengthString(sizeToText(transfer->getTotalBytes()),10,' ',true);

    //state
    if (printstate)
    {
        OUTSTREAM << "  " << getTransferStateStr(transfer->getState());
    }

    OUTSTREAM << endl;
}

void MegaCmdExecuter::printTransferColumnDisplayer(ColumnDisplayer *cd, MegaTransfer *transfer, bool printstate)
{
    //Direction
    string type;
#ifdef _WIN32
    type += utf16ToUtf8((transfer->getType() == MegaTransfer::TYPE_DOWNLOAD)?L"\u25bc":L"\u25b2");
#else
    type += (transfer->getType() == MegaTransfer::TYPE_DOWNLOAD)?"\u21d3":"\u21d1";
#endif
    //TODO: handle TYPE_LOCAL_TCP_DOWNLOAD

    //type (transfer/normal)
    if (transfer->isSyncTransfer())
    {
#ifdef _WIN32
        type += utf16ToUtf8(L"\u21a8");
#else
        type += "\u21f5";
#endif
    }
    else if (transfer->isBackupTransfer())
    {
#ifdef _WIN32
        type += utf16ToUtf8(L"\u2191");
#else
        type += "\u23eb";
#endif
    }

    cd->addValue("TYPE",type);
    cd->addValue("TAG", SSTR(transfer->getTag())); //TODO: do SSTR within ColumnDisplayer

    if (transfer->getType() == MegaTransfer::TYPE_DOWNLOAD)
    {
        // source
        MegaNode * node = api->getNodeByHandle(transfer->getNodeHandle());
        if (node)
        {
            char * nodepath = api->getNodePath(node);
            cd->addValue("SOURCEPATH",nodepath);
            delete []nodepath;

            delete node;
        }
        else
        {
            globalTransferListener->completedTransfersMutex.lock();
            cd->addValue("SOURCEPATH",globalTransferListener->completedPathsByHandle[transfer->getNodeHandle()]);
            globalTransferListener->completedTransfersMutex.unlock();
        }

        //destination
        string dest = transfer->getParentPath() ? transfer->getParentPath() : "";
        dest.append(transfer->getFileName());
        cd->addValue("DESTINYPATH",dest);
    }
    else
    {
        //source
        string source(transfer->getParentPath()?transfer->getParentPath():"");
        source.append(transfer->getFileName());

        cd->addValue("SOURCEPATH",source);

        //destination
        MegaNode * parentNode = api->getNodeByHandle(transfer->getParentHandle());
        if (parentNode)
        {
            char * parentnodepath = api->getNodePath(parentNode);
            cd->addValue("DESTINYPATH",parentnodepath);
            delete []parentnodepath;

            delete parentNode;
        }
        else
        {
            cd->addValue("DESTINYPATH","---------");

            LOG_warn << "Could not find destination (parent handle "<< ((transfer->getParentHandle()==INVALID_HANDLE)?" invalid":" valid")
                     <<" ) for upload transfer. Source=" << transfer->getParentPath() << transfer->getFileName();
        }
    }

    //progress
    float percent;
    if (transfer->getTotalBytes() == 0)
    {
        percent = 0;
    }
    else
    {
        percent = float(transfer->getTransferredBytes()*1.0/transfer->getTotalBytes());
    }

    stringstream osspercent;
    osspercent << percentageToText(percent) << " of " << getFixLengthString(sizeToText(transfer->getTotalBytes()),10,' ',true);
    cd->addValue("PROGRESS",osspercent.str());

    //state
    if (printstate)
    {
        cd->addValue("STATE",getTransferStateStr(transfer->getState()));
    }
}

void MegaCmdExecuter::printBackupHeader(const unsigned int PATHSIZE)
{
    OUTSTREAM << "TAG  " << " ";
    OUTSTREAM << getFixLengthString("LOCALPATH ", PATHSIZE) << " ";
    OUTSTREAM << getFixLengthString("REMOTEPARENTPATH ", PATHSIZE) << " ";
    OUTSTREAM << getRightAlignedString("STATUS", 14);
    OUTSTREAM << endl;
}

void MegaCmdExecuter::printBackupSummary(int tag, const char * localfolder, const char *remoteparentfolder, string status, const unsigned int PATHSIZE)
{
    OUTSTREAM << getFixLengthString(SSTR(tag),5) << " "
              << getFixLengthString(localfolder, PATHSIZE) << " "
              << getFixLengthString((remoteparentfolder?remoteparentfolder:"INVALIDPATH"), PATHSIZE) << " "
              << getRightAlignedString(status, 14)
              << endl;
}

void MegaCmdExecuter::printBackupDetails(MegaScheduledCopy *backup, const char *timeFormat)
{
    if (backup)
    {
        string speriod = (backup->getPeriod() == -1)?backup->getPeriodString():getReadablePeriod(backup->getPeriod()/10);
        OUTSTREAM << "  Max Backups:   " << backup->getMaxBackups() << endl;
        OUTSTREAM << "  Period:         " << "\"" << speriod << "\"" << endl;
        OUTSTREAM << "  Next backup scheduled for: " << getReadableTime(backup->getNextStartTime(), timeFormat);

        OUTSTREAM << endl;
        OUTSTREAM << "  " << " -- CURRENT/LAST BACKUP --" << endl;
        OUTSTREAM << "  " << getFixLengthString("FILES UP/TOT", 15);
        OUTSTREAM << "  " << getFixLengthString("FOLDERS CREATED", 15);
        OUTSTREAM << "  " << getRightAlignedString("PROGRESS     ", 22);

        MegaTransferList * ml = backup->getFailedTransfers();
        if (ml && ml->size())
        {
            OUTSTREAM << "  " << getRightAlignedString("FAILED TRANSFERS", 17);
        }
        OUTSTREAM << endl;

        string sfiles = SSTR(backup->getNumberFiles()) + "/" + SSTR(backup->getTotalFiles());
        OUTSTREAM << "  " << getRightAlignedString(sfiles, 8) << "       ";
        OUTSTREAM << "  " << getRightAlignedString(SSTR(backup->getNumberFolders()), 8) << "       ";
        long long trabytes = backup->getTransferredBytes();
        long long totbytes = backup->getTotalBytes();
        double percent = totbytes?double(trabytes)/double(totbytes):0;

        string sprogress = sizeProgressToText(trabytes, totbytes) + "  " + percentageToText(float(percent));
        OUTSTREAM << "  " << getRightAlignedString(sprogress,22);
        if (ml && ml->size())
        {
            OUTSTREAM << getRightAlignedString(SSTR(backup->getFailedTransfers()->size()), 17);
        }
        delete ml;
        OUTSTREAM << endl;
    }
}

void MegaCmdExecuter::printBackupHistory(MegaScheduledCopy *backup, const char *timeFormat, MegaNode *parentnode, const unsigned int PATHSIZE)
{
    bool firstinhistory = true;
    MegaStringList *msl = api->getBackupFolders(backup->getTag());
    if (msl)
    {
        for (int i = 0; i < msl->size(); i++)
        {
            int datelength = int(getReadableTime(m_time(), timeFormat).size());

            if (firstinhistory)
            {
                OUTSTREAM << "  " << " -- HISTORY OF BACKUPS --" << endl;

                // print header
                OUTSTREAM << "  " << getFixLengthString("NAME", PATHSIZE) << " ";
                OUTSTREAM << getFixLengthString("DATE", datelength+1) << " ";
                OUTSTREAM << getRightAlignedString("STATUS", 11)<< " ";
                OUTSTREAM << getRightAlignedString("FILES", 6)<< " ";
                OUTSTREAM << getRightAlignedString("FOLDERS", 7);
                OUTSTREAM << endl;

                firstinhistory = false;
            }

            string bpath = msl->get(i);
            size_t pos = bpath.find("_bk_");
            string btime = "";
            if (pos != string::npos)
            {
                btime = bpath.substr(pos+4);
            }

            pos = bpath.find_last_of("/\\");
            string backupInstanceName = bpath;
            if (pos != string::npos)
            {
                backupInstanceName = bpath.substr(pos+1);
            }

            string printableDate = "UNKNOWN";
            if (btime.size())
            {
                struct tm dt;
                fillStructWithSYYmdHMS(btime,dt);
                printableDate = getReadableTime(m_mktime(&dt), timeFormat);
            }

            string backupInstanceStatus="NOT_FOUND";
            long long nfiles = 0;
            long long nfolders = 0;
            if (parentnode)
            {
                std::unique_ptr<MegaNode> backupInstanceNode = nodebypath(msl->get(i));
                if (backupInstanceNode)
                {
                    backupInstanceStatus = backupInstanceNode->getCustomAttr("BACKST");
                    auto listener = std::make_unique<SynchronousRequestListener>();
                    api->getFolderInfo(backupInstanceNode.get(), listener.get());
                    listener->wait();
                    if (checkNoErrors(listener->getError(), "get folder info"))
                    {
                        auto info = listener->getRequest()->getMegaFolderInfo();
                        nfiles += info->getNumFiles();
                        nfolders += info->getNumFolders();
                    }
                }
            }

            OUTSTREAM << "  " << getFixLengthString(backupInstanceName, PATHSIZE) << " ";
            OUTSTREAM << getFixLengthString(printableDate, datelength+1) << " ";
            OUTSTREAM << getRightAlignedString(backupInstanceStatus, 11) << " ";
            OUTSTREAM << getRightAlignedString(SSTR(nfiles), 6)<< " ";
            OUTSTREAM << getRightAlignedString(SSTR(nfolders), 7);
            //OUTSTREAM << getRightAlignedString("PROGRESS", 10);// some info regarding progress or the like in case of failure could be interesting. Although we don't know total files/folders/bytes
            OUTSTREAM << endl;

        }
        delete msl;
    }
}

void MegaCmdExecuter::printBackup(int tag, MegaScheduledCopy *backup, const char *timeFormat, const unsigned int PATHSIZE, bool extendedinfo, bool showhistory, MegaNode *parentnode)
{
    if (backup)
    {
        const char *nodepath = NULL;
        bool deleteparentnode = false;

        if (!parentnode)
        {
            parentnode = api->getNodeByHandle(backup->getMegaHandle());
            if (parentnode)
            {
                nodepath = api->getNodePath(parentnode);
                deleteparentnode = true;
            }
        }
        else
        {
            nodepath = api->getNodePath(parentnode);
        }

        printBackupSummary(tag, backup->getLocalFolder(),nodepath,backupSatetStr(backup->getState()), PATHSIZE);
        if (extendedinfo)
        {
            printBackupDetails(backup, timeFormat);
        }
        delete []nodepath;

        if (showhistory && parentnode)
        {
            printBackupHistory(backup, timeFormat, parentnode, PATHSIZE);
        }

        if (deleteparentnode)
        {
            delete parentnode;
        }
    }
    else
    {
        OUTSTREAM << "BACKUP not found " << endl;
    }
}

void MegaCmdExecuter::printBackup(backup_struct *backupstruct, const char *timeFormat, const unsigned int PATHSIZE, bool extendedinfo, bool showhistory)
{
    if (backupstruct->tag >= 0)
    {
        MegaScheduledCopy *backup = api->getScheduledCopyByTag(backupstruct->tag);
        if (backup)
        {
            printBackup(backupstruct->tag, backup, timeFormat, PATHSIZE, extendedinfo, showhistory);
            delete backup;
        }
        else
        {
            OUTSTREAM << "BACKUP not found: " << backupstruct->tag << endl;
        }
    }
    else
    { //merely print configuration
        printBackupSummary(backupstruct->tag, backupstruct->localpath.c_str(),"UNKNOWN"," FAILED", PATHSIZE);
        if (extendedinfo)
        {
            string speriod = (backupstruct->period == -1)?backupstruct->speriod:getReadablePeriod(backupstruct->period/10);
            OUTSTREAM << "         Period: " << "\"" << speriod << "\"" << endl;
            OUTSTREAM << "   Max. Backups: " << backupstruct->numBackups << endl;
        }
    }
}

void MegaCmdExecuter::doFind(MegaNode* nodeBase, const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, string word, int printfileinfo, string pattern, bool usepcre, m_time_t minTime, m_time_t maxTime, int64_t minSize, int64_t maxSize)
{
    struct criteriaNodeVector pnv;
    pnv.pattern = pattern;

    vector<MegaNode *> listOfMatches;
    pnv.nodesMatching = &listOfMatches;
    pnv.usepcre = usepcre;

    pnv.minTime = minTime;
    pnv.maxTime = maxTime;
    pnv.minSize = minSize;
    pnv.maxSize = maxSize;
    auto opt = getOption(cloptions, "type", "");
    pnv.mType = opt == "f" ? MegaNode::TYPE_FILE : (opt == "d" ? MegaNode::TYPE_FOLDER : MegaNode::TYPE_UNKNOWN);


    processTree(nodeBase, includeIfMatchesCriteria, (void*)&pnv);


    for (std::vector< MegaNode * >::iterator it = listOfMatches.begin(); it != listOfMatches.end(); ++it)
    {
        MegaNode * n = *it;
        if (n)
        {
            string pathToShow;

            if ( word.size() > 0 && ( (word.find("/") == 0) || (word.find("..") != string::npos)) )
            {
                char * nodepath = api->getNodePath(n);
                pathToShow = string(nodepath);
                delete [] nodepath;
            }
            else
            {
                pathToShow = getDisplayPath("", n);
            }
            if (getFlag(clflags, "print-only-handles"))
            {
                OUTSTREAM << "H:" << handleToBase64(n->getHandle()) << "" << endl;
            }
            else if (printfileinfo)
            {
                dumpNode(n, timeFormat, clflags, cloptions, 3, false, 1, pathToShow.c_str());
            }
            else
            {
                OUTSTREAM << pathToShow;

                if (getFlag(clflags, "show-handles"))
                {
                    OUTSTREAM << " <H:" << handleToBase64(n->getHandle()) << ">";
                }

                OUTSTREAM << endl;
            }
            //notice: some nodes may be dumped twice

            delete n;
        }
    }

    listOfMatches.clear();
}

string MegaCmdExecuter::getLPWD()
{
    string relativePath = ".";
    LocalPath localRelativePath = LocalPath::fromRelativePath(relativePath);
    LocalPath localAbsolutePath;
    if (!mFsAccessCMD->expanselocalpath(localRelativePath, localAbsolutePath))
    {
        LOG_err << " Unable to expanse local path . ";
        return "UNKNOWN";
    }
    LOG_verbose << "LocalPath localRelativePath = LocalPath::fromRelativePath(" << relativePath << "): " << localAbsolutePath.toPath(false);
    return localAbsolutePath.toPath(false);
}


void MegaCmdExecuter::moveToDestination(const std::unique_ptr<MegaNode>& n, string destiny)
{
    assert(n);

    char* nodepath = api->getNodePath(n.get());
    LOG_debug << "Moving : " << nodepath << " to " << destiny;
    delete []nodepath;

    string newname;
    std::unique_ptr<MegaNode> tn = nodebypath(destiny.c_str(), nullptr, &newname); // target node

    // we have four situations:
    // 1. target path does not exist - fail
    // 2. target node exists and is folder - move
    // 3. target node exists and is file - delete and rename (unless same)
    // 4. target path exists, but filename does not - rename
    if (tn)
    {
        if (tn->getHandle() == n->getHandle())
        {
            LOG_err << "Source and destiny are the same";
        }
        else
        {
            if (newname.size()) //target not found, but tn has what was before the last "/" in the path.
            {
                if (tn->getType() == MegaNode::TYPE_FILE)
                {
                    setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                    LOG_err << destiny << ": Not a directory";
                    return;
                }
                else //move and rename!
                {
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(nullptr);
                    api->moveNode(n.get(), tn.get(), megaCmdListener);
                    megaCmdListener->wait();

                    if (checkNoErrors(megaCmdListener->getError(), "move"))
                    {
                        MegaCmdListener *megaCmdListener = new MegaCmdListener(nullptr);
                        api->renameNode(n.get(), newname.c_str(), megaCmdListener);
                        megaCmdListener->wait();

                        checkNoErrors(megaCmdListener->getError(), "rename");
                        delete megaCmdListener;
                    }
                    else
                    {
                        LOG_debug << "Won't rename, since move failed " << n->getName() << " to " << tn->getName() << " : " << megaCmdListener->getError()->getErrorCode();
                    }
                    delete megaCmdListener;
                }
            }
            else //target found
            {
                if (tn->getType() == MegaNode::TYPE_FILE) //move & remove old & rename new
                {
                    // (there should never be any orphaned filenodes)
                    std::unique_ptr<MegaNode> tnParentNode(api->getNodeByHandle(tn->getParentHandle()));
                    if (tnParentNode)
                    {
                        //move into the parent of target node
                        MegaCmdListener *megaCmdListener = new MegaCmdListener(nullptr);
                        std::unique_ptr<MegaNode> parentNode(api->getNodeByHandle(tn->getParentHandle()));
                        api->moveNode(n.get(), parentNode.get(), megaCmdListener);
                        megaCmdListener->wait();

                        if (checkNoErrors(megaCmdListener->getError(), "move node"))
                        {
                            const char* name_to_replace = tn->getName();

                            //remove (replaced) target node
                            if (n.get() != tn.get()) //just in case moving to same location
                            {
                                MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                                api->remove(tn.get(), megaCmdListener); //remove target node
                                megaCmdListener->wait();

                                if (!checkNoErrors(megaCmdListener->getError(), "remove target node"))
                                {
                                    LOG_err << "Couldnt move " << n->getName() << " to " << tn->getName() << " : " << megaCmdListener->getError()->getErrorCode();
                                }
                                delete megaCmdListener;
                            }

                            // rename moved node with the new name
                            if (strcmp(name_to_replace, n->getName()))
                            {
                                MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                                api->renameNode(n.get(), name_to_replace, megaCmdListener);
                                megaCmdListener->wait();

                                if (!checkNoErrors(megaCmdListener->getError(), "rename moved node"))
                                {
                                    LOG_err << "Failed to rename moved node: " << megaCmdListener->getError()->getErrorString();
                                }
                                delete megaCmdListener;
                            }
                        }
                        delete megaCmdListener;
                    }
                    else
                    {
                        setCurrentThreadOutCode(MCMD_INVALIDSTATE);
                        LOG_fatal << "Destiny node is orphan!!!";
                    }
                }
                else // target is a folder
                {
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(nullptr);
                    api->moveNode(n.get(), tn.get(), megaCmdListener);
                    megaCmdListener->wait();

                    checkNoErrors(megaCmdListener->getError(), "move node");
                    delete megaCmdListener;
                }
            }
        }
    }
    else //target not found (not even its folder), cant move
    {
        setCurrentThreadOutCode(MCMD_NOTFOUND);
        LOG_err << destiny << ": No such directory";
    }
}


bool MegaCmdExecuter::isValidFolder(string destiny)
{
    bool isdestinyavalidfolder = true;
    std::unique_ptr<MegaNode> ndestiny = nodebypath(destiny.c_str());;
    if (ndestiny)
    {
        if (ndestiny->getType() == MegaNode::TYPE_FILE)
        {
            isdestinyavalidfolder = false;
        }
    }
    else
    {
        isdestinyavalidfolder = false;
    }
    return isdestinyavalidfolder;
}

std::string MegaCmdExecuter::getNodePathString(MegaNode *n)
{
    const char *path = api->getNodePath(n);
    string toret(path);
    delete[] path;
    return toret;
}

void MegaCmdExecuter::copyNode(MegaNode *n, string destiny, MegaNode * tn, string &targetuser, string &newname)
{
    if (tn)
    {
        if (tn->getHandle() == n->getHandle())
        {
            LOG_err << "Source and destiny are the same";
        }
        else
        {
            if (newname.size()) //target not found, but tn has what was before the last "/" in the path.
            {
                if (n->getType() == MegaNode::TYPE_FILE)
                {
                    LOG_debug << "copy with new name: \"" << getNodePathString(n) << "\" to \"" << destiny << "\" newname=" << newname;
                    //copy with new name
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                    api->copyNode(n, tn, newname.c_str(), megaCmdListener); //only works for files
                    megaCmdListener->wait();
                    checkNoErrors(megaCmdListener->getError(), "copy node");
                    delete megaCmdListener;
                }
                else //copy & rename
                {
                    LOG_debug << "copy & rename: \"" << getNodePathString(n) << "\" to \"" << destiny << "\"";
                    //copy with new name
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                    api->copyNode(n, tn, megaCmdListener);
                    megaCmdListener->wait();
                    if (checkNoErrors(megaCmdListener->getError(), "copy node"))
                    {
                        MegaNode * newNode = api->getNodeByHandle(megaCmdListener->getRequest()->getNodeHandle());
                        if (newNode)
                        {
                            MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                            api->renameNode(newNode, newname.c_str(), megaCmdListener);
                            megaCmdListener->wait();
                            checkNoErrors(megaCmdListener->getError(), "rename new node");
                            delete megaCmdListener;
                            delete newNode;
                        }
                        else
                        {
                            LOG_err << " Couldn't find new node created upon cp";
                        }
                    }
                    delete megaCmdListener;
                }
            }
            else
            { //target exists
                if (tn->getType() == MegaNode::TYPE_FILE)
                {
                    if (n->getType() == MegaNode::TYPE_FILE)
                    {
                        LOG_debug << "overwriding target: \"" << getNodePathString(n) << "\" to \"" << destiny << "\"";

                        // overwrite target if source and target are files
                        MegaNode *tnParentNode = api->getNodeByHandle(tn->getParentHandle());
                        if (tnParentNode) // (there should never be any orphaned filenodes)
                        {
                            const char* name_to_replace = tn->getName();
                            //copy with new name
                            MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                            api->copyNode(n, tnParentNode, name_to_replace, megaCmdListener);
                            megaCmdListener->wait();
                            if (checkNoErrors(megaCmdListener->getError(), "copy with new name") )
                            {
                                MegaHandle newNodeHandle = megaCmdListener->getRequest()->getNodeHandle();
                                delete megaCmdListener;
                                delete tnParentNode;

                                //remove target node
                                if (tn->getHandle() != newNodeHandle )//&& newNodeHandle != n->getHandle())
                                {
                                    megaCmdListener = new MegaCmdListener(NULL);
                                    api->remove(tn, megaCmdListener);
                                    megaCmdListener->wait();
                                    checkNoErrors(megaCmdListener->getError(), "delete target node");
                                    delete megaCmdListener;
                                }
                            }
                        }
                        else
                        {
                            setCurrentThreadOutCode(MCMD_INVALIDSTATE);
                            LOG_fatal << "Destiny node is orphan!!!";
                        }
                    }
                    else
                    {
                        setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                        LOG_err << "Cannot overwrite file with folder";
                        return;
                    }
                }
                else //copying into folder
                {
                    LOG_debug << "Copying into folder: \"" << getNodePathString(n) << "\" to \"" << destiny << "\"";

                    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                    api->copyNode(n, tn, megaCmdListener);
                    megaCmdListener->wait();
                    checkNoErrors(megaCmdListener->getError(), "copy node");
                    delete megaCmdListener;
                }
            }
        }
    }
    else if (targetuser.size())
    {
        LOG_debug << "Sending to user: \"" << getNodePathString(n) << "\" to \"" << targetuser << "\"";

        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
        api->sendFileToUser(n,targetuser.c_str(),megaCmdListener);
        megaCmdListener->wait();
        checkNoErrors(megaCmdListener->getError(), "send file to user");
        delete megaCmdListener;
    }
    else
    {
        setCurrentThreadOutCode(MCMD_NOTFOUND);
        LOG_err << destiny << " Couldn't find destination";
    }
}

bool MegaCmdExecuter::establishBackup(string pathToBackup, MegaNode *n, int64_t period, string speriod,  int numBackups)
{
    bool attendpastbackups = true; //TODO: receive as parameter
    static int backupcounter = 0;
    LocalPath localAbsolutePath = LocalPath::fromAbsolutePath(pathToBackup); //this one would converts it to absolute if it's relative
    LocalPath expansedAbsolutePath;
    if (!mFsAccessCMD->expanselocalpath(localAbsolutePath, expansedAbsolutePath))
    {
        setCurrentThreadOutCode(MCMD_NOTFOUND);
        LOG_err << " Failed to expanse path";
    }

    auto megaCmdListener = std::make_unique<MegaCmdListener>(api, nullptr);
    api->setScheduledCopy(expansedAbsolutePath.toPath(false).c_str(), n, attendpastbackups, period, speriod.c_str(), numBackups, megaCmdListener.get());
    megaCmdListener->wait();
    if (checkNoErrors(megaCmdListener->getError(), "establish backup"))
    {
        backup_struct *thebackup = nullptr;
        bool sendBackupEvent = false;
        {
            std::lock_guard g(mtxBackupsMap);
            if (ConfigurationManager::configuredBackups.find(megaCmdListener->getRequest()->getFile()) != ConfigurationManager::configuredBackups.end())
            {
                thebackup = ConfigurationManager::configuredBackups[megaCmdListener->getRequest()->getFile()];
                if (thebackup->id == -1)
                {
                    thebackup->id = backupcounter++;
                }
            }
            else
            {
                thebackup = new backup_struct;
                thebackup->id = backupcounter++;
                ConfigurationManager::configuredBackups[megaCmdListener->getRequest()->getFile()] = thebackup;
                sendBackupEvent = true;
            }
            thebackup->active = true;
            thebackup->handle = megaCmdListener->getRequest()->getNodeHandle();
            thebackup->localpath = string(megaCmdListener->getRequest()->getFile());
            thebackup->numBackups = numBackups;
            thebackup->period = period;
            thebackup->speriod = speriod;
            thebackup->failed = false;
            thebackup->tag = megaCmdListener->getRequest()->getTransferTag();
        }

        if (sendBackupEvent)
        {
            auto wasFirstBackupConfiguredOpt = ConfigurationManager::savePropertyValue("firstBackupConfigured", true);
            if (!wasFirstBackupConfiguredOpt || !*wasFirstBackupConfiguredOpt)
            {
                sendEvent(StatsManager::MegacmdEvent::FIRST_CONFIGURED_SCHEDULED_BACKUP, api, false);
            }
            else
            {
                sendEvent(StatsManager::MegacmdEvent::SUBSEQUENT_CONFIGURED_SCHEDULED_BACKUP, api, false);
            }
        }

        std::unique_ptr<char[]> nodepath(api->getNodePath(n));
        LOG_info << "Added backup: " << megaCmdListener->getRequest()->getFile() << " to " << nodepath;
        return true;
    }
    else
    {
        bool foundbytag = false;
        // find by tag in configured (modification failed)
        for (std::map<std::string, backup_struct *>::iterator itr = ConfigurationManager::configuredBackups.begin();
             itr != ConfigurationManager::configuredBackups.end(); itr++)
        {
            if (itr->second->tag == megaCmdListener->getRequest()->getTransferTag())
            {
                backup_struct *thebackup = itr->second;

                foundbytag = true;
                thebackup->handle = megaCmdListener->getRequest()->getNodeHandle();
                thebackup->localpath = string(megaCmdListener->getRequest()->getFile());
                thebackup->numBackups = megaCmdListener->getRequest()->getNumRetry();
                thebackup->period = megaCmdListener->getRequest()->getNumber();
                thebackup->speriod = string(megaCmdListener->getRequest()->getText());;
                thebackup->failed = true;
            }
        }


        if (!foundbytag)
        {
            std::map<std::string, backup_struct *>::iterator itr = ConfigurationManager::configuredBackups.find(megaCmdListener->getRequest()->getFile());
            if ( itr != ConfigurationManager::configuredBackups.end())
            {
                if (megaCmdListener->getError()->getErrorCode() != MegaError::API_EEXIST)
                {
                    itr->second->failed = true;
                }
                itr->second->id = backupcounter++;
            }
        }
    }
    return false;
}

void MegaCmdExecuter::confirmCancel(const char* confirmlink, const char* pass)
{
    auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
    api->confirmCancelAccount(confirmlink, pass, megaCmdListener.get());
    megaCmdListener->wait();

    if (megaCmdListener->getError()->getErrorCode() == MegaError::API_ETOOMANY)
    {
        LOG_err << "Confirm cancel account failed: too many attempts";
    }
    else if (megaCmdListener->getError()->getErrorCode() == MegaError::API_ENOENT ||
             megaCmdListener->getError()->getErrorCode() == MegaError::API_EKEY)
    {
        LOG_err << "Confirm cancel account failed: invalid link/password";
    }
    // We consider ESID (bad session ID) as successful because of a data race in the SDK
    else if (megaCmdListener->getError()->getErrorCode() == MegaError::API_ESID ||
             checkNoErrors(megaCmdListener->getError(), "confirm cancel account"))
    {
        OUTSTREAM << "CONFIRM Account cancelled successfully" << endl;

        megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
        api->localLogout(megaCmdListener.get());
        actUponLogout(megaCmdListener.get(), false);
    }
}

void MegaCmdExecuter::processPath(string path, bool usepcre, bool& firstone, void (*nodeprocessor)(MegaCmdExecuter *, MegaNode *, bool), MegaCmdExecuter *context)
{

    if (isRegExp(path))
    {
        vector<std::unique_ptr<MegaNode>> nodes = nodesbypath(path.c_str(), usepcre);
        if (nodes.empty())
        {
            setCurrentThreadOutCode(MCMD_NOTFOUND);
            LOG_err << "Path not found: " << path;
        }

        for (const auto& n : nodes)
        {
            assert(n);
            nodeprocessor(context, n.get(), firstone);
            firstone = false;
        }
    }
    else // non-regexp
    {
        std::unique_ptr<MegaNode> n = nodebypath(path.c_str());
        if (n)
        {
            nodeprocessor(context, n.get(), firstone);
            firstone = false;
        }
        else
        {
            setCurrentThreadOutCode(MCMD_NOTFOUND);
            LOG_err << "Path not found: " << path;
        }
    }
}


#ifdef HAVE_LIBUV

void forwarderRemoveWebdavLocation(MegaCmdExecuter* context, MegaNode *n, bool firstone) {
    context->removeWebdavLocation(n, firstone);
}
void forwarderAddWebdavLocation(MegaCmdExecuter* context, MegaNode *n, bool firstone) {
    context->addWebdavLocation(n, firstone);
}
void forwarderRemoveFtpLocation(MegaCmdExecuter* context, MegaNode *n, bool firstone) {
    context->removeFtpLocation(n, firstone);
}
void forwarderAddFtpLocation(MegaCmdExecuter* context, MegaNode *n, bool firstone) {
    context->addFtpLocation(n, firstone);
}

void MegaCmdExecuter::removeWebdavLocation(MegaNode *n, bool firstone, string name)
{
    char *actualNodePath = api->getNodePath(n);
    api->httpServerRemoveWebDavAllowedNode(n->getHandle());

    mtxWebDavLocations.lock();
    list<string> servedpaths = ConfigurationManager::getConfigurationValueList("webdav_served_locations");
    size_t sizeprior = servedpaths.size();
    servedpaths.remove(actualNodePath);
    size_t sizeafter = servedpaths.size();
    if (!sizeafter)
    {
        api->httpServerStop();
        ConfigurationManager::savePropertyValue("webdav_port", -1); //so as not to load server on startup
    }
    ConfigurationManager::savePropertyValueList("webdav_served_locations", servedpaths);
    mtxWebDavLocations.unlock();

    if (sizeprior != sizeafter)
    {
        OUTSTREAM << (name.size()?name:actualNodePath) << " no longer served via webdav" << endl;
    }
    else
    {
        setCurrentThreadOutCode(MCMD_NOTFOUND);
        LOG_err << (name.size()?name:actualNodePath) << " is not served via webdav";
    }
    delete []actualNodePath;
}

void MegaCmdExecuter::addWebdavLocation(MegaNode *n, bool firstone, string name)
{
    std::unique_ptr<char[]> actualNodePath(api->getNodePath(n));
    std::unique_ptr<char[]> l(api->httpServerGetLocalWebDavLink(n));

    OUTSTREAM << "Serving via webdav " << (name.size() ? name : actualNodePath.get()) << ": " << l.get() << endl;

    {
        std::lock_guard g(mtxWebDavLocations);
        list<string> servedpaths = ConfigurationManager::getConfigurationValueList("webdav_served_locations");

        if (!ConfigurationManager::getConfigurationValue("firstWebDavConfigured", false))
        {
            sendEvent(StatsManager::MegacmdEvent::FIRST_CONFIGURED_WEBDAV, api, false);
            ConfigurationManager::savePropertyValue("firstWebDavConfigured", true);
        }
        else if (std::find(servedpaths.begin(), servedpaths.end(), actualNodePath.get()) == servedpaths.end())
        {
            // Send event only if not already on the list
            sendEvent(StatsManager::MegacmdEvent::SUBSEQUENT_CONFIGURED_WEBDAV, api, false);
        }

        servedpaths.push_back(actualNodePath.get());
        servedpaths.sort();
        servedpaths.unique();
        ConfigurationManager::savePropertyValueList("webdav_served_locations", servedpaths);
    }
}

void MegaCmdExecuter::removeFtpLocation(MegaNode *n, bool firstone, string name)
{
    char *actualNodePath = api->getNodePath(n);
    api->ftpServerRemoveAllowedNode(n->getHandle());

    mtxFtpLocations.lock();
    list<string> servedpaths = ConfigurationManager::getConfigurationValueList("ftp_served_locations");
    size_t sizeprior = servedpaths.size();
    servedpaths.remove(actualNodePath);
    size_t sizeafter = servedpaths.size();
    if (!sizeafter)
    {
        api->ftpServerStop();
        ConfigurationManager::savePropertyValue("ftp_port", -1); //so as not to load server on startup
    }
    ConfigurationManager::savePropertyValueList("ftp_served_locations", servedpaths);
    mtxFtpLocations.unlock();

    if (sizeprior != sizeafter)
    {
        OUTSTREAM << (name.size()?name:actualNodePath) << " no longer served via ftp" << endl;
    }
    else
    {
        setCurrentThreadOutCode(MCMD_NOTFOUND);
        LOG_err << (name.size()?name:actualNodePath)  << " is not served via ftp";
    }
    delete []actualNodePath;
}

void MegaCmdExecuter::addFtpLocation(MegaNode *n, bool firstone, string name)
{
    std::unique_ptr<char[]> actualNodePath(api->getNodePath(n));
    std::unique_ptr<char[]> l(api->ftpServerGetLocalLink(n));

    OUTSTREAM << "Serving via ftp " << (name.size() ? name : n->getName())  << ": " << l.get() << endl;

    {
        std::lock_guard g(mtxFtpLocations);
        list<string> servedpaths = ConfigurationManager::getConfigurationValueList("ftp_served_locations");

        if (!ConfigurationManager::getConfigurationValue("firstFtpConfigured", false))
        {
            sendEvent(StatsManager::MegacmdEvent::FIRST_CONFIGURED_FTP, api, false);
            ConfigurationManager::savePropertyValue("firstFtpConfigured", true);
        }
        else if (std::find(servedpaths.begin(), servedpaths.end(), actualNodePath.get()) == servedpaths.end())
        {
            // Send event only if not already on the list
            sendEvent(StatsManager::MegacmdEvent::SUBSEQUENT_CONFIGURED_FTP, api, false);
        }

        servedpaths.push_back(actualNodePath.get());
        servedpaths.sort();
        servedpaths.unique();
        ConfigurationManager::savePropertyValueList("ftp_served_locations", servedpaths);
    }
}

#endif


void MegaCmdExecuter::catFile(MegaNode *n)
{
    if (n->getType() != MegaNode::TYPE_FILE)
    {
        LOG_err << " Unable to cat: not a file";
        setCurrentThreadOutCode(MCMD_EARGS);
        return;
    }

    long long nsize = api->getSize(n);
    if (!nsize)
    {
        return;
    }
    long long end = nsize;
    long long start = 0;

    MegaCmdCatTransferListener *mcctl = new MegaCmdCatTransferListener(&OUTSTREAM, api, sandboxCMD);
    api->startStreaming(n, start, end-start, mcctl);
    mcctl->wait();
    if (checkNoErrors(mcctl->getError(), "cat streaming from " +SSTR(start) + " to " + SSTR(end) ))
    {
        char * npath = api->getNodePath(n);
        LOG_verbose << "Streamed: " << npath << " from " << start << " to " << end;
        delete []npath;
    }

    delete mcctl;
}

void MegaCmdExecuter::printInfoFile(MegaNode *n, bool &firstone, int PATHSIZE)
{
    char * nodepath = api->getNodePath(n);
    char *fattrs = n->getFileAttrString();
    if (fattrs == NULL)
    {
        LOG_warn << " Unable to get attributes for node " << nodepath;
    }

    if (firstone)
    {
        OUTSTREAM << getFixLengthString("FILE", PATHSIZE);
        OUTSTREAM << getFixLengthString("WIDTH", 7);
        OUTSTREAM << getFixLengthString("HEIGHT", 7);
        OUTSTREAM << getFixLengthString("FPS", 4);
        OUTSTREAM << getFixLengthString("PLAYTIME", 10);
        OUTSTREAM << endl;
        firstone = false;
    }

    OUTSTREAM << getFixLengthString(nodepath, PATHSIZE-1) << " ";
    delete []nodepath;

    OUTSTREAM << getFixLengthString( (n->getWidth() == -1) ? "---" : SSTR(n->getWidth()) , 6) << " ";
    OUTSTREAM << getFixLengthString( (n->getHeight() == -1) ? "---" : SSTR(n->getHeight()) , 6) << " ";

    if (fattrs == NULL)
    {
        OUTSTREAM << getFixLengthString("---", 3) << " ";
    }
    else
    {
        MediaProperties mp = MediaProperties::decodeMediaPropertiesAttributes(fattrs, (uint32_t*)(n->getNodeKey()->data() + FILENODEKEYLENGTH / 2) );
        OUTSTREAM << getFixLengthString( (mp.fps == 0) ? "---" : SSTR(mp.fps) , 3) << " ";
    }
    OUTSTREAM << getFixLengthString( (n->getHeight() == -1) ? "---" : getReadablePeriod(n->getDuration()) , 10) << " ";

    OUTSTREAM << endl;
}

bool MegaCmdExecuter::printUserAttribute(int a, string user, bool onlylist)
{
    const char *catrn = api->userAttributeToString(a);
    string attrname = catrn;
    delete [] catrn;

    const char *catrln = api->userAttributeToLongName(a);
    string longname = catrln;
    delete [] catrln;

    if (attrname.size())
    {
        if (onlylist)
        {
            OUTSTREAM << longname << " (" << attrname << ")" << endl;
        }
        else
        {
            MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
            if (user.size())
            {
                api->getUserAttribute(user.c_str(), a, megaCmdListener);
            }
            else
            {
                api->getUserAttribute(a, megaCmdListener);
            }
            megaCmdListener->wait();
            if (checkNoErrors(megaCmdListener->getError(), string("get user attribute ") + attrname))
            {
                //int iattr = int(megaCmdListener->getRequest()->getParamType());
                const char *value = megaCmdListener->getRequest()->getText();
                //if (!value) value = megaCmdListener->getRequest()->getMegaStringMap()->;
                string svalue;
                try
                {
                    if (value)
                    {
                        svalue = string(value);
                    }
                    else
                    {

                        svalue = "NOT PRINTABLE";
                    }

                }
                catch (exception e)
                {
                    svalue = "NOT PRINTABLE";
                }
                OUTSTREAM << "\t" << longname << " (" << attrname << ")" << " = " << svalue << endl;
            }

            delete megaCmdListener;
        }
        return true;
    }
    return false;
}

bool MegaCmdExecuter::setProxy(const std::string &url, const std::string &username, const std::string &password, int proxyType)
{
    MegaProxy mpx;

    if (url.size())
    {
        mpx.setProxyURL(url.c_str());
    }

    if (username.size())
    {
        mpx.setCredentials(username.c_str(), password.c_str());
    }

    mpx.setProxyType(proxyType);

    std::vector<MegaApi *> megaapis;
    //TODO: add apiFolders to that list
    megaapis.push_back(api);
    bool failed = false;
    for (auto api: megaapis)
    {
        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
        api->setProxySettings(&mpx, megaCmdListener);
        megaCmdListener->wait();
        if (checkNoErrors(megaCmdListener->getError(), "(un)setting proxy"))
        {
            //TODO: connectivity check! // review connectivity check branch
        }
        else
        {
            failed = true;
        }
        delete megaCmdListener;
    }

    if (!failed)
    {
        ConfigurationManager::savePropertyValue("proxy_url", mpx.getProxyURL()?mpx.getProxyURL():"");
        ConfigurationManager::savePropertyValue("proxy_type", mpx.getProxyType());

        ConfigurationManager::savePropertyValue("proxy_username", mpx.getUsername()?mpx.getUsername():"");
        ConfigurationManager::savePropertyValue("proxy_password", mpx.getPassword()?mpx.getPassword():"");

        if (mpx.getProxyType() == MegaProxy::PROXY_NONE)
        {
            OUTSTREAM << "Proxy unset correctly" << endl ;
        }
        else
        {
            OUTSTREAM << "Proxy set: " << (mpx.getProxyURL()?mpx.getProxyURL():"")
                      << " type = " << getProxyTypeStr(mpx.getProxyType()) << endl;

            broadcastMessage(string("Using proxy: ").append((mpx.getProxyURL()?mpx.getProxyURL():""))
                             .append(" type = ").append(getProxyTypeStr(mpx.getProxyType())), true);
        }
    }
    else
    {
        LOG_err << "Unable to configure proxy";
        broadcastMessage("Unable to configure proxy", true);
    }

    return !failed;
}

bool checkAtLeastNArgs(const vector<string> &words, size_t n)
{
    if (words.size() < n)
    {
        setCurrentThreadOutCode(MCMD_EARGS);
        assert(!words.empty());
        LOG_err << words[0] << ": Invalid number of arguments";
        LOG_err << "Usage: " << getUsageStr(words[0].c_str());
        return false;
    }
    return true;
}

bool checkExactlyNArgs(const vector<string> &words, size_t n)
{
    if (words.size() != n)
    {
        setCurrentThreadOutCode(MCMD_EARGS);
        assert(!words.empty());
        LOG_err << words[0] << ": Invalid number of arguments";
        LOG_err << "Usage: " << getUsageStr(words[0].c_str());
        return false;
    }
    return true;
}

void MegaCmdExecuter::executecommand(vector<string> words, map<string, int> *clflags, map<string, string> *cloptions)
{
    if (words[0] == "ls")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        int recursive = getFlag(clflags, "R") + getFlag(clflags, "r");
        int extended_info = getFlag(clflags, "a");
        int show_versions = getFlag(clflags, "versions");
        bool summary = getFlag(clflags, "l");
        bool firstprint = true;
        bool humanreadable = getFlag(clflags, "h");
        bool treelike = getFlag(clflags,"tree");
        recursive += treelike?1:0;

        if ((int)words.size() > 1)
        {
            unescapeifRequired(words[1]);

            string rNpath = "NULL";
            if (words[1].find('/') != string::npos)
            {
                string cwpath = getCurrentPath();
                if (words[1].find(cwpath) == string::npos)
                {
                    rNpath = "";
                }
                else
                {
                    rNpath = cwpath;
                }
            }

            if (isRegExp(words[1]))
            {
                vector<string> *pathsToList = nodesPathsbypath(words[1].c_str(), getFlag(clflags,"use-pcre"));
                if (pathsToList && pathsToList->size())
                {
                    for (std::vector< string >::iterator it = pathsToList->begin(); it != pathsToList->end(); ++it)
                    {
                        string nodepath= *it;
                        MegaNode *ncwd = api->getNodeByHandle(cwd);
                        if (ncwd)
                        {
                            std::unique_ptr<MegaNode> n = nodebypath(nodepath.c_str());
                            if (n)
                            {
                                if (!n->getType() == MegaNode::TYPE_FILE)
                                {
                                    OUTSTREAM << nodepath << ": " << endl;
                                }
                                if (summary)
                                {
                                    if (firstprint)
                                    {
                                        dumpNodeSummaryHeader(getTimeFormatFromSTR(getOption(cloptions, "time-format","SHORT")), clflags, cloptions);
                                        firstprint = false;
                                    }
                                    dumpTreeSummary(n.get(), getTimeFormatFromSTR(getOption(cloptions, "time-format","SHORT")), clflags, cloptions, recursive, show_versions, 0, humanreadable, rNpath);
                                }
                                else
                                {
                                    vector<bool> lfs;
                                    dumptree(n.get(), treelike, lfs, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, recursive, extended_info, show_versions, 0, rNpath);
                                }
                                if ((!n->getType() == MegaNode::TYPE_FILE ) && ((it + 1) != pathsToList->end()))
                                {
                                    OUTSTREAM << endl;
                                }
                            }
                            else
                            {
                                LOG_debug << "Unexpected: matching path has no associated node: " << nodepath << ". Could have been deleted in the process";
                            }
                            delete ncwd;
                        }
                        else
                        {
                            setCurrentThreadOutCode(MCMD_INVALIDSTATE);
                            LOG_err << "Couldn't find woking folder (it might been deleted)";
                        }
                    }
                    pathsToList->clear();
                    delete pathsToList;
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << "Couldn't find \"" << words[1] << "\"";
                }
            }
            else
            {
                std::unique_ptr<MegaNode> n = nodebypath(words[1].c_str());
                if (n)
                {
                    if (summary)
                    {
                        if (firstprint)
                        {
                            dumpNodeSummaryHeader(
                                getTimeFormatFromSTR(getOption(cloptions, "time-format", "SHORT")),
                                clflags, cloptions);
                            firstprint = false;
                        }
                        dumpTreeSummary(
                            n.get(),
                            getTimeFormatFromSTR(getOption(cloptions, "time-format", "SHORT")),
                            clflags, cloptions, recursive, show_versions,
                            0, humanreadable, rNpath);
                    }
                    else
                    {
                        if (treelike) OUTSTREAM << words[1] << endl;
                        vector<bool> lfs;
                        dumptree(n.get(), treelike, lfs, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, recursive, extended_info, show_versions, 0, rNpath);
                    }
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << "Couldn't find " << words[1];
                }
            }
        }
        else
        {
            std::unique_ptr<MegaNode> n(api->getNodeByHandle(cwd));
            if (n)
            {
                if (summary)
                {
                    if (firstprint)
                    {
                        dumpNodeSummaryHeader(
                            getTimeFormatFromSTR(getOption(cloptions, "time-format", "SHORT")),
                            clflags, cloptions);
                        firstprint = false;
                    }
                    dumpTreeSummary(n.get(), getTimeFormatFromSTR(getOption(cloptions, "time-format", "SHORT")), clflags, cloptions, recursive, show_versions, 0,
                                    humanreadable, "NULL");
                }
                else
                {
                    if (treelike) OUTSTREAM << "." << endl;
                    vector<bool> lfs;
                    dumptree(n.get(), treelike, lfs, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, recursive, extended_info, show_versions);
                }
            }
        }
        return;
    }
    else if (words[0] == "find")
    {
        string pattern = getOption(cloptions, "pattern", "*");
        int printfileinfo = getFlag(clflags,"l");

        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }

        m_time_t minTime = -1;
        m_time_t maxTime = -1;
        string mtimestring = getOption(cloptions, "mtime", "");
        if ("" != mtimestring && !getMinAndMaxTime(mtimestring, &minTime, &maxTime))
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Invalid time " << mtimestring;
            return;
        }

        int64_t minSize = -1;
        int64_t maxSize = -1;
        string sizestring = getOption(cloptions, "size", "");
        if ("" != sizestring && !getMinAndMaxSize(sizestring, &minSize, &maxSize))
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Invalid time " << sizestring;
            return;
        }


        if (words.size() <= 1)
        {
            std::unique_ptr<MegaNode> n(api->getNodeByHandle(cwd));
            doFind(n.get(), getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, "", printfileinfo, pattern, getFlag(clflags,"use-pcre"), minTime, maxTime, minSize, maxSize);
        }
        for (int i = 1; i < (int)words.size(); i++)
        {
            if (isRegExp(words[i]))
            {
                vector<std::unique_ptr<MegaNode>> nodesToFind = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                if (nodesToFind.size())
                {
                    for (const auto& node : nodesToFind)
                    {
                        assert(node);
                        doFind(node.get(), getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, words[i], printfileinfo, pattern, getFlag(clflags,"use-pcre"), minTime, maxTime, minSize, maxSize);
                    }
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << words[i] << ": No such file or directory";
                }
            }
            else
            {
                std::unique_ptr<MegaNode> n = nodebypath(words[i].c_str());
                if (!n)
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << "Couldn't find " << words[i];
                }
                else
                {
                    doFind(n.get(), getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, words[i], printfileinfo, pattern, getFlag(clflags,"use-pcre"), minTime, maxTime, minSize, maxSize);
                }
            }
        }
    }
#if defined(_WIN32) || defined(__APPLE__)
    else if (words[0] == "update")
    {
        string sauto = getOption(cloptions, "auto", "");
        transform(sauto.begin(), sauto.end(), sauto.begin(), [](char c) { return char(::tolower(c)); });

        if (sauto == "off")
        {
            stopcheckingForUpdates();
            OUTSTREAM << "Automatic updates disabled" << endl;
        }
        else if (sauto == "on")
        {
            startcheckingForUpdates();
            OUTSTREAM << "Automatic updates enabled" << endl;
        }
        else if (sauto == "query")
        {
            OUTSTREAM << "Automatic updates " << (ConfigurationManager::getConfigurationValue("autoupdate", false)?"enabled":"disabled") << endl;
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("update");
        }

        return;
    }
#endif
    else if (words[0] == "cd")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 1)
        {
            std::unique_ptr<MegaNode> n = nodebypath(words[1].c_str());
            if (n)
            {
                if (n->getType() == MegaNode::TYPE_FILE)
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << words[1] << ": Not a directory";
                }
                else
                {
                    cwd = n->getHandle();
                    updateprompt(api);
                }
            }
            else
            {
                setCurrentThreadOutCode(MCMD_NOTFOUND);
                LOG_err << words[1] << ": No such file or directory";
            }
        }
        else
        {
            MegaNode * rootNode = api->getRootNode();
            if (!rootNode)
            {
                LOG_err << "nodes not fetched";
                setCurrentThreadOutCode(MCMD_NOFETCH);
                delete rootNode;
                return;
            }
            cwd = rootNode->getHandle();
            updateprompt(api);

            delete rootNode;
        }

        return;
    }
    else if (words[0] == "rm")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 1)
        {
            if (isCurrentThreadInteractive())
            {
                //clear all previous nodes to confirm delete (could have been not cleared in case of ctrl+c)
                mNodesToConfirmDelete.clear();
            }

            bool force = getFlag(clflags, "f");
            bool none = false;

            for (unsigned int i = 1; i < words.size(); i++)
            {
                unescapeifRequired(words[i]);
                if (isRegExp(words[i]))
                {
                    vector<std::unique_ptr<MegaNode>> nodesToDelete = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                    if (nodesToDelete.empty())
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        LOG_err << words[i] << ": No such file or directory";
                    }

                    for (const auto& node : nodesToDelete)
                    {
                        assert(node);

                        int confirmationCode = deleteNode(node, api, getFlag(clflags, "r"), force);
                        if (confirmationCode == MCMDCONFIRM_ALL)
                        {
                            force = true;
                        }
                        else if (confirmationCode == MCMDCONFIRM_NONE)
                        {
                            none = true;
                        }
                    }
                }
                else if (!none)
                {
                    std::unique_ptr<MegaNode> nodeToDelete = nodebypath(words[i].c_str());
                    if (!nodeToDelete)
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        LOG_err << words[i] << ": No such file or directory";
                    }
                    else
                    {
                        int confirmationCode = deleteNode(nodeToDelete, api, getFlag(clflags, "r"), force);
                        if (confirmationCode == MCMDCONFIRM_ALL)
                        {
                            force = true;
                        }
                        else if (confirmationCode == MCMDCONFIRM_NONE)
                        {
                            none = true;
                        }
                    }
                }
            }
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("rm");
        }

        return;
    }
    else if (words[0] == "mv")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 2)
        {
            string destiny = words[words.size()-1];
            unescapeifRequired(destiny);

            if (words.size() > 3 && !isValidFolder(destiny))
            {
                setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                LOG_err << destiny << " must be a valid folder";
                return;
            }

            for (unsigned int i=1;i<(words.size()-1);i++)
            {
                string source = words[i];
                unescapeifRequired(source);

                if (isRegExp(source))
                {
                    vector<std::unique_ptr<MegaNode>> nodesToList = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                    if (nodesToList.empty())
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        LOG_err << source << ": No such file or directory";
                    }

                    bool destinyisok = true;
                    if (nodesToList.size() > 1 && !isValidFolder(destiny))
                    {
                        destinyisok = false;
                        setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                        LOG_err << destiny << " must be a valid folder";
                    }

                    if (destinyisok)
                    {
                        for (const auto& node : nodesToList)
                        {
                            assert(node);
                            moveToDestination(node, destiny);
                        }
                    }
                }
                else
                {
                    std::unique_ptr<MegaNode> n = nodebypath(source.c_str());
                    if (n)
                    {
                        moveToDestination(n, destiny);
                    }
                    else
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        LOG_err << source << ": No such file or directory";
                    }
                }
            }

        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("mv");
        }

        return;
    }
    else if (words[0] == "cp")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 2)
        {
            string destiny = words[words.size()-1];
            string targetuser;
            string newname;
            std::unique_ptr<MegaNode> tn = nodebypath(destiny.c_str(), &targetuser, &newname);

            if (words.size() > 3 && !isValidFolder(destiny) && !targetuser.size())
            {
                setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                LOG_err << destiny << " must be a valid folder";
                return;
            }

            for (unsigned int i=1;i<(words.size()-1);i++)
            {
                string source = words[i];

                if (isRegExp(source))
                {
                    vector<std::unique_ptr<MegaNode>> nodesToCopy = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                    if (nodesToCopy.empty())
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        LOG_err << source << ": No such file or directory";
                    }

                    bool destinyisok = true;
                    if (nodesToCopy.size() > 1 && !isValidFolder(destiny) && !targetuser.size())
                    {
                        destinyisok = false;
                        setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                        LOG_err << destiny << " must be a valid folder";
                    }

                    if (destinyisok)
                    {
                        for (const auto& n : nodesToCopy)
                        {
                            assert(n);
                            copyNode(n.get(), destiny, tn.get(), targetuser, newname);
                        }
                    }
                }
                else
                {
                    std::unique_ptr<MegaNode> n = nodebypath(source.c_str());
                    if (n)
                    {
                        copyNode(n.get(), destiny, tn.get(), targetuser, newname);
                    }
                    else
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        LOG_err << source << ": No such file or directory";
                    }
                }
            }
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("cp");
        }

        return;
    }
    else if (words[0] == "du")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }

        int PATHSIZE = getintOption(cloptions,"path-display-size");
        if (!PATHSIZE)
        {
            // get screen size for output purposes
            unsigned int width = getNumberOfCols(75);
            PATHSIZE = min(50,int(width-22));
        }
        PATHSIZE = max(0, PATHSIZE);

        long long totalSize = 0;
        long long currentSize = 0;
        long long totalVersionsSize = 0;
        string dpath;
        if (words.size() == 1)
        {
            words.push_back(".");
        }

        bool humanreadable = getFlag(clflags, "h");
        bool show_versions_size = getFlag(clflags, "versions");
        bool firstone = true;

        for (unsigned int i = 1; i < words.size(); i++)
        {
            unescapeifRequired(words[i]);
            if (isRegExp(words[i]))
            {
                vector<std::unique_ptr<MegaNode>> nodesToList = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                for (const auto& n : nodesToList)
                {
                    assert(n);

                    if (firstone)//print header
                    {
                        OUTSTREAM << getFixLengthString("FILENAME", PATHSIZE) << getFixLengthString("SIZE", 12, ' ', true);
                        if (show_versions_size)
                        {
                            OUTSTREAM << getFixLengthString("S.WITH VERS", 12, ' ', true);;
                        }
                        OUTSTREAM << endl;
                        firstone = false;
                    }
                    currentSize = api->getSize(n.get());
                    totalSize += currentSize;

                    dpath = getDisplayPath(words[i], n.get());
                    OUTSTREAM << getFixLengthString(dpath+":",PATHSIZE) << getFixLengthString(sizeToText(currentSize, true, humanreadable), 12, ' ', true);
                    if (show_versions_size)
                    {
                        long long sizeWithVersions = getVersionsSize(n.get());
                        OUTSTREAM << getFixLengthString(sizeToText(sizeWithVersions, true, humanreadable), 12, ' ', true);
                        totalVersionsSize += sizeWithVersions;
                    }

                    OUTSTREAM << endl;
                }
            }
            else
            {
                std::unique_ptr<MegaNode> n = nodebypath(words[i].c_str());
                if (!n)
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << words[i] << ": No such file or directory";
                    return;
                }

                currentSize = api->getSize(n.get());
                totalSize += currentSize;
                dpath = getDisplayPath(words[i], n.get());
                if (dpath.size())
                {
                    if (firstone)//print header
                    {
                        OUTSTREAM << getFixLengthString("FILENAME",PATHSIZE) << getFixLengthString("SIZE", 12, ' ', true);
                        if (show_versions_size)
                        {
                            OUTSTREAM << getFixLengthString("S.WITH VERS", 12, ' ', true);;
                        }
                        OUTSTREAM << endl;
                        firstone = false;
                    }

                    OUTSTREAM << getFixLengthString(dpath+":",PATHSIZE) << getFixLengthString(sizeToText(currentSize, true, humanreadable), 12, ' ', true);
                    if (show_versions_size)
                    {
                        long long sizeWithVersions = getVersionsSize(n.get());
                        OUTSTREAM << getFixLengthString(sizeToText(sizeWithVersions, true, humanreadable), 12, ' ', true);
                        totalVersionsSize += sizeWithVersions;
                    }
                    OUTSTREAM << endl;

                }
            }
        }

        if (!firstone)
        {
            for (int i = 0; i < PATHSIZE+12 +(show_versions_size?12:0) ; i++)
            {
                OUTSTREAM << "-";
            }
            OUTSTREAM << endl;

            OUTSTREAM << getFixLengthString("Total storage used:",PATHSIZE) << getFixLengthString(sizeToText(totalSize, true, humanreadable), 12, ' ', true);
            //OUTSTREAM << "Total storage used: " << setw(22) << sizeToText(totalSize, true, humanreadable);
            if (show_versions_size)
            {
                OUTSTREAM << getFixLengthString(sizeToText(totalVersionsSize, true, humanreadable), 12, ' ', true);
            }
            OUTSTREAM << endl;
        }
        return;
    }
    else if (words[0] == "cat")
    {
        if (words.size() < 2)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("cat");
            return;
        }

        for (int i = 1; i < (int)words.size(); i++)
        {
            if (isPublicLink(words[i]))
            {
                string publicLink = words[i];
                if (!decryptLinkIfEncrypted(api, publicLink, cloptions))
                {
                    return;
                }

                if (getLinkType(publicLink) == MegaNode::TYPE_FILE)
                {
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                    api->getPublicNode(publicLink.c_str(), megaCmdListener);
                    megaCmdListener->wait();

                    if (!checkNoErrors(megaCmdListener->getError(), "cat public node"))
                    {
                        if (megaCmdListener->getError()->getErrorCode() == MegaError::API_EARGS)
                        {
                            LOG_err << "The link provided might be incorrect: " << publicLink.c_str();
                        }
                        else if (megaCmdListener->getError()->getErrorCode() == MegaError::API_EINCOMPLETE)
                        {
                            LOG_err << "The key is missing or wrong " << publicLink.c_str();
                        }
                    }
                    else
                    {
                        if (megaCmdListener->getRequest()->getFlag())
                        {
                            LOG_err << "Key not valid " << publicLink.c_str();
                            setCurrentThreadOutCode(MCMD_EARGS);
                        }
                        else
                        {
                            MegaNode *n = megaCmdListener->getRequest()->getPublicMegaNode();
                            if (n)
                            {
                                catFile(n);
                                delete n;
                            }
                        }

                    }
                    delete megaCmdListener;
                }
                else //TODO: detect if referenced file within public link and in that case, do login and cat it
                {
                    LOG_err << "Public link is not a file";
                    setCurrentThreadOutCode(MCMD_EARGS);
                }
            }
            else if (!api->isFilesystemAvailable())
            {
                setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
                LOG_err << "Unable to cat " << words[i] << ": Not logged in.";
            }
            else
            {
                unescapeifRequired(words[i]);
                if (isRegExp(words[i]))
                {
                    vector<std::unique_ptr<MegaNode>> nodes = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                    if (nodes.empty())
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        LOG_err << "Nodes not found: " << words[i];
                    }

                    for (const auto& n : nodes)
                    {
                        assert(n);
                        catFile(n.get());
                    }
                }
                else
                {
                    std::unique_ptr<MegaNode> n = nodebypath(words[i].c_str());
                    if (n)
                    {
                        catFile(n.get());
                    }
                    else
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        LOG_err << "Node not found: " << words[i];
                    }
                }
            }
        }
    }
    else if (words[0] == "mediainfo")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() < 2)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("mediainfo");
            return;
        }

        int PATHSIZE = getintOption(cloptions,"path-display-size");
        if (!PATHSIZE)
        {
            // get screen size for output purposes
            unsigned int width = getNumberOfCols(75);
            PATHSIZE = min(50,int(width-28));
        }
        PATHSIZE = max(0, PATHSIZE);

        bool firstone = true;
        for (int i = 1; i < (int)words.size(); i++)
        {
            unescapeifRequired(words[i]);
            if (isRegExp(words[i]))
            {
                vector<std::unique_ptr<MegaNode>> nodes = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                if (nodes.empty())
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << "Nodes not found: " << words[i];
                }

                for (const auto& n : nodes)
                {
                    assert(n);
                    printInfoFile(n.get(), firstone, PATHSIZE);
                }
            }
            else
            {
                std::unique_ptr<MegaNode> n = nodebypath(words[i].c_str());
                if (n)
                {
                    printInfoFile(n.get(), firstone, PATHSIZE);
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << "Node not found: " << words[i];
                }
            }
        }
    }
    else if (words[0] == "get")
    {
        bool background = getFlag(clflags,"q");

        int clientID = getintOption(cloptions, "clientID", -1);
        if (words.size() > 1 && words.size() < 4)
        {
            string path = "./";
            if (background)
            {
                clientID = -1;
            }

            auto megaCmdMultiTransferListener = std::make_shared<MegaCmdMultiTransferListener>(api, sandboxCMD, nullptr, clientID);

            bool ignorequotawarn = getFlag(clflags,"ignore-quota-warn");
            bool destinyIsFolder = false;

            if (isPublicLink(words[1]))
            {
                string publicLink = words[1];
                if (!decryptLinkIfEncrypted(api, publicLink, cloptions))
                {
                    return;
                }

                if (getLinkType(publicLink) == MegaNode::TYPE_FILE)
                {
                    if (words.size() > 2)
                    {
                        path = words[2];
                        destinyIsFolder = IsFolder(path);
                        if (destinyIsFolder)
                        {
                            if (! (path.find_last_of("/") == path.size()-1) && ! (path.find_last_of("\\") == path.size()-1))
                            {
#ifdef _WIN32
                                path+="\\";
#else
                                path+="/";
#endif
                            }
                            if (!canWrite(path))
                            {
                                setCurrentThreadOutCode(MCMD_NOTPERMITTED);
                                LOG_err << "Write not allowed in " << path;
                                return;
                            }
                        }
                        else
                        {
                            if (!TestCanWriteOnContainingFolder(path))
                            {
                                return;
                            }
                        }
                    }
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                    api->getPublicNode(publicLink.c_str(), megaCmdListener);
                    megaCmdListener->wait();

                    if (!megaCmdListener->getError())
                    {
                        LOG_fatal << "No error in listener at get public node";
                    }
                    else if (!checkNoErrors(megaCmdListener->getError(), "get public node"))
                    {
                        if (megaCmdListener->getError()->getErrorCode() == MegaError::API_EARGS)
                        {
                            LOG_err << "The link provided might be incorrect: " << publicLink.c_str();
                        }
                        else if (megaCmdListener->getError()->getErrorCode() == MegaError::API_EINCOMPLETE)
                        {
                            LOG_err << "The key is missing or wrong " << publicLink.c_str();
                        }
                    }
                    else
                    {
                        if (megaCmdListener->getRequest() && megaCmdListener->getRequest()->getFlag())
                        {
                            LOG_err << "Key not valid " << publicLink.c_str();
                        }
                        if (megaCmdListener->getRequest())
                        {
                            if (destinyIsFolder && getFlag(clflags,"m"))
                            {
                                while( (path.find_last_of("/") == path.size()-1) || (path.find_last_of("\\") == path.size()-1))
                                {
                                    path=path.substr(0,path.size()-1);
                                }
                            }
                            MegaNode *n = megaCmdListener->getRequest()->getPublicMegaNode();
                            downloadNode(words[1], path, api, n, background, ignorequotawarn, clientID, megaCmdMultiTransferListener);
                            delete n;
                        }
                        else
                        {
                            LOG_err << "Empty Request at get";
                        }
                    }
                    delete megaCmdListener;
                }
                else if (getLinkType(publicLink) == MegaNode::TYPE_FOLDER)
                {
                    if (words.size() > 2)
                    {
                        path = words[2];
                        destinyIsFolder = IsFolder(path);
                        if (destinyIsFolder)
                        {
                            if (! (path.find_last_of("/") == path.size()-1) && ! (path.find_last_of("\\") == path.size()-1))
                            {
#ifdef _WIN32
                                path+="\\";
#else
                                path+="/";
#endif
                            }
                            if (!canWrite(words[2]))
                            {
                                setCurrentThreadOutCode(MCMD_NOTPERMITTED);
                                LOG_err << "Write not allowed in " << words[2];
                                return;
                            }
                        }
                        else
                        {
                            setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                            LOG_err << words[2] << " is not a valid Download Folder";
                            return;
                        }
                    }

                    MegaApi* apiFolder = getFreeApiFolder();
                    if (!apiFolder)
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        LOG_err << "No available Api folder. Use configure to increase exported_folders_sdks";
                        return;
                    }
                    char *accountAuth = api->getAccountAuth();
                    apiFolder->setAccountAuth(accountAuth);
                    delete []accountAuth;

                    MegaCmdListener *megaCmdListener = new MegaCmdListener(apiFolder, NULL);
                    apiFolder->loginToFolder(publicLink.c_str(), megaCmdListener);
                    megaCmdListener->wait();
                    if (checkNoErrors(megaCmdListener->getError(), "login to folder"))
                    {
                        MegaCmdListener *megaCmdListener2 = new MegaCmdListener(apiFolder, NULL);
                        apiFolder->fetchNodes(megaCmdListener2);
                        megaCmdListener2->wait();
                        if (checkNoErrors(megaCmdListener2->getError(), "access folder link " + publicLink))
                        {
                            MegaNode *nodeToDownload = NULL;
                            bool usedRoot = false;
                            string shandle = getPublicLinkHandle(publicLink);
                            if (shandle.size())
                            {
                                handle thehandle = apiFolder->base64ToHandle(shandle.c_str());
                                nodeToDownload = apiFolder->getNodeByHandle(thehandle);
                            }
                            else
                            {
                                nodeToDownload = apiFolder->getRootNode();
                                usedRoot = true;
                            }

                            if (nodeToDownload)
                            {
                                if (destinyIsFolder && getFlag(clflags,"m"))
                                {
                                    while( (path.find_last_of("/") == path.size()-1) || (path.find_last_of("\\") == path.size()-1))
                                    {
                                        path=path.substr(0,path.size()-1);
                                    }
                                }
                                MegaNode *authorizedNode = apiFolder->authorizeNode(nodeToDownload);
                                if (authorizedNode != NULL)
                                {
                                    downloadNode(words[1], path, api, authorizedNode, background, ignorequotawarn, clientID, megaCmdMultiTransferListener);
                                    delete authorizedNode;
                                }
                                else
                                {
                                    LOG_debug << "Node couldn't be authorized: " << publicLink << ". Downloading as non-loged user";
                                    downloadNode(words[1], path, apiFolder, nodeToDownload, background, ignorequotawarn, clientID, megaCmdMultiTransferListener);
                                }
                                delete nodeToDownload;
                            }
                            else
                            {
                                setCurrentThreadOutCode(MCMD_INVALIDSTATE);
                                if (usedRoot)
                                {
                                    LOG_err << "Couldn't get root folder for folder link";
                                }
                                else
                                {
                                    LOG_err << "Failed to get node corresponding to handle within public link " << shandle;
                                }
                            }
                        }
                        delete megaCmdListener2;

                        {
                            std::unique_ptr<MegaCmdListener> megaCmdListener(new MegaCmdListener(apiFolder));
                            apiFolder->logout(false, megaCmdListener.get());
                            megaCmdListener->wait();
                            if (megaCmdListener->getError()->getErrorCode() != MegaError::API_OK)
                            {
                                LOG_err << "Couldn't logout from apiFolder";
                            }
                        }
                    }
                    delete megaCmdListener;
                    freeApiFolder(apiFolder);
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                    LOG_err << "Invalid link: " << publicLink;
                }
            }
            else //remote file
            {
                if (!api->isFilesystemAvailable())
                {
                    setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
                    LOG_err << "Not logged in.";
                    return;
                }
                unescapeifRequired(words[1]);

                if (isRegExp(words[1]))
                {
                    vector<std::unique_ptr<MegaNode>> nodesToGet = nodesbypath(words[1].c_str(), getFlag(clflags,"use-pcre"));
                    if (words.size() > 2)
                    {
                        path = words[2];
                        destinyIsFolder = IsFolder(path);
                        if (destinyIsFolder)
                        {
                            if (! (path.find_last_of("/") == path.size()-1) && ! (path.find_last_of("\\") == path.size()-1))
                            {
#ifdef _WIN32
                                path+="\\";
#else
                                path+="/";
#endif
                            }
                            if (!canWrite(words[2]))
                            {
                                setCurrentThreadOutCode(MCMD_NOTPERMITTED);
                                LOG_err << "Write not allowed in " << words[2];
                                return;
                            }
                        }
                        else if (nodesToGet.size() > 1) //several files into one file!
                        {
                            setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                            LOG_err << words[2] << " is not a valid Download Folder";
                            return;
                        }
                        else //destiny non existing or a file
                        {
                            if (!TestCanWriteOnContainingFolder(path))
                            {
                                return;
                            }
                        }
                    }

                    if (destinyIsFolder && getFlag(clflags,"m"))
                    {
                        while( (path.find_last_of("/") == path.size()-1) || (path.find_last_of("\\") == path.size()-1))
                        {
                            path=path.substr(0,path.size()-1);
                        }
                    }

                    for (const auto& n : nodesToGet)
                    {
                        assert(n);
                        downloadNode(words[1], path, api, n.get(), background, ignorequotawarn, clientID, megaCmdMultiTransferListener);
                    }

                    if (nodesToGet.empty())
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        LOG_err << "Couldn't find " << words[1];
                    }

                }
                else //not regexp
                {
                    std::unique_ptr<MegaNode> n = nodebypath(words[1].c_str());
                    if (n)
                    {
                        if (words.size() > 2)
                        {
                            if (n->getType() == MegaNode::TYPE_FILE)
                            {
                                path = words[2];
                                destinyIsFolder = IsFolder(path);
                                if (destinyIsFolder)
                                {
                                    if (! (path.find_last_of("/") == path.size()-1) && ! (path.find_last_of("\\") == path.size()-1))
                                    {
#ifdef _WIN32
                                        path+="\\";
#else
                                        path+="/";
#endif
                                    }
                                    if (!canWrite(words[2]))
                                    {
                                        setCurrentThreadOutCode(MCMD_NOTPERMITTED);
                                        LOG_err << "Write not allowed in " << words[2];
                                        return;
                                    }
                                }
                                else
                                {
                                    if (!TestCanWriteOnContainingFolder(path))
                                    {
                                        return;
                                    }
                                }
                            }
                            else
                            {
                                path = words[2];
                                destinyIsFolder = IsFolder(path);
                                if (destinyIsFolder)
                                {
                                    if (! (path.find_last_of("/") == path.size()-1) && ! (path.find_last_of("\\") == path.size()-1))
                                    {
#ifdef _WIN32
                                        path+="\\";
#else
                                        path+="/";
#endif
                                    }
                                    if (!canWrite(words[2]))
                                    {
                                        setCurrentThreadOutCode(MCMD_NOTPERMITTED);
                                        LOG_err << "Write not allowed in " << words[2];
                                        return;
                                    }
                                }
                                else
                                {
                                    setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                                    LOG_err << words[2] << " is not a valid Download Folder";
                                    return;
                                }
                            }
                        }
                        if (destinyIsFolder && getFlag(clflags,"m"))
                        {
                            while( (path.find_last_of("/") == path.size()-1) || (path.find_last_of("\\") == path.size()-1))
                            {
                                path=path.substr(0,path.size()-1);
                            }
                        }
                        downloadNode(words[1], path, api, n.get(), background, ignorequotawarn, clientID, megaCmdMultiTransferListener);
                    }
                    else
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        LOG_err << "Couldn't find file";
                    }
                }
            }

            if (!background)
            {
                megaCmdMultiTransferListener->waitMultiEnd();

                if (megaCmdMultiTransferListener->getFinalerror() != MegaError::API_OK)
                {
                    setCurrentThreadOutCode(megaCmdMultiTransferListener->getFinalerror());
                    LOG_err << "Download failed. error code: " << MegaError::getErrorString(megaCmdMultiTransferListener->getFinalerror());
                }

                if (megaCmdMultiTransferListener->getProgressinformed() || getCurrentThreadOutCode() == MCMD_OK )
                {
                    informProgressUpdate(PROGRESS_COMPLETE, megaCmdMultiTransferListener->getTotalbytes(), clientID);
                }
            }

        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("get");
        }

        return;
    }
    else if (words[0] == "backup")
    {
        bool dodelete = getFlag(clflags,"d");
        bool abort = getFlag(clflags,"a");
        bool listinfo = getFlag(clflags,"l");
        bool listhistory = getFlag(clflags,"h");

//        //TODO: do the following functionality
//        bool stop = getFlag(clflags,"s");
//        bool resume = getFlag(clflags,"r");
//        bool initiatenow = getFlag(clflags,"i");

        int PATHSIZE = getintOption(cloptions,"path-display-size");
        if (!PATHSIZE)
        {
            // get screen size for output purposes
            unsigned int width = getNumberOfCols(75);
            PATHSIZE = min(60,int((width-46)/2));
        }
        PATHSIZE = max(0, PATHSIZE);

        bool firstbackup = true;
        string speriod=getOption(cloptions, "period");
        int numBackups = int(getintOption(cloptions, "num-backups", -1));

        if (words.size() == 3)
        {
            string local = words.at(1);
            string remote = words.at(2);
            unescapeifRequired(local);
            unescapeifRequired(remote);

            createOrModifyBackup(local, remote, speriod, numBackups);
        }
        else if (words.size() == 2)
        {
            string local = words.at(1);
            unescapeifRequired(local);

            MegaScheduledCopy *backup = api->getScheduledCopyByPath(local.c_str());
            if (!backup)
            {
                backup = api->getScheduledCopyByTag(toInteger(local, -1));
            }
            map<string, backup_struct *>::iterator itr;
            if (backup)
            {
                int backupid = -1;
                for ( itr = ConfigurationManager::configuredBackups.begin(); itr != ConfigurationManager::configuredBackups.end(); itr++ )
                {
                    if (itr->second->tag == backup->getTag())
                    {
                        backupid = itr->second->id;
                        break;
                    }
                }
                if (backupid == -1)
                {
                    LOG_err << " Requesting info of unregistered backup: " << local;
                }

                if (dodelete)
                {
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);
                    api->removeScheduledCopy(backup->getTag(), megaCmdListener);
                    megaCmdListener->wait();
                    if (checkNoErrors(megaCmdListener->getError(), "remove backup"))
                    {
                        if (backupid != -1)
                        {
                          ConfigurationManager::configuredBackups.erase(itr);
                        }
                        mtxBackupsMap.lock();
                        ConfigurationManager::saveBackups(&ConfigurationManager::configuredBackups);
                        mtxBackupsMap.unlock();
                        OUTSTREAM << " Backup removed succesffuly: " << local << endl;
                    }
                }
                else if (abort)
                {
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);
                    api->abortCurrentScheduledCopy(backup->getTag(), megaCmdListener);
                    megaCmdListener->wait();
                    if (checkNoErrors(megaCmdListener->getError(), "abort backup"))
                    {
                        OUTSTREAM << " Backup aborted succesffuly: " << local << endl;
                    }
                }
                else
                {
                    if (speriod.size() || numBackups != -1)
                    {
                        createOrModifyBackup(backup->getLocalFolder(), "", speriod, numBackups);
                    }
                    else
                    {
                        if(firstbackup)
                        {
                            printBackupHeader(PATHSIZE);
                            firstbackup = false;
                        }

                        printBackup(backup->getTag(), backup, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), PATHSIZE, listinfo, listhistory);
                    }
                }
                delete backup;
            }
            else
            {
                if (dodelete) //remove from configured backups
                {
                    bool deletedok = false;
                    for ( itr = ConfigurationManager::configuredBackups.begin(); itr != ConfigurationManager::configuredBackups.end(); itr++ )
                    {
                        if (itr->second->tag == -1 && itr->second->localpath == local)
                        {
                            mtxBackupsMap.lock();
                            ConfigurationManager::configuredBackups.erase(itr);
                            ConfigurationManager::saveBackups(&ConfigurationManager::configuredBackups);
                            mtxBackupsMap.unlock();
                            OUTSTREAM << " Backup removed succesffuly: " << local << endl;
                            deletedok = true;

                            break;
                        }
                    }

                    if (!deletedok)
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        OUTSTREAM << "Backup not found: " << local << endl;
                    }
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << "Backup not found: " << local;
                }
            }
        }
        else if (words.size() == 1) //list backups
        {
            mtxBackupsMap.lock();
            for (map<string, backup_struct *>::iterator itr = ConfigurationManager::configuredBackups.begin(); itr != ConfigurationManager::configuredBackups.end(); itr++ )
            {
                if(firstbackup)
                {
                    printBackupHeader(PATHSIZE);
                    firstbackup = false;
                }
                printBackup(itr->second, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), PATHSIZE, listinfo, listhistory);
            }
            if (!ConfigurationManager::configuredBackups.size())
            {
                setCurrentThreadOutCode(MCMD_NOTFOUND);
                OUTSTREAM << "No backup configured. " << endl << " Usage: " << getUsageStr("backup") << endl;
            }
            mtxBackupsMap.unlock();

        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("backup");
        }
    }
    else if (words[0] == "put")
    {
        int clientID = getintOption(cloptions, "clientID", -1);

        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }

        bool background = getFlag(clflags,"q");
        if (background)
        {
            clientID = -1;
        }

        MegaCmdMultiTransferListener *megaCmdMultiTransferListener = new MegaCmdMultiTransferListener(api, sandboxCMD, NULL, clientID);

        bool ignorequotawarn = getFlag(clflags,"ignore-quota-warn");

        if (words.size() > 1)
        {
            string targetuser;
            string newname = "";
            string destination = "";

            std::unique_ptr<MegaNode> n;

            if (words.size() > 2)
            {
                destination = words[words.size() - 1];
                n = nodebypath(destination.c_str(), &targetuser, &newname);

                if (!n && getFlag(clflags,"c"))
                {
                    string destinationfolder(destination,0,destination.find_last_of("/"));
                    newname=string(destination,destination.find_last_of("/")+1,destination.size());
                    MegaNode *cwdNode = api->getNodeByHandle(cwd);
                    makedir(destinationfolder,true,cwdNode);
                    n = nodebypath(destinationfolder.c_str());
                    delete cwdNode;
                }
            }
            else
            {
                n.reset(api->getNodeByHandle(cwd));
                words.push_back(".");
            }

            if (n)
            {
                if (n->getType() != MegaNode::TYPE_FILE)
                {
                    for (int i = 1; i < max(1, (int)words.size() - 1); i++)
                    {
                        if (words[i] == ".")
                        {
                            words[i] = getLPWD();
                        }

#ifdef HAVE_GLOB_H
                        if (!newname.size() && !fs::exists(words[i]) && hasWildCards(words[i]))
                        {
                            auto paths = resolvewildcard(words[i]);
                            if (!paths.size())
                            {
                                setCurrentThreadOutCode(MCMD_NOTFOUND);
                                LOG_err << words[i] << " not found";
                            }
                            for (auto path : paths)
                            {
                                uploadNode(path, api, n.get(), newname, background, ignorequotawarn, clientID, megaCmdMultiTransferListener);
                            }
                        }
                        else
#endif
                        {
                            uploadNode(words[i], api, n.get(), newname, background, ignorequotawarn, clientID, megaCmdMultiTransferListener);
                        }
                    }
                }
                else if (words.size() == 3 && !IsFolder(words[1])) //replace file
                {
                    unique_ptr<MegaNode> pn(api->getNodeByHandle(n->getParentHandle()));
                    if (pn)
                    {
#ifdef HAVE_GLOB_H
                        if (!fs::exists(words[1]) && hasWildCards(words[1]))
                        {
                            LOG_err << "Invalid target for wildcard expression: " << words[1] << ". Folder expected";
                            setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                        }
                        else
#endif
                        {
                            uploadNode(words[1], api, pn.get(), n->getName(), background, ignorequotawarn, clientID, megaCmdMultiTransferListener);
                        }
                    }
                    else
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        LOG_err << "Destination is not valid. Parent folder cannot be found";
                    }
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                    LOG_err << "Destination is not valid (expected folder or alike)";
                }

                megaCmdMultiTransferListener->waitMultiEnd();

                checkNoErrors(megaCmdMultiTransferListener->getFinalerror(), "upload");

                if (megaCmdMultiTransferListener->getProgressinformed() || getCurrentThreadOutCode() == MCMD_OK )
                {
                    informProgressUpdate(PROGRESS_COMPLETE, megaCmdMultiTransferListener->getTotalbytes(), clientID);
                }
                delete megaCmdMultiTransferListener;
            }
            else
            {
                setCurrentThreadOutCode(MCMD_NOTFOUND);
                LOG_err << "Couln't find destination folder: " << destination << ". Use -c to create folder structure";
            }
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("put");
        }

        return;
    }
    else if (words[0] == "log")
    {
        const bool cmdFlag = getFlag(clflags, "c");
        const bool sdkFlag = getFlag(clflags, "s");
        const bool noFlags = !cmdFlag && !sdkFlag;

        if (words.size() == 1)
        {
            if (cmdFlag || noFlags)
            {
                OUTSTREAM << "MEGAcmd log level = " << getLogLevelStr(loggerCMD->getCmdLoggerLevel()) << endl;
            }

            if (sdkFlag || noFlags)
            {
                OUTSTREAM << "SDK log level = " << getLogLevelStr(loggerCMD->getSdkLoggerLevel()) << endl;
            }
        }
        else
        {
            auto newLogLevelOpt = getLogLevelNum(words[1].c_str());
            if (!newLogLevelOpt)
            {
                setCurrentThreadOutCode(MCMD_EARGS);
                LOG_err << "Invalid log level";
                return;
            }
            const int newLogLevel = *newLogLevelOpt;

            if (cmdFlag || noFlags)
            {
                loggerCMD->setCmdLoggerLevel(newLogLevel);
                ConfigurationManager::savePropertyValue("cmdLogLevel", newLogLevel);

                OUTSTREAM << "MEGAcmd log level = " << getLogLevelStr(loggerCMD->getCmdLoggerLevel()) << endl;
            }

            if (sdkFlag || noFlags)
            {
                const bool jsonLogs = (newLogLevel == MegaApi::LOG_LEVEL_MAX);
                loggerCMD->setSdkLoggerLevel(newLogLevel);
                api->setLogJSONContent(jsonLogs);

                ConfigurationManager::savePropertyValue("sdkLogLevel", newLogLevel);
                ConfigurationManager::savePropertyValue("jsonLogs", jsonLogs);

                OUTSTREAM << "SDK log level = " << getLogLevelStr(loggerCMD->getSdkLoggerLevel()) << endl;
            }
        }
    }
    else if (words[0] == "pwd")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        string cwpath = getCurrentPath();

        OUTSTREAM << cwpath << endl;

        return;
    }
    else if (words[0] == "lcd") //this only makes sense for interactive mode
    {
        if (words.size() > 1)
        {
            LocalPath localpath = LocalPath::fromAbsolutePath(words[1]);
            if (mFsAccessCMD->chdirlocal(localpath)) // maybe this is already checked in chdir
            {
                LOG_debug << "Local folder changed to: " << words[1];
            }
            else
            {
                setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                LOG_err << "Not a valid folder: " << words[1];
            }
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("lcd");
        }

        return;
    }
    else if (words[0] == "lpwd")
    {
        string absolutePath = getLPWD();

        OUTSTREAM << absolutePath << endl;
        return;
    }
    else if (words[0] == "ipc")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 1)
        {
            int action;
            string saction;

            if (getFlag(clflags, "a"))
            {
                action = MegaContactRequest::REPLY_ACTION_ACCEPT;
                saction = "Accept";
            }
            else if (getFlag(clflags, "d"))
            {
                action = MegaContactRequest::REPLY_ACTION_DENY;
                saction = "Reject";
            }
            else if (getFlag(clflags, "i"))
            {
                action = MegaContactRequest::REPLY_ACTION_IGNORE;
                saction = "Ignore";
            }
            else
            {
                setCurrentThreadOutCode(MCMD_EARGS);
                LOG_err << "      " << getUsageStr("ipc");
                return;
            }

            std::unique_ptr<MegaContactRequest> cr;
            string shandle = words[1];
            handle thehandle = api->base64ToUserHandle(shandle.c_str());

            if (shandle.find('@') != string::npos)
            {
                cr = getPcrByContact(shandle);
            }
            else
            {
                cr.reset(api->getContactRequestByHandle(thehandle));
            }
            if (cr)
            {
                MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);
                api->replyContactRequest(cr.get(), action, megaCmdListener);
                megaCmdListener->wait();
                if (checkNoErrors(megaCmdListener->getError(), "reply ipc"))
                {
                    OUTSTREAM << saction << "ed invitation by " << cr->getSourceEmail() << endl;
                }
                delete megaCmdListener;
            }
            else
            {
                setCurrentThreadOutCode(MCMD_NOTFOUND);
                LOG_err << "Could not find invitation " << shandle;
            }
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("ipc");
            return;
        }
        return;
    }
    else if (words[0] == "https")
    {
        if (words.size() > 1 && (words[1] == "on" || words[1] == "off"))
        {
            bool onlyhttps = words[1] == "on";
            MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
            api->useHttpsOnly(onlyhttps,megaCmdListener);
            megaCmdListener->wait();
            if (checkNoErrors(megaCmdListener->getError(), "change https"))
            {
                OUTSTREAM << "File transfer now uses " << (api->usingHttpsOnly()?"HTTPS":"HTTP") << endl;
                ConfigurationManager::savePropertyValue("https", api->usingHttpsOnly());
            }
            delete megaCmdListener;
            return;
        }
        else if (words.size() > 1)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("https");
            return;
        }
        else
        {
            OUTSTREAM << "File transfer is done using " << (api->usingHttpsOnly()?"HTTPS":"HTTP") << endl;
        }
        return;
    }
    else if (words[0] == "graphics")
    {
        if (words.size() == 2 && (words[1] == "on" || words[1] == "off"))
        {
            bool enableGraphics = words[1] == "on";

            api->disableGfxFeatures(!enableGraphics);
            ConfigurationManager::savePropertyValue("graphics", !api->areGfxFeaturesDisabled());
        }
        else if (words.size() > 1)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("https");
            return;
        }

        OUTSTREAM << "Graphic features " << (api->areGfxFeaturesDisabled()?"disabled":"enabled") << endl;
        return;
    }
#ifndef _WIN32
    else if (words[0] == "permissions")
    {
        bool filesflagread = getFlag(clflags, "files");
        bool foldersflagread = getFlag(clflags, "folders");

        bool filesflag = filesflagread || (!filesflagread && !foldersflagread);
        bool foldersflag = foldersflagread || (!filesflagread && !foldersflagread);

        bool setperms = getFlag(clflags, "s");

        if ( (!setperms && words.size() > 1) || (setperms && words.size() != 2) || ( setperms && filesflagread  && foldersflagread ) || (setperms && !filesflagread && !foldersflagread))
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("permissions");
            return;
        }

        int permvalue = -1;
        if (setperms)
        {
             if (words[1].size() != 3)
             {
                 setCurrentThreadOutCode(MCMD_EARGS);
                 LOG_err << "Invalid permissions value: " << words[1];
             }
             else
             {
                 int owner = words[1].at(0) - '0';
                 int group = words[1].at(1) - '0';
                 int others = words[1].at(2) - '0';
                 if ( (owner < 6) || (owner == 6 && foldersflag) || (owner > 7) || (group < 0) || (group > 7) || (others < 0) || (others > 7) )
                 {
                     setCurrentThreadOutCode(MCMD_EARGS);
                     LOG_err << "Invalid permissions value: " << words[1];
                 }
                 else
                 {
                     permvalue = (owner << 6) + ( group << 3) + others;
                 }
             }
        }

        if (filesflag)
        {
            if (setperms && permvalue != -1)
            {
                api->setDefaultFilePermissions(permvalue);
                ConfigurationManager::savePropertyValue("permissionsFiles", readablePermissions(permvalue));
            }
            int filepermissions = api->getDefaultFilePermissions();
            int owner  = (filepermissions >> 6) & 0x07;
            int group  = (filepermissions >> 3) & 0x07;
            int others = filepermissions & 0x07;

            OUTSTREAM << "Default files permissions: " << owner << group << others << endl;
        }
        if (foldersflag)
        {
            if (setperms && permvalue != -1)
            {
                api->setDefaultFolderPermissions(permvalue);
                ConfigurationManager::savePropertyValue("permissionsFolders", readablePermissions(permvalue));
            }
            int folderpermissions = api->getDefaultFolderPermissions();
            int owner  = (folderpermissions >> 6) & 0x07;
            int group  = (folderpermissions >> 3) & 0x07;
            int others = folderpermissions & 0x07;
            OUTSTREAM << "Default folders permissions: " << owner << group << others << endl;
        }
    }
#endif
    else if (words[0] == "deleteversions")
    {
        bool deleteall = getFlag(clflags, "all");
        bool forcedelete = getFlag(clflags, "f");
        if (deleteall && words.size()>1)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("deleteversions");
            return;
        }
        if (deleteall)
        {
            string confirmationQuery("Are you sure todelete the version histories of all files? (Yes/No): ");

            int confirmationResponse = forcedelete?MCMDCONFIRM_YES:askforConfirmation(confirmationQuery);

            while ( (confirmationResponse != MCMDCONFIRM_YES) && (confirmationResponse != MCMDCONFIRM_NO) )
            {
                confirmationResponse = askforConfirmation(confirmationQuery);
            }
            if (confirmationResponse == MCMDCONFIRM_YES)
            {

                MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                api->removeVersions(megaCmdListener);
                megaCmdListener->wait();
                if (checkNoErrors(megaCmdListener->getError(), "remove all versions"))
                {
                    OUTSTREAM << "File versions deleted successfully. Please note that the current files were not deleted, just their history." << endl;
                }
                delete megaCmdListener;
            }
        }
        else
        {
            for (unsigned int i = 1; i < words.size(); i++)
            {
                if (isRegExp(words[i]))
                {
                    vector<std::unique_ptr<MegaNode>> nodesToDeleteVersions = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                    if (nodesToDeleteVersions.size())
                    {
                        for (const auto& node : nodesToDeleteVersions)
                        {
                            assert(node);

                            int ret = deleteNodeVersions(node, api, forcedelete);
                            forcedelete = forcedelete || (ret == MCMDCONFIRM_ALL);
                        }
                    }
                    else
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        LOG_err << "No node found: " << words[i];
                    }
                }
                else // non-regexp
                {
                    std::unique_ptr<MegaNode> n = nodebypath(words[i].c_str());
                    if (n)
                    {
                        int ret = deleteNodeVersions(n, api, forcedelete);
                        forcedelete = forcedelete || (ret == MCMDCONFIRM_ALL);
                    }
                    else
                    {
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                        LOG_err << "Node not found: " << words[i];
                    }
                }
            }
        }
    }
#ifdef HAVE_LIBUV
    else if (words[0] == "webdav")
    {
        bool remove = getFlag(clflags, "d");
        bool all = getFlag(clflags, "all");

        if (words.size() == 1 && remove && !all)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("webdav");
            return;
        }

        if (words.size() == 1 && !remove)
        {
            //List served nodes
            MegaNodeList *webdavnodes = api->httpServerGetWebDavAllowedNodes();
            if (webdavnodes)
            {
                bool found = false;

                for (int a = 0; a < webdavnodes->size(); a++)
                {
                    MegaNode *n= webdavnodes->get(a);
                    if (n)
                    {
                        char *link = api->httpServerGetLocalWebDavLink(n); //notice this is not only consulting but also creating,
                        //had it been deleted in the meantime this will recreate it
                        if (link)
                        {
                            if (!found)
                            {
                                OUTSTREAM << "WEBDAV SERVED LOCATIONS:" << endl;
                            }
                            found = true;
                            char * nodepath = api->getNodePath(n);
                            OUTSTREAM << nodepath << ": " << link << endl;
                            delete []nodepath;
                            delete []link;
                        }
                    }
                }

                if(!found)
                {
                    OUTSTREAM << "No webdav links found" << endl;
                }

                delete webdavnodes;

           }
           else
           {
               OUTSTREAM << "Webdav server might not running. Add a new location to serve." << endl;
           }

           return;
        }

        if (!remove)
        {
            //create new link:
            bool tls = getFlag(clflags, "tls");
            int port = getintOption(cloptions, "port", 4443);
            bool localonly = !getFlag(clflags, "public");

            string pathtocert = getOption(cloptions, "certificate", "");
            string pathtokey = getOption(cloptions, "key", "");

            bool serverstarted = api->httpServerIsRunning();
            if (!serverstarted)
            {
                LOG_info << "Starting http server";
                api->httpServerEnableFolderServer(true);
    //            api->httpServerEnableOfflineAttribute(true); //TODO: we might want to offer this as parameter
                if (api->httpServerStart(localonly, port, tls, pathtocert.c_str(), pathtokey.c_str()))
                {
                    ConfigurationManager::savePropertyValue("webdav_port", port);
                    ConfigurationManager::savePropertyValue("webdav_localonly", localonly);
                    ConfigurationManager::savePropertyValue("webdav_tls", tls);
                    if (pathtocert.size())
                    {
                        ConfigurationManager::savePropertyValue("webdav_cert", pathtocert);
                    }
                    if (pathtokey.size())
                    {
                        ConfigurationManager::savePropertyValue("webdav_key", pathtokey);
                    }
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_EARGS);
                    LOG_err << "Failed to initialize WEBDAV server. Ensure the port is free.";
                    return;
                }
            }
        }

        //add/remove
        if (remove && all)
        {
            api->httpServerRemoveWebDavAllowedNodes();
            api->httpServerStop();

            list<string> servedpaths;
            ConfigurationManager::savePropertyValueList("webdav_served_locations", servedpaths);
            ConfigurationManager::savePropertyValue("webdav_port", -1); //so as not to load server on startup
            OUTSTREAM << "Wevdav server stopped: no path will be served." << endl;
        }
        else
        {
            bool firstone = true;
            for (unsigned int i = 1; i < words.size(); i++)
            {
                string pathToServe = words[i];
                if (remove)
                {
                    processPath(pathToServe, getFlag(clflags,"use-pcre"), firstone, forwarderRemoveWebdavLocation, this);
                }
                else
                {
                    processPath(pathToServe, getFlag(clflags,"use-pcre"), firstone, forwarderAddWebdavLocation, this);
                }
            }
        }
    }
    else if (words[0] == "ftp")
    {
        bool remove = getFlag(clflags, "d");
        bool all = getFlag(clflags, "all");

        if (words.size() == 1 && remove && !all)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("ftp");
            return;
        }

        if (words.size() == 1 && !remove)
        {
            //List served nodes
            MegaNodeList *ftpnodes = api->ftpServerGetAllowedNodes();
            if (ftpnodes)
            {
                bool found = false;

                for (int a = 0; a < ftpnodes->size(); a++)
                {
                    MegaNode *n= ftpnodes->get(a);
                    if (n)
                    {
                        char *link = api->ftpServerGetLocalLink(n); //notice this is not only consulting but also creating,
                        //had it been deleted in the meantime this will recreate it
                        if (link)
                        {
                            if (!found)
                            {
                                OUTSTREAM << "FTP SERVED LOCATIONS:" << endl;
                            }
                            found = true;
                            char * nodepath = api->getNodePath(n);
                            OUTSTREAM << nodepath << ": " << link << endl;
                            delete []nodepath;
                            delete []link;
                        }
                    }
                }

                if(!found)
                {
                    OUTSTREAM << "No ftp links found" << endl;
                }

                delete ftpnodes;

           }
           else
           {
               OUTSTREAM << "Ftp server might not running. Add a new location to serve." << endl;
           }

           return;
        }

        if (!remove)
        {
            //create new link:
            bool tls = getFlag(clflags, "tls");
            int port = getintOption(cloptions, "port", 4990);
            bool localonly = !getFlag(clflags, "public");

            string dataPorts = getOption(cloptions, "data-ports");
            size_t seppos = dataPorts.find("-");
            int dataPortRangeBegin = 1500;
            istringstream is(dataPorts.substr(0,seppos));
            is >> dataPortRangeBegin;
            int dataPortRangeEnd = dataPortRangeBegin + 100;
            if (seppos != string::npos && seppos < (dataPorts.size()+1))
            {
                istringstream is(dataPorts.substr(seppos+1));
                is >> dataPortRangeEnd;
            }

            string pathtocert = getOption(cloptions, "certificate", "");
            string pathtokey = getOption(cloptions, "key", "");

            if (tls && (!pathtocert.size() || !pathtokey.size()))
            {
                setCurrentThreadOutCode(MCMD_EARGS);
                LOG_err << "Path to certificate/key not indicated";
                return;
            }

            bool serverstarted = api->ftpServerIsRunning();
            if (!serverstarted)
            {
                if (api->ftpServerStart(localonly, port, dataPortRangeBegin, dataPortRangeEnd, tls, pathtocert.c_str(), pathtokey.c_str()))
                {
                    ConfigurationManager::savePropertyValue("ftp_port", port);
                    ConfigurationManager::savePropertyValue("ftp_port_data_begin", dataPortRangeBegin);
                    ConfigurationManager::savePropertyValue("ftp_port_data_end", dataPortRangeEnd);
                    ConfigurationManager::savePropertyValue("ftp_localonly", localonly);
                    ConfigurationManager::savePropertyValue("ftp_tls", tls);
                    if (pathtocert.size())
                    {
                        ConfigurationManager::savePropertyValue("ftp_cert", pathtocert);
                    }
                    if (pathtokey.size())
                    {
                        ConfigurationManager::savePropertyValue("ftp_key", pathtokey);
                    }
                    LOG_info << "Started ftp server at port " << port << ". Data Channel Port Range: " << dataPortRangeBegin << "-" << dataPortRangeEnd;
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_EARGS);
                    LOG_err << "Failed to initialize FTP server. Ensure the port is free.";
                    return;
                }
            }
        }

        //add/remove
        if (remove && all)
        {
            api->ftpServerRemoveAllowedNodes();
            api->ftpServerStop();

            list<string> servedpaths;
            ConfigurationManager::savePropertyValueList("ftp_served_locations", servedpaths);
            ConfigurationManager::savePropertyValue("ftp_port", -1); //so as not to load server on startup
            OUTSTREAM << "ftp server stopped: no path will be served." << endl;
        }
        else
        {
            bool firstone = true;
            for (unsigned int i = 1; i < words.size(); i++)
            {
                string pathToServe = words[i];
                if (remove)
                {
                    processPath(pathToServe, getFlag(clflags,"use-pcre"), firstone, forwarderRemoveFtpLocation, this);
                }
                else
                {
                    processPath(pathToServe, getFlag(clflags,"use-pcre"), firstone, forwarderAddFtpLocation, this);
                }
            }
        }
    }
#endif
#ifdef ENABLE_SYNC
    else if (words[0] == "exclude")
    {
        bool excludeAdd = getFlag(clflags, "a");
        bool excludeRemove = getFlag(clflags, "d");
        if (excludeAdd == excludeRemove)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("exclude");
            return;
        }

        {
            SyncIgnore::Args args;
            args.mAction = (excludeAdd ? SyncIgnore::Action::Add : SyncIgnore::Action::Remove);

            std::transform(words.begin() + 1, words.end(),
                           std::inserter(args.mFilters, args.mFilters.end()),
                           SyncIgnore::getFilterFromLegacyPattern);

            SyncIgnore::executeCommand(args);
        }

        {
            SyncIgnore::Args args;
            args.mAction = SyncIgnore::Action::Show;

            OUTSTREAM << endl;
            OUTSTREAM << "Contents of .megaignore.default:" << endl;
            SyncIgnore::executeCommand(args);
        }
    }
    else if (words[0] == "sync")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in";
            return;
        }

        if (!api->isLoggedIn())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in";
            return;
        }

        bool pauseSync = getFlag(clflags, "p") || getFlag(clflags, "pause") || getFlag(clflags, "s") || getFlag(clflags, "disable");
        bool enableSync = getFlag(clflags, "e") || getFlag(clflags, "r") || getFlag(clflags, "enable");
        bool deleteSync = getFlag(clflags, "delete") || getFlag(clflags, "d") || getFlag(clflags, "remove");
        bool showHandles = getFlag(clflags, "show-handles");

        if (!onlyZeroOrOneOf(pauseSync, enableSync, deleteSync))
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Only one action (disable, enable, or remove) can be specified at a time";
            LOG_err << "      " << getUsageStr("sync");
            return;
        }

        if (words.size() == 3) // add a sync
        {
            fs::path localPath = fs::absolute(words[1]);
            if (!fs::exists(localPath))
            {
                setCurrentThreadOutCode(MCMD_NOTFOUND);
                LOG_err << "Local directory " << words[1] << " does not exist";
                return;
            }

            std::unique_ptr<MegaNode> n = nodebypath(words[2].c_str());
            if (!n)
            {
                setCurrentThreadOutCode(MCMD_NOTFOUND);
                LOG_err << "Remote directory " << words[2] << " does not exist";
                return;
            }

            SyncCommand::addSync(*api, localPath, *n);
        }
        else if (words.size() == 2) // manage a sync
        {
            string pathOrId = words[1];
            auto sync = SyncCommand::getSync(*api, pathOrId);
            if (!sync)
            {
                setCurrentThreadOutCode(MCMD_NOTFOUND);
                LOG_err << "Sync not found: " << pathOrId;
                return;
            }

            if (deleteSync)
            {
                SyncCommand::modifySync(*api, *sync, SyncCommand::ModifyOpts::Delete);
            }
            else
            {
                SyncCommand::modifySync(*api, *sync, enableSync ? SyncCommand::ModifyOpts::Enable : SyncCommand::ModifyOpts::Pause);

                // Print the updated sync state if we didnt' remove
                sync = SyncCommand::reloadSync(*api, std::move(sync));
                if (!sync)
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << "Sync not found while reloading: " << pathOrId;
                    return;
                }

                auto syncIssues = mSyncIssuesManager.getSyncIssues();

                ColumnDisplayer cd(clflags, cloptions);
                SyncCommand::printSync(*api, cd, showHandles, *sync, syncIssues);

                OUTSTREAM << cd.str();
            }
        }
        else if (words.size() == 1) // show all syncs
        {
            auto syncIssues = mSyncIssuesManager.getSyncIssues();
            auto syncList = std::unique_ptr<MegaSyncList>(api->getSyncs());
            assert(syncList);

            ColumnDisplayer cd(clflags, cloptions);
            SyncCommand::printSyncList(*api, cd, showHandles, *syncList, syncIssues);

            OUTSTREAM << cd.str();

            if (!syncIssues.empty())
            {
                OUTSTREAM << endl;
                LOG_err << "You have sync issues. Use the \"" << getCommandPrefixBasedOnMode() << "sync-issues\" command to display them.";
            }
            else if (SyncCommand::isAnySyncUploadDelayed(*api))
            {
                OUTSTREAM << endl;
                OUTSTREAM << "Some of your \"Pending\" sync uploads are being delayed due to very frequent changes. They will be uploaded once the delay finishes. "
                          << "Use the \"" << getCommandPrefixBasedOnMode() << "sync-config\" command to show the upload delay." << endl;
            }
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << getUsageStr("sync");
            return;
        }
    }
    else if (words[0] == "sync-ignore")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in";
            return;
        }

        if (!api->isLoggedIn())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in";
            return;
        }

        if (words.size() < 2)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("sync-ignore");
            return;
        }

        bool ignoreShow = getFlag(clflags, "show");
        bool ignoreAdd = getFlag(clflags, "add");
        bool ignoreAddExclusion = getFlag(clflags, "add-exclusion");
        bool ignoreRemove = getFlag(clflags, "remove");
        bool ignoreRemoveExclusion = getFlag(clflags, "remove-exclusion");

        if (!onlyZeroOrOneOf(ignoreShow, ignoreAdd, ignoreAddExclusion, ignoreRemove, ignoreRemoveExclusion))
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Only one action (show, add, add-exclusion, remove, or remove-exclusion) can be specified at a time";
            LOG_err << "      " << getUsageStr("sync-ignore");
            return;
        }

        SyncIgnore::Args args;

        args.mAction = SyncIgnore::Action::Show;
        if (ignoreAdd || ignoreAddExclusion)
        {
            args.mAction = SyncIgnore::Action::Add;
        }
        else if (ignoreRemove || ignoreRemoveExclusion)
        {
            args.mAction = SyncIgnore::Action::Remove;
        }

        // Show cannot have filters
        if (args.mAction == SyncIgnore::Action::Show && words.size() != 2)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("sync-ignore");
            return;
        }

        string pathOrId = words.back(); // the last word is (ID|localpath|"DEFAULT")
        if (toLower(pathOrId) != "default")
        {
            auto sync = SyncCommand::getSync(*api, pathOrId);
            if (!sync)
            {
                setCurrentThreadOutCode(MCMD_NOTFOUND);
                LOG_err << "Sync " << pathOrId << " was not found";
                return;
            }

            args.mMegaIgnoreDirPath = std::string(sync->getLocalFolder());
        }

        auto filterInserter = [ignoreAddExclusion, ignoreRemoveExclusion] (const string& word)
        {
            if (ignoreAddExclusion || ignoreRemoveExclusion)
            {
                return "-" + word;
            }
            return word;
        };

        std::transform(words.begin() + 1, words.end() - 1,
                       std::inserter(args.mFilters, args.mFilters.end()), filterInserter);

        SyncIgnore::executeCommand(args);
    }
    else if (words[0] == "sync-config")
    {
        using namespace GlobalSyncConfig;

        if (words.size() != 1)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << getUsageStr("sync-config");
            return;
        }

        auto duWaitSecsOpt = getFlag(clflags, "delayed-uploads-wait-seconds");
        auto duMaxAttemptsOpt = getFlag(clflags, "delayed-uploads-max-attempts");

        bool all = !duWaitSecsOpt && !duMaxAttemptsOpt;

        {
            auto duConfigOpt = DelayedUploads::getCurrentConfig(*api);
            if (!duConfigOpt)
            {
                LOG_err << "Failed to retrieve delayed sync uploads config";
                return;
            }

            DelayedUploads::Config duConfig = *duConfigOpt;
            if (all || duWaitSecsOpt)
            {
                OUTSTREAM << "Delayed uploads wait time: " << duConfig.mWaitSecs << " seconds" << endl;
            }
            if (all || duMaxAttemptsOpt)
            {
                OUTSTREAM << "Max attempts until uploads are delayed: " << duConfig.mMaxAttempts << endl;
            }
            return;
        }
    }
#endif
    else if (words[0] == "configure")
    {
        std::optional<std::string_view> key;
        std::optional<std::string_view> value;
        if (words.size() > 1)
        {
            key = words[1];
        }
        if (words.size() > 2)
        {
            value= words[2];
        }

        bool found = false;

        for (auto &vc : Instance<ConfiguratorMegaApiHelper>::Get().getConfigurators())
        {
            auto name = vc.mKey.c_str();
            if (key && *key != name) continue;

            found = true;

            if (value)
            {
                if (vc.mValidator && !vc.mValidator.value()(value->data()))
                {
                    LOG_err << "Failed to set " << vc.mKey << " (" << vc.mDescription << "). Invalid value";
                    setCurrentThreadOutCode(MCMD_EARGS);
                    return;
                }

                if (!vc.mSetter(api, std::string(name), value->data()))
                {
                    if (getCurrentThreadOutCode() == MCMD_OK) // presuming error would be logged otherwise
                    {
                        LOG_err << "Failed to set " << vc.mKey << " (" << vc.mDescription << "). Setting failed";
                        setCurrentThreadOutCode(MCMD_EARGS);
                    }
                    return;
                }
                ConfigurationManager::saveProperty(name, value->data());
            }

            auto &getter = vc.mMegaApiGetter ? *vc.mMegaApiGetter : vc.mGetter;

            auto newValueOpt = getter(api, name);
            if (newValueOpt)
            {
                OUTSTREAM << name << " = " << *newValueOpt << std::endl;
                if (value && *value != *newValueOpt)
                {
                    LOG_warn << "Setting " << vc.mKey << " resulted in a value different than the one intended. " << *value <<
                                " vs " << *newValueOpt;
                }
            }
            else if (key)
            {
                LOG_err << "Configuration value is unset: " << *key;
                setCurrentThreadOutCode(MCMD_NOTFOUND);
            }
        }

        if (!found)
        {
            LOG_err << "Provided configuration key does not exist: " << *key;
            setCurrentThreadOutCode(MCMD_EARGS);
        }
    }
    else if (words[0] == "cancel")
    {
        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
        api->cancelAccount(megaCmdListener);
        megaCmdListener->wait();
        if (checkNoErrors(megaCmdListener->getError(), "cancel account"))
        {
            OUTSTREAM << "Account pending cancel confirmation. You will receive a confirmation link. Use \"confirmcancel\" with the provided link to confirm the cancellation" << endl;
        }
        delete megaCmdListener;
    }
    else if (words[0] == "confirmcancel")
    {
        if (words.size() < 2)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("confirmcancel");
            return;
        }

        const char * confirmlink = words[1].c_str();
        if (words.size() > 2)
        {
            const char * pass = words[2].c_str();
            confirmCancel(confirmlink, pass);
        }
        else if (isCurrentThreadInteractive())
        {
            link = confirmlink;
            confirmingcancel = true;
            setprompt(LOGINPASSWORD);
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Extra args required in non-interactive mode. Usage: " << getUsageStr("confirmcancel");
        }
    }
    else if (words[0] == "login")
    {
        LoginGuard loginGuard;
        int clientID = getintOption(cloptions, "clientID", -1);

        if (api->isLoggedIn())
        {
            setCurrentThreadOutCode(MCMD_INVALIDSTATE);
            LOG_err << "Already logged in. Please log out first.";
            return;
        }

        if (words.size() < 2)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("login");
            return;
        }

        bool accountLogin = words[1].find('@') != std::string::npos;
        bool folderLinkLogin = !accountLogin && words[1].find('#') != std::string::npos;
        bool resumeFolderLink = getFlag(clflags, "resume");
        string authKey = getOption(cloptions, "auth-key", "");

        if (!folderLinkLogin)
        {
            if (resumeFolderLink)
            {
                setCurrentThreadOutCode(MCMD_EARGS);
                LOG_err << "Explicit resumption only required for folder links logins.";
                return;
            }
            if (!authKey.empty())
            {
                setCurrentThreadOutCode(MCMD_EARGS);
                LOG_err << "Auth key only required for login in writable folder links.";
                return;
            }
        }

        if (accountLogin)
        {
            if (words.size() > 2)
            {
                MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL,NULL,clientID);
                sandboxCMD->resetSandBox();
                api->login(words[1].c_str(), words[2].c_str(), megaCmdListener);
                if (actUponLogin(megaCmdListener) == MegaError::API_EMFAREQUIRED )
                {
                    MegaCmdListener *megaCmdListener2 = new MegaCmdListener(NULL,NULL,clientID);
                    string pin2fa = getOption(cloptions, "auth-code", "");
                    if (!pin2fa.size())
                    {
                        pin2fa = askforUserResponse("Enter the code generated by your authentication app: ");
                    }
                    LOG_verbose << " Using confirmation pin: " << pin2fa;
                    api->multiFactorAuthLogin(words[1].c_str(), words[2].c_str(), pin2fa.c_str(), megaCmdListener2);
                    actUponLogin(megaCmdListener2);
                    delete megaCmdListener2;
                    return;

                }
                delete megaCmdListener;
            }
            else
            {
                login = words[1];
                if (isCurrentThreadInteractive())
                {
                    setprompt(LOGINPASSWORD);
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_EARGS);
                    LOG_err << "Extra args required in non-interactive mode. Usage: " << getUsageStr("login");
                }
            }
        }
        else if (folderLinkLogin)  // folder link indicator
        {
            string publicLink = words[1];
            if (!decryptLinkIfEncrypted(api, publicLink, cloptions))
            {
                return;
            }

            std::unique_ptr<MegaCmdListener>megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
            sandboxCMD->resetSandBox();
            api->loginToFolder(publicLink.c_str(), authKey.empty() ? nullptr : authKey.c_str(),
                               resumeFolderLink, megaCmdListener.get());

            actUponLogin(megaCmdListener.get());
            return;
        }
        else // session resumption
        {
            LOG_info << "Resuming session...";
            MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
            sandboxCMD->resetSandBox();
            api->fastLogin(words[1].c_str(), megaCmdListener);
            actUponLogin(megaCmdListener);
            delete megaCmdListener;
            return;
        }

        return;
    }
    else if (words[0] == "psa")
    {
        int psanum = sandboxCMD->lastPSAnumreceived;

#ifndef NDEBUG
        if (words.size() >1)
        {
            psanum = toInteger(words[1], 0);
        }
#endif
        bool discard = getFlag(clflags, "discard");

        if (discard)
        {
            MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
            api->setPSA(psanum, megaCmdListener);
            megaCmdListener->wait();
            if (checkNoErrors(megaCmdListener->getError(), "set psa " + SSTR(psanum)))
            {
                OUTSTREAM << "PSA discarded" << endl;
            }
            delete megaCmdListener;
        }

        if (!checkAndInformPSA(NULL, true) && !discard) // even when discarded: we need to read the next
        {
            OUTSTREAM << "No PSA available" << endl;
            setCurrentThreadOutCode(MCMD_NOTFOUND);
        }
    }
    else if (words[0] == "mount")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        listtrees();

        verifySharedFolders(api);

        return;
    }
    else if (words[0] == "share")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        string with = getOption(cloptions, "with", "");
        if (getFlag(clflags, "a") && ( "" == with ))
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << " Required --with=user";
            LOG_err <<  "      " << getUsageStr("share");
            return;
        }

        string slevel = getOption(cloptions, "level", "NONE");

        int level_NOT_present_value = -214;
        int level;
        if (slevel == "NONE")
        {
            level = level_NOT_present_value;
        }
        else
        {
            level = getShareLevelNum(slevel.c_str());
        }
        if (( level != level_NOT_present_value ) && (( level < -1 ) || ( level > 3 )))
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Invalid level of access";
            return;
        }
        bool listPending = getFlag(clflags, "p");

        if (words.size() <= 1)
        {
            words.push_back(string(".")); //cwd
        }

        for (int i = 1; i < (int)words.size(); i++)
        {
            unescapeifRequired(words[i]);
            if (isRegExp(words[i]))
            {
                vector<std::unique_ptr<MegaNode>> nodes = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                if (nodes.empty())
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    if (words[i].find("@") != string::npos)
                    {
                        LOG_err << "Could not find " << words[i] << ". Use --with=" << words[i] << " to specify the user to share with";
                    }
                    else
                    {
                        LOG_err << "Node not found: " << words[i];
                    }
                }

                for (const auto& n : nodes)
                {
                    assert(n);

                    if (getFlag(clflags, "a"))
                    {
                        LOG_debug << " sharing ... " << n->getName() << " with " << with;
                        if (level == level_NOT_present_value)
                        {
                            level = MegaShare::ACCESS_READ;
                        }

                        if (n->getType() == MegaNode::TYPE_FILE)
                        {
                            setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                            LOG_err << "Cannot share file: " << n->getName() << ". Only folders allowed. You can send file to user's inbox with cp (see \"cp --help\")";
                        }
                        else
                        {
                            shareNode(n.get(), with, level);
                        }
                    }
                    else if (getFlag(clflags, "d"))
                    {
                        if ("" != with)
                        {
                            LOG_debug << " deleting share ... " << n->getName() << " with " << with;
                            disableShare(n.get(), with);
                        }
                        else
                        {
                            MegaShareList* outShares = api->getOutShares(n.get());
                            if (outShares)
                            {
                                for (int i = 0; i < outShares->size(); i++)
                                {
                                    if (outShares->get(i)->getNodeHandle() == n->getHandle())
                                    {
                                        LOG_debug << " deleting share ... " << n->getName() << " with " << outShares->get(i)->getUser();
                                        disableShare(n.get(), outShares->get(i)->getUser());
                                    }
                                }

                                delete outShares;
                            }
                        }
                    }
                    else
                    {
                        if (( level != level_NOT_present_value ) || ( with != "" ))
                        {
                            setCurrentThreadOutCode(MCMD_EARGS);
                            LOG_err << "Unexpected option received. To create/modify a share use -a";
                        }
                        else if (listPending)
                        {
                            dumpListOfAllShared(n.get(), words[i]);
                        }
                        else
                        {
                            dumpListOfShared(n.get(), words[i]);
                        }
                    }
                }
            }
            else // non-regexp
            {
                std::unique_ptr<MegaNode> n = nodebypath(words[i].c_str());
                if (n)
                {
                    if (getFlag(clflags, "a"))
                    {
                        LOG_debug << " sharing ... " << n->getName() << " with " << with;
                        if (level == level_NOT_present_value)
                        {
                            level = MegaShare::ACCESS_READ;
                        }
                        shareNode(n.get(), with, level);
                    }
                    else if (getFlag(clflags, "d"))
                    {
                        if ("" != with)
                        {
                            LOG_debug << " deleting share ... " << n->getName() << " with " << with;
                            disableShare(n.get(), with);
                        }
                        else
                        {
                            std::unique_ptr<MegaShareList> outShares(api->getOutShares(n.get()));
                            if (outShares)
                            {
                                for (int i = 0; i < outShares->size(); i++)
                                {
                                    if (outShares->get(i)->getNodeHandle() == n->getHandle())
                                    {
                                        LOG_debug << " deleting share ... " << n->getName() << " with " << outShares->get(i)->getUser();
                                        disableShare(n.get(), outShares->get(i)->getUser());
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        if ((level != level_NOT_present_value ) || ( with != "" ))
                        {
                            setCurrentThreadOutCode(MCMD_EARGS);
                            LOG_err << "Unexpected option received. To create/modify a share use -a";
                        }
                        else if (listPending)
                        {
                            dumpListOfAllShared(n.get(), words[i]);
                        }
                        else
                        {
                            dumpListOfShared(n.get(), words[i]);
                        }
                    }
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << "Node not found: " << words[i];
                }
            }
        }

        verifySharedFolders(api);

        return;
    }
    else if (words[0] == "users")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }

        bool verify = getFlag(clflags, "verify");
        bool unverify = getFlag(clflags, "unverify");

        if (verify || unverify)
        {
            if (!checkExactlyNArgs(words, 2))
            {
                return;
            }
            const auto &contact = words[1];
            std::unique_ptr<MegaUser> user(api->getContact(contact.c_str()));
            if (!user)
            {
                setCurrentThreadOutCode(MCMD_NOTFOUND);
                LOG_err << "Contact not found.";
                return;
            }

            auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
            if (unverify)
            {
                api->resetCredentials(user.get(), megaCmdListener.get());
            }
            else
            {
                api->verifyCredentials(user.get(), megaCmdListener.get());
            }
            megaCmdListener->wait();

            if (megaCmdListener->getError()->getErrorCode() == MegaError::API_EINCOMPLETE)
            {
                setCurrentThreadOutCode(megaCmdListener->getError()->getErrorCode());
                LOG_err << "Failed to " << " set contact as " << (unverify ? "no longer " : "") << "verified, your account's security may need upgrading";
            }
            else if (checkNoErrors(megaCmdListener->getError(), unverify ? "unverify credentials" : "verify credentials"))
            {
                OUTSTREAM << "Contact " << contact << " set as " << (unverify ? "no longer " : "") << "verified." << endl;
            }
            return;
        }

        if (getFlag(clflags, "help-verify"))
        {
            if (words.size() == 1) // General information
            {
                OUTSTREAM << "In order to share data with your contacts you may need to verify them." << endl
                          << endl
                          << "Verifying means ensuring that the contact is who he/she claims to be." << endl
                          << "To ensure that, both of you will need to share your credentials," << endl
                          << " i.e. some numbers that uniquely identify you." << endl
                          << "You can see a contact's credentials (and yours) and instructions on verifying," << endl
                          << " by typing \"" << getCommandPrefixBasedOnMode() << "users --help-verify contact@email\"." << endl
                          << endl
                          << "To see which contacts are not verified, you can list them using \"" << getCommandPrefixBasedOnMode() << "users -n\"" << endl
                          << "If you want the above listing to include information regarding your share folders," << endl
                          << " type \"" <<getCommandPrefixBasedOnMode() << "users -sn\"." << endl
                          << endl;
                return;
            }

            // Listing credentials and instructions for an specific contact:
            if (!checkExactlyNArgs(words, 2))
            {
                return;
            }
            const auto &contact = words[1];
            std::unique_ptr<MegaUser> user(api->getContact(contact.c_str()));
            if (!user)
            {
                setCurrentThreadOutCode(MCMD_NOTFOUND);
                LOG_err << "Contact not found.";
                return;
            }

            auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
            api->getUserCredentials(user.get(), megaCmdListener.get());
            megaCmdListener->wait();
            if (!checkNoErrors(megaCmdListener->getError(), "get user credentials"))
            {
                return;
            }

            auto contactCredentials = megaCmdListener->getRequest()->getPassword();
            if (!contactCredentials)
            {
                setCurrentThreadOutCode(MCMD_NOTFOUND);
                LOG_err << "Contact credentials not found.";
                return;
            }

            std::unique_ptr<char[]> myCredentials(api->getMyCredentials());
            if (!myCredentials)
            {
                setCurrentThreadOutCode(MCMD_NOTFOUND);
                LOG_err << "Own credentials not found.";
                return;
            }

            auto beautifyCreds = [](std::string x)
            {
                auto nspaces = x.size() / 4;
                for (size_t i = 1 ; i < nspaces ;  i++)
                {
                    x.insert( i * 4 + i - 1, i == (nspaces / 2) ? "\n" : " ");
                }
                return x;
            };

            OUTSTREAM << "Updated verification credentials were received for your contact: ";

            OUTSTREAM << contact << endl;

            {
                std::stringstream ss;
                ss << "Your Contact's credentials:\n";
                ss << beautifyCreds(contactCredentials);
                printCenteredContentsT(OUTSTREAM, ss.str(), 32, true);
            }
            {
                std::stringstream ss;
                ss << "Your credentials:\n";
                ss << beautifyCreds(myCredentials.get());
                printCenteredContentsT(OUTSTREAM, ss.str(), 32, true);
            }

            OUTSTREAM << "Compare the listed credentials with the ones reported by your contact." << endl;

            OUTSTREAM << "This is best done in real life by meeting face to face.\n"
                         "If you have another already-verified channel such as verified OTR or PGP, you may also use that.";

            OUTSTREAM << endl << "If both credentials match, type \"" << getCommandPrefixBasedOnMode() << "users --verify " << contact << "\" to set the contact as verified." << endl;

            OUTSTREAM << endl << "Important: verification is two sided. You need to tell your contact to do the same for you, using MEGAcmd or MEGA website to verify your credentials." << endl;

            return;
        }
        if (getFlag(clflags, "d") && ( words.size() <= 1 ))
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Contact to delete not specified";
            return;
        }
        MegaUserList* usersList = api->getContacts();
        if (usersList)
        {
            for (int i = 0; i < usersList->size(); i++)
            {
                MegaUser *user = usersList->get(i);

                if (getFlag(clflags, "d") && ( words.size() > 1 ) && ( words[1] == user->getEmail()))
                {
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                    api->removeContact(user, megaCmdListener);
                    megaCmdListener->wait();
                    if (checkNoErrors(megaCmdListener->getError(), "delete contact"))
                    {
                        OUTSTREAM << "Contact " << words[1] << " removed successfully" << endl;
                    }
                    delete megaCmdListener;
                }
                else
                {
                    if (!(( user->getVisibility() != MegaUser::VISIBILITY_VISIBLE ) && !getFlag(clflags, "h")))
                    {
                        auto email = user->getEmail();
                        std::string nameOrEmail;

                        if (getFlag(clflags,"n")) //Show Names
                        {
                            // name:
                            auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
                            api->getUserAttribute(user, ATTR_FIRSTNAME, megaCmdListener.get());
                            megaCmdListener->wait();
                            if (megaCmdListener->getError()->getErrorCode() == MegaError::API_OK
                                    && megaCmdListener->getRequest()->getText()
                                    && *megaCmdListener->getRequest()->getText() // not empty
                                    )
                            {
                                nameOrEmail += megaCmdListener->getRequest()->getText();
                            }
                            // surname:
                            megaCmdListener.reset(new MegaCmdListener(nullptr));
                            api->getUserAttribute(user, ATTR_LASTNAME, megaCmdListener.get());
                            megaCmdListener->wait();
                            if (megaCmdListener->getError()->getErrorCode() == MegaError::API_OK
                                    && megaCmdListener->getRequest()->getText()
                                    && *megaCmdListener->getRequest()->getText() // not empty
                                    )
                            {
                                if (!nameOrEmail.empty())
                                {
                                    nameOrEmail+=" ";
                                }
                                nameOrEmail += megaCmdListener->getRequest()->getText();
                            }

                            if (!nameOrEmail.empty())
                            {
                                OUTSTREAM << "[" << nameOrEmail << "] ";
                            }
                        }

                        if (nameOrEmail.empty())
                        {
                            nameOrEmail = email;
                        }

                        OUTSTREAM << email;

                        if (user->getTimestamp())
                        {
                            OUTSTREAM << ". Contact since " << getReadableTime(user->getTimestamp(), getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")));
                        }

                        OUTSTREAM << ". " << visibilityToString(user->getVisibility())
                                  << endl;

                        bool printedSomeUnaccesibleInShare = false;
                        bool printedSomeUnaccesibleOutShare = false;
                        auto printShares = [&, this](std::unique_ptr<MegaShareList> &shares, const std::string &title, bool isInShare = false)
                        {
                            if (!shares)
                            {
                                return;
                            }

                            bool first_share = true;

                            for (int j = 0, total = shares->size(); j < total; j++)
                            {
                                auto share = shares->get(j);
                                assert(share);
                                if (!strcmp(share->getUser(), email))
                                {
                                    bool thisOneisUnverified = !share->isVerified();

                                    std::unique_ptr<MegaNode> n (api->getNodeByHandle(share->getNodeHandle()));
                                    if (n)
                                    {
                                        if (first_share)
                                        {
                                            OUTSTREAM << "> " << title << ":" << endl;
                                            first_share = false;
                                        }

                                        if (thisOneisUnverified)
                                        {
                                            if (isInShare)
                                            {
                                                printedSomeUnaccesibleInShare = true;
                                                OUTSTREAM << " (**)";
                                            }
                                            else
                                            {
                                                printedSomeUnaccesibleOutShare = true;
                                                OUTSTREAM << "  (*)";
                                            }
                                        }
                                        else
                                        {
                                            OUTSTREAM << "  ";
                                        }

                                        if (isInShare)
                                        {
                                            OUTSTREAM << "//from/";
                                        }

                                        if (isInShare && (thisOneisUnverified || !n->isNodeKeyDecrypted()))
                                        {
                                            OUTSTREAM << email << (thisOneisUnverified ? ":[UNVERIFIED]" : "[UNDECRYPTABLE]") << endl;
                                        }
                                        else if (isInShare)
                                        {
                                            dumpNode(n.get(), getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, 2, false, 0, getDisplayPath("/", n.get()).c_str());
                                        }
                                        else //outShare:
                                        {
                                            string path = getDisplayPath("", n.get());
                                            printOutShareInfo(path.c_str(), nullptr/*dont print email*/, share->getAccess(), share->isPending(), share->isVerified());
                                        }
                                    }
                                }
                            }
                        };

                        bool isUserVerified = api->areCredentialsVerified(user);
                        bool doPrint(getFlag(clflags, "s"));
                        if (doPrint)
                        {
                            std::unique_ptr<MegaShareList> outSharesByAllUsers (api->getOutShares()); // this one retrieves both verified & unverified ones
                            printShares(outSharesByAllUsers, std::string("Folders shared with ") + nameOrEmail);

                            std::unique_ptr<MegaShareList> inSharesByAllUsers (api->getInSharesList()); // this one does not return unverified ones
                            printShares(inSharesByAllUsers , std::string("Folders shared by ") + nameOrEmail, true);

                            std::unique_ptr<MegaShareList> unverifiedInShares (api->getUnverifiedInShares());
                            printShares(unverifiedInShares , std::string("Folders shared by ") + nameOrEmail, true);
                        }
                        assert(!printedSomeUnaccesibleInShare || !isUserVerified);

                        if (!isUserVerified)
                        {
                            std::stringstream ss;
                            ss << "Contact [" << nameOrEmail << "] is not verified";

                            if (printedSomeUnaccesibleInShare || printedSomeUnaccesibleOutShare)
                            {
                                ss << "\n";
                                if (printedSomeUnaccesibleOutShare)
                                {
                                    ss << "Shares marked with (*) may not be accessible by your contact.\n";
                                }
                                if (printedSomeUnaccesibleInShare)
                                {
                                    ss << "Shares marked with (**) may not be accessible by you.\n";
                                }
                            }

                            ss << "\nType \"" << getCommandPrefixBasedOnMode() <<"users --help-verify " << email
                               << "\" to get instructions on verification" << endl;

                            printCenteredContentsT(OUTSTREAM, ss.str(), 78, true);
                            OUTSTREAM << endl;
                        }
                    }
                }
            }

            delete usersList;
        }

        return;
    }
    else if (words[0] == "mkdir")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        int globalstatus = MCMD_OK;
        if (words.size()<2)
        {
            globalstatus = MCMD_EARGS;
        }
        bool printusage = false;
        for (unsigned int i = 1; i < words.size(); i++)
        {
            unescapeifRequired(words[i]);
            //git first existing node in the asked path:
            MegaNode *baseNode;

            string rest = words[i];
            if (rest.find("//bin/") == 0)
            {
                baseNode = api->getRubbishNode();
                rest = rest.substr(6);
            }//elseif //in/
            else if(rest.find("/") == 0)
            {
                baseNode = api->getRootNode();
                rest = rest.substr(1);
            }
            else
            {
                baseNode = api->getNodeByHandle(cwd);
            }

            while (baseNode && rest.length())
            {
                size_t possep = rest.find_first_of("/");
                if (possep == string::npos)
                {
                    break;
                }

                string next = rest.substr(0, possep);
                if (next == ".")
                {
                    rest = rest.substr(possep + 1);
                    continue;
                }
                else if(next == "..")
                {
                    MegaNode *aux = baseNode;
                    baseNode = api->getNodeByHandle(baseNode->getParentHandle());

                    if (aux!=baseNode) // let's be paranoid
                    {
                        delete aux;
                    }
                }
                else
                {
                    MegaNodeList *children = api->getChildren(baseNode);
                    if (children)
                    {
                        bool found = false;
                        for (int i = 0; i < children->size(); i++)
                        {
                            MegaNode *child = children->get(i);
                            if (next == child->getName())
                            {
                                MegaNode *aux = baseNode;
                                baseNode = child->copy();
                                found = true;
                                if (aux!=baseNode) // let's be paranoid
                                {
                                    delete aux;
                                }
                                break;
                            }
                        }
                        delete children;
                        if (!found)
                        {
                            break;
                        }
                    }
                }

                rest = rest.substr(possep + 1);
            }
            if (baseNode)
            {
                int status = makedir(rest,getFlag(clflags, "p"),baseNode);
                if (status != MCMD_OK)
                {
                    globalstatus = status;
                }
                if (status == MCMD_EARGS)
                {
                    printusage = true;
                }
                delete baseNode;
            }
            else
            {
                setCurrentThreadOutCode(MCMD_INVALIDSTATE);
                LOG_err << "Folder navigation failed";
                return;
            }

        }

        setCurrentThreadOutCode(globalstatus);
        if (printusage)
        {
            LOG_err << "      " << getUsageStr("mkdir");
        }

        return;
    }
    else if (words[0] == "attr")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }

        if (words.size() <= 1)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr(words[0].c_str());
            return;
        }

        bool removingAttr(getFlag(clflags, "d"));
        bool settingattr(getFlag(clflags, "s"));
        bool forceCustom = getFlag(clflags, "force-non-official");

        string nodePath = words.size() > 1 ? words[1] : "";
        string attribute = words.size() > 2 ? words[2] : "";
        string attrValue = words.size() > 3 ? words[3] : "";

        bool listAll(attribute.empty());

        std::unique_ptr<MegaNode> node (nodebypath(nodePath.c_str()) );
        if (!node)
        {
            setCurrentThreadOutCode(MCMD_NOTFOUND);
            LOG_err << "Couldn't find node: " << nodePath;
            return;
        }

        auto isOfficial = [](const std::string &what)
        {
            for ( auto &a : {"s4"})
            {
                if (what == a)
                {
                    return true;
                }
            }
            return false;
        };

        if (settingattr || removingAttr)
        {
            if (attribute.empty())
            {
                setCurrentThreadOutCode(MCMD_EARGS);
                LOG_err << "Attribute not specified";
                LOG_err << "      " << getUsageStr(words[0].c_str());
                return;
            }

            auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
            if (forceCustom || !isOfficial(attribute))
            {
                const char *cattrValue = removingAttr ? NULL : attrValue.c_str();
                api->setCustomNodeAttribute(node.get(), attribute.c_str(), cattrValue, megaCmdListener.get());
            }
            else if (attribute == "s4")
            {
                const char *cattrValue = removingAttr ? NULL : attrValue.c_str();
                api->setNodeS4(node.get(), cattrValue, megaCmdListener.get());
            }
            else
            {
                assert(false);
                LOG_err << "Not implemented official attribute support";
                setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                return;
            }

            megaCmdListener->wait();
            if (checkNoErrors(megaCmdListener->getError(), "set node attribute: " + attribute))
            {
                OUTSTREAM << "Node attribute " << attribute << ( removingAttr ? " removed" : " updated" ) << " correctly" << endl;
                //reload the node for printing updated values
                node.reset(api->getNodeByHandle(megaCmdListener->getRequest()->getNodeHandle()));
                if (!node)
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << "Couldn't find node after updating its attribute: " << nodePath;
                    return;
                }
            }
        }

        //List node custom attributes
        std::unique_ptr<MegaStringList> attrlist (node->getCustomAttrNames());
        if (attrlist)
        {
            if (listAll)
            {
                OUTSTREAM << "The node has " << attrlist->size() << " custom attributes:" << endl;
            }
            for (int a = 0; a < attrlist->size(); a++)
            {
                string iattr = attrlist->get(a);
                if (listAll || ( attribute == iattr && (forceCustom || !isOfficial(iattr))))
                {
                    const char* iattrval = node->getCustomAttr(iattr.c_str());
                    if (getFlag(clflags, "print-only-value"))
                    {
                        OUTSTREAM << ( iattrval ? iattrval : "NULL" ) << endl;
                    }
                    else
                    {
                        OUTSTREAM << "\t" << iattr << " = " << ( iattrval ? iattrval : "NULL" ) << endl;
                    }
                }
            }
        }

        // List official node attributes:
        if (listAll || !forceCustom) //otherwise no sense in listing official ones
        {
            bool showOficialsHeader(listAll);
            using NameGetter = std::pair<const char *, std::function<const char *(MegaNode *)>>;
            for (auto & pair : {
                 NameGetter{"s4", [](MegaNode *node){return node->getS4();}}
                })
            {
                if (!listAll && ( attribute != pair.first ))
                {
                    continue; // looking for another one
                }
                auto officialAttrValue = pair.second(node.get());

                if (officialAttrValue && strlen(officialAttrValue))
                {
                    if (showOficialsHeader)
                    {
                        OUTSTREAM << "Official attributes:" << endl;
                        showOficialsHeader = false;
                    }
                    if (getFlag(clflags, "print-only-value"))
                    {
                        OUTSTREAM << ( pair.second(node.get()) ) << endl;
                    }
                    else
                    {
                        OUTSTREAM << "\t" << pair.first << " = " << pair.second(node.get()) << endl;
                    }
                }
            }
        }
        return;
    }
    else if (words[0] == "userattr")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        bool settingattr = getFlag(clflags, "s");
        bool listattrs = getFlag(clflags, "list");
        bool listall = words.size() == 1;

        int attribute = api->userAttributeFromString(words.size() > 1 ? words[1].c_str() : "-1");
        string attrValue = words.size() > 2 ? words[2] : "";
        string user = getOption(cloptions, "user", "");
        if (settingattr && user.size())
        {
            LOG_err << "Can't change other user attributes";
            return;
        }


        if (settingattr)
        {
            if (attribute != -1)
            {
                const char *catrn = api->userAttributeToString(attribute);
                string attrname = catrn;
                delete [] catrn;

                MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                api->setUserAttribute(attribute, attrValue.c_str(), megaCmdListener);
                megaCmdListener->wait();
                if (checkNoErrors(megaCmdListener->getError(), string("set user attribute ") + attrname))
                {
                    OUTSTREAM << "User attribute " << attrname << " updated correctly" << endl;
                    printUserAttribute(attribute, user, listattrs);
                }
                delete megaCmdListener;
            }
            else
            {
                setCurrentThreadOutCode(MCMD_EARGS);
                LOG_err << "Attribute not specified";
                LOG_err << "      " << getUsageStr("userattr");
                return;
            }
        }
        else // list
        {
            if (listall)
            {
                int a = 0;
                bool found = false;
                do
                {
                    found = printUserAttribute(a, user, listattrs);
                    a++;
                } while (found && listall);
            }
            else
            {
                if (!printUserAttribute(attribute, user, listattrs))
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << "Attribute not found: " << words[1];
                }
            }
        }

        return;
    }
    else if (words[0] == "thumbnail")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 1)
        {
            string nodepath = words[1];
            string path = words.size() > 2 ? words[2] : "./";
            std::unique_ptr<MegaNode> n = nodebypath(nodepath.c_str());
            if (n)
            {
                MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                bool setting = getFlag(clflags, "s");
                if (setting)
                {
                    api->setThumbnail(n.get(), path.c_str(), megaCmdListener);
                }
                else
                {
                    api->getThumbnail(n.get(), path.c_str(), megaCmdListener);
                }
                megaCmdListener->wait();
                if (checkNoErrors(megaCmdListener->getError(), ( setting ? "set thumbnail " : "get thumbnail " ) + nodepath + " to " + path))
                {
                    OUTSTREAM << "Thumbnail for " << nodepath << ( setting ? " loaded from " : " saved in " ) << megaCmdListener->getRequest()->getFile() << endl;
                }
                delete megaCmdListener;
            }
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr(words[0].c_str());
            return;
        }
        return;
    }
    else if (words[0] == "preview")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 1)
        {
            string nodepath = words[1];
            string path = words.size() > 2 ? words[2] : "./";
            std::unique_ptr<MegaNode> n = nodebypath(nodepath.c_str());
            if (n)
            {
                MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                bool setting = getFlag(clflags, "s");
                if (setting)
                {
                    api->setPreview(n.get(), path.c_str(), megaCmdListener);
                }
                else
                {
                    api->getPreview(n.get(), path.c_str(), megaCmdListener);
                }
                megaCmdListener->wait();
                if (checkNoErrors(megaCmdListener->getError(), ( setting ? "set preview " : "get preview " ) + nodepath + " to " + path))
                {
                    OUTSTREAM << "Preview for " << nodepath << ( setting ? " loaded from " : " saved in " ) << megaCmdListener->getRequest()->getFile() << endl;
                }
                delete megaCmdListener;
            }
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr(words[0].c_str());
            return;
        }
        return;
    }
    else if (words[0] == "tree")
    {
        vector<string> newcom;
        newcom.push_back("ls");
        (*clflags)["tree"] = 1;
        if (words.size()>1)
        {
            for (vector<string>::iterator it = ++words.begin(); it != words.end();it++)
            {
                newcom.push_back(*it);
            }
        }

        return executecommand(newcom, clflags, cloptions);
    }
    else if (words[0] == "debug")
    {
        vector<string> newcom;
        newcom.push_back("log");
        newcom.push_back("5");

        return executecommand(newcom, clflags, cloptions);
    }
    else if (words[0] == "passwd")
    {
        string confirmationQuery("Changing password will close all your sessions. Are you sure?(Yes/No): ");
        bool force = getFlag(clflags, "f");
        int confirmationResponse = force?MCMDCONFIRM_ALL:askforConfirmation(confirmationQuery);
        if (confirmationResponse != MCMDCONFIRM_YES && confirmationResponse != MCMDCONFIRM_ALL)
        {
            return;
        }

        if (api->isLoggedIn())
        {
            if (words.size() == 1)
            {
                if (isCurrentThreadInteractive())
                {
                    setprompt(NEWPASSWORD);
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_EARGS);
                    LOG_err << "Extra args required in non-interactive mode. Usage: " << getUsageStr("passwd");
                }
            }
            else if (words.size() == 2)
            {
                string pin2fa = getOption(cloptions, "auth-code", "");
                changePassword(words[1].c_str(), pin2fa);
            }
            else
            {
                setCurrentThreadOutCode(MCMD_EARGS);
                LOG_err << "      " << getUsageStr("passwd");
            }
        }
        else
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
        }

        return;
    }
    else if (words[0] == "speedlimit")
    {
        bool uploadSpeed = getFlag(clflags, "u");
        bool downloadSpeed = getFlag(clflags, "d");
        bool uploadCons = getFlag(clflags, "upload-connections");
        bool downloadCons = getFlag(clflags, "download-connections");

        bool moreThanOne = !onlyZeroOrOneOf(uploadSpeed, downloadSpeed, uploadCons, downloadCons);
        bool noParam = onlyZeroOf(uploadSpeed, downloadSpeed, uploadCons, downloadCons);
        bool hr = getFlag(clflags,"h");

        if (words.size() > 2 || moreThanOne)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("speedlimit");
            return;
        }
        if (words.size() > 1) // setting
        {
            long long value = textToSize(words[1].c_str());
            if (value == -1)
            {
                string s = words[1] + "B";
                value = textToSize(s.c_str());
            }
            if (noParam)
            {
                api->setMaxDownloadSpeed(value);
                api->setMaxUploadSpeed(value);
                ConfigurationManager::savePropertyValue("maxspeedupload", value);
                ConfigurationManager::savePropertyValue("maxspeeddownload", value);
            }
            else if (uploadSpeed)
            {
                api->setMaxUploadSpeed(value);
                ConfigurationManager::savePropertyValue("maxspeedupload", value);
            }
            else if (downloadSpeed)
            {
                api->setMaxDownloadSpeed(value);
                ConfigurationManager::savePropertyValue("maxspeeddownload", value);
            }
            else if (uploadCons || downloadCons)
            {
                auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
                api->setMaxConnections(uploadCons ? 1 : 0, value, megaCmdListener.get());
                if (!checkNoErrors(megaCmdListener.get(), uploadCons ? "change max upload connections" : "change max download connections"))
                {
                    return;
                }

                ConfigurationManager::savePropertyValue(uploadCons ? "maxuploadconnections" : "maxdownloadconnections", value);
            }
        }

        // listing:
        if (noParam)
        {
            long long us = api->getMaxUploadSpeed();
            long long ds = api->getMaxDownloadSpeed();
            OUTSTREAM << "Upload speed limit = " << (us?sizeToText(us,false,hr):"unlimited") << ((us && hr)?"/s":(us?" B/s":""))  << endl;
            OUTSTREAM << "Download speed limit = " << (ds?sizeToText(ds,false,hr):"unlimited") << ((ds && hr)?"/s":(us?" B/s":"")) << endl;
            auto upConns =  ConfigurationManager::getConfigurationValue("maxuploadconnections", -1);
            auto downConns =  ConfigurationManager::getConfigurationValue("maxdownloadconnections", -1);
            if (upConns != -1)
            {
                 OUTSTREAM << "Upload max connections = " << upConns << std::endl;
            }
            if (downConns != -1)
            {
                 OUTSTREAM << "Download max connections = " << downConns << std::endl;;
            }
        }
        else if (uploadSpeed)
        {
            long long us = api->getMaxUploadSpeed();
            OUTSTREAM << "Upload speed limit = " << (us?sizeToText(us,false,hr):"unlimited") << ((us && hr)?"/s":(us?" B/s":"")) << endl;
        }
        else if (downloadSpeed)
        {
            long long ds = api->getMaxDownloadSpeed();
            OUTSTREAM << "Download speed limit = " << (ds?sizeToText(ds,false,hr):"unlimited") << ((ds && hr)?"/s":(ds?" B/s":"")) << endl;
        }
        else if (uploadCons || downloadCons)
        {
            if (uploadCons)
            {
                auto upConns =  ConfigurationManager::getConfigurationValue("maxuploadconnections", -1);
                if (upConns != -1)
                {
                    OUTSTREAM << "Upload max connections = " << upConns << std::endl;;
                }
            }
            else
            {
                auto downConns =  ConfigurationManager::getConfigurationValue("maxdownloadconnections", -1);

                if (downConns != -1)
                {
                    OUTSTREAM << "Download max connections = " << downConns << std::endl;;
                }
            }
        }
        return;
    }
    else if (words[0] == "invite")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 1)
        {
            string email = words[1];
            if (!isValidEmail(email))
            {
                setCurrentThreadOutCode(MCMD_INVALIDEMAIL);
                LOG_err << "No valid email provided";
                LOG_err << "      " << getUsageStr("invite");
            }
            else
            {
                int action = MegaContactRequest::INVITE_ACTION_ADD;
                if (getFlag(clflags, "d"))
                {
                    action = MegaContactRequest::INVITE_ACTION_DELETE;
                }
                if (getFlag(clflags, "r"))
                {
                    action = MegaContactRequest::INVITE_ACTION_REMIND;
                }

                string message = getOption(cloptions, "message", "");
                MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                api->inviteContact(email.c_str(), message.c_str(), action, megaCmdListener);
                megaCmdListener->wait();
                if (checkNoErrors(megaCmdListener->getError(), action==MegaContactRequest::INVITE_ACTION_DELETE?"remove invitation":"(re)invite user"))
                {
                    OUTSTREAM << "Invitation to user: " << email << " " << (action==MegaContactRequest::INVITE_ACTION_DELETE?"removed":"sent") << endl;
                }
                else if (megaCmdListener->getError()->getErrorCode() == MegaError::API_EACCESS)
                {
                    ostringstream os;
                    os << "Reminder not yet available: " << " available after 15 days";
                    MegaContactRequestList *ocrl = api->getOutgoingContactRequests();
                    if (ocrl)
                    {
                        for (int i = 0; i < ocrl->size(); i++)
                        {
                            if (ocrl->get(i)->getTargetEmail() && megaCmdListener->getRequest()->getEmail() && !strcmp(ocrl->get(i)->getTargetEmail(), megaCmdListener->getRequest()->getEmail()))
                            {
                                os << " (" << getReadableTime(getTimeStampAfter(ocrl->get(i)->getModificationTime(), "15d")) << ")";
                            }
                        }

                        delete ocrl;
                    }
                    LOG_err << os.str();
                }
                delete megaCmdListener;
            }
        }

        return;
    }
    else if (words[0] == "errorcode")
    {
        if (words.size() != 2)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("errorcode");
            return;
        }

        int errCode = -1999;
        istringstream is(words[1]);
        is >> errCode;
        if (errCode == -1999)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Invalid error code: " << words[1];
            return;
        }

        if (errCode > 0)
        {
            errCode = -errCode;
        }

        string errString = getMCMDErrorString(errCode);
        if (errString == "UNKNOWN")
        {
            errString = MegaError::getErrorString(errCode);
        }

        OUTSTREAM << errString << endl;

    }
    else if (words[0] == "signup")
    {
        if (api->isLoggedIn())
        {
            setCurrentThreadOutCode(MCMD_INVALIDSTATE);
            LOG_err << "Please loggout first ";
        }
        else if (words.size() > 1)
        {
            string email = words[1];
            string defaultName = email;
            replaceAll(defaultName, "@"," ");

            if (words.size() > 2)
            {
                string name = getOption(cloptions, "name", defaultName);
                string passwd = words[2];
                signup(name, passwd, email);
            }
            else
            {
                login = words[1];
                name = getOption(cloptions, "name", defaultName);
                signingup = true;
                if (isCurrentThreadInteractive())
                {
                    setprompt(NEWPASSWORD);
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_EARGS);
                    LOG_err << "Extra args required in non-interactive mode. Usage: " << getUsageStr("signup");
                }
            }
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("signup");
        }

        return;
    }
    else if (words[0] == "whoami")
    {
        MegaUser *u = api->getMyUser();
        if (u)
        {
            OUTSTREAM << "Account e-mail: " << u->getEmail() << endl;
            if (getFlag(clflags, "l"))
            {
                std::unique_ptr<::mega::MegaAccountDetails> storageDetails;
                std::unique_ptr<::mega::MegaAccountDetails> extAccountDetails;
                {
                    auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
                    api->getAccountDetails(megaCmdListener.get());
                    megaCmdListener->wait();
                    if (checkNoErrors(megaCmdListener->getError(), "failed to get account details"))
                    {
                        storageDetails = std::unique_ptr<::mega::MegaAccountDetails>(megaCmdListener->getRequest()->getMegaAccountDetails());
                    }
                }
                {
                    auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
                    api->getExtendedAccountDetails(true, true, true, megaCmdListener.get());
                    megaCmdListener->wait();
                    if (checkNoErrors(megaCmdListener->getError(), "get extended account details"))
                    {
                        extAccountDetails = std::unique_ptr<::mega::MegaAccountDetails>(megaCmdListener->getRequest()->getMegaAccountDetails());
                    }
                }

                actUponGetExtendedAccountDetails(std::move(storageDetails), std::move(extAccountDetails));
            }
            delete u;
        }
        else
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
        }

        return;
    }
    else if (words[0] == "df")
    {
        bool humanreadable = getFlag(clflags, "h");
        MegaUser *u = api->getMyUser();
        if (u)
        {
            MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
            api->getSpecificAccountDetails(true, false, false, -1, megaCmdListener);
            megaCmdListener->wait();
            if (checkNoErrors(megaCmdListener->getError(), "failed to get used storage"))
            {
                unique_ptr<MegaAccountDetails> details(megaCmdListener->getRequest()->getMegaAccountDetails());
                if (details)
                {
                    long long usedTotal = 0;
                    long long rootStorage = 0;
                    long long inboxStorage = 0;
                    long long rubbishStorage = 0;
                    long long insharesStorage = 0;

                    long long storageMax = details->getStorageMax();

                    unique_ptr<MegaNode> root(api->getRootNode());
                    unique_ptr<MegaNode> inbox(api->getVaultNode());
                    unique_ptr<MegaNode> rubbish(api->getRubbishNode());
                    unique_ptr<MegaNodeList> inShares(api->getInShares());

                    if (!root || !inbox || !rubbish)
                    {
                        LOG_err << " Error retrieving storage details. Root node missing";
                        return;
                    }

                    MegaHandle rootHandle = root->getHandle();
                    MegaHandle inboxHandle = inbox->getHandle();
                    MegaHandle rubbishHandle = rubbish->getHandle();

                    rootStorage = details->getStorageUsed(rootHandle);
                    OUTSTREAM << "Cloud drive:          "
                              << getFixLengthString(sizeToText(rootStorage, true, humanreadable), 12, ' ', true) << " in "
                              << getFixLengthString(SSTR(details->getNumFiles(rootHandle)),7,' ',true) << " file(s) and "
                              << getFixLengthString(SSTR(details->getNumFolders(rootHandle)),7,' ',true) << " folder(s)" << endl;


                    inboxStorage = details->getStorageUsed(inboxHandle);
                    OUTSTREAM << "Inbox:                "
                              << getFixLengthString(sizeToText(inboxStorage, true, humanreadable), 12, ' ', true ) << " in "
                              << getFixLengthString(SSTR(details->getNumFiles(inboxHandle)),7,' ',true) << " file(s) and "
                              << getFixLengthString(SSTR(details->getNumFolders(inboxHandle)),7,' ',true) << " folder(s)" << endl;

                    rubbishStorage = details->getStorageUsed(rubbishHandle);
                    OUTSTREAM << "Rubbish bin:          "
                              << getFixLengthString(sizeToText(rubbishStorage, true, humanreadable), 12, ' ', true) << " in "
                              << getFixLengthString(SSTR(details->getNumFiles(rubbishHandle)),7,' ',true) << " file(s) and "
                              << getFixLengthString(SSTR(details->getNumFolders(rubbishHandle)),7,' ',true) << " folder(s)" << endl;


                    if (inShares)
                    {
                        for (int i = 0; i < inShares->size(); i++)
                        {
                            MegaNode *n = inShares->get(i);
                            long long thisinshareStorage = details->getStorageUsed(n->getHandle());
                            insharesStorage += thisinshareStorage;
                            if (i == 0)
                            {
                                OUTSTREAM << "Incoming shares:" << endl;
                            }

                            string name = n->getName();
                            name += ": ";
                            name.append(max(0, int (21 - name.size())), ' ');

                            OUTSTREAM << " " << name
                                      << getFixLengthString(sizeToText(thisinshareStorage, true, humanreadable), 12, ' ', true) << " in "
                                      << getFixLengthString(SSTR(details->getNumFiles(n->getHandle())),7,' ',true) << " file(s) and "
                                      << getFixLengthString(SSTR(details->getNumFolders(n->getHandle())),7,' ',true) << " folder(s)" << endl;
                        }
                    }

                    usedTotal = sandboxCMD->receivedStorageSum;

                    float percent = float(usedTotal * 1.0 / storageMax);
                    if (percent < 0 ) percent = 0;

                    string sof= percentageToText(percent);
                    sof +=  " of ";
                    sof +=  sizeToText(storageMax, true, humanreadable);

                    for (int i = 0; i < 75 ; i++)
                    {
                        OUTSTREAM << "-";
                    }
                    OUTSTREAM << endl;

                    OUTSTREAM << "USED STORAGE:         " << getFixLengthString(sizeToText(usedTotal, true, humanreadable), 12, ' ', true)
                              << "  " << getFixLengthString(sof, 39, ' ', true) << endl;

                    for (int i = 0; i < 75 ; i++)
                    {
                        OUTSTREAM << "-";
                    }
                    OUTSTREAM << endl;

                    long long usedinVersions = details->getVersionStorageUsed(rootHandle)
                            + details->getVersionStorageUsed(inboxHandle)
                            + details->getVersionStorageUsed(rubbishHandle);


                    OUTSTREAM << "Total size taken up by file versions: "
                              << getFixLengthString(sizeToText(usedinVersions, true, humanreadable), 12, ' ', true) << endl;
                }
            }
            delete megaCmdListener;
            delete u;
        }
        else
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
        }

        return;
    }
    else if (words[0] == "export")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        ::mega::m_time_t expireTime = 0;
        string sexpireTime = getOption(cloptions, "expire", "");
        if ("" != sexpireTime)
        {
            expireTime = getTimeStampAfter(sexpireTime);
        }
        if (expireTime < 0)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Invalid time " << sexpireTime;
            return;
        }

        const bool add = getFlag(clflags, "a");

        auto passwordOpt = getOptionAsOptional(*cloptions, "password");

        // When the user passes "--password" without "=" it gets treated as a flag, so
        // it's inserted into `clflags`. We'll treat this as hasPassword=true as well
        // to ensure we log the "password is empty" error to the user.
        if (!passwordOpt && getFlag(clflags, "password"))
        {
            passwordOpt = "";
        }

        if (!add && (passwordOpt || expireTime > 0 || getFlag(clflags, "f") || getFlag(clflags, "writable") || getFlag(clflags, "mega-hosted")))
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Option can only be used when adding an export (with -a)";
            LOG_err << "Usage: " << getUsageStr("export");
            return;
        }

        // This will be true for '--password', '--password=', and '--password=""'
        // Note: --password='' will use the '' string as the actual password
        if (passwordOpt && passwordOpt->empty())
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Password cannot be empty";
            return;
        }

        if (words.size() <= 1)
        {
            LOG_warn << "No file/folder argument provided, will export the current working folder";
            words.push_back(string("."));
        }

        for (size_t i = 1; i < words.size(); i++)
        {
            unescapeifRequired(words[i]);
            if (isRegExp(words[i]))
            {
                vector<std::unique_ptr<MegaNode>> nodes = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                if (nodes.empty())
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << "Nodes not found: " << words[i];
                }

                for (const auto& n : nodes)
                {
                    assert(n);

                    if (add)
                    {
                        if (!n->isExported())
                        {
                            LOG_debug << " exporting ... " << n->getName() << " expireTime=" << expireTime;
                            exportNode(n.get(), expireTime, passwordOpt, clflags, cloptions);
                        }
                        else
                        {
                            setCurrentThreadOutCode(MCMD_EXISTS);
                            LOG_err << "Node " << words[i] << " is already exported. "
                                    << "Use -d to delete it if you want to change its parameters. Note: the new link may differ";
                        }
                    }
                    else if (getFlag(clflags, "d"))
                    {
                        LOG_debug << " deleting export ... " << n->getName();
                        disableExport(n.get());
                    }
                    else
                    {
                        int exportedCount = dumpListOfExported(n.get(), getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, words[i]);
                        if (exportedCount == 0)
                        {
                            setCurrentThreadOutCode(MCMD_NOTFOUND);
                            OUTSTREAM << words[i] << " is not exported. Use -a to export it" << endl;
                        }
                    }
                }
            }
            else
            {
                std::unique_ptr<MegaNode> n = nodebypath(words[i].c_str());
                if (n)
                {
                    auto nodeName = [CONST_CAPTURE(words), i] { return (words[i] == "." ? "current folder" : "<" + words[i] + ">"); };

                    if (add)
                    {
                        if (!n->isExported())
                        {
                            LOG_debug << " exporting ... " << n->getName();
                            exportNode(n.get(), expireTime, passwordOpt, clflags, cloptions);
                        }
                        else
                        {
                            setCurrentThreadOutCode(MCMD_EXISTS);
                            LOG_err << nodeName() << " is already exported. "
                                    << "Use -d to delete it if you want to change its parameters. Note: the new link may differ";
                        }
                    }
                    else if (getFlag(clflags, "d"))
                    {
                        LOG_debug << " deleting export ... " << n->getName();
                        disableExport(n.get());
                    }
                    else
                    {
                        int exportedCount = dumpListOfExported(n.get(), getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, words[i]);
                        if (exportedCount == 0)
                        {
                            setCurrentThreadOutCode(MCMD_NOTFOUND);
                            OUTSTREAM << "Couldn't find anything exported below " << nodeName()
                                      << ". Use -a to export " << (words[i].size() ? "it" : "something") << endl;
                        }
                    }
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << "Node not found: " << words[i];
                }
            }
        }
    }
    else if (words[0] == "import")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        string remotePath = "";
        std::unique_ptr<MegaNode> dstFolder;

        if (words.size() > 1) //link
        {
            if (isPublicLink(words[1]))
            {
                string publicLink = words[1];
                if (!decryptLinkIfEncrypted(api, publicLink, cloptions))
                {
                    return;
                }

                if (words.size() > 2)
                {
                    remotePath = words[2];
                    dstFolder = nodebypath(remotePath.c_str());
                }
                else
                {
                    dstFolder.reset(api->getNodeByHandle(cwd));
                    remotePath = "."; //just to inform (alt: getpathbynode)
                }
                if (dstFolder && (!dstFolder->getType() == MegaNode::TYPE_FILE))
                {
                    if (getLinkType(publicLink) == MegaNode::TYPE_FILE)
                    {
                        MegaCmdListener *megaCmdListener = new MegaCmdListener(nullptr);
                        api->importFileLink(publicLink.c_str(), dstFolder.get(), megaCmdListener);
                        megaCmdListener->wait();

                        if (checkNoErrors(megaCmdListener->getError(), "import node"))
                        {
                            MegaNode *imported = api->getNodeByHandle(megaCmdListener->getRequest()->getNodeHandle());
                            if (imported)
                            {
                                char *importedPath = api->getNodePath(imported);
                                OUTSTREAM << "Import file complete: " << importedPath << endl;
                                delete []importedPath;
                            }
                            else
                            {
                                LOG_warn << "Import file complete: Couldn't get path of imported file";
                            }
                            delete imported;
                        }

                        delete megaCmdListener;
                    }
                    else if (getLinkType(publicLink) == MegaNode::TYPE_FOLDER)
                    {
                        MegaApi* apiFolder = getFreeApiFolder();
                        if (!apiFolder)
                        {
                            setCurrentThreadOutCode(MCMD_NOTFOUND);
                            LOG_err << "No available Api folder. Use configure to increase exported_folders_sdks";
                            return;
                        }
                        char *accountAuth = api->getAccountAuth();
                        apiFolder->setAccountAuth(accountAuth);
                        delete []accountAuth;

                        MegaCmdListener *megaCmdListener = new MegaCmdListener(apiFolder, NULL);
                        apiFolder->loginToFolder(publicLink.c_str(), megaCmdListener);
                        megaCmdListener->wait();
                        if (checkNoErrors(megaCmdListener->getError(), "login to folder"))
                        {
                            MegaCmdListener *megaCmdListener2 = new MegaCmdListener(apiFolder, NULL);
                            apiFolder->fetchNodes(megaCmdListener2);
                            megaCmdListener2->wait();
                            if (checkNoErrors(megaCmdListener2->getError(), "access folder link " + publicLink))
                            {
                                MegaNode *nodeToImport = NULL;
                                bool usedRoot = false;
                                string shandle = getPublicLinkHandle(publicLink);
                                if (shandle.size())
                                {
                                    handle thehandle = apiFolder->base64ToHandle(shandle.c_str());
                                    nodeToImport = apiFolder->getNodeByHandle(thehandle);
                                }
                                else
                                {
                                    nodeToImport = apiFolder->getRootNode();
                                    usedRoot = true;
                                }

                                if (nodeToImport)
                                {
                                    MegaNode *authorizedNode = apiFolder->authorizeNode(nodeToImport);
                                    if (authorizedNode != NULL)
                                    {
                                        MegaCmdListener *megaCmdListener3 = new MegaCmdListener(apiFolder, NULL);
                                        api->copyNode(authorizedNode, dstFolder.get(), megaCmdListener3);
                                        megaCmdListener3->wait();

                                        if (checkNoErrors(megaCmdListener->getError(), "import folder node"))
                                        {
                                            MegaNode *importedFolderNode = api->getNodeByHandle(megaCmdListener3->getRequest()->getNodeHandle());
                                            char *pathnewFolder = api->getNodePath(importedFolderNode);
                                            if (pathnewFolder)
                                            {
                                                OUTSTREAM << "Imported folder complete: " << pathnewFolder << endl;
                                                delete []pathnewFolder;
                                            }
                                            delete importedFolderNode;
                                        }
                                        delete megaCmdListener3;
                                        delete authorizedNode;
                                    }
                                    else
                                    {
                                        setCurrentThreadOutCode(MCMD_EUNEXPECTED);
                                        LOG_debug << "Node couldn't be authorized: " << publicLink;
                                    }
                                    delete nodeToImport;
                                }
                                else
                                {
                                    setCurrentThreadOutCode(MCMD_INVALIDSTATE);
                                    if (usedRoot)
                                    {
                                        LOG_err << "Couldn't get root folder for folder link";
                                    }
                                    else
                                    {
                                        LOG_err << "Failed to get node corresponding to handle within public link " << shandle;
                                    }
                                }
                            }

                            {
                                std::unique_ptr<MegaCmdListener> megaCmdListener(new MegaCmdListener(apiFolder));
                                apiFolder->logout(false, megaCmdListener.get());
                                megaCmdListener->wait();
                                if (megaCmdListener->getError()->getErrorCode() != MegaError::API_OK)
                                {
                                    LOG_err << "Couldn't logout from apiFolder";
                                }
                            }
                            delete megaCmdListener2;
                        }
                        delete megaCmdListener;
                        freeApiFolder(apiFolder);
                    }
                    else
                    {
                        setCurrentThreadOutCode(MCMD_EARGS);
                        LOG_err << "Invalid link: " << publicLink;
                        LOG_err << "      " << getUsageStr("import");
                    }
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                    LOG_err << "Invalid destiny: " << remotePath;
                }
            }
            else
            {
                setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                LOG_err << "Invalid link: " << words[1];
            }
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("import");
        }

        return;
    }
    else if (words[0] == "reload")
    {
        int clientID = getintOption(cloptions, "clientID", -1);

        OUTSTREAM << "Reloading account..." << endl;
        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL, NULL, clientID);
        sandboxCMD->mNodesCurrentPromise.initiatePromise();
        api->fetchNodes(megaCmdListener);
        actUponFetchNodes(api, megaCmdListener);
        delete megaCmdListener;
        return;
    }
    else if (words[0] == "logout")
    {
        OUTSTREAM << "Logging out..." << endl;
        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
        bool keepSession = getFlag(clflags, "keep-session");
        char * dumpSession = NULL;

        if (keepSession) //local logout
        {
            dumpSession = api->dumpSession();
            api->localLogout(megaCmdListener);
        }
        else
        {
            api->logout(false, megaCmdListener);
        }
        actUponLogout(megaCmdListener, keepSession);
        if (keepSession)
        {
            OUTSTREAM << "Session closed but not deleted. Warning: it will be restored the next time you execute the application. Execute \"logout\" to delete the session permanently." << endl;

            if (dumpSession)
            {
                OUTSTREAM << "You can also login with the session id: " << dumpSession << endl;
                delete []dumpSession;
            }
        }
        delete megaCmdListener;

        return;
    }
    else if (words[0] == "confirm")
    {
        if (getFlag(clflags, "security"))
        {
            auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
            api->upgradeSecurity(megaCmdListener.get());
            megaCmdListener->wait();
            if (checkNoErrors(megaCmdListener->getError(), "confirm security upgrade"))
            {
                removeGreetingMatching("Your account's security needs upgrading");
                OUTSTREAM << "Account security upgrade. You shall no longer see an account security warning." << endl;
            }
            return;
        }

        if (words.size() > 2)
        {
            string link = words[1];
            string email = words[2];
            // check email corresponds with link:
            MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
            api->querySignupLink(link.c_str(), megaCmdListener);
            megaCmdListener->wait();
            if (checkNoErrors(megaCmdListener->getError(), "check email corresponds to link"))
            {
                if (megaCmdListener->getRequest()->getFlag())
                {
                    OUTSTREAM << "Account " << email << " confirmed successfully. You can login with it now" << endl;
                }
                else if (megaCmdListener->getRequest()->getEmail() && email == megaCmdListener->getRequest()->getEmail())
                {
                    string passwd;
                    if (words.size() > 3)
                    {
                        passwd = words[3];
                        confirm(passwd, email, link);
                    }
                    else
                    {
                        this->login = email;
                        this->link = link;
                        confirming = true;
                        if (isCurrentThreadInteractive() && !isCurrentThreadCmdShell())
                        {
                            setprompt(LOGINPASSWORD);
                        }
                        else
                        {
                            setCurrentThreadOutCode(MCMD_EARGS);
                            LOG_err << "Extra args required in non-interactive mode. Usage: " << getUsageStr("confirm");
                        }
                    }
                }
                else
                {
                    setCurrentThreadOutCode(MCMD_INVALIDEMAIL);
                    LOG_err << email << " doesn't correspond to the confirmation link: " << link;
                }
            }

            delete megaCmdListener;
        }
        else
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("confirm");
        }

        return;
    }
    else if (words[0] == "session")
    {
        char * dumpSession = api->dumpSession();
        if (dumpSession)
        {
            OUTSTREAM << "Your (secret) session is: " << dumpSession << endl;
            delete []dumpSession;
        }
        else
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
        }
        return;
    }
    else if (words[0] == "history")
    {
        return;
    }
    else if (words[0] == "version")
    {
        OUTSTREAM << "MEGAcmd version: " << MEGACMD_MAJOR_VERSION << "." << MEGACMD_MINOR_VERSION << "." << MEGACMD_MICRO_VERSION << "." << MEGACMD_BUILD_ID << ": code " << MEGACMD_CODE_VERSION
#ifdef _WIN64
                  << " (64 bits)"
#endif
                  << endl;

        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
        api->getLastAvailableVersion("BdARkQSQ",megaCmdListener);
        if (!megaCmdListener->trywait(2000))
        {
            if (!megaCmdListener->getError())
            {
                LOG_fatal << "No MegaError at getLastAvailableVersion: ";
            }
            else if (megaCmdListener->getError()->getErrorCode() != MegaError::API_OK)
            {
                LOG_debug << "Couldn't get latests available version: " << megaCmdListener->getError()->getErrorString();
            }
            else
            {
                if (megaCmdListener->getRequest()->getNumber() != MEGACMD_CODE_VERSION)
                {
                    OUTSTREAM << "---------------------------------------------------------------------" << endl;
                    OUTSTREAM << "--        There is a new version available of megacmd: " << getLeftAlignedStr(megaCmdListener->getRequest()->getName(), 12) << "--" << endl;
                    OUTSTREAM << "--        Please, download it from https://mega.nz/cmd             --" << endl;
#if defined(__APPLE__)
                    OUTSTREAM << "--        Before installing enter \"exit\" to close MEGAcmd          --" << endl;
#endif
                    OUTSTREAM << "---------------------------------------------------------------------" << endl;
                }
            }
            delete megaCmdListener;
        }
        else
        {
            LOG_debug << "Couldn't get latests available version (petition timed out)";

            api->removeRequestListener(megaCmdListener);
            delete megaCmdListener;
        }





        if (getFlag(clflags,"c"))
        {
            OUTSTREAM << "Changes in the current version:" << endl;
            string thechangelog = megacmdchangelog;
            if (thechangelog.size())
            {
                replaceAll(thechangelog,"\n","\n * ");
                OUTSTREAM << " * " << thechangelog << endl << endl;
            }

            OUTSTREAM << "Full changelog available at https://github.com/meganz/MEGAcmd/blob/master/build/megacmd/megacmd.changes" << endl << endl;
        }
        if (getFlag(clflags,"l"))
        {
            OUTSTREAM << "MEGA SDK version: " << SDK_COMMIT_HASH << endl;

            OUTSTREAM << "MEGA SDK Credits: https://github.com/meganz/sdk/blob/master/CREDITS.md" << endl;
            OUTSTREAM << "MEGA SDK License: https://github.com/meganz/sdk/blob/master/LICENSE" << endl;
            OUTSTREAM << "MEGAcmd License: https://github.com/meganz/megacmd/blob/master/LICENSE" << endl;
            OUTSTREAM << "MEGA Terms: https://mega.nz/terms" << endl;
            OUTSTREAM << "MEGA General Data Protection Regulation Disclosure: https://mega.nz/gdpr" << endl;

            OUTSTREAM << "Features enabled:" << endl;

#ifdef USE_CRYPTOPP
            OUTSTREAM << "* CryptoPP" << endl;
#endif

#ifdef USE_SQLITE
          OUTSTREAM << "* SQLite" << endl;
#endif

#ifdef USE_BDB
            OUTSTREAM << "* Berkeley DB" << endl;
#endif

#ifdef USE_INOTIFY
           OUTSTREAM << "* inotify" << endl;
#endif

#ifdef HAVE_FDOPENDIR
           OUTSTREAM << "* fdopendir" << endl;
#endif

#ifdef HAVE_SENDFILE
            OUTSTREAM << "* sendfile" << endl;
#endif

#ifdef _LARGE_FILES
           OUTSTREAM << "* _LARGE_FILES" << endl;
#endif

#ifdef USE_FREEIMAGE
            OUTSTREAM << "* FreeImage" << endl;
#endif

#ifdef USE_PCRE
            OUTSTREAM << "* PCRE" << endl;
#endif

#ifdef ENABLE_SYNC
           OUTSTREAM << "* sync subsystem" << endl;
#endif
        }
        return;
    }
    else if (words[0] == "masterkey")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        OUTSTREAM << api->exportMasterKey() << endl;
        api->masterKeyExported(); //we do not wait for this to end
    }
    else if (words[0] == "showpcr")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        bool incoming = getFlag(clflags, "in");
        bool outgoing = getFlag(clflags, "out");

        if (!incoming && !outgoing)
        {
            incoming = true;
            outgoing = true;
        }

        if (outgoing)
        {
            MegaContactRequestList *ocrl = api->getOutgoingContactRequests();
            if (ocrl)
            {
                if (ocrl->size())
                {
                    OUTSTREAM << "Outgoing PCRs:" << endl;
                }
                for (int i = 0; i < ocrl->size(); i++)
                {
                    auto cr = ocrl->get(i);
                    OUTSTREAM << " " << getLeftAlignedStr(cr->getTargetEmail(),22);

                    char * sid = api->userHandleToBase64(cr->getHandle());

                    OUTSTREAM << "\t (id: " << sid << ", creation: " << getReadableTime(cr->getCreationTime(), getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")))
                              << ", modification: " << getReadableTime(cr->getModificationTime(), getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822"))) << ")";

                    delete[] sid;
                    OUTSTREAM << endl;
                }

                delete ocrl;
            }
        }

        if (incoming)
        {
            MegaContactRequestList *icrl = api->getIncomingContactRequests();
            if (icrl)
            {
                if (icrl->size())
                {
                    OUTSTREAM << "Incoming PCRs:" << endl;
                }

                for (int i = 0; i < icrl->size(); i++)
                {
                    auto cr = icrl->get(i);
                    OUTSTREAM << " " << getLeftAlignedStr(cr->getSourceEmail(), 22);

                    MegaHandle id = cr->getHandle();
                    char sid[12];
                    Base64::btoa((unsigned char*)&( id ), sizeof( id ), sid);

                    OUTSTREAM << "\t (id: " << sid << ", creation: " << getReadableTime(cr->getCreationTime(), getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")))
                              << ", modification: " << getReadableTime(cr->getModificationTime(), getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822"))) << ")";
                    if (cr->getSourceMessage())
                    {
                        OUTSTREAM << endl << "\t" << "Invitation message: " << cr->getSourceMessage();
                    }

                    OUTSTREAM << endl;
                }

                delete icrl;
            }
        }
        return;
    }
    else if (words[0] == "killsession")
    {
        bool all = getFlag(clflags, "a");

        if ((words.size() <= 1 && !all) || (all && words.size() > 1))
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("killsession");
            return;
        }
        string thesession;
        MegaHandle thehandle = UNDEF;

        if (all)
        {
            // Kill all sessions (except current)
            words.push_back("all");
        }

        for (unsigned int i = 1; i < words.size() ; i++)
        {
            thesession = words[i];
            if (thesession == "all")
            {
                thehandle = INVALID_HANDLE;
            }
            else
            {
                thehandle = api->base64ToUserHandle(thesession.c_str());
            }


            MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
            api->killSession(thehandle, megaCmdListener);
            megaCmdListener->wait();
            if (checkNoErrors(megaCmdListener->getError(), "kill session " + thesession + ". Maybe the session was not valid."))
            {
                if (thesession != "all")
                {
                    OUTSTREAM << "Session " << thesession << " killed successfully" << endl;
                }
                else
                {
                    OUTSTREAM << "All sessions killed successfully" << endl;
                }
            }

            delete megaCmdListener;
        }

        return;
    }
    else if (words[0] == "transfers")
    {
        bool showcompleted = getFlag(clflags, "show-completed");
        bool onlycompleted = getFlag(clflags, "only-completed");
        bool onlyuploads = getFlag(clflags, "only-uploads");
        bool onlydownloads = getFlag(clflags, "only-downloads");
        bool showsyncs = getFlag(clflags, "show-syncs");
        bool printsummary = getFlag(clflags, "summary");

        int PATHSIZE = getintOption(cloptions,"path-display-size");
        if (!PATHSIZE)
        {
            // get screen size for output purposes
            unsigned int width = getNumberOfCols(75);
            PATHSIZE = min(60,int((width-46)/2));
        }
        PATHSIZE = max(0, PATHSIZE);

        if (getFlag(clflags,"c"))
        {
            if (getFlag(clflags,"a"))
            {
                if (onlydownloads || (!onlyuploads && !onlydownloads) )
                {
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                    api->cancelTransfers(MegaTransfer::TYPE_DOWNLOAD, megaCmdListener);
                    megaCmdListener->wait();
                    if (checkNoErrors(megaCmdListener->getError(), "cancel all download transfers"))
                    {
                        OUTSTREAM << "Download transfers cancelled successfully." << endl;
                    }
                    delete megaCmdListener;
                }
                if (onlyuploads || (!onlyuploads && !onlydownloads) )
                {
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                    api->cancelTransfers(MegaTransfer::TYPE_UPLOAD, megaCmdListener);
                    megaCmdListener->wait();
                    if (checkNoErrors(megaCmdListener->getError(), "cancel all upload transfers"))
                    {
                        OUTSTREAM << "Upload transfers cancelled successfully." << endl;
                    }
                    delete megaCmdListener;
                }

            }
            else
            {
                if (words.size() < 2)
                {
                    setCurrentThreadOutCode(MCMD_EARGS);
                    LOG_err << "      " << getUsageStr("transfers");
                    return;
                }
                for (unsigned int i = 1; i < words.size(); i++)
                {
                    std::unique_ptr<MegaTransfer> transfer(api->getTransferByTag(toInteger(words[i],-1)));
                    if (transfer)
                    {
                        if (transfer->isSyncTransfer())
                        {
                            LOG_err << "Unable to cancel transfer with tag " << words[i] << ". Sync transfers cannot be cancelled";
                            setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                        }
                        else
                        {
                            MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                            api->cancelTransfer(transfer.get(), megaCmdListener);
                            megaCmdListener->wait();
                            if (checkNoErrors(megaCmdListener->getError(), "cancel transfer with tag " + words[i] + "."))
                            {
                                OUTSTREAM << "Transfer " << words[i]<< " cancelled successfully." << endl;
                            }
                            delete megaCmdListener;
                        }
                    }
                    else
                    {
                        LOG_err << "Could not find transfer with tag: " << words[i];
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                    }
                }
            }

            return;
        }

        if (getFlag(clflags,"p") || getFlag(clflags,"r"))
        {
            if (getFlag(clflags,"a"))
            {
                if (onlydownloads || (!onlyuploads && !onlydownloads) )
                {
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                    api->pauseTransfers(getFlag(clflags,"p"), MegaTransfer::TYPE_DOWNLOAD, megaCmdListener);
                    megaCmdListener->wait();
                    if (checkNoErrors(megaCmdListener->getError(), (getFlag(clflags,"p")?"pause all download transfers":"resume all download transfers")))
                    {
                        OUTSTREAM << "Download transfers "<< (getFlag(clflags,"p")?"pause":"resume") << "d successfully." << endl;
                    }
                    delete megaCmdListener;
                }
                if (onlyuploads || (!onlyuploads && !onlydownloads) )
                {
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                    api->pauseTransfers(getFlag(clflags,"p"), MegaTransfer::TYPE_UPLOAD, megaCmdListener);
                    megaCmdListener->wait();
                    if (checkNoErrors(megaCmdListener->getError(), (getFlag(clflags,"p")?"pause all download transfers":"resume all download transfers")))
                    {
                        OUTSTREAM << "Upload transfers "<< (getFlag(clflags,"p")?"pause":"resume") << "d successfully." << endl;
                    }
                    delete megaCmdListener;
                }

            }
            else
            {
                if (words.size() < 2)
                {
                    setCurrentThreadOutCode(MCMD_EARGS);
                    LOG_err << "      " << getUsageStr("transfers");
                    return;
                }
                for (unsigned int i = 1; i < words.size(); i++)
                {
                    std::unique_ptr<MegaTransfer> transfer(api->getTransferByTag(toInteger(words[i],-1)));
                    if (transfer)
                    {
                        if (transfer->isSyncTransfer())
                        {
                            LOG_err << "Unable to "<< (getFlag(clflags,"p")?"pause":"resume") << " transfer with tag " << words[i] << ". Sync transfers cannot be "<< (getFlag(clflags,"p")?"pause":"resume") << "d";
                            setCurrentThreadOutCode(MCMD_INVALIDTYPE);
                        }
                        else
                        {
                            MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                            api->pauseTransfer(transfer.get(), getFlag(clflags,"p"), megaCmdListener);
                            megaCmdListener->wait();
                            if (checkNoErrors(megaCmdListener->getError(), (getFlag(clflags,"p")?"pause transfer with tag ":"resume transfer with tag ") + words[i] + "."))
                            {
                                OUTSTREAM << "Transfer " << words[i]<< " "<< (getFlag(clflags,"p")?"pause":"resume") << "d successfully." << endl;
                            }
                            delete megaCmdListener;
                        }
                    }
                    else
                    {
                        LOG_err << "Could not find transfer with tag: " << words[i];
                        setCurrentThreadOutCode(MCMD_NOTFOUND);
                    }
                }
            }

            return;
        }

        //show transfers
        std::unique_ptr<MegaTransferData> transferdata(api->getTransferData());

        int ndownloads = transferdata->getNumDownloads();
        int nuploads = transferdata->getNumUploads();

        if (printsummary)
        {
            long long transferredDownload = 0, transferredUpload = 0, totalDownload = 0, totalUpload = 0;
            for (int i = 0; i< ndownloads; i++)
            {
                MegaTransfer *t = api->getTransferByTag(transferdata->getDownloadTag(i));
                {
                    if (t)
                    {
                        transferredDownload += t->getTransferredBytes();
                        totalDownload += t->getTotalBytes();
                        delete t;
                    }
                }
            }
            for (int i = 0; i< nuploads; i++)
            {
                MegaTransfer *t = api->getTransferByTag(transferdata->getUploadTag(i));
                {
                    if (t)
                    {
                        transferredUpload += t->getTransferredBytes();
                        totalUpload += t->getTotalBytes();
                        delete t;
                    }
                }
            }

            float percentDownload = !totalDownload?0:float(transferredDownload*1.0/totalDownload);
            float percentUpload = !totalUpload?0:float(transferredUpload*1.0/totalUpload);

            OUTSTREAM << getFixLengthString("NUM DOWNLOADS", 16, ' ', true);
            OUTSTREAM << getFixLengthString("DOWNLOADED", 12, ' ', true);
            OUTSTREAM << getFixLengthString("TOTAL", 12, ' ', true);
            OUTSTREAM << getFixLengthString("%   ", 8, ' ', true);
            OUTSTREAM << "     ";
            OUTSTREAM << getFixLengthString("NUM UPLOADS", 16, ' ', true);
            OUTSTREAM << getFixLengthString("UPLOADED", 12, ' ', true);
            OUTSTREAM << getFixLengthString("TOTAL", 12, ' ', true);
            OUTSTREAM << getFixLengthString("%   ", 8, ' ', true);
            OUTSTREAM << endl;

            OUTSTREAM << getFixLengthString(SSTR(ndownloads), 16, ' ', true);
            OUTSTREAM << getFixLengthString(sizeToText(transferredDownload), 12, ' ', true);
            OUTSTREAM << getFixLengthString(sizeToText(totalDownload), 12, ' ', true);
            OUTSTREAM << getFixLengthString(percentageToText(percentDownload),8,' ',true);

            OUTSTREAM << "     ";
            OUTSTREAM << getFixLengthString(SSTR(nuploads), 16, ' ', true);
            OUTSTREAM << getFixLengthString(sizeToText(transferredUpload), 12, ' ', true);
            OUTSTREAM << getFixLengthString(sizeToText(totalUpload), 12, ' ', true);
            OUTSTREAM << getFixLengthString(percentageToText(percentUpload),8,' ',true);
            OUTSTREAM << endl;
            return;
        }



        int limit = getintOption(cloptions, "limit", min(10,ndownloads+nuploads+(int)globalTransferListener->completedTransfers.size()));

        if (!transferdata)
        {
            setCurrentThreadOutCode(MCMD_EUNEXPECTED);
            LOG_err << "No transferdata.";
            return;
        }

        bool downloadpaused = api->areTransfersPaused(MegaTransfer::TYPE_DOWNLOAD);
        bool uploadpaused = api->areTransfersPaused(MegaTransfer::TYPE_UPLOAD);

        int indexUpload = 0;
        int indexDownload = 0;
        int shown = 0;

        int showndl = 0;
        int shownup = 0;
        unsigned int shownCompleted = 0;

        vector<MegaTransfer *> transfersDLToShow;
        vector<MegaTransfer *> transfersUPToShow;
        vector<MegaTransfer *> transfersCompletedToShow;

        if (showcompleted)
        {
            globalTransferListener->completedTransfersMutex.lock();
            size_t totalcompleted = globalTransferListener->completedTransfers.size();
            for (size_t i = 0;(i < totalcompleted)
                 && (shownCompleted < totalcompleted)
                 && (shownCompleted < (size_t)(limit+1)); //Note limit+1 to seek for one more to show if there are more to show!
                 i++)
            {
                MegaTransfer *transfer = globalTransferListener->completedTransfers.at(i);
                if (
                    (
                            (transfer->getType() == MegaTransfer::TYPE_UPLOAD && (onlyuploads || (!onlyuploads && !onlydownloads) ))
                        ||  (transfer->getType() == MegaTransfer::TYPE_DOWNLOAD && (onlydownloads || (!onlyuploads && !onlydownloads) ) )
                    )
                    &&  !(!showsyncs && transfer->isSyncTransfer())
                    )
                {

                    transfersCompletedToShow.push_back(transfer);
                    shownCompleted++;
                }
            }
            globalTransferListener->completedTransfersMutex.unlock();
        }

        shown += shownCompleted;

        while (!onlycompleted)
        {
            //MegaTransfer *transfer = transferdata->get(i);
            MegaTransfer *transfer = NULL;
            //Next transfer to show
            if (onlyuploads && !onlydownloads && indexUpload < transferdata->getNumUploads()) //Only uploads
            {
                transfer = api->getTransferByTag(transferdata->getUploadTag(indexUpload++));
            }
            else
            {
                if ( (!onlydownloads || (onlydownloads && onlyuploads)) //both
                     && ( (shown >= (limit/2) ) || indexDownload == ndownloads ) // /already chosen half slots for dls or no more dls
                     && indexUpload < transferdata->getNumUploads()
                     )
                    //This is not 100 % perfect, it could show with a limit of 10 5 downloads and 3 uploads with more downloads on the queue.
                {
                    transfer = api->getTransferByTag(transferdata->getUploadTag(indexUpload++));

                }
                else if(indexDownload < ndownloads)
                {
                    transfer =  api->getTransferByTag(transferdata->getDownloadTag(indexDownload++));
                }
            }

            if (!transfer) break; //finish

            if (
                    (showcompleted || transfer->getState() != MegaTransfer::STATE_COMPLETED)
                    &&  !(onlyuploads && transfer->getType() != MegaTransfer::TYPE_UPLOAD && !onlydownloads )
                    &&  !(onlydownloads && transfer->getType() != MegaTransfer::TYPE_DOWNLOAD && !onlyuploads )
                    &&  !(!showsyncs && transfer->isSyncTransfer())
                    &&  (shown < (limit+1)) //Note limit+1 to seek for one more to show if there are more to show!
                    )
            {
                shown++;
                if (transfer->getType() == MegaTransfer::TYPE_DOWNLOAD)
                {
                    transfersDLToShow.push_back(transfer);
                    showndl++;
                }
                else
                {
                    transfersUPToShow.push_back(transfer);
                    shownup++;
                }
            }
            else
            {
                delete transfer;
            }
            if (shown>limit || transfer == NULL) //we-re done
            {
                break;
            }
        }

        vector<MegaTransfer *>::iterator itCompleted = transfersCompletedToShow.begin();
        vector<MegaTransfer *>::iterator itDLs = transfersDLToShow.begin();
        vector<MegaTransfer *>::iterator itUPs = transfersUPToShow.begin();

        ColumnDisplayer cd(clflags, cloptions);
        cd.addHeader("SOURCEPATH", false);
        cd.addHeader("DESTINYPATH", false);

        for (unsigned int i=0;i<showndl+shownup+shownCompleted; i++)
        {
            MegaTransfer *transfer = NULL;
            bool deleteTransfer = true;
            if (itDLs == transfersDLToShow.end() && itCompleted == transfersCompletedToShow.end())
            {
                transfer = (MegaTransfer *) *itUPs;
                itUPs++;
            }
            else if (itCompleted == transfersCompletedToShow.end())
            {
                transfer = (MegaTransfer *) *itDLs;
                itDLs++;
            }
            else
            {
                transfer = (MegaTransfer *) *itCompleted;
                itCompleted++;
                deleteTransfer=false;
            }
            if (i == 0) //first
            {
                if (uploadpaused || downloadpaused)
                {
                    OUTSTREAM << "            " << (downloadpaused?"DOWNLOADS":"") << ((uploadpaused && downloadpaused)?" AND ":"")
                              << (uploadpaused?"UPLOADS":"") << " ARE PAUSED " << endl;
                }
            }
            if (i==(unsigned int)limit) //we are in the extra one (not to be shown)
            {
                OUTSTREAM << " ...  Showing first " << limit << " transfers ..." << endl;
                if (deleteTransfer)
                {
                    delete transfer;
                }
                break;
            }

            printTransferColumnDisplayer(&cd, transfer);

            if (deleteTransfer)
            {
                delete transfer;
            }
        }
        OUTSTREAM << cd.str();
    }
    else if (words[0] == "locallogout")
    {
        OUTSTREAM << "Logging out locally..." << endl;
        cwd = UNDEF;
        return;
    }
    else if (words[0] == "proxy")
    {
        bool autoProxy = getFlag(clflags, "auto");
        bool noneProxy = getFlag(clflags, "none");
        int proxyType = -1;

        string username = getOption(cloptions, "username", "");
        string password;
        if (username.size())
        {
            password = getOption(cloptions, "password", "");
            if (!password.size())
            {
                password = askforUserResponse("Enter password: ");
            }
        }

        string urlProxy;
        if (words.size() > 1)
        {
            proxyType = MegaProxy::PROXY_CUSTOM;
            urlProxy = words[1];
        }
        else if (autoProxy)
        {
            proxyType = MegaProxy::PROXY_AUTO;
        }
        else if (noneProxy)
        {
            proxyType = MegaProxy::PROXY_NONE;
        }

        if (proxyType == -1)
        {
            int configuredProxyType = ConfigurationManager::getConfigurationValue("proxy_type", -1);
            auto configuredProxyUrl = ConfigurationManager::getConfigurationSValue("proxy_url");

            auto configuredProxyUsername = ConfigurationManager::getConfigurationSValue("proxy_username");
            auto configuredProxyPassword = ConfigurationManager::getConfigurationSValue("proxy_password");

            if (configuredProxyType == -1)
            {
                OUTSTREAM << "No proxy configuration found" << endl;
                return;
            }
            if (configuredProxyType == MegaProxy::PROXY_NONE)
            {
                OUTSTREAM << "Proxy disabled" << endl;
                return;
            }


            OUTSTREAM << "Proxy configured. ";

            if (configuredProxyType == MegaProxy::PROXY_AUTO)
            {
                OUTSTREAM << endl << " Type = AUTO";
            }
            else
            {
                OUTSTREAM << endl << " Type = CUSTOM";
            }

            if (configuredProxyUrl.size())
            {
                OUTSTREAM << endl << " URL = " << configuredProxyUrl;
            }
            if (configuredProxyUsername.size())
            {
                OUTSTREAM << endl << " username = " << configuredProxyUsername;
            }
            if (configuredProxyPassword.size())
            {
                OUTSTREAM << endl << " password = " << configuredProxyPassword;
            }

            OUTSTREAM << endl;
        }
        else
        {
            setProxy(urlProxy, username, password, proxyType);
        }

    }
#ifdef WITH_FUSE
    else if (words[0] == "fuse-add")
    {
        if (!api->isFilesystemAvailable() || !api->isLoggedIn())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in";
            return;
        }

#ifdef WIN32
        if (words.size() != 2 && (!getenv("MEGACMD_FUSE_ALLOW_LOCAL_PATHS") || words.size() != 3))
#else
        if (words.size() != 3)
#endif
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << getUsageStr("fuse-add");
            return;
        }

        bool remoteIsFirstArg(words.size() == 2);

        const std::string& localPathStr = remoteIsFirstArg ? std::string("") : words[1];
        const std::string& remotePathStr = remoteIsFirstArg ? words[1] : words[2];

        if (remotePathStr.empty()
#ifndef WIN32
            || localPathStr.empty()
#endif
            )
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Path cannot be empty";
            LOG_err << "      " << getUsageStr("fuse-add");
            return;
        }

        const fs::path localPath = localPathStr;
#ifndef WIN32
        if (std::error_code ec; !fs::exists(localPath, ec) || ec)
        {
            setCurrentThreadOutCode(MCMD_NOTFOUND);
            LOG_err << "Local path " << localPath << " does not exist";
            return;
        }
#endif

        std::unique_ptr<MegaNode> node = nodebypath(remotePathStr.c_str());
        if (node == nullptr)
        {
            setCurrentThreadOutCode(MCMD_NOTFOUND);
            LOG_err << "Remote path \"" << remotePathStr << "\" does not exist";
            return;
        }

        const std::string name = getOption(cloptions, "name", "");
        const bool disabled = getFlag(clflags, "disabled");
        const bool transient = getFlag(clflags, "transient");
        const bool readOnly = getFlag(clflags, "read-only");

        FuseCommand::addMount(*api, localPath, *node, disabled, transient, readOnly, name);
    }
    else if (words[0] == "fuse-remove")
    {
        if (!api->isFilesystemAvailable() || !api->isLoggedIn())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in";
            return;
        }

        if (words.size() != 2)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << getUsageStr("fuse-remove");
            return;
        }

        const std::string& identifier = words[1];
        auto mount = FuseCommand::getMountByNameOrPath(*api, identifier);
        if (!mount)
        {
            setCurrentThreadOutCode(MCMD_NOTFOUND);
            return;
        }

        FuseCommand::removeMount(*api, *mount);
    }
    else if (words[0] == "fuse-enable")
    {
        if (!api->isFilesystemAvailable() || !api->isLoggedIn())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in";
            return;
        }

        if (words.size() != 2)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << getUsageStr("fuse-enable");
            return;
        }

        const std::string& identifier = words[1];
        auto mount = FuseCommand::getMountByNameOrPath(*api, identifier);
        if (!mount)
        {
            setCurrentThreadOutCode(MCMD_NOTFOUND);
            return;
        }

        const bool temporarily = getFlag(clflags, "temporarily");
        FuseCommand::enableMount(*api, *mount, temporarily);
    }
    else if (words[0] == "fuse-disable")
    {
        if (!api->isFilesystemAvailable() || !api->isLoggedIn())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in";
            return;
        }

        if (words.size() != 2)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << getUsageStr("fuse-disable");
            return;
        }

        const std::string& identifier = words[1];
        auto mount = FuseCommand::getMountByNameOrPath(*api, identifier);
        if (!mount)
        {
            setCurrentThreadOutCode(MCMD_NOTFOUND);
            return;
        }

        const bool temporarily = getFlag(clflags, "temporarily");
        FuseCommand::disableMount(*api, *mount, temporarily);
    }
    else if (words[0] == "fuse-show")
    {
        if (!api->isFilesystemAvailable() || !api->isLoggedIn())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in";
            return;
        }

        if (words.size() > 2)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << getUsageStr("fuse-show");
            return;
        }

        const bool showAllMounts = (words.size() == 1);
        if (showAllMounts)
        {
            int rowCountLimit = getintOption(cloptions, "limit", 0);
            if (rowCountLimit < 0)
            {
                setCurrentThreadOutCode(MCMD_EARGS);
                LOG_err << "Row count limit cannot be less than 0";
                return;
            }

            if (rowCountLimit == 0)
            {
                rowCountLimit = std::numeric_limits<int>::max();
            }

            const bool disablePathCollapse = getFlag(clflags, "disable-path-collapse");
            const bool onlyEnabled = getFlag(clflags, "only-enabled");

            ColumnDisplayer cd(clflags, cloptions);
            FuseCommand::printAllMounts(*api, cd, onlyEnabled, disablePathCollapse, rowCountLimit);
        }
        else
        {
            const std::string& identifier = words[1];
            auto mount = FuseCommand::getMountByNameOrPath(*api, identifier);
            if (!mount)
            {
                setCurrentThreadOutCode(MCMD_NOTFOUND);
                return;
            }

            FuseCommand::printMount(*api, *mount);
        }
    }
    else if (words[0] == "fuse-config")
    {
        if (!api->isFilesystemAvailable() || !api->isLoggedIn())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in";
            return;
        }

        if (words.size() != 2)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << getUsageStr("fuse-config");
            return;
        }

        auto configDeltaOpt = loadFuseConfigDelta(*cloptions);
        if (!configDeltaOpt)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            return;
        }

        const std::string& identifier = words[1];
        auto mount = FuseCommand::getMountByNameOrPath(*api, identifier);
        if (!mount)
        {
            setCurrentThreadOutCode(MCMD_NOTFOUND);
            return;
        }

        FuseCommand::changeConfig(*api, *mount, *configDeltaOpt);
    }
#endif
    else if (words[0] == "sync-issues")
    {
        if (!api->isFilesystemAvailable() || !api->isLoggedIn())
        {
            setCurrentThreadOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in";
            return;
        }

        bool disableWarning = getFlag(clflags, "disable-warning");
        bool enableWarning = getFlag(clflags, "enable-warning");
        if (!onlyZeroOrOneOf(disableWarning, enableWarning))
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Only one warning action (enable/disable) can be specified at a time";
            LOG_err << "      " << getUsageStr("sync-issues");
            return;
        }

        int rowCountLimit = getintOption(cloptions, "limit", 10);
        if (rowCountLimit < 0)
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Row count limit cannot be less than 0";
            return;
        }

        if (rowCountLimit == 0)
        {
            rowCountLimit = std::numeric_limits<int>::max();
        }

        bool disablePathCollapse = getFlag(clflags, "disable-path-collapse");

        if (disableWarning)
        {
            mSyncIssuesManager.disableWarning();
            return;
        }
        if (enableWarning)
        {
            mSyncIssuesManager.enableWarning();
            return;
        }

        auto syncIssues = mSyncIssuesManager.getSyncIssues();
#ifdef MEGACMD_TESTING_CODE
        // Do not trust empty results (SDK may send them after spurious scans delayed 20ds. SDK-4813)
        timelyRetry(std::chrono::milliseconds(2300), std::chrono::milliseconds(200),
                    [&syncIssues]() { return !syncIssues.empty(); },
                    [this, &syncIssues, firstTime{true}]() mutable
        {
            if (firstTime)
            {
                LOG_warn << "sync-issues first retrieval returned empty";
                firstTime = false;
            }
            syncIssues = mSyncIssuesManager.getSyncIssues();
            LOG_warn << "sync-issues retrieval returned empty. Retried returned = " << syncIssues.size();
        });
#endif

        ColumnDisplayer cd(clflags, cloptions);

        bool detailSyncIssue = getFlag(clflags, "detail");
        if (detailSyncIssue) // get the details of one or more issues
        {
            bool showAll = getFlag(clflags, "all");
            if (showAll)
            {
                if (words.size() != 1)
                {
                    LOG_err << getUsageStr("sync-issues");
                    return;
                }
                SyncIssuesCommand::printAllIssuesDetail(*api, cd, syncIssues, disablePathCollapse, rowCountLimit);
            }
            else
            {
                if (words.size() != 2)
                {
                    LOG_err << getUsageStr("sync-issues");
                    return;
                }

                const std::string syncIssueId = words.back();

                auto syncIssuePtr = syncIssues.getSyncIssue(syncIssueId);
                if (!syncIssuePtr)
                {
                    setCurrentThreadOutCode(MCMD_NOTFOUND);
                    LOG_err << "Sync issue \"" << syncIssueId << "\" does not exist";
                    return;
                }

                SyncIssuesCommand::printSingleIssueDetail(*api, cd, *syncIssuePtr, disablePathCollapse, rowCountLimit);
            }
        }
        else // show all sync issues
        {
            if (syncIssues.empty())
            {
                OUTSTREAM << "There are no sync issues" << endl;
                return;
            }

            SyncIssuesCommand::printAllIssues(*api, cd, syncIssues, disablePathCollapse, rowCountLimit);
        }
    }
#if defined(DEBUG) || defined(MEGACMD_TESTING_CODE)
    else if (words[0] == "echo")
    {
        if (words.size() < 2 || words[1].empty())
        {
            setCurrentThreadOutCode(MCMD_EARGS);
            LOG_err << "Missing message to echo";
            return;
        }

        std::string str = words[1];
#ifdef _WIN32
        if (str == "<win-invalid-utf8>")
        {
            str = "\xf0\x8f\xbf\xbf";
        }
#endif

        if (getFlag(clflags, "log-as-err"))
        {
            LOG_err << str;
        }
        else
        {
            OUTSTREAM << str << endl;
        }
    }
#endif
    else
    {
        setCurrentThreadOutCode(MCMD_EARGS);
        LOG_err << "Invalid command: " << words[0];
    }
}

}//end namespace

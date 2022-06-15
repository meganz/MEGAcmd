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
#include "megacmd.h"

#include "megacmdutils.h"
#include "megacmdtransfermanager.h"
#include "configurationmanager.h"
#include "megacmdlogger.h"
#include "comunicationsmanager.h"
#include "listeners.h"
#include "megacmdversion.h"

#include <iomanip>
#include <string>
#include <ctime>

#include <set>

#include <signal.h>


#if (__cplusplus >= 201700L)
    #include <filesystem>
    namespace fs = std::filesystem;
    #define MEGACMDEXECUTER_FILESYSTEM
#elif !defined(__MINGW32__) && !defined(__ANDROID__) && (!defined(__GNUC__) || (__GNUC__*100+__GNUC_MINOR__) >= 503)
#define MEGACMDEXECUTER_FILESYSTEM
#ifdef WIN32
    #include <filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#endif
#endif


using namespace mega;

namespace megacmd {
using namespace std;

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

MegaCmdExecuter::MegaCmdExecuter(MegaApi *api, MegaCMDLogger *loggerCMD, MegaCmdSandbox *sandboxCMD)
{
    signingup = false;
    confirming = false;

    this->api = api;
    this->loggerCMD = loggerCMD;
    this->sandboxCMD = sandboxCMD;
    this->globalTransferListener = new MegaCmdGlobalTransferListener(api, sandboxCMD);
    api->addTransferListener(globalTransferListener);
    cwd = UNDEF;
    fsAccessCMD = new MegaFileSystemAccess();
    session = NULL;
}

MegaCmdExecuter::~MegaCmdExecuter()
{
    delete fsAccessCMD;
    delete []session;
    for (std::vector< MegaNode * >::iterator it = nodesToConfirmDelete.begin(); it != nodesToConfirmDelete.end(); ++it)
    {
        delete *it;
    }
    nodesToConfirmDelete.clear();
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

        OUTSTREAM << "INSHARE on //from/" << share->getUser() << ":" << n->getName() << " (" << getAccessLevelStr(share->getAccess()) << ")" << endl;
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
// returns NULL if path malformed or not found
MegaNode* MegaCmdExecuter::nodebypath(const char* ptr, string* user, string* namepart)
{
    if (ptr && ptr[0] == 'H' && ptr[1] == ':')
    {
        MegaNode * n = api->getNodeByHandle(api->base64ToHandle(ptr+2));
        if (n)
        {
            return n;
        }
    }

    string rest;
    MegaNode *baseNode = getBaseNode(ptr, rest);

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
            MegaNode * nextNode = NULL;
            if (curName == "..")
            {
                nextNode = api->getParentNode(baseNode);
            }
            else
            {
                replaceAll(curName, "\\\\", "\\"); //unescape '\\'
                replaceAll(curName, "\\ ", " "); //unescape '\ '
                bool isversion = nodeNameIsVersion(curName);
                if (isversion)
                {
                    MegaNode *childNode = api->getChildNode(baseNode, curName.substr(0,curName.size()-11).c_str());
                    if (childNode)
                    {
                        MegaNodeList *versionNodes = api->getVersions(childNode);
                        if (versionNodes)
                        {
                            for (int i = 0; i < versionNodes->size(); i++)
                            {
                                MegaNode *versionNode = versionNodes->get(i);
                                if ( curName.substr(curName.size()-10) == SSTR(versionNode->getModificationTime()) )
                                {
                                    nextNode = versionNode->copy();
                                    break;
                                }
                            }
                            delete versionNodes;
                        }
                        delete childNode;
                    }
                }
                else
                {
                    nextNode = api->getChildNode(baseNode,curName.c_str());
                }
            }

            // mv command target? return name part of not found
            if (namepart && !nextNode && ( possep == string::npos)) //if this is the last part, we will pass that one, so that a mv command know the name to give the new node
            {
                *namepart = rest;
                return baseNode;
            }

            if (nextNode != baseNode)
            {
                delete baseNode;
            }
            baseNode = nextNode;
        }

        if (possep != string::npos && possep != (rest.size() - 1) )
        {
            rest = rest.substr(possep+1);
        }
        else
        {
            return baseNode;
        }
    }

    return NULL;
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
void MegaCmdExecuter::getNodesMatching(MegaNode *parentNode, deque<string> pathParts, vector<MegaNode *> *nodesMatching, bool usepcre)
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
                nodesMatching->push_back(parentNode->copy());
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
                    nodesMatching->push_back(newparentNode);
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
                                    nodesMatching->push_back(versionNode->copy());
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
                        nodesMatching->push_back(childNode->copy());
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
        baseNode = api->getInboxNode();
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
        baseNode = api->getInboxNode();
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
            sandboxCMD->lastPSAnumreceived = int(megaCmdListener->getRequest()->getNumber());

            LOG_debug << "Informing PSA #" << megaCmdListener->getRequest()->getNumber() << ": " << megaCmdListener->getRequest()->getName();

            stringstream oss;

            oss << "<" << megaCmdListener->getRequest()->getName() << ">";
            oss << megaCmdListener->getRequest()->getText();

            string action = megaCmdListener->getRequest()->getPassword();
            string link = megaCmdListener->getRequest()->getLink();
            if (!action.length())
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


bool MegaCmdExecuter::checkNoErrors(int errorCode, string message)
{
    MegaErrorPrivate e(errorCode);
    return checkNoErrors(&e, message);
}

bool MegaCmdExecuter::checkNoErrors(MegaError *error, string message, SyncError syncError)
{
    if (!error)
    {
        LOG_fatal << "No MegaError at request: " << message;
        return false;
    }
    if (error->getErrorCode() == MegaError::API_OK)
    {
        if (syncError)
        {
            LOG_info << "Able to " << message << ", but received syncError: " << MegaSync::getMegaSyncErrorCode(syncError);
        }
        return true;
    }

    setCurrentOutCode(error->getErrorCode());
    if (error->getErrorCode() == MegaError::API_EBLOCKED)
    {
        auto reason = sandboxCMD->getReasonblocked();
        LOG_err << "Failed to " << message << ". Account blocked." <<( reason.empty()?"":" Reason: "+reason);
    }
    else if ((error->getErrorCode() == MegaError::API_EPAYWALL) || (error->getErrorCode() == MegaError::API_EOVERQUOTA && sandboxCMD->storageStatus == MegaApi::STORAGE_STATE_RED))
    {
        LOG_err << "Failed to " << message << ": Reached storage quota. "
                         "You can change your account plan to increase your quota limit. "
                         "See \"help --upgrade\" for further details";
    }
    else
    {
        if (syncError)
        {
            LOG_err << "Failed to " << message << ": " << error->getErrorString()
                    << ". " << MegaSync::getMegaSyncErrorCode(syncError);
        }
        else
        {
            LOG_err << "Failed to " << message << ": " << error->getErrorString();
        }

    }

    return false;
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
 * @return List of MegaNode*.  You take the ownership of those MegaNode*
 */
vector <MegaNode*> * MegaCmdExecuter::nodesbypath(const char* ptr, bool usepcre, string* user)
{
    vector<MegaNode *> *nodesMatching = new vector<MegaNode *> ();

    if (ptr && ptr[0] == 'H' && ptr[1] == ':')
    {
        MegaNode * n = api->getNodeByHandle(api->base64ToHandle(ptr+2));
        if (n)
        {
            nodesMatching->push_back(n);
            return nodesMatching;
        }
    }

    string rest;
    MegaNode *baseNode = getBaseNode(ptr, rest);

    if (baseNode)
    {
        if (!rest.size())
        {
            nodesMatching->push_back(baseNode);
            return nodesMatching;
        }

        deque<string> c;
        getPathParts(rest, &c);

        if (!c.size())
        {
            nodesMatching->push_back(baseNode);
            return nodesMatching;
        }
        else
        {
            getNodesMatching(baseNode, c, nodesMatching, usepcre);
        }
        delete baseNode;
    }
    else if (!strncmp(ptr,"//from/",max(3,min((int)strlen(ptr)-1,7)))) //pattern trying to match inshares
    {
        unique_ptr<MegaShareList> inShares(api->getInSharesList());
        if (inShares)
        {
            string matching = ptr;
            unescapeifRequired(matching);
            for (int i = 0; i < inShares->size(); i++)
            {
                MegaNode* n = api->getNodeByHandle(inShares->get(i)->getNodeHandle());
                string tomatch = string("//from/")+inShares->get(i)->getUser() + ":"+n->getName();
                if (patternMatches(tomatch.c_str(), matching.c_str(), false))
                {
                    nodesMatching->push_back(n);
                }
                else
                {
                    delete n;
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
                        if (outShares->get(i)->getNodeHandle() == n->getHandle())
                        {
                            OUTSTREAM << ", shared with " << outShares->get(i)->getUser() << ", access "
                                      << getAccessLevelStr(outShares->get(i)->getAccess());
                        }
                    }

                    MegaShareList* pendingoutShares = api->getPendingOutShares(n);
                    if (pendingoutShares)
                    {
                        for (int i = 0; i < pendingoutShares->size(); i++)
                        {
                            if (pendingoutShares->get(i)->getNodeHandle() == n->getHandle())
                            {
                                OUTSTREAM << ", shared (still pending)";
                                if (pendingoutShares->get(i)->getUser())
                                {
                                    OUTSTREAM << " with " << pendingoutShares->get(i)->getUser();
                                }
                                OUTSTREAM << " access " << getAccessLevelStr(pendingoutShares->get(i)->getAccess());
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
                            char * publicLink = n->getPublicLink();
                            OUTSTREAM << ": " << publicLink;

                            if (n->getWritableLinkAuthKey())
                            {
                                string authKey(n->getWritableLinkAuthKey());
                                if (authKey.size())
                                {
                                    string authToken(publicLink);
                                    authToken = authToken.substr(strlen("https://mega.nz/folder/")).append(":").append(authKey);
                                    OUTSTREAM << " AuthToken="<< authToken;
                                }
                            }

                            delete []publicLink;
                        }
                    }
                    delete outShares;
                }

                if (n->isInShare())
                {
                    OUTSTREAM << ", inbound " << api->getAccess(n) << " share";
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
        MegaNodeList *versionNodes = api->getVersions(n);
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

void MegaCmdExecuter::dumpNodeSummaryHeader(const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions)
{
    int datelength = int(getReadableTime(m_time(), timeFormat).size());

    OUTSTREAM << "FLAGS";
    OUTSTREAM << " ";
    OUTSTREAM << getFixLengthString("VERS", 4);
    OUTSTREAM << " ";
    OUTSTREAM << getFixLengthString("SIZE  ", 10 -1, ' ', true); //-1 because of "FLAGS"
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
            OUTSTREAM << getFixLengthString(sizeToText(n->getSize()), 10, ' ', true);
        }
        else
        {
            OUTSTREAM << getFixLengthString(SSTR(n->getSize()), 10, ' ', true);
        }
    }
    else
    {
        OUTSTREAM << getFixLengthString("-", 10, ' ', true);
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



#ifdef ENABLE_BACKUPS

void MegaCmdExecuter::createOrModifyBackup(string local, string remote, string speriod, int numBackups)
{
    LocalPath locallocal = LocalPath::fromPath(local, *fsAccessCMD);
    std::unique_ptr<FileAccess> fa = fsAccessCMD->newfileaccess();
    if (!fa->isfolder(locallocal))
    {
        setCurrentOutCode(MCMD_NOTFOUND);
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
            setCurrentOutCode(MCMD_EARGS);
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
        setCurrentOutCode(MCMD_EARGS);
        LOG_err << "      " << getUsageStr("backup");
        return;
    }

    MegaNode *n = NULL;
    if (remote.size())
    {
        n = nodebypath(remote.c_str());
    }
    else
    {
        MegaScheduledCopy *backup = api->getScheduledCopyByPath(local.c_str());
        if (!backup)
        {
            backup = api->getScheduledCopyByTag(toInteger(local, -1));
        }
        if (backup)
        {
            n = api->getNodeByHandle(backup->getMegaHandle());
            delete backup;
        }
    }

    if (n)
    {
        if (n->getType() != MegaNode::TYPE_FOLDER)
        {
            setCurrentOutCode(MCMD_INVALIDTYPE);
            LOG_err << remote << " must be a valid folder";
        }
        else
        {
            if (establishBackup(local, n, period, speriod, numBackups) )
            {
                mtxBackupsMap.lock();
                ConfigurationManager::saveBackups(&ConfigurationManager::configuredBackups);
                mtxBackupsMap.unlock();
                OUTSTREAM << "Backup established: " << local << " into " << remote << " period="
                          << ((period != -1)?getReadablePeriod(period/10):"\""+speriod+"\"")
                          << " Number-of-Backups=" << numBackups << endl;
            }
        }
        delete n;
    }
    else
    {
        setCurrentOutCode(MCMD_NOTFOUND);
        LOG_err << remote << " not found";
    }
}
#endif

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
                        dumpNodeSummary(vers->get(i), timeFormat, clflags, cloptions, humanreadable, nametoshow.c_str() );
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
bool MegaCmdExecuter::TestCanWriteOnContainingFolder(string *path)
{
#ifdef _WIN32
    replaceAll(*path,"/","\\");
#endif

    auto containingFolder = LocalPath::fromPath(*path, *fsAccessCMD);

    // Where does our name begin?
    auto index = containingFolder.getLeafnameByteIndex(*fsAccessCMD);

    // We have a parent.
    if (index)
    {
        // Remove the current leaf name.
        containingFolder.truncate(index);
    }
    else
    {
        containingFolder = LocalPath::fromPath(".", *fsAccessCMD);
    }

    std::unique_ptr<FileAccess> fa = fsAccessCMD->newfileaccess();
    if (!fa->isfolder(containingFolder))
    {
        setCurrentOutCode(MCMD_INVALIDTYPE);
        LOG_err << containingFolder.toPath(*fsAccessCMD) << " is not a valid Download Folder";
        return false;
    }

    if (!canWrite(containingFolder.platformEncoded()))
    {
        setCurrentOutCode(MCMD_NOTPERMITTED);
        LOG_err << "Write not allowed in " << containingFolder.toPath(*fsAccessCMD);
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

/**
 * @brief listnodeshares For a node, it prints all the shares it has
 * @param n
 * @param name
 */
void MegaCmdExecuter::listnodeshares(MegaNode* n, string name)
{
    MegaShareList* outShares = api->getOutShares(n);
    if (outShares)
    {
        for (int i = 0; i < outShares->size(); i++)
        {
            OUTSTREAM << (name.size() ? name : n->getName());

            if (outShares->get(i))
            {
                OUTSTREAM << ", shared with " << outShares->get(i)->getUser() << " (" << getAccessLevelStr(outShares->get(i)->getAccess()) << ")"
                          << endl;
            }
            else
            {
                OUTSTREAM << ", shared as exported folder link" << endl;
            }
        }

        delete outShares;
    }
}

void MegaCmdExecuter::dumpListOfShared(MegaNode* n_param, string givenPath)
{
    vector<MegaNode *> listOfShared;
    processTree(n_param, includeIfIsShared, (void*)&listOfShared);
    if (!listOfShared.size())
    {
        setCurrentOutCode(MCMD_NOTFOUND);
        LOG_err << "No shared found for given path: " << givenPath;
    }
    for (std::vector< MegaNode * >::iterator it = listOfShared.begin(); it != listOfShared.end(); ++it)
    {
        MegaNode * n = *it;
        if (n)
        {
            string pathToShow = getDisplayPath(givenPath, n);
            //dumpNode(n, 3, 1,pathToShow.c_str());
            listnodeshares(n, pathToShow);

            delete n;
        }
    }

    listOfShared.clear();
}

//includes pending and normal shares
void MegaCmdExecuter::dumpListOfAllShared(MegaNode* n_param, const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, string givenPath)
{
    vector<MegaNode *> listOfShared;
    processTree(n_param, includeIfIsSharedOrPendingOutShare, (void*)&listOfShared);
    for (std::vector< MegaNode * >::iterator it = listOfShared.begin(); it != listOfShared.end(); ++it)
    {
        MegaNode * n = *it;
        if (n)
        {
            string pathToShow = getDisplayPath(givenPath, n);
            dumpNode(n, timeFormat, clflags, cloptions, 3, false, 1, pathToShow.c_str());
            //notice: some nodes may be dumped twice

            delete n;
        }
    }

    listOfShared.clear();
}

void MegaCmdExecuter::dumpListOfPendingShares(MegaNode* n_param, const char *timeFormat, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, string givenPath)
{
    vector<MegaNode *> listOfShared;
    processTree(n_param, includeIfIsPendingOutShare, (void*)&listOfShared);

    for (std::vector< MegaNode * >::iterator it = listOfShared.begin(); it != listOfShared.end(); ++it)
    {
        MegaNode * n = *it;
        if (n)
        {
            string pathToShow = getDisplayPath(givenPath, n);
            dumpNode(n, timeFormat, clflags, cloptions, 3, false, 1, pathToShow.c_str());

            delete n;
        }
    }

    listOfShared.clear();
}


void MegaCmdExecuter::loginWithPassword(char *password)
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
            setCurrentOutCode(megaCmdListener2->getError()->getErrorCode());
            LOG_err << "Password unchanged: invalid authentication code";
        }
        else if (checkNoErrors(megaCmdListener2->getError(), "change password with auth code"))
        {
            OUTSTREAM << "Password changed succesfully" << endl;
        }

    }
    else if (!checkNoErrors(megaCmdListener->getError(), "change password"))
    {
        LOG_err << "Please, ensure you enter the old password correctly";
    }
    else
    {
        OUTSTREAM << "Password changed succesfully" << endl;
    }
    delete megaCmdListener;
}

void str_localtime(char s[32], ::mega::m_time_t t)
{
    struct tm tms;
    strftime(s, 32, "%c", m_localtime(t, &tms));
}


void MegaCmdExecuter::actUponGetExtendedAccountDetails(SynchronousRequestListener *srl, int timeout)
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
            LOG_err << "GetExtendedAccountDetails took too long, it may have failed. No further actions performed";
            return;
        }
    }

    if (checkNoErrors(srl->getError(), "failed to GetExtendedAccountDetails"))
    {
        char timebuf[32], timebuf2[32];

        LOG_verbose << "actUponGetExtendedAccountDetails ok";

        MegaAccountDetails *details = srl->getRequest()->getMegaAccountDetails();
        if (details)
        {
            OUTSTREAM << "    Available storage: "
                      << getFixLengthString(sizeToText(details->getStorageMax()), 9, ' ', true)
                      << "ytes" << endl;
            MegaNode *n = api->getRootNode();
            if (n)
            {
                OUTSTREAM << "        In ROOT:      "
                          << getFixLengthString(sizeToText(details->getStorageUsed(n->getHandle())), 9, ' ', true) << "ytes in "
                          << getFixLengthString(SSTR(details->getNumFiles(n->getHandle())),5,' ',true) << " file(s) and "
                          << getFixLengthString(SSTR(details->getNumFolders(n->getHandle())),5,' ',true) << " folder(s)" << endl;
                delete n;
            }

            n = api->getInboxNode();
            if (n)
            {
                OUTSTREAM << "        In INBOX:     "
                          << getFixLengthString( sizeToText(details->getStorageUsed(n->getHandle())), 9, ' ', true ) << "ytes in "
                          << getFixLengthString(SSTR(details->getNumFiles(n->getHandle())),5,' ',true) << " file(s) and "
                          << getFixLengthString(SSTR(details->getNumFolders(n->getHandle())),5,' ',true) << " folder(s)" << endl;
                delete n;
            }

            n = api->getRubbishNode();
            if (n)
            {
                OUTSTREAM << "        In RUBBISH:   "
                          << getFixLengthString(sizeToText(details->getStorageUsed(n->getHandle())), 9, ' ', true) << "ytes in "
                          << getFixLengthString(SSTR(details->getNumFiles(n->getHandle())),5,' ',true) << " file(s) and "
                          << getFixLengthString(SSTR(details->getNumFolders(n->getHandle())),5,' ',true) << " folder(s)" << endl;
                delete n;
            }

            long long usedinVersions = details->getVersionStorageUsed();

            OUTSTREAM << "        Total size taken up by file versions: "
                      << getFixLengthString(sizeToText(usedinVersions), 12, ' ', true) << "ytes"<< endl;


            MegaNodeList *inshares = api->getInShares();
            if (inshares)
            {
                for (int i = 0; i < inshares->size(); i++)
                {
                    n = inshares->get(i);
                    OUTSTREAM << "        In INSHARE " << n->getName() << ": "
                              << getFixLengthString(sizeToText(details->getStorageUsed(n->getHandle())), 9, ' ', true) << "ytes in "
                              << getFixLengthString(SSTR(details->getNumFiles(n->getHandle())),5,' ',true) << " file(s) and "
                              << getFixLengthString(SSTR(details->getNumFolders(n->getHandle())),5,' ',true) << " folder(s)" << endl;
                }
            }
            delete inshares;

            OUTSTREAM << "    Pro level: " << details->getProLevel() << endl;
            if (details->getProLevel())
            {
                if (details->getProExpiration())
                {
                    str_localtime(timebuf, details->getProExpiration());
                    OUTSTREAM << "        " << "Pro expiration date: " << timebuf << endl;
                }
            }
            char * subscriptionMethod = details->getSubscriptionMethod();
            OUTSTREAM << "    Subscription type: " << subscriptionMethod << endl;
            delete []subscriptionMethod;
            OUTSTREAM << "    Account balance:" << endl;
            for (int i = 0; i < details->getNumBalances(); i++)
            {
                MegaAccountBalance * balance = details->getBalance(i);
                char sbalance[50];
                sprintf(sbalance, "    Balance: %.3s %.02f", balance->getCurrency(), balance->getAmount());
                OUTSTREAM << "    " << "Balance: " << sbalance << endl;
            }

            if (details->getNumPurchases())
            {
                OUTSTREAM << "Purchase history:" << endl;
                for (int i = 0; i < details->getNumPurchases(); i++)
                {
                    MegaAccountPurchase *purchase = details->getPurchase(i);

                    char spurchase[150];

                    str_localtime(timebuf, purchase->getTimestamp());
                    sprintf(spurchase, "ID: %.11s Time: %s Amount: %.3s %.02f Payment method: %d\n",
                        purchase->getHandle(), timebuf, purchase->getCurrency(), purchase->getAmount(), purchase->getMethod());
                    OUTSTREAM << "    " << spurchase << endl;
                }
            }

            if (details->getNumTransactions())
            {
                OUTSTREAM << "Transaction history:" << endl;
                for (int i = 0; i < details->getNumTransactions(); i++)
                {
                    MegaAccountTransaction *transaction = details->getTransaction(i);
                    char stransaction[100];
                    str_localtime(timebuf, transaction->getTimestamp());
                    sprintf(stransaction, "ID: %.11s Time: %s Amount: %.3s %.02f\n",
                        transaction->getHandle(), timebuf, transaction->getCurrency(), transaction->getAmount());
                    OUTSTREAM << "    " << stransaction << endl;
                }
            }

            int alive_sessions = 0;
            OUTSTREAM << "Current Active Sessions:" << endl;
            char sdetails[500];
            for (int i = 0; i < details->getNumSessions(); i++)
            {
                MegaAccountSession * session = details->getSession(i);
                if (session->isAlive())
                {
                    sdetails[0]='\0';
                    str_localtime(timebuf, session->getCreationTimestamp());
                    str_localtime(timebuf2, session->getMostRecentUsage());

                    char *sid = api->userHandleToBase64(session->getHandle());

                    if (session->isCurrent())
                    {
                        sprintf(sdetails, "    * Current Session\n");
                    }

                    char * userAgent = session->getUserAgent();
                    char * country = session->getCountry();
                    char * ip = session->getIP();

                    sprintf(sdetails, "%s    Session ID: %s\n    Session start: %s\n    Most recent activity: %s\n    IP: %s\n    Country: %.2s\n    User-Agent: %s\n    -----\n",
                    sdetails,
                    sid,
                    timebuf,
                    timebuf2,
                    ip,
                    country,
                    userAgent
                    );
                    OUTSTREAM << sdetails;
                    delete []sid;
                    delete []userAgent;
                    delete []country;
                    delete []ip;
                    alive_sessions++;
                }
                delete session;
            }

            if (alive_sessions)
            {
                OUTSTREAM << alive_sessions << " active sessions opened" << endl;
            }
            delete details;
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
        LOG_verbose << " EBLOCKED after fetch nodes. quering for reason...";

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
        session = srl->getApi()->dumpSession();
        ConfigurationManager::saveSession(session);


        LOG_verbose << "actUponFetchNodes ok";

        return true;
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
    setCurrentOutCode(srl->getError()->getErrorCode());

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
            session = srl->getApi()->dumpSession();
            ConfigurationManager::saveSession(session);
        }

        /* Restoring configured values */
        mtxSyncMap.lock();
        ConfigurationManager::loadsyncs();
        mtxSyncMap.unlock();
#ifdef ENABLE_BACKUPS
        mtxBackupsMap.lock();
        ConfigurationManager::loadbackups();
        mtxBackupsMap.unlock();
#endif

        ConfigurationManager::loadExcludedNames();
        ConfigurationManager::loadConfiguration(false);
        std::vector<string> vexcludednames(ConfigurationManager::excludedNames.begin(), ConfigurationManager::excludedNames.end());
        api->setExcludedNames(&vexcludednames);

        long long maxspeeddownload = ConfigurationManager::getConfigurationValue("maxspeeddownload", -1);
        if (maxspeeddownload != -1) api->setMaxDownloadSpeed(maxspeeddownload);
        long long maxspeedupload = ConfigurationManager::getConfigurationValue("maxspeedupload", -1);
        if (maxspeedupload != -1) api->setMaxUploadSpeed(maxspeedupload);

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

#ifdef HAVE_DOWNLOADS_COMMAND
        bool downloads_tracking_enabled = ConfigurationManager::getConfigurationValue("downloads_tracking_enabled", false);
        if (downloads_tracking_enabled)
        {
            DownloadsManager::Instance().start();
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
    MegaCmdListener * megaCmdListener = new MegaCmdListener(api, NULL, clientID);
    api->fetchNodes(megaCmdListener);
    if (!actUponFetchNodes(api, megaCmdListener))
    {
        //Ideally we should enter an state that indicates that we are not fully logged.
        //Specially when the account is blocked
        return;
    }

    // This is the actual acting upon fetch nodes ended correctly:

    //automatic now:
    //api->enableTransferResumption();

    MegaNode *cwdNode = ( cwd == UNDEF ) ? NULL : api->getNodeByHandle(cwd);
    if (( cwd == UNDEF ) || !cwdNode)
    {
        MegaNode *rootNode = api->getRootNode();
        cwd = rootNode->getHandle();
        delete rootNode;
    }
    if (cwdNode)
    {
        delete cwdNode;
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
        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
        api->getAccountDetails(megaCmdListener);
        megaCmdListener->wait();
        // we don't call getAccountDetails on startup always: we ask on first login (no "ask4storage") or previous state was STATE_RED | STATE_ORANGE
        // if we were green, don't need to ask: if there are changes they will be received via action packet indicating STATE_CHANGE
    }

    checkAndInformPSA(NULL); // this needs broacasting in case there's another Shell running.
    // no need to enforce, because time since last check should has been restored

#ifdef ENABLE_BACKUPS
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
                LOG_debug << "Succesfully resumed backup: " << thebackup->localpath << " to " << nodepath;
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
#endif

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
                    MegaNode *n = nodebypath(pathToServe.c_str());
                    if (n)
                    {
                        char *l = api->httpServerGetLocalWebDavLink(n);
                        char *actualNodePath = api->getNodePath(n);
                        LOG_debug << "Serving via webdav: " << actualNodePath << ": " << l;

                        if (pathToServe != actualNodePath)
                        {
                            it = servedpaths.erase(it);
                            servedpaths.insert(it,string(actualNodePath));
                            modified = true;
                        }
                        delete []l;
                        delete []actualNodePath;
                        delete n;
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
                    MegaNode *n = nodebypath(pathToServe.c_str());
                    if (n)
                    {
                        char *l = api->ftpServerGetLocalLink(n);
                        char *actualNodePath = api->getNodePath(n);
                        LOG_debug << "Serving via ftp: " << pathToServe << ": " << l << ". Data Channel Port Range: " << dataPortRangeBegin << "-" << dataPortRangeEnd;

                        if (pathToServe != actualNodePath)
                        {
                            it = servedpaths.erase(it);
                            servedpaths.insert(it,string(actualNodePath));
                            modified = true;
                        }
                        delete []l;
                        delete []actualNodePath;
                        delete n;
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

void MegaCmdExecuter::cleanSlateTranfers()
{
#ifdef HAVE_DOWNLOADS_COMMAND
    bool downloads_cleanslate = ConfigurationManager::getConfigurationValue("downloads_cleanslate_enabled", false);

    if (downloads_cleanslate)
    {
        auto megaCmdListener = ::mega::make_unique<MegaCmdListener>(nullptr);
        api->cancelTransfers(MegaTransfer::TYPE_DOWNLOAD, megaCmdListener.get());
        megaCmdListener->wait();
        if (checkNoErrors(megaCmdListener->getError(), "cancel all download transfers"))
        {
            LOG_debug << "Download transfers cancelled successfully.";
        }
        megaCmdListener.reset(new MegaCmdListener(nullptr));
        api->cancelTransfers(MegaTransfer::TYPE_UPLOAD, megaCmdListener.get());
        megaCmdListener->wait();
        if (checkNoErrors(megaCmdListener->getError(), "cancel all upload transfers"))
        {
            LOG_debug << "Upload transfers cancelled successfully.";
        }

        {
            globalTransferListener->completedTransfersMutex.lock();
            globalTransferListener->completedTransfers.clear();
            globalTransferListener->completedTransfersMutex.unlock();
        }

        DownloadsManager::Instance().purge();
    }
#endif
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

    if (srl->getError()->getErrorCode() == MegaError::API_ESID || checkNoErrors(srl->getError(), "logout"))
    {
        LOG_verbose << "actUponLogout logout ok";
        cwd = UNDEF;
        delete []session;
        session = NULL;
        mtxSyncMap.lock();
        ConfigurationManager::unloadConfiguration();
        if (!keptSession)
        {
            ConfigurationManager::saveSession("");
            ConfigurationManager::saveBackups(&ConfigurationManager::configuredBackups);
            ConfigurationManager::saveSyncs(&ConfigurationManager::oldConfiguredSyncs);
        }
        ConfigurationManager::clearConfigurationFile();

#ifdef HAVE_DOWNLOADS_COMMAND
        DownloadsManager::Instance().shutdown(true);
#endif
        mtxSyncMap.unlock();
    }
    updateprompt(api);
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
    if (nodesToConfirmDelete.size())
    {
        MegaNode * nodeToConfirmDelete = nodesToConfirmDelete.front();
        nodesToConfirmDelete.erase(nodesToConfirmDelete.begin());
        doDeleteNode(nodeToConfirmDelete,api);
    }


    if (nodesToConfirmDelete.size())
    {
        string newprompt("Are you sure to delete ");
        newprompt+=nodesToConfirmDelete.front()->getName();
        newprompt+=" ? (Yes/No/All/None): ";
        setprompt(AREYOUSURETODELETE,newprompt);
    }
    else
    {
        setprompt(COMMAND);
    }

}

void MegaCmdExecuter::discardDelete()
{
    if (nodesToConfirmDelete.size()){
        delete nodesToConfirmDelete.front();
        nodesToConfirmDelete.erase(nodesToConfirmDelete.begin());
    }
    if (nodesToConfirmDelete.size())
    {
        string newprompt("Are you sure to delete ");
        newprompt+=nodesToConfirmDelete.front()->getName();
        newprompt+=" ? (Yes/No/All/None): ";
        setprompt(AREYOUSURETODELETE,newprompt);
    }
    else
    {
        setprompt(COMMAND);
    }
}


void MegaCmdExecuter::confirmDeleteAll()
{

    while (nodesToConfirmDelete.size())
    {
        MegaNode * nodeToConfirmDelete = nodesToConfirmDelete.front();
        nodesToConfirmDelete.erase(nodesToConfirmDelete.begin());
        doDeleteNode(nodeToConfirmDelete,api);
    }

    setprompt(COMMAND);
}

void MegaCmdExecuter::discardDeleteAll()
{
    while (nodesToConfirmDelete.size()){
        delete nodesToConfirmDelete.front();
        nodesToConfirmDelete.erase(nodesToConfirmDelete.begin());
    }
    setprompt(COMMAND);
}


void MegaCmdExecuter::doDeleteNode(MegaNode *nodeToDelete,MegaApi* api)
{
    char *nodePath = api->getNodePath(nodeToDelete);
    if (nodePath)
    {
        LOG_verbose << "Deleting: "<< nodePath;
    }
    else
    {
        LOG_warn << "Deleting node whose path could not be found " << nodeToDelete->getName();
    }
    MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);
    MegaNode *parent = api->getParentNode(nodeToDelete);
    if (parent && parent->getType() == MegaNode::TYPE_FILE)
    {
        api->removeVersion(nodeToDelete, megaCmdListener);
    }
    else
    {
        api->remove(nodeToDelete, megaCmdListener);
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
    delete nodeToDelete;

}

int MegaCmdExecuter::deleteNodeVersions(MegaNode *nodeToDelete, MegaApi* api, int force)
{
    if (nodeToDelete->getType() == MegaNode::TYPE_FILE && api->getNumVersions(nodeToDelete) < 2)
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
            MegaNodeList *children = api->getChildren(nodeToDelete);
            if (children)
            {
                for (int i = 0; i < children->size(); i++)
                {
                    MegaNode *child = children->get(i);
                    deleteNodeVersions(child, api, true);
                }
                delete children;
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

            MegaNodeList *versionsToDelete = api->getVersions(nodeToDelete);
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
int MegaCmdExecuter::deleteNode(MegaNode *nodeToDelete, MegaApi* api, int recursive, int force)
{
    if (( nodeToDelete->getType() != MegaNode::TYPE_FILE ) && !recursive)
    {
        char *nodePath = api->getNodePath(nodeToDelete);
        setCurrentOutCode(MCMD_INVALIDTYPE);
        LOG_err << "Unable to delete folder: " << nodePath << ". Use -r to delete a folder recursively";
        delete nodeToDelete;
        delete []nodePath;
    }
    else
    {
        if (!getCurrentThreadIsCmdShell() && interactiveThread() && !force && nodeToDelete->getType() != MegaNode::TYPE_FILE)
        {
            bool alreadythere = false;
            for (std::vector< MegaNode * >::iterator it = nodesToConfirmDelete.begin(); it != nodesToConfirmDelete.end(); ++it)
            {
                if (((MegaNode*)*it)->getHandle() == nodeToDelete->getHandle())
                {
                    alreadythere= true;
                }
            }
            if (!alreadythere)
            {
                nodesToConfirmDelete.push_back(nodeToDelete);
                if (getprompt() != AREYOUSURETODELETE)
                {
                    string newprompt("Are you sure to delete ");
                    newprompt+=nodeToDelete->getName();
                    newprompt+=" ? (Yes/No/All/None): ";
                    setprompt(AREYOUSURETODELETE,newprompt);
                }
            }
            else
            {
                delete nodeToDelete;
            }

            return MCMDCONFIRM_NO; //default return
        }
        else if (!force && nodeToDelete->getType() != MegaNode::TYPE_FILE)
        {
            string confirmationQuery("Are you sure to delete ");
            confirmationQuery+=nodeToDelete->getName();
            confirmationQuery+=" ? (Yes/No/All/None): ";

            int confirmationResponse = askforConfirmation(confirmationQuery);

            if (confirmationResponse == MCMDCONFIRM_YES || confirmationResponse == MCMDCONFIRM_ALL)
            {
                LOG_debug << "confirmation received";
                doDeleteNode(nodeToDelete, api);
            }
            else
            {
                delete nodeToDelete;
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
            LOG_verbose << " Updating temporal bandwith ";
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
            OUTSTREAM << "You have reached your bandwith quota";
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
                OUTSTREAM << "Transfer not started: proceding will exceed transfer quota. "
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

    api->startDownload(node, path.c_str(), new ATransferListener(multiTransferListener, source));
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

    LocalPath locallocal = LocalPath::fromPath(path, *fsAccessCMD);
    std::unique_ptr<FileAccess> fa = fsAccessCMD->newfileaccess();
    if (!fa->fopen(locallocal, true, false))
    {
        setCurrentOutCode(MCMD_NOTFOUND);
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


    if (newname.size())
    {

        api->startUpload(removeTrailingSeparators(path).c_str(), node, newname.c_str(), thelistener);
    }
    else
    {
        api->startUpload(removeTrailingSeparators(path).c_str(), node, thelistener);
    }

    if (singleNonBackgroundTransferListener)
    {
        assert(!background);
        singleNonBackgroundTransferListener->wait();
#ifdef _WIN32
            Sleep(100); //give a while to print end of transfer
#endif
        if (singleNonBackgroundTransferListener->getError()->getErrorCode() == API_EREAD)
        {
            setCurrentOutCode(MCMD_NOTFOUND);
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
    MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);
    api->getAccountDetails(megaCmdListener);
    megaCmdListener->wait();
    if (checkNoErrors(megaCmdListener->getError(), "export node"))
    {
        MegaAccountDetails *details = megaCmdListener->getRequest()->getMegaAccountDetails();
        prolevel = details->getProLevel();
    }
    delete megaCmdListener;
    return prolevel > 0;
}

void MegaCmdExecuter::exportNode(MegaNode *n, int64_t expireTime, std::string password, bool force, bool writable)
{
    bool copyrightAccepted = false;

    copyrightAccepted = ConfigurationManager::getConfigurationValue("copyrightAccepted", false) || force;
    if (!copyrightAccepted)
    {
        MegaNodeList * mnl = api->getPublicLinks();
        copyrightAccepted = mnl->size();
        delete mnl;
    }

    int confirmationResponse = copyrightAccepted?MCMDCONFIRM_YES:MCMDCONFIRM_NO;
    if (!copyrightAccepted)
    {
        string confirmationQuery("MEGA respects the copyrights of others and requires that users of the MEGA cloud service comply with the laws of copyright.\n"
                                 "You are strictly prohibited from using the MEGA cloud service to infringe copyrights.\n"
                                 "You may not upload, download, store, share, display, stream, distribute, email, link to, "
                                 "transmit or otherwise make available any files, data or content that infringes any copyright "
                                 "or other proprietary rights of any person or entity.");

        confirmationQuery+=" Do you accept this terms? (Yes/No): ";

        confirmationResponse = askforConfirmation(confirmationQuery);
    }

    if (confirmationResponse == MCMDCONFIRM_YES || confirmationResponse == MCMDCONFIRM_ALL)
    {
        ConfigurationManager::savePropertyValue("copyrightAccepted",true);
        MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);
        api->exportNode(n, expireTime, writable, megaCmdListener);
        megaCmdListener->wait();
        if (checkNoErrors(megaCmdListener->getError(), "export node"))
        {
            MegaNode *nexported = api->getNodeByHandle(megaCmdListener->getRequest()->getNodeHandle());
            if (nexported)
            {
                char *nodepath = api->getNodePath(nexported);
                char *publiclink = nexported->getPublicLink();
                string publicPassProtectedLink = string();

                if (amIPro() && password.size() )
                {
                    MegaCmdListener *megaCmdListener2 = new MegaCmdListener(api, NULL);
                    api->encryptLinkWithPassword(publiclink, password.c_str(), megaCmdListener2);
                    megaCmdListener2->wait();
                    if (checkNoErrors(megaCmdListener2->getError(), "protect public link with password"))
                    {
                        publicPassProtectedLink = megaCmdListener2->getRequest()->getText();
                    }
                    delete megaCmdListener2;
                }
                else if (password.size())
                {
                    LOG_err << "Only PRO users can protect links with passwords. Showing UNPROTECTED link";
                }

                OUTSTREAM << "Exported " << nodepath << ": "
                          << (publicPassProtectedLink.size()?publicPassProtectedLink:publiclink);

                if (nexported->getWritableLinkAuthKey())
                {
                    string authKey(nexported->getWritableLinkAuthKey());
                    if (authKey.size())
                    {
                        string authToken((publicPassProtectedLink.size()?publicPassProtectedLink:publiclink));
                        authToken = authToken.substr(strlen("https://mega.nz/folder/")).append(":").append(authKey);
                        OUTSTREAM << "\n          AuthToken = " << authToken;
                    }
                }

                if (nexported->getExpirationTime())
                {
                    OUTSTREAM << " expires at " << getReadableTime(nexported->getExpirationTime());
                }
                OUTSTREAM << endl;
                delete[] nodepath;
                delete[] publiclink;
                delete nexported;
            }
            else
            {
                setCurrentOutCode(MCMD_NOTFOUND);
                LOG_err << "Exported node not found!";
            }
        }
        delete megaCmdListener;
    }
}

void MegaCmdExecuter::disableExport(MegaNode *n)
{
    if (!n->isExported())
    {
        setCurrentOutCode(MCMD_INVALIDSTATE);
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
            setCurrentOutCode(MCMD_NOTFOUND);
            LOG_err << "Exported node not found!";
        }
    }

    delete megaCmdListener;
}

void MegaCmdExecuter::shareNode(MegaNode *n, string with, int level)
{
    MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);

    api->share(n, with.c_str(), level, megaCmdListener);
    megaCmdListener->wait();
    if (checkNoErrors(megaCmdListener->getError(), ( level != MegaShare::ACCESS_UNKNOWN ) ? "share node" : "disable share"))
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
                OUTSTREAM << "Shared " << nodepath << " : " << megaCmdListener->getRequest()->getEmail()
                          << " accessLevel=" << megaCmdListener->getRequest()->getAccess() << endl;
            }
            delete[] nodepath;
            delete nshared;
        }
        else
        {
            setCurrentOutCode(MCMD_NOTFOUND);
            LOG_err << "Shared node not found!";
        }
    }

    delete megaCmdListener;
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

void MegaCmdExecuter::getInfoFromFolder(MegaNode *n, MegaApi *api, long long *nfiles, long long *nfolders, long long *nversions)
{
    std::unique_ptr<MegaCmdListener>megaCmdListener = ::mega::make_unique<MegaCmdListener>(nullptr);
    api->getFolderInfo(n, megaCmdListener.get());
    *nfiles = 0;
    *nfolders = 0;
    megaCmdListener->wait();
    if (checkNoErrors(megaCmdListener->getError(), "get folder info"))
    {
        MegaFolderInfo * mfi = megaCmdListener->getRequest()->getMegaFolderInfo();
        if (mfi)
        {
            *nfiles  = mfi->getNumFiles();
            *nfolders  = mfi->getNumFolders();
            if (nversions)
            {
                *nversions = mfi->getNumVersions();
            }
        }
    }
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
                    MegaNode * n = nodebypath(nodepath.c_str());
                    if (n)
                    {
                        if (n->getType() != MegaNode::TYPE_FILE)
                        {
                            nodepath += "/";
                        }
                        if (!( discardFiles && ( n->getType() == MegaNode::TYPE_FILE )))
                        {
                            paths.push_back(nodepath);
                        }

                        delete n;
                    }
                    else
                    {
                        LOG_debug << "Unexpected: matching path has no associated node: " << nodepath << ". Could have been deleted in the process";
                    }
                    delete ncwd;
                }
                else
                {
                    setCurrentOutCode(MCMD_INVALIDSTATE);
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

vector<string> MegaCmdExecuter::listlocalpathsstartingby(string askedPath, bool discardFiles)
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

#ifdef MEGACMDEXECUTER_FILESYSTEM
    for (fs::directory_iterator iter(fs::u8path(containingfolder)); iter != fs::directory_iterator(); ++iter)
    {
        if (!discardFiles || iter->status().type() == fs::file_type::directory)
        {
#ifdef _WIN32
            wstring path = iter->path().wstring();
#else
            string path = iter->path().string();
#endif
            if (removeprefix) path = path.substr(2);
            if (requiresseparatorafterunit) path.insert(2, 1, sep);
            if (iter->status().type() == fs::file_type::directory)
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
#endif
#ifdef _WIN32
            paths.push_back(toUtf8String(path));
#else
            paths.push_back(path);
#endif

        }
    }

#elif defined(HAVE_DIRENT_H)
    DIR *dir;
    if ((dir = opendir (containingfolder.c_str())) != NULL)
    {
        struct dirent *entry;
        while ((entry = readdir (dir)) != NULL)
        {
            if (!discardFiles || entry->d_type == DT_DIR)
            {
                string path = containingfolder;
                if (path != "/")
                    path.append(1, sep);
                path.append(entry->d_name);
                if (removeprefix) path = path.substr(2);
                if (path.size() && entry->d_type == DT_DIR)
                {
                    path.append(1, sep);
                }
                paths.push_back(path);
            }
        }

        closedir(dir);
    }
#endif
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

    MegaNode *n = nodebypath(nodePath.c_str());
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
        delete n;
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
#ifdef MEGACMDEXECUTER_FILESYSTEM
    for (fs::directory_iterator iter(fs::u8path(location)); iter != fs::directory_iterator(); ++iter)
    {
        toret.push_back(iter->path().filename().u8string());
    }

#elif defined(HAVE_DIRENT_H)
    DIR *dir;
    struct dirent *entry;
    if ((dir = opendir (location.c_str())) != NULL)
    {
        while ((entry = readdir (dir)) != NULL)
        {
            if (IsFolder(location + entry->d_name))
            {
                toret.push_back(entry->d_name + string("/"));
            }
            else
            {
                toret.push_back(entry->d_name);
            }
        }
        closedir (dir);
    }
#endif
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
        setCurrentOutCode(MCMD_INVALIDSTATE);
        LOG_err << "Please provide a valid name (with name and surname separated by \" \")";
        return;
    }

    OUTSTREAM << "Singinup up. name=" << firstname << ". surname=" << lastname<< endl;

    api->createAccount(email.c_str(), passwd.c_str(), firstname.c_str(), lastname.c_str(), megaCmdListener);
    megaCmdListener->wait();
    if (checkNoErrors(megaCmdListener->getError(), "create account <" + email + ">"))
    {
        OUTSTREAM << "Account <" << email << "> created succesfully. You will receive a confirmation link. Use \"confirm\" with the provided link to confirm that account" << endl;
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
        OUTSTREAM << "Account " << email << " confirmed succesfully. You can login with it now" << endl;
    }

    delete megaCmdListener2;
}

void MegaCmdExecuter::confirmWithPassword(string passwd)
{
    return confirm(passwd, login, link);
}

bool MegaCmdExecuter::IsFolder(string path)
{
#ifdef _WIN32
    replaceAll(path,"/","\\");
#endif
    LocalPath localpath = LocalPath::fromPath(path, *fsAccessCMD);
    std::unique_ptr<FileAccess> fa = fsAccessCMD->newfileaccess();
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
#ifdef ENABLE_BACKUPS
    else if (transfer->isBackupTransfer())
    {
#ifdef _WIN32
        OUTSTREAM << "B";
#else
        OUTSTREAM << "\u23eb";
#endif
    }
#endif
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
    type += getutf8fromUtf16((transfer->getType() == MegaTransfer::TYPE_DOWNLOAD)?L"\u25bc":L"\u25b2");
#else
    type += (transfer->getType() == MegaTransfer::TYPE_DOWNLOAD)?"\u21d3":"\u21d1";
#endif
    //TODO: handle TYPE_LOCAL_TCP_DOWNLOAD

    //type (transfer/normal)
    if (transfer->isSyncTransfer())
    {
#ifdef _WIN32
        type += getutf8fromUtf16(L"\u21a8");
#else
        type += "\u21f5";
#endif
    }
#ifdef ENABLE_BACKUPS
    else if (transfer->isBackupTransfer())
    {
#ifdef _WIN32
        type += getutf8fromUtf16(L"\u2191");
#else
        type += "\u23eb";
#endif
    }
#endif

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

void MegaCmdExecuter::printSyncHeader(ColumnDisplayer &cd)
{
    // add headers with unfixed width (those that can be ellided)
    cd.addHeader("LOCALPATH", false);
    cd.addHeader("REMOTEPATH", false);
    return;

}

#ifdef ENABLE_BACKUPS

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
                OUTSTREAM << "  " << " -- SAVED BACKUPS --" << endl;

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
                MegaNode *backupInstanceNode = nodebypath(msl->get(i));
                if (backupInstanceNode)
                {
                    backupInstanceStatus = backupInstanceNode->getCustomAttr("BACKST");

                    getNumFolderFiles(backupInstanceNode, api, &nfiles, &nfolders);

                }

                delete backupInstanceNode;
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
        printBackupSummary(backupstruct->tag, backupstruct->localpath.c_str(),"UNKOWN"," FAILED", PATHSIZE);
        if (extendedinfo)
        {
            string speriod = (backupstruct->period == -1)?backupstruct->speriod:getReadablePeriod(backupstruct->period/10);
            OUTSTREAM << "         Period: " << "\"" << speriod << "\"" << endl;
            OUTSTREAM << "   Max. Backups: " << backupstruct->numBackups << endl;
        }
    }
}
#endif

void MegaCmdExecuter::printSync(MegaSync *sync, long long nfiles, long long nfolders, megacmd::ColumnDisplayer &cd,  std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions)
{
    cd.addValue("ID", syncBackupIdToBase64(sync->getBackupId()));

    cd.addValue("LOCALPATH", sync->getLocalFolder());

    if (getFlag(clflags, "show-handles"))
    {
        cd.addValue("REMOTEHANDLE", string("<H:").append(handleToBase64(sync->getMegaHandle()).append(">")));
    }

    cd.addValue("REMOTEPATH", sync->getLastKnownMegaFolder());

    string state;
    if (sync->isTemporaryDisabled())
    {
        state = "TempDisabled";
    }
    else if (sync->isActive())
    {
        state = "Enabled";
    }
    else if (!sync->isEnabled())
    {
        state = "Disabled";
    }
    else
    {
        state = "Failed";
    }
    cd.addValue("ACTIVE", state);

    string pathstate;
    if (!sync->isActive())
    {
        pathstate = "Inactive";
    }
    else
    {
        string pathLocalFolder(sync->getLocalFolder());
        pathLocalFolder = rtrim(pathLocalFolder, '/');
    #ifdef _WIN32
        pathLocalFolder = rtrim(pathLocalFolder, '\\');
    #endif
        string localizedLocalPath;
        fsAccessCMD->path2local(&pathLocalFolder,&localizedLocalPath);

        int statepath = api->syncPathState(&localizedLocalPath);
        pathstate = getSyncPathStateStr(statepath);
    }
    cd.addValue("STATUS", pathstate);

    cd.addValue("ERROR", sync->getError() ? sync->getMegaSyncErrorCode() : "NO");

    std::unique_ptr<MegaNode> n{api->getNodeByHandle(sync->getMegaHandle())};
    cd.addValue("SIZE", sizeToText(api->getSize(n.get())));
    cd.addValue("FILES", SSTR(nfiles));
    cd.addValue("DIRS", SSTR(nfolders));

    return;
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
            if (printfileinfo)
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
    string absolutePath = "Unknown";
    LocalPath localRelativePath = LocalPath::fromPath(relativePath, *fsAccessCMD);
    LocalPath localAbsolutePath;
    if (fsAccessCMD->expanselocalpath(localRelativePath, localAbsolutePath))
    {
        absolutePath = localAbsolutePath.toPath(*fsAccessCMD);
    }

    return absolutePath;
}


void MegaCmdExecuter::move(MegaNode * n, string destiny)
{
    MegaNode* tn; //target node
    string newname;

    // source node must exist
    if (!n)
    {
        return;
    }


    char * nodepath = api->getNodePath(n);
    LOG_debug << "Moving : " << nodepath << " to " << destiny;
    delete []nodepath;

    // we have four situations:
    // 1. target path does not exist - fail
    // 2. target node exists and is folder - move
    // 3. target node exists and is file - delete and rename (unless same)
    // 4. target path exists, but filename does not - rename
    if (( tn = nodebypath(destiny.c_str(), NULL, &newname)))
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
                    setCurrentOutCode(MCMD_INVALIDTYPE);
                    LOG_err << destiny << ": Not a directory";
                    delete tn;
                    delete n;
                    return;
                }
                else //move and rename!
                {
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                    api->moveNode(n, tn, megaCmdListener);
                    megaCmdListener->wait();
                    if (checkNoErrors(megaCmdListener->getError(), "move"))
                    {
                        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                        api->renameNode(n, newname.c_str(), megaCmdListener);
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
                    MegaNode *tnParentNode = api->getNodeByHandle(tn->getParentHandle());
                    if (tnParentNode)
                    {
                        delete tnParentNode;

                        //move into the parent of target node
                        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                        std::unique_ptr<MegaNode> parentNode {api->getNodeByHandle(tn->getParentHandle())};
                        api->moveNode(n, parentNode.get(), megaCmdListener);
                        megaCmdListener->wait();
                        if (checkNoErrors(megaCmdListener->getError(), "move node"))
                        {
                            const char* name_to_replace = tn->getName();

                            //remove (replaced) target node
                            if (n != tn) //just in case moving to same location
                            {
                                MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                                api->remove(tn, megaCmdListener); //remove target node
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
                                api->renameNode(n, name_to_replace, megaCmdListener);
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
                        setCurrentOutCode(MCMD_INVALIDSTATE);
                        LOG_fatal << "Destiny node is orphan!!!";
                    }
                }
                else // target is a folder
                {
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                    api->moveNode(n, tn, megaCmdListener);
                    megaCmdListener->wait();
                    checkNoErrors(megaCmdListener->getError(), "move node");
                    delete megaCmdListener;
                }
            }
        }
        delete tn;
    }
    else //target not found (not even its folder), cant move
    {
        setCurrentOutCode(MCMD_NOTFOUND);
        LOG_err << destiny << ": No such directory";
    }
}


bool MegaCmdExecuter::isValidFolder(string destiny)
{
    bool isdestinyavalidfolder = true;
    MegaNode *ndestiny = nodebypath(destiny.c_str());;
    if (ndestiny)
    {
        if (ndestiny->getType() == MegaNode::TYPE_FILE)
        {
            isdestinyavalidfolder = false;
        }
        delete ndestiny;
    }
    else
    {
        isdestinyavalidfolder = false;
    }
    return isdestinyavalidfolder;
}

void MegaCmdExecuter::restartsyncs()
{
    std::unique_ptr<MegaSyncList> syncs{api->getSyncs()};
    for (int i = 0; i < syncs->size(); i++)
    {
        MegaSync *sync = syncs->get(i);
        if (sync->isActive())
        {
            LOG_info << "Restarting sync "<< sync->getLocalFolder() << " to " << sync->getLastKnownMegaFolder();

            auto megaCmdListener = ::mega::make_unique<MegaCmdListener>(nullptr);
            api->disableSync(sync, megaCmdListener.get());
            megaCmdListener->wait();

            if (checkNoErrors(megaCmdListener->getError(), "disable sync"))
            {
                LOG_verbose << "Disabled sync "<< sync->getLocalFolder() << " to " << sync->getLastKnownMegaFolder();

                auto megaCmdListener = ::mega::make_unique<MegaCmdListener>(nullptr);
                api->enableSync(sync, megaCmdListener.get());
                megaCmdListener->wait();

                if (checkNoErrors(megaCmdListener->getError(), "re-enable sync"))
                {
                    LOG_verbose << "Re-enabled sync "<< sync->getLocalFolder() << " to " << sync->getLastKnownMegaFolder();
                }
                else
                {
                    setCurrentOutCode(MCMD_INVALIDSTATE);
                    LOG_err << "Failed to restart sync: " << sync->getLocalFolder() << " to " << sync->getLastKnownMegaFolder()
                            << ". You will need to manually reenable or restart MEGAcmd";
                }
            }
        }
    }
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
                            setCurrentOutCode(MCMD_INVALIDSTATE);
                            LOG_fatal << "Destiny node is orphan!!!";
                        }
                    }
                    else
                    {
                        setCurrentOutCode(MCMD_INVALIDTYPE);
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
        setCurrentOutCode(MCMD_NOTFOUND);
        LOG_err << destiny << " Couldn't find destination";
    }
}


#ifdef ENABLE_BACKUPS
bool MegaCmdExecuter::establishBackup(string pathToBackup, MegaNode *n, int64_t period, string speriod,  int numBackups)
{
    bool attendpastbackups = true; //TODO: receive as parameter
    static int backupcounter = 0;
    LocalPath localrelativepath = LocalPath::fromPath(pathToBackup, *fsAccessCMD);
    LocalPath localabsolutepath;
    fsAccessCMD->expanselocalpath(localrelativepath, localabsolutepath);

    MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);
    api->setScheduledCopy(localabsolutepath.toPath(*fsAccessCMD).c_str(), n, attendpastbackups, period, speriod.c_str(), numBackups, megaCmdListener);
    megaCmdListener->wait();
    if (checkNoErrors(megaCmdListener->getError(), "establish backup"))
    {
        mtxBackupsMap.lock();

        backup_struct *thebackup = NULL;
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
        }
        thebackup->active = true;
        thebackup->handle = megaCmdListener->getRequest()->getNodeHandle();
        thebackup->localpath = string(megaCmdListener->getRequest()->getFile());
        thebackup->numBackups = numBackups;
        thebackup->period = period;
        thebackup->speriod = speriod;
        thebackup->failed = false;
        thebackup->tag = megaCmdListener->getRequest()->getTransferTag();

        char * nodepath = api->getNodePath(n);
        LOG_info << "Added backup: " << megaCmdListener->getRequest()->getFile() << " to " << nodepath;
        mtxBackupsMap.unlock();
        delete []nodepath;
        delete megaCmdListener;

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
    delete megaCmdListener;
    return false;
}
#endif

void MegaCmdExecuter::confirmCancel(const char* confirmlink, const char* pass)
{
    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
    api->confirmCancelAccount(confirmlink, pass, megaCmdListener);
    megaCmdListener->wait();
    if (megaCmdListener->getError()->getErrorCode() == MegaError::API_ETOOMANY)
    {
        LOG_err << "Confirm cancel account failed: too many attempts";
    }
    else if (megaCmdListener->getError()->getErrorCode() == MegaError::API_ENOENT
             || megaCmdListener->getError()->getErrorCode() == MegaError::API_EKEY)
    {
        LOG_err << "Confirm cancel account failed: invalid link/password";
    }
    else if (checkNoErrors(megaCmdListener->getError(), "confirm cancel account"))
    {
        OUTSTREAM << "CONFIRM Account cancelled succesfully" << endl;
        MegaCmdListener *megaCmdListener2 = new MegaCmdListener(NULL);
        api->localLogout(megaCmdListener2);
        actUponLogout(megaCmdListener2, false);
        delete megaCmdListener2;
    }
    delete megaCmdListener;
}

void MegaCmdExecuter::processPath(string path, bool usepcre, bool &firstone, void (*nodeprocessor)(MegaCmdExecuter *, MegaNode *, bool), MegaCmdExecuter *context)
{

    if (isRegExp(path))
    {
        vector<MegaNode *> *nodes = nodesbypath(path.c_str(), usepcre);
        if (nodes)
        {
            if (!nodes->size())
            {
                setCurrentOutCode(MCMD_NOTFOUND);
                LOG_err << "Path not found: " << path;
            }
            for (std::vector< MegaNode * >::iterator it = nodes->begin(); it != nodes->end(); ++it)
            {
                MegaNode * n = *it;
                if (n)
                {
                    nodeprocessor(context, n, firstone);
                    firstone = false;
                    delete n;
                }
                else
                {
                    setCurrentOutCode(MCMD_NOTFOUND);
                    LOG_err << "Path not found: " << path;
                    return;
                }
            }
        }
    }
    else // non-regexp
    {
        MegaNode *n = nodebypath(path.c_str());
        if (n)
        {
            nodeprocessor(context, n, firstone);
            firstone = false;
            delete n;
        }
        else
        {
            setCurrentOutCode(MCMD_NOTFOUND);
            LOG_err << "Path not found: " << path;
            return;
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
        setCurrentOutCode(MCMD_NOTFOUND);
        LOG_err << (name.size()?name:actualNodePath) << " is not served via webdav";
    }
    delete []actualNodePath;
}

void MegaCmdExecuter::addWebdavLocation(MegaNode *n, bool firstone, string name)
{
    char *actualNodePath = api->getNodePath(n);
    char *l = api->httpServerGetLocalWebDavLink(n);
    OUTSTREAM << "Serving via webdav " << (name.size()?name:actualNodePath) << ": " << l << endl;

    mtxWebDavLocations.lock();
    list<string> servedpaths = ConfigurationManager::getConfigurationValueList("webdav_served_locations");
    servedpaths.push_back(actualNodePath);
    servedpaths.sort();
    servedpaths.unique();
    ConfigurationManager::savePropertyValueList("webdav_served_locations", servedpaths);
    mtxWebDavLocations.unlock();

    delete []l;
    delete []actualNodePath;
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
        setCurrentOutCode(MCMD_NOTFOUND);
        LOG_err << (name.size()?name:actualNodePath)  << " is not served via ftp";
    }
    delete []actualNodePath;
}

void MegaCmdExecuter::addFtpLocation(MegaNode *n, bool firstone, string name)
{
    char *actualNodePath = api->getNodePath(n);
    char *l = api->ftpServerGetLocalLink(n);
    OUTSTREAM << "Serving via ftp " << (name.size()?name:n->getName())  << ": " << l << endl;

    mtxFtpLocations.lock();
    list<string> servedpaths = ConfigurationManager::getConfigurationValueList("ftp_served_locations");
    servedpaths.push_back(actualNodePath);
    servedpaths.sort();
    servedpaths.unique();
    ConfigurationManager::savePropertyValueList("ftp_served_locations", servedpaths);
    mtxFtpLocations.unlock();

    delete []l;
    delete []actualNodePath;
}

#endif


void MegaCmdExecuter::catFile(MegaNode *n)
{
    if (n->getType() != MegaNode::TYPE_FILE)
    {
        LOG_err << " Unable to cat: not a file";
        setCurrentOutCode(MCMD_EARGS);
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

void MegaCmdExecuter::executecommand(vector<string> words, map<string, int> *clflags, map<string, string> *cloptions)
{
    MegaNode* n = NULL;
    if (words[0] == "ls")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
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
                            MegaNode * n = nodebypath(nodepath.c_str());
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
                                    dumpTreeSummary(n, getTimeFormatFromSTR(getOption(cloptions, "time-format","SHORT")), clflags, cloptions, recursive, show_versions, 0, humanreadable, rNpath);
                                }
                                else
                                {
                                    vector<bool> lfs;
                                    dumptree(n, treelike, lfs, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, recursive, extended_info, show_versions, 0, rNpath);
                                }
                                if (( !n->getType() == MegaNode::TYPE_FILE ) && (( it + 1 ) != pathsToList->end()))
                                {
                                    OUTSTREAM << endl;
                                }
                                delete n;
                            }
                            else
                            {
                                LOG_debug << "Unexpected: matching path has no associated node: " << nodepath << ". Could have been deleted in the process";
                            }
                            delete ncwd;
                        }
                        else
                        {
                            setCurrentOutCode(MCMD_INVALIDSTATE);
                            LOG_err << "Couldn't find woking folder (it might been deleted)";
                        }
                    }
                    pathsToList->clear();
                    delete pathsToList;
                }
                else
                {
                    setCurrentOutCode(MCMD_NOTFOUND);
                    LOG_err << "Couldn't find \"" << words[1] << "\"";
                }
            }
            else
            {
                n = nodebypath(words[1].c_str());
                if (n)
                {
                    if (summary)
                    {
                        if (firstprint)
                        {
                            dumpNodeSummaryHeader(getTimeFormatFromSTR(getOption(cloptions, "time-format","SHORT")), clflags, cloptions);
                            firstprint = false;
                        }
                        dumpTreeSummary(n, getTimeFormatFromSTR(getOption(cloptions, "time-format","SHORT")), clflags, cloptions, recursive, show_versions, 0, humanreadable, rNpath);
                    }
                    else
                    {
                        if (treelike) OUTSTREAM << words[1] << endl;
                        vector<bool> lfs;
                        dumptree(n, treelike, lfs, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, recursive, extended_info, show_versions, 0, rNpath);
                    }
                    delete n;
                }
                else
                {
                    setCurrentOutCode(MCMD_NOTFOUND);
                    LOG_err << "Couldn't find " << words[1];
                }
            }
        }
        else
        {
            n = api->getNodeByHandle(cwd);
            if (n)
            {
                if (summary)
                {
                    if (firstprint)
                    {
                        dumpNodeSummaryHeader(getTimeFormatFromSTR(getOption(cloptions, "time-format","SHORT")), clflags, cloptions);
                        firstprint = false;
                    }
                    dumpTreeSummary(n, getTimeFormatFromSTR(getOption(cloptions, "time-format","SHORT")), clflags, cloptions, recursive, show_versions, 0, humanreadable);
                }
                else
                {
                    if (treelike) OUTSTREAM << "." << endl;
                    vector<bool> lfs;
                    dumptree(n, treelike, lfs, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, recursive, extended_info, show_versions);
                }
                delete n;
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
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }

        m_time_t minTime = -1;
        m_time_t maxTime = -1;
        string mtimestring = getOption(cloptions, "mtime", "");
        if ("" != mtimestring && !getMinAndMaxTime(mtimestring, &minTime, &maxTime))
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "Invalid time " << mtimestring;
            return;
        }

        int64_t minSize = -1;
        int64_t maxSize = -1;
        string sizestring = getOption(cloptions, "size", "");
        if ("" != sizestring && !getMinAndMaxSize(sizestring, &minSize, &maxSize))
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "Invalid time " << sizestring;
            return;
        }


        if (words.size() <= 1)
        {
            n = api->getNodeByHandle(cwd);
            doFind(n, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, "", printfileinfo, pattern, getFlag(clflags,"use-pcre"), minTime, maxTime, minSize, maxSize);
            delete n;
        }
        for (int i = 1; i < (int)words.size(); i++)
        {
            if (isRegExp(words[i]))
            {
                vector<MegaNode *> *nodesToFind = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                if (nodesToFind->size())
                {
                    for (std::vector< MegaNode * >::iterator it = nodesToFind->begin(); it != nodesToFind->end(); ++it)
                    {
                        MegaNode * nodeToFind = *it;
                        if (nodeToFind)
                        {
                            doFind(nodeToFind, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, words[i], printfileinfo, pattern, getFlag(clflags,"use-pcre"), minTime, maxTime, minSize, maxSize);
                            delete nodeToFind;
                        }
                    }
                    nodesToFind->clear();
                }
                else
                {
                    setCurrentOutCode(MCMD_NOTFOUND);
                    LOG_err << words[i] << ": No such file or directory";
                }
                delete nodesToFind;
            }
            else
            {
                n = nodebypath(words[i].c_str());
                if (!n)
                {
                    setCurrentOutCode(MCMD_NOTFOUND);
                    LOG_err << "Couldn't find " << words[i];
                }
                else
                {
                    doFind(n, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, words[i], printfileinfo, pattern, getFlag(clflags,"use-pcre"), minTime, maxTime, minSize, maxSize);
                    delete n;
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
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("update");
        }

        return;
    }
#endif
    else if (words[0] == "cd")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 1)
        {
            if (( n = nodebypath(words[1].c_str())))
            {
                if (n->getType() == MegaNode::TYPE_FILE)
                {
                    setCurrentOutCode(MCMD_NOTFOUND);
                    LOG_err << words[1] << ": Not a directory";
                }
                else
                {
                    cwd = n->getHandle();

                    updateprompt(api);
                }
                delete n;
            }
            else
            {
                setCurrentOutCode(MCMD_NOTFOUND);
                LOG_err << words[1] << ": No such file or directory";
            }
        }
        else
        {
            MegaNode * rootNode = api->getRootNode();
            if (!rootNode)
            {
                LOG_err << "nodes not fetched";
                setCurrentOutCode(MCMD_NOFETCH);
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
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 1)
        {
            if (interactiveThread() && nodesToConfirmDelete.size())
            {
                //clear all previous nodes to confirm delete (could have been not cleared in case of ctrl+c)
                for (std::vector< MegaNode * >::iterator it = nodesToConfirmDelete.begin(); it != nodesToConfirmDelete.end(); ++it)
                {
                    delete *it;
                }
                nodesToConfirmDelete.clear();
            }

            bool force = getFlag(clflags, "f");
            bool none = false;

            for (unsigned int i = 1; i < words.size(); i++)
            {
                unescapeifRequired(words[i]);
                if (isRegExp(words[i]))
                {
                    vector<MegaNode *> *nodesToDelete = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                    if (nodesToDelete->size())
                    {
                        for (std::vector< MegaNode * >::iterator it = nodesToDelete->begin(); !none && it != nodesToDelete->end(); ++it)
                        {
                            MegaNode * nodeToDelete = *it;
                            if (nodeToDelete)
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
                        nodesToDelete->clear();
                    }
                    else
                    {
                        setCurrentOutCode(MCMD_NOTFOUND);
                        LOG_err << words[i] << ": No such file or directory";
                    }
                    delete nodesToDelete;
                }
                else if (!none)
                {
                    MegaNode * nodeToDelete = nodebypath(words[i].c_str());
                    if (nodeToDelete)
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
                    else
                    {
                        setCurrentOutCode(MCMD_NOTFOUND);
                        LOG_err << words[i] << ": No such file or directory";
                    }
                }
            }
        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("rm");
        }

        return;
    }
    else if (words[0] == "mv")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 2)
        {
            string destiny = words[words.size()-1];
            unescapeifRequired(destiny);

            if (words.size() > 3 && !isValidFolder(destiny))
            {
                setCurrentOutCode(MCMD_INVALIDTYPE);
                LOG_err << destiny << " must be a valid folder";
                return;
            }

            for (unsigned int i=1;i<(words.size()-1);i++)
            {
                string source = words[i];
                unescapeifRequired(source);

                if (isRegExp(source))
                {
                    vector<MegaNode *> *nodesToList = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                    if (nodesToList)
                    {
                        if (!nodesToList->size())
                        {
                            setCurrentOutCode(MCMD_NOTFOUND);
                            LOG_err << source << ": No such file or directory";
                        }

                        bool destinyisok=true;
                        if (nodesToList->size() > 1 && !isValidFolder(destiny))
                        {
                            destinyisok = false;
                            setCurrentOutCode(MCMD_INVALIDTYPE);
                            LOG_err << destiny << " must be a valid folder";
                        }

                        if (destinyisok)
                        {
                            for (std::vector< MegaNode * >::iterator it = nodesToList->begin(); it != nodesToList->end(); ++it)
                            {
                                MegaNode * n = *it;
                                if (n)
                                {
                                    move(n, destiny);
                                    delete n;
                                }
                            }
                        }

                        nodesToList->clear();
                        delete nodesToList;
                    }
                }
                else
                {
                    if (( n = nodebypath(source.c_str())) )
                    {
                        move(n, destiny);
                        delete n;
                    }
                    else
                    {
                        setCurrentOutCode(MCMD_NOTFOUND);
                        LOG_err << source << ": No such file or directory";
                    }
                }
            }

        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("mv");
        }

        return;
    }
    else if (words[0] == "cp")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 2)
        {
            string destiny = words[words.size()-1];
            string targetuser;
            string newname;
            MegaNode *tn = nodebypath(destiny.c_str(), &targetuser, &newname);

            if (words.size() > 3 && !isValidFolder(destiny) && !targetuser.size())
            {
                setCurrentOutCode(MCMD_INVALIDTYPE);
                LOG_err << destiny << " must be a valid folder";
                return;
            }

            for (unsigned int i=1;i<(words.size()-1);i++)
            {
                string source = words[i];

                if (isRegExp(source))
                {
                    vector<MegaNode *> *nodesToCopy = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                    if (nodesToCopy)
                    {
                        if (!nodesToCopy->size())
                        {
                            setCurrentOutCode(MCMD_NOTFOUND);
                            LOG_err << source << ": No such file or directory";
                        }

                        bool destinyisok=true;
                        if (nodesToCopy->size() > 1 && !isValidFolder(destiny) && !targetuser.size())
                        {
                            destinyisok = false;
                            setCurrentOutCode(MCMD_INVALIDTYPE);
                            LOG_err << destiny << " must be a valid folder";
                        }

                        if (destinyisok)
                        {
                            for (std::vector< MegaNode * >::iterator it = nodesToCopy->begin(); it != nodesToCopy->end(); ++it)
                            {
                                MegaNode * n = *it;
                                if (n)
                                {
                                    copyNode(n, destiny, tn, targetuser, newname);
                                    delete n;
                                }
                            }
                        }
                        nodesToCopy->clear();
                        delete nodesToCopy;
                    }
                }
                else if (( n = nodebypath(source.c_str())))
                {
                    copyNode(n, destiny, tn, targetuser, newname);
                    delete n;
                }
                else
                {
                    setCurrentOutCode(MCMD_NOTFOUND);
                    LOG_err << source << ": No such file or directory";
                }
            }
            delete tn;
        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("cp");
        }

        return;
    }
    else if (words[0] == "du")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
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
                vector<MegaNode *> *nodesToList = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                if (nodesToList)
                {
                    for (std::vector< MegaNode * >::iterator it = nodesToList->begin(); it != nodesToList->end(); ++it)
                    {
                        MegaNode * n = *it;
                        if (n)
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
                            currentSize = api->getSize(n);
                            totalSize += currentSize;

                            dpath = getDisplayPath(words[i], n);
                            OUTSTREAM << getFixLengthString(dpath+":",PATHSIZE) << getFixLengthString(sizeToText(currentSize, true, humanreadable), 12, ' ', true);
                            if (show_versions_size)
                            {
                                long long sizeWithVersions = getVersionsSize(n);
                                OUTSTREAM << getFixLengthString(sizeToText(sizeWithVersions, true, humanreadable), 12, ' ', true);
                                totalVersionsSize += sizeWithVersions;
                            }

                            OUTSTREAM << endl;
                            delete n;
                        }
                    }

                    nodesToList->clear();
                    delete nodesToList;
                }
            }
            else
            {
                if (!( n = nodebypath(words[i].c_str())))
                {
                    setCurrentOutCode(MCMD_NOTFOUND);
                    LOG_err << words[i] << ": No such file or directory";
                    return;
                }

                currentSize = api->getSize(n);
                totalSize += currentSize;
                dpath = getDisplayPath(words[i], n);
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
                        long long sizeWithVersions = getVersionsSize(n);
                        OUTSTREAM << getFixLengthString(sizeToText(sizeWithVersions, true, humanreadable), 12, ' ', true);
                        totalVersionsSize += sizeWithVersions;
                    }
                    OUTSTREAM << endl;

                }
                delete n;
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
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("cat");
            return;
        }

        for (int i = 1; i < (int)words.size(); i++)
        {
            if (isPublicLink(words[i]))
            {
                string publicLink = words[i];
                if (isEncryptedLink(publicLink))
                {
                    string linkPass = getOption(cloptions, "password", "");
                    if (!linkPass.size())
                    {
                        linkPass = askforUserResponse("Enter password: ");
                    }

                    if (linkPass.size())
                    {
                        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                        api->decryptPasswordProtectedLink(publicLink.c_str(), linkPass.c_str(), megaCmdListener);
                        megaCmdListener->wait();
                        if (checkNoErrors(megaCmdListener->getError(), "decrypt password protected link"))
                        {
                            publicLink = megaCmdListener->getRequest()->getText();
                            delete megaCmdListener;
                        }
                        else
                        {
                            setCurrentOutCode(MCMD_NOTPERMITTED);
                            LOG_err << "Invalid password";
                            delete megaCmdListener;
                            return;
                        }
                    }
                    else
                    {
                        setCurrentOutCode(MCMD_EARGS);
                        LOG_err << "Need a password to decrypt provided link (--password=PASSWORD)";
                        return;
                    }
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
                            setCurrentOutCode(MCMD_EARGS);
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
                    setCurrentOutCode(MCMD_EARGS);
                }
            }
            else if (!api->isFilesystemAvailable())
            {
                setCurrentOutCode(MCMD_NOTLOGGEDIN);
                LOG_err << "Unable to cat " << words[i] << ": Not logged in.";
            }
            else
            {
                unescapeifRequired(words[i]);
                if (isRegExp(words[i]))
                {
                    vector<MegaNode *> *nodes = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                    if (nodes)
                    {
                        if (!nodes->size())
                        {
                            setCurrentOutCode(MCMD_NOTFOUND);
                            LOG_err << "Nodes not found: " << words[i];
                        }
                        for (std::vector< MegaNode * >::iterator it = nodes->begin(); it != nodes->end(); ++it)
                        {
                            MegaNode * n = *it;
                            if (n)
                            {
                                catFile(n);
                                delete n;
                            }
                        }
                    }
                }
                else
                {
                    MegaNode *n = nodebypath(words[i].c_str());
                    if (n)
                    {
                        catFile(n);
                        delete n;
                    }
                    else
                    {
                        setCurrentOutCode(MCMD_NOTFOUND);
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
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() < 2)
        {
            setCurrentOutCode(MCMD_EARGS);
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
                vector<MegaNode *> *nodes = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                if (nodes)
                {
                    if (!nodes->size())
                    {
                        setCurrentOutCode(MCMD_NOTFOUND);
                        LOG_err << "Nodes not found: " << words[i];
                    }
                    for (std::vector< MegaNode * >::iterator it = nodes->begin(); it != nodes->end(); ++it)
                    {
                        MegaNode * n = *it;
                        if (n)
                        {
                            printInfoFile(n, firstone, PATHSIZE);
                            delete n;
                        }
                    }
                }
            }
            else
            {
                MegaNode *n = nodebypath(words[i].c_str());
                if (n)
                {
                    printInfoFile(n, firstone, PATHSIZE);
                    delete n;
                }
                else
                {
                    setCurrentOutCode(MCMD_NOTFOUND);
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
                if (isEncryptedLink(publicLink))
                {
                    string linkPass = getOption(cloptions, "password", "");
                    if (!linkPass.size())
                    {
                        linkPass = askforUserResponse("Enter password: ");
                    }

                    if (linkPass.size())
                    {
                        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                        api->decryptPasswordProtectedLink(publicLink.c_str(), linkPass.c_str(), megaCmdListener);
                        megaCmdListener->wait();
                        if (checkNoErrors(megaCmdListener->getError(), "decrypt password protected link"))
                        {
                            publicLink = megaCmdListener->getRequest()->getText();
                            delete megaCmdListener;
                        }
                        else
                        {
                            setCurrentOutCode(MCMD_NOTPERMITTED);
                            LOG_err << "Invalid password";
                            delete megaCmdListener;
                            return;
                        }
                    }
                    else
                    {
                        setCurrentOutCode(MCMD_EARGS);
                        LOG_err << "Need a password to decrypt provided link (--password=PASSWORD)";
                        return;
                    }
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
                                setCurrentOutCode(MCMD_NOTPERMITTED);
                                LOG_err << "Write not allowed in " << path;
                                return;
                            }
                        }
                        else
                        {
                            if (!TestCanWriteOnContainingFolder(&path))
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
                                setCurrentOutCode(MCMD_NOTPERMITTED);
                                LOG_err << "Write not allowed in " << words[2];
                                return;
                            }
                        }
                        else
                        {
                            setCurrentOutCode(MCMD_INVALIDTYPE);
                            LOG_err << words[2] << " is not a valid Download Folder";
                            return;
                        }
                    }

                    MegaApi* apiFolder = getFreeApiFolder();
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
                                setCurrentOutCode(MCMD_INVALIDSTATE);
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
                    }
                    delete megaCmdListener;
                    freeApiFolder(apiFolder);
                }
                else
                {
                    setCurrentOutCode(MCMD_INVALIDTYPE);
                    LOG_err << "Invalid link: " << publicLink;
                }
            }
            else //remote file
            {
                if (!api->isFilesystemAvailable())
                {
                    setCurrentOutCode(MCMD_NOTLOGGEDIN);
                    LOG_err << "Not logged in.";
                    return;
                }
                unescapeifRequired(words[1]);

                if (isRegExp(words[1]))
                {
                    vector<MegaNode *> *nodesToGet = nodesbypath(words[1].c_str(), getFlag(clflags,"use-pcre"));
                    if (nodesToGet)
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
                                    setCurrentOutCode(MCMD_NOTPERMITTED);
                                    LOG_err << "Write not allowed in " << words[2];
                                    for (std::vector< MegaNode * >::iterator it = nodesToGet->begin(); it != nodesToGet->end(); ++it)
                                    {
                                        delete (MegaNode *)*it;
                                    }
                                    delete nodesToGet;
                                    return;
                                }
                            }
                            else if (nodesToGet->size()>1) //several files into one file!
                            {
                                setCurrentOutCode(MCMD_INVALIDTYPE);
                                LOG_err << words[2] << " is not a valid Download Folder";
                                for (std::vector< MegaNode * >::iterator it = nodesToGet->begin(); it != nodesToGet->end(); ++it)
                                {
                                    delete (MegaNode *)*it;
                                }
                                delete nodesToGet;
                                return;
                            }
                            else //destiny non existing or a file
                            {
                                if (!TestCanWriteOnContainingFolder(&path))
                                {
                                    for (std::vector< MegaNode * >::iterator it = nodesToGet->begin(); it != nodesToGet->end(); ++it)
                                    {
                                        delete (MegaNode *)*it;
                                    }
                                    delete nodesToGet;
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
                        for (std::vector< MegaNode * >::iterator it = nodesToGet->begin(); it != nodesToGet->end(); ++it)
                        {
                            MegaNode * n = *it;
                            if (n)
                            {
                                downloadNode(words[1], path, api, n, background, ignorequotawarn, clientID, megaCmdMultiTransferListener);
                                delete n;
                            }
                        }
                        if (!nodesToGet->size())
                        {
                            setCurrentOutCode(MCMD_NOTFOUND);
                            LOG_err << "Couldn't find " << words[1];
                        }

                        nodesToGet->clear();
                        delete nodesToGet;
                    }
                }
                else //not regexp
                {
                    MegaNode *n = nodebypath(words[1].c_str());
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
                                        setCurrentOutCode(MCMD_NOTPERMITTED);
                                        LOG_err << "Write not allowed in " << words[2];
                                        return;
                                    }
                                }
                                else
                                {
                                    if (!TestCanWriteOnContainingFolder(&path))
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
                                        setCurrentOutCode(MCMD_NOTPERMITTED);
                                        LOG_err << "Write not allowed in " << words[2];
                                        return;
                                    }
                                }
                                else
                                {
                                    setCurrentOutCode(MCMD_INVALIDTYPE);
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
                        downloadNode(words[1], path, api, n, background, ignorequotawarn, clientID, megaCmdMultiTransferListener);
                        delete n;
                    }
                    else
                    {
                        setCurrentOutCode(MCMD_NOTFOUND);
                        LOG_err << "Couldn't find file";
                    }
                }
            }

            if (background) //TODO: do the same for uploads?
            {
#ifdef HAVE_DOWNLOADS_COMMAND
                megaCmdMultiTransferListener->waitMultiStart();

                for (auto & dlId : megaCmdMultiTransferListener->getStartedTransfers())
                {
                    OUTSTREAM << "Started background transfer <" << dlId.mPath << ">. Tag = " << dlId.mTag << ". Object Identifier: " << dlId.getObjectID() << endl;
                }
#endif
            }
            else
            {
                megaCmdMultiTransferListener->waitMultiEnd();

                if (megaCmdMultiTransferListener->getFinalerror() != MegaError::API_OK)
                {
                    setCurrentOutCode(megaCmdMultiTransferListener->getFinalerror());
                    LOG_err << "Download failed. error code: " << MegaError::getErrorString(megaCmdMultiTransferListener->getFinalerror());
                }

                if (megaCmdMultiTransferListener->getProgressinformed() || getCurrentOutCode() == MCMD_OK )
                {
                    informProgressUpdate(PROGRESS_COMPLETE, megaCmdMultiTransferListener->getTotalbytes(), clientID);
                }
            }

        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("get");
        }

        return;
    }
#ifdef ENABLE_BACKUPS

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
                        setCurrentOutCode(MCMD_NOTFOUND);
                        OUTSTREAM << "Backup not found: " << local << endl;
                    }
                }
                else
                {
                    setCurrentOutCode(MCMD_NOTFOUND);
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
                setCurrentOutCode(MCMD_NOTFOUND);
                OUTSTREAM << "No backup configured. " << endl << " Usage: " << getUsageStr("backup") << endl;
            }
            mtxBackupsMap.unlock();

        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("backup");
        }
    }
#endif
    else if (words[0] == "put")
    {
        int clientID = getintOption(cloptions, "clientID", -1);

        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
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

            MegaNode *n = NULL;

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
                n = api->getNodeByHandle(cwd);
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
                        if (!newname.size()
#ifdef MEGACMDEXECUTER_FILESYSTEM
                                && !fs::exists(words[i])
#endif
                                && hasWildCards(words[i]))
                        {
                            auto paths = resolvewildcard(words[i]);
                            if (!paths.size())
                            {
                                setCurrentOutCode(MCMD_NOTFOUND);
                                LOG_err << words[i] << " not found";
                            }
                            for (auto path : paths)
                            {
                                uploadNode(path, api, n, newname, background, ignorequotawarn, clientID, megaCmdMultiTransferListener);
                            }
                        }
                        else
#endif
                        {
                            uploadNode(words[i], api, n, newname, background, ignorequotawarn, clientID, megaCmdMultiTransferListener);
                        }
                    }
                }
                else if (words.size() == 3 && !IsFolder(words[1])) //replace file
                {
                    unique_ptr<MegaNode> pn(api->getNodeByHandle(n->getParentHandle()));
                    if (pn)
                    {
#if defined(HAVE_GLOB_H) && defined(MEGACMDEXECUTER_FILESYSTEM)
                        if (!fs::exists(words[1]) && hasWildCards(words[1]))
                        {
                            LOG_err << "Invalid target for wildcard expression: " << words[1] << ". Folder expected";
                            setCurrentOutCode(MCMD_INVALIDTYPE);
                        }
                        else
#endif
                        {
                            uploadNode(words[1], api, pn.get(), n->getName(), background, ignorequotawarn, clientID, megaCmdMultiTransferListener);
                        }
                    }
                    else
                    {
                        setCurrentOutCode(MCMD_NOTFOUND);
                        LOG_err << "Destination is not valid. Parent folder cannot be found";
                    }
                }
                else
                {
                    setCurrentOutCode(MCMD_INVALIDTYPE);
                    LOG_err << "Destination is not valid (expected folder or alike)";
                }
                delete n;


                megaCmdMultiTransferListener->waitMultiEnd();

                checkNoErrors(megaCmdMultiTransferListener->getFinalerror(), "upload");

                if (megaCmdMultiTransferListener->getProgressinformed() || getCurrentOutCode() == MCMD_OK )
                {
                    informProgressUpdate(PROGRESS_COMPLETE, megaCmdMultiTransferListener->getTotalbytes(), clientID);
                }
                delete megaCmdMultiTransferListener;
            }
            else
            {
                setCurrentOutCode(MCMD_NOTFOUND);
                LOG_err << "Couln't find destination folder: " << destination << ". Use -c to create folder structure";
            }
        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("put");
        }

        return;
    }
    else if (words[0] == "log")
    {
        if (words.size() == 1)
        {
            if (!getFlag(clflags, "s") && !getFlag(clflags, "c"))
            {
                OUTSTREAM << "MEGAcmd log level = " << getLogLevelStr(loggerCMD->getCmdLoggerLevel()) << endl;
                OUTSTREAM << "SDK log level = " << getLogLevelStr(loggerCMD->getApiLoggerLevel()) << endl;
            }
            else if (getFlag(clflags, "s"))
            {
                OUTSTREAM << "SDK log level = " << getLogLevelStr(loggerCMD->getApiLoggerLevel()) << endl;
            }
            else if (getFlag(clflags, "c"))
            {
                OUTSTREAM << "MEGAcmd log level = " << getLogLevelStr(loggerCMD->getCmdLoggerLevel()) << endl;
            }
        }
        else
        {
            int newLogLevel = getLogLevelNum(words[1].c_str());
            if (newLogLevel == -1)
            {
                setCurrentOutCode(MCMD_EARGS);
                LOG_err << "Invalid log level";
                return;
            }
            newLogLevel = max(newLogLevel, (int)MegaApi::LOG_LEVEL_FATAL);
            newLogLevel = min(newLogLevel, (int)MegaApi::LOG_LEVEL_MAX);
            if (!getFlag(clflags, "s") && !getFlag(clflags, "c"))
            {
                loggerCMD->setCmdLoggerLevel(newLogLevel);
                loggerCMD->setApiLoggerLevel(newLogLevel);
                OUTSTREAM << "MEGAcmd log level = " << getLogLevelStr(loggerCMD->getCmdLoggerLevel()) << endl;
                OUTSTREAM << "SDK log level = " << getLogLevelStr(loggerCMD->getApiLoggerLevel()) << endl;
            }
            else if (getFlag(clflags, "s"))
            {
                loggerCMD->setApiLoggerLevel(newLogLevel);
                OUTSTREAM << "SDK log level = " << getLogLevelStr(loggerCMD->getApiLoggerLevel()) << endl;
            }
            else if (getFlag(clflags, "c"))
            {
                loggerCMD->setCmdLoggerLevel(newLogLevel);
                OUTSTREAM << "MEGAcmd log level = " << getLogLevelStr(loggerCMD->getCmdLoggerLevel()) << endl;
            }
        }

        return;
    }
    else if (words[0] == "pwd")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
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
            LocalPath localpath = LocalPath::fromPath(words[1], *fsAccessCMD);
            if (fsAccessCMD->chdirlocal(localpath)) // maybe this is already checked in chdir
            {
                LOG_debug << "Local folder changed to: " << words[1];
            }
            else
            {
                setCurrentOutCode(MCMD_INVALIDTYPE);
                LOG_err << "Not a valid folder: " << words[1];
            }
        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
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
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
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
                setCurrentOutCode(MCMD_EARGS);
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
                setCurrentOutCode(MCMD_NOTFOUND);
                LOG_err << "Could not find invitation " << shandle;
            }
        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
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
            setCurrentOutCode(MCMD_EARGS);
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
            setCurrentOutCode(MCMD_EARGS);
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
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("permissions");
            return;
        }

        int permvalue = -1;
        if (setperms)
        {
             if (words[1].size() != 3)
             {
                 setCurrentOutCode(MCMD_EARGS);
                 LOG_err << "Invalid permissions value: " << words[1];
             }
             else
             {
                 int owner = words[1].at(0) - '0';
                 int group = words[1].at(1) - '0';
                 int others = words[1].at(2) - '0';
                 if ( (owner < 6) || (owner == 6 && foldersflag) || (owner > 7) || (group < 0) || (group > 7) || (others < 0) || (others > 7) )
                 {
                     setCurrentOutCode(MCMD_EARGS);
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
            setCurrentOutCode(MCMD_EARGS);
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
                    OUTSTREAM << "File versions deleted succesfully. Please note that the current files were not deleted, just their history." << endl;
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
                    vector<MegaNode *> *nodesToDeleteVersions = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                    if (nodesToDeleteVersions && nodesToDeleteVersions->size())
                    {
                        for (std::vector< MegaNode * >::iterator it = nodesToDeleteVersions->begin(); it != nodesToDeleteVersions->end(); ++it)
                        {
                            MegaNode * nodeToDeleteVersions = *it;
                            if (nodeToDeleteVersions)
                            {
                                int ret = deleteNodeVersions(nodeToDeleteVersions, api, forcedelete);
                                forcedelete = forcedelete || (ret == MCMDCONFIRM_ALL);
                            }
                        }
                    }
                    else
                    {
                        setCurrentOutCode(MCMD_NOTFOUND);
                        LOG_err << "No node found: " << words[i];
                    }
                    delete nodesToDeleteVersions;
                }
                else // non-regexp
                {
                    MegaNode *n = nodebypath(words[i].c_str());
                    if (n)
                    {
                        int ret = deleteNodeVersions(n, api, forcedelete);
                        forcedelete = forcedelete || (ret == MCMDCONFIRM_ALL);
                    }
                    else
                    {
                        setCurrentOutCode(MCMD_NOTFOUND);
                        LOG_err << "Node not found: " << words[i];
                    }
                    delete n;
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
            setCurrentOutCode(MCMD_EARGS);
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
                    setCurrentOutCode(MCMD_EARGS);
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
            setCurrentOutCode(MCMD_EARGS);
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
                setCurrentOutCode(MCMD_EARGS);
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
                    setCurrentOutCode(MCMD_EARGS);
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

        // automatic now:
        //api->enableTransferResumption();

        if (getFlag(clflags, "a"))
        {
            for (unsigned int i=1;i<words.size(); i++)
            {
                ConfigurationManager::addExcludedName(words[i]);
            }
            if (words.size()>1)
            {
                std::vector<string> vexcludednames(ConfigurationManager::excludedNames.begin(), ConfigurationManager::excludedNames.end());
                api->setExcludedNames(&vexcludednames);
                if (getFlag(clflags, "restart-syncs"))
                {
                    restartsyncs();
                }
            }
            else
            {
                setCurrentOutCode(MCMD_EARGS);
                LOG_err << "      " << getUsageStr("exclude");
                return;
            }
        }
        else if (getFlag(clflags, "d"))
        {
            for (unsigned int i=1;i<words.size(); i++)
            {
                ConfigurationManager::removeExcludedName(words[i]);
            }
            if (words.size()>1)
            {
                std::vector<string> vexcludednames(ConfigurationManager::excludedNames.begin(), ConfigurationManager::excludedNames.end());
                api->setExcludedNames(&vexcludednames);
                if (getFlag(clflags, "restart-syncs"))
                {
                    restartsyncs();
                }
            }
            else
            {
                setCurrentOutCode(MCMD_EARGS);
                LOG_err << "      " << getUsageStr("exclude");
                return;
            }
        }


        OUTSTREAM << "List of excluded names:" << endl;

        for (set<string>::iterator it=ConfigurationManager::excludedNames.begin(); it!=ConfigurationManager::excludedNames.end(); ++it)
        {
            OUTSTREAM << *it << endl;
        }

        if ( !getFlag(clflags, "restart-syncs") && (getFlag(clflags, "a") || getFlag(clflags, "d")) )
        {
            OUTSTREAM << endl <<  "Changes will not be applied immediately to operations being performed in active syncs."
                      << " See \"exclude --help\" for further info" << endl;
        }
    }
    else if (words[0] == "sync")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (!api->isLoggedIn())
        {
            LOG_err << "Not logged in";
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            return;
        }

        int PATHSIZE = getintOption(cloptions,"path-display-size");
        if (!PATHSIZE)
        {
            // get screen size for output purposes
            unsigned int width = getNumberOfCols(75);
            PATHSIZE = min(60,int((width-46)/2));
        }
        PATHSIZE = max(0, PATHSIZE);

        bool headershown = false;
        if (words.size() == 3) //add a sync
        {
            LocalPath localRelativePath = LocalPath::fromPath(words[1], *fsAccessCMD);
            LocalPath localAbsolutePath = localRelativePath;
            fsAccessCMD->expanselocalpath(localRelativePath, localAbsolutePath);

            MegaNode* n = nodebypath(words[2].c_str());
            if (n)
            {
                if (n->getType() == MegaNode::TYPE_FILE)
                {
                    LOG_err << words[2] << ": Remote sync root must be folder.";
                }
                else if (api->getAccess(n) >= MegaShare::ACCESS_FULL)
                {
                    MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                    api->syncFolder(MegaSync::TYPE_TWOWAY, localAbsolutePath.toPath(*fsAccessCMD).c_str(), nullptr, n->getHandle(), nullptr, megaCmdListener);
                    megaCmdListener->wait();
                    if (checkNoErrors(megaCmdListener->getError(), "sync folder", static_cast<SyncError>(megaCmdListener->getRequest()->getNumDetails())))
                    {
                        string localpath(megaCmdListener->getRequest()->getFile());
                        SyncError syncError = static_cast<SyncError>(megaCmdListener->getRequest()->getNumDetails());

                        char * nodepath = api->getNodePath(n);
                        LOG_info << "Added sync: " << localpath << " to " << nodepath;
                        if (syncError != NO_SYNC_ERROR)
                        {
                            LOG_err << "Sync added as temporarily disabled. Reason: " << MegaSync::getMegaSyncErrorCode(syncError);
                        }

                        delete []nodepath;
                    }

                    delete megaCmdListener;
                }
                else
                {
                    setCurrentOutCode(MCMD_NOTPERMITTED);
                    LOG_err << words[2] << ": Syncing requires full access to path, current access: " << api->getAccess(n);
                }
                delete n;
            }
            else
            {
                setCurrentOutCode(MCMD_NOTFOUND);
                LOG_err << "Couldn't find remote folder: " << words[2];
            }
        }
        else if (words.size() == 2) //manage one sync
        {
            string pathOrId{words[1].c_str()};
            auto stop = getFlag(clflags, "s") || getFlag(clflags, "disable");
            auto resume = getFlag(clflags, "r") || getFlag(clflags, "enable");
            auto remove = getFlag(clflags, "d") || getFlag(clflags, "remove");

            //1 - find the sync
            std::unique_ptr<MegaSync> sync;
            sync.reset(api->getSyncByBackupId(base64ToSyncBackupId(pathOrId)));
            if (!sync)
            {
                sync.reset(api->getSyncByPath(pathOrId.c_str()));
            }

            if (!sync)
            {
                setCurrentOutCode(MCMD_NOTFOUND);
                LOG_err << "Sync not found: " << pathOrId;
                return;
            }

            if (stop)
            {
                auto megaCmdListener = ::mega::make_unique<MegaCmdListener>(nullptr);
                api->disableSync(sync.get(), megaCmdListener.get());
                megaCmdListener->wait();
                if (checkNoErrors(megaCmdListener->getError(), "stop sync", static_cast<SyncError>(megaCmdListener->getRequest()->getNumDetails())))
                {
                    LOG_info << "Sync disabled (stopped): " << sync->getLocalFolder() << " to " << sync->getLastKnownMegaFolder();
                }
            }
            else if (resume)
            {
                auto megaCmdListener = ::mega::make_unique<MegaCmdListener>(nullptr);
                api->enableSync(sync.get(), megaCmdListener.get());
                megaCmdListener->wait();
                if (checkNoErrors(megaCmdListener->getError(), "enable sync", static_cast<SyncError>(megaCmdListener->getRequest()->getNumDetails())))
                {
                    LOG_info << "Sync re-enabled: " << sync->getLocalFolder() << " to " << sync->getLastKnownMegaFolder();
                }
            }
            else if (remove)
            {
                auto megaCmdListener = ::mega::make_unique<MegaCmdListener>(nullptr);
                api->removeSync(sync.get(), megaCmdListener.get());
                megaCmdListener->wait();
                if (checkNoErrors(megaCmdListener->getError(), "remove sync", static_cast<SyncError>(megaCmdListener->getRequest()->getNumDetails())))
                {
                    OUTSTREAM << "Sync removed: " << sync->getLocalFolder() << " to " << sync->getLastKnownMegaFolder() << endl;
                    return;
                }
            }

            // In any case, print the udpated sync state:
            sync.reset(api->getSyncByBackupId(sync->getBackupId())); //first retrieve it again, to get updated data!
            if (!sync)
            {
                setCurrentOutCode(MCMD_NOTFOUND);
                LOG_err << "Sync not found: " << pathOrId;
                return;
            }

            ColumnDisplayer cd(clflags, cloptions);

            long long nfiles = 0;
            long long nfolders = 0;
            nfolders++; //add the sync itself
            std::unique_ptr<MegaNode> node{api->getNodeByHandle(sync->getMegaHandle())};
            getInfoFromFolder(node.get(), api, &nfiles, &nfolders);

            if (!headershown)
            {
                headershown = true;
                printSyncHeader(cd);
            }

            printSync(sync.get(), nfiles, nfolders, cd, clflags, cloptions);

           OUTSTRINGSTREAM oss;
           cd.print(oss);
           OUTSTREAM << oss.str();

        }
        else if (words.size() == 1) //print all syncs
        {
            ColumnDisplayer cd(clflags, cloptions);

            std::unique_ptr<MegaSyncList> syncs{api->getSyncs()};
            for (int i = 0; i < syncs->size(); i++)
            {
                MegaSync *sync = syncs->get(i);

                long long nfiles = 0;
                long long nfolders = 0;
                nfolders++; //add the sync itself
                std::unique_ptr<MegaNode> node{api->getNodeByHandle(sync->getMegaHandle())};
                if (node)
                {
                    getInfoFromFolder(node.get(), api, &nfiles, &nfolders);
                }
                else
                {
                    LOG_warn << "Remote node not found";
                }

                if (!headershown)
                {
                    headershown = true;
                    printSyncHeader(cd);
                }

                printSync(sync, nfiles, nfolders, cd, clflags, cloptions);
            }
            OUTSTRINGSTREAM oss;
            cd.print(oss);
            OUTSTREAM << oss.str();
        }
        return;
    }
#endif
    else if (words[0] == "cancel")
    {
        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
        api->cancelAccount(megaCmdListener);
        megaCmdListener->wait();
        if (checkNoErrors(megaCmdListener->getError(), "cancel account"))
        {
            OUTSTREAM << "Account pendind cancel confirmation. You will receive a confirmation link. Use \"confirmcancel\" with the provided link to confirm the cancelation" << endl;
        }
        delete megaCmdListener;
    }
    else if (words[0] == "confirmcancel")
    {
        if (words.size() < 2)
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("confirmcancel");
            return;
        }

        const char * confirmlink = words[1].c_str();
        if (words.size() > 2)
        {
            const char * pass = words[2].c_str();
            confirmCancel(confirmlink, pass);
        }
        else if (interactiveThread())
        {
            link = confirmlink;
            confirmingcancel = true;
            setprompt(LOGINPASSWORD);
        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "Extra args required in non-interactive mode. Usage: " << getUsageStr("confirmcancel");
        }
    }
    else if (words[0] == "login")
    {
        LoginGuard loginGuard;
        int clientID = getintOption(cloptions, "clientID", -1);

        if (!api->isLoggedIn())
        {
            if (words.size() > 1)
            {
                if (strchr(words[1].c_str(), '@'))
                {
                    // full account login
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
                        if (interactiveThread())
                        {
                            setprompt(LOGINPASSWORD);
                        }
                        else
                        {
                            setCurrentOutCode(MCMD_EARGS);
                            LOG_err << "Extra args required in non-interactive mode. Usage: " << getUsageStr("login");
                        }
                    }
                }
                else
                {
                    const char* ptr;
                    if (( ptr = strchr(words[1].c_str(), '#')))  // folder link indicator
                    {
                        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                        sandboxCMD->resetSandBox();

                        string authKey = getOption(cloptions, "auth-key", "");
                        if (authKey.empty())
                        {
                            api->loginToFolder(words[1].c_str(), megaCmdListener);
                        }
                        else
                        {
                            api->loginToFolder(words[1].c_str(), authKey.c_str(), megaCmdListener);
                        }

                        actUponLogin(megaCmdListener);
                        delete megaCmdListener;
                        return;
                    }
                    else
                    {
                        LOG_info << "Resuming session...";
                        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                        sandboxCMD->resetSandBox();
                        api->fastLogin(words[1].c_str(), megaCmdListener);
                        actUponLogin(megaCmdListener);
                        delete megaCmdListener;
                        return;
                    }
                }
            }
            else
            {
                setCurrentOutCode(MCMD_EARGS);
                LOG_err << "      " << getUsageStr("login");
            }
        }
        else
        {
            setCurrentOutCode(MCMD_INVALIDSTATE);
            LOG_err << "Already logged in. Please log out first.";
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
            setCurrentOutCode(MCMD_NOTFOUND);
        }
    }
    else if (words[0] == "mount")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        listtrees();
        return;
    }
    else if (words[0] == "share")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        string with = getOption(cloptions, "with", "");
        if (getFlag(clflags, "a") && ( "" == with ))
        {
            setCurrentOutCode(MCMD_EARGS);
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
            setCurrentOutCode(MCMD_EARGS);
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
                vector<MegaNode *> *nodes = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                if (nodes)
                {
                    if (!nodes->size())
                    {
                        setCurrentOutCode(MCMD_NOTFOUND);
                        if (words[i].find("@") != string::npos)
                        {
                            LOG_err << "Could not find " << words[i] << ". Use --with=" << words[i] << " to specify the user to share with";
                        }
                        else
                        {
                            LOG_err << "Node not found: " << words[i];
                        }
                    }
                    for (std::vector< MegaNode * >::iterator it = nodes->begin(); it != nodes->end(); ++it)
                    {
                        MegaNode * n = *it;
                        if (n)
                        {
                            if (getFlag(clflags, "a"))
                            {
                                LOG_debug << " sharing ... " << n->getName() << " with " << with;
                                if (level == level_NOT_present_value)
                                {
                                    level = MegaShare::ACCESS_READ;
                                }

                                if (n->getType() == MegaNode::TYPE_FILE)
                                {
                                    setCurrentOutCode(MCMD_INVALIDTYPE);
                                    LOG_err << "Cannot share file: " << n->getName() << ". Only folders allowed. You can send file to user's inbox with cp (see \"cp --help\")";
                                }
                                else
                                {
                                    shareNode(n, with, level);
                                }
                            }
                            else if (getFlag(clflags, "d"))
                            {
                                if ("" != with)
                                {
                                    LOG_debug << " deleting share ... " << n->getName() << " with " << with;
                                    disableShare(n, with);
                                }
                                else
                                {
                                    MegaShareList* outShares = api->getOutShares(n);
                                    if (outShares)
                                    {
                                        for (int i = 0; i < outShares->size(); i++)
                                        {
                                            if (outShares->get(i)->getNodeHandle() == n->getHandle())
                                            {
                                                LOG_debug << " deleting share ... " << n->getName() << " with " << outShares->get(i)->getUser();
                                                disableShare(n, outShares->get(i)->getUser());
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
                                    setCurrentOutCode(MCMD_EARGS);
                                    LOG_err << "Unexpected option received. To create/modify a share use -a";
                                }
                                else if (listPending)
                                {
                                    dumpListOfPendingShares(n, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, words[i]);
                                }
                                else
                                {
                                    dumpListOfShared(n, words[i]);
                                }
                            }
                            delete n;
                        }
                    }

                    nodes->clear();
                    delete nodes;
                }
                else
                {
                    setCurrentOutCode(MCMD_NOTFOUND);
                    LOG_err << "Node not found: " << words[i];
                }
            }
            else // non-regexp
            {
                MegaNode *n = nodebypath(words[i].c_str());
                if (n)
                {
                    if (getFlag(clflags, "a"))
                    {
                        LOG_debug << " sharing ... " << n->getName() << " with " << with;
                        if (level == level_NOT_present_value)
                        {
                            level = MegaShare::ACCESS_READ;
                        }
                        shareNode(n, with, level);
                    }
                    else if (getFlag(clflags, "d"))
                    {
                        if ("" != with)
                        {
                            LOG_debug << " deleting share ... " << n->getName() << " with " << with;
                            disableShare(n, with);
                        }
                        else
                        {
                            MegaShareList* outShares = api->getOutShares(n);
                            if (outShares)
                            {
                                for (int i = 0; i < outShares->size(); i++)
                                {
                                    if (outShares->get(i)->getNodeHandle() == n->getHandle())
                                    {
                                        LOG_debug << " deleting share ... " << n->getName() << " with " << outShares->get(i)->getUser();
                                        disableShare(n, outShares->get(i)->getUser());
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
                            setCurrentOutCode(MCMD_EARGS);
                            LOG_err << "Unexpected option received. To create/modify a share use -a";
                        }
                        else if (listPending)
                        {
                            dumpListOfPendingShares(n, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, words[i]);
                        }
                        else
                        {
                            dumpListOfShared(n, words[i]);
                        }
                    }
                    delete n;
                }
                else
                {
                    setCurrentOutCode(MCMD_NOTFOUND);
                    LOG_err << "Node not found: " << words[i];
                }
            }
        }

        return;
    }
    else if (words[0] == "users")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (getFlag(clflags, "d") && ( words.size() <= 1 ))
        {
            setCurrentOutCode(MCMD_EARGS);
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
                        OUTSTREAM << "Contact " << words[1] << " removed succesfully" << endl;
                    }
                    delete megaCmdListener;
                }
                else
                {
                    if (!(( user->getVisibility() != MegaUser::VISIBILITY_VISIBLE ) && !getFlag(clflags, "h")))
                    {
                        if (getFlag(clflags,"n"))
                        {
                            string name;
                            MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                            api->getUserAttribute(user, ATTR_FIRSTNAME, megaCmdListener);
                            megaCmdListener->wait();
                            if (megaCmdListener->getError()->getErrorCode() == MegaError::API_OK)
                            {
                                if (megaCmdListener->getRequest()->getText() && strlen(megaCmdListener->getRequest()->getText()))
                                {
                                    name += megaCmdListener->getRequest()->getText();
                                }
                            }
                            delete megaCmdListener;

                            megaCmdListener = new MegaCmdListener(NULL);
                            api->getUserAttribute(user, ATTR_LASTNAME, megaCmdListener);
                            megaCmdListener->wait();
                            if (megaCmdListener->getError()->getErrorCode() == MegaError::API_OK)
                            {
                                if (megaCmdListener->getRequest()->getText() && strlen(megaCmdListener->getRequest()->getText()))
                                {
                                    if (name.size())
                                    {
                                        name+=" ";
                                    }
                                    name+=megaCmdListener->getRequest()->getText();
                                }
                            }
                            if (name.size())
                            {
                                OUTSTREAM << name << ": ";
                            }

                            delete megaCmdListener;
                        }


                        OUTSTREAM << user->getEmail() << ", " << visibilityToString(user->getVisibility());
                        if (user->getTimestamp())
                        {
                            OUTSTREAM << " since " << getReadableTime(user->getTimestamp(), getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")));
                        }
                        OUTSTREAM << endl;

                        if (getFlag(clflags, "s"))
                        {
                            MegaShareList *shares = api->getOutShares();
                            if (shares)
                            {
                                bool first_share = true;
                                for (int j = 0; j < shares->size(); j++)
                                {
                                    if (!strcmp(shares->get(j)->getUser(), user->getEmail()))
                                    {
                                        MegaNode * n = api->getNodeByHandle(shares->get(j)->getNodeHandle());
                                        if (n)
                                        {
                                            if (first_share)
                                            {
                                                OUTSTREAM << "\tSharing:" << endl;
                                                first_share = false;
                                            }

                                            OUTSTREAM << "\t";
                                            dumpNode(n, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, 2, false, 0, getDisplayPath("/", n).c_str());
                                            delete n;
                                        }
                                    }
                                }

                                delete shares;
                            }
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
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
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
                setCurrentOutCode(MCMD_INVALIDSTATE);
                LOG_err << "Folder navigation failed";
                return;
            }

        }

        setCurrentOutCode(globalstatus);
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
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 1)
        {
            int cancel = getFlag(clflags, "d");
            bool settingattr = getFlag(clflags, "s");

            string nodePath = words.size() > 1 ? words[1] : "";
            string attribute = words.size() > 2 ? words[2] : "";
            string attrValue = words.size() > 3 ? words[3] : "";
            n = nodebypath(nodePath.c_str());

            if (n)
            {
                if (settingattr || cancel)
                {
                    if (attribute.size())
                    {
                        const char *cattrValue = cancel ? NULL : attrValue.c_str();
                        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                        api->setCustomNodeAttribute(n, attribute.c_str(), cattrValue, megaCmdListener);
                        megaCmdListener->wait();
                        if (checkNoErrors(megaCmdListener->getError(), "set node attribute: " + attribute))
                        {
                            OUTSTREAM << "Node attribute " << attribute << ( cancel ? " removed" : " updated" ) << " correctly" << endl;
                            delete n;
                            n = api->getNodeByHandle(megaCmdListener->getRequest()->getNodeHandle());
                        }
                        delete megaCmdListener;
                    }
                    else
                    {
                        setCurrentOutCode(MCMD_EARGS);
                        LOG_err << "Attribute not specified";
                        LOG_err << "      " << getUsageStr("attr");
                        return;
                    }
                }

                //List node custom attributes
                MegaStringList *attrlist = n->getCustomAttrNames();
                if (attrlist)
                {
                    if (!attribute.size())
                    {
                        OUTSTREAM << "The node has " << attrlist->size() << " attributes" << endl;
                    }
                    for (int a = 0; a < attrlist->size(); a++)
                    {
                        string iattr = attrlist->get(a);
                        if (!attribute.size() || ( attribute == iattr ))
                        {
                            const char* iattrval = n->getCustomAttr(iattr.c_str());
                            OUTSTREAM << "\t" << iattr << " = " << ( iattrval ? iattrval : "NULL" ) << endl;
                        }
                    }

                    delete attrlist;
                }

                delete n;
            }
            else
            {
                setCurrentOutCode(MCMD_NOTFOUND);
                LOG_err << "Couldn't find node: " << nodePath;
                return;
            }
        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("attr");
            return;
        }


        return;
    }
    else if (words[0] == "userattr")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
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
                setCurrentOutCode(MCMD_EARGS);
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
                    setCurrentOutCode(MCMD_NOTFOUND);
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
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 1)
        {
            string nodepath = words[1];
            string path = words.size() > 2 ? words[2] : "./";
            n = nodebypath(nodepath.c_str());
            if (n)
            {
                MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                bool setting = getFlag(clflags, "s");
                if (setting)
                {
                    api->setThumbnail(n, path.c_str(), megaCmdListener);
                }
                else
                {
                    api->getThumbnail(n, path.c_str(), megaCmdListener);
                }
                megaCmdListener->wait();
                if (checkNoErrors(megaCmdListener->getError(), ( setting ? "set thumbnail " : "get thumbnail " ) + nodepath + " to " + path))
                {
                    OUTSTREAM << "Thumbnail for " << nodepath << ( setting ? " loaded from " : " saved in " ) << megaCmdListener->getRequest()->getFile() << endl;
                }
                delete megaCmdListener;
                delete n;
            }
        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("attr");
            return;
        }
        return;
    }
    else if (words[0] == "preview")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 1)
        {
            string nodepath = words[1];
            string path = words.size() > 2 ? words[2] : "./";
            n = nodebypath(nodepath.c_str());
            if (n)
            {
                MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                bool setting = getFlag(clflags, "s");
                if (setting)
                {
                    api->setPreview(n, path.c_str(), megaCmdListener);
                }
                else
                {
                    api->getPreview(n, path.c_str(), megaCmdListener);
                }
                megaCmdListener->wait();
                if (checkNoErrors(megaCmdListener->getError(), ( setting ? "set preview " : "get preview " ) + nodepath + " to " + path))
                {
                    OUTSTREAM << "Preview for " << nodepath << ( setting ? " loaded from " : " saved in " ) << megaCmdListener->getRequest()->getFile() << endl;
                }
                delete megaCmdListener;
                delete n;
            }
        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("attr");
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
                if (interactiveThread())
                {
                    setprompt(NEWPASSWORD);
                }
                else
                {
                    setCurrentOutCode(MCMD_EARGS);
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
                setCurrentOutCode(MCMD_EARGS);
                LOG_err << "      " << getUsageStr("passwd");
            }
        }
        else
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
        }

        return;
    }
    else if (words[0] == "speedlimit")
    {
        if (words.size() > 2)
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("speedlimit");
            return;
        }
        if (words.size() > 1)
        {
            long long maxspeed = textToSize(words[1].c_str());
            if (maxspeed == -1)
            {
                string s = words[1] + "B";
                maxspeed = textToSize(s.c_str());
            }
            if (!getFlag(clflags, "u") && !getFlag(clflags, "d"))
            {
                api->setMaxDownloadSpeed(maxspeed);
                api->setMaxUploadSpeed(maxspeed);
                ConfigurationManager::savePropertyValue("maxspeedupload", maxspeed);
                ConfigurationManager::savePropertyValue("maxspeeddownload", maxspeed);
            }
            else if (getFlag(clflags, "u"))
            {
                api->setMaxUploadSpeed(maxspeed);
                ConfigurationManager::savePropertyValue("maxspeedupload", maxspeed);
            }
            else if (getFlag(clflags, "d"))
            {
                api->setMaxDownloadSpeed(maxspeed);
                ConfigurationManager::savePropertyValue("maxspeeddownload", maxspeed);
            }
        }

        bool hr = getFlag(clflags,"h");

        if (!getFlag(clflags, "u") && !getFlag(clflags, "d"))
        {
            long long us = api->getMaxUploadSpeed();
            long long ds = api->getMaxDownloadSpeed();
            OUTSTREAM << "Upload speed limit = " << (us?sizeToText(us,false,hr):"unlimited") << ((us && hr)?"/s":(us?" B/s":""))  << endl;
            OUTSTREAM << "Download speed limit = " << (ds?sizeToText(ds,false,hr):"unlimited") << ((ds && hr)?"/s":(us?" B/s":"")) << endl;
        }
        else if (getFlag(clflags, "u"))
        {
            long long us = api->getMaxUploadSpeed();
            OUTSTREAM << "Upload speed limit = " << (us?sizeToText(us,false,hr):"unlimited") << ((us && hr)?"/s":(us?" B/s":"")) << endl;
        }
        else if (getFlag(clflags, "d"))
        {
            long long ds = api->getMaxDownloadSpeed();
            OUTSTREAM << "Download speed limit = " << (ds?sizeToText(ds,false,hr):"unlimited") << ((ds && hr)?"/s":(ds?" B/s":"")) << endl;
        }

        return;
    }
    else if (words[0] == "invite")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        if (words.size() > 1)
        {
            string email = words[1];
            if (!isValidEmail(email))
            {
                setCurrentOutCode(MCMD_INVALIDEMAIL);
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
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("errorcode");
            return;
        }

        int errCode = -1999;
        istringstream is(words[1]);
        is >> errCode;
        if (errCode == -1999)
        {
            setCurrentOutCode(MCMD_EARGS);
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
            setCurrentOutCode(MCMD_INVALIDSTATE);
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
                if (interactiveThread())
                {
                    setprompt(NEWPASSWORD);
                }
                else
                {
                    setCurrentOutCode(MCMD_EARGS);
                    LOG_err << "Extra args required in non-interactive mode. Usage: " << getUsageStr("signup");
                }
            }
        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
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
                MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                api->getExtendedAccountDetails(true, true, true, megaCmdListener);
                actUponGetExtendedAccountDetails(megaCmdListener);
                delete megaCmdListener;
            }
            delete u;
        }
        else
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
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
                    unique_ptr<MegaNode> inbox(api->getInboxNode());
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
                            n = inShares->get(i);
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
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
        }

        return;
    }
    else if (words[0] == "export")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
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
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "Invalid time " << sexpireTime;
            return;
        }

        string linkPass = getOption(cloptions, "password", "");
        bool add = getFlag(clflags, "a");

        if (linkPass.size() && !add)
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "You need to use -a to add an export. Usage: " << getUsageStr("export");
            return;
        }

        if (words.size() <= 1)
        {
            words.push_back(string(".")); //cwd
        }

        for (int i = 1; i < (int)words.size(); i++)
        {
            unescapeifRequired(words[i]);
            if (isRegExp(words[i]))
            {
                vector<MegaNode *> *nodes = nodesbypath(words[i].c_str(), getFlag(clflags,"use-pcre"));
                if (nodes)
                {
                    if (!nodes->size())
                    {
                        setCurrentOutCode(MCMD_NOTFOUND);
                        LOG_err << "Nodes not found: " << words[i];
                    }
                    for (std::vector< MegaNode * >::iterator it = nodes->begin(); it != nodes->end(); ++it)
                    {
                        MegaNode * n = *it;
                        if (n)
                        {
                            if (add)
                            {
                                LOG_debug << " exporting ... " << n->getName() << " expireTime=" << expireTime;
                                exportNode(n, expireTime, linkPass, getFlag(clflags,"f"), getFlag(clflags,"writable"));
                            }
                            else if (getFlag(clflags, "d"))
                            {
                                LOG_debug << " deleting export ... " << n->getName();
                                disableExport(n);
                            }
                            else
                            {
                                if (dumpListOfExported(n, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, words[i]) == 0 )
                                {
                                    OUTSTREAM << words[i] << " is not exported. Use -a to export it" << endl;
                                }
                            }
                            delete n;
                        }
                    }

                    nodes->clear();
                    delete nodes;
                }
                else
                {
                    setCurrentOutCode(MCMD_NOTFOUND);
                    LOG_err << "Node not found: " << words[i];
                }
            }
            else
            {
                MegaNode *n = nodebypath(words[i].c_str());
                if (n)
                {
                    if (add)
                    {
                        LOG_debug << " exporting ... " << n->getName();
                        exportNode(n, expireTime, linkPass, getFlag(clflags,"f"), getFlag(clflags,"writable"));
                    }
                    else if (getFlag(clflags, "d"))
                    {
                        LOG_debug << " deleting export ... " << n->getName();
                        disableExport(n);
                    }
                    else
                    {
                        if (dumpListOfExported(n, getTimeFormatFromSTR(getOption(cloptions, "time-format","RFC2822")), clflags, cloptions, words[i]) == 0 )
                        {
                            OUTSTREAM << "Couldn't find anything exported below ";
                            if (words[i] == ".")
                            {
                                OUTSTREAM << "current folder";
                            }
                            else
                            {
                                OUTSTREAM << "<";
                                OUTSTREAM << words[i];
                                OUTSTREAM << ">";
                            }
                            OUTSTREAM << ". Use -a to export " << (words[i].size()?"it":"something") << endl;
                        }
                    }
                    delete n;
                }
                else
                {
                    setCurrentOutCode(MCMD_NOTFOUND);
                    LOG_err << "Node not found: " << words[i];
                }
            }
        }

        return;
    }
    else if (words[0] == "import")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
            LOG_err << "Not logged in.";
            return;
        }
        string remotePath = "";
        MegaNode *dstFolder = NULL;
        if (words.size() > 1) //link
        {
            if (isPublicLink(words[1]))
            {
                string publicLink = words[1];
                if (isEncryptedLink(publicLink))
                {
                    string linkPass = getOption(cloptions, "password", "");
                    if (!linkPass.size())
                    {
                        linkPass = askforUserResponse("Enter password: ");
                    }

                    if (linkPass.size())
                    {
                        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                        api->decryptPasswordProtectedLink(publicLink.c_str(), linkPass.c_str(), megaCmdListener);
                        megaCmdListener->wait();
                        if (checkNoErrors(megaCmdListener->getError(), "decrypt password protected link"))
                        {
                            publicLink = megaCmdListener->getRequest()->getText();
                            delete megaCmdListener;
                        }
                        else
                        {
                            setCurrentOutCode(MCMD_NOTPERMITTED);
                            LOG_err << "Invalid password";
                            delete megaCmdListener;
                            return;
                        }
                    }
                    else
                    {
                        setCurrentOutCode(MCMD_EARGS);
                        LOG_err << "Need a password to decrypt provided link (--password=PASSWORD)";
                        return;
                    }
                }

                if (words.size() > 2)
                {
                    remotePath = words[2];
                    dstFolder = nodebypath(remotePath.c_str());
                }
                else
                {
                    dstFolder = api->getNodeByHandle(cwd);
                    remotePath = "."; //just to inform (alt: getpathbynode)
                }
                if (dstFolder && ( !dstFolder->getType() == MegaNode::TYPE_FILE ))
                {
                    if (getLinkType(publicLink) == MegaNode::TYPE_FILE)
                    {
                        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);

                        api->importFileLink(publicLink.c_str(), dstFolder, megaCmdListener);
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
                                        api->copyNode(authorizedNode, dstFolder, megaCmdListener3);
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
                                        setCurrentOutCode(MCMD_EUNEXPECTED);
                                        LOG_debug << "Node couldn't be authorized: " << publicLink;
                                    }
                                    delete nodeToImport;
                                }
                                else
                                {
                                    setCurrentOutCode(MCMD_INVALIDSTATE);
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
                        }
                        delete megaCmdListener;
                        freeApiFolder(apiFolder);
                    }
                    else
                    {
                        setCurrentOutCode(MCMD_EARGS);
                        LOG_err << "Invalid link: " << publicLink;
                        LOG_err << "      " << getUsageStr("import");
                    }
                }
                else
                {
                    setCurrentOutCode(MCMD_INVALIDTYPE);
                    LOG_err << "Invalid destiny: " << remotePath;
                }
                delete dstFolder;
            }
            else
            {
                setCurrentOutCode(MCMD_INVALIDTYPE);
                LOG_err << "Invalid link: " << words[1];
            }
        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr("import");
        }

        return;
    }
    else if (words[0] == "reload")
    {
        int clientID = getintOption(cloptions, "clientID", -1);

        OUTSTREAM << "Reloading account..." << endl;
        MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL, NULL, clientID);
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
                    OUTSTREAM << "Account " << email << " confirmed succesfully. You can login with it now" << endl;
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
                        if (interactiveThread() && !getCurrentThreadIsCmdShell())
                        {
                            setprompt(LOGINPASSWORD);
                        }
                        else
                        {
                            setCurrentOutCode(MCMD_EARGS);
                            LOG_err << "Extra args required in non-interactive mode. Usage: " << getUsageStr("confirm");
                        }
                    }
                }
                else
                {
                    setCurrentOutCode(MCMD_INVALIDEMAIL);
                    LOG_err << email << " doesn't correspond to the confirmation link: " << link;
                }
            }

            delete megaCmdListener;
        }
        else
        {
            setCurrentOutCode(MCMD_EARGS);
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
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
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
            OUTSTREAM << "MEGA SDK version: " << MEGA_MAJOR_VERSION << "." << MEGA_MINOR_VERSION << "." << MEGA_MICRO_VERSION << endl;

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
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
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
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
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
                    MegaContactRequest * cr = ocrl->get(i);
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
                    MegaContactRequest * cr = icrl->get(i);
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
            setCurrentOutCode(MCMD_EARGS);
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
#ifdef HAVE_DOWNLOADS_COMMAND
    else if (words[0] == "downloads")
    {
        if (getFlag(clflags, "purge"))
        {
            OUTSTREAM << "Cancelling and cleaning all transfers and history ..." << endl;
            cleanSlateTranfers();
            OUTSTREAM << "... done" << endl;
            return;
        }
        if (getFlag(clflags, "enable-clean-slate"))
        {
            bool downloads_cleanslate_enabled_before = ConfigurationManager::getConfigurationValue("downloads_cleanslate_enabled", false);

            if (!downloads_cleanslate_enabled_before)
            {
                ConfigurationManager::savePropertyValue("downloads_cleanslate_enabled", true);
            }

            OUTSTREAM << "Enabled clean slate: transfers from previous executions will be discarded upon restart" << endl;
            return;
        }
        else if (getFlag(clflags, "disable-clean-slate"))
        {
            bool downloads_cleanslate_enabled_before = ConfigurationManager::getConfigurationValue("downloads_cleanslate_enabled", false);

            if (downloads_cleanslate_enabled_before)
            {
                ConfigurationManager::savePropertyValue("downloads_cleanslate_enabled", false);
            }
            OUTSTREAM << "Disabled clean slate: transfers from previous executions will not be discarded upon restart" << endl;
            return;
        }


        bool queryEnabled = getFlag(clflags, "query-enabled");
        if (getFlag(clflags, "enable-tracking"))
        {
            bool downloads_tracking_enabled_before = ConfigurationManager::getConfigurationValue("downloads_tracking_enabled", false);

            if (!downloads_tracking_enabled_before)
            {
                ConfigurationManager::savePropertyValue("downloads_tracking_enabled", true);
                DownloadsManager::Instance().start();
            }

            queryEnabled = true;
        }
        else if (getFlag(clflags, "disable-tracking"))
        {
            bool downloads_tracking_enabled_before = ConfigurationManager::getConfigurationValue("downloads_tracking_enabled", false);

            if (downloads_tracking_enabled_before)
            {
                ConfigurationManager::savePropertyValue("downloads_tracking_enabled", false);
                DownloadsManager::Instance().shutdown(true);
            }

            queryEnabled = true;
        }

        if (queryEnabled)
        {
            bool downloads_tracking_enabled = ConfigurationManager::getConfigurationValue("downloads_tracking_enabled", false);
            OUTSTREAM << "Download tracking is " << (downloads_tracking_enabled ? "enabled" : "disabled") << endl;
            return;
        }

        if (getFlag(clflags, "report-all"))
        {
            OUTSTRINGSTREAM oss;
            DownloadsManager::Instance().printAll(oss, clflags, cloptions);
            OUTSTREAM << oss.str();
            return;
        }

        /// report:
        if (words.size() < 2)
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "      " << getUsageStr(words[0].c_str());
            return;
        }

        for (unsigned i = 1 ; i < words.size(); i++)
        {
            if (!words[i].empty())
            {
                OUTSTRINGSTREAM oss;
                DownloadsManager::Instance().printOne(oss, words[i], clflags, cloptions);
                OUTSTREAM << oss.str();
            }
        }
    }
#endif
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
                    setCurrentOutCode(MCMD_EARGS);
                    LOG_err << "      " << getUsageStr("transfers");
                    return;
                }
                for (unsigned int i = 1; i < words.size(); i++)
                {
                    MegaTransfer *transfer = api->getTransferByTag(toInteger(words[i],-1));
                    if (transfer)
                    {
                        if (transfer->isSyncTransfer())
                        {
                            LOG_err << "Unable to cancel transfer with tag " << words[i] << ". Sync transfers cannot be cancelled";
                            setCurrentOutCode(MCMD_INVALIDTYPE);
                        }
                        else
                        {
                            MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                            api->cancelTransfer(transfer, megaCmdListener);
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
                        LOG_err << "Coul not find transfer with tag: " << words[i];
                        setCurrentOutCode(MCMD_NOTFOUND);
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
                    setCurrentOutCode(MCMD_EARGS);
                    LOG_err << "      " << getUsageStr("transfers");
                    return;
                }
                for (unsigned int i = 1; i < words.size(); i++)
                {
                    MegaTransfer *transfer = api->getTransferByTag(toInteger(words[i],-1));
                    if (transfer)
                    {
                        if (transfer->isSyncTransfer())
                        {
                            LOG_err << "Unable to "<< (getFlag(clflags,"p")?"pause":"resume") << " transfer with tag " << words[i] << ". Sync transfers cannot be "<< (getFlag(clflags,"p")?"pause":"resume") << "d";
                            setCurrentOutCode(MCMD_INVALIDTYPE);
                        }
                        else
                        {
                            MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                            api->pauseTransfer(transfer, getFlag(clflags,"p"), megaCmdListener);
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
                        LOG_err << "Coul not find transfer with tag: " << words[i];
                        setCurrentOutCode(MCMD_NOTFOUND);
                    }
                }
            }

            return;
        }

        //show transfers
        MegaTransferData* transferdata = api->getTransferData();

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
            delete transferdata;
            return;
        }



        int limit = getintOption(cloptions, "limit", min(10,ndownloads+nuploads+(int)globalTransferListener->completedTransfers.size()));

        if (!transferdata)
        {
            setCurrentOutCode(MCMD_EUNEXPECTED);
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

        delete transferdata;

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
        OUTSTRINGSTREAM oss;
        cd.print(oss);
        OUTSTREAM << oss.str();
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
    else
    {
        setCurrentOutCode(MCMD_EARGS);
        LOG_err << "Invalid command: " << words[0];
    }
}

}//end namespace

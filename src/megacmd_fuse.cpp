/**
 * (c) 2013 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of MEGAcmd.
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
#include "megacmd_fuse.h"

namespace FuseCommand {
std::string_view getDisclaimer()
{
    return "Disclaimer:\n"
           "    - Streaming is not supported; entire files need to be downloaded completely before being opened.\n"
           "    - FUSE uses a local cache located in the MEGAcmd configuration folder. Make sure you have enough available space "
           "in your hard drive to accommodate new files. Restarting MEGAcmd server can help discard old files.\n"
           "    - File writes might be deferred. When files are updated in the local mount point, a transfer will be initiated. "
           "Your files will be available in MEGA only after pending transfers finish.";
}

std::string_view getBetaMsg()
{
    return "FUSE commands are in early BETA. They are not available in macOS. If you experience any issues, please contact support@mega.nz.";
}

std::string_view getIdentifierParameter()
{
    return " name|localPath   The identifier of the mount we want to remove. It can be one of the following:\n"
           "                   Name: the user-friendly name of the mount, specified when it was added or by fuse-config.\n"
           "                   Local path: The local mount point in the filesystem.";
}
}

#ifdef WITH_FUSE

#include "listeners.h"
#include "megacmdlogger.h"
#include "configurationmanager.h"

using namespace mega;
using namespace megacmd;

namespace FuseCommand {
namespace {

constexpr const char* sFirstMountConfigKey = "firstFuseMountConfigured";

std::string getNodePath(MegaApi& api, MegaNode& node)
{
    std::unique_ptr<const char[]> nodePathPtr(api.getNodePath(&node));
    return (nodePathPtr ? nodePathPtr.get() : "<path not found>");
}

std::string getDefaultNodeName(MegaNode& node)
{
    switch (node.getType())
    {
        case MegaNode::TYPE_FILE:
        case MegaNode::TYPE_FOLDER:  return node.getName();
        case MegaNode::TYPE_ROOT:    return "MEGA";
        case MegaNode::TYPE_RUBBISH: return "MEGA Rubbish";
        case MegaNode::TYPE_VAULT:   return "MEGA Vault";
        case MegaNode::TYPE_UNKNOWN:
        default:                     return "<unknown>";
    }
}

// convenience function that returns the unique identifier (name) surrounded by quotes
std::string getMountId(const MegaMount& mount)
{
    return std::string("\"").append(mount.getFlags()->getName()).append("\"");
}

bool shouldRememberChange(const MegaMount& mount, bool temporarily)
{
    MegaMountFlags* mountFlags = mount.getFlags();
    assert(mountFlags != nullptr);

    if (!mountFlags->getPersistent())
    {
        return false;
    }

    return !temporarily;
}

std::unique_ptr<MegaMount> getMountByName(MegaApi& api, const std::string& name)
{
    return std::unique_ptr<MegaMount>(api.getMountInfo(name.c_str()));
}

std::unique_ptr<MegaMount> getMountByPath(MegaApi& api, const std::string& path, bool& clash)
{
    std::unique_ptr<MegaMountList> mounts(api.listMounts(false));
    if (mounts == nullptr)
    {
        return nullptr;
    }

    std::unique_ptr<MegaMount> firstFound;

    for (size_t i = 0; i < mounts->size(); ++i)
    {
        const MegaMount* mount = mounts->get(i);
        assert(mount != nullptr);

        if (mount->getPath() == path)
        {
            if (!firstFound)
            {
                firstFound.reset(mount->copy());
            }
            else
            {
                clash = true;
                return nullptr;
            }
        }
    }
    return firstFound;
}

std::unique_ptr<MegaMount> createMount(const fs::path& localPath, MegaNode& node, bool disabled, bool transient, bool readOnly, std::string name)
{
    std::unique_ptr<MegaMount> mount(MegaMount::create());
    assert(mount != nullptr);

    mount->setHandle(node.getHandle());

    const std::string localPathAsUtf8 = pathAsUtf8(localPath);
    mount->setPath(localPathAsUtf8.c_str());

    MegaMountFlags* mountFlags = mount->getFlags();
    assert(mountFlags != nullptr);

    if (name.empty())
    {
       name = getDefaultNodeName(node);
    }

    mountFlags->setName(name.c_str());
    mountFlags->setReadOnly(readOnly);
    mountFlags->setEnableAtStartup(!transient && !disabled);
    mountFlags->setPersistent(!transient);

    return mount;
}

void printFirstMountMessage()
{
    OUTSTREAM << "\n---------------------\n";
    OUTSTREAM << getDisclaimer();
    OUTSTREAM << "\n";
    OUTSTREAM << getBetaMsg();
    OUTSTREAM << "\n---------------------\n\n";
}
} // end namespace

std::unique_ptr<MegaMount> getMountByNameOrPath(MegaApi& api, const std::string& identifier)
{
    auto mount = getMountByName(api, identifier);
    if (mount != nullptr)
    {
        return mount;
    }

    bool clash = false;

    mount = getMountByPath(api, identifier, clash);
    if (clash)
    {
        assert(mount == nullptr);
        LOG_err << "Multiple mounts exist with path \"" << identifier << "\". Use mount name to select one";
    }
    else if (mount == nullptr)
    {
        LOG_err << "Identifier \"" << identifier << "\" does not correspond to a valid mount name or local path";
    }
    return mount;
}

std::string getActionString(std::string_view action, const fs::path &localPath, const std::string &nodePath)
{
    return std::string(action).append(" mount from \"").append(localPath.string()).append("\" to \"").append(nodePath).append("\"");
}

std::string getActionString(std::string_view action, const mega::MegaMount& mount)
{
    return std::string(action).append(" mount ").append(getMountId(mount)).append(" on \"").append(mount.getPath()).append("\"");
}

void addMount(mega::MegaApi& api, const fs::path& localPath, MegaNode& node, bool disabled, bool transient, bool readOnly, const std::string& name)
{
    const std::string nodePath = getNodePath(api, node);

    auto mountToCreate = createMount(localPath, node, disabled, transient, readOnly, name);
    auto listener = std::make_unique<MegaCmdListener>(nullptr);

    {
        DisableMountErrorsBroadcastingGuard disableErrorBroadcasting;
        api.addMount(mountToCreate.get(), listener.get());
        if (!checkNoErrors(listener.get(), getActionString("add", localPath, nodePath)))
        {
            return;
        }
    }


    OUTSTREAM << "Added a new mount";
    if (listener->getRequest()->getFile() && strlen(listener->getRequest()->getFile()))
    {
        OUTSTREAM << " from \"" << listener->getRequest()->getFile() << "\"";
    }
    OUTSTREAM << " to \"" << nodePath << '"' << endl;

    const bool isFirstMount = !ConfigurationManager::getConfigurationValue(sFirstMountConfigKey, false);
    if (isFirstMount)
    {
        printFirstMountMessage();

        sendEvent(StatsManager::MegacmdEvent::FIRST_CONFIGURED_FUSE_MOUNT, &api, false);
        ConfigurationManager::savePropertyValue(sFirstMountConfigKey, true);
    }
    else
    {
        sendEvent(StatsManager::MegacmdEvent::SUBSEQUENT_CONFIGURED_FUSE_MOUNT, &api, false);
    }

    if (!disabled)
    {
        enableMount(api, *mountToCreate, transient);
    }
}

void removeMount(mega::MegaApi& api, const mega::MegaMount& mount)
{
    {
        DisableMountErrorsBroadcastingGuard disableErrorBroadcasting;

        auto isEnabled = api.isMountEnabled(mount.getFlags()->getName());

        if (isEnabled)
        {
            auto listener = std::make_unique<MegaCmdListener>(nullptr);
            api.disableMount(mount.getFlags()->getName(), listener.get(), true /*remember*/);
            if (!checkNoErrors(listener.get(), getActionString("disable before removing", mount)))
            {
                return;
            }
        }

        auto listener = std::make_unique<MegaCmdListener>(nullptr);
        api.removeMount(mount.getFlags()->getName(), listener.get());
        if (!checkNoErrors(listener.get(), getActionString("remove", mount)))
        {
            return;
        }
    }

    OUTSTREAM << "Removed mount " << getMountId(mount) << " on \"" << mount.getPath() << '"' << endl;
}

void enableMount(mega::MegaApi& api, const mega::MegaMount& mount, bool temporarily)
{
    auto listener = std::make_unique<MegaCmdListener>(nullptr);
    const bool remember = shouldRememberChange(mount, temporarily);

    {
        DisableMountErrorsBroadcastingGuard disableErrorBroadcasting;
        api.enableMount(mount.getFlags()->getName(), listener.get(), remember);
        if (!checkNoErrors(listener.get(), getActionString("enable", mount)))
        {
            return;
        }
    }

    auto mountEnabled = getMountByNameOrPath(api, mount.getFlags()->getName());
    if (!mountEnabled)
    {
        LOG_err << "Unable to get mount info after enabled";
        setCurrentThreadOutCode(MCMD_NOTFOUND);
        return;
    }


    OUTSTREAM << (temporarily ? "Temporarily enabled" : "Enabled") << " mount "
              << getMountId(*mountEnabled) << " on \"" << mountEnabled->getPath() << '"' << endl;
}

void disableMount(mega::MegaApi& api, const mega::MegaMount& mount, bool temporarily)
{
    auto listener = std::make_unique<MegaCmdListener>(nullptr);
    const bool remember = shouldRememberChange(mount, temporarily);


    {
        DisableMountErrorsBroadcastingGuard disableErrorBroadcasting;
        api.disableMount(mount.getFlags()->getName(), listener.get(), remember);
        if (!checkNoErrors(listener.get(), getActionString("disable", mount)))
        {
            return;
        }
    }

    OUTSTREAM << (temporarily ? "Temporarily disabled" : "Disabled") << " mount "
              << getMountId(mount) << " on \"" << mount.getPath() << '"' << endl;
}

void printMount(mega::MegaApi& api, const mega::MegaMount& mount)
{
    const MegaHandle handle = mount.getHandle();
    std::unique_ptr<char[]> remotePath(api.getNodePathByNodeHandle(handle));

    const MegaMountFlags* flags = mount.getFlags();
    assert(flags != nullptr);

    std::ostringstream oss;
    oss << "Showing details of mount " << getMountId(mount) << "\n"
        << "  Local path:         " << mount.getPath() << "\n"
        << "  Remote path:        " << (remotePath ? remotePath.get() : "<not found>") << "\n"
        << "  Name:               " << flags->getName() << "\n"
        << "  Persistent:         " << (flags->getPersistent() ? "YES" : "NO") << "\n"
        << "  Enabled:            " << (api.isMountEnabled(flags->getName()) ? "YES" : "NO") << "\n"
        << "  Enable at startup:  " <<  (flags->getEnableAtStartup() ? "YES" : "NO") << "\n"
        << "  Read-only:          " << (flags->getReadOnly() ? "YES" : "NO") << "\n";

    OUTSTREAM << oss.str();
}

void printAllMounts(mega::MegaApi& api, ColumnDisplayer& cd, bool onlyEnabled, bool disablePathCollapse, int rowCountLimit)
{
    std::unique_ptr<MegaMountList> mounts(api.listMounts(onlyEnabled));
    if (mounts == nullptr)
    {
        LOG_err << "Failed to retrieve the list of mounts";
        return;
    }

    if (mounts->size() == 0)
    {
        OUTSTREAM << "There are no mounts" << (onlyEnabled ? " enabled" : "") << endl;
        return;
    }

    cd.addHeader("LOCAL_PATH", disablePathCollapse);
    cd.addHeader("REMOTE_PATH", disablePathCollapse);

    for (size_t i = 0; i < mounts->size() && i < rowCountLimit; ++i)
    {
        assert(mounts->get(i) != nullptr);
        const MegaMount& mount = *mounts->get(i);

        const MegaHandle handle = mount.getHandle();
        std::unique_ptr<char[]> remotePath(api.getNodePathByNodeHandle(handle));

        const MegaMountFlags* flags = mount.getFlags();
        assert(flags != nullptr);

        cd.addValue("NAME", flags->getName());
        cd.addValue("LOCAL_PATH", mount.getPath());
        cd.addValue("REMOTE_PATH", remotePath ? remotePath.get() : "<not found>");
        cd.addValue("PERSISTENT", flags->getPersistent() ? "YES" : "NO");
        cd.addValue("ENABLED", api.isMountEnabled(mount.getFlags()->getName()) ? "YES" : "NO");
    }

    OUTSTREAM << cd.str();
    OUTSTREAM << endl;
    if (rowCountLimit < mounts->size())
    {
        OUTSTREAM << "Note: showing " << rowCountLimit << " out of " << mounts->size() << " mounts. "
                  << "Use \"" << getCommandPrefixBasedOnMode() << "fuse-show --limit=0\" to see all of them." << endl;
    }
    OUTSTREAM << "Use \"" << getCommandPrefixBasedOnMode() << "fuse-show <NAME|LOCAL_PATH>\" to get further details on a specific mount." << endl;
}

bool ConfigDelta::isAnyFlagSet() const
{
    return mEnableAtStartup || mPersistent || mReadOnly || mName;
}

bool ConfigDelta::isPersistentStartupInvalid() const
{
    bool enableAtStartup = mEnableAtStartup && *mEnableAtStartup;
    bool transient = mPersistent && !(*mPersistent);
    return enableAtStartup && transient;
}

void changeConfig(mega::MegaApi& api, const mega::MegaMount& mount, const ConfigDelta& delta)
{
    assert(delta.isAnyFlagSet());
    assert(!delta.isPersistentStartupInvalid());

    MegaMountFlags* flags = mount.getFlags();
    assert(flags != nullptr);

    if (delta.mEnableAtStartup)
    {
        flags->setEnableAtStartup(*delta.mEnableAtStartup);
    }
    if (delta.mPersistent)
    {
        flags->setPersistent(*delta.mPersistent);
    }
    if (delta.mReadOnly)
    {
        flags->setReadOnly(*delta.mReadOnly);
    }
    std::string currentName(mount.getFlags()->getName());
    if (delta.mName)
    {
        flags->setName(delta.mName->c_str());
    }

    auto listener = std::make_unique<MegaCmdListener>(nullptr);


    {
        DisableMountErrorsBroadcastingGuard disableErrorBroadcasting;
        api.setMountFlags(flags, currentName.c_str(), listener.get());
        if (!checkNoErrors(listener.get(), getActionString("change the flags of", mount)))
        {
            return;
        }
    }

    OUTSTREAM << "Mount " << getMountId(mount) << " now has the following flags\n"
              << "  Name:               " << flags->getName() << "\n"
              << "  Enable at startup:  " << (flags->getEnableAtStartup() ? "YES" : "NO") << "\n"
              << "  Persistent:         " << (flags->getPersistent() ? "YES" : "NO") << "\n"
              << "  Read-only:          " << (flags->getReadOnly() ? "YES" : "NO") << "\n";
}
}

#endif // WITH_FUSE

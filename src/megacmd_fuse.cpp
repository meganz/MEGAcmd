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

#include <functional>

#include "listeners.h"
#include "megacmdlogger.h"

using namespace mega;
using namespace megacmd;
using std::string;

namespace FuseCommand {

namespace {
using FuseOperation = std::function<void(const char*, MegaRequestListener*, bool)>;

void fuseBadArgument(const string& command)
{
    setCurrentOutCode(MCMD_EARGS);
    LOG_err << getUsageStr(command.c_str());
}

string fuseToString(const MegaMountFlags& flags)
{
    ostringstream ostream;
    ostream << "Enable at Startup: "
            << (flags.getEnableAtStartup() ? "YES" : "NO")
            << "\n"
            << "Name: "
            << flags.getName()
            << "\n"
            << "Persistent: "
            << (flags.getPersistent() ? "YES" : "NO")
            << "\n"
            << "Read only: "
            << (flags.getReadOnly() ? "YES" : "NO");

    return ostream.str();
}

string fuseToString(MegaApi& api, const MegaMount& mount)
{
    // Convenience.
    using CharPtr = unique_ptr<char[]>;

    CharPtr handle(api.handleToBase64(mount.getHandle()));
    CharPtr path(api.getNodePathByNodeHandle(mount.getHandle()));

    ostringstream ostream;
    ostream << "Mount \""
            << mount.getPath()
            << "\":\n"
            << "Enabled: "
            << (api.isMountEnabled(mount.getPath()) ? "YES" : "NO")
            << "\n"
            << "Remote Handle: "
            << handle.get()
            << "\n"
            << "Remote Path: "
            << (path ? "\"" : "")
            << (path ? path.get() : "N/A")
            << (path ? "\"" : "")
            << "\n"
            << fuseToString(*mount.getFlags())
            << "\n";

    return ostream.str();
}

string fuseNameOrPath(MegaApi& api, const string& command, const FromStringMap<string>& options)
{
    // By name or path but never both.
    auto byName = options.find("by-name");
    auto byPath = options.find("by-path");

    if (byName != options.end() && byPath != options.end())
    {
        fuseBadArgument(command);
        return "";
    }

    // Mount selected by path.
    if (byPath != options.end())
    {
        if (byPath->second.empty())
        {
            fuseBadArgument(command);
            return "";
        }

        return byPath->second;
    }

    // No name specified.
    if (byName == options.end() || byName->second.empty())
    {
        fuseBadArgument(command);
        return "";
    }

    // Mount selected by name.
    unique_ptr<MegaStringList> paths(api.getMountPaths(byName->second.c_str()));

    // Only a single mount is known by this name.
    if (paths->size() == 1)
    {
        return paths->get(0);
    }

    if (paths->size() > 1)
    {
        // Multiple mounts are known by this name.
        LOG_err << "Multiple mounts are known by the name \""
                << byName->second
                << "\".";
    }
    else
    {
        // No mounts are known by this name,
        LOG_err << "There are no mounts named \""
                << byName->second
                << "\".";
    }

    return "";
}

template<typename T>
bool fuseArgument(const Args& args, const Options& options, T& destination, const string& name)
{
    // Has the argument been specified?
    auto i = options.find(name);

    // Argument hasn't been specified.
    if (i == options.end())
    {
        return true;
    }

    std::istringstream istream(i->second);

    // Try and extract argument value.
    if (!(istream >> destination))
    {
        fuseBadArgument(args.front());
        return false;
    }

    // Skip trailing whitespace.
    while (istream && std::isspace(istream.get()));

    // Argument must be an atom.
    if (istream.bad() || !istream.eof())
    {
        fuseBadArgument(args.front());
        return false;
    }

    return true;
}

void fuseListMounts(MegaApi& api, const Args& args, const Flags& flags, const Options& options)
{
    // Convenience.
    using MountListPtr = unique_ptr<MegaMountList>;

    // Should we only list enabled mounts?
    auto onlyEnabled = getFlag(&flags, "only-enabled");

    // Try and get a list of mounts.
    MountListPtr mounts(api.listMounts(onlyEnabled));

    // Couldn't retrieve the mount list.
    if (!mounts)
    {
        LOG_err << "Unable to retrieve a list of mounts.";
        return;
    }

    // There are no mounts to list.
    if (!mounts->size())
    {
        OUTSTREAM << "There are no "
                  << (onlyEnabled ? "enabled " : "")
                  << "mounts."
                  << endl;
        return;
    }

    // Display each mount in turn.
    size_t i = 0;

    OUTSTREAM << fuseToString(api, *mounts->get(i));

    while (++i < mounts->size())
    {
        OUTSTREAM << "\n"
                  << fuseToString(api, *mounts->get(i));
    }

    OUTSTREAM << flush;
}

void fuseMountInfo(MegaApi& api, const Args& args, const Flags& flags, const Options& options)
{
    // Convenience.
    using MountPtr = unique_ptr<MegaMount>;

    // Make sure the user's selected a mount by name or by path.
    auto path = fuseNameOrPath(api, args.front(), options);

    // No mounts with the specified name.
    if (path.empty())
    {
        return;
    }

    // Try and retrieve a description of the specified mount.
    MountPtr mount(api.getMountInfo(path.c_str()));

    // Mount doesn't exist or we couldn't retrieve its description.
    if (!mount)
    {
        LOG_err << "Unable to describe the mount at \""
                << path
                << "\".";
        return;
    }

    OUTSTREAM << fuseToString(api, *mount)
              << flush;
}

void fuseOperate(MegaApi& api, const Args& args, const Flags& flags, const FuseOperation operation, const string& action, const Options& options)
{
    // Convenience.
    using ListenerPtr = unique_ptr<MegaCmdListener>;

    auto& command = args.front();

    // Make sure the user's selected a mount by name or by path.
    auto path = fuseNameOrPath(api, command, options);

    // No mount exists with the specified name.
    if (path.empty())
    {
        return;
    }

    // Should the new mount's state be remembered?
    //
    // That is, if we disable a mount, should it remain disabled when
    // we next start up the application?
    //
    // Note that this flag has no meaning for remove operations.
    auto remember = flags.count("remember") > 0;

    // Execute the operation.
    ListenerPtr listener(new MegaCmdListener(nullptr));

    operation(path.c_str(), listener.get(), remember);

    // Wait for the operation to complete.
    listener->wait();

    // Operation was successful.
    auto* error = listener->getError();
    assert(error);

    if (error->getErrorCode() != MegaError::API_OK)
    {
        // Operation failed.
        LOG_err << "Unable to "
                << action
                << " the mount on \""
                << path
                << "\" due to error: "
                << MegaMount::getResultString(error->getMountResult());
        return;

    }

    OUTSTREAM << "Successfully "
                << action
                << "d the mount on \""
                << path
                << "\"."
                << endl;
}
} // end namespace

void addMount(MegaApi& api, std::unique_ptr<MegaNode>&& remoteNode, const Args& args, const Flags& flags, const Options& options)
{
    // Convenience.
    using ListenerPtr = unique_ptr<MegaCmdListener>;
    using MountPtr = unique_ptr<MegaMount>;

    // Make sure the user's given us three args.
    if (args.size() != 3)
    {
        fuseBadArgument(args.front());
        return;
    }

    // Clarity.
    auto& localPath = args[1];
    auto& remotePath = args[2];

    if (localPath.empty() || remotePath.empty())
    {
        fuseBadArgument(args.front());
        return;
    }

    auto name = getOption(&options, "name");
    auto persistent = !getFlag(&flags, "transient");
    auto readOnly = getFlag(&flags, "read-only");

    // Determine remote node handle.
    MegaHandle remoteHandle = UNDEF;

    // Node was found.
    if (remoteNode)
    {
        // Latch the remote node's handle.
        remoteHandle = remoteNode->getHandle();

        // Compute the mount's name if not specified.
        if (name.empty())
        {
            switch (remoteNode->getType())
            {
            case MegaNode::TYPE_FILE:
            case MegaNode::TYPE_FOLDER:
                name = remoteNode->getName();
                break;
            case MegaNode::TYPE_ROOT:
                name = "MEGA";
                break;
            case MegaNode::TYPE_RUBBISH:
                name = "MEGA Rubbish";
                break;
            case MegaNode::TYPE_VAULT:
                name = "MEGA Vault";
                break;
            case MegaNode::TYPE_UNKNOWN:
                name = "Unknown";
                break;
            }
        }
    }

    // Describe the new mount.
    MountPtr mount(MegaMount::create());

    // Specify mapping.
    mount->setHandle(remoteHandle);
    mount->setPath(localPath.c_str());

    // Populate flags.
    auto* mountFlags = mount->getFlags();

    mountFlags->setName(name.c_str());
    mountFlags->setReadOnly(readOnly);

    if (persistent)
    {
        mountFlags->setEnableAtStartup(true);
    }

    // Try and add the new mount.
    ListenerPtr listener(new MegaCmdListener(nullptr));

    api.addMount(mount.get(), listener.get());

    // Wait for a result.
    listener->wait();

    // Mount was added successfully.
    auto* error = listener->getError();
    assert(error);

    if (error->getErrorCode() != MegaError::API_OK)
    {
        LOG_err << "Unable to add mount from \""
                << remotePath
                << "\" to \""
                << localPath
                << "\" due to error: "
                << MegaMount::getResultString(error->getMountResult());
        return;
    }

    OUTSTREAM << "Successfully added a new mount from \""
                << remotePath
                << "\" to \""
                << localPath
                << "\"."
                << endl;
}

void disableMount(MegaApi& api, const Args& args, const Flags& flags, const Options& options)
{
    fuseOperate(api, args,
                flags,
                std::bind(&MegaApi::disableMount,
                          api,
                          std::placeholders::_1,
                          std::placeholders::_2,
                          std::placeholders::_3),
                "disable",
                options);
}

void enableMount(MegaApi& api, const Args& args, const Flags& flags, const Options& options)
{
    fuseOperate(api, args,
                flags,
                std::bind(&MegaApi::enableMount,
                          api,
                          std::placeholders::_1,
                          std::placeholders::_2,
                          std::placeholders::_3),
                "enable",
                options);
}

void removeMount(MegaApi& api, const Args& args, const Flags& flags, const Options& options)
{
    fuseOperate(api, args,
                flags,
                std::bind(&MegaApi::removeMount,
                          api,
                          std::placeholders::_1,
                          std::placeholders::_2),
                "remove",
                options);
}

// Convenience.
#define DEFINE_LOG_LEVELS(expander) \
    expander(ERROR) \
    expander(WARNING) \
    expander(INFO) \
    expander(DEBUG)

void flags(MegaApi& api, const Args& args, const Flags& /*flags*/, const Options& options)
{
    static std::map<int, string> fromLogLevel = {
#define FROM_LOG_LEVEL(name) {MegaFuseFlags::LOG_LEVEL_##name, #name},
        DEFINE_LOG_LEVELS(FROM_LOG_LEVEL)
#undef FROM_LOG_LEVEL
    }; // fromLogLevel

    static std::map<string, int> toLogLevel = {
#define TO_LOG_LEVEL(name) {#name, MegaFuseFlags::LOG_LEVEL_##name},
        DEFINE_LOG_LEVELS(TO_LOG_LEVEL)
#undef TO_LOG_LEVEL
    }; // toLogLevel

    auto displayFuseFlags = [&] (MegaFuseFlags& flags)
    {
        OUTSTREAM << "Log Level: "
                  << fromLogLevel[flags.getLogLevel()]
                  << "\n"
                  << flush;
    }; // displayFuseFlags

    auto updateFuseFlags = [&] (MegaFuseFlags& flags)
    {
        auto logLevel = fromLogLevel[flags.getLogLevel()];

        if (!fuseArgument(args, options, logLevel, "log-level"))
        {
            return false;
        }

        if (!toLogLevel.count(logLevel))
        {
            fuseBadArgument(args.front());
            return false;
        }

        // Update log level.
        flags.setLogLevel(toLogLevel[logLevel]);

        api.setFUSEFlags(&flags);

        // We're all done.
        return true;
    }; // updateFuseFlags

    // Convenience.
    using FlagsPtr = std::unique_ptr<MegaFuseFlags>;

    // Get our hands on FUSE's current flags.
    FlagsPtr flags(api.getFUSEFlags());

    // Try and update FUSE's flags.
    if (!updateFuseFlags(*flags))
    {
        return;
    }

    // Display updated flags.
    displayFuseFlags(*flags);
}

#undef DEFINE_LOG_LEVELS

void mountFlags(MegaApi& api, const Args& args, const Flags& flags, const Options& options)
{
    // Convenience.
    using FlagsPtr = unique_ptr<MegaMountFlags>;
    using ListenerPtr = unique_ptr<MegaCmdListener>;

    // Make sure the user's selected a mount by name or by path.
    auto path = fuseNameOrPath(api, args.front(), options);

    // No mount exists with the specified name.
    if (path.empty())
    {
        return;
    }

    // Convenience.
    auto disableAtStartup = flags.count("disabled-at-startup");
    auto enableAtStartup = flags.count("enabled-at-startup");
    auto persistent = flags.count("persistent");
    auto readOnly = flags.count("read-only");
    auto transient = flags.count("transient");
    auto writable = flags.count("writable");

    // Check for mutually exclusive flags.

    // Transient mounts are always disabled on startup.
    disableAtStartup -= disableAtStartup * transient;

    if (enableAtStartup)
    {
        if (disableAtStartup)
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "A mount is either disabled or enabled at startup.";
            return;
        }

        if (transient)
        {
            setCurrentOutCode(MCMD_EARGS);
            LOG_err << "A transient mount cannot be enabled at startup.";
            return;
        }
    }

    if (transient && persistent)
    {
        setCurrentOutCode(MCMD_EARGS);
        LOG_err << "A mount is either persistent or transient.";
        return;
    }

    if (readOnly && writable)
    {
        setCurrentOutCode(MCMD_EARGS);
        LOG_err << "A mount is either read-only or writable.";
        return;
    }

    // Try and retrieve the specified mount's flags.
    FlagsPtr mountFlags(api.getMountFlags(path.c_str()));

    // Mount doesn't exist or couldn't determine the flags.
    if (!mountFlags)
    {
        LOG_err << "Unable to retrieve flags for the mount at \""
                << path
                << "\".";
        return;
    }

    // Are we changing the mount's flags?
    bool changing = false;

    if (options.count("name"))
    {
        changing = (mountFlags->setName(options.at("name").c_str()), true);
    }

    if (persistent || transient)
    {
        changing = (mountFlags->setPersistent(persistent), true);
    }

    if (disableAtStartup || enableAtStartup)
    {
        changing = (mountFlags->setEnableAtStartup(enableAtStartup), true);
    }

    if (readOnly || writable)
    {
        changing = (mountFlags->setReadOnly(readOnly), true);
    }

    // Not changing the mount's flags.
    if (!changing)
    {
        OUTSTREAM << "No configuration has been changed.\n"
                  << "Mount \""
                  << path
                  << "\" has the following configurations:\n"
                  << fuseToString(*mountFlags)
                  << endl;
        return;
    }

    // Try and update the mount's flags.
    ListenerPtr listener(new MegaCmdListener(nullptr));

    api.setMountFlags(mountFlags.get(), path.c_str(), listener.get());

    // Wait for the operation to complete.
    listener->wait();

    // Were we able to update the mount's flags?
    auto* error = listener->getError();
    assert(error);

    if (error->getErrorCode() != MegaError::API_OK)
    {
        // API doesn't let us change a mount's flags yet.
        LOG_err << "Unable to change the flags of mount \""
                << path
                << "\" due to error: "
                << MegaMount::getResultString(error->getMountResult());
        return;
    }

    OUTSTREAM << "Mount \""
                << path
                << "\" now has the following flags:\n"
                << fuseToString(*mountFlags)
                << endl;
}

void showMounts(MegaApi& api, const Args& args, const Flags& flags, const Options& options)
{
    auto byName = options.count("by-name");
    auto byPath = options.count("by-path");
    auto onlyEnabled = flags.count("only-enabled");

    // Only one of them is allowed
    if (byName + byPath + onlyEnabled > 1)
    {
        fuseBadArgument(args.front());
        return;
    }

    if (byName || byPath)
    {
        // One mount
        fuseMountInfo(api, args, flags, options);
    }
    else
    {
        // A list of mounts
        fuseListMounts(api, args, flags, options);
    }
}

} // end namespace

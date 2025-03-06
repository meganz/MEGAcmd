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

#pragma once

#ifdef WITH_FUSE

#include "megaapi.h"
#include "megacmdcommonutils.h"

namespace FuseCommand
{
    std::unique_ptr<mega::MegaMount> getMountByIdOrPathOrName(mega::MegaApi& api, const std::string& identifier);

    void addMount(mega::MegaApi& api, const fs::path& localPath, mega::MegaNode& node, bool disabled, bool transient, bool readOnly, const std::string& name);
    void removeMount(mega::MegaApi& api, const mega::MegaMount& mount);

    void enableMount(mega::MegaApi& api, const mega::MegaMount& mount, bool temporarily);
    void disableMount(mega::MegaApi& api, const mega::MegaMount& mount, bool temporarily);

    void printMount(mega::MegaApi& api, const mega::MegaMount& mount);
    void printAllMounts(mega::MegaApi& api, megacmd::ColumnDisplayer& cd, bool onlyEnabled, bool disablePathCollapse, int rowCountLimit);

    struct ConfigDelta
    {
        std::optional<bool> mEnableAtStartup;
        std::optional<bool> mPersistent;
        std::optional<bool> mReadOnly;
        std::optional<std::string> mName;

        bool isAnyFlagSet() const;
        bool isPersistentStartupInvalid() const;
    };

    void changeConfig(mega::MegaApi& api, const mega::MegaMount& mount, const ConfigDelta& delta);
}

#endif // WITH_FUSE

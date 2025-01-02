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

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "megaapi.h"

namespace FuseCommand
{
    using Args = std::vector<std::string>;
    using Flags = std::map<std::string, int>;
    using Options = std::map<std::string, std::string>;

    void addMount(mega::MegaApi& api, std::unique_ptr<mega::MegaNode>&& remoteNode, const Args& arguments, const Flags& flags, const Options& options);
    void disableMount(mega::MegaApi& api, const Args& args, const Flags& flags, const Options& options);
    void enableMount(mega::MegaApi& api, const Args& args, const Flags& flags, const Options& options);
    void removeMount(mega::MegaApi& api, const Args& args, const Flags& flags, const Options& options);

    void flags(mega::MegaApi& api, const Args& args, const Flags& flags, const Options& options);
    void mountFlags(mega::MegaApi& api, const Args& args, const Flags& flags, const Options& options);

    void showMounts(mega::MegaApi& api, const Args& args, const Flags& flags, const Options& options);
}

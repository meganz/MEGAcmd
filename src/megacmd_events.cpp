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

#include "megacmd_events.h"

#include <array>


namespace megacmd::StatsManager {

const char * defaultEventMsg(MegacmdEvent ev)
{
    static auto sDefaultMsgs = []()
    {
        std::array<const char *, static_cast<int>(MegacmdEvent::LastEvent) - FIRST_EVENT_NUMBER> defaultMsgs;
#define SOME_GENERATOR_MACRO(name, _, msg) defaultMsgs[static_cast<int>(MegacmdEvent::name) - FIRST_EVENT_NUMBER] = msg;
        GENERATE_FROM_MEGACMD_EVENTS(SOME_GENERATOR_MACRO)
        #undef SOME_GENERATOR_MACRO
        return defaultMsgs;
    }();

    return sDefaultMsgs[static_cast<int>(ev) - FIRST_EVENT_NUMBER];
}

const char * eventName(MegacmdEvent ev)
{
    static auto sNames = []()
    {
        std::array<const char *, static_cast<int>(MegacmdEvent::LastEvent) - FIRST_EVENT_NUMBER> names;
#define SOME_GENERATOR_MACRO(name, _, __) names[static_cast<int>(MegacmdEvent::name) - FIRST_EVENT_NUMBER] = #name;
        GENERATE_FROM_MEGACMD_EVENTS(SOME_GENERATOR_MACRO)
        #undef SOME_GENERATOR_MACRO
        return names;
    }();

    return sNames[static_cast<int>(ev) - FIRST_EVENT_NUMBER];
}

} // end of namespaces

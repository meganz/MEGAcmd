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

namespace megacmd::StatsManager {

    /** MEGAcmd events **/
    // Allocated ranges:
    //    - regular: [98'900,98'999]
    //    - extended [860'000,879'999]
    // Params:
    //   - event name
    //   - number
    //   - Default message

    #define GENERATE_FROM_MEGACMD_EVENTS(GENERATOR_MACRO) \
        GENERATOR_MACRO(UPDATE                                          , 98900, "MEGAcmd update") \
        GENERATOR_MACRO(UPDATE_START                                    , 98901, "MEGAcmd auto-update start") \
        GENERATOR_MACRO(UPDATE_RESTART                                  , 98902, "MEGAcmd updated requiring restart") \
        GENERATOR_MACRO(FIRST_CONFIGURED_SYNC                           , 98903, "MEGAcmd first sync configured") \
        GENERATOR_MACRO(WAITED_TOO_LONG_FOR_NODES_CURRENT               , 98904, "MEGAcmd nodes current wait timed out")

    static constexpr auto FIRST_EVENT_NUMBER = 98900u;

    enum class MegacmdEvent
    {
    #define SOME_GENERATOR_MACRO(name, num, __) name = num,
      GENERATE_FROM_MEGACMD_EVENTS(SOME_GENERATOR_MACRO)
    #undef SOME_GENERATOR_MACRO
        LastEvent
    };

    const char *defaultEventMsg(MegacmdEvent ev);
    const char *eventName(MegacmdEvent ev);

} // end of namespaces

/**
 * (c) 2026 by Mega Limited, Auckland, New Zealand
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

#include <set>
#include <string>

#include <gtest/gtest.h>

#include "configurationmanager.h"

using megacmd::ConfiguratorMegaApiHelper;

namespace
{
// Invokes fn with the configurator whose key matches. Returns false if no such key exists.
// ValueConfigurator is a private nested type, so it is reached through a generic lambda.
template<typename Fn>
bool withConfigurator(ConfiguratorMegaApiHelper& helper, const std::string& key, Fn&& fn)
{
    for (const auto& configurator : helper.getConfigurators())
    {
        if (configurator.mKey == key)
        {
            fn(configurator);
            return true;
        }
    }
    return false;
}
}

TEST(ConfiguratorMegaApiHelperTest, fileServiceReclaimKeysAreRegistered)
{
    ConfiguratorMegaApiHelper helper;

    std::set<std::string> keys;
    for (const auto& configurator : helper.getConfigurators())
    {
        keys.insert(configurator.mKey);
    }

    EXPECT_EQ(keys.count("file_service_reclaim_age_threshold"), 1u);
    EXPECT_EQ(keys.count("file_service_reclaim_batch_size"), 1u);
    EXPECT_EQ(keys.count("file_service_reclaim_delay"), 1u);
    EXPECT_EQ(keys.count("file_service_reclaim_period"), 1u);
    EXPECT_EQ(keys.count("file_service_reclaim_threshold"), 1u);
    EXPECT_EQ(keys.count("file_service_reclaim_target"), 1u);
}

TEST(ConfiguratorMegaApiHelperTest, everyConfiguratorHasSetterGetterAndValidator)
{
    ConfiguratorMegaApiHelper helper;

    for (const auto& configurator : helper.getConfigurators())
    {
        EXPECT_TRUE(static_cast<bool>(configurator.mSetter)) << configurator.mKey;
        EXPECT_TRUE(static_cast<bool>(configurator.mGetter)) << configurator.mKey;
        ASSERT_TRUE(configurator.mValidator.has_value()) << configurator.mKey;
        EXPECT_TRUE(static_cast<bool>(configurator.mValidator.value())) << configurator.mKey;
    }
}

TEST(ConfiguratorMegaApiHelperTest, reclaimThresholdValidatorAcceptsBytesOffAndSentinels)
{
    ConfiguratorMegaApiHelper helper;

    const bool found = withConfigurator(helper, "file_service_reclaim_threshold", [](const auto& configurator)
    {
        const auto& validate = configurator.mValidator.value();

        // High-water mark in bytes, plus the "no minimum" (0) and "disabled" (-1 or "off") sentinels.
        EXPECT_TRUE(validate("0"));
        EXPECT_TRUE(validate("10737418240"));
        EXPECT_TRUE(validate("-1"));
        EXPECT_TRUE(validate("off"));
        EXPECT_TRUE(validate("OFF"));

        EXPECT_FALSE(validate("disabled"));
        EXPECT_FALSE(validate("none"));
        EXPECT_FALSE(validate("-2"));
        EXPECT_FALSE(validate("abc"));
        EXPECT_FALSE(validate(""));
        EXPECT_FALSE(validate("12x"));
        EXPECT_FALSE(validate(" -1"));
        EXPECT_FALSE(validate("+1"));
        EXPECT_FALSE(validate("9223372036854775808"));
    });
    EXPECT_TRUE(found);
}

TEST(ConfiguratorMegaApiHelperTest, reclaimNonNegativeValidatorsRejectNegativesAndGarbage)
{
    ConfiguratorMegaApiHelper helper;

    const bool foundDelay = withConfigurator(helper, "file_service_reclaim_delay", [](const auto& configurator)
    {
        const auto& validate = configurator.mValidator.value();
        EXPECT_TRUE(validate("0"));
        EXPECT_TRUE(validate("60"));
        EXPECT_TRUE(validate("2147483647"));
        EXPECT_FALSE(validate("2147483648"));
        EXPECT_FALSE(validate("-1"));
        EXPECT_FALSE(validate(" -1"));
        EXPECT_FALSE(validate(" 5"));
        EXPECT_FALSE(validate("+5"));
        EXPECT_FALSE(validate("18446744073709551616"));
        EXPECT_FALSE(validate("abc"));
        EXPECT_FALSE(validate("12x"));
    });
    EXPECT_TRUE(foundDelay);

    // Period shares the seconds cap; 0 is not a valid interval.
    const bool foundPeriod = withConfigurator(helper, "file_service_reclaim_period", [](const auto& configurator)
    {
        const auto& validate = configurator.mValidator.value();
        EXPECT_FALSE(validate("0"));
        EXPECT_TRUE(validate("7200"));
        EXPECT_TRUE(validate("2147483647"));
        EXPECT_FALSE(validate("2147483648"));
        EXPECT_FALSE(validate("18446744073709551615"));
    });
    EXPECT_TRUE(foundPeriod);

    // Batch size must be at least one file per batch.
    const bool foundBatch = withConfigurator(helper, "file_service_reclaim_batch_size", [](const auto& configurator)
    {
        const auto& validate = configurator.mValidator.value();
        EXPECT_FALSE(validate("0"));
        EXPECT_TRUE(validate("1"));
        EXPECT_TRUE(validate("4"));
        EXPECT_FALSE(validate("-1"));
        EXPECT_FALSE(validate(" -1"));
    });
    EXPECT_TRUE(foundBatch);

    // Age threshold is stored as an int (minutes); values beyond INT_MAX are rejected.
    const bool foundAge = withConfigurator(helper, "file_service_reclaim_age_threshold", [](const auto& configurator)
    {
        const auto& validate = configurator.mValidator.value();
        EXPECT_TRUE(validate("0"));
        EXPECT_TRUE(validate("4320"));
        EXPECT_TRUE(validate("2147483647"));
        EXPECT_FALSE(validate("2147483648"));
        EXPECT_FALSE(validate("-1"));
    });
    EXPECT_TRUE(foundAge);
}

#ifdef WITH_FUSE
TEST(ConfiguratorMegaApiHelperTest, fuseLogLevelValidatorTakesNamesAndNumbers)
{
    ConfiguratorMegaApiHelper helper;

    const bool found = withConfigurator(helper, "fuse_log_level", [](const auto& configurator)
    {
        const auto& validate = configurator.mValidator.value();

        // The same names and numbers the "log" command takes.
        EXPECT_TRUE(validate("ERROR"));
        EXPECT_TRUE(validate("debug"));
        EXPECT_TRUE(validate("VERBOSE"));
        EXPECT_TRUE(validate("4"));

        EXPECT_FALSE(validate("NONSENSE"));
        EXPECT_FALSE(validate(""));
    });
    EXPECT_TRUE(found);
}
#endif

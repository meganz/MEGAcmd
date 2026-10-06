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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "comunicationsmanager.h"
#include "listeners.h"
#include "megacmdsandbox.h"

using mega::MegaError;
using mega::MegaErrorPrivate;
using mega::MegaTransfer;
using megacmd::ComunicationsManager;
using megacmd::MegaCmdGlobalTransferListener;
using megacmd::MegaCmdSandbox;
using megacmd::transferQuotaWaitMessage;
using testing::HasSubstr;
using testing::StartsWith;

namespace megacmd {
extern ComunicationsManager *cm;
}

namespace {

class FakeTransfer : public MegaTransfer
{
    int mType;
    int mTag;
    int mState;

public:
    FakeTransfer(int type, int tag, int state) : mType(type), mTag(tag), mState(state) {}

    MegaTransfer *copy() override { return new FakeTransfer(mType, mTag, mState); }

    int getType() const override { return mType; }
    int getTag() const override { return mTag; }
    int getState() const override { return mState; }
};

// Broadcasts go through the global communications manager; this one has no clients.
class NoClientsManager : public ComunicationsManager
{
public:
    void sendPartialOutput(megacmd::CmdPetition *, OUTSTRING *) override {}
    void sendPartialOutput(megacmd::CmdPetition *, char *, size_t, bool) override {}
    void sendPartialError(megacmd::CmdPetition *, OUTSTRING *) override {}
    void sendPartialError(megacmd::CmdPetition *, char *, size_t, bool) override {}
};

class TransferQuotaWait : public testing::Test
{
    NoClientsManager mManager;

protected:
    MegaCmdSandbox mSandbox;
    MegaCmdGlobalTransferListener mListener{nullptr, &mSandbox};

    void SetUp() override { megacmd::cm = &mManager; }
    void TearDown() override { megacmd::cm = nullptr; }

    void transferQuotaRunsOut(int type, int tag, long long seconds)
    {
        FakeTransfer transfer(type, tag, MegaTransfer::STATE_RETRYING);
        MegaErrorPrivate overquota(MegaError::API_EOVERQUOTA, seconds);
        mListener.onTransferTemporaryError(nullptr, &transfer, &overquota);
    }

    void downloadMovesAgain(int tag)
    {
        FakeTransfer transfer(MegaTransfer::TYPE_DOWNLOAD, tag, MegaTransfer::STATE_ACTIVE);
        mListener.onTransferUpdate(nullptr, &transfer);
    }
};

}

TEST(TransferQuotaWaitMessage, saysHowLongToWait)
{
    auto message = transferQuotaWaitMessage(630);
    ASSERT_TRUE(message.has_value());
    EXPECT_THAT(*message, StartsWith("Transfer quota exceeded. You can't continue downloading"));
    EXPECT_THAT(*message, HasSubstr("wait for 10 minutes until more free quota becomes available"));
}

TEST(TransferQuotaWaitMessage, isNothingOnceTheWaitIsOver)
{
    EXPECT_FALSE(transferQuotaWaitMessage(0).has_value());
    EXPECT_FALSE(transferQuotaWaitMessage(-30).has_value());
}

TEST_F(TransferQuotaWait, greetingShowsTheWaitWhileADownloadWaits)
{
    EXPECT_FALSE(mListener.transferQuotaGreeting().has_value());

    // 630 seconds reads as 10 minutes even if a second passes before the greeting
    transferQuotaRunsOut(MegaTransfer::TYPE_DOWNLOAD, 7, 630);

    auto greeting = mListener.transferQuotaGreeting();
    ASSERT_TRUE(greeting.has_value());
    EXPECT_THAT(*greeting, HasSubstr("wait for 10 minutes"));
}

TEST_F(TransferQuotaWait, greetingStaysUntilTheLastWaitingDownloadMovesAgain)
{
    transferQuotaRunsOut(MegaTransfer::TYPE_DOWNLOAD, 7, 630);
    transferQuotaRunsOut(MegaTransfer::TYPE_DOWNLOAD, 8, 630);

    downloadMovesAgain(7);
    EXPECT_TRUE(mListener.transferQuotaGreeting().has_value());

    downloadMovesAgain(8);
    EXPECT_FALSE(mListener.transferQuotaGreeting().has_value());
}

TEST_F(TransferQuotaWait, logoutForgetsTheWaitingDownloads)
{
    transferQuotaRunsOut(MegaTransfer::TYPE_DOWNLOAD, 7, 630);

    mListener.forgetWaitingTransfers();

    EXPECT_FALSE(mListener.transferQuotaGreeting().has_value());
}

TEST_F(TransferQuotaWait, onlyDownloadsWaitForTransferQuota)
{
    transferQuotaRunsOut(MegaTransfer::TYPE_UPLOAD, 9, 630);

    EXPECT_FALSE(mListener.transferQuotaGreeting().has_value());
}

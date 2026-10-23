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

#include <chrono>
#include <thread>

#include "comunicationsmanager.h"
#include "listeners.h"
#include "megacmdsandbox.h"

using mega::MegaError;
using mega::MegaErrorPrivate;
using mega::MegaEvent;
using mega::MegaEventPrivate;
using mega::MegaTransfer;
using megacmd::ComunicationsManager;
using megacmd::MegaCmdGlobalListener;
using megacmd::MegaCmdGlobalTransferListener;
using megacmd::MegaCmdSandbox;
using megacmd::transferQuotaWaitMessage;
using megacmd::transferQuotaWaitNotice;
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

// The notice is shared by the whole server, so every test starts and ends with no wait known.
class TransferQuotaWait : public testing::Test
{
    NoClientsManager mManager;

protected:
    MegaCmdSandbox mSandbox;
    MegaCmdGlobalTransferListener mTransferListener{nullptr, &mSandbox};
    MegaCmdGlobalListener mGlobalListener{nullptr, &mSandbox};

    void SetUp() override
    {
        megacmd::cm = &mManager;
        transferQuotaWaitNotice().forget();
    }

    void TearDown() override
    {
        transferQuotaWaitNotice().forget();
        megacmd::cm = nullptr;
    }

    void transferQuotaRunsOut(int type, long long seconds)
    {
        FakeTransfer transfer(type, 7, MegaTransfer::STATE_RETRYING);
        MegaErrorPrivate overquota(MegaError::API_EOVERQUOTA, seconds);
        mTransferListener.onTransferTemporaryError(nullptr, &transfer, &overquota);
    }

    void streamingReadRunsOut(long long seconds)
    {
        MegaEventPrivate event(MegaEvent::EVENT_STREAM_OVERQUOTA);
        event.setNumber(seconds);
        mGlobalListener.onEvent(nullptr, &event);
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

TEST_F(TransferQuotaWait, announcesAWaitOnce)
{
    EXPECT_TRUE(transferQuotaWaitNotice().onWait(630));
    EXPECT_FALSE(transferQuotaWaitNotice().onWait(630));
}

TEST_F(TransferQuotaWait, announcesAgainWhenTheQuotaRunsOutAfterTheWait)
{
    EXPECT_TRUE(transferQuotaWaitNotice().onWait(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(2100));
    EXPECT_TRUE(transferQuotaWaitNotice().onWait(1));
}

TEST_F(TransferQuotaWait, aDownloadThatRunsOutAnnouncesTheWait)
{
    transferQuotaRunsOut(MegaTransfer::TYPE_DOWNLOAD, 630);

    EXPECT_FALSE(transferQuotaWaitNotice().onWait(630));
}

TEST_F(TransferQuotaWait, aStreamingReadThatRunsOutAnnouncesTheWait)
{
    streamingReadRunsOut(630);

    EXPECT_FALSE(transferQuotaWaitNotice().onWait(630));
}

TEST_F(TransferQuotaWait, onlyDownloadsAnnounceATransferQuotaWait)
{
    transferQuotaRunsOut(MegaTransfer::TYPE_UPLOAD, 630);

    EXPECT_TRUE(transferQuotaWaitNotice().onWait(630));
}

TEST_F(TransferQuotaWait, logoutForgetsTheWait)
{
    EXPECT_TRUE(transferQuotaWaitNotice().onWait(630));

    transferQuotaWaitNotice().forget();

    EXPECT_TRUE(transferQuotaWaitNotice().onWait(630));
}

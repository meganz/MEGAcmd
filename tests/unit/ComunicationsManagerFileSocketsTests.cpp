/**
 * (c) 2025 by Mega Limited, Auckland, New Zealand
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

#ifndef _WIN32

#include <gtest/gtest.h>

#include <atomic>
#include <csignal>
#include <cstring>
#include <future>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "Instruments.h"
#include "comunicationsmanagerfilesockets.h"
#include "megacmdcommonutils.h"

using namespace megacmd;

class TestSocketClient
{
private:
    int mSocket;
    bool mConnected;

public:
    TestSocketClient() : mSocket(-1), mConnected(false)
    {
        mSocket = socket(AF_UNIX, SOCK_STREAM, 0);
        if (mSocket >= 0)
        {
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;

            auto socketPath = getOrCreateSocketPath(false);
            strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

            if (connect(mSocket, (struct sockaddr*)&addr, sizeof(addr)) == 0)
            {
                mConnected = true;
            }
        }
    }

    ~TestSocketClient()
    {
        if (mSocket >= 0)
        {
            close(mSocket);
        }
    }

    bool isConnected() const
    {
        return mConnected;
    }

    int getSocket() const
    {
        return mSocket;
    }

    bool sendData(const std::string& data)
    {
        if (!isConnected())
        {
            return false;
        }
        return send(mSocket, data.c_str(), data.size(), 0) >= 0;
    }

    bool sendRaw(const void* data, size_t size)
    {
        if (!isConnected())
        {
            return false;
        }
        return send(mSocket, data, size, 0) >= 0;
    }

    ssize_t receive(void* buffer, size_t size, int flags = 0)
    {
        if (!isConnected())
        {
            return -1;
        }
        return recv(mSocket, buffer, size, flags);
    }

    void closeConnection()
    {
        if (mSocket >= 0)
        {
            close(mSocket);
            mSocket = -1;
            mConnected = false;
        }
    }

    TestSocketClient(const TestSocketClient&) = delete;
    TestSocketClient& operator=(const TestSocketClient&) = delete;
};

std::unique_ptr<CmdPetition> getPetitionAfterSend(ComunicationsManagerFileSockets &manager,
                                                    TestSocketClient &client,
                                                    const std::string &data)
{
    std::promise<void> waitStarted;
    auto waitStartedFuture = waitStarted.get_future();
    std::thread waitThread([&manager, &waitStarted]() {
        waitStarted.set_value();
        manager.waitForPetition();
    });

    // Wait for waitForPetition to start
    waitStartedFuture.wait();

    if (!client.sendData(data))
    {
        waitThread.join();
        return nullptr;
    }

    // Poll for petition availability instead of fixed sleep
    constexpr int maxAttempts = 10;
    for (int i = 0; i < maxAttempts; i++)
    {
        if (manager.receivedPetition())
        {
            auto petition = manager.getPetition();
            waitThread.join();
            return petition;
        }
        usleep(10000);
    }

    waitThread.join();
    return nullptr;
}

#ifdef __MACH__
// On macOS, MSG_NOSIGNAL doesn't work, so we need to ignore SIGPIPE
// to prevent the process from being killed when writing to closed socket
struct sigaction ignoreSigpipeAndGetOldHandler()
{
    struct sigaction sa, oldSa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGPIPE, &sa, &oldSa);
    return oldSa;
}

void restoreSigpipeHandler(const struct sigaction& oldSa)
{
    sigaction(SIGPIPE, &oldSa, nullptr);
}
#endif

class ComunicationsManagerFileSocketsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        TestInstruments::Instance().clearAll();
    }

    void TearDown() override
    {
        TestInstruments::Instance().clearAll();
    }
};

class ComunicationsManagerFileSocketsWithClientTest : public ComunicationsManagerFileSocketsTest
{
protected:
    ComunicationsManagerFileSockets manager;
    std::unique_ptr<TestSocketClient> client;

    void SetUp() override
    {
        ComunicationsManagerFileSocketsTest::SetUp();
        int result = manager.initialize();
        ASSERT_EQ(result, 0);

        client = std::make_unique<TestSocketClient>();
        ASSERT_TRUE(client->isConnected());
    }
};

TEST_F(ComunicationsManagerFileSocketsTest, InitializeSuccess)
{
    ComunicationsManagerFileSockets manager;
    int result = manager.initialize();
    EXPECT_EQ(result, 0);
}

TEST_F(ComunicationsManagerFileSocketsTest, InitializeWithLongSocketPath)
{
    std::string longName(200, 'a');
    auto guard = TestInstrumentsEnvVarGuard("MEGACMD_SOCKET_NAME", longName);

    ComunicationsManagerFileSockets manager;
    int result = manager.initialize();
    EXPECT_TRUE(result == 0 || result == -1);
}

TEST_F(ComunicationsManagerFileSocketsWithClientTest, GetPetitionReadsData)
{
    std::string testData = "test command";
    auto petition = getPetitionAfterSend(manager, *client, testData);

    ASSERT_NE(petition, nullptr);
    EXPECT_EQ(petition->getLine(), testData);
}

TEST_F(ComunicationsManagerFileSocketsWithClientTest, GetPetitionReadsLargeData)
{
    std::string largeData(2048, 'A');
    auto petition = getPetitionAfterSend(manager, *client, largeData);

    ASSERT_NE(petition, nullptr);
    EXPECT_EQ(petition->getLine(), largeData);
}

TEST_F(ComunicationsManagerFileSocketsWithClientTest, ReturnAndClosePetitionSendsData)
{
    auto petition = getPetitionAfterSend(manager, *client, "test");
    ASSERT_NE(petition, nullptr);

    OUTSTRINGSTREAM response;
    response << "response data";
    manager.returnAndClosePetition(std::move(petition), &response, 0);

    int outCode;
    ssize_t n = client->receive(&outCode, sizeof(outCode));
    ASSERT_GT(n, 0);
    EXPECT_EQ(outCode, 0);

    char buffer[1024];
    n = client->receive(buffer, sizeof(buffer) - 1);
    ASSERT_GT(n, 0);
    buffer[n] = '\0';
    EXPECT_STREQ(buffer, "response data");
}

TEST_F(ComunicationsManagerFileSocketsWithClientTest, SendPartialOutput)
{
    auto petition = getPetitionAfterSend(manager, *client, "test");
    ASSERT_NE(petition, nullptr);

    std::string partialData = "partial output";
    manager.sendPartialOutput(petition.get(), partialData.data(), partialData.size(), false);

    int outCode;
    client->receive(&outCode, sizeof(outCode));
    EXPECT_EQ(outCode, MCMD_PARTIALOUT);

    size_t size;
    client->receive(&size, sizeof(size));
    EXPECT_EQ(size, partialData.size());

    char buffer[1024];
    ssize_t n = client->receive(buffer, size);
    ASSERT_GT(n, 0);
    buffer[size] = '\0';
    EXPECT_STREQ(buffer, partialData.c_str());
}

TEST_F(ComunicationsManagerFileSocketsWithClientTest, SendPartialError)
{
    auto petition = getPetitionAfterSend(manager, *client, "test");
    ASSERT_NE(petition, nullptr);

    std::string errorData = "error message";
    manager.sendPartialError(petition.get(), errorData.data(), errorData.size(), false);

    int outCode;
    client->receive(&outCode, sizeof(outCode));
    EXPECT_EQ(outCode, MCMD_PARTIALERR);
}

TEST_F(ComunicationsManagerFileSocketsWithClientTest, SendPartialOutputHandlesEPIPE)
{
    auto petition = getPetitionAfterSend(manager, *client, "test");
    ASSERT_NE(petition, nullptr);

    client->closeConnection();

#ifdef __MACH__
    auto oldSa = ignoreSigpipeAndGetOldHandler();
#endif

    std::string data = "test data";
    manager.sendPartialOutput(petition.get(), data.data(), data.size(), false);

    EXPECT_TRUE(petition->clientDisconnected);

#ifdef __MACH__
    restoreSigpipeHandler(oldSa);
#endif
}

TEST_F(ComunicationsManagerFileSocketsWithClientTest, SendPartialOutputValidatesUTF8)
{
    auto petition = getPetitionAfterSend(manager, *client, "test");
    ASSERT_NE(petition, nullptr);

    char invalidUtf8[] = "\xC0\x80";
    manager.sendPartialOutput(petition.get(), invalidUtf8, 2, false);
}

TEST_F(ComunicationsManagerFileSocketsWithClientTest, RegisterStateListener)
{
    auto petition = getPetitionAfterSend(manager, *client, "test");
    ASSERT_NE(petition, nullptr);

    auto listener = manager.registerStateListener(std::move(petition));
    EXPECT_NE(listener, nullptr);
}

TEST_F(ComunicationsManagerFileSocketsTest, GetMaxStateListeners)
{
    ComunicationsManagerFileSockets manager;
    int maxListeners = manager.getMaxStateListeners();
    EXPECT_GT(maxListeners, 0);
    EXPECT_LE(maxListeners, FD_SETSIZE);
}

TEST_F(ComunicationsManagerFileSocketsWithClientTest, InformStateListener)
{
    auto petition = getPetitionAfterSend(manager, *client, "test");
    ASSERT_NE(petition, nullptr);

    auto listener = manager.registerStateListener(std::move(petition));
    ASSERT_NE(listener, nullptr);

    std::string message = "state update";
    int result = manager.informStateListener(listener, message);

    EXPECT_EQ(result, 0);

    char buffer[1024];
    ssize_t n = client->receive(buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
    EXPECT_GT(n, 0);
    buffer[n] = '\0';
    EXPECT_STREQ(buffer, message.c_str());
}

TEST_F(ComunicationsManagerFileSocketsWithClientTest, InformStateListenerHandlesEPIPE)
{
    auto petition = getPetitionAfterSend(manager, *client, "test");
    ASSERT_NE(petition, nullptr);

    auto listener = manager.registerStateListener(std::move(petition));

    client->closeConnection();

#ifdef __MACH__
    auto oldSa = ignoreSigpipeAndGetOldHandler();
#endif

    std::string message = "state update";
    int result = manager.informStateListener(listener, message);

    EXPECT_EQ(result, -1);

#ifdef __MACH__
    restoreSigpipeHandler(oldSa);
#endif
}

TEST_F(ComunicationsManagerFileSocketsWithClientTest, InformStateListenerValidatesUTF8)
{
    auto petition = getPetitionAfterSend(manager, *client, "test");
    ASSERT_NE(petition, nullptr);

    auto listener = manager.registerStateListener(std::move(petition));

    std::string invalidUtf8 = "\xC0\x80";
    int result = manager.informStateListener(listener, invalidUtf8);
}

TEST_F(ComunicationsManagerFileSocketsWithClientTest, GetConfirmation)
{
    auto petition = getPetitionAfterSend(manager, *client, "test");
    ASSERT_NE(petition, nullptr);

    std::thread responseThread([this]() {
        int outCode;
        client->receive(&outCode, sizeof(outCode));
        EXPECT_EQ(outCode, MCMD_REQCONFIRM);

        char buffer[1024];
        client->receive(buffer, sizeof(buffer) - 1);

        int response = 1;
        client->sendRaw(&response, sizeof(response));
    });

    std::string message = "Confirm action?";
    int result = manager.getConfirmation(petition.get(), message);

    responseThread.join();

    EXPECT_EQ(result, 1);
}

TEST_F(ComunicationsManagerFileSocketsWithClientTest, GetUserResponse)
{
    auto petition = getPetitionAfterSend(manager, *client, "test");
    ASSERT_NE(petition, nullptr);

    std::thread responseThread([this]() {
        int outCode;
        client->receive(&outCode, sizeof(outCode));
        EXPECT_EQ(outCode, MCMD_REQSTRING);

        char buffer[1024];
        client->receive(buffer, sizeof(buffer) - 1);

        std::string response = "user input";
        client->sendData(response);
    });

    std::string message = "Enter value:";
    std::string result = manager.getUserResponse(petition.get(), message);

    responseThread.join();

    EXPECT_EQ(result, "user input");
}

TEST_F(ComunicationsManagerFileSocketsWithClientTest, GetUserResponseLargeData)
{
    auto petition = getPetitionAfterSend(manager, *client, "test");
    ASSERT_NE(petition, nullptr);

    std::thread responseThread([this]() {
        int outCode;
        client->receive(&outCode, sizeof(outCode));
        EXPECT_EQ(outCode, MCMD_REQSTRING);

        char buffer[1024];
        client->receive(buffer, sizeof(buffer) - 1);

        std::string largeResponse(3000, 'A');
        client->sendData(largeResponse);
    });

    std::string message = "Enter value:";
    std::string result = manager.getUserResponse(petition.get(), message);

    responseThread.join();

    EXPECT_EQ(result, std::string(3000, 'A'));
}

TEST_F(ComunicationsManagerFileSocketsTest, StopWaiting)
{
    ComunicationsManagerFileSockets manager;
    manager.initialize();
    manager.stopWaiting();
}

TEST_F(ComunicationsManagerFileSocketsTest, StressMultipleConcurrentClients)
{
    ComunicationsManagerFileSockets manager;
    manager.initialize();

    constexpr int numClients = 20;
    std::vector<std::unique_ptr<TestSocketClient>> clients;
    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);

    for (int i = 0; i < numClients; i++)
    {
        clients.push_back(std::make_unique<TestSocketClient>());
    }

    std::thread waitThread([&manager]() {
        for (int i = 0; i < 20; i++)
        {
            manager.waitForPetition();
            usleep(10000);
        }
    });
    waitThread.detach();

    for (int i = 0; i < numClients; i++)
    {
        threads.emplace_back([&manager, &clients, i, &successCount]() {
            auto& client = clients[i];
            if (!client->isConnected())
            {
                return;
            }

            std::string testData = "test command " + std::to_string(i);
            client->sendData(testData);
        });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    constexpr int maxAttempts = 50;
    for (int i = 0; i < maxAttempts; i++)
    {
        if (manager.receivedPetition())
        {
            auto petition = manager.getPetition();
            if (petition != nullptr && !petition->getLine().empty())
            {
                successCount++;
            }
        }
        if (successCount.load() >= numClients)
        {
            break;
        }
        usleep(10000);
    }

    EXPECT_GT(successCount.load(), 0);
}

TEST_F(ComunicationsManagerFileSocketsTest, StressMaxStateListeners)
{
    ComunicationsManagerFileSockets manager;
    manager.initialize();

    int maxListeners = manager.getMaxStateListeners();
    EXPECT_GT(maxListeners, 0);

    constexpr int testListeners = 10;
    std::vector<std::unique_ptr<TestSocketClient>> clients;
    std::vector<CmdPetition*> listeners;
    int registeredCount = 0;

    std::thread waitThread([&manager]() {
        for (int i = 0; i < testListeners; i++)
        {
            manager.waitForPetition();
            usleep(5000);
        }
    });
    waitThread.detach();

    for (int i = 0; i < testListeners; i++)
    {
        clients.push_back(std::make_unique<TestSocketClient>());
        if (clients.back()->isConnected())
        {
            clients.back()->sendData("test");
        }
    }

    // Poll for petitions with timeout
    constexpr int maxAttempts = 50;
    for (int attempt = 0; attempt < maxAttempts; attempt++)
    {
        if (manager.receivedPetition())
        {
            auto petition = manager.getPetition();
            if (petition != nullptr)
            {
                auto listener = manager.registerStateListener(std::move(petition));
                if (listener != nullptr)
                {
                    listeners.push_back(listener);
                    registeredCount++;
                }
            }
        }
        if (registeredCount >= testListeners)
        {
            break;
        }
        usleep(10000);
    }

    EXPECT_LE(registeredCount, maxListeners);
    EXPECT_GT(registeredCount, 0);
}

TEST_F(ComunicationsManagerFileSocketsTest, StressConcurrentPartialOutputs)
{
    ComunicationsManagerFileSockets manager;
    manager.initialize();

    constexpr int numClients = 20;
    std::vector<std::unique_ptr<TestSocketClient>> clients;
    std::vector<std::unique_ptr<CmdPetition>> petitions;

    for (int i = 0; i < numClients; i++)
    {
        clients.push_back(std::make_unique<TestSocketClient>());
    }

    std::thread waitThread([&manager]() {
        for (int i = 0; i < numClients; i++)
        {
            manager.waitForPetition();
            usleep(10000);
        }
    });
    waitThread.detach();

    for (int i = 0; i < numClients; i++)
    {
        if (clients[i]->isConnected())
        {
            clients[i]->sendData("test");
        }
    }

    // Poll for petitions with timeout
    constexpr int maxAttempts = 50;
    for (int attempt = 0; attempt < maxAttempts; attempt++)
    {
        if (manager.receivedPetition())
        {
          if (auto petition = manager.getPetition(); petition != nullptr)
          {
              petitions.push_back(std::move(petition));
          }
        }
        if (static_cast<int>(petitions.size()) >= numClients)
        {
            break;
        }
        usleep(10000);
    }

    EXPECT_GT(petitions.size(), 0);

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    for (size_t i = 0; i < petitions.size(); i++)
    {
        threads.emplace_back([&manager, &petitions, i, &successCount]() {
            std::string data = "partial output " + std::to_string(i);
            manager.sendPartialOutput(petitions[i].get(), data.data(), data.size(), false);
            successCount++;
        });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(successCount.load(), static_cast<int>(petitions.size()));
}

TEST_F(ComunicationsManagerFileSocketsTest, StressRapidConnectDisconnect)
{
    ComunicationsManagerFileSockets manager;
    manager.initialize();

    std::thread waitThread([&manager]() {
        for (int i = 0; i < 30; i++)
        {
            manager.waitForPetition();
            usleep(5000);
        }
    });
    waitThread.detach();

    // Small delay to ensure waitThread is running
    usleep(10000);

    for (int i = 0; i < 30; i++)
    {
        TestSocketClient client;
        if (client.isConnected())
        {
            client.sendData("test " + std::to_string(i));
            client.closeConnection();
        }
        usleep(2000);
    }

    // Poll for petitions with timeout
    int receivedCount = 0;
    constexpr int maxAttempts = 50;
    for (int attempt = 0; attempt < maxAttempts; ++attempt)
    {
        if (manager.receivedPetition())
        {
          if (auto petition = manager.getPetition(); petition != nullptr)
          {
            receivedCount++;
          }
        }
        if (receivedCount >= 30)
        {
            break;
        }
        usleep(10000);
    }

    EXPECT_GT(receivedCount, 0);
}

TEST_F(ComunicationsManagerFileSocketsTest, StressLargeDataMultipleClients)
{
    ComunicationsManagerFileSockets manager;
    manager.initialize();

    constexpr int numClients = 10;
    std::vector<std::unique_ptr<TestSocketClient>> clients;

    for (int i = 0; i < numClients; i++)
    {
        clients.push_back(std::make_unique<TestSocketClient>());
    }

    std::thread waitThread([&manager]() {
        for (int i = 0; i < numClients; i++)
        {
            manager.waitForPetition();
            usleep(10000);
        }
    });
    waitThread.detach();

    for (int i = 0; i < numClients; i++)
    {
        if (clients[i]->isConnected())
        {
            std::string largeData(5000, static_cast<char>('A' + i));
            clients[i]->sendData(largeData);
        }
    }

    // Poll for petitions with timeout (larger timeout for large data)
    int successCount = 0;
    constexpr int maxAttempts = 150;
    for (int attempt = 0; attempt < maxAttempts; attempt++)
    {
        if (manager.receivedPetition())
        {
            auto petition = manager.getPetition();
            if (petition != nullptr)
            {
                // Try to match with expected data
                for (int i = 0; i < numClients; i++)
                {
                    std::string expected(5000, static_cast<char>('A' + i));
                    if (petition->getLine() == expected)
                    {
                        successCount++;
                        break;
                    }
                }
            }
        }
        if (successCount >= numClients)
        {
            break;
        }
        usleep(10000);
    }

    EXPECT_GT(successCount, 0);
}

#endif // _WIN32

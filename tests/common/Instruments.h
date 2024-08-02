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

//#include "EventsDefinitions.h"
//TODO: Have the above use MEGAcmd definitions!

#include <cstdlib>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <variant>
#include <future>
#include <variant>
#include <unordered_map>
#include <optional>
#include <map>
#include <gtest/gtest.h>

using MegaCmdEvent = int; //TODO: instead have MEGAcmd events used

struct InstrumentsException : public std::exception
{
    virtual ~InstrumentsException() = default;
};

class TestInstruments
{
public:
    static TestInstruments& Instance();

    void clearAll();

    //
    // Flags
    //
    enum class Flag
    {
        SOME_MEGACMD_INSTRUMENT_FLAG,
    };

    bool flag(Flag flag);
    void setFlag(Flag flag);
    void resetFlag(Flag flag);
    void clearFlags();

    void throwIfFlagAndReset(Flag flag);

    //
    // Events
    //
    enum class Event
    {
        SERVER_ABOUT_TO_START_WAITING_FOR_PETITIONS,
        STALLED_ISSUES_LIST_UPDATED,
    };

    typedef std::function<void()> EventCallback;
    void onEventOnce(Event event, EventCallback&&);
    void onEventOnce(Event event, EventCallback&);
    void clearEvent(Event event);
    void clearEvent(MegaCmdEvent event);
    void fireEvent(Event event);
    void fireEvent(MegaCmdEvent event);
    void clearEvents();

    //
    // Test Values
    //
    enum class TestValue
    {
        AMIPRO_LEVEL,
        STALLED_ISSUES_LIST_SIZE
    };

    using TestValue_t = std::variant<
        uint64_t,
        int64_t,
        std::string,
        std::chrono::steady_clock::time_point>;

    std::optional<TestValue_t> testValue(TestValue key);
    void setTestValue(TestValue key, TestValue_t value);
    void resetTestValue(TestValue key);
    void increaseTestValue(TestValue key);
    void clearTestValues();

    //
    // Probes
    //
    bool probe(const std::string &);
    template <typename T>
    void setProbe(T&& str)
    {
        std::lock_guard<std::recursive_mutex> lock(mMutex);
        mProbes.insert(std::forward<T>(str));
    }
    size_t probeCount() const
    {
        return mProbes.size();
    }
    void clearProbes();

private:
    std::recursive_mutex mMutex;
    std::unordered_set<Flag> mFlags;
    std::unordered_set<std::string> mProbes;

    // Only triggered once, only one allowed
    std::unordered_map<Event, EventCallback> mSingleEventHandlers;
    std::unordered_map<MegaCmdEvent, EventCallback> mMegaCmdSingleEventHandlers;

    // Triggering does not imply execution
    std::multimap<Event, EventCallback> mEventMultiHandlers;
    std::multimap<MegaCmdEvent, EventCallback> mMegaCmdEventMultiHandlers;

    std::unordered_map<TestValue, TestValue_t> mTestValues;

public:

    template <typename Cb>
    void onEventOnce(MegaCmdEvent event, Cb &&handler)
    {
        std::lock_guard<std::recursive_mutex> lock(mMutex);
        mMegaCmdSingleEventHandlers.insert_or_assign(event, std::forward<Cb>(handler));
    }

    // Caveat, these occur with the mutex locked while looping the array, try not to include TI calls within callbacks!
    template <typename Cb>
    std::multimap<Event, EventCallback>::iterator
       onEveryEvent(Event event, Cb &&handler)
    {
        std::lock_guard<std::recursive_mutex> lock(mMutex);
        return mEventMultiHandlers.emplace(event, std::forward<Cb>(handler));
    }

    template <typename Cb>
    std::multimap<MegaCmdEvent, EventCallback>::iterator
       onEveryEvent(MegaCmdEvent event, Cb &&handler)
    {
        std::lock_guard<std::recursive_mutex> lock(mMutex);
        return mMegaCmdEventMultiHandlers.emplace(event, std::forward<Cb>(handler));
    }

    void removeEventMultiHandle(const std::multimap<Event, EventCallback>::iterator &it)
    {
        std::lock_guard<std::recursive_mutex> lock(mMutex);
        mEventMultiHandlers.erase(it);
    }

    void removeEventMultiHandle(const std::multimap<MegaCmdEvent, EventCallback>::iterator &it)
    {
        std::lock_guard<std::recursive_mutex> lock(mMutex);
        mMegaCmdEventMultiHandlers.erase(it);
    }

    /**
     * @brief general purpose scope guard class
     */
    template <typename ExitCallback>
    class ScopeGuard
    {
        ExitCallback mExitCb;
    public:
        ScopeGuard(ExitCallback&& exitCb) : mExitCb{std::move(exitCb)} { }
        ~ScopeGuard() { mExitCb(); }
    };

    template <typename Cb>
    auto onEveryEventGuard(Event event, Cb &&handler)
    {
        auto it = onEveryEvent(event, std::forward<Cb>(handler));

        return ScopeGuard([this, it{std::move(it)}](){
            removeEventMultiHandle(it);
        });
    }

};

class TestInstrumentsFlagGuard
{
    const TestInstruments::Flag mFlag;
public:
    TestInstrumentsFlagGuard(TestInstruments::Flag flag)
        : mFlag(flag)
    {
        TestInstruments::Instance().setFlag(mFlag);
    }
    ~TestInstrumentsFlagGuard()
    {
        TestInstruments::Instance().resetFlag(mFlag);
    }
};

class TestInstrumentsWaitForEventGuard
{
    bool mEventHappened;
    bool mDoNotWait = false;
    std::mutex mEventHappenedMutex;
    std::condition_variable mEventHappenedCv;
public:
    TestInstrumentsWaitForEventGuard(TestInstruments::Event event)
        : mEventHappened(false)
    {
        TestInstruments::Instance().onEventOnce(event,
        [this]() {
            std::lock_guard<std::mutex> guard(mEventHappenedMutex);
            mEventHappened = true;
            mEventHappenedCv.notify_all();
        });
    }

    bool disableWaitAndGetValue()
    {
        std::unique_lock<std::mutex> lock(mEventHappenedMutex);
        mDoNotWait = true;
        return mEventHappened;
    }

    bool waitForEvent(std::chrono::duration<int64_t> timeout = std::chrono::seconds(30))
    {
        std::unique_lock<std::mutex> lock(mEventHappenedMutex);
        if (mDoNotWait)
        {
            return mEventHappened;
        }
        mDoNotWait = true;
        mEventHappenedCv.wait_for(lock, timeout, [this](){ return mEventHappened; });
        EXPECT_TRUE(mEventHappened);
        return mEventHappened;
    }

    ~TestInstrumentsWaitForEventGuard()
    {
        waitForEvent();
    }
};

struct TestInstrumentsTestValueGuard
{
    TestInstruments::TestValue mTestValueKey;
    TestInstrumentsTestValueGuard(TestInstruments::TestValue key)
    {
        mTestValueKey = key;
    }
    TestInstrumentsTestValueGuard(TestInstruments::TestValue key, TestInstruments::TestValue_t value)
    {
        mTestValueKey = key;
        TestInstruments::Instance().setTestValue(key, value);
    }
    virtual ~TestInstrumentsTestValueGuard()
    {
        TestInstruments::Instance().resetTestValue(mTestValueKey);
    }
};

class TestInstrumentsClearGuard
{
public:
    ~TestInstrumentsClearGuard()
    {
        TestInstruments::Instance().clearAll();
    }
};

class TestInstrumentsProbesGuard
{
    const std::vector<std::string> mProbes;
public:
    TestInstrumentsProbesGuard(std::vector<std::string>&& probles)
        : mProbes(std::move(probles))
    { }
    ~TestInstrumentsProbesGuard()
    {
        TestInstruments::Instance().clearProbes();
    }
    bool probe() const
    {
        for (const auto& probe : mProbes)
        {
            if (!TestInstruments::Instance().probe(probe))
            {
                return false;
            }
        }
        return true;
    }
};

class TestInstrumentsEnvVarGuard
{
private:
    std::string mVar;
    bool mHasInitValue;
    std::string mInitValue;

public:
    TestInstrumentsEnvVarGuard(std::string variable, const std::string &value)
        : mVar(std::move(variable)), mHasInitValue(false), mInitValue()
    {
        const char *initValue = getenv(mVar.c_str());
        if (initValue != nullptr)
        {
            mHasInitValue = true;
            mInitValue = std::string(initValue);
        }
#ifdef _WIN32
        auto envStr = std::string(mVar).append("=").append(value);
        _putenv(envStr.c_str());
#else
        setenv(mVar.c_str(), value.c_str(), 1);
#endif
    }
    virtual ~TestInstrumentsEnvVarGuard()
    {
        if (mHasInitValue)
        {
#ifdef _WIN32
            auto envStr = std::string(mVar).append("=").append(mInitValue);
            _putenv(envStr.c_str());
#else
            setenv(mVar.c_str(), mInitValue.c_str(), 1);
#endif
        }
        else
        {
#ifdef _WIN32
            auto envStr = std::string(mVar).append("=");
            _putenv(envStr.c_str());
#else
            unsetenv(mVar.c_str());
#endif
        }
    }

protected:
    explicit TestInstrumentsEnvVarGuard(std::string variable)
        : mVar(std::move(variable)), mHasInitValue(false), mInitValue()
    {
        const char *initValue = getenv(mVar.c_str());
        if (initValue != nullptr)
        {
            mHasInitValue = true;
            mInitValue = std::string(initValue);
        }
#ifdef _WIN32
        auto envStr = std::string(mVar).append("=");
        _putenv(envStr.c_str());
#else
        unsetenv(mVar.c_str());
#endif
    }
};

class TestInstrumentsUnsetEnvVarGuard : TestInstrumentsEnvVarGuard
{
public:
    explicit TestInstrumentsUnsetEnvVarGuard(std::string variable)
        : TestInstrumentsEnvVarGuard(variable)
    {
    }
};

/**
 * Convenience class that tracks the occurrence of events within its lifetime.
 *
 * Caveat: onEventOnce only allows for a single handler,
 * this will install/replace event handlers for tracked events
 * These handlers will be removed after EventTracker dtor
 */
class EventTracker
{
    using TI = TestInstruments;

    std::mutex mMutex;
    std::condition_variable mCV;
    std::map<std::variant<MegaCmdEvent, TI::Event>, bool> mTrackedEvents;

    void initializeTracking()
    {
        for (auto &pTracked : mTrackedEvents)
        {
            std::visit([this, &pTracked](const auto & event){
                TI::Instance().onEventOnce(event, [this, &pTracked](){
                    pTracked.second = true;
                    mCV.notify_all();
                });
            }, pTracked.first);
        }
    }

public:
    EventTracker(const std::vector<std::variant<MegaCmdEvent, TI::Event>> &events)
    {
        for (auto &event : events)
        {
            mTrackedEvents[event] = false;
        }
        initializeTracking();
    }

    ~EventTracker()
    {
        for (auto &pTracked : mTrackedEvents)
        {
            std::visit([](const auto & event){
                TI::Instance().clearEvent(event);
            }, pTracked.first);
        }
    }

    bool eventHappened(const std::variant<MegaCmdEvent, TI::Event> &event)
    {
        std::unique_lock lock(mMutex);
        assert(mTrackedEvents.find(event) != mTrackedEvents.end());
        return mTrackedEvents[event];
    }

    template <typename T = std::ratio<1>>
    bool waitForEvent(const std::variant<MegaCmdEvent, TI::Event> &event, std::chrono::duration<int64_t, T> timeout = std::chrono::seconds(30))
    {
        std::unique_lock lock(mMutex);
        mCV.wait_for(lock, timeout, [this, &event](){ return mTrackedEvents[event]; });
        return mTrackedEvents[event];
    }
};

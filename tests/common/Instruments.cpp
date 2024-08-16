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

#include "Instruments.h"

TestInstruments& TestInstruments::Instance()
{
    static TestInstruments instance;
    return instance;
}

void TestInstruments::clearAll()
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mFlags.clear();
    clearEvents();
    mProbes.clear();
}

bool TestInstruments::flag(Flag flag)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mFlags.find(flag) != mFlags.end();
}

void TestInstruments::setFlag(Flag flag)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mFlags.insert(flag);
}

void TestInstruments::resetFlag(Flag flag)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mFlags.erase(flag);
}

void TestInstruments::clearFlags()
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mFlags.clear();
}

void TestInstruments::throwIfFlagAndReset(TestInstruments::Flag flag)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (mFlags.find(flag) != mFlags.end())
    {
        mFlags.erase(flag);
        throw InstrumentsException();
    }
}

void TestInstruments::onEventOnce(Event event, EventCallback&& handler)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mSingleEventHandlers.insert_or_assign(event, std::move(handler));
}

void TestInstruments::onEventOnce(Event event, EventCallback& handler)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mSingleEventHandlers.insert_or_assign(event, handler);
}

void TestInstruments::clearEvent(Event event)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mSingleEventHandlers.erase(event);
    mEventMultiHandlers.erase(event);
}

void TestInstruments::clearEvent(MegaCmdEvent event)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mMegaCmdSingleEventHandlers.erase(event);
    mMegaCmdEventMultiHandlers.erase(event);
}

void TestInstruments::fireEvent(Event event)
{
    std::vector<EventCallback> handlersToExecute;
    {
        std::lock_guard<std::recursive_mutex> lock(mMutex);
        auto handlerIt = mSingleEventHandlers.find(event);
        if (handlerIt != mSingleEventHandlers.end())
        {
            handlersToExecute.push_back(std::move(handlerIt->second));
            mSingleEventHandlers.erase(handlerIt);
        }

        auto range = mEventMultiHandlers.equal_range(event);
        for (auto handlerIt = range.first; handlerIt != range.second; handlerIt++)
        {
            handlerIt->second();
        }
    }
    for (auto &handler : handlersToExecute)
    {
        handler();
    }
}

void TestInstruments::fireEvent(MegaCmdEvent event)
{
    std::vector<EventCallback> handlersToExecute;
    {
        std::lock_guard<std::recursive_mutex> lock(mMutex);
        auto handlerIt = mMegaCmdSingleEventHandlers.find(event);
        if (handlerIt != mMegaCmdSingleEventHandlers.end())
        {
            handlersToExecute.push_back(std::move(handlerIt->second));
            mMegaCmdSingleEventHandlers.erase(handlerIt);
        }

        auto range = mMegaCmdEventMultiHandlers.equal_range(event);
        for (auto handlerIt = range.first; handlerIt != range.second; handlerIt++)
        {
            handlerIt->second();
        }
    }
    for (auto &handler : handlersToExecute)
    {
        handler();
    }
}

void TestInstruments::clearEvents()
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mSingleEventHandlers.clear();
    mMegaCmdSingleEventHandlers.clear();
    mEventMultiHandlers.clear();
    mMegaCmdEventMultiHandlers.clear();
}

std::optional<TestInstruments::TestValue_t> TestInstruments::testValue(TestValue key)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    auto it = mTestValues.find(key);
    if (it != mTestValues.end())
    {
        return it->second;
    }
    return std::nullopt;
}

void TestInstruments::setTestValue(TestValue key, TestValue_t value)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mTestValues.insert_or_assign(key, value);
}

void TestInstruments::resetTestValue(TestValue key)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mTestValues.erase(key);
}

void TestInstruments::increaseTestValue(TestValue key)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    auto it = mTestValues.find(key);
    if (it == mTestValues.end())
    {
        return;
    }
    mTestValues.insert_or_assign(key, ++std::get<uint64_t>(it->second));
}

void TestInstruments::clearTestValues()
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mTestValues.clear();
}

bool TestInstruments::probe(const std::string &str)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mProbes.find(str) != mProbes.end();
}

void TestInstruments::clearProbes()
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mProbes.clear();
}

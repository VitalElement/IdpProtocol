// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "TestRuntime.h"
#include "Application.h"
#include "Dispatcher.h"
#include "Trace.h"

uint64_t TestRuntime::s_systemTime = 0;
void* TestRuntime::s_internalData = nullptr;

uint64_t TestRuntime::GetSystemTime ()
{
    return s_systemTime;
}

void TestRuntime::IterateRuntime (uint32_t timeMs, int iterations)
{
    s_systemTime += timeMs;

    for (int i = 0; i < iterations; i++)
    {
        Dispatcher::GetMainDispatcher ().ProcessQueue ();
        Dispatcher::GetPriorityDispatcher ().ProcessQueue ();
    }
}

void TestRuntime::ResetTime ()
{
    s_systemTime = 0;
}

void TestRuntime::Initialise ()
{
    Trace::InitialiseDefaults ();
    Trace::InitialiseStdOut ();
    Trace::SetGetTimeStampAction (GetSystemTime);
    Trace::SetBuffer (new uint8_t[512], 512);
    Trace::ColorsEnabled (false);

    Trace::WriteLine ("");
    Trace::WriteLine ("");

    if (s_internalData != nullptr)
    {
        delete (DispatcherActions*) s_internalData;

        s_internalData = nullptr;
    }

    new TestApplication ();

    ResetTime ();
    Dispatcher::DeleteDispatchers ();
    DispatcherTimer::InitialiseTimers (TestRuntime::GetSystemTime);

    auto& actions = *new DispatcherActions ();
    actions.EnterCriticalSection = [] {};
    actions.ExitCriticalSection = [] {};
    actions.Yield = [] {};

    s_internalData = &actions;

    Dispatcher::CreateMainDispatcher (actions);
    Dispatcher::CreatePriorityDispatcher (actions);
}

uint64_t TestSystemTime::GetSystemTime ()
{
    return TestRuntime::GetSystemTime ();
}

TestApplication::TestApplication () : Application (*new TestSystemTime ())
{
}

DispatcherActions& TestApplication::GetDispatcherActions ()
{
    return *new DispatcherActions ();
}
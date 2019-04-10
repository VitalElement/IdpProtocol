// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.


#pragma once
#include "Application.h"
#include <stdint.h>

class TestSystemTime : public ISystemTime
{
    uint64_t GetSystemTime ();
};

class TestRuntime
{
  public:
    static uint64_t GetSystemTime ();

    static void IterateRuntime (uint32_t timeMs = 0, int iterations = 1);

    static void ResetTime ();

    static void Initialise ();

  private:
    static uint64_t s_systemTime;
    static void* s_internalData;
};

class TestApplication : public Application
{
  public:
    TestApplication ();

    virtual ~TestApplication ()
    {
    }
};
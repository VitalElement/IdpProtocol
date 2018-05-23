// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include "IAdaptor.h"
#include <memory>
#include <stdbool.h>
#include <stdint.h>

class SimpleAdaptor : public IAdaptor
{
  private:
    SimpleAdaptor* _remote;

  public:
    SimpleAdaptor ();

    virtual ~SimpleAdaptor ();

    void SetRemote (SimpleAdaptor& remote);

    bool Transmit (std::shared_ptr<IdpPacket> packet);

    void Receive (std::shared_ptr<IdpPacket> packet);

    const char* Name ()
    {
        return "Simple.Adaptor";
    }
};
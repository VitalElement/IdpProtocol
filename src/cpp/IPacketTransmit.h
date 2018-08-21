// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include "IdpNode.h"
#include "IdpPacket.h"
#include <memory>
#include <stdbool.h>
#include <stdint.h>

class IdpNode;

/**
 *  IPacketTransmit
 */
class IPacketTransmit
{
  public:
    virtual bool Transmit (std::shared_ptr<IdpPacket> packet) = 0;

    virtual ~IPacketTransmit ()
    {
    }
};

class IAdaptorToRouterPort
{
  public:
    virtual ~IAdaptorToRouterPort ()
    {
    }

    virtual bool Transmit (uint16_t adaptorId,
                           std::shared_ptr<IdpPacket> packet) = 0;
};

// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include "Event.h"
#include "IdpPacket.h"
#include <stdint.h>

/**
 *  DataReceivedEventArgs
 */
class DataReceivedEventArgs : public EventArgs
{
  public:
    /**
     * Instantiates a new instance of DataReceivedEventArgs
     */
    DataReceivedEventArgs (std::shared_ptr<IdpPacket> data);
    ~DataReceivedEventArgs ();

    std::shared_ptr<IdpPacket> Packet;
};

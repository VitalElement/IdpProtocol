// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "DataReceivedEventArgs.h"

DataReceivedEventArgs::DataReceivedEventArgs (std::shared_ptr<IdpPacket> packet)
{
    Packet = packet;
}

DataReceivedEventArgs::~DataReceivedEventArgs ()
{
}

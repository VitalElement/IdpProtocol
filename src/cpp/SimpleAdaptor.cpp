// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.

#include "SimpleAdaptor.h"

SimpleAdaptor::SimpleAdaptor ()
{
    _remote = nullptr;
}

SimpleAdaptor::~SimpleAdaptor ()
{
}

void SimpleAdaptor::SetRemote (SimpleAdaptor& remote)
{
    _remote = &remote;

    IsActive (true);
}

// IPacketTransmit;
bool SimpleAdaptor::Transmit (std::shared_ptr<IdpPacket> packet)
{
    if (_remote != nullptr)
    {
        _remote->OnReceive (packet);

        return true;
    }

    return false;
}

// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using System;

namespace IdpProtocol
{
    public interface IPacketTransmit
    {
        bool Transmit(IdpPacket packet);
    }

    public interface IAdaptorToRouterPort
    {
        bool Transmit(UInt16 adaptorId, IdpPacket packet);
    }
}

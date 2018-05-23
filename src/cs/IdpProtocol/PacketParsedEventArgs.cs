// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using System;

namespace IdpProtocol
{
    public class PacketParsedEventArgs : EventArgs
    {
        public PacketParsedEventArgs(IdpPacket packet)
        {
            Packet = packet;
        }

        public IdpPacket Packet { get; }
    }
}

using System;

namespace IdpProtocol
{
    public sealed class IdpTracePacket
    {
        public required DateTime TimestampUtc { get; init; }

        public required IdpTrafficDirection Direction { get; init; }

        public required UInt16 Source { get; init; }

        public required UInt16 Destination { get; init; }

        public required byte[] RawPacketBytes { get; init; }

        public required IdpPacket Packet { get; init; }
    }
}

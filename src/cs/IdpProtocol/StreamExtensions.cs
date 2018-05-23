// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using System.IO;

namespace IdpProtocol
{
    public static class StreamExtensions
    {
        public static long ReceivedBytes(this Stream stream)
        {
            return stream.Length - stream.Position;
        }

        public static void SendPacket(this Stream stream, IdpPacket packet)
        {
            stream.Write(packet.Data, 0, packet.Data.Length);
        }
    }
}
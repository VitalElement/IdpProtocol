// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using MiscUtil.Conversion;
using System;
using System.IO;
using System.Net.Sockets;
using System.Threading;

namespace IdpProtocol
{
    public class IdpParser
    {
        private Stream incomingStream;

        private Func<bool> currentState;
        private UInt32 currentPacketLength;
        private IdpFlags currentPacketFlags;
        private IdpPacket currentPacket;
        private UInt16 currentPacketSource;
        private UInt32 currentPacketCRC;

        public event EventHandler<PacketParsedEventArgs> PacketParsed;
        public bool ReadFailed { get; private set; }

        public IdpParser(Stream source)
        {
            incomingStream = source;
            currentState = WaitingForStx;
        }

        public bool Parse()
        {
            bool dataProcessed = false;
            ReadFailed = false;

            if (incomingStream is NetworkStream ns)
            {
                if (!ns.DataAvailable)
                {
                    return dataProcessed;
                }
            }
            else if(incomingStream.Length == 0)
            {
                return dataProcessed;
            }

            while (currentState())
            {
                dataProcessed = true;
            }

            return dataProcessed;
        }

        private void Reset()
        {
            currentState = WaitingForStx;
            currentPacket = null;
            currentPacketLength = 0;
            currentPacketCRC = 0;
            currentPacketFlags = IdpFlags.None;
        }

        private bool TryReadData(int length, out byte[] data)
        {
            data = new byte[length];

            var read = 0;

            while (read < length)
            {
                var currentRead = incomingStream.Read(data, read, data.Length - read);

                if (currentRead == 0)
                {
                    ReadFailed = true;
                    data = null;
                    return false;
                }

                read += currentRead;

                if (read < length)
                {
                    Thread.Sleep(2);
                }
            }

            return true;
        }

        private bool TryReadByte(out byte value)
        {
            value = 0;

            if (!TryReadData(1, out var data))
            {
                return false;
            }

            value = data[0];
            return true;
        }

        private bool WaitingForStx()
        {
            if (TryReadByte(out var data) && data == 0x02)
            {
                currentState = WaitingForLength;
                return true;
            }

            return false;
        }

        private bool WaitingForLength()
        {
            if (TryReadData(4, out var lengthBytes))
            {
                currentPacketLength = EndianBitConverter.Big.ToUInt32(lengthBytes, 0);

                if(currentPacketLength > 1000000)
                {
                    Reset();
                    return false;
                }

                currentState = WaitingForFlags;

                return true;
            }

            return false;
        }

        private bool WaitingForFlags()
        {
            if (TryReadByte(out var value))
            {
                currentPacketFlags = (IdpFlags)value;

                if ((currentPacketFlags.HasFlag(IdpFlags.CRC) && currentPacketLength > 11) ||
                    (!currentPacketFlags.HasFlag(IdpFlags.CRC) && currentPacketLength > 7))
                {
                    currentState = WaitingForSource;

                    return true;
                }

                Reset();
            }

            return false;
        }

        private bool WaitingForSource()
        {
            if (TryReadData(2, out var sourceBytes))
            {
                currentPacketSource = EndianBitConverter.Big.ToUInt16(sourceBytes, 0);

                currentState = WaitingForDestination;

                return true;
            }

            return false;
        }

        private bool WaitingForDestination()
        {
            if (TryReadData(2, out var destinationBytes))
            {
                var destination = EndianBitConverter.Big.ToUInt16(destinationBytes, 0);

                currentState = WaitingForPayload;

                currentPacket = new IdpPacket(currentPacketLength - 11, currentPacketFlags, currentPacketSource, destination, true);

                return true;
            }

            return false;
        }

        private bool WaitingForPayload()
        {
            if (TryReadData((int)currentPacketLength - 11, out byte[] currentPacketPayload))
            {
                currentPacket.Write(currentPacketPayload);
                currentState = WaitingForEtx;

                return true;
            }

            return false;
        }

        private bool WaitingForEtx()
        {
            if (TryReadByte(out var value))
            {
                if (value == 0x03)
                {
                    currentPacket.Write(value);

                    if (currentPacketFlags.HasFlag(IdpFlags.CRC))
                    {
                        currentState = WaitingForCrc;
                    }
                    else
                    {
                        currentState = ValidatingPacket;
                    }

                    return true;
                }
                else
                {
                    Reset();
                    return true;
                }
            }

            return false;
        }

        private bool WaitingForCrc()
        {
            if (TryReadData(4, out var lengthBytes))
            {
                currentPacketCRC = BitConverter.ToUInt32(lengthBytes, 0);

                currentState = ValidatingPacket;

                return true;
            }

            return false;
        }

        private bool ValidatingPacket()
        {
            bool emitPayload = true;

            if (currentPacketFlags.HasFlag(IdpFlags.CRC))
            {
                // todo validate crc.

                emitPayload = true;
            }

            if (emitPayload)
            {
                PacketParsed(this, new PacketParsedEventArgs(currentPacket));
            }

            Reset();

            return false;
        }
    }
}

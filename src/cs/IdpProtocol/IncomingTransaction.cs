// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using MiscUtil.Conversion;
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace IdpProtocol
{
    public class IncomingTransaction
    {
        public int _readIndex;
        private readonly IdpPacket _packet;

        internal IncomingTransaction(IdpPacket packet)
        {
            _packet = packet;

            Data = packet.Data;
            _readIndex = 10;

            CommandId = Read<UInt16>();
            TransactionId = Read<UInt32>();
            Flags = Read<IdpCommandFlags>();

            Source = packet.Source;
            Destination = packet.Destination;
        }

        public UInt16 Source { get; }

        public UInt16 Destination { get; }

        public byte[] Data { get; }

        /*public Span<byte> SpanToEnd()
        {
            return new Span<byte>(_data, _readIndex, _data.Length - _readIndex);
        }*/

        public byte[] BufferToEnd ()
        {
            var result = new byte[Data.Length - _readIndex];

            Array.Copy(Data, _readIndex, result, 0, result.Length);

            return result;
        }

        public IdpCommandFlags Flags { get; }
        public UInt16 CommandId { get; }
        public UInt32 TransactionId { get; }

        public Stream StreamToEnd()
        {
            var result = new MemoryStream(Data, _readIndex, Data.Length - _readIndex);

            return result;
        }

        public string ReadUtf8String()
        {
            int index = _readIndex;
            bool stringFound = false;

            while (index < Data.Length)
            {
                if (Data[index] == '\0')
                {
                    stringFound = true;
                    break;
                }

                index++;
            }

            if (stringFound)
            {
                var result = Encoding.UTF8.GetString(Data, _readIndex, index - _readIndex);

                _readIndex += index - _readIndex;

                return result;
            }
            else
            {
                return "";
            }
        }

        public T Read<T>() where T : struct
        {
            object result = null;
            bool increment = true;

            if (typeof(T) == typeof(bool))
            {
                result = EndianBitConverter.Big.ToBoolean(Data, _readIndex);

                increment = false;

                _readIndex++;
            }
            else if (typeof(T) == typeof(Int32))
            {
                result = EndianBitConverter.Big.ToInt32(Data, _readIndex);
            }
            else if (typeof(T) == typeof(UInt32))
            {
                result = EndianBitConverter.Big.ToUInt32(Data, _readIndex);
            }
            else if (typeof(T) == typeof(Int16))
            {
                result = EndianBitConverter.Big.ToInt16(Data, _readIndex);
            }
            else if (typeof(T) == typeof(UInt16))
            {
                result = EndianBitConverter.Big.ToUInt16(Data, _readIndex);
            }
            else if (typeof(T) == typeof(char))
            {
                result = EndianBitConverter.Big.ToChar(Data, _readIndex);
            }
            else if (typeof(T) == typeof(byte))
            {
                result = Data[_readIndex];
            }
            else if (typeof(T) == typeof(float))
            {
                result = EndianBitConverter.Big.ToSingle(Data, _readIndex);
            }
            else if (typeof(T) == typeof(IdpCommandFlags))
            {
                result = (IdpCommandFlags)Data[_readIndex];

                increment = false;

                _readIndex++;
            }
            else if (typeof(T) == typeof(IdpResponseCode))
            {
                result = (IdpResponseCode)Data[_readIndex];

                increment = false;

                _readIndex++;
            }
            else if (typeof(T) == typeof(Guid))
            {
                result = new Guid(Read<int>(), Read<short>(), Read<byte>(),
                    Read<byte>(), Read<byte>(), Read<byte>(), 
                    Read<byte>(), Read<byte>(), Read<byte>(), 
                    Read<byte>(), Read<byte>());

                increment = false;
            }
            else
            {
                throw new NotSupportedException($"Unable to decode type {typeof(T).FullName} from response.");
            }

            if (increment)
            {
                _readIndex += Marshal.SizeOf(result);
            }

            return (T)result;
        }
    }
}

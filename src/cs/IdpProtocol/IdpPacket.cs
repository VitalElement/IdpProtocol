// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using MiscUtil.Conversion;
using System;
using System.Runtime.InteropServices;

namespace IdpProtocol
{
    public enum IdpFlags
    {
        None = 0,
        CRC = 0x01
    }

    public class IdpPacket
    {
        private UInt32 _writeIndex;
        private UInt32 _length;
        private bool _isSealed;
        private int _readIndex;

        public IdpPacket(UInt32 payloadLength, IdpFlags flags, UInt16 source, UInt16 destination, bool isSealed = false)
        {
            _writeIndex = 0;
            _readIndex = 0;
            _isSealed = false;

            _length = 11 + payloadLength;

            if (flags.HasFlag(IdpFlags.CRC))
            {
                _length += 4;
            }

            Data = new byte[_length];

            Source = source;
            Destination = destination;

            Write((byte)0x02);
            Write(_length);
            Write((byte)flags);
            Write(source);
            Write(destination);

            _isSealed = isSealed;
        }

        public void Seal()
        {
            if (!_isSealed)
            {
                Write((byte)0x03);

                if (Flags.HasFlag(IdpFlags.CRC))
                {
                    //todo implement crc generation.
                    Write((UInt32)0xAA55AA55);
                }
            }

            _isSealed = true;
        }

        public void ResetRead()
        {
            _readIndex = 0;
        }

        public void ResetReadToPayload()
        {
            _readIndex = 10;
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

        public UInt16 Source { get; }

        public UInt16 Destination { get; }

        public byte[] Data { get; }

        public IdpFlags Flags => (IdpFlags)Data[5];

        public void Write(byte[] data)
        {
            Buffer.BlockCopy(data, 0, Data, (int)_writeIndex, data.Length);

            _writeIndex += (uint)data.Length;
        }

        public void Write<T>(T value) where T : struct
        {
            int size = Marshal.SizeOf(value);
            var bytes = new byte[size];
            IntPtr ptr = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(value, ptr, true);
            Marshal.Copy(ptr, bytes, 0, size);
            Marshal.FreeHGlobal(ptr);

            if (BitConverter.IsLittleEndian)
            {
                Array.Reverse(bytes);
            }

            Write(bytes);
        }
    }
}

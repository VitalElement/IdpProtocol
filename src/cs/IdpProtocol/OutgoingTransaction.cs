// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;

namespace IdpProtocol
{
    public class OutgoingTransaction
    {
        private List<byte> _data;
        private UInt32 _writeIndex;

        public static OutgoingTransaction Create (UInt16 commandId, UInt32 transactionId, IdpCommandFlags flags = IdpCommandFlags.ResponseExpected)
        {
            return new OutgoingTransaction(commandId, transactionId, flags);
        }

        public OutgoingTransaction(UInt16 commandId, UInt32 transactionId, IdpCommandFlags flags = IdpCommandFlags.ResponseExpected)
        {
            _data = new List<byte>();
            CommandId = commandId;
            TransactionId = transactionId;

            Write(commandId);
            Write(transactionId);
            Write((byte)flags);
        }

        public UInt16 CommandId { get; }

        public UInt32 TransactionId { get; }

        public IdpPacket ToPacket(UInt16 source, UInt16 destination)
        {
            var result = new IdpPacket((uint)_data.Count, IdpFlags.None, source, destination);

            result.Write(_data.ToArray());
            result.Seal();

            return result;
        }

        public OutgoingTransaction WithResponseCode(IdpResponseCode responseCode)
        {
            WriteAt((byte)responseCode, 7);

            return this;
        }

        public OutgoingTransaction WriteAt(byte[] data, int index)
        {
            _data.RemoveRange(index, data.Length);

            _data.InsertRange(index, data);

            return this;
        }

        public OutgoingTransaction WriteAt<T>(T value, int index) where T : struct
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

            return WriteAt(bytes, index);
        }

        public OutgoingTransaction Write(byte[] data)
        {
            return Write(data, 0, data.Length);
        }

        public OutgoingTransaction Write (byte[] data, int startIndex, int length)
        {
            _data.AddRange(data.Skip(startIndex).Take(length));

            _writeIndex+= (uint) length;

            return this;
        }

        public OutgoingTransaction Write<T>(T value) where T : struct
        {
            if (value is bool boolValue)
            {
                return Write(Convert.ToByte(boolValue));
            }
            else if (value is Guid guid)
            {
                var data = guid.ToByteArray();
                UInt32 data1 = BitConverter.ToUInt32(data, 0);
                UInt16 data2 = BitConverter.ToUInt16(data, 4);
                UInt16 data3 = BitConverter.ToUInt16(data, 6);

                Write(data1);
                Write(data2);
                Write(data3);
                Write(data, 8, 8);

                return this;
            }
            else
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

                return Write(bytes);
            }
        }
    }
}

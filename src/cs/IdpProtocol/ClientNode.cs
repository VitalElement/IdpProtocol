// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using System;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace IdpProtocol
{
    public enum ClientCommand
    {
        Connect = 0xD000,
        Disconnect
    };

    public abstract class ClientNode : NetworkQueryNode
    {
        private UInt16 _serverAddress;

        public ClientNode(UInt16 serverAddress) : base(Guid.Parse("3BD3137C-26A3-4826-88A0-26D8AD066C4E"), "Network.Client")
        {
            _serverAddress = serverAddress;
        }

        public ClientNode() : this(UnassignedAddress)
        {

        }

        public async Task<bool> ConnectAsync()
        {
            var type = GetType();
            var guidAttribute = type.GetCustomAttributes(typeof(GuidAttribute), true).OfType<GuidAttribute>().FirstOrDefault();

            if (guidAttribute == null)
            {
                throw new Exception("T must have a Guid Attribute specifying the interface.");
            }

            var guid = Guid.Parse(guidAttribute.Value);

            var remoteAddress = await QueryInterfaceAsync(guid);

            if (remoteAddress != 0)
            {
                SetAddress(remoteAddress);

                return true;
            }
            else
            {
                SetAddress(UnassignedAddress);
            }

            return false;
        }

        public async Task<bool> ConnectAsync(Guid guid)
        {
            var remoteAddress = await QueryInterfaceAsync(guid);

            if (remoteAddress != 0)
            {
                SetAddress(remoteAddress);

                return true;
            }

            return false;
        }

        public async Task<bool> ConnectClientAsync()
        {
            var (success, response) = await SendRequestAsync(OutgoingTransaction.Create((UInt16)ClientCommand.Connect, CreateTransactionId()));

            if (success && response != null && response.ResponseCode == IdpResponseCode.OK)
            {
                return true;
            }

            return false;
        }

        public async Task<bool> DisconnectAsync()
        {
            var (success, response) = await SendRequestAsync(OutgoingTransaction.Create((UInt16)ClientCommand.Disconnect, CreateTransactionId()));

            if (success && response != null && response.ResponseCode == IdpResponseCode.OK)
            {
                return true;
            }

            return false;
        }

        internal void SetAddress(UInt16 address)
        {
            _serverAddress = address;
        }

        public async Task<(bool success, IdpResponse response)> SendRequestAsync(OutgoingTransaction request, UInt32 timeout = IdpNode.DefaultTimeoutMsec)
        {
            return await SendRequestAsync(_serverAddress, request, timeout);
        }

        public bool SendRequest(OutgoingTransaction transaction)
        {
            return SendRequest(_serverAddress, transaction);
        }
    }
}

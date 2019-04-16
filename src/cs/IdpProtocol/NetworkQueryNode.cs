// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using System;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace IdpProtocol
{
    public abstract class NetworkQueryNode : IdpNode
    {
        public NetworkQueryNode(Guid nodeGuid, string nodeName) : base(nodeGuid, nodeName)
        {
        }

        public event EventHandler<InterfaceFoundEventArgs> InterfaceFound;

        private void OnQueryInterfaceResponse(IdpResponse response)
        {
            if (response.ResponseCode == IdpResponseCode.OK)
            {
                var vid = response.Transaction.Read<UInt16>();
                var pid = response.Transaction.Read<UInt16>();

                InterfaceFound?.Invoke(this, new InterfaceFoundEventArgs(vid, pid, response.Transaction.Source));
            }
        }

        public void QueryInterface(UInt16 vid, UInt16 pid)
        {
            Manager.RegisterResponseHandler((UInt16)NodeCommand.QueryInterface, response => OnQueryInterfaceResponse(response));

            SendRequest(0, OutgoingTransaction.Create((UInt16)NodeCommand.QueryInterface, CreateTransactionId()).Write(vid).Write(pid));
        }

        public async Task<UInt16> QueryInterfaceAsync(Guid guid)
        {
            UInt16 result = 0;

            var (success, response) = await SendRequestAsync(0,
                                                  OutgoingTransaction.Create((UInt16)NodeCommand.QueryInterface, CreateTransactionId(), IdpCommandFlags.None)
                                                  .Write(guid));

            if (success && response != null && response.ResponseCode == IdpResponseCode.OK)
            {
                result = response.Transaction.Source;
            }

            return result;
        }

        public async Task<T> QueryInterfaceAsync<T>() where T : ClientNode
        {
            var guidAttribute = typeof(T).GetCustomAttributes(typeof(GuidAttribute), true).OfType<GuidAttribute>().FirstOrDefault();

            if (guidAttribute == null)
            {
                throw new Exception("T must have a Guid Attribute specifying the interface.");
            }

            var guid = Guid.Parse(guidAttribute.Value);

            var remoteAddress = await QueryInterfaceAsync(guid);

            if (remoteAddress == 0)
            {
                return null;
            }

            var result = Activator.CreateInstance(typeof(T)) as T;

            result.SetAddress(remoteAddress);

            result.TransmitEndpoint = TransmitEndpoint;

            return result;
        }
    }
}

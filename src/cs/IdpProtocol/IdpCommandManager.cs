// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using System;
using System.Collections.Generic;
using System.Reactive.Linq;
using System.Reactive.Subjects;
using System.Threading.Tasks;

namespace IdpProtocol
{
    public enum IdpCommandFlags
    {
        None,
        ResponseExpected = 0x01,
    }

    public enum IdpResponseCode
    {
        OK,
        UnknownCommand,
        InvalidParameters,
        UnknownError,
        NotReady,
        Deferred, //!< when the handler wants to defer the response until later on.
        Internal  //!< Special case where processing needs to occur at packet level.
    }

    public class IdpCommandManager
    {
        public delegate IdpResponseCode CommandHandler(IncomingTransaction incomingTransaction, OutgoingTransaction outgoingTransaction);

        public delegate void ResponseHandler(IdpResponse response);

        private Dictionary<UInt16, CommandHandler> _commandHandlers;

        private Dictionary<UInt16, ResponseHandler> _responseHandlers;

        private Subject<IdpResponse> _onResponseReceived;

        public IdpCommandManager()
        {
            _commandHandlers = new Dictionary<UInt16, CommandHandler>();

            _responseHandlers = new Dictionary<UInt16, ResponseHandler>();

            _onResponseReceived = new Subject<IdpResponse>();

            RegisterCommand(0xA000, (incomingTransaction, outgoingTransaction) =>
            {
                var response = new IdpResponse(incomingTransaction);

                if (_responseHandlers.ContainsKey(response.ResponseId))
                {
                    _responseHandlers[response.ResponseId](response);
                }
                else
                {
                    _onResponseReceived.OnNext(response);
                }

                return IdpResponseCode.OK;
            });
        }

        public void RegisterCommand(UInt16 commandId, CommandHandler onReceived)
        {
            _commandHandlers[commandId] = onReceived;
        }

        public void RegisterResponseHandler(UInt16 commandId, ResponseHandler handler)
        {
            _responseHandlers[commandId] = handler;
        }

        public async Task<IdpResponse> WaitForResponseAsync(UInt32 transactionId, UInt32 timeoutMs)
        {
            try
            {
                var result = await _onResponseReceived.FirstOrDefaultAsync(r => r.TransactionId == transactionId).Timeout(TimeSpan.FromMilliseconds(timeoutMs));

                return result;
            }
            catch (TimeoutException)
            {
                return null;
            }
        }

        public IdpPacket ProcessPayload(UInt16 nodeAddress, IdpPacket packet)
        {
            var incoming = new IncomingTransaction(packet);
            var outgoing = new OutgoingTransaction(0xA000, incoming.TransactionId, IdpCommandFlags.None);

            if (_commandHandlers.ContainsKey(incoming.CommandId))
            {
                outgoing.Write((byte)IdpResponseCode.OK);
                outgoing.Write(incoming.CommandId);

                var responseCode = _commandHandlers[incoming.CommandId](incoming, outgoing);

                if (incoming.Flags.HasFlag(IdpCommandFlags.ResponseExpected))
                {
                    outgoing.WithResponseCode(responseCode);

                    return outgoing.ToPacket(nodeAddress, packet.Source);
                }
            }
            else
            {
                outgoing.Write((byte)IdpResponseCode.UnknownCommand);
                outgoing.Write(incoming.CommandId);

                return outgoing.ToPacket(nodeAddress, packet.Source);
            }

            return null;
        }
    }
}

// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using System;
using System.Reactive.Concurrency;
using System.Reactive.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace IdpProtocol
{
    public enum NodeCommand
    {
        Response = 0xA000,
        Ping = 0xA001,
        GetNodeInfo = 0xA002,
        QueryInterface = 0xA003,
        Reset = 0xA004,

        RecommendEnumeration = 0xA005,

        RouterDetect = 0xA006,
        RouterEnumerateNode = 0xA007,
        RouterPrepareToEnumerateAdaptors = 0xA008,
        RouterEnumerateAdaptor = 0xA009,
        MarkAdaptorConnected = 0xA00A,

        RouterPoll = 0xA00B
    };

    public class IdpNode
    {
        public const UInt16 UnassignedAddress = 0xFFFF;
        public const UInt16 RouterPollAddress = 0xFFFE;
        public const UInt32 DefaultTimeoutMsec = 5000;
        private TaskCompletionSource<bool> _enumerationSource;
        private UInt32 _currentTransactionId;
        private DateTime _lastPing = DateTime.Now;

        public IdpNode(Guid guid, string name, UInt16 address = UnassignedAddress)
        {
            Manager = new IdpCommandManager();

            Address = address;
            Guid = guid;
            Name = name;

            _currentTransactionId = 1;

            Enabled = true;

            Manager.RegisterResponseHandler((UInt16)NodeCommand.Ping, response =>
            {
                _lastPing = DateTime.Now;
            });

            Manager.RegisterCommand((UInt16)NodeCommand.Ping, (i, o) =>
            {
                return IdpResponseCode.OK;
            });

            Manager.RegisterCommand((UInt16)NodeCommand.GetNodeInfo, (i, o) =>
            {
                o.Write(Guid);

                var utf8Name = Encoding.UTF8.GetBytes(Name);

                o.Write(utf8Name);
                o.Write((byte)0); // null terminator, is this required?

                o.Write((byte)1);

                SetEnumerated();

                return IdpResponseCode.OK;
            });

            Manager.RegisterCommand((UInt16)NodeCommand.QueryInterface, (i, o) =>
            {
                var requestedGuid = i.Read<Guid>();

                if (requestedGuid == Guid)
                {
                    o.Write(Guid.ToByteArray());

                    return IdpResponseCode.OK;
                }

                return IdpResponseCode.Deferred;
            });

            Manager.RegisterCommand((UInt16)NodeCommand.Reset, (i, o) =>
            {
                OnReset();

                return IdpResponseCode.OK;
            });

            _enumerationSource = new TaskCompletionSource<bool>();

            Observable.Interval(TimeSpan.FromSeconds(1)).ObserveOn(CurrentThreadScheduler.Instance).Subscribe(async _ =>
            {
                await OnPollTimerTickAsync();
            });
        }

        public virtual async Task OnPollTimerTickAsync ()
        {
            if (Address != UnassignedAddress && Enabled)
            {
                SendRequest(1, OutgoingTransaction.Create((UInt16)NodeCommand.Ping, CreateTransactionId(), IdpCommandFlags.None));

                /*if (response.success && response.response != null && response.response.ResponseCode == IdpResponseCode.OK)
                {
                    if (DateTime.Now - _lastPing > TimeSpan.FromSeconds(4))
                    {
                        OnReset();
                    }
                }
                else if (response.response != null || response.success == false)
                {
                    OnReset();
                }*/
            }
        }

        public UInt32 CreateTransactionId ()
        {
            return _currentTransactionId++;
        }

        protected virtual void OnReset()
        {
            Address = UnassignedAddress;
        }

        public Task<bool> WaitForEnumeration()
        {
            return _enumerationSource.Task;
        }

        public IdpPacket ProcessPacket(IdpPacket packet)
        {
            if (Enabled)
            {
                return Manager.ProcessPayload(Address, packet);
            }

            return null;
        }

        public async Task<(bool success, IdpResponse response)> SendRequestAsync(UInt16 destination, OutgoingTransaction request, UInt32 timeout = IdpNode.DefaultTimeoutMsec)
        {
            return await SendRequestAsync(Address, destination, request, timeout);
        }

        public async Task<(bool success, IdpResponse response)> SendRequestAsync(UInt16 source, UInt16 destination, OutgoingTransaction transaction, UInt32 timeout)
        {
            var responseTask = Manager.WaitForResponseAsync(transaction.TransactionId, timeout);

            if (SendRequest(source, destination, transaction))
            {
                return (true, await responseTask);
            }
            else
            {
                return (false, null);
            }
        }


        public bool SendRequest(UInt16 source, UInt16 destination, OutgoingTransaction transaction)
        {
            if (Connected && Enabled)
            {
                return TransmitEndpoint.Transmit(transaction.ToPacket(source, destination));
            }

            return false;
        }

        public bool SendRequest(UInt16 destination, OutgoingTransaction transaction)
        {
            return SendRequest(Address, destination, transaction);
        }

        public void SetEnumerated ()
        {
            if (!_enumerationSource.Task.IsCompleted)
            {
                _enumerationSource.SetResult(true);
            }
        }

        public void Reset ()
        {
            Address = UnassignedAddress;
            _enumerationSource = new TaskCompletionSource<bool>();
        }

        public IPacketTransmit TransmitEndpoint { get; set; }

        public bool Connected => TransmitEndpoint != null;

        public bool Enabled { get; set; }

        public Guid Guid { get; }
        public UInt16 Address { get; internal set; }

        public string Name { get; }

        public IdpCommandManager Manager { get; }
    }
}

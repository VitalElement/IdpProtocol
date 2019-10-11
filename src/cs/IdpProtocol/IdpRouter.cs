// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;

namespace IdpProtocol
{
    public class IdpRouter : IdpNode, IAdaptorToRouterPort, IPacketTransmit
    {
        private List<IdpNode> _unenumeratedNodes;
        private Dictionary<UInt16, IdpNode> _enumeratedNodes;
        private Dictionary<UInt16, IdpAdaptor> _adaptors;
        private Dictionary<UInt16, UInt16> _routingTable;
        private UInt16 _nextAdaptorId;
        private IdpAdaptor _currentlyEnumeratingAdaptor;
        private int _lastAdaptorId = -1;

        public static readonly Guid RouterGuid = Guid.Parse("A1EE332D-5C7C-42FE-9519-54BDAC40CF21");

        public IdpRouter() : base(RouterGuid, "Network.Router")
        {
            _unenumeratedNodes = new List<IdpNode>();
            _enumeratedNodes = new Dictionary<UInt16, IdpNode>();
            _adaptors = new Dictionary<UInt16, IdpAdaptor>();
            _routingTable = new Dictionary<UInt16, UInt16>();

            Manager.RegisterCommand((UInt16)NodeCommand.RouterDetect, (i, o) =>
            {
                var address = i.Read<UInt16>();

                if (Address == UnassignedAddress)
                {
                    Address = address;
                    o.Write(true);
                    o.WithResponseCode(IdpResponseCode.OK);

                    SendRequest(i.Source, o);

                    return IdpResponseCode.Deferred;
                }
                else
                {
                    o.Write(false);
                    return IdpResponseCode.OK;
                }
            });

            Manager.RegisterCommand((UInt16)NodeCommand.RouterEnumerateNode, (i, o) =>
                                    {
                                        var address = i.Read<UInt16>();

                                        return OnEnumerateNodesCommand(i.Source, address, o);
                                    });

            Manager.RegisterCommand((UInt16)NodeCommand.RouterEnumerateAdaptor, (i, o) =>
                                    {
                                        var address = i.Read<UInt16>();
                                        var transactionId = i.Read<UInt32>();

                                        return OnEnumerateAdaptorCommand(i.Source, address, transactionId, o);
                                    });

            Manager.RegisterCommand((UInt16)NodeCommand.RouterPrepareToEnumerateAdaptors, (i, o) =>
            {
                foreach (var adaptor in _adaptors.Values)
                {
                    if (!adaptor.IsActive)
                    {
                        adaptor.IsRenumerated = false;
                    }
                }

                return IdpResponseCode.OK;
            });

            Manager.RegisterCommand((UInt16)NodeCommand.MarkAdaptorConnected, (i, o) =>
            {
                if (_currentlyEnumeratingAdaptor != null)
                {
                    _currentlyEnumeratingAdaptor.IsEnumerated = true;

                    _currentlyEnumeratingAdaptor = null;
                }
                else if(_lastAdaptorId != -1)
                {
                    _adaptors[(ushort)_lastAdaptorId].IsEnumerated = true;

                    _lastAdaptorId = -1;
                }

                return IdpResponseCode.OK;
            });

            TransmitEndpoint = this;

            _nextAdaptorId = 1;
        }

        /*public override async Task OnPollTimerTickAsync()
        {
            await base.OnPollTimerTickAsync();

            foreach(var adaptorEntry in _adaptors.Where(x=>x.Value.IsEnumerated))
            {
                var adaptor = adaptorEntry.Value;

                var outgoingTransaction = OutgoingTransaction.Create((UInt16)NodeCommand.RouterPoll, CreateTransactionId());

                var result = adaptor.Transmit(outgoingTransaction.ToPacket(Address, RouterPollAddress));

                if(result)
                {
                    var response = await Manager.WaitForResponseAsync(outgoingTransaction.TransactionId, 2500);

                    if(response == null || response.ResponseCode != IdpResponseCode.OK)
                    {
                        adaptor.IsEnumerated = false;
                    }
                }
                else
                {
                    adaptor.IsEnumerated = false;
                }
            }
        }*/

        protected override void OnReset()
        {
            foreach (var node in _enumeratedNodes.Values.ToList())
            {
                if (node.Address != 0x0001)
                {
                    MarkUnenumerated(node);
                }
            }

            foreach (var adaptor in _adaptors.Values)
            {
                adaptor.IsEnumerated = false;
            }

            base.OnReset();
        }

        private IdpNode FindNode(UInt16 address)
        {
            if (_enumeratedNodes.ContainsKey(address))
            {
                return _enumeratedNodes[address];
            }

            return null;
        }

        private IdpResponseCode OnEnumerateNodesCommand(UInt16 source, UInt16 address, OutgoingTransaction outgoing)
        {
            if (_unenumeratedNodes.Count != 0)
            {
                var node = _unenumeratedNodes.First();

                node.Address = address;
                //node.SetEnumerated();

                if (MarkEnumerated(node))
                {
                    outgoing.Write(true);
                    outgoing.WithResponseCode(IdpResponseCode.OK);

                    SendRequest(address, source, outgoing);

                    return IdpResponseCode.Deferred;
                }
                else
                {
                    node.Address = UnassignedAddress;

                    outgoing.Write(false);

                    return IdpResponseCode.InvalidParameters;
                }
            }
            else
            {
                outgoing.Write(false);
            }

            return IdpResponseCode.OK;
        }

        private IdpResponseCode OnEnumerateAdaptorCommand(UInt16 source, UInt16 address, UInt32 transactionId, OutgoingTransaction outgoing)
        {
            var adaptor = GetNextUnenumeratedAdaptor();

            if (adaptor == null)
            {
                outgoing.Write(false);

                return IdpResponseCode.OK;
            }
            else
            {
                adaptor.IsRenumerated = true;

                bool adaptorProbed = false;

                if (!adaptor.IsEnumerated)
                {
                    var request = new OutgoingTransaction((UInt16)NodeCommand.RouterDetect, transactionId, IdpCommandFlags.None).Write(address);

                    adaptorProbed = SendRequest(adaptor, source, 0xFFFF, request);

                    if (adaptorProbed)
                    {
                        _currentlyEnumeratingAdaptor = adaptor;
                    }
                }

                outgoing.Write(true).Write(adaptorProbed).WithResponseCode(IdpResponseCode.OK);

                SendRequest(source, outgoing);

                return IdpResponseCode.Deferred;
            }
        }

        private bool SendRequest(IPacketTransmit adaptor, UInt16 source, UInt16 destination, OutgoingTransaction request)
        {
            return adaptor.Transmit(request.ToPacket(source, destination));
        }

        private IdpAdaptor GetNextUnenumeratedAdaptor()
        {
            return _adaptors.Values.FirstOrDefault(a => !a.IsEnumerated);
        }

        private IdpAdaptor GetNextReUnenumeratedAdaptor()
        {
            return _adaptors.Values.FirstOrDefault(a => !a.IsRenumerated);
        }

        public bool Transmit(ushort adaptorId, IdpPacket packet)
        {
            var source = packet.Source;

            if (source != UnassignedAddress && adaptorId != 0xFFFF)
            {
                //if (!_adaptors[adaptorId].IsEnumerated)
                {
                    _lastAdaptorId = adaptorId;
                }

                if (!(source == 1 && _routingTable.ContainsKey(source)))
                {
                    _routingTable[source] = adaptorId;
                }
            }

            return Route(packet);
        }

        public bool MarkEnumerated(IdpNode node)
        {
            bool result = false;

            if (FindNode(node.Address) == null)
            {
                _unenumeratedNodes.Remove(node);

                _enumeratedNodes[node.Address] = node;

                result = true;
            }

            return result;
        }

        public void MarkUnenumerated(IdpNode node)
        {
            if (!_unenumeratedNodes.Contains(node))
            {
                _unenumeratedNodes.Add(node);
            }

            if (_enumeratedNodes.ContainsKey(node.Address))
            {
                _enumeratedNodes.Remove(node.Address);
            }
        }

        public bool AddNode(IdpNode node)
        {
            bool result = false;

            if (node.Address == UnassignedAddress)
            {
                _unenumeratedNodes.Add(node);
                result = true;
            }
            else
            {
                if (FindNode(node.Address) == null)
                {
                    _enumeratedNodes[node.Address] = node;
                    result = true;
                }
            }

            if (result)
            {
                node.TransmitEndpoint = this;
                node.Enabled = true;
            }

            return result;
        }

        public void RemoveNode(IdpNode node)
        {
            if (node.Address == UnassignedAddress)
            {
                _unenumeratedNodes.Remove(node);
            }
            else if (FindNode(node.Address) != null)
            {
                _enumeratedNodes.Remove(node.Address);
            }

            node.Address = UnassignedAddress;
        }

        public bool AddAdaptor(IdpAdaptor adaptor)
        {
            bool result = false;

            adaptor.LocalPort = this;

            adaptor.Id = _nextAdaptorId;

            _adaptors[_nextAdaptorId] = adaptor;

            _nextAdaptorId++;

            return result;
        }

        public bool Route(IdpPacket packet)
        {
            var destination = packet.Destination;
            var source = packet.Source;

            packet.ResetReadToPayload();
            var command = packet.Read<UInt16>();

            packet.ResetRead();

            //Debug.WriteLine($"Router: 0x{Address.ToString("X4")} from: 0x{source.ToString("X4")}, to: 0x{ destination.ToString("X4")} Command: 0x{ command.ToString("X4")} ({(NodeCommand)command})");

            if (destination == 0 && Address != UnassignedAddress)
            {
                foreach (var adaptor in _adaptors.Values)
                {
                    if (!_routingTable.ContainsKey(source) || adaptor != _adaptors[_routingTable[source]])
                    {
                        packet.ResetRead();

                        adaptor.Transmit(packet);
                    }
                }

                foreach (var node in _enumeratedNodes.Values)
                {
                    packet.ResetRead();

                    var packetResponse = node.ProcessPacket(packet);

                    if (packetResponse != null)
                    {
                        Route(packetResponse);
                    }
                }


                packet.ResetRead();

                var response = ProcessPacket(packet);

                if (response != null)
                {
                    Route(response);
                }

                return true;
            }
            else if (destination == RouterPollAddress && Address != UnassignedAddress)
            {
                var response = ProcessPacket(packet);

                if(response != null)
                {
                    return Route(response);
                }

                return false;
            }
            else
            {
                IdpNode node = null;

                if (destination == Address)
                {
                    node = this;
                }
                else
                {
                    node = FindNode(destination);
                }

                if (node != null)
                {
                    var responsePacket = node.ProcessPacket(packet);

                    if (responsePacket != null)
                    {
                        return Route(responsePacket);
                    }

                    return true;
                }
                else
                {
                    if (_routingTable.ContainsKey(destination))
                    {
                        return _adaptors[_routingTable[destination]].Transmit(packet);
                    }
                    else
                    {
                        if (destination == 0xFFFF && Address != 0xFFFF)
                        {
                            return false;
                        }

                        return _adaptors.FirstOrDefault().Value.Transmit(packet);
                    }
                }
            }
        }

        public bool Transmit(IdpPacket packet)
        {
            return Transmit(0xFFFF, packet);
        }
    }
}

// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace IdpProtocol
{
    public enum NodeEnumerationState
    {
        Pending,
        DetectingRouter,
        EnumeratingNodes,
        StartEnumeratingAdaptors,
        EnumeratingAdaptors,
        Idle
    };

    public class NodeInfo
    {
        public NodeInfo(NodeInfo parent, UInt16 address)
        {
            Parent = parent;
            Address = address;
            LastSeen = DateTime.Now;
            Guid = Guid.Empty;
            EnumerationState = NodeEnumerationState.Idle;
            Parent = parent;
            Name = null;
            Children = new List<NodeInfo>();
        }

        public UInt16 Address { get; }

        internal DateTime LastSeen { get; set; }

        public Guid Guid { get; internal set; }

        public NodeInfo Parent { get; }

        public string Name { get; internal set; }

        internal NodeEnumerationState EnumerationState { get; set; }

        public List<NodeInfo> Children { get; }

        public bool IsRouter => Guid == IdpRouter.RouterGuid;
    }

    public class MasterNode : IdpNode
    {
        private UInt16 _nextAddress;
        private Stack<UInt16> _freeAddresses;
        private Dictionary<UInt16, NodeInfo> _nodeInfo;
        private NodeInfo _root;
        NodeInfo _currentEnumerationNode;

        public const UInt16 MasterNodeAddress = 0x0001;
        public static readonly Guid MasterGuid = Guid.Parse("554C0A67-F228-47B5-8155-8C5436D533DA");

        public MasterNode() : base(MasterGuid, "Network.Master", MasterNodeAddress)
        {
            _freeAddresses = new Stack<ushort>();
            _nodeInfo = new Dictionary<ushort, NodeInfo>();

            _nextAddress = 2;
            NodeTimeout = 5000;
            _root = new NodeInfo(null, MasterNodeAddress);
            _root.Guid = MasterGuid;

            _root.Name = "Network.Master";
            _root.EnumerationState = NodeEnumerationState.Pending;

            _currentEnumerationNode = null;

            _nodeInfo[Address] = _root;

            Manager.RegisterResponseHandler((UInt16)NodeCommand.Ping, response =>
            {
                OnPollResponse(response.Transaction.Source);
            });
        }

        public bool IsEnumerating { get; private set; }

        protected override void OnReset()
        {

        }

        private NodeInfo FindNode(UInt16 address)
        {
            if (_nodeInfo.ContainsKey(address))
            {
                return _nodeInfo[address];
            }

            return null;
        }

        private UInt16 GetFreeAddress()
        {
            if (_freeAddresses.Count == 0)
            {
                return _nextAddress++;
            }
            else
            {
                return _freeAddresses.Pop();
            }
        }

        private void OnPollResponse(UInt16 address)
        {
            if (address != Address)
            {
                var node = FindNode(address);

                if (node != null)
                {
                    node.LastSeen = DateTime.Now;
                }
            }
        }

        private void InvalidateNodes()
        {
            var currentTime = DateTime.Now;

            foreach (var node in _nodeInfo.Values.ToList())
            {
                if (node != _root && currentTime >= node.LastSeen + TimeSpan.FromMilliseconds(NodeTimeout))
                {
                    _nodeInfo.Remove(node.Address);

                    _freeAddresses.Push(node.Address);
                }
            }
        }

        private async Task OnNodeAdded(UInt16 parentAddress, UInt16 address)
        {
            _nodeInfo[address] = new NodeInfo(_nodeInfo[parentAddress], address);

            _nodeInfo[parentAddress].Children.Add(_nodeInfo[address]);

            var (success, response) = await SendRequestAsync(address, OutgoingTransaction.Create((UInt16)NodeCommand.GetNodeInfo, CreateTransactionId()));

            if (response != null && response.ResponseCode == IdpResponseCode.OK)
            {
                _nodeInfo[address].Guid = response.Transaction.Read<Guid>();

                _nodeInfo[address].Name = response.Transaction.ReadUtf8String();

                // TODO read string.

                if (_nodeInfo[address].IsRouter)
                {
                    _nodeInfo[address].EnumerationState = NodeEnumerationState.Pending;
                }
                else
                {
                    _nodeInfo[address].EnumerationState = NodeEnumerationState.Idle;
                }

                // normally on enumerate called here...
            }
        }

        private void VisitNodes(NodeInfo root, Func<NodeInfo, bool> visitor)
        {
            if (visitor(root))
            {
                foreach (var child in root.Children)
                {
                    VisitNodes(child, visitor);
                }
            }
        }

        private NodeInfo GetNextEnumerationNode()
        {
            NodeInfo result = null;

            VisitNodes(_root, node =>
            {
                if (node.EnumerationState != NodeEnumerationState.Idle)
                {
                    result = node;
                    return false;
                }

                return true;
            });

            return result;
        }

        private async Task EnumerateAsync()
        {
            NodeInfo node = null;

            while (true)
            {
                node = GetNextEnumerationNode();

                if (node == null)
                {
                    break;
                }

                _currentEnumerationNode = node;

                if (node == _root)
                {
                    _currentEnumerationNode.EnumerationState = NodeEnumerationState.DetectingRouter;

                    await DetectRouter();
                }
                else if (node.IsRouter)
                {
                    switch (node.EnumerationState)
                    {
                        case NodeEnumerationState.Pending:
                        case NodeEnumerationState.EnumeratingNodes:
                            await EnumerateRouterNode(node.Address);
                            break;

                        case NodeEnumerationState.StartEnumeratingAdaptors:
                            await StartEnumeratingAdaptors(node.Address);
                            break;

                        case NodeEnumerationState.EnumeratingAdaptors:
                            await EnumerateRouterAdaptor(node.Address);
                            break;

                        default:
                            break;
                    }
                }
            }
        }

        private async Task DetectRouter()
        {
            var address = GetFreeAddress();

            var (success, response) = await SendRequestAsync(0xFFFF, OutgoingTransaction.Create((UInt16)NodeCommand.RouterDetect, CreateTransactionId()).Write(address));

            if (success && response != null && response.ResponseCode == IdpResponseCode.OK)
            {
                var enumerated = response.Transaction.Read<bool>();

                if (enumerated)
                {
                    _currentEnumerationNode.EnumerationState = NodeEnumerationState.Idle;

                    await OnNodeAdded(Address, address);

                    return;
                }
            }

            _freeAddresses.Push(address);

            // normally on enumerate called.
        }

        private async Task EnumerateRouterNode(UInt16 routerAddress)
        {
            var address = GetFreeAddress();

            var (success, response) = await SendRequestAsync(routerAddress, OutgoingTransaction.Create((UInt16)NodeCommand.RouterEnumerateNode, CreateTransactionId()).Write(address));

            if (response != null && response.ResponseCode == IdpResponseCode.OK)
            {
                var enumerated = response.Transaction.Read<bool>();

                if (enumerated)
                {
                    await OnNodeAdded(routerAddress, address);

                    return;
                }
            }

            _freeAddresses.Push(address);

            _currentEnumerationNode.EnumerationState = NodeEnumerationState.StartEnumeratingAdaptors;
        }

        private async Task StartEnumeratingAdaptors(UInt16 routerAddress)
        {
            var (success, response) = await SendRequestAsync(routerAddress,
                                                  OutgoingTransaction.Create((UInt16)NodeCommand.RouterPrepareToEnumerateAdaptors, CreateTransactionId()));

            if (response != null && response.ResponseCode == IdpResponseCode.OK)
            {
                if (_currentEnumerationNode != null)
                {
                    _currentEnumerationNode.EnumerationState = NodeEnumerationState.EnumeratingAdaptors;
                }
            }
            else
            {
                if (_currentEnumerationNode != null)
                {
                    _currentEnumerationNode.EnumerationState = NodeEnumerationState.Idle;
                }
            }
        }

        private async Task EnumerateRouterAdaptor(UInt16 routerAddress)
        {
            var address = GetFreeAddress();

            var responseTask = Manager.WaitForResponseAsync((UInt16)NodeCommand.RouterDetect, 500);

            var (success, response) = await SendRequestAsync(routerAddress, OutgoingTransaction.Create((UInt16)NodeCommand.RouterEnumerateAdaptor, CreateTransactionId()).Write(address));

            if (response != null && response.ResponseCode == IdpResponseCode.OK)
            {
                var enumerated = response.Transaction.Read<bool>();

                if (enumerated)
                {
                    var routerDetectResponse = await responseTask;

                    if (routerDetectResponse != null && routerDetectResponse.ResponseCode == IdpResponseCode.OK)
                    {
                        var routerEnumerated = routerDetectResponse.Transaction.Read<bool>();

                        if (routerEnumerated)
                        {
                            await OnNodeAdded(routerAddress, address);

                            return;
                        }

                    }
                }
            }

            _freeAddresses.Push(address);

            _currentEnumerationNode.EnumerationState = NodeEnumerationState.Idle;
        }

        public bool HasNode(UInt16 address)
        {
            return _nodeInfo.ContainsKey(address);
        }

        public NodeInfo GetNodeInfo(UInt16 address)
        {
            return _nodeInfo[address];
        }

        public async Task EnumerateNetworkAsync()
        {
            if (Connected)
            {
                IsEnumerating = true;

                VisitNodes(_root, node =>
                {
                    if (node.IsRouter)
                    {
                        node.EnumerationState = NodeEnumerationState.Pending;
                    }
                    else if (node != _root)
                    {
                        node.EnumerationState = NodeEnumerationState.Idle;
                    }

                    return true;
                });

                await EnumerateAsync();

                IsEnumerating = false;
            }
        }

        public void PollNetwork()
        {
            //InvalidateNodes();

            SendRequest(0x0000, OutgoingTransaction.Create(0xA002, CreateTransactionId()));
        }

        public void ResetNetwork()
        {
            SendRequest(0x0000, OutgoingTransaction.Create((UInt16)NodeCommand.Reset, CreateTransactionId(), IdpCommandFlags.None));
        }

        public void TraceNetworkTree(NodeInfo node = null, UInt32 level = 0)
        {
            if (node == null)
            {
                node = _root;
            }

            Console.Write("");

            for (int i = 0; i < level * 4; i++)
            {
                Console.Write("-");
            }

            Console.Write(">");

            Console.Write(string.IsNullOrEmpty(node.Name) ? "Unnamed" : node.Name);
            Console.WriteLine($" ({node.Address})");

            foreach (var child in node.Children)
            {
                TraceNetworkTree(child, level + 1);
            }
        }

        public int NodeTimeout { get; set; }
    }
}

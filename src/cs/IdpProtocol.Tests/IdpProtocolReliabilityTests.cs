using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Xunit;

namespace IdpProtocol.Tests
{
    public class IdpProtocolReliabilityTests
    {
        [Fact]
        public void Can_Parse_Frame_With_1Byte_Payload_NoCRC()
        {
            var incomingData = new MemoryStream();
            var parser = new IdpParser(incomingData);
            var data = new byte[] { 0x02, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x01, 0x00, 0x02, 0xAA, 0x03 };

            incomingData.Write(data, 0, data.Length);
            incomingData.Seek(0, SeekOrigin.Begin);

            IdpPacket packet = null;
            parser.PacketParsed += (sender, e) => packet = e.Packet;

            parser.Parse();

            Assert.NotNull(packet);
            Assert.Equal((ushort)0x0001, packet.Source);
            Assert.Equal((ushort)0x0002, packet.Destination);
            Assert.Equal(0xAA, packet.Data[10]);
        }

        [Fact]
        public void Parse_Flags_ReadFailure_When_Stream_Ends_MidFrame()
        {
            var parser = new IdpParser(new MemoryStream(new byte[] { 0x02 }));

            parser.Parse();

            Assert.True(parser.ReadFailed);
        }

        [Fact]
        public void StreamAdaptor_Raises_ConnectionError_When_Input_Stream_Closes()
        {
            var connectionError = new ManualResetEventSlim();
            var adaptor = new StreamAdaptor(System.Reactive.Concurrency.CurrentThreadScheduler.Instance,
                new DisconnectingMemoryStream(new byte[] { 0x02 }),
                new MemoryStream());

            adaptor.ConnectionError += (sender, args) => connectionError.Set();
            adaptor.Start();

            Assert.True(connectionError.Wait(TimeSpan.FromSeconds(2)));

            adaptor.Dispose();
        }

        [Fact]
        public async Task WaitForResponseAsync_Receives_Response_Without_ResponseHandler()
        {
            var manager = new IdpCommandManager();
            var responseTask = manager.WaitForResponseAsync(42, 1000);

            manager.ProcessPayload(1, CreateResponsePacket(42, (ushort)NodeCommand.Ping));

            var response = await responseTask;

            Assert.NotNull(response);
            Assert.Equal((uint)42, response.TransactionId);
            Assert.Equal((ushort)NodeCommand.Ping, response.ResponseId);
        }

        [Fact]
        public async Task WaitForResponseAsync_Takes_Precedence_Over_ResponseHandler()
        {
            var manager = new IdpCommandManager();
            var handlerCalled = false;

            manager.RegisterResponseHandler((ushort)NodeCommand.Ping, _ => handlerCalled = true);

            var responseTask = manager.WaitForResponseAsync(42, 1000);

            manager.ProcessPayload(1, CreateResponsePacket(42, (ushort)NodeCommand.Ping));

            var response = await responseTask;

            Assert.NotNull(response);
            Assert.False(handlerCalled);
        }

        [Fact]
        public async Task ResponseHandler_Still_Receives_Response_When_No_Waiter_Exists()
        {
            var manager = new IdpCommandManager();
            var responseReceived = new TaskCompletionSource<IdpResponse>(TaskCreationOptions.RunContinuationsAsynchronously);

            manager.RegisterResponseHandler((ushort)NodeCommand.Ping, response => responseReceived.TrySetResult(response));

            manager.ProcessPayload(1, CreateResponsePacket(42, (ushort)NodeCommand.Ping));

            var completed = await Task.WhenAny(responseReceived.Task, Task.Delay(1000));

            Assert.Same(responseReceived.Task, completed);
            Assert.Equal((ushort)NodeCommand.Ping, responseReceived.Task.Result.ResponseId);
        }

        [Fact]
        public void QueryInterface_With_ResponseExpected_None_Still_Emits_Reply()
        {
            var guid = Guid.Parse("A1EE332D-5C7C-42FE-9519-54BDAC40CF21");
            var node = new IdpNode(guid, "Router", 0x0020);
            var transmitter = new RecordingTransmit();

            node.TransmitEndpoint = transmitter;

            var request = OutgoingTransaction.Create((ushort)NodeCommand.QueryInterface, 7, IdpCommandFlags.None)
                .Write(guid)
                .ToPacket(0x0001, node.Address);

            var response = node.ProcessPacket(request);

            Assert.Null(response);
            Assert.Single(transmitter.Packets);

            var packet = transmitter.Packets[0];
            packet.ResetReadToPayload();

            Assert.Equal((ushort)NodeCommand.Response, packet.Read<ushort>());
            Assert.Equal((uint)7, packet.Read<uint>());
            Assert.Equal((byte)IdpCommandFlags.None, packet.Read<byte>());
            Assert.Equal((byte)IdpResponseCode.OK, packet.Read<byte>());
            Assert.Equal((ushort)NodeCommand.QueryInterface, packet.Read<ushort>());
            Assert.Equal(node.Address, packet.Source);
            Assert.Equal((ushort)0x0001, packet.Destination);
        }

        [Fact]
        public void QueryInterface_With_NonMatching_Guid_Does_Not_Emit_Reply()
        {
            var node = new IdpNode(Guid.Parse("A1EE332D-5C7C-42FE-9519-54BDAC40CF21"), "Router", 0x0020);
            var transmitter = new RecordingTransmit();

            node.TransmitEndpoint = transmitter;

            var request = OutgoingTransaction.Create((ushort)NodeCommand.QueryInterface, 7, IdpCommandFlags.None)
                .Write(Guid.Parse("554C0A67-F228-47B5-8155-8C5436D533DA"))
                .ToPacket(0x0001, node.Address);

            var response = node.ProcessPacket(request);

            Assert.Null(response);
            Assert.Empty(transmitter.Packets);
        }

        [Fact]
        public async Task EnumerateRouterAdaptor_Uses_Embedded_RouterDetect_TransactionId()
        {
            var master = new MasterNode();
            var transmitter = new RecordingTransmit();
            var routerAddress = (ushort)0x0020;
            ushort newAddress = 0;
            uint nestedTransactionId = 0;

            AddRouterNode(master, routerAddress);
            master.TransmitEndpoint = transmitter;

            transmitter.OnTransmit = packet =>
            {
                packet.ResetReadToPayload();
                var commandId = packet.Read<ushort>();
                var transactionId = packet.Read<uint>();
                packet.Read<byte>();

                if (commandId == (ushort)NodeCommand.RouterEnumerateAdaptor)
                {
                    var requestedAddress = packet.Read<ushort>();
                    newAddress = requestedAddress;
                    nestedTransactionId = packet.Read<uint>();

                    master.Manager.ProcessPayload(master.Address,
                        CreateResponsePacket(transactionId, (ushort)NodeCommand.RouterEnumerateAdaptor, routerAddress,
                            master.Address, writer =>
                            {
                                writer.Write(true);
                                writer.Write(true);
                            }));

                    master.Manager.ProcessPayload(master.Address,
                        CreateResponsePacket(nestedTransactionId, (ushort)NodeCommand.RouterDetect, newAddress,
                            master.Address, writer => writer.Write(true)));

                    return true;
                }

                if (commandId == (ushort)NodeCommand.GetNodeInfo)
                {
                    master.Manager.ProcessPayload(master.Address,
                        CreateResponsePacket(transactionId, (ushort)NodeCommand.GetNodeInfo, newAddress,
                            master.Address, writer =>
                            {
                                writer.Write(Guid.NewGuid());
                                writer.Write(Encoding.UTF8.GetBytes("Node"));
                                writer.Write((byte)0);
                                writer.Write((uint)5000);
                            }));

                    return true;
                }

                return true;
            };

            await InvokeEnumerateRouterAdaptor(master, routerAddress);

            Assert.NotEqual(0u, nestedTransactionId);
            Assert.True(master.HasNode(newAddress));
            Assert.Contains(transmitter.Packets, packet =>
            {
                packet.ResetReadToPayload();
                return packet.Read<ushort>() == (ushort)NodeCommand.MarkAdaptorConnected;
            });
        }

        private static IdpPacket CreateResponsePacket(uint transactionId, ushort responseId, ushort source = 0x0002,
            ushort destination = 0x0001, Action<OutgoingTransaction> writePayload = null)
        {
            var transaction = new OutgoingTransaction((ushort)NodeCommand.Response, transactionId, IdpCommandFlags.None)
                .Write((byte)IdpResponseCode.OK)
                .Write(responseId);

            writePayload?.Invoke(transaction);

            return transaction.ToPacket(source, destination);
        }

        private static void AddRouterNode(MasterNode master, ushort routerAddress)
        {
            var root = master.GetNodeInfo(master.Address);
            var routerNode = new NodeInfo(root, routerAddress);

            root.Children.Add(routerNode);

            var nodeInfoField = typeof(MasterNode).GetField("_nodeInfo", BindingFlags.Instance | BindingFlags.NonPublic);
            var nodeInfo = (Dictionary<ushort, NodeInfo>)nodeInfoField.GetValue(master);
            nodeInfo[routerAddress] = routerNode;

            var currentEnumerationNodeField = typeof(MasterNode).GetField("_currentEnumerationNode", BindingFlags.Instance | BindingFlags.NonPublic);
            currentEnumerationNodeField.SetValue(master, routerNode);
        }

        private static Task InvokeEnumerateRouterAdaptor(MasterNode master, ushort routerAddress)
        {
            var method = typeof(MasterNode).GetMethod("EnumerateRouterAdaptor", BindingFlags.Instance | BindingFlags.NonPublic);
            return (Task)method.Invoke(master, new object[] { routerAddress });
        }

        private sealed class RecordingTransmit : IPacketTransmit
        {
            public List<IdpPacket> Packets { get; } = new List<IdpPacket>();
            public Func<IdpPacket, bool> OnTransmit { get; set; }

            public bool Transmit(IdpPacket packet)
            {
                Packets.Add(packet);

                return OnTransmit?.Invoke(packet) ?? true;
            }
        }

        private sealed class DisconnectingMemoryStream : MemoryStream
        {
            public DisconnectingMemoryStream(byte[] buffer) : base(buffer)
            {
            }

            public override int Read(byte[] buffer, int offset, int count)
            {
                if (Position >= Length)
                {
                    return 0;
                }

                return base.Read(buffer, offset, count);
            }
        }
    }
}

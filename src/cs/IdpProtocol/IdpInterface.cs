// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using System;
using System.IO;
using System.Reactive.Concurrency;
using System.Reactive.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace IdpProtocol
{
    public class StreamAdaptor : IdpAdaptor, IDisposable
    {
        private bool _isDisposing;
        private bool _isStarted;
        private IdpParser _parser;

        private Stream _inputStream;
        private Stream _outputStream;

        public event EventHandler ConnectionError;

        public IIdpTraceSink? TraceSink { get; set; }

        public StreamAdaptor(IScheduler scheduler) : this(scheduler, new SimplePipeStream(), new SimplePipeStream())
        {
            IsActive = true;
        }

        public StreamAdaptor(IScheduler scheduler, Stream inputStream, Stream outputStream)
        {
            _parser = new IdpParser(inputStream);

            _inputStream = inputStream;

            _outputStream = outputStream;

            Observable.FromEventPattern<PacketParsedEventArgs>(_parser, nameof(_parser.PacketParsed)).ObserveOn(scheduler)
                        .Subscribe(args =>
                        {
                            TraceSink?.Trace(CreateTracePacket(IdpTrafficDirection.Inbound, args.EventArgs.Packet));
                            OnReceive(args.EventArgs.Packet);
                        });
        }

        public Stream InputStream => _inputStream;

        public Stream OutputStream => _outputStream;

        public void Dispose()
        {
            _isDisposing = true;
        }

        public void Start()
        {
            if (!_isStarted)
            {
                _isStarted = true;
                IsActive = true;

                var thread = new Thread(async () =>
                {
                    try
                    {
                        while (!_isDisposing)
                        {
                            _parser.Parse();

                            if (_parser.ReadFailed)
                            {
                                throw new EndOfStreamException("IDP input stream closed.");
                            }

                            await Task.Delay(1);
                        }
                    }
                    catch (Exception e)
                    {
                        ConnectionError?.Invoke(this, EventArgs.Empty);
                    }
                });

                thread.Priority = ThreadPriority.AboveNormal;
                thread.Start();
            }
        }

        public override bool Transmit(IdpPacket packet)
        {
            if (!_isDisposing && OutputStream != null)
            {
                try
                {
                    OutputStream.Write(packet.Data, 0, packet.Data.Length);
                    OutputStream.Flush();
                    TraceSink?.Trace(CreateTracePacket(IdpTrafficDirection.Outbound, packet));
                }
                catch (Exception e)
                {
                    ConnectionError?.Invoke(this, EventArgs.Empty);
                    return false;
                }


                return true;
            }

            return false;
        }

        private static IdpTracePacket CreateTracePacket(IdpTrafficDirection direction, IdpPacket packet)
        {
            return new IdpTracePacket
            {
                TimestampUtc = DateTime.UtcNow,
                Direction = direction,
                Source = packet.Source,
                Destination = packet.Destination,
                RawPacketBytes = (byte[])packet.Data.Clone(),
                Packet = packet
            };
        }
    }
}

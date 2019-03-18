// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using System;

namespace IdpProtocol
{
    public abstract class IdpAdaptor : IPacketTransmit
    {
        private bool _isActive;
        private IdpParser _idpParser;

        public IdpAdaptor()
        {
            Id = 0;
        }

        internal bool IsEnumerated { get; set; }

        internal bool IsRenumerated { get; set; }

        public bool IsActive
        {
            get => _isActive;
            set
            {
                if(value != _isActive)
                {
                    _isActive = value;

                    if(value)
                    {
                        IsEnumerated = false;
                    }
                }
            }
        }

        internal UInt16 Id { get; set; }

        internal IAdaptorToRouterPort LocalPort { private get; set; }

        public abstract bool Transmit(IdpPacket packet);

        protected void OnReceive(IdpPacket packet)
        {
            if (Id != 0 && LocalPort != null)
            {
                LocalPort.Transmit(Id, packet);
            }
        }
    }
}

// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using System;

namespace IdpProtocol
{
    public class InterfaceFoundEventArgs
    {
        public InterfaceFoundEventArgs(UInt16 vid, UInt16 pid, UInt16 address)
        {
            Vid = vid;
            Pid = pid;
            RemoteAddress = address;
        }

        public UInt16 Vid { get; }
        public UInt16 Pid { get; }

        public UInt16 RemoteAddress { get; }
    }
}

// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for full license information.

using System;

namespace IdpProtocol
{
    public class IdpResponse
    {
        public IdpResponseCode ResponseCode { get; }
        public UInt16 ResponseId { get; }

        public IdpResponse(IncomingTransaction incomingTransaction)
        {
            Transaction = incomingTransaction;

            ResponseCode = incomingTransaction.Read<IdpResponseCode>();

            ResponseId = incomingTransaction.Read<UInt16>();
        }

        public UInt32 TransactionId => Transaction.TransactionId;

        public IncomingTransaction Transaction { get; }
    }
}

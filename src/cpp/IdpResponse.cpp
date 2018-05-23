// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "IdpResponse.h"

IdpResponse::IdpResponse (
    std::shared_ptr<IncomingTransaction> incomingTransaction)
{
    _transaction = incomingTransaction;

    _responseCode = (IdpResponseCode) incomingTransaction->Read<uint8_t> ();

    _responseId = incomingTransaction->Read<uint16_t> ();
}

IdpResponseCode IdpResponse::ResponseCode ()
{
    return _responseCode;
}

uint16_t IdpResponse::ResponseId ()
{
    return _responseId;
}

uint32_t IdpResponse::TransactionId ()
{
    return Transaction ()->TransactionId ();
}

std::shared_ptr<IncomingTransaction> IdpResponse::Transaction ()
{
    return _transaction;
}

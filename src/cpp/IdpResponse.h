// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include "IncomingTransaction.h"

/**
 *  IdpResponse
 */
class IdpResponse
{
  private:
    IdpResponseCode _responseCode;
    uint16_t _responseId;
    std::shared_ptr<IncomingTransaction> _transaction;

  public:
    IdpResponse (std::shared_ptr<IncomingTransaction> incomingTransaction);

    IdpResponseCode ResponseCode ();

    uint16_t ResponseId ();

    uint32_t TransactionId ();

    std::shared_ptr<IncomingTransaction> Transaction ();
};

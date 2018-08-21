// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include "IStream.h"
#include "IdpResponse.h"
#include "IncomingTransaction.h"
#include "OutgoingTransaction.h"
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <stdint.h>

typedef std::function<IdpResponseCode (
    std::shared_ptr<IncomingTransaction> incomingTransaction,
    std::shared_ptr<OutgoingTransaction> outgoingTransaction)>
    CommandHandler;

typedef std::function<void(std::shared_ptr<IdpResponse> response)>
    ResponseHandler;

typedef struct
{
    ResponseHandler handler;
    uint64_t expiry;
} ResponseHandlerTimeout;

/**
 *  IdpCommandManager
 */
class IdpCommandManager
{
  public:
    /**
     * Instantiates a new instance of IdpCommandManager
     */
    IdpCommandManager ();
    ~IdpCommandManager ();

    void RegisterCommand (uint16_t commandId, CommandHandler handler);

    void RegisterOneTimeResponseHandler (uint32_t transactionId,
                                         ResponseHandler handler,
                                         uint32_t timeoutMs = 1750);

    void UnregisterOneTimeResponseHandler (uint32_t transactionId);

    void RegisterResponseHandler (uint16_t commandId, ResponseHandler handler);

    std::shared_ptr<IdpPacket>
        ProcessPayload (uint16_t nodeAddress,
                        std::shared_ptr<IdpPacket> packet);

  private:
    void InvalidateTimeouts ();

    std::map<uint16_t, CommandHandler> _commandHandlers;
    std::map<uint32_t, ResponseHandlerTimeout> _transactionHandlers;
    std::map<uint16_t, ResponseHandler> _responseHandlers;
};

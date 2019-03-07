// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "IdpCommandManager.h"
#include "Application.h"
#include "DispatcherTimer.h"
#include "IdpPacket.h"
#include "IdpResponse.h"
#include <unistd.h>

IdpCommandManager::IdpCommandManager ()
{
    RegisterCommand (
        0xA000, [&](std::shared_ptr<IncomingTransaction> incoming,
                    std::shared_ptr<OutgoingTransaction> outgoing) {
            auto response =
                std::shared_ptr<IdpResponse> (new IdpResponse (incoming));

            auto it = _transactionHandlers.find (response->TransactionId ());

            if (it != _transactionHandlers.end ())
            {
                auto current = _transactionHandlers[response->TransactionId ()];

                _transactionHandlers.erase (it);

                current.handler (response);
            }
            else
            {
                auto it = _responseHandlers.find (response->ResponseId ());

                if (it != _responseHandlers.end ())
                {
                    auto current = it->second;

                    current (response);
                }


                return IdpResponseCode::OK;
            }
        });

    auto& pollTimer = *new DispatcherTimer (10);

    pollTimer.Tick +=
        [&](auto sender, auto& e) { this->InvalidateTimeouts (); };

    pollTimer.Start ();
}

IdpCommandManager::~IdpCommandManager ()
{
}

void IdpCommandManager::InvalidateTimeouts ()
{
    auto currentTime = Application::GetApplicationTime ();

    auto it = _transactionHandlers.begin ();

    while (it != _transactionHandlers.end ())
    {
        auto current = it->second;

        if (current.expiry < currentTime)
        {
            it = _transactionHandlers.erase (it);

            current.handler (std::shared_ptr<IdpResponse> (nullptr));
        }
        else
        {
            ++it;
        }
    }
}


void IdpCommandManager::RegisterOneTimeResponseHandler (uint32_t transactionId,
                                                        ResponseHandler handler,
                                                        uint32_t timeoutMs)
{
    auto currentTime = Application::GetApplicationTime ();

    auto handlerInfo = ResponseHandlerTimeout ();
    handlerInfo.handler = handler;
    handlerInfo.expiry = currentTime + timeoutMs;
    _transactionHandlers[transactionId] = handlerInfo;
}

void IdpCommandManager::UnregisterOneTimeResponseHandler (
    uint32_t transactionId)
{
    auto it = _transactionHandlers.find (transactionId);

    if (it != _transactionHandlers.end ())
    {
        _transactionHandlers.erase (it);
    }
}

void IdpCommandManager::RegisterResponseHandler (uint16_t commandId,
                                                 ResponseHandler handler)
{
    _responseHandlers[commandId] = handler;
}

void IdpCommandManager::RegisterCommand (uint16_t commandId,
                                         CommandHandler handler)
{
    _commandHandlers[commandId] = handler;
}

std::shared_ptr<IdpPacket>
    IdpCommandManager::ProcessPayload (uint16_t nodeAddress,
                                       std::shared_ptr<IdpPacket> packet)
{
    std::shared_ptr<IncomingTransaction> incomingTransaction (
        new IncomingTransaction (packet));

    auto& incoming = *incomingTransaction;

    auto outgoingTransaction = OutgoingTransaction::Create (
        0xA000, incoming.TransactionId (), IdpCommandFlags::None);

    auto& outgoing = *outgoingTransaction;

    if (_commandHandlers.find (incoming.CommandId ()) !=
        _commandHandlers.end ())
    {
        outgoing.Write ((uint8_t) IdpResponseCode::OK);
        outgoing.Write (incoming.CommandId ());

        auto responseCode = _commandHandlers[incoming.CommandId ()](
            incomingTransaction, outgoingTransaction);

        if ((uint8_t) incoming.Flags () &
                (uint8_t) IdpCommandFlags::ResponseExpected &&
            responseCode != IdpResponseCode::Deferred)
        {
            outgoing.WithResponseCode (responseCode);

            return outgoing.ToPacket (nodeAddress, packet->Source ());
        }
    }
    else
    {
        outgoing.Write ((uint8_t) IdpResponseCode::UnknownCommand);
        outgoing.Write (incoming.CommandId ());

        return outgoing.ToPacket (nodeAddress, packet->Source ());
    }

    return nullptr;
}

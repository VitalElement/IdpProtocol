// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "OutgoingTransaction.h"

OutgoingTransaction::OutgoingTransaction (uint16_t commandId,
                                          uint32_t transactionId,
                                          IdpCommandFlags flags)
{
    _writeIndex = 0;
    _commandId = commandId;
    _transactionId = transactionId;
}

OutgoingTransaction::~OutgoingTransaction ()
{
}

std::shared_ptr<OutgoingTransaction>
    OutgoingTransaction::Create (uint16_t commandId, uint32_t transactionId,
                                 IdpCommandFlags flags)
{
    auto result = new OutgoingTransaction (commandId, transactionId, flags);

    auto ptrResult = std::shared_ptr<OutgoingTransaction> (result);

    ptrResult->Write (commandId);
    ptrResult->Write (transactionId);
    ptrResult->Write ((uint8_t) flags);

    return ptrResult;
}

uint16_t OutgoingTransaction::CommandId ()
{
    return _commandId;
}

uint32_t OutgoingTransaction::TransactionId ()
{
    return _transactionId;
}

std::shared_ptr<IdpPacket> OutgoingTransaction::ToPacket (uint16_t source,
                                                          uint16_t destination)
{
    auto result = std::shared_ptr<IdpPacket> (
        new IdpPacket (_data.size (), IdpFlags::None, source, destination));

    result->Write (_data.data (), _data.size ());

    result->Seal ();

    return result;
}

std::shared_ptr<OutgoingTransaction>
    OutgoingTransaction::WithResponseCode (IdpResponseCode responseCode)
{
    WriteAt ((uint8_t) responseCode, 7);

    return shared_from_this ();
}

std::shared_ptr<OutgoingTransaction>
    OutgoingTransaction::WriteAt (void* data, uint32_t length, uint32_t index)
{
    uint8_t* ptr = (uint8_t*) data;

    for (uint32_t i = 0; i < length; i++)
    {
        _data[index + i] = ptr[i];
    }

    return shared_from_this ();
}

std::shared_ptr<OutgoingTransaction>
    OutgoingTransaction::Write (void* data, uint32_t length)
{
    uint8_t* ptr = (uint8_t*) data;

    for (uint32_t i = 0; i < length; i++)
    {
        _data.push_back (ptr[i]);
    }

    _writeIndex += length;

    return shared_from_this ();
}

std::shared_ptr<OutgoingTransaction>
    OutgoingTransaction::WriteGuid (Guid_t& guid)
{
    Write (guid.Data1)
        ->Write (guid.Data2)
        ->Write (guid.Data3)
        ->Write (guid.Data4, 8);

    return shared_from_this ();
}

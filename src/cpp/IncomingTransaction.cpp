// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "IncomingTransaction.h"

IncomingTransaction::IncomingTransaction (std::shared_ptr<IdpPacket> packet)
{
    _packet = packet;

    _data = packet->Buffer ();
    _readIndex = 10;

    _commandId = Read<uint16_t> ();
    _transactionId = Read<uint32_t> ();
    _flags = (IdpCommandFlags) Read<uint8_t> ();

    _source = packet->Source ();
    _destination = packet->Destination ();
}

IncomingTransaction::~IncomingTransaction ()
{
}

std::shared_ptr<IdpPacket> IncomingTransaction::Packet ()
{
    return _packet;
}

IdpCommandFlags IncomingTransaction::Flags ()
{
    return _flags;
}

uint16_t IncomingTransaction::CommandId ()
{
    return _commandId;
}

uint32_t IncomingTransaction::TransactionId ()
{
    return _transactionId;
}

uint16_t IncomingTransaction::Source ()
{
    return _source;
}

uint16_t IncomingTransaction::Destination ()
{
    return _destination;
}

const char* IncomingTransaction::ReadCString ()
{
    // todo make this safe.
    auto length = strlen ((const char*) (_data.get () + _readIndex)) + 1;

    auto result = new char[length];

    Read (result, length);

    return result;
}

Guid_t IncomingTransaction::ReadGuid ()
{
    auto data1 = Read<uint32_t> ();
    auto data2 = Read<uint16_t> ();
    auto data3 = Read<uint16_t> ();
    uint8_t data4[8];
    Read (data4, 8);

    return Guid_t (data1, data2, data3, data4);
}

void IncomingTransaction::Read (void* destination, uint32_t length)
{
    memcpy (destination, (void*) (_data.get () + _readIndex), length);
    _readIndex += length;
}

void* IncomingTransaction::ConsumeData(uint32_t length)
{
    void* data = (void *) (_data.get() + _readIndex);
    _readIndex += length;

    return data;
}

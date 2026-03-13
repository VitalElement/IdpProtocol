// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "IncomingTransaction.h"
#include <cstring>

IncomingTransaction::IncomingTransaction (std::shared_ptr<IdpPacket> packet)
{
    _packet = packet;

    _data = packet->Buffer ();
    _readIndex = 10;
    _readLimit = packet->Length ();

    if (((uint8_t) packet->Flags () & (uint8_t) IdpFlags::CRC) ==
        (uint8_t) IdpFlags::CRC)
    {
        if (_readLimit < 4)
        {
            ThrowException (-1, "Invalid IDP packet length");
        }

        _readLimit -= 4;
    }

    if (_readLimit < 11)
    {
        ThrowException (-1, "Invalid IDP packet length");
    }

    // Exclude ETX from transaction payload reads.
    _readLimit -= 1;

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
    if (_readIndex > _readLimit)
    {
        ThrowException (-1, "IDP packet read out of bounds");
    }

    auto remaining = _readLimit - _readIndex;
    auto terminator = static_cast<const char*> (
        memchr (_data.get () + _readIndex, '\0', remaining));

    if (terminator == nullptr)
    {
        ThrowException (-1, "Unterminated string in IDP packet");
    }

    auto length = (uint32_t) (terminator - (const char*) (_data.get () + _readIndex)) + 1;

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
    EnsureReadable (length);
    memcpy (destination, (void*) (_data.get () + _readIndex), length);
    _readIndex += length;
}

void* IncomingTransaction::ConsumeData(uint32_t length)
{
    EnsureReadable (length);
    void* data = (void *) (_data.get() + _readIndex);
    _readIndex += length;

    return data;
}

void IncomingTransaction::EnsureReadable (uint32_t length)
{
    if (_readIndex > _readLimit || length > (_readLimit - _readIndex))
    {
        ThrowException (-1, "IDP packet read out of bounds");
    }
}

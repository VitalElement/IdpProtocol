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
    _readFault = false;

    if (((uint8_t) packet->Flags () & (uint8_t) IdpFlags::CRC) ==
        (uint8_t) IdpFlags::CRC)
    {
        if (_readLimit < 4)
        {
            _readFault = true;
            _readLimit = 0;
        }
        else
        {
            _readLimit -= 4;
        }
    }

    if (_readLimit < 11)
    {
        _readFault = true;
        _readLimit = 10;
    }
    else
    {
        // Exclude ETX from transaction payload reads.
        _readLimit -= 1;
    }

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
    if (!EnsureReadable (1))
    {
        auto result = new char[1];
        result[0] = 0;
        return result;
    }

    auto remaining = _readLimit - _readIndex;
    auto terminator = static_cast<const char*> (
        memchr (_data.get () + _readIndex, '\0', remaining));

    if (terminator == nullptr)
    {
        _readFault = true;
        auto result = new char[1];
        result[0] = 0;
        return result;
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
    if (!EnsureReadable (length))
    {
        memset (destination, 0, length);
        return;
    }

    memcpy (destination, (void*) (_data.get () + _readIndex), length);
    _readIndex += length;
}

void* IncomingTransaction::ConsumeData(uint32_t length)
{
    if (!EnsureReadable (length))
    {
        return nullptr;
    }

    void* data = (void *) (_data.get() + _readIndex);
    _readIndex += length;

    return data;
}

bool IncomingTransaction::EnsureReadable (uint32_t length)
{
    if (_readIndex > _readLimit || length > (_readLimit - _readIndex))
    {
        _readFault = true;
        _readIndex = _readLimit;
        return false;
    }

    return true;
}

bool IncomingTransaction::HasReadFault ()
{
    return _readFault;
}

uint32_t IncomingTransaction::Remaining ()
{
    if (_readIndex > _readLimit)
    {
        return 0;
    }

    return _readLimit - _readIndex;
}

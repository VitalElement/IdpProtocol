// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "IdpPacket.h"

IdpPacket::IdpPacket (uint32_t payloadLength, IdpFlags flags, uint16_t source,
                      uint16_t destination, bool sealed)
{
    _writeIndex = 0;
    _readIndex = 0;
    _isSealed = false;

    auto length = 11 + payloadLength;

    if (((uint8_t) flags & (uint8_t) IdpFlags::CRC) == (uint8_t) IdpFlags::CRC)
    {
        length += 4;
    }

    _buffer = std::shared_ptr<uint8_t> (new uint8_t[length],
                                        std::default_delete<uint8_t[]> ());

    Write ((uint8_t) 0x02);
    Write (length);
    Write ((uint8_t) flags);
    Write (source);
    Write (destination);

    _isSealed = sealed;
}

IdpFlags IdpPacket::Flags ()
{
    return (IdpFlags) Data ()[5];
}

uint16_t IdpPacket::Source ()
{
    uint16_t result;

    memcpy (&result, (const void*) (Data () + 6), sizeof (uint16_t));

    if (BitConverter::IsLittleEndian ())
    {
        result = BitConverter::SwapEndian (result);
    }

    return result;
}

uint16_t IdpPacket::Destination ()
{
    uint16_t result;

    memcpy (&result, (const void*) (Data () + 8), sizeof (uint16_t));

    if (BitConverter::IsLittleEndian ())
    {
        result = BitConverter::SwapEndian (result);
    }

    return result;
}

void IdpPacket::Seal ()
{
    Write ((uint8_t) 0x03);

    if (((uint8_t) Flags () & (uint8_t) IdpFlags::CRC) ==
        (uint8_t) IdpFlags::CRC)
    {
        // TODO write CRC.
        Write ((uint32_t) 0xAA55AA55);
    }

    _isSealed = true;
}

void IdpPacket::Write (const void* data, uint32_t length)
{
    memcpy (_buffer.get () + _writeIndex, data, length);

    _writeIndex += length;
}

uint8_t* IdpPacket::Data ()
{
    return _buffer.get ();
}

std::shared_ptr<uint8_t> IdpPacket::Buffer ()
{
    return _buffer;
}

uint8_t* IdpPacket::Payload ()
{
    return _buffer.get () + 10;
}

uint8_t* IdpPacket::WritePointer ()
{
    return _buffer.get () + _writeIndex;
}

void IdpPacket::IncrementWritePointer (uint32_t length)
{
    _writeIndex += length;
}

uint32_t IdpPacket ::Length ()
{
    uint32_t result;

    memcpy (&result, (const void*) (Data () + 1), sizeof (uint32_t));

    if (BitConverter::IsLittleEndian ())
    {
        result = BitConverter::SwapEndian (result);
    }

    return result;
}

uint32_t IdpPacket::PayloadLength ()
{
    uint32_t length = Length () - 11;

    if (((uint8_t) Flags () & (uint8_t) IdpFlags::CRC) ==
        (uint8_t) IdpFlags::CRC)
    {
        length -= 4;
    }

    return length;
}

void IdpPacket::ResetRead ()
{
    _readIndex = 0;
}

void IdpPacket::ResetReadToPayload ()
{
    _readIndex = 10;
}

IdpPacket::~IdpPacket ()
{
}

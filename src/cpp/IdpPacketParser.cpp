// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "IdpPacketParser.h"
#include "BitConverter.h"
#include "DataReceivedEventArgs.h"

IdpPacketParser::IdpPacketParser ()
{
    Reset ();

    _pollTimer = std::unique_ptr<DispatcherTimer> (new DispatcherTimer (2));

    _pollTimer->Tick += [&](auto sender, auto& e) {
        this->Parse();
    };
}

void IdpPacketParser::Parse ()
{
    if (Stream ().IsValid ())
    {
        while ((this->*_currentState) ())
        {
        }
    }
    else
    {
        this->Reset ();
    }
}

IdpPacketParser::~IdpPacketParser ()
{
}

void IdpPacketParser::Reset ()
{
    _currentPacket = nullptr;
    _currentPacketHasCRC = false;
    _currentPacketLength = 0;
    _currentState = &IdpPacketParser::WaitingForStx;
}

uint32_t IdpPacketParser::PayloadLength ()
{
    return _currentPacketLength - (_currentPacketHasCRC ? 15 : 11);
}

void IdpPacketParser::Start ()
{
    if (_stream != nullptr)
    {
        _pollTimer->Start ();
    }
}

void IdpPacketParser::Stop ()
{
    _pollTimer->Stop ();
}

IStream& IdpPacketParser::Stream ()
{
    return *_stream;
}

void IdpPacketParser::Stream (IStream* value)
{
    _stream = value;
    Reset();
}

bool IdpPacketParser::WaitingForStx ()
{
    uint8_t data;

    if (_stream->TryRead (data) && data == 0x02)
    {
        _currentState = &IdpPacketParser::ReadingLength;

        return true;
    }

    return false;
}

bool IdpPacketParser::ReadingLength ()
{
    if (_stream->TryRead (_currentPacketLength))
    {
        if (BitConverter::IsLittleEndian ())
        {
            _currentPacketLength =
                BitConverter::SwapEndian (_currentPacketLength);
        }

        if (_currentPacketLength > 1000000)
        {
            Reset ();
            return false;
        }
        _currentState = &IdpPacketParser::ReadingFlags;

        return true;
    }

    return false;
}

bool IdpPacketParser::ReadingFlags ()
{
    uint8_t data;

    if (_stream->TryRead (data))
    {
        _currentPacketHasCRC = data & 0x01;

        _currentPacketFlags = (IdpFlags) data;

        _currentState = &IdpPacketParser::ReadingSource;

        return true;
    }

    return false;
}

bool IdpPacketParser::ReadingSource ()
{
    uint16_t data;

    if (_stream->TryRead (data))
    {
        _currentPacketSource = data;

        if (BitConverter::IsLittleEndian ())
        {
            _currentPacketSource =
                BitConverter::SwapEndian (_currentPacketSource);
        }

        _currentState = &IdpPacketParser::ReadingDestination;
        return true;
    }

    return false;
}

bool IdpPacketParser::ReadingDestination ()
{
    uint16_t data;

    if (_stream->TryRead (data))
    {
        if (BitConverter::IsLittleEndian ())
        {
            data = BitConverter::SwapEndian (data);
        }

        _currentPacket = std::shared_ptr<IdpPacket> (
            new IdpPacket (_currentPacketLength - 11, _currentPacketFlags,
                           _currentPacketSource, data, false));

        _currentState = &IdpPacketParser::WaitingForPayload;
        return true;
    }

    return false;
}

bool IdpPacketParser::WaitingForPayload ()
{
    if (_stream->TryRead (_currentPacket->WritePointer (), PayloadLength ()))
    {
        _currentPacket->IncrementWritePointer (PayloadLength ());

        _currentState = &IdpPacketParser::WaitingForEtx;

        return true;
    }

    return false;
}

bool IdpPacketParser::WaitingForEtx ()
{
    uint8_t data;

    if (_stream->TryRead (data))
    {
        if (data == 0x03)
        {
            _currentPacket->Write (data);

            if (_currentPacketHasCRC)
            {
                _currentState = &IdpPacketParser::ReadingCRC;
            }
            else
            {
                _currentState = &IdpPacketParser::Validating;
            }

            return true;
        }
        else
        {
            Reset ();
        }
    }

    return false;
}

bool IdpPacketParser::ReadingCRC ()
{
    if (_stream->TryRead (_currentPacketCRC))
    {
        _currentPacket->Write (_currentPacketCRC);

        _currentState = &IdpPacketParser::Validating;

        return true;
    }

    return false;
}

bool IdpPacketParser::Validating ()
{
    bool isValid = !_currentPacketHasCRC;

    if (_currentPacketHasCRC)
    {
        // TODO validate CRC
    }

    if (isValid)
    {
        DataReceived (this, new DataReceivedEventArgs (_currentPacket));
    }

    Reset ();

    return false;
}

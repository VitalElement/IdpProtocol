// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include <memory>
#include <stdint.h>

#include "DataReceivedEventArgs.h"
#include "Dispatcher.h"
#include "Event.h"
#include "IStream.h"
#include "IdpPacket.h"

/**
 *  IdpPacketParser
 */
class IdpPacketParser
{
    typedef bool (IdpPacketParser::*IDPState) ();

  public:
    /**
     * Instantiates a new instance of IdpPacketParser
     */
    IdpPacketParser ();
    ~IdpPacketParser ();

    void Start ();
    void Stop ();

    Event DataReceived;
    Event PacketError;

    IStream& Stream ();
    void Stream (IStream& value);

    void Parse ();

  private:
    uint32_t _currentPacketLength;
    bool _currentPacketHasCRC;
    uint32_t _currentPacketCRC;
    IdpFlags _currentPacketFlags;
    uint16_t _currentPacketSource;

    IStream* _stream;
    std::unique_ptr<DispatcherTimer> _pollTimer;
    std::shared_ptr<IdpPacket> _currentPacket;

    IDPState _currentState;

    uint32_t PayloadLength ();

    void Reset ();
    bool WaitingForStx ();
    bool ReadingLength ();
    bool ReadingFlags ();
    bool ReadingSource ();
    bool ReadingDestination ();
    bool WaitingForPayload ();
    bool WaitingForEtx ();
    bool ReadingCRC ();
    bool Validating ();
};

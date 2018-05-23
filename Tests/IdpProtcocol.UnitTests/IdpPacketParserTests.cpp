// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "catch.hpp"

#include "DataReceivedEventArgs.h"
#include "IdpPacket.h"
#include "IdpPacketParser.h"
#include "TestRuntime.h"
#include "TestStream.h"

#include "DispatcherTimer.h"

TEST_CASE ("Can parse a minimal packet with 1byte payload")
{
    TestRuntime::Initialise ();

    auto& parser = *new IdpPacketParser ();

    auto parserEndPointStream = new TestStream ();

    auto originatorEndPointStream = parserEndPointStream->GetEndpoint ();

    parser.Stream (*parserEndPointStream);

    auto packet = new IdpPacket (1, IdpFlags::None);

    uint8_t payloadValue = 0xAA;
    uint32_t payloadLength = 1;

    packet->Write (payloadValue);

    payloadValue = 0x55;
    packet->Seal ();

    originatorEndPointStream->Write (packet->Data (), packet->Length ());

    delete packet;
    packet = nullptr;

    parser.Start ();

    parser.DataReceived += [&](auto sender, auto& e) {
        auto& args = static_cast<DataReceivedEventArgs&> (e);

        payloadValue = args.Packet->Payload ()[0];
        payloadLength = args.Packet->PayloadLength ();
    };

    parser.Parse ();

    TestRuntime::IterateRuntime ();

    TestRuntime::IterateRuntime (20);

    REQUIRE (payloadValue == 0xAA);
    REQUIRE (payloadLength == 1);
}

TEST_CASE ("Can reconstruct a packet including ETX")
{
    TestRuntime::Initialise ();

    auto& parser = *new IdpPacketParser ();

    auto parserEndPointStream = new TestStream ();

    auto originatorEndPointStream = parserEndPointStream->GetEndpoint ();

    parser.Stream (*parserEndPointStream);

    auto packet = new IdpPacket (1, IdpFlags::None);

    uint8_t payloadValue = 0xAA;
    uint32_t payloadLength = 1;
    uint8_t lastByte = 0;

    packet->Write (payloadValue);

    payloadValue = 0x55;
    packet->Seal ();

    originatorEndPointStream->Write (packet->Data (), packet->Length ());

    delete packet;
    packet = nullptr;

    parser.Start ();

    parser.DataReceived += [&](auto sender, auto& e) {
        auto& args = static_cast<DataReceivedEventArgs&> (e);

        payloadValue = args.Packet->Payload ()[0];
        payloadLength = args.Packet->PayloadLength ();
        lastByte = *(args.Packet->Data () + (args.Packet->Length () - 1));
    };

    parser.Parse ();

    TestRuntime::IterateRuntime ();

    TestRuntime::IterateRuntime (20);

    REQUIRE (lastByte == 0x03);

    delete packet;
    packet = nullptr;
}

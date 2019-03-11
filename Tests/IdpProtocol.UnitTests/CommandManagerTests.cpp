// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.

#include "catch.hpp"

#include "IStream.h"
#include "IdpCommandManager.h"
#include "IdpPacketParser.h"
#include "TestRuntime.h"
#include "TestStream.h"

static std::shared_ptr<IdpPacket> ProcessPayload (IdpCommandManager& manager,
                                                  uint8_t* buffer,
                                                  uint32_t length)
{
    // Create an input stream with a message.
    auto stream = std::shared_ptr<TestStream> (new TestStream (1024));

    auto packet =
        std::shared_ptr<IdpPacket> (new IdpPacket (length, IdpFlags::None));

    packet->Write (buffer, length);

    packet->Seal ();

    return manager.ProcessPayload (0, packet);
}

static IncomingTransaction& GetTransaction (std::shared_ptr<IdpPacket> response)
{
    return *new IncomingTransaction (response);
}

static IdpCommandManager& GetManager ()
{
    return *new IdpCommandManager ();
}

static IncomingTransaction& TestTransaction (IdpCommandManager& manager,
                                             uint8_t* buffer, uint32_t length)
{
    auto received = ProcessPayload (manager, buffer, length);

    REQUIRE ((received != nullptr));

    auto& idpResponse = GetTransaction (received);

    return idpResponse;
}

TEST_CASE ("Command Manager responds with unknown command when unknown command "
           "is received.")
{
    TestRuntime::Initialise ();

    uint8_t data[] = { 0xA0,
                       0x01,
                       0x00,
                       0x00,
                       0x00,
                       0x01,
                       (uint8_t) IdpCommandFlags::ResponseExpected,
                       0x55 };

    auto& manager = GetManager ();
    auto& idpResponse = TestTransaction (manager, data, sizeof (data));

    REQUIRE (idpResponse.CommandId () == 0xA000);
    REQUIRE (idpResponse.Flags () == IdpCommandFlags::None);

    auto responseCode = (IdpResponseCode) idpResponse.Read<uint8_t> ();
    auto responseId = idpResponse.Read<uint16_t> ();

    REQUIRE (responseCode == IdpResponseCode::UnknownCommand);
    REQUIRE (responseId == 0xA001);

    delete &manager;
    delete &idpResponse;
}

TEST_CASE ("Command Manager responds with unknown command when unknown command "
           "is received and request doesnt set ResponseExpected")
{
    TestRuntime::Initialise ();

    uint8_t data[] = {
        0xA0, 0x01, 0x00, 0x00, 0x00, 0x01, (uint8_t) IdpCommandFlags::None,
        0x55
    };

    auto& manager = GetManager ();
    auto& idpResponse = TestTransaction (manager, data, sizeof (data));

    REQUIRE (idpResponse.CommandId () == 0xA000);
    REQUIRE (idpResponse.Flags () == IdpCommandFlags::None);

    auto responseCode = (IdpResponseCode) idpResponse.Read<uint8_t> ();
    auto responseId = idpResponse.Read<uint16_t> ();

    REQUIRE (responseCode == IdpResponseCode::UnknownCommand);
    REQUIRE (responseId == 0xA001);

    delete &manager;
    delete &idpResponse;
}

TEST_CASE ("Command Manager responds with OK when a known command is received.")
{
    TestRuntime::Initialise ();

    uint8_t data[] = { 0xA0,
                       0x01,
                       0x00,
                       0x00,
                       0x00,
                       0x01,
                       (uint8_t) IdpCommandFlags::ResponseExpected,
                       0x55 };

    auto& manager = GetManager ();

    manager.RegisterCommand (
        0xA001, [&](std::shared_ptr<IncomingTransaction> incomingTransaction,
                    std::shared_ptr<OutgoingTransaction> outgoingTransaction) {
            auto& incoming = *incomingTransaction;
            auto& outgoing = *outgoingTransaction;

            return IdpResponseCode::OK;
        });

    auto& idpResponse = TestTransaction (manager, data, sizeof (data));

    REQUIRE (idpResponse.CommandId () == 0xA000);
    REQUIRE (idpResponse.Flags () == IdpCommandFlags::None);

    auto responseCode = (IdpResponseCode) idpResponse.Read<uint8_t> ();
    auto responseId = idpResponse.Read<uint16_t> ();

    REQUIRE (responseCode == IdpResponseCode::OK);
    REQUIRE (responseId == 0xA001);

    delete &manager;
    delete &idpResponse;
}

TEST_CASE ("Command Manager doesnt respond if a known command doesnt set "
           "Response Expected")
{
    TestRuntime::Initialise ();

    uint8_t data[] = {
        0xA0, 0x01, 0x00, 0x00, 0x00, 0x01, (uint8_t) IdpCommandFlags::None,
        0x55
    };

    auto& manager = GetManager ();

    bool handlerCalled = false;

    manager.RegisterCommand (
        0xA001, [&](std::shared_ptr<IncomingTransaction> incomingTransaction,
                    std::shared_ptr<OutgoingTransaction> outgoingTransaction) {
            auto& incoming = *incomingTransaction;
            auto& outgoing = *outgoingTransaction;
            handlerCalled = true;
            return IdpResponseCode::OK;
        });

    auto received = ProcessPayload (manager, data, sizeof (data));

    REQUIRE (received == nullptr);
    REQUIRE (handlerCalled);

    delete &manager;
}

TEST_CASE ("Command can read parameters from incoming stream")
{
    TestRuntime::Initialise ();

    uint8_t data[] = { 0xA0,
                       0x01,
                       0x00,
                       0x00,
                       0x00,
                       0x01,
                       (uint8_t) IdpCommandFlags::ResponseExpected,
                       0xAA,
                       0x55,
                       0xAA,
                       0x55 };

    uint32_t incomingParameter = 0;

    auto& manager = GetManager ();

    manager.RegisterCommand (
        0xA001, [&](std::shared_ptr<IncomingTransaction> incomingTransaction,
                    std::shared_ptr<OutgoingTransaction> outgoingTransaction) {
            auto& incoming = *incomingTransaction;
            auto& outgoing = *outgoingTransaction;

            incomingParameter = incoming.Read<uint32_t> ();
            return IdpResponseCode::OK;
        });

    auto& idpResponse = TestTransaction (manager, data, sizeof (data));

    REQUIRE (incomingParameter == 0xAA55AA55);

    delete &manager;
    delete &idpResponse;
}

TEST_CASE ("Command can write parameters to response stream")
{
    TestRuntime::Initialise ();

    uint8_t data[] = { 0xA0,
                       0x01,
                       0x00,
                       0x00,
                       0x00,
                       0x01,
                       (uint8_t) IdpCommandFlags::ResponseExpected,
                       0x55 };

    auto& manager = GetManager ();

    manager.RegisterCommand (
        0xA001, [&](std::shared_ptr<IncomingTransaction> incomingTransaction,
                    std::shared_ptr<OutgoingTransaction> outgoingTransaction) {
            auto& incoming = *incomingTransaction;
            auto& outgoing = *outgoingTransaction;
            outgoing.Write<uint32_t> (0xAA55AA55);
            return IdpResponseCode::OK;
        });

    auto& idpResponse = TestTransaction (manager, data, sizeof (data));

    REQUIRE (idpResponse.CommandId () == 0xA000);
    REQUIRE (idpResponse.Flags () == IdpCommandFlags::None);

    auto responseCode = (IdpResponseCode) idpResponse.Read<uint8_t> ();
    auto responseId = idpResponse.Read<uint16_t> ();

    REQUIRE (responseCode == IdpResponseCode::OK);
    REQUIRE (responseId == 0xA001);

    auto parameter = idpResponse.Read<uint32_t> ();

    REQUIRE (parameter == 0xAA55AA55);

    delete &manager;
    delete &idpResponse;
}

TEST_CASE ("Command can give invalid response")
{
    TestRuntime::Initialise ();

    uint8_t data[] = { 0xA0,
                       0x01,
                       0x00,
                       0x00,
                       0x00,
                       0x01,
                       (uint8_t) IdpCommandFlags::ResponseExpected,
                       0x55 };

    auto& manager = GetManager ();

    manager.RegisterCommand (
        0xA001, [&](std::shared_ptr<IncomingTransaction> incomingTransaction,
                    std::shared_ptr<OutgoingTransaction> outgoingTransaction) {
            auto& incoming = *incomingTransaction;
            auto& outgoing = *outgoingTransaction;
            return IdpResponseCode::InvalidParameters;
        });

    auto& idpResponse = TestTransaction (manager, data, sizeof (data));

    auto responseCode = (IdpResponseCode) idpResponse.Read<uint8_t> ();

    REQUIRE (responseCode == IdpResponseCode::InvalidParameters);

    delete &manager;
    delete &idpResponse;
}

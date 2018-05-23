// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "IdpNode.h"
#include "Application.h"

IdpNode::IdpNode (Guid_t guid, const char* name, uint16_t address)
{
    _commandManager = new IdpCommandManager ();
    _address = address;
    _transmitEndpoint = nullptr;
    _enabled = true;
    _currentTransactionId = 1;

    _guid = guid;

    _name = name;
    _lastPing = 0;

    Manager ().RegisterCommand (
        static_cast<uint16_t> (NodeCommand::Ping),
        [&](std::shared_ptr<IncomingTransaction> incoming,
            std::shared_ptr<OutgoingTransaction> outgoing) {
            _lastPing = Application::GetApplicationTime ();

            return IdpResponseCode::OK;
        });

    Manager ().RegisterCommand (
        static_cast<uint16_t> (NodeCommand::GetNodeInfo),
        [&](std::shared_ptr<IncomingTransaction> incoming,
            std::shared_ptr<OutgoingTransaction> outgoing) {
            outgoing->WriteGuid (_guid);

            outgoing->Write ((void*) _name, strlen (_name) + 1);

            return IdpResponseCode::OK;
        });

    Manager ().RegisterCommand (
        static_cast<uint16_t> (NodeCommand::QueryInterface),
        [&](std::shared_ptr<IncomingTransaction> incoming,
            std::shared_ptr<OutgoingTransaction> outgoing) {
            Guid_t requestedGuid = incoming->ReadGuid ();


            if (_guid == requestedGuid)
            {
                outgoing->WriteGuid (_guid)->WithResponseCode (
                    IdpResponseCode::OK);

                this->SendRequest (incoming->Source (), outgoing);
            }

            return IdpResponseCode::Deferred;
        });

    Manager ().RegisterCommand (
        static_cast<uint16_t> (NodeCommand::Reset),
        [&](std::shared_ptr<IncomingTransaction> incoming,
            std::shared_ptr<OutgoingTransaction> outgoing) {
            this->OnReset ();

            return IdpResponseCode::OK;
        });

    _pingTimer = new DispatcherTimer (1000, false);

    _pingTimer->Tick += [&](auto sender, auto& e) {
        auto elapsedTime = Application::GetApplicationTime () - _lastPing;

        if (elapsedTime > 4000)
        {
            this->OnReset ();
        }
    };
}

IdpNode::~IdpNode ()
{
    delete _commandManager;
}

uint32_t IdpNode::CreateTransactionId ()
{
    return _currentTransactionId++;
}

void IdpNode::OnReset ()
{
    this->Address (UnassignedAddress);
}

const char* IdpNode::GetNodeCommandDescription (NodeCommand command)
{
    switch (command)
    {
        case NodeCommand::GetNodeInfo:
            return "GetNodeInfo     ";

        case NodeCommand::Ping:
            return "Ping            ";

        case NodeCommand::RouterDetect:
            return "RouterDetect    ";

        case NodeCommand::RouterEnumerateAdaptor:
            return "EnumerateAdaptor";

        case NodeCommand::RouterEnumerateNode:
            return "EnumerateNode   ";

        case NodeCommand::Response:
            return "Response        ";

        case NodeCommand::QueryInterface:
            return "QueryInterface  ";

        case NodeCommand::MarkAdaptorConnected:
            return "Mark Adaptor Ctd";

        case NodeCommand::Reset:
            return "Network Reset   ";

        case NodeCommand::RouterPrepareToEnumerateAdaptors:
            return "Begin Enum Adapt";

        default:
            return "Unknown         ";
    }
}

IdpCommandManager& IdpNode::Manager ()
{
    return *_commandManager;
}

uint16_t IdpNode::Address ()
{
    return _address;
}

void IdpNode::Address (uint16_t address)
{
    _address = address;

    if (address != UnassignedAddress && address != 0x0001)
    {
        _lastPing = Application::GetApplicationTime ();

        _pingTimer->Start ();
    }
    else
    {
        _pingTimer->Stop ();
    }
}

std::shared_ptr<IdpPacket>
    IdpNode::ProcessPacket (std::shared_ptr<IdpPacket> packet)
{
    if (_enabled)
    {
        return Manager ().ProcessPayload (_address, packet);
    }

    return nullptr;
}

IPacketTransmit& IdpNode::TransmitEndpoint ()
{
    return *_transmitEndpoint;
}

void IdpNode::TransmitEndpoint (IPacketTransmit& value)
{
    _transmitEndpoint = &value;
}

bool IdpNode::Connected ()
{
    return _transmitEndpoint != nullptr;
}

bool IdpNode::SendRequest (uint16_t destination,
                           std::shared_ptr<OutgoingTransaction> request,
                           ResponseHandler handler)
{
    Manager ().RegisterOneTimeResponseHandler (request->TransactionId (),
                                               handler);

    auto result = SendRequest (Address (), destination, request);

    if (!result)
    {
        Manager ().UnregisterOneTimeResponseHandler (request->TransactionId ());
    }

    return result;
}

bool IdpNode::SendRequest (uint16_t destination,
                           std::shared_ptr<OutgoingTransaction> request)
{
    return SendRequest (Address (), destination, request);
}

bool IdpNode::SendRequest (uint16_t source, uint16_t destination,
                           std::shared_ptr<OutgoingTransaction> request)
{
    if (Connected ())
    {
        return TransmitEndpoint ().Transmit (
            request->ToPacket (source, destination));
    }

    return false;
}

bool IdpNode::Enabled ()
{
    return _enabled;
}

void IdpNode::Enabled (bool value)
{
    _enabled = value;
}

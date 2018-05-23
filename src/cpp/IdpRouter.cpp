// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "IdpRouter.h"
#include "Trace.h"
#include <algorithm>

static constexpr uint16_t AdaptorNone = 0xFFFF;

IdpRouter::IdpRouter () : IdpNode (RouterGuid, "Network.Router")
{
    _currentlyEnumeratingAdaptor = nullptr;

    Manager ().RegisterCommand (
        static_cast<uint16_t> (NodeCommand::RouterDetect),
        [&](std::shared_ptr<IncomingTransaction> incoming,
            std::shared_ptr<OutgoingTransaction> outgoing) {
            auto address = incoming->Read<uint16_t> ();

            if (this->Address () == UnassignedAddress)
            {
                this->Address (address);
                outgoing->Write (true);
                outgoing->WithResponseCode (IdpResponseCode::OK);

                IdpNode::SendRequest (incoming->Source (), outgoing);

                return ::IdpResponseCode::Deferred;
            }
            else
            {
                outgoing->Write (false);
                return IdpResponseCode::OK;
            }
        });

    Manager ().RegisterCommand (
        static_cast<uint16_t> (NodeCommand::RouterEnumerateNode),
        [&](std::shared_ptr<IncomingTransaction> incoming,
            std::shared_ptr<OutgoingTransaction> outgoing) {
            auto address = incoming->Read<uint16_t> ();

            return this->HandleEnumerateNodesCommand (incoming->Source (),
                                                      address, outgoing);
        });

    Manager ().RegisterCommand (
        static_cast<uint16_t> (NodeCommand::RouterEnumerateAdaptor),
        [&](std::shared_ptr<IncomingTransaction> incoming,
            std::shared_ptr<OutgoingTransaction> outgoing) {
            auto address = incoming->Read<uint16_t> ();
            auto transactionId = incoming->Read<uint32_t> ();

            return this->HandleEnumerateAdaptorCommand (
                incoming->Source (), address, transactionId, outgoing);
        });

    Manager ().RegisterCommand (
        static_cast<uint16_t> (NodeCommand::RouterPrepareToEnumerateAdaptors),
        [&](std::shared_ptr<IncomingTransaction> incoming,
            std::shared_ptr<OutgoingTransaction> outgoing) {
            auto it = _adaptors.begin ();

            while (it != _adaptors.end ())
            {
                it->second->IsReEnumerated (false);

                it++;
            }
            return IdpResponseCode::OK;
        });

    Manager ().RegisterCommand (
        static_cast<uint16_t> (NodeCommand::MarkAdaptorConnected),
        [&](std::shared_ptr<IncomingTransaction> incoming,
            std::shared_ptr<OutgoingTransaction> outgoing) {
            Trace::WriteLine ("Marking adaptor enumerated");

            if (_currentlyEnumeratingAdaptor != nullptr)
            {
                _currentlyEnumeratingAdaptor->IsEnumerated (true);

                _currentlyEnumeratingAdaptor = nullptr;
            }

            return IdpResponseCode::OK;
        });


    // Routers endpoint is themselves, routing tables will find the correct
    // adaptor.
    TransmitEndpoint (*this);

    _nextAdaptorId = 1;
}

IdpRouter::~IdpRouter ()
{
}

void IdpRouter::OnReset ()
{
    auto it = _enumeratedNodes.begin ();

    while (it != _enumeratedNodes.end ())
    {
        if (it->second->Address () != 0x0001)
        {
            MarkUnenumerated (*it->second);

            it = _enumeratedNodes.erase (it);
        }
        else
        {
            it++;
        }
    }


    auto it1 = _adaptors.begin ();

    while (it1 != _adaptors.end ())
    {
        it1->second->IsEnumerated (false);

        it1++;
    }

    IdpNode::OnReset ();
}

bool IdpRouter::SendRequest (IPacketTransmit& adaptor, uint16_t source,
                             uint16_t destination,
                             std::shared_ptr<OutgoingTransaction> request)
{
    return adaptor.Transmit (request->ToPacket (source, destination));
}

IdpNode* IdpRouter::FindNode (uint16_t address)
{
    auto it = _enumeratedNodes.find (address);

    if (it != _enumeratedNodes.end ())
    {
        return _enumeratedNodes[address];
    }

    return nullptr;
}

bool IdpRouter::AddAdaptor (IAdaptor& adaptor)
{
    adaptor.SetLocal (*this);

    adaptor.AdaptorId (_nextAdaptorId); // TODO assign unique id.

    _adaptors[_nextAdaptorId] = &adaptor;

    _nextAdaptorId++;

    return true;
}

bool IdpRouter::AddNode (IdpNode& node)
{
    bool result = false;

    if (node.Address () == UnassignedAddress)
    {
        _unenumeratedNodes.push_front (&node);
        result = true;
    }
    else
    {
        auto existing = FindNode (node.Address ());

        if (existing == nullptr)
        {
            _enumeratedNodes[node.Address ()] = &node;

            result = true;
        }
    }

    if (result)
    {
        node.TransmitEndpoint (*this);
    }

    node.Enabled (true);

    return result;
}

void IdpRouter::RemoveNode (IdpNode& node)
{
    if (node.Address () == UnassignedAddress)
    {
        _unenumeratedNodes.remove (&node);
    }
    else if (FindNode (node.Address ()) != nullptr)
    {
        auto it = _enumeratedNodes.find (node.Address ());

        if (it != _enumeratedNodes.end ())
        {
            _enumeratedNodes.erase (it);
        }
    }

    node.Address (UnassignedAddress);
}

bool IdpRouter::MarkEnumerated (IdpNode& node)
{
    bool result = false;

    if (FindNode (node.Address ()) == nullptr)
    {
        auto it = std::find (_unenumeratedNodes.begin (),
                             _unenumeratedNodes.end (), &node);

        if (it != _unenumeratedNodes.end ())
        {
            _unenumeratedNodes.erase (it);
        }

        _enumeratedNodes[node.Address ()] = &node;

        result = true;
    }

    return result;
}

void IdpRouter::MarkUnenumerated (IdpNode& node)
{
    auto it = std::find (_unenumeratedNodes.begin (), _unenumeratedNodes.end (),
                         &node);

    if (it == _unenumeratedNodes.end ())
    {
        _unenumeratedNodes.push_front (&node);
    }
}

bool IdpRouter::Transmit (std::shared_ptr<IdpPacket> packet)
{
    Transmit (AdaptorNone, packet);

    return true;
}

IdpResponseCode IdpRouter::HandleEnumerateNodesCommand (
    uint16_t source, uint16_t address,
    std::shared_ptr<OutgoingTransaction> outgoing)
{
    if (!_unenumeratedNodes.empty ())
    {
        auto& node = *_unenumeratedNodes.back ();

        node.Address (address);

        if (MarkEnumerated (node))
        {
            outgoing->Write (true);
            outgoing->WithResponseCode (IdpResponseCode::OK);

            IdpNode::SendRequest (address, source, outgoing);

            return IdpResponseCode::Deferred;
        }
        else
        {
            node.Address (UnassignedAddress);

            outgoing->Write (false);

            return IdpResponseCode::InvalidParameters;
        }
    }
    else
    {
        outgoing->Write (false);
    }

    return IdpResponseCode::OK;
}

IdpResponseCode IdpRouter::HandleEnumerateAdaptorCommand (
    uint16_t source, uint16_t address, uint32_t transcationId,
    std::shared_ptr<OutgoingTransaction> outgoing)
{
    auto adaptor = GetNextUnenumeratedAdaptor (true);

    if (adaptor == nullptr)
    {
        Trace::WriteLine ("No Adaptor found on Router: %u", "IdpRouter",
                          Address ());
        outgoing->Write (false);

        return IdpResponseCode::OK;
    }
    else
    {
        adaptor->IsReEnumerated (true);

        auto outgoingTransaction =
            OutgoingTransaction::Create (
                static_cast<uint16_t> (NodeCommand::RouterDetect),
                transcationId, IdpCommandFlags::None)
                ->Write (address);

        bool adaptorSent = false;

        if (!adaptor->IsEnumerated ())
        {
            adaptorSent =
                SendRequest (*adaptor, source, 0xFFFF, outgoingTransaction);

            if (adaptorSent)
            {
                _currentlyEnumeratingAdaptor = adaptor;
            }
        }

        Trace::WriteLine ("Adaptor (%s) found on Router: %u, active: %s",
                          "IdpRouter", adaptor->Name (), Address (),
                          adaptor->IsEnumerated () ? "true" : "false");

        outgoing->Write (true)
            ->Write (adaptorSent)
            ->WithResponseCode (IdpResponseCode::OK);

        IdpNode::SendRequest (source, outgoing);

        return IdpResponseCode::Deferred;
    }
}

IAdaptor* IdpRouter::GetNextUnenumeratedAdaptor (bool reenumeration)
{
    auto it = _adaptors.begin ();

    while (it != _adaptors.end ())
    {
        if (reenumeration && !it->second->IsReEnumerated ())
        {
            break;
        }

        if (!reenumeration && !it->second->IsEnumerated ())
        {
            break;
        }

        it++;
    }

    if (it != _adaptors.end ())
    {
        return it->second;
    }

    return nullptr;
}

void IdpRouter::Transmit (uint16_t adaptorId, std::shared_ptr<IdpPacket> packet)
{
    auto source = packet->Source ();

    if (source != UnassignedAddress && adaptorId != 0xFFFF)
    {
        _adaptors[adaptorId]->IsEnumerated (true);

        if (!(source == 1 &&
              _routingTable.find (source) != _routingTable.end ()))
        {
            _routingTable[source] =
                adaptorId; // replace any existing route with
                           // the one the packet just came from.
        }
    }

    Route (packet);
}

void IdpRouter::Route (std::shared_ptr<IdpPacket> packet)
{
    auto destination = packet->Destination ();
    auto source = packet->Source ();

    packet->ResetReadToPayload ();
    auto command = packet->Read<uint16_t> ();
    auto transactionId = packet->Read<uint32_t> ();
    packet->Read<uint8_t> ();

    if (command == (uint16_t) NodeCommand::Response)
    {
        packet->Read<uint8_t> ();
        auto response = packet->Read<uint16_t> ();

        Trace::Write (
            "R:0x%04x S:0x%04x D:0x%04x "
            "C:0x%04x <%s ",
            "IdpRouter", Address (), source, destination, command,
            IdpNode::GetNodeCommandDescription ((NodeCommand) response));
    }
    else
    {
        Trace::Write (
            "R:0x%04x S:0x%04x D:0x%04x "
            "C:0x%04x >%s ",
            "IdpRouter", Address (), source, destination, command,
            IdpNode::GetNodeCommandDescription ((NodeCommand) command));
    }

    if (packet->Length () <= 32)
    {
        Trace::AppendBuffer (packet->Data (), packet->Length ());
    }

    Trace::AppendLine ("");

    packet->ResetRead ();

    if (destination == 0)
    {
        auto ait = _adaptors.begin ();

        auto receivedOn = _routingTable.find (source);

        while (ait != _adaptors.end ())
        {
            if (receivedOn == _routingTable.end () ||
                ait->second != _adaptors[receivedOn->second])
            {
                packet->ResetRead ();

                ait->second->Transmit (packet);
            }

            ait++;
        }

        auto it = _enumeratedNodes.begin ();

        while (it != _enumeratedNodes.end ())
        {
            packet->ResetRead ();

            auto response = it->second->ProcessPacket (packet);

            if (response != nullptr)
            {
                Route (response);
            }

            it++;
        }

        packet->ResetRead ();

        auto response = ProcessPacket (packet);

        if (response != nullptr)
        {
            Route (response);
        }
    }
    else
    {
        IdpNode* node = nullptr;

        if (destination == Address ())
        {
            node = this;
        }
        else
        {
            node = FindNode (packet->Destination ());
        }

        if (node != nullptr)
        {
            auto responsePacket = node->ProcessPacket (packet);

            if (responsePacket != nullptr)
            {
                Route (responsePacket);
            }
        }
        else
        {
            // todo find adaptor with that node.
            auto it = _routingTable.find (destination);

            if (it != _routingTable.end ())
            {
                _adaptors[it->second]->Transmit (packet);
            }
            else if (destination != UnassignedAddress)
            {
                // Probably safe to assume that we should transmit in same route
                // as master?
                Trace::WriteLine ("Packet Dropped: Unknown Route", "IdpRouter");
            }
        }
    }
}

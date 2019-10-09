// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "NotifyingStreamAdaptor.h"
#include "Trace.h"

NotifyingStreamAdaptor::NotifyingStreamAdaptor ()
{
    _parser = new IdpPacketParser ();

    Parser ().Stream (*_connection);

    Parser ().DataReceived += [this](auto sender, auto& e) {
        auto& args = static_cast<DataReceivedEventArgs&> (e);

        this->OnReceive (args.Packet);
    };
}

NotifyingStreamAdaptor::~NotifyingStreamAdaptor ()
{
    delete _parser;
}

std::shared_ptr<INotifyingStream> NotifyingStreamAdaptor::Connection ()
{
    return _connection;
}

void NotifyingStreamAdaptor::Connection (
    std::shared_ptr<INotifyingStream> value)
{
    if (_connection != nullptr)
    {
        if (_connectionHandler != nullptr)
        {
            _connection->DataReceived -= *_connectionHandler;
        }
    }

    _connection = value;

    if (_connection != nullptr)
    {
        Trace::WriteLine ("Connection Assigned", "NSAdaptor");
        _connectionHandler =
            &(_connection->DataReceived +=
              [this](auto sender, auto& e) { this->Parser ().Parse (); });

        this->Parser ().Stream (*_connection);

        this->IsReEnumerated (false);
        this->IsEnumerated (false);
        this->IsActive (true);
    }
    else
    {
        this->IsActive (false);
    }
}

IdpPacketParser& NotifyingStreamAdaptor::Parser ()
{
    return *_parser;
}

bool NotifyingStreamAdaptor::Transmit (std::shared_ptr<IdpPacket> packet)
{
    if (_connection != nullptr && _connection->IsValid ())
    {
        auto data = packet->Data ();
        auto length = packet->Length ();
        uint32_t sent = 0;
        int retries = 0;

        while (sent < length)
        {
            auto currentSend = _connection->Write (data + sent, length - sent);

            if (currentSend == -1 || !_connection->IsValid ())
            {
                Trace::WriteLine ("Transmit Failed",
                                  "Notifying Stream Adaptor");

                return false;
            }
            else
            {
                if (currentSend == 0)
                {
                    retries++;

                    if (retries > 100)
                    {
                        return false;
                    }
                }
                else
                {
                    retries = 0;
                }

                sent += currentSend;
            }
        }

        return true;
    }

    return false;
}

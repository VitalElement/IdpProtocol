// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include "IAdaptor.h"
#include "IStream.h"
#include "IdpPacketParser.h"
#include <stdbool.h>
#include <stdint.h>

/**
 *  NotifyingStreamAdaptor
 */
class NotifyingStreamAdaptor : public IAdaptor
{
  private:
    IdpPacketParser* _parser;
    std::shared_ptr<INotifyingStream> _connection;
    EventHandler* _connectionHandler;

    IdpPacketParser& Parser ();


  public:
    /**
     * Instantiates a new instance of NotifyingStreamAdaptor
     */
    NotifyingStreamAdaptor ();

    virtual ~NotifyingStreamAdaptor ();

    std::shared_ptr<INotifyingStream> Connection ();
    void Connection (std::shared_ptr<INotifyingStream> value);

    bool Transmit (std::shared_ptr<IdpPacket> packet);

    const char* Name ()
    {
        return "Stream.Adaptor";
    }
};

// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include "BitConverter.h"
#include "Guid.h"
#include "IdpPacket.h"
#include "IdpTransaction.h"
#include <cstring>
#include <memory>
#include <stdint.h>

/**
 *  IncomingTransaction
 */
class IncomingTransaction
{
  public:
    /**
     * Instantiates a new instance of IdpResponse
     */
    IncomingTransaction (std::shared_ptr<IdpPacket> packet);
    ~IncomingTransaction ();

    IdpCommandFlags Flags ();

    uint16_t CommandId ();

    uint32_t TransactionId ();

    std::shared_ptr<IdpPacket> Packet ();

    template<typename T>
    T Read ()
    {
        T result;

        memcpy (&result, _data.get () + _readIndex, sizeof (T));
        _readIndex += sizeof (T);

        if (BitConverter::IsLittleEndian ())
        {
            result = BitConverter::SwapEndian (result);
        }

        return result;
    }

    void Read (void* destination, uint32_t length);

    /**
     * Returns a pointer to the current data entry, and consumes
     * the data for another read.
     * 
     * ZeroCopy.
     */ 
    void* ConsumeData(uint32_t length);

    /**
     * Reads a UTF8 encoded string from the buffer.
     * Note: The result is heap allocated and user must call delete[]
     * when they are finished with the value.
     */
    const char* ReadCString ();

    Guid_t ReadGuid ();


    uint16_t Source ();
    uint16_t Destination ();

  private:
    uint32_t _readIndex;
    IdpCommandFlags _flags;
    uint16_t _commandId;
    uint32_t _transactionId;
    uint16_t _source;
    uint16_t _destination;
    std::shared_ptr<uint8_t> _data;
    std::shared_ptr<IdpPacket> _packet;
};

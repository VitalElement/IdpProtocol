// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include "Guid.h"
#include "IdpPacket.h"
#include "IdpTransaction.h"
#include <memory>
#include <stdint.h>
#include <vector>

/**
 *  IdpRequest
 */
class OutgoingTransaction
    : public std::enable_shared_from_this<OutgoingTransaction>
{
  private:
    /**
     * Instantiates a new instance of IdpRequest
     */
    OutgoingTransaction (
        uint16_t commandId, uint32_t transactionId,
        IdpCommandFlags flags = IdpCommandFlags::ResponseExpected);

  public:
    ~OutgoingTransaction ();

    uint16_t CommandId ();

    uint32_t TransactionId ();

    std::shared_ptr<IdpPacket> ToPacket (uint16_t source, uint16_t destination);

    static std::shared_ptr<OutgoingTransaction>
        Create (uint16_t commandId, uint32_t transactionId,
                IdpCommandFlags flags = IdpCommandFlags::ResponseExpected);

    std::shared_ptr<OutgoingTransaction>
        WithResponseCode (IdpResponseCode responseCode);

    std::shared_ptr<OutgoingTransaction> Write (void* data, uint32_t length);

    std::shared_ptr<OutgoingTransaction> WriteAt (void* data, uint32_t length,
                                                  uint32_t index);

    std::shared_ptr<OutgoingTransaction> WriteGuid (Guid_t& guid);

    template<typename T>
    std::shared_ptr<OutgoingTransaction> Write (T data)
    {
        if (BitConverter::IsLittleEndian ())
        {
            data = BitConverter::SwapEndian (data);
        }

        return Write (&data, sizeof (T));
    }

    template<typename T>
    std::shared_ptr<OutgoingTransaction> WriteAt (T data, uint32_t index)
    {
        if (BitConverter::IsLittleEndian ())
        {
            data = BitConverter::SwapEndian (data);
        }

        return WriteAt (&data, sizeof (T), index);
    }


  private:
    std::vector<uint8_t> _data;
    uint32_t _writeIndex;
    uint16_t _commandId;
    uint32_t _transactionId;
};

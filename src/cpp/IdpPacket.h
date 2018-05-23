// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include "BitConverter.h"
#include <cstring>
#include <memory>
#include <stdint.h>

enum class IdpFlags
{
    None = 0,
    CRC = 0x01,
    RAW = 0x02
};

/**
 *  IdpPacket
 */
class IdpPacket
{
  public:
    /**
     * Instantiates a new instance of IdpPacket
     */
    IdpPacket (uint32_t payloadLength, IdpFlags flags, uint16_t source = 0,
               uint16_t destination = 0, bool sealed = false);

    ~IdpPacket ();

    void Seal ();

    uint8_t* Data ();

    std::shared_ptr<uint8_t> Buffer ();

    uint8_t* Payload ();

    uint32_t PayloadLength ();

    uint8_t* WritePointer ();

    void IncrementWritePointer (uint32_t length);

    void ResetRead ();

    void ResetReadToPayload ();

    uint32_t Length ();

    IdpFlags Flags ();

    uint16_t Source ();
    uint16_t Destination ();

    template<typename T>
    void Write (T data)
    {
        if (!_isSealed)
        {
            if (BitConverter::IsLittleEndian ())
            {
                data = BitConverter::SwapEndian (data);
            }

            memcpy (_buffer.get () + _writeIndex, &data, sizeof (T));
            _writeIndex += sizeof (T);
        }
    }

    template<typename T>
    T Read ()
    {
        T result;

        memcpy (&result, _buffer.get () + _readIndex, sizeof (T));
        _readIndex += sizeof (T);

        if (BitConverter::IsLittleEndian ())
        {
            result = BitConverter::SwapEndian (result);
        }

        return result;
    }

    void Write (const void* data, uint32_t length);

  private:
    std::shared_ptr<uint8_t> _buffer;
    uint32_t _writeIndex;
    uint32_t _readIndex;
    bool _isSealed;
};

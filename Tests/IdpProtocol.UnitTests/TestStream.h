// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "CircularBuffer.h"
#include "IStream.h"
#include <memory>


class TestStream : public IStream
{
  public:
    TestStream (uint32_t bufferSize = 1024)
        : receiveBuffer (*new CircularBuffer<uint8_t> (new uint8_t[bufferSize],
                                                       bufferSize)),
          transmitBuffer (*new CircularBuffer<uint8_t> (new uint8_t[bufferSize],
                                                        bufferSize))
    {
    }

    std::shared_ptr<TestStream> GetEndpoint ()
    {
        return std::shared_ptr<TestStream> (
            new TestStream (transmitBuffer, receiveBuffer));
    }

    bool IsValid ()
    {
        return true;
    }

    int32_t BytesReceived ()
    {
        if (receiveBuffer.count == 0)
        {
            return -1;
        }
        else
        {
            return receiveBuffer.count;
        }
    }

    void Close ()
    {
    }

    int32_t Read (void* buffer, uint32_t length)
    {
        if (BytesReceived () > 0)
        {
            int32_t returnLength = length;

            if (returnLength > BytesReceived ())
            {
                returnLength = BytesReceived ();
            }

            uint8_t* writeBuffer = static_cast<uint8_t*> (buffer);

            for (int32_t i = 0; i < returnLength; i++)
            {
                writeBuffer[i] = receiveBuffer.Read ();
            }

            return returnLength;
        }
        else
        {
            return -1;
        }
    }

    int32_t Write (const void* data, uint32_t length)
    {
        const uint8_t* dataBuffer = static_cast<const uint8_t*> (data);

        for (uint32_t i = 0; i < length; i++)
        {
            transmitBuffer.Write (dataBuffer[i]);
        }

        return length;
    }

  private:
    TestStream (CircularBuffer<uint8_t>& receiveBuffer,
                CircularBuffer<uint8_t>& transmitBuffer)
        : receiveBuffer (receiveBuffer), transmitBuffer (transmitBuffer)
    {
    }

    CircularBuffer<uint8_t>& receiveBuffer;
    CircularBuffer<uint8_t>& transmitBuffer;
};
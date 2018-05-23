// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include "IPacketTransmit.h"

/**
 * Interface that provides a bridge between a router and the outside world.
 * Implementer only required to implement the Transmit Method and call
 * OnReceive() whenever a packet is ready to be processed.
 */
class IAdaptor : public IPacketTransmit
{
  protected:
    uint16_t _id;
    IAdaptorToRouterPort* _local;
    bool _isEnumerated;
    bool _isReEnumerated;
    bool _isActive;

  public:
    IAdaptor ()
    {
        _id = 0;
        _local = nullptr;
        _isEnumerated = false;
        _isReEnumerated = false;
        _isActive = false;
    }

    virtual ~IAdaptor ()
    {
    }

    virtual const char* Name () = 0;

    bool IsActive ()
    {
        return _isActive;
    }

    void IsActive (bool active)
    {
        if (_isActive != active)
        {
            _isActive = active;

            if (active)
            {
                IsEnumerated (false);
            }
        }
    }

    void AdaptorId (uint16_t id)
    {
        _id = id;
    }

    void SetLocal (IAdaptorToRouterPort& local)
    {
        _local = &local;
    }

    void OnReceive (std::shared_ptr<IdpPacket> packet)
    {
        if (_id != 0 && _local != nullptr)
        {
            _local->Transmit (_id, packet);
        }
    }

    bool IsEnumerated ()
    {
        return _isEnumerated;
    }

    void IsEnumerated (bool isEnumerated)
    {
        _isEnumerated = isEnumerated;
    }

    bool IsReEnumerated ()
    {
        return _isReEnumerated;
    }

    void IsReEnumerated (bool value)
    {
        _isReEnumerated = value;
    }
};

/******************************************************************************
 *  Description:
 *
 *       Author:
 *
 *******************************************************************************/
#pragma once

#include "DispatcherTimer.h"
#include "IdpNode.h"
#include <stdbool.h>
#include <stdint.h>

/**
 *  IdpClientNode
 */
class IdpClientNode : IdpNode
{
    uint64_t _lastPing;
    uint16_t _serverAddress;
    DispatcherTimer* _pollTimer;
    Guid_t _serverGuid;

  public:
    /**
     * Instantiates a new instance of IdpClientNode
     */
    IdpClientNode (Guid_t serverGuid, Guid_t guid, const char* name,
                   uint16_t address = UnassignedAddress);

    virtual ~IdpClientNode ();

  private:
    void QueryInterface (Guid_t guid);
};

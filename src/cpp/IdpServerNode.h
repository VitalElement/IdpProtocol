/******************************************************************************
 *  Description:
 *
 *       Author:
 *
 *******************************************************************************/
#pragma once

#include "IdpNode.h"
#include <stdbool.h>
#include <stdint.h>

enum class ClientCommand
{
    Connect = 0xD000,
    Disconnect
};

/**
 *  IdpClientNode
 */
class IdpServerNode : public IdpNode
{
    uint16_t _clientAddress;

  public:
    /**
     * Instantiates a new instance of IdpClientNode
     */
    IdpServerNode (Guid_t guid, const char* name,
                   uint16_t address = UnassignedAddress);

    virtual ~IdpServerNode ();

  protected:
    bool IsClientConnected ();
    uint16_t ClientAddress ();
};

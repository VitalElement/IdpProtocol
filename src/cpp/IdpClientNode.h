/******************************************************************************
 *  Description:
 *
 *       Author:
 *
 *******************************************************************************/
#pragma once

#include "DispatcherTimer.h"
#include "IdpServerNode.h"
#include <stdbool.h>
#include <stdint.h>

/**
 *  IdpClientNode
 */
class IdpClientNode : public IdpNode
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

    void Connect ();

    bool SendRequest (std::shared_ptr<OutgoingTransaction> request,
                      ResponseHandler handler);

    bool SendRequest (std::shared_ptr<OutgoingTransaction> request);

    Event Connected;
    Event Disconnected;

  protected:
    bool IsConnected ();

  private:
    void QueryInterface (Guid_t guid);
    void OnDisconnect ();
    void OnConnect (uint16_t serverAddress);
};

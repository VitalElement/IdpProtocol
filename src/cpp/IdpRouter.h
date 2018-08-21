// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include "IAdaptor.h"
#include "IdpNode.h"
#include <list>
#include <map>
#include <stdbool.h>
#include <stdint.h>

/**
 *  IdpRouter
 */
class IdpRouter : public IdpNode,
                  public IAdaptorToRouterPort,
                  public IPacketTransmit
{
  private:
    std::list<IdpNode*> _unenumeratedNodes;
    std::map<uint16_t, IdpNode*> _enumeratedNodes;

    std::map<uint16_t, IAdaptor*> _adaptors;

    IAdaptor* _currentlyEnumeratingAdaptor;

    std::map<uint16_t, uint16_t> _routingTable;

    uint16_t _nextAdaptorId;

    virtual void OnReset ();


    IdpNode* FindNode (uint16_t address);

    IdpResponseCode HandleEnumerateNodesCommand (
        uint16_t source, uint16_t address,
        std::shared_ptr<OutgoingTransaction> outgoing);

    IdpResponseCode HandleEnumerateAdaptorCommand (
        uint16_t source, uint16_t address, uint32_t transactionId,
        std::shared_ptr<OutgoingTransaction> outgoing);

    bool SendRequest (IPacketTransmit& adaptor, uint16_t source,
                      uint16_t destination,
                      std::shared_ptr<OutgoingTransaction> request);

    IAdaptor* GetNextUnenumeratedAdaptor (bool reenumeration = false);

  public:
    /**
     * Instantiates a new instance of IdpRouter
     */
    IdpRouter ();

    virtual ~IdpRouter ();

    bool Transmit (std::shared_ptr<IdpPacket> packet);

    bool Transmit (uint16_t adaptorId, std::shared_ptr<IdpPacket> packet);

    bool MarkEnumerated (IdpNode& node);
    void MarkUnenumerated (IdpNode& node);

    bool AddNode (IdpNode& node);
    void RemoveNode (IdpNode& node);

    bool AddAdaptor (IAdaptor& adaptor);

    bool Route (std::shared_ptr<IdpPacket> packet);

    void OnPollTimerTick ();
};

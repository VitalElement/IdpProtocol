// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include "DispatcherTimer.h"
#include "Guid.h"
#include "IPacketTransmit.h"
#include "IdpCommandManager.h"
#include <memory>
#include <stdbool.h>
#include <stdint.h>

class IPacketTransmit;

const Guid_t MasterGuid = Guid_t ("554C0A67-F228-47B5-8155-8C5436D533DA");
const Guid_t RouterGuid = Guid_t ("A1EE332D-5C7C-42FE-9519-54BDAC40CF21");

constexpr uint16_t UnassignedAddress = 0xFFFF;
constexpr uint16_t RouterPollAddress = 0xFFFE;

enum class NodeCommand : uint16_t
{
    Response = 0xA000,
    Ping = 0xA001,
    GetNodeInfo = 0xA002,
    QueryInterface = 0xA003,
    Reset = 0xA004,

    RecommendEnumeration = 0xA005,

    RouterDetect = 0xA006,
    RouterEnumerateNode = 0xA007,
    RouterPrepareToEnumerateAdaptors = 0xA008,
    RouterEnumerateAdaptor = 0xA009,
    MarkAdaptorConnected = 0xA00A,

    RouterPoll = 0xA00B
};

enum class EnumerationTarget : uint16_t
{
    Router,
    Node,
    Adaptor
};

/**
 *  IdpNode
 */
class IdpNode
{
  private:
    IdpCommandManager* _commandManager;
    uint16_t _address;
    IPacketTransmit* _transmitEndpoint;
    bool _enabled;
    uint32_t _currentTransactionId;

    const char* _name;
    uint64_t _lastPing;
    DispatcherTimer* _pingTimer;

  protected:
    Guid_t _guid;

  public:
    /**
     * Instantiates a new instance of IdpNode
     */
    IdpNode (Guid_t guid, const char* name,
             uint16_t address = UnassignedAddress);

    virtual ~IdpNode ();

    static const char* GetNodeCommandDescription (NodeCommand command);

    IdpCommandManager& Manager ();

    std::shared_ptr<IdpPacket>
        ProcessPacket (std::shared_ptr<IdpPacket> packet);

    bool SendRequest (uint16_t destination,
                      std::shared_ptr<OutgoingTransaction> request,
                      ResponseHandler handler);

    bool SendRequest (uint16_t destination,
                      std::shared_ptr<OutgoingTransaction> request);

    bool SendRequest (uint16_t source, uint16_t destination,
                      std::shared_ptr<OutgoingTransaction> request);

    virtual void OnAddressAssigned (uint16_t address);

    virtual void OnReset ();

    virtual void OnPollTimerTick ();

    uint16_t Address ();
    void Address (uint16_t address);

    bool Enabled ();
    void Enabled (bool value);

    IPacketTransmit& TransmitEndpoint ();
    void TransmitEndpoint (IPacketTransmit& value);

    bool Connected ();

    uint16_t Vid ();

    uint16_t Pid ();

    const char* Name ();

    uint32_t CreateTransactionId ();
};

// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#pragma once

#include "Application.h"
#include "DispatcherTimer.h"
#include "Guid.h"
#include "IdpNode.h"
#include "Trace.h"
#include <functional>
#include <list>
#include <map>
#include <stack>
#include <stdbool.h>
#include <stdint.h>

enum class NodeEnumerationState : uint8_t
{
    Pending,
    DetectingRouter,
    EnumeratingNodes,
    StartEnumeratingAdaptors,
    EnumeratingAdaptors,
    Idle
};

struct NodeInfo
{
    NodeInfo (NodeInfo* parent, uint16_t address)
    {
        Address = address;
        Name = nullptr;
        LastSeen = Application::GetApplicationTime ();
        EnumerationState = NodeEnumerationState::Idle;
        Parent = parent;
        Name = nullptr;
    }

    bool IsRouter ()
    {
        return Guid == RouterGuid;
    }

    const char* Name;

    uint16_t Address;
    uint64_t LastSeen;
    Guid_t Guid;
    NodeEnumerationState EnumerationState;

    NodeInfo* Parent;
    std::list<NodeInfo*> Children;
};

/**
 *  MasterNode
 */
class MasterNode : public IdpNode
{
  private:
    DispatcherTimer* _pollTimer;
    uint16_t _nextAddress;
    std::stack<uint16_t> _freeAddresses;
    std::map<uint16_t, NodeInfo*> _nodeInfo;
    NodeInfo* _root;
    uint32_t _nodeTimeout;
    NodeInfo* _currentEnumerationNode;
    bool _isEnumerating;

    NodeInfo* FindNode (uint16_t address);

    uint16_t GetFreeAddress ();

    void HandlePollResponse (uint16_t address);

    void InvalidateNodes ();

    void OnNodeAdded (uint16_t parentAddress, uint16_t address);

    void VisitNodes (NodeInfo* root, std::function<bool(NodeInfo&)> visitor);

    NodeInfo* GetNextEnumerationNode ();

    void OnEnumerate ();

    void DetectRouter ();
    void EnumerateRouterNode (uint16_t routerAddress);
    void StartEnumerateRouterAdaptors (uint16_t routerAddress);
    void EnumerateRouterAdaptor (uint16_t routerAddress);

  public:
    /**
     * Instantiates a new instance of MasterNode
     */
    MasterNode ();

    virtual ~MasterNode ();

    bool IsEnumerating ();

    virtual void OnReset ();

    NodeInfo* NetworkTree ();

    bool HasNode (uint16_t address);
    NodeInfo& GetNodeInfo (uint16_t address);

    uint32_t NodeTimeout ();
    void NodeTimeout (uint32_t value);

    void ResetNetwork ();

    void EnumerateNetwork ();

    void PollNetwork ();

    void TraceNetworkTree (NodeInfo* node = nullptr, uint32_t level = 0);
};

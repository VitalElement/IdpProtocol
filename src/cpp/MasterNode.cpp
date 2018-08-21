// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include "MasterNode.h"
#include "Application.h"
#include "Trace.h"

constexpr uint16_t MasterNodeAddress = 1;

MasterNode::MasterNode ()
    : IdpNode (MasterGuid, "Network.Master", MasterNodeAddress)
{
    _nodesChanged = false;
    _isEnumerating = false;
    _nextAddress = 2;
    _nodeTimeout = 5000;
    _root = new NodeInfo (nullptr, MasterNodeAddress);
    _root->Guid = _guid;
    _root->Name = "Network.Master";
    _root->EnumerationState = NodeEnumerationState::Pending;

    _currentEnumerationNode = nullptr;

    _nodeInfo[Address ()] = _root;

    Manager ().RegisterCommand (
        static_cast<uint16_t> (NodeCommand::Ping),
        [&](std::shared_ptr<IncomingTransaction> incoming,
            std::shared_ptr<OutgoingTransaction> outgoing) {
            if (this->HandlePollResponse (incoming->Source ()))
            {
                return IdpResponseCode::OK;
            }
            else
            {
                return IdpResponseCode::NotReady;
            }
        });

    Manager ().RegisterCommand (
        static_cast<uint16_t> (NodeCommand::RecommendEnumeration),
        [&](std::shared_ptr<IncomingTransaction> incoming,
            std::shared_ptr<OutgoingTransaction> outgoing) {
            if (!this->IsEnumerating ())
            {
                this->EnumerateNetwork ();
            }

            return IdpResponseCode::OK;
        });

    _pollTimer = new DispatcherTimer (1000);

    _pollTimer->Tick += [&](auto sender, auto& e) {
        _pollTimer->Stop ();

        this->EnumerateNetwork ();
    };
}

MasterNode::~MasterNode ()
{
}

NodeInfo* MasterNode::NetworkTree ()
{
    return _root;
}

void MasterNode::VisitNodes (NodeInfo* root,
                             std::function<bool(NodeInfo&)> visitor)
{
    if (visitor (*root))
    {
        auto it = root->Children.begin ();

        while (it != root->Children.end ())
        {
            VisitNodes (*it, visitor);
            it++;
        }
    }
}

NodeInfo* MasterNode::GetNextEnumerationNode ()
{
    NodeInfo* result = nullptr;

    VisitNodes (_root, [&](NodeInfo& node) {
        if (node.EnumerationState != NodeEnumerationState::Idle)
        {
            result = &node;
            return false;
        }

        return true;
    });

    return result;
}

bool MasterNode::HandlePollResponse (uint16_t address)
{
    if (address != Address ())
    {
        auto node = FindNode (address);

        if (node != nullptr)
        {
            node->LastSeen = Application::GetApplicationTime ();

            return true;
        }
    }

    return false;
}

NodeInfo* MasterNode::FindNode (uint16_t address)
{
    auto it = _nodeInfo.find (address);

    if (it != _nodeInfo.end ())
    {
        return _nodeInfo[address];
    }

    return nullptr;
}

NodeInfo& MasterNode::GetNodeInfo (uint16_t address)
{
    return *_nodeInfo[address];
}

bool MasterNode::HasNode (uint16_t address)
{
    return _nodeInfo.find (address) != _nodeInfo.end ();
}

uint16_t MasterNode::GetFreeAddress ()
{
    if (_freeAddresses.empty ())
    {
        return _nextAddress++;
    }
    else
    {
        auto result = _freeAddresses.top ();

        _freeAddresses.pop ();

        return result;
    }
}

bool MasterNode::IsEnumerating ()
{
    return _isEnumerating;
}

void MasterNode::OnEnumerate ()
{
    auto node = GetNextEnumerationNode ();

    if (node != nullptr)
    {
        _currentEnumerationNode = node;

        if (node == _root)
        {
            _currentEnumerationNode->EnumerationState =
                NodeEnumerationState::DetectingRouter;

            DetectRouter ();
        }
        else if (node->IsRouter ())
        {
            switch (node->EnumerationState)
            {
                case NodeEnumerationState::Pending:
                case NodeEnumerationState::EnumeratingNodes:
                    _currentEnumerationNode->EnumerationState =
                        NodeEnumerationState::EnumeratingNodes;
                    EnumerateRouterNode (node->Address);
                    break;

                case NodeEnumerationState::StartEnumeratingAdaptors:
                    StartEnumerateRouterAdaptors (node->Address);
                    break;

                case NodeEnumerationState::EnumeratingAdaptors:
                    EnumerateRouterAdaptor (node->Address);
                    break;

                default:
                    break;
            }
        }
    }
    else if (_currentEnumerationNode != nullptr)
    {
        _currentEnumerationNode = nullptr;

        _isEnumerating = false;

        PollNetwork ();

        if (_nodesChanged)
        {
            _nodesChanged = false;
            Trace::WriteLine ("Network structure changed", "MasterNode");
            TraceNetworkTree ();
        }

        _pollTimer->Start ();
    }
}

void MasterNode::ResetNetwork ()
{
    SendRequest (0, OutgoingTransaction::Create (
                        static_cast<uint16_t> (NodeCommand::Reset),
                        CreateTransactionId (), IdpCommandFlags::None));
}

void MasterNode::OnReset ()
{
}

void MasterNode::EnumerateNetwork ()
{
    if (Connected () && !_isEnumerating)
    {
        _isEnumerating = true;

        VisitNodes (_root, [&](NodeInfo& node) {
            if (node.IsRouter ())
            {
                node.EnumerationState = NodeEnumerationState::Pending;
            }
            else
            {
                if (&node == _root)
                {
                    if (node.Children.size () == 0)
                    {
                        node.EnumerationState = NodeEnumerationState::Pending;
                    }
                }
                else
                {
                    node.EnumerationState = NodeEnumerationState::Idle;
                }
            }

            return true;
        });

        OnEnumerate ();
    }
}

void MasterNode::DetectRouter ()
{
    auto address = GetFreeAddress ();

    auto outgoingTransaction = OutgoingTransaction::Create (
        static_cast<uint16_t> (NodeCommand::RouterDetect),
        CreateTransactionId ());

    outgoingTransaction->Write (address);

    if (!SendRequest (0xFFFF, outgoingTransaction,
                      [&, address](std::shared_ptr<IdpResponse> response) {
                          if (response != nullptr &&
                              response->ResponseCode () == IdpResponseCode::OK)
                          {
                              auto nodeEnumerated =
                                  response->Transaction ()->Read<bool> ();

                              if (nodeEnumerated)
                              {
                                  _currentEnumerationNode->EnumerationState =
                                      NodeEnumerationState::Idle;

                                  this->OnNodeAdded (this->Address (), address);

                                  return;
                              }
                          }

                          _freeAddresses.push (address);

                          this->OnEnumerate ();
                      }))
    {
        _currentEnumerationNode->EnumerationState = NodeEnumerationState::Idle;

        this->OnEnumerate ();
    }
}

void MasterNode::EnumerateRouterNode (uint16_t routerAddress)
{
    auto address = GetFreeAddress ();

    auto outgoingTransaction =
        OutgoingTransaction::Create (
            static_cast<uint16_t> (NodeCommand::RouterEnumerateNode),
            CreateTransactionId ())
            ->Write (address);

    bool sent = SendRequest (
        routerAddress, outgoingTransaction,
        [&, address, routerAddress](std::shared_ptr<IdpResponse> response) {
            if (response != nullptr &&
                response->ResponseCode () == IdpResponseCode::OK)
            {
                auto nodeEnumerated = response->Transaction ()->Read<bool> ();

                if (nodeEnumerated)
                {
                    this->OnNodeAdded (routerAddress, address);

                    return;
                }
            }

            if (_currentEnumerationNode != nullptr)
            {
                _currentEnumerationNode->EnumerationState =
                    NodeEnumerationState::StartEnumeratingAdaptors;
            }

            _freeAddresses.push (address);

            this->OnEnumerate ();
        });

    if (!sent)
    {
        Trace::WriteLine ("Enumerate nodes failed.");

        _currentEnumerationNode->EnumerationState = NodeEnumerationState::Idle;

        this->OnEnumerate ();
    }
}

void MasterNode::StartEnumerateRouterAdaptors (uint16_t routerAddress)
{
    auto outgoingTransaction = OutgoingTransaction::Create (
        static_cast<uint16_t> (NodeCommand::RouterPrepareToEnumerateAdaptors),
        CreateTransactionId ());

    bool sent =
        SendRequest (routerAddress, outgoingTransaction,
                     [&](std::shared_ptr<IdpResponse> response) {
                         if (response != nullptr &&
                             response->ResponseCode () == IdpResponseCode::OK)
                         {
                             if (_currentEnumerationNode != nullptr)
                             {
                                 _currentEnumerationNode->EnumerationState =
                                     NodeEnumerationState::EnumeratingAdaptors;
                             }
                         }
                         else
                         {
                             if (_currentEnumerationNode != nullptr)
                             {
                                 _currentEnumerationNode->EnumerationState =
                                     NodeEnumerationState::Idle;
                             }
                         }

                         this->OnEnumerate ();
                     });

    if (!sent)
    {
        Trace::WriteLine ("Begin Enumerate adaptors failed.");

        _currentEnumerationNode->EnumerationState = NodeEnumerationState::Idle;

        this->OnEnumerate ();
    }
}

void MasterNode::EnumerateRouterAdaptor (uint16_t routerAddress)
{
    auto address = GetFreeAddress ();

    auto routerDetectTransactionId = CreateTransactionId ();

    auto outgoingTransaction =
        OutgoingTransaction ::Create (
            static_cast<uint16_t> (NodeCommand::RouterEnumerateAdaptor),
            CreateTransactionId ())
            ->Write (address)
            ->Write (routerDetectTransactionId);

    Manager ().RegisterOneTimeResponseHandler (
        routerDetectTransactionId,
        [&, address, routerAddress,
         routerDetectTransactionId](std::shared_ptr<IdpResponse> response) {
            if (response != nullptr &&
                response->ResponseCode () == IdpResponseCode::OK)
            {
                auto nodeEnumerated = response->Transaction ()->Read<bool> ();

                if (nodeEnumerated)
                {
                    this->OnNodeAdded (routerAddress, address);

                    auto markAdaptorConnected = OutgoingTransaction::Create (
                        static_cast<uint16_t> (
                            NodeCommand::MarkAdaptorConnected),
                        CreateTransactionId (), IdpCommandFlags::None);

                    this->SendRequest (address, markAdaptorConnected);
                    return;
                }
                else
                {
                    if (_currentEnumerationNode != nullptr)
                    {
                        _currentEnumerationNode->EnumerationState =
                            NodeEnumerationState::Idle;
                    }
                }
            }

            _freeAddresses.push (address);

            this->OnEnumerate ();
        });


    bool sent = SendRequest (
        routerAddress, outgoingTransaction,
        [&, address, routerAddress,
         routerDetectTransactionId](std::shared_ptr<IdpResponse> response) {
            if (response != nullptr &&
                response->ResponseCode () == IdpResponseCode::OK)
            {
                auto adaptorEnumerated =
                    response->Transaction ()->Read<bool> ();

                if (adaptorEnumerated)
                {
                    auto adaptorProbed =
                        response->Transaction ()->Read<bool> ();

                    if (adaptorProbed)
                    {
                        // we need to await the detect router response
                        // from the new router.

                        // this may also timeout. reset timeout here?
                        // or set a trasaction id that gets relayed,
                        // and make timeouts work on tid not command
                        // id.
                        return;
                    }
                    else
                    {
                        this->Manager ().UnregisterOneTimeResponseHandler (
                            routerDetectTransactionId);
                    }
                }
                else
                {
                    this->Manager ().UnregisterOneTimeResponseHandler (
                        routerDetectTransactionId);

                    if (_currentEnumerationNode != nullptr)
                    {
                        _currentEnumerationNode->EnumerationState =
                            NodeEnumerationState::Idle;
                    }
                }
            }
            else
            {
                this->Manager ().UnregisterOneTimeResponseHandler (
                    routerDetectTransactionId);

                if (_currentEnumerationNode != nullptr)
                {
                    _currentEnumerationNode->EnumerationState =
                        NodeEnumerationState::Idle;
                }
            }

            _freeAddresses.push (address);
            this->OnEnumerate ();
        });

    if (!sent)
    {
        this->Manager ().UnregisterOneTimeResponseHandler (
            routerDetectTransactionId);

        Trace::WriteLine ("Enumerate adaptor failed.");

        _currentEnumerationNode->EnumerationState = NodeEnumerationState::Idle;

        this->OnEnumerate ();
    }
}

void MasterNode::OnNodeAdded (uint16_t parentAddress, uint16_t address)
{
    _nodesChanged = true;

    _nodeInfo[address] = new NodeInfo (_nodeInfo[parentAddress], address);

    _nodeInfo[parentAddress]->Children.push_back (_nodeInfo[address]);

    auto nodeInfoTransaction = OutgoingTransaction::Create (
        static_cast<uint16_t> (NodeCommand::GetNodeInfo),
        CreateTransactionId ());

    SendRequest (address, nodeInfoTransaction,
                 [&, address](std::shared_ptr<IdpResponse> response) {
                     if (response != nullptr &&
                         response->ResponseCode () == IdpResponseCode::OK)
                     {
                         _nodeInfo[address]->Guid =
                             response->Transaction ()->ReadGuid ();

                         if (_nodeInfo[address]->Name != nullptr)
                         {
                             delete[] _nodeInfo[address]->Name;
                         }

                         _nodeInfo[address]->Name =
                             response->Transaction ()->ReadCString ();

                         if (_nodeInfo[address]->IsRouter ())
                         {
                             _nodeInfo[address]->EnumerationState =
                                 NodeEnumerationState::Pending;
                         }
                         else
                         {
                             _nodeInfo[address]->EnumerationState =
                                 NodeEnumerationState::Idle;
                         }

                         this->OnEnumerate ();
                     }
                 });
}

void MasterNode::InvalidateNodes ()
{
    auto currentTime = Application::GetApplicationTime ();

    auto it = _nodeInfo.begin ();

    while (it != _nodeInfo.end ())
    {
        auto address = it->first;
        auto current = it->second;

        if (current != _root && currentTime >= current->LastSeen + _nodeTimeout)
        {
            auto childIt = current->Children.begin ();

            while (childIt != current->Children.end ())
            {
                (*childIt)->Parent = nullptr;

                childIt++;
            }

            if (current->Parent != nullptr)
            {
                current->Parent->Children.remove (current);
            }

            it = _nodeInfo.erase (it);

            delete current;
            _freeAddresses.push (address);

            _nodesChanged = true;
        }
        else
        {
            it++;
        }
    }
}

void MasterNode::PollNetwork ()
{
    InvalidateNodes ();
}

uint32_t MasterNode::NodeTimeout ()
{
    return _nodeTimeout;
}

void MasterNode::NodeTimeout (uint32_t value)
{
    _nodeTimeout = value;
}

void MasterNode::TraceNetworkTree (NodeInfo* node, uint32_t level)
{
    if (node == nullptr)
    {
        node = _root;
    }

    Trace::Write ("");

    for (uint32_t i = 0; i < level * 4; i++)
    {
        Trace::Append ("-");
    }

    Trace::Append (">");

    Trace::Append (node->Name == nullptr ? "Unnamed" : node->Name);
    Trace::AppendLine (" (%u)", node->Address);

    auto it = node->Children.begin ();

    while (it != node->Children.end ())
    {
        TraceNetworkTree (*it, level + 1);
        it++;
    }
}

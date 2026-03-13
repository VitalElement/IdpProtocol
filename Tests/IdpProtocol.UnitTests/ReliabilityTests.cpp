// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.

#include "catch.hpp"

#include "IdpClientNode.h"
#include "IdpCommandManager.h"
#include "IdpRouter.h"
#include "MasterNode.h"
#include "NotifyingStreamAdaptor.h"
#include "TestRuntime.h"

static const Guid_t ReliabilityTestGuid =
    Guid_t ("dfda0b6f-7ee4-4906-8b1c-15f455fbb77c");

class SilentNotifyingStream : public INotifyingStream
{
  public:
    SilentNotifyingStream ()
    {
        _valid = true;
    }

    virtual ~SilentNotifyingStream ()
    {
    }

    bool IsValid ()
    {
        return _valid;
    }

    int32_t BytesReceived ()
    {
        return -1;
    }

    void Close ()
    {
        _valid = false;
    }

    int32_t Read (void* buffer, uint32_t length)
    {
        return -1;
    }

    int32_t Write (const void* data, uint32_t length)
    {
        return length;
    }

    void FireDataReceived ()
    {
        DataReceived (this, EventArgs::Empty);
    }

  private:
    bool _valid;
};

TEST_CASE ("Command manager can be destroyed while its timeout timer is active")
{
    TestRuntime::Initialise ();

    auto manager = new IdpCommandManager ();

    delete manager;

    TestRuntime::IterateRuntime (50, 50);

    REQUIRE (true);
}

TEST_CASE ("Destroying a node with an active ping timer leaves no live callbacks")
{
    TestRuntime::Initialise ();

    auto node = new IdpNode (ReliabilityTestGuid, "Reliability.Node");
    node->Address (2);

    delete node;

    TestRuntime::IterateRuntime (2000, 100);

    REQUIRE (true);
}

TEST_CASE ("Destroying a client node with an active reconnect timer leaves no live callbacks")
{
    TestRuntime::Initialise ();

    auto client = new IdpClientNode (ReliabilityTestGuid,
                                     ReliabilityTestGuid,
                                     "Reliability.Client");

    client->Connect ();

    delete client;

    TestRuntime::IterateRuntime (2000, 100);

    REQUIRE (true);
}

TEST_CASE ("Destroying a stream adaptor detaches from the notifying stream")
{
    TestRuntime::Initialise ();

    auto stream = std::shared_ptr<SilentNotifyingStream> (
        new SilentNotifyingStream ());

    auto adaptor = new NotifyingStreamAdaptor ();
    adaptor->Connection (stream);

    delete adaptor;

    stream->FireDataReceived ();

    REQUIRE (true);
}

TEST_CASE ("Master node continues enumeration after GetNodeInfo times out")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();
    auto& router = *new IdpRouter ();
    auto& silentNode = *new IdpNode (ReliabilityTestGuid, "Silent.Node");
    auto& healthyNode = *new IdpNode (ReliabilityTestGuid, "Healthy.Node");

    router.AddNode (masterNode);
    router.AddNode (silentNode);
    router.AddNode (healthyNode);

    silentNode.Enabled (false);

    masterNode.EnumerateNetwork ();

    TestRuntime::IterateRuntime (2500, 500);

    REQUIRE_FALSE (masterNode.IsEnumerating ());
    REQUIRE (masterNode.HasNode (router.Address ()));
    REQUIRE (masterNode.NetworkTree ()->Children.size () == 1);
}

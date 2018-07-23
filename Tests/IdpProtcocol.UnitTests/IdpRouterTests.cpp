// Copyright (c) VitalElement. All rights reserved.
// Licensed under the MIT license. See licence.md file in the project root for
// full license information.
#include <stdint.h>

#include "IdpRouter.h"
#include "MasterNode.h"
#include "SimpleAdaptor.h"
#include "TestRuntime.h"
#include "catch.hpp"

static const Guid_t TestGuid = Guid_t ("dfda0b6f-7ee4-4906-8b1c-15f455fbb77c");

TEST_CASE ("Detect Router Command Response Is Handled by Master")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();

    auto& idpRouter = *new IdpRouter ();

    idpRouter.AddNode (masterNode);

    masterNode.EnumerateNetwork ();
    masterNode.TraceNetworkTree ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    REQUIRE (idpRouter.Address () == 2);
    REQUIRE (masterNode.HasNode (idpRouter.Address ()));

    auto nodeInfo = masterNode.GetNodeInfo (idpRouter.Address ());

    REQUIRE (nodeInfo.Guid == RouterGuid);

    auto networkTree = masterNode.NetworkTree ();

    REQUIRE (!networkTree->Children.empty ());
}

TEST_CASE ("Can Enumerate Nodes on First Router")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();

    auto& router = *new IdpRouter ();

    auto& childNode1 = *new IdpNode (
        Guid_t ("dfda0b6f-7ee4-4906-8b1c-15f455fbb77c"), "Child.Node.1");
    auto& childNode2 = *new IdpNode (
        Guid_t ("dfda0b6f-7ee4-4906-8b1c-15f455fbb77c"), "Child.Node.2");

    router.AddNode (masterNode);
    router.AddNode (childNode1);
    router.AddNode (childNode2);

    masterNode.EnumerateNetwork ();
    masterNode.TraceNetworkTree ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    REQUIRE (router.Address () == 2);
    REQUIRE (masterNode.HasNode (router.Address ()));
    REQUIRE (masterNode.HasNode (childNode1.Address ()));

    auto nodeInfo = masterNode.GetNodeInfo (childNode1.Address ());
    REQUIRE (nodeInfo.Guid == TestGuid);

    REQUIRE (masterNode.HasNode (childNode2.Address ()));

    nodeInfo = masterNode.GetNodeInfo (childNode2.Address ());

    REQUIRE (nodeInfo.Guid == TestGuid);
}

TEST_CASE ("Can Detect Secondary Router")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();

    auto& router = *new IdpRouter ();

    auto& router2 = *new IdpRouter ();

    auto& childNode1 = *new IdpNode (TestGuid, "Child.Node.1");
    auto& childNode2 = *new IdpNode (TestGuid, "Child.Node.2");

    router.AddNode (masterNode);
    router.AddNode (childNode1);
    router.AddNode (childNode2);

    auto& adaptor1 = *new SimpleAdaptor ();
    auto& adaptor2 = *new SimpleAdaptor ();

    router.AddAdaptor (adaptor1);
    router2.AddAdaptor (adaptor2);

    adaptor1.SetRemote (adaptor2);
    adaptor2.SetRemote (adaptor1);

    masterNode.EnumerateNetwork ();

    masterNode.TraceNetworkTree ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    REQUIRE (masterNode.HasNode (router.Address ()));
    REQUIRE (masterNode.HasNode (childNode1.Address ()));
    REQUIRE (masterNode.HasNode (childNode2.Address ()));
    REQUIRE (masterNode.HasNode (router2.Address ()));

    auto nodeInfo = masterNode.GetNodeInfo (router2.Address ());

    REQUIRE (nodeInfo.Guid == RouterGuid);
}

TEST_CASE ("Can Enumerate Child Nodes on Secondary Router")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();
    auto& router = *new IdpRouter ();
    auto& router2 = *new IdpRouter ();

    auto& childNode1 = *new IdpNode (TestGuid, "Child.Node.1");
    auto& childNode2 = *new IdpNode (TestGuid, "Child.Node.2");

    router.AddNode (masterNode);
    router.AddNode (childNode1);
    router.AddNode (childNode2);

    auto& adaptor1 = *new SimpleAdaptor ();
    auto& adaptor2 = *new SimpleAdaptor ();

    router.AddAdaptor (adaptor1);
    router2.AddAdaptor (adaptor2);

    adaptor1.SetRemote (adaptor2);
    adaptor2.SetRemote (adaptor1);

    auto& remoteNode1 = *new IdpNode (TestGuid, "Remote.Node.1");
    auto& remoteNode2 = *new IdpNode (TestGuid, "Remote.Node.2");

    router2.AddNode (remoteNode1);
    router2.AddNode (remoteNode2);

    masterNode.EnumerateNetwork ();
    masterNode.TraceNetworkTree ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    REQUIRE (adaptor1.IsEnumerated ());
    REQUIRE (adaptor1.IsReEnumerated ());
    REQUIRE (adaptor2.IsEnumerated ());
    REQUIRE (adaptor2.IsReEnumerated ());

    masterNode.EnumerateNetwork ();
    masterNode.TraceNetworkTree ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    REQUIRE (masterNode.HasNode (remoteNode1.Address ()));
    REQUIRE (masterNode.HasNode (remoteNode2.Address ()));
}

TEST_CASE ("Can enumerate nodes added in future")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();
    auto& router = *new IdpRouter ();
    auto& router2 = *new IdpRouter ();

    auto& childNode1 = *new IdpNode (TestGuid, "Child.Node.1");
    auto& childNode2 = *new IdpNode (TestGuid, "Child.Node.2");

    router.AddNode (masterNode);
    router.AddNode (childNode1);
    router.AddNode (childNode2);

    auto& adaptor1 = *new SimpleAdaptor ();
    auto& adaptor2 = *new SimpleAdaptor ();

    router.AddAdaptor (adaptor1);
    router2.AddAdaptor (adaptor2);

    adaptor1.SetRemote (adaptor2);
    adaptor2.SetRemote (adaptor1);

    auto& remoteNode1 = *new IdpNode (TestGuid, "Remote.Node.1");
    auto& remoteNode2 = *new IdpNode (TestGuid, "Remote.Node.2");

    router2.AddNode (remoteNode1);
    router2.AddNode (remoteNode2);

    masterNode.EnumerateNetwork ();
    masterNode.TraceNetworkTree ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    auto& childNode3 = *new IdpNode (TestGuid, "Child.Node.3");
    auto& remoteNode3 = *new IdpNode (TestGuid, "Remote.Node.3");

    router.AddNode (childNode3);
    router2.AddNode (remoteNode3);

    masterNode.EnumerateNetwork ();
    masterNode.TraceNetworkTree ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    REQUIRE (childNode3.Address () != UnassignedAddress);
    REQUIRE (masterNode.HasNode (childNode3.Address ()));

    REQUIRE (remoteNode3.Address () != UnassignedAddress);
    REQUIRE (masterNode.HasNode (remoteNode3.Address ()));
}

/*   TEST_CASE ("Master Node Handles Enumeration Timeout")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();

    auto& childNode1 = *new IdpNode (0, 0, "");
    auto& childNode2 = *new IdpNode (0, 0, "");

    auto& idpRouter = *new IdpRouter ();

    idpRouter.AddNode (masterNode);
    idpRouter.AddNode (childNode1);
    idpRouter.AddNode (childNode2);

    childNode1.Enabled (false);

    masterNode.EnumerateNetwork ();

    TestRuntime::IterateRuntime (2000, 100);

    masterNode.EnumerateNetwork ();

    REQUIRE (childNode1.Address () == 0);
    REQUIRE (childNode2.Address () == 2);
}

TEST_CASE ("Master Node Recovers Timedout Enumeration")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();

    auto& childNode1 = *new IdpNode (0, 0, "");
    auto& childNode2 = *new IdpNode (0, 0, "");

    auto& idpRouter = *new IdpRouter ();

    idpRouter.AddNode (masterNode);
    idpRouter.AddNode (childNode1);
    idpRouter.AddNode (childNode2);

    childNode1.Enabled (false);

    masterNode.EnumerateNetwork ();

    TestRuntime::IterateRuntime (2000, 100);

    masterNode.EnumerateNetwork ();

    REQUIRE (childNode1.Address () == 0);
    REQUIRE (childNode2.Address () == 2);

    childNode1.Enabled (true);

    masterNode.EnumerateNetwork ();

    REQUIRE (childNode1.Address () == 3);
    REQUIRE (childNode2.Address () == 2);
}
*/

TEST_CASE ("MegaNetwork Enumerates")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();
    auto& router1 = *new IdpRouter ();
    auto& router2 = *new IdpRouter ();
    auto& router3 = *new IdpRouter ();

    auto& childNode1 = *new IdpNode (TestGuid, "Child.Node.1");

    router1.AddNode (masterNode);
    router1.AddNode (childNode1);

    auto& adaptor1 = *new SimpleAdaptor ();
    auto& adaptor2 = *new SimpleAdaptor ();

    router1.AddAdaptor (adaptor1);
    router2.AddAdaptor (adaptor2);

    adaptor1.SetRemote (adaptor2);
    adaptor2.SetRemote (adaptor1);

    auto& remoteNode1 = *new IdpNode (TestGuid, "Remote.Node.1");
    auto& remoteNode2 = *new IdpNode (TestGuid, "Remote.Node.2");
    auto& remoteNode3 = *new IdpNode (TestGuid, "Remote.Node.3");

    router2.AddNode (remoteNode1);
    router2.AddNode (remoteNode2);
    router2.AddNode (remoteNode3);

    auto& adaptor3 = *new SimpleAdaptor ();
    auto& adaptor4 = *new SimpleAdaptor ();

    router1.AddAdaptor (adaptor3);
    router3.AddAdaptor (adaptor4);

    adaptor3.SetRemote (adaptor4);
    adaptor4.SetRemote (adaptor3);

    auto& remoteNode4 = *new IdpNode (TestGuid, "Remote.Node.4");
    auto& remoteNode5 = *new IdpNode (TestGuid, "Remote.Node.5");
    auto& remoteNode6 = *new IdpNode (TestGuid, "Remote.Node.6");

    router3.AddNode (remoteNode4);
    router3.AddNode (remoteNode5);
    router3.AddNode (remoteNode6);

    masterNode.EnumerateNetwork ();
    masterNode.TraceNetworkTree ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    auto nodeInfo = masterNode.GetNodeInfo (remoteNode1.Address ());
    REQUIRE (nodeInfo.Parent->Address == router2.Address ());

    nodeInfo = masterNode.GetNodeInfo (remoteNode2.Address ());
    REQUIRE (nodeInfo.Parent->Address == router2.Address ());

    nodeInfo = masterNode.GetNodeInfo (remoteNode3.Address ());
    REQUIRE (nodeInfo.Parent->Address == router2.Address ());

    nodeInfo = masterNode.GetNodeInfo (remoteNode4.Address ());
    REQUIRE (nodeInfo.Parent->Address == router3.Address ());

    nodeInfo = masterNode.GetNodeInfo (remoteNode5.Address ());
    REQUIRE (nodeInfo.Parent->Address == router3.Address ());

    nodeInfo = masterNode.GetNodeInfo (remoteNode6.Address ());
    REQUIRE (nodeInfo.Parent->Address == router3.Address ());
}

TEST_CASE ("Nodes can be polled")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();
    auto& router = *new IdpRouter ();
    auto& router2 = *new IdpRouter ();

    auto& childNode1 = *new IdpNode (TestGuid, "Child.Node.1");
    auto& childNode2 = *new IdpNode (TestGuid, "Child.Node.2");

    router.AddNode (masterNode);
    router.AddNode (childNode1);
    router.AddNode (childNode2);

    auto& adaptor1 = *new SimpleAdaptor ();
    auto& adaptor2 = *new SimpleAdaptor ();

    router.AddAdaptor (adaptor1);
    router2.AddAdaptor (adaptor2);

    adaptor1.SetRemote (adaptor2);
    adaptor2.SetRemote (adaptor1);

    auto& remoteNode1 = *new IdpNode (TestGuid, "Remote.Node.1");
    auto& remoteNode2 = *new IdpNode (TestGuid, "Remote.Node.2");

    router2.AddNode (remoteNode1);
    router2.AddNode (remoteNode2);

    masterNode.EnumerateNetwork ();
    masterNode.TraceNetworkTree ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    TestRuntime::IterateRuntime (100);

    masterNode.PollNetwork ();

    REQUIRE (masterNode.GetNodeInfo (childNode1.Address ()).LastSeen == 100);
    REQUIRE (masterNode.GetNodeInfo (childNode2.Address ()).LastSeen == 100);
    REQUIRE (masterNode.GetNodeInfo (remoteNode1.Address ()).LastSeen == 100);
    REQUIRE (masterNode.GetNodeInfo (remoteNode2.Address ()).LastSeen == 100);
    REQUIRE (masterNode.GetNodeInfo (router.Address ()).LastSeen == 100);
    REQUIRE (masterNode.GetNodeInfo (router2.Address ()).LastSeen == 100);

    childNode2.Enabled (false);
    remoteNode2.Enabled (false);

    TestRuntime::IterateRuntime (100);

    masterNode.PollNetwork ();

    REQUIRE (masterNode.GetNodeInfo (childNode1.Address ()).LastSeen == 200);
    REQUIRE (masterNode.GetNodeInfo (childNode2.Address ()).LastSeen == 100);
    REQUIRE (masterNode.GetNodeInfo (remoteNode1.Address ()).LastSeen == 200);
    REQUIRE (masterNode.GetNodeInfo (remoteNode2.Address ()).LastSeen == 100);
    REQUIRE (masterNode.GetNodeInfo (router.Address ()).LastSeen == 200);
    REQUIRE (masterNode.GetNodeInfo (router2.Address ()).LastSeen == 200);

    childNode2.Enabled (true);
    remoteNode2.Enabled (true);

    TestRuntime::IterateRuntime (100);

    masterNode.PollNetwork ();

    REQUIRE (masterNode.GetNodeInfo (childNode1.Address ()).LastSeen == 300);
    REQUIRE (masterNode.GetNodeInfo (childNode2.Address ()).LastSeen == 300);
    REQUIRE (masterNode.GetNodeInfo (remoteNode1.Address ()).LastSeen == 300);
    REQUIRE (masterNode.GetNodeInfo (remoteNode2.Address ()).LastSeen == 300);
    REQUIRE (masterNode.GetNodeInfo (router.Address ()).LastSeen == 300);
    REQUIRE (masterNode.GetNodeInfo (router2.Address ()).LastSeen == 300);
}

TEST_CASE ("Master times out nodes that dont respond to polling")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();
    auto& router = *new IdpRouter ();
    auto& router2 = *new IdpRouter ();

    auto& childNode1 = *new IdpNode (TestGuid, "Child.Node.1");
    auto& childNode2 = *new IdpNode (TestGuid, "Child.Node.2");

    router.AddNode (masterNode);
    router.AddNode (childNode1);
    router.AddNode (childNode2);

    auto& adaptor1 = *new SimpleAdaptor ();
    auto& adaptor2 = *new SimpleAdaptor ();

    router.AddAdaptor (adaptor1);
    router2.AddAdaptor (adaptor2);

    adaptor1.SetRemote (adaptor2);
    adaptor2.SetRemote (adaptor1);

    auto& remoteNode1 = *new IdpNode (TestGuid, "Remote.Node.1");
    auto& remoteNode2 = *new IdpNode (TestGuid, "Remote.Node.2");

    router2.AddNode (remoteNode1);
    router2.AddNode (remoteNode2);

    masterNode.EnumerateNetwork ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    masterNode.TraceNetworkTree ();

    TestRuntime::IterateRuntime (100);

    masterNode.PollNetwork ();

    REQUIRE (masterNode.GetNodeInfo (childNode1.Address ()).LastSeen == 100);
    REQUIRE (masterNode.GetNodeInfo (childNode2.Address ()).LastSeen == 100);
    REQUIRE (masterNode.GetNodeInfo (remoteNode1.Address ()).LastSeen == 100);
    REQUIRE (masterNode.GetNodeInfo (remoteNode2.Address ()).LastSeen == 100);
    REQUIRE (masterNode.GetNodeInfo (router.Address ()).LastSeen == 100);
    REQUIRE (masterNode.GetNodeInfo (router2.Address ()).LastSeen == 100);

    childNode2.Enabled (false);

    TestRuntime::IterateRuntime (2500);

    masterNode.PollNetwork ();

    TestRuntime::IterateRuntime (2500);

    masterNode.PollNetwork ();

    REQUIRE (masterNode.HasNode (childNode1.Address ()));
    REQUIRE_FALSE (masterNode.HasNode (childNode2.Address ()));
    REQUIRE (masterNode.GetNodeInfo (childNode1.Address ()).LastSeen == 5100);

    masterNode.TraceNetworkTree ();

    router.RemoveNode (childNode2);

    router2.AddNode (childNode2);
    childNode2.Enabled (true);

    masterNode.EnumerateNetwork ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    masterNode.TraceNetworkTree ();

    REQUIRE (masterNode.HasNode (childNode2.Address ()));

    masterNode.PollNetwork ();

    REQUIRE (masterNode.GetNodeInfo (childNode2.Address ()).LastSeen == 5100);
}

TEST_CASE ("Master queries nodes for Node Info")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();
    auto& router = *new IdpRouter ();
    auto& router2 = *new IdpRouter ();

    auto& childNode1 = *new IdpNode (TestGuid, "Vendor.Bootloader");
    auto& childNode2 = *new IdpNode (TestGuid, "Vendor.CameraController");

    router.AddNode (masterNode);
    router.AddNode (childNode1);
    router.AddNode (childNode2);

    auto& adaptor1 = *new SimpleAdaptor ();
    auto& adaptor2 = *new SimpleAdaptor ();

    router.AddAdaptor (adaptor1);
    router2.AddAdaptor (adaptor2);

    adaptor1.SetRemote (adaptor2);
    adaptor2.SetRemote (adaptor1);

    auto& remoteNode1 = *new IdpNode (TestGuid, "Vendor.Bootloader");
    auto& remoteNode2 = *new IdpNode (TestGuid, "Vendor.CameraController");

    router2.AddNode (remoteNode1);
    router2.AddNode (remoteNode2);

    masterNode.EnumerateNetwork ();
    masterNode.TraceNetworkTree ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    REQUIRE (masterNode.GetNodeInfo (childNode1.Address ()).Guid == TestGuid);
    REQUIRE (masterNode.GetNodeInfo (childNode2.Address ()).Guid == TestGuid);
    REQUIRE (masterNode.GetNodeInfo (remoteNode1.Address ()).Guid == TestGuid);
    REQUIRE (masterNode.GetNodeInfo (remoteNode2.Address ()).Guid == TestGuid);

    REQUIRE (strcmp ("Vendor.Bootloader",
                     masterNode.GetNodeInfo (childNode1.Address ()).Name) == 0);

    REQUIRE (strcmp ("Vendor.CameraController",
                     masterNode.GetNodeInfo (childNode2.Address ()).Name) == 0);

    REQUIRE (strcmp ("Vendor.Bootloader",
                     masterNode.GetNodeInfo (remoteNode1.Address ()).Name) ==
             0);

    REQUIRE (strcmp ("Vendor.CameraController",
                     masterNode.GetNodeInfo (remoteNode2.Address ()).Name) ==
             0);
}

TEST_CASE ("Enumerate Router Adaptor Responds false immediately if adaptor is "
           "not ready")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();

    auto& router = *new IdpRouter ();

    auto& router2 = *new IdpRouter ();

    auto& childNode1 = *new IdpNode (TestGuid, "Child.Node.1");
    auto& childNode2 = *new IdpNode (TestGuid, "Child.Node.2");

    router.AddNode (masterNode);
    router.AddNode (childNode1);
    router.AddNode (childNode2);

    auto& adaptor1 = *new SimpleAdaptor ();
    auto& adaptor2 = *new SimpleAdaptor ();

    router.AddAdaptor (adaptor1);
    router2.AddAdaptor (adaptor2);

    masterNode.EnumerateNetwork ();
    masterNode.TraceNetworkTree ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    REQUIRE (masterNode.HasNode (router.Address ()));
    REQUIRE (masterNode.HasNode (childNode1.Address ()));
    REQUIRE (masterNode.HasNode (childNode2.Address ()));

    // Under test, no timeouts can occur, so if the adaptor itself didnt
    // notify the router that it wasnt able to transmit, then enumeration would
    // not complete.

    REQUIRE_FALSE (masterNode.IsEnumerating ());
    REQUIRE (router2.Address () == UnassignedAddress);
}

TEST_CASE (
    "Master can continue enumerating after an inactive adaptor is enumerated.")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();
    auto& router1 = *new IdpRouter ();
    auto& router2 = *new IdpRouter ();
    auto& router3 = *new IdpRouter ();

    auto& childNode1 = *new IdpNode (TestGuid, "Child.Node.1");

    router1.AddNode (masterNode);
    router1.AddNode (childNode1);

    auto& adaptor1 = *new SimpleAdaptor ();
    auto& adaptor2 = *new SimpleAdaptor ();

    router1.AddAdaptor (adaptor1);
    router2.AddAdaptor (adaptor2);

    adaptor2.SetRemote (adaptor1);

    auto& remoteNode1 = *new IdpNode (TestGuid, "Remote.Node.1");
    auto& remoteNode2 = *new IdpNode (TestGuid, "Remote.Node.2");
    auto& remoteNode3 = *new IdpNode (TestGuid, "Remote.Node.3");

    router2.AddNode (remoteNode1);
    router2.AddNode (remoteNode2);
    router2.AddNode (remoteNode3);

    auto& adaptor3 = *new SimpleAdaptor ();
    auto& adaptor4 = *new SimpleAdaptor ();

    router1.AddAdaptor (adaptor3);
    router3.AddAdaptor (adaptor4);

    adaptor3.SetRemote (adaptor4);
    adaptor4.SetRemote (adaptor3);

    auto& remoteNode4 = *new IdpNode (TestGuid, "Remote.Node.4");
    auto& remoteNode5 = *new IdpNode (TestGuid, "Remote.Node.5");
    auto& remoteNode6 = *new IdpNode (TestGuid, "Remote.Node.6");

    router3.AddNode (remoteNode4);
    router3.AddNode (remoteNode5);
    router3.AddNode (remoteNode6);

    masterNode.EnumerateNetwork ();
    masterNode.TraceNetworkTree ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    REQUIRE_FALSE (remoteNode4.Address () == UnassignedAddress);

    auto nodeInfo = masterNode.GetNodeInfo (remoteNode4.Address ());
    REQUIRE (nodeInfo.Parent->Address == router3.Address ());

    nodeInfo = masterNode.GetNodeInfo (remoteNode5.Address ());
    REQUIRE (nodeInfo.Parent->Address == router3.Address ());

    nodeInfo = masterNode.GetNodeInfo (remoteNode6.Address ());
    REQUIRE (nodeInfo.Parent->Address == router3.Address ());
}

TEST_CASE (
    "Adaptors that become active after initial enumeration get enumerated")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();
    auto& router1 = *new IdpRouter ();
    auto& router2 = *new IdpRouter ();
    auto& router3 = *new IdpRouter ();

    auto& childNode1 = *new IdpNode (TestGuid, "Child.Node.1");

    router1.AddNode (masterNode);
    router1.AddNode (childNode1);

    auto& adaptor1 = *new SimpleAdaptor ();
    auto& adaptor2 = *new SimpleAdaptor ();

    router1.AddAdaptor (adaptor1);
    router2.AddAdaptor (adaptor2);

    adaptor2.SetRemote (adaptor1);

    auto& remoteNode1 = *new IdpNode (TestGuid, "Remote.Node.1");
    auto& remoteNode2 = *new IdpNode (TestGuid, "Remote.Node.2");
    auto& remoteNode3 = *new IdpNode (TestGuid, "Remote.Node.3");

    router2.AddNode (remoteNode1);
    router2.AddNode (remoteNode2);
    router2.AddNode (remoteNode3);

    auto& adaptor3 = *new SimpleAdaptor ();
    auto& adaptor4 = *new SimpleAdaptor ();

    router1.AddAdaptor (adaptor3);
    router3.AddAdaptor (adaptor4);

    adaptor3.SetRemote (adaptor4);
    adaptor4.SetRemote (adaptor3);

    auto& remoteNode4 = *new IdpNode (TestGuid, "Remote.Node.4");
    auto& remoteNode5 = *new IdpNode (TestGuid, "Remote.Node.5");
    auto& remoteNode6 = *new IdpNode (TestGuid, "Remote.Node.6");

    router3.AddNode (remoteNode4);
    router3.AddNode (remoteNode5);
    router3.AddNode (remoteNode6);

    masterNode.EnumerateNetwork ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    auto nodeInfo = masterNode.GetNodeInfo (remoteNode4.Address ());
    REQUIRE (nodeInfo.Parent->Address == router3.Address ());

    nodeInfo = masterNode.GetNodeInfo (remoteNode5.Address ());
    REQUIRE (nodeInfo.Parent->Address == router3.Address ());

    nodeInfo = masterNode.GetNodeInfo (remoteNode6.Address ());
    REQUIRE (nodeInfo.Parent->Address == router3.Address ());

    REQUIRE (remoteNode1.Address () == UnassignedAddress);
    REQUIRE (remoteNode2.Address () == UnassignedAddress);
    REQUIRE (remoteNode3.Address () == UnassignedAddress);

    adaptor1.SetRemote (adaptor2);

    masterNode.EnumerateNetwork ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    nodeInfo = masterNode.GetNodeInfo (remoteNode1.Address ());
    REQUIRE (nodeInfo.Parent->Address == router2.Address ());

    nodeInfo = masterNode.GetNodeInfo (remoteNode2.Address ());
    REQUIRE (nodeInfo.Parent->Address == router2.Address ());

    nodeInfo = masterNode.GetNodeInfo (remoteNode3.Address ());
    REQUIRE (nodeInfo.Parent->Address == router2.Address ());
}

TEST_CASE ("Network is reenumerated following collapse")
{
    TestRuntime::Initialise ();

    auto& masterNode = *new MasterNode ();
    auto& router = *new IdpRouter ();
    auto& router2 = *new IdpRouter ();

    auto& childNode1 = *new IdpNode (TestGuid, "Child.Node.1");
    auto& childNode2 = *new IdpNode (TestGuid, "Child.Node.2");

    router.AddNode (masterNode);
    router.AddNode (childNode1);
    router.AddNode (childNode2);

    auto& adaptor1 = *new SimpleAdaptor ();
    auto& adaptor2 = *new SimpleAdaptor ();

    router.AddAdaptor (adaptor1);
    router2.AddAdaptor (adaptor2);

    adaptor1.SetRemote (adaptor2);
    adaptor2.SetRemote (adaptor1);

    auto& remoteNode1 = *new IdpNode (TestGuid, "Remote.Node.1");
    auto& remoteNode2 = *new IdpNode (TestGuid, "Remote.Node.2");

    router2.AddNode (remoteNode1);
    router2.AddNode (remoteNode2);

    masterNode.EnumerateNetwork ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());

    masterNode.PollNetwork ();

    TestRuntime::IterateRuntime (10000);

    masterNode.PollNetwork ();

    TestRuntime::IterateRuntime (10000);
    TestRuntime::IterateRuntime (1000);

    masterNode.EnumerateNetwork ();

    REQUIRE_FALSE (masterNode.IsEnumerating ());
}
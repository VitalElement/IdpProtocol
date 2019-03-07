/******************************************************************************
 *  Description:
 *
 *       Author:
 *
 ******************************************************************************/

#include "IdpClientNode.h"
#include "Application.h"
#include "Trace.h"

IdpClientNode::IdpClientNode (Guid_t serverGuid, Guid_t guid, const char* name,
                              uint16_t address)
    : IdpNode (guid, name, address)
{
    _serverAddress = UnassignedAddress;

    _serverGuid = serverGuid;

    _lastPing = 0;

    _pollTimer = new DispatcherTimer (1000, false);

    _pollTimer->Tick += [&](auto sender, auto& e) {
        if (_serverAddress == UnassignedAddress)
        {
            QueryInterface (_serverGuid);
        }
        else
        {
            auto elapsedTime = Application::GetApplicationTime () - _lastPing;

            if (elapsedTime > 4000)
            {
                OnDisconnect ();
                _lastPing = Application::GetApplicationTime ();
            }
            else
            {
                if (!this->SendRequest (
                        _serverAddress,
                        OutgoingTransaction ::Create (
                            static_cast<uint16_t> (NodeCommand::Ping),
                            this->CreateTransactionId ()),
                        [&](std::shared_ptr<IdpResponse> response) {
                            if (response->ResponseCode () ==
                                IdpResponseCode::OK)
                            {
                                _lastPing = Application::GetApplicationTime ();
                            }
                        }))
                {
                    OnDisconnect ();
                }
            }
        }
    };
}

IdpClientNode::~IdpClientNode ()
{
}

void IdpClientNode::OnConnect (uint16_t serverAddress)
{
    if (_serverAddress == UnassignedAddress)
    {
        Trace::WriteLine ("Client Connected", "IdpClientNode");

        _serverAddress = serverAddress;

        Connected (this, EventArgs::Empty);

        _lastPing = Application::GetApplicationTime ();
    }
}

void IdpClientNode::OnDisconnect ()
{
    if (_serverAddress != UnassignedAddress)
    {
        Trace::WriteLine ("Client Disconnected", "IdpClientNode");

        _serverAddress = UnassignedAddress;

        Disconnected (this, EventArgs::Empty);
    }
}

void IdpClientNode::Connect ()
{
    Trace::WriteLine ("Connecting Client", "IdpClientNode");

    _lastPing = Application::GetApplicationTime ();

    QueryInterface (_serverGuid);

    _pollTimer->Start ();
}

bool IdpClientNode::IsConnected ()
{
    return _serverAddress != UnassignedAddress;
}

void IdpClientNode::QueryInterface (Guid_t guid)
{
    SendRequest (0,
                 OutgoingTransaction::Create (
                     (uint16_t) NodeCommand::QueryInterface,
                     CreateTransactionId (), IdpCommandFlags::None)
                     ->WriteGuid (guid),
                 [&](std::shared_ptr<IdpResponse> response) {
                     if (response != nullptr &&
                         response->ResponseCode () == IdpResponseCode::OK &&
                         _serverAddress == UnassignedAddress)
                     {
                         OnConnect (response->Transaction ()->Source ());
                     }
                 });
}

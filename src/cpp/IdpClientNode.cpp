/******************************************************************************
 *  Description:
 *
 *       Author:
 *
 ******************************************************************************/

#include "IdpClientNode.h"
#include "Application.h"

IdpClientNode::IdpClientNode (Guid_t serverGuid, Guid_t guid, const char* name,
                              uint16_t address)
    : IdpNode (guid, name, address)
{
    _serverAddress = UnassignedAddress;

    _serverGuid = serverGuid;

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
            }
            else
            {
                if (!this->SendRequest (
                        _serverAddress,
                        OutgoingTransaction ::Create (
                            static_cast<uint16_t> (NodeCommand::Ping),
                            this->CreateTransactionId ())))
                {
                }
            }
        }
    };
}

IdpClientNode::~IdpClientNode ()
{
}

void IdpClientNode::QueryInterface (Guid_t guid)
{
    SendRequest (0,
                 OutgoingTransaction::Create (
                     (uint16_t) NodeCommand::QueryInterface,
                     CreateTransactionId (), IdpCommandFlags::None)
                     ->WriteGuid (guid),
                 [&](std::shared_ptr<IdpResponse> response) {
                     if (response->ResponseCode () == IdpResponseCode::OK &&
                         _serverAddress == UnassignedAddress)
                     {
                         _serverAddress = response->Transaction ()->Source ();
                     }
                 });
}

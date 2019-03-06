/******************************************************************************
 *  Description:
 *
 *       Author:
 *
 ******************************************************************************/

#include "IdpServerNode.h"

IdpServerNode::IdpServerNode (Guid_t guid, const char* name, uint16_t address)
    : IdpNode (guid, name, address)
{
    _clientAddress = UnassignedAddress;

    Manager ().RegisterCommand (static_cast<uint16_t> (ClientCommand::Connect),
                                [&](std::shared_ptr<IncomingTransaction> i,
                                    std::shared_ptr<OutgoingTransaction> o) {
                                    _clientAddress = i->Source ();

                                    return IdpResponseCode::OK;
                                });

    Manager ().RegisterCommand (
        static_cast<uint16_t> (ClientCommand::Disconnect),
        [&](std::shared_ptr<IncomingTransaction> i,
            std::shared_ptr<OutgoingTransaction> o) {
            _clientAddress = UnassignedAddress;

            return IdpResponseCode::OK;
        });

    Manager ().RegisterResponseHandler (
        static_cast<uint16_t> (NodeCommand::QueryInterface),
        [&](std::shared_ptr<IdpResponse> response) {
            if (response->ResponseCode () == IdpResponseCode::OK &&
                _clientAddress == UnassignedAddress)
            {
                _clientAddress = response->Transaction ()->Source ();
            }
        });
}

IdpServerNode::~IdpServerNode ()
{
}

bool IdpServerNode::IsClientConnected ()
{
    return _clientAddress != UnassignedAddress;
}

uint16_t IdpServerNode::ClientAddress ()
{
    return _clientAddress;
}

void IdpServerNode::QueryInterface (Guid_t guid)
{
    SendRequest (0, OutgoingTransaction::Create (
                        (uint16_t) NodeCommand::QueryInterface,
                        CreateTransactionId (), IdpCommandFlags::None)
                        ->WriteGuid (guid));
}

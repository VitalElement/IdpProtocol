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

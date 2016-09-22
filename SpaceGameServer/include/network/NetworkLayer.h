#ifndef _NETWORK_LAYER_H_
#define _NETWORK_LAYER_H_

#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"

#include <map>

#define SERVER_PORT 60000 //TODO useconfig file or prompt
#define MAX_CLIENTS 10
//TODO close connection, reinit connection etc

class INetworkService;

namespace RakNet
{
	class RakString;
}

class NetworkLayer
{
public:
	typedef char NetworkChannel;

	NetworkLayer();
	~NetworkLayer()	{ RakNet::RakPeerInterface::DestroyInstance(mPeer);	}

	void init();
	void registerNetworkService(INetworkService* _networkService) { mNetworkService = _networkService; }

	void send(const RakNet::AddressOrGUID& _clientAdress, const RakNet::RakString& _message, PacketPriority _priority, PacketReliability _reliability, NetworkChannel _orderingChannel, RakNet::MessageID _messageType) const;
	void send(const RakNet::AddressOrGUID& _clientAdress, RakNet::BitStream& _bitStream, PacketPriority _priority, PacketReliability _reliability, NetworkChannel _orderingChannel, RakNet::MessageID _messageType) const;
	
	void getNetworkData();

protected:
	RakNet::RakPeerInterface *mPeer;

	INetworkService* mNetworkService;

	RakNet::SocketDescriptor mSocketDescriptor;

	void handlePacket(RakNet::Packet* _packet);
};

#endif //_NETWORK_LAYER_H_
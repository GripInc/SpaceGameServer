#include "network/NetworkLayer.h"

#include "network/INetworkService.h"

#include "BitStream.h"
#include "RakString.h"
#include <iostream>
#include <sstream>

#include "manager/LoggerManager.h"

namespace
{
	const std::string LOG_CLASS_TAG = "NetworkLayer";
}

NetworkLayer::NetworkLayer()
{
	mPeer = RakNet::RakPeerInterface::GetInstance();
	mPeer->SetOccasionalPing(true);

	//DEBUG
	//mPeer->ApplyNetworkSimulator(0.2, 1000, 1000);
}

void NetworkLayer::init()
{
	RakNet::SocketDescriptor socketDescriptor(SERVER_PORT, 0); //TODO set real values
	mPeer->Startup(MAX_CLIENTS, &socketDescriptor, 1); //TODO set real values

	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "init", "Starting the server.", true);
	// We need to let the server accept incoming connections from the clients
	mPeer->SetMaximumIncomingConnections(MAX_CLIENTS);
}

void NetworkLayer::handlePacket(RakNet::Packet* _packet)
{
	switch (_packet->data[0])
	{
	case ID_REMOTE_DISCONNECTION_NOTIFICATION:
		LoggerManager::getInstance().logI(LOG_CLASS_TAG, "handlePacket", "Another client has disconnected.", true);
		break;
	case ID_REMOTE_CONNECTION_LOST:
		LoggerManager::getInstance().logI(LOG_CLASS_TAG, "handlePacket", "Another client has lost the connection.", true);
		break;
	case ID_REMOTE_NEW_INCOMING_CONNECTION:
		LoggerManager::getInstance().logI(LOG_CLASS_TAG, "handlePacket", "Another client has connected.", true);
		break;
	case ID_NEW_INCOMING_CONNECTION:
		LoggerManager::getInstance().logI(LOG_CLASS_TAG, "handlePacket", "A connection is incoming.", true);
		break;
	case ID_NO_FREE_INCOMING_CONNECTIONS:
		LoggerManager::getInstance().logI(LOG_CLASS_TAG, "handlePacket", "The server is full.", true);
		break;
	case ID_DISCONNECTION_NOTIFICATION:
		LoggerManager::getInstance().logI(LOG_CLASS_TAG, "handlePacket", "A client has disconnected.", true);
		break;
	case ID_CONNECTION_LOST:
		LoggerManager::getInstance().logI(LOG_CLASS_TAG, "handlePacket", "A client lost the connection.", true);
		break;
	default:
		//Packets destinated to be treated by network service
		mNetworkService->handlePacket(_packet);
		break;
	}
}

void NetworkLayer::send(const RakNet::AddressOrGUID& _client, const RakNet::RakString& _message, PacketPriority _priority, PacketReliability _reliability, NetworkChannel _orderingChannel, RakNet::MessageID _messageType) const
{
	//TODO use compressed? -> seems compressed for understandable words only

	RakNet::BitStream bitStreamOut;
	bitStreamOut.Write((RakNet::MessageID)_messageType);
	bitStreamOut.Write(_message);
	mPeer->Send(&bitStreamOut, _priority, _reliability, _orderingChannel, _client, false);
}

void NetworkLayer::send(const RakNet::AddressOrGUID& _client, RakNet::BitStream& _bitStream, PacketPriority _priority, PacketReliability _reliability, NetworkChannel _orderingChannel, RakNet::MessageID _messageType) const
{
	RakNet::BitStream bitStreamOut;
	bitStreamOut.Write((RakNet::MessageID)_messageType);
	bitStreamOut.Write(_bitStream, _bitStream.GetNumberOfBitsUsed() - _bitStream.GetReadOffset());
	mPeer->Send(&bitStreamOut, _priority, _reliability, _orderingChannel, _client, false);
}

void NetworkLayer::getNetworkData()
{
	RakNet::Packet *packet;

	for (packet = mPeer->Receive(); packet != NULL; mPeer->DeallocatePacket(packet), packet = mPeer->Receive())
	{
		handlePacket(packet);
	}
}
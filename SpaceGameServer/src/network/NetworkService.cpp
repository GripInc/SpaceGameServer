#include "network/NetworkService.h"

#include "controller/GameController.h"

#include "model/PlayersData.h"
#include "model/GameSettings.h"
#include "model/InputState.h"

#include <iostream>

#include "RakString.h"
#include "BitStream.h"

#include "manager/LoggerManager.h"

namespace
{
	const std::string LOG_CLASS_TAG = "NetworkService";
}

const char NetworkService::LEVEL_1_CHANNEL = 1;

NetworkService* NetworkService::mInstance;

NetworkService& NetworkService::getInstance()
{
	if(!mInstance)
		mInstance = new NetworkService();

	return *mInstance;
}

void NetworkService::init(NetworkLayer* _networkLayer, GameController* _gameController)
{
	mNetworkLayer = _networkLayer;
	mGameController = _gameController;
}

void NetworkService::processNetworkBuffer()
{
	mNetworkLayer->getNetworkData();
}

void NetworkService::handlePacket(RakNet::Packet* _packet)
{
	RakNet::BitStream stream(_packet->data, _packet->length, false);
	RakNet::MessageID messageType;
	stream.Read((RakNet::MessageID)messageType);

	switch (messageType)
	{
	//A client asked its data
	case ID_GAME_MESSAGE_GET_PLAYER_DATA:
		{
			RakNet::RakString playerId;
			stream.Read(playerId);

			LoggerManager::getInstance().logI(LOG_CLASS_TAG, "handlePacket", "Received ID_GAME_MESSAGE_GET_PLAYER_DATA from player " + std::string(playerId.C_String()) + " with GUID:" + std::string(_packet->guid.ToString()), false);

			PlayerData* playerData = PlayersData::getInstance().getPlayerData(playerId.C_String());
			if(playerData)
			{
				mGameController->addPlayer(_packet->guid, playerData);
				sendPlayerData(playerData, _packet->systemAddress);
			}
			else
			{
				LoggerManager::getInstance().logE(LOG_CLASS_TAG, "handlePacket", "No player data for player " + std::string(playerId.C_String()));
				assert(false);
			}
		}
		break;
	//A client required to launch a ship in space
	case ID_GAME_MESSAGE_REQUIRE_LAUNCH:
		{
			LoggerManager::getInstance().logI(LOG_CLASS_TAG, "handlePacket", "Received ID_GAME_MESSAGE_REQUIRE_LAUNCH from GUID:" + std::string(_packet->guid.ToString()), false);

			std::string sector;
			Ogre::Vector3 position;
			Ogre::Quaternion orientation;
			UniqueId shipUniqueId;
			SectorTick sectorTick;
			
			mGameController->instantiateClientShip(_packet->guid, sector, position, orientation, shipUniqueId, sectorTick);
			sendPlayerLaunchPoint(_packet->guid, _packet->systemAddress, sector, position, orientation, shipUniqueId, sectorTick);
		}
		break;
	case ID_GAME_MESSAGE_INPUT_STATE:
		{
			LoggerManager::getInstance().logI(LOG_CLASS_TAG, "handlePacket", "Received ID_GAME_MESSAGE_INPUT_STATE from GUID:" + std::string(_packet->guid.ToString()), false);

			handleClientInput(_packet->guid, stream);
		}
		break;
	default:
		LoggerManager::getInstance().logW(LOG_CLASS_TAG, "handlePacket", "A message with unknown identifier '" + StringUtils::toStr(messageType) + "' has arrived from GUID:" + std::string(_packet->guid.ToString()));
	}
}

void NetworkService::sendPlayerData(const PlayerData* _playerData, const RakNet::SystemAddress& _clientAdress) const
{
	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "sendPlayerData", "Send player data to player " + _playerData->mPlayerId + "; Address:" + std::string(_clientAdress.ToString()), false);
	mNetworkLayer->send(_clientAdress, RakNet::RakString(_playerData->mOriginalJsonNode.toStyledString().c_str()), IMMEDIATE_PRIORITY, RELIABLE_ORDERED, LEVEL_1_CHANNEL, ID_GAME_MESSAGE_GET_PLAYER_DATA);
}

void NetworkService::sendPlayerLaunchPoint(const RakNet::RakNetGUID& _clientId, const RakNet::SystemAddress& _clientAdress, const std::string& _sector, Ogre::Vector3& _position, const Ogre::Quaternion& _orientation, UniqueId _uniqueId, SectorTick _sectorTick) const
{
	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "sendPlayerLaunchPoint", "_sector is : " + _sector + "; _uniqueId is : " + StringUtils::toStr(_uniqueId) + "; _sectorTick is : " + StringUtils::toStr(_sectorTick) + "; GUID is :" + std::string(_clientId.ToString()), false);

	RakNet::BitStream stream;
	stream.Write(RakNet::RakString(_sector.c_str()));
	stream.Write(_position);
	stream.Write(_orientation);
	stream.Write(_uniqueId);
	stream.Write(_sectorTick);
	mNetworkLayer->send(_clientAdress, stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, LEVEL_1_CHANNEL, ID_GAME_MESSAGE_REQUIRE_LAUNCH);
}

void NetworkService::handleClientInput(const RakNet::RakNetGUID& _clientId, RakNet::BitStream& _data)
{
	SectorTick tick;
	InputState inputState;

	_data.Read(tick);
	_data.Read(inputState);

	//Push client input
	mGameController->addInput(_clientId, tick, inputState);
}

void NetworkService::broadcastSector(const std::set<RakNet::RakNetGUID>& _clientsIds, RakNet::BitStream& _data)
{
	//Make log string
	std::string clientIdsList = "";
	std::set<RakNet::RakNetGUID>::const_iterator it = _clientsIds.begin();
	const std::set<RakNet::RakNetGUID>::const_iterator itEnd = _clientsIds.end();
	for(; it != itEnd; ++it)
	{
		clientIdsList += (*it).ToString();
		clientIdsList += ";";
	}
	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "broadcastSector", "Broadcasting to clients : " + clientIdsList, false);

	const std::set<RakNet::RakNetGUID>::const_iterator clientsIdsItEnd = _clientsIds.end();
	for(std::set<RakNet::RakNetGUID>::const_iterator clientsIdsIt = _clientsIds.begin(); clientsIdsIt != clientsIdsItEnd; ++clientsIdsIt)
		mNetworkLayer->send(*clientsIdsIt, _data, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, LEVEL_1_CHANNEL, ID_GAME_MESSAGE_SECTOR_STATE);
}
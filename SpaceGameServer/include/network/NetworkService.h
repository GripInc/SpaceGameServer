#ifndef _NETWORK_SERVICE_H_
#define _NETWORK_SERVICE_H_

#include "network/INetworkService.h"
#include "network/NetworkLayer.h"

#include <set>

#include "SpaceGameTypes.h"

class PlayerData;
class GameController;

namespace RakNet
{
	struct SystemAddress;
}

namespace Ogre
{
	class Vector3;
	class Quaternion;
}

class NetworkService : public INetworkService
{
public:
	static const char LEVEL_1_CHANNEL;

	///Singleton
	static NetworkService& getInstance();
	void init(NetworkLayer* _networkLayer, GameController* _gameController);

	enum GameMessages 
	{ 
		ID_GAME_MESSAGE_GET_PLAYER_DATA = ID_USER_PACKET_ENUM,
		ID_GAME_MESSAGE_REQUIRE_LAUNCH,
		ID_GAME_MESSAGE_INPUT_STATE,
		ID_GAME_MESSAGE_SECTOR_STATE
	};

	void processNetworkBuffer();
	virtual void handlePacket(RakNet::Packet* _packet);

	void broadcastSector(const std::set<RakNet::RakNetGUID>& _clientIds, RakNet::BitStream& _data);

protected:
	///Singleton
	static NetworkService* mInstance;
	NetworkService() {}

	NetworkLayer* mNetworkLayer;
	GameController* mGameController;

	void sendPlayerData(const PlayerData* _playerData, const RakNet::SystemAddress& _clientAdress) const;
	void sendPlayerLaunchPoint(const RakNet::RakNetGUID& _clientId, const RakNet::SystemAddress& _clientAdress, const std::string& _sector, Ogre::Vector3& _position, const Ogre::Quaternion& _orientation, UniqueId _uniqueId, SectorTick _sectorTick) const;
	void handleClientInput(const RakNet::RakNetGUID& _clientId, RakNet::BitStream& _data);
};

#endif //_NETWORK_SERVICE_H_
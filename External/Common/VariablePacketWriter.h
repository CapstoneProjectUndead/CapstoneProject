#pragma once
//==================================
// **** 클라/서버 공용 헤더 파일 ****
//=================================+

// 앞으로 가변길이 패킷은 여기서 처리한다.

template<typename T>
class S_WRITE
{
public:
	SendBufferRef CloseAndReturn()
	{
		// 패킷 사이즈 계산
		pkt->SetPacketSize(bw.WriteSize());

		sendBuffer->Close(bw.WriteSize());
		return sendBuffer;
	}

protected:
	T* pkt = nullptr;
	SendBufferRef	sendBuffer;
	BufferWriter	bw;
};


class S_PLAYERLIST_WRITE : public S_WRITE<S_PLAYER_LIST>
{
public:
	using User = S_PLAYER_LIST::Player;
	using UserList = PacketList<S_PLAYER_LIST::Player>;

	// 고정된 부분들은 생성자 인자로 받아서
	// 생성자 내부에서 직렬화한다. 
	S_PLAYERLIST_WRITE()
	{
		//_sendBuffer = GSendBufferManager->Open(4096);
		//_bw = BufferWriter(_sendBuffer->Buffer(), _sendBuffer->AllocSize());
		//
		//_pkt = _bw.Reserve<PKT_S_TEST>();
		//_pkt->packetSize = 0; // To Fill
		//_pkt->packetId = S_TEST;
		//_pkt->id = id;
		//_pkt->hp = hp;
		//_pkt->attack = attack;
		//_pkt->buffsOffset = 0; // To Fill
		//_pkt->buffsCount = 0; // To Fill

		sendBuffer = std::make_shared<SendBuffer>(4096);
		bw = BufferWriter(sendBuffer->Buffer(), 4096);

		pkt = bw.Reserve<S_PLAYER_LIST>(1);
		//pkt->SetPacketSize(4096);
		pkt->SetPacketType((UINT)PacketType::_S_PLAYER_LIST);
	}

	S_PLAYERLIST_WRITE(uint32 roomId, SCENE_TYPE type)
	{
		sendBuffer = std::make_shared<SendBuffer>(4096);
		bw = BufferWriter(sendBuffer->Buffer(), 4096);

		pkt = bw.Reserve<S_PLAYER_LIST>(1);
		pkt->SetPacketType((UINT)PacketType::_S_PLAYER_LIST);
		pkt->room_id = roomId;
		pkt->scene_type = type;
	}

	UserList ReserveUserList(uint16 userCount)
	{
		S_PLAYER_LIST::Player* firstUserList = bw.Reserve<S_PLAYER_LIST::Player>(userCount);
		pkt->buff_offset = (uint64)firstUserList - (uint64)pkt;
		pkt->player_count = userCount;
		return UserList(firstUserList, userCount);
	}
};

class S_ROOMLIST_WRITE : public S_WRITE<S_Room_List>
{
public:
	using Room = S_Room_List::Room;
	using RoomList = PacketList<S_Room_List::Room>;

	// 고정된 부분들은 생성자 인자로 받아서
	// 생성자 내부에서 직렬화한다. 
	S_ROOMLIST_WRITE()
	{
		sendBuffer = std::make_shared<SendBuffer>(4096);
		bw = BufferWriter(sendBuffer->Buffer(), 4096);

		pkt = bw.Reserve<S_Room_List>(1);
		//pkt->SetPacketSize(4096);
		pkt->SetPacketType((UINT)PacketType::_S_ROOM_LIST);
	}

	RoomList ReserveRoomList(uint16 roomCount)
	{
		S_Room_List::Room* firstRoomList = bw.Reserve<S_Room_List::Room>(roomCount);
		pkt->buff_offset = (uint64)firstRoomList - (uint64)pkt;
		pkt->room_count = roomCount;
		return RoomList(firstRoomList, roomCount);
	}
};

class S_SPAWN_ITEMLIST_WRITE : public S_WRITE<S_Spawn_Item_List>
{
public:
	using Item = S_Spawn_Item_List::Item;
	using ItemList = PacketList<S_Spawn_Item_List::Item>;

	S_SPAWN_ITEMLIST_WRITE(SCENE_TYPE sceneType)
	{
		sendBuffer = std::make_shared<SendBuffer>(4096);
		bw = BufferWriter(sendBuffer->Buffer(), 4096);

		pkt = bw.Reserve<S_Spawn_Item_List>(1);
		pkt->SetPacketType((UINT)PacketType::_S_SPAWN_ITEM_LIST);
		pkt->scene_type = sceneType;
	}

	ItemList ReserveItemList(uint32 itemCount)
	{
		S_Spawn_Item_List::Item* firstItem = bw.Reserve<S_Spawn_Item_List::Item>(itemCount);
		pkt->buff_offset = (uint64)firstItem - (uint64)pkt;
		pkt->item_count = itemCount;
		return ItemList(firstItem, itemCount);
	}
};

class S_MINEABLELIST_WRITE : public S_WRITE<S_MineableList>
{
public:
	using Mineable = S_MineableList::Mineable;
	using MineableList = PacketList<S_MineableList::Mineable>;

	S_MINEABLELIST_WRITE(SCENE_TYPE sceneType)
	{
		sendBuffer = std::make_shared<SendBuffer>(4096);
		bw = BufferWriter(sendBuffer->Buffer(), 4096);

		pkt = bw.Reserve<S_MineableList>(1);
		pkt->SetPacketType((UINT)PacketType::_S_MINEABLE_LIST);
		pkt->scene_type = sceneType;
	}

	MineableList ReserveMineableList(uint32 count)
	{
		S_MineableList::Mineable* first = bw.Reserve<S_MineableList::Mineable>(count);
		pkt->buff_offset = (uint64)first - (uint64)pkt;
		pkt->mineable_count = count;
		return MineableList(first, count);
	}
};

class S_ADDITEMLIST_WRITE : public S_WRITE<S_AddItemList>
{
public:
	using Item = S_AddItemList::Item;
	using ItemList = PacketList<S_AddItemList::Item>;

	S_ADDITEMLIST_WRITE(uint64 playerId, SCENE_TYPE sceneType)
	{
		sendBuffer = std::make_shared<SendBuffer>(4096);
		bw = BufferWriter(sendBuffer->Buffer(), 4096);

		pkt = bw.Reserve<S_AddItemList>(1);
		pkt->SetPacketType((UINT)PacketType::_S_ADD_ITEM_LIST);
		pkt->player_id = playerId;
		pkt->scene_type = sceneType;
	}

	ItemList ReserveItemList(uint16 itemCount)
	{
		S_AddItemList::Item* firstItem = bw.Reserve<S_AddItemList::Item>(itemCount);
		pkt->buff_offset = (uint64)firstItem - (uint64)pkt;
		pkt->item_count = itemCount;
		return ItemList(firstItem, itemCount);
	}
};

class S_GAMESETTLEMENT_WRITE : public S_WRITE<S_GameSettlement>
{
public:
	using Entry = S_GameSettlement::TreasureEntry;
	using TreasureList = PacketList<S_GameSettlement::TreasureEntry>;

	S_GAMESETTLEMENT_WRITE(uint32 baseCoin, uint32 finalCoin,
	                       bool is_returned, bool allReturnedBonus,
	                       SCENE_TYPE sceneType)
	{
		sendBuffer = std::make_shared<SendBuffer>(4096);
		bw = BufferWriter(sendBuffer->Buffer(), 4096);

		pkt = bw.Reserve<S_GameSettlement>(1);
		pkt->SetPacketType((UINT)PacketType::_S_GAME_SETTLEMENT);
		pkt->base_coin          = baseCoin;
		pkt->final_coin         = finalCoin;
		pkt->is_returned           = is_returned;
		pkt->all_returned_bonus = allReturnedBonus;
		pkt->scene_type         = sceneType;
	}

	TreasureList ReserveTreasureList(uint16 count)
	{
		Entry* first = bw.Reserve<Entry>(count);
		pkt->buff_offset      = (uint16)((uint64)first - (uint64)pkt);
		pkt->treasure_count   = count;
		return TreasureList(first, count);
	}
};
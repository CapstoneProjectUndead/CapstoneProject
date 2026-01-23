#pragma once
//#include "protocol.h"

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
	T*				pkt = nullptr;
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
		pkt->SetPacketType((UINT)PacketType::_S_PLAYERLIST);
	}

	UserList ReserveUserList(uint16 userCount)
	{
		S_PLAYER_LIST::Player* firstUserList = bw.Reserve<S_PLAYER_LIST::Player>(userCount);
		pkt->buff_offset = (uint64)firstUserList - (uint64)pkt;
		pkt->player_count = userCount;
		return UserList(firstUserList, userCount);
	}
};

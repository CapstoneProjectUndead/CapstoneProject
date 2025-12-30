#pragma once
#include "NetAddress.h"
#include "OVER_EXP.h"

enum S_STATE 
{ 
	ST_FREE, 
	ST_ALLOC, 
	ST_INGAME, 
};

class Session
{
	friend class Listener;
	friend class Service;
	friend class IOCP;

public:
	Session();
	virtual ~Session();

public:
	bool					DoConnect();
	void					DoDisconnect(const WCHAR* cause);
	void					DoSend(SendBufferRef sendBuffer);

public:
	SOCKET					GetSocket() { return socket; }
	void					SetNetAddress(NetAddress address) { net_address = address; }
	NetAddress				GetNetAddress() { return net_address; }
	void					SetService(Service* service) { service_ref = service; }
	Service*				GetService() { return service_ref; }
	void					SetState(S_STATE _state) { state = _state; }
	S_STATE					GetState() { return state; }
	bool					IsConnected() { return is_connect; }

private:
	void					DoRecv();
	void					RegisterSend();

	void					HandleConnect();
	void					HandleDisConnect();
	void					HandleRecv(int numOfBytes);
	void					HandleSend(int numOfBytes);

	bool					OnRecv(BYTE* buffer, int32 numOfBytes);

protected:
	// ÄÁÅÙÃ÷ ÄÚµå¿¡¼­ ÀçÁ¤ÀÇ
	virtual void			OnConnected() {}
	virtual void			OnSend(int32 len) {}
	virtual void			OnDisconnected() {}
	virtual void			ProcessPacket(Session*, char*, int32 pktSize) abstract;

private:
	mutex					lock;
	queue<SendBufferRef>	send_queue;
	atomic<bool>			send_registered;

private:
	SOCKET					socket;
	Service*				service_ref;
	NetAddress				net_address;
	atomic<bool>			is_connect;
	int						prev_remain;
	S_STATE					state;

private:
	OVER_EXP				recv_over;
	OVER_EXP				connect_over;
	OVER_EXP				disconnect_over;
};

#pragma pack(push, 1)
struct PacketHeader
{
public:
	PacketHeader(uint16 size, uint16 type) : packet_size(size), packet_type(type) {}

	uint16 GetSize() { return packet_size; }
	uint16 GetType() { return packet_type; }

	void SetPacketSize(uint16 size) { packet_size = size; }
	void SetPacketType(uint16 type) { packet_type = type; }

private:
	uint16 packet_size;
	uint16 packet_type;
};
#pragma pack(pop)
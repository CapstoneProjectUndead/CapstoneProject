#pragma once
#include "RecvBuffer.h"

class Listener;
class Session;

enum COMP_TYPE
{
	OP_CONNECT,
	OP_DISCONNECT,
	OP_ACCEPT,
	OP_RECV,
	OP_SEND,
};

class OVER_EXP
{
	enum
	{
		BUFFER_SIZE = 0x10000, // 64KB
		BUFFER_COUNT = 10
	};

public:
	WSAOVERLAPPED			wsa_over;
	WSABUF					wsabuf;
	vector<char>			recv_buf;
	COMP_TYPE				comp_type;

	Listener*				listener_ref;
	shared_ptr<Session>		session_ref;

	int32					recv_buffer_capacity;

	// ¼Û½Å¿ë
	vector<SendBufferRef> send_buffers;

	OVER_EXP();
	OVER_EXP(queue<SendBufferRef>& q, vector<WSABUF>& wsaBuffers);

	void	Init() { ZeroMemory(&wsa_over, sizeof(wsa_over)); }
	int32	BufferSize() { return recv_buffer_capacity; }
};
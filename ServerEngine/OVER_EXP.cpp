#include "pch.h"
#include "OVER_EXP.h"
#include "Session.h"
#include "SendBuffer.h"

OVER_EXP::OVER_EXP()
	: listener_ref(nullptr)
	, session_ref(nullptr)
	, recv_buffer_capacity(0)
{
	Init();
	comp_type = OP_RECV;
	recv_buf.resize(BUFFER_SIZE * BUFFER_COUNT);
	recv_buffer_capacity = BUFFER_SIZE * BUFFER_COUNT;
}

OVER_EXP::OVER_EXP(queue<SendBufferRef>& q, vector<WSABUF>& wsaBuffers)
	: listener_ref(nullptr)
	, session_ref(nullptr)
	, recv_buffer_capacity(0)
{
	Init();
	comp_type = OP_SEND;

	while (q.empty() == false)
	{
		SendBufferRef sendbuffer = q.front();
		send_buffers.push_back(sendbuffer);

		wsabuf.buf = reinterpret_cast<char*>(sendbuffer->Buffer());
		wsabuf.len = static_cast<LONG>(sendbuffer->WriteSize());
		wsaBuffers.push_back(wsabuf);

		q.pop();
	} 
}
#include "pch.h"
#include "SendBuffer.h"

SendBuffer::SendBuffer(int32 bufferSize)
	: write_size(0)
{
	buffer.resize(bufferSize);
}

SendBuffer::~SendBuffer()
{
}

void SendBuffer::CopyData(void* data, int32 len)
{
	assert(Capacity() >= len);
	::memcpy(buffer.data(), data, len);
	write_size = len;
}

void SendBuffer::Close(uint32 writeSize)
{
	write_size = writeSize;
}
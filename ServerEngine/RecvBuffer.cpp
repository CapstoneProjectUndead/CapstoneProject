#include "pch.h"
#include "RecvBuffer.h"

RecvBuffer::RecvBuffer(int32 bufferSize)
	: read_pos(0)
	, write_pos(0)
	, buffer_size(bufferSize)
{
	capacity = bufferSize * BUFFER_COUNT;
	buffer.resize(capacity);
}

RecvBuffer::~RecvBuffer()
{
}

void RecvBuffer::Clean()
{
	int32 dataSize = DataSize();
	if (dataSize == 0)
	{
		// 딱 마침 읽기+쓰기 커서가 동일한 위치라면, 둘 다 리셋.
		read_pos = write_pos = 0;
	}
	else
	{
		// 여유 공간이 버퍼 1개 크기 미만이면, 데이터를 앞으로 땅긴다.
		if (FreeSize() < buffer_size)
		{
			::memcpy(&buffer[0], &buffer[read_pos], dataSize);
			read_pos = 0;
			write_pos = dataSize;
		}
	}
}

bool RecvBuffer::Write(int32 numOfByte)
{
	if (numOfByte > FreeSize())
		return false;

	write_pos += numOfByte;
	return true;
}

bool RecvBuffer::Read(int32 numOfByte)
{
	if (numOfByte > DataSize())
		return false;

	read_pos += numOfByte;
	return true;
}

#include "pch.h"
#include "BufferReader.h"

BufferReader::BufferReader()
	: buffer(nullptr)
	, size(0)
	, pos(0)
{
}

BufferReader::BufferReader(BYTE* buffer, uint32 size, uint32 pos)
	: buffer(buffer)
	, size(size)
	, pos(pos)
{
}

BufferReader::~BufferReader()
{
}

bool BufferReader::Peek(void* dest, uint32 len)
{
	if (FreeSize() < len)
		return false;

	::memcpy(dest, &buffer[pos], len);
	return true;
}

bool BufferReader::Read(void* dest, uint32 len)
{
	if (Peek(dest, len) == false)
		return false;

	pos += len;
	return true;
}
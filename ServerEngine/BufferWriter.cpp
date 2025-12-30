#include "pch.h"
#include "BufferWriter.h"

BufferWriter::BufferWriter()
	: buffer(nullptr)
	, size(0)
	, pos(0)
{
}

BufferWriter::BufferWriter(BYTE* buffer, uint32 size, uint32 pos)
	: buffer(buffer)
	, size(size)
	, pos(pos)
{

}

BufferWriter::~BufferWriter()
{
}

bool BufferWriter::Write(void* src, uint32 len)
{
	if (FreeSize() < len)
		return false;

	::memcpy(&buffer[pos], src, len);
	pos += len;
	return true;
}
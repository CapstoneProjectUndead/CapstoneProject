#pragma once

class BufferReader
{
public:
	BufferReader();
	BufferReader(BYTE* buffer, uint32 size, uint32 pos = 0);
	~BufferReader();

	BYTE*					Buffer() { return buffer; }
	uint32					Size() { return size; }
	uint32					ReadSize() { return pos; }
	uint32					FreeSize() { return size - pos; }

	template<typename T>
	bool					Peek(T* dest) { return Peek(dest, sizeof(T)); }
	bool					Peek(void* dest, uint32 len);

	template<typename T>
	bool					Read(T* dest) { return Read(dest, sizeof(T)); }
	bool					Read(void* dest, uint32 len);

	template<typename T>
	BufferReader& operator>>(OUT T& dest);

private:
	BYTE*					buffer;
	uint32					size;
	uint32					pos;
};

template<typename T>
inline BufferReader& BufferReader::operator>>(OUT T& dest)
{
	dest = *reinterpret_cast<T*>(&buffer[pos]);
	pos += sizeof(T);
	return *this;
}


#pragma once

class BufferWriter
{
public:
	BufferWriter();
	BufferWriter(BYTE* buffer, uint32 size, uint32 pos = 0);
	~BufferWriter();

	BYTE*					Buffer() { return buffer; }
	uint32					Size() { return size; }
	uint32					WriteSize() { return pos; }
	uint32					FreeSize() { return size - pos; }

	template<typename T>
	bool					Write(T* src) { return Write(src, sizeof(T)); }
	bool					Write(void* src, uint32 len);

	template<typename T>
	T* Reserve(uint16 count);

	template<typename T>
	BufferWriter& operator<<(T&& src);

	BufferWriter& operator<<(char* src)
	{
		COPY_STRING(buffer, src);
		pos += ::strlen(src);
		return *this;
	}

private:
	BYTE*					buffer;
	uint32					size;
	uint32					pos;
};

template<typename T>
T* BufferWriter::Reserve(uint16 count)
{
	if (FreeSize() < sizeof(T) * count)
		return nullptr;

	T* ret = reinterpret_cast<T*>(&buffer[pos]);
	pos += sizeof(T) * count;
	return ret;
}

// const T&
template<typename T>
BufferWriter& BufferWriter::operator<<(T&& src)
{
	using DataType = std::remove_reference_t<T>;
	*reinterpret_cast<DataType*>(&buffer[pos]) = std::forward<DataType>(src);
	pos += sizeof(T);
	return *this;
}

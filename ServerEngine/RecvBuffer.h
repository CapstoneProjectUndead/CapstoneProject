#pragma once

class RecvBuffer
{
	enum { BUFFER_COUNT = 10 };

public:
	RecvBuffer(int32 bufferSize);
	~RecvBuffer();

	BYTE* ReadPos() { return &buffer[read_pos]; }
	BYTE* WritePos() { return &buffer[write_pos]; }
	
	int32 DataSize() { return write_pos - read_pos; }
	int32 FreeSize() { return capacity - write_pos; }

	bool Read(int32 numOfByte);
	bool Write(int32 numOfByte);

	void Clean();

private:
	std::vector<BYTE>	buffer;
	int32				capacity;
	int32				buffer_size;
	int32				read_pos;
	int32				write_pos;
};


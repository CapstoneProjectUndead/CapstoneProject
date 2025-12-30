#pragma once

class SendBuffer
{
public:
	SendBuffer(int32 bufferSize);
	~SendBuffer();

	BYTE*				Buffer() { return buffer.data(); }
	int32				WriteSize() { return write_size; }
	int32				Capacity() { return static_cast<int32>(buffer.size()); }

	void				CopyData(void* data, int32 len);
	void				Close(uint32 writeSize);

private:
	vector<BYTE>		buffer;
	uint32				write_size;
};


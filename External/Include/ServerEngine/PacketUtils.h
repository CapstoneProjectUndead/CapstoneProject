#pragma once


template<typename T, typename C>
class PacketIterator
{
public:
	PacketIterator(C& container, uint16 index) : container(container), index(index) {}

	bool				operator!=(const PacketIterator& other) const { return index != other.index; }
	const T&			operator*() const { return container[index]; }
	T&					operator*() { return container[index]; }
	T*					operator->() { return &container[index]; }
	PacketIterator&		operator++() { index++; return *this; }
	PacketIterator		operator++(int32) { PacketIterator ret = *this; ++index; return ret; }

private:
	C&				container;
	uint16			index;
};

template<typename T>
class PacketList
{
public:
	PacketList() : data(nullptr), count(0) {}
	PacketList(T* data, uint16 count) : data(data), count(count) {}

	T& operator[](uint16 index)
	{
		ASSERT_CRASH(index < count);
		return data[index];
	}

	uint16 Count() { return count; }

	// ranged-base for Áö¿ø
	PacketIterator<T, PacketList<T>> begin() { return PacketIterator<T, PacketList<T>>(*this, 0); }
	PacketIterator<T, PacketList<T>> end() { return PacketIterator<T, PacketList<T>>(*this, count); }

private:
	T*			data;
	uint16		count;
};
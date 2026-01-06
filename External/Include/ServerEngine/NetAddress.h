#pragma once

class NetAddress
{
public:
	NetAddress() = default;
	NetAddress(SOCKADDR_IN sockAddr);
	NetAddress(std::wstring ip, uint16 port);

	SOCKADDR_IN&	GetSockAddress() { return sock_address; }
	std::wstring	GetIpAddress();
	uint16			GetPort() { return ::ntohs(sock_address.sin_port); }

public:
	static IN_ADDR	Ip2Address(const WCHAR* ip);

private:
	SOCKADDR_IN		sock_address = {};
};


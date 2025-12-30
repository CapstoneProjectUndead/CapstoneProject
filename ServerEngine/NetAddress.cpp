#include "pch.h"
#include "NetAddress.h"

NetAddress::NetAddress(SOCKADDR_IN sockAddr)
{
}

NetAddress::NetAddress(wstring ip, uint16 port)
{
	// 1. 구조체의 모든 바이트를 0으로 초기화 (예상치 못한 쓰레기값 방지)
	::memset(&sock_address, 0, sizeof(sock_address));

	// 2. 주소 체계(AF_INET = IPv4)를 설정
	sock_address.sin_family = AF_INET;

	// 3. IP 주소 설정 (INADDR_ANY는 모든 NIC(네트워크 인터페이스)에서 수신 허용)
	sock_address.sin_addr = Ip2Address(ip.c_str());

	// 4. 포트 번호 설정 (네트워크 바이트 순서로 변환)
	sock_address.sin_port = ::htons(port);
}

wstring NetAddress::GetIpAddress()
{
	WCHAR buffer[100];
	::InetNtopW(AF_INET, &sock_address.sin_addr, buffer, len32(buffer));
	return wstring(buffer);
}

IN_ADDR NetAddress::Ip2Address(const WCHAR* ip)
{
	IN_ADDR address;
	::InetPtonW(AF_INET, ip, &address);
	return address;
}
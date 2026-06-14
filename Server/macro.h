#pragma once
// Server쪽 macro

#define CON CDBManager::GetInstance().GetCon()
#define MAKE_SEND_BUFFER(pkt) CClientPacketHandler::MakeSendBuffer(pkt);

// protocol.h(공용)에도 동일 정의가 있어 중복정의 방지
#ifndef COPY_STRING
#define COPY_STRING(dest, src)  memset(dest, 0, sizeof(dest)); memcpy(dest, src, strlen(src));
#endif

#define CAST_CS(session) static_pointer_cast<CClientSession>(session)
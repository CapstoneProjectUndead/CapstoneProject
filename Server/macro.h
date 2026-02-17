#pragma once
// ServerÂÊ macro

#define CON CDBManager::GetInstance().GetCon()
#define MAKE_SEND_BUFFER(pkt) CClientPacketHandler::MakeSendBuffer(pkt);

#define CAST_CS(session) static_pointer_cast<CClientSession>(session)
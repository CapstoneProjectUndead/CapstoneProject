#include "pch.h"
#include "IOCP.h"
#include "SocketHelper.h"
#include "Listener.h"
#include "Session.h"
#include "Service.h"

IOCP::IOCP()
	: iocp_handle{}
{
	Initialize();
}

IOCP::~IOCP()
{
	::CloseHandle(iocp_handle);
}

bool IOCP::Initialize()
{
	iocp_handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	if (iocp_handle != INVALID_HANDLE_VALUE)
		return false;

	return true;
}

bool IOCP::Register(HANDLE socket, ULONG_PTR key)
{
	return ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), iocp_handle, key, 0);
}

void IOCP::WorkerThreadLoop(uint32 timeoutMs)
{
	while (true)
	{
		DWORD numOfBytes = 0;
		ULONG_PTR key = 0;
		WSAOVERLAPPED* p_over;

		BOOL result = ::GetQueuedCompletionStatus(iocp_handle, &numOfBytes, (PULONG_PTR)&key, &p_over, timeoutMs);

		int64 client_id = static_cast<int>(key);
		OVER_EXP* exp_over = reinterpret_cast<OVER_EXP*>(p_over);

		if (nullptr == exp_over)
			return;

		auto compType = exp_over->comp_type;
		auto session = exp_over->session_ref;
		auto listener = exp_over->listener_ref;
		auto service = (session ? session->GetService() : nullptr);

		if (result)
		{
			switch (compType)
			{
			case COMP_TYPE::OP_ACCEPT:
			{
				if (nullptr == listener)
					assert(nullptr);

				listener->HandleAccept(exp_over);
			}
			break;
			case COMP_TYPE::OP_CONNECT:
			{
				if (nullptr == session)
					assert(nullptr);

				session->HandleConnect();
			}
				break;
			case COMP_TYPE::OP_DISCONNECT:
			{
				if (nullptr == session)
					assert(nullptr);

				session->HandleDisConnect();
			}
			break;
			case COMP_TYPE::OP_RECV:
			{
				if (nullptr == session)
					assert(nullptr);

				session->HandleRecv(numOfBytes);
			}
			break;
			case COMP_TYPE::OP_SEND:
			{
				if (nullptr == session)
					assert(nullptr);

				session->HandleSend(numOfBytes);

				// Send 할 때 등록한 Overlapped는 동적할당 했기 때문에
				// 여기서 해제해야 한다.
				delete exp_over;
			}
			break;
			default:
				break;
			}
		}
		else
		{
			int err_no = ::WSAGetLastError();

			if (WAIT_TIMEOUT == err_no)
				continue;

			cout << "[" << client_id << "] GQCS Error : ";

			// 서버와 연결의 실패 알림도 iocp가 스레드를 깨워서
			// 알려주기 때문에 아래 에러 코드와 비교해 본다.
			// 해당 에러코드는 서버가 켜져있지 않았을 때 반환되었다.
			if (1225 == err_no)
			{
				cout << "err_num 1225" << endl;
				LogUtil::error_display(err_no);
				return;
			}

			switch (exp_over->comp_type)
			{
			case COMP_TYPE::OP_RECV:
			{
				Service* service = exp_over->session_ref->GetService();
				exp_over->session_ref->HandleRecv(numOfBytes);

				if (SERVICE_TYPE::Client == service->GetServiceType())
					cout << "서버와 접속이 끊겼습니다." << endl;
			}
			break;
			case COMP_TYPE::OP_SEND: 
			{
				Service* service = exp_over->session_ref->GetService();
				exp_over->session_ref->HandleSend(numOfBytes);
				delete exp_over;
			}
			break;
			default:
				LogUtil::error_display(err_no);
				break;
			}

			continue;
		}
	}
}
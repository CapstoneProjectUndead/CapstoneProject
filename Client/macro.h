
#define KEY_CHECK(Key, State) CKeyManager::GetInstance().GetKeyState(Key) == State
#define KEY_TAP(Key) KEY_CHECK(Key, KEY_STATE::TAP)
#define KEY_PRESSED(Key) KEY_CHECK(Key, KEY_STATE::PRESSED)
#define KEY_RELEASED(Key) KEY_CHECK(Key, KEY_STATE::RELEASED)
#define KEY_NONE(Key) KEY_CHECK(Key, KEY_STATE::NONE)

#define GET_DEVICE   gGameFramework.GetDevice().Get()
#define GET_CMD_LIST gGameFramework.GetCommandList().Get()
#define GET_CLIENT_WIDTH gGameFramework.GetClientWidth()
#define GET_CLIENT_HEIGHT gGameFramework.GetClientHeight()
#define GET_CMD_QUEUE gGameFramework.GetCommandQueue()

#define IS_CONNECT true == CNetworkManager::GetInstance().GetClientService()->GetConnection()

#define COPY_STRING(dest, src)  memset(dest, 0, sizeof(dest)); memcpy(dest, src, strlen(src));

#define BASE_UI_WIDTH  800.f
#define BASE_UI_HEIGHT 600.f

#define G_RATIO_X (ImGui::GetIO().DisplaySize.x / BASE_UI_WIDTH)
#define G_RATIO_Y (ImGui::GetIO().DisplaySize.y / BASE_UI_HEIGHT)

#define GET_SERVER_SESSION CServerSessionManager::GetInstance().GetServerSession();
#define SERVER_SESSION CServerSessionManager::GetInstance().GetServerSession()

#define MAKE_SEND_BUFFER(pkt) CServerPacketHandler::MakeSendBuffer(pkt);
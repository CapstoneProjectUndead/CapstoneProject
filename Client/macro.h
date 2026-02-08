
#define KEY_CHECK(Key, State) CKeyManager::GetInstance().GetKeyState(Key) == State
#define KEY_TAP(Key) KEY_CHECK(Key, KEY_STATE::TAP)
#define KEY_PRESSED(Key) KEY_CHECK(Key, KEY_STATE::PRESSED)
#define KEY_RELEASED(Key) KEY_CHECK(Key, KEY_STATE::RELEASED)
#define KEY_NONE(Key) KEY_CHECK(Key, KEY_STATE::NONE)

#define GET_DEVICE   gGameFramework.GetDevice().Get()
#define GET_CMD_LIST gGameFramework.GetCommandList().Get()

#define IS_CONNECT true == CNetworkManager::GetInstance().GetClientService()->GetConnection()

#define COPY_STRING(dest, src)  memset(dest, 0, sizeof(dest)); memcpy(dest, src, strlen(src));

#define G_RATIO_X (ImGui::GetIO().DisplaySize.x / FRAME_BUFFER_WIDTH)
#define G_RATIO_Y (ImGui::GetIO().DisplaySize.y / FRAME_BUFFER_HEIGHT)
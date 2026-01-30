
#define KEY_CHECK(Key, State) CKeyManager::GetInstance().GetKeyState(Key) == State
#define KEY_TAP(Key) KEY_CHECK(Key, KEY_STATE::TAP)
#define KEY_PRESSED(Key) KEY_CHECK(Key, KEY_STATE::PRESSED)
#define KEY_RELEASED(Key) KEY_CHECK(Key, KEY_STATE::RELEASED)
#define KEY_NONE(Key) KEY_CHECK(Key, KEY_STATE::NONE)

#define GET_DEVICE   gGameFramework.GetDevice().Get()
#define GET_CMD_LIST gGameFramework.GetCommandList().Get()

#define IS_CONNECT true == CNetworkManager::GetInstance().GetClientService()->GetConnection()
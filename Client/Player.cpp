#include "stdafx.h"
#include "Player.h"
#include "Camera.h"
#include "Timer.h"
#include "KeyManager.h"

#undef min
#undef max

extern HWND ghWnd;

// Player
CPlayer::CPlayer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	is_visible = true;
	SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));

	// 메쉬 설정
	std::shared_ptr<CMesh> mesh = std::make_shared<CCubeMesh>(device, commandList);
	SetMesh(mesh);
}

void CPlayer::Update(float elapsedTime)
{
     if (is_my_player)
        return;

    /*==========================
        [1] 위치 동기화 (월드 보간)
    ==========================*/

     XMFLOAT3 serverPos{ dest_info.x, dest_info.y, dest_info.z };
     XMFLOAT3 clientPos = position;

     XMFLOAT3 toTarget = Vector3::Subtract(serverPos, clientPos);
     float dist = Vector3::Length(toTarget);

     const float SNAP_DIST = 3.0f;      // 너무 멀면 순간이동
     const float STOP_DIST = 0.05f;     // 이 안이면 정지
     const float FOLLOW_SPEED = 1.0f; // 네 캐릭터 이동속도
     const float DAMPING = 10.0f;       // 클수록 더 빨리 멈춤

     // 1. 너무 멀면 스냅
     if (dist > SNAP_DIST)
     {
         SetPosition(serverPos);
         velocity = XMFLOAT3(0, 0, 0);
         return;
     }

     // 2. 서버가 RUN 상태일 때만 따라간다
     if (dist > STOP_DIST)
     {
         // 방향
         XMFLOAT3 dir = Vector3::Normalize(toTarget);

         // 목표 속도
         XMFLOAT3 desiredVel = Vector3::ScalarProduct(dir, FOLLOW_SPEED);

         // velocity 보간 
         velocity = Vector3::Add(
             velocity,
             Vector3::ScalarProduct(
                 Vector3::Subtract(desiredVel, velocity),
                 std::min(1.0f, DAMPING * elapsedTime),
                 false
             )
         );

         // 위치 이동
         XMFLOAT3 frameMove = Vector3::ScalarProduct(velocity, elapsedTime, false);
         position = Vector3::Add(position, frameMove);

         // 방향 동기화
         SetYaw(dest_info.yaw);
     }
     else
     {
         // 서버가 IDLE 이거나 거의 도착 → 부드럽게 정지
         velocity = Vector3::ScalarProduct(
             velocity,
             std::max(0.0f, 1.0f - DAMPING * elapsedTime),
             false
         );

         XMFLOAT3 frameMove = Vector3::ScalarProduct(velocity, elapsedTime, false);
         position = Vector3::Add(position, frameMove);
     }

    /*==========================
        [2] 회전 동기화 (Yaw / Pitch)
    ==========================*/

    float targetYaw   = dest_info.yaw;
    float targetPitch = dest_info.pitch;

    // 현재 각도 (멤버 변수)
    float curYaw   = yaw;
    float curPitch = pitch;

    // Yaw 360도 경계 보정
    float deltaYaw = targetYaw - curYaw;
    if (deltaYaw > 180.f)  deltaYaw -= 360.f;
    if (deltaYaw < -180.f) deltaYaw += 360.f;

    float deltaPitch = targetPitch - curPitch;

    // 회전 보간 속도
    const float rotSpeed = 8.0f;

    // 프레임 보간
    yaw   += deltaYaw   * rotSpeed * elapsedTime;
    pitch += deltaPitch * rotSpeed * elapsedTime;

    // Pitch 제한 (필수)
    pitch = std::clamp(pitch, -89.9f, 89.9f);

    // 회전 적용
    SetYawPitch(yaw, pitch);
    UpdateWorldMatrix();
}

void CPlayer::Move(const XMFLOAT3 shift)
{
	position = Vector3::Add(position, shift);
	//if (camera) camera->Move(shift);
}
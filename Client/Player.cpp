#include "stdafx.h"
#include "Player.h"
#include "Camera.h"
#include "Timer.h"
#include "KeyManager.h"

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
    if (false == is_my_player)
    {
        // --- [1. 이동 동기화] ---
        XMFLOAT3 serverPos{ dest_info.x, dest_info.y, dest_info.z };
        XMFLOAT3 clientPos = position;

        float dist = Vector3::Distance(serverPos, clientPos);

        // 2 & 3. 거리 차이에 따른 보정 (순간이동 혹은 보간)
        if (dist > 3.f)
        {
            SetPosition(serverPos);
        }
        else if (dist > 1.f)
        {
            XMFLOAT3 newPos = Vector3::VInterpTo(position, serverPos, elapsedTime, 8.f);
            SetPosition(newPos);
        }

        // [이동 로직 보완] dist가 작을 때(지연 없을 때) 자연스럽게 이동
        if (dist > 0.01f && dist <= 1.f)
        {
            XMFLOAT3 diff = Vector3::Subtract(serverPos, clientPos);
            XMFLOAT3 moveDir = Vector3::Normalize(diff);

            // 단순히 elapsedTime만큼 가는게 아니라, 남은 거리에 비례해서 이동해야 부드럽다.
            float moveDist = dist * elapsedTime;
            if (moveDist > dist) 
                moveDist = dist;

            Move(moveDir, moveDist); // Move(방향, 거리)
        }

        // --- [2. 회전 동기화] ---
        // 현재 내(클라의 상대캐릭터) 행렬에서 현재 각도 추출
        float curPitch = XMConvertToDegrees(asinf(-look.y));
        float curYaw = XMConvertToDegrees(atan2f(look.x, look.z));
        float curRoll = XMConvertToDegrees(atan2f(right.y, up.y));

        // 목표 각도(서버가 준 값)와의 차이 계산
        float deltaPitch = dest_info.pitch - curPitch;
        float deltaYaw = dest_info.yaw - curYaw;
        float deltaRoll = dest_info.roll - curRoll;

        // Yaw 회전 보정 (360도 지점 튀는 현상 방지)
        if (deltaYaw > 180.f) deltaYaw -= 360.f;
        if (deltaYaw < -180.f) deltaYaw += 360.f;

        // 회전 속도 설정 및 적용
        float rotSpeed = 1.0f;
        if (fabs(deltaPitch) > 0.1f || fabs(deltaYaw) > 0.1f || fabs(deltaRoll) > 0.1f)
        {
            // 목표 각도를 향해 매 프레임 조금씩 Rotate
            Rotate(deltaPitch * elapsedTime,
                deltaYaw * rotSpeed * elapsedTime,
                deltaRoll * rotSpeed * elapsedTime);
        }
    }
}

void CPlayer::Move(const XMFLOAT3 shift)
{
	position = Vector3::Add(position, shift);
	//if (camera) camera->Move(shift);
}
#include "stdafx.h"
#include "MeshComponent.inl"
#include "Character.h"
#include "Movement.h"
#include "Animator.h"
#include "Mesh.h"
#include "Collider.h"
#include "PhysicsManager.h"

CCharacter::CCharacter()
	: CObject()
{
}

void CCharacter::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    std::string fileName{ "../Modeling/undead_char.bin" };
    auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

    // 1) Mesh 로드 + totalBounds 계산
    BoundingBox totalBounds;
    bool firstBounds = true;

    for (const auto& child : frameRoot->childrens) {
        if (child->mesh.positions.empty())
            continue;

        // mesh component
        auto meshComp = std::make_shared<CMeshComponent>();
        SetComponent(meshComp);
        meshComp->SetMeshFromFile<CSkinnedVertex>(device, commandList, child);

        // bounds merge
        if (firstBounds) {
            totalBounds = child->mesh.bounds;
            firstBounds = false;
        }
        else {
            BoundingBox::CreateMerged(totalBounds, totalBounds, child->mesh.bounds);
        }
    }

    // 4) ColliderComponent 생성
    XMFLOAT3 pivot{ 0.0f, totalBounds.Extents.y, 0.0f };
    std::unique_ptr< CColliderShape> shape = std::make_unique<CSphereShape>(totalBounds.Extents.x, pivot);
    auto boxCollider = std::make_shared<CColliderComponent>(shape);
    SetComponent(boxCollider);
    CPhysicsManager::GetInstance().SetCollider(boxCollider);

    auto debugMesh = std::make_shared<CMeshComponent>();
    SetComponent(debugMesh);
    std::shared_ptr<CMesh> meshss = std::make_shared<CSphereMesh>(device, commandList, totalBounds.Extents.x, pivot);
    debugMesh->SetMesh(meshss);

    // 4) Animator
    auto animator = std::make_shared<CAnimatorComponent>();
    animator->Initialize(fileName, "../Modeling/undead_ani.bin");
    animator->Play("Ganga_walk");
    SetComponent(animator);
    SetShdaer("skinning");

    // 5) Movement
    SetComponent(std::make_shared<CMovementComponent>());

    CObject::Initialize(device, commandList);
}
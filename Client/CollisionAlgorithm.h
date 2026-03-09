#pragma once

class CColliderShape;
class CObject;

struct CollisionInfo {
    XMVECTOR normal;
    float depth;
    bool collided = false;
    CObject* other_object{};    // 충돌한 상대방
};

// 충돌 layer
enum EColLayer : uint32_t {
    NONE = 0,
    GROUND = 1 << 0,    // 바닥, 지형
    WALL = 1 << 1,      // 고정된 벽
    OBJECT = 1 << 2,    // 상자 등 동적 오브젝트
    PLAYER = 1 << 3,    // 플레이어
    TRIGGER = 1 << 4,   // 이벤트 구역 (물리 충돌 X, 겹침만 O)

    // 미리 정의된 조합 (편의용)
    ENVIRONMENT = GROUND | WALL,
    ALL_SOLID = GROUND | WALL | OBJECT
};

// 충돌 체크 알고리즘
namespace GJKAlgorithm {
    struct Simplex {
        XMVECTOR points[4];
        int size = 0;

        void push_front(XMVECTOR p) {
            for (int i = size; i > 0; i--) points[i] = points[i - 1];
            points[0] = p;
            size = min(size + 1, 4);
        }
    };

    // 단일 물체의 Support Point 찾기
    XMVECTOR GetSupport(const std::vector<XMFLOAT3>& vertices, XMVECTOR direction);
    // directX collider 호환용
    XMVECTOR GetSupportSphere(const BoundingSphere& sphere, XMVECTOR d);
    XMVECTOR GetSupportAABB(const BoundingBox& box, XMVECTOR d);
    XMVECTOR GetSupportOBB(const BoundingOrientedBox& obb, XMVECTOR d);

    // 두 물체 사이의 Minkowski Difference Support Point
    XMVECTOR GetMinkowskiSupport(const std::vector<XMFLOAT3>& vertsA, const std::vector<XMFLOAT3>& vertsB, XMVECTOR direction);

    bool Intersects(const std::vector<XMFLOAT3>& vertsA, const std::vector<XMFLOAT3>& vertsB, Simplex& outSimplex);
    bool GenericIntersects(std::function<XMVECTOR(XMVECTOR)> supportA, std::function<XMVECTOR(XMVECTOR)> supportB, Simplex& outSimplex);

    bool NextSimplex(Simplex& s, XMVECTOR& d);
    // 케이스
    bool LineCase(Simplex& s, XMVECTOR& d);
    bool TriangleCase(Simplex& s, XMVECTOR& d);
    bool TetrahedronCase(Simplex& s, XMVECTOR& d);

    // EPA: 충돌 깊이값/노말 계산 알고리즘
    struct EPAFace {
        XMVECTOR normal;
        float distance;
        uint32_t v[3]; // 정점 인덱스 (polytope 배열 내 인덱스)
    };

    // 지평선(Horizon) 모서리를 관리하기 위한 구조체
    struct EPAEdge {
        uint32_t a, b;
        bool operator==(const EPAEdge& other) const {
            return (a == other.a && b == other.b) || (a == other.b && b == other.a);
        }
    };

    // 면의 법선과 원점으로부터의 거리를 계산하는 헬퍼 함수
    EPAFace CreateFace(const std::vector<XMVECTOR>& polytope, uint32_t i, uint32_t j, uint32_t k, XMVECTOR center);

    CollisionInfo SolveEPA(const GJKAlgorithm::Simplex& simplex, const CColliderShape* shapeA, const CColliderShape* shapeB);
}
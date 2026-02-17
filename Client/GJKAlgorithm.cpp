#include "stdafx.h"
#include "GJKAlgorithm.h"
#include "Collider.h"

XMVECTOR GJKAlgorithm::GetSupport(const std::vector<XMFLOAT3>& vertices, XMVECTOR direction)
{
    float maxDot = -FLT_MAX;
    XMVECTOR bestVertex = XMVectorZero();

    for (const auto& v : vertices) {
        XMVECTOR vVec = XMLoadFloat3(&v);
        float dot = XMVectorGetX(XMVector3Dot(vVec, direction));
        if (dot > maxDot) {
            maxDot = dot;
            bestVertex = vVec;
        }
    }
    return bestVertex;
}

XMVECTOR GJKAlgorithm::GetSupportSphere(const BoundingSphere& sphere, XMVECTOR d)
{
    XMVECTOR dir = XMVector3Normalize(d);
    XMVECTOR center = XMLoadFloat3(&sphere.Center);
    return center + dir * sphere.Radius;
}

XMVECTOR GJKAlgorithm::GetSupportAABB(const BoundingBox& box, XMVECTOR d)
{
    XMVECTOR center = XMLoadFloat3(&box.Center);
    XMVECTOR extents = XMLoadFloat3(&box.Extents);

    // 방향 d의 각 축 부호에 따라 Extent를 더하거나 뺌
    XMFLOAT3 dir; XMStoreFloat3(&dir, d);
    XMVECTOR offset = XMVectorSet(
        dir.x > 0 ? box.Extents.x : -box.Extents.x,
        dir.y > 0 ? box.Extents.y : -box.Extents.y,
        dir.z > 0 ? box.Extents.z : -box.Extents.z,
        0.0f
    );
    return center + offset;
}

XMVECTOR GJKAlgorithm::GetSupportOBB(const BoundingOrientedBox& obb, XMVECTOR d)
{
    XMVECTOR center = XMLoadFloat3(&obb.Center);
    XMVECTOR extents = XMLoadFloat3(&obb.Extents);
    XMVECTOR orientation = XMLoadFloat4(&obb.Orientation);

    // 방향 d를 OBB의 로컬 공간으로 변환
    XMVECTOR invOrientation = XMQuaternionInverse(orientation);
    XMVECTOR localD = XMVector3Rotate(d, invOrientation);

    XMFLOAT3 lD; XMStoreFloat3(&lD, localD);
    XMVECTOR localOffset = XMVectorSet(
        lD.x > 0 ? obb.Extents.x : -obb.Extents.x,
        lD.y > 0 ? obb.Extents.y : -obb.Extents.y,
        lD.z > 0 ? obb.Extents.z : -obb.Extents.z,
        0.0f
    );

    // 다시 월드 공간으로 변환
    return center + XMVector3Rotate(localOffset, orientation);
}

XMVECTOR GJKAlgorithm::GetMinkowskiSupport(const std::vector<XMFLOAT3>& vertsA, const std::vector<XMFLOAT3>& vertsB, XMVECTOR direction)
{
    return GetSupport(vertsA, direction) - GetSupport(vertsB, -direction);
}

bool GJKAlgorithm::Intersects(const std::vector<XMFLOAT3>& vertsA, const std::vector<XMFLOAT3>& vertsB, Simplex& outSimplex)
{
    outSimplex.size = 0;
    XMVECTOR direction = XMVectorSet(1, 0, 0, 0); // 초기 방향

    // 첫 번째 점
    XMVECTOR support = GetMinkowskiSupport(vertsA, vertsB, direction);
    outSimplex.push_front(support);

    // 원점을 향해 방향 설정
    direction = -support;

    while (true) {
        support = GetMinkowskiSupport(vertsA, vertsB, direction);

        // 새로 찾은 점이 원점 방향으로 전진하지 못하면 충돌 안 함
        if (XMVectorGetX(XMVector3Dot(support, direction)) <= 0) return false;

        outSimplex.push_front(support);

        // 심플렉스 내부 검사 및 다음 방향 설정
        if (NextSimplex(outSimplex, direction)) return true;
    }
}

bool GJKAlgorithm::GenericIntersects(std::function<XMVECTOR(XMVECTOR)> supportA, std::function<XMVECTOR(XMVECTOR)> supportB, Simplex& outSimplex)
{
    outSimplex.size = 0;
    XMVECTOR direction = XMVectorSet(1, 0, 0, 0);

    auto getMinkowski = [&](XMVECTOR d) {
        // A의 가장 먼 점 - B의 반대방향 가장 먼 점
        return supportA(d) - supportB(-d);
        };

    // 첫 번째 점
    XMVECTOR support = getMinkowski(direction);
    outSimplex.push_front(support);

    // 원점을 향해 방향 설정
    direction = -support;

    while (true) {
        support = getMinkowski(direction);

        // 새로 찾은 점이 원점 방향으로 전진하지 못하면 충돌 안 함
        if (XMVectorGetX(XMVector3Dot(support, direction)) <= 0) return false;

        outSimplex.push_front(support);

        // 심플렉스 내부 검사 및 다음 방향 설정
        if (NextSimplex(outSimplex, direction)) return true;
    }
}

bool GJKAlgorithm::LineCase(Simplex& s, XMVECTOR& d)
{
    XMVECTOR a = s.points[0], b = s.points[1];
    XMVECTOR ab = b - a, ao = -a;

    if (XMVectorGetX(XMVector3Dot(ab, ao)) > 0) {
        d = XMVector3Cross(XMVector3Cross(ab, ao), ab); // ab에 수직이면서 원점 방향
    }
    else {
        s.size = 1; d = ao;
    }
    return false;
}

bool GJKAlgorithm::TriangleCase(Simplex& s, XMVECTOR& d)
{
    XMVECTOR a = s.points[0], b = s.points[1], c = s.points[2];
    XMVECTOR ab = b - a, ac = c - a, ao = -a;
    XMVECTOR abc = XMVector3Cross(ab, ac);

    if (XMVectorGetX(XMVector3Dot(XMVector3Cross(abc, ac), ao)) > 0) {
        if (XMVectorGetX(XMVector3Dot(ac, ao)) > 0) {
            s.points[1] = c; s.size = 2; d = XMVector3Cross(XMVector3Cross(ac, ao), ac);
        }
        else return LineCase(s, d);
    }
    else {
        if (XMVectorGetX(XMVector3Dot(XMVector3Cross(ab, abc), ao)) > 0) return LineCase(s, d);
        else {
            if (XMVectorGetX(XMVector3Dot(abc, ao)) > 0) { d = abc; }
            else { s.points[1] = c; s.points[2] = b; d = -abc; }
        }
    }
    return false;
}

bool GJKAlgorithm::TetrahedronCase(Simplex& s, XMVECTOR& d)
{
    XMVECTOR a = s.points[0], b = s.points[1], c = s.points[2], d_pt = s.points[3];
    XMVECTOR ab = b - a, ac = c - a, ad = d_pt - a, ao = -a;
    XMVECTOR abc = XMVector3Cross(ab, ac), acd = XMVector3Cross(ac, ad), adb = XMVector3Cross(ad, ab);

    if (XMVectorGetX(XMVector3Dot(abc, ao)) > 0) { s.size = 3; return TriangleCase(s, d); }
    if (XMVectorGetX(XMVector3Dot(acd, ao)) > 0) { s.points[1] = c; s.points[2] = d_pt; s.size = 3; return TriangleCase(s, d); }
    if (XMVectorGetX(XMVector3Dot(adb, ao)) > 0) { s.points[1] = d_pt; s.points[2] = b; s.size = 3; return TriangleCase(s, d); }

    return true; // 원점이 사면체 내부에 있음!
}

bool GJKAlgorithm::NextSimplex(Simplex& s, XMVECTOR& d)
{
    switch (s.size) {
    case 2: return LineCase(s, d);
    case 3: return TriangleCase(s, d);
    case 4: return TetrahedronCase(s, d);
    }
    return false;
}

GJKAlgorithm::EPAFace GJKAlgorithm::CreateFace(const std::vector<XMVECTOR>& polytope, uint32_t i, uint32_t j, uint32_t k)
{
    XMVECTOR a = polytope[i];
    XMVECTOR b = polytope[j];
    XMVECTOR c = polytope[k];

    XMVECTOR ab = b - a;
    XMVECTOR ac = c - a;
    // 외적을 통해 법선 계산 (CW/CCW 순서 주의)
    XMVECTOR normal = XMVector3Normalize(XMVector3Cross(ab, ac));

    // 법선이 원점 반대 방향을 향하도록 보정 (Convex 내부에서 밖을 향하게)
    float distance = XMVectorGetX(XMVector3Dot(normal, a));
    if (distance < 0) {
        normal = -normal;
        distance = -distance;
    }

    return { normal, distance, {i, j, k} };
}


GJKAlgorithm::CollisionInfo GJKAlgorithm::SolveEPA(const GJKAlgorithm::Simplex& simplex, const CColliderShape* shapeA, const CColliderShape* shapeB)
{
    // 1. 초기 폴리토프 설정 (GJK의 최종 4개 점)
    std::vector<XMVECTOR> polytope;
    for (int i = 0; i < simplex.size; ++i) polytope.push_back(simplex.points[i]);

    // 2. 초기 면(Face) 구성 (사면체이므로 4개의 삼각형)
    std::list<EPAFace> faces;
    faces.push_back(CreateFace(polytope, 0, 1, 2));
    faces.push_back(CreateFace(polytope, 0, 2, 3));
    faces.push_back(CreateFace(polytope, 0, 3, 1));
    faces.push_back(CreateFace(polytope, 1, 3, 2));

    const int MAX_ITERATIONS = 32; // 무한 루프 방지
    for (int iter = 0; iter < MAX_ITERATIONS; ++iter) {
        if (faces.empty()) break;

        // 3. 원점과 가장 가까운 면 찾기
        auto closestFaceIt = faces.begin();
        float minDistance = FLT_MAX;

        for (auto it = faces.begin(); it != faces.end(); ++it) {
            if (it->distance < minDistance) {
                minDistance = it->distance;
                closestFaceIt = it;
            }
        }
        // 만약 면이 하나도 남지 않는 예외 상황이 오면 즉시 중단

        // 4. 가장 가까운 면의 법선 방향으로 새로운 Support 포인트 탐색
        XMVECTOR searchDir = closestFaceIt->normal;
        XMVECTOR p = shapeA->GetSupport(searchDir) - shapeB->GetSupport(-searchDir);

        // 5. 종료 조건: 새로 찾은 점이 현재 면보다 더 바깥에 있지 않으면 종료
        float d = XMVectorGetX(XMVector3Dot(p, searchDir));
        if (d - minDistance < 0.0001f) {
            CollisionInfo info;
            info.collided = true;
            info.normal = searchDir;
            info.depth = d;
            return info;
        }

        // 6. 지평선(Horizon) 추출: 새 점 p를 바라보는 면들을 제거하고 경계 모서리를 찾음
        std::vector<EPAEdge> edges;
        for (auto it = faces.begin(); it != faces.end(); ) {
            // 점 p가 면의 바깥쪽에 있는지 확인 (dot > 0)
            if (XMVectorGetX(XMVector3Dot(it->normal, p - polytope[it->v[0]])) > 0.0001f) {
                EPAEdge faceEdges[3] = { {it->v[0], it->v[1]}, {it->v[1], it->v[2]}, {it->v[2], it->v[0]} };
                for (auto& e : faceEdges) {
                    auto found = std::find(edges.begin(), edges.end(), e);
                    if (found != edges.end()) edges.erase(found);
                    else edges.push_back(e);
                }
                it = faces.erase(it);
            }
            else ++it;
        }

        // 지평선(Edge)이 하나도 안 나오면 더 이상 확장 불가
        if (edges.empty()) break;

        // 7. 폴리토프 확장: 지평선의 각 모서리와 새 점 p를 연결하여 새 면 생성
        uint32_t newIdx = (uint32_t)polytope.size();
        polytope.push_back(p);
        bool addedNewFace = false;
        for (auto& edge : edges) {
            EPAFace newFace = CreateFace(polytope, edge.a, edge.b, newIdx);
            // [수정 3] 생성된 면의 거리가 유효한지 체크 (0이면 생성 실패)
            if (newFace.distance >= 0) {
                faces.push_back(newFace);
                addedNewFace = true;
            }
        }
        if (!addedNewFace) break; // 새 면이 안 만들어지면 루프 탈출
    }

    return {}; // 최대 반복 횟수 초과 시
}
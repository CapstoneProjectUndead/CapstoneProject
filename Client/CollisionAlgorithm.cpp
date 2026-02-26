#include "stdafx.h"
#include "CollisionAlgorithm.h"
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

    // GJK 무한 루프 방지를 위한 카운터
    int iterations = 0;
    const int MAX_GJK_ITERATIONS = 32;
    while (iterations < MAX_GJK_ITERATIONS) {
        iterations++;

        // 1. 방향 벡터가 너무 작으면(0이면) 더 이상 계산 불가
        if (XMVectorGetX(XMVector3LengthSq(direction)) < 1e-6f) break;

        support = getMinkowski(direction);

        // 2. 이미 심플렉스에 있는 점이 또 나오면 루프 종료 (발전이 없음)
        for (int i = 0; i < outSimplex.size; ++i) {
            if (XMVector4Equal(support, outSimplex.points[i])) return false;
        }

        if (XMVectorGetX(XMVector3Dot(support, direction)) <= 0) return false;

        outSimplex.push_front(support);

        if (NextSimplex(outSimplex, direction)) return true;
    }

    return false; // 32번 넘게 돌면 그냥 충돌 안 한 걸로 처리 (안전)
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

CollisionInfo GJKAlgorithm::SolveEPA(const GJKAlgorithm::Simplex& simplex, const CColliderShape* shapeA, const CColliderShape* shapeB)
{
    std::vector<XMVECTOR> polytope;
    for (int i = 0; i < simplex.size; ++i) polytope.push_back(simplex.points[i]);

    std::list<EPAFace> faces;
    faces.push_back(CreateFace(polytope, 0, 1, 2));
    faces.push_back(CreateFace(polytope, 0, 2, 3));
    faces.push_back(CreateFace(polytope, 0, 3, 1));
    faces.push_back(CreateFace(polytope, 1, 3, 2));

    const int MAX_ITERATIONS = 10;
    for (int iter = 0; iter < MAX_ITERATIONS; ++iter) {
        if (faces.empty()) break;

        auto closestFaceIt = faces.begin();
        float minDistance = FLT_MAX;

        for (auto it = faces.begin(); it != faces.end(); ++it) {
            if (it->distance < minDistance) {
                minDistance = it->distance;
                closestFaceIt = it;
            }
        }

        XMVECTOR searchDir = closestFaceIt->normal;
        XMVECTOR p = shapeA->GetSupport(searchDir) - shapeB->GetSupport(-searchDir);

        // 수렴 조건 강화
        float d = XMVectorGetX(XMVector3Dot(p, searchDir));
        if (d - minDistance < 0.001f) { // 이 차이가 너무 작으면 더 이상 확장 무의미
            return { searchDir, d, true };
        }

        // 중복 점 체크
        for (const auto& v : polytope) {
            if (XMVectorGetX(XMVector3LengthSq(p - v)) < 0.00001f) {
                return { searchDir, minDistance, true };
            }
        }

        std::vector<EPAEdge> edges;
        for (auto it = faces.begin(); it != faces.end(); ) {
            // p가 면 바깥에 있는 경우만 제거
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

        // 지평선이 없으면 중단
        if (edges.empty()) break;

        uint32_t newIdx = (uint32_t)polytope.size();
        polytope.push_back(p);
        for (auto& edge : edges) {
            faces.push_back(CreateFace(polytope, edge.a, edge.b, newIdx));
        }
    }

    // 루프가 끝까지 돌아도 결과를 못찾으면 그냥 가장 가까웠던 면 정보라도 반환
    return { XMVectorSet(0, 1, 0, 0), 0.0f, true };
}

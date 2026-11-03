#pragma once
// Shared mesh structures for the Line of Sight: Vietnam converter.
// Header-only; C++17.
#include <algorithm>
#include <array>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace losv {
struct Vec2 { float u = 0.0f, v = 0.0f; };
struct Vec3 { float x = 0.0f, y = 0.0f, z = 0.0f; };
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x*s, a.y*s, a.z*s}; }
inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
inline float length(Vec3 a) { return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z); }
inline Vec3 normalized(Vec3 a) {
    const float n = length(a);
    return n > 1.0e-20f ? a*(1.0f/n) : Vec3{0,1,0};
}
struct Corner { Vec3 position; Vec2 uv; Vec3 normal; bool hasNormal = false; };
struct Triangle { std::array<Corner,3> corners; std::uint32_t material = 0; };
struct Material { std::string name; std::string texture; };
struct MeshObject { std::string name; std::vector<Triangle> triangles; };
struct Model {
    std::vector<MeshObject> objects;
    std::vector<Material> materials;
    std::size_t sourceGeometryOffset = 0;
    std::size_t sourceObjectCount = 0;
    std::vector<std::string> warnings;
};

// Coordinate transforms are explicit to avoid silently changing game coordinates.
enum class AxisOrder { XYZ, XZY, YXZ, YZX, ZXY, ZYX };
struct CoordinateOptions {
    AxisOrder order = AxisOrder::XYZ;
    bool negateX = false, negateY = false, negateZ = false;
    bool flipV = false;
    float scale = 1.0f;
};
inline Vec3 transformPosition(Vec3 p, const CoordinateOptions& o) {
    Vec3 q{};
    switch (o.order) {
        case AxisOrder::XYZ: q = {p.x,p.y,p.z}; break;
        case AxisOrder::XZY: q = {p.x,p.z,p.y}; break;
        case AxisOrder::YXZ: q = {p.y,p.x,p.z}; break;
        case AxisOrder::YZX: q = {p.y,p.z,p.x}; break;
        case AxisOrder::ZXY: q = {p.z,p.x,p.y}; break;
        case AxisOrder::ZYX: q = {p.z,p.y,p.x}; break;
    }
    if (o.negateX) q.x = -q.x;
    if (o.negateY) q.y = -q.y;
    if (o.negateZ) q.z = -q.z;
    return q * o.scale;
}
inline bool reversesHandedness(const CoordinateOptions& o) {
    int inversions = 0;
    int a=0, b=1, c=2;
    switch(o.order) {
        case AxisOrder::XYZ: a=0; b=1; c=2; break;
        case AxisOrder::XZY: a=0; b=2; c=1; break;
        case AxisOrder::YXZ: a=1; b=0; c=2; break;
        case AxisOrder::YZX: a=1; b=2; c=0; break;
        case AxisOrder::ZXY: a=2; b=0; c=1; break;
        case AxisOrder::ZYX: a=2; b=1; c=0; break;
    }
    if (a>b) ++inversions;
    if (a>c) ++inversions;
    if (b>c) ++inversions;
    if (o.negateX) ++inversions;
    if (o.negateY) ++inversions;
    if (o.negateZ) ++inversions;
    return (inversions % 2) != 0;
}
inline void applyCoordinates(Model& model, const CoordinateOptions& options) {
    const bool flipWinding = reversesHandedness(options);
    for (auto& object : model.objects) for (auto& tri : object.triangles) {
        for (auto& corner : tri.corners) {
            corner.position = transformPosition(corner.position, options);
            if (options.flipV) corner.uv.v = 1.0f - corner.uv.v;
            if (corner.hasNormal) {
                corner.normal = normalized(transformPosition(corner.normal, CoordinateOptions{
                    options.order, options.negateX, options.negateY, options.negateZ, false, 1.0f}));
            }
        }
        if (flipWinding) std::swap(tri.corners[1], tri.corners[2]);
    }
}

// Recalculate normals from triangle geometry. Smooth mode averages adjacent
// face normals at exactly matching positions; flat mode assigns one normal per face.
inline void calculateNormals(Model& model, bool smooth = true) {
    using Key = std::array<std::uint32_t,3>;
    auto keyOf=[](Vec3 p) {
        Key k{};
        std::memcpy(&k[0], &p.x, 4); std::memcpy(&k[1], &p.y, 4); std::memcpy(&k[2], &p.z, 4);
        return k;
    };
    std::map<Key,Vec3> sums;
    if (smooth) {
        for (const auto& obj : model.objects) for (const auto& tri : obj.triangles) {
            const Vec3 a=tri.corners[0].position, b=tri.corners[1].position, c=tri.corners[2].position;
            const Vec3 n=cross(b-a,c-a);
            for (const auto& corner : tri.corners) sums[keyOf(corner.position)] = sums[keyOf(corner.position)] + n;
        }
    }
    for (auto& obj : model.objects) for (auto& tri : obj.triangles) {
        const Vec3 a=tri.corners[0].position, b=tri.corners[1].position, c=tri.corners[2].position;
        const Vec3 face=normalized(cross(b-a,c-a));
        for (auto& corner : tri.corners) {
            corner.normal = smooth ? normalized(sums[keyOf(corner.position)]) : face;
            corner.hasNormal = true;
        }
    }
}
} // namespace losv

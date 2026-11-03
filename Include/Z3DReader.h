#pragma once
// Reader for the recovered Line of Sight: Vietnam Z3D binary layout.
// It deliberately rejects unknown layouts instead of guessing their structure.
#include "MeshData.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace losv {
namespace z3d {
constexpr std::size_t kObjectNameOffset=8, kObjectNameSize=256;
constexpr std::size_t kMaterialCountOffset=328, kMaterialTableOffset=332;
constexpr std::size_t kMaterialStride=792, kMaterialNameOffset=0, kTextureNameOffset=256;
constexpr std::size_t kMaterialStringSize=256, kGeometryCountSize=4, kTriangleSize=100, kCornerSize=32, kMaterialIndexOffset=96;
constexpr std::uint32_t kMaxMaterials=512;

inline std::uint32_t readU32(const std::vector<std::uint8_t>& b, std::size_t p) {
    if (p > b.size() || b.size()-p < 4) throw std::runtime_error("Unexpected EOF reading uint32.");
    return std::uint32_t(b[p]) | (std::uint32_t(b[p+1])<<8) |
           (std::uint32_t(b[p+2])<<16) | (std::uint32_t(b[p+3])<<24);
}
inline float readF32(const std::vector<std::uint8_t>& b, std::size_t p) {
    const auto bits=readU32(b,p); float v{}; std::memcpy(&v,&bits,4); return v;
}
inline void writeU32(std::vector<std::uint8_t>& b, std::size_t p, std::uint32_t v) {
    if (p > b.size() || b.size()-p < 4) throw std::runtime_error("Write outside Z3D buffer.");
    for (int i=0;i<4;++i) b[p+i]=static_cast<std::uint8_t>((v>>(i*8))&0xff);
}
inline void writeF32(std::vector<std::uint8_t>& b, std::size_t p, float v) {
    std::uint32_t bits{}; std::memcpy(&bits,&v,4); writeU32(b,p,bits);
}
inline std::string fixedString(const std::vector<std::uint8_t>& b, std::size_t p, std::size_t n) {
    if (p>b.size()) return {};
    n=std::min(n,b.size()-p); std::size_t len=0;
    while(len<n && b[p+len]!=0) ++len;
    std::string s(reinterpret_cast<const char*>(b.data()+p),len);
    while(!s.empty() && (s.back()==' '||s.back()=='\t'||s.back()=='\r'||s.back()=='\n')) s.pop_back();
    return s;
}
inline std::vector<std::uint8_t> readBytes(const std::filesystem::path& path) {
    std::ifstream in(path,std::ios::binary);
    if(!in) throw std::runtime_error("Cannot open: "+path.string());
    in.seekg(0,std::ios::end); const auto end=in.tellg();
    if(end<0) throw std::runtime_error("Cannot determine file size: "+path.string());
    in.seekg(0,std::ios::beg); std::vector<std::uint8_t> b(static_cast<std::size_t>(end));
    if(!b.empty() && !in.read(reinterpret_cast<char*>(b.data()),static_cast<std::streamsize>(b.size())))
        throw std::runtime_error("Failed to read: "+path.string());
    return b;
}
inline Model parse(const std::vector<std::uint8_t>& b) {
    if(b.size()<kMaterialTableOffset) throw std::runtime_error("File is too small for recovered LOS Vietnam Z3D header.");
    const std::uint32_t materialCount=readU32(b,kMaterialCountOffset);
    if(materialCount==0 || materialCount>kMaxMaterials) throw std::runtime_error("Unsupported/invalid material count at offset 328.");
    const std::size_t tableBytes=std::size_t(materialCount)*kMaterialStride;
    if(tableBytes>b.size()-kMaterialTableOffset) throw std::runtime_error("Material table extends beyond end of file.");
    const std::size_t geometryCountOffset=kMaterialTableOffset+tableBytes;
    if (b.size()-geometryCountOffset < kGeometryCountSize) throw std::runtime_error("Missing geometry triangle-count field.");
    const std::uint32_t declaredTriangleCount=readU32(b,geometryCountOffset);
    const std::size_t geometryOffset=geometryCountOffset+kGeometryCountSize;
    const std::size_t geometryBytes=b.size()-geometryOffset;
    if(geometryBytes==0 || geometryBytes%kTriangleSize!=0) throw std::runtime_error("Geometry section is not a whole number of 100-byte triangle records; unsupported layout or trailing data.");
    const std::size_t triangleCount=geometryBytes/kTriangleSize;
    if(declaredTriangleCount!=triangleCount) throw std::runtime_error("Geometry triangle-count field does not match file length.");
    if(triangleCount>10'000'000) throw std::runtime_error("Unreasonable triangle count; refusing allocation.");
    Model model; model.sourceGeometryOffset=geometryOffset; model.sourceObjectCount=1;
    std::string objectName=fixedString(b,kObjectNameOffset,kObjectNameSize);
    if(objectName.empty()) objectName="Z3D_Object";
    model.objects.push_back({objectName,{}});
    model.materials.reserve(materialCount);
    for(std::uint32_t i=0;i<materialCount;++i) {
        const std::size_t m=kMaterialTableOffset+std::size_t(i)*kMaterialStride;
        Material mat; mat.name=fixedString(b,m+kMaterialNameOffset,kMaterialStringSize);
        mat.texture=fixedString(b,m+kTextureNameOffset,kMaterialStringSize);
        if(mat.name.empty()) throw std::runtime_error("Material slot "+std::to_string(i)+" has no readable name.");
        model.materials.push_back(std::move(mat));
    }
    auto& obj=model.objects.front(); obj.triangles.reserve(triangleCount);
    for(std::size_t t=0;t<triangleCount;++t) {
        const std::size_t rec=geometryOffset+t*kTriangleSize;
        Triangle tri; tri.material=readU32(b,rec+kMaterialIndexOffset);
        if(tri.material>=materialCount) throw std::runtime_error("Triangle references material slot outside material table.");
        for(std::size_t c=0;c<3;++c) {
            const std::size_t q=rec+c*kCornerSize;
            auto& v=tri.corners[c];
            v.position={readF32(b,q),readF32(b,q+4),readF32(b,q+8)};
            v.uv={readF32(b,q+16),readF32(b,q+20)};
            const float vals[]={v.position.x,v.position.y,v.position.z,v.uv.u,v.uv.v};
            for(float x:vals) if(!std::isfinite(x)||std::fabs(x)>1.0e8f) throw std::runtime_error("Non-finite or implausible vertex/UV value; unsupported layout.");
        }
        obj.triangles.push_back(std::move(tri));
    }
    model.warnings.push_back("Recovered layout exposes one named object per file; multi-object Z3D chunks are not yet documented.");
    model.warnings.push_back("Source normal semantics are unknown; normals must be recalculated for OBJ export.");
    return model;
}
inline Model read(const std::filesystem::path& path) { return parse(readBytes(path)); }
} // namespace z3d
} // namespace losv

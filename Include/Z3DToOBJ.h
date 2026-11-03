#pragma once
#include "Z3DReader.h"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace losv {
namespace obj {
inline std::string token(std::string s) {
    if(s.empty()) return "unnamed";
    for(char& c:s) { const unsigned char u=static_cast<unsigned char>(c); if(!(std::isalnum(u)||c=='_'||c=='-'||c=='.')) c='_'; }
    return s;
}
inline std::string materialToken(std::size_t i,const Material& m) {
    return "mat_"+std::to_string(i)+"_"+token(m.name);
}
inline void writeMtl(const Model& model,const std::filesystem::path& path) {
    std::ofstream out(path); if(!out) throw std::runtime_error("Cannot create MTL: "+path.string());
    out<<"# Line of Sight: Vietnam material export\n# Copy original texture files beside this MTL.\n\n"<<std::fixed<<std::setprecision(6);
    for(std::size_t i=0;i<model.materials.size();++i) {
        const auto& m=model.materials[i];
        out<<"newmtl "<<materialToken(i,m)<<"\nKa 0.2 0.2 0.2\nKd 1 1 1\nKs 0 0 0\nNs 10\nd 1\nillum 2\n";
        if(!m.texture.empty()) out<<"map_Kd "<<std::filesystem::path(m.texture).filename().generic_string()<<"\n";
        out<<"# source material: "<<m.name<<"\n\n";
    }
    if(!out) throw std::runtime_error("Failed writing MTL: "+path.string());
}
inline void write(const Model& model,const std::filesystem::path& objPath,bool smoothNormals=true) {
    if(model.objects.empty()) throw std::runtime_error("Model contains no objects.");
    Model copy=model; calculateNormals(copy,smoothNormals);
    auto mtlPath=objPath; mtlPath.replace_extension(".mtl"); writeMtl(copy,mtlPath);
    std::ofstream out(objPath); if(!out) throw std::runtime_error("Cannot create OBJ: "+objPath.string());
    out<<std::setprecision(9)<<"# Converted from Line of Sight: Vietnam Z3D\n"
       <<"# Positions and UVs retain source values unless coordinate options were selected.\n"
       <<"mtllib "<<mtlPath.filename().generic_string()<<"\n";
    std::size_t vertexBase=1, uvBase=1, normalBase=1;
    for(const auto& object:copy.objects) {
        out<<"\no "<<token(object.name)<<"\n";
        for(const auto& tri:object.triangles) for(const auto& c:tri.corners)
            out<<"v "<<c.position.x<<' '<<c.position.y<<' '<<c.position.z<<"\n";
        for(const auto& tri:object.triangles) for(const auto& c:tri.corners)
            out<<"vt "<<c.uv.u<<' '<<c.uv.v<<"\n";
        for(const auto& tri:object.triangles) for(const auto& c:tri.corners)
            out<<"vn "<<c.normal.x<<' '<<c.normal.y<<' '<<c.normal.z<<"\n";
        std::uint32_t current=std::numeric_limits<std::uint32_t>::max();
        for(std::size_t i=0;i<object.triangles.size();++i) {
            const auto& tri=object.triangles[i];
            if(tri.material>=copy.materials.size()) throw std::runtime_error("Triangle has invalid material index during OBJ export.");
            if(current!=tri.material) { current=tri.material; out<<"usemtl "<<materialToken(current,copy.materials[current])<<"\n"; }
            const std::size_t f=vertexBase+i*3;
            out<<"f "<<f<<'/'<<(uvBase+i*3)<<'/'<<(normalBase+i*3)<<' '
               <<f+1<<'/'<<(uvBase+i*3+1)<<'/'<<(normalBase+i*3+1)<<' '
               <<f+2<<'/'<<(uvBase+i*3+2)<<'/'<<(normalBase+i*3+2)<<"\n";
        }
        const std::size_t n=object.triangles.size()*3; vertexBase+=n; uvBase+=n; normalBase+=n;
    }
    if(!out) throw std::runtime_error("Failed writing OBJ: "+objPath.string());
}
inline void convert(const std::filesystem::path& input,const std::filesystem::path& output,
                    const CoordinateOptions& coordinates={},bool smoothNormals=true) {
    Model model=z3d::read(input); applyCoordinates(model,coordinates); write(model,output,smoothNormals);
}
} // namespace obj
} // namespace losv

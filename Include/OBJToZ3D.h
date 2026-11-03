#pragma once
// OBJ -> recovered LOS Vietnam Z3D layout writer.
// IMPORTANT: an original Z3D template is required to retain opaque engine metadata.
#include "Z3DReader.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <unordered_map>

namespace losv {
namespace objread {
struct ObjIndex { int v=0, vt=0, vn=0; };
inline int resolveIndex(int index,std::size_t count,const char* what) {
    if(index==0) throw std::runtime_error(std::string("OBJ ")+what+" index zero is invalid.");
    const long long resolved=index>0 ? static_cast<long long>(index)-1 : static_cast<long long>(count)+index;
    if(resolved<0 || resolved>=static_cast<long long>(count)) throw std::runtime_error(std::string("OBJ ")+what+" index out of range.");
    return static_cast<int>(resolved);
}
inline ObjIndex parseFaceIndex(const std::string& token) {
    ObjIndex idx; std::stringstream ss(token); std::string part;
    if(!std::getline(ss,part,'/')) throw std::runtime_error("Malformed OBJ face index.");
    idx.v=std::stoi(part);
    if(std::getline(ss,part,'/')) { if(!part.empty()) idx.vt=std::stoi(part); if(std::getline(ss,part,'/')&&!part.empty()) idx.vn=std::stoi(part); }
    return idx;
}
inline void loadMtl(const std::filesystem::path& path,std::vector<Material>& materials,
                    std::unordered_map<std::string,std::uint32_t>& ids) {
    std::ifstream in(path); if(!in) return;
    std::string line; int active=-1;
    while(std::getline(in,line)) {
        std::istringstream ss(line); std::string cmd; ss>>cmd;
        if(cmd.empty()||cmd[0]=='#') continue;
        if(cmd=="newmtl") { std::string name; ss>>name; if(name.empty()) continue;
            auto it=ids.find(name); if(it==ids.end()) { active=static_cast<int>(materials.size()); ids[name]=static_cast<std::uint32_t>(active); materials.push_back({name,{}}); }
            else active=static_cast<int>(it->second);
        } else if(cmd=="map_Kd" && active>=0) { std::string tex; std::getline(ss,tex); const auto p=tex.find_first_not_of(" \t"); if(p!=std::string::npos) materials[static_cast<std::size_t>(active)].texture=tex.substr(p); }
    }
}
inline Model read(const std::filesystem::path& path) {
    std::ifstream in(path); if(!in) throw std::runtime_error("Cannot open OBJ: "+path.string());
    std::vector<Vec3> positions,normals; std::vector<Vec2> uvs; Model model;
    std::unordered_map<std::string,std::uint32_t> materialIds;
    std::vector<std::filesystem::path> mtlFiles;
    model.objects.push_back({path.stem().string(),{}});
    std::string line; std::uint32_t activeMaterial=0; bool anyFace=false;
    while(std::getline(in,line)) {
        std::istringstream ss(line); std::string cmd; ss>>cmd;
        if(cmd.empty()||cmd[0]=='#') continue;
        if(cmd=="v") { Vec3 v; if(!(ss>>v.x>>v.y>>v.z)) throw std::runtime_error("Malformed OBJ vertex."); positions.push_back(v); }
        else if(cmd=="vt") { Vec2 uv; if(!(ss>>uv.u>>uv.v)) throw std::runtime_error("Malformed OBJ texture coordinate."); uvs.push_back(uv); }
        else if(cmd=="vn") { Vec3 n; if(!(ss>>n.x>>n.y>>n.z)) throw std::runtime_error("Malformed OBJ normal."); normals.push_back(n); }
        else if(cmd=="o"||cmd=="g") { std::string name; std::getline(ss,name); const auto p=name.find_first_not_of(" \t"); if(p!=std::string::npos && !anyFace) model.objects.front().name=name.substr(p); }
        else if(cmd=="mtllib") { std::string f; while(ss>>f) mtlFiles.push_back(path.parent_path()/f); }
        else if(cmd=="usemtl") { std::string name; ss>>name; if(name.empty()) name="Default";
            auto it=materialIds.find(name); if(it==materialIds.end()) { activeMaterial=static_cast<std::uint32_t>(model.materials.size()); materialIds[name]=activeMaterial; model.materials.push_back({name,{}}); } else activeMaterial=it->second;
        }
        else if(cmd=="f") {
            std::vector<ObjIndex> face; std::string item; while(ss>>item) face.push_back(parseFaceIndex(item));
            if(face.size()<3) throw std::runtime_error("OBJ face has fewer than three vertices.");
            anyFace=true;
            // Fan triangulation supports convex n-gons; non-convex polygons should be triangulated upstream.
            for(std::size_t k=1;k+1<face.size();++k) {
                const ObjIndex ids[3]={face[0],face[k],face[k+1]}; Triangle tri; tri.material=activeMaterial;
                for(int c=0;c<3;++c) {
                    auto& dst=tri.corners[static_cast<std::size_t>(c)];
                    dst.position=positions.at(static_cast<std::size_t>(resolveIndex(ids[c].v,positions.size(),"vertex")));
                    if(ids[c].vt) dst.uv=uvs.at(static_cast<std::size_t>(resolveIndex(ids[c].vt,uvs.size(),"UV")));
                    if(ids[c].vn) { dst.normal=normals.at(static_cast<std::size_t>(resolveIndex(ids[c].vn,normals.size(),"normal"))); dst.hasNormal=true; }
                }
                model.objects.front().triangles.push_back(std::move(tri));
            }
        }
    }
    for(const auto& mtl:mtlFiles) loadMtl(mtl,model.materials,materialIds);
    if(model.materials.empty()) model.materials.push_back({"Default",{}});
    if(model.objects.front().triangles.empty()) throw std::runtime_error("OBJ contains no faces.");
    // A usemtl encountered before its mtllib may have created placeholders; merge MTL properties by name.
    for(const auto& mtl:mtlFiles) {
        std::vector<Material> parsed; std::unordered_map<std::string,std::uint32_t> temp; loadMtl(mtl,parsed,temp);
        for(const auto& src:parsed) for(auto& dst:model.materials) if(dst.name==src.name && !src.texture.empty()) dst.texture=src.texture;
    }
    model.sourceObjectCount=1;
    model.warnings.push_back("OBJ object/group boundaries are merged into one Z3D object because the recovered Z3D layout exposes one object-name field.");
    model.warnings.push_back("Z3D corner normal storage is not understood; OBJ normals are not written into the proprietary file.");
    return model;
}
} // namespace objread

namespace z3dwrite {
inline void writeFixedString(std::vector<std::uint8_t>& b,std::size_t p,std::size_t n,const std::string& value) {
    if(p>b.size()||b.size()-p<n) throw std::runtime_error("Fixed string field outside output buffer.");
    std::fill(b.begin()+static_cast<std::ptrdiff_t>(p),b.begin()+static_cast<std::ptrdiff_t>(p+n),0);
    const std::size_t count=std::min(n-1,value.size()); std::copy_n(value.begin(),count,b.begin()+static_cast<std::ptrdiff_t>(p));
}
inline void write(const Model& model,const std::filesystem::path& output,const std::filesystem::path& templatePath) {
    if(model.objects.empty()||model.objects.front().triangles.empty()) throw std::runtime_error("No mesh triangles to write.");
    if(model.materials.empty()||model.materials.size()>z3d::kMaxMaterials) throw std::runtime_error("Material count is outside supported range.");
    const auto templ=z3d::readBytes(templatePath);
    (void)z3d::parse(templ); // Validate template compatibility before copying opaque fields.
    const std::uint32_t templateMaterialCount=z3d::readU32(templ,z3d::kMaterialCountOffset);
    if(templ.size()<z3d::kMaterialTableOffset+std::size_t(templateMaterialCount)*z3d::kMaterialStride+z3d::kTriangleSize)
        throw std::runtime_error("Template has no geometry record to use as a corner metadata template.");
    const std::size_t triangleCount=[&](){std::size_t n=0;for(const auto& o:model.objects)n+=o.triangles.size();return n;}();
    const std::size_t geometryCountOffset=z3d::kMaterialTableOffset+model.materials.size()*z3d::kMaterialStride;
    const std::size_t outSize=geometryCountOffset+z3d::kGeometryCountSize+triangleCount*z3d::kTriangleSize;
    std::vector<std::uint8_t> out(outSize,0);
    // Preserve all bytes before the material-count field, including unknown header fields.
    std::copy_n(templ.begin(),z3d::kMaterialCountOffset,out.begin());
    z3d::writeU32(out,z3d::kMaterialCountOffset,static_cast<std::uint32_t>(model.materials.size()));
    const std::string objectName=model.objects.front().name.empty()?"OBJ_Imported":model.objects.front().name;
    writeFixedString(out,z3d::kObjectNameOffset,z3d::kObjectNameSize,objectName);
    for(std::size_t i=0;i<model.materials.size();++i) {
        const std::size_t dst=z3d::kMaterialTableOffset+i*z3d::kMaterialStride;
        const std::size_t src=z3d::kMaterialTableOffset+std::min<std::size_t>(i,templateMaterialCount-1)*z3d::kMaterialStride;
        std::copy_n(templ.begin()+static_cast<std::ptrdiff_t>(src),z3d::kMaterialStride,out.begin()+static_cast<std::ptrdiff_t>(dst));
        writeFixedString(out,dst+z3d::kMaterialNameOffset,256,model.materials[i].name);
        writeFixedString(out,dst+z3d::kTextureNameOffset,256,model.materials[i].texture);
    }
    const std::size_t templateGeometry=z3d::kMaterialTableOffset+std::size_t(templateMaterialCount)*z3d::kMaterialStride+z3d::kGeometryCountSize;
    z3d::writeU32(out,geometryCountOffset,static_cast<std::uint32_t>(triangleCount));
    std::size_t triOut=geometryCountOffset+z3d::kGeometryCountSize;
    for(const auto& object:model.objects) for(const auto& tri:object.triangles) {
        if(tri.material>=model.materials.size()) throw std::runtime_error("OBJ face references missing material slot.");
        for(std::size_t c=0;c<3;++c) {
            const std::size_t src=templateGeometry+c*z3d::kCornerSize;
            const std::size_t dst=triOut+c*z3d::kCornerSize;
            std::copy_n(templ.begin()+static_cast<std::ptrdiff_t>(src),z3d::kCornerSize,out.begin()+static_cast<std::ptrdiff_t>(dst));
            const auto& v=tri.corners[c];
            const float vals[]={v.position.x,v.position.y,v.position.z,v.uv.u,v.uv.v};
            for(float x:vals) if(!std::isfinite(x)||std::fabs(x)>1.0e8f) throw std::runtime_error("OBJ contains non-finite or implausible coordinates.");
            z3d::writeF32(out,dst,v.position.x); z3d::writeF32(out,dst+4,v.position.y); z3d::writeF32(out,dst+8,v.position.z);
            z3d::writeF32(out,dst+16,v.uv.u); z3d::writeF32(out,dst+20,v.uv.v);
        }
        z3d::writeU32(out,triOut+z3d::kMaterialIndexOffset,tri.material);
        triOut+=z3d::kTriangleSize;
    }
    const auto tempPath=output.string()+".tmp";
    { std::ofstream file(tempPath,std::ios::binary); if(!file) throw std::runtime_error("Cannot create output Z3D.");
      file.write(reinterpret_cast<const char*>(out.data()),static_cast<std::streamsize>(out.size()));
      if(!file) throw std::runtime_error("Failed writing output Z3D."); }
    std::error_code ec; std::filesystem::rename(tempPath,output,ec);
    if(ec) { std::filesystem::remove(output,ec); ec.clear(); std::filesystem::rename(tempPath,output,ec); }
    if(ec) throw std::runtime_error("Cannot finalize output Z3D: "+ec.message());
}
inline void convert(const std::filesystem::path& input,const std::filesystem::path& output,
                    const std::filesystem::path& templatePath,const CoordinateOptions& coordinates={}) {
    Model model=objread::read(input); applyCoordinates(model,coordinates); write(model,output,templatePath);
}
} // namespace z3dwrite
} // namespace losv

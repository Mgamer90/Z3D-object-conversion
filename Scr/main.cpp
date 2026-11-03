#include "BatchConverter.h"
#include "OBJToZ3D.h"
#include <cstdlib>
#include <iostream>
#include <string>

namespace fs=std::filesystem;
static void usage(const char* exe) {
    std::cout<<"Line of Sight: Vietnam Z3D / OBJ converter (C++17)\n\n"
      <<"Commands:\n"
      <<"  "<<exe<<" batch-to-obj <input-folder> <output-folder> [--recursive] [--axis XYZ|XZY|YXZ|YZX|ZXY|ZYX] [--scale N] [--negate-x] [--negate-y] [--negate-z] [--flip-v] [--flat-normals]\n"
      <<"  "<<exe<<" to-obj <input.Z3D> <output.obj> [coordinate options]\n"
      <<"  "<<exe<<" to-z3d <input.obj> <output.Z3D> --template <original.Z3D> [coordinate options]\n\n"
      <<"Coordinate options default to unchanged source coordinates.\n"
      <<"OBJ->Z3D requires a compatible original Z3D template to preserve unknown metadata.\n";
}
static losv::AxisOrder parseAxis(const std::string& s) {
    if(s=="XYZ") return losv::AxisOrder::XYZ;
    if(s=="XZY") return losv::AxisOrder::XZY;
    if(s=="YXZ") return losv::AxisOrder::YXZ;
    if(s=="YZX") return losv::AxisOrder::YZX;
    if(s=="ZXY") return losv::AxisOrder::ZXY;
    if(s=="ZYX") return losv::AxisOrder::ZYX;
    throw std::runtime_error("Unknown axis order: "+s);
}
static void parseOptions(int argc,char** argv,int start,losv::CoordinateOptions& c,bool& smooth,bool& recursive,fs::path* templ=nullptr) {
    for(int i=start;i<argc;++i) {
        const std::string a=argv[i];
        if(a=="--recursive") recursive=true;
        else if(a=="--flat-normals") smooth=false;
        else if(a=="--negate-x") c.negateX=true;
        else if(a=="--negate-y") c.negateY=true;
        else if(a=="--negate-z") c.negateZ=true;
        else if(a=="--flip-v") c.flipV=true;
        else if(a=="--axis" && i+1<argc) c.order=parseAxis(argv[++i]);
        else if(a=="--scale" && i+1<argc) c.scale=std::stof(argv[++i]);
        else if(a=="--template" && i+1<argc && templ) *templ=argv[++i];
        else throw std::runtime_error("Unknown or incomplete option: "+a);
    }
    if(!std::isfinite(c.scale)||c.scale<=0.0f) throw std::runtime_error("Scale must be a finite positive number.");
}
int main(int argc,char** argv) {
    if(argc<2) { usage(argv[0]); return 2; }
    try {
        const std::string cmd=argv[1];
        if(cmd=="batch-to-obj") {
            if(argc<4) { usage(argv[0]); return 2; }
            losv::CoordinateOptions coords; bool smooth=true,recursive=false;
            parseOptions(argc,argv,4,coords,smooth,recursive);
            const fs::path outDir=argv[3];
            const auto report=losv::convertZ3DFolder(argv[2],outDir,recursive,coords,smooth);
            losv::writeReportCSV(report,outDir/"conversion_report.csv");
            losv::writeReportText(report,outDir/"conversion_report.txt");
            std::cout<<"Batch complete. Success: "<<report.succeeded()<<", unsupported/failed: "<<report.failed()<<"\n"
                     <<"Reports: "<<(outDir/"conversion_report.csv")<<" and conversion_report.txt\n";
            return report.failed()?1:0;
        }
        if(cmd=="to-obj") {
            if(argc<4) { usage(argv[0]); return 2; }
            losv::CoordinateOptions coords; bool smooth=true,recursive=false;
            parseOptions(argc,argv,4,coords,smooth,recursive);
            losv::obj::convert(argv[2],argv[3],coords,smooth);
            std::cout<<"Converted Z3D to OBJ: "<<argv[3]<<"\n"; return 0;
        }
        if(cmd=="to-z3d") {
            if(argc<4) { usage(argv[0]); return 2; }
            losv::CoordinateOptions coords; bool smooth=true,recursive=false; fs::path templ;
            parseOptions(argc,argv,4,coords,smooth,recursive,&templ);
            if(templ.empty()) throw std::runtime_error("Missing --template original.Z3D");
            losv::z3dwrite::convert(argv[2],argv[3],templ,coords);
            std::cout<<"Wrote template-based Z3D: "<<argv[3]<<"\n"; return 0;
        }
        usage(argv[0]); return 2;
    } catch(const std::exception& e) { std::cerr<<"ERROR: "<<e.what()<<"\n"; return 1; }
}

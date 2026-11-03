#pragma once
#include "MeshData.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <vector>

namespace losv {
struct ConversionEntry {
    std::filesystem::path input, output;
    std::string status, message;
    std::size_t objects=0, triangles=0, materials=0;
};
struct ConversionReport {
    std::vector<ConversionEntry> entries;
    std::size_t succeeded() const { std::size_t n=0; for(const auto& e:entries) if(e.status=="SUCCESS") ++n; return n; }
    std::size_t failed() const { return entries.size()-succeeded(); }
};
inline std::string csvEscape(const std::string& s) {
    std::string r="\""; for(char c:s) { if(c=='\"') r+="\"\""; else r+=c; } return r+"\"";
}
inline void writeReportCSV(const ConversionReport& report,const std::filesystem::path& path) {
    std::ofstream out(path); if(!out) throw std::runtime_error("Cannot create report: "+path.string());
    out<<"input,output,status,objects,triangles,materials,message\n";
    for(const auto& e:report.entries) out<<csvEscape(e.input.string())<<','<<csvEscape(e.output.string())<<','
        <<csvEscape(e.status)<<','<<e.objects<<','<<e.triangles<<','<<e.materials<<','<<csvEscape(e.message)<<'\n';
}
inline void writeReportText(const ConversionReport& report,const std::filesystem::path& path) {
    std::ofstream out(path); if(!out) throw std::runtime_error("Cannot create report: "+path.string());
    out<<"Line of Sight: Vietnam Z3D batch conversion report\n"
       <<"Files: "<<report.entries.size()<<" | Successful: "<<report.succeeded()<<" | Unsupported/failed: "<<report.failed()<<"\n\n";
    for(const auto& e:report.entries) out<<'['<<e.status<<"] "<<e.input.string()<<"\n  Output: "<<e.output.string()
        <<"\n  Objects: "<<e.objects<<" | Triangles: "<<e.triangles<<" | Materials: "<<e.materials
        <<"\n  Details: "<<e.message<<"\n\n";
}
} // namespace losv

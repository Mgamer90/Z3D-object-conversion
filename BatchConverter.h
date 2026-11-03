#pragma once
#include "ConversionReport.h"
#include "Z3DToOBJ.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <stdexcept>

namespace losv {
inline std::string lowerExtension(const std::filesystem::path& p) {
    std::string e=p.extension().string(); for(char& c:e) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return e;
}
inline ConversionReport convertZ3DFolder(const std::filesystem::path& inputDir,
                                         const std::filesystem::path& outputDir,
                                         bool recursive=false,
                                         const CoordinateOptions& coordinates={},
                                         bool smoothNormals=true) {
    namespace fs=std::filesystem;
    if(!fs::exists(inputDir)||!fs::is_directory(inputDir)) throw std::runtime_error("Input path is not a directory: "+inputDir.string());
    fs::create_directories(outputDir);
    ConversionReport report;
    auto process=[&](const fs::directory_entry& entry) {
        if(!entry.is_regular_file()||lowerExtension(entry.path())!=".z3d") return;
        ConversionEntry row; row.input=entry.path();
        const fs::path relative=fs::relative(entry.path(),inputDir);
        row.output=outputDir/relative.parent_path()/(entry.path().stem().string()+".obj");
        fs::create_directories(row.output.parent_path());
        try {
            Model model=z3d::read(row.input); applyCoordinates(model,coordinates); obj::write(model,row.output,smoothNormals);
            row.status="SUCCESS"; row.objects=model.objects.size(); row.materials=model.materials.size();
            for(const auto& o:model.objects) row.triangles+=o.triangles.size();
            row.message="Converted; one named object recovered from this file.";
            for(const auto& w:model.warnings) row.message+=" | "+w;
        } catch(const std::exception& e) { row.status="UNSUPPORTED"; row.message=e.what(); }
        report.entries.push_back(std::move(row));
    };
    if(recursive) { for(const auto& e:fs::recursive_directory_iterator(inputDir)) process(e); }
    else { for(const auto& e:fs::directory_iterator(inputDir)) process(e); }
    return report;
}
} // namespace losv

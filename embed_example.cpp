// Example: call the converters directly from another C++ application.
#include "BatchConverter.h"
#include "OBJToZ3D.h"
#include <iostream>

int main() {
    try {
        // Convert one Z3D file to OBJ, keeping original coordinates.
        losv::CoordinateOptions coords;
        losv::obj::convert("barbfence01.Z3D", "barbfence01.obj", coords, true);

        // Convert a whole directory. Set recursive=true to include subfolders.
        const auto report = losv::convertZ3DFolder("models", "obj_export", true, coords, true);
        losv::writeReportCSV(report, "obj_export/conversion_report.csv");
        losv::writeReportText(report, "obj_export/conversion_report.txt");

        // Convert an edited OBJ back to Z3D using an original file as metadata template.
        losv::z3dwrite::convert("edited.obj", "edited.Z3D", "barbfence01.Z3D", coords);

        std::cout << "Succeeded: " << report.succeeded() << ", failed: " << report.failed() << '\n';
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}

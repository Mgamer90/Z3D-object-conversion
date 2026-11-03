# Line of Sight: Vietnam Z3D / OBJ Converter

Header-based C++17 converter. No third-party libraries are required.

## Build

### MinGW-w64
```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic -Iinclude src/main.cpp -o losv_converter.exe
```

### Visual Studio Developer Command Prompt
```bat
cl /std:c++17 /EHsc /O2 /W4 /I include src\main.cpp /Fe:losv_converter.exe
```

## Commands

Batch-convert a folder:
```bash
losv_converter.exe batch-to-obj input output
```

Include subfolders:
```bash
losv_converter.exe batch-to-obj input output --recursive
```

Single Z3D to OBJ:
```bash
losv_converter.exe to-obj barbfence01.Z3D barbfence01.obj
```

OBJ back to Z3D (requires a compatible original Z3D template):
```bash
losv_converter.exe to-z3d edited.obj edited.Z3D --template barbfence01.Z3D
```

Coordinate options (available for both directions):
```text
--axis XYZ|XZY|YXZ|YZX|ZXY|ZYX
--scale 1.0
--negate-x --negate-y --negate-z
--flip-v
```

OBJ normal generation:
```text
(default) smooth vertex normals
--flat-normals
```

## Headers

- `MeshData.h`: shared mesh/material structures, coordinate transforms and normal generation.
- `Z3DReader.h`: strict reader for the recovered LOS Vietnam binary layout.
- `Z3DToOBJ.h`: OBJ/MTL exporter.
- `OBJToZ3D.h`: OBJ parser and template-based Z3D writer.
- `BatchConverter.h`: folder scanning and batch conversion.
- `ConversionReport.h`: CSV and text reports.

## Format and safety limitations

The known sample has a 328-byte pre-material header, a material count at byte 328, 792-byte material records beginning at byte 332, then a 4-byte triangle count followed by 100-byte triangle records. Each triangle has three 32-byte corners and a 32-bit material index.

This is a recovered layout, not an official format specification. The parser deliberately rejects files that do not match it. The inspected format exposes one object-name field; separate object chunks have not been established, so one object is reported per Z3D file. OBJ object/group boundaries are merged when writing Z3D.

The reverse writer requires an original compatible Z3D template. It preserves the opaque header and copies opaque material/corner metadata from that template while updating known names, texture filenames, positions, UVs and material indices. It does not recover unknown engine-specific fields, source normals, collision data, LODs, animation, or editor-only metadata. Validate generated files in the original Line of Sight: Vietnam editor before using them in-game. Always keep backups.

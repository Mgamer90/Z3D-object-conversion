# Recovered LOS: Vietnam Z3D layout notes

Based on binary inspection and successful parsing of `barbfence01.Z3D`.

| Field | Offset / size | Current interpretation |
|---|---:|---|
| Leading float | 0 / 4 | Unknown (sample bit pattern corresponds to 1.1) |
| Header integer | 4 / 4 | Unknown (sample value 1) |
| Object name | 8 / 256 | Null-terminated fixed-width string |
| Material count | 328 / 4 | Little-endian uint32 |
| Material table | 332 / count × 792 | Fixed-size material records |
| Material name | material + 0 / 256 | Null-terminated string |
| Diffuse texture | material + 256 / 256 | Null-terminated texture filename |
| Triangle count | 332 + count × 792 / 4 | Little-endian uint32 |
| Triangle records | after triangle count / 100 each | Three 32-byte corners + uint32 material index |
| Corner position | corner + 0 / 12 | Three little-endian float32 values |
| Unknown corner field | corner + 12 / 4 | Sample is constant `00 00 00 FF`; meaning unknown |
| UV | corner + 16 / 8 | Two little-endian float32 values |
| Unknown corner tail | corner + 24 / 8 | Zero in inspected sample |
| Triangle material | triangle + 96 / 4 | Little-endian material-slot index |

The parser checks the declared triangle count against the remaining file length and validates every material index and all position/UV floats.

## Known sample

`barbfence01.Z3D`: 1 object name (`Rectangle01`), 21 material slots, 112 triangles, 98 unique exact XYZ positions, and 17 unique exact UV pairs. It references `sign01.tga`, `POST.tga`, and `barb01.tga`.

## Unknowns / not implemented

- Meaning of the leading header values and most fields inside each 792-byte material record.
- Whether other game/editor versions use another header or trailing sections.
- Whether some Z3D files contain multiple object chunks; the inspected layout has one object-name field and one geometry block.
- Native normal, tangent, collision, LOD, animation, and editor-only data.

## Reverse writer behavior

OBJ-to-Z3D requires a compatible original Z3D template. The writer retains the first 328 header bytes, clones material records from the template (using its first record if the OBJ needs more slots), and clones the first triangle's per-corner opaque bytes. It then updates the known object/material names, texture filenames, triangle count, XYZ, UV and material index fields. This is a best-effort compatible writer, not a complete reconstruction of every proprietary field. Test the output in the original editor before game use.

# About
This is a version of PCB, the printed circuit board design tool used in the gEDA suite, with an export function to write the layout into a KiCad layout file.

Not all features are supported:
- Footprints are translated to KiCad footprints with pins, pads and lines. No arcs or attributes
- Text size is not preserved
- Thermals on pins and vias may be different, because PCB allowed manual setting, while KiCad connects automatically based on assigned nets.
- Polygon areas can be different in KiCad, because of different handling of clearances and holes in KiCad and PCB.

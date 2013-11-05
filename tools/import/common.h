#ifndef COMMON_H
#define COMMON_H

enum VoxelType
  {
    _UChar,
    _Char,
    _UShort,
    _Short,
    _Int,
    _Float,
    _Rgb,
    _Rgba
  };

  enum VoxelUnit {
    _Nounit = 0,
    _Angstrom,
    _Nanometer,
    _Micron,
    _Millimeter,
    _Centimeter,
    _Meter,
    _Kilometer,
    _Parsec,
    _Kiloparsec
  };

#endif // COMMON_H

# incbin

Include binary files in your C/C++ applications with ease

## Example

    #include "incbin.h"

    INCBIN(Icon, "icon.png");

    // This translation unit now has three symbols
    // const unsigned char gIconData[];
    // const unsigned char *gIconEnd;
    // const unsigned int gIconSize;

    // Reference in other translation units like this
    INCBIN_EXTERN(Icon);

    // This translation unit now has two extern symbols
    // extern const unsigned char gIconData[];
    // extern const unsigned char *gIconEnd;
    // extern const unsigned int gIconSize;

## Portability

Known to work on the following compilers

* GCC
* Clang
* PathScale
* Intel
* Solaris
* Green Hills
* XCode
* ArmCC
* MSVC _See MSVC below_

If your compiler is not listed, as long as it supports GCC inline assembler, this
should work.

## Alignment

The data included by this tool will be aligned on the architectures word boundary
unless `__SSE__`, `__AVX__` or `__neon__` is defined, then it's aligned on a byte
boundary that respects SIMD convention. The table of the alignments for SIMD
alignment is as follows

| SIMD | Alignment |
|------|-----------|
| SSE  | 16        |
| Neon | 16        |
| AVX  | 32        |

## Explanation

`INCBIN` is a macro which uses the inline assembler provided by almost all
compilers to include binary files. It achieves this by utilizing the `.incbin`
directive of the inline assembler. It then uses the assembler to calculate the
size of the included binary and exports two global symbols that can be externally
referenced in other translation units which contain the data and size of the
included binary data respectively.

## MSVC

Supporting MSVC is slightly harder as MSVC lacks an inline assembler which can
include data. To support this we ship a tool which can process source files
containing `INCBIN` macro usage and generate an external source file containing
the data of all of them combined. This file is named `data.c` by default.
Just include it into your build and use the `incbin.h` to reference data as
needed. It's suggested you integrate this tool as part of your projects's
pre-build events so that this can be automated. A more comprehensive list of
options for this tool can be viewed by invoking the tool with `-help`

## Miscellaneous

Documentation for the API is provided by the header using Doxygen notation.
For licensing information see UNLICENSE.

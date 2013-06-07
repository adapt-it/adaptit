// prototypes for functions called in tellenc.cpp
#ifndef _TELLENC_h
#define _TELLENC_h

const char* tellenc2(const unsigned char* const buffer, const size_t len);

// BEW added 16Aug11, for support of CollabUtilities.cpp's MoveTextToFolderAndSave()
const char* check_ucs_bom(const unsigned char* const buffer);


// GDLC Removed conditionals for PPC Mac (with gcc4.0 they are no longer needed)
void init_utf8_char_table();
const char* tellenc(const char* const buffer, const size_t len);

#endif

// prototypes for functions called in tellenc.cpp
#ifndef _TELLENC_h
#define _TELLENC_h

const char* tellenc2(const unsigned char* const buffer, const size_t len);

// GDLC Temporary work around for PPC STL library bug
#if defined(__WXMAC__) && defined(__POWERPC__ )
// tellenc() not used in PPC builds pending bug fix in PPC STL
#else
void init_utf8_char_table();
const char* tellenc(const char* const buffer, const size_t len);
#endif

#endif

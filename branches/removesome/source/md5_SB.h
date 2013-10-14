/////////////////////////////////////////////////////////////////////////////
// Name:        md5_SB.h   
// Purpose:     MD5 file checksum  (SB = 'Single Byte')
// Author:      Francesco Montorsi; converted to CBString, Bruce Waters 17June2013
// Created:     2005/07/13
// RCS-ID:      $Id: md5.h 3303 2013-06-17 02:51:00Z Bruce Waters $
// Copyright:   (c) 2005 Francesco Montorsi
// Licence:     wxWidgets licence + RDS Data Security license
/////////////////////////////////////////////////////////////////////////////

/*
 **********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved. **
 **                                                                  **
 ** License to copy and use this software is granted provided that   **
 ** it is identified as the "RSA Data Security, Inc. MD5 Message     **
 ** Digest Algorithm" in all material mentioning or referencing this **
 ** software or this function.                                       **
 **                                                                  **
 ** License is also granted to make and use derivative works         **
 ** provided that such works are identified as "derived from the RSA **
 ** Data Security, Inc. MD5 Message Digest Algorithm" in all         **
 ** material mentioning or referencing the derived work.             **
 **                                                                  **
 ** RSA Data Security, Inc. makes no representations concerning      **
 ** either the merchantability of this software or the suitability   **
 ** of this software for any particular purpose.  It is provided "as **
 ** is" without express or implied warranty of any kind.             **
 **                                                                  **
 ** These notices must be retained in any copies of any part of this **
 ** documentation and/or software.                                   **
 **********************************************************************
 */
 

#ifndef _WX_MD5_SB_H_
#define _WX_MD5_SB_H_

// wxWidgets headers
//#include "wx/webupdatedef.h"		// for the WXDLLIMPEXP_WEBUPDATE macro
#include "wx/string.h"
//#include "wx/wfstream.h"


class CBString;
// ------------
// 
// MD5 from RSA
// ------------

#define MD5_HASHBYTES 16

typedef struct md5Context {
        unsigned int buf[4];
        unsigned int bits[2];
        unsigned char in[64];
} md5_CTX;

void   md5Init(md5_CTX *context);
//void   MD5Update(MD5_CTX *context, unsigned char const *buf, unsigned len);
void   md5Update(md5_CTX *context, unsigned char *buf, unsigned len);
void   md5Final(unsigned char digest[MD5_HASHBYTES], md5_CTX *context);
void   md5Transform(unsigned int buf[4], unsigned int const in[16]);
char * md5End(md5_CTX *, char *);



//! A utility class to calculate MD5 checksums from files or strings.
class md5_SB
{
public:
	md5_SB();
	virtual ~md5_SB();

public:

	//! Returns the MD5 checksum for the given file
//	static wxString GetFileMD5(wxInputStream &str);
//	static wxString GetFileMD5(const wxString &filename);

	//! Returns the MD5 for the given string.
	//static CBString GetMD5(const CBString &str);
	static CBString GetMD5(CBString &str); // BEW removed constant-ness, I got into trouble with compiler
};

#endif		// _WX_MD5_SB_H_

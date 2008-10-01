/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			BString.h
/// \author			Bruce Waters
/// \date_created	October 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CBString class.
/// CBString is a CBString look-alike, but it is strictly single-byte strings throughout,
/// so I can have CString functionalities for char strings (null terminated) in a
/// Unicode application. MFC's CString is wide characters when _UNICODE is
/// defined, which is a nuisance for handling my UTF-8 XML parser. So I'll code
/// my XML-supporting stuff using CBString. I'll only need MFC's CString for accepting
/// UTF-16 (converted internally in my callbacks from PCDATA which was read
/// in as UTF-8). Code for the conversions is elsewhere in my app already.
/// My CBString implementation will use standard C string functions (eg. strcpy, etc)
/// and use heap pointers for storage. (No hassles with locking and unlocking!!)
/// CBString lengths are int, so can be long. No attempt will be made to support MBCS
/// characters and so the implementation will be simpler than on the Dana. And error
/// checking will be almost nothing, and no error messages, to keep it as concise as possible.s
/// \derivation The BString class is not a derived class.
/////////////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _CBSTRING_H_
#define _CBSTRING_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "BString.h"
#endif

// helpers
// WX Note: GetResStr() requires MFC library code
//char* 	GetResStr(unsigned int strID);

/// CBString is a CBString look-alike, but it is strictly single-byte strings throughout,
/// so I can have CString functionalities for char strings (null terminated) in a
/// Unicode application. MFC's CString is wide characters when _UNICODE is
/// defined, which is a nuisance for handling my UTF-8 XML parser. So I'll code
/// my XML-supporting stuff using CBString. I'll only need MFC's CString for accepting
/// UTF-16 (converted internally in my callbacks from PCDATA which was read
/// in as UTF-8). Code for the conversions is elsewhere in my app already.
/// My CBString implementation will use standard C string functions (eg. strcpy, etc)
/// and use heap pointers for storage. (No hassles with locking and unlocking!!)
/// CBString lengths are int, so can be long. No attempt will be made to support MBCS
/// characters and so the implementation will be simpler than on the Dana. And error
/// checking will be almost nothing, and no error messages, to keep it as concise as possible.s
/// \derivation The BString class is not a derived class.
class CBString
{
	private:
	
	char* 	pstr; 	// pointer to the C string
	int		len;  	// length, excluding terminating null byte

	public:
	
	// constructors
	CBString();							// default constructor
	CBString(const char* s);				// constructor initialized from C string
	CBString(const CBString& s);			// copy constructor


	// implementation
	int 		Delete(int nIndex,int nCount=1);
	void 		Empty();
	int	 		Find(const char* pSubStr,int nStart=0);
	int			Find(char ch,int nStart=0);
	int			Find(CBString subStr,int nStart=0);
	void 		Format(const char* pControlStr, ...);
	// WX NOTE: The Format() function below requires MFC library code
	//void			Format(unsigned int ID, ...); // ID is a string resource ID
	char 		GetAt(int nIndex);
	int			GetLength() const;
	int 		Insert(int nIndex, const char* pStr);	
	int 		Insert(int nIndex, const CBString s);	
	int 		Insert(int nIndex, char ch);	
	bool 		IsEmpty();
	CBString 	Left(int n);	
	CBString& 	MakeReverse();
	CBString 	Mid(const int nStart, const int nCount = -1);
	int 			Remove(char ch);
	CBString 	Right(int n);
	CBString 	Right(const char* pLoc);
	void			SetAt(int nIndex, char ch);
	CBString 	SpanIncluding(CBString subStr,CBString* pRemainder=NULL); 
	CBString 	SpanIncluding(const char* pSubStr,CBString* pRemainder=NULL); 
	CBString 	SpanExcluding(CBString subStr,CBString* pRemainder=NULL); 
	CBString 	SpanExcluding(const char* pSubStr,CBString* pRemainder=NULL);
	void			Truncate(int nBytes); 
	wxString	Convert8To16();
	
	char* 		FindToPtr(const char* pSubStr,int nStart=0);
	char* 		GetBuffer();	
	char*		GetBuffer(int nMinBufferLength);
	void		ReleaseBuffer(int nNewLength = -1);
	operator 	char*();
	//operator	const char*();

			
	// overrides and operators
	CBString& 		operator=(const CBString& s);
	CBString& 		operator=(const char* s);
	CBString& 		operator=(const char& ch);
	CBString& 		operator+=(const CBString& s);
	CBString& 		operator+=(const char* s);
	CBString& 		operator+=(char ch);
	char operator 	[](int nIndex) const;
	
	
	// the following works if s is a char* (ie. C-string) due to
	// the compiler doing an implicit conversion (this returns a
	// copy, and leaves this constant)
	CBString 		operator+(const CBString& s);
	
	// friends
	friend CBString 	operator+(const CBString& s1, char ch);
	friend CBString 	operator+(char ch, const CBString& s2);
	
	// The next is required to handle s1 being a C-string
	friend CBString	operator+(char* s1,const CBString& s2);

	// Note: because there is a constructor which initializes from a C-string, due to
	// implicit conversion this group of functions will also work if s1 is a C-string
	friend int 		Compare(const CBString& s1, const CBString& s2);
	friend bool 	operator<(const CBString& s1, const CBString& s2);
	friend bool 	operator>(const CBString& s1, const CBString& s2);
	friend bool 	operator==(const CBString& s1, const CBString& s2);
	friend bool 	operator!=(const CBString& s1, const CBString& s2);
	
	// The next group will work if s2 is a C-string
	friend int 		Compare(const CBString& s1, const char* s2);
	friend bool 	operator<(const CBString& s1, const char* s2);
	friend bool 	operator>(const CBString& s1, const char* s2);
	friend bool 	operator==(const CBString& s1, const char* s2);
	friend bool 	operator!=(const CBString& s1, const char* s2);
	
	// destructor
	virtual 		~CBString();
};

#endif // for _CBSTRING_H_


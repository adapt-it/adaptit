/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			BString.cpp
/// \author			Bruce Waters
/// \date_created	October 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the CBString class.
/// The CBString class creates byte sized MFC look-alike CBStrings.
/// This implementation is deliberately lightweight; it uses C string functions
/// wherever possible & does almost no error checking (errors should be
/// picked up when debugging, and typically would be pstr NULL or a bogus
/// len value; once debugged, the module should work without error)
/// \derivation The BString class is not a derived class.
/////////////////////////////////////////////////////////////////////////////

// **** The following conditional compile directives added for wxWidgets support
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "BString.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif
// **** The above conditional compile directives added for wxWidgets support


//#include "stdafx.h"
#include "Adapt_It.h"
#include "BString.h"

// the following typedefs allow my Palm OS integer types to be used unchanged
typedef short unsigned int UInt16;
typedef unsigned int UInt32;
typedef short int Int16;
typedef int Int32;

// This implementation is deliberately lightweight; it uses C string functions
// wherever possible & does almost no error checking (errors should be
// picked up when debugging, and typically would be pstr NULL or a bogus
// len value; once debugged, the module should work without error)

/*********************************************
*
* Underlying comparison function which all the
* comparison operators use. Returns < 0 if s1 < s2,
* 0 if s1 == s2, > 0 if s1 > s2
*
**********************************************/

int Compare(const CBString& s1, const CBString& s2)
{
	int result = 0;
	CBString* ps1 = const_cast<CBString*>(&s1);
	CBString* ps2 = const_cast<CBString*>(&s2);
	result = strcmp(ps1->pstr,ps2->pstr);
	return result;
}

/*********************************************
*
* Comparison < operator (friend of CBString)
* Can be used for comparing CBStrings or a C-string
* as first argument with a CBString as second argument
*
**********************************************/

bool operator<(const CBString& s1, const CBString& s2)
{
	int result = Compare(s1,s2);
	if (result < 0)
		return true;
	else
		return false;
}

/*********************************************
*
* Comparison > operator (friend of CBString) 
* Can be used for comparing CBStrings or a C-string
* first argument with a CBString as second argument
*
**********************************************/

bool operator>(const CBString& s1, const CBString& s2)
{
	int result = Compare(s1,s2);
	if (result > 0)
		return true;
	else
		return false;
}

/*********************************************
*
* Comparison == operator (friend of CBString) can
* be used for comparing CBStrings or a C-string
* first argument with a CBString as second argument
*
**********************************************/

bool operator==(const CBString& s1, const CBString& s2)
{
	int result = Compare(s1,s2);
	if (result == 0)
		return true;
	else
		return false;
}

/*********************************************
*
* Inequality != operator (friend of CBString) can
* be used for inequality of CBStrings or a C-string
* first argument with a CBString as second argument
*
**********************************************/

bool operator!=(const CBString& s1, const CBString& s2)
{
	int result = Compare(s1,s2);
	if (result == 0)
		return false;
	else
		return true;
}
// Now the ones which take a C-string as second argument
// - for these we need to overload the underlying
// Compare function too.

int Compare(const CBString& s1, const char* s2)
{
	int result = 0;
	CBString* ps1 = const_cast<CBString*>(&s1);
	result = strcmp(ps1->pstr,s2);
	return result;
}

/*********************************************
*
* Comparison < operator (friend of CBString) can
* be used for comparing CBString with a C-string 
* as second argument
*
**********************************************/

bool operator<(const CBString& s1, const char* s2)
{
	int result = Compare(s1,s2);
	if (result < 0)
		return true;
	else
		return false;
}

/*********************************************
*
* Comparison > operator (friend of CBString) can
* be used for comparing CBString with a C-string 
* as second argument
*
**********************************************/

bool operator>(const CBString& s1, const char* s2)
{
	int result = Compare(s1,s2);
	if (result > 0)
		return true;
	else
		return false;
}

/*********************************************
*
* Comparison == operator (friend of CBString) can
* be used for comparing CBString with a C-string 
* as second argument
*
**********************************************/

bool operator==(const CBString& s1, const char* s2)
{
	int result = Compare(s1,s2);
	if (result == 0)
		return true;
	else
		return false;
}

/*********************************************
*
* Inequality != operator (friend of CBString) can
* be used for inequality of CBString with a C-string 
* as second argument
*
**********************************************/

bool operator!=(const CBString& s1, const char* s2)
{
	int result = Compare(s1,s2);
	if (result == 0)
		return false;
	else
		return true;
}

/*********************************************
*
* Addition operator + (friend of CBString) can be used for appending
* a CBString s2 to a  C-string s1, returning a copy of the result. 
*
**********************************************/

CBString operator+(char* s1,const CBString& s2)
{
	int s1Len = strlen(s1);
	CBString* ps = const_cast<CBString*>(&s2);
	int s2Len = ps->len;
	char* pBuff = new char[s1Len + s2Len + 1];
	memset(pBuff,0,s1Len + s2Len + 1);
	strcpy(pBuff,s1);
	strcat(pBuff,ps->pstr);
	CBString dest(pBuff);
	delete[] pBuff;
	return dest; // return a copy
}

/*********************************************
*
* Addition operator + (friend of CBString)
* Use for appending a char to a CBString s1,
* returning a copy of the result.
*
**********************************************/
CBString operator+(const CBString& s1, char ch)
{
	// temporarily defeat the const specification
	CBString* ps = const_cast<CBString*>(&s1);
	char* pStr = new char[ps->len + 2];
	memset(pStr,0,ps->len + 2);
	strncpy(pStr,ps->pstr,ps->len);
	pStr[ps->len] = ch;
	CBString str(pStr);
	delete[] pStr;
	return str;
}

/*********************************************
*
* Addition operator + (friend of CBString)
* Use for appending a char to a C-string s1,
* returning a copy of the result.
*
**********************************************/

CBString operator+(char ch, const CBString& s2)
{
	// temporarily defeat the const specification
	CBString* ps = const_cast<CBString*>(&s2);
	
	// get a repository for the result
	char* pStr = new char[ps->len + 2];
	memset(pStr,0,ps->len + 2);
	
	// do the concatenation
	pStr[0] = ch;
	strcpy(pStr+1,ps->pstr); 
	CBString dest(pStr);
	// BEW added 05Nov05, failure to delete pStr buffer leaked memory
	delete[] pStr;
	return dest;
}

/*********************************************
*
* GetResStr(unsigned int strID)
*
* Returns: a pointer to char, which points to a copied
* C-string in a dynamic memory chunk.
* We assume the resource string is valid, found, and returned
* without error!
*
**********************************************/
/* // WX Note: GetResStr() requires MFC library code
char* GetResStr(unsigned int strID)
{
	HINSTANCE hApp = AfxGetInstanceHandle();
	char*  pRscBuffer = new char[256]; // caller must dispose of this buffer!
	int howmanycharscopied =  LoadStringA(hApp,strID,pRscBuffer,256);
	return pRscBuffer;
}
*/

/***********************************************************************************
************************************************************************************
*
* CBString class methods
*
************************************************************************************
************************************************************************************/



/*********************************************
*
* Default constructor
*
**********************************************/

CBString::CBString()
{
		len = 0;
		pstr = new char[1];
		memset(pstr,0,1);	
}


/*********************************************
*
* Construct CBString from a C-string
*
**********************************************/

CBString::CBString(const char* s)
{
	len = strlen(s);
	pstr = new char[len + 1];
	memset(pstr,0,len + 1);
	if(pstr)
	{
		strcpy(pstr,s);
	}
	else
	{
		len = 0;
		pstr = NULL;
	}
}

/*********************************************
*
* Copy constructor
*
**********************************************/

CBString::CBString(const CBString& s)
{
	len = s.len;
	pstr = new char[len + 1];
	memset(pstr,0,len + 1);
	if(pstr)
	{
		strcpy(pstr,s.pstr);
	}
	else
	{
		len = 0;
		pstr = NULL;
	}
}

/*********************************************
*
* Destructor
*
**********************************************/

CBString::~CBString()
{
	if (pstr)
		delete[] pstr;
}

/*********************************************
*
* operator char*()
*
* Returns: a pointer to the owned C-string
* This is a typecasting operator.
* 
*
**********************************************/

CBString::operator char*()
{
	if (pstr)
		return pstr;
	else
		return (char*)NULL;
}

/*********************************************
*
* operator const char*()
*
* Returns: a pointer to the const C-string
* This is a typecasting operator.
* 
*
**********************************************/
/*
CBString::operator const char*()
{
	if (pstr)
		return const_cast<char*>(pstr);
	else
		return (const char*)NULL;
}
*/
/*********************************************
*
* Delete(int nIndex,UInt nCount)
*
* Returns: the new length of the string
*
* Call this member function to delete a character or 
* characters from a string starting with the character
* at nIndex. If nCount is longer than the string, the
* remainder of the string will be removed.
*
* If nIndex does not lie within the string, it
* returns the length of the string unchanged
* and deletes nothing.
*
**********************************************/

int CBString::Delete(int nIndex,int nCount)
{
	if (nIndex > len-1)
		return len;
	if (nIndex + nCount >= len)
	{
		// we will be deleting all the rest
		nCount = len - nIndex; // don't exceed bounds
		
		// create a replacement buffer of the needed size
		char* pRep = new char[nIndex + 1];
		
		// move the start string portion into it & add null
		memmove(pRep,pstr,nIndex);
		pRep[nIndex] = '\0';

		// now make hRep the owned handle, and delete the original
		len = nIndex;
		delete[] pstr;
		pstr = pRep;
		return len;
	}
	else
	{
		//we will be deleting internally within the string
		char* p = pstr + nIndex; // point to first char
		
		// create a replacement buffer of the needed size
		char* pRep = new char[len - nCount + 1];
	
		// move the start and end string portions into it
		memmove(pRep,pstr,nIndex);
		p += nCount; // point to first character of the end section
		
		// copy the last part, including the terminating null
		memmove(pRep + nIndex,p,len - nIndex - nCount +1);
		
		// now make hRep the owned handle, and delete the original
		delete[] pstr;
		len = len - nCount;
		pstr = pRep;
		return len;
	}
}

/*********************************************
*
* Empty()
*
* Returns: nothing
* Clears the CBString to be an array containing
* just the null byte.
*
**********************************************/

void CBString::Empty()
{
	delete[] pstr;
	pstr = new char[1];
	memset(pstr,0,1);
	len = 0;
}

/*********************************************
*
* Find(const char* pSubStr, int nStart = 0)
*
* Returns: The zero-based index of the first character 
* in this CBString object that matches the requested 
* substring; -1 if the substring is not found. 
*
* The search starts from the character at nStart.
* If pSubStr empty, it returns the index of the first
* character, i.e. zero
*
**********************************************/

int CBString::Find(const char* pSubStr,int nStart)
{
	int subLen = strlen(pSubStr);
	if (subLen == 0)
		return 0;
	if (nStart >= len)
		return -1;
	char* pStart = pstr + nStart; // where to start from
	char* pFound = strstr(pStart,pSubStr);
	if (pFound)
		return (int)(pFound - pstr);
	else
		return -1;
}

/*********************************************
*
* Find(const char ch, int nStart = 0)
*
* Returns: The zero-based index of the first instance 
* of ch in this CBString object; -1 if the character ch
* is not found. 
*
* The search starts from the character at nStart.
*
**********************************************/

int CBString::Find(char ch,int nStart)
{
	if (nStart >= len)
		return -1;
	char* pStart = pstr + nStart; // where to start from
	char* pFound = strchr(pStart,(int)ch);
	if (pFound)
		return (int)(pFound - pstr);
	else
		return -1;
}

/*********************************************
*
* CBString::Find(const CBString subStr, int nStart=0)
*
* Returns: The zero-based index of the first character 
* in the owned C-string that matches the passed in CBString
* subStr; -1 if subStr is not found.
*
**********************************************/

int CBString::Find(CBString subStr,int nStart)
{
	// next line should always give valid pSubStr
	char* pSubStr = subStr.GetBuffer(); 
	int count = Find(pSubStr,nStart);
	return count;
}

/*********************************************
*
* FindToPtr(const char* pSubStr, int nStart = 0)
*
* Returns: a pointer to the first character 
* in this CBString object that matches the requested 
* substring; NULL if the substring is not found. 
*
* The search starts from the character at nStart.
* If pSubStr is empty, it returns a pointer to the
* start of the invoking CBString's string data
*
**********************************************/

char* CBString::FindToPtr(const char* pSubStr,int nStart)
{
	int subLen = strlen(pSubStr);
	if (subLen == 0)
		return pstr;
	if (nStart >= len)
		return (char*)NULL;
	char* pStart = pstr + nStart; // where to start from
	char* pFound = strstr(pStart,pSubStr);
	if (pFound)
		return pFound;
	else
		return (char*)NULL;
}

/*********************************************
*
* Format(const char* pControlStr, ...)
*
* Returns: nothing
*
* Formats a variable number of arguments according
* to the format specifications within the pControlStr
* format-control C-string. There MUST be as many arguments
* as there are format specifications within pControlStr.
*
* Format specifications are as in fprinf() in stdio.h.
* They are: % to start the specification, followed
* by 0 to 4 optional specification arguments (not
* listed here) followed by the obligatory conversion
* specifier, the more useful ones being:
* d  signed decimal (same as i)
* e  floating point or double in scientific format
* f  floating point or double
* s  null terminated character array
* c  a character
* p  pointer, output in hex format using x
* #s pascal string (length byte followed by byte array)
* x  unsigned hex
* i  signed decimal
* u  unsigned decimal
* h  short int (signed or unsigned, ie. int)
*
* Note1: the first parameter can be a CBString only if it
* is explicitly cast to char*
*
* Note2: implicit conversion does not happen here, so
* arguments must be actual wanted types or explicitly cast
* to the wanted types - for example, do not use CBString
* as one of the variable arguments, instead use 
* (char*)CBString
**********************************************/

void CBString::Format(const char* pControlStr, ...)
{
	char* buffer = new char[512]; // allow the final string to be 511 bytes long
	memset(buffer,0,512);
	int len_buffer;
	va_list args;
	va_start(args,pControlStr);
	len_buffer = vsprintf(buffer,pControlStr,args);
	va_end(args);
	if (pstr)
		delete[] pstr; // abandon any earlier string, free its chunk
	char* pStr = new char[len_buffer + 1]; // make buffer of exact length
	memset(pStr,0,len_buffer + 1);
	strcpy(pStr,buffer);
	delete[] buffer; // delete the 512 byte buffer
	pstr = pStr; // attach the formatted string to this
	len = len_buffer; // set the length
}

/*********************************************
*
* Format(unsigned int ID, ...)
*
* Returns: nothing
*
* Takes a resource ID for a format control string, and formats the
* string using the supplied arguments.
*
**********************************************/
/* //WX NOTE: 
void CBString::Format(unsigned int ID, ...)
{
	char* buffer = new char[512]; // allow the final string to be 511 bytes long
	memset(buffer,0,512);
	char* pControlStr = new char[512];
	pControlStr = GetResStr(ID);
	int len_buffer;
	va_list args;
	va_start(args,ID);
	len_buffer = vsprintf(buffer,pControlStr,args);
	va_end(args);
	if (pstr)
		delete[] pstr; // abandon any earlier string, free its chunk
	char* pStr = new char[len_buffer + 1]; // make buffer of exact length
	memset(pStr,0,len_buffer + 1);
	strcpy(pStr,buffer);
	delete[] buffer; // delete the 512 byte buffer
	pstr = pStr; // attach the formatted string
	len = len_buffer; // set the length
	delete[] pControlStr;
}
*/

/*********************************************
*
* GetAt(int nIndex)
*
* Returns the character at index nIndex. nIndex must be >= 0,
* and < the value returned by GetLength()
*
* Bounds checking is always done on nIndex, and if it is
* out of range, a null character is returned.
*
**********************************************/

char CBString::GetAt(int nIndex)
{
	char ch = '\0';
	if (nIndex >= 0 && nIndex < len)
	{
		return pstr[nIndex];
	}
	return ch;
}

/*********************************************
*
* GetBuffer()
*
* Returns: a pointer to the owned C-string
*
**********************************************/

char* CBString::GetBuffer()
{
	return pstr;	
}

/*********************************************
*
* GetBuffer(int nMinBufferLength)
*
* Returns: a pointer to the owned C-string
*
* Comment: overloaded version. If the string length
*	is greater than what is requested (ie. passed in
*	nMinBufferLength value), then no change; otherwise
*	the existing string is thrown away and an empty
*	memory chunk of nMinBufferLength size, set to zeros,
*	is created. (Be sure to allow for a null byte at
*	the end by passing in a large enough value.)
*
**********************************************/

char* CBString::GetBuffer(int nMinBufferLength)
{
	if (len > nMinBufferLength)
		return pstr;
	else
	{
		delete[] pstr;
		len = 0;
		pstr = new char[nMinBufferLength];
		memset(pstr,0,nMinBufferLength);
		return pstr;
	}
}

/*********************************************
*
* ReleaseBuffer()
*
* Sets the length of the string pointed at by
* a previous GetBuffer() call to be nNewLength; but
* if the default -1 is used, the string is assumed
* to be null byte terminated and its length is 
* calculated and len set to that value instead
*
**********************************************/
void CBString::ReleaseBuffer(int nNewLength)
{
	if (nNewLength >= 0)
	{
		char* pNewBuff = new char[nNewLength + 1]; // +1 for null byte
		memset(pNewBuff,0,nNewLength + 1);
		strncpy(pNewBuff,pstr,nNewLength);
		pNewBuff[nNewLength] = '\0';
		delete[] pstr;
		pstr = pNewBuff;
	}
	else
	{
		len = strlen(pstr);
	}
}

/*********************************************
*
* GetLength()
*
* Returns the length (in bytes) of the string,
* not including the null byte.
*
**********************************************/

int CBString::GetLength() const
{
	return len;
}


/*********************************************
*
* int Insert(int nIndex, char ch)
*
* Returns: the new length of the string (in bytes)
*
* Call this member function to insert a character at 
* the given index within the string. The nIndex parameter
* identifies the first character that will be moved to make
* room for it. If nIndex is zero, the insertion will occur
* before the entire string. If nIndex is higher than the 
* length of the string, the function will append the
* the character to the string.
*
***********************************************/

int CBString::Insert(int nIndex, char ch)
{
	if (nIndex == 0)
	{
		*this = ch + *this;
		return len;
	}
	
	// the general case
	if (nIndex > 0 && nIndex <= len - 1)
	{
		char* pStr = new char[len +2]; // null byte plus one more for ch
		strncpy(pStr,pstr,nIndex); // copy the first part
		pStr[nIndex] = ch; // insert the character
		strncpy(pStr + nIndex + 1, pstr + nIndex, len - nIndex + 1); // copy the remainder
		len += 1;
		delete[] pstr;
		pstr = pStr;
	}
	else if (nIndex > len - 1)
	{
		// concatenate
		*this += ch;
	}
	return len;
}

/*********************************************
*
* int Insert(int nIndex, const char* pStr)
*
* Returns: the new length of the string (in bytes)
*
* Call this member function to insert a substring at 
* the given index within the string. The nIndex parameter
* identifies the first character that will be moved to make
* room for the substring. If nIndex is zero, the insertion
* will occur before the entire string. If nIndex is higher
* than the length of the string, the function will append
* the new material provided by pStr to the owned string.
*
***********************************************/

int CBString::Insert(int nIndex, const char* pStr)
{
	int sLen = strlen(pStr);
	char* pNewStr = new char[len + sLen + 1];
	if (nIndex == 0)
	{
		strcpy(pNewStr,pStr);
		strcat(pNewStr,pstr);
		delete[] pstr;
		pstr = pNewStr;
		len += sLen;
		return len;
	}
	
	// the general case
	if (nIndex > 0 && nIndex <= len - 1)
	{
		strncpy(pNewStr,pstr,nIndex); // copy the first part
		pNewStr[nIndex] = '\0';
		strcat(pNewStr,pStr); // concatenate the passed in string
		// copy the remainder, including null byte
		strncpy(pNewStr + nIndex + sLen, pstr + nIndex, len - nIndex + 1);
		len += sLen;
		delete[] pstr;
		pstr = pNewStr;
	}
	else if (nIndex > len - 1)
	{
		// concatenate
		*this += pStr;
		delete[] pNewStr;
	}
	return len;
}

/*********************************************
*
* int Insert(int nIndex, const CBString s)
*
* Returns: the new length of the string
*
* Call this member function to insert a substring s at 
* the given index within the string. The nIndex parameter
* identifies the first character that will be moved to make
* room for the substring. If nIndex is zero, the insertion
* will occur before the entire string. If nIndex is higher
* than the length of the string, the function will append
* the new material provided by s to the owned string.
*
***********************************************/

int CBString::Insert(int nIndex, const CBString s)
{
	int length = 0;

	// need to remove const temporarily
	CBString* pcs = const_cast<CBString*>(&s);
	length = Insert(nIndex,pcs->pstr);
	return length;
}

/*********************************************
*
* IsEmpty()
*
* Returns a boolean value,  true if the CBString
* stores an empty C-string, false otherwise.
*
**********************************************/

bool CBString::IsEmpty()
{
	return pstr[0] == '\0';
}

/*********************************************
*
* Left(int n)
*
* Returns: A CBString which is a copy of the first
* n bytes; if n is greater than the length of the string, it
* returns a copyo of the whole string. 
*
**********************************************/

CBString CBString::Left(int n)
{
	if (n >= len)
		return *this; // return a copy
	char* pStr = new char[n + 1];
	memset(pStr,0,n + 1); // ensure null terminated
	strncpy(pStr,pstr,n);
	CBString shortie(pStr);
	delete[] pStr;
	return shortie;
}

/*********************************************
*
* MakeReverse()
*
* Returns: the reversed this
*
**********************************************/

CBString& CBString::MakeReverse()
{
	if (len == 0 || len == 1)
		return *this; 	// must be at least 2 bytes long
						// to generate something different
	//_strrev(pstr);
	// whm 27Sep06 _strrev() is apparently not available under Linux so here's a substitute
	char temp;
	int len, i, j;
	i = j = len = temp = 0;
	len = strlen(pstr);
	for (i=0, j=len-1; i<=j; i++, j--)
	{
		temp = pstr[i];
		pstr[i] = pstr[j];
		pstr[j] = temp;
	}
	return *this;
}

/*********************************************
*
* Truncate(int nBytes)
*
* Shortens the string at the right end by chopping
* off the last nBytes bytes. If nBytes is
* equal to or greater than the length of the
* string, the string is reduced to an empty string.
*
**********************************************/

void CBString::Truncate(int nBytes)
{
	// size this resizes smaller, it is virtually
	// guaranteed to never fail, so we won't use
	// try/catch mechanism
	if (nBytes >= len)
	{
		delete[] pstr;
		char* pStr = new char[1];
		memset(pStr,0,1); // null byte only
		len = 0;
	}
	else
	{
		int newLen = len - nBytes;
		char* pStr = new char[newLen + 1];
		memset(pStr,0,newLen + 1);
		strncpy(pStr,pstr,newLen);
		delete[] pstr;
		pstr = pStr;
		len = newLen;
	}
}

/*********************************************
*
* Mid(const int nStart, const int nCount = -1)
*
* Extracts nCount bytes beginning with the byte
* at nStart, or extracts the rest of the string,
* whichever is the lesser. If the second parameter 
* is absent, it extracts the rest of the string. 
* Returns a copy of the extracted substring.
*
**********************************************/

CBString CBString::Mid(const int nStart,const int nCount)
{
	int newLen;
	if (nCount == -1)
	{
		newLen = len - nStart;
	}
	else
	{
		// extract nCount or all, depending on nCount value
		if (nStart + nCount >= len)
			newLen = len - nStart;
		else
			newLen = nCount;
	}
	char* pStr = new char[newLen + 1];
	memset(pStr,0,newLen + 1);
	strncpy(pStr,pstr + nStart,newLen);
	CBString newStr(pStr);
	delete[] pStr;
	return newStr;
}

/*********************************************
*
* int Remove(char ch)
*
* Returns: the count of bytes removed
*
* Removes all the instances of the character ch from the
* string. The removal is case-sensitive. If ch == 't' then
* only instances of t are removed, and T is untouched.
*
***********************************************/

int CBString::Remove(char ch)
{
	char* pStr = new char[len + 1];
	memset(pStr,0,len + 1);
	char* pEnd = pstr + len;
	char* pLastLoc = pstr;
	char* pFound = NULL;
	int count = 0;
	int nSpan = 0;
	int nCopied = 0;
	do {
		if (pLastLoc >= pEnd)
			break;
		pFound = strchr(pLastLoc,(int)ch);
		if (pFound)
		{
			count++;
			nSpan = pFound - pLastLoc;
			strncpy(pStr + nCopied,pLastLoc,nSpan);
			nCopied += nSpan;
			pLastLoc = pFound + 1;
		}
		else
		{
			// copy the last bit
			strcat(pStr,pLastLoc);
		}
	} while (pFound != NULL);
	len -= count;
	delete[] pstr;
	pstr = pStr;
	return count;
}

/*********************************************
*
* Right(int n)
*
* Extracts the last (that is, rightmost) n characters 
* characters from this CBString object and returns a 
* copy of the extracted substring. If n exceeds the 
* string length, then the entire string is extracted.
*
**********************************************/

CBString CBString::Right(int n)
{
	CBString copy = *this;
	copy.MakeReverse();
	copy = copy.Left(n);// RHS returns a tempory copied CBString,
									// so copy is not erased before the assignment
	copy.MakeReverse();
	return copy;
}

/*********************************************
*
* Right(const char* pLoc)
*
* Extracts the rest of the string starting from the
* character pointed at by pLoc, returning a copy of 
* the substring extracted. The returned substring
* could be an empty string in appropriate circumstances -
* such as pLoc being NULL, or out of bounds.
*
**********************************************/

CBString CBString::Right(const char* pLoc)
{
	CBString s; // empty
	if (pLoc == NULL || pLoc < pstr || pLoc >= (pstr + len))
	{
		return s;
	}
	int nStart = (int)(pLoc - pstr);
	s =  this->Mid(nStart);
	return s;
}

/*********************************************
*
* SetAt(int nIndex, char ch)
*
* Returns: nothing
*
* Overwrites the character at byte index nIndex with
* the char passed in. Does nothing if nIndex is out of bounds
*
**********************************************/

void CBString::SetAt(int nIndex, char ch)
{
	if (nIndex < 0 || nIndex >= len)
		return;
	pstr[nIndex] = ch;
}

/*********************************************
*
* SpanIncluding(const char* pSubStr,CBString* pRemainder = NULL)
*
* Returns: a copied substring that contains characters in 
* the string that are in pSubStr, beginning with the first
* character in the string and ending when a character is
* found in the string that is not in pSubStr. SpanIncluding
* returns an empty substring if the first character in the
* string is not in the specified set.
* pSubStr is a C-string interpretted as an unordered
* set of characters.
* The rest of the string is returned in pRemainder. Pass NULL
* for this parameter if you don't want the remainder.
*
**********************************************/

CBString CBString::SpanIncluding(const char* pSubStr,CBString* pRemainder)
{
	char* pStr = new char[len + 1]; // where we will store extracted substring
	memset(pStr,0,len + 1);
	char* pLast = pStr;
	char* pFound = NULL;
	char* pNext = pstr;
	int count = 0;
	while (count < len)
	{

		pFound = strchr((char*)pSubStr,(int)*pNext);

		if (pFound)
		{
			*pLast++ = *pNext++; // copy across, and update pointers
			count++;
		}
		else
			break;
	}
	CBString s(pStr);
	delete[] pStr;
	if (pRemainder != NULL)
	{
		if (*pNext == '\0')
			(*pRemainder).Empty();
		else
			*pRemainder = pNext;
	}
	return s;
}	

/*********************************************
*
* SpanIncluding(CBString subStr,CBString* pRemainder = NULL)
*
* Returns: a copied substring that contains characters in 
* the string that are in subStr, beginning with the first
* character in the string and ending when a character is
* found in the string that is not in subStr. SpanIncluding
* returns an empty substring if the first character in the
* string is not in the specified set.
* subStr contains a C-string interpretted as an unordered
* set of characters.
* The rest of the string is returned in remainder. Pass NULL
* for this parameter if you don't want the remainder.
*
**********************************************/

CBString CBString::SpanIncluding(CBString subStr,CBString* pRemainder)
{
	CBString dest = SpanIncluding(subStr.pstr,pRemainder);
	return dest;
}

/*********************************************
*
* SpanExcluding(const char* pSubStr,CBString* pRemainder = NULL)
*
* Returns: a copied substring that contains characters in 
* the string that are not in pSubStr, beginning with the first
* character in the string and ending when a character is
* found in the string that is in pSubStr. SpanExcluding
* returns the whole string if no characters in the
* string are in the specified set; or nothing if the first character
* of the string is also in the substring.
* pSubStr is a C-string interpretted as an unordered
* set of characters.
* The rest of the string is returned in pRemainder. Pass NULL
* for this parameter if you don't want the remainder.
*
**********************************************/

CBString CBString::SpanExcluding(const char* pSubStr,CBString* pRemainder)
{
	char* pStr = new char[len + 1]; // where we will store extracted substring
	memset(pStr,0,len + 1);
	char* pLast = pStr;
	char* pFound = NULL;
	char* pNext = pstr;
	int count = 0;
	while (count < len)
	{

		pFound = strchr((char*)pSubStr,(int)*pNext);

		if (pFound)
		{
			break;
		}
		else
		{
			*pLast++ = *pNext++; // copy across, and update pointers
			count++;
		}
	}
	CBString s(pStr);
	delete[] pStr;
	if (pRemainder != NULL)
	{
		if (*pNext == '\0')
			(*pRemainder).Empty();
		else
			*pRemainder = pNext;
	}
	return s;
}

/*********************************************
*
* SpanExcluding(CBString subStr,CBString* pRemainder = NULL)
*
* Returns: a copied substring that contains characters in 
* the string that are not in subStr, beginning with the first
* character in the string and ending when a character is
* found in the string that is in subStr. SpanExcluding
* returns the whole string if no characters in the
* string are in the specified set.
* subStr is a C-string interpretted as an unordered
* set of characters. 
* The rest of the string is returned in pRemainder. Pass NULL
* for this parameter if you don't want the remainder.
*
**********************************************/

CBString CBString::SpanExcluding(CBString subStr,CBString* pRemainder)
{
	CBString dest = SpanExcluding(subStr.pstr,pRemainder);
	return dest;
}

/*********************************************
*
* Convert8to16()
*
* Returns:	A UTF-16 CString which is converted by the U82T macro
*			starting from this's stored char string
* Comment:
*	Convert8to16 is used for converting a single-byte character string
*	stored in the CBString instance. The latter is ASCII or UTF-8 (such
*	as returned from parsing XML data), and converting it to a UTF-16
*	string in a CString instance - and returning a copy of same.
*
*	BEW changed 8Apr06 to accomodate the buffer-safe new conversion macros in VS 2003,
*	which use malloc for buffer allocation of long string to be converted, etc.
*	(Note: CW2A is unreliable, so don't use it for converting UTF-16 back to UTF-8)
*
**********************************************/

wxString CBString::Convert8To16()
{
#ifdef _UNICODE
#if !wxUSE_UNICODE
#error "This program can't be built without wxWidgets library built with wxUSE_UNICODE set to 1"
#endif
	wxWCharBuffer buf(wxConvUTF8.cMB2WC(this->GetBuffer()));
	if(!buf.data())
		return buf;
	return wxString(buf); //wxWCharBuffer(L"");
#else
	wxWCharBuffer buf(wxConvUTF8.cMB2WC(this->GetBuffer()));
	if(!buf.data())
		return wxConvCurrent->cWC2WX(buf);
	return wxString(buf); //wxCharBuffer("");
#endif

	// MFC Code below:
	//wxString rv; // Return Value
	//CA2T pConvertedStr((LPSTR)this->GetBuffer(),eUTF8);
	/* previous MFC code:
	USES_CONVERSION_U8;
	int length = this->GetLength(); // how many bytes in the UTF-8 string
	// UTF-8 is not economical, so the UTF-16 string should be shorter, but we will allow plenty of room
	UINT len2 = sizeof(TCHAR) * (length + 1); 
	LPTSTR pConvertedStr = (LPTSTR)alloca(len2); 
	memset(pConvertedStr,0,len2); // set all to zero
	pConvertedStr = U82T((LPSTR)this->GetBuffer());
	*/
	//rv = pConvertedStr;
	//return rv;
}

/*********************************************
*
* operator+(const CBString& s)
*
* Returns: a copied string which is the concatenation
* of this with s. This and s remain unchanged on exit.
*
* Note: because of implicit conversion, this can
* also be used to append a C-string to a CBString.
* (string1 = string1 + string2; is safe syntax - it
* does not produce intermediate overlapping strings, but
* string1 += string2 gives the same result & is much quicker)
*
**********************************************/

CBString CBString::operator+(const CBString& s)
{
		CBString* ps = const_cast<CBString*>(&s);
		
		// make a buffer for receiving the concatenated strings
		char* pStr = new char[len + ps->len + 1];
		memset(pStr,0,len + ps->len + 1);
		
		//copy first string
		strcpy(pStr,pstr);
		
		// concatenate the second string
		strcat(pStr,ps->pstr);

		// create the CBString to be returned
		CBString dest(pStr);
		dest.len = len + ps->len;
		delete[] pStr;
		return dest;
}

/*********************************************
*
* Assignment operator: CBString to CBString
*
**********************************************/

CBString& CBString::operator=(const CBString& s)
{
	if (this == &s)
		return *this;
	int newLen = s.len;
	if (pstr != NULL)
		delete[] pstr;
	char* pStr = new char[newLen + 1];
	memset(pStr,0,newLen + 1);
	if (s.pstr != NULL)
	{
		strcpy(pStr,s.pstr);
		len = s.len;
	}
	else
	{
		pStr[0] = '\0';
		len = 0;
	}
	pstr = pStr;
	return *this;
}

/*********************************************
*
* Assignment operator: C-string to CBString
*
**********************************************/

CBString& CBString::operator=(const char* s)
{
	int newLen = strlen(s);
	if (pstr != NULL)
		delete[] pstr;
	char* pStr = new char[newLen + 1];
	memset(pStr,0,newLen + 1);
	strcpy(pStr,s);
	len = newLen;
	pstr = pStr;
	return *this;
}

/*********************************************
*
* Assignment operator: char to CBString
*
**********************************************/

CBString& CBString::operator=(const char& ch)
{
	if (pstr != NULL)
		delete[] pstr;
	char* pStr = new char[2];
	memset(pStr,0,2);
	pStr[0] = ch;
	len = 1;
	pstr = pStr;
	return *this;
}

/*********************************************
*
* operator+=(const CBString& s)
*
* Returns: reference to the concatenated strings
*
* That is, it concatenates s to itself and returns
* itself.
*
**********************************************/

CBString& CBString::operator+=(const CBString& s)
{
	int newLen = s.len + len;
	char* pStr = new char[newLen + 1];
	memset(pStr,0,newLen + 1);
	
	// copy this's string first, then append s's
	strcpy(pStr,pstr);
	strcat(pStr,s.pstr);
	delete[] pstr;
	pstr = pStr;
	len = newLen;
	return *this;
}

/*********************************************
*
* operator+=(const char* s)
*
* Returns: reference to the concatenation of
* a C-string to the CBString
*
* That is, it concatenates s to itself and returns
* itself.
*
**********************************************/

CBString& CBString::operator+=(const char* s)
{
	int newLen = strlen(s);
	newLen += len;
	char* pStr = new char[newLen + 1];
	memset(pStr,0,newLen + 1);
	
	// copy this's string first, then append s's
	strcpy(pStr,pstr);
	strcat(pStr,s);
	delete[] pstr;
	pstr = pStr;
	len = newLen;
	return *this;
}

/*********************************************
*
* operator+=(char ch)
*
* Returns: reference to the concatenation of
* a char to the CBString
*
**********************************************/

CBString& CBString::operator+=(char ch)
{
	int newLen = len + 1;
	char* pStr = new char[newLen + 1];
	memset(pStr,0,newLen + 1);
	
	// copy this's string first, then append s's
	strcpy(pStr,pstr);
	pStr[len] = ch;
	pStr[newLen] = '\0'; // append a null
	delete[] pstr;
	pstr = pStr;
	len = newLen;
	return *this;
}

/*********************************************
*
* operator [](int nIndex) const
*
* Returns the character at index nIndex.
*
* The [ ] operator CANNOT be used to set the value 
* of a character in the owned C-string. For that 
* use SetAt( ). 
*
**********************************************/

char CBString::operator [](int nIndex) const
{
	char ch = pstr[nIndex];
	return ch;
}





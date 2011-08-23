/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			helpers.cpp
/// \author			Bruce Waters, revised for wxWidgets by Bill Martin
/// \date_created	6 January 2005
/// \date_revised	20 July 2010
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file containing some helper functions used by Adapt It.
/// BEW 12Apr10, all changes for supporting doc version 5 are done for this file
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "helpers.h"
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

#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/colour.h>
#include <wx/dir.h>
#include <wx/textfile.h>

#include "Adapt_It.h"
#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"
#include "SourcePhrase.h"
#include "MainFrm.h"
#include "BString.h"
#include "WaitDlg.h"
#include "XML.h"
#include "SplitDialog.h"
#include "ExportFunctions.h"
#include "PlaceInternalMarkers.h"
#include "Uuid_AI.h" // for uuid support
// the following includes support friend functions for various classes
#include "TargetUnit.h"
#include "KB.h"
#include "Pile.h"
#include "Strip.h"
#include "Layout.h"
#include "KBEditor.h"
#include "RefString.h"
#include "helpers.h"
#include "tellenc.h"
#include "md5.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;
extern wxString szProjectConfiguration;

extern wxChar gSFescapechar; // the escape char used for start of a standard format marker

extern const wxChar* filterMkr; // defined in the Doc
extern const wxChar* filterMkrEnd; // defined in the Doc
const int filterMkrLen = 8;
const int filterMkrEndLen = 9;

extern bool gbIsGlossing;
extern bool gbGlossingUsesNavFont;
extern bool gbForceUTF8;
extern int  gnOldSequNum;
extern int  gnBeginInsertionsSequNum;
extern int  gnEndInsertionsSequNum;
extern bool gbTryingMRUOpen;
extern bool gbConsistencyCheckCurrent;
extern bool gbInhibitMakeTargetStringCall;


/// Length of the byte-order-mark (BOM) which consists of the three bytes 0xEF, 0xBB and 0xBF
/// in UTF-8 encoding.
#define nBOMLen 3

/// Length of the byte-order-mark (BOM) which consists of the two bytes 0xFF and 0xFE in
/// in UTF-16 encoding.
#define nU16BOMLen 2

#ifdef _UNICODE

/// The UTF-8 byte-order-mark (BOM) consists of the three bytes 0xEF, 0xBB and 0xBF
/// in UTF-8 encoding. Some applications like Notepad prefix UTF-8 files with
/// this BOM.
static wxUint8 szBOM[nBOMLen] = {0xEF, 0xBB, 0xBF};

/// The UTF-16 byte-order-mark (BOM) which consists of the two bytes 0xFF and 0xFE in
/// in UTF-16 encoding, reverse order if big-endian
static wxUint8 szU16BOM[nU16BOMLen] = {0xFF, 0xFE};
static wxUint8 szU16BOM_BigEndian[nU16BOMLen] = {0xFE, 0xFF};

#endif


//  helper functions


//*************************************
// Friend functions for various classes
//*************************************

// friend function for CKB class, to access private m_bGlossingKB flag
bool GetGlossingKBFlag(CKB* pKB)
{
	return pKB->m_bGlossingKB;
}

//*************************************
// end of friends
//*************************************

//////////////////////////////////////////////////////////////
/// \return the number of words in the string
/// \param      <- str  the string to check (by reference)
/// \remarks
/// A Helper function for counting the number of words in a string.
/// Note: Any initial and final whitespace is trimmed from the string
/// and the trimmed string is returned by reference to the caller.
/////////////////////////////////////////////////////////////
int TrimAndCountWordsInString(wxString& str)
{
	// this function works like the CountSourceWords() function in KB.cpp
	// except that medial space, tab, CR, or LF or any combination of them
	// in succession are all considered as a single whitespace delimiter
	if (str.Length() == 0)
		return 0;

	wxString tempStr = str; // work on a copy of the str
	tempStr.Trim(TRUE); // remove whitespace from right
	tempStr.Trim(FALSE); // remove whitespace from left
	str = tempStr; // update the string in the caller so it has no leading or trailing spaces

	// count the word tokens in str using a wxStringTokenizer method
	wxStringTokenizer tkz(tempStr); // by default any white space delimits the tokens
	return tkz.CountTokens();
}

//////////////////////////////////////////////////////////////
/// \return nothing
/// \remarks
/// A Helper function for doing a time delay in 1/100ths of a second.
/// Uses the global m_nCurDelay which is located in CAdapt_itApp.
/////////////////////////////////////////////////////////////
void DoDelay()
{
	// set up a time delay loop, according to the m_nCurDelay value (1/100ths sec)
	wxDateTime startTime;
	startTime.SetToCurrent(); // need to initialize or get run-time error

	int millisecsStart = startTime.GetMillisecond();
	int secsStart = startTime.GetSecond();
	int nSpan = gpApp->m_nCurDelay * 10; // convert hundredths to milliseconds
	wxDateTime currentTime;

	// work out the time to be elapsed before breaking out of loop
	int newmillisecs = millisecsStart + nSpan;
	int endsecs = secsStart + newmillisecs/1000; // same or up to 3 seconds larger
	int endMsecs = newmillisecs % 1000; // modulo operator, to get remainder millisecs
	if (endsecs > secsStart)
	{
		// the delay will take us into the next second's interval
m:					//_ftime(&currentTime);
		currentTime.SetToCurrent();
		if (currentTime.GetSecond() < endsecs)
			goto m; // loop till we get to the final second's interval
	}
	// loop for the remaining milliseconds in the delay
n:
	currentTime.SetToCurrent();
	if (currentTime.GetSecond() == endsecs && currentTime.GetMillisecond() <= endMsecs)
		goto n;
}

//////////////////////////////////////////////////////////////
/// \return an int representing the point size
/// \param height	-> the (negative) lfHeight value in the LOGFONT
/// \remarks
/// A Helper function for converting negative lfHeight values in a LOGFONT
/// to the equivalent point size. The absolute values of lfHeight
/// are greater than point size by an increment which is on a sliding
/// scale. Algorithm: if H = abs(lfHeight) or H= abs(lfHeight)-1 is a
/// multiple of 4, then increment Inc = H/4; else Inc =
/// (abs(lfHeight)+1)/4. Then point size = abs(lfHeight)-Inc
/////////////////////////////////////////////////////////////
int NegHeightToPointSize(const long& height)
{
	// Note: Bruce changed this algorithm in version 2.3.0 to use
	// MFC's GetDeviceCaps method of CClientDC to return pixels per
	// inch (dpiY) in the vertical dimension. Then used the following:
	// pointsize = (int)((abs(height) * 72) / dpiY + 0.5). He also
	// checked if there pView == NULL and if so set the return value
	// to 12 points default. I'm going to leave it as it was for now.
	int absHeight, H, inc;
	if(height>=0L) return (int)height;
	absHeight = (int)-height;
	if((H=absHeight) % 4 == 0) goto A;
	else if((H=absHeight-1) % 4 == 0) goto A;
	else H=absHeight+1;
A:	inc=H/4;
	return absHeight-inc;
}

//////////////////////////////////////////////////////////////
/// \return a long integer representing the lfHeight value of a font
/// \param pointSize	-> the font's point size
/// \remarks
/// A helper function for converting pointsize to a lfHeight value; defined for use in
/// versions 2.4.0 and onwards - these versions allow user-defined
/// font size for the vernacular text in dialogs.
/// Returns the (negative) lfHeight value corresponding to the passed in
/// pointsize.
/////////////////////////////////////////////////////////////
long PointSizeToNegHeight(const int& pointSize) // whm 9Mar04
{
	long lfHeight1, lfHeight2, lfHeight3;
	lfHeight1 = -1*((4*pointSize + 1)/3);
	lfHeight2 = -1*((4*pointSize)/3);
	lfHeight3 = -1*((4*pointSize - 1)/3);
	if (abs(lfHeight1) % 4 == 0) return lfHeight1;
	else if (abs(lfHeight2) % 4 == 0) return lfHeight2;
	else return lfHeight3;
}

// The PointSizeToLFHeight() function below by bw for v.2.4.0
// Not Used in the wxWidgets version
// helper for converting pointsize to a lfHeight value; defined for use in
//  versions 2.4.0 and onwards - these versions allow user-defined
// font size for the vernacular text in dialogs.
// Returns the (positive) lfHeight value corresponding to the passed in
// pointsize
//static int PointSizeToLFHeight(const int pointsize)
//{
//	int nHeight;
//	float dpiY;
//	CAdapt_ItApp* pApp = (CAdapt_ItApp*)AfxGetApp();
//	if (pApp != 0)
//	{
//		CAdapt_ItView* pView = pApp->GetView();
//		if (pView == 0)
//			goto a;
//		CClientDC  dc(pView);
//		dpiY = dc.GetDeviceCaps(LOGPIXELSY);
//		nHeight = (int) ((dpiY * pointsize) / 72);
//	}
//	else
//	{
//		// return any reasonable value if no view exists yet
//a:		return 16;
//	}
//	return nHeight;
//}



// helper for copying LOGFONT structs
// We're not using the MFC LOGFONT structure in the wxWidgets version.
//static void CopyLOGFONT(LOGFONT& dest, LOGFONT* src)
//{
//	dest.lfHeight = src->lfHeight;
//	dest.lfWidth = src->lfWidth;
//	dest.lfEscapement = src->lfEscapement;
//	dest.lfOrientation = src->lfOrientation;
//	dest.lfWeight = src->lfWeight;
//	dest.lfItalic = src->lfItalic;
//	dest.lfUnderline = src->lfUnderline;
//	dest.lfStrikeOut = src->lfStrikeOut;
//	dest.lfCharSet = src->lfCharSet;
//	dest.lfOutPrecision = src->lfOutPrecision;
//	dest.lfClipPrecision = src->lfClipPrecision;
//	dest.lfQuality = src->lfQuality;
//	dest.lfPitchAndFamily = src->lfPitchAndFamily;
//	_tcscpy(dest.lfFaceName, src->lfFaceName);
//}

//////////////////////////////////////////////////////////////
/// \return a 32-bit encoded int converted from a wxColour value
/// \param colour	-> the wxColour value to be converted
/// \remarks
/// The 32-bit encoded int is of the type used for MFC's COLORREF.
/////////////////////////////////////////////////////////////
int WxColour2Int(wxColour colour)
{
	// Converts a wxColour value to an (int)COLORREF equivalent
	int colourInt = 0x00000000;
	// MFC's COLORREF value is of the form 0x00bbggrr, where
	// rr is 8 bit value for red
	// gg is the 8 bit value for green, and
	// bb is the 8 bit value for blue
	wxUint32 r, g, b = 0x00000000;
	// Shift the Blue value to the bb position in the 32 bit int
	b = colour.Blue() << 16;
	// Shift the Green value to the gg position in the 32 bit int
	g = colour.Green() << 8;
	// Leave the Red value in the rr position of the 32 bit int
	r = colour.Red();
	// Now OR all three values to build our composit COLORREF 32 bit int
	// and return it.
	return colourInt | b | g | r;
}

//////////////////////////////////////////////////////////////
/// \return the wxColour equivalent to a bit-encoded (COLORREF) int
/// \param colourInt	-> the int in the form of 0x00bbggrr
/// \remarks
/// Converts the 32 bit int as used in a COLORREF to 8 bit RGB values and constructs
/// the wxColor using the RGB values.
/////////////////////////////////////////////////////////////
wxColour Int2wxColour(int colourInt)
{
	// Converts an (int)COLORREF value to a wxColour
	wxUint8 rr,gg,bb;
	wxUint32 shift = 0x00000000;
	shift = colourInt;
	// MFC's COLORREF value is of the form 0x00bbggrr, where
	// rr is 8 bit value for red
	// gg the 8 bit value for green, and
	// bb the 8 bit value for blue
	// Now convert the 32 bit int to 8 bit RGB values:
	// Shift right 8 bits and mask them with 0x000000ff to get the rr value
	rr = shift & 0x000000ff;
	// Shift right another 8 bits and mask them with 0x000000ff to get the gg value
	shift = shift >> 8;
	gg = shift & 0x000000ff;
	// Shift right another 8 bits and mask them with 0x000000ff to get the bb value
	shift = shift >> 8;
	bb = shift & 0x000000ff;
	// Return the wxColour value using wxColour's RGB constructor
	return wxColour(rr, gg, bb);
}

//////////////////////////////////////////////////////////////
/// \return the number represented by a byte string of zeros and ones
/// \param digits	-> the byte string of zeros and ones to be converted
///  helper for converting an up-to-32-digits number string
///  of 0s and 1s as an unsigned integer value
/////////////////////////////////////////////////////////////
unsigned int Btoi(CBString& digits)
{
	int len = digits.GetLength();
	int pos;
	unsigned int n = 0;
	char c;
	unsigned int posValue = 1;
	digits.MakeReverse();
	for (pos = 1; pos <= len; pos++)
	{
		c = digits[pos-1];
		if (c == '1')
			n += posValue;
		// get the next positionValue
		posValue *= 2;
	}
	return n;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return TRUE if the file at path is an XML file (has .xml or .XML extension), otherwise returns FALSE.
/// \param	path			-> the file to be checked
///////////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsXMLfile(wxString path)
{
	path = MakeReverse(path);
	wxString xmlExtn = _T(".xml"); // Adapt It code always uses lower case xml, never XML
	xmlExtn = MakeReverse(xmlExtn);
	int value = wxStrncmp(path,xmlExtn,4); //  _tcsncmp is not available in gcc; use wxStrncmp
	if (value == 0)
		return TRUE;
	else
	{
		// just in case the user renamed the file to have upper case XML, test that too
		xmlExtn = _T(".XML");
		xmlExtn = MakeReverse(xmlExtn);
		value = wxStrncmp(path,xmlExtn,4);
		if (value == 0)
			return TRUE;
	}
	// if we get here, it's neither .XML nor .xml, so return FALSE and the
	// caller should then assume we are dealing with a binary .adt file
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return a byte string (CBString) representing the the book ID of the document at FilePath, or
///			an empty string if no ID code could be found.
/// \param	FilePath	-> the path of the file to be checked
/// \remarks
/// SearchXMLFileContentForBookID searches in the first 1KB of the document's XML file (it contains
/// UTF-8 text, of which ASCII is a subset, and since the book IDs are all English-based alphanumeric
/// strings, we can do the processing using byte-oriented string operations, so CBString can be used
/// for doing the work - it will be byte oriented for regular or unicode compilations. It there is
/// an \id marker and following space, it will be in the m="" attribute, in the first element, and
/// the book ID will occur earlier in the file than \id, in both the s="" attribute and the k="" attribute
/// so we use these facts to perform the extraction of the ID, and we send it back as a byte string to
/// the caller - the caller can then convert to UTF-16 if necessary. We send back an empty string if no
/// ID code could be found.
/// whm 3Sept06 revised for cross-platform wxWidgets use.
/// SearchXMLFileContentForBookID is called only from within the FileContainsBookIndicator function.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
CBString SearchXMLFileContentForBookID(wxString FilePath)
{
	const int BufferLength = 1024;

	CBString returnStr;
	returnStr.Empty();

	char Buffer[BufferLength];
	memset(Buffer,0,BufferLength);
	CBString str;

	int nLength;

	if (::wxFileExists(FilePath))
	{
		wxFile file;
		if (!file.Open(FilePath, wxFile::read))
		{
			// do nothing; just return empty string below
			return returnStr;
		}
		// file is now open, so find its logical length (always in bytes)
		nLength = file.Length();
		if (nLength >= BufferLength)
			nLength = BufferLength-1;
		wxUint32 numRead = file.Read(Buffer,nLength);
		Buffer[numRead] = '\0'; // ensure terminating null

		// remaining code from original MFC function
		// it was a good read, so make the data into a CBString
		str = Buffer;
		// try to find an \id marker plus trailing space in the buffer
		int curPos = str.Find("\\id ");
		if (curPos > 0)
		{
			// there is an \id marker, so get the book code -- we'll look for it in
			// the preceding s="" attribute, which will be the first such attribute in
			// the first S tag in the file.
			int nFound = str.Find("S s=\"");
			if (nFound > 0)
			{
				// nFound is the offset to the S s="  5 character substring, the three bytes following
				// that contain the book ID code -- providing we got the right s attribute instance
				// -- we check for that below
				str = str.Mid(nFound + 5);
				if (nFound < curPos)
				{
					// we found the s attribute earier in the same element, so we got the one
					// which must have the book ID in it
					returnStr = str.Left(3);
				}
			}
		}
		file.Close();
	}
	return returnStr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/// \return                   the extracted substring
/// \param pBufStart      ->  the start of the buffer containing the total string from which to 
///                           extract a substring
/// \param pBufEnd        ->  the end of the buffer containing the total string from which to 
///                           extract a substring (points  at null wxChar terminating the string)
/// \param first          ->  index to the wxChar which is first to be extracted
/// \param firstAfter     ->  index to the wxChar which is one wxChar past the last to be
///                           extracted (at the end, obtaining the last substring to extract,
///                           firstAfter would point at pBufEnd)
/// \remarks   
/// The only tests we make are that first <= last, and last <= pBufEnd
/// Used in GetUpdatedText_UsfmsUnchanged() of CollabUtilities.cpp
//////////////////////////////////////////////////////////////////////////////////////////////////
wxString ExtractSubstring(const wxChar* pBufStart, const wxChar* pBufEnd, size_t first, size_t last)
{
	wxASSERT(first <= last);
	wxASSERT((wxChar*)((size_t)pBufStart + last) <= pBufEnd);
	wxChar* ptr = NULL;
	wxChar* pBufferStart = (wxChar*)pBufStart;
	size_t span = (size_t)(last - first);
	if (pBufferStart + span <= pBufEnd)
	{
		ptr = pBufferStart + (size_t)first;
		return wxString(ptr,span);
	}
	else
	{
		wxString emptyStr = _T("");
		return emptyStr;
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////
/// \return             the extracted substring
/// \param str      ->  the string from which to extract a substring
/// \param first    ->  index to the wxChar which is first to be extracted
/// \param last     ->  index to the wxChar which is last to be extracted
/// \remarks   
/// If str is empty, an empty string is returned
/// If last < first, an assert trips in the debug build, empty string returned in release build
/// If first > (length - 1), an assert trips in the debug build, empty string returned in release build
/// If last = first, an empty string is returned
/// If last > (length - 1) of str, all of the substring from first onwards is returned
//////////////////////////////////////////////////////////////////////////////////////////////////
/* so far it is looking like I won't need this
wxString ExtractSubstring(const wxString& str, int first, int last)
{
	wxString outStr; outStr.Empty();
	if (last > first)
	{
#ifdef __WXDEBUG__
		wxASSERT(FALSE);
#endif
		return outStr;
	}
	int length = str.Len();
	if (first > length -1)
	{
#ifdef __WXDEBUG__
		wxASSERT(FALSE);
#endif
		return outStr;
	}
	if (first == last)
	{
		return outStr;
	}
	if (length == 0)
	{
		return outStr;
	}
	if (last >= length)
	{
		outStr = str.Mid(first);
		return outStr;
	}
	int index;
	for (index = first; index <= last; index++)
	{
		outStr += str.GetChar(index);

	}
	return outStr;
}
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return the index of the first space of one or more spaces which terminate the string, or if there are
///			no spaces at the end, return the length of the string (that is, the position at which appending
///			something would take place); if the string is empty, return wxNOT_FOUND
/// \param	str	-> reference to the wxString
/// \remarks
/// BEW added 17Jan09; handy utility when inserting is wanted prior to any string final spaces.
/// That is, if L is the length of the string, and the string ends with two spaces, then L-2 is returned.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/*int FindLocationBeforeFinalSpaces(wxString& str)
{
	int length = wxNOT_FOUND;
	if (str.IsEmpty())
		return length; // wxNOT_FOUND
	length = (int)str.Len(); // its not empty, so get its length
	wxString strReversed = MakeReverse(str);
	const wxChar* pRev = strReversed.GetData();
	int count = 0;
	// count the initial spaces
	while (pRev[count] == _T(' '))
	{
		count++;
	}
	if (count == 0)
		return length;
	else
		return length - count;
}
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return the index of the first non-space of one or more spaces which begins the string, or if there are
///			no spaces at the beginning, return the zero; if the string is empty, return wxNOT_FOUND
/// \param	str	-> reference to the wxString
/// \remarks
/// BEW added20Jan09; handy utility when inserting is wanted following any string-initial spaces.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/*int FindLocationAfterInitialSpaces(wxString& str)
{
	int count = wxNOT_FOUND;
	if (str.IsEmpty())
		return count; // wxNOT_FOUND
	const wxChar* pStr = str.GetData();
	count = 0;
	// count the initial spaces
	while (pStr[count] == _T(' '))
	{
		count++;
	}
	if (count == 0)
		return 0;
	else
		return count;
}
*/

/* *************************************************************************
*
*	Helpers added by Jonathan Field, 2005
*	BEW ammended 27Oct05 to support unicode application compilation better
*   whm modified 5 April 2006 for wxWidgets version
*
***************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return a string representing the concatenated path parts
/// \param	Bit1		-> the first part of the path to be concatenated
/// \param	Bit2		-> the seconf part of the path to be concatenated
/// \remarks
/// The original MFC function with hard coded "\\" path separator will not work for
/// cross-platform coding because backslash is not used as path separator in
/// Linux or Mac. Instead the wxWidgets version calls the wxFileName::GetPathSeparator()
/// method to concatenate the path parts.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
wxString ConcatenatePathBits(wxString Bit1, wxString Bit2)
{
	return Bit1 + wxFileName::GetPathSeparator() + Bit2;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return TRUE if a file at Path exists, otherwise returns FALSE.
/// \param	Path			-> the file to be checked for existence
/// \remarks
/// Note: The MFC GetFileAttributes API is not cross platform, so this function
/// just calls the wxWidgets ::wxFileExists() global method.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FileExists(wxString Path)
{
	return wxFileExists(Path);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return TRUE if fileAndPathTrueIfNewer has a more recent mod time than fileAndPathFalseIfNewer, 
///             otherwise returns FALSE if fileAndPathFalseIfNewer is same date or newer
/// \param	fileAndPathTrueIfNewer  -> the file being compared, TRUE if this one is newer mod date
/// \param	fileAndPathFalseIfNewer  -> the file being compared, FALSE if this one is newer mod date
/// \remarks
/// Uses methods of wxFilename and wxDataTime to check modification dates of the files. Assumes that
/// the caller has verified that both paths exist before calling.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FileHasNewerModTime(wxString fileAndPathTrueIfNewer, wxString fileAndPathFalseIfNewer)
{
	wxFileName fnTrueIfNewer(fileAndPathTrueIfNewer);
	wxFileName fnFalseIfNewer(fileAndPathFalseIfNewer);
	wxDateTime dtTrueIfNewerAccessTime,dtTrueIfNewerModTime,dtTrueIfNewerCreateTime;
	wxDateTime dtFalseIfNewerAccessTime,dtFalseIfNewerModTime,dtFalseIfNewerCreateTime;
	fnTrueIfNewer.GetTimes(&dtTrueIfNewerAccessTime,&dtTrueIfNewerModTime,&dtTrueIfNewerCreateTime);
	fnFalseIfNewer.GetTimes(&dtFalseIfNewerAccessTime,&dtFalseIfNewerModTime,&dtFalseIfNewerCreateTime);
	// Check if we have a newer version in fileAndPathTrueIfNewer
	return dtTrueIfNewerModTime > dtFalseIfNewerModTime;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return TRUE if a book indicator is found in the file at FilePath.
/// \param	FilePath							-> the file to be checked for a book indicator on an SFM/USFM id tag
/// \param	out_BookIndicatorInSpecifiedFile	<- a string that stores the book indicator if it exists, otherwise empty
/// \remarks
/// The wxWidgets version only supports the examination of XML based Adapt It documents.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FileContainsBookIndicator(wxString FilePath, wxString& out_BookIndicatorInSpecifiedFile)
{
	 // Strategy :
		//Set out_BookIndicatorInSpecifiedFile to nothing.
		//Open the specified file and read through looking for an SFM/USFM id tag.
		//If none found, return false.
		//If any found :
		//  Try to parse it to extract a book indicator.
		//  If successful, store the book indicator in out_BookIndicatorInSpecifiedFile and return true.
		//  Otherwise, return false.

		//  BEW added 27Oct05: the function as Jonathan original coded it works on the binary *.adt file,
		//  and he worked out what is needed by expecting this visually -- hence the explicit search for
		//  the digit 3 (relevant to MFC's binary output structures) -- when this code gets moved to
		//  other operating systems, this will probably break. BEW on 27Oct05 also added code to handle *.xml
		//  document files -- which were not implemented when Jonathan did this work

	out_BookIndicatorInSpecifiedFile.Empty();

	// BEW modified 27Oct05; for support of .xml document files as well as legacy .adt files
	bool bIsXML = IsXMLfile(FilePath);
	if (bIsXML)
	{
		CBString idString = SearchXMLFileContentForBookID(FilePath);
		#ifndef _UNICODE
		out_BookIndicatorInSpecifiedFile = (char*)idString;
		#else
		out_BookIndicatorInSpecifiedFile = gpApp->Convert8to16(idString);
		#endif
		return out_BookIndicatorInSpecifiedFile.IsEmpty() ? false : true;
	}
	// wx version doesn't do binary serialization
	//else
	//{
	//	// we assume an *.adt binary file (NOTE: for non-MFC implimentations, the pattern and mask may need
	//	// to be redefined)
	//	#ifndef _UNICODE
	//	wxInt8 Pattern[] = {'\\', 'i', 'd', ' ', 0, 0, 3, 0, 0, 0}; // BYTE
	//	wxInt8 PatternMask[] = {-1, -1, -1, -1, -1, -1, -1, 0, 0, 0};
	//	#define FIRST_BYTE_INDEX (sizeof Pattern - 3)
	//	#define SECOND_BYTE_INDEX (sizeof Pattern - 2)
	//	#define THIRD_BYTE_INDEX (sizeof Pattern - 1)
	//	#else
	//	wxInt8 Pattern[] = {'\\', 0, 'i', 0, 'd', 0, ' ', 0, 0xFF, 0xFE, 0xFF, 0, 0xFF, 0xFE, 0xFF, 0, 0xFF, 0xFE, 0xFF, 3, 0, 0, 0, 0, 0, 0};
	//	wxInt8 PatternMask[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1};
	//	#define FIRST_BYTE_INDEX (sizeof Pattern - 6)
	//	#define SECOND_BYTE_INDEX (sizeof Pattern - 4)
	//	#define THIRD_BYTE_INDEX (sizeof Pattern - 2)
	//	#endif

	//	if (SearchFileContentForFixedLengthPattern(FilePath, Pattern, PatternMask, sizeof Pattern))
	//	{
	//		char BookIdAs8BitChars[4];
	//		BookIdAs8BitChars[0] = (char)Pattern[FIRST_BYTE_INDEX];
	//		BookIdAs8BitChars[1] = (char)Pattern[SECOND_BYTE_INDEX];
	//		BookIdAs8BitChars[2] = (char)Pattern[THIRD_BYTE_INDEX];
	//		BookIdAs8BitChars[3] = 0; // ASCIIZ null-terminator.
	//		CBString s(BookIdAs8BitChars);

	//		#ifndef _UNICODE
	//		wxString s2 = (char*)s;
	//		#else
	//		wxString s2 = s.Convert8To16();
	//		#endif
	//		if (s2.Length() == 3 &&
	//			IsAnsiLetterOrDigit(s2.GetChar(0)) &&
	//			IsAnsiLetterOrDigit(s2.GetChar(1)) &&
	//			IsAnsiLetterOrDigit(s2.GetChar(2)))
	//		{
	//			out_BookIndicatorInSpecifiedFile = s2;
	//			return true;
	//		}
	//	}
	//	#undef FIRST_BYTE_INDEX
	//	#undef SECOND_BYTE_INDEX
	//	#undef THIRD_BYTE_INDEX
	//}
	return false;
}

/*
// whm note: This is not needed in wx version because it is used only for searches on binary
// serialization files.
// Opens the specified file, and searches through it in small chunks at a time (so that
// the memory requirements are low) looking for the specified pattern.
// Returns true if the pattern is found; false otherwise.
// PatternMask allows a multibyte varying pattern to be found.  PatternMask is a parallel array to Pattern.
// If the entry in PatternMask is non-zero, then the entry in Pattern must be found.  If the entry in PatternMask is
// zero, then any character in the file will match, and if a match is found on the entire pattern, then the 'found'
// characters will be copied into the Pattern array, allowing the caller to determine the variable portion of the pattern.
// In short, this function serves a very niche need and is not likely to be used frequently.
// Assumptions :
//   PatternLength is considerably less than BufferLength.
// whm 3Sept06 revised for cross-platform compatibility with wxWidgets
bool SearchFileContentForFixedLengthPattern(wxString FilePath, wxInt8 *Pattern, wxInt8 *PatternMask, wxUint32 PatternLength)
{
	const int BufferLength = 1024;
	bool rv = false;
	bool Abort = false;
	wxUint8 Buffer[BufferLength];

	int nBytesRead;
	wxUint32 dw;
	wxUint32 dw2;
	wxUint32 nBytesInBuffer;
	if (::wxFileExists(FilePath))
	{
		wxFile file;
		if (!file.Open(FilePath, wxFile::read))
		{
			// do nothing; just return empty string below
			return rv;
		}
		// Loop through the file, reading in chunks, and searching for the specified pattern.
		nBytesInBuffer = 0;
		do
		{
			//                    BufferReceivingBytes    numberOfBytesToRead           numberOfBytesRead
			//if (ReadFile(hFile, Buffer + BytesInBuffer, BufferLength - BytesInBuffer, &BytesRead, NULL))
			nBytesRead = file.Read(Buffer + nBytesInBuffer,BufferLength - nBytesInBuffer);
			if (nBytesRead > 0)
			{
				nBytesInBuffer += nBytesRead;
				if (nBytesInBuffer >= PatternLength) {
					for (dw = 0; dw <= nBytesInBuffer - PatternLength; ++dw) {
						for (dw2 = 0; dw2 < PatternLength; ++dw2) {
							if (PatternMask[dw2] != 0 && Buffer[dw + dw2] != Pattern[dw2])
								break;
						}
						if (dw2 == PatternLength) {
							for (dw2 = 0; dw2 < PatternLength; ++dw2) {
								if (PatternMask[dw2] == 0) {
									Pattern[dw2] = Buffer[dw + dw2];
								}
							}
							rv = true;
							break;
						}
					}
					if (nBytesInBuffer >= PatternLength && !rv) {
						for (dw2 = 0; dw2 < PatternLength - 1; ++dw2) {
							Buffer[dw2] = Buffer[nBytesInBuffer - PatternLength + 1 + dw2];
						}
						nBytesInBuffer = PatternLength - 1;
					}
				}
			}
			else
				Abort = true;
		} while (!rv && !Abort && nBytesRead != 0);
	}
	return rv;

	// Original MFC code below:
	//const int BufferLength = 1024;
	//bool rv = false;
	//bool Abort = false;
	//BYTE Buffer[BufferLength];
	//DWORD BytesRead;
	//DWORD dw;
	//DWORD dw2;
	//DWORD BytesInBuffer;
	//HANDLE hFile;
	//
	//hFile = CreateFile(FilePath, FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	//if (hFile != INVALID_HANDLE_VALUE) {
	//	// Loop through the file, reading in chunks, and searching for the specified pattern.
	//	BytesInBuffer = 0;
	//	do {
	//		if (ReadFile(hFile, Buffer + BytesInBuffer, BufferLength - BytesInBuffer, &BytesRead, NULL)) {
	//			BytesInBuffer += BytesRead;
	//			if (BytesInBuffer >= PatternLength) {
	//				for (dw = 0; dw <= BytesInBuffer - PatternLength; ++dw) {
	//					for (dw2 = 0; dw2 < PatternLength; ++dw2) {
	//						if (PatternMask[dw2] != 0 && Buffer[dw + dw2] != Pattern[dw2])
	//							break;
	//					}
	//					if (dw2 == PatternLength) {
	//						for (dw2 = 0; dw2 < PatternLength; ++dw2) {
	//							if (PatternMask[dw2] == 0) {
	//								Pattern[dw2] = Buffer[dw + dw2];
	//							}
	//						}
	//						rv = true;
	//						break;
	//					}
	//				}
	//				if (BytesInBuffer >= PatternLength && !rv) {
	//					for (dw2 = 0; dw2 < PatternLength - 1; ++dw2) {
	//						Buffer[dw2] = Buffer[BytesInBuffer - PatternLength + 1 + dw2];
	//					}
	//					BytesInBuffer = PatternLength - 1;
	//				}
	//			}
	//		} else Abort = true;
	//	} while (!rv && !Abort && BytesRead != 0);
	//	CloseHandle(hFile);
	//}
	//return rv;
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return TRUE if the character c represents a valid alpha character.
/// \param	c			-> the character to be examined
/// \remarks
/// The wxWidgets function uses wxIsalpha(c) for its internal comparison. wxIsalpha works properly for
/// ANSI or Unicode builds.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsAnsiLetter(wxChar c)
{
	return wxIsalpha(c) != FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return TRUE if the character c represents a valid digit.
/// \param	c			-> the character to be examined
/// \remarks
/// The wxWidgets function uses wxIsdigit(c) for its internal comparison. wxIsdigit works properly for
/// ANSI or Unicode builds.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsAnsiDigit(wxChar c)
{
	return wxIsdigit(c) != 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return TRUE only if s contains at least one character and all characters in s are digits.
///			If s is empty, or contains any whitespace or other non-digit characters, this function
///			returns false.
/// \param	s			-> the string to be examined
/// \remarks
/// The wxWidgets function calls IsAnsiDigit internally, which calls wxIsdigit as it checks the string.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsAnsiDigitsOnly(wxString s)
{
	int i;
	for (i = 0; i < (int)s.Length(); ++i) {
		if (!IsAnsiDigit(s[i])) return false;
	}
	return i > 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return TRUE if the character c represents a valid alphanumeric letter or digit.
/// \param	c			-> the character to be examined
/// \remarks
/// The wxWidgets function uses wxIsalnum(c) for its internal comparison. wxIsalnum works properly for
/// ANSI or Unicode builds.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsAnsiLetterOrDigit(wxChar c)
{
	return wxIsalnum(c) != FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return TRUE if the string s represents a valid filename on the current platform, otherwise FALSE if
///			string s contains forbidden characters.
/// \param	s			-> the string containing a path and/or file name
/// \remarks
/// The original MFC code as written in MFC will not work for wxWidgets and cross-platform code
/// because different platforms have different inventory of valid filename characters.
/// It is better to use the methods of the wxWidgets library, namely wxFileName::GetForbiddenChars()
/// which returns the characters that are not valid on the current system.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsValidFileName(wxString s)
{
	// the wx function GetForbiddenChars() returns a string of characters that are
	// not valid for filenames and directory names in the current platform/os.
	wxString forbiddenChars = wxFileName::GetForbiddenChars();
	if (FindOneOf(s,forbiddenChars) == -1)
		return TRUE;
	else
		return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return a string that contains only the file name without a path.
/// \param	FullPath			-> the string containing a path and file name
/// \remarks
/// Removes the path part of a file name that includes the path.
/// The original MFC code is not suitable for cross-platform code because different platforms have
/// different path and filename conventions.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
wxString StripPath(wxString FullPath)
{
	wxFileName fn(FullPath);
	return fn.GetName();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return a pointer to a new Source Phrase List which has been split off from a main list.
/// \param	MainList			-> the main list of source phrases from which a portion is split off
/// \param	FirstIndexToKeep	-> the index of the first source phrase in MainList marking the end of the split off part
/// \remarks
/// Creates a new Source Phrase List to hold the split-off portion, and returns a pointer to that new List.
/// MainList is modified by having the first FirstIndexToKeep entries removed into the new List.
/// BEW 23Mar10 updated for support of doc version 5 (change needed)
///////////////////////////////////////////////////////////////////////////////////////////////////////////
SPList *SplitOffStartOfList(SPList *MainList, int FirstIndexToKeep)
{
	SPList *rv = new SPList();

	// BEW added 15Aug07, to handle moving final endmarkers to the end of
	// the split off part, when otherwise the splitting would divide
	// a marker .... endmarker matched pair (eg. footnote)
	SPList::Node* pos = MainList->Item(FirstIndexToKeep);
	wxASSERT(pos != NULL);

	// Jonathan's original code block (note: FirstIndexToKeep is only
	// used as a counter so as to determine when to quit the loop, it
	// is not used for accessing list members)
	while (FirstIndexToKeep != 0 && MainList->GetCount() > 0)
	{
		pos = MainList->GetFirst();
		CSourcePhrase* pSrcPhrase = pos->GetData();
		MainList->Erase(pos);
		rv->Append(pSrcPhrase);
		--FirstIndexToKeep;
	}
	return rv;
}

// return the target punctuation string corresponding to input rStr parameter which is a
// source punctuation string this function should work fine without modification, for ANSI
// or UNICODE (UTF-16) builds
// BEW 11Oct10, moved from being a protected member function of the view, to being a
// global helper function -- it's now used in MakeTargetStringIncludingPunctuation(), a view
// function, and in a function, MakeFixedSpaceTranslation(), in XML.cpp; since it draws on
// data in the app class, it is more appropriate as a global function
// BEW changed 4Feb11, if the source punct character was not matched in the source list,
// then we MUST NOT put it in the returned string s, we just fall though and the loop
// iterates through any others to try find matches
wxString GetConvertedPunct(const wxString& rStr)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	wxString s;
	s.Empty();

	if (rStr.IsEmpty())
	{
		return s;
	}

	wxChar ch;
	wxChar doubleCh[2];
	int len = rStr.Length();
	int l;
	bool bFoundDouble;
	int k;
	int i;
	bool bFound;

	for (int j = 0; j < len; j++)
	{
		// first try two-character source punctuation, if no match, 
		// then try single-char matching
		bool bMatchedTwo = FALSE;
		k = j;
		++k;
		if (k >= len)
			goto a; // no room for a 2-char match
		doubleCh[0] = rStr.GetChar(j);
		doubleCh[1] = rStr.GetChar(k);
		bFoundDouble = FALSE;
		for (l = 0; l < MAXTWOPUNCTPAIRS; l++)
		{
			wxChar srcChar[2];
			srcChar[0] = pApp->m_twopunctPairs[l].twocharSrc[0];
			if (srcChar[0] == _T('\0'))
				goto a; // this is a null entry, so there is nothing to search for
			srcChar[1] = pApp->m_twopunctPairs[l].twocharSrc[1];
			if (srcChar[1] == _T('\0'))
			{
				if (srcChar[0] == doubleCh[0])
				{
					// we have matched, but the user only specified a single src punct
					bMatchedTwo = TRUE;
					break;
				}
			}
			else
			{
				if (srcChar[0] == doubleCh[0] && srcChar[1] == doubleCh[1])
				{
					// we have matched a 2-char punct pair
					bMatchedTwo = TRUE;
					break;
				}
			}
		}

		if (bMatchedTwo)
		{
			wxChar chMatched[2];
			chMatched[0] = pApp->m_twopunctPairs[l].twocharTgt[0];
			if (chMatched[0] == _T('\0'))
				goto b; // user must want this pair converted to nothing
			chMatched[1] = pApp->m_twopunctPairs[l].twocharTgt[1];
			if (chMatched[1] == _T('\0'))
			{
				// target punct pair is just a single char, so append it
				s += chMatched[0];
			}
			else
			{
				// target punct pair is a pair, so append them both
				s += chMatched[0];
				s += chMatched[1];
			}
			goto b;
		}

		// try match a single char source punctuation
a:		ch = rStr.GetChar(j);

		bFound = FALSE;
		for (i = 0; i < MAXPUNCTPAIRS; i++)
		{
			wxChar srcChar = pApp->m_punctPairs[i].charSrc;
			if (srcChar == ch)
			{
				// matched source char, so get its corresponding target char
				bFound = TRUE;
				break;
			}
		}

		if (!bFound)
		{
			// if not found, copy original character to the converted punct string
			// BEW changed 4Feb11, if the source punct character was not matched in
			// the source list, then we MUST NOT put it in the returned string s, we just
			// fall though and the loop iterates through any others to try find matches
			//s += ch;
		}
		else
		{
			wxChar chMatched = pApp->m_punctPairs[i].charTgt;
			if (chMatched != (wxChar)0)
			{
				s += chMatched;
			}
		}

b:		if (bMatchedTwo)
			j += 1;
	}

	return s; // if we get here, we got no match, which is an error
}

// functions added by whm
// whm Note: the following group of functions share a lot of code with 
// their counterparts in the Doc class. However, these functions are all 
// used on buffers that might be in existence when the actual Doc does 
// not exist. Hence I've put underscores in their function names so they 
// won't be confused with the similarly named functions in the 
// CAdapt_ItDoc class.
bool Is_AnsiLetter(wxChar c)
{
	return wxIsalpha(c) != FALSE;
}

bool Is_ChapterMarker(wxChar* pChar)
{
	if (*pChar != _T('\\'))
		return FALSE;
	wxChar* ptr = pChar;
	ptr++;
	if (*ptr == _T('c'))
	{
		ptr++;
		return Is_NonEol_WhiteSpace(ptr);
	}
	else
		return FALSE;
}

bool Is_VerseMarker(wxChar *pChar, int& nCount)
{
	if (*pChar != _T('\\'))
		return FALSE;
	wxChar* ptr = pChar;
	ptr++;
	if (*ptr == _T('v'))
	{
		ptr++;
		if (*ptr == _T('n'))
		{
			// must be an Indonesia branch \vn 'verse number' marker
			// if white space follows
			ptr++;
			nCount = 3;
		}
		else
		{
			nCount = 2;
		}
		return Is_NonEol_WhiteSpace(ptr);
	}
	else
		return FALSE;
}

wxString GetStringFromBuffer(const wxChar* ptr, int itemLen)
{
	return wxString(ptr,itemLen);
}

int Parse_Number(wxChar *pChar, wxChar *pEnd) // whm 10Aug11 added wxChar *pEnd parameter
{
	wxChar* ptr = pChar;
	int length = 0;
	// whm 10Aug11 added ptr < pEnd test in while loop below because a 
	// number can be at the end of a string followed by a null character. If the 
	// ptr < pEnd test is not included the Is_NonEol_WhiteSpace() call would 
	// return a FALSE result and the while loop would iterate one too many times 
	// returning a bad length
	while (ptr < pEnd && !Is_NonEol_WhiteSpace(ptr) && *ptr != _T('\n') && *ptr != _T('\r'))
	{
		ptr++;
		length++;
	}
	return length;
}
/* 
// BEW removed 4Aug11, it is not used anywhere yet, but I've also updated it 
// to support the special spaces and joiners recommended by Dennis Drescher
bool Is_WhiteSpace(wxChar *pChar, bool& IsEOLchar)
{
	// The standard white-space characters are the following: space 0x20, tab 0x09, 
	// carriage-return 0x0D, newline 0x0A, vertical tab 0x0B, and form-feed 0x0C.
	// returns true if pChar points to a standard white-space character.
	// We also let the caller know if it is an eol char by returning TRUE in the
	// isEOLchar reference param, or FALSE if it is not an eol char.
	if (*pChar == _T('\r')) // 0x0D CR 
	{
		// this is an eol char so we let the caller know
		IsEOLchar = TRUE;
		return TRUE;
	}
	else if (*pChar == _T('\n')) // 0x0A LF (newline)
	{
		// this is an eol char so we let the caller know
		IsEOLchar = TRUE;
		return TRUE;
	}
	else if (*pChar == _T(' ')) // 0x20 space
	{
		IsEOLchar = FALSE;
		return TRUE;
	}
	else if (*pChar == _T('\t')) // 0x09 tab
	{
		IsEOLchar = FALSE;
		return TRUE;
	}
	else if (*pChar == _T('\v')) // 0x0B vertical tab
	{
		IsEOLchar = FALSE;
		return TRUE;
	}
	else if (*pChar == _T('\f')) // 0x0C FF form-feed
	{
		IsEOLchar = FALSE;
		return TRUE;
	}
	else
	{
		IsEOLchar = FALSE;
		// BEW 4Aug11 changed the code to not test each individually, but just test if
		// wxChar value falls in the range 0x2000 to 0x200D - which is much quicker; and
		// treat U+2060 individually
		wxChar WJ = (wxChar)0x2060; // WJ is "Word Joiner"
		if (*pChar == WJ || ((UInt32)*pChar >= 0x2000 && (UInt32)*pChar <= 0x200D))
		{
			return TRUE;
		}
	}
	return FALSE;
}
*/

/* deprecated, use the version with support for exotic spacers and joiners
bool Is_NonEol_WhiteSpace(wxChar *pChar)
{
#ifdef _UNICODE
	// BEW 29Jul11, support ZWSP (zero-width space character, U+200B) as well, and from
	// Ad Korten's email, also hair space, thin space, and zero width joiner (he may
	// specify one or two others later -- if so add them only here)
	wxChar ZWSP = (wxChar)0x200B; // ZWSP
	wxChar THSP = (wxChar)0x2009; // THSP is "THin SPace
	wxChar HSP = (wxChar)0x200A; // HSP is "Hair SPace" (thinner than THSP)
	wxChar ZWJ = (wxChar)0x200D; // ZWJ is "Zero Width Joiner"
	if (*pChar == ZWSP || *pChar == THSP || *pChar == HSP || *pChar == ZWJ)
		return TRUE;
#endif
	// The standard white-space characters are the following: space 0x20, tab 0x09, 
	// carriage-return 0x0D, newline 0x0A, vertical tab 0x0B, and form-feed 0x0C.
	// returns true if pChar points to a non EOL white-space character, but FALSE
	// if pChar points to an EOL char or any other character.
	if (*pChar == _T(' ')) // 0x20 space
	{
		return TRUE;
	}
	else if (*pChar == _T('\t')) // 0x09 tab
	{
		return TRUE;
	}
	else if (*pChar == _T('\v')) // 0x0B vertical tab
	{
		return TRUE;
	}
	else if (*pChar == _T('\f')) // 0x0C FF form-feed
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
*/
bool Is_NonEol_WhiteSpace(wxChar *pChar)
{
	// The standard white-space characters are the following: space 0x20, tab 0x09, 
	// carriage-return 0x0D, newline 0x0A, vertical tab 0x0B, and form-feed 0x0C.
	// returns true if pChar points to a non EOL white-space character, but FALSE
	// if pChar points to an EOL char or any other character.
#ifdef _UNICODE
	wxChar NBSP = (wxChar)0x00A0; // standard Non-Breaking SPace
#else
	wxChar NBSP = (unsigned char)0xA0;  // standard Non-Breaking SPace
#endif
	if (*pChar == _T(' ')) // 0x20 space
	{
		return TRUE;
	}
	else if (*pChar == _T('\t')) // 0x09 tab
	{
		return TRUE;
	}
	else if (*pChar == _T('\v')) // 0x0B vertical tab
	{
		return TRUE;
	}
	else if (*pChar == _T('\f')) // 0x0C FF form-feed
	{
		return TRUE;
	}
	else if (*pChar == NBSP) // non-breaking space
	{
		return TRUE;
	}
	else
	{
#ifdef _UNICODE
		// BEW 3Aug11, support ZWSP (zero-width space character, U+200B) as well, and from
		// Dennis Drescher's email of 3Aug11, also various others
		// BEW 4Aug11 changed the code to not test each individually, but just test if
		// wxChar value falls in the range 0x2000 to 0x200D - which is much quicker; and
		// treat U+2060 individually
		wxChar WJ = (wxChar)0x2060; // WJ is "Word Joiner"
		if (*pChar == WJ || ((UInt32)*pChar >= 0x2000 && (UInt32)*pChar <= 0x200D))
		{
			return TRUE;
		}
#endif
	}
	return FALSE;
}


int Parse_NonEol_WhiteSpace(wxChar *pChar)
{
	int	length = 0;
	wxChar* ptr = pChar;
	// whm 10Aug11 added. If ptr is pointing at the null at the end of the buffer
	// just return the default zero length for safety sake.
	if (*ptr == _T('\0'))
		return length;
	while (Is_NonEol_WhiteSpace(ptr))
	{
		length++;
		ptr++;
	}
	return length;
}

int Parse_Marker(wxChar *pChar, wxChar *pEnd)
{
    // whm: see note in the Doc's version of ParseMarker for reasons why I've modified
    // this function to add a pointer to the end of the buffer in its signature.
	int len = 0;
	wxChar* ptr = pChar;
	wxChar* pBegin = ptr;
	while (ptr < pEnd && !Is_NonEol_WhiteSpace(ptr) && *ptr != _T('\n') && *ptr != _T('\r') && *ptr != _T('\0') && gpApp->m_forbiddenInMarkers.Find(*ptr) == wxNOT_FOUND)
	{
		if (ptr != pBegin && (*ptr == gSFescapechar || *ptr == _T(']'))) 
			break; 
		ptr++;
		len++;
		if (*(ptr -1) == _T('*')) // whm ammended 17May06 to halt after asterisk (end marker)
			break;
	}
	return len;
}

bool Is_Marker(wxChar *pChar, wxChar *pEnd)
{
	// whm modified 2May11 from the IsMarker function in the Doc to add checks for pEnd
	// to prevent it from accessing memory beyond the buffer. The Doc's version should
	// also have this check, otherwise a spurious '\' at or near the end of the buffer 
	// would result in trying to access a char in memory beyond the actual end of the
	// buffer.
	// 
    // also use bool IsAnsiLetter(wxChar c) for checking character after backslash is an
    // alphabetic one; and in response to issues Bill raised in an email on Jan 31st about
    // spurious marker match positives, make the test smarter so that more which is not a
    // genuine marker gets rejected (and also, use IsMarker() in ParseWord() etc, rather
    // than testing for *ptr == gSFescapechar)
	if (*pChar == gSFescapechar)
	{
		// reject \n but allow the valid USFM markers \nb \nd \nd* \no \no* \ndx \ndx*
		if ((pChar + 1) < pEnd && *(pChar + 1) == _T('n')) // whm added (pChar + 1) < pEnd test
		{
			if ((pChar + 2) < pEnd && IsAnsiLetter(*(pChar + 2))) // whm added (pChar + 2) < pEnd test
			{
				// assume this is one of the allowed USFM characters listed in the above
				// comment
				return TRUE;
			}
			else if ((pChar + 2) < pEnd && (Is_NonEol_WhiteSpace((pChar + 2)) || (*pChar + 2) == _T('\n') || (*pChar + 2) == _T('\r'))) // whm added (pChar + 2) < pEnd test
			{
				// it's an \n escaped linefeed indicator, not an SFM
				return FALSE;
			}
			else
			{
                // the sequence \n followed by some nonalphabetic character nor
                // non-whitespace character is unlikely to be a valid SFM or USFM, so
                // return FALSE here too -- if we later want to make the function more
                // specific, we can put extra tests here
                return FALSE;
			}
		}
		else if ((pChar + 1) < pEnd && !Is_AnsiLetter(*(pChar + 1))) // whm added (pChar + 1) < pEnd test
		{
			return FALSE;
		}
		else
		{
			// after the backslash is an alphabetic character, so assume its a valid marker
			return TRUE;
		}
	}
	else
	{
		// not pointing at a backslash, so it is not a marker
		return FALSE;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// \return	a substring that contains characters in the string that are in charSet, beginning with
///		the first character in the string and ending when a character is found in the inputStr
///		that is not in charSet. If the first character of inputStr is not in the charSet, then
///		it returns an empty string.
/// \param	inputStr	-> the string whose characters are examined
/// \param	charSet		-> a string of characters that may be included in the span of characters returned
/// \remarks
/// The wxWidgets wxString class has no SpanIncluding() method so this function
/// does the equivalent of the CString.SpanIncluding() method
///////////////////////////////////////////////////////////////////////////////////////////////////
wxString SpanIncluding(wxString inputStr, wxString charSet)
{
    // SpanIncluding() returns a substring that contains characters in the string that are
    // in charSet, beginning with the first character in the string and ending when a
    // character is found in the inputStr that is not in charSet. If the first character of
    // inputStr is not in the charSet, then it returns an empty string.
	wxString span = _T("");
	if (charSet.Length() == 0)
		return span;
	for (size_t i = 0; i < inputStr.Length(); i++)
	{
		if (charSet.Find(inputStr[i]) != wxNOT_FOUND)
			span += inputStr[i];
		else
			return span;
	}
	return span;
}

// overload version (slightly different behaviour if charSet is empty), and with the
// additional property that encountering ~ (the USFM fixed space marker) unilaterally
// halts the scan, as also does encountering a backslash, and as also does coming to a
// closing bracket character, ], or a carriage return or linefeed
wxString SpanIncluding(wxChar* ptr, wxChar* pEnd, wxString charSet)
{
    // returns a substring that contains characters in the string that are in charSet,
    // beginning with the character at the passed in ptr location and ending when a
    // character is found in the caller's buffer that is not in charSet. If the first
    // character tested is not in the charSet, then it returns an empty string. If charSet
    // is empty, the only way to stop it other than at pEnd (which could be thousands of
    // characters ahead) would be to stop at some default character type - we'll assume
    // whitespace for that job, or a backslash, as these are the likely useful default
    // halters for our type of data (ie. USFM markup). Pointing at ~ always halts the
    // scan.
	wxString span = _T("");
	if (ptr == pEnd)
		return span;
	if (charSet.Len() == 0)
	{
		// charSet is empty, so use white space and backslash as scan terminator
		while (ptr < pEnd)
		{
			if (!IsWhiteSpace(ptr) && *ptr != _T(']') && *ptr != gSFescapechar && *ptr != _T('~'))
				span += *ptr++;
			else
				return span;
		}
		return span;
	}
	else
	{
		while (ptr < pEnd)
		{
			if (charSet.Find(*ptr) != wxNOT_FOUND && *ptr != _T(']') && *ptr != _T('~')
				&& *ptr != _T('\n') && *ptr != _T('\r'))
				span += *ptr++;
			else
				return span;
		}
	}
	return span;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// \return	all characters preceding the first occurrence of a character from charSet. The character
///				from charSet and all characters following it in the inputStr are not returned.
/// \param	inputStr	-> the string whose characters are examined
/// \param	charSet		-> a string of characters any one of which will preclude adding the remainder of characters
/// \remarks
/// The wxWidgets wxString class has no SpanIncluding() method so this function
/// does the equivalent of the CString.SpanIncluding() method
///////////////////////////////////////////////////////////////////////////////////////////////////
wxString SpanExcluding(wxString inputStr, wxString charSet)
{
	// SpanExcluding() extracts and returns all characters preceding the first
	// occurrence of a character from charSet. The character from charSet and
	// all characters following it in the inputStr are not returned.
	wxString span = _T("");
	if (charSet.Len() == 0)
		return inputStr; // return the original string if charSet is empty
	for (size_t i = 0; i < inputStr.Length(); i++)
	{
		if (charSet.Find(inputStr[i]) == wxNOT_FOUND)
			span += inputStr[i];
		else
			return span;
	}
	return span;
}

// Returns the "word proper", that is, ptr on entry must point to the start of the word
// proper, and the function will do smart parsing using the currently passed in
// punctuation character set (charSet) and do the parsing at the end of the word from past
// the word backwards until it comes to the first character not in the punctuation set
// (and it knows how to parse over any endmarker which may follow the word proper) - then
// it constructs the word and returns it to the caller - throwing away any other info
// gained except that returned in the 3rd param, wordBuildingForPostWordLoc. This param
// will have one or more characters in it only if the following is true:
// (1) each such character was, when the document was first created, a word-building 
// character.
// (2) at some previous time, the user changed the punctuation settings so that that
// character / those characters became punctuation characters, AND
// (3) there was at least one inline binding endmarker following the word (that being so,
// the punctuation status change for those characters will have shifted them to
// immediately follow the inline binding endmarker - where they would be still when this
// function is called)
// (4) prior to calling this function, the user has changed the punctuation settings again
// so that one or more or all of those characters were removed from the punctuation list,
// so that the ones so removed have become word-building characters
// (5) if this function is called and all of 1 to 4 are true, then those characters'
// changed status will be recognised and they'll be stored in the
// wordBuildingForPostWordLoc parameter - the caller can then check for this being
// non-empty and do something with them. (Normally, if 4 has happened, at that time such
// characters will have been moved back to the end of the stem, and so this function
// should, theoretically, never find any such characters not yet moved but needing to be;
// because we don't use it in the reconstitute doc call for rebuilding the document when
// the user changes punctuation settings)
// Rationale for this function....
// The overloaded SpanExluding(wxChar* ptr, ....) function is dangerous, as I only used it
// to parse over a word as far as following punctuation - and that goes belly up if there
// is an embedded punctuation character (which can happen if user changes punct settings)
// so I've deprecated it; and the following ParseWordInwardsFromEitherEnd() replaces it.
// This new function should include a space in the punctuation string because parsing
// inwards we could be parsing over detatched quotes, and some languages require
// punctuation to be offset from the word proper by a space. The function it calls will
// automatically add a space to charSet, so it is safe to pass in spacelessPuncts string.
// BEW created 28Jan11, the code is adapted from the second half of doc class's
// FinishOffConjoinedWordsParse() function
wxString ParseWordInwardsFromEnd(wxChar* ptr, wxChar* pEnd, 
								 wxString& wordBuildingForPostWordLoc, wxString charSet)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	wxChar* p = ptr; // using this function, we expect any punctuation or markers or both
					 // which precede the word proper, have been parsed already, and hence
					 // at entry ptr should be pointing at the first word of the word to be
					 // parsed over and returned to the caller
	wxChar* pHaltLoc = NULL;
	bool bFoundInlineBindingEndMarker = FALSE;
	bool bFoundFixedSpaceMarker = FALSE;
	bool bFoundClosingBracket = FALSE;
	bool bFoundHaltingWhitespace = FALSE;
	int nFixedSpaceOffset = -1;
	int nEndMarkerCount = 0;
	pHaltLoc = pDoc->FindParseHaltLocation( p, pEnd, &bFoundInlineBindingEndMarker, 
					&bFoundFixedSpaceMarker, &bFoundClosingBracket, 
					&bFoundHaltingWhitespace, nFixedSpaceOffset, nEndMarkerCount);
	wxString aSpan(ptr,pHaltLoc); // this could be up to a [ or ], or a 
								  // whitespace or a beginmarker
	// now parse backwards to extract the span's info
	wxString wordProper; // emptied at start of ParseSpanBackwards() call below
	wxString firstFollPuncts; // ditto
	wxString inlineBindingEndMarkers; // ditto
	wxString secondFollPuncts; // ditto
	wxString ignoredWhiteSpaces; // ditto
	pDoc->ParseSpanBackwards( aSpan, wordProper, firstFollPuncts, nEndMarkerCount, 
						inlineBindingEndMarkers, secondFollPuncts, 
						ignoredWhiteSpaces, wordBuildingForPostWordLoc, charSet);
	return wordProper;
}

/* deprecated -- it's dangerous for parsing across a word, see comments for ParseWordInwardsFromEnd() above
// overload version (slightly different behaviour if charSet is empty) and with the
// additional property that encountering ~ (the USFM fixed space marker) unilaterally
// halts the scan, as does encountering a backslash or a carriage return or linefeed
wxString SpanExcluding(wxChar* ptr, wxChar* pEnd, wxString charSet)
{
    // Extracts and returns all characters from the passed in ptr location, and preceding
    // the first occurrence of a character from charSet, or when pEnd is reached, whichever
    // is first. The character from charSet and all characters following it in the caller's
    // buffer are not returned. This overload is useful in our USFM parsing functionality.
    // Note: charSet may be an empty string. If this is the case, we parse until whitespace
    // or pEnd is reached, whichever is first. (space is in charSet, if the latter is
    // non-empty) ~ always halts the scan; so does backslash
	wxString span = _T("");
	if (charSet.Len() == 0)
	{
		while (ptr < pEnd)
		{
			if (!IsWhiteSpace(ptr) && *ptr != _T('~') && *ptr != gSFescapechar)
				span += *ptr++;
			else
				return span;
		}
		return span;
	}
	else
	{
		while (ptr < pEnd)
		{
			if (charSet.Find(*ptr) == wxNOT_FOUND && *ptr != _T('~') && *ptr != gSFescapechar
				&& *ptr != _T('\n') && *ptr != _T('\r'))
				span += *ptr++;
			else
				return span;
		}
	}
	return span;
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// \return	a string whose characters are in reverse order from those in inputStr.
/// \param	inputStr	-> the string whose characters are to be reversed
/// \remarks
/// This algorithm is not speed optimized so it is best used on relatively short strings only.
/// The wxWidgets wxString class has no MakeReverse() method so this function
/// does the equivalent of the CString.MakeReverse() method
///////////////////////////////////////////////////////////////////////////////////////////////////
wxString MakeReverse(wxString inputStr)
{
	if (inputStr.Length() == 0)
		return inputStr;
	wxString reversed = _T("");
	size_t siz = inputStr.Length();
	reversed.Alloc(siz);
	for (size_t i = siz; i > 0; i--)
		reversed += inputStr[i-1];
	return reversed;
}

// return an empty string if there are no SF markers in markers string, else return the
// last of however many are stored in markers (if space follows the marker, the space is
// not returned with the marker); return the empty string if there is no marker present
wxString GetLastMarker(wxString markers)
{
	wxString mkr = _T("");
	wxString backslash = gSFescapechar;
	if (markers.Find(backslash) == wxNOT_FOUND)
		return mkr;
	else
	{
		int offset = wxNOT_FOUND;
		int lastOffset = offset;
		markers.Trim(); // trim whitespace off of end of string
		offset = markers.Find(backslash);
		while (offset != wxNOT_FOUND)
		{
			lastOffset = offset;
			offset = FindFromPos(markers, backslash, offset + 1);
		}
		mkr = markers.Mid(lastOffset);
		mkr = gpApp->GetDocument()->GetWholeMarker(mkr);
		return mkr;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
/// \return	the zero-based index position of the subStr if it exists in inputStr with a search starting
///			at startAtPos; or -1 if the subStr is not found in inputStr from startAtPos to the end of
///			the string.
/// \param	inputStr	-> the string in which the find operation is to be done
/// \param	subStr		-> the sub-string which is being searched for
/// \param	startAtPos	-> the index in inputStr at which the search operation begins
/// \remarks
/// The wxWidgets wxString class has no second parameter in its Find() method, so
/// this function can be used to Find starting at a certain position in the string.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
///////////////////////////////////////////////////////////////////////////////////////////////////
int FindFromPos(const wxString& inputStr, const wxString& subStr, int startAtPos)
{
	// returns the zero based index position of the subStr if it exists in inputStr
	// with a search starting from startAtPos; or -1 if the subStr is not found
	// in inputStr from startAtPos to the end of the string
	int len = inputStr.Length();
	if (len == 0 || startAtPos >= len)
		return -1;
	const wxChar* pBuf = inputStr.GetData();
	wxChar* pBufStart = (wxChar*)pBuf;
	wxChar* pEnd = pBufStart + len;
	wxASSERT(*pEnd == (wxChar)0);
	wxChar* pStartChar = pBufStart + startAtPos;
	int offset = 0;
	int ct;
	while(pStartChar < pEnd)
	{
		if (*pStartChar == subStr.GetChar(0))
		{
			// we're at a char which matches first char of subStr, so look ahead in the buffer and
			// see if remainder of subStr also matches.
			ct = 0;
			while (ct < (int)subStr.Length() && pStartChar < pEnd)
			{
				if (*(pStartChar+ct) != subStr.GetChar(ct))
				{
					break;
				}
				ct++;
			}
			if (ct == (int)subStr.Length())
			{
				// we scanned all of subStr successfully without a mismatch, so return the result
				return startAtPos + offset;
			}
		}
		// if we get to here we've not found a potential match of subStr yet, so continue checking
		offset++;
		pStartChar++;
	}
	// if we get here there are no matches, so return -1
	return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// \return	the zero based index of the first character in inputStr that
///			is also in charSet, or -1 if there is no match.
/// \param	inputStr	-> the string in which the search operation is to be done
/// \param	charSet		-> the sub-string any character of which, if found, will satisfy/end the search
/// \remarks
/// The wxWidgets wxString class has no FindOneOf() method so this function
/// does the equivalent of the CString.FindOneOf() method
///////////////////////////////////////////////////////////////////////////////////////////////////
int FindOneOf(wxString inputStr, wxString charSet)
{
	// returns the zero based index of the first character in inputStr that
	// is also in charSet, or -1 if there is no match
	for (size_t i = 0; i < inputStr.Length(); i++)
	{
		if (charSet.Find(inputStr[i]) != -1)
			return i;
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// \return	a string in which insertStr has been inserted into targetStr at ipos.
/// \param	targetStr	-> the string in which the insertStr is to be inserted
/// \param	ipos		-> the zero-based index of targetStr where insertStr is to be inserted
/// \param	insertStr	-> the string which is to be inserted in the targetStr
/// \remarks
/// The wxString class has no Insert() method so this function
/// does the equivalent of the CString::Insert() and CBString methods.
/// Note: If the ipos index is beyond the range of targetStr; following MFC's and
/// Bruce's CBString::Insert methods, we just concatenate insertStr onto the
/// back end of targetStr. If ipos is negative, no insert is done, an assert is issued,
/// and the function simply returns targetStr.
///////////////////////////////////////////////////////////////////////////////////////////////////
wxString InsertInString(wxString targetStr, int ipos, wxString insertStr)
{
	// wxString doesn't have an Insert method.
	// Since an Insert method is not used much within Adapt It speed is
	// usually not an issue, so this function does it by brute force
	wxString sTemp = _T("");
	if (ipos >= (int)targetStr.Length())
	{
		// the ipos index is beyond the range of the string; following MFC's and
		// Bruce's CBString::Insert methods, we'll just concatenate insertStr onto the
		// back end of str
		return targetStr + insertStr;
	}
	if (ipos < 0)
	{
		// a negative ipos would be an error; safest in this situation
		// to do a debug assert and just return the input string
		wxASSERT(FALSE);
		return targetStr;
	}
	// the general case
	for (int i = 0; i < (int)targetStr.Length(); i++)
	{
		if (i != ipos)
			sTemp += targetStr.GetChar(i);
		else
		{
			// insert insertStr before str[i]
			sTemp += insertStr;
			sTemp += targetStr.GetChar(i);
		}
	}
	return sTemp;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// \return	a wxRect whose coordinates are arranged so that both the height and width are positive.
/// \param	rect		-> the rectangle which is to be normalized
/// \remarks
/// The wxWidgets wxRect has no NormalizeRect method so this function does the same
/// thing that MFC says theirs does.
/// MFC docs say about NormalizeRect: "Normalizes CRect so that both the height and width
/// are positive. The rectangle is normalized for fourth-quadrant positioning, which Windows
/// typically uses for coordinates. NormalizeRect compares the top and bottom values, and
/// swaps them if the top is greater than the bottom. Similarly, it swaps the left and right
/// values if the left is greater than the right. This function is useful when dealing with
/// different mapping modes and inverted rectangles."
///////////////////////////////////////////////////////////////////////////////////////////////////
wxRect NormalizeRect(const wxRect rect)
{
	wxRect inRect = rect;
	if (inRect.GetLeft() > inRect.GetRight())
	{
		int lv = inRect.GetLeft();
		int rv = inRect.GetRight();
		inRect.SetLeft(rv);
		inRect.SetRight(lv);
	}
	if (inRect.GetTop() > inRect.GetBottom())
	{
		int tv = inRect.GetTop();
		int bv = inRect.GetBottom();
		inRect.SetTop(bv);
		inRect.SetBottom(tv);
	}
	return inRect;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// \return	nothing.
/// \param	pCopyFromFont		-> pointer to the font from which the basic attributes are copied
/// \param	pCopyToFont			<- pointer to the font to which the basic attributes are copied
/// \remarks
/// Copies only the basic attributes from pCopyFromFont to pCopyToFont.
/// Attributes that are copied are Encoding, Family, FaceName and Weight.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CopyFontBaseProperties(const wxFont* pCopyFromFont, wxFont*& pCopyToFont)
{
	// This function just copies the base properies (encoding, family, facename,
	// and weight) from copyFromFont to copyToFont
	wxASSERT(pCopyFromFont != NULL);
	wxASSERT(pCopyToFont != NULL);
	pCopyToFont->SetEncoding(pCopyFromFont->GetEncoding());
	pCopyToFont->SetFamily(pCopyFromFont->GetFamily());
	pCopyToFont->SetFaceName(pCopyFromFont->GetFaceName());
	pCopyToFont->SetWeight(pCopyFromFont->GetWeight());
	// we don't copy pFont's style, underlined or point size attributes
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// \return	nothing.
/// \param	pFontCopyFrom		-> pointer to the font from which all attributes are copied
/// \param	pFontCopyTo			<- pointer to the font to which all attributes are copied
/// \remarks
/// Copies all font attributes from pFontCopyFrom to pFontCopyTo. Attributes that are copied
/// are Encoding, Family, FaceName, PointSize, Style, Underlined, and Weight.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CopyAllFontAttributes(const wxFont* pFontCopyFrom, wxFont*& pFontCopyTo)
{
	wxASSERT(pFontCopyTo != NULL);
	wxASSERT(pFontCopyFrom != NULL);
	// copy all font characteristics (available through wxFont methods)
	pFontCopyTo->SetEncoding(pFontCopyFrom->GetEncoding());
	pFontCopyTo->SetFamily(pFontCopyFrom->GetFamily());
	pFontCopyTo->SetFaceName(pFontCopyFrom->GetFaceName());
	pFontCopyTo->SetPointSize(pFontCopyFrom->GetPointSize());
	pFontCopyTo->SetStyle(pFontCopyFrom->GetStyle());
	pFontCopyTo->SetUnderlined(pFontCopyFrom->GetUnderlined());
	pFontCopyTo->SetWeight(pFontCopyFrom->GetWeight());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// \return	the number of significant digits padded to the left in the resubinaryValue string of
///			characters representing the binary number.
/// \param	decimalValue	-> the number which is to be represented as a string in binary notation
/// \param	binaryValue		<- the string value of decimalValue formatted as a binary string
/// \remarks
/// There is no standard C atoi() function as used in MFC, especially one that uses a
/// radix value for representing a binary number as a string. DecimalToBinary() creates
/// the string value of an integer formatted as a binary string.
///////////////////////////////////////////////////////////////////////////////////////////////////
short DecimalToBinary(unsigned long decimalValue, char binaryValue[32])
{
	short index, significant_digits = 0;
	unsigned long ulValue;
	binaryValue[31] = 0;
	for (index = 30; index >= 0; index--)
	{
		ulValue = decimalValue / (1 << index);
		if (ulValue > 0)
		{
			binaryValue[index] = (char)('0' + ulValue);
			decimalValue = decimalValue % (1 << index);
			if (!significant_digits)
				significant_digits = index;
		}
		else
		{
			binaryValue[index] = '0';
		}
	}
	return significant_digits;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if sanity check passes; FALSE otherwise
/// \param      pListBox  -> pointer to a wxListBox, wxComboBox, or wxCheckListBox
/// \remarks
/// Called from: all dialog classes which have a wxListBox, wxComboBox or wxCheckListBox.
/// Ensures that the list/combo box is valid, has at least one item and that at least one item in
/// the list is selected. Returns FALSE if the list/combo box is NULL, or if the box doesn't have
/// any items in it, or if it can't select an item for some reason.
/// Note: When using ListBoxPassesSanityCheck() the pListBox parameter passed in needs to be
/// cast to the name of the actual derived class being used, i.e., (wxListBox*), (wxCheckListBox*)
/// or (wxComboBox*).
////////////////////////////////////////////////////////////////////////////////////////////
bool ListBoxPassesSanityCheck(wxControlWithItems* pListBox)
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear().
	// Also it is sometimes possible that a user could remove a selection from a list box on
	// non-Windows platforms, so ListBoxPassesSanityCheck() does the following to ensure that any
	// wxListBox, wxCheckListBox, or wxComboBox is ready for action:
	// 1. We check to see if pListBox is really there (not NULL).
	// 2. We return FALSE if the control does not have any items in it.
	// 3. We check to ensure that at least one item is selected when the control has at least one item
	//    in it. Item 0 (first item) is selected if no items were selected on entry. In this case, if
	//    item 0 cannot be selected we return FALSE.
	// 4. We return TRUE if pListBox has passed the sanity checks (1, 2, 3 above).

	// Sanity check #1 check to see if pListBox is really there (not NULL)
	if (pListBox == NULL)
	{
		return FALSE;
	}

	// Sanity check #2 check to see if the listbox has any items in it to work with
	if (pListBox->GetCount() == 0)
		return FALSE;

	// If we get here we know there is at least one item in the list. Ensure we have a valid selection.
	// Sanity check #3 check that at least one item in the control is selected/highlighted
	int nSelTemp;
	nSelTemp = pListBox->GetSelection();
	if (nSelTemp == -1)
	{
		// no selection, so select the first item in list
		nSelTemp = 0;
	}
    // According to wxControlWithItems docs, ::SetSelection(int n) "...does not cause any command events
    // to be emitted nor does it deselect any other items in the controls which support multiple
    // selections."
	pListBox->SetSelection(nSelTemp);
	nSelTemp = pListBox->GetSelection();
	if (nSelTemp == -1)
	{
		// cannot get first item selected, so report FALSE to caller
		return FALSE;
	}
	// Sanity checking is done so tell caller that everything is sane.
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if the collected back translation came from the target text line;
///		FALSE if it came from the gloss text line
/// \param      pSrcPhrases  -> pointer to the m_pSourcePhrases list of the document
/// \param	nInitialSequNum	->	sequence number of the first CSourcePhrase instance
///					to be checked for presence of \bt
/// \remarks
/// a helper function for determining if the original collection of back translation information,
/// which started at the CSourcePhrase instance corresponding to nInitialSequNum in the document
/// list pSrcPhrases, was taken from the target text line -- but we check only as far as five
/// consecutive pSrcPhrase instances because we should be able to always detect which is the case
/// by just testing the first one or two instances, so certainly 5 should be plenty. If after five
/// we still don't know, then default to returning TRUE.
/// BEW added 26Oct08
/// BEW 30Mar10, updated for support of doc version 5 (some changes needed)
////////////////////////////////////////////////////////////////////////////////////////////
bool IsCollectionDoneFromTargetTextLine(SPList* pSrcPhrases, int nInitialSequNum)
{
		int nIteratorSN = nInitialSequNum;
		SPList::Node* pos = pSrcPhrases->Item(nIteratorSN);
		wxASSERT(pos != NULL); // we'll assume FindIndex() won't fail,
							   // so just ASSERT for a debug mode check
		CSourcePhrase* pSrcPhrase = pos->GetData(); // this has the \bt marker
		pos = pos->GetNext();
		int offset = -1;
		wxString collectedStr = pSrcPhrase->GetCollectedBackTrans();
		if (collectedStr.IsEmpty())
		{
			// unexpected, so return default TRUE & process with 'verse' assumption
			return TRUE;
		}

		// repurpose the offset local variable
		offset = -1;
		bool bAdaptionInCollectedStr = FALSE;
		bool bGlossInCollectedStr = FALSE;
		int counter = 0;
		do {
			offset = collectedStr.Find(pSrcPhrase->m_adaption);
			if (offset >= 0)
			{
				// m_adaption string was found within the collected text
				bAdaptionInCollectedStr = TRUE;
			}
			offset = -1;
			offset = collectedStr.Find(pSrcPhrase->m_gloss);
			if (offset >= 0)
			{
				// m_glossStr string was found within the collected text
				bGlossInCollectedStr = TRUE;
			}
			if (bAdaptionInCollectedStr && !bGlossInCollectedStr)
			{
				// collection was done from target text line
				return TRUE;
			}
			if (bGlossInCollectedStr && !bAdaptionInCollectedStr)
			{
				// collection was done in gloss line
				return FALSE;
			}
			counter++; // count the iterations

			// situation is still indeterminate (each string was in collectedStr), so
			// iterate to check the m_adaption and m_gloss members of the next
			// pSrcPhrase... first reinitialize the booleans
			bAdaptionInCollectedStr = FALSE;
			bGlossInCollectedStr = FALSE;
			if (pos == NULL)
			{
				// there isn't a "next" pSrcPhrase to check, so a return is forced
				return TRUE; // still unresolved, so default to returning TRUE
			}
			else
			{
				//pSrcPhrase = (CSourcePhrase*)pSrcPhrases->GetNext(pos);
				pSrcPhrase = pos->GetData();
				pos = pos->GetNext();
			}
			offset = -1;
		} while (counter <= 30); // increased value from 5 to 30, 7Sep10 BEW
		// still indeterminate, so default to returning TRUE
		return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////
/// \return     a filename based on the name in the first param but incremented with a possible
///             suffix and a unique ascending set of digits, i.e., fileName001.txt, fileName002.txt, 
///             fileName003.txt etc.
/// \param      baseFilePathAndName  -> path and file name of the base file to "increment" such
///                                     as fileName.txt
/// \param      UniqueFileIncrementMethod  -> an enum that can be incrementViaNextAvailableNumber
///                                         or incrementViaDate_TimeStamp
/// \param	    digitWidth	->	the depth of the incrementing: fileName2.txt had digitWidth of 1,
///                             fileName02.txt has digitWidth of 2, fileName002.txt had digitWidth
///                             of 3, etc
/// \param      suffix   -> any string suffix to be added between the base name and the incrementing
///                         digits, i.e., fileName_Old_002.txt where "_Old_" is the suffix
/// \remarks
/// A helper function for generating a file name using the file name given in baseFilePathAndName.
/// If the file name at baseFilePathAndName does not exist, this function returns the
/// baseFilePathAndName with any suffix suffixed to the file title. If the input baseFilePathAndName 
/// exists, the function increments the name only part of baseFilePathAndName, so that it is unique 
/// from the base file name and any other previously incremented file names created by this function 
/// at the same path. The file name returned preserves any extension that was on the baseFilePathAndName. 
/// If suffix is not a null string, the function adds the suffix to the end of the name followed by an 
/// ascending set of digits at the end of the name before retaining any existing extension.
////////////////////////////////////////////////////////////////////////////////////////////
wxString GetUniqueIncrementedFileName(wxString baseFilePathAndName, enum UniqueFileIncrementMethod,
									  bool bAlwaysModify, int digitWidth, wxString suffix)
{
	wxString PathSeparator = wxFileName::GetPathSeparator();
	wxFileName fn(baseFilePathAndName);
	wxString folderPathOnly = fn.GetPath(); // folder path only of the baseFilePathAndName (not including the file name)
	wxString fileTitleOnly = fn.GetName(); // file title only without dot and extension
	wxString fileTitleAndExtension = fn.GetFullName(); // file title and extension
	wxString fileExtensionOnly = fn.GetExt(); // file extension only
	bool bExists = ::wxFileExists(baseFilePathAndName);
	wxString uniqueName;
	if (bExists || bAlwaysModify)
	{
		if (incrementViaDate_TimeStamp)
		{
			wxString incStr;
			incStr = GetDateTimeNow(adaptItDT);
			incStr.Replace(_T(":"),_T("_"),TRUE); // TRUE - replace all
			incStr.Replace(_T("-"),_T("_"),TRUE); // TRUE - replace all
			uniqueName = folderPathOnly + PathSeparator + fileTitleOnly + suffix + incStr + _T(".") + fileExtensionOnly;
		}
		else // for incrementViaNextAvailableNumber
		{
			int inc = 1;
			wxString incStr;
			incStr.Empty();
			incStr << inc;
			int strLen = incStr.Length();
			while (strLen < digitWidth)
			{
				incStr = _T("0") + incStr; // prefix with zeros
				strLen = incStr.Length();
			}
			uniqueName = folderPathOnly + PathSeparator + fileTitleOnly + suffix + incStr + _T(".") + fileExtensionOnly;
			
			while (::wxFileExists(uniqueName))
			{
				inc++;
				incStr.Empty();
				incStr << inc;
				int strLen = incStr.Length();
				while (strLen < digitWidth)
				{
					incStr = _T("0") + incStr; // prefix with zeros
					strLen = incStr.Length();
				}
				uniqueName = folderPathOnly + PathSeparator + fileTitleOnly + suffix + incStr + _T(".") + fileExtensionOnly;
			}
		}
	}
	else
	{
		// the incoming baseFilePathAndName was already unique in the destination folder, so do not
		// change the filename by adding a suffix or incStr.
		uniqueName = folderPathOnly + PathSeparator + fileTitleOnly + _T(".") + fileExtensionOnly;;
	}
	return uniqueName;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString with any multiple spaces reduced to single spaces
/// \param		rString		-> the string we are reading characters from
/// \remarks
/// Called from: the Doc's OnOpenDocument(); ExportTargetText_For_Collab(),
/// ExportFreeTransText_For_Collab(), DoExportSfmText() and 
/// RebuildText_For_Collaboration().
/// Cleans up rString by reducing multiple spaces to single spaces.
/// whm 11Aug11 revised this function to do its work much faster using both a 
/// read buffer and a write buffer instead of reading each character from the 
/// source buffer and concatenating the character onto a wxString - a process 
/// which can be rather slow and inefficient for lengthy strings. Also I'm 
/// using an algorithm that eliminates the unconditional jump the original
/// function had.
///////////////////////////////////////////////////////////////////////////////
wxString RemoveMultipleSpaces(wxString& rString)
{
	// We are building a destination buffer which will eventually contain a string
	// that is the same size or smaller than the string in the source buffer,
	// i.e., smaller by the number of any multiple spaces that get reduced to single 
	// spaces.
	int nLen = rString.Length();
	wxASSERT(nLen >= 0);
	
	// Set up a write buffer for atemp which is the same size as rString.
	// Then copy characters using pointers from the rString buffer to the atemp buffer (not
	// advancing the pointer for where multiple spaces are adjacent to each other).
	wxString destString;
	destString.Empty();
	// Our copy-to buffer must be writeable so we use GetWriteBuf() for it.
	wxChar* pDestBuffChar = destString.GetWriteBuf(nLen + 1); // pDestBuffChar is the pointer to the write buffer
	wxChar* pEndDestBuff = pDestBuffChar + nLen;
	pEndDestBuff = pEndDestBuff; // avoid compiler warning in release build

	const wxChar* pSourceBuffChar = rString.GetData(); // pSourceBuffChar is the pointer to the read buffer
	wxChar* pEndSourceBuff = (wxChar*)pSourceBuffChar + nLen;
	wxASSERT(*pEndSourceBuff == _T('\0')); // ensure there is a null there
	wxChar* pNextSource;
	while (pSourceBuffChar < pEndSourceBuff)
	{
		if (*pSourceBuffChar == _T(' '))
		{
			pNextSource = (wxChar*)pSourceBuffChar;
			pNextSource++;
			if (pNextSource < pEndSourceBuff && *pNextSource == _T(' '))
			{
				// Advance only the pSourceBuffChar pointer and continue which
				// will bypass writing this space into the destination buffer 
				// because the next character in the source buffer is a space.
				pSourceBuffChar++;
				continue; // skip this space because another one follows this one
			};
		}
		*pDestBuffChar = *pSourceBuffChar;
		// advance both buffer pointers
		pDestBuffChar++;
		pSourceBuffChar++;
	}
	wxASSERT(pSourceBuffChar == pEndSourceBuff);
	wxASSERT(pDestBuffChar <= pEndDestBuff);
	*pDestBuffChar = _T('\0'); // terminate the dest buffer string with null char
	destString.UngetWriteBuf();
	
	return destString;
}

int	sortCompareFunc(const wxString& first, const wxString& second)
{
#ifdef __WXMSW__
	// for Microsoft Windows, we want a caseless compare
	return first.CmpNoCase(second);
#else
	// for Unix (Mac) or Linux, we want a case-sensitive compare
	return first.Cmp(second);
#endif
}

bool IsReadOnlyProtection_LockFile(wxString& filename)
{
	// if the following two substrings are present in filename, where the firstPart is
	// string initial and lastPart is string final, we assume the filename is a read-only
	// protection filename and return TRUE, else return FALSE
	wxString firstPart = _T("~AIROP-");
	wxString lastPart = _T(".lock");
	int offset = filename.Find(firstPart);
	if (offset == 0)
	{
		offset = filename.Find(lastPart);
		if (offset == wxNOT_FOUND)
			return FALSE;
		else
		{
			// return TRUE only if lastPart is filename-final, else return FALSE
			int filenameLength = filename.Len();
			int strLastSubstringLength = lastPart.Len();
			if (offset + strLastSubstringLength == filenameLength)
				return TRUE;
		}
	}
	return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if all went well (and the list has at least one item in it); FALSE
///             if there was an error or the list is empty
/// \param      pathToFolder   ->   absolute path to the folder which is being enumerated
/// \param      pFolders       ->   pointer to a wxArrayString instance which stores the
///                                 enumerated folders (last folder in the path, the rest
///                                 are stripped off)
/// \param      bSort          ->   boolean to indicate whether the list should be sorted
///                                 prior to returning; default is TRUE (to get sort done)
/// \param      bSuppressMessage  -> default FALSE, allows the message for an empty path to
///                                 be displayable; set TRUE to suppress the message
/// \remarks
/// Called from: the AdminMoveOrCopy class, when building a folder's list of folder names.
/// Fills the string array pFolders with the folder names in the passed in folder
/// specified by pathToFolder (it must exist - caller must ensure that). Returns FALSE
/// if there were no folder names obtained from the enumeration; otherwise, returns TRUE
/// indicating there were folders present.
/// The sort operation is option, but defaults to TRUE if not specified. On a MS Windows
/// plaform, a caseless sort is done, on Linux or Unix (or Mac) it is a case-enabled sort.
/// A side effect is to set the current working directory to the folder passed in.
/// So far (January 2010) we use the bSuppressMessage = TRUE on in order to suppress the
/// empty path warning during initialization of the AdminMoveOrCopy class, where we want
/// the destination folder path to be empty explicitly when the dialog is first opened.
/// For this we use a CAdapt_ItApp::m_bAdminMoveOrCopyIsInitializing member variable which
/// is set TRUE at the start of initialization, cleared to FALSE when initialization is
/// done, and is FALSE at all other times.
////////////////////////////////////////////////////////////////////////////////////////
bool GetFoldersOnly(wxString& pathToFolder, wxArrayString* pFolders, bool bSort,
					bool bSuppressMessage)
{
	bool bOK = TRUE;
	wxDir dir;
	if (pathToFolder.IsEmpty() )
	{
		if (!bSuppressMessage)
		{
			wxMessageBox(_(
"No path to a folder is defined. Perhaps you cancelled the dialog for setting the destination folder."),
			_("Error, empty path specification"), wxICON_WARNING);
		}
		return FALSE;
	}
	else
	{
		// wxDir must call .Open() before enumerating files
		bOK = (::wxSetWorkingDirectory(pathToFolder) && dir.Open(pathToFolder));
		if (!bOK)
		{
			// unlikely to fail, but just in case...
			if (!bSuppressMessage)
			{
				wxString msg;
				msg = msg.Format(
_("Failed to make the directory  %s  the current working directory prior to getting the directory's child directories; perhaps try again."),
				pathToFolder.c_str());
				wxMessageBox(msg, _("Error, no working directory"), wxICON_WARNING);
			}
			return FALSE;
		}
		else
		{
			wxString str = _T("");
			bool bWorking = dir.GetFirst(&str,wxEmptyString,wxDIR_DIRS); // get directories only
			// whm note: wxDIR_FILES finds only files; it ignores directories, and . and ..
			// to include directories, OR with wxDIR_DIRS
			// to include hidden files, OR with wxDIR_HIDDEN
			// to include . and .. OR with wxDIR_DOTDOT
			// For our Adapt It purposes, we don't use hidden files, so we won't
			// show any such to the user - but moving or copying folders will move or
			// copy any contained hidden files
			while (bWorking)
			{
				if (str.IsEmpty())
					continue;
				// check the directory actually exists, this should always succeed
				bool bExists = dir.Exists(str);
				if (!bExists)
				{
					// this error is highly unlikely to ever occur, so an English-only message
					// will do here
					wxString msg;
					msg = msg.Format(_T(
						"The directory %s was detected but testing for its existence failed. You probably should try again."), str.c_str());
					wxMessageBox(msg, _("Error, not a directory"), wxICON_ERROR);
					return FALSE;
				}
				wxASSERT(!str.IsEmpty());
				pFolders->Add(str); // add the folder name to the list (makes a copy of str)

				// try find the next one
				bWorking = dir.GetNext(&str);
			}

			// if the list is still empty, return FALSE
			if (pFolders->IsEmpty())
				return FALSE;

			// do the sort of the list of folder names, if requested
			if (bSort)
			{
				pFolders->Sort(sortCompareFunc);
			}
		}
	}
	return TRUE;
}

// BEW added 22Jan10: string tokenization is a pain in the butt in wxWidgets, because the
// developers do not try to give uniform behaviours for CR versus CR+LF across all
// platforms (eg. multiline text controls), so a function is needed for tokenizing which
// allows the options we want. The following is it. wxStringTokenizer gets close, but I
// want something which will permit the options and collect the resulting strings into a
// wxArrayString passed in by reference. Parameters: delimiters is the set of allowed
// delimiters, such as _T(":\n\r") (colon, newline, carriagereturn), str is the string we
// want to tokenize, array is the array in which we return the tokenized strings, and
// bStoreEmptyStringsToo allows us to control whether empty strings get appended to array
// or not, default is to append them. Returns the count of the strings stored in array.
//
// Note: it internally trim spaces from the end tokenized strings, and really I've made
// this function for getting logical lines from a multiline wxTextCtrl, so don't make space
// or tab one of the delimiters if what you want is logical lines - otherwise it will
// return an array of words rather than an array of lines
long SmartTokenize(wxString& delimiters, wxString& str, wxArrayString& array,
					  bool bStoreEmptyStringsToo)
{
	wxString aToken;
	array.Empty();
	wxStringTokenizerMode mode = wxTOKEN_RET_EMPTY_ALL;
	wxStringTokenizer tokenizer(str,delimiters,mode);
	while (tokenizer.HasMoreTokens())
	{
		aToken = tokenizer.GetNextToken();
		aToken.Trim(FALSE); // FALSE means trim white space from left end
		aToken.Trim(); // default TRUE means trim white space from right end
		if (aToken.IsEmpty())
		{
			if (bStoreEmptyStringsToo)
				array.Add(aToken);
		}
		else
		{
			array.Add(aToken);
		}
	}
	return (long)array.GetCount();
}

// BEW created 11Oct10, this utility parses strToParse, extracting in sequence each SF
// marker and ignoring any other non-marker substrings. It works whether or not there is a
// space between markers. Returns TRUE if at least one was added to arr, otherwise returns
// FALSE. The passed in string array, arr, is cleared on entry. 
// A typical use for this utility is to get an array of endmarkers, which using
// AddNewStringsToArray(), can be added to m_pMedialMarkers member of a merged
// CSourcePhrase. (SmartTokenize() above won't work when there are no delimiters between
// the markers, such as a sequence of endmarkers - these are stored without spaces.)
bool GetSFMarkersAsArray(wxString& strToParse, wxArrayString& arr)
{
	arr.Clear();
	bool bGotSomething = FALSE;
	const wxChar* pBuff = strToParse.GetData();
	wxChar* ptr;
	wxChar* pStart = (wxChar*)pBuff;
	int itemLen = 0;
	wxString srchStr = gSFescapechar;
	int offset = FindFromPos(strToParse, srchStr, 0);
	while (offset != wxNOT_FOUND)
	{
		bGotSomething = TRUE;
		ptr = pStart + offset;
		// this version of ParseMarker() is in helpers.cpp
		itemLen = ParseMarker(ptr); // internally has a check for end-of-buffer zero
		wxString mkr(ptr, itemLen);
		wxASSERT(itemLen > 1);
		arr.Add(mkr);
		// advance offset beyond start of mkr and try for next one
		offset++;
		offset = FindFromPos(strToParse, srchStr, offset);
	}
	return bGotSomething;
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if all went well (and the list has at least one item in it); FALSE
///             if there was an error or the list is empty
/// \param      pathToFolder   ->   absolute path to the folder which is being enumerated
/// \param      pFiles         ->   pointer to a wxArrayString instance which stores the
///                                 enumerated files (filenames, path info stripped off)
/// \param      bSort          ->   boolean to indicate whether the list should be sorted
///                                 prior to returning; default is TRUE (to get sort done)
/// \remarks
/// Called from: the AdminMoveOrCopy class, when building a folder's list of filenames.
/// Fills the string array pFiles with the filenames (including extension) in the folder
/// specified by pathToFolder (it must exist - caller must ensure that). Returns FALSE
/// if there were no filenames obtained from the folder (we ignore hidden files and also
/// "." and ".."); otherwise, returns TRUE indicating there were files present.
/// The sort operation is option, but defaults to TRUE if not specified. On a MS Windows
/// plaform, a caseless sort is done, on Linux or Unix (or Mac) it is a case-enabled sort.
/// A side effect is to set the current working directory to the folder passed in.
////////////////////////////////////////////////////////////////////////////////////////
bool GetFilesOnly(wxString& pathToFolder, wxArrayString* pFiles, bool bSort,
				  bool bSuppressMessage)
{
	if (pathToFolder.IsEmpty())
	{
		// return silently, GetFoldersOnly() will already have put up the appropriate
		// message
		return FALSE;
	}
	wxDir dir;
	// wxDir must call .Open() before enumerating files
	bool bOK = (::wxSetWorkingDirectory(pathToFolder) && dir.Open(pathToFolder));
	if (!bOK)
	{
		// unlikely to fail, but just in case...
		if (!bSuppressMessage)
		{
			wxString msg;
			msg = msg.Format(
_("Failed to make the directory  %s  the current working directory prior to getting the folder's files; perhaps try again."),
			pathToFolder.c_str());
			wxMessageBox(msg, _("Error, no working directory"), wxICON_WARNING);
		}
		return FALSE;
	}
	else
	{
		wxString str = _T("");
		wxString filename = _T("");
		bool bWorking = dir.GetFirst(&str,wxEmptyString,wxDIR_FILES);
		// whm note: wxDIR_FILES finds only files; it ignores directories, and . and ..
		// to include directories, OR with wxDIR_DIRS
		// to include hidden files, OR with wxDIR_HIDDEN
		// to include . and .. OR with wxDIR_DOTDOT
		// For our Adapt It purposes, we don't use hidden files, so we won't
		// show any such to the user - but moving folders will move any
		// contained hidden files
		while (bWorking)
		{
			if (str.IsEmpty())
				continue;
			wxFileName fn;
			fn.Assign(str); // assign the full path to the wxFileName object
            // for wxPathFormat format we use the value wxPATH_NATIVE on the assumption
            // that even when accessing a foreign machine's different OS, the path
            // separator returned on the local machine is the native one (if this
            // assumption proves false, I'll have to make checks and do conditional blocks
            // for making calls using wxPath_UNIX (for Linux or Mac) versus wxPath_WIN

            // get the filename part (including extension), we don't care about rest of
            // path and so it's thrown away in the next call; also don't add to the array
            // any read-only protection file encountered
			filename = fn.GetFullName();
			if (!IsReadOnlyProtection_LockFile(filename))
			{
				pFiles->Add(filename); // add the filename to the list
			}

			// try find the next one
			bWorking = dir.GetNext(&str);
		}

		// if the list is still empty, return FALSE
		if (pFiles->IsEmpty())
			return FALSE;

		// do the sort of the list of filenames, if requested
		if (bSort)
		{
			pFiles->Sort(sortCompareFunc);
		}
	}
	return TRUE;
}

wxString ChangeHyphensToUnderscores(wxString& name)
{
	// filter out any hyphens, return the resulting string
	wxString newName = _T("");
	size_t length = name.Len();
	size_t index;
	wxChar chr;
	for (index = 0; index < length; index++)
	{
		chr = name.GetChar(index);
		if (chr != _T('-'))
		{
			newName += chr;
		}
		else
		{
			newName += _T('_');
		}
	}
	return newName;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return         TRUE if there is a \free marker in m_markers of pSrcPhrase, but with
///			        no content; else returns FALSE (and it also returns FALSE if there
///                 is no \free marker present)
///
///	\param pSrcPhrase	->	pointer to the CSourcePhrase instance whose m_markers
///					        member may or may not contain a free translation (filtered)
/// \remarks
///    Used by the CCell.cpp Draw() function to control the colouring of the "green" wedge;
///    if a free translation field is empty, the wedge will display khaki, if a
///    backtranslation field is empty it will display a pastel blue, if both are empty it
///    will display red -- the alternate colouring idea was suggested by John Nystrom in
///    order to give the user visual feedback about when a \free or \bt field has no
///    content, so he can enter it manually or by other means; a companion function to this
///    present one is IsBackTranslationContentEmpty(), which works similarly but for a \bt
///    or \bt derived marker's field.
///    BEW 19Feb10, updated for doc version 5, and also moved it to helpers.cpp
/////////////////////////////////////////////////////////////////////////////////
bool IsFreeTranslationContentEmpty(CSourcePhrase* pSrcPhrase)
{
	return pSrcPhrase->GetFreeTrans().IsEmpty();
}

/////////////////////////////////////////////////////////////////////////////////
/// \return         TRUE if there is a \bt or \bt derivative marker in m_markers of
///			        pSrcPhrase, but with no content, else returns FALSE (and it also
///			        returns FALSE if there is no \bt or \bt derivative marker present)
///
/// Parameters:
///	pSrcPhrase	->	pointer to the CSourcePhrase instance whose m_markers member
///					may or may not contain a back translation (filtered)
///
/// Remarks:
///    Used by the CCell.cpp Draw() function to control the colouring of the "green" wedge;
///    if a free translation field is empty, the wedge will display khaki, if a
///    backtranslation field is empty it will display a pastel blue, if both are empty it
///    will display red -- the alternate colouring idea was suggested by John Nystrom in
///    order to give the user visual feedback about when a \free or \bt field has no
///    content, so he can enter it manually or by other means; a companion function to this
///    present one is IsFreeTranslationContentEmpty(), which works similarly but for a
///    \free field.
///    BEW 19Feb10, moved from view class, and updated to support doc version 5, and
///    the colours used and when they are used is changed a little, aqua is added too
/////////////////////////////////////////////////////////////////////////////////
bool IsBackTranslationContentEmpty(CSourcePhrase* pSrcPhrase)
{
	return pSrcPhrase->GetCollectedBackTrans().IsEmpty();
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return                 a string with the filtered information and wrapped by
///                         appropriate markers but not any \~FILTER or \~FILTER* wrappers
/// \param  pSrcPhrase  ->  the CSourcePhrase instance (it could be a placeholder) from
///                         which we want to extract the filtered information as a string,
///                         and we include free translations, etc as 'filtered info' here
/// \param  bDoCount    ->  count words, putting them between |@ and @| delimiters if
///                         TRUE, but skip doing this if FALSE (RTF interlinear does not
///                         need the count done, for instance)
/// \param  bCountInTargetText ->   TRUE for target text word counting, FALSE for source
///                                 text word counting
/// \param  bIncludeNote -> default TRUE; set FALSE when note is not to be returned in the
///                         string along with the other filtered stuff (as when input is
///                         a placeholder, which won't as of 11Oct10 have had a note transferred
///                         to it - even if right association happened)
/// \remarks
/// Called solely (3 different locations) in RebuildSourceText(), for exporting src text.
/// Gets any content in m_collectedBackTrans, m_freeTrans (and for those, adds \bt, \free
/// and \free* markers for any such info which is not empty) and then appends any info from
/// m_filteredInfo (after removing filter bracket markers), adds a final space, and sends
/// the result back to the caller as a wxString.
/// BEW created 6Apr10 in order to simplify getting the filtered info as it would appear
/// in an export of, say, source text or other text lines - since for doc version 5
/// filtered info is stored in several disparate members, rather than just in m_markers
/// along with unfiltered info. Also gets a note and adds \note & \note* unless the boolean
/// param specifies it not be supplied. (See 11Oct10 changes described just below.)
/// BEW 8Sep10, altered order of export of filtered stuff, it's now 1. filtered info, 2.
/// collected back trans, 3. note, 4 free translation. (This fits better with OXES export)
/// BEW 11Oct10, I've decided a note should not be moved to or from a placeholder. That
/// has implications for this function. Two of the three locations want the note handled,
/// but the location where input is a placeholder, we don't want a note handled. So I've
/// added a further boolean param, bool bIncludeNote = TRUE in the prototype.
//////////////////////////////////////////////////////////////////////////////////////////
wxString GetFilteredStuffAsUnfiltered(CSourcePhrase* pSrcPhrase, bool bDoCount, 
									  bool bCountInTargetText, bool bIncludeNote)
{
	wxString str; str.Empty();
	SPList* pSrcPhrases = gpApp->m_pSourcePhrases;
	wxASSERT(pSrcPhrases->GetCount() > 0);
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();

	// markers needed, since doc version 5 doesn't store some filtered
	// stuff using them
	wxString freeMkr(_T("\\free"));
	wxString freeEndMkr = freeMkr + _T("*");
	wxString noteMkr(_T("\\note"));
	wxString noteEndMkr = noteMkr + _T("*");
	wxString backTransMkr(_T("\\bt"));
	wxString aSpace = _T(" ");

	// scratch strings, in wxWidgets these local string objects start off empty
	wxString markersStr;
	wxString endMarkersStr;
	wxString freeTransStr;
	wxString noteStr;
	wxString collBackTransStr;
	wxString filteredInfoStr;

	// get the other string information we want, putting it in the
	// scratch strings
	GetMarkersAndFilteredStrings(pSrcPhrase, markersStr, endMarkersStr,
					freeTransStr, noteStr, collBackTransStr, filteredInfoStr);
	// remove any filter bracketing markers if filteredInfoStr has content
	if (!filteredInfoStr.IsEmpty())
	{
		filteredInfoStr = pDoc->RemoveAnyFilterBracketsFromString(filteredInfoStr);
	}
	// make filteredInfoStr information now come first
	if (!filteredInfoStr.IsEmpty())
	{
		// this data has any markers and endmarkers already 'in place'
		str.Trim();
		str += aSpace + filteredInfoStr;
	}
	if (!collBackTransStr.IsEmpty())
	{
		// add the marker too
		str.Trim();
		str += backTransMkr;
		str += aSpace + collBackTransStr;
	}
	// the CSourcePhrase instance which stores a free translation section is always the
	// anchor one, so we can reliably call CountWordsInFreeTranslationSection() below
	if (!freeTransStr.IsEmpty() || pSrcPhrase->m_bStartFreeTrans)
	{
		str.Trim();
		str += aSpace + freeMkr;
		if (bDoCount)
		{
			// BEW addition 06Oct05; a \free .... \free* section pertains to a
			// certain number of consecutive sourcephrases starting at this one if
			// m_freeTrans has content, but the knowledge of how many
			// sourcephrases is marked in the latter instances by which ones have
			// the m_bStartFreeTrans == TRUE and m_bEndFreeTrans == TRUE, and if we
			// just export the filtered free translation content we will lose all
			// information about its extent in the document. So we have to compute
			// how many target words are involved in the section, and store that
			// count in the exported file -- and the obvious place to do it is
			// after the \free marker and its following space. We will store it as
			// follows: |@nnnn@|<space> so that we can search for the number and
			// find it quickly and remove it if we later import the exported file
			// into a project as source text.
			// (Note: the following call has to do its word counting in the SPList,
			// because only there is the filtered information, if any, still hidden
			// and therefore unable to mess up the word count.)
			int nWordCount = CountWordsInFreeTranslationSection(bCountInTargetText,
											pSrcPhrases, pSrcPhrase->m_nSequNumber);
			// construct an easily findable unique string containing the number
			wxString entry = _T("|@");
			entry << nWordCount; // converts int to string automatically
			entry << _T("@| ");
			// append it after a delimiting space
			str += aSpace + entry;
		}
		if (freeTransStr.IsEmpty())
		{
			// we need to support empty free translation sections
			str += aSpace + freeEndMkr;
		}
		else
		{
			// now the free translation string itself & endmarker
			str += aSpace + freeTransStr;
			str += freeEndMkr; // don't need space too
		}
	}
	// notes being after free trans means that OXES parsing is easier, as all notes -
	// including one at the free translation anchor point, then become 'embedded' in the
	// sacred text - though the one at the anchor point is 'embedded' at the start of the
	// sacred  text, it's not stretching things to far to consider it embedded like any
	// others in that section of text
	if (bIncludeNote)
	{
		if (!noteStr.IsEmpty() || pSrcPhrase->m_bHasNote)
		{
			str.Trim();
			str += aSpace + noteMkr;
			if (noteStr.IsEmpty())
			{
				// although other parts of the app don't as yet support empty notes, we'll do
				// so here
				str += aSpace + noteEndMkr;
			}
			else
			{
				str += aSpace + noteStr;
				str += noteEndMkr; // don't need space too
			}
		}
	}
	// moved the block for export of filteredInfo from here to at start of the blocks above
	str.Trim(FALSE); // finally, remove any LHS whitespace
	// ensure it ends with a space
	str.Trim();
	str += aSpace;
	return str;
}

void GetMarkersAndFilteredStrings(CSourcePhrase* pSrcPhrase,
								  wxString& markersStr,
								  wxString& endMarkersStr,
								  wxString& freeTransStr,
								  wxString& noteStr,
								  wxString& collBackTransStr,
								  wxString& filteredInfoStr)
{
	markersStr = pSrcPhrase->m_markers;
	endMarkersStr = pSrcPhrase->GetEndMarkers();
	freeTransStr = pSrcPhrase->GetFreeTrans();
	noteStr = pSrcPhrase->GetNote();
	collBackTransStr = pSrcPhrase->GetCollectedBackTrans();
	filteredInfoStr = pSrcPhrase->GetFilteredInfo();
}

void EmptyMarkersAndFilteredStrings(
								  wxString& markersStr,
								  wxString& endMarkersStr,
								  wxString& freeTransStr,
								  wxString& noteStr,
								  wxString& collBackTransStr,
								  wxString& filteredInfoStr)
{
	markersStr.Empty();
	endMarkersStr.Empty();
	freeTransStr.Empty();
	noteStr.Empty();
	collBackTransStr.Empty();
	filteredInfoStr.Empty();
}

///////////////////////////////////////////////////////////////////////////////////////
/// \return                     the modified value of the passed in Tstr
/// \param  pMergedSrcPhrase -> a merger, which may have non-empty m_pMedialMarkers member
/// \param  Tstr             -> the string into which there might need to be placed
///                             markers and endmarkers, possibly a placement dialog
///                             invoked on it as well; it's pMergedSrcPhrase->m_targetStr
///                             from the caller
/// \param  bDoCount        ->  whether or not to do a word count of the free trans section
/// \param  bCountInTargetText -> do the count using target text words if TRUE, else
///                             source test words
/// \remarks
/// This function is used in the preparation of source text data (Sstr), and editable data
/// (Tstr), and if necessary it will call the PlaceInternalMarkers class to place medial
/// markers into the editable string. It is used in RebuildTargetText() to compose
/// non-editable and editable strings, these are source text and target text, from a merger
/// (in the case of glosses, the glass would have been added after the merger since
/// glossing mode does not permit mergers). It's also used in doc's
/// ReconstituteAfterPunctuationChange().
/// Mergers are illegal across filtered info, such info may only occur on the first
/// CSourcePhrase in a merger, and so we know there cannot be any medial filtered
/// information to be dealt with - we only have to consider m_markers and m_endMarkers.
/// Also, since a placement dialog doesn't need to do placement in any of the stored
/// initial filtered information, if present, we separate that information in markersPrefix
/// and don't present it to the user if the dialog is made visible, but just add it in
/// after the dialog is dismissed. Note: we only produce Sstr in order to support a
/// referential source text in the top edit box of the placement dialog - which won't be
/// called if there are no medial markers stored in the merged src phrase. The modified
/// Tstr is what we return.
/// BEW 1Apr10, written for support of doc version 5
/// BEW 8Sep10, altered order of export of filtered stuff, it's now 1. filtered info, 2.
/// collected back trans, 3. free translation, 4. note (This fits better with OXES export)
/// BEW 17Sep10, added a character count, wrapped by @# and #@ at start of note text, so
/// that notes on a merger will carry with them a count of the characters in the adaptation
/// phrase - this is done only for the special case of an sfm export for use in building
/// oxes xml; also, we'll pass the actual phrase on to oxes in the format @#nnn:phrase#@
/// BEW 11Oct10, extensively refactored so as to deal with the richer document model, v5,
/// in which there is m_follOuterPunct, and four wxString members for inline markers and
/// endmarkers, binding versus non-binding
/// BEW 13Dec10, added support for [ and ] brackets. When Tstr passed in contains just [
/// or ] we return Tstr immediately. (No word counting is done, because in exports in a
/// later function which the caller calls we join the brackets to the text which they
/// bracket and so they don't contribute to the word count)
/////////////////////////////////////////////////////////////////////////////////////////
wxString FromMergerMakeTstr(CSourcePhrase* pMergedSrcPhrase, wxString Tstr, bool bDoCount, 
							bool bCountInTargetText)
{
	// to support [ and ] brackets which, if they occur, are the only content, bleed this
	// possibility out here, because the code below is not designed to support such a
	// situation
	if (Tstr == _T("["))
	{
		return Tstr;
	}
	if (Tstr == _T("]"))
	{
		return Tstr;
	}

	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	SPList* pSrcPhrases = gpApp->m_pSourcePhrases;
	wxASSERT(pMergedSrcPhrase->m_pSavedWords->GetCount() > 1); // must be a genuine merger
	// and the caller tests that it is not word1~word2 conjoined by fixed space -
	// the latter is stored as a pseudo merger, and is handled within FromSingleMakeTstr()
	SPList* pSrcPhraseSublist = pMergedSrcPhrase->m_pSavedWords;
	SPList::Node* pos = pSrcPhraseSublist->GetFirst();
	wxASSERT(pos != 0);
	SPList::Node* posLast = pSrcPhraseSublist->GetLast();
	wxASSERT(posLast != 0);
	bool bHasInternalMarkers = FALSE;
	bool bFirst = TRUE;
	bool bLast = FALSE;
	//bool bNonFinalEndmarkers = FALSE;

	wxArrayString markersToPlaceArray;
	wxArrayString markersAtVeryEndArray; // for endmarkers on pMergedSrcPhrase

    // store here any string of filtered information stored on pMergedSrcPhrase (in any or
    // all of m_freeTrans, m_note, m_collectedBackTrans, m_filteredInfo) which is on the
    // first CSourcePhrase instance
	wxString markersPrefix; markersPrefix.Empty();
	wxString Sstr; // Sstr needed only if internally we must use the placement
				   // dialog; we don't need to return it to the caller

	// markers needed, since doc version 5 may store some filtered stuff without using them
	wxString freeMkr(_T("\\free"));
	wxString freeEndMkr = freeMkr + _T("*");
	wxString noteMkr(_T("\\note"));
	wxString noteEndMkr = noteMkr + _T("*");
	wxString backTransMkr(_T("\\bt"));
	markersPrefix.Empty(); // clear it out -- this is where we accumulate a series of
		// mkr+content+/-endmkr substrings, to form a prefix for inserting before the
		// visible content stored on this merged CSourcePhrase
	Sstr.Empty(); // clear it out

	wxString finalSuffixStr; finalSuffixStr.Empty(); // put collected-string-final 
													 // m_endMarkers content here
	bool bFinalEndmarkers = FALSE; // set TRUE when finalSuffixStr has content 
								   // to be added at loop end
	wxString aSpace = _T(" ");
	wxString markersStr;
	wxString endMarkersStr;
	wxString freeTransStr;
	wxString noteStr;
	wxString collBackTransStr;
	wxString filteredInfoStr;
	// it's helpful to keep the various inline markers as encountered, so we can easily
	// put this in the correct place within Sstr (where their location is determinate)
	wxString nBEMkrs; // for inline non-binding endmarkers
	wxString iNBMkrs; // for inline non-binding (begin)markers
	wxString bEMkrs; // for inline binding endmarkers
	wxString iBMkrs; // for inline binding (begin)markers (for first CSourcePhrase in
					 // the loop which traverses the original ones)
	wxString xrefStr; // for \x* .... \x* cross reference info (it is stored preceding
					  // m_markers content, but other filtered info goes before m_markers
	wxString otherFiltered; // leftovers after \x ...\x* is removed from filtered info

	// the first one, pMergedSrcPhrase as passed in, has to provide the stuff for the
	// markersPrefix wxString...
	GetMarkersAndFilteredStrings(pMergedSrcPhrase, markersStr, endMarkersStr,
					freeTransStr, noteStr, collBackTransStr, filteredInfoStr);
	// remove any filter bracketing markers if filteredInfoStr has content
	if (!filteredInfoStr.IsEmpty())
	{
		filteredInfoStr = pDoc->RemoveAnyFilterBracketsFromString(filteredInfoStr);

		// separate out any crossReference info (plus marker & endmarker) if within this
		// filtered information
		SeparateOutCrossRefInfo(filteredInfoStr, xrefStr, otherFiltered);		
	}

    // for the first CSourcePhrase, we store any filtered info within the prefix
    // string, and any content in m_markers, if present, must be put at the start
    // of Tstr and Sstr; remove LHS whitespace when done
    // BEW 8Sep10, changed the order to be: 1. filtered info, 2. collected bt
	// 3. note 4. free trans, to help with OXES parsing - we want any free trans to
	// immediately precede the text it belongs to, and OXES likes any annotations (ie. our
	// notes) to be listed in sequence after the start of the verse but before the trGroup
	// for the verse (and with a character count from start of verse text), and we don't
	// want filtered info to be considered as "belonging" to the text which our free
	// translation belongs to, for OXES purposes, as the location where we store filtered
	// info is a matter only of convenience, so we need to make filtered info be first.
	bool bAttachFilteredInfo = TRUE;
	bool bAttach_m_markers = TRUE;
	// next call ignores m_markers, and the otherFiltered string input has no
	// crossReference info left in it - these info types are handled by a separate call,
	// the one below to GetUnfilteredCrossRefsAndMMarkers()
	markersPrefix = GetUnfilteredInfoMinusMMarkersAndCrossRefs(pMergedSrcPhrase,
							pSrcPhrases, otherFiltered, collBackTransStr,
							freeTransStr, noteStr, bDoCount, bCountInTargetText); // m_markers
								// and xrefStr handled in a separate function, later below
	markersPrefix.Trim(FALSE); // finally, remove any LHS whitespace
	// make sure it ends with a space
	markersPrefix.Trim();
	markersPrefix << aSpace;

	// BEW 11Oct10, the initial stuff is now more complex, so we just can't insert
	// markersStr preceding the passed in m_targetStr value; so we'll define a new local
	// string, strInitialStuff in which to build the stuff which precedes m_targetStr and
	// then we'll insert it later below
	wxString strInitialStuff;
	// now collect any beginmarkers and associated data from m_markers, into strInitialStuff,
	// and if there is content in xrefStr (and bAttachFilteredInfo is TRUE) then put that
	// content after the markersStr (ie. m_markers) content; delay placement until later
	strInitialStuff = GetUnfilteredCrossRefsAndMMarkers(strInitialStuff, markersStr, xrefStr,
						bAttachFilteredInfo, bAttach_m_markers);
	// for Sstr, which we only show to the user so he/she has the source to guide what is
	// done in the target by manual editing in the placement dialog, we don't include any
	// of the filtered info, so just put markersStr into Sstr
	Sstr << markersStr;
    // This markersStr data has, in docV5, only beginmarkers and things like chapter num or
    // verse num, and so forth. We'll show this to the user in the dialog, but not as
    // marker material for placement. This stuff will be the first, if it exists, in Sstr
    // and Tstr (except for filtered stuff, which will precede if it exists)

	// if there is an inline non-binding beginmarker, it should be next; we can append it
	// to strInitialStuff, and likewise to Sstr...
	// USFM examples from UBS illustrate non-binding begin markers follow
	// begin markers that get stored in m_markers, that's why this is here
	if (!pMergedSrcPhrase->GetInlineNonbindingMarkers().IsEmpty())
	{
		// we store these markers with a delimiting space, so its already in place (and we
		// can place this now in strInitialStuff, no dlg for placement required, because
		// any such marker precedes initial punctuation)
		iNBMkrs = pMergedSrcPhrase->GetInlineNonbindingMarkers();
		Sstr += iNBMkrs;
		strInitialStuff += iNBMkrs;
	}
	
	// There may be an inline binding beginmarker. We'll require the user to place it
	// using the dialog, because it may need to be inserted following preceding
	// punctuation already on the passed in m_targetStr (which is now Tstr).
	// From this point on we can't, because of the 11Oct10 changes to CSourcePhrase, just
	// add such a marker to Sstr - we'll instead have to recompose the inner core of
	// source word +- inline binding marker and endmarker +- preceding and/or following
	// punctuation in blocks further down, within the loop which spans the sequence of
	// original CSourcePhrase instances
	if (!pMergedSrcPhrase->GetInlineBindingMarkers().IsEmpty())
	{
		iBMkrs = pMergedSrcPhrase->GetInlineBindingMarkers();
		// there should always be a final space in m_inlineBindingMarkers, 
		// and we'll ensure it
		iBMkrs.Trim(FALSE);
		iBMkrs.Trim();
		iBMkrs += aSpace;
		markersToPlaceArray.Add(iBMkrs);
		bHasInternalMarkers = TRUE;
	}
	else
	{
		iBMkrs.Empty();
	}
    // Any inline binding endmarker on pMergedSrcPhrase will not be internal to the merger,
    // but because of the possibility of final punctuation ( it would need to be inserted
    // before any such) it will have to be placed within the placement dialog. But in Sstr
    // it is determinate in location, so we can do it by code - but only after the LAST
    // CSourcePhrase instance in the loop below, and only after the inner core of that
    // source text word plus mkrs plus puncts is rebuilt; but here we can get the endmarker
    // into bEMkrs, if it exists.
    // For Tstr, we must store this (and other endmarkers on pMergedSrcPhrase) on a
    // different array than markersToPlaceArray because otherwise the order of markers
    // shown to the user would be unhelpful - endmarkers from pMergedSrcPhrase are
    // pertinent to the LAST CSourcePhrase instane in the loop, and so should be moved into
    // markersToPlaceArray only after the loop below has ended.
	if (!pMergedSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
	{
		bEMkrs = pMergedSrcPhrase->GetInlineBindingEndMarkers();
        // there should never be a final space in m_inlineBindingMarkers, and
        // we'll ensure it (& such markers don't exist in the PNG 1998 SFM
        // standard)
		bEMkrs.Trim(FALSE);
		bEMkrs.Trim();
		markersAtVeryEndArray.Add(bEMkrs);
		bHasInternalMarkers = TRUE;
		bFinalEndmarkers = TRUE;
	}
	else
	{
		bEMkrs.Empty();
	}
    // Any endmarkers (ie. contents of m_endMarkers on pMergedSrcPhrase which is the parent
    // of the stored originals) on the first CSourcePhrase are potentially not "medial",
    // but because there might be final punctuation preceding which this/these endmarkers
    // may need to be inserted, the user will have to place them from the dialog. But for
    // Sstr they are determinate and so can be automatically placed later below, where the
    // bFinalEndmarkers == TRUE block is. (endMarkersStr is set above in the
    // GetMarkersAndFilteredStrings() call)
	if (!endMarkersStr.IsEmpty())
	{
		// we've endmarkers from m_endMarkers pf pMergedSrcPhrase that we have to 
		// deal with 
		markersAtVeryEndArray.Add(endMarkersStr);
		bHasInternalMarkers = TRUE;

		// and for Sstr's use...
		bFinalEndmarkers = TRUE; // use this below to do the final append
		finalSuffixStr = endMarkersStr;
	}
	else
	{
		endMarkersStr.Empty();
	}

	// finally, there could be an inline non-binding endmarker stored on pMergedSrcPhrase,
	// so get it into nBEMkr if so. It will have to be added to the markersAtVeryEndArray
	// array for placement in the placement dialog (we'll do this for safety, but probably
	// it would be the very last thing in the string and so not need to be placed by the
	// dialog, nevertheless we let the user do it), but for Sstr it can be placed by our
	// code below using the value in nBEMkr
	if (!pMergedSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
	{
		nBEMkrs = pMergedSrcPhrase->GetInlineNonbindingEndMarkers();
		nBEMkrs.Trim(FALSE);
		nBEMkrs.Trim();
		// for Tstr, placement via dlg
		markersAtVeryEndArray.Add(nBEMkrs);
		bHasInternalMarkers = TRUE;
		
		// for Sstr, placement below by our code
		bFinalEndmarkers = TRUE; // use this below to do the final append
								 // of any nBEMkrs content
	}
	else
	{
		nBEMkrs.Empty();
	}

	// now that we've got everything we can off of pMergedSrcPhrase, we can prefix the
	// strInitialStuff to Tstr here
	if (!strInitialStuff.IsEmpty())
	{
		Tstr = strInitialStuff + Tstr;
	}

    // Now loop over each of the original CSourcePhrase instances in the merger to get the
    // markers in medial location stored on them, the first pSrcPhrase has to be given
    // special treatment, as does the last; compose Sstr, but Tstr is passed in, and at
    // most only needs markers and endmarkers added, and, since the 11Oct10 changes for
    // docV5 additions, inline markers and m_follOuterPunct handled appropriately. Also any
    // filtered info is composed already and stored in markersPrefix - with special
    // handling for any free translation. There is, of course, never any filtered info
    // internal to a merger - our interface don't allow merging when filtered info would be
    // made internal to the merger.
	wxString strCore; strCore.Empty();
	wxString inlineNBMkrs_forLoop;
	wxString markersStr_forLoop;
	wxString endMarkersStr_forLoop;
	while (pos != NULL)
	{
		if (pos == posLast)
		{
			bLast = TRUE;
		}
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();

		// get the filtered stuff only for the non-first CSourcPhrase instance, because we
		// got it already for the first, before we entered the loop, and we don't want a
		// further call on the first of the saved ones to wipe out values
		if (!bFirst)
		{
			// empty the scratch strings
			EmptyMarkersAndFilteredStrings(markersStr_forLoop, endMarkersStr_forLoop, 
						freeTransStr, noteStr, collBackTransStr, filteredInfoStr);
            // get the other string information we want, putting it in the scratch strings;
            // in these original stored CSourcePhrase instances, there won't be any
            // filtered info (nor note, stored free translation, nor collected back
            // translation) and so we can chuck filteredInfoStr, noteStr, etc - it is only
            // markersStr_forLoop and endMarkersStr_forLoop we want from this call; and
            // further below, any inline markers and endmarkers stores as a result of
            // 11Oct10 docV5 changes
			GetMarkersAndFilteredStrings(pSrcPhrase, markersStr_forLoop, 
						endMarkersStr_forLoop, freeTransStr, noteStr, collBackTransStr,
						filteredInfoStr);
			// for the non-first pSrcPhrase instances, we'll use markersStr and
			// endMarkersStr in the code below the next block
		}

		// deal with the last pSrcPhrase - the begin markers on it won't have been seen
		// yet by any of the code above or below
		if (bLast)
		{
			// this block is for m_markers and m_endMarkers stuff, and handling inline
			// markers and m_follOuterPunct for the LAST pSrcPhrase of the stored list of
			// originals in pMergedSrcPhrase; and also accumulation of the source text
			// with markers back in the correct locations, for Sstr. Since
			// pMergedSrcPhrase in its m_follPunct, m_follOuterPunct, and endmarkers
			// stores stores the same as what this last pSrcPhrase does in the same
			// members, and since we've already saved and are holding over those values
			// and will place them after the loop finishes, we only have to deal in this
			// block with m_precPunct and beginmarkers on this pSrcPhrase
			
			// BEW 11Oct10, we have to rebuild the core of the source string starting with
			// m_key so start doing that now, & continue in the blocks below
			strCore = pSrcPhrase->m_key;

			// look for an inline non-binding (begin)marker - for Tstr, it is medial and
			// to be placed using the dialog; for strCore, it can be placed by code, once
			// strCore is finished
			if (!pSrcPhrase->GetInlineNonbindingMarkers().IsEmpty())
			{
				inlineNBMkrs_forLoop = pSrcPhrase->GetInlineNonbindingMarkers();
				markersToPlaceArray.Add(inlineNBMkrs_forLoop);
				bHasInternalMarkers = TRUE;
			}
			else
			{
				inlineNBMkrs_forLoop.Empty();
			}
			// look for m_markers content, for Tstr it is medial and must be inserted
			// using the placement dialog; for strCore is can be placed after strCore is
			// finished being built -- we got values for markersStr_forLoop and
			// endMarkersStr_forLoop in a block above, where !bFirst was TRUE
			if (!markersStr_forLoop.IsEmpty())
			{
				markersToPlaceArray.Add(markersStr_forLoop);
				bHasInternalMarkers = TRUE;
			}
			else
			{
				markersStr_forLoop.Empty();
			}
			// handle inline binding (begin)markers, if present; for Tstr, it's for
			// placement using the dialog; for strCore, it can be inserted immediately
			if (!pSrcPhrase->GetInlineBindingMarkers().IsEmpty())
			{
				wxString iBdgMrks = pSrcPhrase->GetInlineBindingMarkers();
				markersToPlaceArray.Add(iBdgMrks);
				bHasInternalMarkers = TRUE;
				strCore = iBdgMrks + strCore; // for Sstr
			}
 			// next, insert any preceding punctuation from this pSrcPhrase, but just for
			// strCore, since Tstr already has it when passed in
			if (!pSrcPhrase->m_precPunct.IsEmpty())
			{
				strCore = pSrcPhrase->m_precPunct + strCore;
			}
			// strCore is now finished, so we can insert any preceding inline non-binding
			// beginmarker and any preceding m_markers content, the latter comes first;
			// Tstr is already handled
			if (!inlineNBMkrs_forLoop.IsEmpty())
			{
				strCore = inlineNBMkrs_forLoop + strCore;
			}
			if (!markersStr_forLoop.IsEmpty())
			{
				strCore = markersStr_forLoop + strCore;
			}
			// now we can append strCore to Sstr and clear it
			if (!strCore.IsEmpty())
			{
				Sstr += strCore;
				strCore.Empty();
			}
			// Do not clear bLast to FALSE, else the !bFirst && !bLast block below will
			// wrongly be entered again & another duplicate src text last word will be
			// added to Sstr!!
		}

		// we compose the prefix string, and the original source string, but the prefix
		// string is built only from what is stored filtered on the first CSourcePhrase
		if (bFirst)
		{
			bFirst = FALSE; // prevent this block from being re-entered

			// BEW 11Oct10, we have to rebuild the core of the source string starting with
			// m_key so do that now
			strCore = pSrcPhrase->m_key;
			if (!iBMkrs.IsEmpty())
			{
				// it is on both this pSrcPhrase, and pMergedSrcPhrase, so since we've got
				// it already from the latter, use it now because this is the first
				// pSrcPhrase
				strCore = iBMkrs + strCore;
			}
			// check now for an inline binding endmarker to match, stored on this
			// pSrcPhrase. If there, it won't have been seen by the code above and so we
			// must add it to markersToPlaceArray next as it is medial to the merger, but
			// for strCore we can append it immediately
			if (!pSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
			{
				markersToPlaceArray.Add(pSrcPhrase->GetInlineBindingEndMarkers());
				bHasInternalMarkers = TRUE;
				strCore += pSrcPhrase->GetInlineBindingEndMarkers(); // for Sstr
			}
            // preceding punctuation, if any, was passed in in-place in Tstr, but for
            // strCore we must prefix it now if it exists
			if (!pSrcPhrase->m_precPunct.IsEmpty())
			{
				strCore = pSrcPhrase->m_precPunct + strCore;
			}
			// following punctuation, if any, was passed in in-place in Tstr, but for
			// strCore we must suffix it now if it exists
			if (!pSrcPhrase->m_follPunct.IsEmpty())
			{
				strCore += pSrcPhrase->m_follPunct;
			}
			// if, when examining pMergedSrcPhrase, we found an inline non-binding
			// beginmarker, it was appended to Sstr before we entered this loop, and so we
			// can ignore looking for that one on pSrcPhrase
			;
			// strCore is complete, and it can be appended now to Sstr
			if (!strCore.IsEmpty())
			{
				Sstr += strCore;
				strCore.Empty();
			}
            // there might be endmarkers in the m_endMarkers member, and these have not
            // been seen by any of our code yet. For Tstr they are medial to the phrase and
            // so will need to be placed using the placement dialog; for Sstr they can be
            // appended to immediately, as they come next if they exist
			if (!pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				endMarkersStr = pSrcPhrase->GetEndMarkers();
				markersToPlaceArray.Add(endMarkersStr);
				bHasInternalMarkers = TRUE;
				Sstr += endMarkersStr;
			}
			else
			{
				endMarkersStr.Empty();
			}
			// next, we have to append m_follOuterPunct content, if any exists
			wxString outers; outers.Empty();
			if (!pSrcPhrase->GetFollowingOuterPunct().IsEmpty())
			{
				// for Tstr, this or its equivalent target text punctuation will have been
				// input already in the passed in Tstr, so we don't need to so anything
				// here except fix up Sstr with what it needs
				wxString outers = pSrcPhrase->GetFollowingOuterPunct();
				Sstr += outers;
			}
			// finally there could be an inline non-binding endmarker. For Tstr this will
			// be medial, and require placing using the placement dialog. For Sstr, we can
			// append it now, if it exists
			if (!pSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
			{
				wxString iNonBEMkrs = pSrcPhrase->GetInlineNonbindingEndMarkers();
				markersToPlaceArray.Add(iNonBEMkrs);
				bHasInternalMarkers = TRUE;
				Sstr << iNonBEMkrs;
			}

			// must add a space to Sstr, since at least one more word follows
			Sstr << aSpace;

		} // end TRUE block for test: if (bFirst)
		else if (!bFirst && !bLast)
		{
			// this block is for m_markers and m_endMarkers stuff only, and accumulation
			// for Sstr (no filtered info can be in the remaining CSourcePhrase instances)
			
			// BEW 11Oct10, we have to rebuild the core of the source string starting with
			// m_key so start doing that now, & continue in the blocks below
			strCore = pSrcPhrase->m_key;

			// look for an inline non-binding (begin)marker - for Tstr, it is medial and
			// to be placed using the dialog; for strCore, it can be placed by code, once
			// strCore is finished
			if (!pSrcPhrase->GetInlineNonbindingMarkers().IsEmpty())
			{
				inlineNBMkrs_forLoop = pSrcPhrase->GetInlineNonbindingMarkers();
				markersToPlaceArray.Add(inlineNBMkrs_forLoop);
				bHasInternalMarkers = TRUE;
			}
			else
			{
				inlineNBMkrs_forLoop.Empty();
			}
			// look for m_markers content, for Tstr it is medial and must be inserted
			// using the placement dialog; for strCore is can be placed after strCore is
			// finished being built -- we got values for markersStr_forLoop and
			// endMarkersStr_forLoop in a block above, where !bFirst was TRUE
			if (!markersStr_forLoop.IsEmpty())
			{
				markersToPlaceArray.Add(markersStr_forLoop);
				bHasInternalMarkers = TRUE;
			}
			else
			{
				markersStr_forLoop.Empty();
			}
			// handle inline binding (begin)markers, if present; for Tstr, it's for
			// placement using the dialog, for strCore, it can be inserted immediately
			if (!pSrcPhrase->GetInlineBindingMarkers().IsEmpty())
			{
				wxString iBdgMrks = pSrcPhrase->GetInlineBindingMarkers();
				markersToPlaceArray.Add(iBdgMrks);
				bHasInternalMarkers = TRUE;
				strCore = iBdgMrks + strCore; // for Sstr
			}
			// check now for an inline binding endmarker to match, stored on this
			// pSrcPhrase. If there, it won't have been seen by the code above and so we
			// must add it to markersToPlaceArray next as it is medial to the merger, but
			// for strCore we can append it immediately
			if (!pSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
			{
				markersToPlaceArray.Add(pSrcPhrase->GetInlineBindingEndMarkers()); // for Tstr
				bHasInternalMarkers = TRUE;
				strCore += pSrcPhrase->GetInlineBindingEndMarkers(); // for Sstr
			}
			// next, insert any preceding punctuation from this pSrcPhrase, but just for
			// strCore, since Tstr already has it when passed in
			if (!pSrcPhrase->m_precPunct.IsEmpty())
			{
				strCore = pSrcPhrase->m_precPunct + strCore;
			}
			// following punctuation, if any, was passed in in-place in Tstr, but for
			// strCore we must suffix it now if it exists
			if (!pSrcPhrase->m_follPunct.IsEmpty())
			{
				strCore += pSrcPhrase->m_follPunct;
			}
			// strCore is now finished, so we can insert any preceding inline non-binding
			// beginmarker and any preceding m_markers content, the latter comes first;
			// Tstr is already handled
			if (!inlineNBMkrs_forLoop.IsEmpty())
			{
				strCore = inlineNBMkrs_forLoop + strCore;
			}
			if (!markersStr_forLoop.IsEmpty())
			{
				strCore = markersStr_forLoop + strCore;
			}
			// now we can append strCore to Sstr and clear it
			if (!strCore.IsEmpty())
			{
				Sstr += strCore;
				strCore.Empty();
			}
			// any m_endMarkers material will either be medial and so need to be put
			// in the list for placement via the dialog, for Tstr; for Sstr we can append
			// it immediately
			if (!endMarkersStr_forLoop.IsEmpty())
			{
				markersToPlaceArray.Add(endMarkersStr_forLoop);
				bHasInternalMarkers = TRUE;
				Sstr += endMarkersStr_forLoop;
			}
			// next, we have to append m_follOuterPunct content, if any exists
			wxString outers; outers.Empty();
			if (!pSrcPhrase->GetFollowingOuterPunct().IsEmpty())
			{
				// for Tstr, this or its equivalent target text punctuation will have been
				// input already in the passed in Tstr, so we don't need to so anything
				// here except fix up Sstr with what it needs
				wxString outers = pSrcPhrase->GetFollowingOuterPunct();
				Sstr += outers;
			}
			// finally there could be an inline non-binding endmarker. For Tstr this will
			// be medial, and require placing using the placement dialog. For Sstr, we can
			// append it now, if it exists
			if (!pSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
			{
				wxString iNonBEMkrs = pSrcPhrase->GetInlineNonbindingEndMarkers();
				markersToPlaceArray.Add(iNonBEMkrs);
				bHasInternalMarkers = TRUE;
				Sstr << iNonBEMkrs;
			}

			// must add a space to Sstr, since at least one more word follows
			Sstr << aSpace;

		} // end of TRUE block for test: else if (!bFirst && !bLast)

	} // end of while loop

	// Finally, add any final endmarkers from pMergedSrcPhrase held over till now; for
	// Tstr, they are in the a second array, markersAtVeryEndArray, - so copy the items 
	// across to the array for placement instead using the placement dialog. For Sstr we
	// have them stored already in strings and can finish off Sstr in the block below.
	// (for the call on next line, we are not interested in the returned boolean, and
	// third param, bExcludeDuplicates is default FALSE)
	if (!markersAtVeryEndArray.IsEmpty())
	{
		bHasInternalMarkers = TRUE;
		// this call will finish off the markersToPlaceArray array needed for the Place
		// dialog
		AddNewStringsToArray(&markersToPlaceArray, &markersAtVeryEndArray);
		markersAtVeryEndArray.Clear();
	}
	if (bFinalEndmarkers)
	{
		if (!bEMkrs.IsEmpty())
		{
			Sstr += bEMkrs;
		}
		// add any m_follPunct from pMergedSrcPhrase
		if (!pMergedSrcPhrase->m_follPunct.IsEmpty())
		{
			Sstr += pMergedSrcPhrase->m_follPunct;
		}
		if (!endMarkersStr.IsEmpty())
		{
			Sstr += endMarkersStr;
		}
		// handle any content in m_follOuterPunct
		if (!pMergedSrcPhrase->GetFollowingOuterPunct().IsEmpty())
		{
			Sstr += pMergedSrcPhrase->GetFollowingOuterPunct();
		}
		// finally, any final inline non-binding endmarker
		if (!nBEMkrs.IsEmpty())
		{
			Sstr += nBEMkrs;
		}
	}

	// if there are internal markers, put up the dialog to place them
	if (bHasInternalMarkers)
	{
		// Note: because the setters are called before ShowModal() is called,
		// initialization of the internal controls' pointers etc has to be done in the
		// creator, rather than as is normally done (ie. in InitDialog()) because the
		// latter is called from ShowModal(, which is too late; once the dialog is called,
		// we no longer care about what is within Sstr
		CPlaceInternalMarkers dlg((wxWindow*)gpApp->GetMainFrame());

		// set up the text controls and list box with their data; these setters enable the
		// data passing to be done without the use of globals
		dlg.SetNonEditableString(Sstr);
		dlg.SetUserEditableString(Tstr);
		dlg.SetPlaceableDataStrings(&markersToPlaceArray);

		// show the dialog
		dlg.ShowModal();

		// get the post-placement resulting string
		Tstr = dlg.GetPostPlacementString();

		// remove initial and final whitespace
		Tstr.Trim(FALSE);
		Tstr.Trim();
	}

	// now add the prefix string material not shown in the Place... dialog,
	// if it is not empty
	if (!markersPrefix.IsEmpty())
	{
		markersPrefix.Trim();
		markersPrefix += aSpace; // ensure a final space
		Tstr = markersPrefix + Tstr;
	}
	Tstr.Trim();
	Tstr << aSpace; // have a final space

	markersToPlaceArray.Clear();
	return Tstr;
}

///////////////////////////////////////////////////////////////////////////////////////
/// \return                     the modified value of the passed in Gstr gloss text
/// \param  pMergedSrcPhrase -> a merger, which may have non-empty m_pMedialMarkers member
/// \param  Gstr             -> the string into which there might need to be placed
///                             markers and endmarkers, possibly a placement dialog
///                             invoked on it as well
/// \remarks
/// This function is used in the preparation of editable / exportable glosses data (Gstr),
/// and if necessary it will call the PlaceInternalMarkers class to place medial markers
/// into the editable string. It also constructs a source text string, with reduced marker
/// content, suitable for guiding the user in any needed marker placement if medial markers
/// are to be replaced in the Placement dialog. It is used in RebuildGlossesText() to
/// compose non-editable and editable strings, these are source text and gloss text, from a
/// merger (the gloss would have been added after the merger since glossing mode does not
/// permit mergers). Mergers are illegal across filtered info, such info may only occur on
/// the first CSourcePhrase in a merger, and so we know there cannot be any medial filtered
/// information to be dealt with - we only have to consider m_markers and m_endMarkers.
/// Also, since a placement dialog doesn't need to do placement in any of the stored
/// initial filtered information, if present, we separate that information in markersPrefix
/// and don't present it to the user if the dialog is made visible, but just add it in
/// after the dialog is dismissed - but we only collect the filtered information in
/// m_filteredInfo, not any free or collected back translations, nor notes, as these may
/// not be relevant to the glossing text anyway.
/// Note: we only produce Sstr in order to support a referential source text in the top
/// edit box of the placement dialog - which won't be called if there are no medial
/// markers stored in the merged src phrase. The modified Gstr is what we return.
/// BEW 1Apr10, written for support of doc version 5
/// BEW 11Oct10, removed support for filtered info - it shouldn't be in a glosses export
/////////////////////////////////////////////////////////////////////////////////////////
wxString FromMergerMakeGstr(CSourcePhrase* pMergedSrcPhrase)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	wxASSERT(pMergedSrcPhrase->m_pSavedWords->GetCount() > 1); // must be a genuine merger
	SPList* pSrcPhraseSublist = pMergedSrcPhrase->m_pSavedWords;
	SPList::Node* pos = pSrcPhraseSublist->GetFirst();
	wxASSERT(pos != 0);
	SPList::Node* posLast;
	posLast = pSrcPhraseSublist->GetLast();
	wxASSERT(posLast != 0);
	bool bHasInternalMarkers = pMergedSrcPhrase->m_bHasInternalMarkers;
	bool bFirst = TRUE;
	bool bNonFinalEndmarkers = FALSE;
	bool bHasFilteredMaterial = FALSE;
	wxString tempStr;

	// store here any string of filtered information stored on pMergedSrcPhrase (in
	// m_filteredInfo only though)
	wxString Sstr; Sstr.Empty();
	wxString markersPrefix; markersPrefix.Empty();
	wxString Gstr = pMergedSrcPhrase->m_gloss; // could be empty
	bHasFilteredMaterial = HasFilteredInfo(pMergedSrcPhrase);

	// markers needed, since doc version 5 may store some filtered stuff without using them
	wxString freeMkr(_T("\\free"));
	wxString freeEndMkr = freeMkr + _T("*");
	wxString noteMkr(_T("\\note"));
	wxString noteEndMkr = noteMkr + _T("*");
	wxString backTransMkr(_T("\\bt"));
	markersPrefix.Empty(); // clear it out

	wxString finalSuffixStr; finalSuffixStr.Empty(); // put collected-string-final endmarkers here
	bool bFinalEndmarkers = FALSE; // set TRUE when finalSuffixStr has content to be added at loop end

	wxString aSpace = _T(" ");
	wxString markersStr;
	wxString endMarkersStr;
	wxString freeTransStr;
	wxString noteStr;
	wxString collBackTransStr;
	wxString filteredInfoStr;

	// the first one, pMergedSrcPhrase as passed in, has to provide the stuff for the
	// markersPrefix wxString...
	GetMarkersAndFilteredStrings(pMergedSrcPhrase, markersStr, endMarkersStr,
					freeTransStr, noteStr, collBackTransStr,filteredInfoStr);
	/* BEW removed 11Oct10
	// remove any filter bracketing markers if filteredInfoStr has content
	if (!filteredInfoStr.IsEmpty())
	{
		filteredInfoStr = pDoc->RemoveAnyFilterBracketsFromString(filteredInfoStr);
	}
	*/
	if (!endMarkersStr.IsEmpty())
	{
		bFinalEndmarkers = TRUE;
		finalSuffixStr = endMarkersStr;
	}

	/* BEW removed 11Oct10
    // from the merger we store any filtered info within the prefix
    // string, but any content in m_markers, if present, must be put at the start
	// of Gstr and Sstr; remove LHS whitespace when done; ignore free translation, note
	// and collected back translation even if they exist here
	if (!filteredInfoStr.IsEmpty())
	{
		// this data has any markers and endmarkers already 'in place'
		if (markersPrefix.IsEmpty())
		{
			markersPrefix << filteredInfoStr;
		}
		else
		{
			markersPrefix.Trim();
			markersPrefix += aSpace + filteredInfoStr;
		}
	}
	*/
	markersPrefix.Trim(FALSE); // finally, remove any LHS whitespace
	// make sure it ends with a space
	markersPrefix.Trim();
	markersPrefix << aSpace;

	if (!markersStr.IsEmpty())
	{
        // this data has any markers and endmarkers already 'in place', and we'll
        // put this in Gstr, so it may be shown to the user - because it may
        // contain a marker for which there is a later 'medial' matching endmarker
        // which has to be placed by the dialog, so it helps to be able to see what
        // the matching marker is and where it is; this stuff will be the first, if
        // it exists, in Sstr and Gstr
		Sstr << markersStr;
		Gstr = markersStr + Gstr;
	}

    // loop over each of the original CSourcePhrase instances in the merger, the first has
    // to be given special treatment; compose Sstr, but Gstr is passed in, and at most only
    // needs markers and endmarkers added, and any filtered info for both is returned via
    // markersPrefix
    // BEW comment 11Oct10, filtered info is now ignored, for glosses export
	while (pos != NULL)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();

		// get the filtered stuff only for the non-first CSourcPhrase instance, because we
		// got it already for the first, before we entered the loop, and we don't want a
		// further call on the first of the saved ones to wipe out values
		if (!bFirst)
		{
			// empty the scratch strings
			EmptyMarkersAndFilteredStrings(markersStr, endMarkersStr, freeTransStr, noteStr,
											collBackTransStr, filteredInfoStr);
			// get the other string information we want, putting it in the scratch strings
			GetMarkersAndFilteredStrings(pSrcPhrase, markersStr, endMarkersStr,
							freeTransStr, noteStr, collBackTransStr,filteredInfoStr);
		}

		// we compose the source string, but the prefix string is built already, so we
		// are interested in, markers before and after for the non-initial pSrcPhrase
		// instances, and after, but only for the first
		if (bFirst)
		{
			bFirst = FALSE; // prevent this block from being re-entered

			Sstr << pSrcPhrase->m_srcPhrase;
			if (!pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				Sstr << pSrcPhrase->GetEndMarkers();
			}
		} // end TRUE block for test: if (bFirst)
		else
		{
			// this block is for m_markers and m_endMarkers stuff only, and accumulation
			// for Sstr (no filtered info can be in the remaining CSourcePhrase instances)

            // m_markers material belongs in the list for later placement in Gstr, but the
            // list is medial markers is already populated correctly before entry, however
            // for Sstr we must place m_markers content automatically because its position
            // is determinate and not subject to relocation in the placement dialog
			if (!markersStr.IsEmpty())
			{
				bHasInternalMarkers = TRUE;
				Sstr << markersStr;
			}

			// any m_endMarkers material will either be medial and so need to be put
			// in the list, or final (if this is the last CSourcePhrase of the
			// retranslation, that is, if m_bEndRetranslation is TRUE)
			bNonFinalEndmarkers = FALSE;
			if (!endMarkersStr.IsEmpty())
			{
				// we've endmarkers we have to deal with
				if (pos != NULL)
				{
					// not at the list's end, so these endmarkers are medial, but we only
					// need to ensure that they are added to Sstr, m_pMedialMarkers is
					// already correctly populated
					bHasInternalMarkers = TRUE;
					bNonFinalEndmarkers = TRUE;
				}
				else
				{
                    // the endmarkers are at the end of the list so we can
                    // place these automatically (don't need to use the dialog)
					bFinalEndmarkers = TRUE; // use this below to do the final append
				}
			}

			// add the source text's word, if it is not an empty string (and it can't be
			// a placeholder as we don't allow them in the formation of mergers)
			if (!pSrcPhrase->m_srcPhrase.IsEmpty())
			{
				Sstr.Trim();
				Sstr << aSpace << pSrcPhrase->m_srcPhrase;
			}
			// add any needed non-final endmarkers
			if (bNonFinalEndmarkers)
			{
				Sstr << endMarkersStr;
			}
		}
	} // end of while loop

	// lastly, add any final endmarkers
	Sstr << finalSuffixStr;
	if (bFinalEndmarkers)
	{
		Gstr << finalSuffixStr;
	}

	// if there are internal markers, put up the dialog to place them
	if (bHasInternalMarkers)
	{
		// Note: because the setters are called before ShowModal() is called,
		// initialization of the internal controls' pointers etc has to be done in the
		// creator, rather than as is normally done (ie. in InitDialog()) because the
		// latter is called from ShowModal(, which is too late; once the dialog is called,
		// we no longer care about what is within Sstr
		CPlaceInternalMarkers dlg((wxWindow*)gpApp->GetMainFrame());

		// BEW added test, 07Oct05
		if (Gstr[0] != _T(' '))
		{
			// need to add an initial space if there is not one there already, to make the
			// dialog's placement algorithm fail-safe
			Gstr = _T(" ") + Gstr;
		}

		// set up the text controls and list box with their data; these setters enable the
		// data passing to be done without the use of globals
		dlg.SetNonEditableString(Sstr);
		dlg.SetUserEditableString(Gstr);
		// BEW changed 11Oct10, in keeping with the exclusion of inline markers other than
		// those for footnotes, endnotes or cross references, we here remove any inline
		// binding or non-binding markers that are in the m_pMedialMarkers list. Anything
		// which remains should be placed in the dialog. If nothing remains, the Gstr is
		// correct "as is".
		wxArrayString arrTemp;
		size_t count = pMergedSrcPhrase->m_pMedialMarkers->GetCount();
		wxString mkr;
		size_t index;
		for (index = 0; index < count; index++)
		{
			mkr = pMergedSrcPhrase->m_pMedialMarkers->Item(index);
			wxString rev = MakeReverse(mkr);
			if (rev[0] == _T('*'))
				rev = rev.Mid(1); // remove * if it is present
			wxString mkr2 = MakeReverse(rev);
			wxString shortMkr = mkr2.Left(2); // footnote, endnote, xref give \f or \x for this
			USFMAnalysis* pSfm =  pDoc->LookupSFM((wxChar*)mkr.GetData());
			if (mkr == _T("\\fig"))
			{
				// exclude this one from acceptance
				;
			}
			else if (!pSfm->inLine  || (pSfm->inLine && (shortMkr == _T("\\f") || shortMkr == _T("\\x"))))
			{
				// we accept for placement any which satisfy that 2nd test
				arrTemp.Add(mkr);
			}
		}
		if (!arrTemp.IsEmpty())
		{
			dlg.SetPlaceableDataStrings(&arrTemp);

			// show the dialog
			dlg.ShowModal();

			// get the post-placement resulting string
			Gstr = dlg.GetPostPlacementString();
		}
	}

	// now add the prefix string material not shown in the Place... dialog,
	// if it is not empty
	if (!markersPrefix.IsEmpty())
	{
		markersPrefix.Trim();
		markersPrefix += aSpace; // ensure a final space
		Gstr = markersPrefix + Gstr;
	}
	Gstr.Trim();
	Gstr << aSpace; // have a final space
	return Gstr;
}

// A useful utility which ignores filtered information and m_markers, but collects, in
// sequence, other punctuation and marker information which precedes the phrase. This
// material is stored in m_inlineNonbindingMarkers, m_precPunct, and
// m_inlineBindingEndMarkers, in that order. Any of such which is non-empty is appended,
// with delimiting spaces where appropriate (to comply with good USFM markup standards) to
// the passed in appendHere string, which is then returned to the caller. This function is
// similar to AppendSrcPHraseBeginningInfo() of ExportFunctions.cpp, but the latter handles
// filtered info and m_markers as well. GetSrcPhraseBeginningInfo() is used in
// FromMergerMakeSstr() and I may use it in others too
// BEW created 11Oct10 for helping with doc version 5 additions to CSourcePhrase
wxString GetSrcPhraseBeginningInfo(wxString appendHere, CSourcePhrase* pSrcPhrase, 
					 bool& bAddedSomething)
{
    bAddedSomething = FALSE;
	if (!pSrcPhrase->GetInlineNonbindingMarkers().IsEmpty())
	{
		appendHere += pSrcPhrase->GetInlineNonbindingMarkers();
		bAddedSomething = TRUE;
	}
	if (!pSrcPhrase->m_precPunct.IsEmpty())
	{
		appendHere += pSrcPhrase->m_precPunct;
		appendHere.Trim(); // make sure no bogus space follows
		bAddedSomething = TRUE;
	}
	if (!pSrcPhrase->GetInlineBindingMarkers().IsEmpty())
	{
		wxString binders = pSrcPhrase->GetInlineBindingMarkers();
		binders.Trim(FALSE); // make sure no bogus space precedes
		appendHere += binders;
		bAddedSomething = TRUE;
	}
	return appendHere;
}

///////////////////////////////////////////////////////////////////////////////////////
/// \return                     the reconstituted source text (excluding some, see below)
/// \param  pMergedSrcPhrase -> a merger, which may have non-empty m_pMedialMarkers member
/// \remarks
/// This function is used in the preparation of source text data (Sstr). It is used in
/// RebuildSourceText() in the export of USFM markup for the source text when the passed in
/// CSourcePhrase instance is a merger; it is also used in
/// ReconstituteAfterPunctuationChange() - which is a function defined in CAdapt_ItDoc.
/// Mergers are illegal across filtered info, such info may only occur on the first
/// CSourcePhrase in a merger, and so we know there cannot be any medial filtered
/// information to be dealt with - we only have to consider m_markers and m_endMarkers --
/// and since 11Oct10 (see below) inline non-binding markers and endmarkers, inline binding
/// markers and endmarkers, and m_follOuterPunct. Also, no placement dialog is needed
/// because, for source text, the location of SF markers in the recomposed source text is
/// 100% determinate. Note: With the non-initial CSourcePhrase members we must take into
/// account any markers and punctuation information medial to the merger.
/// BEW 7Apr10, written for support of doc version 5
/// BEW 11Oct10, additions to the code for extra doc version 5 storage members,
/// m_follOuterPunct and four wxString members for inline binding and non-binding markers
/// or endmarkers.
/////////////////////////////////////////////////////////////////////////////////////////
wxString FromMergerMakeSstr(CSourcePhrase* pMergedSrcPhrase)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	wxASSERT(pMergedSrcPhrase->m_pSavedWords->GetCount() > 1); // must be a genuine merger
	SPList* pSrcPhraseSublist = pMergedSrcPhrase->m_pSavedWords;
	SPList::Node* pos = pSrcPhraseSublist->GetFirst();
	wxASSERT(pos != 0);
	SPList::Node* posLast = pSrcPhraseSublist->GetLast();
	wxASSERT(posLast != 0);

	wxString str; // accumulate the source text here

	// markers needed, since doc version 5 may store some filtered stuff without using them
	wxString freeMkr(_T("\\free"));
	wxString freeEndMkr = freeMkr + _T("*");
	wxString noteMkr(_T("\\note"));
	wxString noteEndMkr = noteMkr + _T("*");
	wxString backTransMkr(_T("\\bt"));
	str.Empty(); // clear it out

	wxString aSpace = _T(" ");
	wxString markersStr;
	wxString endMarkersStr;
	wxString freeTransStr;
	wxString noteStr;
	wxString collBackTransStr;
	wxString filteredInfoStr;

    // loop over each of the original CSourcePhrase instances in the merger, the first has
    // to be given special treatment (the caller has already dealt with m_markers and any
    // other filtered information stored on the merger); otherwise we only care about
    // m_markers, m_srcPhrase and m_endMarkers, and we ignore m_markers for the first; the
    // rest of the wxStrings above are ignored as they will be empty strings only
	bool bFirst = TRUE;
	bool bLast = FALSE;
	wxString beforeStr;
	wxString afterStr;
	bool bAddedSomething = FALSE;
	while (pos != NULL)
	{
		if (pos == posLast)
		{
			bLast = TRUE;
		}
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();

		// filtered info can only be on the first, and it's copied to pMergedSrcPhrase
		// anyway, so get it from the latter (if present)
		if (bFirst)
		{
			// bFirst will be cleared to FALSE in the lower bFirst == TRUE block below

			// empty the scratch strings
			EmptyMarkersAndFilteredStrings(markersStr, endMarkersStr, freeTransStr, noteStr,
											collBackTransStr, filteredInfoStr);
			// get the other string information we want, putting it in the scratch strings
			GetMarkersAndFilteredStrings(pMergedSrcPhrase, markersStr, endMarkersStr,
							freeTransStr, noteStr, collBackTransStr, filteredInfoStr);
			if (!filteredInfoStr.IsEmpty())
			{
				filteredInfoStr = pDoc->RemoveAnyFilterBracketsFromString(filteredInfoStr);
			}
		}

		if (bLast)
		{
            // like the !bFirst && !bLast block, but the last function call has a different
            // parameter, it uses pMergedSrcPhrase to collect endmarker and following
            // punctuation information from the merged sourcephrase rather than from the
            // saved original (non-merged) last pSrcPhrase we are currently looking at
			beforeStr.Empty(); 
			bAddedSomething = FALSE;
			// bIncludeNote is TRUE, bDoCountForFreeTrans is TRUE, bCountInTargetTextLine
			// is FALSE, for last 3 params
			beforeStr = AppendSrcPhraseBeginningInfo(beforeStr, pSrcPhrase, bAddedSomething,
													TRUE,TRUE, FALSE);
			if (bAddedSomething)
			{
				str << beforeStr;
				beforeStr.Empty();
			}
			str << pSrcPhrase->m_key;
			afterStr.Empty();
			afterStr = AppendSrcPhraseEndingInfo(afterStr, pMergedSrcPhrase);
			if (!afterStr.IsEmpty())
			{
				str << afterStr;
				afterStr.Empty();
			}
			str.Trim();
			str << aSpace;
		}

		if (bFirst)
		{
			bFirst = FALSE; // prevent this block from being re-entered

			beforeStr.Empty();
			beforeStr = GetSrcPhraseBeginningInfo(beforeStr, pMergedSrcPhrase, bAddedSomething);
			if (bAddedSomething)
			{
				str << beforeStr;
				beforeStr.Empty();
			}
			str << pSrcPhrase->m_key;
			afterStr.Empty();
			afterStr = AppendSrcPhraseEndingInfo(afterStr, pSrcPhrase);
			if (!afterStr.IsEmpty())
			{
				str << afterStr;
				afterStr.Empty();
			}
			str.Trim();
			str << aSpace;
		} // end TRUE block for test: if (bFirst)
		else if (!bFirst && !bLast)
		{
			// like the bFirst block, but the first function call is different, since it
			// needs to check for m_markers info on the saved original (non-merged)
			// pSrcPhrase we are currently looking at
			beforeStr.Empty(); 
			bAddedSomething = FALSE;
			// bIncludeNote is TRUE, bDoCountForFreeTrans is TRUE, bCountInTargetTextLine
			// is FALSE, for last 3 params
			beforeStr = AppendSrcPhraseBeginningInfo(beforeStr, pSrcPhrase, bAddedSomething,
													TRUE,TRUE, FALSE);
			if (bAddedSomething)
			{
				str << beforeStr;
				beforeStr.Empty();
			}
			str << pSrcPhrase->m_key;
			afterStr.Empty();
			afterStr = AppendSrcPhraseEndingInfo(afterStr, pSrcPhrase);
			if (!afterStr.IsEmpty())
			{
				str << afterStr;
				afterStr.Empty();
			}
			str.Trim();
			str << aSpace;
		}
	} // end of while loop

	// finally, ensure there is just a single final space
	str.Trim();
	str << aSpace;
	return str;
}

// Pass in a string which could have \free .... \free* content, and / or \note ... \note*
// content and / or \bt .... (this will end when next marker follows, or string end)
// information, and send the leftovers back to the caller
// Note: we assume the filtered info we want to remove is only present once per marker
// type. If this is a false assumption, the user will see the additional bits in the Edit
// Source Text dialog. (Not likely to happen though.)
// BEW created 11Oct10 as a helper for OnEditSourceText()
wxString RemoveCustomFilteredInfoFrom(wxString str)
{
	int offset = wxNOT_FOUND;
	wxString startStr; startStr.Empty();
	wxString mkr;
	int length = 0;
	wxString backslash = _T("\\");
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();

	offset = str.Find(_T("\\bt ")); // only look for our own \bt markers, other \bt-based
									// ones are to be retained
	if (offset != wxNOT_FOUND)
	{
		// it's ours, remove it and the collected back trans info
		startStr = str.Left(offset);
		str = str.Mid(offset); // \bt starts at start of str now
		mkr = pDoc->GetWholeMarker(str);
		length = mkr.Len();
		str = str.Mid(length); // get past the marker
		//length = str.Len();
		offset = str.Find(backslash);
		if (offset == wxNOT_FOUND)
		{
			// the whole string is a collected back translation 
			str.Empty();
			return str;
		}
		else
		{
			str = str.Mid(offset); // keep from the backslash onwards
		}
	}
	if (!startStr.IsEmpty())
	{
		str = startStr + str;
	}
	// collected back translation info has been removed, next remove any free 
	// translation info
	startStr.Empty();
	offset = str.Find(_T("\\free"));
	if (offset != wxNOT_FOUND)
	{
		// it's ours, remove it and the free trans info
		startStr = str.Left(offset);
		str = str.Mid(offset); // \free starts at start of str now
		mkr = pDoc->GetWholeMarker(str);
		length = mkr.Len();
		str = str.Mid(length); // get past the \free marker
		// find where the \free* endmarker is
		offset = str.Find(_T("\\free*"));
		wxASSERT(offset != wxNOT_FOUND);
		str = str.Mid(offset + 6); // 6 is sizeof(\free*)
	}
	if (!startStr.IsEmpty())
	{
		str = startStr + str;
	}
	// finally, remove any note
	startStr.Empty();
	offset = str.Find(_T("\\note"));
	if (offset != wxNOT_FOUND)
	{
		// it's ours, remove it and the free trans info
		startStr = str.Left(offset);
		str = str.Mid(offset); // \note starts at start of str now
		mkr = pDoc->GetWholeMarker(str);
		length = mkr.Len();
		str = str.Mid(length); // get past the \free marker
		// find where the \free* endmarker is
		offset = str.Find(_T("\\note*"));
		wxASSERT(offset != wxNOT_FOUND);
		str = str.Mid(offset + 6); // 6 is sizeof(\note*)
	}
	if (!startStr.IsEmpty())
	{
		str = startStr + str;
	}
	return str;
}

///////////////////////////////////////////////////////////////////////////////////////
/// \return                     the modified value of Tstr
/// \param  pSingleSrcPhrase -> a single CSourcePhrase instance which therefore cannot have
///                             a non-empty m_pMedialMarkers member; so no placement dialog
///                             will need to be called (but pseudo-merger ~ fixed-space
///                             wordpair will need special treatment)
/// \param  Tstr             -> the string into which there might need to be
///                             placed m_markers and m_endmarkers material, and then
///                             prefixed with any filtered information
/// \remarks
/// This function is used for reconstituting markers and endmarkers, and preceding that,
/// any filtered out information (which always comes first, if present), and the result is
/// return to the caller as the modified Tstr. (Tstr may be passed in containing
/// adaptation text, or gloss text.)
/// BEW 1Apr10, written for support of doc version 5
/// BEW 8Sep10, altered order of export of filtered stuff, it's now 1. filtered info, 2.
/// collected back trans, 3. free translation 4. note (This fits better with OXES export)
/// BEW 17Sep10, added a character count, wrapped by @# and #@ at start of note text, so
/// that notes on a CSourcePhrase will carry with them a count of the characters in the
/// adaptation word - this is done only for the special case of an sfm export for use in
/// building oxes xml; also, we'll pass the actual word on to oxes in the format
/// @#nnn:word#@
/// BEW 11Oct10, extensively refactored so as to deal with the richer document model, v5,
/// in which there is m_follOuterPunct, and four wxString members for inline markers and
/// endmarkers, binding versus non-binding; plus also handle conjoining with ~ USFM fixed
/// space symbol (we treat this as a unit, but stucturally it is a merger of 2
/// CSourcePhrase instances)
/// Tstr, as passed in, is pSingleSrcPhrase's m_targetStr member, so it may have
/// punctuation before and/or after, and the after punctuation may have content from
/// m_follOuterPunct tacked on as well. In addition, being target text, target language
/// punctuation may differ from the source language, and also the user may have typed
/// different (overriding) punctuation anyway, so we can't copy the contents of
/// m_follPunct, m_precPunct nor m_follOuterPunct in order to rebuild. Instead, we'll have
/// to take the punctuation we find, strip it off the start and end, and then add in any
/// markers that are required. Since the original source text may have had "outer"
/// punctuation, we have to also give the user the chance to restore an endmarker
/// preceding one or more of the following punctuation which he deems should be considered
/// to be "outer" punctuation. The only way to do this is with the marker placement
/// dialog. However, we don't want to use this dialog at every word exported! So the rules
/// are the following: Open the placement dialog if:
/// (1) pSingleSrcPhrase->m_follOuterPunct has something stored in it, OR
/// (2) pSingleSrcPhrase->m_follOuterPunct is empty, AND pSingleSrcPhrase->m_endMarkers
/// has something in it, AND pSingleSourcePhrase->m_follPunct has something in it.
/// Case (2) allows for the possibility that the user may select to type outer punctuation
/// as a 'correction' to the lack of it in the source text load, and then will need the
/// placement dialog to open to allow him to locate the endmarker preceding it.
/// Also, when there is ~ conjoining, we now fully support inline binding marker and
/// endmarker on either or both words of the conjoined pair.
/// BEW 13Dec10, added support for [ and ] brackets. When Tstr passed in contains just [
/// or ] we return Tstr immediately. (No word counting is done, because in exports in a
/// later function which the caller calls we join the brackets to the text which they
/// bracket and so they don't contribute to the word count)
/////////////////////////////////////////////////////////////////////////////////////////
wxString FromSingleMakeTstr(CSourcePhrase* pSingleSrcPhrase, wxString Tstr, bool bDoCount, 
							bool bCountInTargetText)
{
	// to support [ and ] brackets which, if they occur, are the only content, bleed this
	// possibility out here, because the code below is not designed to support such a
	// situation
	if (Tstr == _T("["))
	{
		return Tstr;
	}
	if (Tstr == _T("]"))
	{
		return Tstr;
	}

	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	SPList* pSrcPhrases = gpApp->m_pSourcePhrases;
	bool bHasOuterFollPunct = FALSE;
	bool bIsAmbiguousForEndmarkerPlacement = FALSE;

	// is it normal instance, or one which stores a word pair conjoined with USFM fixed
	// space symbol ~
	bool bIsFixedSpaceConjoined = IsFixedSpaceSymbolWithin(pSingleSrcPhrase);
	bool bBindingMkrsToReplace = FALSE;
	wxString rebuiltTstr; rebuiltTstr.Empty();

	if (bIsFixedSpaceConjoined)
	{
		// we don't need to access the embedded CSourcePhrase pair, so long as there are
		// no inline binding begin or end markers stored on them - because we can get the
		// correct output string in that case from m_targetStr alone. But if one or both
		// the embedded instances is storing binding markers, then we have to rebuild the
		// m_targetStr the long way, putting the binding markers back where they belong.
		CSourcePhrase* pSPWord1 = NULL;
		CSourcePhrase* pSPWord2 = NULL;
		SPList::Node* posFirst = pSingleSrcPhrase->m_pSavedWords->GetFirst();
		SPList::Node* posSecond = pSingleSrcPhrase->m_pSavedWords->GetLast();
		pSPWord1 = posFirst->GetData();
		pSPWord2 = posSecond->GetData();
		// we only need test for an inline binding endmarker on the first word or and
		// inline binding beginmarker on the second word. If there is neither, then any
		// inline binding beginmarker on the first word and/or any inline binding
		// endmarker on the second word can be built from the code below without having to
		// call RebuildFixedSpaceTstr()
		if (	!pSPWord1->GetInlineBindingEndMarkers().IsEmpty()	||
				!pSPWord2->GetInlineBindingMarkers().IsEmpty() )
		{
			bBindingMkrsToReplace = TRUE;
            // we assume that ~ conjoining will NEVER precede a \f* or \fe* endmarker, and
            // therefore no ambiguity for following puncts, that is, whether outer or not,
            // will ever arise (if it does, we don't support it)
			rebuiltTstr = RebuildFixedSpaceTstr(pSingleSrcPhrase); // use it below
		}
	}

    // store here any string of filtered information stored on pSingleSrcPhrase (in any or
    // all of m_freeTrans, m_note, m_collectedBackTrans, m_filteredInfo)
	wxString markersPrefix; markersPrefix.Empty();

	// markers needed, since doc version 5 may store some filtered stuff without using them
	wxString freeMkr(_T("\\free"));
	wxString freeEndMkr = freeMkr + _T("*");
	wxString noteMkr(_T("\\note"));
	wxString noteEndMkr = noteMkr + _T("*");
	wxString backTransMkr(_T("\\bt"));
	markersPrefix.Empty(); // clear it out
	wxString finalSuffixStr; finalSuffixStr.Empty(); // put collected-string-final endmarkers here

	wxString aSpace = _T(" ");
	wxString markersStr;
	wxString endMarkersStr;
	wxString freeTransStr;
	wxString noteStr;
	wxString collBackTransStr;
	wxString filteredInfoStr;
	wxString xrefStr; // for \x* .... \x* cross reference info (it is stored preceding
					  // m_markers content, but other filtered info goes before m_markers
	wxString otherFiltered; // leftovers after \x ...\x* is removed from filtered info

	// empty the scratch strings
	EmptyMarkersAndFilteredStrings(markersStr, endMarkersStr, freeTransStr, noteStr,
									collBackTransStr, filteredInfoStr);
	// get the other string information we want, putting it in the scratch strings
	GetMarkersAndFilteredStrings(pSingleSrcPhrase, markersStr, endMarkersStr,
					freeTransStr, noteStr, collBackTransStr,filteredInfoStr);
	// remove any filter bracketing markers if filteredInfoStr has content
	if (!filteredInfoStr.IsEmpty())
	{
		filteredInfoStr = pDoc->RemoveAnyFilterBracketsFromString(filteredInfoStr);

		// separate out any crossReference info (plus marker & endmarker) if within this
		// filtered information
		SeparateOutCrossRefInfo(filteredInfoStr, xrefStr, otherFiltered);
	}

    // for the one and only CSourcePhrase, we store any filtered info within the prefix
    // string, and any content in m_markers, if present, must be put at the start
    // of Tstr; remove LHS whitespace when done
    // BEW 8Sep10, changed the order to be: 1. filtered info, 2. collected bt
	// 3. note 4. free trans, to help with OXES parsing - we want any free trans to
	// immediately precede the text it belongs to, and OXES likes any annotations (ie. our
	// notes) to be listed in sequence after the start of the verse but before the trGroup
	// for the verse (and with a character count from start of verse text), and we don't
	// want filtered info to be considered as "belonging" to the text which our free
	// translation belongs to, for OXES purposes, as the location where we store filtered
	// info is a matter only of convenience, so we need to make filtered info be first.
	bool bAttachFilteredInfo = TRUE;
	bool bAttach_m_markers = TRUE;
	// next call ignores m_markers, and the otherFiltered string input has no
	// crossReference info left in it - these info types are handled by a separate call,
	// the one below to GetUnfilteredCrossRefsAndMMarkers()
	markersPrefix = GetUnfilteredInfoMinusMMarkersAndCrossRefs(pSingleSrcPhrase,
							pSrcPhrases, otherFiltered, collBackTransStr,
							freeTransStr, noteStr, bDoCount, bCountInTargetText); // m_markers 
								// and xrefStr handled in a separate function, later below
	
	// BEW 11Oct10, the initial stuff is now more complex, so we can no longer insert
	// markersStr preceding the passed in m_targetStr value; so we'll define a new local
	// string, strInitialStuff in which to build the stuff which precedes m_targetStr and
	// then we'll insert it later below, after setting it with a function call
	wxString strInitialStuff;
	// now collect any beginmarkers and associated data from m_markers, into strInitialStuff,
	// and if there is content in xrefStr (and bAttachFilteredInfo is TRUE) then put that
	// content after the markersStr (ie. m_markers) content; delay placement until later
	strInitialStuff = GetUnfilteredCrossRefsAndMMarkers(strInitialStuff, markersStr, xrefStr,
												bAttachFilteredInfo, bAttach_m_markers);
	markersPrefix.Trim(FALSE); // finally, remove any LHS whitespace
	// make sure it ends with a space
	markersPrefix.Trim();
	markersPrefix << aSpace;

	// The task now, whether for conjoined pairs or a normal single target text word, is to 
	// remove preceding and following punctuation into storage strings temporarily. This
	// works the same whether USFM fixes space conjoins or not, provided there are no
	// inline binding begin or end markers when there is conjoining - but if there is, we
	// must use the rebuilt Tstr from the call above instead. 
    // Then we can add markers and, if necessary, call the placement dialog to handle
    // endmarker insertion where there may be ambiguity as to the correct endmarker
    // location.
	wxArrayString markersToPlaceArray;
	wxString endMkrsToPlace; endMkrsToPlace.Empty();
	wxString nonbindingEndMkrsToPlace; nonbindingEndMkrsToPlace.Empty();
	wxString theSymbol = _T("~");
	CSourcePhrase* pSP = pSingleSrcPhrase; // RHS is too long to type all the time
	wxString tgtStr;
	wxString initialPuncts;
	wxString finalPuncts;
	wxString tgtBaseStr;

    if (bBindingMkrsToReplace)
	{
		// need to use the rebuilt Tstr
		Tstr = rebuiltTstr;
	}
	else
	{
		// build Tstr: use this block when there is no conjoining with USFM fixed space ~
		// marker, or when there is but there are no "medial" inline binding marker or
		// endmarker involved (i.e. none to the left of the ~ marker and right of word1,
		// nor none to the right of ~ marker and left of word2)
		int itsLength = Tstr.Len();
		initialPuncts = SpanIncluding(Tstr,gpApp->m_punctuation[1]); // space is in m_punctuation
		int initialsLen = initialPuncts.Len();
		int lenNonInitials = itsLength - initialsLen; // this could result in zero
		wxString reversed = MakeReverse(Tstr); // if itsLength was 1, and no puncts, this will equal Tstr
		if (lenNonInitials > 1)
		{
			// needs to be 2 or more for there to be the possibility of at least one
			// character of following punctuation
			finalPuncts = SpanIncluding(reversed, gpApp->m_punctuation[1]); // space is in m_punctuation
		}
		else
		{
			finalPuncts.Empty(); // can't possibly be any following punctuation
		}
		int finalsLen = finalPuncts.Len();
		if (initialsLen == 0)
		{
			tgtBaseStr = Tstr;
			if (finalsLen > 0)
			{
				finalPuncts = MakeReverse(finalPuncts);
				tgtBaseStr = reversed.Mid(finalsLen);
				tgtBaseStr = MakeReverse(tgtBaseStr);
			}
		}
		else
		{
			tgtBaseStr = Tstr.Mid(initialsLen);
			if (finalsLen > 0)
			{
				finalPuncts = MakeReverse(finalPuncts);
				tgtBaseStr = MakeReverse(tgtBaseStr);
				tgtBaseStr = tgtBaseStr.Mid(finalsLen);
				tgtBaseStr = MakeReverse(tgtBaseStr);
			}
		}
		// initialPuncts, tgtBaseStr, finalPuncts now hold the desired substrings ready for us
		// to add markers. First, determine if we are going to need the placement dialog
		if (!pSP->GetFollowingOuterPunct().IsEmpty())
		{
			bHasOuterFollPunct = TRUE;
		}
		if (!finalPuncts.IsEmpty() && !pSP->GetEndMarkers().IsEmpty())
		{
			if (finalsLen > 1)
			{
				bIsAmbiguousForEndmarkerPlacement = TRUE;
			}
			endMkrsToPlace = pSP->GetEndMarkers();
			markersToPlaceArray.Add(endMkrsToPlace);
		}
		if (!finalPuncts.IsEmpty() && !pSP->GetInlineNonbindingEndMarkers().IsEmpty())
		{
			if (finalsLen > 1)
			{
				bIsAmbiguousForEndmarkerPlacement = TRUE;
			}
			nonbindingEndMkrsToPlace = pSP->GetInlineNonbindingEndMarkers();
			markersToPlaceArray.Add(nonbindingEndMkrsToPlace);
		}

		// build the core of Tstr, using tgtStr and starting with tgtBaseStr
		if (!pSP->GetInlineBindingMarkers().IsEmpty())
		{
			tgtStr = pSP->GetInlineBindingMarkers() + tgtBaseStr;
		}
		else
		{
			tgtStr = tgtBaseStr;
		}
		if (!pSP->GetInlineBindingEndMarkers().IsEmpty())
		{
			tgtStr += pSP->GetInlineBindingEndMarkers();
		}
		// put the punctuation back in place
		if (initialsLen > 0)
		{
			tgtStr = initialPuncts + tgtStr;
		}
		if (finalsLen > 0)
		{
			tgtStr += finalPuncts;
		}
		Tstr = tgtStr; // we've got any inline binding markers in place, now for the rest
	} // end of else block for test: if (bBindingMkrsToReplace)

	// an inline non-binding (begin)marker is next, if there is one
	if (!pSP->GetInlineNonbindingMarkers().IsEmpty())
	{
		Tstr = pSP->GetInlineNonbindingMarkers() + Tstr;
	}

	// add any m_markers content and unfiltered xref material if present, preceding what 
	// we have so far
	if (!strInitialStuff.IsEmpty())
	{
        // this data has any markers 'in place' & things like verse number etc if relevant,
        // and any crossReference material will be after the verse number (other unfiltered
        // filtered into to come from what is in markersPrefix will be preceding the \v )
		Tstr = strInitialStuff + Tstr;
	}

    // any endmarkers on pSingleSrcPhrase are not "medial", and can be added
	// automatically too provided there is no ambiguity about placement, but if there is
	// ambiguity, we will call the placement dialog for the user to get it right
	if (!bHasOuterFollPunct && !bIsAmbiguousForEndmarkerPlacement)
	{
		// there is no ambiguity, so no placement dialog needed
		if (!endMarkersStr.IsEmpty())
		{
			Tstr << endMarkersStr;
		}

		// if there is an inline non-binding endmarker, append it
		if (!pSP->GetInlineNonbindingEndMarkers().IsEmpty())
		{
			Tstr << pSP->GetInlineNonbindingEndMarkers();

		}
	}
	else
	{
		// there is ambiguity, so do the placement using the dialog
		wxString xrefStr;
		wxString mMarkersStr;
		wxString otherFiltered;
		bool bAttachFiltered = FALSE;
		bool bAttach_m_markers = TRUE;
		wxString Sstr = FromSingleMakeSstr(pSingleSrcPhrase, bAttachFiltered,
			bAttach_m_markers, mMarkersStr, xrefStr, otherFiltered, TRUE, FALSE); // need Sstr for the dialog

		CPlaceInternalMarkers dlg((wxWindow*)gpApp->GetMainFrame());

		// set up the text controls and list box with their data; these setters enable the
		// data passing to be done without the use of globals
		dlg.SetNonEditableString(Sstr);
		dlg.SetUserEditableString(Tstr);
		dlg.SetPlaceableDataStrings(&markersToPlaceArray);

		// show the dialog
		dlg.ShowModal();

		// get the post-placement resulting string
		Tstr = dlg.GetPostPlacementString();
	}

    // now add the prefix string material if it is not empty
	if (!markersPrefix.IsEmpty())
	{
		markersPrefix.Trim();
		markersPrefix << aSpace; // ensure a final space
		Tstr = markersPrefix + Tstr;
	}
	Tstr.Trim(FALSE);
	Tstr.Trim();
	// don't have a final space, the caller will add one if it is needed
	return Tstr;
}

// BEW created 11Oct10, for support of improved doc version 5 functionality. 
// Used in the FromSingleMakeTstr() function, when there are inline binding markes within
// the conjoining with USFM fixed space marker ~ joining a pair of words.
wxString RebuildFixedSpaceTstr(CSourcePhrase* pSingleSrcPhrase)
{
	// this function is called only when there are inline binding marker and/or endmarker
	// to replace within the target string in one or other or both of the conjoined words
	// which are having their target text exported
	wxString tgtStr = pSingleSrcPhrase->m_targetStr; // decompose this and rebuild it
											// with the inline binding markers in place
	CSourcePhrase* pSPWord1 = NULL;
	CSourcePhrase* pSPWord2 = NULL;
	wxString word1; word1.Empty();
	wxString word2; word2.Empty();
	wxString word1PrecPunct;
	wxString word1FollPunct;
	wxString word2PrecPunct;
	wxString word2FollPunct;
	wxString word1BindingMkr;
	wxString word1BindingEndMkr;
	wxString word2BindingMkr;
	wxString word2BindingEndMkr;
	SPList::Node* posFirst = pSingleSrcPhrase->m_pSavedWords->GetFirst();
	SPList::Node* posSecond = pSingleSrcPhrase->m_pSavedWords->GetLast();
	pSPWord1 = posFirst->GetData();
	pSPWord2 = posSecond->GetData();
	wxString FixedSpace = _T("~");

	const wxChar* pBuf = tgtStr.GetData();
	wxChar* ptr = (wxChar*)pBuf;
	int length = tgtStr.Len();
	wxChar* pEnd = ptr + length;
	// use target language punctuation characters in the extraction tests
	word1PrecPunct = SpanIncluding(ptr, pEnd, gpApp->m_punctuation[1]); 
	length = word1PrecPunct.Len();
	ptr += length;
	// next, get word1
	// BEW 28Jan11, deprecated dangerour SpanExcluding() to use 
	// ParseWordInwardsFromEnd() instead
	wxString wordBuildersForPostWordLoc;
	word1 = ParseWordInwardsFromEnd(ptr, pEnd, wordBuildersForPostWordLoc, 
									gpApp->m_punctuation[1]);
	if (!wordBuildersForPostWordLoc.IsEmpty())
	{
		// handle any punctuation characters the user has reverted to being word-building
		// ones by making a change to the target punctuation set in Preferences...
		word1 += wordBuildersForPostWordLoc;
		wordBuildersForPostWordLoc.Empty(); // so it won't interfere
											// with the second call below
	}
	length = word1.Len();
	ptr += length;
	// next, get following puncts from word1
	word1FollPunct = SpanIncluding(ptr, pEnd, gpApp->m_punctuation[1]); 
	length = word1FollPunct.Len();
	ptr += length;
	wxASSERT(*ptr == _T('~'));
	ptr += 1; // jump ~
	// next, get preceding puncts from word2
	word2PrecPunct = SpanIncluding(ptr, pEnd, gpApp->m_punctuation[1]); 
	length = word2PrecPunct.Len();
	ptr += length;
	// next, get word2
	// BEW 28Jan11, deprecated dangerour SpanExcluding() to use 
	// ParseWordInwardsFromEnd() instead
	word2 = ParseWordInwardsFromEnd(ptr, pEnd, wordBuildersForPostWordLoc,
									gpApp->m_punctuation[1]);
	if (!wordBuildersForPostWordLoc.IsEmpty())
	{
		// handle any punctuation characters the user has reverted to being word-building
		// ones by making a change to the target punctuation set in Preferences...
		word2 += wordBuildersForPostWordLoc;
		wordBuildersForPostWordLoc.Empty();
	}
	length = word2.Len();
	ptr += length;
	// next, get following puncts from word2
	word2FollPunct = SpanIncluding(ptr, pEnd, gpApp->m_punctuation[1]); 
	length = word2FollPunct.Len();
	ptr += length;
	// we have the decomposition finished, now rebuild with the inline binding markers in
	// their correct locations
	tgtStr.Empty();
	if (!word1PrecPunct.IsEmpty())
		tgtStr = word1PrecPunct;
	if (!pSPWord1->GetInlineBindingMarkers().IsEmpty())
		tgtStr += pSPWord1->GetInlineBindingMarkers();
	tgtStr += word1;
	if (!pSPWord1->GetInlineBindingEndMarkers().IsEmpty())
		tgtStr += pSPWord1->GetInlineBindingEndMarkers();
	if (!word1FollPunct.IsEmpty())
		tgtStr += word1FollPunct;
	tgtStr += FixedSpace; // ~
	if (!word2PrecPunct.IsEmpty())
		tgtStr += word2PrecPunct;
	if (!pSPWord2->GetInlineBindingMarkers().IsEmpty())
		tgtStr += pSPWord2->GetInlineBindingMarkers();
	tgtStr += word2;
	if (!pSPWord2->GetInlineBindingEndMarkers().IsEmpty())
		tgtStr += pSPWord2->GetInlineBindingEndMarkers();
	if (!word2FollPunct.IsEmpty())
		tgtStr += word2FollPunct;
	return tgtStr;
}

// if FALSE is returned, examine *pbIsEither value, if TRUE then the outcome was
// indeterminate (either set could be selected meaningfully), if FALSE, then PNG 1998 SFM
// set is indicated
bool IsUsfmDocument(SPList* pList, bool* pbIsEither)
{
	// by passing in the list, we could call this on a subrange, or call several times on
	// several subranges, etc; but normally we'll just make a single call on the
	// m_pSourcePhrases list and rely on the subranges set up herein (providing there are
	// enough instances, if not, we test every instance in the document)
	*pbIsEither = FALSE; // initialize, assuming the result will not be indeterminate
	int pngCount = 0;
	int usfmCount = 0;
	size_t endRange[] = {333, 667, 1000}; // to initialize, some may be changed below
	size_t startRange[] = {0, 330, 1000}; // to initialize, some may be changed below
	size_t maxIndex = pList->GetCount() - 1;
	bool bThreeRanges = TRUE;
	if (maxIndex < 1000)
	{
		bThreeRanges = FALSE;
	}
	SPList::Node* pos = pList->GetFirst();
	// set up the ranges, based on doc size (as number of CSourcePhrase instances)
	size_t rangeIndex;
	size_t rangeEndIndex;
	if (bThreeRanges)
	{
		size_t aDivision = maxIndex / 3;
		endRange[0] = wxMin(330,aDivision);
		startRange[1] = aDivision;
		startRange[2] = startRange[1] + aDivision;
		endRange[1] = wxMin(startRange[1] + 330, startRange[2] - 1);
		endRange[2] = wxMin(startRange[2] + 330, maxIndex);
		rangeEndIndex = 2;
	}
	else
	{
		endRange[0] = maxIndex;
		rangeEndIndex = 0;
	}
	for (rangeIndex = 0; rangeIndex <= rangeEndIndex; rangeIndex++)
	{
		size_t start = startRange[rangeIndex];
		size_t end = endRange[rangeIndex];
		size_t index = 0;
		for (index = start; index <= end; index++)
		{
			pos = pList->Item(index);
			CSourcePhrase* pSrcPhrase = pos->GetData();

			// count indicators of USFM
			size_t usfmIndicators = EvaluateMarkerSetForIndicatorCount(pSrcPhrase, UsfmOnly);
			usfmCount += usfmIndicators;
			// count indicators of PNG SFM 1998 marker set
			size_t pngIndicators = EvaluateMarkerSetForIndicatorCount(pSrcPhrase, PngOnly);
			pngCount += pngIndicators;
		}
	}

	// do the logic for working out which set it used for the markup, or which set
	// dominates if there is ambiguity
	if ((usfmCount == 0 && pngCount == 0) || (usfmCount == pngCount && usfmCount > 0))
	{
		// we have no diagnostics for either set, or we have some but equal counts from
		// each set (that means that either set would be okay, but the user should default
		// to USFM always unless there is a clear indication of dominance of PNG 1998 SFM
		// set in the document's data)
		*pbIsEither = TRUE;
		return FALSE;
	}
	if (pngCount > usfmCount)
	{
        // the 1998 PNG SFM set is the winner (there are very few clear indicators of PNG
        // SFM set (it's common markers are also markers within USFM), but USFM has LOTS of
        // unique indicators, so for PNG set to have a higher count is a huge indicator
        // that it is PNG 1998 SFM set that is the document's markup standard)
		*pbIsEither = FALSE;
		return FALSE;
	}
	// if we've not returned yet, then USFM is the winner	
	*pbIsEither = FALSE;
	return TRUE;
}

// this function does the grunt work for the bool IsUsfmDocument() function above
size_t EvaluateMarkerSetForIndicatorCount(CSourcePhrase* pSrcPhrase, enum SfmSet set)
{
	size_t count = 0;
	// These arrays are cleared each time passed in to GetSFMarkersAsArray()
	wxArrayString endMkrsArray; endMkrsArray.Clear(); 
	wxArrayString beginMkrsArray; beginMkrsArray.Clear();
	bool bNoProblems = TRUE;
	wxString aSpace = _T(" ");
	wxString str; // a scratch string in which to store string returned from an access function
	wxString mkr;
	wxString mkrPlusSpace; // use this for testing the fast access strings defined on 
						   // the app (see start of OnInit())
	size_t index;
	size_t arrayCount;
	switch (set)
	{
	case PngOnly:
		// test for indicators that the sfm set is the 1998 PNG SFM set, count any found
		{
			if (!pSrcPhrase->m_markers.IsEmpty())
			{
				bNoProblems = GetSFMarkersAsArray(pSrcPhrase->m_markers, beginMkrsArray);
			}
			if (bNoProblems)
			{
				arrayCount = beginMkrsArray.GetCount();
				if (arrayCount > 0)
				{
					for (index = 0; index < arrayCount; index++)
					{
						mkr = beginMkrsArray.Item(index);
						if (mkr != _T("\\f") && mkr != _T("\\x") && mkr != _T("\\fe"))
						{
							// those markers are in both SFM sets, so we are interested
							// only in markers which are not one of those
							mkrPlusSpace = mkr + aSpace;
							if (gpApp->m_pngIndicatorMarkers.Find(mkrPlusSpace) != wxNOT_FOUND)
							{
								// it's one of the diagnostic markers for PNG 1998 SFM set
								count++;
							}
						}
					}
				}
			}
			str = pSrcPhrase->GetEndMarkers();
			if (!str.IsEmpty())
			{
				bNoProblems = GetSFMarkersAsArray(str, endMkrsArray);
			}
			if (bNoProblems)
			{
				// the only endmarkers in PNG 1998 SFM set are \F and \fe, they are
				// aliases for each other, being footnote end markers
				arrayCount = endMkrsArray.GetCount();
				if (arrayCount > 0)
				{
					for (index = 0; index < arrayCount; index++)
					{
						mkr = endMkrsArray.Item(index);
						if (mkr == _T("\\F") || mkr == _T("\\fe"))
						{
							// it's a diagnostic marker for PNG 1998 SFM set
							count++;
						}
					}
				}
			}
		}
		break;
	default:
	case UsfmOnly:
		// test for indicators that the sfm set is USFM, count any found
		{
			// for USFM, Adapt It stores \f and \x and \fe in m_markers, and \f* and \x*
			// and \fe* in m_endMarkers; hence any content in any of the inline marker or
			// endmarker string members of CSourcePhrase won't be one of those, and will
			// constitute an indicator of USFM (we'll not bother to check for more than
			// one marker in such members, although two or more are possible though unlikely)
			// Also, since the inline markers come in marker/endmarker pairs, it is
			// sufficient to just test for the endmarkers - then the counts won't be
			// hugely inflated compared to the png sfm set's counts
			if (!pSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
			{
				count++;
			}
			if (!pSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
			{
				count++;
			}
			// test for some diagnostic beginmarkers
			if (!pSrcPhrase->m_markers.IsEmpty())
			{
				bNoProblems = GetSFMarkersAsArray(pSrcPhrase->m_markers, beginMkrsArray);
			}
			if (bNoProblems)
			{
				arrayCount = beginMkrsArray.GetCount();
				if (arrayCount > 0)
				{
					for (index = 0; index < arrayCount; index++)
					{
						mkr = beginMkrsArray.Item(index);
						if (mkr != _T("\\f"))
						{
                            // the \f marker is a begin-marker belonging to both SFM sets,
                            // so we are interested only in begin-markers which are not that
                            // one
							mkrPlusSpace = mkr + aSpace;
							if (gpApp->m_usfmIndicatorMarkers.Find(mkrPlusSpace) != wxNOT_FOUND)
							{
								// it's one of the diagnostic markers for USFM set
								count++;
							}
						}
					}
				}
			}
			// test for endmarkers with an asterisk, including \f* \x* and/or \fe* - these
			// are only in USFM, as are any other markers which end with asterisk (we are
			// looking in m_endMarkers, as we've already tested the inline endmarker
			// storage above)
			str = pSrcPhrase->GetEndMarkers();
			if (!str.IsEmpty())
			{
				bNoProblems = GetSFMarkersAsArray(str, endMkrsArray);
			}
			if (bNoProblems)
			{
				// the only endmarkers in PNG 1998 SFM set are \F and \fe, they are
				// aliases for each other, being footnote end markers
				arrayCount = endMkrsArray.GetCount();
				if (arrayCount > 0)
				{
					for (index = 0; index < arrayCount; index++)
					{
						mkr = endMkrsArray.Item(index);
						if (mkr == _T("\\f*") || mkr == _T("\\fe*") || mkr == _T("\\x*"))
						{
							// it's a diagnostic marker for USFM set
							count++;
						}
						else if (mkr[mkr.Len() - 1] == _T('*'))
						{
							// it ends with an asterisk, so it's diagnostic of USFM
							count++;
						}
					}
				}
			}
			// check out the m_filtereInfo member of pSrcPhrase too, because if \f ... \f*
			// is filtered, it may be the only indicator for USFM and we'd get an
			// indeterminate result without it, but changing to the PngOnly marker set
			// would give a malformed document if, in that set, the \f marker was
			// unfiltered - so we need to look in this member too
			if (!pSrcPhrase->GetFilteredInfo().IsEmpty())
			{
				wxArrayString filteredMarkers;		filteredMarkers.Clear();
				wxArrayString filteredEndMarkers;	filteredEndMarkers.Clear();
				wxArrayString filteredContent;		filteredContent.Clear();
				bool bIsOkay = pSrcPhrase->GetFilteredInfoAsArrays(&filteredMarkers, 
											&filteredEndMarkers, &filteredContent);
				if (bIsOkay)
				{
					// look for \f* or \x* or \fe* being endmarkers for filtered footnote,
					// or cross reference, or endnote - these are diagnostic of USFM
					size_t arrayCount2 = filteredEndMarkers.GetCount();
					if (arrayCount2 > 0)
					{
						size_t index2;
						for (index2 = 0; index2 < arrayCount2; index2++)
						{
							mkr = filteredEndMarkers.Item(index2);
							if (mkr == _T("\\f*") || mkr == _T("\\fe*") || mkr == _T("\\x*"))
							{
								// it's a diagnostic marker for USFM set
								count++;
							}
							// BEW 17Aug11, beware, mkr might be an empty string and the
							// test below, without an IsEmpty() to protect it, would then
							// lead to an index of -1 (huge) into the string - and wx will
							// trip an assert in string.h
							else if (!mkr.IsEmpty())
							{
								if (mkr[mkr.Len() - 1] == _T('*'))
								{
									// it ends with an asterisk, so it's diagnostic of USFM
									count++;
								}
							}
						}
					}
					// look for any filtered begin-markers diagnostic of USFM, other than
					// \f which is in both SFM sets
					size_t arrayCount3 = filteredMarkers.GetCount();
					if (arrayCount3 > 0)
					{
						size_t index3;
						for (index3 = 0; index3 < arrayCount3; index3++)
						{
							mkr = filteredMarkers.Item(index3);
							if (mkr != _T("\\f"))
							{
								// the \f marker is a begin-marker belonging to both SFM sets,
								// so we are interested only in begin-markers which are not that
								// one
								mkrPlusSpace = mkr + aSpace;
								if (gpApp->m_usfmIndicatorMarkers.Find(mkrPlusSpace) != wxNOT_FOUND)
								{
									// it's one of the diagnostic markers for USFM set
									count++;
								}
							}
						}
					}

				} // end of TRUE block for test: if (bIsOkay)
			}
		}
		break;
	}
	return count;
}

// returns nothing
// param   inStr       ->  a string of extracted filtered information (with \!FILTER and 
//                         \~FILTER* filter bracketing markers all removed)
// param   xrefStr     <-  reference to a string in which to return \x.....to....\x* inclusive
//                         or empty string if there is no cross reference info in inStr
// param   othersFilteredStr   <- reference to a string in which to return, concatenated,
//                         anything which precedes \x and all that follows \x*; but if xrefStr
//                         is empty, then othersFilteredStr contains a copy of inStr
// BEW created 11Oct10, for support of beefed up doc version 5 support of USFM
void SeparateOutCrossRefInfo(wxString inStr, wxString& xrefStr, wxString& othersFilteredStr)
{
	wxString mkrPlusSpace = _T("\\x ");
	wxString endMkr = _T("\\x*");
	xrefStr.Empty();
	othersFilteredStr.Empty();
	if (inStr.IsEmpty())
		return;
	int offset;
	wxString startStr;
	wxString endStr;
	offset = inStr.Find(mkrPlusSpace);
	if (offset == wxNOT_FOUND)
	{
		// there's not crossReference information in the input string, so send it all to
		// othersFilteredStr
		othersFilteredStr = inStr;
		return;
	}
	else
	{
		// there's crossReference material in the input string, so extract it, put it in
		// xrefStr and concatenate what precedes and follows and return in othersFilteredStr
		startStr = inStr.Left(offset);
		xrefStr = inStr.Mid(offset); // need to chop of everything from \x* onwards yet
		offset = xrefStr.Find(endMkr);
		wxASSERT(offset != wxNOT_FOUND);
		xrefStr = xrefStr.Left(offset + 3); // 3, in order to include \x* in the xrefStr
		endStr = xrefStr.Mid(offset + 3);
		endStr.Trim(FALSE); // trim left end
		othersFilteredStr = startStr + endStr;
	}
}

/// return      The recomposed source text string, including punctuation and markers, but
///             which members are included is controlled by booleans passed in
/// pSingleSrcPhrase        ->  the non-merged sourcephrase, or a ~ conjoined pair
/// bAttachFilteredInfo     ->  if TRUE, cross reference and other filtered info, if present
///                             are returned in the recomposed source text string (note,
///                             cross reference is returned after m_markers material, if
///                             any, and the remaining filtered material before m_markers
///                             material (if any))
/// bAttach_m_markers       ->  include any m_markers information in the returned string
///                             (sometimes the caller has already got the m_markers info
///                             and so we need to be able to suppress having it returned
///                             again)
/// mMarkersStr             <-  contents of m_markers member, unilaterally returned (or empty)
/// xrefStr                 <-  filtered \x .... \x* information, including markers, if
///                             any such is stored in the m_filteredInfo member; unilaterally
///                             returned
/// filteredInfoStr         <-  the contents of m_filteredInfo member, with any cross reference
///                             information (ie. \x ....\x*) removed; or an empty string if there
///                             is no filtered info. Unilaterally returned
/// BEW created 11Oct10 for support of additions to doc version 5 for better USFM support
wxString FromSingleMakeSstr(CSourcePhrase* pSingleSrcPhrase, bool bAttachFilteredInfo,
				bool bAttach_m_markers, wxString& mMarkersStr, wxString& xrefStr,
				wxString& filteredInfoStr, bool bDoCount, bool bCountInTargetText)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	SPList* pSrcPhrases = gpApp->m_pSourcePhrases;

	// is it normal instance, or one which stores a word pair conjoined with USFM fixed
	// space symbol ~  ?
	bool bIsFixedSpaceConjoined = IsFixedSpaceSymbolWithin(pSingleSrcPhrase);
	wxString Sstr;
	// Clear next three ready for returning what we find to caller (caller may use any or
	// all of it, or decline it, but we return it anyway via the signature), the booleans
	// in the signature control whether the filtered stuff and m_markers stuff is to be
	// included in the returned string or not.
	xrefStr.Empty();
	filteredInfoStr.Empty();
	mMarkersStr.Empty();

    // store here any string of filtered information stored on pSingleSrcPhrase (in any or
    // all of m_freeTrans, m_note, m_collectedBackTrans, m_filteredInfo)
	wxString markersPrefix; markersPrefix.Empty();

	wxString finalSuffixStr; finalSuffixStr.Empty(); // put collected-string-final endmarkers here

	wxString aSpace = _T(" ");
	wxString markersStr;
	wxString endMarkersStr;
	wxString freeTransStr;
	wxString noteStr;
	wxString collBackTransStr;
	wxString filteredInfoStr2;

	// empty the scratch strings
	EmptyMarkersAndFilteredStrings(markersStr, endMarkersStr, freeTransStr, noteStr,
									collBackTransStr, filteredInfoStr2);
	// get the other string information we want, putting it in the scratch strings
	GetMarkersAndFilteredStrings(pSingleSrcPhrase, markersStr, endMarkersStr,
					freeTransStr, noteStr, collBackTransStr,filteredInfoStr2);
	// remove any filter bracketing markers if filteredInfoStr2 has content
	if (!filteredInfoStr2.IsEmpty())
	{
		filteredInfoStr2 = pDoc->RemoveAnyFilterBracketsFromString(filteredInfoStr2);

		// separate out any cross reference information - it must be placed following
		// information in m_markers, if the caller wants it; other filtered info is to be
		// placed preceding m_markers, if the caller wants it
		SeparateOutCrossRefInfo(filteredInfoStr2, xrefStr, filteredInfoStr);
	}
	mMarkersStr = markersStr; // unilaterally returned to caller, in case it wants it

	if (bAttachFilteredInfo)
	{

		// for the one and only CSourcePhrase, we store any filtered info within the prefix
		// string, and any content in m_markers, if present, must be put at the start
		// of Sstr if filtered info is xref info (which comes after m_markers), but if there
		// is other filtered info, it precedes m_markers info; remove LHS whitespace when done
		markersPrefix = GetUnfilteredInfoMinusMMarkersAndCrossRefs(pSingleSrcPhrase,
							pSrcPhrases, filteredInfoStr, collBackTransStr,
							freeTransStr, noteStr, bDoCount, bCountInTargetText); // m_markers 
								// and xrefStr are handled in a separate function, later below
		 
	} // end of TRUE block for test: if (bAttachFilteredInfo)
	else
	{
		// caller does not want filtered information in the returned source text string,
		// but that still leaves m_markers content, if the caller wants it - do it below
		;
	}
	markersPrefix.Trim(FALSE); // finally, remove any LHS whitespace
	// make sure it ends with a space
	markersPrefix.Trim();
	markersPrefix << aSpace;

    // BEW 11Oct10, for ~ conjoining, we assume that between the two conjoined words will
    // only be ~ plus or minus following punctuation on the first, plus or minus preceding
    // punctuation on the second; but we also allow inline binding markers on either word.
    // For pair-initial and pair final locations, we assume that there can be the full
    // complement of punctuation and markers also, and we'll allow an inline binding marker
    // / endmarker provided that the first word has a beginmarker, and the second word has
    // an endmarker (but not necessarily a matching pair).
	// The task now, whether for conjoined pairs or a normal single source text word, is to 
	// build up a srcStr which has (1) punctuation (2) inline binding mrk & endmkr if
	// present, and the key (the building is more complex for a conjoined pair, but is
	// still determinate so no user help is needed in this step). After we have srcStr, we
	// can then proceed to test for and add other markers and any m_follOuterPunct content.
	wxString theSymbol = _T("~");
	CSourcePhrase* pSP = pSingleSrcPhrase; // RHS is too long to type all the time
	wxString srcStr = pSP->m_key;
	if (bIsFixedSpaceConjoined)
	{
		CSourcePhrase* pWord1;
		CSourcePhrase* pWord2;
		wxString wrd1;
		wxString wrd2;
		int offset = srcStr.Find(theSymbol);
		wxASSERT(offset != wxNOT_FOUND);
		wrd1 = srcStr.Left(offset);
		wrd2 = srcStr.Mid(offset + 1);
		SPList* pSrcPhrases = pSP->m_pSavedWords;
		SPList::Node* posFirst = pSrcPhrases->GetFirst();
		SPList::Node* posLast = pSrcPhrases->GetLast();
		pWord1 = posFirst->GetData();
		pWord2 = posLast->GetData();
		// what comes nearest the word is preceding and following inline binding marker
		// and endmarker, if they exist here
		if (!pWord1->GetInlineBindingMarkers().IsEmpty())
		{
			wrd1 = pWord1->GetInlineBindingMarkers() + wrd1;
		}
		if (!pWord1->GetInlineBindingEndMarkers().IsEmpty())
		{
			wrd1 += pWord1->GetInlineBindingEndMarkers();
		}
		if (!pWord1->m_follPunct.IsEmpty())
		{
			wrd1 += pWord1->m_follPunct;
		}
		// what comes nearest the word2 is preceding and following inline binding marker
		// and endmarker, if they exist here
		if (!pWord2->GetInlineBindingMarkers().IsEmpty())
		{
			wrd2 = pWord2->GetInlineBindingMarkers() + wrd2;
		}
		if (!pWord2->GetInlineBindingEndMarkers().IsEmpty())
		{
			wrd2 += pWord2->GetInlineBindingEndMarkers();
		}
		if (!pWord2->m_precPunct.IsEmpty())
		{
			wrd2 = pWord2->m_precPunct + wrd2;
		}
		// form the composite, after making sure no spurious spaces are in the substrings
		wrd1.Trim(); // trim white space from the end of wrd1
		wrd2.Trim(FALSE); // trim white space from the start of wrd2
		srcStr = wrd1 + theSymbol + wrd2;
	}
	else
	{
		// it's a normal single CSourcePhrase instance
		if (!pSP->GetInlineBindingMarkers().IsEmpty())
		{
			srcStr = pSP->GetInlineBindingMarkers() + srcStr;
		}
		if (!pSP->GetInlineBindingEndMarkers().IsEmpty())
		{
			srcStr += pSP->GetInlineBindingEndMarkers();
		}
	}
    // the punctuation situation at either end is in common regardless of which block was
    // processed above
	if (!pSP->m_precPunct.IsEmpty())
	{
		srcStr = pSP->m_precPunct + srcStr;
	}
	if (!pSP->m_follPunct.IsEmpty())
	{
		srcStr += pSP->m_follPunct;
	}
	// we have now got the srcStr base that we need, now do the rest - which is the same
	// whether we have a word pair ~ conjoined or not
	Sstr = srcStr;

	// check for inline non-binding marker, it follows \v etc, and so comes next as we
	// work outwards in a left direction
	if (!pSP->GetInlineNonbindingMarkers().IsEmpty())
	{
		Sstr = pSP->GetInlineNonbindingMarkers() + Sstr;
	}

	// now precede what we have with any beginmarkers and associated data from m_markers,
	// and if there is content in xrefStr (and bAttachFilteredInfo is TRUE) then put that
	// content after the markersStr (ie. m_markers) content -- append this stuff to
	// markersPrefix and then we can prefix the latter to Sstr
	markersPrefix = GetUnfilteredCrossRefsAndMMarkers(markersPrefix, markersStr, xrefStr,
												bAttachFilteredInfo, bAttach_m_markers);

    // any endmarkers on pSingleSrcPhrase are not "medial", and can be added
    // automatically too
	if (!endMarkersStr.IsEmpty())
	{
		Sstr << endMarkersStr;
	}

	// there might be content in m_follOuterPunct, append it next
	if (!pSP->GetFollowingOuterPunct().IsEmpty())
	{
		Sstr += pSP->GetFollowingOuterPunct();
	}

	// finally, there could be an inline non-binding endmarker, like \wj* (words of Jesus)
	if (!pSP->GetInlineNonbindingEndMarkers().IsEmpty())
	{
		Sstr += pSP->GetInlineNonbindingEndMarkers();
	}

    // now add the prefix string material if it is not empty
	if (!markersPrefix.IsEmpty())
	{
		markersPrefix.Trim();
		markersPrefix << aSpace; // ensure a final space
		Sstr = markersPrefix + Sstr;
	}
	Sstr.Trim(FALSE);
	Sstr.Trim(); // don't return it with a final space, leave that to the caller
	return Sstr;
}


// the next 3 functions are similar or identical to member functions of the document class
// which are used in the parsing of text files to produce a document; one (ParseMarker())
// is a modification of the one in the doc class; these are needed here for use in
// CSourcePhrase - later we could replace IsWhiteSpace() and ParseWhiteSpace() in the doc
// class with these ( *** TODO *** ?)
/*
// deprecated, copy the one from doc which now has support for 16 extra exotic spaces &
// joiners - as requested by Dennis Drescher in email 3Aug11
bool IsWhiteSpace(const wxChar *pChar)
{
#ifdef _UNICODE
	// BEW 29Jul11, support ZWSP (zero-width space character, U+200B) as well, and from
	// Ad Korten's email, also hair space, thin space, and zero width joiner (he may
	// specify one or two others later -- if so add them only here)
	wxChar ZWSP = (wxChar)0x200B; // ZWSP
	wxChar THSP = (wxChar)0x2009; // THSP is "THin SPace
	wxChar HSP = (wxChar)0x200A; // HSP is "Hair SPace" (thinner than THSP)
	wxChar ZWJ = (wxChar)0x200D; // ZWJ is "Zero Width Joiner"
	if (*pChar == ZWSP || *pChar == THSP || *pChar == HSP || *pChar == ZWJ)
		return TRUE;
#endif
	// returns true for tab 0x09, return 0x0D or space 0x20
	if (wxIsspace(*pChar) == 0)// _istspace not recognized by g++ under Linux
		return FALSE;
	else
		return TRUE;
}
*/
bool IsWhiteSpace(const wxChar *pChar)
{
#ifdef _UNICODE
	wxChar NBSP = (wxChar)0x00A0; // standard Non-Breaking SPace
#else
	wxChar NBSP = (unsigned char)0xA0;  // standard Non-Breaking SPace
#endif
	// handle common ones first...
	// returns true for tab 0x09, return 0x0D or space 0x20
	// _istspace not recognized by g++ under Linux; the wxIsspace() fn and those it relies
	// on return non-zero if a space type of character is passed in
	if (wxIsspace(*pChar) != 0 || *pChar == NBSP)
	{
		return TRUE;
	}
	else
	{
#ifdef _UNICODE
		// BEW 3Aug11, support ZWSP (zero-width space character, U+200B) as well, and from
		// Dennis Drescher's email of 3Aug11, also various others
		// BEW 4Aug11 changed the code to not test each individually, but just test if
		// wxChar value falls in the range 0x2000 to 0x200D - which is much quicker; and
		// treat U+2060 individually
		wxChar WJ = (wxChar)0x2060; // WJ is "Word Joiner"
		if (*pChar == WJ || ((UInt32)*pChar >= 0x2000 && (UInt32)*pChar <= 0x200D))
		{
			return TRUE;
		}
#endif
	}
	return FALSE;
}


int ParseWhiteSpace(const wxChar *pChar)
{
	int	length = 0;
	wxChar* ptr = (wxChar*)pChar;
	while (IsWhiteSpace(ptr))
	{
		length++;
		ptr++;
	}
	return length;
}
// BEW 11Oct10, modified the algorithm because when parsing *f\ it returns incorrect 2
// rather than correct 3; the solution I adopted is to look at what pChar points at on
// input, if at a backslash, then the test can include *; if at * we assume the marker is
// reversed and add 1 to what Bill's original code came up with; ] is also a halt location
// BEW 8Dec10 -- kept getting failures because when a reversed string was passed in it
// often didn't match with the original assumption that * would be at its start. So I've
// tried to make it smarter, and if the situation is really indeterminate, return a value
// of zero.
int ParseMarker(const wxChar *pChar)
{
	// this algorithm differs from the one in CAdapt_ItDoc class by not having the code to
	// break immediately after an asterisk; we don't want the latter because this function
	// will be used to parse over a reversed string, and a reversed endmarker will have an
	// initial asterisk and we don't want to halt the loop there
	
    // BEW 8Dec10, it just isn't safe to assume that if the unreversed input didn't have a
    // * at the end of the string buffer, then it must be an unreversed string that was
    // input and that backslash starts it. If the SFM set is PngOnly, there could be a
    // final \fe or \F with a space following, and reversing that would mean that space
    // starts the string. Or there could be embedded binding markers and finding an
    // endmarker may find one of those and it won't be at the string end necessarily, etc
    // etc. I'm very uncomfortable with this function, it is dangerous if not used
    // appropriately. It currently is called (once in helpers.cpp and 3 times in xml.cpp,
    // and in the doc function, ParseSpanBackwards()). We may just manage to get away with
    // using it - the places where it is used don't have to deal with reversed PngOnly
    // marker set's markers I think
	int len = 0;
	wxChar* ptr = (wxChar*)pChar;
	wxChar* pBegin = ptr;
	if (*ptr == _T('*'))
	{
		// assume it is a reversed endmarker
		while (!IsWhiteSpace(ptr) && *ptr != _T('\0') && 
				gpApp->m_forbiddenInMarkers.Find(*ptr) == wxNOT_FOUND)
		{
			if (ptr != pBegin && *ptr == gSFescapechar)
			{
				if (*ptr == gSFescapechar)
				{
					// we must count the backslash too
					len++;
				}
				break;
			}
			ptr++;
			len++;
		}
		if ( gpApp->m_forbiddenInMarkers.Find(*ptr) != wxNOT_FOUND)
		{
			// we found a forbidden character for a marker before finding the backslash,
			// so this can't be a SFM or USFM or even an unknown marker, so return zero
			len = 0;
		}
	}
	else
	{
		// let's hope the caller really gets it right its an SF marker here, because we
		// don't return 0 if it isn't - I'll add an assert to maybe catch the problem
		// during development
		//wxASSERT(*pChar == gSFescapechar);
		// Our first check needs to be for an initial backslash, it that succeeds, we've
		// an unreversed string passed in and we can just parse the marker that starts it.
		// If we don't have an initial marker, we've got a mess we have to do something
		// with. We won't really know - this uncertainty in knowing whether it was
		// reversed or not calls for a change - perhaps pass in a boolean which tells us
		// whether or not the input string was reversed.
		if (*pChar == gSFescapechar)
		{
			ptr++; // get past the initial backslash
			len++;
			while (!IsWhiteSpace(ptr) && *ptr != gSFescapechar && *ptr != _T('\0') && *ptr != _T(']')
					&& gpApp->m_forbiddenInMarkers.Find(*ptr) == wxNOT_FOUND)
			{
				if (*ptr == _T('*'))
				{
					// we must count the asterisk too
					len++;
					break;
				}
				ptr++;
				len++;
			}
		}
		else // neither expectation has been met
		{
			// Connundrum. What do we assume here? We'll assume a reversed string, as that
			// is more likely to have got us here. I'll check for an initial space, and if
			// so, assume its a PngOnly SFM endmarker, and just parse to the backslash (it
			// must be either "<space>F\...." or "<space>ef\...." we are dealing with -
			// anything else isn't kosher for the PngOnly set of markers.
			if (gpApp->gCurrentSfmSet == PngOnly && *pBegin == _T(' '))
			{
				if (*(pBegin + 1) == _T('F') && *(pBegin + 2) == gSFescapechar)
					return 3; // include the delimiting space in the marker string
				if (*(pBegin + 1) == _T('e') && *(pBegin + 2) == _T('f') && *(pBegin + 3) == gSFescapechar) 
					return 4;
			}
			// if not a reversed "\fe " or reversed "\F ", what else might we be dealing
			// with? Dunno. We'll just assume there's no actual endmarker, and return a
			// length of 0 and let the caller deal with it
			return 0;
		}
	}
	return len;
}

// BEW created 20Jan11, to avoid adding duplicates of ints already in the passed in
// wxArrayInt, whether or not keep_strips_keep_piles is used for RecalcLayout() - the
// contents won't be used if another layout_selector enum valus is in effect, as
// RecalcLayout() would recreate the strips and repopulate the partner piles in such
// situations 
void AddUniqueInt(wxArrayInt* pArrayInt, int nInt)
{
	int count = pArrayInt->GetCount();
	if (count == 0)
	{
		pArrayInt->Add(nInt);
	}
	else
	{
		int index = pArrayInt->Index(nInt);
		if (index == wxNOT_FOUND)
		{
			// it's not in there yet, so Add() it
			pArrayInt->Add(nInt);
		}
		else
		{
			// it's in the array already, so ignore it
			return;
		}
	}
}

// Any strings in pPossiblesArray not already in pBaseStrArray, append them to
// pBaseStrArray, return TRUE if at least one was added, FALSE if none were added
// BEW 11Oct10, added param bExcludeDuplicates, with default FALSE (which means everything
// is accepted whether a duplicate or not)
bool AddNewStringsToArray(wxArrayString* pBaseStrArray, wxArrayString* pPossiblesArray,
						  bool bExcludeDuplicates)
{
	bool bAddedSomething = FALSE;
	int possIndex;
	int possCount = pPossiblesArray->GetCount();
	for (possIndex = 0; possIndex < possCount; possIndex++)
	{
		wxString aString = pPossiblesArray->Item(possIndex);
		if (bExcludeDuplicates)
		{
			int nFoundIndex = pBaseStrArray->Index(aString); // uses case sensitive compare
			if (nFoundIndex == wxNOT_FOUND)
			{
				// add this one to pBaseStrArray
				pBaseStrArray->Add(aString);
				bAddedSomething = TRUE;
			}
		}
		else
		{
			// accept all comers
			pBaseStrArray->Add(aString);
			bAddedSomething = TRUE;
		}	
	}
	return bAddedSomething;
}

// BEW 12Jan11, in first test, changed pSrcPhrase->m_bHasFreeTrans to use
// pSrcPhrase->m_bStartFreeTrans instead, because the CSourcePhrase on which the latter
// flag is TRUE is the one which does the storage of the filtered information, in
// particular the free translation if there is one (using the other flag would give a TRUE
// returned when a CSourcePhrase instance within the free translation was tested, even
// though it had no filtered info stored on it)
bool HasFilteredInfo(CSourcePhrase* pSrcPhrase)
{
	if (pSrcPhrase->m_bStartFreeTrans || !pSrcPhrase->GetFreeTrans().IsEmpty())
	{
		return TRUE;
	}
	if (pSrcPhrase->m_bHasNote || !pSrcPhrase->GetNote().IsEmpty())
	{
		return TRUE;
	}
	if (!pSrcPhrase->GetCollectedBackTrans().IsEmpty())
	{
		return TRUE;
	}
	if (!pSrcPhrase->GetFilteredInfo().IsEmpty())
	{
		return TRUE;
	}
	return FALSE;
}

// determines if nFirstSequNum up to nFirstSequNum + nCount - 1 all lie within a
// retranslation; if TRUE, then also returns the first and last sequence numbers for the
// retranslation in the last 2 parameters; these parameters are not defined if FALSE is
// returned
bool IsContainedByRetranslation(int	nFirstSequNum,
												int	nCount,
												int& nSequNumFirst,
												int& nSequNumLast)
{
	wxASSERT(!gbIsGlossing); // when glossing this should never be called
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp != NULL);
	SPList* pList = pApp->m_pSourcePhrases;
	wxASSERT(pList != NULL);
	CSourcePhrase* pSrcPhrase;

	SPList::Node* pos = pList->Item(nFirstSequNum);
	wxASSERT(pos != NULL);
	int count = 0;
	bool bFoundEnd = FALSE;
	while(pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase != NULL);
		count++;
		if (pSrcPhrase->m_bEndRetranslation)
			bFoundEnd = TRUE; // next iteration will go out of retranslation,
		// or into a following one
		if (!pSrcPhrase->m_bRetranslation ||
			(pSrcPhrase->m_bBeginRetranslation && bFoundEnd))
		{
			return FALSE;
		}
		if (count >= nCount)
			break;
	}

	// lies within the retranslation, so get the bounds
	nSequNumFirst = nFirstSequNum;
	int nFirst = nFirstSequNum+1;
	pos = pList->Item(nFirstSequNum);
a:	pSrcPhrase = (CSourcePhrase*)pos->GetData();
	pos = pos->GetPrevious();
	if (pSrcPhrase->m_bRetranslation && !pSrcPhrase->m_bEndRetranslation)
	{
		nSequNumFirst = --nFirst;
		goto a;
	}
	nSequNumLast = nFirstSequNum + count - 1;
	int nLast = nFirstSequNum + count - 1;
b:	nLast += 1;
	pos = pList->Item(nLast);
	pSrcPhrase = (CSourcePhrase*)pos->GetData();
	pos = pos->GetNext();
	if (pSrcPhrase->m_bRetranslation && !pSrcPhrase->m_bBeginRetranslation)
	{
		nSequNumLast = nLast;
		goto b;
	}

	return TRUE;
}

// BEW 16Feb10, no changes needed for support of doc version 5
bool IsNullSrcPhraseInSelection(SPList* pList)
{
	CSourcePhrase* pSrcPhrase;
	SPList::Node* pos = pList->GetFirst();
	if (pos == NULL)
		return FALSE; // there isn't any selection
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		if (pSrcPhrase->m_bNullSourcePhrase)
			return TRUE;
	}
	return FALSE;
}

// BEW 16Feb10, no changes needed for support of doc version 5
bool IsRetranslationInSelection(SPList* pList)
{
	CSourcePhrase* pSrcPhrase;
	SPList::Node* pos = pList->GetFirst();
	if (pos == NULL)
		return FALSE; // there isn't any selection
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		if (pSrcPhrase->m_bRetranslation)
			return TRUE;
	}
	return FALSE;
}

// BEW 11Oct10, for support of doc version 5
bool IsFixedSpaceSymbolInSelection(SPList* pList)
{
	//wxString theSymbol = _T("~"); // USFM fixedspace symbol
	CSourcePhrase* pSrcPhrase;
	SPList::Node* pos = pList->GetFirst();
	if (pos == NULL)
		return FALSE; // there isn't any selection
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		if (IsFixedSpaceSymbolWithin(pSrcPhrase))
			return TRUE;
	}
	return FALSE;
}

// BEW 11Oct10, for support of doc version 5
bool IsFixedSpaceSymbolWithin(CSourcePhrase* pSrcPhrase)
{
	wxString theSymbol = _T("~"); // USFM fixedspace symbol
	if (pSrcPhrase == NULL)
	{
		return FALSE; // there isn't an instance
	}
	else
	{
		if (pSrcPhrase->m_key.Find(theSymbol) != wxNOT_FOUND)
			return TRUE;
	}
	return FALSE;
}

bool IsFixedSpaceSymbolWithin(wxString& str)
{
	wxString theSymbol = _T("~"); // USFM fixedspace symbol
	if (str.IsEmpty())
		return FALSE;
	if (str.Find(theSymbol) != wxNOT_FOUND)
	{
		return TRUE;
	}
	return FALSE;
}

// uuid support
wxString GetUuid()
{
	// it is returned as "hhhhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhhhh"
	// (32 hex digits plus 4 hyphens = total 36 chars followed by null char (37th))
	wxString anUuid;
	Uuid_AI* pUuidGen = new Uuid_AI(); // generates the UUID
	anUuid = pUuidGen->GetUUID();
	delete pUuidGen;
	//wxLogDebug(_T("UUID =    %s"), anUuid);
	return anUuid;
}

// support getting current date-time in format "YYYY:MM:DD hh:mm:ssZ", which is useful as a
// potential sort key; that is: "year:month:day hours:minutes:seconds" Z suffix is 'Zulu
// time zone', that is, UTC (GMT).
// BEW created 10May10, modified 9July10 to make it be UTC time, and adding Z suffix
// BEW note, 26Aug10: Paratext normalizes to UTC, by converting to Zulu time, and it's
// formatted datetime has no space between date and time, and a capital Z following
// without a space delimiter. OXES, however, uses as much as the creating application
// wants to send - so we will send date and time, but not fractions of a second, and the
// standard for the date format separator is hyphen, and colon for the time delimiter, and
// there should be a single T between date and time if both are present.
wxString GetDateTimeNow(enum AppPreferedDateTime dt)
{
	wxDateTime theDateTime = wxDateTime::Now();
	// Adapt It and Paratext want it as UTC datetime, but OXES accepts as much as we want
	// to give, but UTC with offset +HH:SS or -HH:SS if we want to include the latter.
	if (dt == adaptItDT || dt == paratextDT)
	{
		// param noDST is default false, so it does daylight savings time adjustment too
		theDateTime = theDateTime.ToUTC(); 
	}									   
	wxString dateTimeStr;
	// each app or standard wants something a bit different
	switch (dt)
	{
	case paratextDT:
		{
			dateTimeStr = theDateTime.Format(_T("%Y-%m-%dT%H:%M:%SZ")).c_str();
		}
		break;
	case oxesDT:
		{
			// I'm giving OXES local time, but this can be changed if the TE team want
			dateTimeStr = theDateTime.Format(_T("%Y-%m-%dT%H:%M:%S")).c_str();
		}
		break;
	case oxesDateOnly:
		{
			// I'm giving OXES local timezone's date, but this can be changed if the TE team want
			dateTimeStr = theDateTime.Format(_T("%Y-%m-%d")).c_str(); // chop off time spec
		}
		break;
	default:
	case adaptItDT:
		{
			dateTimeStr = theDateTime.Format(_T("%Y-%m-%dT%H:%M:%SZ")).c_str();
		}
		break;
	}
	return dateTimeStr;
}

// for KB metadata, to get a string indicating who supplied the adaptation (or gloss)
// for KB metadata, to create a string indicating who supplied the adaptation (or gloss)
// For LAN-based collaboration, we will try set the string: "userID:machineID" such as
// watersb:BEW for the "watersb" account on the computer visible on the LAN as "BEW"; and
// anticipating the future when we hope to have both web-based KB sharing, and / or
// AI-in-the-cloud, we'll want to store something different - such as (at least) an IP
// address, so we'll have a bool parameter to distinguish the web versus non-web sourcing
// possibilities
// BEW created 10May10
wxString SetWho(bool bOriginatedFromTheWeb)
{
	wxString strUserID;
	wxString strMachineID;
	wxString strIDFromWeb;
	if (bOriginatedFromTheWeb)
	{
		// *** TODO ***  when our design has matured

		strIDFromWeb = _T("0000:0000:0000:0000"); // give it garbage for now
		return strIDFromWeb;
	}
	else
	{
		strUserID = ::wxGetUserId(); // returns empty string if unsuccessful
		if (strUserID.IsEmpty())
		{
			// we must supply a default if nothing was returned
			strUserID = _T("unknownUser");
		}
		wxString strMachineID = ::wxGetHostName(); // returns empty string if not found
		if (strMachineID.IsEmpty())
		{
			// we must supply a default if nothing was returned
			strMachineID = _T("unnamedMachine");
		}
		wxString whoStr;
		whoStr = whoStr.Format(_T("%s:%s"),strUserID.c_str(),strMachineID.c_str()).c_str();
		return whoStr;
	}
}

/* don't need this, it's legal to use wxFile::Length() and assign result to size_t
size_t GetFileSize_t(wxString& absPathToFile)
{
	size_t len = 0;
	wxFile f;
	bool bOpened = f.Open(absPathToFile,wxFile::read);
	if (!bOpened)
	{
		return len;
	}
	wxFileOffset start = 0;
	wxFileOffset end = 0;
	end = f.SeekEnd();
	// if non-Windows compilers complain about assigning the difference between two
	// wxFileOffset values (these are type wxLongLong_t) to a size_t, then comment out the
	// len = end - start; line, and uncomment out the following loop
	//wxFileOffset index;
	//for (index = start; index < end; index++)
	//{
	//	len++;
	//}
	len = end - start; // this will work fine for all filesizes AI will encounter
	f.Close();
	return len;
}
*/

// test that the file is suitable for creating an Adapt It adaptation document (we'll
// believe that any extensions are reliable indicators of the file contents, for those
// without extensions we'll use the tellenc.cpp function tellenc2()
// (some tests of it are at Adapt_It.cpp about line 7783, using files in C:\Card1 folder)
// BEW 16Aug11, changed to allow ucs-4le (little-endian) and ucs-4 (big-endian) as well
bool IsLoadableFile(wxString& absPathToFile)
{
	wxArrayString illegalExtensions;
	wxString extn = _T("doc"); // legacy Word
	illegalExtensions.Add(extn);
	extn = _T("docx"); // Word 7
	illegalExtensions.Add(extn);
	extn = _T("rtf");
	illegalExtensions.Add(extn);
	extn = _T("odt");
	illegalExtensions.Add(extn);
	extn = _T("html");
	illegalExtensions.Add(extn);
	extn = _T("htm");
	illegalExtensions.Add(extn);
	extn = _T("xhtml");
	illegalExtensions.Add(extn);
	extn = _T("xml");
	illegalExtensions.Add(extn);
	extn = _T("aic");
	illegalExtensions.Add(extn);
	extn = _T("KB"); // the old legacy Adapt It's binary knowledge base file extension
	illegalExtensions.Add(extn);
	extn = _T("adt"); // the old legacy Adapt It's binary doc extension
	illegalExtensions.Add(extn);
	extn = _T("aip"); // Adapt It's packed document extension
	illegalExtensions.Add(extn);
	extn = _T("lift");
	illegalExtensions.Add(extn);
	extn = _T("oxes");
	illegalExtensions.Add(extn);
	extn = _T("osis");
	illegalExtensions.Add(extn);
	extn = _T("xls"); // Excel
	illegalExtensions.Add(extn);
	extn = _T("xsl"); // for xhtml formatting
	illegalExtensions.Add(extn);
	extn = _T("pub"); // Publisher
	illegalExtensions.Add(extn);
	extn = _T("ppt"); // Powerpoint
	illegalExtensions.Add(extn);
	extn = _T("pps"); // Powerpoint show
	illegalExtensions.Add(extn);
	extn = _T("mdb"); // MS Access
	illegalExtensions.Add(extn);

	wxArrayString legalExtensions;
	extn = _T("txt");
	legalExtensions.Add(extn);
	extn = _T("sfm");
	legalExtensions.Add(extn);
	extn = _T("sfc");
	legalExtensions.Add(extn);
	extn = _T("ptx");
	legalExtensions.Add(extn);

	wxFileName fn(absPathToFile);
	wxString fullName = fn.GetFullName(); // file title & extension
	wxString extension = fn.GetExt(); // gets extension without preceding .
	fullName = fullName.Lower();
	extension = extension.Lower();

	// exclude any files commencing with a period
	if (fullName.GetChar(0) == _T('.'))
		return FALSE;

	// check the known legals first
	int index;
	int max = legalExtensions.GetCount();
	bool bIsLegal = FALSE;
	for (index = 0; index < max; index++)
	{
		// if one of the legal extensions matches, then declare it legal
		if (extension == legalExtensions.Item(index))
		{
			bIsLegal = TRUE;
		}
	}
	if (bIsLegal) return TRUE;

	// if not one of the legals, check for illegals
	bool bIsIllegal = FALSE;
	max = illegalExtensions.GetCount();
	for (index = 0; index < max; index++)
	{
		// if one of the illegal extensions matches, then declare it illegal
		if (extension == illegalExtensions.Item(index))
		{
			bIsIllegal = TRUE;
		}
	}
	if (bIsIllegal) return FALSE;

	// if not one of our known illegals, have a look inside the file
	// (use CBString, which is for single-byte strings, and suits use of tellenc.cpp)
	wxFile f;
	bool bOpened = f.Open(absPathToFile,wxFile::read);
	if (!bOpened)
	{
		// don't expect this error, so use English message
		wxMessageBox(_T("IsLoadableFile() could not open file. File will be treated as non-loadable."),
			_T("Error"),wxICON_WARNING);
		return FALSE;
	}
	//size_t len = GetFileSize_t(absPathToFile); // not needed
	size_t len = f.Length(); // it's legal to assign to size_t
	wxMemoryBuffer* pBuffer = new wxMemoryBuffer(len + 2);
	// ensure the last two bytes are nulls, and create acceptable pointers for calls below
	char* ptr = (char*)pBuffer->GetData();
	const unsigned char* const saved_ptr = (const unsigned char* const)ptr;
	*(ptr + len) = '\0';
	*(ptr + len + 1) = '\0';
	// get the file opened and read it into a memory buffer
	size_t numRead = f.Read(ptr,len);
	if (numRead < len)
	{
		// don't expect this error, so use English message
		wxMessageBox(_T("IsLoadableFile() read in less then all of the file. File will be treated as non-loadable."),
			_T("Error"),wxICON_WARNING);
		delete pBuffer;
		f.Close();
		return FALSE;
	}
	// now find out what the file's data is
	CBString resultStr;
	resultStr.Empty();
	// GDLC Removed conditionals for PPC Mac (with gcc4.0 they are no longer needed)
	resultStr = tellenc2(saved_ptr, len); // xml files are returned as "binary" too
										  // so hopefull html files without an extension
										  // will likewise be "binary" & so be rejected

	f.Close();
	// check it's not xml
	bool bIsXML = FALSE;
	char cstr[6] = {'\0','\0','\0','\0','\0','\0'};
	cstr[0] = *(ptr + 0);
	cstr[1] = *(ptr + 1);
	cstr[2] = *(ptr + 2);
	cstr[3] = *(ptr + 3);
	cstr[4] = *(ptr + 4);
	CBString aStr = cstr;
	if (aStr == "<?xml")
	{
		bIsXML = TRUE;
	}
	delete pBuffer;

	if (bIsXML || resultStr == "binary" || resultStr == "ucs-4" || resultStr == "ucs-4le")
	{
		return FALSE;
	}
	// this means it's either utf-8, or utf-16 (big endian), or utf-16le (little endian)
	// or a single-byte encoding (or a multibyte one)
	return TRUE;
}

bool IsLittleEndian(wxString& theText)
{
	// wchar_t is 2 bytes in Windows, 4 in Unix; we support non-Unicode app only for
	// Windows, which is low-endian 
	bool bIsLittleEndian = TRUE;
	unsigned int len;
	len = theText.Len();
#ifdef _UNICODE
	unsigned int size_in_bytes = len * sizeof(wxChar); // len*2 or len*4
	const wxChar* pUtf16Buf = theText.GetData();
	char* ptr = (char*)pUtf16Buf;
	const unsigned char* const pCharBuf = (const unsigned char* const)ptr;
	CBString resultStr;
	resultStr.Empty();
	resultStr = tellenc2(pCharBuf, size_in_bytes);
	if (resultStr == "utf-16" || resultStr == "ucs-4")
	{
		// this is running on a big-endian machine
		bIsLittleEndian = FALSE;
	}
#endif
	return bIsLittleEndian;
}



/* works, but should only be used for small files
// returns TRUE if it succeeds, else FALSE, pText is a multiline text control (best if it
// has horiz and vert scroll bars enabled), pPath is a pointer to the absolute path to the
// file, numLines is how many of the first lines of the file are to be put into the control
// - for all of them, use -1; if a file has fewer lines than the numLines value passed in,
// all of its lines will be shown; the function uses the wxTextFile class to do the grunt
// work
bool PopulateTextCtrlByLines(wxTextCtrl* pText, wxString* pPath, int numLines)
{
	wxASSERT(pText);
	wxASSERT(!pPath->IsEmpty());
	pText->Clear();
	wxString path = *pPath;
	size_t lineCount = 0;
	wxTextFile f;
	bool bOpened = f.Open(path); // Unicode build assumes file data is valid UTF-8
	if (bOpened)
	{
		lineCount = f.GetLineCount();
		if (numLines == -1)
		{
			numLines = lineCount;
		}
		else
		{
			if ((size_t)numLines > lineCount)
			{
				numLines = lineCount;
			}
		}
		wxString eolStr = f.GetEOL(); // whatever is native for current platform
		size_t index;
		wxString str;
		for (index = 0; index < (size_t)numLines; index++)
		{
			str = f.GetLine(index);
			pText->AppendText(str);
			pText->AppendText(eolStr);
		}
		pText->SetSelection(0,0); // don't want anything selected

	}
	return bOpened;
}
*/

// returns TRUE if it succeeds, else FALSE, also returns false if the file has no data; of
// if a number of other error conditions arises as reported by tellenc().
//
// pText is a multiline text control (best if it has horiz and vert scroll bars enabled),
// pPath is a pointer to the absolute path to the file, numKilobytes is how many kB of the
// start of the file are to be put into the control - for all of the file, use -1; if a
// file has fewer bytes than the value computed from the number of kilobytes passed in, all
// of its contents will be shown; internally we convert the numKilobytes value to a byte
// count by multiplying by 1024; the function uses the wxFile class to do the grunt work
// for getting the file open, and GetFileNew() for grabbing the data and converting it to
// have just Unix \n line-endings regardless of platform we are running on.
bool PopulateTextCtrlWithChunk(wxTextCtrl* pText, wxString* pPath, int numKilobytes)
{
	wxASSERT(pText);
	wxASSERT(!pPath->IsEmpty());
	pText->Clear();
	wxString path = *pPath;
	size_t numBytes = numKilobytes * 1024;
	size_t filesize = 0;
	wxFile f;
	bool bOpened = f.Open(path); // default for Open is wxFile::read
	if (bOpened)
	{
		filesize = f.Length();
		if (filesize == 0)
		{
			return FALSE;
		}
		if (numKilobytes == -1)
		{
			numBytes = filesize;
		}
		else
		{
			if ((size_t)numBytes > filesize)
			{
				numBytes = filesize;
			}
		}
		f.Close(); // GetNewFile() will Open it again internally
		wxString eolStr = _T("\n");
		wxString str;
		wxString* pStr = &str;
		wxUint32 nLength; // returns length of data plus 1 (for the null char)
		// get the data into str, it will be UTF-16 in the Unicode app, ANSI in the
		// Regular app, and since the number of kilobytes wanted is passed in, the
		// GetNewFile() function will normalize the data to only use Unix \n (linefeed) as
		// the line terminator - this can be counted on, and the data will be passed back
		// in the first param. We'll ask for 16 kB of the file, or all of it if it is
		// smaller than that.
		enum getNewFileState retValue = GetNewFile(pStr, nLength, path, 16);
		if (retValue == getNewFile_success)
		{
			pText->ChangeValue(str);
			size_t lastPos = pText->GetLastPosition();
			pText->SetSelection(lastPos,lastPos); // don't want anything selected
		}
		else
		{
			if (retValue == getNewFile_error_ansi_CRLF_not_in_sequence)
			{
				// this error is pretty well absolutely certain never to occur, so we
				// won't bother to localize it; just inform the user and abort the Peek
				wxMessageBox(_T("Input data malformed, running Adapt It (Regular): CR (carriage return) and LF (linefeed) characters are present in the data, but are not paired as a sequence of two bytes. This should never happen!"),
				_T("Incredible data format error!"),wxICON_ERROR);
			}
			else
			{
				// a beep will do for other errors
				wxBell();
			}
			return FALSE;
		}
	}
	return TRUE;
}

/* does what strstr() does, so this is not needed
char* StrStrAI(char* super, char* sub)
{
	size_t len = strlen(super);
	size_t subLen = strlen(sub);
	if (subLen > len)
	{
		// no match is possible
		return (char*)NULL;
	}
	char* iter = super;
	size_t i;
	bool bMatches;
	char* pEnd = super + len - subLen;
	for (iter = super; iter != pEnd; iter++)
	{
		if (*iter == *sub)
		{
			// a potential match
			bMatches = TRUE;
			for (i = 1; i < subLen; i++)
			{
				if (*(iter + i) != *(sub + i))
				{
					// no match, keep truckin
					bMatches = FALSE;
					break;
				}
			}
			if (bMatches)
			{
				return iter;
			}
		}
	}
	return (char*)NULL;
}
*/

// tests for whether a passed in bare marker (ie. an SFM or USFM with its backslash stripped
// off) matches any of the bare markers stored in the passed in arr array
bool IsBareMarkerInArray(wxString& bareMkr, wxArrayString& arr)
{
	int count = arr.GetCount();
	if (count == 0)
		return FALSE;
	int i;
	for(i = 0; i < count; i++)
	{
		wxString s = arr.Item(i);
		if (bareMkr == s)
			return TRUE;
	}
	return FALSE;
}

// we do no checks, it's up to the caller to ensure that dest buffer has enough room for
// byteCount bytes to be copied
char* strncpy_utf16(char* dest, char* src, size_t byteCount)
{
	char* iter; // source text iterator
	char* iter2; // destination iterator
	char* pHaltLoc = src + byteCount;
	for (iter = src, iter2 = dest; iter < pHaltLoc; iter++, iter2++)
	{
		*iter2 = *iter;
	}
	return dest;
}

// Note 1: in the next function, we don't attempt a match test with one of the codes in the
// iso639-3 codes list, on the grounds that this function will be used for testing in a PT
// or BE collaboration scenario, and if a code is invalid from one app or the other, the
// matchup won't happen - so that matchup failure accomplishes the same protection as would
// be accomplished by including here code to read in the file and check that the value of
// 'code' plus a tab is found somewhere in the buffer. If we want a validity check for
// other scenarios - then using code from LanguageCodesDlg.cpp's InitDialog() function
// could be put in here to do the file input and subsequent code check.
// Note 2: The iso639-3 list has only 3-letter codes. But IsEthnologueCodeValid() will
// accept an older 2-letter code (like en for English) as valid too.
bool IsEthnologueCodeValid(wxString& code)
{
	if (code.IsEmpty())
		return FALSE;
	int length = code.Len();
	if (length < 2 || length > 3)
		return FALSE;
	wxChar aChar = _T('\0');
	bool bIsAlphabetic = FALSE;
	int index;
	for (index = 0; index < length; index++)
	{
		aChar = code.GetChar(0);
		bIsAlphabetic = IsAnsiLetter(aChar);
		if (!bIsAlphabetic)
		{
			return FALSE;
		}
	}
	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
/// \return		enum getNewFileState indicating success or error state when reading the
///             file.
/// \param		pstrBuffer	<- a wxString which receives the text file once loaded
/// \param		nLength		<- the length of the loaded text file
/// \param		pathName	-> path and name of the file to read into pstrBuffer
/// \param      numKBOnly   -> default is 0 (used as a filter to cause only the legacy
///                            code to be used), otherwise, the number of kilobytes that
///                            are wanted. (This can be safely greater than the file's
///                            size.)
/// \remarks
/// Called from: the Doc's OnNewDocument(), and helper.cpp's PopulateTextCtrlWithChunk().
/// Opens and reads a standard format input file into our wxString buffer pstrBuffer which
/// is used by the caller to tokenize and build the in-memory data structures used by the
/// View to present the data to the user. Note: the pstrBuffer is null-terminated.
///
/// BEW 19July10, moved here from CAdapt_ItDoc class, as it is to be used in more than just
/// loading a new file, but also for peeking at a file to verify it is a loadable one
///
/// BEW 19July10, added 4th param, numKBOnly, with default value 0. If the param has the
/// value zero, the whole file is read in and the legacy code applies.
/// If it has a non-zero value (number of kilobytes), then extra code switches in as follows:
/// (a) the returned string will be equal to or less than the number of kB requested (the
/// end may be truncated a little to ensure that, say, valid UTF-8 character byte strings
/// are preserved), and if the whole file is shorter than the requested amount, the whole
/// file is shown,
/// (b) the returned string will have had CR line terminators changed to LR, or CR & LF
/// terminators (Windows) converted to LR, and if the input was UTF-16, null-byte extended
/// CR and LF characters are converted to a null-byte extended LF terminator. (This
/// guarantee of Unix line termination is what wxTextCtrl, for multiline style, wants.
/// These manipulations are not done if the legacy code applies, that is, if numKBOnly is 0)
/// (The new internal manipulations are done in such a way that the input encoding is
/// unchanged by them.)
///////////////////////////////////////////////////////////////////////////////
enum getNewFileState GetNewFile(wxString*& pstrBuffer, wxUint32& nLength,
								wxString pathName, int numKBOnly)
{
    // Bruce's Note on GetNewFileBaseFunct():
    // BEW changed 8Apr06: to remove alloca()-dependence for UTF-8 conversion... Note: the
    // legacy (ie. pre 3.0.9 version) form of this function used Bob Eaton's conversion
    // macros defined in SHConv.h. But since ATL 7.0 has introduced 'safe' heap buffer
    // versions of conversion macros, these will be used here. (If Adapt_It.cpp's
    // Conver8to16(), Convert16to8(), DoInputConversion() and ConvertAndWrite() are
    // likewise changed to the safe macros, then SHConv.h could be eliminated from the
    // app's code entirely. And, of course, whatever the export functionalities use has to
    // be checked and changed too...)
    //
    // whm revised 19Jun09 to simplify (via returning an enum value) and move error
    // messages and presentation of the standard file dialog back to the caller
    // OnNewDocument. The tellenc.cpp encoding detection algorithm was also incorporated to
    // detect encodings, detect when an input file is actually a binary file (i.e., Word
    // documents are binary); detect when the Regular version attempts to load a Unicode
    // input file; and detect and properly handle when the user inputs a file with 8-bit
    // encoding into the Unicode version (converting it as much as possible - similarly to
    // what the legacy MFC app did). The revision also eliminates some memory leaks that
    // would happen if the routine returned prematurely with an error.

	// wxWidgets Notes:
	// 1. See MFC code for version 2.4.0 where Bruce needed to monkey
	//    with the call to GetNewFile() function using GetNewFileBaseFunct()
	//    and GetNewFileUsingPtr() in order to get the Chinese localized
	//    version to correctly load resources. I've not implemented those
	//    changes to GetNewFile's behavior here because the wxWidgets version
	//    handles all resources differently.
	// 2. GetNewFile() is called by OnNewDocument() in order to get a
	//    standard format input file into our wxString buffer pstrBuffer
	//    which is used by the caller to tokenize and build the in-memory
	//    data structures used by the View to present the data to the user.
	//    It also remembers where the input file came from by storing its
	//    path in m_lastSourceInputPath.
	wxUint32 byteCount = 0;
	bool bShorten = FALSE;
	char CR = 0;
	char LF = 0;
	char SP = 0;
	char BSLASH = 0;
	char NIX = -1;
	bool bHasCR = FALSE;
	bool bHasLF = FALSE;
	bool bHasCRLF = FALSE;
	char* pEnd = NULL; // will point to next char (a null) after data in byte buffer
	char* pBegin = NULL; // will point  at the first char of the byte buffer
	char* ptr = NULL; // a pointer to char in the byte buffer
	char* aux = NULL;
	wxUint32 offset = 0; // offset into the byte buffer
	wxUint32 originalBuffLen = 0;
	char* pbyteBuff_COPY = NULL; // where we may build the data with only \n line separators
	bool bTakeAll = TRUE; // set FALSE if file is longer than what we want to truncate to
	if (numKBOnly > 0)
	{
		// we use these only if the 4th input parameter was non-zero
		CR = 0x0D;
		LF = 0x0A;
		SP = ' ';
		BSLASH = '\\';
		NIX = '\0';
		byteCount = numKBOnly * 1024;
		bShorten = TRUE;
	}

	// get a CFile and check length of file
	// Since the wxWidgets version doesn't use exceptions, we'll
	// make use of the Open() method which will return false if
	// there was a problem opening the file.
	wxFile file;
	if (!file.Open(pathName, wxFile::read))
	{
		return getNewFile_error_at_open;
	}

	// file is now open, so find its logical length (always in bytes)
	nLength = file.Length();

    // BEW 19July10, if 4th input param numKBOnly is non-zero, we have to take only a max
    // of byteCount bytes, so we shorten to that much & then take of the end until we come
    // to a suitable stopping point; we also replace \r\n, or \r, or \n as line terminators
    // with the Unix \n only, copying to a second buffer, and then using that to overwrite
    // the first buffer, and do any needed UTF-16 conversions too, so that for the
    // numKBOnly case we will be sure to return a wxString with only \n line terminators,
    // to give the caller a well-defined situation to deal with; but if numKBOnly is 0, we
    // just do the legacy code which takes the whole byte string and after any Unicode
    // convertions returns the lot as UTF-16 (or as ANSI if this is the non-Unicode build)
	if (bShorten)
	{
		// if we are asking for more data than the file has, then adjust to get the whole
		// file; otherwise, reduce nLength to be just the data we want
		if (byteCount >= nLength)
		{
			byteCount = nLength;
			bTakeAll = TRUE; // when TRUE we don't later need to chop off any bytes at the end
		}
		else
		{
			// the file is longer than the initial stuff that we want, so adjust nLength smaller
			nLength = byteCount;
			bTakeAll = FALSE;
		}
	}

    // whm Design Note: There is no real need to separate the reading of the file into
    // Unicode and non-Unicode versions. In both cases we could use a pointer to wxChar to
    // point to our byte buffer, since in Unicode mode we use char* and in ANSI mode,
    // wxChar resolves to just char* anyway. We could then read the file into the byte
    // buffer and use tellenc only once before handling the results with _UNICODE
    // conditional compiles.

	bool bIsLittleEndian;
	bIsLittleEndian = TRUE; // this is valid for ANSI build, on non-Win platforms 
								 // we only support Unicode
#ifndef _UNICODE // ANSI version, no unicode support

	// create the required buffer and then read in the file (no conversions needed)
	// BEW changed 8Apr06; use malloc to remove the limitation of the finite stack size
	char* pBuf = (wxChar*)malloc(nLength + 1); // allow for terminating null byte
	char* pBegin_COPY = NULL;
	memset(pBuf,0,nLength + 1);
	wxUint32 numRead = file.Read(pBuf,(wxUint32)nLength);
	pBuf[numRead] = '\0'; // add terminating null
	nLength = numRead; // in case we didn't get all of it, use what we got
	originalBuffLen = nLength + sizeof(wxChar); // save it in case we need it below
	nLength += sizeof(wxChar); // allow for terminating null (sets m_nInputFileLength in the caller)

	if (bShorten)
	{
		// find out if CR and /or LF is within the data (can use strchr() for this)
		char* pCharLoc = strchr(pBuf,(int)CR);
		bHasCR = pCharLoc == NULL ? FALSE : TRUE;
		pCharLoc = strchr(pBuf,(int)LF);
		bHasLF = pCharLoc == NULL ? FALSE : TRUE;

		// avoid compiler warnings, pBegin and offset are not needed for the ANSI build
		pBegin = pBegin;
		offset = offset;

		// test for CR + LF sequence
		pCharLoc = NULL;
		char eol[3] = {CR,LF,NIX}; // a c-string, so use strstr()
		pCharLoc = strstr(pBuf,eol); // strstr() doesn't find CR or LF
		bHasCRLF = pCharLoc == NULL ? FALSE : TRUE;
		wxUint32 i;

		// we are ready to copy the substrings across, but we don't need to do so if there
		// are only LF line terminators in the data; but for CR+LF, and CR only, we'll
		// need to use the pbyteBuff_COPY buffer, do the transfers, then write it all
		// back over the contents in pbyteBuff after first clearing it to null bytes
		if ( (bHasCR && bHasLF) || (bHasCR && !bHasLF))
		{
			bool bHasBothCRandLF = bHasCR && bHasLF; // if FALSE, the data has only CR
			if (bHasBothCRandLF)
			{
				// this is the Windows string case, where line termination is CR+LF so
				// this is more complex - the data will shorten so we need to use the
				// second buffer and copy substrings there etc. However, the data could be
				// UTF-8, or UTF-16, so these are handled in different ways. For UTF-8, CR
				// and LF will be in sequence, and so we can search for that byte pair.
				// For UTF-16, CR and LF will each be followed by a nullbyte, so we can't
				// use c-string functions, and we'll need a scanning loop instead

                // make a second buffer for receiving copied data substrings, with Unix
                // line ending \n only
				pbyteBuff_COPY = (char*)malloc(nLength);
				memset(pbyteBuff_COPY,0,nLength); // fill with nulls, at least 2 bytes at
												  // the end will remain null
				// do the loop...
				if (bHasCRLF)
				{
					// assume ANSI - so find each CR followed immediately by an LF - for
					// this we can use strstr() safely
					aux = pBuf;
					ptr = pBuf;
					wxUint32 lineLen = 0;
					pEnd = pBuf + (nLength - sizeof(wxChar));
					pBegin_COPY = pbyteBuff_COPY; // could reuse pBegin, but that is more opaque
					ptr = strstr(aux,eol);
					while (ptr != NULL)
					{
						// copy the substring across
						lineLen = (wxUint32)(ptr - aux);
						pBegin_COPY = strncpy(pBegin_COPY,aux,lineLen);
						pBegin_COPY += lineLen; // point past what we just copied
						aux = ptr + sizeof(eol) - 1; // advance aux in the pBuf
											// buffer; -1 because eol has a null byte
						ptr = aux;

						// add an LF char after the substring in the copy buffer
						*pBegin_COPY = LF;
						// update location in copy buffer
						++pBegin_COPY;

						// break out another line, if there is another which ends with the eol
						// string
						ptr = strstr(aux,eol);
					}
					// get the last substring, which may not have been terminated by CR+LF
					lineLen = (wxUint32)(pEnd - aux);
					if (lineLen > 0)
					{
						pBegin_COPY = strncpy(pBegin_COPY,aux,lineLen);
						pBegin_COPY += lineLen; // this points at the next byte after
													 // the last byte that was copied
					}
				} // end TRUE block for test: if (bHasCRLF)
				else
				{
					// strange... we've got both CR and LF in the data, but they don't
					// occur as a sequence, so treat this as a data error
					free((void*)pBuf);
					free((void*)pbyteBuff_COPY);
					return getNewFile_error_ansi_CRLF_not_in_sequence;
				}

				// now overwrite the old data in pbyteBuff
				memset(pBuf,0,originalBuffLen); // fill with nulls, originalBuffLen will be
												// greater than the number of chars
												// we want to copy back there
				wxUint32 numberOfBytes = (wxUint32)(pBegin_COPY - pbyteBuff_COPY);
				pBuf = strncpy(pBuf, pbyteBuff_COPY, numberOfBytes);
				numRead = numberOfBytes;
				nLength = numberOfBytes + sizeof(wxChar);

			} // end of TRUE block for text: if (bHasBothCRandLF)
			else
			{
				// this is easy, just find each CR and overwrite with LF until done
				for (i = 0; i< nLength - sizeof(wxChar); i++)
				{
					if (*(pBuf + i) == CR)
					{
						// CR is not a valid UTF-8 non-initial byte, so this is safe
						*(pBuf + i) = LF; // overwrite CR with LF
					}
				}
				numRead = nLength;
			}
		} // end of TRUE block for test: if ( (bHasCR && bHasLF) || (bHasCR && !bHasLF))
	} // end of TRUE block for test: if (bShorten)

	// The following source code is used by permission. It is taken and adapted
	// from work by Wu Yongwei Copyright (C) 2006-2008 Wu Yongwei <wuyongwei@gmail.com>.
	// See tellenc.cpp source file for Copyright, Permissions and Restrictions.

	init_utf8_char_table();
	const char* enc = tellenc(pBuf, numRead - 1); // don't include null char at buffer end
	if (!(enc) || strcmp(enc, "unknown") == 0)
	{
		gpApp->m_srcEncoding = wxFONTENCODING_DEFAULT;
	}
	else if (strcmp(enc, "latin1") == 0) // "latin1" is a subset of "windows-1252"
	{
		gpApp->m_srcEncoding = wxFONTENCODING_ISO8859_1; // West European (Latin1)
	}
	else if (strcmp(enc, "windows-1252") == 0)
	{
		gpApp->m_srcEncoding = wxFONTENCODING_CP1252; // Microsoft analogue of ISO8859-1 "WinLatin1"
	}
	else if (strcmp(enc, "ascii") == 0)
	{
		// File was all pure ASCII characters, so assume same as Latin1
		gpApp->m_srcEncoding = wxFONTENCODING_ISO8859_1; // West European (Latin1)
	}
	else if (strcmp(enc, "utf-8") == 0
		|| strcmp(enc, "utf-16") == 0
		|| strcmp(enc, "utf-16le") == 0
		|| strcmp(enc, "ucs-4") == 0
		|| strcmp(enc, "ucs-4le") == 0)
	{
		free((void*)pBuf);
		return getNewFile_error_unicode_in_ansi;
	}
	else if (strcmp(enc, "binary") == 0)
	{
		free((void*)pBuf);
		return getNewFile_error_opening_binary;
	}
	else if (strcmp(enc, "gb2312") == 0)
	{
		gpApp->m_srcEncoding = wxFONTENCODING_GB2312; // same as wxFONTENCODING_CP936 Simplified Chinese
	}
	else if (strcmp(enc, "cp437") == 0)
	{
		gpApp->m_srcEncoding = wxFONTENCODING_CP437; // original MS-DOS codepage
	}
	else if (strcmp(enc, "big5") == 0)
	{
		gpApp->m_srcEncoding = wxFONTENCODING_BIG5; // same as wxFONTENCODING_CP950 Traditional Chinese
	}

	*pstrBuffer = pBuf; // copy to the caller's CString (on the heap) before malloc
						// buffer is destroyed

	free((void*)pBuf);

#else	// Unicode version supports ASCII, ANSI (but may not be rendered right
    // when converted using CP_ACP), UTF-8, and UTF-16 input (code taken from Bob Eaton's
    // modifications to AsyncLoadRichEdit.cpp for Carla Studio) We use a temporary buffer
    // allocated on the stack for input, and the conversion macros (which allocated another
    // temp buffer on the stack), to end up with UTF-16 for interal string encoding BEW
    // changed 8Apr06, requested by Geoffrey Hunt, to remove the file size limitation
    // caused by using the legacy macros, which use alloca() to do the conversions in a
    // stack buffer; the VS 2003 macros are size-safe, and use malloc for long strings.
	wxUint32 nNumRead;
	bool bHasBOM = FALSE;

    wxUint32 nBuffLen = nLength + sizeof(wxChar); // sizeof(wxChar) is for null bytes

	// get a byte buffer, initialize it to all null bytes, then read the file's data into it
	char* pbyteBuff = (char*)malloc(nBuffLen);
	char* pBegin_COPY = NULL;
	memset(pbyteBuff,0,nBuffLen); // fill with nulls
	originalBuffLen = nBuffLen; // save it in case we need it below
	nNumRead = (wxUint32)file.Read(pbyteBuff,nLength);
	nLength = nNumRead + sizeof(wxChar);

	// BEW added 16Aug11, determine the endian value for the machine we are running on
	wxString theBuf = wxString((wxChar*)pbyteBuff);
	bIsLittleEndian = IsLittleEndian(theBuf);

	if (bShorten)
	{
		// endian value complicates things, so since the code below is for little-endian
		// platforms, leave it except for any needed utf16 tweaks to be added, and have a
		// separate block of similar code for big-endian platforms
		if (bIsLittleEndian)
		{
			// find out if CR is within the data (can't use strchr() because it may be null
			// byte extended ascii now in the form UTF-16, so the null bytes will terminate
			// the search prematurely, so have to scan over the data in a loop
			char* pCharLoc = NULL;
			wxUint32 i;
			for (i = 0; i< nLength - sizeof(wxChar); i++)
			{
				if (*(pbyteBuff + i) == CR)
				{
					pCharLoc = pbyteBuff + i;
					break;
				}
			}
			bHasCR = pCharLoc == NULL ? FALSE : TRUE;

			// if we shortened the data to be only a certain number of bytes, we may have made
			// the final character invalid (eg. chopping the end off a UTF-8 byte sequence),
			// so scan backwards in pbyteBuff from the last byte read, until we come to a
			// suitable place to halt; adjust nLength accordingly and the removed bytes need
			// to have their locations cleared to null bytes
            // BEW 16Aug11: Note, these byte pointer monkeyings here are sensitive to
            // whether or not we have utf-8 or utf-16, so some extra code is needed here;
            // later we work out what encoding we have, but here we have to assume it could
            // be utf-16 or -8
			if (!bTakeAll)
			{
				offset = nLength;
				pBegin = pbyteBuff;
				pEnd = (char*)(pBegin + nLength);
				wxUint32 counter = 0;
				ptr = pEnd; // start, pointing after last byte read in
				do {
					// this will back up over null bytes, so add the sizeof(wxChar)
					// back in to the length when done
					--ptr;
					counter++;
				} while ((*ptr != SP && *ptr != BSLASH && *ptr != LF && *ptr != CR) && ptr > pBegin);
				if (bHasCR)
				{
					// we may have exited at the \n of an \r\n sequence, so check if the
					// preceding byte is a CR and if so, move back over it too, and count it;
					// check also if 2nd-preceding is CR (for UTF16 text there will be an
					// intervening null byte) & instead back up 2 bytes if so
					if (*(ptr - 1) == CR)
					{
						// it's utf-8, so back up 1 byte
						--ptr;
						counter++; 
					}
					else if (*(ptr - 2) == CR && *(ptr -1) == NIX)
					{
						// it's utf-16, so back up 2 bytes, and we are at the WORD
						// boundary too
						ptr -= 2;
						counter += 2; 
					}
				}
				if (nLength - counter == 0)
				{
					// free the original read in (const) char data's chunk
					free((void*)pbyteBuff);
					return getNewFile_error_no_data_read;
				}
				nLength -= counter;
				//if (counter > sizeof(wxChar)) // BEW changed 16Aug11 - anything backed
				//over has to be made into a null byte
				if (counter > 0)
				{
					// the unwanted bytes have to be made into null bytes
					wxUint32 index;
					for (index = 0; index < counter; index++)
					{
						*(ptr + index) = NIX;
					}
					nLength += sizeof(wxChar); // put the final two null bytes back in
				} // nLength now points at a null byte after a byte sequence of data
				  // which is valid in the given encoding, even if UTF-16 was input in either
				  // endian value, plus an extra couple of null bytes which will remain null
				  // in order to terminate the wxString which we return the data to the caller
			} // end of TRUE block for test: if (!bTakeAll)

			// find out if one or other or both CR and LF are still within the data
			pCharLoc = NULL;
			for (i = 0; i< nLength - sizeof(wxChar); i++)
			{
				if (*(pbyteBuff + i) == CR)
				{
					pCharLoc = pbyteBuff + i;
					break;
				}
			}
			bHasCR = pCharLoc == NULL ? FALSE : TRUE;

			pCharLoc = NULL;
			for (i = 0; i< nLength - sizeof(wxChar); i++)
			{
				if (*(pbyteBuff + i) == LF)
				{
					pCharLoc = pbyteBuff + i;
					break;
				}
			}
			bHasLF = pCharLoc == NULL ? FALSE : TRUE;

			// test for CR + LF in UTF-16 -- this test is for little-endian platforms
			pCharLoc = NULL;
			char eol_utf16[4] = {CR,NIX,LF,NIX};
			for (i = 0; i < nLength - sizeof(wxChar) - sizeof(eol_utf16); i++)
			{
				if (*(pbyteBuff + i) == CR)
				{
					// we have a potential CR + emptybyte + LF + emptybyte sequence
					char* pPotential = pbyteBuff + i;
					if (	*(pPotential + 0) == eol_utf16[0]	&&
							*(pPotential + 1) == eol_utf16[1]	&&
							*(pPotential + 2) == eol_utf16[2]	&&
							*(pPotential + 3) == eol_utf16[3]
					)
					{
						// we have a match
						pCharLoc = pPotential;
						break;
					}
				}
			}
			// when the following is TRUE, we also know it is UTF-16 data
			bHasCRLF = pCharLoc == NULL ? FALSE : TRUE;

			// we are ready to copy the substrings across, but we don't need to do so if there
			// are only LF line terminators in the data; but for CR+LF, and CR only, we'll
			// need to use the pbyteBuff_COPY buffer, do the transfers, then write it all
			// back over the contents in pbyteBuff after first clearing it to null bytes
			if ( (bHasCR && bHasLF) || (bHasCR && !bHasLF))
			{
				bool bHasBothCRandLF = bHasCR && bHasLF; // if FALSE, the data has only CR
				if (bHasBothCRandLF)
				{
					// This  is the Windows string case, where line termination is CR+LF so
					// this is more complex - the data will shorten so we need to use the
					// second buffer and copy substrings there etc. However, the data could be
					// UTF-8, or UTF-16, so these are handled in different ways. For UTF-8, CR
					// and LF will be in sequence, and so we can search for that byte pair.
					// For UTF-16, CR and LF will each be followed by a nullbyte, so we can't
					// use c-string functions, and we'll need a scanning loop instead

					// make a second buffer for receiving copied data substrings, with Unix
					// line ending \n only (remember, the data may be already UTF-16, which
					// could be zero byte extended ASCII, and so we can't assume there won't be
					// null bytes in it)
					pbyteBuff_COPY = (char*)malloc(nLength);
					memset(pbyteBuff_COPY,0,nLength); // fill with nulls, at least 2 bytes at
													  // the end will remain null
					// do the loop...
					if (bHasCRLF)
					{
                        // it's utf-16 data, so we've a more complex scanning loop to
                        // do - but it's similar to the one above for determining the
                        // bHasBothCRandLF value, so just tweak that
						aux = pbyteBuff;
						ptr = pbyteBuff;
						wxUint32 lineLen = 0;
						pEnd = pbyteBuff + (nLength - sizeof(wxChar));
						pBegin_COPY = pbyteBuff_COPY; // the iterator within pbyteBuff_COPY
						for (i = 0; i< nLength - sizeof(wxChar) - sizeof(eol_utf16); i++)
						{
							if (*(pbyteBuff + i) == CR)
							{
								// we have a potential CR + emptybyte + LF + emptybyte sequence
								char* pPotential = pbyteBuff + i;
								if (	*(pPotential + 0) == eol_utf16[0]	&&
										*(pPotential + 1) == eol_utf16[1]	&&
										*(pPotential + 2) == eol_utf16[2]	&&
										*(pPotential + 3) == eol_utf16[3]
									)
								{
									// we have a match; aux points to start of this substring,
									// and ptr points to char just past the last in the substring
									ptr = pPotential;
									lineLen = (wxUint32)(ptr - aux);
									// next call, can't use strncpy as it halts at the first
									// null byte (and in utf-16 built from ascii, there are
									// heaps of them present)
									pBegin_COPY = strncpy_utf16(pBegin_COPY,aux,lineLen);
									pBegin_COPY += lineLen; // point past what we just copied
									// advance aux in the pbyteBuff buffer past the eol_utf16
									// bytes and put ptr there too
									aux = ptr + sizeof(eol_utf16);
									ptr = aux;

									// add an LF char after the substring in the copy buffer,
									// then a null byte after it - since this block is for
									// little-endian
									*pBegin_COPY = LF;
									++pBegin_COPY;
									*pBegin_COPY = NIX;
									++pBegin_COPY;

									// advance the loop index past the 4 bytes of the CR & LF
									// in the source buffer
									i += sizeof(eol_utf16);

								}
							}
						} // end of for loop
						// get the last substring, which may not have been terminated by CR+LF
						lineLen = (wxUint32)(pEnd - aux);
						if (lineLen > 0)
						{
							pBegin_COPY = strncpy_utf16(pBegin_COPY,aux,lineLen);
							pBegin_COPY += lineLen; // this points at the next byte after
														 // the last byte that was copied
						}
					} // end TRUE block for test: if (bHasCRLF)
					else
					{
						// do the loop, it's not UTF-16, so could be ANSI or UTF-8; - find each CR
						// followed immediately by LF - we can use strstr() & strncpy() safely
						aux = pbyteBuff;
						ptr = pbyteBuff;
						char eol[3] = {CR,LF,NIX}; // must be a c-string for use in strstr()
						pEnd = pbyteBuff + (nLength - sizeof(wxChar));
						ptr = strstr(aux,eol);
						wxUint32 lineLen = 0;
						pBegin_COPY = pbyteBuff_COPY; // the *_COPY buffer's iterator
						while (ptr != NULL)
						{
							// copy the substring across
							lineLen = (wxUint32)(ptr - aux);
							pBegin_COPY = strncpy(pBegin_COPY,aux,lineLen);
							pBegin_COPY += lineLen; // point past what we just copied
							// advance ptr in the pbyteBuff buffer
							aux = ptr + sizeof(eol) -1; // point past the CR&LF
							ptr = aux;

							// add an LF char after the substring
							*pBegin_COPY = LF;
							// update location in copy buffer
							++pBegin_COPY;

							// break out another line, if there is another which ends with the eol string
							ptr = strstr(aux,eol);
						}
						// get the last substring, which may not have been terminated by CR+LF
						lineLen = (wxUint32)(pEnd - aux);
						if (lineLen > 0)
						{
							pBegin_COPY = strncpy(pBegin_COPY,aux,lineLen);
							pBegin_COPY += lineLen; // this points at the next byte after
														 // the last byte that was copied
						}
					} // end FALSE block for test: if (bHasCRLF)

					// now overwrite the old data in pbyteBuff
					memset(pbyteBuff,0,originalBuffLen); // fill with nulls, originalBuffLen will be
														 // greater than the number of chars
														 // we want to copy back there, by several
					wxUint32 numberOfBytes = (wxUint32)(pBegin_COPY - pbyteBuff_COPY);
					// because the copy could be having to copy nullbyte extended ascii made
					// into UTF-16, we can't use strncpy() here, but only our variant which
					// can handle that kind of data
					pbyteBuff = strncpy_utf16(pbyteBuff, pbyteBuff_COPY, numberOfBytes);
					nNumRead = numberOfBytes;
					nLength = numberOfBytes + sizeof(wxChar);

					// free the memory used for the second buffer
					free((void*)pbyteBuff_COPY);

				} // end of TRUE block for text: if (bHasBothCRandLF)
				else
				{
					// this is easy, just find each CR and overwrite with LF until done
					for (i = 0; i< nLength - sizeof(wxChar); i++)
					{
						if (*(pbyteBuff + i) == CR)
						{
							// CR is not a valid UTF-8 non-initial byte, so this is safe
							*(pbyteBuff + i) = LF; // overwrite CR with LF
						}
					}
					nNumRead = nLength;
				}
			} // end of TRUE block for test: if ( (bHasCR && bHasLF) || (bHasCR && !bHasLF))
		} // end of TRUE block for test: if (bIsLittleEndian)
		else
		{
			// it's a big-endian machine....
			
			// find out if CR is within the data (can't use strchr() because it may be null
			// byte extended ascii now in the form UTF-16, so the null bytes will terminate
			// the search prematurely, so have to scan over the data in a loop
			char* pCharLoc = NULL;
			wxUint32 i;
			for (i = 0; i< nLength - sizeof(wxChar); i++)
			{
				if (*(pbyteBuff + i) == CR)
				{
					pCharLoc = pbyteBuff + i;
					break;
				}
			}
			bHasCR = pCharLoc == NULL ? FALSE : TRUE;

			// if we shortened the data to be only a certain number of bytes, we may have made
			// the final character invalid (eg. chopping the end off a UTF-8 byte sequence),
			// so scan backwards in pbyteBuff from the last byte read, until we come to a
			// suitable place to halt; adjust nLength accordingly and the removed bytes need
			// to have their locations cleared to null bytes
            // BEW 16Aug11: Note, these byte pointer monkeyings here are sensitive to
            // whether or not we have utf-8 or utf-16, so some extra code is needed here;
            // later we work out what encoding we have, but here we have to assume it could
            // be utf-16 or -8
			if (!bTakeAll)
			{
				offset = nLength;
				pBegin = pbyteBuff;
				pEnd = (char*)(pBegin + nLength);
				wxUint32 counter = 0;
				ptr = pEnd; // start, pointing after last byte read in
				do {
					// this will back up over null bytes, so add the sizeof(wxChar)
					// back in to the length when done; since the data is big-endian, if
					// it is UTF-16 a null byte will precede any of the test characters in
					// the while test below
					--ptr;
					counter++;
				} while ((*ptr != SP && *ptr != BSLASH && *ptr != LF && *ptr != CR) && ptr > pBegin);
				if (bHasCR)
				{
					// we may have exited at the \n of an \r\n sequence, so check if the
					// preceding byte is a CR and if so, move back over it too, and count it;
					// check also if 2nd-preceding is CR (for UTF16 text there will be an
					// intervening null byte) & instead back up 2 bytes if so
					if (*(ptr - 1) == CR)
					{
						// it's utf-8, so back up 1 byte
						--ptr;
						counter++; 
					}
					else if (*(ptr - 2) == CR && *(ptr -1) == NIX)
					{
						// it's utf-16, so back up 2 bytes, and a further one to get to the
						// WORD boundary too
						ptr -= 2;
						counter += 2;
						if (*(ptr - 1) == NIX)
						{
							--ptr;
							counter++;
						}
					}
				}
				if (nLength - counter == 0)
				{
					// free the original read in (const) char data's chunk
					free((void*)pbyteBuff);
					return getNewFile_error_no_data_read;
				}
				nLength -= counter;
				//if (counter > sizeof(wxChar)) // BEW changed 16Aug11 - anything backed
				//over has to be made into a null byte
				if (counter > 0)
				{
					// the unwanted bytes have to be made into null bytes
					wxUint32 index;
					for (index = 0; index < counter; index++)
					{
						*(ptr + index) = NIX;
					}
					nLength += sizeof(wxChar); // put the final two null bytes back in
				} // nLength now points at a null byte after a byte sequence of data
				  // which is valid in the given encoding, even if UTF-16 was input in either
				  // endian value, plus an extra couple of null bytes which will remain null
				  // in order to terminate the wxString which we return the data to the caller
			} // end of TRUE block for test: if (!bTakeAll)

			// find out if one or other or both CR and LF are still within the data
			pCharLoc = NULL;
			for (i = 0; i< nLength - sizeof(wxChar); i++)
			{
				if (*(pbyteBuff + i) == CR)
				{
					pCharLoc = pbyteBuff + i;
					break;
				}
			}
			bHasCR = pCharLoc == NULL ? FALSE : TRUE;

			pCharLoc = NULL;
			for (i = 0; i< nLength - sizeof(wxChar); i++)
			{
				if (*(pbyteBuff + i) == LF)
				{
					pCharLoc = pbyteBuff + i;
					break;
				}
			}
			bHasLF = pCharLoc == NULL ? FALSE : TRUE;

			// test for CR + LF in UTF-16 -- this test is for big-endian platforms
			pCharLoc = NULL;
			//char eol_utf16[4] = {CR,NIX,LF,NIX};
			char eol_utf16[4] = {NIX,CR,NIX,LF};
			for (i = 0; i < nLength - sizeof(wxChar) - sizeof(eol_utf16); i++)
			{
				if (*(pbyteBuff + i) == CR)
				{
					// we have a potential CR + emptybyte + LF + emptybyte sequence
					char* pPotential = pbyteBuff + i;
					if (	*(pPotential + 0) == eol_utf16[0]	&&
							*(pPotential + 1) == eol_utf16[1]	&&
							*(pPotential + 2) == eol_utf16[2]	&&
							*(pPotential + 3) == eol_utf16[3]
						)
					{
						// we have a match
						pCharLoc = pPotential;
						break;
					}
				}
			}
			// when the following is TRUE, we also know it is UTF-16 data
			bHasCRLF = pCharLoc == NULL ? FALSE : TRUE;

			// we are ready to copy the substrings across, but we don't need to do so if there
			// are only LF line terminators in the data; but for CR+LF, and CR only, we'll
			// need to use the pbyteBuff_COPY buffer, do the transfers, then write it all
			// back over the contents in pbyteBuff after first clearing it to null bytes
			if ( (bHasCR && bHasLF) || (bHasCR && !bHasLF))
			{
				bool bHasBothCRandLF = bHasCR && bHasLF; // if FALSE, the data has only CR
				if (bHasBothCRandLF)
				{
					// This  is the Windows string case, where line termination is CR+LF so
					// this is more complex - the data will shorten so we need to use the
					// second buffer and copy substrings there etc. However, the data could be
					// UTF-8, or UTF-16, so these are handled in different ways. For UTF-8, CR
					// and LF will be in sequence, and so we can search for that byte pair.
					// For UTF-16, CR and LF will each be followed by a nullbyte, so we can't
					// use c-string functions, and we'll need a scanning loop instead.
					// Hmmm, do any big-endian machines use CR LF line endings? Probably
					// not. Oh well, just in case....

					// make a second buffer for receiving copied data substrings, with Unix
					// line ending \n only (remember, the data may be already UTF-16, which
					// could be zero byte extended ASCII, and so we can't assume there won't be
					// null bytes in it)
					pbyteBuff_COPY = (char*)malloc(nLength);
					memset(pbyteBuff_COPY,0,nLength); // fill with nulls, at least 2 bytes at
													  // the end will remain null
					// do the loop...
					if (bHasCRLF)
					{
                        // it's utf-16 data, so we've a more complex scanning loop to
                        // do - but it's similar to the one above for determining the
                        // bHasBothCRandLF value, so just tweak that
						aux = pbyteBuff;
						ptr = pbyteBuff;
						wxUint32 lineLen = 0;
						pEnd = pbyteBuff + (nLength - sizeof(wxChar));
						pBegin_COPY = pbyteBuff_COPY; // the iterator within pbyteBuff_COPY
						for (i = 0; i< nLength - sizeof(wxChar) - sizeof(eol_utf16); i++)
						{
							// bigendian requires a different test
							if (*(pbyteBuff + i) == NIX && *(pbyteBuff + i + 1) == CR)
							{
								// we have a potential emptybyte + CR + emptybyte + LF sequence
								char* pPotential = pbyteBuff + i;
								if (	*(pPotential + 0) == eol_utf16[0]	&&
										*(pPotential + 1) == eol_utf16[1]	&&
										*(pPotential + 2) == eol_utf16[2]	&&
										*(pPotential + 3) == eol_utf16[3]
									)
								{
									// we have a match; aux points to start of this substring,
									// and ptr points to char just past the last in the substring
									ptr = pPotential;
									lineLen = (wxUint32)(ptr - aux);
									// next call, can't use strncpy as it halts at the first
									// null byte (and in utf-16 built from ascii, there are
									// heaps of them present)
									pBegin_COPY = strncpy_utf16(pBegin_COPY,aux,lineLen);
									pBegin_COPY += lineLen; // point past what we just copied
									// advance aux in the pbyteBuff buffer past the eol_utf16
									// bytes and put ptr there too
									aux = ptr + sizeof(eol_utf16);
									ptr = aux;

									// add an LF char after the substring in the copy buffer,
									// and a null byte before it - since this block is for
									// big-endian
									*pBegin_COPY = NIX;
									++pBegin_COPY;
									*pBegin_COPY = LF;
									++pBegin_COPY;

									// advance the loop index past the 4 bytes of the CR & LF
									// in the source buffer
									i += sizeof(eol_utf16);
								}
							}
						} // end of for loop
						// get the last substring, which may not have been terminated by CR+LF
						lineLen = (wxUint32)(pEnd - aux);
						if (lineLen > 0)
						{
							pBegin_COPY = strncpy_utf16(pBegin_COPY,aux,lineLen);
							pBegin_COPY += lineLen; // this points at the next byte after
														 // the last byte that was copied
						}
					} // end TRUE block for test: if (bHasCRLF)
					else
					{
						// do the loop, it's not UTF-16, so could be ANSI or UTF-8; - find each CR
						// followed immediately by LF - we can use strstr() & strncpy() safely
						aux = pbyteBuff;
						ptr = pbyteBuff;
						char eol[3] = {CR,LF,NIX}; // must be a c-string for use in strstr()
						pEnd = pbyteBuff + (nLength - sizeof(wxChar));
						ptr = strstr(aux,eol);
						wxUint32 lineLen = 0;
						pBegin_COPY = pbyteBuff_COPY; // the *_COPY buffer's iterator
						while (ptr != NULL)
						{
							// copy the substring across
							lineLen = (wxUint32)(ptr - aux);
							pBegin_COPY = strncpy(pBegin_COPY,aux,lineLen);
							pBegin_COPY += lineLen; // point past what we just copied
							// advance ptr in the pbyteBuff buffer
							aux = ptr + sizeof(eol) -1; // point past the CR&LF
							ptr = aux;

							// add an LF char after the substring
							*pBegin_COPY = LF;
							// update location in copy buffer
							++pBegin_COPY;

							// break out another line, if there is another which ends with the eol string
							ptr = strstr(aux,eol);
						}
						// get the last substring, which may not have been terminated by CR+LF
						lineLen = (wxUint32)(pEnd - aux);
						if (lineLen > 0)
						{
							pBegin_COPY = strncpy(pBegin_COPY,aux,lineLen);
							pBegin_COPY += lineLen; // this points at the next byte after
														 // the last byte that was copied
						}
					} // end FALSE block for test: if (bHasCRLF)

					// now overwrite the old data in pbyteBuff
					memset(pbyteBuff,0,originalBuffLen); // fill with nulls, originalBuffLen will be
														 // greater than the number of chars
														 // we want to copy back there, by several
					wxUint32 numberOfBytes = (wxUint32)(pBegin_COPY - pbyteBuff_COPY);
					// because the copy could be having to copy nullbyte extended ascii made
					// into UTF-16, we can't use strncpy() here, but only our variant which
					// can handle that kind of data
					pbyteBuff = strncpy_utf16(pbyteBuff, pbyteBuff_COPY, numberOfBytes);
					nNumRead = numberOfBytes;
					nLength = numberOfBytes + sizeof(wxChar);

					// free the memory used for the second buffer
					free((void*)pbyteBuff_COPY);

				} // end of TRUE block for text: if (bHasBothCRandLF)
				else
				{
					// this is easy, just find each CR and overwrite with LF until done
					for (i = 0; i< nLength - sizeof(wxChar); i++)
					{
						if (*(pbyteBuff + i) == CR)
						{
							// CR is not a valid UTF-8 non-initial byte, so this is safe
							*(pbyteBuff + i) = LF; // overwrite CR with LF
						}
					}
					nNumRead = nLength;
				}
			} // end of TRUE block for test: if ( (bHasCR && bHasLF) || (bHasCR && !bHasLF))
		} // end of else block for test: if (bIsLittleEndian)

	} // end of TRUE block for test: if (bShorten)


	// now we have to find out what kind of encoding the data is in, and set the
	// encoding and we convert to UTF-16 in the DoInputConversion() function
	if (nNumRead <= 0)
	{
		// free the original read in (const) char data's chunk
		free((void*)pbyteBuff);
		return getNewFile_error_no_data_read;
	}

	// BEW 16Aug11: DON'T ASSUME IT HAS A BOM -- use tellenc2() to get the encoding if BOM is absent

    // check for UTF-16 first; we allow it, but don't expect it (but we won't assume it
    // would have a BOM)
	if (bIsLittleEndian && !memcmp(pbyteBuff,szU16BOM,nU16BOMLen))
	{
		// it's UTF-16 - little-endian
		gpApp->m_srcEncoding = wxFONTENCODING_UTF16;
		bHasBOM = TRUE;
	}
	else if (!bIsLittleEndian && !memcmp(pbyteBuff,szU16BOM_BigEndian,nU16BOMLen))
	{
		// it's UTF-16 - big-endian
		gpApp->m_srcEncoding = wxFONTENCODING_UTF16;
		bHasBOM = TRUE;
	}
	else
	{
		// see if it is UTF-8, whether with or without a BOM; if so,
		if (!memcmp(pbyteBuff,szBOM,nBOMLen))
		{
			// the UTF-8 BOM is present
			gpApp->m_srcEncoding = wxFONTENCODING_UTF8;
			bHasBOM = TRUE;
		}
		else
		{
			if (gbForceUTF8)
			{
				// the app is mucking up the source data conversion, so the user wants
				// to force UTF8 encoding to be used; DocPage.cpp has handler for the
				// checkbox, which is off by default (ie. UTF-8 is not forced, by default)
				gpApp->m_srcEncoding = wxFONTENCODING_UTF8;
			}
			else
			{
                // The MFC version uses Microsoft's IMultiLanguage2 interface to detect
                // whether the file buffer contains UTF-8, UTF-16 or some form of 8-bit
                // encoding (using GetACP()), but Microsoft's stuff is not cross-platform,
                // nor open source.
                //
                // One possibility for encoding detection is to use IBM's International
                // Components for Unicode (icu) under the LGPL. This is a very large, bulky
                // library of tools and would considerably inflate the size of Adapt It's
                // distribution.

                // The following source code is used by permission. It is taken and adapted
                // from work by Wu Yongwei Copyright (C) 2006-2008 Wu Yongwei
                // <wuyongwei@gmail.com>. See tellenc.cpp source file for Copyright,
                // Permissions and Restrictions.

				// GDLC Removed conditionals for PPC Mac (with gcc4.0 they are no longer needed)
				
				// BEW 16Aug11 -- Note, tellenc() calls tellenc2(), and tellenc() may
				// still return utf-16 (ie. big-endian) or utf-16le (little-endian) here,
				// since we've not handled the BOM-less case above
				init_utf8_char_table();
				const char* enc = tellenc(pbyteBuff, nLength - sizeof(wxChar)); // don't include null char at buffer end
				if (!(enc) || strcmp(enc, "unknown") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_DEFAULT;
				}
				else if (strcmp(enc, "latin1") == 0) // "latin1" is a subset of "windows-1252"
				{
					gpApp->m_srcEncoding = wxFONTENCODING_ISO8859_1; // West European (Latin1)
				}
				else if (strcmp(enc, "windows-1252") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_CP1252; // Microsoft analogue of ISO8859-1 "WinLatin1"
				}
				else if (strcmp(enc, "ascii") == 0)
				{
					// File was all pure ASCII characters, so assume same as Latin1
					gpApp->m_srcEncoding = wxFONTENCODING_ISO8859_1; // West European (Latin1)
				}
				else if (strcmp(enc, "utf-8") == 0) // Only valid UTF-8 sequences
				{
					gpApp->m_srcEncoding = wxFONTENCODING_UTF8;
				}
				else if (strcmp(enc, "utf-16") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_UTF16;
				}
				else if (strcmp(enc, "utf-16le") == 0)
				{
					// UTF-16 big and little endian are both handled by wxFONTENCODING_UTF16
					gpApp->m_srcEncoding = wxFONTENCODING_UTF16LE; // == same as wxFONTENCODING_UTF16
				}
				else if (strcmp(enc, "ucs-4") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_UTF32;
				}
				else if (strcmp(enc, "ucs-4le") == 0)
				{
					 gpApp->m_srcEncoding = wxFONTENCODING_UTF32LE;
				}
				else if (strcmp(enc, "binary") == 0)
				{
					// free the original read in (const) char data's chunk
					free((void*)pbyteBuff);
					return getNewFile_error_opening_binary;
				}
				else if (strcmp(enc, "gb2312") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_GB2312; // same as wxFONTENCODING_CP936 Simplified Chinese
				}
				else if (strcmp(enc, "cp437") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_CP437; // original MS-DOS codepage
				}
				else if (strcmp(enc, "big5") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_BIG5; // same as wxFONTENCODING_CP950 Traditional Chinese
				}
			}
		}
	}

	// do the converting and transfer the converted data to pstrBuffer (which then
	// persists while doc lives) -- if m_srcEndoding is wxFONTENCODING_UTF16, then
	// conversion is skipped and the text (minus the BOM if present) is returned 'as is'
	gpApp->DoInputConversion(*pstrBuffer,pbyteBuff,gpApp->m_srcEncoding,bHasBOM);

	// update nLength (ie. m_nInputFileLength in the caller, include terminating null in
	// the count)
	nLength = pstrBuffer->Len() + 1; // # of UTF16 characters + null character
											// (2 bytes)
	// free the original read in (const) char data's chunk
	free((void*)pbyteBuff);

#endif
	file.Close();
	return getNewFile_success;
}

// BEW created July2010. Used only in OnBnClickedMove() handler from AdminMoveOrCopy.cpp.
// Tests if the folder(s) selected by the user include the "__SOURCE_INPUTS" folder -- the
// latter contains only a monocline list of loadable (for document creation purposes)
// files - typically USFM marked up plain text data. If TRUE is returned, the
// OnBnClickedMove() handler exits without doing any moving, with a warning to the user to
// explain why.
bool SelectedFoldersContainSourceDataFolder(wxArrayString* pFolders)
{
	wxASSERT(!pFolders->IsEmpty());
	size_t count = pFolders->GetCount();
	size_t index;
	for (index = 0; index < count; index++)
	{
		wxString filename = pFolders->Item(index);
		if (filename == gpApp->m_sourceInputsFolderName)
		{
			return TRUE;
		}
	}
	return FALSE;
}

// BEW created 9Aug10, this function takes a first param which is an input array of names,
// and a second param which is an input array of unwanted names (which possibly, but not
// obligatorily so) partly or wholely overlaps those in the first array, and removes from
// the inventory of names in the first param any which are also in the second param's
// inventory. It also removes any names in the array of originals which are empty strings;
// but any empty strings in the unwanteds array are not removed from the unwanteds array.
// 
// The function has general application. However it was specifically created for the
// situation where the originals (the first param's array) are filenames from the Source
// Data folder, and the unwanteds (the second param's array) are document filenames from the
// Adaptations folder of the current project, or, if book mode is currently on, from the
// current book folder. The intention behind such removals is to prevent the user from
// re-creating a document from a loadable source text file when/if he forgets he already
// has created one with that particular name -- because doing so would cause that
// document's adaptations to be lost. (The GUI will tell the user that he can have the
// loadable source text file displayed if he first uses File / Save As... to rename the
// document file.) We assume the first parameter's array is a sorted one.
void RemoveNameDuplicatesFromArray(wxArrayString& originals, wxArrayString& unwanteds,
								   bool bSorted, enum ExtensionAction extAction)
{
	wxArrayInt arrRemoveIndices;
	if (unwanteds.GetCount() == 0)
	{
		return; // nothing exists to test for a name clash
	}
	size_t outerIndex;
	size_t innerIndex;
	// do the test only if there is at least one name to check
	if (originals.GetCount() > 0)
	{
		for (outerIndex = 0; outerIndex < originals.GetCount(); outerIndex++)
		{
			wxString anOriginalName = originals.Item(outerIndex);
			if (anOriginalName.IsEmpty())
			{
				// remove any empty strings in the originals array
				originals.RemoveAt(outerIndex,1);
			}
			else
			{
				// use an inner loop to test for a name clash with the name stored in
				// anOriginalName
				if (extAction == excludeExtensionsFromComparison)
				{
					// we assume the string is a filename, possibly with an extension, and
					// if it has one then we remove it
					wxFileName fn(anOriginalName);
					anOriginalName = fn.GetName(); // the title part only, without extension
					/* for  debugging
					int offset = wxNOT_FOUND;
					offset = anOriginalName.Find(_T("endmarkers"));
					if (offset != wxNOT_FOUND)
					{
						wxString str = anOriginalName;
					}
					*/
				}
				for (innerIndex = 0; innerIndex < unwanteds.GetCount(); innerIndex++)
				{
					wxString anUnwantedName = unwanteds.Item(innerIndex);
					if (extAction == excludeExtensionsFromComparison)
					{
						// we assume the string is a filename, possibly with an extension, and
						// if it has one then we remove it
						wxFileName fn(anUnwantedName);
						anUnwantedName = fn.GetName(); // the title part only, without extension
						/* for  debugging
						int offset = wxNOT_FOUND;
						offset = anUnwantedName.Find(_T("endmarkers"));
						if (offset != wxNOT_FOUND)
						{
							wxString str2 = anUnwantedName;
						}
						*/
					}
					if (!anUnwantedName.IsEmpty())
					{
						if (anOriginalName == anUnwantedName)
						{
							// we have a name clash, so store the index so as to later remove
							// the indexed string
							//originals.RemoveAt(outerIndex,1);<<-- no, shorting outer limit prevents all items from being checked
							arrRemoveIndices.Add(outerIndex);
						}
					}
				}
			}
		}
	}

	// now do any required removals, in reverse order so that index values stay synced to
	// the respective stored wxString instances to be removed; if there are none to be
	// removed though, then skip this block
	if (arrRemoveIndices.GetCount() != 0)
	{
		int limit = (int)originals.GetCount();
		int index;
		int nextItem = (int)arrRemoveIndices.GetCount() - 1;
		int nextRemovalIndex = arrRemoveIndices.Item(nextItem);
		for (index = limit - 1; index >= 0; index--)
		{
			if (index == nextRemovalIndex)
			{
				originals.RemoveAt(index,1);
				if (originals.IsEmpty())
				{
					return;
				}

				// prepare for the next one for removal
				if (nextItem > 0)
				{
					nextItem--;
					nextRemovalIndex = arrRemoveIndices.Item(nextItem);
				}
				else
				{
					nextRemovalIndex = wxNOT_FOUND; // -1
				}
			}
		}
	}

	// sort the result, if sorting was requested
	if (bSorted)
	{
		// same compare function as is used for MoveOrCopyFoldersOrFiles dialog's list
		// controls; it does a caseless compare for Windows OS platform, but a
		// case-sensitive compare for Linux and Mac OSX
		originals.Sort(sortCompareFunc);
	}
}

// BEW 26Aug10, added ChangeParatextPrivatesToCustomMarkers( in order to support the USFM
// 2.3 new feature, where a \z prefix is supplied to 3rd party developers for custom
// markers which Paratext will ignore.
// BEW 30Aug10, OXES documentation lists the expected annotation custom SF marker to be
// \zAnnotation and so I've change to that (no longer \zNote)
//  This function will change:
// \zbt            to      \bt
// \zfree          to      \free
// \zfree*         to      \free*
// \zAnnotation    to      \note
// \zAnnotation*   to      \note*
// and the location where it will be called is after the return from the dialog in the
// document member function OnNewDocument(). These replacements are done before the (U)SFM
// parser gets to see the filed in source text data, and so the parser can be left to
// respond to \bt, \free and \note as was the case earlier. 
// Note: we retain the use of \bt, \free, and \note internally. The \z-forms only appear
// in our SFM exports; and not in LIFT, OXES, or RTF exports, as in all of these markers
// are replaced by other formatting protocols. This function undoes what
// ChangeCustomMarkersToParatextPrivates(wxString& buffer) did; the latter is defined in
// the ExportFunctions.cpp class.
void ChangeParatextPrivatesToCustomMarkers(wxString& buffer)
{
	// converting the start markers, also converts all their matching endmarkers too
	wxString oldFree = _T("\\zfree");
	wxString newFree = _T("\\free");
	wxString oldNote = _T("\\zAnnotation");
	wxString newNote = _T("\\note");
	wxString oldBt = _T("\\zbt "); // include space, any of SAG's \bt-derived markers we
								  // will not convert; and our \bt has no endmarker so
								  // this is safe to do
	wxString newBt = _T("\\bt "); // need the space here too, since we are replacing

	// the conversions are done with the 3rd param, replaceAll, left at default TRUE
	int count = buffer.Replace(oldFree,newFree);
	count = buffer.Replace(oldNote,newNote);
	count = buffer.Replace(oldBt,newBt);
}

// BEW 11Oct10, get arrays containing src & tgt puncts added, and another two for puncts removed
void AnalysePunctChanges(wxString& srcPunctsBefore, wxString& tgtPunctsBefore,
			 wxString& srcPunctsAfter, wxString& tgtPunctsAfter,
			 wxArrayInt*& pSrcPunctsRemovedArray, wxArrayInt*& pSrcPunctsAddedArray,
			 wxArrayInt*& pTgtPunctsRemovedArray, wxArrayInt*& pTgtPunctsAddedArray)
{
	// analyse the source language punctuation characters; first the removals (we ignore
	// pairs, it's sufficient to take each single non-null punct character). Any src punt
	// from the 'before' set which is not in the 'after' set, is a removed src punct wxChar
	size_t srcLen = srcPunctsBefore.Len();
	size_t i;
	wxChar chr;
	pSrcPunctsRemovedArray->Clear();
	for (i = 0; i < srcLen; i++)
	{
		chr = srcPunctsBefore[i];
		if (chr == _T(' '))
			// if we have a delimiting space, just iterate
			continue;
		if (srcPunctsAfter.Find(chr) == wxNOT_FOUND)
		{
			// this one is missing, so store it in the removals array
			pSrcPunctsRemovedArray->Add((size_t)chr);
		}
	}
	// now the src puncts additions, for these, anything in srcPunctsAfter which is not in
	// srcPunctsBefore, is a wxChar added, to add it to the pSrcPunctsAddedArray
	srcLen = srcPunctsAfter.Len();
	pSrcPunctsAddedArray->Clear();
	for (i = 0; i < srcLen; i++)
	{
		chr = srcPunctsAfter[i];
		if (chr == _T(' '))
			// if we have a delimiting space, just iterate
			continue;
		if (srcPunctsBefore.Find(chr) == wxNOT_FOUND)
		{
			// this one is missing, so store it in the additions array
			pSrcPunctsAddedArray->Add((size_t)chr);
		}
	}
	// Now do the same, but for tgt punctuation removals and additions
	pTgtPunctsRemovedArray->Clear();
	size_t tgtLen = tgtPunctsBefore.Len();
	for (i = 0; i < tgtLen; i++)
	{
		chr = tgtPunctsBefore[i];
		if (chr == _T(' '))
			// if we have a delimiting space, just iterate
			continue;
		if (tgtPunctsAfter.Find(chr) == wxNOT_FOUND)
		{
			// this one is missing, so store it in the removals array
			pTgtPunctsRemovedArray->Add((size_t)chr);
		}
	}
	// now the tgt puncts additions, for these, anything in tgtPunctsAfter which is not in
	// tgtPunctsBefore, is a wxChar added, to add it to the pTgtPunctsAddedArray
	tgtLen = tgtPunctsAfter.Len();
	pTgtPunctsAddedArray->Clear();
	for (i = 0; i < tgtLen; i++)
	{
		chr = tgtPunctsAfter[i];
		if (chr == _T(' '))
			// if we have a delimiting space, just iterate
			continue;
		if (tgtPunctsBefore.Find(chr) == wxNOT_FOUND)
		{
			// this one is missing, so store it in the additions array
			pTgtPunctsAddedArray->Add((size_t)chr);
		}
	}
}

// Tests if one of the substring in testStr is a match for any substring in strItems; the
// substrings should be delimited by at least one space, and the function doesn't care
// what meaning the strings have. I'm building it, however, to test whether one of the set
// of USFM markers within the space-delimited string, testStr, matches a marker within the
// space-delimited string strItems. Internally it tokenizes testStr storing the individual
// markers within wxArrayString and uses Find() to check if one exists in strItems. Calls
// long SmartTokenize(wxString& delimiters, wxString& str, wxArrayString& array,
//	                   bool bStoreEmptyStringsToo) & can be used to test for endmarkers
// Return TRUE if there is at least one match, FALSE if nothing matches
bool IsSubstringWithin(wxString& testStr, wxString& strItems)
{
	wxString delims = _T(" ");
	wxArrayString arr;
	wxString aSpace = _T(' ');
	long countL = SmartTokenize(delims, testStr, arr, FALSE);
	long index;
	wxString str;
	// make it work with endmarkers in testStr, which aren't necessarily space-delimited
	wxString asterisk = _T('*');
	int offset = testStr.Find(asterisk);
	for (index = 0L; index < countL; index++)
	{
		str = arr.Item((size_t)index);
		if (offset == wxNOT_FOUND)
		{
			// no endmarkers, so assume that if they are markers, they are 
			// beginmarkers
			str += aSpace; // include the following space in the search string
		}
		int nFound = strItems.Find(str);
		if (nFound != wxNOT_FOUND)
			return TRUE;
	}
	return FALSE;
}



/* unused so far
SPList::Node* SPList_ReplaceItem(SPList*& pList, CSourcePhrase* pOriginalSrcPhrase, 
	CSourcePhrase* pNewSrcPhrase, bool bDeleteOriginal, bool bDeletePartnerPileToo)
{
	SPList::Node* pos = pList->Find(pOriginalSrcPhrase);
	if (pos == NULL)
		return pos;
	int sequNum = pOriginalSrcPhrase->m_nSequNumber;
	SPList::Node* nextPos = pos;
	nextPos->GetNext(); // we insert before this Node (this may not exist, if so, append)
	pList->Erase(pos); // removes the Node* from the list
	if (bDeleteOriginal)
	{
		gpApp->GetDocument()->DeleteSingleSrcPhrase(pOriginalSrcPhrase,bDeletePartnerPileToo);
	}
	if (nextPos == NULL)
	{
		// at list end, so do an append
		pos = pList->Append(pNewSrcPhrase);

	}
	else
	{
		// insert before nextPos 
		pos = pList->Insert(nextPos,pNewSrcPhrase);
	}
	pNewSrcPhrase->m_nSequNumber = sequNum;
	return pos;
}
*/

// BEW created, 13Jul11, in support of Paratext contentless USFM marked up chapter or book
// files. To allow space to be retained at the end of the empty \v number line.
// Used in: FormatMarkerBufferForOutput(wxString& text, enum ExportType expType) see the
// ExportFunctions.cpp file
bool KeepSpaceBeforeEOLforVerseMkr(wxChar* pChar)
{
	// the previous character must be a space
	if (*(pChar - 1) != _T(' '))
	{
		return FALSE;
	}
	wxChar* pCh = pChar - 1;
	// then the previous alphanumeric ones are to be jumped until the next space is
	// encountered but return FALSE if a newline or backslash is found first; and don't go
	// back more than 5 more characters - if no space by then, return FALSE
	bool bFoundSpace = FALSE;
	int i;
	pCh = pCh - 1; // point at next previous one
	for (i = 0; i < 5; i++)
	{
		if (IsAnsiLetterOrDigit(*pCh))
		{
			// back over it
			pCh = pCh - 1;
		}
		else if (*pCh == _T('\\') || *pCh == _T('\n'))
		{
			return FALSE;
		}
		else if (*pCh == _T(' '))
		{
			// found a space
			bFoundSpace = TRUE;
			break;
		}
		else
		{
			// don't expect anything else, if we get it, must not be what we want so
			// return FALSE
			return FALSE;
		}
	}
	if (!bFoundSpace)
	{
		// didn't succeed in finding expected USFM structure
		return FALSE;
	}
	// if control reaches here, we must be pointing at a found space
	// we succeed if the previous two chars are \v or previous 3 are \vn
	// so check for these
	if ( (*(pCh - 1) == _T('v') && *(pCh - 2) == _T('\\')) || 
		 (*(pCh - 1) == _T('n') && *(pCh - 2) == _T('v') && *(pCh - 3) == _T('\\')))
	{
		return TRUE;
	}
	return FALSE;
}

// a handy check for whether or not the wxChar which ptr points at is ~ or [ or ]
// BEW created 25Jan11, used in FindParseHaltLocation() of doc class
bool IsFixedSpaceOrBracket(wxChar* ptr)
{
	if (*ptr == _T('~') || *ptr == _T(']') || *ptr == _T('['))
	{
		return TRUE;
	}
	return FALSE;
}

void ConvertSPList2SPArray(SPList* pList, SPArray* pArray)
{
	SPList::Node* pos= pList->GetFirst();
	while (pos != NULL)
	{
		CSourcePhrase* pSrcPhrase = pos->GetData();
		pos = pos->GetNext();
		pArray->Add(pSrcPhrase);
	}
}

void ExtractSubarray(SPArray* pInputArray, int nStartAt, int nEndAt, SPArray* pSubarray)
{
	int nTotal = pInputArray->GetCount();
	if (nStartAt < 0 || nEndAt >= nTotal)
	{
		// tell the developer there is a bounds error & return
		wxString msg;
		msg = msg.Format(_T("ExtractSubarray() bounds error: total %d, startAt %d, endAt %d\n The subarray has not been populated."),
			nTotal,nStartAt,nEndAt);
		wxMessageBox(msg, _T("Index out of bounds"),wxICON_ERROR);
		pSubarray->Clear();
		return;
	}
	int index;
	for (index = nStartAt; index <= nEndAt; index++)
	{
		CSourcePhrase* pSrcPhrase = pInputArray->Item(index);
		pSubarray->Add(pSrcPhrase);
	}
}

// Moved the code in this function from DoFileSave() to here
void UpdateDocWithPhraseBoxContents(bool bAttemptStoreToKB, bool& bNoStore, 
									bool bSuppressWarningOnStoreKBFailure)
{
//#ifdef __WXDEBUG__

//#else
    // BEW 6July10, code added for handling situation when the phrase box location has just
    // been made a <Not In KB> one (which marks all CRefString instances for that key as
    // m_bDeleted set TRUE and also stores a <Not In KB> for that key in the adaptation KB)
    // and then the user saves or closes and saves the document. Without this extra code,
    // the block immediatly below would re-store the active location's adaptation string
    // under the same key, thereby undeleting one of the deleted CRefString entries in the
    // KB for that key -- so we must prevent this happening by testing for m_bNotInKB set
    // TRUE in the CSourcePhrase instance there and if so, inhibiting the save
    bool bInhibitSave = FALSE;
	bool bOK = TRUE;
	CAdapt_ItView* pView = gpApp->GetView();
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	bNoStore = FALSE; // initialize to default value (i.e. assumes a KB store attempt
					  // will succeed if tried)

    // BEW changed 26Oct10 to remove the wxASSERT because if the phrasebox went past the
    // doc end, and the user wants a save or saveAs, then this assert will trip; so we have
    // to check and get start of doc as default active location in such a situation
	//wxASSERT(pApp->m_pActivePile != NULL); // whm added 18Aug10
	if (gpApp->m_pActivePile == NULL)
	{
		// phrase box is not visible, put it at sequnum = 0 location
		gpApp->m_nActiveSequNum = 0;
		gpApp->m_pActivePile = pDoc->GetPile(gpApp->m_nActiveSequNum);
		CSourcePhrase* pSrcPhrase = gpApp->m_pActivePile->GetSrcPhrase();
		pView->Jump(gpApp,pSrcPhrase);
	}
	CSourcePhrase* pActiveSrcPhrase = gpApp->m_pActivePile->GetSrcPhrase();
	if (gpApp->m_pTargetBox != NULL)
	{
		if (gpApp->m_pTargetBox->IsShown())// not focused on app closure
		{
			if (!gbIsGlossing)
			{
				pView->MakeTargetStringIncludingPunctuation(pActiveSrcPhrase, gpApp->m_targetPhrase);
				pView->RemovePunctuation(pDoc, &gpApp->m_targetPhrase, from_target_text);
			}
			gbInhibitMakeTargetStringCall = TRUE;
			if (gbIsGlossing)
			{
				if (bAttemptStoreToKB)
				{
					bOK = gpApp->m_pGlossingKB->StoreText(pActiveSrcPhrase, gpApp->m_targetPhrase);
				}
			}
			else
			{
				// do the store, but don't store if it is a <Not In KB> location
				if (pActiveSrcPhrase->m_bNotInKB)
				{
					bInhibitSave = TRUE;
					bOK = TRUE; // need this, otherwise the message below will get shown
					// set the m_adaption member of the active CSourcePhrase instance,
					// because not doing the StoreText() call means it would not otherwise
					// get set; the above MakeTargetStringIncludingPunctuation() has
					// already set the m_targetStr member to include any punctuation
					// stored or typed
					pActiveSrcPhrase->m_adaption = gpApp->m_targetPhrase; // punctuation was removed above
				}
				else
				{
					if (bAttemptStoreToKB)
					{
						bOK = gpApp->m_pKB->StoreText(pActiveSrcPhrase, gpApp->m_targetPhrase);
					}
				}
			}
			gbInhibitMakeTargetStringCall = FALSE;
			if (!bOK)
			{
				// something is wrong if the store did not work, but we can tolerate the error 
				// & continue
				if (!bSuppressWarningOnStoreKBFailure)
				{
				wxMessageBox(_(
"Warning: the word or phrase was not stored in the knowledge base. This error is not destructive and can be ignored."),
				_T(""),wxICON_EXCLAMATION);
				bNoStore = TRUE;
				}
			}
			else
			{
				if (gbIsGlossing)
				{
					pActiveSrcPhrase->m_bHasGlossingKBEntry = TRUE;
				}
				else
				{
					if (!bInhibitSave)
					{
						pActiveSrcPhrase->m_bHasKBEntry = TRUE;
					}
					else
					{
						bNoStore = TRUE;
					}
				}
			}
		}
	}
//#endif
}

// BEW created 17Jan11, used when converting back from docV5 to docV4 (only need this for
// 5.2.4, but it could be put into the Save As... code for 6.x.y too - if so, we can retain
// it for version 6 and higher)
bool HasParagraphMkr(wxString& str)
{
	int offset = str.Find(_T("\\p"));
	if (offset == wxNOT_FOUND)
		return FALSE; // most calls will return here
	wxString wholeMkr;
	wxString theRest = str.Mid(offset); // the rest contains \p and whatever is after it
	wholeMkr = gpApp->GetDocument()->GetWholeMarker(theRest);
	if (wholeMkr == _T("\\p"))
	{
		return TRUE;
	}
	return FALSE; // it's some other marker beginning with \p, (such as \periph, \pn, \pn*,
				  // \pb, \pro, \pro*), or it could be one of the more obscure USFM paragraph
				  //  markers like \pmo \pm \pmc \pmr \pi1 \pi2 etc \pc \pr \ph1 \ph2 etc;
				  //  but since legacy versions only used flag bit 11, for m_bParagraph, and
				  //  it was always only set by \p being in m_markers, we'll not bother to
				  //  do anything more sophisticated than the code above
}
/*
// a diagnostic function used for chasing a bug due to partner piles not being fully
// populated when CSourcePhrase replacements were done (as in vertical editing)
void ShowSPandPile(int atSequNum, int whereTis)
{
	SPList* pList = gpApp->m_pSourcePhrases;
	SPList::Node* pos = pList->Item(atSequNum);
	CSourcePhrase* pSrcPhrase = pos->GetData();
	CLayout* pLayout = gpApp->m_pLayout;
	PileList* pPiles = pLayout->GetPileList();
	PileList::Node* posPiles = pPiles->Item(atSequNum);
	CPile* pPile = posPiles->GetData();
	// CPile private members
	//CLayout*		m_pLayout;
	//CSourcePhrase*	m_pSrcPhrase;
	//CStrip*			m_pOwningStrip;
	//CCell*			m_pCell[MAX_CELLS]; // 1 source line, 1 target line, & one gloss line per strip
	//int				m_nPile; // what my index is in the strip object
	//int				m_nWidth;
	//int				m_nMinWidth;
	//bool			m_bIsCurrentFreeTransSection; // BEW added 24Jun05 for support of free translations
	wxString msg1;
	if (pPile->GetStrip() == NULL)
	{
		// strip pointer is NULL, so strip index is undefined
		msg1 = msg1.Format(_T("\n              CPile: Where = %d ; m_pSrcPhrase %x ; m_pOwningStrip %x ; m_nPile %d ; Strip index is UNDEFINED"),
		whereTis, pPile->GetSrcPhrase(), pPile->GetStrip(), pPile->GetPileIndex());
	}
	else
	{
		// strip pointer is not NULL, so can get value of strip index
		msg1 = msg1.Format(_T("\n              CPile: Where = %d ; m_pSrcPhrase %x ; m_pOwningStrip %x ; m_nPile %d ; m_nStrip %d"),
		whereTis, pPile->GetSrcPhrase(), pPile->GetStrip(), pPile->GetPileIndex(), pPile->GetStrip()->GetStripIndex());
	}
	wxLogDebug(msg1);
	wxString msg2;
	msg2 = msg2.Format(_T("CSourcePhrase:   SequNum %d ; m_pSrcPhrase %x ; m_srcPhrase %s ; m_targetStr %s ; m_gloss %s"),
		pSrcPhrase->m_nSequNumber, pSrcPhrase, pSrcPhrase->m_srcPhrase.c_str(), pSrcPhrase->m_targetStr.c_str(),
		pSrcPhrase->m_gloss.c_str());
	wxLogDebug(msg2);
}

// a debugging helper, to see contents of m_pLayout's m_invalidStripArray, a wxArrayInt
void ShowInvalidStripRange()
{
	CLayout* pLayout = gpApp->m_pLayout;
	wxArrayInt* pInvalidStripArray = pLayout->GetInvalidStripArray();
	int nCount = pInvalidStripArray->GetCount();
	int index = 0;
	int aValue;
	wxString thisValue;
	wxString listOfValues;
	if (!pInvalidStripArray->IsEmpty() && nCount > 0)
	{
		listOfValues.Empty();
		listOfValues = _T("Invalid strips in m_pInvalidStripArray: ");
		for (index = 0; index < nCount; index++)
		{
			thisValue.Empty();
			aValue = pInvalidStripArray->Item(index);
			thisValue = thisValue.Format(_T(" %d  "),aValue);
			listOfValues += thisValue;
			wxLogDebug(listOfValues);
		}
	}
	else
	{
		wxLogDebug(_T("No invalid strips in m_pInvalidStripArray"));
	}
}
*/

#ifdef __WXMAC__
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return Free memory in megabytes
/// \remarks
/// Note: The wxWidgets function ::wxGetFreeMemory returns a nonsense value on Mac. This function
/// interrogates the internal structures of the Mach kernel on MacOS X to get a value for free memory.
/// Added GDLC 6May11 to avoid including the MachOS headers inside the class CAdapt_ItApp.
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#undef __BEGIN_DECLS
#undef __END_DECLS
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }

#include <mach/mach.h>
#include <mach/mach_host.h>

wxMemorySize MacGetFreeMemory()
{
	mach_port_t				host_port = mach_host_self();
	mach_msg_type_number_t	host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
	vm_size_t				pagesize;
	vm_statistics_data_t	vm_stat;
	
	host_page_size(host_port, &pagesize);
	
	if (host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size) != KERN_SUCCESS) return -1LL;
	
	//	natural_t   mem_used = (vm_stat.active_count + vm_stat.inactive_count + vm_stat.wire_count) * pagesize;
	natural_t   mem_free = vm_stat.free_count * pagesize;
	//	natural_t   mem_total = mem_used + mem_free;
	
	return static_cast <wxMemorySize> (mem_free);
}

#endif	/* __WXMAC__ */



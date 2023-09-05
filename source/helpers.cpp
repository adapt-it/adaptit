/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			helpers.cpp
/// \author			Bruce Waters, revised for wxWidgets by Bill Martin
/// \date_created	6 January 2005
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file containing some helper functions used by Adapt It.
/// BEW 12Apr10, all changes for supporting doc version 5 are done for this file
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "helpers.h"
#endif

// whm 25Jun2015 added the following wxCHECK_GCC_VERSION() statement to prevent
//"unrecognized command line options" when compiling with GCC version 4.8 or earlier
#include <wx/defs.h>
#if defined(__GNUC__) && !wxCHECK_GCC_VERSION(4, 6)
	#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	#pragma GCC diagnostic ignored "-Wsign-compare"
	#pragma GCC diagnostic ignored "-Wwrite-strings"
	#pragma GCC diagnostic ignored "-Wsizeof-pointer-memaccess"
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

#include <wx/docview.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/colour.h>
#include <wx/dir.h>
#include <wx/textfile.h>
#include <wx/stdpaths.h>
#include <wx/fileconf.h> // for wxFileConfig
#include <wx/display.h> // for multiple monitor metrics,
						// used for dialog relocation away from active pile
#include <wx/string.h>

#include "Adapt_It.h"
#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"
#include "Adapt_ItCanvas.h"
#include "SourcePhrase.h"
#include "MainFrm.h"
#include "WaitDlg.h"
#include "XML.h"
#include "SplitDialog.h"
#include "ExportFunctions.h"
#include "PlaceInternalMarkers.h"
#include "Uuid_AI.h" // for uuid support
//#include "wxUUID.h"
// the following includes support friend functions for various classes
#include "TargetUnit.h"
#include "KB.h"
#include "Pile.h"
#include "Strip.h"
#include "Layout.h"
#include "KBEditor.h"
#include "RefString.h"
#include "KBSharingAuthenticationDlg.h"
#include "helpers.h"
#include "tellenc.h"
#include "RefStringMetadata.h"
#include "LanguageCodesDlg.h"
#include "UsernameInput.h"
#include "KbServer.h"
#include "md5_SB.h"
#include "ConsistentChanger.h"

//#if defined (_KBSERVER)
#include <stdio.h>
#include <stdlib.h>
// BEW integrating the external .c functions, as extern "C" functions (no name mangling) probably needs
// these extra includes, for wchar_t support, wxChar, & wxString
#include <string.h>
#include <time.h>
// whm 6May2022 commented out the next include since it breaks Linux and Mac builds. (BEW 9May22 commenting out broke nothing)
// This #include path was previously commented out in KbServer.cpp line 76, KbServer.h line 160,
// and two places in KBSharingMgrTabbedDlg.cpp lines 58 and 59.
//#include "C:\Program Files (x86)\MariaDB 10.5\include\mysql\mysql.h"

// GDLC 20OCT12 md5.h is not needed for compiling helpers.cpp
//#include "md5.h"

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
//extern bool gbTryingMRUOpen; // whm 1Oct12 removed
extern bool gbConsistencyCheckCurrent;

// globals needed due to moving functions here from mainly the view class
// next group for auto-capitalization support
extern bool	gbAutoCaps;
extern bool	gbNonSourceIsUpperCase;

/// Length of the byte-order-mark (BOM) which consists of the three bytes 0xEF, 0xBB and 0xBF
/// in UTF-8 encoding.
#define nBOMLen 3

/// Length of the byte-order-mark (BOM) which consists of the two bytes 0xFF and 0xFE in
/// in UTF-16 encoding.
#define nU16BOMLen 2

#ifdef _UNICODE
/* Used only in the old form of GetNewFile()
/// The UTF-8 byte-order-mark (BOM) consists of the three bytes 0xEF, 0xBB and 0xBF
/// in UTF-8 encoding. Some applications like Notepad prefix UTF-8 files with
/// this BOM.
static wxUint8 szBOM[nBOMLen] = {0xEF, 0xBB, 0xBF};

/// The UTF-16 byte-order-mark (BOM) which consists of the two bytes 0xFF and 0xFE in
/// in UTF-16 encoding, reverse order if big-endian
static wxUint8 szU16BOM[nU16BOMLen] = {0xFF, 0xFE};
static wxUint8 szU16BOM_BigEndian[nU16BOMLen] = {0xFE, 0xFF};
*/
#endif


//  helper functions


std::string MakeStdString(wxString str)
{
	const wxCharBuffer tempBuf = str.mb_str(wxConvUTF8);
	std::string myStr(tempBuf);
	return myStr;
}


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

// BEW added 25Aug20 to enable waitDlg to persist long enough for user to read
void DoMessageDelay(int hundredths)
{
	// set up a time delay loop, according to the m_nCurDelay value (1/100ths sec)
	wxDateTime startTime;
	startTime.SetToCurrent(); // need to initialize or get run-time error

	int millisecsStart = startTime.GetMillisecond();
	int secsStart = startTime.GetSecond();
	int nSpan = hundredths * 10; // convert hundredths to milliseconds
	wxDateTime currentTime;

	// work out the time to be elapsed before breaking out of loop
	int newmillisecs = millisecsStart + nSpan;
	int endsecs = secsStart + newmillisecs / 1000; // same or up to 3 seconds larger
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
/// \return nothing
/// \remarks
/// A Helper function for doing a time delay in 1/100ths of a second.
/// Based on helper function DoDelay() but does not use the global
/// m_nCurDelay; instead the time in hundredths is taken from the
/// int parameter.
/////////////////////////////////////////////////////////////
void Delay(int hundredths)
{
    // set up a time delay loop, according to the m_nCurDelay value (1/100ths sec)
    wxDateTime startTime;
    startTime.SetToCurrent(); // need to initialize or get run-time error

    int millisecsStart = startTime.GetMillisecond();
    int secsStart = startTime.GetSecond();
    int nSpan = hundredths * 10; // convert hundredths to milliseconds
    wxDateTime currentTime;

    // work out the time to be elapsed before breaking out of loop
    int newmillisecs = millisecsStart + nSpan;
    int endsecs = secsStart + newmillisecs / 1000; // same or up to 3 seconds larger
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

//////////////////////////////////////////////////////////////
/// \return nothing
/// \param val	-> the int value to be converted to a wxString
/// \param str  <- the wxString equivalent of the val
///  helper for converting an integer to a wxString
/////////////////////////////////////////////////////////////
void wxItoa(int val, wxString& str)
{
	wxString valStr;
	valStr << val;
	str = valStr;
}

//////////////////////////////////////////////////////////////
/// \return nothing
/// \param val	-> the int value to be converted to a CBString
/// \param str  <- the CBString equivalent of the val
///  helper for converting an integer to a CBString
/////////////////////////////////////////////////////////////
void wxItoa(int val, CBString& str)
{
	wxString valStr;
	valStr << val;
	wxCharBuffer tempBuf = valStr.mb_str(wxConvUTF8);
	str = CBString(tempBuf);
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

void ExtractVerseNumbersFromBridgedVerse(wxString tempStr,int& nLowerNumber,int& nUpperNumber)
{
	// whm 18Jul12 moved from GetSourceTextFromEditor sources to helpers.
	// The incoming tempStr will have a bridged verse string in the form "3-5".
	// We parse it and convert its lower and upper number sub-strings into int values to
	// return to the caller via the reference int parameters.
	int nPosDash;
	wxString str = tempStr;
	str.Trim(FALSE);
	str.Trim(TRUE);
	wxString subStrLower, subStrUpper;
	nPosDash = str.Find('-',TRUE);
	wxASSERT(nPosDash != wxNOT_FOUND);
	subStrLower = str.Mid(0,nPosDash);
	subStrLower.Trim(TRUE); // trim any whitespace at right end
	subStrUpper = str.Mid(nPosDash+1);
	subStrUpper.Trim(FALSE); // trim any whitespace at left end
	nLowerNumber = wxAtoi(subStrLower);
	nUpperNumber = wxAtoi(subStrUpper);
}

wxString AbbreviateColonSeparatedVerses(const wxString str)
{
	// whm 18Jul12 moved from GetSourceTextFromEditor sources to helpers.
	// Abbreviates a colon separated list of verses that originally looks like:
	// 1:3:4:5:6:9:10-12:13:14:20:22:24:25,26:30:
	// changing it to this abbreviated from:
	// 1, 3-6, 9, 10-12, 13-14, 20, 22, 24-26, 30
	// Note: Bridged verses in the original are not combined with their contiguous
	// neighbors, so 9, 10-12, 13-14 does not become 9-14.
	wxString tempStr;
	tempStr.Empty();
	wxStringTokenizer tokens(str,_T(":"),wxTOKEN_DEFAULT); // empty tokens are never returned
	wxString aToken;
	int lastVerseValue = 0;
	int currentVerseValue = 0;
	bool bBridgingVerses = FALSE;
	while (tokens.HasMoreTokens())
	{
		aToken = tokens.GetNextToken();
		aToken.Trim(FALSE); // FALSE means trim white space from left end
		aToken.Trim(TRUE); // TRUE means trim white space from right end
		int len = aToken.Length();
		int ct;
		bool bHasNonDigitChar = FALSE;
		for (ct = 0; ct < len; ct++)
		{
			if (!wxIsdigit(aToken.GetChar(ct)) && aToken.GetChar(ct) != _T('-'))
			{
				// the verse has a non digit char other than a '-' char so let it stand by
				// itself
				bHasNonDigitChar = TRUE;
			}
		}
		if (aToken.Find(_T('-')) != wxNOT_FOUND || bHasNonDigitChar)
		{
			// the token is a bridged verse number string, i.e., 2-3
			// or has an unrecognized non-digit char in it, so we let
			// it stand by itself in the abbriviated tempStr
            // whm 20Sept2017 modified to fix logic. If bBridgingVerses is TRUE,
            // we need to close off the current bridge before adding this token's bridge
            tempStr += _T('-');
            tempStr << lastVerseValue;
            tempStr += _T(", ") + aToken;
            bBridgingVerses = FALSE;
		}
		else
		{
			// the token is a normal verse number string
			currentVerseValue = wxAtoi(aToken);
			if (lastVerseValue == 0)
			{
				// we're at the first verse element, which will always get stored as is
				tempStr = aToken;
			}
			else if (currentVerseValue - lastVerseValue == 1)
			{
				// the current verse is in sequence with the last verse, continue
				bBridgingVerses = TRUE;
			}
			else
			{
				// the currenttVerseValue and lastVerseValue are not contiguous
				if (bBridgingVerses)
				{
					tempStr += _T('-');
					tempStr << lastVerseValue;
					tempStr += _T(", ") + aToken;
				}
				else
				{
					tempStr += _T(", ") + aToken;
				}
				bBridgingVerses = FALSE;
			}
			lastVerseValue = currentVerseValue;
		}
	}
	if (bBridgingVerses)
	{
		// close off end verse of the bridge at the end
		tempStr += _T('-');
		tempStr << lastVerseValue;
	}

	return tempStr;
}

/*
// whm 20Sept2017 The following function had faulty logic. I've changed the
// code that called this function to use clear logic, so this function is
// fautly and now unused.
bool EmptyVerseRangeIncludesAllVersesOfChapter(wxString emptyVersesStr)
{
	// whm 18Jul12 moved from GetSourceTextFromEditor sources to helpers.
	// The incoming emptyVersesStr will be of the abbreviated form created by the
	// AbbreviateColonSeparatedVerses() function, i.e., "1, 2-6, 28-29" when the
	// chapter has been partly drafted, or "1-29" when all verses are empty. To
	// determine whether the empty verse range includes all verses of the chapter
	// or not, we parse the incoming emptyVersesStr to see if it is of the later
	// "1-29" form. There will be a single '-' character and no comma delimiters.
    // The caller ensures that the incoming emptyVersesStr will NOT be an empty
    // string.
    wxASSERT(!emptyVersesStr.IsEmpty());
	bool bAllVersesAreEmpty = FALSE;
	wxString tempStr = emptyVersesStr;
	// whm modified 6Feb12 to correct the situation where a range such as 2-47 was
	// being returned as TRUE, because the range didn't start at verse 1.
	// Check if the first number in emptyVersesStr is a 1 or not. If it is not a
	// 1, then we know that it won't include all verses of the chapter.
	wxStringTokenizer tkz(tempStr,_T(",-")); // tokenize by commas and hyphens
	wxString token;
	int tokCt = 1;
	while (tkz.HasMoreTokens())
	{
		token = tkz.GetNextToken();
		token.Trim(FALSE);
		token.Trim(TRUE);
		if (tokCt == 1 && token != _T("1"))
			return FALSE;
		tokCt++;
	}

	// Check if there is no comma in the emptyVersesStr. Lack of a comma indicates
	// that all verses are empty
	if (tempStr.Find(',') == wxNOT_FOUND)
	{
		// There is no ',' that would indicate a gap in the emptyVersesStr
		bAllVersesAreEmpty = TRUE;
	}
	// Just to be sure do another test to ensure there is a '-' and only one '-'
	// in the string.
    // whm 20Sept2017removed following test. Its logic is flawed. Test is not needed
	//if (tempStr.Find('-') != wxNOT_FOUND && tempStr.Find('-',FALSE) == tempStr.Find('-',TRUE))
	//{
	//	// the position of '-' in the string is the same whether
	//	// determined from the left end or the right end of the string
	//	bAllVersesAreEmpty = TRUE;
	//}

	return bAllVersesAreEmpty;
}
*/

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
#ifdef _DEBUG
		wxASSERT(FALSE);
#endif
		return outStr;
	}
	int length = str.Len();
	if (first > length -1)
	{
#ifdef _DEBUG
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


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \return TRUE if file at path is empty (either zero length or 3 bytes of BOM), FALSE if file has content
/// \param	path		-> the path to the file being checked
/// \remarks
/// Assumes that ::wxFileExists(path) has been called previously to verify that the file at path does exist.
/// Checks if the length of the file at path is 3 bytes or less. If so returns TRUE, otherwise returns FALSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FileIsEmpty(wxString path)
{
	wxLogNull logNo;
	wxULongLong fileLen = wxFileName::GetSize(path);
	// If file has zero bytes it is empty by definition
	bool bIsEmpty = FALSE;
	if (fileLen == 0 || fileLen == wxInvalidSize)
	{
		bIsEmpty = TRUE;
		return bIsEmpty;
	}
	// If file only has 3 bytes and those bytes are "\xEF\xBB\xBF" then it is an "empty" file that has a UTF8 BOM.
	if (fileLen == 3)
	{
		wxFile f(path,wxFile::read);
		bool bOpenedOK = f.IsOpened();
		if (!bOpenedOK)
		{
			// If we can't open the file assume it is "empty" since fileLen == 3
			bIsEmpty = TRUE;
			return bIsEmpty;
		}
		wxFileOffset fileLenBytes;
		fileLenBytes= f.Length();
		fileLenBytes = fileLenBytes; // to avoid compiler "set but not used" warning in release build
		wxASSERT(fileLenBytes == 3); // should agree with wxFileName::GetSize() call above
		// read the raw byte data into pByteBuf (char buffer on the heap)
		char* pByteBuf = (char*)malloc(3 + 1);
		memset(pByteBuf,0,3 + 1); // fill with nulls
		f.Read(pByteBuf,3);
		wxASSERT(pByteBuf[3] == '\0'); // should end in NULL
		f.Close();
		if (*pByteBuf == '\xEF' && *(pByteBuf+1) == '\xBB' && *(pByteBuf+2) == '\xBF')
		{
			bIsEmpty = TRUE;
			return bIsEmpty;
		}
		free((void*)pByteBuf);

	}
	return bIsEmpty;
}


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
#ifdef Output_Default_Style_Strings
	// Whenever we are building the Unix-default strings we don't want the app to determine
	// if there is a newer version of the AI_USFM_full.xml file, so return FALSE
	return FALSE;
#else
	wxFileName fnTrueIfNewer(fileAndPathTrueIfNewer);
	wxFileName fnFalseIfNewer(fileAndPathFalseIfNewer);
	wxDateTime dtTrueIfNewerAccessTime,dtTrueIfNewerModTime,dtTrueIfNewerCreateTime;
	wxDateTime dtFalseIfNewerAccessTime,dtFalseIfNewerModTime,dtFalseIfNewerCreateTime;
	fnTrueIfNewer.GetTimes(&dtTrueIfNewerAccessTime,&dtTrueIfNewerModTime,&dtTrueIfNewerCreateTime);
	fnFalseIfNewer.GetTimes(&dtFalseIfNewerAccessTime,&dtFalseIfNewerModTime,&dtFalseIfNewerCreateTime);
	// Check if we have a newer version in fileAndPathTrueIfNewer
	return dtTrueIfNewerModTime > dtFalseIfNewerModTime;
#endif
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
/// \return TRUE only if s contains at least one character and all characters in s are
///         either digits or letters.
///			If s is empty, or contains any whitespace or other non-digit characters, this function
///			returns false.
/// \param	s			-> the string to be examined
/// \remarks
/// BEW added 24May12,
/// The wxWidgets function calls IsAnsiLetterOrDigit() internally
///////////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsAnsiLettersOrDigitsOnly(wxString s)
{
	int i;
	for (i = 0; i < (int)s.Length(); ++i) {
		if (!IsAnsiLetterOrDigit(s[i])) return false;
	}
	return i > 0;
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
		MainList->DeleteNode(pos);
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
	//bool bFoundDouble; // set but not used
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
		//bFoundDouble = FALSE;
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
			// BEW changed 24Feb12, the above "MUST NOT" is too strong, because it meant
			// that a space between successive closing quotes would be removed, and we
			// don't want that to happen. We'll accept a space provided a quote character
			// follows it.
			// BEW 21Dec22 we need to support hairspace here (U+200A). It's a whitespace
			// and so is not listed in the punctuation correspondences; yet it could precede
			// a closing quote - and if it does, we need to retain it for the GUI restoration
			// of puncts. So hack a solution here.
			wxChar hairspace = (wxChar)0x200A;
			if (ch == _T(' ') || ch == hairspace)
			{
				// it's a space or hairspace 
				bool bIsHairspace = ch == hairspace ? TRUE : FALSE;
				bool bIsLatinSpace = ch == _T(' ') ? TRUE : FALSE;
				if (j + 1 < len)
				{
					// there's another character available, check if its a quote one
					//wxChar chNext = rStr.GetChar(j + 1);
					//bool bQuote = pApp->GetDocument()->IsClosingQuote(&chNext);
					//if (bQuote)
					if (bIsLatinSpace || bIsHairspace)
					{
						if (bIsHairspace)
						{
							// ch is hairspace
							s += hairspace;
						}
						else if (bIsLatinSpace)
						{
							// ch is normal (latin) space
							s += _T(' ');
						}
					}
					else
					{
						// If neither, check for opening quote
						bool bQuote = FALSE; // init
						bQuote = pApp->GetDocument()->IsOpeningQuote(&ch);
						if (bQuote)
						{
							s += ch;
						}
						// The else block below should pick up a closing quote at
						// the next iteration
					}
				}
			}
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

// BEW 24Oct14, no changes needed for support of USFM nested markers
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

// BEW 24Oct14, no changes needed for support of USFM nested markers
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

// BEW 29Aug23, this might prove useful for EOL decisions, and how many white chars there are at pChar
bool IsEndOfLine(wxChar* pChar, int& nCount)
{
	wxChar* ptr = pChar;
	wxChar* pNext = (ptr + 1);
	nCount = 0; //init
	wxChar CR = _T('\r');
	wxChar LF = _T('\n');
	// Longest first
	if (*ptr == CR && *pNext == LF)
	{
		// Windows
		nCount = 2;
		return TRUE;
	}
	else if (*ptr == LF)
	{
		// Linux
		nCount = 1;
		return TRUE;
	}
	else if (*ptr == CR)
	{
		// Mac
		nCount = 1;
		return TRUE;
	}
	return FALSE; // and nCount still zero
}


wxString GetStringFromBuffer(const wxChar* ptr, int itemLen)
{
	return wxString(ptr,itemLen);
}

// whm 18Aug2023 Note:
// The Parse_Number() function below is only defined here in helpers.cpp,
// and it is called by functions in the following two source files:
// In Adapt_It.cpp:
// 	bool CAdapt_ItApp::BookHasChapterAndVerseReference(wxString fileAndPath, wxString chapterStr, wxString verseStr) - 2x
// In CollabUtilities.cpp:
//	bool CopyTextFromTempFolderToBibleditData(wxString projectPath, wxString bookName,
//		int chapterNumber, wxString tempFilePathName, wxArrayString& errors)
//	wxArrayString GetUsfmStructureAndExtent(wxString& fileBuffer) - 2x
int Parse_Number(wxChar *pChar, wxChar *pEnd) // whm 10Aug11 added wxChar *pEnd parameter
{
	wxChar* ptr = pChar;
	int length = 0;
	// whm 10Aug11 added ptr < pEnd test in while loop below because a
	// number can be at the end of a string followed by a null character. If the
	// ptr < pEnd test is not included the Is_NonEol_WhiteSpace() call would
	// return a FALSE result and the while loop would iterate one too many times
	// returning a bad length
	// whm added 4Sep11 test to halt parsing if a backslash is encountered
	while (ptr < pEnd && !Is_NonEol_WhiteSpace(ptr) && *ptr != _T('\n') && *ptr != _T('\r') && *ptr != _T('\\'))
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

//#if defined(FWD_SLASH_DELIM)

wxString ZWSPtoFwdSlash(wxString& str)
{
	if (gpApp->m_bFwdSlashDelimiter)
	{
		wxString ZWSP = (wxChar)0x200B;
		wxString slash = _T('/');
		// Do the replacement only if support for / as word-break is enabled
		str.Replace(ZWSP, slash); // ignore returned count of replacements
	}
	return str; // return by value, in case we want to assign it elsewhere
}

wxString FwdSlashtoZWSP(wxString& str)
{
	if (gpApp->m_bFwdSlashDelimiter)
	{
		wxString ZWSP = (wxChar)0x200B;
		wxString slash = _T('/');
		// Do the replacement only if support for / as word-break is enabled
		str.Replace(slash, ZWSP); // ignore returned count of replacements
	}
	return str; // return by value, in case we want to assign it elsewhere
}

// the following function was tested in OnInit() at lines 20250-54 & works right
bool HasFwdSlashWordBreak(CSourcePhrase* pSrcPhrase)
{
	if (gpApp->m_bFwdSlashDelimiter)
	{
		wxString slash = _T('/');
		wxString wdBreakStr = pSrcPhrase->GetSrcWordBreak();
		if (wdBreakStr.Find(slash) != wxNOT_FOUND)
			return TRUE;
	}
	// If control gets to here, the answer is "no" so return FALSE
	return FALSE;
}

/// \return			Return by value the output string resulting from the input string, str, being
///					processed by the passed in CC table, pathToCCTable. Return the input string
///					unchanged if the app's m_bFwdSlashDelimiter flag is FALSE (Dennis's feature is
///					in OFF state whenever the Preferences setting for that flag is FALSE)
/// \param	whichTable   ->		What table to use. It has to be the path to a cc table within the
///								folder _CCTABLE_INPUTS_OUTPUTS, located in the Adapt It Unicode Work
///								folder; that folder is pointed at by app->m_ccTableInputsAndOutputsFolderPath
/// \param  str			  ->	The text string (source or target text) which is to have / delimiter
///								either inserted at punctuation, or removed at punctuation, according
///								two which cc table the calling function supplies. The function is
///								not generic, so it cannot be used for processing any str with any
///								desired cc table; this is deliberate, so that no setup of table path
///								needs to be done prior to calling. We use enum values to select the table.
/// \remarks
/// BEW created 23Apr15. For support of using / as a pseudo-whitespace word-breaking character. This
/// feature is to be turned on or off by a checkbox in Preferences.
/// There is no limit on str size - internally its length is computed and malloc used to set up
/// processing buffer for input and output. The code is reused & tweaked from that in the view class's
/// DoConsistentChanges() function
wxString DoFwdSlashConsistentChanges(enum FwdSlashDelimiterSupport whichTable, wxString& str)
{
	if (str.IsEmpty())
		return _T("");
	// If support is turned off, return a copy of the passed in string
	if (!gpApp->m_bFwdSlashDelimiter)
	{
		return str;
	}
	// If control gets to here, the cc tables are to be used...
	wxString outputStr = _T("");
	wxString pathToCCTable = gpApp->m_ccTableInputsAndOutputsFolderPath;
	wxASSERT(!pathToCCTable.IsEmpty());
	wxString tableName = _T("");
	switch (whichTable)
	{
	case insertAtPunctuation:
		pathToCCTable += gpApp->PathSeparator + _T("FwdSlashInsertAtPuncts.cct");
		break;
	case removeAtPunctuation:
		pathToCCTable += gpApp->PathSeparator + _T("FwdSlashRemoveAtPuncts.cct");
		break;
	}

	// Next, test for the cct file being present. If it isn't, warn the user and just return
	// the unmodified input string
	bool bFileExists = wxFileExists(pathToCCTable);
	if (!bFileExists)
	{
		wxString title = _("No table file");
		wxString msg;
		msg = msg.Format(_("The consistent changes table file: %s , does not exist in the folder _CCTABLE_INPUTS_OUTPUTS"),
			pathToCCTable.c_str());
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        gpApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		gpApp->LogUserAction(msg);
		return str; // return the input string unchanged
	}

	// str will be a UTF-16 wxString. We have to convert to UTF8, run the
	// resulting string through the CCProcessBuffer() function with a minimum of string copying
	// to maximize speed, and then convert back to UTF-16 and return it to the caller as an LPTSTR;
	// the strings stored in the buffers will be null delimited

	// Uses the buffer-safe new conversion macros in VS 2003, which
	// use malloc for buffer allocation of long string to be converted, etc.
	int nInLength = 0;
	int nOutLength = 0;

	// For this implimentation, we get the input string as utf-8, and its length (in bytes)
	// and use malloc to create a suitable output buffer - we don't know how much bloat
	// the cc changes will cause when inserting / at punctuation locations, but since words
	// can be assumed to be at least an average of 6 bytes, and punctuation somewhat uncommon,
	// we will make the output buffer just 20% larger than the input one. That ought to be
	// safe for large files; for small files, expand by a larger factor

	// The wxString::mb_str() method returns a wxCharBuffer. The wxConvUTF8 is a predefined
	// instance of the wxMBConvUTF8 class which converts between Unicode (UTF-16) and UTF-8.
	wxCharBuffer tempBuf = str.mb_str(wxConvUTF8);
	CBString psz(tempBuf); // use this for the input buffer, with a (char*) cast
	nInLength = strlen(psz) + 1; // + 1 for the null byte at the end
	int anOutputLen = nInLength;
	if (anOutputLen < 500)
		anOutputLen += anOutputLen; // double
	else if (anOutputLen < 2000)
		anOutputLen = (anOutputLen * 3)/2; // 50% bigger
	else
		anOutputLen = (anOutputLen * 6)/5; // 20% bigger
	char* pOutStr = (char*)malloc((size_t)anOutputLen);
	memset(pOutStr, 0, (size_t)anOutputLen); // fill with nulls
	nOutLength = anOutputLen; // utf8ProcessBuffer requires it to be set to the output buffer size (in bytes)

	CConsistentChanger* pConsistentChanger = new CConsistentChanger;
	wxString sError; // will be empty if loading proceded without error
	wxString ccErrorStr;
	sError = pConsistentChanger->loadTableFromFile(pathToCCTable);
	if (!sError.IsEmpty())
	{
		// There was an error, tell the user and return the input string unchanged
		wxString msg2;
		msg2 = msg2.Format(_("  Warning: the consistent changes table: %s, was not successfully loaded."),
			pathToCCTable.c_str());
		sError += msg2;
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        gpApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(sError, _("Table load failure"), wxICON_INFORMATION | wxOK);
		gpApp->LogUserAction(sError);
		free(pOutStr);
		delete pConsistentChanger;
		return str;
	}
	int iResult = 0;

	// whm note: the following line is where consistent changes does its work
	iResult = pConsistentChanger->utf8ProcessBuffer((char*)psz, nInLength, pOutStr, &nOutLength);

	// if there was an error, just return the unaltered original string & warn user
	if (iResult)
	{
		ccErrorStr.Format(_(" Processing the CC table failed, for table: %s   Error code: %d"),
			pathToCCTable.c_str(), iResult);
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        gpApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(ccErrorStr, _T(""), wxICON_EXCLAMATION | wxOK);
		gpApp->LogUserAction(ccErrorStr);
		free(pOutStr);
		delete pConsistentChanger;
		return str;
	}

	// convert back to UTF-16 and return the converted string
	pOutStr[nOutLength] = '\0';	// ensure it is null terminated at the correct location
	CBString tempBuff(pOutStr);
	outputStr = gpApp->Convert8to16(tempBuff);

	// make sure to free the malloc buffer before returning, and the pConsistentChanger instance
	free(pOutStr);
	delete pConsistentChanger;
	return outputStr;
}
//#endif

// BEW 23Apr15 provisional support added for / being treated as a word-breaking
// character (as if it is whitespace) -- support certain east asian languages
// 
// whm 18Aug2023 Note: The following Is_NonEol_WhiteSpace() function is only 
// called within functions defined here in helpers.cpp which include these:
//	bool Is_ChapterMarker(wxChar* pChar)
//	bool Is_VerseMarker(wxChar* pChar, int& nCount)
//	int Parse_Number(wxChar* pChar, wxChar* pEnd)
//	int Parse_NonEol_WhiteSpace(wxChar* pChar)
//	int Parse_Marker(wxChar* pChar, wxChar* pEnd)
//	bool Is_Marker(wxChar* pChar, wxChar* pEnd)
//
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
//#if defined(FWD_SLASH_DELIM)
		// BEW 23Apr15, support / as if a whitespace word-breaker
		if (gpApp->m_bFwdSlashDelimiter)
		{
			if (*pChar == _T('/'))
				return TRUE;
		}
//#endif
		// BEW 3Aug11, support ZWSP (zero-width space character, U+200B) as well, and from
		// Dennis Drescher's email of 3Aug11, also various others
		// BEW 4Aug11 changed the code to not test each individually, but just test if
		// wxChar value falls in the range 0x2000 to 0x200B - which is much quicker; and
		// treat U+2060 individually
		wxChar WJ = (wxChar)0x2060; // WJ is "Word Joiner"
		if (*pChar == WJ || ((UInt32)*pChar >= 0x2000 && (UInt32)*pChar <= 0x200B))
		{
			return TRUE;
		}
#endif
	}
	return FALSE;
}

// whm 18Aug2023 Note:
// This Parse_NonEol_WhiteSpace() function is only defined here in helpers.cpp.
// It is not called by other functions here in helpers.cpp, but is called by functions
// in the following source files:
// In Adapt_It.cpp:
//	bool CAdapt_ItApp::BookHasChapterAndVerseReference(wxString fileAndPath, wxString chapterStr, wxString verseStr)
// 
int Parse_NonEol_WhiteSpace(wxChar *pChar)
{
	int	length = 0;
	wxChar* ptr = pChar;
	// whm 10Aug11 added. If ptr is pointing at the null at the end of the buffer
	// just return the default zero length for safety sake.
	if (*ptr == _T('\0'))
		return length;
	while (Is_NonEol_WhiteSpace(ptr)) // BEW 23Apr15 contains tweak for support of / as word-breaker
	{
		length++;
		ptr++;
	}
	return length;
}

// BEW 24Oct14, no changes needed to support USFM nested markers
// 
// whm 18Aug2023 Note:
// The Parse_Marker() function defined here in helpers.cpp is not defined elsewhere in our code base,
// and is only called from these functions within CollabUtilities.cpp:
//	CopyTextFromTempFolderToBibleditData()
//	GetUsfmStructureAndExtent() - 3x
//	MapMd5ArrayToItsText()
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

// whm 18Aug2023 Note:
// The Is_Marker() function defined here in helpers.cpp is not defined elsewhere in our code base,
// and is only called from these functions within CollabUtilities.cpp:
//	wxArrayString GetUsfmStructureAndExtent(wxString& fileBuffer) - 3x
//	void MapMd5ArrayToItsText(wxString& text, wxArrayPtrVoid& mappingsArr, wxArrayString& md5Arr)
//	wxString RemoveIDMarkerAndCode(wxString text)
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

// whm 25Oct2022 added
// Parses out the chapter number string and verse number string from a Ch:Vs style RefStr input.
// Returns the ChStr and VsStr in reference parameters.
// If the RefStr has no colon character 0:0 is returned. 
// If Ch part is mising 0 is returned for that part, i.e., 0:...; if Vs part is missing 0 is returned for that part, i.e., ...:0.
void ParseChVsFromReference(wxString RefStr, wxString& ChStr, wxString& VsStr)
{
	wxString Ch = _T("0");
	wxString Vs = _T("0");
	ChStr = Ch;
	VsStr = Vs;
	if (RefStr.IsEmpty())
		return;
	wxString Colon = _T(":");
	if (RefStr.Find(Colon) == wxNOT_FOUND)
	{
		return;

	}
	else
	{
		int colonPos = RefStr.Find(Colon);
		ChStr = RefStr.Mid(0, colonPos);
		if (ChStr.IsEmpty())
			ChStr = Ch;
		VsStr = RefStr.Mid(colonPos + 1);
		if (VsStr.IsEmpty())
			VsStr = Vs;
		return;
	}
}

// whm 25Oct2022 added
// Returns the Chapter and Verse reference at the active location, if any, in the form of "ch:vs"
// Note: the active location may be within a bridged verse such as ch:vs1-vs3, or ch:vs1,vs3
// See NormalizeChVsRefToInitialVsOfAnyBridge() function below to normalize a rederence to only
// indicate the beginning verse of a bridged or comma'ed reference.
wxString  GetChVsRefFromActiveLocation()
{
	// get a pointer to the view
	CAdapt_ItView* pView = (CAdapt_ItView*)gpApp->GetView();
	wxASSERT(pView != NULL);
	CSourcePhrase* pSrcPhrase = NULL;
	pSrcPhrase = pView->GetSrcPhrase(gpApp->m_nActiveSequNum);
	wxASSERT(pSrcPhrase);
	wxString ChVsStr;
	ChVsStr = pView->GetChapterAndVerse(pSrcPhrase);
	return ChVsStr;
}

// whm 25Oct2022 added
// Returns the Chapter and Verse reference at the active location, if any, in the normalized form "ch:vs"
// This function normalizes a rederence to only indicate the beginning verse of a bridged or comma'ed 
// reference if the reference is a bridged or comma'ed reference, i.e., ch:vs1-vs3 or ch:vs1,vs3
// If there is no bridged or comma'ed reference in the input string, the input string is simply
// returned unchanged.
wxString  NormalizeChVsRefToInitialVsOfAnyBridge(wxString bridgedRef)
{
	wxString str;
	str = bridgedRef;
	wxString Ch;
	wxString Vs;
	wxString resultStr;
	ParseChVsFromReference(str, Ch, Vs);

	int posHyphen = Vs.Find(_T("-"));
	int posComma = Vs.Find(_T(","));
	if (posHyphen != wxNOT_FOUND)
		Vs = Vs.Mid(0, posHyphen);
	if (posComma != wxNOT_FOUND)
		Vs = Vs.Mid(0, posComma);
	resultStr = Ch + _T(":") + Vs;
	return resultStr;
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
// BEW 12Nov22 added, in support of space or spaces after a word but before an endMkr
// BEW 17Dec22, added hairspace (U+200A) to the while test, so latin space or hairspace
// will be treated alike (hairspace needed for MATBVM.SFM from Gerald Harkins)
int	CountSpaces(wxChar* pChar, wxChar* pEnd)
{
	wxChar hairspace = (wxChar)0x200A;
	int counter = 0; wxChar* ptr = pChar;
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	//while ((ptr < pEnd) && ((*ptr == _T(' ')) || (*ptr == hairspace)) ) BEW 26May23 removed, not broad enough
	while ((ptr < pEnd) && pDoc->IsWhiteSpace(ptr))
	{
		ptr++;
		counter++;
	}
	return counter;
}

wxString MakeUNNNN_Hex(wxString& chStr) 
{
	wxString prefix = _T(""); // some people said U+ makes the strings too wide, so leave
							 // it off_T("U+");
	// whm 11Jun12 Note: I think chStr will always have at least a value of T('\0'), so
	// GetChar(0) won't ever be called on an empty string, but to be safe test for empty
	// string.
	wxChar theChar;
	if (!chStr.IsEmpty())
		theChar = chStr.GetChar(0);
	else
		theChar = _T('\0');
	wxChar str[6] = { _T('\0'),_T('\0'),_T('\0'),_T('\0'),_T('\0'),_T('\0') };
	wxChar* pStr = str;
	wxSnprintf(pStr, 6, _T("%x"), (int)theChar);
	wxString s = pStr;
	if (s == _T("0"))
	{
		s.Empty();
		return s;
	}
	int len = s.Length();
	if (len == 2)
		s = _T("00") + s;
	else if (len == 3)
		s = _T("0") + s;
	return prefix + s;
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

// BEW created 17Nov16 for
// use in CAdapt_ItApp::EnsureProperCapitalization()
wxString GetTargetPunctuation(wxString wordOrPhrase, bool bFromWordEnd)
{
	wxString puncts = wxEmptyString;
	if (wordOrPhrase.IsEmpty())
		return puncts;
	CAdapt_ItApp* pApp = &wxGetApp();
	wxString punctSet = pApp->m_punctuation[1];
	int offset = wxNOT_FOUND;
	// Ensure latin space is included in the puncts set, in this function
	wxChar space = _T(' ');
	offset = punctSet.Find(space);
	if (offset == wxNOT_FOUND)
	{
		// Add space to the set, because there may be nested quotes and
		// in that circumstance we want to treat space as punctuation
		punctSet += space;
	}
	size_t index;
	if (bFromWordEnd)
	{
		// We are getting punctuation which is word- or phrase-final
		wxString strReversed = MakeReverse(wordOrPhrase);
		size_t len = strReversed.Length();
		for (index = 0; index < len; index++)
		{
			// Get next character
			wxChar ch = strReversed.GetChar(index);
			// Is it in the punctuation set?
			offset = punctSet.Find(ch);
			// Break out if we've come to a character not in the punctuation set
			if (offset == wxNOT_FOUND)
			{
				break;
			}
			// We found a punct char, so accumulate it (we are in reverse order still)
			puncts += ch;
		}
		// Restore natural order, if the string is 2 or more chars long
		if (puncts.Length() > 1)
		{
			puncts = MakeReverse(puncts);
		}
	}
	else
	{
		// We are getting punctuation which is word- or phrase-initial
		size_t len = wordOrPhrase.Length();
		for (index = 0; index < len; index++)
		{
			// Get next character
			wxChar ch = wordOrPhrase.GetChar(index);
			// Is it in the punctuation set?
			offset = punctSet.Find(ch);
			// Break out if we've come to a character not in the punctuation set
			if (offset == wxNOT_FOUND)
			{
				break;
			}
			// We found a punct char, so accumulate it
			puncts += ch;
		}
	}
	return puncts;
}


wxString MakeSpacelessPunctsString(CAdapt_ItApp* pApp, enum WhichLang whichLang)
{
	wxString spacelessPuncts;
	// BEW 7Nov16 make sure the opening and closing double-chevrons are also in
	// the punctuation set
	wxString ch = _T(' ');	// initialize to a space, so that SetChar() will
							// not assert with a bounds error below
	int offset = wxNOT_FOUND;
	// use syntax  ch.SetChar(0, L'\x201C');
	ch.SetChar(0, L'\x00AB'); // hex for left-pointing double angle quotation mark
	offset = (pApp->m_punctuation[0]).Find(ch);
	if (offset == wxNOT_FOUND)
	{
		pApp->m_punctuation[0] += ch;
	}
	ch.SetChar(0, L'\x00BB'); // hex for right-pointing double angle quotation mark
	offset = (pApp->m_punctuation[0]).Find(ch);
	if (offset == wxNOT_FOUND)
	{
		pApp->m_punctuation[0] += ch;
	}
	// Now do same for m_punctuation[1] the target puncts set
	ch.SetChar(0, L'\x00AB'); // hex for left-pointing double angle quotation mark
	offset = (pApp->m_punctuation[1]).Find(ch);
	if (offset == wxNOT_FOUND)
	{
		pApp->m_punctuation[1] += ch;
	}
	ch.SetChar(0, L'\x00BB'); // hex for right-pointing double angle quotation mark
	offset = (pApp->m_punctuation[1]).Find(ch);
	if (offset == wxNOT_FOUND)
	{
		pApp->m_punctuation[1] += ch;
	}
	// Since this function is called for each of srcLang and tgtLang after
	// Preferences... dialog has been dismissed, we allow for the user to
	// have removed ' as a punctuation character. If not there, and the flag
	// is still defaulted to TRUE, then insert the ' in src and tgt punct sets
	// The status of ' is checked at lines 697 to 715 of PunctCorrespPage.cpp
	// and the m_bSingleQuoteAsPunct flag value updated according to what is
	// found, TRUE if in the punct set, FALSE if not
	wxString str = _T("'");
	if (pApp->m_bSingleQuoteAsPunct == TRUE) // BEW 7Nov16 TRUE is the new default,
											 // but user can remove ' in Prefs to override
	// BEW 7Nov16, make sure ' and ~ are at the end of the src and tgt m_punctuation strings
	{
		offset = (pApp->m_punctuation[0]).Find(str);
		if (offset == wxNOT_FOUND)
		{
			pApp->m_punctuation[0] += str;
		}
		offset = (pApp->m_punctuation[1]).Find(str);
		if (offset == wxNOT_FOUND)
		{
			pApp->m_punctuation[1] += str;
		}
	}
	// Do the same for the USFM fixed space marker (~)
	str = _T("~");
	offset = (pApp->m_punctuation[0]).Find(str);
	if (offset == wxNOT_FOUND)
	{
		pApp->m_punctuation[0] += str;
	}
	offset = (pApp->m_punctuation[1]).Find(str);
	if (offset == wxNOT_FOUND)
	{
		pApp->m_punctuation[1] += str;
	}

	// BEW 11Jan11, added test here so that the function can be used on target text as
	// well as on source text
	if (whichLang == targetLang)
	{
		spacelessPuncts = pApp->m_punctuation[1];
	}
	else
	{
		spacelessPuncts = pApp->m_punctuation[0];
	}
	while (spacelessPuncts.Find(_T(' ')) != -1)
	{
		// remove all spaces, leaving only the list of punctation characters
		spacelessPuncts.Remove(spacelessPuncts.Find(_T(' ')),1);
	}
	wxASSERT(!spacelessPuncts.IsEmpty());
	return spacelessPuncts;
}

// used in IsInWordProper() of doc class; return TRUE if *ptr is within str, else FALSE
bool IsOneOf(wxChar* ptr, wxString& str)
{
	int offset = str.Find(*ptr);
	return offset != wxNOT_FOUND;
}

// Return FALSE if no character within charSet (these should be a string of characters,
// typically punctuation ones by design, and no space character in the inventory) is a
// match for the wxChar pointed at by pStart. Return TRUE if it is a match, and in that
// case only, scan succeeding characters following pStart in the caller's buffer
// (typically a wxString's buffer) until either a character not in charSet is encounted -
// count the matched characters, return the count in the span param.
// This function is a utility one used for constructing bleached strings of spaces, #
// (representing a word) and punctuation characters from a string of text, to determine if
// verse text in Adapt It has undergone a change of punctuation from what is in the same
// verse's text within Paratext or Bibledit; during collaboration. pStart points to a
// location within a string at which the next character is to be tested. pEnd points past
// the last character of the string being tested. The span param is used by the caller to
// extract the spanned substring (when TRUE is returned)
// BEW created 22May14
bool IsOneOfAndIfSoGetSpan(wxChar* pStart, wxChar* pEnd, wxString& charSet, int& span)
{
	wxChar* ptr = pStart;
	span = 0;
	int offset = charSet.Find(*ptr);
	if (offset == wxNOT_FOUND)
	{
		return FALSE;
	}
	// There is at least one character from charSet in the substring starting at pStart
	ptr++;
	span++;
	// Find any others, & count them
	do {
		offset = charSet.Find(*ptr);
		if (offset == wxNOT_FOUND)
			break;
		// Advance ptr and count the character
		ptr++;
		span++;
	} while (ptr < pEnd);
	return TRUE;
}

// See the IsOneOfAndIfSoGetSpan() function above. IsNotOneOfNorSpaceAndIfSoGetSpan() is
// used for detecting a non-space character, and then scanning across all such until a
// space or a punctuation character (or buffer end) halts the scan. The scanned over
// characters are counted and the count is returned in span. Return TRUE if at least one
// is scanned over, FALSE if a space or punctuation character, or buffer end, is pointed
// at by pStart on entry
bool IsNotOneOfNorSpaceAndIfSoGetSpan(wxChar* pStart, wxChar* pEnd, wxString& charSet, int& span)
{
	wxChar* ptr = pStart;
	span = 0;
	if (*ptr == _T(' '))
		return FALSE;
	int offset = charSet.Find(*ptr);
	if (offset != wxNOT_FOUND)
		return FALSE;
	// There is at least one character not from charSet, and not a space, in the substring
	// starting at pStart
	ptr++;
	span++;
	// Find any others, & count them
	do {
		if (*ptr == _T(' '))
			break;
		offset = charSet.Find(*ptr);
		if (offset != wxNOT_FOUND)
			break;
		// Advance ptr and count the character
		ptr++;
		span++;
	} while (ptr < pEnd);
	return TRUE;
}

/* deprecated 10Jul15
// Generates and returns a string something like this: # #. # # #, "# #?
// from an inputStr like this: Jesus wept. The disciples said, "How come?
// The # character is used to represent anything which is not punctuation and not white
// space. All consecutive white spaces are reduced to one. Punctuation is retained where
// it is found in relation to the bleached word and delimiting space. The intent is to
// generate, in collaboration, a string like this from the AI verse, and a similar or
// identical one from the matched Paratext verse. If the strings are then identical, we
// know that no punctuation has been changed. If punctuation is changed or moved, the
// strings will not be exactly the same - and in that case our collaboration File > Save
// will know the from-AI verse data must be sent to PT adaptation project overwrite
// whatever is the current in_PT matching verse contents.
// If the user only ever changes punctuation from within AI, this function would not be
// needed because AI's md5sum arrays will detect that kind of punctuation change. What's
// not detectable that way is when the user changes punctuation settings for a verse
// within the Paratext (or Bibledit) source text project's verse - Adapt It receives those
// changes as fait accomplis, and without this present function those punctuation changes
// would be undetected, and result in the PT or BE verse being retained unchanged
// (provided no text or marker changes were made of course). The collaboration function,
// HasInfoChanged() calls this function.
wxString ReduceStringToStructuredPuncts(wxString& inputStr)
{
	wxString bleached = _T("");
	wxString hash = _T("#");
	wxString space = _T(" ");
	// This function is used only with adaptation text and/or free translation text, so we
	// can assume target text punctuation characters
	wxString charSet = MakeSpacelessPunctsString(gpApp, targetLang);
	int spanLen = 0; // initialize
	size_t inputStrLen = inputStr.size();
	{	// scoped block for wxStringBuffer
		wxStringBuffer pBuff(inputStr, inputStrLen); // we only read it
		wxChar* pBuffStart = pBuff;
//	wxChar* pBuffStart = inputStr.GetWriteBuf(inputStrLen); // not needed with wxStringBuffer
		wxChar* pEnd = pBuffStart + inputStrLen;
		wxChar* ptr = pBuffStart; // ptr is our scanning pointer

		while (ptr < pEnd)
		{
			if (IsWhiteSpace(ptr))
			{
				spanLen = ParseWhiteSpace(ptr);
				bleached += space;
				ptr = ptr + spanLen;
			}
			else if (IsNotOneOfNorSpaceAndIfSoGetSpan(ptr, pEnd, charSet, spanLen))
			{
				wxString out = wxString(ptr,(size_t)spanLen);
				bleached += hash;
				ptr = ptr + spanLen;
			}
			else if (IsOneOfAndIfSoGetSpan(ptr, pEnd, charSet, spanLen))
			{
				wxString out = wxString(ptr,(size_t)spanLen); // get the punctuation substring
				bleached += out;
				ptr = ptr + spanLen;
			}
		}
	}	// end of scoped block for wxStringBuffer
//	inputStr.UngetWriteBuf(inputStrLen); // not needed with wxStringBuffer
	return bleached;
}
*/

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
// The overloaded SpanExcluding(wxChar* ptr, ....) function is dangerous, as I only used it
// to parse over a word as far as following punctuation - and that goes belly up if there
// is an embedded punctuation character (which can happen if user changes punct settings)
// so I've deprecated it; and the following ParseWordInwardsFromEitherEnd() replaces it.
// This new function should include a space in the punctuation string because parsing
// inwards we could be parsing over detatched quotes, and some languages require
// punctuation to be offset from the word proper by a space. The function it calls will
// automatically add a space to charSet, so it is safe to pass in spacelessPuncts string.
// BEW created 28Jan11, the code is adapted from the second half of doc class's
// FinishOffConjoinedWordsParse() function
wxString ParseWordInwardsFromEnd(wxChar* ptr, wxChar* pEnd,	wxString& wordBuildingForPostWordLoc,
								 wxString charSet, bool bTokenizingTargetText)
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
					&bFoundHaltingWhitespace, nFixedSpaceOffset, nEndMarkerCount,
					bTokenizingTargetText);
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

// BEW 24Oct14, a helper for testing a marker, or its tag, for presence of the +
// character following the backslash, and returning the base marker, or base tag,
// respectively if so. Pass in \tag, tag, \+tag, or +tag (tag may include a final *)
// Returns: TRUE if +tag or \+tag was passed in, FALSE if tag or \tag was passed in.
// and the signature will return the tag in baseMkrOrTag and TRUE in bWholeMkrPassedIn if +
// Params:
// pMkrOrTag       ->  The input marker, or baseMkr passed in
// tagOnly         <-  The marker tag, which will include a final * if it is an endmarker tag
// baseOfEndMkr    <-  Empty string if baseMkrOrTag did not have a final *, if it does, then
//                     return the tag with the * removed
// bWholeMkrOrTag  <-  TRUE if the pMkrOrTag string passed in starts with a backslash,
//                     FALSE if not
bool IsNestedMarkerOrMarkerTag(wxString* pMkrOrTag, wxString& tagOnly, wxString& baseOfEndMkr, bool& bWholeMkrPassedIn)
{
	bool bNested = FALSE;
	tagOnly.Empty(); bWholeMkrPassedIn = FALSE;
	wxString tag = *pMkrOrTag;
	// BEW29Oct22 protect Get Char(0)
	int tagLen = tag.Len();
	if (tagLen > 0)
	{
		if (pMkrOrTag->GetChar(0) == gSFescapechar)
		{
			bWholeMkrPassedIn = TRUE;
			tag = tag.Mid(1); // strip off the initial backslash
		}
	}
	else
	{
		// empty tag, return FALSE
		return FALSE;
	}
	if (gpApp->gCurrentSfmSet == PngOnly)
	{
		// 1998 SFM marker set does not support nested markers, so set
		// params intelligently and return
		baseOfEndMkr.Empty();
		tagOnly = tag;
		return FALSE;
	}
	// Continue, it must be UsfmOnly or UsfmAndPng (and the latter we treat as the former)
	// BEW 29Oct22 protect Get Char(0) do an assert here, as tag is not empty as determined above
	wxASSERT(!tag.IsEmpty());
	if (tag.GetChar(0) == _T('+'))
	{
		// It's a nested marker or tag for such, so remove the initial +
		bNested = TRUE;
		tagOnly = tag.Mid(1); // beware, this could have * of endmarker on it
	}
	else
	{
		// It's not a nested marker
		tagOnly = tag;
	}
	// For lookups of the USFM struct, an endmarker needs to be stripped of the
	// final *, and returned in baseOfEndMkr; if it's not an endmarker, return
	// an empty string in it
	int length = tagOnly.Len();
	if (length > 0) // BEW 29Oct22 protect the Get Char() call
	{
		if (tagOnly.GetChar(length - 1) == _T('*'))
		{
			baseOfEndMkr = tagOnly.Truncate(length - 1);
		}
		else
		{
			baseOfEndMkr.Empty();
		}
	}
	return bNested;
}
// Overrided version of above
bool IsNestedMarkerOrMarkerTag(wxChar* ptrToMkr, wxString& tagOnly,
							   wxString& baseOfEndMkr, bool& bWholeMkrPassedIn)
{
	wxString wholeMkr;
	wholeMkr = gpApp->GetDocument()->GetWholeMarker(ptrToMkr);
	return IsNestedMarkerOrMarkerTag(&wholeMkr, tagOnly, baseOfEndMkr, bWholeMkrPassedIn);
}


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
// BEW 24Oct14, no changes needed for support of USFM nested markers
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
	// whm 11Jun12 added.
	wxCHECK_MSG(!subStr.IsEmpty(),-1, _T("Programming error. FindFromPos() function's incoming subStr is empty!"));
	// returns the zero based index position of the subStr if it exists in inputStr
	// with a search starting from startAtPos; or -1 if the subStr is not found
	// in inputStr from startAtPos to the end of the string
	int len = inputStr.Length();
	if (len == 0 || (int)subStr.Len() > len || startAtPos >= len)
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
		if (!subStr.IsEmpty() && *pStartChar == subStr.GetChar(0)) // BEW 29Oct22 added protection for Get Char(0)
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
	// BEW added 24May13, folderPathOnly may be empty, in which case the PathSeparator
	// ends up first in the string. Windows will accept this in a wxFileDialog, but Linux
	// won't, and it crashes the app (an assert from deep in wxWidgets, & error message
	// "...the filename shouldn't contain the path"). So test for initial PathSeparator and
	// remove it if there and then return the result
	int offset = uniqueName.Find(PathSeparator);
	if (folderPathOnly.IsEmpty() && offset == 0)
	{
		// remove it
		uniqueName = uniqueName.Mid(1);
	}
	return uniqueName;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString with consecutive whitespace characters reduced to a
///             single space wherever the whitespace occurs
/// \param		rString		-> the string we are reading characters from
/// \remarks
/// Called from: used in BuildFootnoteOrEndnoteParts() in Xhtml.cpp
/// Searches for whitespace, converts each such to a single space, and then calls
/// RemoveMultipleSpaces() to reduce multiple spaces to a single space
///////////////////////////////////////////////////////////////////////////////
wxString ChangeWhitespaceToSingleSpace(wxString& rString)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	wxString destStr = rString;
	wxChar aSpace = _T(' ');
	if (!rString.IsEmpty())
	{
		size_t length = destStr.Len();
		size_t i;
		wxChar aChar;
		for (i = 0; i < length; i++)
		{
			aChar = destStr[i];
			if (pDoc->IsWhiteSpace(&aChar))
			{
				destStr.SetChar(i, aSpace);
			}
		}
		destStr = RemoveMultipleSpaces(destStr);
	}
	return destStr;
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
	// whm 8Jun12 revised to use wxStringBuffer instead of calling GetWriteBuf() and UngetWriteBuf() on the wxString
	// Our copy-to buffer must be writeable.
	// Create the wxStringBuffer in a specially scoped block. This is crucial here
	// in this function since the wxString textStr2 is accessed directly within
	// this function in the return destString; statement at the end of the function.
	{ // begin special scoped block
		wxStringBuffer pDestBuff(destString,nLen + 1);
		wxChar* pDestBuffChar = pDestBuff; // wxChar* pDestBuffChar = destString.GetWriteBuf(nLen + 1); // pDestBuffChar is the pointer to the write buffer
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
	} // end of special scoping block
	//destString.UngetWriteBuf(); // whm 8Jun12 removed - not needed with wxStringBuffer above

	return destString;
}

void RemoveFilterMarkerFromString(wxString& filterMkrStr, wxString wholeMarker)
{
	// if the wholeMarker already exists in filterMkrStr, remove it.
	// Assumes wholeMarker begins with backslash, and ensures it ends with a delimiting space.
	wholeMarker.Trim(TRUE); // trim right end
	wholeMarker.Trim(FALSE); // trim left end
	wxASSERT(!wholeMarker.IsEmpty());
	// then add the necessary final space
	wholeMarker += _T(' ');
	// whm modified 8Jul12. If wholeMarker is \x , \f , or \fe remove it and any associated
	// content markers.
	if (wholeMarker == _T("\\x "))
	{
		// Remove the \x marker as well as its associated content markers: \xo \xk \xq \xt \xot \xnt \xdc
		// Use the wxArrayString m_crossRefMarkerSet which contains the cross reference marker
		// plus all of the associated content markers; each includes the initial backslash and following
		// space.
		int nTot = (int)gpApp->m_crossRefMarkerSet.GetCount();
		int posn;
		int i;
		wxString marker;
		for (i = 0; i < nTot; i++)
		{
			marker = gpApp->m_crossRefMarkerSet.Item(i);
			posn = filterMkrStr.Find(marker);
			wxASSERT(posn != wxNOT_FOUND);
			if (posn != wxNOT_FOUND)
				filterMkrStr.Remove(posn, marker.Length());
		}
	}
	else if (wholeMarker == _T("\\f ") || wholeMarker == _T("\\fe "))
	{
		// Remove the \f and \fe markers as well as their associated content markers: \fr \fk \fq \fqa \fl
		// \fp \ft \fdc \fv \fm
		// Use the wxArrayString m_footnoteMarkerSet which contains the footnote and endnote markers
		// plus all of the associated content markers; each includes the initial backslash and following
		// space.
		int nTot = (int)gpApp->m_footnoteMarkerSet.GetCount();
		int posn;
		int i;
		wxString marker;
		for (i = 0; i < nTot; i++)
		{
			marker = gpApp->m_footnoteMarkerSet.Item(i);
			posn = filterMkrStr.Find(marker);
			wxASSERT(posn != wxNOT_FOUND);
			if (posn != wxNOT_FOUND)
				filterMkrStr.Remove(posn, marker.Length());
		}
	}
	else
	{
		int posn = filterMkrStr.Find(wholeMarker);
		if (posn != -1)
		{
			// The wholeMarker does exist in the string, so remove it.
			// NOTE: By removing a marker from the filter marker string we are creating a
			// string that can no longer be compared with other filter marker strings by
			// means of the == or != operators. Comparison of such filter marker strings will now
			// necessarily require a special function StringsContainSameMarkers() be used
			// in every place where marker strings are compared.
			filterMkrStr.Remove(posn, wholeMarker.Length());
		}
	}
}

void AddFilterMarkerToString(wxString& filterMkrStr, wxString wholeMarker)
{
	// if the wholeMarker does not already exist in filterMkrStr, append it.
	// Assumes wholeMarker begins with backslash, and insures it ends with a delimiting space.
	wholeMarker.Trim(TRUE); // trim right end
	wholeMarker.Trim(FALSE); // trim left end
	wxASSERT(!wholeMarker.IsEmpty());
	// then add the necessary final space
	wholeMarker += _T(' ');

	if (wholeMarker == _T("\\x "))
	{
		// Add the \x marker as well as its associated content markers: \xo \xk \xq \xt \xot \xnt \xdc
		// to the filterMkrStr.
		// Use the wxArrayString m_crossRefMarkerSet which contains the cross reference marker
		// plus all of the associated content markers; each includes the initial backslash and following
		// space.
		int nTot = (int)gpApp->m_crossRefMarkerSet.GetCount();
		int i;
		wxString marker;
		for (i = 0; i < nTot; i++)
		{
			marker = gpApp->m_footnoteMarkerSet.Item(i);
			if (filterMkrStr.Find(marker) == wxNOT_FOUND)
			{
				// The wholeMarker doesn't already exist in the string, so append it.
				// NOTE: By appending a marker to the filter marker string we are creating a
				// string that can no longer be compared with other filter marker strings by
				// means of the == or != operators. Comparison of such filter marker strings will now
				// necessarily require a special function StringsContainSameMarkers() be used
				// in every place where marker strings are compared.
				filterMkrStr += marker;
			}
		}

	}
	else if (wholeMarker == _T("\\f ") || wholeMarker == _T("\\fe "))
	{
		// Add the \f and \fe markers as well as their associated content markers: \fr \fk \fq \fqa \fl
		// \fp \ft \fdc \fv \fm to the filterMkrStr.
		// Use the wxArrayString m_footnoteMarkerSet which contains the footnote and endnote markers
		// plus all of the associated content markers; each includes the initial backslash and following
		// space.
		int nTot = (int)gpApp->m_footnoteMarkerSet.GetCount();
		int i;
		wxString marker;
		for (i = 0; i < nTot; i++)
		{
			marker = gpApp->m_footnoteMarkerSet.Item(i);
			if (filterMkrStr.Find(marker) == wxNOT_FOUND)
			{
				// The wholeMarker doesn't already exist in the string, so append it.
				// NOTE: By appending a marker to the filter marker string we are creating a
				// string that can no longer be compared with other filter marker strings by
				// means of the == or != operators. Comparison of such filter marker strings will now
				// necessarily require a special function StringsContainSameMarkers() be used
				// in every place where marker strings are compared.
				filterMkrStr += marker;
			}
		}

	}
	else
	{
		if (filterMkrStr.Find(wholeMarker) == wxNOT_FOUND)
		{
			// The wholeMarker doesn't already exist in the string, so append it.
			// NOTE: By appending a marker to the filter marker string we are creating a
			// string that can no longer be compared with other filter marker strings by
			// means of the == or != operators. Comparison of such filter marker strings will now
			// necessarily require a special function StringsContainSameMarkers() be used
			// in every place where marker strings are compared.
			filterMkrStr += wholeMarker;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString representing the absolute path of appName if found
///             by searching the system's PATH environment variable; otherwise
///             an empty string
/// \param		appName		-> the name of the program we are searching for
/// \remarks
/// Called from: the GetAdaptItInstallPrefixForLinux() function in helpers.
/// Searches for the program appName at locations specified in the system's
/// PATH environment variable. The first path on which appName is found is
/// returned. If appName is not found on any paths specified in the system's
/// PATH environment variable, an empty string is returned. This function is
/// used to find
///////////////////////////////////////////////////////////////////////////////
wxString GetProgramLocationFromSystemPATH(wxString appName)
{
	// whm added 6Dec11. Patterned after Julian Smart's code in wxWidgets
	// docs re "Writing installers for wxWidgets applications".
    // The passed in appName string might be for example, _T("bibledit-rdwrt"),
    // or _T("bibledit-gkt") or _T("adaptit") or _T("adaptit-bibledit-rdwrt"),
    // or any program executable that might be found by searching the system's
    // PATH environment variable.
    wxString str;
    wxPathList pathList;
    pathList.AddEnvList(wxT("PATH"));
    str = pathList.FindAbsoluteValidPath(appName);
	// The value for str here would normally be "/usr/local/bin" for non-packaged
	// builds or "/usr/bin" for packaged builds.
    if (!str.IsEmpty())
        return wxPathOnly(str);
    else
        return wxEmptyString;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString representing the absolute path of appName if found
///             by searching the system's PATH environment variable; otherwise
///             an empty string
/// \param		appName		-> the name of the program we are searching for
/// \remarks
/// Called from: the GetAdaptItInstallPrefixForLinux() function in helpers.
/// Searches for the program appName at locations specified in the system's
/// PATH environment variable. The first path on which appName is found is
/// returned. If appName is not found on any paths specified in the system's
/// PATH environment variable, an empty string is returned.
///////////////////////////////////////////////////////////////////////////////
wxString GetAdaptItInstallPrefixForLinux()
{
    // Note: wxWidgets defines argc and argv as uninitialized public variables in App.h
    //       The system's argv[0] parameter (to Main) is a string containing the runninig
    //       app's name.
	wxString prefix;
	prefix.Empty();
    wxString str;
    str = GetProgramLocationFromSystemPATH(gpApp->argv[0]);
	// If str is empty it means that adaptit has not been installed from a
	// package or by invoking 'sudo make install' from a local build. At least
	// it is not on the system PATH. In such cases it must have been invoked
	// after a local make process probably from some location like:
	// /home/wmartin/subversion/adaptit/bin/linux/UnicodeDebug
	// and no command was invoked to install the program after make was
	// called. When no install has been done, we just return an empty string
	// for the install prefix.
	//
    // When str is not empty the value of str will be the path to the installed
    // adaptit executable file, without a terminating path separator, normally
    // /usr/local/bin or /usr/bin.
	// Since /bin is the usual/standard directory regardless of what type
	// of install (package or local make install), the prefix is the first
	// part of the path preceding the /bin part, i.e., /usr/local or /usr.
	int offset;
	offset = str.Find(_T("/bin"));
	if (offset != wxNOT_FOUND)
	{
		prefix = str.Mid(0,offset);
	}

#ifdef __WXGTKzzz__
	wxStandardPaths stdPaths;
	wxString stdPathsPrefix;
	stdPathsPrefix.Empty();
	stdPathsPrefix = stdPaths.GetInstallPrefix();
	// When no install has been done GetInstallPrefix() seems to return
	// something like /home/wmartin/subversion/adaptit/bin/linux/UnicodeDebug
	// or /home/bruce/subversion/adaptit/bin/linux/bin/Debug (note the
	// two /bin/ folders in the path - as reported by Bruce)
	// which is the directory the executable is currently being run from.
	// If as part of the make process we were doing the equivalent of our
	// post-build that get called in Visual Studio, this value for a
	// prefix would be OK. But, for now we will just return set a prefix
	// string value when the app was found on the system PATH (above).
	// Uncomment the next line if/when we do such post-build copying of
	// adaptit's auxiliary files to the build folder.
	//prefix = stdPathsPrefix;
	wxLogDebug(_T("The stdPathsPrefix is: %s"),stdPathsPrefix.c_str());
#endif
	return prefix;
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
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            gpApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(_(
"No path to a folder is defined. Perhaps you cancelled the dialog for setting the destination folder."),
			_("Error, empty path specification"), wxICON_EXCLAMATION | wxOK);
		}
		return FALSE;
	}
	else
	{
		// wxDir must call .Open() before enumerating files
		// whm 8Apr2021 added wxLogNull block below
		{
			wxLogNull logNo;	// eliminates any spurious messages from the system if the wxSetWorkingDirectory() call returns FALSE
			bOK = (::wxSetWorkingDirectory(pathToFolder) && dir.Open(pathToFolder));
		} // end of wxLogNull scope
		if (!bOK)
		{
			// unlikely to fail, but just in case...
			if (!bSuppressMessage)
			{
				wxString msg;
				msg = msg.Format(
_("Failed to make the directory  %s  the current working directory prior to getting the directory's child directories; perhaps try again."),
				pathToFolder.c_str());
                // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                gpApp->m_bUserDlgOrMessageRequested = TRUE;
                wxMessageBox(msg, _("Error, no working directory"), wxICON_EXCLAMATION | wxOK);
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
					wxMessageBox(msg, _("Error, not a directory"), wxICON_ERROR | wxOK);
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

//#if defined (_KBSERVER) // -- BEW made it not be confined to kbserver support on 21Oct13
// a handy utility for counting how many space-delimited words occur in str
int CountSpaceDelimitedWords(wxString& str)
{
	if (str.IsEmpty())
		return 0;
	wxString delimiters = _T(' ');
	wxArrayString words;
	bool bStoreEmptyStringsToo = FALSE;
	int wordCount = (int)SmartTokenize(delimiters,str,words,bStoreEmptyStringsToo);
	return wordCount;
}
//#endif

 // BEW 16Sep22 using code from pSrcPhrase->GetTgtWordCount()
int GetWordCountIncludingZWSP(wxString& str)
{
	if (str.IsEmpty())
	{
		return 0;
	}
	wxChar zwsp = (wxChar)0x200B;
	wxString aToken = wxEmptyString;
	//wxString delims = _T(" \n\r\t~'/'"); 
	wxString delims = _T(" \n\r\t~"); // remove the '/' and retry
	delims += zwsp;
	delims += _T('/');
	wxStringTokenizer tkz(str, delims); //my choice of delimiters includes / and zero-width-space
		// and adding USFM's tilde (fixed space) so that ~ between words is tokenized as 2 words
	int numFields = tkz.CountTokens();
	return numFields;
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
// Note: it internally trims spaces from the end tokenized strings, and really I've made
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
// BEW 24Oct14, no changes needed for support of USFM nested markers
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
	bool bOK;
	// whm 8Apr2021 added wxLogNull block below
	{
		wxLogNull logNo;	// eliminates any spurious messages from the system if the wxSetWorkingDirectory() call returns FALSE
		bOK = (::wxSetWorkingDirectory(pathToFolder) && dir.Open(pathToFolder));
	} // end of wxLogNull scope
	if (!bOK)
	{
		// unlikely to fail, but just in case...
		if (!bSuppressMessage)
		{
			wxString msg;
			msg = msg.Format(
_("Failed to make the directory  %s  the current working directory prior to getting the folder's files; perhaps try again."),
			pathToFolder.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            gpApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg, _("Error, no working directory"), wxICON_EXCLAMATION | wxOK);
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
/// (in the case of glosses, the gloss would have been added after the merger since
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
/// BEW 21Jul14, refactored to support storage of wordbreaks, for later reconstituting in exports
/// BEW 22Jun15, refactored to no longer include filtered information in the target text export
/////////////////////////////////////////////////////////////////////////////////////////
wxString FromMergerMakeTstr(CSourcePhrase* pMergedSrcPhrase, wxString Tstr, bool bDoCount,
							bool bCountInTargetText)
{
	// BEW 22Jun15, refactored, so the following are unused. Prevent compiler warning
	wxUnusedVar(bCountInTargetText);
	wxUnusedVar(bDoCount);

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
	//SPList* pSrcPhrases = gpApp->m_pSourcePhrases;
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
	bool bIsAmbiguousForEndmarkerPlacement = FALSE; // BEW 20May23 added, so AutoPlace...() will compile & link ok
	//bool bNonFinalEndmarkers = FALSE;

	// Add the word delimiter
	//Tstr = PutSrcWordBreak(pMergedSrcPhrase) + Tstr;

	wxArrayString markersToPlaceArray;
	wxArrayString markersAtVeryEndArray; // for endmarkers on pMergedSrcPhrase

    // store here any string of filtered information stored on pMergedSrcPhrase (in any or
    // all of m_freeTrans, m_note, m_collectedBackTrans, m_filteredInfo) which is on the
    // first CSourcePhrase instance
	// BEW 22Jun15, now only use markersPrefix, if at all, for begin-markers; we no longer
	// restore any of the filtered data
	wxString markersPrefix; markersPrefix.Empty();
	wxString Sstr; // Sstr needed only if internally we must use the placement
				   // dialog; we don't need to return it to the caller

	// markers needed, since doc version 5 may store some filtered stuff without using them
	//wxString freeMkr(_T("\\free"));
	//wxString freeEndMkr = freeMkr + _T("*");
	//wxString noteMkr(_T("\\note"));
	//wxString noteEndMkr = noteMkr + _T("*");
	//wxString backTransMkr(_T("\\bt"));
	markersPrefix.Empty();
	Sstr.Empty();
	wxString finalSuffixStr; finalSuffixStr.Empty(); // put collected string-final
													 // m_endMarkers content here
	bool bFinalEndmarkers = FALSE; // set TRUE when finalSuffixStr has content
								   // to be added at loop end
	wxString aSpace = _T(" ");
	wxString markersStr;
	wxString endMarkersStr;
	// BEW 22Jun15 we retain freeTransStr, noteStr, collBackTransStr, filteredInfoStr
	// because these are parameter in lower down function calls; those function calls
	// are made in other types of export, so our approach here is to let these strings
	// be populated, but then clear them to empty before their contents can be used
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
	// BEW 22Jun15, likewise, we retain xrefStr and otherFiltered, but either don't
	// calculate them any more, or if in function calls, let them be calculated but
	// then empty them immediately the functions return
	wxString xrefStr; // for \x* .... \x* cross reference info (it is stored preceding
					  // m_markers content, but other filtered info goes before m_markers
	wxString otherFiltered; // leftovers after \x ...\x* is removed from filtered info

	// the first one, pMergedSrcPhrase as passed in, has to provide the stuff for the
	// markersPrefix wxString...
	GetMarkersAndFilteredStrings(pMergedSrcPhrase, markersStr, endMarkersStr,
					freeTransStr, noteStr, collBackTransStr, filteredInfoStr);
	{
		// BEW 22Jun15 refactoring, we just empty these of any content - no filtered
		// data should be in the export
		freeTransStr.Empty(); noteStr.Empty(); collBackTransStr.Empty(); filteredInfoStr.Empty();
	}
	// remove any filter bracketing markers if filteredInfoStr has content
	// BEW 22Jun15 this is no longer needed either
	/*
	if (!filteredInfoStr.IsEmpty())
	{
		filteredInfoStr = pDoc->RemoveAnyFilterBracketsFromString(filteredInfoStr);

		// separate out any crossReference info (plus marker & endmarker) if within this
		// filtered information
		SeparateOutCrossRefInfo(filteredInfoStr, xrefStr, otherFiltered);
	}
	*/
	// BEW 22Jun15 the legacy comment following no longer applies, we remove code that computes
	// filtered information. prefix string will, at most, only have unfiltered markers
    // Legacy comment: for the first CSourcePhrase, we store any filtered info within the prefix
    // string, and any content in m_markers, if present, must be put at the start
    // of Tstr and Sstr; remove LHS whitespace when done
    // BEW 8Sep10, changed the order to be: 1. filtered info, 2. collected bt 3.
	// note 4. free trans
	bool bAttachFilteredInfo = TRUE; // BEW 22Jun15, need to pass this as TRUE to calls below
	bool bAttach_m_markers = TRUE;
	// next call ignores m_markers, and the otherFiltered string input has no
	// crossReference info left in it - these info types are handled by a separate call,
	// the one below to GetUnfilteredCrossRefsAndMMarkers()
	// BEW 22Jun15, this call no longer needs to be done
	//markersPrefix = GetUnfilteredInfoMinusMMarkersAndCrossRefs(pMergedSrcPhrase,
	//						pSrcPhrases, otherFiltered, collBackTransStr,
	//						freeTransStr, noteStr, bDoCount, bCountInTargetText); // m_markers
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
	// BEW 22Jun15, the following function was refactored so that filtered info is not returned
	// It is used in From MergerMakeTstr(), FromSingleMakeTstr() and FromSingleMakeSstr() and
	// in each case, it must not return filtered info, so the internal tweaks apply in all calls
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
	wxString lastWordBreak; // needed, because pNextSrcPhrase will be NULL when we are looking
							// for an appropriate wordbreak to insert before the last word
	while (pos != NULL)
	{
		if (pos == posLast)
		{
			bLast = TRUE;
		}
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		lastWordBreak = pSrcPhrase->GetSrcWordBreak();

		// BEW 21Jul14, we add the word delimiter from the 'next' CSourcePhrase,
		// but we'll have to use the penultimate delimiter for the last word - use lastWordBreak
		CSourcePhrase* pNextSrcPhrase = NULL;
		if (pos != NULL)
		{
			pNextSrcPhrase = (CSourcePhrase*)pos->GetData();
		}

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
			{
				// BEW 22Jun15 refactoring, we just empty these of any content - no filtered
				// data should be in the export
				freeTransStr.Empty(); noteStr.Empty(); collBackTransStr.Empty(); filteredInfoStr.Empty();
			}
			// for the non-first pSrcPhrase instances, we'll use markersStr and
			// endMarkersStr in the code below the next block

			// BEW 21Jul14 add the word delimiter from the 'next' CSourcePhrase
			if (pNextSrcPhrase != NULL)
			{
				Sstr += PutSrcWordBreak(pNextSrcPhrase);
			}
			else
			{
				// For the last one, we will have to re-use the delimiter for the penultimate
				// word instead, since pNextSrcPhrase is NULL (BEW fixed, 11Sep14)
				Sstr += lastWordBreak;
			}
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
			// we can insert any preceding inline non-binding beginmarker and any
			// preceding m_markers content, the latter comes first; Tstr is already handled
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
				// input already in the passed in Tstr, so we don't need to do anything
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
			//Sstr << aSpace; <<-- as of 21Jul14, we add the word delimiter before the material
			// which is to follow after it -- see above near the loop for Sstr building's start

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

			//Sstr << aSpace; <<-- as of 21Jul14, we add the word delimiter before the material
			// which is to follow after it -- see above near the loop for Sstr building's start

		} // end of TRUE block for test: else if (!bFirst && !bLast)

	} // end of while loop

	// Finally, add any final endmarkers from pMergedSrcPhrase held over till now; for
	// Tstr, they are in the second array, markersAtVeryEndArray, - so copy the items
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
	if (bFinalEndmarkers) // this block is for building the end of Sstr
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
		if (!pMergedSrcPhrase->GetEndMarkers().IsEmpty())
		{
			Sstr += pMergedSrcPhrase->GetEndMarkers();
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
		// There is ambiguity, so do the placement using the dialog -- BEW 22Feb12 added a
		// check for m_tgtMkrPattern having content; if it does, use that for the Tstr
		// value (before markersPrefix's contents get added), and so refrain from showing
		// the placement dialog; but if the string is empty, then show the dialog, if we
		// can't auto-place what's in the array of markers for placement
		if (pMergedSrcPhrase->m_tgtMkrPattern.IsEmpty())
		{
			// BEW added 11Sep14, If there is just a single marker to be placed, try do it
			// automatically. If the markersToPlaceArray is returned empty, then we won't
			// need to show the placement dialog
			Tstr = AutoPlaceSomeMarkers(Tstr, Sstr, pMergedSrcPhrase, &markersToPlaceArray, bIsAmbiguousForEndmarkerPlacement); // BEW added bool 20May23

			if (!markersToPlaceArray.IsEmpty())
			{
				// There's something the user needs to deal with manually

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
				// BEW 24Aug11 don't show the dialog if there is nothing in the array, Tstr would
				// be already correct
				if (!markersToPlaceArray.IsEmpty())
				{
					dlg.SetPlaceableDataStrings(&markersToPlaceArray);

					// show the dialog
					dlg.ShowModal();

					// get the post-placement resulting string
					Tstr = dlg.GetPostPlacementString();
#if defined (_DEBUG)
					wxLogDebug(_T("FromMergerMakeTstr() in helpers.cpp line %d, sequNum = %d, GetPostPlacementString returned [%s]"),
						__LINE__, Tstr.c_str());
#endif
				}
			}
			// remove initial and final whitespace
			Tstr.Trim(FALSE);
			Tstr.Trim();

			// as of version  6.2.0, we store the result whenever produced, so that
			// the placement dialog isn't opened again (unless the user puts phrase
			// box at this CSourcePhrase's location and edits either puncts or word(s)
			// to be placed differently - that causes the m_tgtMkrPattern string to be
			// cleared, and then this and other placement dialogs would show again, if
			// relevant -- that is, if there is a placement ambiguity requiring that
			// they show)
			pMergedSrcPhrase->m_tgtMkrPattern = Tstr;

			// make sure the doc is dirty, so the user will be prompted to save it -
			// we don't want this setting to get lost unnecessarily
			pDoc->Modify(TRUE);
		}
		else
		{
			// it's non-empty, so use it as Tstr's value
			wxString s;
			s = pMergedSrcPhrase->m_tgtMkrPattern;
			// remove any initial or final whitespace, just in case there was some
			s = s.Trim(FALSE);
			s = s.Trim();
			Tstr = s;
		}
	}

	// now add the prefix string material not shown in the Place... dialog,
	// if it is not empty
	if (!markersPrefix.IsEmpty())
	{
		markersPrefix.Trim();
		if (!markersPrefix.IsEmpty())
		{
			markersPrefix += aSpace; // ensure a final space after markers
			Tstr = markersPrefix + Tstr;
		}
	}
	Tstr.Trim();

	// BEW 21Jul14, don't add space after, use PutSrcWOrdBreak() before, at top
	//Tstr << aSpace; // have a final space
	// BEW 19May23 new doc boolean, so that FromSingleMakeTstr() will know where to store its result
	if (!pMergedSrcPhrase->m_tgtMkrPattern.IsEmpty())
	{
		pDoc->m_bTstrFromMergerCalled = TRUE;
	}
	else
	{
		pDoc->m_bTstrFromMergerCalled = FALSE; // BEW added 30Aug23, gotta initialize if m_tgtMkrPattern is empty
	}
	markersToPlaceArray.Clear();
	return Tstr;
}

// BEW created 11Sep14 to try do as many auto-placements of markers when ambiguity for
// placement is present. Currently only a single endmarker is done, but later more code
// could be added to try handle more. However, ambiguities usually are at the very end
// of something, when punctuation, and endmarkers, can be mixed in together; when not
// at the end of something, inline binding and other markers usually can be placed
// without ambiguity. So we try to handle the few that are easy to place because
// they occur at the end of everything, except when there is outer punctuation. Any we
// auto-place, we remove from the string array - and if that empties the array, then
// the caller will not put up the Placement dialog. But if one or more markers remain
// in the array for placement, those which remain will require the Placement dialog
// to be shown for the user to place those manually. It should be possible to auto-place
// at lots of the places in the document which otherwise would ask for manual placement
// The function supports the UsfmOnly versus PngOnly difference in marker sets.
wxString AutoPlaceSomeMarkers(wxString TheStr, wxString Sstr, CSourcePhrase* pSingleSrcPhrase,
	wxArrayString* arrMkrsPtr, bool& bIsAmbiguousForEndmarkerPlacement)
{
	wxString aSpace = _T(" ");
#if defined (_DEBUG)
	{
		wxLogDebug(_T("helpers.cpp AutoPlaceSomeMarkers() START line %d, TheStr= [%s] , Sstr= [%s] , sequNum = %d, arrMkrsPtr count = %d"),   
			__LINE__, TheStr.c_str(), Sstr.c_str(), pSingleSrcPhrase->m_nSequNumber, arrMkrsPtr->GetCount());
		if (pSingleSrcPhrase->m_nSequNumber >= 11)
		{
			int halt_here = 1;
		}
	}
#endif
	if (arrMkrsPtr->IsEmpty())
	{
		wxLogDebug(_T("helpers.cpp AutoPlaceSomeMarkers() line %d, NO MARKERS TO BE PLACED: returning TheStr= [%s] , Sstr= [%s] , sequNum = %d"),
			__LINE__, TheStr.c_str(), Sstr.c_str(), pSingleSrcPhrase->m_nSequNumber);
		return TheStr; // there are no markers to place, so TheStr is correct "as is"
	}
	else
	{
		bool bIsAmbiguous = bIsAmbiguousForEndmarkerPlacement; // reset to FALSE, if we can avoid need of Placement dlg
		CAdapt_ItDoc* pDoc = gpApp->GetDocument();
		wxString spacelessPunctsForTgt = MakeSpacelessPunctsString(gpApp, targetLang);
		wxString spacelessPunctsForSrc = MakeSpacelessPunctsString(gpApp, sourceLang); // needed for creating the 'old ones' array
		wxArrayString oldSrcBitsArr;

		bool bCreatedArrayOK = pDoc->CreateOldSrcBitsArr(pSingleSrcPhrase, oldSrcBitsArr, spacelessPunctsForSrc);
		if (bCreatedArrayOK)
		{
			// The 'old bits' array got created okay, these have the substring with src puncts, from what's
			// in pSrcPhrase->m_srcSinglePattern ( new CSourcePhase member string, created in docVersion 10)



			// TODO from pSPhr->m_targetStr, remove everything except the following (tgt text) puncts. Then try matching
			// and transferring values. There may be white spaces stored within too. Check for same number of puncts.
			// If puncts inventory in pSPhr->m_targetStr is fewer or more, must use Placement dlg to get it right. If the
			// inventories are the same, we can do what's needed here, and set bIsAmbiguousForEndmarkerPlacement back to
			// FALSE, and return from here. Then Placement dlg will be skipped.












		} // end of TRUE block for test: if (bCreatedArrayOK)
	}

	// BEW 20/May23, legacy (docVersion 6 & later) follows - will show placement dlg if 
	// we cannot auto-fix above (fewer or more puncts, must cause this block to be entered)
	wxASSERT(bIsAmbiguousForEndmarkerPlacement == TRUE);
	size_t count = Sstr.Len(); 
	if (gpApp->gCurrentSfmSet == PngOnly)
	{
		// Png sfm set has only one endmarker - either \fe or \F, either means 'end footnote'
		count = arrMkrsPtr->GetCount();
		if (count == 1)
		{
			wxString aMarker = arrMkrsPtr->Item(0);
			if (aMarker == _T("\\fe") || aMarker == _T("\\F") ||
				aMarker == _T("\\fe ") || aMarker == _T("\\F "))
			{
				size_t length = aMarker.Len();
				if (aMarker.GetChar(1) == _T('f'))
				{
					if (length == 3)
					{
						// Need to add a following space
						TheStr += aMarker;
						TheStr += aSpace;
						arrMkrsPtr->Clear();
						return TheStr;
					}
					else
					{
						// Terminates with a space already
						TheStr += aMarker;
						arrMkrsPtr->Clear();
						return TheStr;
					}
				}
				else
				{
					// must be either \F or \F followed by a space
					if (length == 2)
					{
						// Need to add a following space
						TheStr += aMarker;
						TheStr += aSpace;
						arrMkrsPtr->Clear();
						return TheStr;
					}
					else
					{
						// Terminates with a space already
						TheStr += aMarker;
						arrMkrsPtr->Clear();
						return TheStr;
					}
				}
			}
			else
			{
				// Not so, therefore let user deal with it manually
				return TheStr;
			}
		}
		else
		{
			// More than one marker - that's unexpected, so let these placements be done manually
			return TheStr;
		}
	}
	else
	{
		// Probably is UsfmOnly set, but could be UsfmAndPng - we'll ignore the latter
		// and assume the former
		// Handle the most common and easiest first - when \f* or \x* or \fe* occur at the end
		CAdapt_ItDoc* pDoc = gpApp->GetDocument();
		size_t count = arrMkrsPtr->GetCount();
		wxString fEndMkr = _T("\\f*");
		wxString xEndMkr = _T("\\x*");
		wxString feEndMkr = _T("\\fe*");
		//if (count == 1 && pMergedSrcPhrase->GetFollowingOuterPunct().IsEmpty())
		if (count >= 1 && pSingleSrcPhrase->GetFollowingOuterPunct().IsEmpty()) // BEW 18May23 allow entry here if 1 or more to place
		{
			// We've new got one or more endMkrs to deal with, here is where to refactor for
			// getting the placement dialog to show only when new auto-fix code has to handle
			// fewer or more ending puncts than are present in the Sstr passed in







			// TODO
			wxString aMarker = arrMkrsPtr->Item(0);
			// Trim any whitespace off its end (a beginmarker can be expected to have a terminating
			// space stored with it, an endmarker should be stored without a final space, but we'll
			// place safe just in case that's not so
			aMarker.Trim();
			if (count == 1 && (aMarker == fEndMkr || aMarker == xEndMkr || aMarker == feEndMkr) )
			{
				// Just append to whatever TheStr has at its end
				TheStr += aMarker;
				arrMkrsPtr->Clear();
				return TheStr;
			}
			else
			{
				// It's not one of those endmarkers, but (1) is an endmarker, and (2) is one
				// of the inline non-binding endmarkers, then we'll append it like in the
				// block above. Such a one would be expected to also follow any outer
				// punctuation - so we don't need to test for that. Any other options,
				// we'll handle manually
				const wxChar* pBuff = aMarker.GetData();
				wxChar* pEnd = (wxChar*)pBuff + (size_t)aMarker.Len();
				wxASSERT(*pEnd == _T('\0')); // ensure there is a null there;
				wxChar* ptr = (wxChar*)pBuff;
				bool bIsEndmarker = pDoc->IsEndMarker(ptr,pEnd);
				if (bIsEndmarker)
				{
					int offset = wxNOT_FOUND;
					wxString addSpaceToEndMkr = aMarker + aSpace; // for the search string
					offset = gpApp->m_inlineNonbindingEndMarkers.Find(addSpaceToEndMkr);
					if (offset != wxNOT_FOUND)
					{
						// It's an inline non-binding endmarker (e.g. \wj* for wordsOfJesus) or
						// one of about 3 or 4 other possibilities -- just append it
						TheStr += aMarker;
						arrMkrsPtr->Clear();
						return TheStr;
					}
					else
					{
						// Not an one of the inline non-binding endmarkers, so let the user look
						// at it and place it manually
						return TheStr;
					}
				}
				else
				{
					// Not an endmarker, that's unexpected, so let the user look at it and
					// place it manually
					return TheStr;
				}
			}
		}
		else
		{
			// There is more than one marker to place. This is too ambiguous, so we better
			// let the user place them manually.
			return TheStr;
		}
	}
#ifdef __WXGTK__
	return TheStr; // Linux will probably want this to be here, Windows thinks it's unreachable
#endif
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
	posLast = posLast; // avoid warning
	bool bHasInternalMarkers = pMergedSrcPhrase->m_bHasInternalMarkers;
	bool bFirst = TRUE;
	bool bNonFinalEndmarkers = FALSE;
	bool bIsAmbiguousForEndmarkerPlacement = FALSE; // BEW 20May23 was not needed here, but because of
		// my docVersion 10 refactorings, this bool will be in signature of AutoPlaceSomeEndMarkers()
		// and AutoPlace..() is called herein, so need it to avoid link failure
	
	wxString tempStr;

	// store here any string of filtered information stored on pMergedSrcPhrase (in
	// m_filteredInfo only though)
	wxString Sstr; Sstr.Empty();
	wxString markersPrefix; markersPrefix.Empty();
	wxString Gstr = pMergedSrcPhrase->m_gloss; // could be empty
	//bHasFilteredMaterial = HasFilteredInfo(pMergedSrcPhrase);

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
	/* BEW removed 11Oct10 because even if it has content, we'll ignore it anyway
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
		// BEW 22Feb12, added test of m_glossMkrPattern string - if it has content, use it
		// to set Gstr, and don't show the placement dialog
		if (pMergedSrcPhrase->m_glossMkrPattern.IsEmpty())
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
				// whm 8Jun12 changed LookupSFM argument below to use wxStringBuffer. Note: The other
				// override of LookupSFM() requires an argument which is a bare marker which is not
				// the case here.
				USFMAnalysis* pSfm =  pDoc->LookupSFM(wxStringBuffer(mkr,mkr.Length())); //pDoc->LookupSFM((wxChar*)mkr.GetData());
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

			// BEW added 11Sep14, try to do autoplacement if there is just one marker to deal
			// with, it is likely to be \f*, \x* or \fe* & probably is the only one, if any
			// remain, then show the dialog instead; Sstr is not needed internally yet, but
			// one day may be
			Gstr = AutoPlaceSomeMarkers(Gstr, Sstr, pMergedSrcPhrase, &arrTemp, bIsAmbiguousForEndmarkerPlacement); // BEW 20May23 added bool

			if (!arrTemp.IsEmpty())
			{
				dlg.SetPlaceableDataStrings(&arrTemp);

				// show the dialog
				dlg.ShowModal();

				// get the post-placement resulting string
				Gstr = dlg.GetPostPlacementString();
#if defined (_DEBUG)
				wxLogDebug(_T("FromMergerMakeGstr() in helpers.cpp line %d, sequNum = %d, GetPostPlacementString returned [%s]"),
					__LINE__, Gstr.c_str());
#endif

                // as of version 6.2.0, we store the result whenever produced, so that the
                // placement dialog isn't opened again (unless the user puts phrase box at
                // this CSourcePhrase's location and edits either puncts or word(s) to be
                // placed differently - that causes the m_glossMkrPattern string to be
                // cleared (and m_tgtMkrPattern and m_punctsPattern too), and then this and
                // other placement dialogs would show again, if relevant -- that is, if
                // there is a placement ambiguity requiring that they show)
                Gstr = Gstr.Trim(FALSE);
                Gstr = Gstr.Trim();
				pMergedSrcPhrase->m_glossMkrPattern = Gstr;

				// make sure the doc is dirty, so the user will be prompted to save it -
				// we don't want this setting to get lost unnecessarily
				pDoc->Modify(TRUE);
			}
		}
		else
		{
			// it's non-empty, so use it as Gstr's value
			Gstr = pMergedSrcPhrase->m_glossMkrPattern;
		}
	}

	// now add the prefix string material not shown in the Place... dialog,
	// if it is not empty
	if (!markersPrefix.IsEmpty())
	{
		markersPrefix.Trim();
		if (!markersPrefix.IsEmpty())
		{
			markersPrefix += aSpace; // ensure a final space
			Gstr = markersPrefix + Gstr;
		}
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
		wxString s = pSrcPhrase->GetInlineNonbindingMarkers();
		appendHere += pSrcPhrase->GetInlineNonbindingMarkers();
		// BEW 21Jul14 ensure a latin space follows begin-marker(s)
		if (s.GetChar(s.Len() - 1) != _T(' '))
		{
			appendHere += _T(' ');
		}
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
		wxString s = pSrcPhrase->GetInlineBindingMarkers();
		wxString binders = pSrcPhrase->GetInlineBindingMarkers();
		binders.Trim(FALSE); // make sure no bogus space precedes
		appendHere += binders;
		if (s.GetChar(s.Len() - 1) != _T(' '))
		{
			appendHere += _T(' ');
		}
		bAddedSomething = TRUE;
	}
	return appendHere;
}

// a handy check for whether or not the wxChar which ptr points at is ~
// BEW created 25Jan11, used in FindParseHaltLocation() of doc class
bool IsFixedSpace(wxChar* ptr)
{
	if (*ptr == _T('~') )
	{
		return TRUE;
	}
	return FALSE;
}

// return TRUE if the ] (closing bracket) character is within the passed in string of
// punctuation characters (use the target punctuation character set), FALSE if absent
// Used in IsFixedSpaceAhead(), and the returned boolean is passed in to
// FindParseHaltLocation(); both are functions used by ParseWord()
bool IsClosingBracketWordBuilding(wxString& strPunctuationCharSet)
{
	int offset = strPunctuationCharSet.Find(_T(']'));
	return offset == wxNOT_FOUND;
}

// return TRUE if the [ (closing bracket) character is within the passed in string of
// punctuation characters (use the target punctuation character set), FALSE if absent
// BEW created 6Oct22, for when [ is pointed at by the tokenizing iterator ptr
bool IsOpeningBracketWordBuilding(wxString& strPunctuationCharSet)
{
	int offset = strPunctuationCharSet.Find(_T('['));
	return offset == wxNOT_FOUND;
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
/// BEW 21Jul14 refactored to support ZWSP and replacement of wordbreaks in exports
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
			str << PutSrcWordBreak(pSrcPhrase); // add the wordbreak

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
			// BEW 21Jul14, to support ZWSP etc, we don't add space after everything
			// but rather put either space or special space preceding the material
			// in the caller
			//str << aSpace;
		}

		if (bFirst)
		{
			bFirst = FALSE; // prevent this block from being re-entered

			str << PutSrcWordBreak(pSrcPhrase); // add the wordbreak

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
			// BEW 21Jul14, to support ZWSP etc, we don't add space after everything
			// but rather put either space or special space preceding the material
			// in the caller
			//str << aSpace;
		} // end TRUE block for test: if (bFirst)
		else if (!bFirst && !bLast)
		{
			// like the bFirst block, but the first function call is different, since it
			// needs to check for m_markers info on the saved original (non-merged)
			// pSrcPhrase we are currently looking at
			str << PutSrcWordBreak(pSrcPhrase); // add the wordbreak

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
			// BEW 21Jul14, to support ZWSP etc, we don't add space after everything
			// but rather put either space or special space preceding the material
			// in the caller
			//str << aSpace;
		}
	} // end of while loop

	// finally, ensure there is just a single final space
	str.Trim();
	// BEW 21Jul14, to support ZWSP etc, we don't add space after everything
	// but rather put either space or special space preceding the material
	// in the caller
	//str << aSpace;
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
///                             wordpair will need special treatment & use of the placement
///                             dialog at least once)
/// \param  Tstr             -> the string into which there might need to be
///                             placed m_markers and m_endmarkers material, (earlier versions also then
///                             prefixed with any filtered information, but we do this no longer in version 6.x.x)
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
/// BEW 13Feb12, added code for keeping placement dialog showing to once only, by storing
/// the placement dialog results, and versioning document to docVersion 6
/// BEW 22Jun15 refactored so that filtered information is not included in the export
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
	// BEW 22Jun15, the following are no longer used, so prevent compiler warnings
	wxUnusedVar(bDoCount);
	wxUnusedVar(bCountInTargetText);

	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	//SPList* pSrcPhrases = gpApp->m_pSourcePhrases;
	bool bHasOuterFollPunct = FALSE;
	bool bIsAmbiguousForEndmarkerPlacement = FALSE;
#if defined (_DEBUG)
	{
		wxLogDebug(_T("helpers.cpp FromSingleMakeTstr() START line %d, m_srcPhrase= [%s] , m_targetStr= [%s] , sequNum = %d, Tstr= [%s]"),
			__LINE__, pSingleSrcPhrase->m_srcPhrase.c_str(), pSingleSrcPhrase->m_targetStr.c_str(), pSingleSrcPhrase->m_nSequNumber, Tstr.c_str() );
		if (pSingleSrcPhrase->m_nSequNumber >= 11)
		{
			int halt_here = 1;
		}
	}
#endif
	// is it normal instance, or one which stores a word pair conjoined with USFM fixed
	// space symbol ~
	bool bIsFixedSpaceConjoined;
	bIsFixedSpaceConjoined = FALSE; // BEW 28Aug23 we no longer test for ~ conjoining, we just accept it as a longer word
	//bIsFixedSpaceConjoined = IsFixedSpaceSymbolWithin(pSingleSrcPhrase); // BEW 28Aug23 commented out
	bool bBindingMkrsToReplace = FALSE;
	wxString rebuiltTstr; rebuiltTstr.Empty();

	// BEW 21Jul14 ZWSP etc support -- add the word delimiter before everything else
	//PutSrcWordBreak(pSingleSrcPhrase); // tests for flag internally, if false, adds a legacy space
	/* BEW 28Aug23, commented out this block, we don't want to call RebuildFixedSpaceTstr() any more
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
	*/
    // BEW 22Jun15, store here, if anything, only begin-markers for non-filtered data
	wxString markersPrefix; markersPrefix.Empty();

	// BEW 22Jun15, these are no longer needed
	//wxString freeMkr(_T("\\free"));
	//wxString freeEndMkr = freeMkr + _T("*");
	//wxString noteMkr(_T("\\note"));
	//wxString noteEndMkr = noteMkr + _T("*");
	//wxString backTransMkr(_T("\\bt"));
	markersPrefix.Empty(); // clear it out
	wxString finalSuffixStr; finalSuffixStr.Empty(); // put collected-string-final endmarkers here

	wxString aSpace = _T(" ");
	wxString markersStr;
	wxString endMarkersStr;
	// The next five are retained because they are in the signature of functions which
	// may be called elsewhere, but here we clear these if any content is returned in them
	wxString freeTransStr;
	wxString noteStr;
	wxString collBackTransStr;
	wxString filteredInfoStr;
	wxString xrefStr; // for \x* .... \x* cross reference info (it is stored preceding
					  // m_markers content, but other filtered info goes before m_markers
	wxString otherFiltered; // leftovers after \x ...\x* is removed from filtered info

	// empty the scratch strings (ensures these are each empty at the start)
	EmptyMarkersAndFilteredStrings(markersStr, endMarkersStr, freeTransStr, noteStr,
									collBackTransStr, filteredInfoStr);
	// get the other string information we want, putting it in the scratch strings
	GetMarkersAndFilteredStrings(pSingleSrcPhrase, markersStr, endMarkersStr,
					freeTransStr, noteStr, collBackTransStr,filteredInfoStr);
	{
		// BEW 22Jun15 refactoring, we just empty these of any content - no filtered
		// data should be in the export
		freeTransStr.Empty(); noteStr.Empty(); collBackTransStr.Empty(); filteredInfoStr.Empty();
	}
	// BEW 22Jun15, next calls are no longer needed.
	/*
	// remove any filter bracketing markers if filteredInfoStr has content
	if (!filteredInfoStr.IsEmpty())
	{
		filteredInfoStr = pDoc->RemoveAnyFilterBracketsFromString(filteredInfoStr);

		// separate out any crossReference info (plus marker & endmarker) if within this
		// filtered information
		SeparateOutCrossRefInfo(filteredInfoStr, xrefStr, otherFiltered);
	}
	*/
    // BEW 22Jun15, for the one and only CSourcePhrase, we store in prefixSing any content in m_markers, if present, must be put at the start
    // of Tstr; remove LHS whitespace when done
    // BEW 8Sep10, changed the order to be: 1. filtered info, 2. collected bt 3.
	// note 4. free trans, to help with OXES parsing - we want any free trans to
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
	// the one below to GetUnfilteredCrossRefsAndMMarkers(), so
	// BEW 22Jun15 there is now no need to make this call
	//markersPrefix = GetUnfilteredInfoMinusMMarkersAndCrossRefs(pSingleSrcPhrase,
	//						pSrcPhrases, otherFiltered, collBackTransStr,
	//						freeTransStr, noteStr, bDoCount, bCountInTargetText); // m_markers
								// and xrefStr handled in a separate function, later below

	// BEW 11Oct10, the initial stuff is now more complex, so we can no longer insert
	// markersStr preceding the passed in m_targetStr value; so we'll define a new local
	// string, strInitialStuff in which to build the stuff which precedes m_targetStr and
	// then we'll insert it later below, after setting it with a function call
	wxString strInitialStuff;
	// now collect any beginmarkers and associated data from m_markers, into strInitialStuff,
	// and if there is content in xrefStr (and bAttachFilteredInfo is TRUE) then put that
	// content after the markersStr (ie. m_markers) content; delay placement until later
	// BEW 5Sep14, when collaborating we don't want any auto-unfiltering of filtered
	// information types, so wrap with a test - but we want some of this next stuff (the
	// m_markers and m_endMarkers is wanted, for example), so tests will be done internally
	// BEW 22Jun15, refactored internally so it returns only markers, but no filtered info
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
#if defined (_DEBUG)
	{
		wxLogDebug(_T("helpers.cpp FromSingleMakeTstr() line %d, finalPuncts= [%s] , m_targetStr= [%s] , tgtBaseStr = [%s], Tstr= [%s]"),
			__LINE__, finalPuncts.c_str(), pSingleSrcPhrase->m_targetStr.c_str(), tgtBaseStr.c_str(), Tstr.c_str());
		if (pSingleSrcPhrase->m_nSequNumber >= 11)
		{
			int halt_here = 1;
		}
	}
#endif

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
#if defined (_DEBUG)
		{
			wxLogDebug(_T("helpers.cpp FromSingleMakeTstr() line %d, finalPuncts= [%s] , m_targetStr= [%s] , tgtBaseStr = [%s], Tstr= [%s]"),
				__LINE__, finalPuncts.c_str(), pSingleSrcPhrase->m_targetStr.c_str(), tgtBaseStr.c_str(), Tstr.c_str());
			if (pSingleSrcPhrase->m_nSequNumber >= 11)
			{
				int halt_here = 1;
			}
		}
#endif

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
#if defined (_DEBUG)
		{
			wxLogDebug(_T("helpers.cpp FromSingleMakeTstr() line %d, finalPuncts= [%s] , m_targetStr= [%s] , tgtBaseStr = [%s], Tstr= [%s]"),
				__LINE__, finalPuncts.c_str(), pSingleSrcPhrase->m_targetStr.c_str(), tgtBaseStr.c_str(), Tstr.c_str());
			if (pSingleSrcPhrase->m_nSequNumber >= 11)
			{
				int halt_here = 1;
			}
		}
#endif
		if (!finalPuncts.IsEmpty() && !pSP->GetInlineNonbindingEndMarkers().IsEmpty())
		{
			if (finalsLen > 1)
			{
				bIsAmbiguousForEndmarkerPlacement = TRUE;
			}
			nonbindingEndMkrsToPlace = pSP->GetInlineNonbindingEndMarkers();
			markersToPlaceArray.Add(nonbindingEndMkrsToPlace);
		}
#if defined (_DEBUG)
		{
			wxLogDebug(_T("helpers.cpp FromSingleMakeTstr() line %d, finalPuncts= [%s] , m_targetStr= [%s] , tgtBaseStr = [%s], Tstr= [%s]"),
				__LINE__, finalPuncts.c_str(), pSingleSrcPhrase->m_targetStr.c_str(), tgtBaseStr.c_str(), Tstr.c_str());
			if (pSingleSrcPhrase->m_nSequNumber >= 11)
			{
				int halt_here = 1;
			}
		}
#endif
		// BEW 19Jun12, another scenario for ambiguity is an endmarker (such as \f*)
		// followed by outer following punctuation (such as closing doublequote) and there
		// was no normal punctuation to end the footnote - hence finalPuncts is empty. In
		// this scenario bIsAmbiguousForEndmarkerPlacement is still FALSE, and if we don't
		// add a block to test for this ambiguity situation, the code below will say there
		// is ambiguity but markersToPlaceArray is empty, and so no placement dialog shows
		// and the endmarker and outer punctuation just get lost. So I'm fixing this here.
		bool bAddOuterPuncts = FALSE;
		if (finalPuncts.IsEmpty() && !pSP->GetEndMarkers().IsEmpty()
			&& !pSP->GetFollowingOuterPunct().IsEmpty())
		{
			// this following outer punctuation hasn't been placed yet
			bIsAmbiguousForEndmarkerPlacement = TRUE;
			markersToPlaceArray.Add(pSP->GetEndMarkers());
			bAddOuterPuncts = TRUE; // use this later, when it needs to be done
		}
#if defined (_DEBUG)
		{
			wxLogDebug(_T("helpers.cpp FromSingleMakeTstr() line %d, finalPuncts= [%s] , m_targetStr= [%s] , tgtBaseStr = [%s], Tstr= [%s]"),
				__LINE__, finalPuncts.c_str(), pSingleSrcPhrase->m_targetStr.c_str(), tgtBaseStr.c_str(), Tstr.c_str());
			if (pSingleSrcPhrase->m_nSequNumber >= 11)
			{
				int halt_here = 1;
			}
		}
#endif
		// build the core of Tstr, using tgtStr and starting with tgtBaseStr
		if (!pSP->GetInlineBindingMarkers().IsEmpty())
		{
			tgtStr = pSP->GetInlineBindingMarkers() + tgtBaseStr;
		}
		else
		{
			tgtStr = tgtBaseStr;
		}
#if defined (_DEBUG)
		{
			wxLogDebug(_T("helpers.cpp FromSingleMakeTstr() line %d, finalPuncts= [%s] , m_targetStr= [%s] , tgtBaseStr = [%s], Tstr= [%s]"),
				__LINE__, finalPuncts.c_str(), pSingleSrcPhrase->m_targetStr.c_str(), tgtBaseStr.c_str(), Tstr.c_str());
			if (pSingleSrcPhrase->m_nSequNumber >= 11)
			{
				int halt_here = 1;
			}
		}
#endif
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
#if defined (_DEBUG)
		{
			wxLogDebug(_T("helpers.cpp FromSingleMakeTstr() line %d, finalPuncts= [%s] , m_targetStr= [%s] , tgtBaseStr = [%s], Tstr= [%s]"),
				__LINE__, finalPuncts.c_str(), pSingleSrcPhrase->m_targetStr.c_str(), tgtBaseStr.c_str(), Tstr.c_str());
			if (pSingleSrcPhrase->m_nSequNumber >= 11)
			{
				int halt_here = 1;
			}
		}
#endif
		// BEW 19Jun12 added test and addition of following outer puncts
		if (bAddOuterPuncts)
		{
			tgtStr += pSP->GetFollowingOuterPunct();
		}
		Tstr = tgtStr; // we've got any inline binding markers in place, now for the rest
	} // end of else block for test: if (bBindingMkrsToReplace)

	// an inline non-binding (begin)marker is next, if there is one
	if (!pSP->GetInlineNonbindingMarkers().IsEmpty())
	{
		Tstr = pSP->GetInlineNonbindingMarkers() + Tstr;
	}
#if defined (_DEBUG)
	{
		wxLogDebug(_T("helpers.cpp FromSingleMakeTstr() line %d, finalPuncts= [%s] , m_targetStr= [%s] , tgtBaseStr = [%s], Tstr= [%s]"),
			__LINE__, finalPuncts.c_str(), pSingleSrcPhrase->m_targetStr.c_str(), tgtBaseStr.c_str(), Tstr.c_str());
		if (pSingleSrcPhrase->m_nSequNumber >= 11)
		{
			int halt_here = 1;
		}
	}
#endif
	// add any m_markers content and unfiltered xref material if present, preceding what
	// we have so far
	if (!strInitialStuff.IsEmpty())
	{
        // this data has any markers 'in place' & things like verse number etc if relevant,
        // and any crossReference material will be after the verse number (other unfiltered
        // filtered info to come from what is in markersPrefix will be preceding the \v )
		Tstr = strInitialStuff + Tstr;
	}
#if defined (_DEBUG)
	{
		wxLogDebug(_T("helpers.cpp FromSingleMakeTstr() line %d, finalPuncts= [%s] , m_targetStr= [%s] , tgtBaseStr = [%s], Tstr= [%s]"),
			__LINE__, finalPuncts.c_str(), pSingleSrcPhrase->m_targetStr.c_str(), tgtBaseStr.c_str(), Tstr.c_str());
		if (pSingleSrcPhrase->m_nSequNumber >= 11)
		{
			int halt_here = 1;
		}
	}
#endif

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
		// there is ambiguity, so do the placement using the dialog -- BEW 22Feb12 added a
		// check for m_tgtMkrPattern having content; if it does, use that for the Tstr
		// value (before markersPrefix's contents get added), and so refrain from showing
		// the placement dialog; but if the string is empty, then show the dialog
#if defined (_DEBUG)
		{
			if (pSingleSrcPhrase->m_nSequNumber >= 11)
			{
				int halt_here = 1; wxUnusedVar(halt_here);
			}
		}
#endif

		if (pSingleSrcPhrase->m_tgtMkrPattern.IsEmpty())
		{
			wxString xrefStr;
			wxString mMarkersStr;
			wxString otherFiltered;
			bool bAttachFiltered = FALSE;
			bool bAttach_m_markers = TRUE;
			wxString Sstr = FromSingleMakeSstr(pSingleSrcPhrase, bAttachFiltered,
				bAttach_m_markers, mMarkersStr, xrefStr, otherFiltered, TRUE, FALSE); // need Sstr
					// for the dialog; and we pass it to AutoPlaceSomeMarkers(), but the latter
					// currently does not use it internally (one day, it might)
#if defined (_DEBUG)
			{
				int SstrLen = Sstr.Length();
				wxLogDebug(_T("FromSingleMakeStr() line %d, MADE Sstr= [%s] , has length= %d"), __LINE__, Sstr.c_str(), SstrLen);
				if (pSingleSrcPhrase->m_nSequNumber >= 11)
				{
					int halt_here = 1; wxUnusedVar(halt_here);
				}
			}
#endif
			// BEW 1Sep23 here is where I need to analyse Sstr to get the associations between each
			// endMkr and any whites or puncts following it before the next marker. I'll probably
			// work on a copy of Sstr so that the analysis can progressively remove bits analysed,
			// until there are no more available for analysis
			wxArrayString arrItems;
			wxString separator = _T("\\"); // separate with backslash
			wxString CopiedTstr = Tstr;  // I want to work on a copy, so I can compare with Tstr after analysis
			bIsAmbiguousForEndmarkerPlacement = AnalyseSstr(Sstr, arrItems, separator, CopiedTstr, tgtBaseStr);
			if (bIsAmbiguousForEndmarkerPlacement == FALSE)
			{
				// If control enters here, the Placement dialog is avoided. So do here
				// the extra things needed to make use of the data stored in arrItems
#if defined (_DEBUG)
				{
					int SstrLen = Sstr.Length();
					wxLogDebug(_T("FromSingleMakeStr() line %d, analysis MADE CopiedTstr= [%s] , original Tstr= [%s]"),
						__LINE__, CopiedTstr.c_str(), Tstr.c_str());
					if (pSingleSrcPhrase->m_nSequNumber >= 11)
					{
						int halt_here = 1; wxUnusedVar(halt_here);
					}
				}
#endif
				// Comparing CopiedTstr with original Tstr, the former has it all correct, the original has just ;?” sfter \em*
				// and CopiedTstr will have target text punctuation glyphs, so proceed by using CopiedTstr
				Tstr = CopiedTstr;

				// TODO  after analysing, other things do hereas in post-dialog code e.g. set m_tgtMkrPattern 
				// (do we need to handle mergers? probably) set m_tgtSinglePattern if not a merger
				// or if m_bTstrFromMergerCalled is TRUE, m_tgtMkrPattern, etc












			} // end of TRUE block for test: if (bIsAmbiguousForEndmarkerPlacement == FALSE)
			else
			// BEW added 11Sep14, If there is just a single marker to be placed, try do it
			// automatically. If the markersToPlaceArray is returned empty, then we won't
			// need to show the placement dialog
			if (bIsAmbiguousForEndmarkerPlacement)
			{
				// BEW added above test, 5Sep23 - if TRUE, the Placement dlg will show
				Tstr = AutoPlaceSomeMarkers(Tstr, Sstr, pSingleSrcPhrase, &markersToPlaceArray, bIsAmbiguousForEndmarkerPlacement); // BEW 20May23 added bool

				// BEW 28Oct22, a hack for removing an unsettling outcome (Gerald Harkins reported Oct 2022) as follows.
				// Situation 1. There is a footnote and \f* (or other endMkr) has to be placed manually, but Sstr has
				// more than one final punct character - therefore, ambiguity, and manual placement is needed. But on
				// just having set up collaboration, and there is no text yet within the m_adaption of pSingleSrcPhrase,
				// there's no text to show in the bottom box of the placement dialog. But punctuation my appear there,
				// because pSingleSrcPhrase has one or more in the aggregate of m_follPunct and m_follOuterPunct. Showing
				// just punctuation in the dialog is confusing and unhelpful.
				// Situation 2. Adapting has been done, and at the export for collaboration, there is text to show. The
				// existing code *should* handle the placement ambiguity, so probably nothing to be done. 
				// What to do about Situation 1. My intuition is this: If app->m_bCopySource is FALSE, and if Tstr only
				// contains one or more punctuation characters - then the copy the reconstituted Sstr - because Paratext
				// (or Bibledit) needs the markers in their correct place(s) in the string. And empty the array of markers
				// because placing it not appropriate - only editing is appropriate in this scenario.
				// But, if m_bCopySource is TRUE, then the best thing would be to show a copy of the Sstr text - with 
				// endmarkers removed, and the placement list showing the markers for placement, and the user does the
				// placements in the normal way, and clicks OK. (In either scenario, the workaround of coping from the
				// top box, and pasting into the bottom one, is still available.)
				bool bEmptyOrPunctsOnly = IsEmptyOrPunctuationOnly(Tstr, gpApp->m_punctuation[1]);
				if (bEmptyOrPunctsOnly && (gpApp->m_bCopySource == FALSE))
				{
					bIsAmbiguousForEndmarkerPlacement = FALSE;
					markersToPlaceArray.Empty();
					Tstr.Empty();
				}

				// BEW 28Oct22 tested both options above, for app->m_bCopySource TRUE, and then FALSE. They work, and the
				// user is spared the connundrum of not having a valid Placement dialog with editable lower box and the
				// list with one or more markers to place.
#if defined (_DEBUG)
				{
					if (pSingleSrcPhrase->m_nSequNumber >= 11)
					{
						int halt_here = 1; wxUnusedVar(halt_here);
					}
				}
#endif
				if (!markersToPlaceArray.IsEmpty() && !Tstr.IsEmpty())
				{
					// There's something the user needs to deal with manually

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
#if defined (_DEBUG)
					wxLogDebug(_T("FromSingleMakeTstr() in helpers.cpp line %d, sequNum = %d, GetPostPlacementString returned [%s]"),
						__LINE__, pSingleSrcPhrase->m_nSequNumber, Tstr.c_str());
#endif
				}
				// as of version  6.2.0, we store the result whenever produced, so that
				// the placement dialog isn't opened again (unless the user puts phrase
				// box at this CSourcePhrase's location and edits either puncts or word(s)
				// to be placed differently - that causes the m_tgtMkrPattern string to be
				// cleared, and then this and other placement dialogs would show again, if
				// relevant -- that is, if there is a placement ambiguity requiring that
				// they show); trim off any whitespace before saving in the CSourcePhrase
				// -- the dialog puts a space before and after, so we must get rid of
				// these before saving
				Tstr = Tstr.Trim(FALSE);
				Tstr = Tstr.Trim();
				// make sure the doc is dirty, so the user will be prompted to save it -
				// we don't want this setting to get lost unnecessarily
				pDoc->Modify(TRUE);
			
			} // end of the TRUE block for test:  else if (bIsAmbiguousForEndmarkerPlacement)
			else
			{
				// it's non-empty, so use it as Tstr's value (first ensure there is no
				// preceding or final whitespace)
#if defined (_DEBUG)
				if (pSingleSrcPhrase->m_nSequNumber >= 12)
				{
					int halt_here = 1; wxUnusedVar(halt_here);
				}
#endif
				wxString str = pSingleSrcPhrase->m_tgtMkrPattern;
				str.Trim(FALSE);
				str.Trim();
				Tstr = str;
			} // end of the else block for test: else if (bIsAmbiguousForEndmarkerPlacement)
		} // end of TRUE block for test: 

		// ********** MERGERS: legacy code not yet altered, as at 5Sep 23 ************

		if (pDoc->m_bTstrFromMergerCalled)
		{
			// BEW 19May23 There was a prior call of FromMergerMakeTstr() at this current
			// location, so because a Tstr value was obtained from that call, it is to 
			// be stored in m_tgtMkrPatter - as of docVersion 6 and higher.
			pSingleSrcPhrase->m_tgtMkrPattern = Tstr;
		}
		else
		{
			// There was no prior successful call of FromMergerMakeTstr(), but since 
			// Tstr is also constructed similarly here in FromSingleMakeTstr(), docVersion 10
			// will store it here in the new member of CSourcePhrase: m_tgtSinglePattern
			pSingleSrcPhrase->m_tgtSinglePattern = Tstr;
		}
		// As pSrcPhrase is now likely to be moved to a new location, set the bool to default FALSE
		// (it's also defaulted to FALSE prior to a call of FromMergerMakeTstr() )
		pDoc->m_bTstrFromMergerCalled = FALSE;
	}

    // now add the prefix string material if it is not empty
	if (!markersPrefix.IsEmpty())
	{
		markersPrefix.Trim();
		if (!markersPrefix.IsEmpty())
		{
			markersPrefix << aSpace; // ensure a final space after any markers
			Tstr = markersPrefix + Tstr;
		}
	}
	Tstr.Trim(FALSE);
	Tstr.Trim();
	// don't have a final space, the caller will add one if it is needed
#if defined (_DEBUG)
	{
		if (pSingleSrcPhrase->m_nSequNumber >= 11)
		{
			int halt_here = 1; wxUnusedVar(halt_here);
		}
	}
#endif

	return Tstr;
}

bool IsEmptyOrPunctuationOnly(wxString Tstr, wxString puncts)
{
	if (Tstr.IsEmpty())
	{
		return TRUE;
	}
	int len = Tstr.Length();
	int offset = wxNOT_FOUND;
	wxChar aChar;
	int index;
	for (index = 0; index < len; index++)
	{
		aChar = Tstr.GetChar((size_t)index);
		offset = puncts.Find(aChar);
		if (offset == wxNOT_FOUND)
		{
			// Something else which is not in the puncts character set, is present, so return FALSE
			return FALSE;
		}
	}
	// If the loop doesn't exit before running of chars to check, it only has puncts
	return TRUE;
}


// BEW created 11Oct10, for support of improved doc version 5 functionality.
// Used in the FromSingleMakeTstr() function, when there are inline binding markes within
// the conjoining with USFM fixed space marker ~ joining a pair of words. No marker
// Placement dialog needs to be used for this process.
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
	// BEW 24Oct14 in the next call, TRUE is bool bTokenizingTargetText
	word1 = ParseWordInwardsFromEnd(ptr, pEnd, wordBuildersForPostWordLoc,
									gpApp->m_punctuation[1], TRUE);
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
	// BEW 24Oct14 in the next call, TRUE is bool bTokenizingTargetText
	word2 = ParseWordInwardsFromEnd(ptr, pEnd, wordBuildersForPostWordLoc,
									gpApp->m_punctuation[1], TRUE);
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
	{
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
	}
		break;
	default:
	case UsfmOnly:
	{
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
	} // end of case UsfmOnly:
		break;
	} // end of switch (set)
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
/// bDoCount				->  Whether or not to count the words in any free translation (now deprecated 22Jun15)
/// bCountInTargetText      ->  Whether or do the count of words, in source text line or tgt text line (now deprecated 22Jun15)
/// BEW created 11Oct10 for support of additions to doc version 5 for better USFM support
/// BEW refactored 22Jun15, so that no filtered information is returned.
/// Because of this refactoring, the bAttachFilteredInfo, even if passed in TRUE, will have
/// no filtered info to attach; similarly, xrefStr will be empty always, likewise filteredInfoStr
wxString FromSingleMakeSstr(CSourcePhrase* pSingleSrcPhrase, bool bAttachFilteredInfo,
				bool bAttach_m_markers, wxString& mMarkersStr, wxString& xrefStr,
				wxString& filteredInfoStr, bool bDoCount, bool bCountInTargetText)
{
	//CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	//SPList* pSrcPhrases = gpApp->m_pSourcePhrases;
	wxUnusedVar(bCountInTargetText); // BEW added 22Jun15
	wxUnusedVar(bDoCount); // BEW added 22Jun15
	wxUnusedVar(bAttach_m_markers); // 5May23 unreferenced, no longer needed
	wxUnusedVar(bAttachFilteredInfo); // 5May23 unreferenced, no longer needed
	wxUnusedVar(mMarkersStr); // 8May23 no longer needed
	wxUnusedVar(filteredInfoStr); // 8May23 no longer needed
	wxUnusedVar(xrefStr); // 8May23 no longer needed
	bool bEndPunctsModified = FALSE; // init
	wxString pattern; pattern = wxEmptyString; // init

	wxString Sstr;
	wxString aSpace = _T(" ");
	wxString markersStr = pSingleSrcPhrase->m_markers; // prefix it at end

	//mMarkersStr = markersStr; // unilaterally returned to caller, in case it wants it
	//CSourcePhrase* pSP = pSingleSrcPhrase; // RHS is too long to type all the time

	wxString srcStr = wxEmptyString; // init  (was pSP->m_key;)
#if defined (_DEBUG)
	if (pSingleSrcPhrase->m_nSequNumber >= 19)
	{
		int halt_here = 1;
	}
#endif
	// BEW 4Apr23, if there is a isolated backslash in the document, m_key will be empty, 
	// but m_srcPhrase will be _T("\\"), we want to keep the backslash in the exported
	// source text, so test and set srcStr to it
	if (pSingleSrcPhrase->m_key.IsEmpty() && pSingleSrcPhrase->m_srcPhrase == _T("\\"))
	{
		srcStr = _T("\\");
		if (!pSingleSrcPhrase->m_markers.IsEmpty())
		{
			srcStr = pSingleSrcPhrase->m_markers + srcStr;
		}
		Sstr = srcStr;
		return Sstr;
	}
// 5May23 new code goes here
	wxString extras = wxEmptyString;
	wxString strSaveExtras;
	strSaveExtras = wxEmptyString; // init
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();

	// BEW 10May23, pSrcPhrase (docVersion == 10) has a new wxString, m_oldKey which stores
	// the key parsed in the doc ParseWords() call. m_srcSinglePattern has the m_key value at that
	// time. If a src text edit is later done - making the m_key value different, or PT source text project
	// has been user-edited to change the m_key spelling, then we don't want the m_oldKey value to
	// be constant; instead since m_key may be different, we need to compare the new value with what's
	// in m_oldKey, and if different, use the new value - and make the appropriate value changes within
	// pSrcPhrase->m_srcSinglePattern and m_oldKey, to comply with the new value as well. Do it here, so
	// that the key is up-to-date before we start examining puncts and markers which are before or after it.
	wxString currSrcSinglePattern = pSingleSrcPhrase->m_srcSinglePattern;
	wxString currKey = pSingleSrcPhrase->m_oldKey;
	wxString newKey = pSingleSrcPhrase->m_key;
	wxString newSrcSinglePattern = wxEmptyString;
	if (newKey != currKey)
	{
		// There has been a value change to the m_key member, do the updating required
		int oldKeyLen = currKey.Length();
		wxString rightBit = currSrcSinglePattern.Mid(oldKeyLen); // keep rightBit (may be empty), chuck old key
		newSrcSinglePattern = newKey + rightBit;
		pSingleSrcPhrase->m_srcSinglePattern = newSrcSinglePattern; // it's now updated
		pSingleSrcPhrase->m_oldKey = newKey; // it's now updated
	}

	// FromSingleMakeSstr accesses the new member in CSourcePhrase, m_srcSinglePattern, 
	// not the other four new ones; the one to read for the source pattern is the 2nd param of next call
	extras = pDoc->GetPostwordExtras(pSingleSrcPhrase, pSingleSrcPhrase->m_srcSinglePattern);

	// BEW added 10Jul23  so we can convert extras into an analysable pattern
	strSaveExtras = extras; // after the GetPostwordExtras() call, extras does not have the initial m_key info
			// but it will have whatever else follows, be it puncts, markers, or whitespaces, in their occurrence 
			//order; so strSaveExtras is what is to be used for converting into a pattern for analysis/comparisons
	// TEST
	/* BEW 25Aug23 - this is incomplete, and I may handle punct changes differently, so comment out. Bill was getting a fail here too.
#if defined (_DEBUG)
	if (pSingleSrcPhrase->m_nSequNumber >= 2)
	{
		int halt_here = 1;
		wxString endMkrsRemoved; endMkrsRemoved = wxEmptyString;
		endMkrsRemoved = ConvertEndMkrs2BEN(extras, pSingleSrcPhrase);
	}
#endif
	*/
	// end of TEST

	extras = pDoc->RemoveEndMkrsFromExtras(extras);
	bool bIsOK = FALSE; // init
	int extrasLen = -1; // init - if bAllMated is TRUE, extrasLen should be zero, and residue empty
	wxString residue = wxEmptyString; // init
	// Next call will return TRUE if (a) extras was empty, so m_srcSinglePattern suffices; or (b) there
	// were one or more final puncts (possibly mixed with endmarkers), and matching those in pSrcPhrase
	// successfully with those in extras, with no residue left over, happens - in which case
	// m_srcSinglePattern suffices then also
	bIsOK = pDoc->Qm_srcPhrasePunctsPresentAndNoResidue(pSingleSrcPhrase, extras, extrasLen, residue, bEndPunctsModified); // endMkrsOnly);
	// For option (b) in comment above, the matching algorithm removes each matched up char pair (it
	// handles ">>" as a special case), reducing extras to empty, or to just one or more whitespace chars
	// - which we skip over as they are not puncts, so if extrasLen gets reduced to 0 then matchups succeeded
	
	// BEW 13Jul23, must not forget that there could be non-empty m_precPunct member, if so, start of srcStr
	// with that value
	if (!pSingleSrcPhrase->m_precPunct.IsEmpty())
	{
		srcStr = pSingleSrcPhrase->m_precPunct;
	}
	if (bIsOK && extrasLen == 0 && residue.IsEmpty())
	{
		// The contents of m_srcSinglePattern are the correct post-word mix of puncts and endmarkers, or,
		// there were no word-final puncts (but endMkrs may have been squirreled away on pSingleSrcPhrase)
		// and if so, they will be there in m_srcSinglePattern anyway 
		// (bEndPunctsModified stays FALSE when control has entered this block)
		srcStr += pSingleSrcPhrase->m_srcSinglePattern;
	}
	else
	{
		// BEW 10Jul23 put the function for converting extras string into a pattern, here
		pattern = ConvertExtrasToPattern(strSaveExtras, pSingleSrcPhrase);

		// When if (bIsOK && extrasLen == 0 && residue.IsEmpty()) is FALSE... then matching by string equality
		// for puncts in matching positions failed to reduce the residue to empty. Matchup failure(s) will
		// cause bEndPunctsModified to be TRUE. That's not a problem if the before and after count of puncts
		// to be matched up is the same - we can programmatically make likely correct substitutions by
		// position using the function UpdateSingleSrcPattern() below.
		if (bEndPunctsModified)
		{
			// Put here an algorithm which can handle punctuation changes. There are only three? ways for the
			// user to be able to change source text puncts. 
			// (1) Make the appropriate source text words pSrcPhrase be selected, or at active location,
			// and Select the option "Edit Source Text" - the user can then type a different word, or different
			// puncts, or both in the dialog that pops up. (Markers won't be seen, and should NOT be manually
			// typed in the dialog, they are inviolate constant substrings in the doc's USFM structure).
			// Collaboration does not have to be active to do this, in fact, collaboration suppresses this option
			// and requires changes be made instead in the source text in Paratext (or Bibledit).
			// (2) In a collaboration, the user can enter the source text project in Paratext (or Bibledit)
			// and there type a different word or different punctuations or both. The collaboration will call
			// OnSingleMakeTstr() which internally calls OnSingleMakeSstr() which will cause new values (if
			// changed) be put into m_follPunct and perhaps also into m_inlineBindingEndMarkers and/or into
			// m_inlineNonbindingEndMarkers (though puncts after markers like \wj* etc are very unlikely).
			// If the number of puncts has not changed, then we can use the new set without a Placement dlg,
			// but if the inventory is fewer or more, the only way to be sure of accuracy is to do Placement dlg.
			// (3) If the user, in the phrasebox, manually adds end puncts which differ at least in 1 place
			// from those in pSrcPhrase->m_srcSinglePattern (at same sequ num). Doing that should also then
			// result in view's MakeTargetStringIncludingPuctuation() using the changed punc(s), and that
			// function contains a Placement dialog which we may need to call if the inventory of ending
			// punctuations differs from those in m_srcSinglePattern (at same sequNum)


			// This is a function which matches by positions, since equality tests won't work
			bool bTokenizingTargetText = FALSE; // needed for next call, so we use src spacelessPuncts
			bool bUpdatedOK = pDoc->UpdateSingleSrcPattern(pSingleSrcPhrase, bTokenizingTargetText);


		}
		else
		{
			// If no manual puncts changed, then what's gone wrong could be anything, but most likely there
			// is a residue which is not empty - such as when there are extra puncts added. So just return
			// m_srcSinglePattern 'as is' with any residual puncts appended - could fluke a correct result
			srcStr = pSingleSrcPhrase->m_srcSinglePattern; 
			if (!residue.IsEmpty())
			{
				srcStr << residue;
			}
		}
		// BEW 13Jul23, must not forget that there could be non-empty m_precPunct member, 
		//if so, insert that value ahead of whatever srcStr currently is
		if (!pSingleSrcPhrase->m_precPunct.IsEmpty())
		{
			srcStr = pSingleSrcPhrase->m_precPunct + srcStr;
		}
	} // end of else block for test: if (bIsOK && extrasLen == 0 && residue.IsEmpty())

    // now add the prefix string material if it is not empty
	wxString prefixStr = wxEmptyString;
	if (!bEndPunctsModified)
	{
		if (!markersStr.IsEmpty())
		{
			// prefix it, it may have markers like \s, \s1, \p, \v etc
			prefixStr = markersStr;
		}
		// Next, there could be inline nonbinding beginMkr like \wj
		wxString strNonbinding = pSingleSrcPhrase->GetInlineNonbindingMarkers();
		if (!strNonbinding.IsEmpty())
		{
			prefixStr << strNonbinding;
		}
		// Next, sometimes there may even be character formatting beginMkr(s)
		wxString strBinding = pSingleSrcPhrase->GetInlineBindingMarkers();
		if (!strBinding.IsEmpty())
		{
			prefixStr << strBinding;
		}
		// Now whatever we have, (could be empty) prefix to srcStr
		if (!prefixStr.IsEmpty())
		{
			srcStr = prefixStr + srcStr;
		}
	} // end of TRUE block for test: if (!bEndPuntsModified)
	else
	{







// TODO if needed when end puncts were modified

	} // end of else block for test: if (!bEndPuntsModified)

	/*
	pSingleSrcPhrase->m_srcPhrase = pSingleSrcPhrase->m_srcSinglePattern; // that's the word plus what follows

	if (!pSingleSrcPhrase->m_precPunct.IsEmpty())
	{
		pSingleSrcPhrase->m_srcPhrase = pSingleSrcPhrase->m_precPunct + pSingleSrcPhrase->m_srcPhrase;
	}
	if (!markersStr.IsEmpty())
	{
		markersStr.Trim();
		markersStr << aSpace; // ensure a final space
		pSingleSrcPhrase->m_srcPhrase = markersStr + pSingleSrcPhrase->m_srcPhrase;
	}
	*/
#if defined (_DEBUG)
	if (pSingleSrcPhrase->m_nSequNumber >= 19)
	{
		int halt_here = 1;
	}
#endif

	Sstr = srcStr;
	Sstr.Trim(FALSE); // remove any intial whitespace(s)
	return Sstr;
}

// BEW 10Jul23 added. Analysis for coping with manual punctuation changes, to reduce 
// incidence of Placement dialogs appearing. Markers and puncts, whitespace too are
// given symbols - left to right, according to the order of these in extras string
// passed in. Extras string lacks the initial m_key string, but retains the rest.
// Pass in the current CSourcePhrase pointer, as we need to check it's marker storages
// for each endMkr encountered in traversing over the extras span
wxString ConvertExtrasToPattern(wxString extras, CSourcePhrase* pSP) 
{
	if (extras.IsEmpty())
	{
		return extras;
	}
	// Markers are of 3 kinds, and are constant in form, and so form "islands" within
	// which other things are in their context. Markers are uppercase: B for any inline
	// binding endMkrs, E for any that would belong in m_endMarkers, N for any that would
	// belong in inline non-binding endMkrs storage.
	// Whitespace is represented by w. A punctuation char, by p.
	// A rule: each of B, E, and N are to function like boundaries, we do not allow
	// transfer of puncts or whites across any of these three symbols. This rule allows
	// us to provide a likely robust final string of markers, puncts, and whites, not
	// just when the puncts count does not change, but also when it does - whether more
	// or fewer.
	// Processing makes use of IsWhiteSpace(wxChar* ), and IsPunctuation(wxChar*, bParsingSrcText = TRUE)
	// An example, from my unittest: __test_placement_removal.txt, at sequNum 11: 
	// extras is: \em*;\f*?”\wj* and ConvertExtrasToPattern() should convert that and return  BpEppN
#if defined (_DEBUG)
	if (pSP->m_nSequNumber == 2)
	{
		int halt_here = 1;
	}
#endif

	wxString pattern; pattern = wxEmptyString; // init
	/* // whm 25Aug2023 commented out this call since BEW commented out the other call back in FromSingleMakeSstr()
	wxString endMkrsRemoved;

	//endMkrsRemoved = ConvertEndMkrs2BEN(extras, pSP);  comment out until I do and finish work on a good algorithm within it

#if defined (_DEBUG)
	if (pSP->m_nSequNumber == 2)
	{
		wxLogDebug(_T("ConvertExtrasToPatter() line %d , endMkrsRemoved= [%s]"), __LINE__, endMkrsRemoved.c_str());
		int halt_here = 1;
	}
#endif
	*/







	return pattern;
}

// BEW 12Jul23 added
wxString ConvertEndMkrs2BEN(wxString extras, CSourcePhrase* pSP)
{
	if (extras.IsEmpty())
	{
		return wxEmptyString;
	}
	int extrasLen = extras.Length();
	int offset = wxNOT_FOUND; // init
	int offset2 = wxNOT_FOUND; // init
	int offset3 = wxNOT_FOUND; // init  (use both offset2 and offset 3 when testing both red and blue fast-access strings)
	wxChar backslash = _T('\\');
	wxString strResult = wxEmptyString; // init
	strResult = extras;  // LHS will change (shorten)with each endMkr converted to one of B, E or N
	CAdapt_ItApp* pApp = &wxGetApp();
	CAdapt_ItDoc* pDoc = pApp->GetDocument();
	wxString wholeMkr = wxEmptyString;
	wxString charFormatEMkrs = pApp->m_charFormatEndMkrs;
	wxString blueEMkrs = pApp->m_BlueEndMarkers;
	wxString redEMkrs = pApp->m_RedEndMarkers;
	wxString nonBindingEMkrs = pApp->m_inlineNonbindingEndMarkers;
	wxChar chB = _T('B');
	wxChar chE = _T('E');
	wxChar chN = _T('N');
	// any from charFormatEMkrs should be in pSP->m_inlineBindingEndMarkers;
	// any from blueEMkrs or redEMkrs should be in pSP->m_endMarkers
	// any from nonBindingEMkrs should be in pSP->m_inlineNonbindingEndMarkers;
	// all of these require use of accessors to get and set
#if defined (_DEBUG)
	if (pSP->m_nSequNumber == 2)
	{
		int halt_here = 1;
	}
#endif
	wxString emkr = wxEmptyString; // init
	wxString augEmkr = wxEmptyString; // init
	wxString strAccum = wxEmptyString; // accumulate finished parts herein
	int emkrLen = 0; // init
	offset = strResult.Find(backslash);

	while (offset >= 0)
	{
		if (offset == 0)
		{
			// strResult starts with an endMkr
			emkr = pDoc->GetWholeMarker(strResult);
			augEmkr = emkr + _T(' '); // for the lookups in the fast-access strings
			emkrLen = emkr.Length();
			// What emkr kind is it?
			offset2 = charFormatEMkrs.Find(augEmkr);
			if (offset2 >= 0)
			{
				// It's a B endmkr
				strAccum << chB;
				// Get ready for next iteration
				strResult = strResult.Mid(emkrLen);
				offset = strResult.Find(backslash); // this is where the next endMkr is, in the shortened strResult
				emkr.Empty();
				emkrLen = 0;
			}
			else
			{
				// augEmkr did not belong to the inline Binding endmkr set, check red or blue fast-access strings
				offset2 = redEMkrs.Find(augEmkr);
				offset3 = blueEMkrs.Find(augEmkr);
				if (offset2 >= 0 || offset3 >= 0)
				{
					// It's an E endMkr
					if (offset2 >= 0)
					{
						// It's one of the Red or Blue endMkrs - its length applies to both
						strAccum << chE;
						// Get ready for next iteration
						strResult = strResult.Mid(emkrLen);
						offset = strResult.Find(backslash); // this is where the next endMkr is, in the shortened strResult
						emkr.Empty();
						emkrLen = 0;
					}
					else
					{
						// The only possibility is that it's one of the inline Nonbinding endMkrs. Check
						offset2 = nonBindingEMkrs.Find(augEmkr);
						wxASSERT(offset2 >= 0); // I hope I don't have to deal with wxNOT_FOUND (an unknown 
												// endMkr would be in m_endMarkers, and would not be found
												// in the above block, and trip the assert here
						if (offset2 >= 0)
						{
							strAccum << chN;
							// Get ready for next iteration
							strResult = strResult.Mid(emkrLen);
							offset = strResult.Find(backslash); // this is where the next endMkr is, in the shortened strResult
							emkr.Empty();
							emkrLen = 0;
						}
					} // end of else block for test: if (offset2 >= 0 || offset3 >= 0)
				}

			} // end of else block for test: if (offset2 >= 0) - testing for B
			wxLogDebug(_T("ConvertEndMkrs2BEN() line %d , strResult= [%s] , offset = %d , strAccum = [%s], sn = [%d]"),
							__LINE__, strResult.c_str(), offset, strAccum.c_str(), pSP->m_nSequNumber);
		}
		else
		{
			// strResult does not begin with an endMkr (a later iteration may have B at start, for instance)
			offset = strResult.Find(backslash);
			if (offset >= 0)
			{
				// There is another backslash to be handled on next iteration
				wxString strLeft = strResult.Left(offset); // grab everything up to where the endMkr is
				strAccum << strLeft; // accumulate what was grabbed
				// whm 13Jul2023 removed wxString declaration from strResult below otherwise the statement  
				// doesn't make sense and causes an exception/crash. strResult is declared as wxString above.
				//wxString strResult = strResult.Mid(offset); // shortened strResult has an endMkr at beginning
				strResult = strResult.Mid(offset); // shortened strResult has an endMkr at beginning
				// Get ready for next iteration
				strResult = strResult.Mid(emkrLen);
				offset = strResult.Find(backslash); // this is where the next endMkr is, in the shortened strResult
				emkr.Empty();
				emkrLen = 0;
				wxLogDebug(_T("ConvertEndMkrs2BEN() line %d , strResult= [%s] , offset = %d , strAccum = [%s]"),
					__LINE__, strResult.c_str(), offset, strAccum.c_str());

			}
			else
			{
				// There is no endMkr in what's left, so accumulate the remainder and exit the loop
				strAccum << strResult;
				offset = wxNOT_FOUND; // causes loop exit
			}
		}

	} // end of while loop
	wxLogDebug(_T("ConvertEndMkrs2BEN() line %d , strResult= [%s] , offset = %d , strAccum = [%s]"),
		__LINE__, strResult.c_str(), offset, strAccum.c_str());

	return strResult;
}

/// return      The recomposed end of the source text string, including punctuation and markers,
///             starting with m_key value, and exclude contents of m_filteredInfo_After 
/// pSingleSrcPhrase        ->  the non-merged sourcephrase, or a ~ conjoined pair
/// BEW created 8May2017 to build the source text from a single CSourcePhrase (because
/// merging across filtered information is prohibited, we only have to consider a 
/// singleton), which is undergoing unfiltering. We only build the post-word string,
/// which might be empty, or just punctuation, or just one or more inline endmarkers,
/// or a mix of punctuation and inline endmarkers - while ignoring the CSourcePhrase's
/// m_filteredInfo_After contents. The aim is to get everthing in place, except
/// any unfiltered information, and then pass the returned string to the caller to 
/// place the unfiltered (one kind or more) information in the correct places, using the 
/// metadata stored with the filtered info which is stored in m_filteredInfo_After.
/// The only function which uses this is the document's ReconstituteAfterFilteringChange(),
/// and it is based on code from within helpers.cpp FromSingleMakeSstr(), with some
/// tweaking
/// Created 18Apr17 When ParseWord2() is used, fixed space conjoining can be two or more
/// words, and we do not allow internal punctuation within word1~word2~word3 etc. So
/// we do not have to test for presence of ~, but just used m_key 'as is'
wxString BuildPostWordStringWithoutUnfiltering(CSourcePhrase* pSingleSrcPhrase, wxString& inlineNBMkrs)
{
	wxString Sstr;
	CSourcePhrase* pSP = pSingleSrcPhrase; // RHS is too long to type all the time
	wxString srcStr = pSP->m_key;   // start from this and append info to its end
									// & later put result in Sstr for returning
									// to the caller
	wxString endMarkersStr = pSP->GetEndMarkers(); // might be empty
	inlineNBMkrs.Empty();

	// First, any inline binding endmarkers, such as \it* for italics and/or \k* for a glossary keyword etc
	if (!pSP->GetInlineBindingEndMarkers().IsEmpty())
	{
		srcStr += pSP->GetInlineBindingEndMarkers();
	}
	// Punctuation comes next, from m_follPunct
	if (!pSP->m_follPunct.IsEmpty())
	{
		srcStr += pSP->m_follPunct;
	}
	// Use Sstr now - for no special reason other than I'm simplifying & tweaking a
	// legacy function (FromSingleMakeSstr()) for this special purpose
	Sstr = srcStr;

	// Any endmarkers (these are endmarkers for markers which are not in the
	// small set of inline non-binding endmarkers, such as \wj* wordsOfJesus endmarker)
	if (!endMarkersStr.IsEmpty())
	{
		Sstr << endMarkersStr;
	}
	// There might be content in m_follOuterPunct, append it next
	if (!pSP->GetFollowingOuterPunct().IsEmpty())
	{
		Sstr += pSP->GetFollowingOuterPunct();
	}
	// Finally, there could be an inline non-binding endmarker, like \wj* (words of Jesus)
	// Note: if unfiltering of material from a location which earlier was post-word, such
	// as \x ... content ...\x* is to be done, the caller will need to search for any 
	// inline non-binding endmarkers that get added here, and move them to the end of whatever
	// unfiltered material is restored to visibility in the source text - otherwise, something 
	// like \wj* might end up within the punctuation at the end of a word, rather than after it,
	// therefore we return any non-binding inline markers separately in order to make it easy
	// to move elsewhere if necessary
	if (!pSP->GetInlineNonbindingEndMarkers().IsEmpty())
	{
		inlineNBMkrs = pSP->GetInlineNonbindingEndMarkers();
	}

	// Remove unneeded spaces at either end
	Sstr.Trim(FALSE); // remove any initial whitespace - not likely to be any though
	Sstr.Trim();      // and don't return it with a final space, leave that to the caller
	return Sstr;
}

// the next 3 functions are similar or identical to member functions of the document class
// which are used in the parsing of text files to produce a document; one (ParseMarker())
// is a modification of the one in the doc class; these are needed here for use in
// CSourcePhrase - later we could replace IsWhiteSpace() and ParseWhiteSpace() in the doc
// class with these ( ? )
// 
// whm 18Aug2023 made this helpers.cpp IsWhiteSpace() conform to most recent version that
// was in the Doc - by adding explicit support for HairSpace here which was in the Doc's
// version but missing from here. Then, after ensuring this IsWhiteSpace() was an exact 
// clone of what was in the Doc's version, I modified the Doc's IsWhiteSpace() function  
// to simply call this version here in helpers.cpp which will be the place for any future
// modifications. Note: Since the (3rd) version of IsWhiteSpace() that is defined in the
// Xhtml.cpp file is also supposed to be "cloned from the one in CAdapt_ItDoc class so 
// as to consistent with the parsers in the rest of the app", I modified the same-named
// function in Xhtml.cpp to simply call this global function here in helpers.cpp.
// This function then is now called from all the places where the Doc's version and the
// Xhtml's version were previously being called from.

// BEW 23Apr15 added provisional support for Dennis Walters request for / as a
// like-whitespace wordbreak; only in Unicode version
bool IsWhiteSpace(const wxChar *pChar)
{
#ifdef _UNICODE
	wxChar NBSP = (wxChar)0x00A0; // standard Non-Breaking SPace
	wxChar HairSpace = (wxChar)0x200A; // used between curly quotes in MATBVM.SFM doc
#else
	wxChar NBSP = (unsigned char)0xA0;  // standard Non-Breaking SPace
#endif
	// handle common ones first...
	// returns true for tab 0x09, return 0x0D or space 0x20
	// _istspace not recognized by g++ under Linux; the wxIsspace() fn and those it relies
	// on return non-zero if a space type of character is passed in
	// whm 18Aug2023 Note: The remark above about "_istspace not being recognized by g++ 
	// under Linux" is not longer accurate/significant - wxIsspace() does return TRUE for
	// when *pChar  is _T('\n') - see testing results near end of OnInit() in the App, and
	// so I've removed the explicit test: || *pChar == _T('\n') from the if() test below.
	//if (wxIsspace(*pChar) != 0 || *pChar == NBSP || *pChar == _T('\n') || *pChar == HairSpace)
	if (wxIsspace(*pChar) != 0 || *pChar == NBSP || *pChar == HairSpace)
	{
		return TRUE;
	}
	else
	{
#ifdef _UNICODE
//#if defined(FWD_SLASH_DELIM)
		// BEW 23Apr15, support / as if a whitespace word-breaker
		if (gpApp->m_bFwdSlashDelimiter)
		{
			if (*pChar == _T('/'))
				return TRUE;
		}
//#endif
		// BEW 3Aug11, support ZWSP (zero-width space character, U+200B) as well, and from
		// Dennis Drescher's email of 3Aug11, also various others
		// BEW 4Aug11 changed the code to not test each individually, but just test if
		// wxChar value falls in the range 0x2000 to 0x200B - which is much quicker; and
		// treat U+2060 individually
		wxChar WJ = (wxChar)0x2060; // WJ is "Word Joiner"
		if (*pChar == WJ || ((UInt32)*pChar >= 0x2000 && (UInt32)*pChar <= 0x200B))
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
// BEW 24Oct14, no changes needed for support of USFM nested markers
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
// situations. The function can be used in other contexts of course. The int values can
// be from anything
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

// BEW created 11Sep11, to avoid adding duplicates of string already within
void AddUniqueString(wxArrayString* pArrayStr, wxString& str)
{
	int count = pArrayStr->GetCount();
	if (count == 0)
	{
		pArrayStr->Add(str);
	}
	else
	{
		int index;
		if (gbAutoCaps)
		{
			// case insensitive compare
			index = pArrayStr->Index(str, FALSE); // bCase is FALSE, so A and a
						// are the same character (wxWidgets comparison used)
		}
		else
		{
			//case sensitive (ie. case differentiates)
			index = pArrayStr->Index(str); // bCase is default TRUE, so A and a
						// are different characters (wxWidgets comparison used)
		}
		if (index == wxNOT_FOUND)
		{
			// it's not in there yet, so add it
			pArrayStr->Add(str);
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
// BEW 18Apr17 added support for the new member m_filteredInfo_After. HasFilteredInfo()
// is called in OnLButtonDown() (and maybe elsewhere) and without a test which checks
// for content in m_filteredInfo_After, a click on a wedge icon to view the filtered info
// does nothing if the only filtered info is in m_filteredInfo_After
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
//#if !defined(USE_ LEGACY_ PARSER)
	// BEW 1Nov22 unsure if this next bit is relevant to the legacy parser
	if (!pSrcPhrase->GetFilteredInfo_After().IsEmpty())
	{
		return TRUE;
	}
//#endif
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
// BEW
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
		if (pSrcPhrase->m_bNullSourcePhrase && !pSrcPhrase->m_bNotInKB)
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
// BEW 7Sep22 refactored to support glossing KB with m_adaption/gloss entries
bool IsFixedSpaceSymbolWithin(CSourcePhrase* pSrcPhrase)
{
	wxString theSymbol = _T("~"); // USFM fixedspace symbol
	if (pSrcPhrase == NULL)
	{
		return FALSE; // there isn't an instance
	}
	else
	{
		if (gbIsGlossing)
		{
			// When glossing, if m_adaption has a ~, it would be better to
			// remove it, because CSourcePhrase's tokenizer for tgt text
			// will count word1~word2 as two words 
			if (pSrcPhrase->m_adaption.Find(theSymbol) != wxNOT_FOUND)
			{
				pSrcPhrase->m_adaption.Replace(_T("~"), _T(" "), TRUE); // TRUE means 'replace all'
				return TRUE;
			}
		}
		else
		{
			// legacy adapting mode does not remove ~, but has code for dealing with
			// fixed space which I will retain
			if (pSrcPhrase->m_key.Find(theSymbol) != wxNOT_FOUND)
				return TRUE;
		}
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
//	GDLC 9Nov12 This is Bill's version using Uuid_AI
	Uuid_AI* pUuidGen = new Uuid_AI(); // generates the UUID
	anUuid = pUuidGen->GetUUID();

//	GDLC 9Nov12 This is my version using wxUUID
//	wxUUID* pUuidGen = new wxUUID(); // generates the UUID
//	anUuid = pUuidGen->GetUUID();

	if (pUuidGen != NULL) // whm 11Jun12 added NULL test
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
// BEW changed 22May12, to make the oxesDT and oxesDateOnly ones use . as the separator,
// and no T in between the data and time strings - as per Oxes 1.1.2 standard.
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
			dateTimeStr = theDateTime.Format(_T("%Y.%m.%d %H.%M.%S")).c_str();
		}
		break;
	case oxesDateOnly:
		{
			// I'm giving OXES local timezone's date, but this can be changed if the TE team want
			dateTimeStr = theDateTime.Format(_T("%Y.%m.%d")).c_str(); // chop off time spec
		}
		break;
	case forXHTML:
		{
			// for XHTML we'll use local date and time; with space between, use Paratext's separators
			dateTimeStr = theDateTime.Format(_T("%Y-%m-%d %H:%M:%S")).c_str();
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
	extn = _T("log"); // system log files GDLC 29Sep11
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
	wxASSERT(!fullName.IsEmpty()); // whm 11Jun12 added. GetChar(0) should not be called on an empty string
	if (!fullName.IsEmpty() && fullName.GetChar(0) == _T('.'))
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
			break;	// GDLC If this one matches, further testing is redundant
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
			break;	// GDLC If this one matches, further testing is redundant
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
			_T("Error"),wxICON_EXCLAMATION | wxOK);
		return FALSE;
	}
	//size_t len = GetFileSize_t(absPathToFile); // not needed
	size_t len = f.Length(); // it's legal to assign to size_t
//	GDLC 26Nov11 tellenc merely wants a char buffer
//	wxMemoryBuffer* pBuffer = new wxMemoryBuffer(len);	// GDLC + 2 removed because NULs are not needed
	char* pbyteBuff = (char*)malloc(len);
//	// Create acceptable pointers for calls below
//	char* ptr = (char*)pBuffer->GetData();
//	const unsigned char* const saved_ptr = (const unsigned char* const)pbyteBuff;
//	*(ptr + len) = '\0';	GDLC The NULs are not needed for tellenc
//	*(ptr + len + 1) = '\0';
	// get the file opened and read it into a memory buffer
	size_t numRead = f.Read(pbyteBuff,len);
	if (numRead < len)
	{
		// don't expect this error, so use English message
		wxMessageBox(_T("IsLoadableFile() read in less then all of the file. File will be treated as non-loadable."),
			_T("Error"),wxICON_EXCLAMATION | wxOK);
		if (pbyteBuff != NULL) // whm 11Jun12 added NULL test
			delete pbyteBuff;
		f.Close();
		return FALSE;
	}
	// now find out what the file's data is
	CBString resultStr;
//	resultStr.Empty(); GDLC We don't need this Empty operation
	// GDLC Removed conditionals for PPC Mac (with gcc4.0 they are no longer needed)
	// whm 20Sep11 Note: Since Bruce calls tellenc2() here with len, tellenc does not
	// include the two null bytes at the end of the buffer, and so the two added null bytes
	// serve no purpose. But, more importantly, it is fortunate that tellenc does not
	// include the null chars in its check, otherwise tellenc would return a result of
	// "binary" even for a non-binary file.
	//
	resultStr = tellenc2((const unsigned char *)pbyteBuff, len); // xml files are returned as "binary" too
										  // so hopefull html files without an extension
										  // will likewise be "binary" & so be rejected

	f.Close();
	// check it's not xml
	bool bIsXML = FALSE;
	char cstr[6] = {'\0','\0','\0','\0','\0','\0'};
	cstr[0] = *(pbyteBuff + 0);
	cstr[1] = *(pbyteBuff + 1);
	cstr[2] = *(pbyteBuff + 2);
	cstr[3] = *(pbyteBuff + 3);
	cstr[4] = *(pbyteBuff + 4);
	CBString aStr = cstr;
	if (aStr == "<?xml")
	{
		bIsXML = TRUE;
	}
	if (pbyteBuff != NULL) // whm 11Jun12 added NULL test
		delete pbyteBuff;

//	if (bIsXML || resultStr == "binary" || resultStr == "ucs-4" || resultStr == "ucs-4le")
// GDLC I agree with Bill that we can allow ucs-4 and usc4-le
	if (bIsXML || resultStr == "binary")
	{
		return FALSE;
	}
	// this means it's either utf-8, or utf-16 (big endian), or utf-16le (little endian)
	// or a single-byte encoding (or a multibyte one)
	return TRUE;
}

/*
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
//	resultStr.Empty(); GDLC We don't need this Empty operation
	resultStr = tellenc2(pCharBuf, size_in_bytes);
	if (resultStr == "utf-16" || resultStr == "ucs-4")
	{
		// theText is big-endian
		bIsLittleEndian = FALSE;
	}
#endif
	return bIsLittleEndian;
}
*/

bool IsLittleEndian(const unsigned char* const pCharBuf, unsigned int size_in_bytes)
{
	// wxChar is 2 bytes in Windows, 4 in Mac & Linux; we support non-Unicode app only for
	// Windows, which is low-endian
	bool bIsLittleEndian = TRUE;
#ifdef _UNICODE
	CBString resultStr;
//	resultStr.Empty(); GDLC We don't need this Empty() operation.
	resultStr =tellenc2(pCharBuf, size_in_bytes);
	if (resultStr == "utf-16" || resultStr == "ucs-4")
	{
		// theText is big-endian
		bIsLittleEndian = FALSE;
	}
#else
		size_in_bytes = size_in_bytes; // avoid compiler warning
		// do a little bit of garbage work (can't assign, as it is const)
		char ch[] = {'\0'};
		ch[0] = *pCharBuf; //avoids compiler warning
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
// GDLC Do we really want this wxString str???
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
				_T("Incredible data format error!"),wxICON_ERROR | wxOK);
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
// BEW 24Oct14, no changes needed for support of USFM nested markers (the markers involved
// are never nested ones, but only \free, \note, \bt and derivatives - it only called
// in ExportFunctions.cpp - the ApplyOutputFilterToText_For_Collaboration() function)
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
		wxASSERT(!code.IsEmpty());// whm 11Jun12 added. Should be the case (see first line of function above)
		aChar = code.GetChar(0);
		bIsAlphabetic = IsAnsiLetter(aChar);
		if (!bIsAlphabetic)
		{
			return FALSE;
		}
	}
	return TRUE;
}

// Use this to pass in a 2- or 3-letter ethnologue code, and get back its print name
// string, and inverted name string (internally, gets the file "iso639-3codes.txt" into a
// wxString buffer (the file has the 2-letter codes listed first, then the 3-letter ones,
// and looks up the code, and parses the required string from the rest of that line, and
// returns it in param 2. Return TRUE if no error, FALSE if something went wrong and the
// returned string isn't defined
bool GetLanguageCodePrintName(wxString code, wxString& printName)
{
	printName.Empty();
	wxString iso639_3CodesFileName = _T("iso639-3codes.txt");
	wxString pathToLangCodesFile;
	pathToLangCodesFile = gpApp->GetDefaultPathForXMLControlFiles();
	pathToLangCodesFile += gpApp->PathSeparator;
	pathToLangCodesFile += iso639_3CodesFileName;

	wxFFile f(pathToLangCodesFile); // default mode is "r" or read
	if (!(f.IsOpened() && f.Length() > 0))
	{
		// if not found, return FALSE and don't bother further
		return FALSE;
	}
	wxString* pTempStr;
	pTempStr = new wxString;
	bool bSuccessfulRead;
	bSuccessfulRead = f.ReadAll(pTempStr); // ReadAll doesn't require the file's Length
	if(!bSuccessfulRead)
	{
		// if error on reading data, return FALSE and don't bother further
		if (pTempStr != NULL) // whm 11Jun12 added NULL test
			delete pTempStr;
		return FALSE;
	}
	// whm Note: regardless of the platform, when reading a text file from disk into a
	// wxString, wxWidgets converts the line endings to a single \n char
	wxString searchStr = _T('\n'); // start with the \n character
	searchStr += code + _T("\t"); // add the code followed by tab, to ensure
								  // we don't get a spurious match
	searchStr.LowerCase();
	//int dataLen;
	//dataLen = pTempStr->Len(); // for a debug check only, offset should match at < dataLen
	int offset;
	offset = pTempStr->Find(searchStr);

	// temp for debugging, what's here at offset?
	//wxChar chars[100];
	//for (int ii = 0; ii < 100; ii++)
	//{
	//	chars[ii] = pTempStr->GetChar(offset + ii);
	//}
	if (offset == wxNOT_FOUND)
	{
		if (pTempStr != NULL) // whm 11Jun12 added NULL test
			delete pTempStr;
		return FALSE;
	}
	int len2 = searchStr.Len();
	offset += len2; // offset now points at the start of printName
	int pos = offset;
	wxString aTab = _T("\t");
	offset = FindFromPos(*pTempStr, aTab, pos + 1);
	wxASSERT(offset != wxNOT_FOUND && offset > pos);
	// the printName is the text between locations pos to offset
	wxString s(*pTempStr, pos, offset - pos);
	printName = s;
	if (pTempStr != NULL) // whm 11Jun12 added NULL test
		delete pTempStr;
	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
/// \return		void
/// \param		pBuf		<-> a wxChar buffer whose line endings must be converted to just LF
///	\param		bufLen		 -> length of the wxChar buffer
/// \remarks
/// Called only from: GetNewFile().
/// Converts CR or CR-LF line endings into just LF
///////////////////////////////////////////////////////////////////////////////
static void ConvertLineEndingsForGetNewFile(wxChar* pBuf, wxUint32& bufLen)
{
	// Scan the string converting line endings as necessary
#define	CR	'\r'
#define LF	'\n'
#define NUL	'\0'
	wxChar c;
	wxUint32 i, j;
	for (i=0, j=0; i<bufLen; i++)
	{
		c = pBuf[i];
		if (c == CR)
		{
			// Replace the CR by an LF
			pBuf[j++] = LF;
			// Check whether there is an LF to consume
			c = pBuf[i + 1];
			if (c == LF) ++i;
		}
		else if (c == NUL)
		{
			pBuf[j++] = c;
			break;
		}
		else pBuf[j++] = c;
	}

	// Return the reduced buffer length
	bufLen = j;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		void
/// \param		pTemp		<-> a wxString to be truncated to numwxChars
///	\param		numwxChars	->	the number of wxChars to be left in the string
/// \remarks
/// Called only from: GetNewFile().
/// Truncates the pTemp to numwxChars and then NUL terminates it.
///////////////////////////////////////////////////////////////////////////////
static void ShortenStringForGetNewFile(wxChar* pBuf, wxUint32& bufLen, wxUint32 numwxChars)
{
#define NUL	'\0'
	// If pBuf is longer than the desired number of wxChars then truncate it
	// and ensure that it is NUL terminated.
	if (bufLen > numwxChars)
	{
		bufLen = numwxChars+1;	// Allow for the NUL we will add
		pBuf[numwxChars] = NUL;
	} else
	{
		// If there is no need to truncate pTemp then do nothing, assuming that
		// the conversion process in DoInputConversion() has generated a NUL
		// terminated buffer of wxChars.
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		enum getNewFileState indicating success or error state when reading the
///             file.
/// \param		pstrBuffer	<- a wxString which receives the text file once loaded
/// \param		nLength		<- the length of the returned wxString in wxChars (not needed
///								and will be removed as other parts of the app are adjusted)
/// \param		pathName	-> path and name of the file to read into pstrBuffer
/// \param      numKBOnly   -> default is 0 The number of kilobytes that are wanted.
///                            This can be safely greater than the file's size.)
/// \remarks
/// Called from: the Doc's OnNewDocument(), and helper.cpp's PopulateTextCtrlWithChunk().
/// Opens and reads a standard format input file into a wxString pstrBuffer which
/// is used by the caller to tokenize and build the in-memory data structures used by the
/// View to present the data to the user. Note: the pstrBuffer wxString is null-terminated.
///
/// BEW 19July10, added 4th param, numKBOnly, with default value 0. If this param has the
/// value zero, the whole file is read in. If it has a non-zero value (number of kilobytes),
/// then extra code switches in as follows:
/// (a) the returned string will be limited to a number of wxChars roughly matching the
/// number of kB requested. If the whole file is shorter than the requested amount,
/// the whole file is shown,
/// (b) the returned string will have had all line terminators changed to wxChar LF
/// characters. (This guarantee of Unix line termination is what wxTextCtrl, for
/// multiline style, wants.) These manipulations are done after the text file (in
/// whatever coding it exists) has been converted to wxChars.
/// GDLC 2Dec11 Function completely rewritten using wxConvAuto
///////////////////////////////////////////////////////////////////////////////
enum getNewFileState GetNewFile(wxString*& pstrBuffer, wxUint32& nLength,
								wxString pathName, int numKBOnly)
{
// GDLC TESTING ONLY - REMOVE FROM RELEASE APP
//	numKBOnly = 1;
	// get a CFile and check length of file
	wxFile file;
	if (!file.Open(pathName, wxFile::read))
	{
		return getNewFile_error_at_open;
	}

	// file is now open, so find its logical length (always in bytes)
	wxUint32 numBytesInFile = file.Length();

	// GDLC Calc and keep the actual length of the byte buffer (which is NOT
	// necessarily the same as the length of the resulting wxString)
	// But at this stage we don't know whether the file is ASCII, ANSI, UTF8,
	// UTF16 or UTF32, so we will put a 4 byte NUL after the last byte of the file.
    wxUint32 nBuffLen = numBytesInFile + 4;

	// Get a byte buffer and read the file's data into it then close the file.
	char* pbyteBuff = (char*)malloc(nBuffLen);
	wxUint32 nNumRead = (wxUint32)file.Read(pbyteBuff, numBytesInFile);
	file.Close();

	// GDLC 2Dec11 Null terminate the byte buffer to ensure that DoInputConversion()
	// produces a null terminated wxString.
	pbyteBuff[nNumRead] = '\0';
	pbyteBuff[nNumRead+1] = '\0';
	pbyteBuff[nNumRead+2] = '\0';
	pbyteBuff[nNumRead+3] = '\0';

	if (gbForceUTF8) gpApp->m_srcEncoding = wxFONTENCODING_UTF8;
	else
	{
	// Use tellenc() to find the encoding of the text file. Give tellenc the
	// number of bytes read not including the terminating NUL because tellenc()
	// does not want that.
	init_utf8_char_table();
	const char* enc = tellenc(pbyteBuff, numBytesInFile);
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
		gpApp->m_srcEncoding = wxFONTENCODING_UTF16BE;
	}
	else if (strcmp(enc, "utf-16le") == 0)
	{
		gpApp->m_srcEncoding = wxFONTENCODING_UTF16LE;
	}
	else if (strcmp(enc, "ucs-4") == 0)
	{
		gpApp->m_srcEncoding = wxFONTENCODING_UTF32BE;
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

	// Use DoInputConversion() to convert the text file from its encoding into a wxChar buffer
	wxChar* pBuf;		// DoInputConversion() will allocate the buffer after it calculates
	wxUint32 bufLen;	// the needed size which depends on the encoding of the text file.
	gpApp->DoInputConversion(pBuf, bufLen, pbyteBuff, gpApp->m_srcEncoding, nBuffLen);

	// Free the byte buffer read from the file
	free((void*)pbyteBuff);

	// Convert CR and CRLF line terminations to just LF
	ConvertLineEndingsForGetNewFile(pBuf, bufLen);

	// Limit the size of the wxString if numKBOnly is non-zero
	if (numKBOnly > 0)
	{
		// Calculate the target number of wxChars that should be returned in the wxString
		// Target number of file KB * 1024 multiplied by the ratio of
		// number of wxChars in the wxString to the number of bytes in the original file
		//
		// The number of wxChars resulting from converting the file will vary from 1 wxChar
		// per byte for ASCII files down to about 1 wxChar per 4 bytes for UTF32 files.
		//
		// Note: we are talking about the number of wxChars, NOT the number of bytes in
		// the wxString (which will vary with the build - 1 ANSI, 2 Win, 4 Mac/Lin).
		//
		// Note: by processing all three multiplicands before dividing by numBytesInFile we can
		// avoid conversions to and from floating point to cope with the less than 1.0 ratio.
		wxUint32 numwxChars = (numKBOnly * 1024 * bufLen)/numBytesInFile;

		// Truncate the wxString to the desired number of wxChars
		ShortenStringForGetNewFile(pBuf, bufLen, numwxChars);
	}

	// Return the wxString and its length to the caller.
	*pstrBuffer = wxString(pBuf, bufLen);
	nLength = bufLen;
	if (pBuf != NULL) // whm 11Jun12 added NULL test
		delete pBuf; // don't leak memory
	return getNewFile_success;
}

/*
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
	wxUint32 numBytesInFile = file.Length();
	nLength = numBytesInFile;	// confused thinking retained because of the ANSI section

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
	// GDLC TODO: Investigate dealing with line endings AFTER the input text has been
	// converted into a string of wxChars - it looks like it would be a lot simpler.

	bool bIsLittleEndian;
	bIsLittleEndian = TRUE; // this is valid for ANSI build, on non-Win platforms
								 // we only support Unicode
#ifndef _UNICODE // ANSI version, no unicode support

	// create the required buffer and then read in the file (no conversions needed)
	// BEW changed 8Apr06; use malloc to remove the limitation of the finite stack size
	char* pBuf = malloc(nLength + 1); // allow for terminating null byte
	char* pBegin_COPY = NULL;
	memset(pBuf,0,nLength + 1);	// zero the extra byte for the NUL as well
	wxUint32 numRead = file.Read(pBuf,(wxUint32)nLength);
	nLength = numRead; // in case we didn't get all of it, use what we got
//	GDLC Nov11
	originalBuffLen = nLength + 1; // save it in case we need it below
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
//	GDLC Nov11
	const char* enc = tellenc(pBuf, numRead); // don't include null char at buffer end
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
//	GDLC Nov11
	if (pstrBuffer != NULL) delete pstrBuffer;	// Avoid memory leaks
	pstrBuffer = new wxString(pBuf, nLength);	// GDLC 2Nov11 Modified to ensure safe return of the wxString
//	wxString* pwxBuf = new wxString(pBuf, nLength);	// GDLC 2Nov11 Modified to ensure safe return of the wxString
//	*pstrBuffer = pwxBuf;
//	*pstrBuffer = pBuf; // copy to the caller's CString (on the heap) before malloc
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

// GDLC Calc and keep the actual length of the byte buffer (which is NOT the same as the
// length of the resulting wxString)
    wxUint32 nBuffLen = numBytesInFile + 1; // allow for the NUL byte we will add

	// get a byte buffer, initialize it to all null bytes, then read the file's data into it
	char* pbyteBuff = (char*)malloc(nBuffLen);
	char* pBegin_COPY = NULL;
//	GDLC Nov11 We shouldn't need to initialise it to NUL bytes, just put a NUL at its end
//	memset(pbyteBuff, 0, nBuffLen); // fill with nulls
	originalBuffLen = nBuffLen; // save it in case we need it below
	nNumRead = (wxUint32)file.Read(pbyteBuff, numBytesInFile);
	pbyteBuff[nNumRead] = '\0';
//	GDLC Nov11 No need to add NUL bytes
//	nLength = nNumRead + sizeof(wxChar);

	// BEW added 16Aug11, determine the endian value for the string we have just read in
	// GDLC Nov11 Conversion to a wxString is no longer necessary and this did not do it anyway.
	// wxString theBuf = wxString((wxChar*)pbyteBuff);
	// BEW 5Dec11, this fails for a short text which is "\id JHN xxxxx" where xxxxx is the
	// utf8 characters for an exotic script language (Kangri, in India). Casting doesn't
	// do the required conversions, I'll comment it out and replace with what I know works
	// and leave it to Graeme to change later if necessary
	//wxString theBuf = wxString((wxChar*)pbyteBuff);
	wxString theBuf;
//#if defined(_UNICODE)
//	CBString cbs(pbyteBuff);
//	theBuf = gpApp->Convert8to16(cbs);
//#else
//	theBuf = pbyteBuff;
//#endif
//	bIsLittleEndian = IsLittleEndian(theBuf);
//	GDLC 2Dec11 Don't give tellenc the NUL that we added at the end of the buffer
	bIsLittleEndian = IsLittleEndian((const unsigned char*) pbyteBuff, nNumRead);

	if (bShorten)
	{
		// endian value complicates things, so since the code below is for little-endian
		// strings, leave it except for any needed utf16 tweaks to be added, and have a
		// separate block of similar code for big-endian strings
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
			// it's a big-endian string....

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
	// encoding and we convert to wxChar in the DoInputConversion() function
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
//	GDLC Nov11 Use explicit LE because any system can have files in either type of encoding
//		gpApp->m_srcEncoding = wxFONTENCODING_UTF16;
		gpApp->m_srcEncoding = wxFONTENCODING_UTF16LE;
		bHasBOM = TRUE;
	}
	else if (!bIsLittleEndian && !memcmp(pbyteBuff,szU16BOM_BigEndian,nU16BOMLen))
	{
		// it's UTF-16 - big-endian
//	GDLC Nov11 Use explicit BE because any system can have files in either type of encoding
//		gpApp->m_srcEncoding = wxFONTENCODING_UTF16;
		gpApp->m_srcEncoding = wxFONTENCODING_UTF16BE;
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
// GDLC 2Dec11 Give tellenc the number of bytes read and do not include the terminating NUL
				const char* enc = tellenc(pbyteBuff, nNumRead);
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
					gpApp->m_srcEncoding = wxFONTENCODING_UTF16BE;
				}
				else if (strcmp(enc, "utf-16le") == 0)
				{
					// GDLC Nov11 I think the commentary on the next two lines is wrong.
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

	// GDLC Nov11 The following earlier commentary about UTF-16 may have been correct when this
	// was first developed on Windows, but on Mac and Linux the UTF16 chars read from the disk file
	// get converted into UTF32LE. DoInputConversion now does the whole job of input conversion and
	// removal of a BOM if the input has one.
	// do the converting and transfer the converted data to pstrBuffer (which then
	// persists while doc lives) -- if m_srcEndoding is wxFONTENCODING_UTF16, then
	// conversion is skipped and the text (minus the BOM if present) is returned 'as is'
	// GDLC 16Sep11 Last parameter bHasBOM no longer needed
	//	gpApp->DoInputConversion(*pstrBuffer,pbyteBuff,gpApp->m_srcEncoding,bHasBOM);
	wxString*	pTemp;
	gpApp->DoInputConversion(pTemp, pbyteBuff, gpApp->m_srcEncoding, nNumRead + 1);
	pstrBuffer = pTemp;

// GDLC Nov11 No, we don't need to update the length of the text file that was read, and
// why would the caller need to know anyway? - because it has been converted into a wxString
// which has its own length and from now on AI only needs to know about the length of the
// wxString - the length of the text file in whatever encoding it was in has become irrelevant!
// In fact we are now going to delete the byte buffer containing a copy of what was in the file!
//	nLength = pstrBuffer->Len() + 1; // # of UTF16 characters + null character
// (2 bytes)
	// free the original read in (const) char data's chunk
	free((void*)pbyteBuff);

#endif
	file.Close();
	return getNewFile_success;
}
*/

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
	count = count; // avoid warning
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
	pList->DeleteNode(pos); // removes the Node* from the list
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
// BEW 24Oct14, changed to support USFM nested markers
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
		// BEW 24Oct10 added second subtest to support USFM nested markers
		if (IsAnsiLetterOrDigit(*pCh) || (*pCh == _T('+')))
		{
			// back over it
			pCh = pCh - 1;
		}
		else if (*pCh == _T('\\') || *pCh == _T('\n') || *pCh == _T('\r')) // BEW 30Aug23 added '\r' subtest, for Mac support
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

//#define ShowConversionItems

void ConvertSPList2SPArray(SPList* pList, SPArray* pArray)
{
	SPList::Node* pos= pList->GetFirst();
	while (pos != NULL)
	{
		CSourcePhrase* pSrcPhrase = pos->GetData();
		pos = pos->GetNext();
		pArray->Add(pSrcPhrase);
#if defined(ShowConversionItems) && defined(_DEBUG)
		wxLogDebug(_T("from SPList:  m_chapterVerse: %s   sequNum: %d   m_srcPhrase: %s"),
			pSrcPhrase->m_chapterVerse.c_str(), pSrcPhrase->m_nSequNumber, pSrcPhrase->m_srcPhrase.c_str());
#endif
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
		wxMessageBox(msg, _T("Index out of bounds"),wxICON_ERROR | wxOK);
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
//#ifdef _DEBUG

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
            gpApp->m_bInhibitMakeTargetStringCall = TRUE;
			if (gbIsGlossing)
			{
				// whm 19Sep11 added test for gpApp->m_pGlossingKB being NULL
				if (bAttemptStoreToKB && gpApp->m_pGlossingKB != NULL)
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
					// whm 19Sep11 added test for gpApp->m_pKB being NULL
					if (bAttemptStoreToKB && gpApp->m_pKB != NULL)
					{
						bOK = gpApp->m_pKB->StoreText(pActiveSrcPhrase, gpApp->m_targetPhrase);
					}
				}
			}
            gpApp->m_bInhibitMakeTargetStringCall = FALSE;
			if (!bOK)
			{
				// something is wrong if the store did not work, but we can tolerate the error
				// & continue
				if (!bSuppressWarningOnStoreKBFailure)
				{
                    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                    gpApp->m_bUserDlgOrMessageRequested = TRUE;
                    wxMessageBox(_(
"Warning: the word or phrase was not stored in the knowledge base. This error is not destructive and can be ignored."),
				    _T(""),wxICON_EXCLAMATION | wxOK);
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

// BEW created 1Sept11, use the following for getting the pixel difference for a control's
// label text which starts off with one or more %s specifies, and those are filled out to
// form newLabel; pass in the control in the pWindow param, and internally the function
// will get the label's font, set up a wxWindowDC, measure the two strings, and pass back
// the difference in their widths.
// NOTE: oldLabel and newLabel must be single-line strings, it won't work if they are not
int CalcLabelWidthDifference(wxString& oldLabel, wxString& newLabel, wxWindow* pWindow)
{
#ifdef _DEBUG
	int offset1 = -1;
	int offset2 = -1;
	offset1 = oldLabel.Find(_T('\n'));
	offset2 = newLabel.Find(_T('\r'));
	wxASSERT(offset1 == wxNOT_FOUND && offset2 == wxNOT_FOUND);
#endif
	wxFont labelFont = pWindow->GetFont();
	wxWindowDC dc(pWindow); // associate dialog window with the device context
	dc.SetFont(labelFont);
	wxSize strOriginalExtent = dc.GetTextExtent(oldLabel);
	wxSize strFinalExtent = dc.GetTextExtent(newLabel);
	int difference = strFinalExtent.GetX() - strOriginalExtent.GetX();
	return difference;
}

// BEW created 17Jan11, used when converting back from docV5 to docV4 (only need this for
// 5.2.4, but it could be put into the Save As... code for 6.x.y too - if so, we can retain
// it for version 6 and higher)
// BEW 24Oct14, no changes needed for support of USFM nested markers
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

// BEW created 16Sep11, for putting in InitDialog() so as to move it towards a corner of
// screen away from where phrase box currently is when the dialog is put up
// x       -> (left) always passed in as 0 (it's device coords for dlg left, dlg is the device)
// y       -> (top) always passed in as 0 (it's device coords for dlg top, dlg is the device)
// w       -> (width of dlg) caller calculates this, in pixels
// h       -> (height of dlg) caller calculates this, in pixels
// XPos    -> (left of dlg) in screen coords (beware dual monitors give a huge value if
//             dlg is on the right monitor) calculated in the caller
// YPos    -> (top of dlg) in screen coords, calculated in the caller
// myTopCoord   <-  top, in screen coords, where the top of the dialog is to be placed
//                  (a SetSize() call will do the job, after this function returns)
// myLeftCoord  <-  left, in screen coords, where the left of the dialog is to be placed
//                  (a SetSize() call will do the job, after this function returns)
// whm 10Apr2019 modified to keep the dialog confined to the same monitor that the
// application is running/displaying on. We employ a xOffsetToSecondaryMonitor to
// ensure the dialog stays with the running/displaying app on the same monitor, and
// we get a rectScreen specific to that monitor.
void RepositionDialogToUncoverPhraseBox(CAdapt_ItApp* pApp, int x, int y, int w, int h,
				int XPos, int YPos, int& myTopCoord, int& myLeftCoord)
{
	// work out where to place the dialog window
	CLayout* pLayout = pApp->GetLayout();
	int m_nTwoLineDepth = 2 * pLayout->GetTgtTextHeight();

	//int displayWidth, displayHeight;
	//::wxDisplaySize(&displayWidth,&displayHeight); // returns 1920 by 1080 on my XPS
    //machine even when AI window is on left (smaller) monitor

	wxRect rectScreen;
    // whm 9Apr2019 modified. Determine which monitor the app's main frame is displaying
    // on and get its client area rectScreen.
    unsigned int mainFrmDisplayIndex = 0; // index 0 is the primary monitor, 1 and higher are secondary monitors
    // Detect which display index our AI main frame is displaying on
    mainFrmDisplayIndex = wxDisplay::GetFromWindow(pApp->GetMainFrame());

    unsigned int numMonitors;
    numMonitors = wxDisplay::GetCount();
    // whm 11Apr2019 added the following test to check whether mainFrmDisplayIndex is
    // within the range of detected monitors. If it is a bad value it will generate a crash.
    // We can avoid the crash by checking for a bad mainFrmDisplayIndex value, and if so,
    // we just set some reasonable myTopCoord and myLeftCoord values that would center the
    // dialog on the primary monitor.
    if (mainFrmDisplayIndex == wxNOT_FOUND || mainFrmDisplayIndex < 0 || mainFrmDisplayIndex >(numMonitors - 1))
    {
        // Couldn't determine a valid myMonitor value, so just put the dialog
        // centered on the primary monitor (index 0).
        unsigned int primaryDisplay = 0;
        wxDisplay thisDisplay(primaryDisplay);
        rectScreen = thisDisplay.GetClientArea();
        // Now get the dialog metrics - width and height
        wxRect rectDlg(x, y, w, h);
        rectDlg = NormalizeRect(rectDlg); // in case we ever change from MM_TEXT mode // use our own
        int dlgHeight = rectDlg.GetHeight();
        int dlgWidth = rectDlg.GetWidth();
        wxASSERT(dlgHeight > 0);
        int halfDisplayH, halfDisplayW, halfDlgH, halsDlgW;
        halfDisplayH = rectScreen.GetHeight() / 2;
        halfDisplayW = rectScreen.GetWidth() / 2;
        halfDlgH = dlgHeight / 2;
        halsDlgW = dlgWidth / 2;
        myTopCoord = y + halfDisplayH - halfDlgH;
        myLeftCoord = x + halfDisplayW - halsDlgW;
        wxLogDebug("MainFrm NOT VISIBLE on any monitor - placing dialog centered on primary monitor!");
        return;
    }
    wxASSERT(mainFrmDisplayIndex != wxNOT_FOUND); // if wxNOT_FOUND value -1 is returned the main frame is not displaying on any monitor
    // whm 9Apr2019 removed call of wxGetClientDisplayRect() below as it only gets
    // the primary display's screen, whereas the thisDisplay.GetClientArea() call
    // replacing it, gets the rectScreen of the actual monitor the app is displaying on.
	//rectScreen = wxGetClientDisplayRect(); // a global wx function
    //
    // Create an instance of wxDisplay for thisDisplay and get its rectScreen (may
    // be primary or a secondary display)
    wxDisplay thisDisplay(mainFrmDisplayIndex);
    rectScreen = thisDisplay.GetClientArea();
    // whm Note: More adjustments are needed below to keep the dialog's horizontal
    // positioning on the same monitor that the main frame's window is displaying on.
	int stripheight = m_nTwoLineDepth;
	wxRect rectDlg(x,y,w,h);
	rectDlg = NormalizeRect(rectDlg); // in case we ever change from MM_TEXT mode // use our own
	int dlgHeight = rectDlg.GetHeight();
	int dlgWidth = rectDlg.GetWidth();
	wxASSERT(dlgHeight > 0);
	// BEW 16Sep11, new position calcs needed, the dialog often sits on top of the phrase
	// box - better to try place it above the phrase box, and right shifted, to maximize
	// the viewing area for the layout; or if a low position is required, at bottom right
	int phraseBoxHeight;
	int phraseBoxWidth;
	pApp->m_pTargetBox->GetTextCtrl()->GetSize(&phraseBoxWidth,&phraseBoxHeight); // it's the width we want // whm 12Jul2018 added GetTextCtrl()-> part
    // whm Note: The top-left corner of the phrasebox YPos and XPos, is calculated in the caller
    // via the CalcScrolledPosition() and ClientToScreen() calls. They are in screen pixels
    // based on the whole extended display from the 0,0 point on the primary monitor.
    int xOffsetToSecondaryMonitor = 0; // default to no offset for single monitor calcs
    // Calculate an xOffsetToSecondaryMonitor if thisDisplay is not the primary display.
    if (!thisDisplay.IsPrimary())
    {
        // AI and its dialog is displaying on a secondary monitor, so get an xOffsetToSecondaryMonitor
        // value for use in myLeftCoord below.
        // The primary monitor is always the one having index 0, so get the primary monitor's
        // metrics.
        unsigned int primaryDisplayIndex = 0;
        wxDisplay primaryDisplay(primaryDisplayIndex);
        wxRect rectScreenPrimary = primaryDisplay.GetClientArea();
        xOffsetToSecondaryMonitor = rectScreenPrimary.GetWidth();
    }
	int pixelsAvailableAtTop = YPos - stripheight; // remember box is in line 2 of strip
	int pixelsAvailableAtBottom = rectScreen.GetBottom() - stripheight - pixelsAvailableAtTop - 20; // 20 for status bar
	int pixelsAvailableAtLeft = XPos - 10; // -10 to clear away from the phrase box a little bit
	int pixelsAvailableAtRight = rectScreen.GetWidth() - phraseBoxWidth - XPos;
	bool bAtTopIsBetter = pixelsAvailableAtTop > pixelsAvailableAtBottom;
	bool bAtRightIsBetter = pixelsAvailableAtRight > pixelsAvailableAtLeft;
	if (bAtTopIsBetter)
	{
		if (dlgHeight + 2*stripheight < pixelsAvailableAtTop)
			myTopCoord = pixelsAvailableAtTop - (dlgHeight + 2*stripheight);
		else
		{
			if (dlgHeight > rectScreen.GetBottom())
			{
				//cut off top of dialog in preference to the bottom, where it's buttons are
				myTopCoord = rectScreen.GetBottom() - dlgHeight + 6;
				if (myTopCoord > 0)
					myTopCoord = 0;
			}
			else
				myTopCoord = 0;
		}
	}
	else
	{
		if (YPos + stripheight + dlgHeight < rectScreen.GetBottom())
			myTopCoord = YPos + stripheight;
		else
		{
			myTopCoord = rectScreen.GetBottom() - dlgHeight - 20;
			if (myTopCoord < 0)
				myTopCoord = myTopCoord + 20; // if we have to cut off any, cut off the dialog's top
		}
	}
    // whm 10Apr2019 added the xOffsetToSecondaryMonitor to the myLeftCoord below. When the app is
    // displaying on the primary monitor xOffsetToSecondaryMonitor is 0, but when the app is
    // displaying on a secondary monitor xOffsetToSecondaryMonitor is the width of the primary
    // monitor in pixels. This should work whether the secondary monitor is expanding the desktop
    // to the right or to the left (with negative x offset) of the primary monitor.
	if (bAtRightIsBetter)
	{
		myLeftCoord = rectScreen.GetWidth() - dlgWidth + xOffsetToSecondaryMonitor;
	}
	else
	{
        myLeftCoord = xOffsetToSecondaryMonitor; // was 0;
	}
}

// BEW created 12Dec13, for putting in InitDialog() so as to move it towards a corner of
// screen away from where phrase box currently is when the dialog is put up
// x       -> (left) always passed in as 0 (it's device coords for dlg left, dlg is the device)
// y       -> (top) always passed in as 0 (it's device coords for dlg top, dlg is the device)
// w       -> (width of dlg) caller calculates this, in pixels
// h       -> (height of dlg) caller calculates this, in pixels
// XPos    -> (left of dlg) in screen coords (beware dual monitors give a huge value if
//             dlg is on the right monitor) calculated in the caller
// YPos    -> (top of dlg) in screen coords, calculated in the caller
// myTopCoord   <-  top, in screen coords, where the top of the dialog is to be placed
//                  (a SetSize() call will do the job, after this function returns)
// myLeftCoord  <-  left, in screen coords, where the left of the dialog is to be placed
//                  (a SetSize() call will do the job, after this function returns)
// Internally, supports multiple monitors, so is much smarter than the original of the
// same name but without the "_Version2" in the name - see above
void RepositionDialogToUncoverPhraseBox_Version2(CAdapt_ItApp* pApp, int x, int y, int w, int h,
				int XPos, int YPos, int& myTopCoord, int& myLeftCoord)
{
	// Monitor discovery and metrics. WX has a class for this: wxDisplay
	wxArrayPtrVoid arrDisplays; // to store wxDisplay* object pointers, one for each monitor
	wxArrayPtrVoid arrAreas; // to store wxRect* object pointers, the client display area for each monitor
    unsigned int numMonitors;
    numMonitors = wxDisplay::GetCount();
	unsigned int monIndex;
	wxDisplay* pMonitor = NULL;
	wxRect* pArea = NULL;
	for (monIndex = 0; monIndex < numMonitors; monIndex++)
	{
		pMonitor = new wxDisplay(monIndex); // the monitor with index 0 will always be the
				// system's primary monitor - usually represented as a 1 in graphical displays
		arrDisplays.Add(pMonitor);
		pArea = new wxRect;
		arrAreas.Add(pArea);
		*pArea = pMonitor->GetClientArea();
#if defined (_DEBUG)
		wxLogDebug(_T("RepositionDialog...(): Monitor: index = %d    Area: x = %d,  y = %d, width = %d, height = %d"),
			monIndex, pArea->x, pArea->y, pArea->width, pArea->height);

#endif
	}
	// We prefer to display the dialog centered above the AI frame window, if possible -
	// up to 90 pixels of overlap is allowable at the top; or centered below the frame as
	// second choice - up to 30 pixels of overlap is allowable. If those options are not
	// possible, then we choose whichever corner of the monitor the phrase box is on (we
	// can safely assume the whole AI frame is on the same monitor) is the furthest corner
	// from the phrase box - so the options are top_left, top_right, bottom_left, or
	// bottom_right - but not necessarily in that order. There is usually more unused
	// client space on the right of the AI canvas, so we prefer rightwards placement
	// usually, but leftwards when the phrasebox is towards the right of the client area -
	// and when on the left, usually at the top corner is better because the various bars
	// of the frame are being temporarily obscured - which is better than obscuring strips
	enum DisplayConstraint {
		centered_above,
		centered_below,
		top_right,
		bottom_right,
		top_left,
		bottom_left
	};
	// Note, the system lines up the bases of monitors. So, for example, my primary
	// monitor is a 1920 wide by 1080 pixels deep one. My secondary is a smaller monitor,
	// it's 1680 wide by 1050 deep. A difference of 30 pixels in height. The primary
	// monitor defines the coordinate system. So here's the area rectangles returned in
	// the above loop:
	// Primary: {0,0,1920,1040} Note, the 40 pixel task bar is not included, it is not an
	// area which is drawable on
	// Secondary: {-1680,30,1680,1050} Note, x is negative, my secondary is on the left of
	// the primary monitor, and y is 30, because the system has lined up the bottoms of
	// each monitor, so the secondary's top is 30 pixels lower. These facts define how we
	// do our arithmetic below.

	// Now get the dialog metrics - width and height
	wxRect rectDlg(x,y,w,h);
	rectDlg = NormalizeRect(rectDlg); // in case we ever change from MM_TEXT mode // use our own
	int dlgHeight = rectDlg.GetHeight();
	int dlgWidth = rectDlg.GetWidth();
	wxASSERT(dlgHeight > 0);

	// Get the strip height
	CLayout* pLayout = pApp->GetLayout();
	int m_nTwoLineDepth = 2 * pLayout->GetTgtTextHeight();
	int stripheight = m_nTwoLineDepth;

    // Next get the Phrase box size and position metrics
	int phraseBoxHeight;
	int phraseBoxWidth;
	pApp->m_pTargetBox->GetTextCtrl()->GetSize(&phraseBoxWidth,&phraseBoxHeight); // it's the width we want // whm 12Jul2018 added GetTextCtrl()-> part

	// We need to know where the frame rectangle is, in screen coordinates, & width & height
	CMainFrame* pFrame = pApp->GetMainFrame();
	int frameWidth, frameHeight;
	pFrame->GetSize(&frameWidth, &frameHeight); // get AI's window frame width and height
	int frame_xCoord, frame_yCoord; // for location of it's topLeft
	pFrame->GetScreenPosition(&frame_xCoord, &frame_yCoord);

	// Which monitor is the phrasebox on? People don't work with the canvas spanning two
	// monitors, so we can safely assume where the phrasebox is, is also where the AI
	// frame window is. Once we know the monitor, we can get the display metrics from the
	// arrDisplays array.
	wxPoint phraseBoxTopLeft;
	phraseBoxTopLeft.x = XPos; // the 'left' value, in screen coords
	phraseBoxTopLeft.y = YPos; // the 'top' value, in screen coords
	// From this point we can get the monitor the phrasebox is displaying on
    //
    // whm 11Apr2019 modified. The detection of myMonitor should not be done
    // with the wxDisplay::GetFromPoint() function, because if the phrasebox
    // is scrolled up or down and not visible - beyond the virtual boundary of
    // the monitor, or if the main frame is moved down so the phrasebox is not
    // in view but below the boundary of the monitor, GetFromPoint() won't find it!
    // It is better to attempt detection using the GetFromWindow(pApp->GetMainFrame())
    // function, so I've changed the call below to use GetFromWindow().
    // Testing shows that even with the phrasebox scrolled far above or below the
    // virtual boundary of the monitor, or even when nearly all AI's main frame is
    // moved off the monitor's screen, the GetFromWindow() call will still succeed.
	//unsigned int myMonitor = wxDisplay::GetFromPoint(phraseBoxTopLeft);
    unsigned int myMonitor = wxDisplay::GetFromWindow(pApp->GetMainFrame());
    // whm 11Apr2019 added the following test to check whether myMonitor is within the
    // range of detected monitors. If the user were to position AI's main frame down
    // so that the phraseBoxTopLeft point is not visible on the monitor, then the
    // GetFromPoint(phraseBoxTopLeft) call above will fail and generate a crash.
    // We can avoid the crash by checking for a bad myMonitor value, and if so, we just
    // set some reasonable myTopCoord and myLeftCoord values that would center the
    // dialog on the primary monitor.
    if (myMonitor == wxNOT_FOUND || myMonitor < 0 || myMonitor >(numMonitors - 1))
    {
        // Couldn't determine a valid myMonitor value, so just put the dialog
        // centered on the primary monitor.
        pArea = (wxRect*)arrAreas.Item(0); // get pArea of primary monitor (0)
        int halfDisplayH, halfDisplayW, halfDlgH, halsDlgW;
        halfDisplayH = pArea->GetHeight() / 2;
        halfDisplayW = pArea->GetWidth() / 2;
        halfDlgH = dlgHeight / 2;
        halsDlgW = dlgWidth / 2;
        myTopCoord = y + halfDisplayH - halfDlgH;
        myLeftCoord = x + halfDisplayW - halsDlgW;
        wxLogDebug("PhraseBox NOT VISIBLE on any monitor - placing dialog centered on primary monitor!");
        return;
    }
    //
	// Get the rectangle which is the client area (i.e. where our windows are able to be
	// drawn) on myMonitor, and from it work out the displayWidth and displayHeight
	// (maximums) for that monitor
	wxASSERT(arrAreas.GetCount() >= 1);
	pArea = (wxRect*)arrAreas.Item(myMonitor);
	int displayHeight; // don't need   int displayWidth;
	//displayWidth = pArea->GetWidth(); // don't need
	displayHeight = pArea->GetHeight();
	int heightDiff; // between the two monitors - myMonitor versus primary one,
					// +ve means myMonitor is less tall than primary one, -ve means its higher
	heightDiff = pArea->y; // this is a compensatory factor affecting placement of the
						   // dialog below the AI frame window's bottom; we require it
						   // because the system aligns the screens at their bottom
#if defined(_DEBUG)
        wxLogDebug(_T("\nReposition dlg: displayHeight %d     heightDiff %d"), displayHeight, heightDiff);
#endif
	// From our various metrics, get the set of metrics which enable us to choose the
	// appropriate DisplayConstraint enum value. For horizontal metrics (left or right
	// distances) we want to know distance from start or end of the phrasebox to the
	// monitor's left or right edge. For vertical metrics, we want to know if there is
	// more space above or below the active strip - allowing an extra strip of space so
    // that the box doesn't encroach on needed visual context; but we also want to know
    // what the distance is from the top of the frame to the top of the screen, and from
    // the bottom of the phrase to the bottom of the screen - the first two enums need the
    // latter two
	int pixelsAvailableAbovePhraseBox = YPos - stripheight;
	int pixelsAvailableBelowPhraseBox = displayHeight - (YPos + 2 * stripheight);
	// For the left calculation, distance to left edge of monitor, minus location of left
	// edge of phrase box, minus an extra 10 pixels to keep the dialog way from the phrase
	// box a little bit, if phrase box is to the left of the origin, if to the right, then
	// distance to left of phrasebox minus distance to monitor's left edge, less 10 for
	// same reason as above
	int pixelsAvailableAtLeft; // we calculate a positive value
	if (XPos < 0)
	{
		pixelsAvailableAtLeft = abs(pArea->x) - (abs(XPos) + 10);
	}
	else
	{
		pixelsAvailableAtLeft = (abs(XPos) - 10) - abs(pArea->x);
	}
	// Similar calculations for pixels available from the end of the phrasebox plus an
	// extra ten pixels, to the right edge of the monitor. Again, we calculate a +ve value
	int pixelsAvailableAtRight;
	if (XPos < 0)
	{
		pixelsAvailableAtRight = (abs(XPos) - phraseBoxWidth - 10) - (abs(pArea->x) - pArea->width);
	}
	else
	{
		// here XPos is >= 0
		pixelsAvailableAtRight = (pArea->x + pArea->width) - (XPos + phraseBoxWidth + 10);
	}

	// Define topMaxOverlap and bottomMaxOverlap, in pixels, which determine how much overlap we
	// allow of the dialog over the AI frame rectangle at the top, or bottom. Top has more
	// room - we allow up to 96 pixels, bottom has only the status bar, we allow up to 40;
	// and preferred values for these - 54, and 20 pixels, respectively
	int topMaxOverlap = 96;
	int bottomMaxOverlap = 60; // we'll enroach as far as into 2nd-bottom strip approx
	int topPreferredOverlap = 54; // just the titlebar and the menu bar
	int bottomPreferredOverlap = 40; // status bar and a bit more, the active section is not
									 // ever likely to be at such a low place within the frame
	int pixelsAvailableAboveFrame = frame_yCoord - pArea->y; // note, if this monitor is larger than the
													 // primary monitor, pArea->y will be negative,
													 // and so increase the final value from YPos
#if defined(_DEBUG)
        wxLogDebug(_T("\nReposition dlg: (frame_yCoord %d - pArea->y %d ) = pixelsAvailableAboveFrame %d"),
                   frame_yCoord, pArea->y, pixelsAvailableAboveFrame);
#endif
	if (pixelsAvailableAboveFrame < 0)
	{
		pixelsAvailableAboveFrame = 0;
	}
	int pixelsAvailableBelowFrame;
	if ((frame_yCoord + frameHeight) > pArea->height)
	{
		pixelsAvailableBelowFrame = 0;
	}
	else
	{
		pixelsAvailableBelowFrame = pArea->height - (frame_yCoord + frameHeight);
#if defined(_DEBUG)
//        wxLogDebug(_T("\nReposition dlg: pArea->height %d, less ( frame_yCoord %d + frameHeight %d ) = pixelsAvailableBelowFrame %d"),
//                   pArea->height, frame_yCoord, frameHeight, pixelsAvailableBelowFrame);
#endif
	}
	// Our roughest calculation is to see where most of the obstructable area is, whether
	// above or below the active strip. Other calculations may override what this tells us
	bool bAtTopIsBetter = pixelsAvailableAbovePhraseBox > pixelsAvailableBelowPhraseBox;

#if defined(__WXGTK__)
	// A kludge to fix problem of the dialog, for the centered_below case, displaying too high
	// - by approx an amount equal to the height of the status bar (check top too, it seems to
    // act a bit differently than on Windows there too
    wxSize barsize = pFrame->m_pStatusBar->GetSize();
    int barHeight = barsize.GetHeight();
#if defined(_DEBUG)
        wxLogDebug(_T("\nReposition dlg: barHeight %d + 2 = %d"), barHeight, barHeight + 2);
    int value;
#endif
#endif

	// Determine if display above the frame (and outside it) is possible, or if we can
	// achieve our preferred overlap, or if we can squeeze it in without going beyond our
	// maximum overlap
	bool bCanFitAboveWithoutOverlap = pixelsAvailableAboveFrame >= dlgHeight;
#if defined(__WXGTK__)
    // Kludge to get it right on Linux
    bCanFitAboveWithoutOverlap = (pixelsAvailableAboveFrame - (barHeight + 2) ) >= dlgHeight;
#if defined(_DEBUG)
        value =(pixelsAvailableAboveFrame - (barHeight + 2) );
        wxLogDebug(_T("\nbCanFitAboveWithoutOverlap: %d : from  pixelsAvailableAboveFrame %d - (barHeight+2) %d = %d is greater than %d"),
                   (int)bCanFitAboveWithoutOverlap, pixelsAvailableAboveFrame, barHeight+2, value, dlgHeight);
#endif
#endif
	bool bCanFitAboveWithPreferredOverlap = (pixelsAvailableAboveFrame + topPreferredOverlap) >= dlgHeight;
#if defined(__WXGTK__)
    // Kludge to get it right on Linux
    bCanFitAboveWithPreferredOverlap = (pixelsAvailableAboveFrame - (barHeight + 2) + topPreferredOverlap) >= dlgHeight;
#if defined(_DEBUG)
        value = (pixelsAvailableAboveFrame - (barHeight + 2) + topPreferredOverlap);
        wxLogDebug(_T("\nbCanFitAboveWithPreferredOverlap: %d : from  pixelsAvailableAboveFrame %d - (barHeight+2) %d + topPreferredOverlap %d =  %d  is greater than %d"),
                   (int)bCanFitAboveWithPreferredOverlap, pixelsAvailableAboveFrame, barHeight+2, topPreferredOverlap, value, dlgHeight);
#endif
#endif
	bool bCanFitAboveButOnlyJust = (pixelsAvailableAboveFrame + topMaxOverlap) >= dlgHeight;
#if defined(__WXGTK__)
    // Kludge to get it right on Linux
    bCanFitAboveButOnlyJust = (pixelsAvailableAboveFrame - (barHeight + 2) + topMaxOverlap) >= dlgHeight;
#if defined(_DEBUG)
        value = (pixelsAvailableAboveFrame - (barHeight + 2) + topMaxOverlap);
        wxLogDebug(_T("\nbCanFitAboveButOnlyJust: %d : from  pixelsAvailableAboveFrame %d - (barHeight+2) %d  + topMaxOverlap %d =  %d  is greater than %d"),
                   (int)bCanFitAboveButOnlyJust, pixelsAvailableAboveFrame, barHeight+2, topMaxOverlap, value, dlgHeight);
#endif
#endif

	// Now the "below" ones
	bool bCanFitBelowWithoutOverlap = (pixelsAvailableBelowFrame + heightDiff) >= dlgHeight;
#if defined(__WXGTK__)
    // Kludge to get it right on Linux
    bCanFitBelowWithoutOverlap = (pixelsAvailableBelowFrame - (barHeight + 2) + heightDiff) >= dlgHeight;
#if defined(_DEBUG)
        value = (pixelsAvailableBelowFrame - (barHeight + 2) + heightDiff);
//        wxLogDebug(_T("\nbCanFitBelowWithoutOverlap: %d : from  pixelsAvailableBelowFrame %d  - (barHeight+2) %d  + heightDiff %d =  %d  is greater than %d"),
//                   (int)bCanFitBelowWithoutOverlap, pixelsAvailableBelowFrame, barHeight + 2, heightDiff, value, dlgHeight);
#endif
#endif
	bool bCanFitBelowWithPreferredOverlap = (pixelsAvailableBelowFrame + heightDiff + bottomPreferredOverlap) >= dlgHeight;
#if defined(__WXGTK__)
    // Kludge to get it right on Linux
    bCanFitBelowWithPreferredOverlap = (pixelsAvailableBelowFrame - (barHeight + 2) + heightDiff + bottomPreferredOverlap) >= dlgHeight;
#if defined(_DEBUG)
        value = (pixelsAvailableBelowFrame - (barHeight + 2) + heightDiff + bottomPreferredOverlap);
//        wxLogDebug(_T("\nbCanFitBelowWithPreferredOverlap: %d : from  pixelsAvailableBelowFrame %d  - (barHeight+2) %d  + heightDiff %d + bottomPreferredOverlap %d =  %d  is greater than %d"),
//                   (int)bCanFitBelowWithPreferredOverlap, pixelsAvailableBelowFrame, barHeight + 2, heightDiff, bottomPreferredOverlap, value, dlgHeight);
#endif
#endif
	bool bCanFitBelowButOnlyJust = (pixelsAvailableBelowFrame + heightDiff + bottomMaxOverlap) >= dlgHeight;
#if defined(__WXGTK__)
    // Kludge to get it right on Linux
    bCanFitBelowButOnlyJust = (pixelsAvailableBelowFrame - (barHeight + 2) + heightDiff + bottomMaxOverlap) >= dlgHeight;
#if defined(_DEBUG)
        value = (pixelsAvailableBelowFrame - (barHeight + 2) + heightDiff + bottomMaxOverlap);
//        wxLogDebug(_T("bCanFitBelowButOnlyJust: %d : from  pixelsAvailableBelowFrame %d  - (barHeight+2) %d  + heightDiff %d + bottomPreferredOverlap %d =  %d  is greater than %d"),
//                   (int)bCanFitBelowButOnlyJust, pixelsAvailableBelowFrame, barHeight + 2, heightDiff, bottomMaxOverlap, value, dlgHeight);
#endif
#endif

	DisplayConstraint constraint = centered_above; // initialize to the default enum value (our preferred option)

	// Try first for above the frame, or failing that, below the frame dlg location; if
	// failing both, we'll go for one of the corners
	bool bRightCorner = TRUE; // our preferred default for the corners
	int halfFrameWidth = frameWidth/2;
	int halfDialogWidth = dlgWidth/2;
	if ( bCanFitAboveWithoutOverlap || bCanFitAboveWithPreferredOverlap || bCanFitAboveButOnlyJust)
	{
		constraint = centered_above; // we'll be more specific in the switch below
	}
	else if ( bCanFitBelowWithoutOverlap || bCanFitBelowWithPreferredOverlap || bCanFitBelowButOnlyJust)
	{
		constraint = centered_below; // likewise, we'll be more specific in the switch
	}
	else
	{
		// Not above or below, so the corner choice has to be done in this block
		if (bAtTopIsBetter)
		{
			// Which corner?
			bRightCorner = pixelsAvailableAtRight > pixelsAvailableAtLeft;
			if (bRightCorner)
				constraint = top_right;
			else
				constraint = top_left;
		}
		else // at bottom is better
		{
			// Which corner?
			bRightCorner = pixelsAvailableAtRight > pixelsAvailableAtLeft;
			if (bRightCorner)
				constraint = bottom_right;
			else
				constraint = bottom_left;
		}
	}

#if defined(__WXGTK__)
#if defined(_DEBUG)
        int valueBefore;
        int valueAfter;
#endif
#endif
	// Now we are ready for the calculations for the 10 possible locations - three
	// top centered locations, three bottom centered ones, and the four corners
	switch (constraint)
	{
	case centered_above:
	{
		// The best option is to obscure nothing, so if it will fit without overlap, do
		// that; if not try for our preferred overlap option - if that isn't enough, then
		// take the max overlap option
		if (bCanFitAboveWithoutOverlap)
		{
			// Don't go as far above as possible, just sit it on top of the frame
			myTopCoord = frame_yCoord - dlgHeight;
		}
		else if (bCanFitAboveWithPreferredOverlap)
		{
			// Overlap a bit
			myTopCoord = frame_yCoord - dlgHeight + topPreferredOverlap;
		}
		else
		{
			// Take the max overlap option
			myTopCoord = frame_yCoord - dlgHeight + topMaxOverlap;
		}
		myLeftCoord = frame_xCoord + halfFrameWidth - halfDialogWidth;
#if defined(__WXGTK__)
#if defined(_DEBUG)
		valueBefore = myTopCoord;
		valueAfter = myTopCoord - (barHeight + 2);
		wxLogDebug(_T("\nmyTopCoord: ABOVE:  BEFORE Kludge %d   AFTER kludge %d: "), valueBefore, valueAfter);
		wxLogDebug(_T("\n\n"));
#endif
		myTopCoord -= barHeight + 2;
#endif
	}
		break;
	case centered_below:
	{
		// Try for the most amount of non-obscuring
		if (bCanFitBelowWithoutOverlap)
		{
			myTopCoord = frame_yCoord + frameHeight;
		}
		else if (bCanFitBelowWithPreferredOverlap)
		{
			myTopCoord = frame_yCoord + frameHeight - bottomPreferredOverlap;
		}
		else
		{
			myTopCoord = frame_yCoord + frameHeight - bottomMaxOverlap;
		}
		myLeftCoord = frame_xCoord + halfFrameWidth - halfDialogWidth;
#if defined(__WXGTK__)
#if defined(_DEBUG)
		valueBefore = myTopCoord;
		valueAfter = myTopCoord + (barHeight + 2);
		wxLogDebug(_T("\nmyTopCoord: BEFORE Kludge %d   AFTER kludge %d: "), valueBefore, valueAfter);
		wxLogDebug(_T("\n\n"));
#endif
		myTopCoord += barHeight + 2; // the +2 makes it right
#endif
	}
		break;
	case top_right:
	{
		// Dialog has to lie within myMonitor, and the dialog is located at the
		// top right edge of the monitor
		myTopCoord = pArea->y; // could be -ve if primary monitor is smaller than this one
		myLeftCoord = pArea->x + (pArea->width - dlgWidth);
	}
		break;
	case bottom_right:
	{
		myTopCoord = pArea->y + pArea->height - dlgHeight;
		myLeftCoord = pArea->x + (pArea->width - dlgWidth);
	}
		break;
	case top_left:
	{
		myTopCoord = pArea->y; // could be -ve if primary monitor is smaller than this one
		myLeftCoord = pArea->x;
	}
		break;
	case bottom_left:
	{
		myTopCoord = pArea->y + pArea->height - dlgHeight;
		myLeftCoord = pArea->x;
	}
		break;
	} // end of switch (constraint)

	// clear the display and rectangle objects from the heap
	for (monIndex = 0; monIndex < numMonitors; monIndex++)
	{
		pMonitor = (wxDisplay*)arrDisplays.Item(monIndex);
		delete pMonitor;
		pArea = (wxRect*)arrAreas.Item(monIndex);
		delete pArea;
	}
}


// Return TRUE if the targetStr param of the MakeTargetStringWithPunctuation(), when
// parsed into a series of 'words' (they may contain user-typed punctuation) and each is
// looked up in series, each is found within the pSrcPhrase->m_punctsPattern string, and
// in ascending sequence of offsets returned by Find() in the internal loop - which
// indicates that the user didn't reorder the words in the phrase box, and hasn't typed
// explicit punctuation in the phrase box either. Otherwise, return FALSE.
// Used only in MakeTargetStringWithPunctuation(), at an active location which is not a
// fixedsapce conjoined CSourcePhrase instance.
// BEW created 23Feb12, for use in the docVersion6 feature which removes the redundant
// showing of either a punction placement dialog, or when exporting adaptations, or
// glosses-as-text, a medial marker or final marker placement dialog.
bool IsPhraseBoxAdaptionUnchanged(CSourcePhrase* pSrcPhrase, wxString& tgtPhrase)
{
	wxArrayString arr; arr.Clear();
	if (tgtPhrase.IsEmpty())
	{
		return FALSE; // the safest value for such a situation
	}
	wxString delimiters = _T(' '); // just use space to separate them
	wxString testStr = pSrcPhrase->m_lastAdaptionsPattern;
	wxString aSpace = _T(' ');
	int howmany;
	howmany = (int)SmartTokenize(delimiters, tgtPhrase, arr, FALSE); // FALSE means we won't store empty strings
	int offset = 0;
	int lastOffset = 0;
	int countOfWordsFound = 0;
	if (howmany >= 1)
	{
		int index;
		for (index = 0; index < howmany; index++)
		{
			wxString aWord = arr[index];
			offset = FindFromPos(testStr, aWord, lastOffset);
			if (offset == wxNOT_FOUND)
			{
				return FALSE;
			}
			else
			{
				// aWord was found in testStr
				if (offset == 0 && lastOffset == 0)
				{
					// matched the first word in testStr, so okay so far
					lastOffset = offset + aWord.Len();
					countOfWordsFound++;
					// get to next space (the match might have only matched first part of
					// the word - in which case, we'd want to return FALSE immediately)
					int extraCharsCount = 0;
					while (lastOffset < (int)testStr.Len() && testStr[lastOffset] != aSpace)
					{
						lastOffset++;
						extraCharsCount++;
					}
					if (extraCharsCount > 0)
					{
						// the user has typed a different word
						return FALSE;
					}
				}
				else if (offset > lastOffset)
				{
					// matched a non-initial substring of testStr, okay so far
					lastOffset = offset + aWord.Len();
					countOfWordsFound++;
					// if we matched 'within' a word, then the user must have edited the
					// start of aWord to remove at least a character of it, so that counts
					// as "an edited string" and so FALSE must be returned
					if (offset > 0 && testStr[offset - 1] != aSpace)
					{
						return FALSE;
					}
					// get to next space (the match might have only matched first part of
					// the word - in which case, we'd want to return FALSE immediately)
					int extraCharsCount = 0;
					while (lastOffset < (int)testStr.Len() && testStr[lastOffset] != aSpace)
					{
						lastOffset++;
						extraCharsCount++;
					}
					if (extraCharsCount > 0)
					{
						// the user has typed a different word
						return FALSE;
					}
				}
				else
				{
					// there wasn't a match, so return FALSE
					return FALSE;
				}
			}
		}
	}
	if (countOfWordsFound != howmany)
	{
		// if there were fewer matches then the word count, or more, then the user has
		// done some editing - either to remove one or more words, to to add one or more
		// words, and in either case that counts as an "edited string" and so return FALSE
		return FALSE;
	}
	// if control gets to here, it's pretty certain the strings are identical
	return TRUE;
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
		msg1 = msg1.Format(_T("\n              CPile: Where = %d ; m_pSrcPhrase %p ; m_pOwningStrip %p ; m_nPile %d ; Strip index is UNDEFINED"),
		whereTis, pPile->GetSrcPhrase(), pPile->GetStrip(), pPile->GetPileIndex());
	}
	else
	{
		// strip pointer is not NULL, so can get value of strip index
		msg1 = msg1.Format(_T("\n              CPile: Where = %d ; m_pSrcPhrase %p ; m_pOwningStrip %p ; m_nPile %d ; m_nStrip %d"),
		whereTis, pPile->GetSrcPhrase(), pPile->GetStrip(), pPile->GetPileIndex(), pPile->GetStrip()->GetStripIndex());
	}
	wxLogDebug(msg1);
	wxString msg2;
	msg2 = msg2.Format(_T("CSourcePhrase:   SequNum %d ; m_pSrcPhrase %p ; m_srcPhrase %s ; m_targetStr %s ; m_gloss %s"),
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
/// Added here by GDLC 6May11 to avoid including the MachOS headers inside the class CAdapt_ItApp.
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
	//	GDLC 20MAR14 the natural_t type is now deprecated and is too small for Macs with 12GB of RAM
	//	natural_t   mem_free = vm_stat.free_count * pagesize;
	long long   mem_free = vm_stat.free_count * pagesize;
	//	natural_t   mem_total = mem_used + mem_free;

	return static_cast <wxMemorySize> (mem_free);
}

#endif	/* __WXMAC__ */

/// A helper for KB Sharing, to check certain language codes exist, and if they don't, to
/// let the user set them using the language codes dialog
/// \return             TRUE if the wanted codes exist at exit, FALSE otherwise
/// \param  bSrc        ->  TRUE if this one is to be checked, FALSE if to be ignored
/// \param  bTgt        ->  TRUE if this one is to be checked, FALSE if to be ignored
/// \param  bGloss      ->  TRUE if this one is to be checked, FALSE if to be ignored
/// \param  bFreeTrans  ->  TRUE if this one is to be checked, FALSE if to be ignored
/// \param  bUserCancelled -> TRUE if the user cancels out of the function explicitly,
///                           FALSE if neither of the two cancellation chances are taken
/// \remarks
/// Checks the app's members m_sourceLanguageCode, m_targetLanguageCode,
/// m_glossesLanguageCode, m_freeTransLanguageCode, according to which of the four input
/// paramaters are TRUE, and if one or more of the chosen ones is empty, it puts up the
/// language codes dialog for the user to set the desired codes. Before the dialog is put
/// up, a message is shown explaining what is wanted of the user. The user is free to set
/// codes for text types not being asked for, such settings are stored in the app, but
/// only the wanted codes are actually used by the feature requesting that certain codes
/// be set. For instance, kbserver of type 1 (an adaptation KB) will want only bSrc and
/// bTgt checked, while kbserver of type 2 (a glossing KB) will want only bSrc and bGloss
/// checked. In either case, if the user sets a code for bFreeTrans, it would be stored in
/// the app in the appropriate place, but the KB Sharing feature would ignore that one's
/// setting. The user can cancel out, and so we can't interpret a returned FALSE as a
/// cancel, and therefore we track user's manual cancellation option with bUserCancelled;
/// but currently, return FALSE can only happen when bUserCancelled = TRUE, because either
/// the user supplies the needed codes, and is nagged till he does, or he cancels.
/// We also check when one or more codes are NOCODE (i.e. "qqq"), but if the user then
/// elects to retain one or more NOCODE values, we accept them.
bool CheckLanguageCodes(bool bSrc, bool bTgt, bool bGloss, bool bFreeTrans, bool& bUserCancelled)
{
	// use these next line, and returning TRUE is best for caller ignoring
	wxUnusedVar(bSrc);
	wxUnusedVar(bTgt);
	wxUnusedVar(bGloss);
	wxUnusedVar(bFreeTrans);
	wxUnusedVar(bUserCancelled);
	return TRUE;
	/* BEW 5Sep20, no longer needed, remove
	bUserCancelled = FALSE; // default
	// The next test tests yields TRUE if a wanted code has its app storage member for it
	// empty, or the NOCODE ("qqq") is currently in the app storage member and that member
	// code is wanted. It only takes one empty storage string, or a qqq, to make the test
	// exit with TRUE returned
	if ( ((gpApp->m_sourceLanguageCode.IsEmpty() && bSrc) || ((gpApp->m_sourceLanguageCode == NOCODE) && bSrc)) ||
		 ((gpApp->m_targetLanguageCode.IsEmpty() && bTgt) || ((gpApp->m_targetLanguageCode == NOCODE) && bTgt)) ||
		 ((gpApp->m_glossesLanguageCode.IsEmpty() && bGloss) || ((gpApp->m_glossesLanguageCode == NOCODE) && bGloss)) ||
		 ((gpApp->m_freeTransLanguageCode.IsEmpty() && bFreeTrans) || ((gpApp->m_freeTransLanguageCode == NOCODE) && bFreeTrans)) )
	{
		// Something needs to be done, so load up the dialog's boxes with whatever we
		// currently are storing for all four codes
		wxString srcCode;
		wxString tgtCode;
		wxString glossCode;
		wxString freeTransCode;

		bool bCodesEntered = FALSE;
		while (!bCodesEntered)
		{
			// Call up CLanguageCodesDlg here so the user can enter language
			// codes which are needed
			CLanguageCodesDlg lcDlg((wxWindow*)gpApp->GetMainFrame(), all_possibilities);
			lcDlg.Center();
			// Load any lang codes already stored on the App into the dialog's edit boxes
			lcDlg.m_sourceLangCode = gpApp->m_sourceLanguageCode;
			lcDlg.m_targetLangCode = gpApp->m_targetLanguageCode;
			lcDlg.m_glossLangCode = gpApp->m_glossesLanguageCode;
			lcDlg.m_freeTransLangCode = gpApp->m_freeTransLanguageCode;
            // The language code dialog allows the user to setup codes for src, tgt, gloss
            // and/or free translation languages
			int returnValue = lcDlg.ShowModal();
			if (returnValue == wxID_CANCEL)
			{
				// user cancelled
				bUserCancelled = TRUE;
				return FALSE;
			}
			// Get the results of the user's settings of the codes
			srcCode = lcDlg.m_sourceLangCode;
			tgtCode = lcDlg.m_targetLangCode;
			glossCode = lcDlg.m_glossLangCode;
			freeTransCode = lcDlg.m_freeTransLangCode;

			wxString message,langStr;
			langStr = _T("");
			// Check that codes have been entered in the boxes required according to which
			// passed in booleans were TRUE
			if ( (bSrc && srcCode.IsEmpty()) ||
				 (bTgt && tgtCode.IsEmpty()) ||
				 (bGloss && glossCode.IsEmpty()) ||
				 (bFreeTrans && freeTransCode.IsEmpty()) )
			{
				if (bSrc && srcCode.IsEmpty())
				{
					langStr += _("Source, ");
				}
				if (bTgt && tgtCode.IsEmpty())
				{
					langStr += _("Target, ");
				}
				if (bGloss && glossCode.IsEmpty())
				{
					langStr += _("Glosses, ");
				}
				if (bFreeTrans && freeTransCode.IsEmpty())
				{
					langStr += _("Free translation");
				}
				// Remove final space if present, and final comma if present
				langStr.Trim(); // trim at right end (ie. will be at left if RTL script)
				int thelength = langStr.Len();
				wxChar lastChar = langStr.GetChar(thelength - 1); // get last character
				if (lastChar == _T(','))
				{
					// Remove the final comma
					langStr = langStr.Left(thelength - 1);
				}
				message = message.Format(_("Missing language code for the following language(s):\n\n%s\n\nA code is required for each language that you missed.\nDo you want to try again?"),langStr.c_str());
                // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                gpApp->m_bUserDlgOrMessageRequested = TRUE;
                int response = wxMessageBox(message, _("Language code(s) missing"), wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);

				// Accept whatever we've got so far, at least
				gpApp->m_sourceLanguageCode = srcCode;
				gpApp->m_targetLanguageCode = tgtCode;
				gpApp->m_glossesLanguageCode = glossCode;
				gpApp->m_freeTransLanguageCode = freeTransCode;

				// Check the user's response to the message; either have another go, or
				// cancel out
				if (response == wxNO)
				{
					// user wants to abort
					bUserCancelled = TRUE;
					return FALSE;
				}
				else
				{
					bCodesEntered = FALSE;
				}
			}
			else
			{
				bCodesEntered = TRUE;
			}
		} // loop, while (!bCodesEntered) is TRUE

		// update the App's members with the final results
		gpApp->m_sourceLanguageCode = srcCode;
		gpApp->m_targetLanguageCode = tgtCode;
		gpApp->m_glossesLanguageCode = glossCode;
		gpApp->m_freeTransLanguageCode = freeTransCode;
	}
	return TRUE;
*/
}

// returns TRUE if all's well, FALSE if user hit Cancel button in the internal dialog
bool CheckUsername()
{
	CAdapt_ItApp* pApp = &wxGetApp();
	// Save current values, in case the user cancels
	wxString saveUserID = pApp->m_strUserID;
	wxString saveInformalUsername = pApp->m_strFullname;

	// Don't permit control to return to the caller unless there is a value for each of
	// these three, if the user exits with an OK button click
	if (pApp->m_strUserID == NOOWNER || pApp->m_strFullname == NOOWNER )
	{
		UsernameInputDlg dlg((wxWindow*)pApp->GetMainFrame());
		dlg.Center();
		if (dlg.ShowModal() == wxID_OK)
		{
			pApp->m_strUserID = dlg.m_finalUsername;
			pApp->m_strFullname = dlg.m_finalInformalUsername;

			// whm added 24Oct13. Save the UniqueUsername and InformalUsername
			// Note: This code block below should be the same as the block in
			// Adapt_It.cpp's OnEditChangeUsername().
			// values in the Adapt_It_WX.ini (.Adapt_It_WX) file for safe keeping
			// and the ability to restore these values if the user does a Shift-Down
			// startup of the application to reset the basic config file values.
			bool bWriteOK = FALSE;
			wxString oldPath = pApp->m_pConfig->GetPath(); // is always absolute path "/Recent_File_List"
			pApp->m_pConfig->SetPath(_T("/Usernames"));
			// We want even a null string value for the UniqueUsername and InformalUsername strings
			// to be saved in Adapt_It_WX.ini.
			{ // block for wxLogNull
				wxLogNull logNo; // eliminates spurious message from the system
				bWriteOK = pApp->m_pConfig->Write(_T("unique_user_name"), pApp->m_strUserID);
				if (!bWriteOK)
				{
					wxMessageBox(_T("CheckUsername() m_pConfig->Write() of m_strUserID returned FALSE, processing will continue, but save, shutdown and restart would be wise"));
				}
				bWriteOK = pApp->m_pConfig->Write(_T("informal_user_name"), pApp->m_strFullname);
				if (!bWriteOK)
				{
					wxMessageBox(_T("CheckUsername() m_pConfig->Write() of m_strFullname returned FALSE, processing will continue, but save, shutdown and restart would be wise"));
				}
				pApp->m_pConfig->Flush(); // write now, otherwise write takes place when m_pConfig is destroyed in OnExit().
			}
			// restore the oldPath back to "/Recent_File_List"
			pApp->m_pConfig->SetPath(oldPath);
		}
		else
		{
			// user cancelled, so restore the saved original values & return FALSE
			pApp->m_strUserID = saveUserID;
			pApp->m_strFullname = saveInformalUsername;
			return FALSE;
		}
	}
	return TRUE;
}


//#if defined(_KBSERVER)

CBString MakeDigestPassword(const wxString& user, const wxString& password)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxString colon = _T(':');
	wxString realm = _T("kbserver"); // always this realm
	wxString strForDigest = user + colon + realm + colon + password;
	CBString sbDigest(pApp->Convert16to8(strForDigest));
	CBString digestPassword(md5_SB::GetMD5(sbDigest));
	return digestPassword;
}

bool AuthenticateEtcWithoutServiceDiscovery(CAdapt_ItApp* pApp)
{
	// Prepare an error message in case it is needed
	wxString title = _("Unsuccessful connection attempt");
	wxString msg = _("Tried to connect to the KBserver with ipAddr: %s\nand name: %s but failed.\n Use the command \"Discover KBservers\", and then re-try to connect using\n\"Setup Or Remove Knowledge Base Sharing\"");
	msg = msg.Format(msg, pApp->m_strKbServerIpAddr.c_str(), pApp->m_strKbServerHostname.c_str());
	pApp->m_bUserLoggedIn = FALSE; // initialize


	// In next call, FALSE is: bool bServiceDiscoveryWanted
	// When we enter the project via the wizard (as we must) and it is one which
	// the project config file says is a KB Sharing project, the basic config file
	// should have the ipAddr and hostname for the KBserver last logged in to. We
	// will try to log in with the old credentials, since service discovery will
	// not at this point have had a change to be run and to discover any running
	// KBservers. We assume must users will run the one KBserver before the session,
	// and that it will have the same ipAddr each time. If the ipAddr is wrong, we let
	// the connection attempt fail (returning FALSE) and indicate that sharing has
	// been turned off. If instead we succeed in connecting, then that proves the
	// last used ipAddr is still valid and its KBserver is running - so we add the
	// relevant data (a 'compositeStr' of form <ipaddress>@@@<hostname>)
	// to app::m_ipAddrs_Hostnames array, as if a service discovery run had been
	// made and succeeded in discovering that running KBserver.
	// We run this AuthenticateEtcWithoutServiceDiscovery() function, not from
	// ProjectPage::OnWizardPageChanging() as that would display the authentication
	// dialog in the middle of working through the wizard, from project to
	// and open document; but rather we just there set a boolean,
	// CAdapt_ItApp::m_bEnteringKBserverProject to TRUE, and use that to call this
	// function from CMainFrame's OnIdle() handler - providing it's an adaptations
	// or glosses sharing project. Its dialog will appear just after the document
	// is laid out
	bool bNeedDiscovery = FALSE;
	if (pApp->m_strKbServerIpAddr.IsEmpty())
	{
		//wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		//pApp->m_bUserLoggedIn = FALSE;
		//return FALSE;
		bNeedDiscovery = TRUE;
	}
	// In next call, FALSE is: bool bServiceDiscoveryWanted
	bool bSucceeded = AuthenticateCheckAndSetupKBSharing(pApp, bNeedDiscovery);
	wxString ipaddress = wxEmptyString;
	if (bSucceeded)
	{
		pApp->m_bUserLoggedIn = TRUE; // if we don't set this, there are circumstances
				// where the Controls For K B Sharing command will be disabled
		if (!pApp->m_strKbServerIpAddr.IsEmpty())
		{
			wxString ipAddress = pApp->m_strKbServerIpAddr;

			// Don't proceed to store it if the same ipAddr is already stored within the array
			if (IsIpAddrStoreable(&pApp->m_ipAddrs_Hostnames, pApp->m_strKbServerIpAddr))
			{
				wxASSERT(!ipaddress.IsEmpty());
				wxString compositeStr = ipaddress + _T("@@@");
				compositeStr += pApp->m_strKbServerHostname;
				pApp->m_ipAddrs_Hostnames.Add(compositeStr);
			}
			pApp->m_bLoginFailureErrorSeen = FALSE;
			pApp->m_bUserLoggedIn = TRUE;
			wxLogDebug(_T("helpers.cpp line = %d AuthenticateCheckAndSetupKB Sharing, m_ipAdds_Hostnames entry count = %d"),
				__LINE__, pApp->m_ipAddrs_Hostnames.GetCount());
			return TRUE;
		}
		else
		{
			if (!pApp->m_bLoginFailureErrorSeen)
			{
				wxMessageBox(msg, title, wxICON_WARNING | wxOK);
				pApp->m_bLoginFailureErrorSeen = FALSE;
			}
			pApp->m_bUserLoggedIn = FALSE;
			return FALSE;
		}
	}
	else
	{
		if (!pApp->m_bAuthenticationCancellation)
		{
			// Don't show the message if there was real cancellation from the
			// Authenticate dialog - since it was a user choice, what happens
			// next should not be a failure message
			if (!pApp->m_bLoginFailureErrorSeen)
			{
				// To prevent too many messages appearing, suppress this one
				// if other messages have already been seen
				wxMessageBox(msg, title, wxICON_WARNING | wxOK);
			}
			pApp->m_bLoginFailureErrorSeen = FALSE;
		}
		pApp->m_bAuthenticationCancellation = FALSE; // re-initialize
		pApp->m_bUserLoggedIn = FALSE;
	}
	return FALSE;
}

bool IsIpAddrStoreable(wxArrayString* pArr, wxString& ipAddr)
{
	if (pArr->IsEmpty())
	{
		// If the array is empty, storing it cannot produce a duplicate,
		// so return TRUE
		return TRUE;
	}
	wxString strNoProtocol = ipAddr;
	size_t count = pArr->GetCount();
	size_t index;
	int offset = wxNOT_FOUND;
	// strNoProtocol has just the ipAddress
	for (index = 0; index < count; index++)
	{
		wxString anIpAddr = pArr->Item(index);
		offset = anIpAddr.Find(strNoProtocol);
		if (offset >= 0)
		{
			// We have matched the passed in ipAddr, so it is not storeable
			return FALSE;
		}
	}
	// If control gets to here, there were no matches, so it is storable
	return TRUE;
}

// The following function encapsulates KBserver service discovery, authentication to a running
// KBserver (error if one is not running of course), checks for valid language codes, username,
// and calls to GetKBServer[0] and [1] as required, with error checking and error messages as
// required, and failure to setup sharing if there was error - with user notification visually.
// Use this function only when the user is authenticating. Authentication to the KB Sharing
// Manager requires a different function (see below). bServiceDiscoveryWanted is set or cleared
// in the OnOK() handler of KbSharing Setup instance, where the options are the default - to
// let service discovery search on the LAN for a KBserver, or the user knows an ipAddr and elects
// to type it in (if not shown from last-used stored value on basic config file). If he elects
// to let discovery happen, KBSharingAuthenticationDlg will hide the top multiline message in
// the Authenticate dialog as it applies only when the user is doing a manual type in of the ipAddr
// Returns TRUE for success, FALSE if there was an error
bool AuthenticateCheckAndSetupKBSharing(CAdapt_ItApp* pApp, bool bServiceDiscoveryWanted)
{
    // use this AuthenticateCheckAndSetupKBSharing() function only when the user is
    // authenticating, do not use it for authentication to the KB Sharing Manager -- the
    // latter authentication job is done from the app instance, in the handler which
    // creates and launches the KB Sharing Manager instance

	pApp->m_bUserAuthenticating = TRUE; // user authentications require this be TRUE
	pApp->m_bLoginFailureErrorSeen = FALSE;
	pApp->m_bAuthenticationCancellation = FALSE;
	pApp->m_bUserLoggedIn = FALSE; // initialize

	CMainFrame* pFrame = pApp->GetMainFrame();
	// Make the bServiceDiscoveryWanted param accessible to KBSharingAuthenticationDlg
	// (the "Authenticate" dialog)
	pApp->m_bServiceDiscoveryWanted = bServiceDiscoveryWanted;

	// BEW 11Jan16, save these five, so we can restore them if some kind of failure happens
	pApp->m_saveOldIpAddrStr = pApp->m_chosenIpAddr; // should have valid ipAddr if manual discovery just done
	if (!pApp->m_chosenIpAddr.IsEmpty())
	{
		pApp->m_strKbServerIpAddr = pApp->m_chosenIpAddr; // gets used often & stored in config file
		pApp->m_curIpAddr = pApp->m_chosenIpAddr; // a second place, just in case
	}
	pApp->m_saveOldHostnameStr = pApp->m_strKbServerHostname;
	pApp->m_saveOldUsernameStr = pApp->m_strUserID;
	pApp->m_savePassword = pFrame->GetKBSvrPassword();
	pApp->m_saveSharingAdaptationsFlag = pApp->m_bIsKBServerProject;
	pApp->m_saveSharingGlossesFlag = pApp->m_bIsGlossingKBServerProject;

	bool m_bSharingAdaptations;
	bool m_bSharingGlosses;

    // There may be a password stored in this session from an earlier connect lost
    // for some reason, or there may be no password yet stored. Get the password,
    // or empty string as the case may be, and set a boolean to carry the result forward
    bool bPasswordExists = FALSE; // this would be correct setting if authenticating
								  // to the Manager dlg
	wxString existingPassword = pFrame->GetKBSvrPassword();
	bPasswordExists = existingPassword.IsEmpty() ? FALSE : TRUE;
	bool bShowIpAddrAndUsernameDlg = TRUE; // initialize to the most likely situation
	bool bShowPasswordDlgOnly = FALSE;  // initialize
	bool bAutoConnectKBSvr = FALSE;     // initialize

	// Get the project config file values for the adapting & glossing sharing flags
	m_bSharingAdaptations = pApp->m_bIsKBServerProject_FromConfigFile;
	m_bSharingGlosses = pApp->m_bIsGlossingKBServerProject_FromConfigFile;
	// In the case of a new project, the config file values will both be false, and
	// if that is the case and we do no more here, then use of the Setup Or Remove
	// Knowledge Base Sharing dialog will get ignored. Or if the config files have
	// one or both set true, but the latter dialog has the checkboxes off, then
	// the latter's settings must prevail. That dialog sets the variables the config
	// file would use for storage, so check now and override any of the above settings
	// which differ
	if (pApp->m_bIsKBServerProject == TRUE)
	{
		if (!m_bSharingAdaptations)
		{
			m_bSharingAdaptations = TRUE;
		}
	}
	else
	{
		// User wants adaptations sharing to be OFF
		if (m_bSharingAdaptations)
		{
			m_bSharingAdaptations = FALSE;
		}
	}
	if (pApp->m_bIsGlossingKBServerProject == TRUE)
	{
		if (!m_bSharingGlosses)
		{
			m_bSharingGlosses = TRUE;
		}
	}
	else
	{
		// User wants glosses sharing to be OFF
		if (m_bSharingGlosses)
		{
			m_bSharingGlosses = FALSE;
		}
	}

	// The user may want to turn of sharing by setting both checkboxes to be unticked
	// in the Setup Or Remove Knowledge Base Sharing, and if so, then the above two
	// lines will have FALSE for both left hand variable. Check if so, and turn off
	// sharing, etc and exit without doing any service discovery etc
	if (!m_bSharingAdaptations && !m_bSharingGlosses)
	{
		pApp->m_bIsKBServerProject = FALSE;
		pApp->m_bIsGlossingKBServerProject = FALSE;
		if (pApp->KbServerRunning(1))
		{
			pApp->ReleaseKBServer(1); // the adaptations one
		}
		if (pApp->KbServerRunning(2))
		{
			pApp->ReleaseKBServer(2); // the glossings one
		}
		ShortWaitSharingOff(); //displays "Knowledge base sharing is OFF" for 1.3 seconds
		pApp->m_bUserLoggedIn = FALSE;
		return FALSE;
	}

    // If an adapting or glossing (or both) KBserver is wanted, do service
	// discovery, and if found, get its ipAddr, and then check
	// language codes and username and if all is well, login and set up the sharing
	// instance or instances KbServer[0] and/or KbServer[1]
	if (pApp->m_bIsKBServerProject || pApp->m_bIsGlossingKBServerProject)
	{
		if (!bServiceDiscoveryWanted)
		{ // 1
			bool bSimulateUserCancellation = FALSE; // initialize
			bool bSetupKBserverFailed = FALSE; // initialize

			// The user wants to manually type the ipAddr -- possibly for a web-based KBserver.
			// For this option, we can't assume the old ipAddr will be valid - but we'll show it
			// nevertheles, but with a message above to warn the user it could be incorrect.
			// We also can't assume the password, which may be already stored in the frame
			// member within this session, is going to be the one required for whatever ipAddr
			// gets typed in. We can only assume the username is correct, but it will be
			// checked in the KBSharingAuthenticationDlg dialog's OnOK() handler

			KBSharingAuthenticationDlg dlg(pFrame, pApp->m_bUserAuthenticating); // 2nd param TRUE
			dlg.Center();
			int dlgReturnCode;

here2:		dlgReturnCode = dlg.ShowModal();
			if (dlgReturnCode == wxID_OK)
			{ // 2
				// Check that needed language codes are defined for source, target, and if
				// a glossing kb share is also wanted, that source and glosses codes are
				// set too. Get them set up if not so.
				bool bUserCancelled = FALSE;
				// BEW 27Jan22 When logging in, it's likely there is no pwd value saved in frame's 
				// SetKBSvrPassword(pwd); but if authenticating, and the app's current value of 
				// m_strUserID matches what this dialog shows, and the dlg's m_strNormalPassword has
				//  a value, then assume it's a valid authentication attempt, and store the password
				// in pFrame, so that the appearance of the 4-field authentication dialog can be
				// suppressed because all needed values are determinate once the password for 
				// m_strUserID is known
				wxString pwd = dlg.m_strNormalPassword;
				if (!pwd.IsEmpty())
				{
					// handle storage - if appropriate
					wxString userValue = dlg.m_strNormalUsername;
					if (!userValue.IsEmpty() && pApp->m_strUserID == userValue)
					{
						// This pwd is probably valid for m_strUserID, so save it on pFrame
						pFrame->SetKBSvrPassword(pwd);
					}
				}
				// BEW 27Jan22 let processing continue - most of what follows has been rendered safe
				// for the new Leon-designed kbserver authentication protocols etc, so it more or
				// less does nothing. I've not tried to simplify or remove, as it's long and 
				// complex, and mostly not worth the bother - but some (e.g. potentially forcing
				// service discovery dlg open) may be important to keep

				// If no password entered, the login must fail
				//if (dlg.m_bError) <<-- this was FALSE when accept without typing a pwd
				//if (pwd.IsEmpty())
				//{
				//	pApp->m_bLoginFailureErrorSeen = TRUE;
				//	goto bad2;
				//}
                // We want valid codes for source and target if sharing the adaptations KB,
                // and for source and glosses languages if sharing the glossing KB.
                // (CheckLanguageCodes is in helpers.h & .cpp) We'll start by testing
                // adaptations KB, if that is wanted. Then again for glossing KB if that is
                // wanted (usually it won't be)
				bool bDidFirstOK = TRUE;
				bool bDidSecondOK = TRUE;
				if (m_bSharingAdaptations)
				{ // 3
					bDidFirstOK = CheckLanguageCodes(TRUE, TRUE, FALSE, FALSE, bUserCancelled);
					if (!bDidFirstOK || bUserCancelled)
					{ // 4
						// We must assume the src/tgt codes are wrong or incomplete, or that the
						// user has changed his mind about KB Sharing being on - so turn it off
						/* BEW 25Sep20, deprecate, codes are no longer wanted, names instead
						HandleBadLangCodeOrCancel(pApp->m_saveOldIpAddrStr, pApp->m_saveOldHostnameStr,
							pApp->m_saveOldUsernameStr, pApp->m_savePassword,
							pApp->m_saveSharingAdaptationsFlag, pApp->m_saveSharingGlossesFlag);
						pApp->m_bLoginFailureErrorSeen = TRUE;
						*/
						bSimulateUserCancellation = TRUE;
					}  // 3
				} // 2
				// Now, check for the glossing kb code, if that kb is to be shared
				bUserCancelled = FALSE; // re-initialize
				if (m_bSharingGlosses && !bSimulateUserCancellation)
				{ // 3
					bDidSecondOK = CheckLanguageCodes(TRUE, FALSE, TRUE, FALSE, bUserCancelled);
					if (!bDidSecondOK || bUserCancelled)
					{ // 4
						// We must assume the src/gloss codes are wrong or incomplete, or that the
						// user has changed his mind about KB Sharing being on - so turn it off
						/* BEW 25Sep20, deprecate, codes are no longer wanted, names instead
						HandleBadGlossingLangCodeOrCancel(pApp->m_saveOldIpAddrStr,
							pApp->m_saveOldHostnameStr, pApp->m_saveOldUsernameStr,
							pApp->m_savePassword, pApp->m_saveSharingAdaptationsFlag,
							pApp->m_saveSharingGlossesFlag);
						pApp->m_bLoginFailureErrorSeen = TRUE;
						*/
						bSimulateUserCancellation = TRUE;
					} // 3
				} // 2

				// If control gets to here, we can go ahead and establish the
				// setup(s), provided bSimulateUserCancellation is not TRUE
				if (!bSimulateUserCancellation)
				{ // 3

				// Shut down the old settings, and reestablish connection using the new
				// settings (this may involve an ipAddr change to share using a different KBserver)
				pApp->ReleaseKBServer(1); // the adaptations one
				pApp->ReleaseKBServer(2); // the glossing one
				pApp->m_bIsKBServerProject = FALSE;
				pApp->m_bIsGlossingKBServerProject = FALSE;

				// Do the setup or setups; use bSetupKBserverFailed = TRUE to carry
				// forward any error state, and skip functions that cannot succeed
				if (m_bSharingAdaptations)
				{ // 4
					// We want to share the local adaptations KB
					pApp->m_bIsKBServerProject = TRUE;
					if (!pApp->SetupForKBServer(1)) // try to set up an adapting KB share
					{ // 5
						// an error message will have been shown, so just log the failure
						pApp->LogUserAction(_T("SetupForKBServer(1) failed in AuthenticateCheckAndSetupKBSharing() "));
						pApp->m_bIsKBServerProject = FALSE; // no option but to turn it off
						// Tell the user
						wxString title = _("Setup failed");
						wxString msg =
_("The attempt to share the adaptations knowledge base failed.\nYou can continue working, but sharing of this knowledge base will not happen.");
						wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
						pApp->m_bLoginFailureErrorSeen = TRUE;
						bSetupKBserverFailed = TRUE;
					} // 4
				} // 3
				if (m_bSharingGlosses && !bSetupKBserverFailed)
				{ // 4
					// We want to share the local glossing KB
					pApp->m_bIsGlossingKBServerProject = TRUE;
					if (!pApp->SetupForKBServer(2)) // try to set up a glossing KB share
					{ // 5
						// an error message will have been shown, so just log the failure
						pApp->LogUserAction(_T("SetupForKBServer(2) failed in AuthenticateCheckAndSetupKBSharing() "));
						pApp->m_bIsGlossingKBServerProject = FALSE; // no option but to turn it off
						// Tell the user
						wxString title = _("Setup failed");
						wxString msg =
_("The attempt to share the glossing knowledge base failed.\nYou can continue working, but sharing of of this glossing knowledge base will not happen.");
                        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                        pApp->m_bUserDlgOrMessageRequested = TRUE;
                        wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
						pApp->m_bLoginFailureErrorSeen = TRUE;
						bSetupKBserverFailed = TRUE;
					} // 4
				} // 3
				// ensure sharing starts off enabled
				if (pApp->GetKbServer(1) != NULL && !bSetupKBserverFailed)
				{ // 4
					// Success if control gets to this line
					pApp->GetKbServer(1)->EnableKBSharing(TRUE);
					pApp->m_bUserLoggedIn = TRUE;

					wxLogDebug(_T("helpers.cpp line=%d: AuthenticateCheckAndSetupKB Sharing, m_ipAdds_Hostnames entry count = %d"),
						__LINE__, pApp->m_ipAddrs_Hostnames.GetCount());

					// Don't proceed to store it if the same ipAddr is already stored within the array
					if (IsIpAddrStoreable(&pApp->m_ipAddrs_Hostnames, pApp->m_strKbServerIpAddr))
					{
						wxLogDebug(_T("helpers.cpp line=%d: AuthenticateCheckAndSetupKB Sharing, m_ipAdds_Hostnames entry count = %d"),
							__LINE__, pApp->m_ipAddrs_Hostnames.GetCount());
						// Since the ipAddr is okay, construct the composite string and .Add() it to the
						// app's m_ipAddrs_Hostnames array
						// BEW 6Apr16, make composite:  <ipaddr>@@@<hostname> to pass back to
						// the CServiceDiscovery instance
						wxString composite = pApp->m_strKbServerIpAddr;
						wxString defaultHostname = _("unknown");
						wxString ats = _T("@@@");
						composite += ats + defaultHostname;
						pApp->m_ipAddrs_Hostnames.Add(composite);

						wxLogDebug(_T("helpers.cpp line=%d: AuthenticateCheckAndSetupKB Sharing, m_ipAdds_Hostnames entry count = %d"),
							__LINE__, pApp->m_ipAddrs_Hostnames.GetCount());
					}
				} // 3
				if (pApp->GetKbServer(2) != NULL && !bSetupKBserverFailed)
				{ // 4
					pApp->GetKbServer(2)->EnableKBSharing(TRUE);
					pApp->m_bUserLoggedIn = TRUE;

					// Don't proceed to store it if the same ipAddr is already stored within the array
					if (IsIpAddrStoreable(&pApp->m_ipAddrs_Hostnames, pApp->m_strKbServerIpAddr))
					{
						// Since the ipAddr is okay, construct the composite string and .Add() it to the
						// app's m_ipAddrs_Hostnames array
						// BEW 6Apr16, make composite:  <ipaddr>@@@<hostname> to pass back to
						// the CServiceDiscovery instance
						wxString composite = pApp->m_strKbServerIpAddr;
						wxString defaultHostname = _("unknown");
						wxString ats = _T("@@@");
						composite += ats + defaultHostname;
						pApp->m_ipAddrs_Hostnames.Add(composite);
					}
				} // 3

				} // 2 // end of TRUE block for test: if (!bSimulateUserCancellation)

			} // 1 // end of TRUE block for test: if (dlg.ShowModal() == wxID_OK)

			else if (dlgReturnCode == wxID_CANCEL)
			{ // 2
				// User Cancelled the authentication, so the old ipAddr, username and
				// password have been restored to their storage in the app and
				// frame window instance; so it remains only to restore the old
				// flag values. TRUE param is bJustRestore (the ipAddr, username and
				// password). The function always sets m_bIsKBServerProject and
				// m_bIsGlossingKBServerProject to FALSE
				/* BEW 25Sep20 deprecated, we no longer investigate lang codes
				   HandleBadLangCodeOrCancel(pApp->m_saveOldIpAddrStr, pApp->m_saveOldHostnameStr,
					pApp->m_saveOldUsernameStr, pApp->m_savePassword,
					pApp->m_saveSharingAdaptationsFlag, pApp->m_saveSharingGlossesFlag, TRUE);
				*/
				bSetupKBserverFailed = TRUE;  // this line formerly had label  bad2: which is now unreferenced, so removed
				pApp->m_bAuthenticationCancellation = TRUE;
				pApp->m_bUserLoggedIn = FALSE;
			} // 1
			else
			{  // 2
				// User clicked OK button but in the OnOK() handler, premature return was
				// asked for most likely due to an empty password submitted or a Cancel
				// from within one of the lower level calls, or there was a error saying
				// that the connection could not be made, etc. So allow retry, or a change
				// to the settings, or a Cancel button press at this level instead (the
				// Authenticate dialog includes a Cancel button, pressing
				// it makes the project not be a sharing one for adapting or glossing kbs)
				goto here2; // <<-- back to showing the dialog window
			} // 1

			// Do the feedback to the user with the special wait dialogs here
			if (bSimulateUserCancellation || bSetupKBserverFailed)
			{
				// There was an error, and sharing was turned off
				ShortWaitSharingOff(); //displays "Knowledge base sharing is OFF" for 1.3 seconds
				pApp->m_bUserLoggedIn = FALSE;
				return FALSE;
			}
			else
			{
				// No error, authentication and setup succeeded
				ShortWait();  // shows "Connected to KBserver successfully"
							  // for 1.3 secs (and no title in titlebar)
			}
			pApp->m_bUserLoggedIn = TRUE;
			return TRUE;
		} // end of TRUE block for test: if (!bServiceDiscoveryWanted)
		else // service discovery is wanted
		{
			// BEW 11Jan16. This is the appropriate place for having
			// ConnectUsingDiscoveryResults() and its subsequent code. Prior to this, the flag or
			// flags being TRUE meant that an ipAddr was stored on the basic config file
			// (all other KBserver related config params are in the project config
			// file) - so all we needed to do here was give the correct password in the
			// password dialog. But that assumed the ipAddr is still the same (and that
			// assumption is easily violated by other computers being added or removed
			// from the LAN). Putting service discovery here will catch the changes,
			// and give the user guidance about how to proceed to connect; and if an
			// automatic connection is possible (it should be if the ipAddr has not
			// changed, and there is a stored password available within this session)
			// then it can be done. Any failures should not result in a return from
			// this wizard function, but just cancellation of the sharing setup.
			// We need a few booleans so that if an error happens, we can skip code
			// that should not be attempted subsequently
			bool bServiceDiscoverySucceeded = TRUE; // initialize to TRUE, even though
										// service discovery is yet to be attempted
			bool bSimulateUserCancellation = FALSE; // initialize
			bool bSetupKBserverFailed = FALSE; // initialize

			// ConnectUsingDiscoveryResults() internally creates an instantiation of the
			// ServiceDiscovery class. The pointer CAdapt_ItApp::m_pServDisc points
			// to the ServiceDiscovery instance and is non-NULL while the service discovery
			// runs, but when it is shut down, that pointer needs to again be set to NULL
			wxString curIpAddr = pApp->m_strKbServerIpAddr;
			wxString chosenIpAddr = _T("");
			wxString chosenHostname = _T("");
			enum ServDiscDetail returnedValue = SD_NoResultsYet;
			bool bOK = pApp->ConnectUsingDiscoveryResults(curIpAddr, chosenIpAddr, chosenHostname, returnedValue);
			if (bOK)
			{
				// Got an ipAddress to connect to
				wxASSERT((returnedValue != SD_NoKBserverFound) && (returnedValue != SD_ServiceDiscoveryError) && (
					returnedValue == SD_SameIpAddrAsInConfigFile ||
					returnedValue == SD_IpAddrDiffers_UserAcceptedIt ||
					returnedValue == SD_SingleIpAddr_UserAcceptedIt ||
					returnedValue == SD_MultipleIpAddr_UserChoseOne));

				// Make the chosen ipAddr accessible to authentication (this is the hookup location
				// of the service discovery's ipAddr to the earlier KBserver GUI code) for this situation
				pApp->m_strKbServerIpAddr = chosenIpAddr;
				pApp->m_strKbServerHostname = chosenHostname;
				if (!pApp->m_strUserID.IsEmpty() && returnedValue == SD_SameIpAddrAsInConfigFile)
				{
					// If same ipAddress, then autoconnect if there is a password stored;
					// if no stored password, then just ask for that.
					bShowIpAddrAndUsernameDlg = FALSE; // no need for it
					if (bPasswordExists)
					{
						bShowPasswordDlgOnly = FALSE;
						bAutoConnectKBSvr = TRUE;
					}
					else
					{
						bShowPasswordDlgOnly = TRUE;
						bAutoConnectKBSvr = FALSE;
					}
				}
				else if (returnedValue == SD_IpAddrDiffers_UserAcceptedIt ||
						 returnedValue == SD_SingleIpAddr_UserAcceptedIt ||
					     returnedValue == SD_MultipleIpAddr_UserChoseOne)
				{
					// show Authenticate dlg, with ipAddr & username dlg & password field
					bShowIpAddrAndUsernameDlg = TRUE;
					bShowPasswordDlgOnly = FALSE;
				}
			} // end of TRUE block for test: if (bOK)
			else
			{
				// Something is wrong, or no KBserver has yet been set running; or what's running
				// is not the one the user wants to connect to (treat this as same as a
				// cancellation), or user cancelled, etc
				wxASSERT(returnedValue == SD_NoResultsYet ||
					returnedValue == SD_NoKBserverFound ||
					returnedValue == SD_SingleIpAddr_UserCancelled ||
					returnedValue == SD_ServiceDiscoveryError ||
					returnedValue == SD_NoResultsAndUserCancelled ||
					returnedValue == SD_SingleIpAddr_ButNotChosen ||
					returnedValue == SD_MultipleIpAddr_UserCancelled ||
					returnedValue == SD_MultipleIpAddr_UserChoseNone ||
					returnedValue == SD_ValueIsIrrelevant
					);
				pApp->m_bLoginFailureErrorSeen = TRUE;
                // If nothing was found, there may be several reasons. Advise the user.
				// But a connection at this point cannot be had from service discovery
				// results, so turn sharing off. User can try again, or can type an ipAddr
				// manually after electing not to use service discovery.
				if (returnedValue == SD_NoKBserverFound || returnedValue == SD_NoResultsYet)
				{
					// Defeat! Tell use what might be the problem (be sure to leave
					// pApp->m_strKbServerIpAddr unchanged)
					wxString error_msg = _("No KBserver is running on the local area network yet. Or possibly you forgot to set a KBserver running. Or maybe the machine hosting the KBserver has lost power.\nKnowledge Base sharing will now be turned off.\nFirst get a KBserver running, and then try again to connect to it.\n If necessary, ask your administrator to help you.");
                    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                    pApp->m_bUserDlgOrMessageRequested = TRUE;
                    wxMessageBox(error_msg, _("A KBserver was not discovered"), wxICON_WARNING | wxOK);
				} // end of TRUE block for test: if (returnedValue == SD_NoKBserverFound)

				// An error message will have been seen already; so just treat this as a cancellation
				pApp->ReleaseKBServer(1); // the adapting one
				pApp->ReleaseKBServer(2); // the glossing one
				pApp->m_bIsKBServerProject = FALSE;
				pApp->m_bIsGlossingKBServerProject = FALSE;

				// Restore the earlier settings for ipAddr, username & password
				pApp->m_strKbServerIpAddr = pApp->m_saveOldIpAddrStr;
				pApp->m_strKbServerHostname = pApp->m_saveOldHostnameStr;
				pApp->m_strUserID = pApp->m_saveOldUsernameStr;
				pApp->GetMainFrame()->SetKBSvrPassword(pApp->m_savePassword);

				bServiceDiscoverySucceeded = FALSE;
				pApp->m_bUserLoggedIn = FALSE;

				ShortWaitSharingOff(); //displays "Knowledge base sharing is OFF" for 1.3 seconds
				return FALSE;
			} // end of else block for test: if(bOK)

			// *** End of the block of service discovery code. Results will be carried
			// forward into the following legacy code, using booleans ***

			if (bServiceDiscoverySucceeded)
			{
				if (bShowIpAddrAndUsernameDlg == TRUE)
				{
					// Authenticate to the server. Authentication also chooses, via the ipAddr provided or
					// typed, which particular KBserver we connect to - there may be more than one available
					KBSharingAuthenticationDlg dlg(pFrame, pApp->m_bUserAuthenticating);
					dlg.Center();
					int dlgReturnCode;
here:				dlgReturnCode = dlg.ShowModal();
					if (dlgReturnCode == wxID_OK)
					{
						if (dlg.m_bError)
						{
							pApp->m_bLoginFailureErrorSeen = TRUE;
							pApp->m_bUserLoggedIn = FALSE;
							gpApp->LogUserAction(_T("AuthenticationCheckAndSetupKBSharing() error. Bad ipAddr or username lookup failure"));
							goto bad; // An error was seen, so just treat as a Cancel, bad: is below
						}
						// Since KBSharingSetup.cpp uses the above KBSharingstatelessSetupDlg, we
						// have to ensure that MainFrms's m_kbserverPassword member is set. Also...
						// Check that needed language codes are defined for source, target, and if
						// a glossing kb share is also wanted, that source and glosses codes are
						// set too. Get them set up if not so. If user cancels, don't go ahead with
						// the setup, and in that case if app's m_bIsKBServerProject is TRUE, make
						// it FALSE, and likewise for m_bIsGlossingKBServerProject if relevant
						bool bUserCancelled = FALSE;

						// We want valid codes for source and target if sharing the adaptations KB, and
						// for source and glosses languages if sharing the glossing KB. (CheckLanguageCodes
						// is in helpers.h & .cpp) We'll start by testing adaptations KB, if that is
						// wanted. Then again for glossing KB if that is wanted (usually it won't be)
						bool bDidFirstOK = TRUE;
						bool bDidSecondOK = TRUE;
						if (m_bSharingAdaptations)
						{
							bDidFirstOK = CheckLanguageCodes(TRUE, TRUE, FALSE, FALSE, bUserCancelled);
							if (!bDidFirstOK || bUserCancelled)
							{
								// We must assume the src/tgt codes are wrong or incomplete, or that the
								// user has changed his mind about KB Sharing being on - so turn it off
								/* BEW 25Sep20 deprecated, we no longer call this
								HandleBadLangCodeOrCancel(pApp->m_saveOldIpAddrStr, pApp->m_saveOldHostnameStr,
									pApp->m_saveOldUsernameStr, pApp->m_savePassword,
									pApp->m_saveSharingAdaptationsFlag, pApp->m_saveSharingGlossesFlag);
								pApp->m_bLoginFailureErrorSeen = TRUE;
								*/
								bSimulateUserCancellation = TRUE;
							}
						}
						// Now, check for the glossing kb code, if that kb is to be shared
						bUserCancelled = FALSE; // re-initialize
						if (m_bSharingGlosses && !bSimulateUserCancellation)
						{
							bDidSecondOK = CheckLanguageCodes(TRUE, FALSE, TRUE, FALSE, bUserCancelled);
							if (!bDidSecondOK || bUserCancelled)
							{
								// We must assume the src/gloss codes are wrong or incomplete, or that the
								// user has changed his mind about KB Sharing being on - so turn it off
								/* BEW 25Sep20 deprecated, we no longer call this
								HandleBadGlossingLangCodeOrCancel(pApp->m_saveOldIpAddrStr,
									pApp->m_saveOldHostnameStr, pApp->m_saveOldUsernameStr,
									pApp->m_savePassword, pApp->m_saveSharingAdaptationsFlag,
									pApp->m_saveSharingGlossesFlag);
								pApp->m_bLoginFailureErrorSeen = TRUE;
								*/
								bSimulateUserCancellation = TRUE;
							}
						}

						// If control gets to here, we can go ahead and establish the
						// setup(s), provided bSimulateUserCancellation is not TRUE
						if (!bSimulateUserCancellation)
						{
							// Shut down the old settings, and reestablish connection using the new
							// settings (this may involve an ipAddr change to share using a different KBserver)
							pApp->ReleaseKBServer(1); // the adaptations one
							pApp->ReleaseKBServer(2); // the glossing one
							pApp->m_bIsKBServerProject = FALSE;
							pApp->m_bIsGlossingKBServerProject = FALSE;

							// Do the setup or setups; use bSetupKBserverFailed = TRUE to carry
							// forward any error state, and skip functions that cannot succeed
							if (m_bSharingAdaptations)
							{
								// We want to share the local adaptations KB
								pApp->m_bIsKBServerProject = TRUE;
								if (!pApp->SetupForKBServer(1)) // try to set up an adapting KB share
								{
									// an error message will have been shown, so just log the failure
									pApp->LogUserAction(_T("SetupForKBServer(1) failed in AuthenticateCheckAndSetupKBSharing()  at line 11747"));
									pApp->m_bIsKBServerProject = FALSE; // no option but to turn it off
									// Tell the user
									wxString title = _("Setup failed");
									wxString msg = _("The attempt to share the adaptations knowledge base failed.\nYou can continue working, but sharing of this knowledge base will not happen.");
                                    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                                    pApp->m_bUserDlgOrMessageRequested = TRUE;
                                    wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
									pApp->m_bLoginFailureErrorSeen = TRUE;
									bSetupKBserverFailed = TRUE;
									pApp->m_bUserLoggedIn = FALSE;
								}
							}
							if (m_bSharingGlosses && !bSetupKBserverFailed)
							{
								// We want to share the local glossing KB
								pApp->m_bIsGlossingKBServerProject = TRUE;
								if (!pApp->SetupForKBServer(2)) // try to set up a glossing KB share
								{
									// an error message will have been shown, so just log the failure
									pApp->LogUserAction(_T("SetupForKBServer(2) failed in AuthenticateCheckAndSetupKBSharing()  at line 11919"));
									pApp->m_bIsGlossingKBServerProject = FALSE; // no option but to turn it off
									// Tell the user
									wxString title = _("Setup failed");
									wxString msg = _("The attempt to share the glossing knowledge base failed.\nYou can continue working, but sharing of of this glossing knowledge base will not happen.");
                                    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                                    pApp->m_bUserDlgOrMessageRequested = TRUE;
                                    wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
									pApp->m_bLoginFailureErrorSeen = TRUE;
									bSetupKBserverFailed = TRUE;
									pApp->m_bUserLoggedIn = FALSE;
								}
							}
							// ensure sharing starts off enabled
							if (pApp->GetKbServer(1) != NULL && !bSetupKBserverFailed)
							{
								// Success if control gets to this line
								pApp->GetKbServer(1)->EnableKBSharing(TRUE);
								pApp->m_bUserLoggedIn = TRUE;
							}
							if (pApp->GetKbServer(2) != NULL && !bSetupKBserverFailed)
							{
								pApp->GetKbServer(2)->EnableKBSharing(TRUE);
								pApp->m_bUserLoggedIn = TRUE;
							}

						} // end of TRUE block for test: if (!bSimulateUserCancellation)

					} // end of TRUE block for test: if (dlg.ShowModal() == wxID_OK) -- for authenticating

					else if (dlgReturnCode == wxID_CANCEL)
					{
						// User Cancelled the authentication, so the old ipAddr, username and
						// password have been restored to their storage in the app and
						// frame window instance; so it remains only to restore the old
						// flag values. TRUE param is bJustRestore (the ipAddr, username and
						// password). The function always sets m_bIsKBServerProject and
						// m_bIsGlossingKBServerProject to FALSE
						/* BEW 25Sep20 deprecated, we no longer call this
						HandleBadLangCodeOrCancel(pApp->m_saveOldIpAddrStr, pApp->m_saveOldHostnameStr,
						pApp->m_saveOldUsernameStr, pApp->m_savePassword,
						pApp->m_saveSharingAdaptationsFlag, pApp->m_saveSharingGlossesFlag, TRUE);
						*/
bad:					bSetupKBserverFailed = TRUE;
						pApp->m_bAuthenticationCancellation = TRUE;
						pApp->m_bUserLoggedIn = FALSE;
					}
					else
					{
						// User clicked OK button but in the OnOK() handler, premature return was
						// asked for most likely due to an empty password submitted or a Cancel
						// from within one of the lower level calls, or there was a error, saying
						// that the connection could not be made, etc. So allow retry, or a change
						// to the settings, or a Cancel button press at this level instead (the
						// Authenticate dialog includes a Cancel button, pressing
						// it makes the project not be a sharing one for adapting or glossing kbs)
						goto here; // <<-- back to the showing of the dialog
					}

				} // end of TRUE block for test: if (bShowUrlAndUsernameDlg == TRUE)
				else
				{
					// The Authentication dialog (with ipAddr and username) does not need to be shown. So
					// we either just show the password dialog (if no password yet is stored), or
					// autoconnect (if a password is stored already - this latter option is only offered
					// when we know that the config file's ipAddr is the same as what was just created from
					// the service discovery results - in this situation, we can pretty safely assume
					// that the stored password applies)

					// Control would get here if the "Setup or Remove Knowledge Base Sharing" menu command
					// is clicked a second time, to change the settings (eg. turn on sharing of glossing
					// KB, or some change - such as turning off sharing to one of the KB types)
					wxString theIpAddr = pApp->m_strKbServerIpAddr;
					wxString theHostname = pApp->m_strKbServerHostname;
					wxString theUsername = pApp->m_strUserID;

					wxString thePassword;
					bool bUserCancelled = FALSE;
					if (bPasswordExists && bAutoConnectKBSvr)
					{
						// The ipAddr, username and password are all in existence and known, so autoconnect
						thePassword = pApp->GetMainFrame()->GetKBSvrPassword();
					}
					else if (bShowPasswordDlgOnly)
					{
						// The password is not stored, so we must ask for it - insist on something
						// being typed in
						// show the password dialog
						thePassword = pApp->GetMainFrame()->GetKBSvrPasswordFromUser(theIpAddr, theHostname);

						if (thePassword.IsEmpty())
						{
							wxString title = _("No password was typed, or you cancelled");
							wxString msg = _(
								"No password was typed in, or you chose to Cancel. This will cancel the attempt to Authenticate. You can try again later.");
                            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                            pApp->m_bUserDlgOrMessageRequested = TRUE;
                            wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
							pApp->m_bLoginFailureErrorSeen = TRUE;
							bUserCancelled = TRUE;
							pApp->m_bUserLoggedIn = FALSE;
						}
						else
						{
							// Whatever was typed has to be stored in CMainFrame::m_kbserverPassword so
							// that GetKbServer{0] or [1] can access it in the setup
							pApp->GetMainFrame()->SetKBSvrPassword(thePassword);
							wxLogDebug(_T("AuthenticateCheckAndSetupKBSharing(): the typed password was stored in the CMainFrame instance"));
						}
					}
					if (bUserCancelled)
					{
						bSimulateUserCancellation = TRUE;
					}

					// We want valid codes for source and target if sharing the adaptations
					// KB, and for source and glosses languages if sharing the glossing KB.
					// (CheckLanguageCodes is in helpers.h & .cpp) We'll start by testing
					// adaptations KB, if that is wanted. Then again for glossing KB if
					// that is wanted (usually it won't be)
					bool bDidFirstOK = TRUE;
					bool bDidSecondOK = TRUE;
					if (m_bSharingAdaptations && !bSimulateUserCancellation)
					{
						bDidFirstOK = CheckLanguageCodes(TRUE, TRUE, FALSE, FALSE, bUserCancelled);
						if (!bDidFirstOK || bUserCancelled)
						{
							// We must assume the src/tgt codes are wrong or incomplete, or
							// that the user has changed his mind about KB Sharing being on
							// - so turn it off. The function clears m_bIsKBServerProject
							// and m_bIsGlossingKBServerProject to FALSE
							/* BEW 25Sep20 deprecated, we no longer call this
							HandleBadLangCodeOrCancel(pApp->m_saveOldIpAddrStr, pApp->m_saveOldHostnameStr,
								pApp->m_saveOldUsernameStr, pApp->m_savePassword,
								pApp->m_saveSharingAdaptationsFlag, pApp->m_saveSharingGlossesFlag);
							pApp->m_bLoginFailureErrorSeen = TRUE;
							*/
							bSimulateUserCancellation = TRUE;
							pApp->m_bUserLoggedIn = FALSE;
						}
					}
					// Now, check for the glossing kb code, if that kb is to be shared
					bUserCancelled = FALSE; // re-initialize
					if (m_bSharingGlosses && !bSimulateUserCancellation)
					{
						bDidSecondOK = CheckLanguageCodes(TRUE, FALSE, TRUE, FALSE, bUserCancelled);
						if (!bDidSecondOK || bUserCancelled)
						{
							// We must assume the src/gloss codes are wrong or incomplete, or that the
							// user has changed his mind about KB Sharing being on - so turn it off
							/* BEW 25Sep20 deprecated, we no longer call this
							HandleBadGlossingLangCodeOrCancel(pApp->m_saveOldIpAddrStr, pApp->m_saveOldHostnameStr,
								pApp->m_saveOldUsernameStr, pApp->m_savePassword,
								pApp->m_saveSharingAdaptationsFlag, pApp->m_saveSharingGlossesFlag);
							pApp->m_bLoginFailureErrorSeen = TRUE;
							*/
							bSimulateUserCancellation = TRUE;
							pApp->m_bUserLoggedIn = FALSE;
						}
					}
					// If control gets to here without error, we can go ahead and establish the setup(s)

					if (!bSimulateUserCancellation)
					{

						// Shut down the old settings, and reestablish connection using the new
						// settings (this may involve an iAddr change to share using a different KBserver)
						pApp->ReleaseKBServer(1); // the adaptations one
						pApp->ReleaseKBServer(2); // the glossing one
						pApp->m_bIsKBServerProject = FALSE;
						pApp->m_bIsGlossingKBServerProject = FALSE;

						// Do the setup or setups
						if (m_bSharingAdaptations)
						{
							// We want to share the local adaptations KB
							pApp->m_bIsKBServerProject = TRUE;
							if (!pApp->SetupForKBServer(1)) // try to set up an adapting KB share
							{
								// an error message will have been shown, so just log the failure
								pApp->LogUserAction(_T("SetupForKBServer(1) failed in AuthenticateCheckAndSetupKBSharing()  at line 12093"));
								pApp->m_bIsKBServerProject = FALSE; // no option but to turn it off
								// Tell the user
								wxString title = _("Setup failed");
								wxString msg = _(
									"The attempt to share the adaptations knowledge base failed.\nYou can continue working, but sharing of this knowledge base will not happen.");
                                // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                                pApp->m_bUserDlgOrMessageRequested = TRUE;
                                wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
								pApp->m_bLoginFailureErrorSeen = TRUE;
								bSetupKBserverFailed = TRUE;
								pApp->m_bUserLoggedIn = FALSE;
							}
						}
						if (m_bSharingGlosses && !bSetupKBserverFailed)
						{
							// We want to share the local glossing KB
							pApp->m_bIsGlossingKBServerProject = TRUE;
							if (!pApp->SetupForKBServer(2)) // try to set up a glossing KB share
							{
								// an error message will have been shown, so just log the failure
								pApp->LogUserAction(_T("SetupForKBServer(2) failed in AuthenticateCheckAndSetupKBSharing() at line 12114"));
								pApp->m_bIsGlossingKBServerProject = FALSE; // no option but to turn it off
								// Tell the user
								wxString title = _("Setup failed");
								wxString msg = _(
									"The attempt to share the glossing knowledge base failed.\nYou can continue working, but sharing of of this glossing knowledge base will not happen.");
                                // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                                pApp->m_bUserDlgOrMessageRequested = TRUE;
                                wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
								pApp->m_bLoginFailureErrorSeen = TRUE;
								bSetupKBserverFailed = TRUE;
								pApp->m_bUserLoggedIn = FALSE;
							}
						}
						// ensure sharing starts off enabled; provided there was no error
						if (pApp->GetKbServer(1) != NULL && !bSetupKBserverFailed)
						{
							// Success if control gets to this line
							pApp->GetKbServer(1)->EnableKBSharing(TRUE);
							pApp->m_bUserLoggedIn = TRUE;
						}
						if (pApp->GetKbServer(2) != NULL && !bSetupKBserverFailed)
						{
							pApp->GetKbServer(2)->EnableKBSharing(TRUE);
							pApp->m_bUserLoggedIn = TRUE;
						}

					} // end of TRUE block for test: if (!bSimulateUserCancellation)

				} //end of else block for test: if (bShowIpAddrAndUsernameDlg == TRUE)

			} // end of TRUE block for test: if (bServiceDiscoverySucceeded)

			// Do the feedback to the user with the special wait dialogs here
			if (!bServiceDiscoverySucceeded || bSimulateUserCancellation || bSetupKBserverFailed)
			{
				// There was an error, and sharing was turned off
				ShortWaitSharingOff(); //displays "Knowledge base sharing is OFF" for 1.3 seconds
				pApp->m_bUserLoggedIn = FALSE;
				return FALSE;
			}
			else
			{
				// No error, authentication and setup succeeded
				pApp->m_bUserLoggedIn = TRUE;
				ShortWait();  // shows "Connected to KBserver successfully"
							  // for 1.3 secs (and no title in titlebar)
			}
		} // end of else block for test: if (!bServiceDiscoveryWanted), i.e. it was wanted

	} // end of TRUE block for test: if (pApp->m_bIsKBServerProject || pApp->m_bIsGlossingKBServerProject)
	else
	{
		// If not either type of sharing is wanted, set up nothing, sharing is OFF
		// Don't call ShortWaitSharingOff(20) here, because if the user has not been
		// using KB sharing, he does not need to be informed that it is off
		pApp->m_bUserLoggedIn = FALSE;
		return FALSE;
	} // end of else block for test: if (pApp->m_bIsKBServerProject || pApp->m_bIsGlossingKBServerProject)

	pApp->m_bUserLoggedIn = TRUE;
	return TRUE;
}

//#endif

// Support for ZWSP insertion in any AI wxTextCtrl
void OnCtrlShiftSpacebar(wxTextCtrl* pTextCtrl)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	if (!pApp->m_bEnableZWSPInsertion)
		return;
	wxChar zwsp = (wxChar)0x200B;
	long curPos;
	long from;
	long to;
	pTextCtrl->GetSelection(&from, &to);
	// if there is a selection, the insertion should replace it before doing a
	// ZWSP insertion there; otherwise, we'll just have an insertion point to
	// insert ZWSP at that spot
	if (from != to)
	{
		pTextCtrl->Remove(from, to); // we now have just an insertion point at index to
	}
	curPos = pTextCtrl->GetInsertionPoint();
	wxASSERT(curPos <= pTextCtrl->GetLastPosition());
	wxString contents = pTextCtrl->GetValue();
	wxString left = contents.Left(curPos);
	wxString right = contents.Mid(curPos);
	left += zwsp;
	contents = left + right;
	pTextCtrl->ChangeValue(contents);
	pTextCtrl->SetSelection(curPos+1, curPos+1);
	pTextCtrl->SetFocus();
}

// A global function for doing any normalization operations necessary because a major
// change is about to be done (such as changing doc, changing project, exit from app,
// something requiring doc to be loaded and in a robust state, etc)
void NormalizeState()
{
	CAdapt_ItApp* pApp = &wxGetApp();

	// The user may have just used the clipboard adaptation functionality, and forgotten
	// to close it with Close button before attempting something which requires the cached
	// document to have been restored, or the adapted fragment to not be treated as a
	// valid doc - so force that mode to be closed
	if (pApp->m_bClipboardAdaptMode)
	{
		// The mode is still on. Turn it off, remove the bar, restore the doc or empty
		// view if no doc was loaded. The call will set the above flag to FALSE as well.
		wxCommandEvent dummyEvt;
		pApp->OnButtonCloseClipboardAdaptDlg(dummyEvt);
	}
	// Any other such operations, add below...
}

// Gets the contents of m_srcWordBreak wxString  member of the passed in pSrcPhrase
wxString PutSrcWordBreak(CSourcePhrase* pSrcPhrase)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	if (pApp->m_bUseSrcWordBreak)
	{
		// return whatever character m_srcWordBreak stores, but make it intelligent
		// because it may store CR + LF, and in mergers, retranslations, composing
		// default free translations, doing retranslation reports, we actually don't
		// want any CR or CR+LF being returned. So we'll leave CR+LF stored in the
		// CSourcePhrase, but our Put...() functions will check what's in the string
		// and reduce CR or LF or CR+LF to just a latin space, and then check if there
		// is anything after the latin space and if so, remove that - whatever character
		// or characters it may happen to be. This algorithm will work fine if all that
		// is present is a character such as ZWSP or one of its friends. We'll also
		// change tab to space.
		wxString s = pSrcPhrase->GetSrcWordBreak();
		wxString output; output.Empty();
		// BEW 29Oct22 add protection for Get Char(0)
		if (!s.IsEmpty() && (s.GetChar(0) == _T('\r') || (s.GetChar(0) == _T('\n')) || s.GetChar(0) == _T('\t')) )
		{
			// BEW 15Apr23 if s is '\n' then the TRUE block is entered, but then only space is returned
			// unless a test is explicitly made in the TRUE block for newline. Add that code.
			if (s == _T("\n"))
			{
				output = _T("\n");
			}
			else
			{
				output = _T(" ");
			}
			return output;
		}
		return s;
	}
	else
	// Note: if the CSourcePhrase stores nothing yet in this member,
	// GetSrcWordBreak() will unilaterally return only a normal latin space
	{
		// return a latin space
		return wxString(_T(" "));
	}
}
// Gets the contents of m_tgtWordBreak wxString  member of the passed in pSrcPhrase;
// the internal test uses m_bUseSrcWordBreak -- this is not an error, that flag covers for
// both situations where tgt text is wanted (only in a retranslation), and src text is
// wanted - which is everywhere else, as well as the source text reconstitution within a
// retranslation
wxString PutTgtWordBreak(CSourcePhrase* pSrcPhrase)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	if (pApp->m_bUseSrcWordBreak)
	{
		// return whatever character or string m_srcWordBreak stores - but intelligently,
		// we don't want CR, or LF, or TAB, or CR+LF, put into a retranslation
		wxString s = pSrcPhrase->GetTgtWordBreak();
		wxString output; output.Empty();
		// BEW 29Oct22 added protection for Get Char(0)
		if (!s.IsEmpty() && (s.GetChar(0) == _T('\r') || s.GetChar(0) == _T('\n') || s.GetChar(0) == _T('\t')) )
		{
			output = _T(" ");
			return output;
		}
		return s;
	}
	// Note: if the CSourcePhrase stores nothing yet in this member,
	// GetTgtWordBreak() will unilaterally return only a normal latin space
	else
	{
		// return a latin space
		return wxString(_T(" "));
	}
}
// Get it from m_srcWordBreak in pSrcPhrase, or return a latin space if app's
// m_bZWSPinDoc is FALSE
wxString PutSrcWordBreakFrTr(CSourcePhrase* pSrcPhrase)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	if (pApp->m_bZWSPinDoc)
	{
		// return whatever character or string m_srcWordBreak stores
		return pSrcPhrase->GetSrcWordBreak();
	}
	else
	{
		// return a latin space
		return wxString(_T(" "));
	}
}
//#if defined(_KBSERVER)
void ShortWait()
{
	CAdapt_ItApp* pApp = &wxGetApp();
	pApp->m_pWaitDlg = new CWaitDlg(pApp->GetMainFrame(), TRUE);
	pApp->m_pWaitDlg->m_nWaitMsgNum = 24;	// 24 is "Connected to KBserver successfully"
	pApp->m_pWaitDlg->Centre();
	pApp->m_pWaitDlg->Show(TRUE);
	pApp->m_pWaitDlg->Update();
	pApp->m_msgShownTime = wxDateTime::Now();
	pApp->m_pWaitDlg->Raise(); // send to top of z-order
}

void ShortWaitSharingOff()
{
	CAdapt_ItApp* pApp = &wxGetApp();
	pApp->m_pWaitDlg = new CWaitDlg(pApp->GetMainFrame(), TRUE);
	pApp->m_pWaitDlg->m_nWaitMsgNum = 25;	// 25 is "Knowledge base sharing is OFF"
	pApp->m_pWaitDlg->Centre();
	pApp->m_pWaitDlg->Show(TRUE);
	pApp->m_pWaitDlg->Update();
	pApp->m_msgShownTime = wxDateTime::Now();
	pApp->m_pWaitDlg->Raise(); // send to top of z-order
}

bool Credentials_For_Manager(CAdapt_ItApp* pApp, wxString* pIpAddr,	wxString* pUsername,
							wxString* pPassword, wxString datFilename)
{
	// Using absolute paths...
	wxString separator = pApp->PathSeparator;
	wxString ipaddr, username, pwd, mariaHostAddr;
	ipaddr = *pIpAddr;
	username = *pUsername;
	pwd = *pPassword;
	wxString comma = _T(",");
	wxString distFolderPath = pApp->m_dataKBsharingPath; // store .dat 'input' file here // whm 22Feb2021 changed distPath to m_dataKBsharingPath, which ends with PathSeparator
	wxString datPath = distFolderPath + datFilename;

	// Check that the file already exists, if not, create it
	wxTextFile textFile; // line-oriented file of lines of text
	bool bFileExists = FALSE; // initialise
	bFileExists = wxFileName::FileExists(datPath);

	// Create the lines to be added. Last is comma-separated credentials
	// Comment lines start with # (hash)
	wxString comment1 = _T("# Usage: ipAddress,username,password,");
	wxString comment2 = _T("# Encoding: UTF-16 for Win, or UTF-32 for Linux/OSX");
	wxString comment3 = _T("# dist folder's 'input' file: ");
	comment3 += datFilename;
	wxString credentials = ipaddr + comma +username + comma + pwd + comma;

	if (!bFileExists)
	{
		// Create() must only be called when the file doesn't already exist
		textFile.Create(datPath);
		// Note textFile is empty at this point
		textFile.Open();
		bool bIsOpened = textFile.IsOpened();
		if (bIsOpened)
		{
			textFile.AddLine(comment1);
			textFile.AddLine(comment2);
			textFile.AddLine(comment3);
			textFile.AddLine(credentials);

			// Write it to the dist buffer
			bool bGoodWrite = textFile.Write();
			textFile.Close();
			if (!bGoodWrite)
			{
				return FALSE; // write error
			}
		}
		else
		{
			return FALSE; // opening error
		}
	}
	else
	{
		// file exists
		textFile.Open();
		bool bIsOpened = textFile.IsOpened();
		if (bIsOpened)
		{
			// Line numbering is 0-based
			textFile.RemoveLine(3);
			textFile.AddLine(credentials);

			// Write it to the dist buffer
			bool bGoodWrite = textFile.Write();
			textFile.Close();
			if (!bGoodWrite)
			{
				return FALSE; // write error
			}
		}
		else
		{
			return FALSE; // opening error
		}
	}
	return TRUE;
}

// BEW 23Nov20, use this one from the KB Sharing Manager, so set app's
// m_bUseForeignOption to TRUE, so that the case 1, with "add_foreign_users.dat
// as the input .dat file, can be filled from an alternative path using KB Sharing
// Manager supplied values.
bool Credentials_For_User(wxString* pIpAddr, wxString* pUsername, wxString* pFullname,
	wxString* pPassword, bool bCanAddUsers, wxString datFilename)
{
	gpApp->m_bUseForeignOption = TRUE; // ConfigureMovedDatFile() will therefore
			// get its values from the open users Page in KBSharingManagerTabbedDlg()

	// Using absolute paths...
	wxString separator = gpApp->PathSeparator;
	wxString ipaddr, username, fullname, pwd;
	ipaddr = *pIpAddr;
	username = *pUsername;
	fullname = *pFullname;
	pwd = *pPassword;
	wxString useradmin = bCanAddUsers ? _T("1") : _T("0");

	wxString comma = _T(",");
	wxString distFolderPath = gpApp->m_dataKBsharingPath; // store .dat 'input' file here // whm 22Feb2021 changed distPath to m_dataKBsharingPath, which ends with PathSeparator
	wxString datPath = distFolderPath + datFilename;

	// Check that the file already exists, if not, create it
	wxTextFile textFile; // line-oriented file of lines of text
	bool bFileExists = FALSE; // initialise
	bFileExists = wxFileName::FileExists(datPath);

	// Create the lines to be added. Last is comma-separated credentials
	// Comment lines start with # (hash)
	wxString comment1 = _T("# Usage: ipAddress,username,password,bCanAddUsers");
	wxString comment2 = _T("# Encoding: UTF-16 for Win, or UTF-32 for Linux/OSX");
	wxString comment3 = _T("# dist folder's 'input' file: add_foreign_users.dat");
	wxString comment4 = _T("# Default useradmin value is to be TRUE");
	wxString credentials = ipaddr + comma + username + comma + fullname + comma
							+ pwd + comma + useradmin + comma;
	// BEWARE, don't try escaping any ' in credentials above, the pwd may contain one
	// and should not be changed. Do each field separately, if the field needs it
	if (!bFileExists)
	{
		textFile.Create(datPath);
		// Note textFile is empty at this point
		textFile.Open();
		bool bIsOpened = textFile.IsOpened();
		if (bIsOpened)
		{
			textFile.AddLine(comment1);
			textFile.AddLine(comment2);
			textFile.AddLine(comment3);
			textFile.AddLine(credentials);

			// Put the input data parameters where ConfigureMovedDatFile() can
			// grab them, from AI.cpp & .h instantiation
			wxString temp = username;
			temp = DoEscapeSingleQuote(temp); // may need escaping any '
			wxString m_temp_username = temp;

			temp = fullname;
			temp = DoEscapeSingleQuote(temp); // may need escaping any '
			wxString m_temp_fullname = temp;

			wxString m_temp_password = pwd;

			wxString m_temp_useradmin_flag = useradmin;


			// Write it to the dist buffer
			bool bGoodWrite = textFile.Write();
			textFile.Close();
			if (!bGoodWrite)
			{
				return FALSE; // write error
			}
		}
		else
		{
			return FALSE; // opening error
		}
	}
	else
	{
		// file exists
		textFile.Open(datPath);
		bool bIsOpened = textFile.IsOpened();
		if (bIsOpened)
		{
			// Line numbering is 0-based
			textFile.Clear();
			textFile.AddLine(credentials); // we only need this line

			// Write it to the dist buffer
			bool bGoodWrite = textFile.Write();
			textFile.Close();
			if (!bGoodWrite)
			{
				return FALSE; // write error
			}
		}
		else
		{
			return FALSE; // opening error
		}
	}
	return TRUE;
}


// BEW added 16Feb22 for KBserver support.  BEW deprecated, 24Feb22 - not needed, see AI.cpp 21660++
// The results .dat file (e.g. list_users_results.dat) no longer is guaranteed to have a "success" 
// string in it. Instead, there could be one or more data lines, each with comma-separated fields,
// and the number of commas may differ for different results files. So pass in a minimum count
// which must be present, for TRUE to be returned to the caller. If less than that are present,
// return FALSE. (Good idea if FALSE is returned, for the caller to at least have msg in LogUserAction())
/*  Retain, in case it becomes important for something else - it's not needed for do_create_entry.exe
bool CountCommasForSuccess(wxString dataLine, int minCount)
{
	wxASSERT(minCount > 2); // nothing will have 2 or less comma-separated fields
	wxASSERT(!dataLine.IsEmpty());
	int length = (int)dataLine.Length();
	int index = 0; // initialise
	int count = 0;
	wxChar comma = _T(',');
	wxChar c;
	for (index = 0; index < length; index++)
	{
		c = dataLine.GetChar((int)index); // dataLine will be short, so don't need size_t
		if (c == comma)
		{
			count++;
		}
	}
	// Test for at least minCount present, that's our felicity condition
	if (count >= minCount)
	{
		return TRUE;
	}
	return FALSE;
}
*/

// BEW 30Oct20 created. A utility function, which takes str and
// internally processes to turn every single quote found, ( ' )
// into an escaped single quote (  \'  ). This is to facilitate
// inserting strings into mariaDB/kbserver mysql entry table, or
// user table, which contain internal single quotes in words,
// phrases, or names, or language names, or whatever. Our data
// for sending to a kbserver table never has a single quoted
// string, so any single quotes must be escaped to keep SQL
// requests from failing - since ' is a key-character in SQL
// requests. MySQL will take the \' escaped strings "as is",
// and so requiring un-escaping these when they are output
// back to the AI code. I'll provide functions for that too.
// The approach here goes like this. Adding the backslashes
// will change the str's buffer length if done 'in place' so
// this is not an option, because Pacific languages can have
// huge numbers of glottal stops as single quote ( ' ). Instead
// I'll make a read-only buffer, and scan forward with a pointer
// to wxChar, and provide an empty temporary wxString to accept
// the changes. When done, the passed in str is then deleted,
// and the temp wxString is returned TO THE SAME NAMED wxString
// AS WAS PASSED IN. The returned string then is safe for all
// SQL commands. (I'll also provide an override for dealing with
// a whole file of data, using this function internally.)
// BEW 24Jun22 commas within the fields causes misparsing of the SQL, and \, is not
// a valid escape sequence. The only solution appears to be a documentation one.
// In form the user of kbserver that there must not be commas within any of the
// entry's fields. If there are, wxExecute() will fail prematurely.
// I guess there needs to be a check of source and target at every StoreText() call,
// or StoreTextGoingBack(), and disallow the store with a warning message to the
// use that comma is not allowed within the source or target data.
wxString DoEscapeSingleQuote(wxString& str)
{
	size_t len = (size_t)str.Len();
	//CAdapt_ItApp* pApp = &wxGetApp();
	wxChar quote = _T('\'');
	wxChar* ptr = NULL; // initialise
	wxStringBuffer pBuffer(str, len + 1); // probably the +1 is unnecessary
	wxChar* pBufStart = pBuffer;
	wxChar* pEnd = pBufStart + len;
	wxASSERT(*pEnd == _T('\0'));
	wxChar bkslash = _T('\\');
	ptr = pBuffer;

	wxString tempStr;
	size_t longer = len + len / 5; // allow generous growth without auto lengthening
	tempStr.Alloc(longer);
	while (ptr < pEnd)
	{
		if ((*ptr != bkslash) && (*ptr != quote) )
		{
			// it's not one of these two, so send it to tempStr
			tempStr << *ptr;
			ptr++; // point at next char
		}
		else
		{
			// it's neither a back slash nor a single quote character, so
			// bleed out the combination of \ followed by ', as that is already escaped;
			if ( (*ptr == bkslash) && (*(ptr + 1) == quote) )
			{
				// single quote is already escaped
				tempStr << *ptr; ptr++;
				tempStr << *ptr; ptr++;
			}
			else
			{
				// backslash before anything which is not a quote -
				// just send it to tempStr as is, and iterate
				if (*ptr == bkslash)
				{
					tempStr << bkslash;
					ptr++;
				}
				else
				{
					// It's not a backslash, but it could be a quote - in
					// which case, we must escape it; or it could be some other
					// wxChar, in which case we just send it to tempStr
					if (*ptr == quote)
					{
						tempStr << bkslash;
						tempStr << quote;
						// we've escaped it
						ptr++;
					}
					else
					{
						// It's not a quote, so just send it to tempStr
						tempStr << *ptr;
						ptr++;
					}
				} //end of else block for test: if (*ptr == bkslash)
			} // end of else block for test: if ((*ptr == bkslash) && (*(ptr + 1) == quote))
		} // end of else block for test: if ((*ptr != bkslash) && (*ptr != quote))
	} // end of while loop: while (ptr < pEnd)

	// Clean up, returning tempStr back to the string passed in

	return tempStr;
}

// BEW 2Nov20 created. A utility function, which takes str and
// internally processes to turn every escaped single quote, ( \' )
// into an un-escaped single quote ( ' ). This is to reverse what
// DoEscapeSingleQuote() does. (see above)
wxString DoUnescapeSingleQuote(wxString& str)
{
	wxString escaped_quote = _T("\\'");
	wxString only_quote = _T("'");
	wxString tempStr = str;
	int length = (int)tempStr.Replace(escaped_quote, only_quote);
	wxUnusedVar(length);	
	return tempStr;
}

// Code for this was taken from Bill's test code in OnInit() at 28,360++. Thanks Bill
// Returns nothing. If the file did not get processed, the file is unchanged. If it
// did get processed, it is restored with same name, but with contents having
// single quotes escaped.
void DoEscapeSingleQuote(wxString pathToFolder, wxString filename)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxString separator = pApp->PathSeparator;
	int pathLen = pathToFolder.Len();
	wxChar last = pathToFolder.GetChar(pathLen - 1);
	if (last != separator)
	{
		// add the separator string
		pathToFolder += separator;
	}
	wxString fileAndPath = pathToFolder + filename;
	wxString fileBuffer = wxEmptyString;
	// now read the file into a char buffer on the heap
	//wxFileOffset fileLen;
	size_t fileLen;
	wxFile f(fileAndPath, wxFile::read);
	if (f.IsOpened())
	{
		fileLen = (size_t)f.Length(); // get the number of bytes wanted
		size_t numReadIn = 0; // initialise

		// read the raw byte data into pByteBuf (char buffer on the heap)
		char* pByteBuf = (char*)malloc(fileLen + 1); // + 1 for a null terminator
		memset(pByteBuf, 0, fileLen + 1); // fill with nulls
		numReadIn = f.Read(pByteBuf, fileLen); // get it all, but it may get less than expected
		wxASSERT(pByteBuf[fileLen] == '\0'); // should end in NULL
		wxASSERT(numReadIn == fileLen); // they should agree, if not, we want to diagnose why
		f.Close();

		// Next, turn the heap's buffer of read in data into a wxString
		fileBuffer = wxString(pByteBuf, wxConvUTF8, fileLen);
		// free the malloc buffer, don't need it, fileBuffer has the data now
		free((void*)pByteBuf);
		// Now call the string form of the DoEscapeSingleQuote(wxString& str)
		// to get all single quotes in the file's contents, escaped to  \\'
		// (if any are already escaped, they are left as is & returned)
		fileBuffer = DoEscapeSingleQuote(fileBuffer);
	}
	else
	{
		return; // nothing was done
	}
	//Now we must re-open ff for writing,and write it out
	wxFile ff(fileAndPath, wxFile::write);
	if (ff.IsOpened())
	{
		// Overwrite original contents
		ff.Seek((wxFileOffset)0); // 2nd arg, wxSeekMode, is default wxFromStart
		bool bReadOutOK = ff.Write(fileBuffer, wxConvUTF8);
		ff.Flush();
		if (bReadOutOK)
		{
			ff.Close();
		}
		else
		{
			wxString title = _("Possible loss of data");
			wxString msg = _("Writing out the file of escaped single quotes detected an error. Check your file's data is not truncated.");
			wxMessageBox(msg, title);
			pApp->LogUserAction(msg);
		}
	}
	fileBuffer.Empty();
}

// Returns nothing. If the file did not get processed, the file is unchanged. If it
// did get processed, it is restored with same name, but with contents having
// single escaped quotes un-escaped.
void DoUnescapeSingleQuote(wxString pathToFolder, wxString filename)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxString separator = pApp->PathSeparator;
	int pathLen = pathToFolder.Len();
	wxChar last = pathToFolder.GetChar(pathLen - 1);
	if (last != separator)
	{
		// add the separator string
		pathToFolder += separator;
	}
	wxString fileAndPath = pathToFolder + filename;
	wxString fileBuffer = wxEmptyString;
	// now read the file into a char buffer on the heap
	//wxFileOffset fileLen;
	size_t fileLen;
	wxFile f(fileAndPath, wxFile::read);
	if (f.IsOpened())
	{
		fileLen = (size_t)f.Length(); // get the number of bytes wanted
		size_t numReadIn = 0; // initialise

		// read the raw byte data into pByteBuf (char buffer on the heap)
		char* pByteBuf = (char*)malloc(fileLen + 1); // + 1 for a null terminator
		memset(pByteBuf, 0, fileLen + 1); // fill with nulls
		numReadIn = f.Read(pByteBuf, fileLen); // get it all, but it may get less than expected
		wxASSERT(pByteBuf[fileLen] == '\0'); // should end in NULL
		wxASSERT(numReadIn == fileLen); // they should agree, if not, we want to diagnose why
		f.Close();

		// Next, turn the heap's buffer of read in data into a wxString
		fileBuffer = wxString(pByteBuf, wxConvUTF8, fileLen);
		// free the malloc buffer, don't need it, fileBuffer has the data now
		free((void*)pByteBuf);
		// Now call the string form of the DoUnescapeSingleQuote(wxString& str)
		// to get all escaped single quotes ( \' ) in the file's contents,
		// un-escaped to ( ' )
		// (if any are already un-escaped, they are left as is & returned)
		fileBuffer = DoUnescapeSingleQuote(fileBuffer);
	}
	else
	{
		return; // nothing was done
	}
	//Now we must re-open ff for writing,and write it out
	wxFile ff(fileAndPath, wxFile::write);
	if (ff.IsOpened())
	{
		// Overwrite original contents
		ff.Seek((wxFileOffset)0); // 2nd arg, wxSeekMode, is default wxFromStart
		bool bReadOutOK = ff.Write(fileBuffer, wxConvUTF8);
		ff.Flush();
		if (bReadOutOK)
		{
			ff.Close();
		}
		else
		{
			wxString title = _("Possible loss of data");
			wxString msg = _("Writing out the file of un-escaped single quotes detected an error. Check your file's data is not truncated.");
			wxMessageBox(msg, title);
			pApp->LogUserAction(msg);
		}
	}
	fileBuffer.Empty();
}

//#endif // _KBSERVER

// Remove the subStr from inputStr and return the resulting string. Remove once
// only (default, bRemoveAll is FALSE) - the first one found. If bRemoveAll is
// explicitly TRUE, then every occurrence of subStr is removed, and the result
// returned. If there is no occurrence of subStr, then return the inputStr
// 'as is'. Likewise, if subStr is empty, the passed in inputStr is returned
// unchanged.
// BEW 30Sep19 created
wxString  RemoveSubstring(wxString inputStr, wxString subStr, bool bRemoveAll)
{
	wxString strEmpty(_T(""));
	wxString str = inputStr;
	if (str == strEmpty)
	{
		return str;
	}
	int offset = wxNOT_FOUND; // initialise
	int length = subStr.Len();
	if (length == 0)
	{
		// Nothing to search for
		return str;
	}
	wxString strLeft;
	wxString strRight;
	if (!bRemoveAll)
	{
		// Remove the first occurrence only, if such exists; if not exists,
		// then just send the string back unchanged
		offset = str.Find(subStr);
		if (offset == wxNOT_FOUND)
		{
			return str;
		}
		else
		{
			strLeft = str.Left(offset);
			offset += length; // point past the subStr
			strRight = str.Mid(offset);
			str = strLeft + strRight; // no first subStr is within it now
		}
	}
	else
	{
		// Remove All is wanted. We'll need a do loop. Exit the loop when
		// a pass thru the loop changes nothing, and return what is in str
		do {
			offset = str.Find(subStr);
			if (offset == wxNOT_FOUND)
			{
				break;
			}
			else
			{
				strLeft = str.Left(offset);
				offset += length; // point past the subStr
				strRight = str.Mid(offset);
				str = strLeft + strRight;
			}
		} while (offset != wxNOT_FOUND);
	}
	return str;
}

// Getting the executable path, under Windows (but maybe same for Linux or OSX)
// gets the path single-quoted, and any internal spaces in folder names result
// in failure when the app tries to follow the path. The first space results in
// the earlier path of the path being treated as a command, with the word of the
// path immediately before the space being wrongly treated as a command - which
// it isn't, and the path fails. For example:
// C:\adaptit-git\bin\win32\Unicode Debug\python dLss_win.py
// fails because C:\adaptit-git\bin\win32\Unicode is taken wrongly as the whole
// path, and Unicode is not recognised as a runnable CLI command.
// So this function will take in the path, and double-quote any folder names with
// internal space; and some other tests maybe, and send back the safe path - which
// for this example would be: C:\adaptit-git\bin\win32\"Unicode Debug"\python dLss_win.py
// and it would succeed because python is a runnable command, and dLss_win.py will be
// picked up by the CLI as its file to run under python. (By the way, has to be python3,
// for a successful run of dLss_win.py; and 3.7 is installed on this Win machine)
/*
wxString SafetifyPath(wxString rawpath)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxString path = rawpath;
	int offset = wxNOT_FOUND;
	wxString space = _T(" ");
	// Check for any spaces, if there are none, we've nothing to do here
	// and can just return the unmodified raw string
	offset = path.Find(space);
	if (offset == wxNOT_FOUND)
	{
		return path;
	}
	// We've at least one space to deal with
	wxString separator = pApp->PathSeparator;  // a / or backslash
	wxChar charSeparator = separator[0];

	int length = path.Len();
	wxString newPath = wxEmptyString; // build output string here
	wxChar dblQuote = _T('"');
	bool bAfterFolderStart = FALSE;
	bool bFoundSpace = FALSE;
	bool bScanningToFolderEnd = FALSE;
	bool bBeginningScan = TRUE;
	wxChar charSpace = _T(' ');
	int countFromLastSeparator = 0;
	bool bHasFolderInitialQuote = FALSE;

	{ // begin scoped block
		wxStringBuffer pBuffer(path, length + 1);
		wxChar* pBufStart = pBuffer;
		wxChar* pEnd = pBufStart + length;
		wxASSERT(*pEnd == _T('\0'));
		wxChar* ptr = pBuffer;

		while (ptr < pEnd)
		{
#if defined(_DEBUG)
			if ((*ptr == _T('\\')) && (*(ptr+1) == dblQuote))
			{
				int break_here = 1;
			}
#endif
			// Handle already-quoted multi-word folder names here at the top, these
			// need no quoting insertions, and so we just scan & copy over until the
			// matching " followed by separator which ends the folder name, or to the
			// " followed by buffer end (ptr == pEnd) which ends the buffer
			bool bReachedBufferEnd = FALSE;

			if ((*ptr == charSeparator) && (*(ptr + 1) == dblQuote))
			{
				// use the inner loop (using ptr in the inner loop confuses the
				// compiler and it can result in the inner loop jumping to a
				// former processed but different path - so avoid this)
				bHasFolderInitialQuote = TRUE; // keeps control in the inner loop
											   // while this is TRUE
				newPath.Append(*ptr); // copy over the separator
				ptr++; // advance
				newPath.Append(*ptr); // copy over the double-quote
				ptr++; // advance
				// if we use the inner loop, we want these set FALSE on exit
				// from it at a separator
				bAfterFolderStart = FALSE;
				bScanningToFolderEnd = FALSE;
				bFoundSpace = FALSE;
				bBeginningScan = FALSE;

				// Inner loop, scan to matching " followed by separator - these
				// are the possible end-conditions for the inner loop
				while (bHasFolderInitialQuote && (ptr < pEnd))
				{
					// When two quoted folder names are in sequence, there is an intern
					// 3-character sequence of dblQuote + separator + dblQuote, and we
					// can, after advancing transferring those, continue the inner loop
					// into the second folder name, keeping bHasFolderInitialQuote TRUE
					if (bHasFolderInitialQuote && (*ptr == dblQuote) &&
						(*(ptr + 1) == charSeparator) && (*(ptr + 2) == dblQuote))
					{
						newPath.Append(*ptr); // copy over the double-quote
						ptr++; // advance
						newPath.Append(*ptr); // copy over the separator
						ptr++; // advance
						newPath.Append(*ptr); // copy over the double-quote
						ptr++; // advance
					}
					// Handle the situation where the multi-word folder is
					// last in the buffer
					else if (bHasFolderInitialQuote && (*ptr == dblQuote) && ((ptr + 1) == pEnd))
					{
						newPath.Append(*ptr); // copy over the double-quote
						ptr++; // advance
						wxASSERT(ptr == pEnd);
						bReachedBufferEnd = TRUE;
						bHasFolderInitialQuote = FALSE; // ends inner loop
					}
					else if (bHasFolderInitialQuote && (*ptr == dblQuote) && *(ptr + 1) == charSeparator)
					{
						newPath.Append(*ptr); // copy over the double-quote
						ptr++; // advance

						// but don't copy the following separator - the outer loop will handle that
						bHasFolderInitialQuote = FALSE; // this will end the inner loop
							// but allow processing of the outer loop to continue at the separator
					}
					else
					{
						// Neither end condition has happened, so just copy the character over and
						// iterate the inner loop, advancing ptr after each copy
						newPath.Append(*ptr); // copy over the folder-name's character
						ptr++; // advance
					}
				} // end of inner loop, while (ptr2 < pEnd2)

			} // end of TRUE block for test:
			  // if ((*ptr == _T('\\')) && (*(ptr + 1) == dblQuote))

			if (bReachedBufferEnd)
			{
				break; // from the outer loop, with ptr at pEnd
			}

			if (*ptr == charSpace)
			{
				bFoundSpace = TRUE;
				int newLength = newPath.Len();
				wxString firstBit = newPath.Left(newLength - countFromLastSeparator);
				wxString lastBit = newPath.Mid(newLength - countFromLastSeparator);
				wxASSERT(firstBit.GetChar(firstBit.Len() - 1) == separator);
				// firstBit should be ending in the separator, so we append " there
				firstBit += dblQuote;
				// Recombine the bits
				newPath = firstBit + lastBit;
				// Now add the space to newPath, and advance ptr
				newPath.Append(*ptr); // copy over the double-quote
				ptr++; // advance
				// countFromLastSeparator has done its job, so clear to zero
				countFromLastSeparator = 0;
				// update the state boooleans, so that the block for
				// bScanningToFolderEnd TRUE is scanned to its end, either to
				// next separator (skipping over any spaces - the folder name
				// may have more than two words) or to buffer end if no more
				// separators follow
				bBeginningScan = FALSE;
				bScanningToFolderEnd = TRUE;  // and bFoundSpace set true above
				bAfterFolderStart = FALSE;
				// iterate
			}
			else  // end of TRUE block for test: if (*ptr == charSpace)
			{
				if (*ptr == charSeparator)
				{
					// pointing at a backslash or / separator
					if (!bAfterFolderStart && !bScanningToFolderEnd)
					{
						// entering a folder name
						newPath.Append(*ptr); // copy over the separator
						ptr++; // advance
						countFromLastSeparator = 0; // don't start counting yet
						bBeginningScan = FALSE;

						bAfterFolderStart = TRUE;
						// iterate
					} // end of TRUE block for test: if (!bAfterFolderStart && !bScanningToFolderEnd)
					else
					{
						if (bAfterFolderStart && !bScanningToFolderEnd && (*ptr == charSeparator))
						{
							// transfer it & advance ptr
							newPath.Append(*ptr); // copy over the separator
							ptr++; // advance
							countFromLastSeparator = 0; // counting is about to begin
						}
						// We've come to a separator, transferred it and advance over it
						// but a new folder name may be starting, there may be a space or
						// spaces in the name
						if (!bScanningToFolderEnd && bAfterFolderStart) // && (*ptr != charSeparator))
						{
							// We've just transferred the separator preceding a folder name. This
							// folder name may have no space within it - in which case it won't
							// need any double-quote protection, and that means ptr will traverse
							// to end of buffer or to the next separator without bAtSpace going
							// TRUE. We need to count characters in case there is space.
							// If we come to a space (don't care if one of several, it will be
							// intercepted by a test block above
							// bBeginningScan = FALSE; // might not be needed here
							if (ptr < pEnd)
							{
								if (*ptr == separator)
								{
									bAfterFolderStart = FALSE;
									//continue;
								}
								else
								{
									newPath.Append(*ptr); // copy over the character of the foldername
									ptr++; // advance
									// Count each char traversed... we may come to a space
									countFromLastSeparator++;
								}
							}
						} // end of TRUE block for test: if (!bScanningToFolderEnd && bAfterFolderStart)

					} // end of else block for test: if (!bAfterFolderStart && !bScanningToFolderEnd)


				} // end of TRUE block for test: if (*ptr == charSeparator)
				//else
				if (!bBeginningScan && !bScanningToFolderEnd)
				{
					// If we come to a space, iterate without advancing so that the
					// block near start of the loop intercepts the space
					if (*ptr == charSpace)
					{
						continue;
					}
					else
					{
						// transfer character and advance
						newPath.Append(*ptr);
						ptr++; // advance
						countFromLastSeparator++; // count, we may come to a space
					}
					// iterate
				} // end of TRUE block for test:
				  // if (!bBeginningScan && !bScanningToFolderEnd)
				else
				{
					if (bScanningToFolderEnd && bFoundSpace)
					{
						// Scan over the post-space material until the end or a separator is reached
						if (ptr < pEnd)
						{
							if (*ptr != separator)
							{
								// just transfer the rest of the folder name
								newPath.Append(*ptr);
								ptr++; // advance
								// Check, we may have reached pEnd, in which case we
								// must end the string with a " to match the initial one
								if (ptr == pEnd)
								{
									newPath.Append(dblQuote);
									// iterate (and loop will exit at the while test)
								}
								else
								{
									// Have we advanced ptr to the end of the multi-word
									// folder name, and are now pointing at speparator
									// followed by the opening double-quote of a protected
									// folder name? If so, the code for handling the next
									// bit is at top of the loop, so we've got to add
									// the closing doublequote for the currently being
									// handled unprotected multi-word folder name - before
									// iterating to handle the separator + " sequence
									if ((*ptr == separator) && (*(ptr + 1) == dblQuote))
									{
										newPath.Append(dblQuote);
										// no advance, leave ptr pointing at the separator
									}
									// iterate
								}
							}
							else
							{
								// We've come to a separator, we must add the matching
								// double-quote, default the state booleans, don't advance
								// ptr, and iterate
								newPath += dblQuote;
								bFoundSpace = FALSE;
								bScanningToFolderEnd = FALSE;
								bAfterFolderStart = FALSE;
								countFromLastSeparator = 0;
								bHasFolderInitialQuote = FALSE;
								bBeginningScan = FALSE;
								// iterate, leaving ptr pointing at the separator
							}
						}
						else
						{
							// we've come to end of buffer without finding another
							// separator - so add the matching end dbl quote, and exit
							// the loop
							newPath += dblQuote;
							bFoundSpace = FALSE;
							bScanningToFolderEnd = FALSE;
							bAfterFolderStart = FALSE;
							break;
						}
					} // end of TRUE block for test: if (bScanningToFolderEnd && bFoundSpace
				}
			} // end of else block for test: if (*ptr == charSpace)

			// ============== processing for what's not done above ==============

			// not pointing at space or separator, or at beginning
			// the span and not yet reached first separator
			if (bBeginningScan)
			{
				// starting out on the scanning
				wxASSERT(!bAfterFolderStart && !bScanningToFolderEnd);
				newPath.Append(*ptr);
				countFromLastSeparator = 0; // stays zero until we get to a separator
				// advance
				ptr++;
			}

			else
			{
				;
			} // else block for test: if (bBeginningScan)

		};
	} // end of the  { // begin scoped block
	return newPath;
}
*/

CBString ConvertToUtf8(const wxString& str)
{
	// converts UTF-16 strings to UTF-8
	// whm 21Aug12 modified. No need for #ifdef _UNICODE and #else
	// define blocks which won't compile in ANSI (Debug or Release) builds
	wxCharBuffer tempBuf = str.mb_str(wxConvUTF8);
	return CBString(tempBuf);
}

wxString ConvertToUtf16(CBString& bstr)
{
	// whm 21Aug12 modified. No need for #ifdef _UNICODE and #else
	// define blocks which won't compile in ANSI (Debug or Release) builds
	wxWCharBuffer buf(wxConvUTF8.cMB2WC(bstr.GetBuffer()));
	return wxString(buf);
}

// BEW 27Apr23 a helper to remove any embedded or final null or nulls in a wxString. Leaving them
// in a string can make << += or = appear not to work; the null or nulls are counted in .Length()
// calculations, and legitimate content after first null becomes invisible in the IDE's Output
// window. (Hovering over a string and then over the drop down for m_impl will display the string
// vertically and nulls will show up as 0 in the vertical list display.)
wxString RemoveNulls(wxString inputStr)
{
	if (inputStr.IsEmpty()) return inputStr; // sanity test
	wxChar chNull = (wxChar)0;  // a null
	int strLen = inputStr.Length();
	wxASSERT(strLen > 0);
	wxChar aChar;
	wxString newStr = wxEmptyString;  // safe initialisation, newStr doesn't have <null>
	int index;
	for (index = 0; index < strLen; index++)
	{
		aChar = inputStr[index];
		if (aChar != chNull)
		{
			newStr << aChar;
		}
		else
		{
			// Found a null, so iterate
			continue;
		}
	}
	return newStr;
}

//BEW created 1Sep23 to analyse the contents of an Sstr like: ten10\em*;\f*?”\wj*  in order to
// generate mkrSpan elements to store in the passed in arrItems. Each such is the beginMkr,
// possibly a following whitespace (space probably, if any) then one or more puncts.
// The goal is to determine readable data which tells me what puncts go with which
// endMkrs - so that I can avoid having to show a Placement dlg to do the job
// The boolean return value sets or clears the caller's bIsAmbiguousForEndEndmarkerPlacement.
// The latter may have been set TRUE by legacy FromSingleMakeTstr() code, but if this function
// succeeds in its analysis task, FALSE will be returned, so that the Placement dialog will 
// not get called, and it's fields can be filled out correctly prior to the AutoPlaceSomeMarkers()
// call which follows this function
// params:
// s		  -> pass in the Sstr value
// arrItems   <- store the backslash-separated mkrSpan substrings: - one such for each endMkr
// separator  -> the string _T("\\")
// CopiedTstr <- reference to the (modified herein) value of CopiedTstr to pass back to caller
// tgtWord	  -> the "baseword" (value of m_adaption) with which to build CopiedTstr internally
// Returns bool to set the caller's boolean bIsAmbiguousForEndmarkerPlacement to FALSE if the function
// succeeds in determining the correct mix of puncts and markers without a placement dialog needing to appear
bool AnalyseSstr(wxString s, wxArrayString& arrItems, wxString separator, wxString& CopiedTstr, wxString tgtWord)
{
	// When AI starts up, spaceless src and tgt puncts, final ones, and begining one, are auto-calculated.
	// We can use these from pApp, the functions bool IsPunctuation(wxChar* pChar, bool bSource) tells
	// if the *pChar is punctuation. (bSource is default TRUE, explicitly set FALSE to have the 
	// check work with AI's target puncts set)
	wxString asterisk = _T("*");
	// Internally, we have to allow for whitespace to precede a punct; Nyindrou and other data sometimes has
	// detached final puncts. 
	CAdapt_ItApp* pApp = &wxGetApp();
	// Sanity tests
	if (s.IsEmpty())
	{
		return FALSE; // can't call a Placement dialog if there is no content in Sstr
	}
	// Are there any markers to find?
	wxString backslash = _T('\\');
	int numEndMkrs = s.Replace(backslash, backslash);
	if (numEndMkrs == 0)
	{
		return FALSE; // any final puncts will be attached to word end, without any ambiguity
	}
	wxString srcPuncts = pApp->m_strSpacelessSourcePuncts;
	wxString tgtPuncts = pApp->m_strSpacelessTargetPuncts;
	int nWhitesCount = 0; // there may be white space before an associated punct char (parse separately)
	long tokensCount = 0; // this count will equal the number of backslashes in Sstr + 1
	wxArrayString arrElements;
	wxString delimiters = separator; 
	wxString mkrSpan;
	mkrSpan = wxEmptyString;
	tokensCount = SmartTokenize(delimiters, s, arrElements); // final param: bool bStoreEmptyStringsToo is default TRUE
#if defined (_DEBUG)
	if (tokensCount >= (long)3)
	{
		wxLogDebug(_T("helpers.cpp AnalyseSstr(), line %d , element1= [%s] , element2= [%s] , element3= [%s]"), __LINE__,
			arrElements.Item((size_t)0).c_str(), arrElements.Item((size_t)1).c_str(), arrElements.Item((size_t)2).c_str());
	}
#endif
	// can't use the first element of SmartTokenize() as it's source text (i.e. m_key),
	// and what we want is m_adaption as that is targetText, and targetBaseStr has that. 
	// So throw away arrElement's first element (the material before the first backslash)
	// and pass in the caller's tgtBaseStr as last param: tgtWord - we'll need it below
	arrElements.RemoveAt(0);
	tokensCount--;
	// Because the delimiter was backslash, the elements lack initial backslashes. Fix that.
	wxArrayString arrMkrSpans;
	long index;
	for (index = 0; index < tokensCount; index++)
	{
		mkrSpan = arrElements.Item((size_t)index);
		mkrSpan = backslash + mkrSpan;
		arrMkrSpans.Add(mkrSpan);
	}
#if defined (_DEBUG)
	if (tokensCount >= (long)3)
	{
		wxLogDebug(_T("helpers.cpp AnalyseSstr(), line %d , element1= [%s] , element2= [%s] , element3= [%s]"), __LINE__,
			arrMkrSpans.Item((size_t)0).c_str(), arrMkrSpans.Item((size_t)1).c_str(), arrMkrSpans.Item((size_t)2).c_str());
	}
#endif
	// What remains? The puncts need to be converted to their target text equivalents, then build Tstr to pass back to caller
	CopiedTstr.Empty();
	CopiedTstr << tgtWord; // start building Tstr

	int offset; int mkrSpanLen; wxString wholeMkr; wxString remainder; wxChar space; wxString itsPuncts; int numWhites;
	offset = -1;
	mkrSpanLen = 0;
	wholeMkr = wxEmptyString;
	remainder = wxEmptyString;
	numWhites = 0;
	itsPuncts = wxEmptyString;
	space = _T(' '); // there might be a space before the punctuation in mkrSpan

	for (index = 0; index < tokensCount; index++)
	{
		mkrSpan = arrMkrSpans.Item((size_t)index);
		mkrSpanLen = mkrSpan.Length();
		offset = mkrSpan.Find(asterisk);
		if (offset >= 2)
		{
			// Found the offset to the * at the end of the endMkr
			wholeMkr = mkrSpan.Left(offset + 1);
			remainder = mkrSpan.Mid(offset + 1);
			// BEW 5Sep23 if the marker has no following puncts, remainder will be just the mkr,
			// and then remainder will be empty. GetChar(0) cannot be called on an empty string, causes crash
			wxChar chFirst;
			if (!remainder.IsEmpty())
			{
				chFirst = remainder.GetChar(0);
				if (chFirst == space)
				{
					itsPuncts = remainder.Mid(1);
					numWhites = 1;
				}
				else
				{
					itsPuncts = remainder;
					numWhites = 0;
				}
				itsPuncts = GetConvertedPunct(itsPuncts); // converted to target text punctuation glyphs
				// Collect the bits to complete CopiedTstr
				CopiedTstr << wholeMkr;
				if (numWhites != 0)
				{
					CopiedTstr << space;
				}
				CopiedTstr << itsPuncts;
			}
			else
			{
				CopiedTstr << wholeMkr;
			}
		} // end of TRUE block for test: if (offset >= 2)
	} // end of for loop with test: for (index = 0; index < tokensCount; index++)

#if defined (_DEBUG)
	if (tgtWord == _T("aTEN10tgt"))
	{
		int halt_here = 1;
	}
#endif
	return FALSE;
}

// BEW 2May23 created. But no need for this, existing code handles complex endmkr and puncts mixed
/* leave it here for a while, in case bits of its code are of use sometime soon
bool ParseWordEndMkrsAndPuncts(CSourcePhrase* pSrcPhrase, wxChar* pChar, wxChar* pEnd, int& len, wxString spacelessPuncts)
{
	// sanity tests; the caller already knows if the spacelessPuncts string passed in is for source puncts or target puncts
	if (pSrcPhrase == NULL) { return FALSE; }
	if (spacelessPuncts.IsEmpty()) { return FALSE; }
	if (pChar >= pEnd) { return FALSE; }
	wxChar chSpace = _T(' ');
	if (*pChar == chSpace)
	{
		// Space after a parsed word needs to be considered to be the space which ends off that word's parse,
		// and the space is to be the space before the next pSrcPhrase to be parsed
		return FALSE;
	}
	CAdapt_ItApp* pApp = &wxGetApp();
	CAdapt_ItDoc* pDoc = pApp->GetDocument();

	wxChar gSFescapechar = _T('\\');
	if (*pChar != gSFescapechar)
	{
		// This function, if called, must be because the caller has determined that after the parsed word
		// caller's ptr is pointing at a backslash
		return FALSE;
	}
	// end of sanity checks
	wxChar* ptr = pChar;
	int itemLen = 0; // compute lengths internally using itemLen, to not interfere with caller's len till we are done here
	wxString key = pSrcPhrase->m_key; // m_key cannot be legally empty
	wxASSERT(!key.IsEmpty());
	// Don't leave m_srcPhrase empty, initialise it to key's value, to which we will add following puncts when/if found
	if (pSrcPhrase->m_srcPhrase.IsEmpty())
	{
		pSrcPhrase->m_srcPhrase = key;
	}
	else
	{
		// In case its not empty, append
		pSrcPhrase->m_srcPhrase << key;
	}
	wxString curEndMkrs; wxString myMkr; int myMkrLen;
	bool bStoredEndMkr; wxString curOuterPuncts;
	bStoredEndMkr = FALSE; // initialise
	bool bStoredBindingEndMkr;
	bStoredBindingEndMkr = FALSE; // initialise
	bool bStoredNonbindingEndMkr;
	bStoredNonbindingEndMkr = FALSE;
	USFMAnalysis* pUsfmAnalysis;
	pUsfmAnalysis = NULL; // more initialisations...
	wxString tagOnly = wxEmptyString;
	wxString baseOfEndMkr = wxEmptyString;
	wxString bareMkr = wxEmptyString;
	wxString wholeMkr = wxEmptyString;
	wxString wholeMkrPlusSpace = wxEmptyString;
	bool bIsNestedMkr = FALSE;
	bool bIsRedEndMkr = FALSE;

	while (*ptr == gSFescapechar)
	{
		if (*ptr == gSFescapechar)
		{
			myMkr = pDoc->GetWholeMarker(ptr);
			myMkrLen = myMkr.Length();
			if (pDoc->IsEndMarker(ptr, pEnd))
			{
				pUsfmAnalysis = pDoc->LookupSFM(ptr, tagOnly, baseOfEndMkr, bIsNestedMkr); // BEW 24Oct14 overload
				wxASSERT(pUsfmAnalysis != NULL); // not an unknown marker
				bareMkr = wxEmptyString;
				if (baseOfEndMkr.IsEmpty())
				{
					// It's not an endmarker
					if (bIsNestedMkr)
						bareMkr = _T('+');
					else
						bareMkr.Empty();
				}
				bareMkr += pUsfmAnalysis->marker; // if marker is em, result is either +em or em
				bareMkr += _T('*'); // now it's eiher +em* or em*
				wholeMkr = gSFescapechar + bareMkr; // now it's  \em*, hence it's reconstructed
				wholeMkrPlusSpace = wholeMkr + chSpace; // this string augments it to "\\em* "
								// which is suitable for fast-access string lookups
				if (pUsfmAnalysis->inLine == FALSE)
				{
					// must be an endMkr to be stored in m_endMarkers
					curEndMkrs = pSrcPhrase->GetEndMarkers(); // could be empty
					curEndMkrs += wholeMkr;
					pSrcPhrase->SetEndMarkers(curEndMkrs);
					bStoredEndMkr = TRUE;
					int wholeMkrLen = wholeMkr.Length();
					itemLen += wholeMkrLen;
					ptr += wholeMkrLen;
				}
				else
				{
					// inLine is TRUE, so check for within m_RedEndMarkers or inlineBinding, or inlineNonbinding;
					// those in the m_RedEndMarkers set are in spans, like \f ... \f*, so these will be stored in
					// pSrcPhrase->m_endMarkers member; the other two options would be for binding ones, or non-binding
					int offset;
					offset = pApp->m_RedEndMarkers.Find(wholeMkrPlusSpace);
					if (offset >= 0)
					{
						// store in m_endMarkers also
						curEndMkrs = pSrcPhrase->GetEndMarkers(); // could be empty
						curEndMkrs += wholeMkr;
						pSrcPhrase->SetEndMarkers(curEndMkrs);
						bIsRedEndMkr = TRUE;
						int wholeMkrLen = wholeMkr.Length();
						itemLen += wholeMkrLen;
						ptr += wholeMkrLen;

						int mkrLen = wholeMkr.Length();
						itemLen += mkrLen;
						ptr += mkrLen;
					} // end of TRUE block for test: if (offset >= 0) -- for m_RedEndMarkers set 
					else
					{
						// must be either binding or non-binding, check out which
						offset = pApp->m_charFormatEndMkrs.Find(wholeMkrPlusSpace);
						if (offset >= 0)
						{
							wxString strBinding = pSrcPhrase->GetInlineBindingEndMarkers(); // probably empty
							strBinding += wholeMkr;
							pSrcPhrase->SetInlineBindingEndMarkers(strBinding);
							bStoredBindingEndMkr = TRUE;
						}
						else
						{
							// not in the binding mkrs set, so must be in the nonbinding set
							offset = gpApp->m_inlineNonbindingEndMarkers.Find(wholeMkrPlusSpace);
							wxASSERT(offset >= 0);
							wxString strNonbinding = pSrcPhrase->GetInlineNonbindingEndMarkers();
							strNonbinding += wholeMkr;
							pSrcPhrase->SetInlineNonbindingEndMarkers(strNonbinding);
							bStoredNonbindingEndMkr = TRUE;
						}
						int mkrLen = wholeMkr.Length();
						itemLen += mkrLen;
						ptr += mkrLen;
						// Punctuation may follow, so don't return before checking for final puncts

					} // end of else block for test: if (offset >= 0) -- for m_RedEndMarkers set

				} // end of else block for test: if (pUsfmAnalysis->inLine == FALSE)
				bStoredEndMkr = TRUE;

				len += itemLen;
				// prepare for another iteration
				itemLen = 0;
				itemLen = pDoc->ParseFinalPuncts(ptr, pEnd, spacelessPuncts);
				if (itemLen > 0)
				{
					wxString extraPuncts = wxString(ptr, itemLen);
					if (!bStoredNonbindingEndMkr)
					{
						// Store in m_follPunct when the additional puncts follow either an
						// inline binding marker, or when the endmarker is not inLine TRUE 
						pSrcPhrase->m_follPunct += extraPuncts;
					}
					else
					{
						// Must be an inLine endMarker which is not of binding type, so
						// store in m_follOuterPunct
						wxString strOuterPuncts = pSrcPhrase->GetFollowingOuterPunct();
						strOuterPuncts += extraPuncts;
						pSrcPhrase->SetFollowingOuterPunct(strOuterPuncts);
					}
					pSrcPhrase->m_srcPhrase += extraPuncts;// so user can see it in GUI layout
					len += itemLen;
					ptr += itemLen;
					itemLen = 0;
				} // end of TRUE block for test: if (itemLen > 0)

			} // end of TRUE block for test: if (IsEndMarker(ptr, pEnd))
		} // end of TRUE block for test: if (*ptr == gSFescapechar)

	} // end of while loop with test: while (*ptr == gSFescapechar)
	return TRUE;
}
*/
/* BEW 14Apr23 commented out, because it's never called
int do_upload_local_kbw(void)
{
	int rv = -1;






	return rv;
}
*/

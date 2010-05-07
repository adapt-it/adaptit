/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			helpers.cpp
/// \author			Bruce Waters, revised for wxWidgets by Bill Martin
/// \date_created	6 January 2005
/// \date_revised	15 January 2008
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

#include "Adapt_It.h"
#include "helpers.h"
#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"
#include "SourcePhrase.h"
#include "BString.h"
#include "SplitDialog.h"
#include "SourcePhrase.h"
#include "ExportFunctions.h"
#include "PlaceInternalMarkers.h"
#include "Uuid_AI.h" // for uuid support

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

extern wxChar gSFescapechar; // the escape char used for start of a standard format marker

extern const wxChar* filterMkr; // defined in the Doc
extern const wxChar* filterMkrEnd; // defined in the Doc
const int filterMkrLen = 8;
const int filterMkrEndLen = 9;

extern bool gbIsGlossing;

//  helper functions

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
		Buffer[numRead] = '\0'; // insure terminating null
			
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
	// SpanIncluding() returns a substring that contains characters in the 
	// string that are in charSet, beginning with the first character in 
	// the string and ending when a character is found in the inputStr that
	// is not in charSet. If the first character of inputStr is not in the
	// charSet, then it returns an empty string.
	wxString span = _T("");
	if (charSet.Length() == 0) 
		return span;
	for (size_t i = 0; i < inputStr.Length(); i++)
	{
		if (charSet.Find(inputStr[i]) != -1)
			span += inputStr[i];
		else
			return span;
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
	if (charSet.Length() == 0) 
		return inputStr; // return the original string if charSet is empty
	for (size_t i = 0; i < inputStr.Length(); i++)
	{
		if (charSet.Find(inputStr[i]) == -1)
			span += inputStr[i];
		else
			return span;
	}
	return span;
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
/// Insures that the list/combo box is valid, has at least one item and that at least one item in
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
	// non-Windows platforms, so ListBoxPassesSanityCheck() does the following to insure that any 
	// wxListBox, wxCheckListBox, or wxComboBox is ready for action:
	// 1. We check to see if pListBox is really there (not NULL).
	// 2. We return FALSE if the control does not have any items in it.
	// 3. We check to insure that at least one item is selected when the control has at least one item
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

	// If we get here we know there is at least one item in the list. Insure we have a valid selection. 
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
			// pSrcPhrase
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
		} while (counter <= 5);
		// still indeterminate, so default to returning TRUE
		return TRUE;
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
			_("Error, empty path specification"), wxICON_ERROR);
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
		aToken = aToken.Trim(FALSE); // FALSE means trim white space from left end
		aToken = aToken.Trim(); // default TRUE means trim white space from right end
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
/// \remarks
/// Gets any content in m_collectedBackTrans, m_note, m_freeTrans (and for those, adds
/// \bt, \note, \note*, \free and \free* for any such info which is not empty) and then
/// appends any info from m_filteredInfo (after removing filter bracket markers), adds a
/// final space, and sends the result back to the caller as a wxString
/// BEW created 6Apr10 in order to simplify getting the filtered info as it would appear
/// in an export of, say, source text or other text lines - since for doc version 5
/// filtered info is stored in several disparate members, rather than just in m_markers
/// along with unfiltered info
//////////////////////////////////////////////////////////////////////////////////////////
wxString GetFilteredStuffAsUnfiltered(CSourcePhrase* pSrcPhrase, bool bDoCount, bool bCountInTargetText)
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
	if (!filteredInfoStr.IsEmpty())
	{
		// this data has any markers and endmarkers already 'in place'
		str.Trim();
		str += aSpace + filteredInfoStr;
	}
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
/// \param  Tstr            -> the string into which there might need to be
///                             placed markers and endmarkers, possibly a placement dialog
///                             invoked on it as well
/// \remarks
/// This function is used in the preparation of source text data (Sstr), and editable data
/// (Tstr), and if necessary it will call the PlaceInternalMarkers class to place medial
/// markers into the editable string. It is used in RebuildTargetText() to compose
/// non-editable and editable strings, these are source text and target text, from a merger
/// (in the case of glosses, the glass would have been added after the merger since
/// glossing mode does not permit mergers). Mergers are illegal across filtered info, such
/// info may only occur on the first CSourcePhrase in a merger, and so we know there cannot
/// be any medial filtered information to be dealt with - we only have to consider
/// m_markers and m_endMarkers. Also, since a placement dialog doesn't need to do placement
/// in any of the stored initial filtered information, if present, we separate that
/// information in markersPrefix and don't present it to the user if the dialog is made
/// visible, but just add it in after the dialog is dismissed.
/// Note: we only produce Sstr in order to support a referential source text in the top
/// edit box of the placement dialog - which won't be called if there are no medial
/// markers stored in the merged src phrase. The modified Tstr is what we return.
/// BEW 1Apr10, written for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////////////
wxString FromMergerMakeTstr(CSourcePhrase* pMergedSrcPhrase, wxString Tstr)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	SPList* pSrcPhrases = gpApp->m_pSourcePhrases;
	wxASSERT(pMergedSrcPhrase->m_pSavedWords->GetCount() > 1); // must be a genuine merger
	SPList* pSrcPhraseSublist = pMergedSrcPhrase->m_pSavedWords;
	SPList::Node* pos = pSrcPhraseSublist->GetFirst();
	wxASSERT(pos != 0);
	SPList::Node* posLast = pSrcPhraseSublist->GetLast();
	wxASSERT(posLast != 0);
	bool bHasInternalMarkers = FALSE;
	bool bFirst = TRUE;
	bool bNonFinalEndmarkers = FALSE;

    // store here any string of filtered information stored on pMergedSrcPhrase (in any or
    // all of m_freeTrans, m_note, m_collectedBackTrans, m_filteredInfo) which is on the
    // first CSourcePhrase instance
	wxString markersPrefix; markersPrefix.Empty();
	wxString Sstr; Sstr.Empty(); // Sstr needed only if internally we must use the placement
								 // dialog; we don't need to return it to the caller

	// markers needed, since doc version 5 may store some filtered stuff without using them
	wxString freeMkr(_T("\\free"));
	wxString freeEndMkr = freeMkr + _T("*");
	wxString noteMkr(_T("\\note"));
	wxString noteEndMkr = noteMkr + _T("*");
	wxString backTransMkr(_T("\\bt"));
	markersPrefix.Empty(); // clear it out
	Sstr.Empty(); // clear it out

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
	// remove any filter bracketing markers if filteredInfoStr has content
	if (!filteredInfoStr.IsEmpty())
	{
		filteredInfoStr = pDoc->RemoveAnyFilterBracketsFromString(filteredInfoStr);
	}

    // for the first CSourcePhrase, we store any filtered info within the prefix
    // string, and any content in m_markers, if present, must be put at the start
    // of Tstr and Sstr; remove LHS whitespace when done
	if (!collBackTransStr.IsEmpty())
	{
		// add the marker too
		markersPrefix.Trim();
		markersPrefix += backTransMkr;
		markersPrefix += aSpace + collBackTransStr;
	}
	if (!freeTransStr.IsEmpty() || pMergedSrcPhrase->m_bStartFreeTrans)
	{
		markersPrefix.Trim();
		markersPrefix += aSpace + freeMkr;

        // BEW addition 06Oct05; a \free .... \free* section pertains to a
        // certain number of consecutive sourcephrases starting at this one if
        // m_markers contains the \free marker, but the knowledge of how many
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
		int nWordCount = CountWordsInFreeTranslationSection(TRUE,pSrcPhrases,
				pMergedSrcPhrase->m_nSequNumber); // TRUE means 'count in tgt text'
		// construct an easily findable unique string containing the number
		wxString entry = _T("|@");
		entry << nWordCount; // converts int to string automatically
		entry << _T("@| ");
		// append it after a delimiting space
		markersPrefix += aSpace + entry;
		if (freeTransStr.IsEmpty())
		{
			// we must support empty free translation sections
			markersPrefix += aSpace + freeEndMkr;
		}
		else
		{
			// now the free translation string itself & endmarker
			markersPrefix += aSpace + freeTransStr;
			markersPrefix += freeEndMkr; // don't need space too
		}
	}
	if (!noteStr.IsEmpty() || pMergedSrcPhrase->m_bHasNote)
	{
		markersPrefix.Trim();
		markersPrefix += aSpace + noteMkr;
		if (noteStr.IsEmpty())
		{
			// we don't yet support empty notes elsewhere in the app, but we'll do so here
			markersPrefix += aSpace + noteEndMkr;
		}
		else
		{
			markersPrefix += aSpace + noteStr;
			markersPrefix += noteEndMkr; // don't need space too
		}
	}
	if (!filteredInfoStr.IsEmpty())
	{
		// this data has any markers and endmarkers already 'in place'
		markersPrefix.Trim();
		markersPrefix += aSpace + filteredInfoStr;
	}
	markersPrefix.Trim(FALSE); // finally, remove any LHS whitespace
	// make sure it ends with a space
	markersPrefix.Trim();
	markersPrefix << aSpace;

	if (!markersStr.IsEmpty())
	{
        // this data has any markers and endmarkers already 'in place', and we'll
        // put this in Tstr, so it may be shown to the user - because it may
        // contain a marker for which there is a later 'medial' matching endmarker
        // which has to be placed by the dialog, so it helps to be able to see what
        // the matching marker is and where it is; this stuff will be the first, if
        // it exists, in Sstr and Tstr
		Sstr << markersStr;
		Tstr = markersStr + Tstr;
	}

	// any endmarkers on the first CSourcePhrase are therefore "medial", and
	// any endmarkers on the last one are not medial and so can be added
	// without recourse to the Place... dialog; the last CSourcePhrase of the
	// retranslation will have the m_bEndRetranslation flag set; beware when
	// the retraslation is one word only and so it has m_bBeginRetranslation
	// and m_bEndRetranslation both set; note: the position of the endmarkers
	// is determinate for the Sstr accumulation, so we place them here for
	// that - but only for CSourcePhrase instances other than then end one, as
	// we handle case of endmarkers at the very end lower down in the code
	// after the loop ends
	if (!endMarkersStr.IsEmpty())
	{
		// we've endmarkers we have to deal with
		if (pos != NULL)
		{
            // we are not at the last CSourcePhrase instance, so these endmarkers
            // become medial, so they will already by in m_pMedialMarkers; but
            // place them here at their known location in Sstr
			bHasInternalMarkers = TRUE;
			bNonFinalEndmarkers = TRUE;
		}
		else
		{
            // we are at the phrase's last original CSourcePhrase instance ... so
            // we can place these automatically (don't need to use the Place...
            // dialog)
			bFinalEndmarkers = TRUE; // use this below to do the final append
			finalSuffixStr = endMarkersStr;
		}
	}

	// loop over each of the original CSourcePhrase instances in the merger, the first has
	// to be given special treatment; compose Sstr, but Tstr is passed in, and at most
	// only needs markers and endmarkers added, and any filtered info for both is returned
	// via markersPrefix - with special handling for any free translation
	while (pos <= posLast && pos != NULL)
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
			// remove any filter bracketing markers if filteredInfoStr has content
			if (!filteredInfoStr.IsEmpty())
			{
				filteredInfoStr = pDoc->RemoveAnyFilterBracketsFromString(filteredInfoStr);
			}
		}

		// we compose the prefix string, and the original source string, but the prefix
		// string is built only from what is stored filtered on the first CSourcePhrase
		if (bFirst)
		{
			bFirst = FALSE; // prevent this block from being re-entered

			Sstr << pSrcPhrase->m_srcPhrase;
			if (bNonFinalEndmarkers)
			{
				Sstr << endMarkersStr;
			}
		} // end TRUE block for test: if (bFirst)
		else
		{
			// this block is for m_markers and m_endMarkers stuff only, and accumulation
			// for Sstr (no filtered info can be in the remaining CSourcePhrase instances)
       
            // m_markers material belongs in the list for later placement in Tstr, but the
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
			bool bNonFinalEndmarkers = FALSE;
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
					finalSuffixStr = endMarkersStr;
				}
			}

            // add the source text's word, if it is not an empty string
			if (!pSrcPhrase->m_srcPhrase.IsEmpty())
				Sstr << aSpace << pSrcPhrase->m_srcPhrase;
			// add any needed endmarkers
			if (bNonFinalEndmarkers)
			{
				Sstr << endMarkersStr;
			}
		}

	} // end of while loop

	// finally, add any final endmarkers
	Sstr << finalSuffixStr;
	Tstr << finalSuffixStr;

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
		if (Tstr[0] != _T(' '))
		{
			// need to add an initial space if there is not one there already, to make the
			// dialog's placement algorithm fail-safe
			Tstr = _T(" ") + Tstr;
		}

		// set up the text controls and list box with their data; these setters enable the
		// data passing to be done without the use of globals
		dlg.SetNonEditableString(Sstr);
		dlg.SetUserEditableString(Tstr);
		dlg.SetPlaceableDataStrings(pMergedSrcPhrase->m_pMedialMarkers);

		// show the dialog
		dlg.ShowModal();

		// get the post-placement resulting string
		Tstr = dlg.GetPostPlacementString();
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
	return Tstr;
}

///////////////////////////////////////////////////////////////////////////////////////
/// \return                     the modified value of the passed in Gstr gloss text
/// \param  pMergedSrcPhrase -> a merger, which may have non-empty m_pMedialMarkers member
/// \param  Gstr             -> the string into which there might need to be placed
///                             markers and endmarkers, possibly a placement dialog
///                             invoked on it as well
/// \remarks
/// This function is used in the preparation of source text data (Sstr), and editable data
/// (Gstr), and if necessary it will call the PlaceInternalMarkers class to place medial
/// markers into the editable string. It is used in RebuildGlossesText() to compose
/// non-editable and editable strings, these are source text and gloss text, from a merger
/// (the gloss would have been added after the merger since glossing mode does not permit
/// mergers). Mergers are illegal across filtered info, such info may only occur on the
/// first CSourcePhrase in a merger, and so we know there cannot be any medial filtered
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
/////////////////////////////////////////////////////////////////////////////////////////
wxString FromMergerMakeGstr(CSourcePhrase* pMergedSrcPhrase)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	wxASSERT(pMergedSrcPhrase->m_pSavedWords->GetCount() > 1); // must be a genuine merger
	SPList* pSrcPhraseSublist = pMergedSrcPhrase->m_pSavedWords;
	SPList::Node* pos = pSrcPhraseSublist->GetFirst();
	wxASSERT(pos != 0);
	SPList::Node* posLast = pSrcPhraseSublist->GetLast();
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
	// remove any filter bracketing markers if filteredInfoStr has content
	if (!filteredInfoStr.IsEmpty())
	{
		filteredInfoStr = pDoc->RemoveAnyFilterBracketsFromString(filteredInfoStr);
	}
	if (!endMarkersStr.IsEmpty())
	{
		bFinalEndmarkers = TRUE;
		finalSuffixStr = endMarkersStr;
	}

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
	while (pos <= posLast && pos != NULL)
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
		dlg.SetPlaceableDataStrings(pMergedSrcPhrase->m_pMedialMarkers);

		// show the dialog
		dlg.ShowModal();

		// get the post-placement resulting string
		Gstr = dlg.GetPostPlacementString();
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


///////////////////////////////////////////////////////////////////////////////////////
/// \return                     the reconstituted source text (but without initial marker content)
/// \param  pMergedSrcPhrase -> a merger, which may have non-empty m_pMedialMarkers member
/// \remarks
/// This function is used in the preparation of source text data (Sstr). It is used in
/// RebuildSourceText() in the export of USFM markup for the source text when the passed in
/// CSourcePhrase instance is a merger. Mergers are illegal across filtered info, such info
/// may only occur on the first CSourcePhrase in a merger, and so we know there cannot be
/// any medial filtered information to be dealt with - we only have to consider m_markers
/// and m_endMarkers. Also, no placement dialog is needed because, for source text, the
/// location of SF markers in the recomposed source text is 100% determinate.
/// Note: filtered info and m_markers content from the merger has already been handled in
/// the caller, so we only work with the m_srcPhase member, and m_endMarkers member of the first CSourcePhrase
/// of the merger, but with the non-initial CSourcePhrase members we must take into
/// account any m_markers information as well as the other two members.
/// BEW 7Apr10, written for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////////////
wxString FromMergerMakeSstr(CSourcePhrase* pMergedSrcPhrase)
{
	wxASSERT(pMergedSrcPhrase->m_pSavedWords->GetCount() > 1); // must be a genuine merger
	SPList* pSrcPhraseSublist = pMergedSrcPhrase->m_pSavedWords;
	SPList::Node* pos = pSrcPhraseSublist->GetFirst();
	wxASSERT(pos != 0);
	SPList::Node* posLast = pSrcPhraseSublist->GetLast();
	wxASSERT(posLast != 0);
	//bool bHasInternalMarkers = FALSE;

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
	while (pos <= posLast && pos != NULL)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
	
		// empty the scratch strings
		EmptyMarkersAndFilteredStrings(markersStr, endMarkersStr, freeTransStr, noteStr,
										collBackTransStr, filteredInfoStr);
		// get the other string information we want, putting it in the scratch strings
		GetMarkersAndFilteredStrings(pSrcPhrase, markersStr, endMarkersStr,
						freeTransStr, noteStr, collBackTransStr,filteredInfoStr);

		if (bFirst)
		{
			bFirst = FALSE; // prevent this block from being re-entered
			str << pSrcPhrase->m_srcPhrase;
			if (!pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				str << pSrcPhrase->GetEndMarkers();
			}
		} // end TRUE block for test: if (bFirst)
		else
		{
            // this block is for m_markers, m_srcPhrase and m_endMarkers stuff only, and
            // accumulation to str (no filtered info can be in the remaining CSourcePhrase
            // instances)
       
            // m_markers material is placed immediately whenever found
			if (!markersStr.IsEmpty())
			{
				str.Trim(); // ensure we have just one delimiting space
				str << aSpace << markersStr; 
			}


            // add the source text's word, if it is not an empty string
			if (!pSrcPhrase->m_srcPhrase.IsEmpty())
			{
				str.Trim(); // ensure we have just one delimiting space
				str << aSpace << pSrcPhrase->m_srcPhrase;
			}
			// add any endmarkers found on this instance
			if (!endMarkersStr.IsEmpty())
			{
				// don't need a delimiting space for these, just append
				str << endMarkersStr;
			}
		}
	} // end of while loop

	// finally, ensure there is just a single final space
	str.Trim();
	str << aSpace;
	return str;
}


///////////////////////////////////////////////////////////////////////////////////////
/// \return                     the modified value of Tstr
/// \param  pSingleSrcPhrase -> a single CSourcePhrase instance which therefore cannot have
///                             a non-empty m_pMedialMarkers member; so no placement dialog
///                             will need to be called
/// \param  Tstr            -> the string into which there might need to be
///                             placed m_markers and m_endmarkers material, and prefixed
///                             with any filtered information
/// \remarks
/// This function is used for reconstituting markers and endmarkers, and preceding that,
/// any filtered out information (which always comes first, if present), and the result is
/// return to the caller as the modified Tstr. (Tstr may be passed in containing
/// adaptation text, or gloss text.)
/// BEW 1Apr10, written for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////////////
wxString FromSingleMakeTstr(CSourcePhrase* pSingleSrcPhrase, wxString Tstr)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	SPList* pSrcPhrases = gpApp->m_pSourcePhrases;
	wxASSERT(pSingleSrcPhrase->m_pSavedWords->IsEmpty()); // must be a nnon-merger

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
	}

    // for the one and only CSourcePhrase, we store any filtered info within the prefix
    // string, and any content in m_markers, if present, must be put at the start
    // of Tstr; remove LHS whitespace when done
	if (!collBackTransStr.IsEmpty())
	{
		// add the marker too
		markersPrefix.Trim();
		markersPrefix += backTransMkr;
		markersPrefix += aSpace + collBackTransStr;
	}
	if (!freeTransStr.IsEmpty() || pSingleSrcPhrase->m_bStartFreeTrans)
	{
		markersPrefix.Trim();
		markersPrefix += aSpace + freeMkr;

        // BEW addition 06Oct05; a \free .... \free* section pertains to a
        // certain number of consecutive sourcephrases starting at this one if
        // m_markers contains the \free marker, but the knowledge of how many
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
		int nWordCount = CountWordsInFreeTranslationSection(TRUE,pSrcPhrases,
				pSingleSrcPhrase->m_nSequNumber); // TRUE means 'count in tgt text'
		// construct an easily findable unique string containing the number
		wxString entry = _T("|@");
		entry << nWordCount; // converts int to string automatically
		entry << _T("@| ");
		// append it after a delimiting space
		markersPrefix += aSpace + entry;
		if (freeTransStr.IsEmpty())
		{
			// we must support empty free translation sections
			markersPrefix += aSpace + freeEndMkr;
		}
		else
		{
			// now the free translation string itself & endmarker
			markersPrefix += aSpace + freeTransStr;
			markersPrefix += freeEndMkr; // don't need space too
		}
	}
	if (!noteStr.IsEmpty() || pSingleSrcPhrase->m_bHasNote)
	{
		markersPrefix.Trim();
		markersPrefix += aSpace + noteMkr;
		if (noteStr.IsEmpty())
		{
			// we don't yet support empty notes elsewhere in the app, but we'll do so here
			markersPrefix += aSpace + noteEndMkr;
		}
		else
		{
			markersPrefix += aSpace + noteStr;
			markersPrefix += noteEndMkr; // don't need space too
		}
	}
	if (!filteredInfoStr.IsEmpty())
	{
		// this data has any markers and endmarkers already 'in place'
		markersPrefix.Trim();
		markersPrefix += aSpace + filteredInfoStr;
	}
	markersPrefix.Trim(FALSE); // finally, remove any LHS whitespace
	// make sure it ends with a space
	markersPrefix.Trim();
	markersPrefix << aSpace;

	if (!markersStr.IsEmpty())
	{
        // this data has any markers and endmarkers already 'in place',
        // and we'll put this in Tstr
		Tstr = markersStr + Tstr;
	}

    // any endmarkers on pSingleSrcPhrase are not "medial", and can be added
    // automatically too
	if (!endMarkersStr.IsEmpty())
	{
		Tstr << endMarkersStr;
	}

    // now add the prefix string material if it is not empty
	if (!markersPrefix.IsEmpty())
	{
		markersPrefix.Trim();
		markersPrefix << aSpace; // ensure a final space
		Tstr = markersPrefix + Tstr;
	}
	Tstr.Trim();
	Tstr << aSpace; // have a final space
	return Tstr;
}

// the next 3 functions are similar or identical to member functions of the document class
// which are used in the parsing of text files to produce a document; one (ParseMarker())
// is a modification of the one in the doc class; these are needed here for use in
// CSourcePhrase - later we could replace IsWhatSpace() and ParseWhiteSpace() in the doc
// class with these ( *** TODO *** ?)
bool IsWhiteSpace(const wxChar *pChar)
{
	// returns true for tab 0x09, return 0x0D or space 0x20
	if (wxIsspace(*pChar) == 0)// _istspace not recognized by g++ under Linux
		return FALSE;
	else
		return TRUE;
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
int ParseMarker(const wxChar *pChar)
{
	// this algorithm differs from the one in CAdapt_ItDoc class by not having the code to
	// break immediately after an asterisk; we don't want the latter because this function
	// will be used to parse over a reversed string, and a reversed endmarker will have an
	// initial asterisk and we don't want to halt the loop there
	int len = 0;
	wxChar* ptr = (wxChar*)pChar;
	wxChar* pBegin = ptr;
	while (!IsWhiteSpace(ptr) && *ptr != _T('\0'))
	{
		if (ptr != pBegin && *ptr == gSFescapechar)
			break; 
		ptr++;
		len++;
	}
	return len;
}

// Any strings in pPossiblesArray not already in pBaseStrArray, append them to
// pBaseStrArray, return TRUE if at least one was added, FALSE if none were added
bool AddNewStringsToArray(wxArrayString* pBaseStrArray, wxArrayString* pPossiblesArray)
{
	bool bAddedSomething = FALSE;
	int possIndex;
	int possCount = pPossiblesArray->GetCount();
	for (possIndex = 0; possIndex < possCount; possIndex++)
	{
		wxString aString = pPossiblesArray->Item(possIndex);
		int nFoundIndex = pBaseStrArray->Index(aString); // uses case sensitive compare
		if (nFoundIndex == wxNOT_FOUND)
		{
			// add this one to pBaseStrArray
			pBaseStrArray->Add(aString);
			bAddedSomething = TRUE;
		}
	}
	return bAddedSomething;
}

bool HasFilteredInfo(CSourcePhrase* pSrcPhrase)
{
	if (pSrcPhrase->m_bHasFreeTrans || !pSrcPhrase->GetFreeTrans().IsEmpty())
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

// uuid support
wxString GetGuid()
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




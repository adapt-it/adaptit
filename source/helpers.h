/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			helpers.h
/// \author			Bruce Waters, revised for wxWidgets by Bill Martin
/// \date_created	6 January 2005
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is a header file containing some helper functions used by Adapt It. 
/////////////////////////////////////////////////////////////////////////////
//
#ifndef helpers_h
#define helpers_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "helpers.h"
#endif

#ifndef _string_h_loaded
#define _string_h_loaded
#include "string.h"
#endif

class CBString;
class SPList;	// declared in SourcePhrase.h WX_DECLARE_LIST(CSourcePhrase, SPList); macro 
				// and defined in SourcePhrase.cpp WX_DEFINE_LIST(SPList); macro
class CSourcePhrase;

////////////////////////////////////////////
//  helper functions

void DoDelay();

int NegHeightToPointSize(const long& height);

#include "BString.h"
unsigned int Btoi(CBString& digits);

long PointSizeToNegHeight(const int& pointSize); // whm 9Mar04

// helper for copying LOGFONT structs
// We're not using the MFC LOGFONT structure in the wxWidgets version.
//void CopyLOGFONT(LOGFONT& dest, LOGFONT* src); // unused

// helpers for converting between wxColour and MFC's COLORREF int (wxUint32)
#include <wx/colour.h>
int WxColour2Int(wxColour colour);

wxColour Int2wxColour(int colourInt);

bool IsXMLfile(wxString path);

CBString	SearchXMLFileContentForBookID(wxString FilePath);

//wxString	SearchPlainTextFileForBookID(wxString FilePath); // unused

// next two I added in Jan 2009, but didn't use them. Commented out pending a use for them
//int FindLocationBeforeFinalSpaces(wxString& str); // BEW added 17Jan09
//int FindLocationAfterInitialSpaces(wxString& str); // BEW added 20Jan09

// Helpers added by Jonathan Field 2005
wxString ConcatenatePathBits(wxString Bit1, wxString Bit2);

bool FileExists(wxString Path);

bool FileContainsBookIndicator(wxString FilePath, wxString& out_BookIndicatorInSpecifiedFile);

//bool SearchFileContentForFixedLengthPattern(wxString FilePath, wxInt8 *Pattern, wxInt8 *PatternMask, wxUint32 PatternLength);

bool IsAnsiLetter(wxChar c);

bool IsAnsiDigit(wxChar c);

bool IsAnsiDigitsOnly(wxString s);

bool IsAnsiLetterOrDigit(wxChar c);

bool IsValidFileName(wxString s);

wxString StripPath(wxString FullPath);

SPList *SplitOffStartOfList(SPList *MainList, int FirstIndexToKeep);

wxString RemoveInitialEndmarkers(CSourcePhrase* pSrcPhrase, enum SfmSet currSfmSet,
		bool& bLacksAny, bool bCopyOnly = FALSE); // BEW added 15Aug07 for 3.5.0
							 // & added 4th param on 19May08
// next three for use in the AdminMoveOrCopy class, the handler for Administrator
// menu item Move Or Copy Folders Or Files
bool GetFoldersOnly(wxString& pathToFolder, wxArrayString* pFolders, bool bSort = TRUE);
bool GetFilesOnly(wxString& pathToFolder, wxArrayString* pFiles, bool bSort = TRUE);
int	 sortCompareFunc(const wxString& first, const wxString& second);

// functions added by whm
wxString SpanIncluding(wxString inputStr, wxString charSet);

wxString SpanExcluding(wxString inputStr, wxString charSet);

wxString MakeReverse(wxString inputStr);

int FindFromPos(const wxString& inputStr, const wxString& subStr, int startAtPos);

int FindOneOf(wxString inputStr, wxString charSet);

wxString InsertInString(wxString targetStr, int ipos, wxString insertStr);

wxRect NormalizeRect(const wxRect rect);

void CopyFontBaseProperties(const wxFont* pCopyFromFont, wxFont*& pCopyToFont);

void CopyAllFontAttributes(const wxFont* pFontCopyFrom, wxFont*& pFontCopyTo);

short DecimalToBinary(unsigned long decimalValue, char binaryValue[32]);

bool ListBoxPassesSanityCheck(wxControlWithItems* pListBox);

bool IsCollectionDoneFromTargetTextLine(SPList* pSrcPhrases, int nInitialSequNum);

#endif

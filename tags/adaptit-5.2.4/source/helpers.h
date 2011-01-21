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

// dummy comment to force a recompile

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
bool GetFoldersOnly(wxString& pathToFolder, wxArrayString* pFolders, bool bSort = TRUE,
					bool bSuppressMessage = FALSE);
bool GetFilesOnly(wxString& pathToFolder, wxArrayString* pFiles, bool bSort = TRUE,
				  bool bSuppressMessage = FALSE);
int	 sortCompareFunc(const wxString& first, const wxString& second);
bool IsReadOnlyProtection_LockFile(wxString& filename);

long SmartTokenize(wxString& delimiters, wxString& str, wxArrayString& array, 
					  bool bStoreEmptyStringsToo = TRUE);
wxString ChangeHyphensToUnderscores(wxString& name); // change any hyphen characters 
				// to underscore characters, used in ReadOnlyProtection.cpp


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

// the following returns the members m_markers, m_endMarkers, m_freeTrans, m_Note,
// m_collectedBackTrans, m_filteredInfo, "as is" - the filteredInfoStr will include the
// wrapping filter bracket markers, and freeTransStr and the two following won't have any
// markers at all. We just get what's in the members, and let the caller decide how to
// process the returned strings
void GetMarkersAndFilteredStrings(CSourcePhrase* pSrcPhrase,
								  wxString& markersStr, 
								  wxString& endMarkersStr,
								  wxString& freeTransStr,
								  wxString& noteStr,
								  wxString& collBackTransStr,
								  wxString& filteredInfoStr);
// use the following to empty the caller's local wxString variables for storing these info
// types 
void EmptyMarkersAndFilteredStrings(
								  wxString& markersStr, 
								  wxString& endMarkersStr,
								  wxString& freeTransStr,
								  wxString& noteStr,
								  wxString& collBackTransStr,
								  wxString& filteredInfoStr); 

wxString GetLastMarker(wxString markers);
bool    IsWhiteSpace(const wxChar *pChar);
int     ParseMarker(const wxChar *pChar); // returns a length (num chars in the marker, including backslash)
bool	IsFixedSpaceSymbolWithin(CSourcePhrase* pSrcPhrase);
bool    IsFixedSpaceSymbolWithin(wxString& str); // overload, for checking m_targetPhrase, etc
bool	HasParagraphMkr(wxString& str); // used when converting back from docV5 to docV4

#endif

// BEW created 20Jan11, to avoid adding duplicates of ints already in the passed in
// wxArrayInt, whether or not keep_strips_keep_piles is used for RecalcLayout() - the
// contents won't be used if another layout_selector enum valus is in effect, as
// RecalcLayout() would recreate the strips and repopulate the partner piles in such
// situations 
void AddUniqueInt(wxArrayInt* pArrayInt, int nInt);

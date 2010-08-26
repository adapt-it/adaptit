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

/// An enum for return error-state from GetNewFile()
enum getNewFileState
{
	getNewFile_success,
	getNewFile_error_at_open,
	getNewFile_error_opening_binary,
	getNewFile_error_no_data_read,
	getNewFile_error_unicode_in_ansi,
	getNewFile_error_ansi_CRLF_not_in_sequence
};

// ExtensionAction is used in the function RemoveNameDuplicatesFromArray()
enum ExtensionAction
{
	excludeExtensionsFromComparison,
	includeExtensionsInComparison
};

////////////////////////////////////////////
//  helper functions

//char* StrStrAI(char* super, char* sub);
//
// next one useful for byte-copying a UTF-16 text which was formed from ascii text, and so
// it has zero byte extended UTF-16 characters; strncpy() won't work as it halts at the
// first null byte encountered, no matter what byteCount is requested for copying 
char* strncpy_utf16(char* dest, char* src, size_t byteCount);

int TrimAndCountWordsInString(wxString& str);

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

// end of whm's additions
 
// 2010 additions by BEW

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
bool IsWhiteSpace(const wxChar *pChar);
int ParseWhiteSpace(const wxChar *pChar); // returns a length (num chars of whitespace)
int ParseMarker(const wxChar *pChar); // returns a length (num chars in the marker, including backslash)
// Any strings in pPossiblesArray not already in pBaseStrArray, append them to
// pBaseStrArray, return TRUE if at least one was added, FALSE if none were added
bool AddNewStringsToArray(wxArrayString* pBaseStrArray, wxArrayString* pPossiblesArray);
bool HasFilteredInfo(CSourcePhrase* pSrcPhrase);
bool IsFreeTranslationContentEmpty(CSourcePhrase* pSrcPhrase); // moved from CAdapt_ItView
bool IsBackTranslationContentEmpty(CSourcePhrase* pSrcPhrase); // moved from CAdapt_ItView
wxString GetFilteredStuffAsUnfiltered(CSourcePhrase* pSrcPhrase, bool bDoCount, bool bCountInTargetText);

wxString FromMergerMakeTstr(CSourcePhrase* pMergedSrcPhrase, wxString Tstr);
wxString FromSingleMakeTstr(CSourcePhrase* pSingleSrcPhrase, wxString Tstr);
wxString FromMergerMakeSstr(CSourcePhrase* pMergedSrcPhrase);
wxString FromMergerMakeGstr(CSourcePhrase* pMergedSrcPhrase);

bool	 IsContainedByRetranslation(int nFirstSequNum, int nCount, int& nSequNumFirst,
									   int& nSequNumLast);
bool	 IsNullSrcPhraseInSelection(SPList* pList);
bool	 IsRetranslationInSelection(SPList* pList);

// uuid support
wxString GetUuid();

// for date-time stamping in KB or elsewhere
wxString GetDateTimeNow();

// for KB metadata, to create a string indicating who supplied the adaptation (or gloss)
// For LAN-based collaboration, we will try set the string: "userID:machineID" such as
// watersb:BEW for the "watersb" account on the computer visible on the LAN as "BEW"; and
// anticipating the future when we hope to have both web-based KB sharing, and / or
// AI-in-the-cloud, we'll want to store something different - such as (at least) an IP
// address, so we'll have a bool parameter to distinguish the web versus non-web sourcing
// possibilities
wxString SetWho(bool bOriginatedFromTheWeb = FALSE);

//size_t GetFileSize_t(wxString& absPathToFile);
bool IsLoadableFile(wxString& absPathToFile);
/* unused, but perfect good
bool PopulateTextCtrlByLines(wxTextCtrl* pText, wxString* pPath, int numLines = -1);
*/
bool PopulateTextCtrlWithChunk(wxTextCtrl* pText, wxString* pPath, int numKilobytes = -1);
// GetNewFile was formerly in CAdapt_ItDoc class, and its return enum in Adapt_It.h
// BEW 19July10, added 4th param, so as to be able to get just a certain number of kB of
// the file for viewing (approximate kB, we truncate the end a bit more to ensure the
// characters that remain are valid for the encoding); if the 4th param is zero, it gets
// the whole file and does no truncating at the end
enum getNewFileState GetNewFile(wxString*& pstrBuffer, wxUint32& nLength, 
								wxString pathName, int numKBOnly = 0);

bool SelectedFoldersContainSourceDataFolder(wxArrayString* pFolders);
// BEW created 9Aug10, for support of user-protection from folder navigation
void RemoveNameDuplicatesFromArray(wxArrayString& originals, wxArrayString& unwanted,
								   bool bSorted, enum ExtensionAction extAction);
void ChangeParatextPrivatesToCustomMarkers(wxString& buffer);

#endif	// helpers_h

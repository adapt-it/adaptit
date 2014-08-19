/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			helpers.h
/// \author			Bruce Waters, revised for wxWidgets by Bill Martin
/// \date_created	6 January 2005
/// \rcs_id $Id$
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
#include "Adapt_It.h"
#include "MergeUpdatedSrc.h"
#include <string>

class CBString;
class SPList;	// declared in SourcePhrase.h WX_DECLARE_LIST(CSourcePhrase, SPList); macro 
				// and defined in SourcePhrase.cpp WX_DEFINE_LIST(SPList); macro
class CSourcePhrase;
//class SPArray; // declared in MergeUpdatedSrc.h as a global type, defined in it's .cpp file

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

enum AppPreferedDateTime
{
	adaptItDT,
	paratextDT,
	oxesDT,
	oxesDateOnly,
	forXHTML // BEW added 12Jun12
};

enum CompareUsfmTexts
{
	noDifferences,
	usfmOnlyDiffers,
	textOnlyDiffers,
	usfmAndTextDiffer
};

//enum UniqueFileIncrementMethod // whm moved to Adapt_It.h
//{
//	incrementViaNextAvailableNumber,
//	incrementViaDate_TimeStamp
//};
//
// GDLC Moved to here from AdaptIt_Doc.h to eliminate 40 compile errors
enum WhichLang {
	sourceLang,
	targetLang
};

enum EditorProjectVerseContent
{
	projHasVerseTextInAllBooks,
	projHasNoBooksWithVerseText,
	projHasSomeBooksWithVerseTextSomeWithout,
	projHasNoBooks,
	projHasNoChaptersOrVerses,
	processingError
};

////////////////////////////////////////////
//  helper functions


std::string MakeStdString(wxString str);

//char* StrStrAI(char* super, char* sub);
//
// next one useful for byte-copying a UTF-16 text which was formed from ascii text, and so
// it has zero byte extended UTF-16 characters; strncpy() won't work as it halts at the
// first null byte encountered, no matter what byteCount is requested for copying 
char*     strncpy_utf16(char* dest, char* src, size_t byteCount);

int       TrimAndCountWordsInString(wxString& str);

void      DoDelay();

int       NegHeightToPointSize(const long& height);

#include "BString.h"
unsigned int Btoi(CBString& digits);

// The following two convenience functions added by whm 23Jul12. The wxWidgets
// library has wxAtoi() but not a parallel function that is the equivalent of the
// non-standard Windows/MFC function itoa(). The following two versions of wxItoa()
// differ only in their return values, one returning the wxString equivalent of the
// input integer, and the other returning a CBString equivalent.
void wxItoa(int val, wxString& str);
void wxItoa(int val, CBString& str);

long      PointSizeToNegHeight(const int& pointSize); // whm 9Mar04

// helper for copying LOGFONT structs
// We're not using the MFC LOGFONT structure in the wxWidgets version.
//void CopyLOGFONT(LOGFONT& dest, LOGFONT* src); // unused

// helpers for converting between wxColour and MFC's COLORREF int (wxUint32)
#include <wx/colour.h>
int       WxColour2Int(wxColour colour);

wxColour  Int2wxColour(int colourInt);

bool      IsXMLfile(wxString path);

CBString  SearchXMLFileContentForBookID(wxString FilePath);

//wxString	 SearchPlainTextFileForBookID(wxString FilePath); // unused

// next two I added in Jan 2009, but didn't use them. Commented out pending a use for them
//int FindLocationBeforeFinalSpaces(wxString& str); // BEW added 17Jan09
//int FindLocationAfterInitialSpaces(wxString& str); // BEW added 20Jan09

bool      FileIsEmpty(wxString path);

// Helpers added by Jonathan Field 2005
wxString  ConcatenatePathBits(wxString Bit1, wxString Bit2);

bool      FileExists(wxString Path);

bool      FileHasNewerModTime(wxString fileAndPathTrueIfNewer, wxString fileAndPathFalseIfNewer);

bool      FileContainsBookIndicator(wxString FilePath, wxString& out_BookIndicatorInSpecifiedFile);

//bool    SearchFileContentForFixedLengthPattern(wxString FilePath, wxInt8 *Pattern, wxInt8 *PatternMask, wxUint32 PatternLength);

bool      IsAnsiLetter(wxChar c);

bool      IsAnsiDigit(wxChar c);

bool      IsAnsiDigitsOnly(wxString s);

bool      IsAnsiLetterOrDigit(wxChar c);

bool	  IsAnsiLettersOrDigitsOnly(wxString s);

bool      IsValidFileName(wxString s);

bool      IsUsfmDocument(SPList* pList, bool* pbIsEither); // if FALSE is returned, examine *pbIsEither
	// value, if TRUE then the outcome was indeterminate (either set could be selected meaningfully),
	// if FALSE, then PNG 1998 SFM set is indicated
size_t    EvaluateMarkerSetForIndicatorCount(CSourcePhrase* pSrcPhrase, enum SfmSet set);

wxString  StripPath(wxString FullPath);

SPList    *SplitOffStartOfList(SPList *MainList, int FirstIndexToKeep);

// functions added by whm
// whm Note: the following group of functions share a lot of code with 
// their counterparts in the Doc class. However, these functions are all 
// used on buffers that might be in existence when the actual Doc does 
// not exist. Hence I've put underscores in their function names so they 
// won't be confused with the similarly named functions in the 
// CAdapt_ItDoc class.
bool      Is_AnsiLetter(wxChar c);
bool      Is_ChapterMarker(wxChar* pChar);
bool      Is_VerseMarker(wxChar *pChar, int& nCount);
wxString  GetStringFromBuffer(const wxChar* ptr, int itemLen);
int       Parse_Number(wxChar *pChar, wxChar *pEnd);
//bool    Is_WhiteSpace(wxChar *pChar, bool& IsEOLchar); <<- unused, BEW removed 4Aug11
bool      Is_NonEol_WhiteSpace(wxChar *pChar);
//int     ParseWhiteSpace(wxChar *pChar);
int       Parse_NonEol_WhiteSpace(wxChar *pChar);
int       Parse_Marker(wxChar *pChar, wxChar *pEnd); // modified from the one in the Doc
bool      Is_Marker(wxChar *pChar, wxChar *pEnd);	// modified from the one in the Doc

wxString  ExtractSubstring(const wxChar* pBufStart, const wxChar* pBufEnd, size_t first, size_t last);
void	  ExtractVerseNumbersFromBridgedVerse(wxString tempStr,int& nLowerNumber,
								int& nUpperNumber);
wxString  AbbreviateColonSeparatedVerses(const wxString str);
bool      EmptyVerseRangeIncludesAllVersesOfChapter(wxString emptyVersesStr);

// BEW removed this version of ExtractSubstring() as it's looking like I won't need it
//wxString ExtractSubstring(const wxString& str, int firstChar, int lastChar);
wxString  SpanIncluding(wxString inputStr, wxString charSet);
// the following is an overload for using in a parser
wxString  SpanIncluding(wxChar* ptr, wxChar* pEnd, wxString charSet); // BEW added 11Oct10

wxString  ParseWordInwardsFromEnd(wxChar* ptr, wxChar* pEnd, 
			wxString& wordBuildersForPostWordLoc, wxString charSet); // BEW created 28Jan11

wxString  SpanExcluding(wxString inputStr, wxString charSet);
// the following is an overload for using in a parser  <<-- deprecated 29Jan11
//wxString SpanExcluding(wxChar* ptr, wxChar* pEnd, wxString charSet); // BEW added 11Oct10

wxString  MakeReverse(wxString inputStr);

int       FindFromPos(const wxString& inputStr, const wxString& subStr, int startAtPos);

int       FindFromPosBackwards(const wxString& inputStr, const wxString& subStr, int startAtPos);

int       FindOneOf(wxString inputStr, wxString charSet);

wxString  InsertInString(wxString targetStr, int ipos, wxString insertStr);

bool      IsClosingBracketWordBuilding(wxString& strPunctuationCharSet);

wxRect    NormalizeRect(const wxRect rect);

void      CopyFontBaseProperties(const wxFont* pCopyFromFont, wxFont*& pCopyToFont);

void      CopyAllFontAttributes(const wxFont* pFontCopyFrom, wxFont*& pFontCopyTo);

short     DecimalToBinary(unsigned long decimalValue, char binaryValue[32]);

bool      ListBoxPassesSanityCheck(wxControlWithItems* pListBox);

bool      IsCollectionDoneFromTargetTextLine(SPList* pSrcPhrases, int nInitialSequNum);

wxString  GetUniqueIncrementedFileName(wxString baseFilePathAndName, enum UniqueFileIncrementMethod,
									  bool bAlwaysModify, int digitWidth, wxString suffix);

wxString  RemoveMultipleSpaces(wxString& rString);

void	  RemoveFilterMarkerFromString(wxString& filterMkrStr, wxString wholeMarker);
void      AddFilterMarkerToString(wxString& filterMkrStr, wxString wholeMarker);

// whm added 6Dec11
wxString  GetProgramLocationFromSystemPATH(wxString appName);
wxString  GetAdaptItInstallPrefixForLinux();

// end of whm's additions
 
// 2010 additions by BEW

// use the following for getting the pixel difference for a control's label text which
// starts off with one or more %s specifies, and those are filled out to form newLabel;
// pass in the control in the pWindow param, and internally the function will get the
// label's font, set up a wxWindowDC, measure the two strings, and pass back the
// difference in their widths
int       CalcLabelWidthDifference(wxString& oldLabel, wxString& newLabel, wxWindow* pWindow);

wxString  GetConvertedPunct(const wxString& rStr); // moved from view class to here 11Oct10

// next three for use in the AdminMoveOrCopy class, the handler for Administrator
// menu item Move Or Copy Folders Or Files
bool      GetFoldersOnly(wxString& pathToFolder, wxArrayString* pFolders, bool bSort = TRUE,
					bool bSuppressMessage = FALSE);
bool      GetFilesOnly(wxString& pathToFolder, wxArrayString* pFiles, bool bSort = TRUE,
				  bool bSuppressMessage = FALSE);
int	      sortCompareFunc(const wxString& first, const wxString& second);
bool      IsReadOnlyProtection_LockFile(wxString& filename);

long      SmartTokenize(wxString& delimiters, wxString& str, wxArrayString& array, 
					  bool bStoreEmptyStringsToo = TRUE);
wxString  ChangeHyphensToUnderscores(wxString& name); // change any hyphen characters 
				// to underscore characters, used in ReadOnlyProtection.cpp
wxString ChangeWhitespaceToSingleSpace(wxString& rString);

// the following returns the members m_markers, m_endMarkers, m_freeTrans, m_Note,
// m_collectedBackTrans, m_filteredInfo, "as is" - the filteredInfoStr will include the
// wrapping filter bracket markers, and freeTransStr and the two following won't have any
// markers at all. We just get what's in the members, and let the caller decide how to
// process the returned strings
void      GetMarkersAndFilteredStrings(CSourcePhrase* pSrcPhrase,
								  wxString& markersStr, 
								  wxString& endMarkersStr,
								  wxString& freeTransStr,
								  wxString& noteStr,
								  wxString& collBackTransStr,
								  wxString& filteredInfoStr);
// use the following to empty the caller's local wxString variables for storing these info
// types 
void      EmptyMarkersAndFilteredStrings(
								  wxString& markersStr, 
								  wxString& endMarkersStr,
								  wxString& freeTransStr,
								  wxString& noteStr,
								  wxString& collBackTransStr,
								  wxString& filteredInfoStr); 
bool      GetSFMarkersAsArray(wxString& strToParse, wxArrayString& arr);
wxString  GetLastMarker(wxString markers);
bool      IsOneOfAndIfSoGetSpan(wxString inputStr, wxString& charSet, int& span); // BEW added 22May14
bool      IsNotOneOfNorSpaceAndIfSoGetSpan(wxChar* pStart, wxChar* pEnd, wxString& charSet, int& span); // BEW added 22May14
wxString  ReduceStringToStructuredPuncts(wxString& inputStr); // BEW added 22May14
bool      IsWhiteSpace(const wxChar *pChar);
int       ParseWhiteSpace(const wxChar *pChar); // returns a length (num chars of whitespace)
int       ParseMarker(const wxChar *pChar); // returns a length (num chars in the marker, including backslash)
// Any strings in pPossiblesArray not already in pBaseStrArray, append them to
// pBaseStrArray, return TRUE if at least one was added, FALSE if none were added
// BEW 11Oct10, added bool bExcludeDuplicates parameter, default FALSE; the default now is
// to accept all the contents of the pPossiblesArray without testing if a duplicate is
// being stored; if the flag is TRUE, then only the strings not already in pBaseStrArray
// are accepted
bool      AddNewStringsToArray(wxArrayString* pBaseStrArray, wxArrayString* pPossiblesArray,
							bool bExcludeDuplicates = FALSE);
bool      HasFilteredInfo(CSourcePhrase* pSrcPhrase);
bool      IsFreeTranslationContentEmpty(CSourcePhrase* pSrcPhrase); // moved from CAdapt_ItView
bool      IsBackTranslationContentEmpty(CSourcePhrase* pSrcPhrase); // moved from CAdapt_ItView
wxString  GetFilteredStuffAsUnfiltered(CSourcePhrase* pSrcPhrase, bool bDoCount, 
							bool bCountInTargetText, bool bIncludeNote = TRUE);
wxString  RebuildFixedSpaceTstr(CSourcePhrase* pSingleSrcPhrase); // BEW created 11Oct10
wxString  FromMergerMakeTstr(CSourcePhrase* pMergedSrcPhrase, wxString Tstr, bool bDoCount, 
							bool bCountInTargetText);
wxString  FromSingleMakeTstr(CSourcePhrase* pSingleSrcPhrase, wxString Tstr, bool bDoCount, 
							bool bCountInTargetText);
wxString  FromSingleMakeSstr(CSourcePhrase* pSingleSrcPhrase, bool bAttachFilteredInfo,
							bool bAttach_m_markers, wxString& mMarkersStr, wxString& xrefStr,
							wxString& filteredInfoStr, bool bDoCount, bool bCountInTargetText);
wxString  FromMergerMakeSstr(CSourcePhrase* pMergedSrcPhrase);
wxString  FromMergerMakeGstr(CSourcePhrase* pMergedSrcPhrase);
wxString  GetSrcPhraseBeginningInfo(wxString appendHere, CSourcePhrase* pSrcPhrase, 
							bool& bAddedSomething); // like ExportFunctions.cpp's
					// AppendSrcPhraseBeginningInfo(), except it doesn't try to access
					// filtered information, nor the m_markers member; because this
					// function is used in FromMergerMakeSstr() which accesses those
					// members externally to such a call as this
bool	  IsBareMarkerInArray(wxString& bareMkr, wxArrayString& arr);
bool	  IsContainedByRetranslation(int nFirstSequNum, int nCount, int& nSequNumFirst,
									   int& nSequNumLast);
bool	  IsNullSrcPhraseInSelection(SPList* pList);
bool	  IsRetranslationInSelection(SPList* pList);
bool	  IsFixedSpaceSymbolInSelection(SPList* pList);
bool	  IsFixedSpaceSymbolWithin(CSourcePhrase* pSrcPhrase);
bool	  IsFixedSpaceSymbolWithin(wxString& str); // overload, for checking m_targetPhrase, etc
bool	  IsFixedSpace(wxChar* ptr); // quick way to detect ~ or ] or [ at ptr
bool	  IsSubstringWithin(wxString& testStr, wxString& strItems); // tests if one of strings in
											// testStr is a match for any string in strItems
void	  SeparateOutCrossRefInfo(wxString inStr, wxString& xrefStr, wxString& othersFilteredStr);
wxString  MakeSpacelessPunctsString(CAdapt_ItApp* pApp, enum WhichLang whichLang); // BEW created 16Feb12

// a helper for docVersion 6, used in view's MakeTargetStringIncludingPunctuation()
// function; 
bool	  IsPhraseBoxAdaptionUnchanged(CSourcePhrase* pSrcPhrase, wxString& tgtPhrase);

// A helper for KB Sharing, to check certain language codes exist, and if they don't, to
// let the user set them using the language codes dialog
bool CheckLanguageCodes(bool bSrc, bool bTgt, bool bGloss, bool bFreeTrans, bool& bUserCancelled);
// A helper for checking if username strings needed are set, and if not, to open dialog
// for doing so, we'll also make it possible to do this from the view menu
bool CheckUsername(); // returns TRUE if all's well, FALSE if user hit 
					  // Cancel button in the internal dialog

// A helper for the wxList class (legacy class, using Node*) - to replace the pointed at original
// CSourcePhrase instance (param 2) at whatever Node it is stored on, with the pointed at
// new CSourcePhrase instance (param 3) at the same Node, returning that Node's pointer.
// The bDeleteOriginal parameter, when TRUE, causes the document's function
// DeleteSingleSrcPhrase() to be called with pOriginalSrcPhrase as its parameter, and if
// bDeletePartnerPileToo is TRUE, that partner pile is also located and deleted;
// bDeleteOriginal can be TRUE, and bDeletePartnerPile FALSE (when deleting a
// CSourcePhrase instance not yet shown in the view's layout, for example, so it doesn't
// yet have a partner pile), and bDeleteOriginal can be FALSE, in which case
// bDeletePartnerPileToo is ignored. pList (param 1) can be any list of CSourcePhrase
// instances, but most likely it will be the app's m_pSourcePhrases list which defines the
// document.
// If pOriginalSrcPhrase cannot be found in pList, then NULL is returned and the
// replacement is not made.
// So far, this function is unused
// BEW created 13Jan11
//SPList::Node* SPList_ReplaceItem(SPList*& pList, CSourcePhrase* pOriginalSrcPhrase, 
//						CSourcePhrase* pNewSrcPhrase, bool bDeleteOriginal = TRUE, 
//						bool bDeletePartnerPileToo = TRUE);

// uuid support
wxString  GetUuid();

// for date-time stamping in KB or elsewhere
wxString  GetDateTimeNow(enum AppPreferedDateTime dt = adaptItDT);

// for KB metadata, to create a string indicating who supplied the adaptation (or gloss)
// For LAN-based collaboration, we will try set the string: "userID:machineID" such as
// watersb:BEW for the "watersb" account on the computer visible on the LAN as "BEW"; and
// anticipating the future when we hope to have both web-based KB sharing, and / or
// AI-in-the-cloud, we'll want to store something different - such as (at least) an IP
// address, so we'll have a bool parameter to distinguish the web versus non-web sourcing
// possibilities
wxString  SetWho(bool bOriginatedFromTheWeb = FALSE);

//size_t GetFileSize_t(wxString& absPathToFile);

// next two use tellenc.cpp
bool      IsLoadableFile(wxString& absPathToFile);
// determine endian value for theText, from how the bytes are ordered
// bool IsLittleEndian(wxString& theText);
//	GDLC Nov11 There is no need to supply IsLittleEndian() with a wxString because it calls
//	tellenc2() which takes a const unsigned char* const buffer which is what reading the file
//	gives us anyway - char * to wxString to char * is useless conversion! But we do need a
//	parameter for the length of the buffer.
bool      IsLittleEndian(const unsigned char* const pCharBuf, unsigned int size_in_bytes);

/* unused, but perfect good
bool PopulateTextCtrlByLines(wxTextCtrl* pText, wxString* pPath, int numLines = -1);
*/
bool      PopulateTextCtrlWithChunk(wxTextCtrl* pText, wxString* pPath, int numKilobytes = -1);
// GetNewFile was formerly in CAdapt_ItDoc class, and its return enum in Adapt_It.h
// BEW 19July10, added 4th param, so as to be able to get just a certain number of kB of
// the file for viewing (approximate kB, we truncate the end a bit more to ensure the
// characters that remain are valid for the encoding); if the 4th param is zero, it gets
// the whole file and does no truncating at the end
enum getNewFileState GetNewFile(wxString*& pstrBuffer, wxUint32& nLength, 
								wxString pathName, int numKBOnly = 0);

bool      SelectedFoldersContainSourceDataFolder(wxArrayString* pFolders);
// BEW created 9Aug10, for support of user-protection from folder navigation
void      RemoveNameDuplicatesFromArray(wxArrayString& originals, wxArrayString& unwanted,
								   bool bSorted, enum ExtensionAction extAction);
void      ChangeParatextPrivatesToCustomMarkers(wxString& buffer);
wxString  RemoveCustomFilteredInfoFrom(wxString str); // BEW 11Oct10, removes \free, \note, \bt info
// BEW 11Oct10, get arrays containing src & tgt puncts added, and another two for puncts removed
void      AnalysePunctChanges(wxString& srcPunctsBefore, wxString& tgtPunctsBefore,
			 wxString& srcPunctsAfter, wxString& tgtPunctsAfter,
			 wxArrayInt*& pSrcPunctsRemovedArray, wxArrayInt*& pSrcPunctsAddedArray,
			 wxArrayInt*& pTgtPunctsRemovedArray, wxArrayInt*& pTgtPunctsAddedArray);
// BEW created 17Jan11, used when converting back from docV5 to docV4
bool      HasParagraphMkr(wxString& str);
// BEW created 20Jan11, to avoid adding duplicates of ints already in the passed in
// wxArrayInt, whether or not keep_strips_keep_piles is used for RecalcLayout() - the
// contents won't be used if another layout_selector enum value is in effect, because
// RecalcLayout() would recreate the strips and repopulate the partner piles in such
// situations 
void      AddUniqueInt(wxArrayInt* pArrayInt, int nInt);
void      AddUniqueString(wxArrayString* pArrayStr, wxString& str); // BEW created 11Sep11

// input: pList = ptr to the SPList to be converted to a dynamic array
// output: pArray = ptr to the SPArray which is passed in empty and populated within;
// Note: the pointers will then be managed by a list and an array, so take care when deleting
void      ConvertSPList2SPArray(SPList* pList, SPArray* pArray);

// extract a range of CSourcePhrase instances' ptrs (leave m_nSequNumber values unchanged)
// from the passed in pInputArray, for the range [nStartAt,nEndAt], populating pSubarray
// with the extracted pointers -- Note: the pointers will be then managed by two arrays,
// so take care when deleting
void      ExtractSubarray(SPArray* pInputArray, int nStartAt, int nEndAt, SPArray* pSubarray);

// BEW 9Aug11, This uses code formerly only in DoFileSave() to get the doc updated with the
// current active location's phrase box contents - first putting the box at sequ num = 0 if
// not currently visible. We save the box contents to the KB - except here we make this
// optional - settable by a bool param (param1). The bNoStore ref param2 passes back to
// the caller the value TRUE if no store is done when attempted, of if a <Not In KB> entry
// for the active CSourcePhrase forces no store be done. Param3 controls whether
// or not the message to alert the user to a failure to store the contents in the KB gets
// shown.
void     UpdateDocWithPhraseBoxContents(bool bAttemptStoreToKB, bool& bNoStore, 
									bool bSuppressWarningOnStoreKBFailure = FALSE);

// Use this to work out where to move a modal dialog to; call it in it's InitDialog()
// function at the end of the function, do a dialog frame window GetSize() call first to
// get the (x,y,w,h) rectangle values, pass back the calculated (top,left) in screen
// coords, and supply a dialog window SetSize() to do the reposition -- see
// ConsistencyCheckDlg for an example of use
void     RepositionDialogToUncoverPhraseBox(CAdapt_ItApp* pApp, int x, int y, int w, int h, 
									int XPos, int YPos, int& myTopCoord, int& myLeftCoord);

// Use this to work out where to move a modal dialog to; call it in it's InitDialog()
// function at the end of the function, do a dialog frame window GetSize() call first to
// get the (x,y,w,h) rectangle values, pass back the calculated (top,left) in screen
// coords, and supply a dialog window SetSize() to do the reposition -- see
// Free Translation's Adjust dialog for an example of use. 
// This version makes some checks which in some circumstances can indicate there is a
// secondary monitor and the phrasebox is within that secondary monitor -- indicated by
// left coord of phrase box in device coords being either negative (primary monitor is on
// right, AI is displaying on a monitor to its left), or large positive and greater than
// width of primary monitor (primary monitor is on the left and AI is displaying in a
// secondary monitor to its right); when AI is displaying on the primary monitor, we've no
// way to determine reliably and x-platform that there's a secondary monitor too, so in
// this circumstance we display the Adjust dialog to the top right or bottom right of the
// primary monitor - depending on where the phrase box is.
void     RepositionDialogToUncoverPhraseBox_Version2(CAdapt_ItApp* pApp, int x, int y, int w, int h, 
									int XPos, int YPos, int& myTopCoord, int& myLeftCoord);

// Use this to pass in a 2- or 3-letter ethnologue code, and get back its print name
// string, and inverted name string (internally, gets the file "iso639-3codes.txt" into a
// wxString buffer (the file has the 2-letter codes listed first, then the 3-letter ones,
// and looks up the code, and parses the required string from the rest of that line, and
// returns it in param 2. Return TRUE if no error, FALSE if something went wrong and the
// returned string isn't defined.
bool     GetLanguageCodePrintName(wxString code, wxString& printName);

wxString PutSrcWordBreak(CSourcePhrase* pSrcPhrase); // get it from m_srcWordBreak in pSrcPhrase,
				// or return a latin space if app's m_bUseSrcWordBreak is FALSE - test internally
wxString PutTgtWordBreak(CSourcePhrase* pSrcPhrase); // get it from m_tgtWordBreak in pSrcPhrase,
				// or return a latin space if app's m_bUseSrcWordBreak is FALSE - test internally
				// (yes, I meant m_bUseSrcWordBreak in the above line - it covers both tgt
				// and src contexts when it comes to testing in an if then else test)
wxString PutSrcWordBreakFrTr(CSourcePhrase* pSrcPhrase); // get it from m_srcWordBreak in pSrcPhrase,
				// or return a latin space if app's m_bFreeTransUsesZWSP is FALSE - test internally

//wxString RemoveCharFromString(wxString &str, wxChar ch); // removes all instances of ch from str

// a handy utility for counting how many space-delimited words occur in str
int CountSpaceDelimitedWords(wxString& str);

#if defined (_KBSERVER)

bool CheckForValidUsernameForKbServer(wxString url, wxString username, wxString password); // BEW 6Jun13

bool CheckForSharedKbInKbServer(wxString url, wxString username, wxString password,
					wxString srcLangCode, wxString tgtLangCode, int kbType);
CBString MakeDigestPassword(const wxString& user, const wxString& password);

#endif

// Support for ZWSP insertion in any AI wxTextCtrl (e.g. see OnKeyUp() in ComposeBarEditBox.cpp)
void OnCtrlShiftSpacebar(wxTextCtrl* pTextCtrl);

// A global function for doing any normalization operations necessary because a major
// change is about to be done (such as changing doc, changing project, exit from app,
// something requiring doc to be loaded and in a robust state, etc)
void NormalizeState();


// The following are two diagnostic functions which can be used for chasing any bug
// resulting from the partner piles not having all required values filled out, especially
// m_pSrcPhrase and m_pOwningPile, and so not being properly in sync with the doc list;
// uncomment places
// where the #define _debugLayout is, and the calls below
//void ShowSPandPile(int atSequNum, int whereTis);
//void ShowInvalidStripRange();


#ifdef __WXMAC__
// GDLC 6May11 Added to avoid trying to include the Mach OS headers inside the class CAdapt_ItApp
wxMemorySize MacGetFreeMemory(void);
#endif

#endif	// helpers_h

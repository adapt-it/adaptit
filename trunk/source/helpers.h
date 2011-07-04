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
#include "Adapt_It.h"
#include "MergeUpdatedSrc.h"

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
	oxesDateOnly
};

enum CompareUsfmTexts
{
	noDifferences,
	usfmOnlyDiffers,
	textOnlyDiffers,
	usfmAndTextDiffer
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

bool FileHasNewerModTime(wxString fileAndPathTrueIfNewer, wxString fileAndPathFalseIfNewer);

bool FileContainsBookIndicator(wxString FilePath, wxString& out_BookIndicatorInSpecifiedFile);

//bool SearchFileContentForFixedLengthPattern(wxString FilePath, wxInt8 *Pattern, wxInt8 *PatternMask, wxUint32 PatternLength);

bool IsAnsiLetter(wxChar c);

bool IsAnsiDigit(wxChar c);

bool IsAnsiDigitsOnly(wxString s);

bool IsAnsiLetterOrDigit(wxChar c);

bool IsValidFileName(wxString s);

bool IsUsfmDocument(SPList* pList, bool* pbIsEither); // if FALSE is returned, examine *pbIsEither
	// value, if TRUE then the outcome was indeterminate (either set could be selected meaningfully),
	// if FALSE, then PNG 1998 SFM set is indicated
size_t EvaluateMarkerSetForIndicatorCount(CSourcePhrase* pSrcPhrase, enum SfmSet set);

wxString StripPath(wxString FullPath);

SPList *SplitOffStartOfList(SPList *MainList, int FirstIndexToKeep);

// functions added by whm
// whm Note: the following group of functions share a lot of code with 
// their counterparts in the Doc class. However, these functions are all 
// used on buffers that might be in existence when the actual Doc does 
// not exist. Hence I've put underscores in their function names so they 
// won't be confused with the similarly named functions in the 
// CAdapt_ItDoc class.
bool Is_AnsiLetter(wxChar c);
bool Is_ChapterMarker(wxChar* pChar);
bool Is_VerseMarker(wxChar *pChar, int& nCount);
wxString GetStringFromBuffer(const wxChar* ptr, int itemLen);
int Parse_Number(wxChar *pChar);
bool Is_WhiteSpace(wxChar *pChar, bool& IsEOLchar);
bool Is_NonEol_WhiteSpace(wxChar *pChar);
//int ParseWhiteSpace(wxChar *pChar);
int Parse_NonEol_WhiteSpace(wxChar *pChar);
int Parse_Marker(wxChar *pChar, wxChar *pEnd); // modified from the one in the Doc
bool Is_Marker(wxChar *pChar, wxChar *pEnd);	// modified from the one in the Doc
wxString GetNumberFromChapterOrVerseStr(const wxString& verseStr);
wxArrayString GetUsfmStructureAndExtent(wxString& sourceFileBuffer);
wxString GetInitialUsfmMarkerFromStructExtentString(const wxString str);
wxString GetFinalMD5FromStructExtentString(const wxString str);
enum CompareUsfmTexts CompareUsfmTextStructureAndExtent(const wxArrayString& usfmText1, const wxArrayString& usfmText2);
bool GetNextVerseLine(const wxArrayString usfmText, int& index);
bool IsTextOrPunctsChanged(wxString& oldText, wxString& newText); // text is usually src
bool IsUsfmStructureChanged(wxString& oldText, wxString& newText); // text is usually src

wxString SpanIncluding(wxString inputStr, wxString charSet);
// the following is an overload for using in a parser
wxString SpanIncluding(wxChar* ptr, wxChar* pEnd, wxString charSet); // BEW added 11Oct10

wxString ParseWordInwardsFromEnd(wxChar* ptr, wxChar* pEnd, 
			wxString& wordBuildersForPostWordLoc, wxString charSet); // BEW created 28Jan11

wxString SpanExcluding(wxString inputStr, wxString charSet);
// the following is an overload for using in a parser  <<-- deprecated 29Jan11
//wxString SpanExcluding(wxChar* ptr, wxChar* pEnd, wxString charSet); // BEW added 11Oct10

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

wxString GetUniqueIncrementedFileName(wxString baseFilePathAndName, int digitWidth, wxString suffix);

// end of whm's additions
 
// 2010 additions by BEW

wxString GetConvertedPunct(const wxString& rStr); // moved from view class to here 11Oct10

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
bool GetSFMarkersAsArray(wxString& strToParse, wxArrayString& arr);
wxString GetLastMarker(wxString markers);
bool IsWhiteSpace(const wxChar *pChar);
int ParseWhiteSpace(const wxChar *pChar); // returns a length (num chars of whitespace)
int ParseMarker(const wxChar *pChar); // returns a length (num chars in the marker, including backslash)
// Any strings in pPossiblesArray not already in pBaseStrArray, append them to
// pBaseStrArray, return TRUE if at least one was added, FALSE if none were added
// BEW 11Oct10, added bool bExcludeDuplicates parameter, default FALSE; the default now is
// to accept all the contents of the pPossiblesArray without testing if a duplicate is
// being stored; if the flag is TRUE, then only the strings not already in pBaseStrArray
// are accepted
bool AddNewStringsToArray(wxArrayString* pBaseStrArray, wxArrayString* pPossiblesArray,
						  bool bExcludeDuplicates = FALSE);
bool HasFilteredInfo(CSourcePhrase* pSrcPhrase);
bool IsFreeTranslationContentEmpty(CSourcePhrase* pSrcPhrase); // moved from CAdapt_ItView
bool IsBackTranslationContentEmpty(CSourcePhrase* pSrcPhrase); // moved from CAdapt_ItView
wxString GetFilteredStuffAsUnfiltered(CSourcePhrase* pSrcPhrase, bool bDoCount, 
									  bool bCountInTargetText, bool bIncludeNote = TRUE);
wxString RebuildFixedSpaceTstr(CSourcePhrase* pSingleSrcPhrase); // BEW created 11Oct10
wxString FromMergerMakeTstr(CSourcePhrase* pMergedSrcPhrase, wxString Tstr, bool bDoCount, 
							bool bCountInTargetText);
wxString FromSingleMakeTstr(CSourcePhrase* pSingleSrcPhrase, wxString Tstr, bool bDoCount, 
							bool bCountInTargetText);
wxString FromSingleMakeSstr(CSourcePhrase* pSingleSrcPhrase, bool bAttachFilteredInfo,
				bool bAttach_m_markers, wxString& mMarkersStr, wxString& xrefStr,
				wxString& filteredInfoStr, bool bDoCount, bool bCountInTargetText);
wxString FromMergerMakeSstr(CSourcePhrase* pMergedSrcPhrase);
wxString FromMergerMakeGstr(CSourcePhrase* pMergedSrcPhrase);
wxString GetSrcPhraseBeginningInfo(wxString appendHere, CSourcePhrase* pSrcPhrase, 
					 bool& bAddedSomething); // like ExportFunctions.cpp's
					// AppendSrcPhraseBeginningInfo(), except it doesn't try to access
					// filtered information, nor the m_markers member; because this
					// function is used in FromMergerMakeSstr() which accesses those
					// members externally to such a call as this
bool	 IsBareMarkerInArray(wxString& bareMkr, wxArrayString& arr);
bool	 IsContainedByRetranslation(int nFirstSequNum, int nCount, int& nSequNumFirst,
									   int& nSequNumLast);
bool	 IsNullSrcPhraseInSelection(SPList* pList);
bool	 IsRetranslationInSelection(SPList* pList);
bool	 IsFixedSpaceSymbolInSelection(SPList* pList);
bool	 IsFixedSpaceSymbolWithin(CSourcePhrase* pSrcPhrase);
bool	 IsFixedSpaceSymbolWithin(wxString& str); // overload, for checking m_targetPhrase, etc
bool	 IsFixedSpaceOrBracket(wxChar* ptr); // quick way to detect ~ or ] or [ at ptr
bool	 IsSubstringWithin(wxString& testStr, wxString& strItems); // tests if one of strings in
											// testStr is a match for any string in strItems
void	 SeparateOutCrossRefInfo(wxString inStr, wxString& xrefStr, wxString& othersFilteredStr);


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
wxString GetUuid();

// for date-time stamping in KB or elsewhere
wxString GetDateTimeNow(enum AppPreferedDateTime dt = adaptItDT);

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
wxString RemoveCustomFilteredInfoFrom(wxString str); // BEW 11Oct10, removes \free, \note, \bt info
// BEW 11Oct10, get arrays containing src & tgt puncts added, and another two for puncts removed
void AnalysePunctChanges(wxString& srcPunctsBefore, wxString& tgtPunctsBefore,
			 wxString& srcPunctsAfter, wxString& tgtPunctsAfter,
			 wxArrayInt*& pSrcPunctsRemovedArray, wxArrayInt*& pSrcPunctsAddedArray,
			 wxArrayInt*& pTgtPunctsRemovedArray, wxArrayInt*& pTgtPunctsAddedArray);
// BEW created 17Jan11, used when converting back from docV5 to docV4
bool HasParagraphMkr(wxString& str);
// BEW created 20Jan11, to avoid adding duplicates of ints already in the passed in
// wxArrayInt, whether or not keep_strips_keep_piles is used for RecalcLayout() - the
// contents won't be used if another layout_selector enum value is in effect, because
// RecalcLayout() would recreate the strips and repopulate the partner piles in such
// situations 
void AddUniqueInt(wxArrayInt* pArrayInt, int nInt);

// input: pList = ptr to the SPList to be converted to a dynamic array
// output: pArray = ptr to the SPArray which is passed in empty and populated within;
// Note: the pointers will then be managed by a list and an array, so take care when deleting
void ConvertSPList2SPArray(SPList* pList, SPArray* pArray);

// extract a range of CSourcePhrase instances' ptrs (leave m_nSequNumber values unchanged)
// from the passed in pInputArray, for the range [nStartAt,nEndAt], populating pSubarray
// with the extracted pointers -- Note: the pointers will be then managed by two arrays,
// so take care when deleting
void ExtractSubarray(SPArray* pInputArray, int nStartAt, int nEndAt, SPArray* pSubarray);

// The following are two diagnostic functions which can be used for chasing any bug
// resulting from the partner piles not having all required values filled out, especially
// m_pSrcPhrase and m_pOwningPile, and so not being properly in sync with the doc list;
// uncomment places
// where the #define _debugLayout is, and the calls below
//void ShowSPandPile(int atSequNum, int whereTis);
//void ShowInvalidStripRange();

// returns the absolute path to the folder being used as the Adapt It work folder, whether
// in standard location or in a custom location - but for the custom location only
// provided it is a "locked" custom location (if not locked, then the path to the standard
// location is returned, i.e. m_workFolderPath, rather than m_customWorkFolderPath). The
// "lock" condition ensures that a snooper can't set up a PT or BE collaboration and
// the remote user not being aware of it.
wxString SetWorkFolderPath_For_Collaboration();
bool IsEthnologueCodeValid(wxString& code);
// the next function is created from OnWizardPageChanging() in Projectpage.cpp, and
// tweaked so as to remove support for the latter's context of a wizard dialog
bool HookUpToExistingAIProject(CAdapt_ItApp* pApp, wxString* pProjectName, wxString* pProjectFolderPath);
// a module for doing the layout and getting the view ready for the user to start
// adapting;; it is not limited to being used in a Collaboration scenario
void SetupLayoutAndView(CAdapt_ItApp* pApp, wxString& docTitle);
// move the newSrc string of just-obtained (from PT or BE) source text, currently in the
// .temp folder, to the __SOURCE_INPUTS folder, creating the latter folder if it doesn't
// already exist, and storing in a file with filename constructed from fileTitle plus an
// added .txt extension; if a file of that name already exists there, overwrite it.
bool MoveTextToFolderAndSave(CAdapt_ItApp* pApp, wxString& folderPath, 
				wxString& pathCreationErrors, wxString& theText, wxString& fileTitle);
wxString GetTextFromFileInFolder(CAdapt_ItApp* pApp, wxString folderPath, wxString& fileTitle);
wxString GetTextFromAbsolutePathAndRemoveBOM(wxString& absPath);
bool OpenDocWithMerger(CAdapt_ItApp* pApp, wxString& pathToDoc, wxString& newSrcText, 
					   bool bDoMerger, bool bDoLayout, bool bCopySourceWanted);
void UnloadKBs(CAdapt_ItApp* pApp);

#ifdef __WXMAC__
// GDLC 6May11 Added to avoid trying to include the Mach OS headers inside the class CAdapt_ItApp
wxMemorySize MacGetFreeMemory(void);
#endif

#endif	// helpers_h

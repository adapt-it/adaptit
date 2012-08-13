/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ExportFunctions.h
/// \author			Bruce Waters, revised for wxWidgets by Bill Martin
/// \date_created	31 January 2008
/// \date_revised	31 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is a header file containing export functions used by Adapt It. 
/////////////////////////////////////////////////////////////////////////////
//
#ifndef ExportFunctions_h
#define ExportFunctions_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ExportFunctions.h"
#endif

// wxHashMap uses this macro for its declaration
WX_DECLARE_HASH_MAP( wxString,		// the map key is the bare sfm marker
                    wxString,		// the map value is the string of RTF Tags
                    wxStringHash,
                    wxStringEqual,
                    MapBareMkrToRTFTags ); // the name of the map class declared by this macro

WX_DECLARE_HASH_MAP( wxString,		// the map key is the whole sfm marker (with backslash)
                    wxString,		// the map value is the color string values (see BuildColorTableFromUSFMColorAttributes)
                    wxStringHash,
                    wxStringEqual,
					MapMkrToColorStr );

/* defined in KB.h
enum ExportType
{
	sourceTextExport,
	targetTextExport,
	glossesTextExport,
	freeTransTextExport
};
*/
class CAdapt_ItView; // forward ref
//enum UniqueFileIncrementMethod; // whm 28Jul12 removed; gives gcc compile error

// main export functions:
wxString	AddSpaceIfNotFFEorX(wxString str, CSourcePhrase* pSrcPhrase);
wxString	AppendSrcPhraseBeginningInfo(wxString appendHere, CSourcePhrase* pSrcPhrase, 
					 bool& bAddedSomething, bool bIncludeNote,
					 bool bDoCountForFreeTrans, bool bCountInTargetTextLine); // BEW created 11Oct10
wxString	AppendSrcPhraseEndingInfo(wxString appendHere, CSourcePhrase* pSrcPhrase); // BEW created 11Oct10
void		DoExportAsType(enum ExportType exportType); // BEW removed bForceUTF8Conversion 21July12
void		DoExportInterlinearRTF();
void		DoExportTextToRTF(enum ExportType exportType, wxString exportPath, 
							  wxString exportName, wxString& Buffer);
// BEW created 9Jun12
void		DoExportAsXhtml(enum ExportType exportType, bool bBypassFileDialog_ProtectedNavigation,
							wxString defaultDir, wxString exportFilename, wxString filter, bool bShowMessageIfSucceeded);
// components defined for simplifying the code of DoExportAsXhtml()
bool		DeclineIfUnstructuredData();
bool		DeclineIfNoBookCode(wxString& bookCode);
bool		DeclineIfNoIso639LanguageCode(ExportType exportType, wxString& langCode);
wxString	GetCleanExportedUSFMBaseText(ExportType exportType);
wxString	ApplyNormalizingFiltersToTheUSFMText(wxString text);
bool		WriteXHTML_To_File(wxString exportPath, CBString& text, bool bShowMessageIfSucceeded);
#if defined(__WXDEBUG__)
void		XhtmlExport_DebuggingSupport(); // use only in debug mode -- and 
					// internally has a few #defines to specify what to do
#endif
// next one is usable in any function where the defaultDir for a file dialog needs to be
// computed in a protected navigation on or off support situation
bool		GetDefaultDirectory_ProtectedNav(bool bProtectFromNavigation, 
					wxString fixedOutputPath, wxString lastOutputPath, wxString& defaultDir);
wxString	PrepareUniqueFilenameForExport(wxString exportFilename, bool bDoAlways,
					UniqueFileIncrementMethod enumMethod, bool bUseSuffix);

// end components for simplifying the code of DoExportAsXhtml()

// The following ParseWordRTF() function is the same as the legacy ParseWord() function in the Doc before
// Bruce rewrote it for doc v 5 purposes. I've renamed it to ParseWordRTF and reclaimed it here for RTF output
// purposes.
int			ParseWordRTF(wxChar *pChar, wxString& precedePunct, wxString& followPunct,wxString& SpacelessSrcPunct);

// below are supporting functions for the above main export functions:
int			RebuildSourceText(wxString& srcText, SPList* pList = NULL); // 2nd param for rebuilding from any list
wxString	RebuildText_For_Collaboration(SPList* pList, enum ExportType exportType, bool bFilterCustomMarkers = TRUE);
int			RebuildTargetText(wxString& target, SPList* pList = NULL);
int			RebuildGlossesText(wxString& glossText, SPList* pList = NULL);
int			RebuildFreeTransText(wxString& freeTransText, SPList* pList = NULL);
wxString	ApplyOutputFilterToText(wxString& textStr, wxArrayString& bareMarkerArray,
									wxArrayInt& filterFlagsArray, bool bRTFOutput);
wxString	ApplyOutputFilterToText_For_Collaboration(wxString& textStr, wxArrayString& bareMarkerArray);
int			CountWordsInFreeTranslationSection(bool bCountInTargetText, SPList* pList, int nAnchorSequNum);
SPList::Node* DoPlacementOfMarkersInRetranslation(SPList::Node* firstPos,SPList* pSrcPhrases, wxString& Tstr);
wxString	RemoveCollectedBacktranslations(wxString& str);
// the following 6 added, for doc version 5 support, to hide implementation details 
// for the information storage in CSourcePhrase
bool		AreMarkersOrFilteredInfoStoredHere(CSourcePhrase* pSrcPhrase, bool bIgnoreEndMarkers = TRUE);
bool		IsFootnoteInDoc(CSourcePhrase* pSrcPhrase, bool bIgnoreFilteredFootnotes = FALSE);
bool		IsEndnoteInDoc(CSourcePhrase* pSrcPhrase, bool bIgnoreFilteredEndnotes = FALSE);
bool		IsFreeTransInDoc(CSourcePhrase* pSrcPhrase);
bool		IsBackTransInDoc(CSourcePhrase* pSrcPhrase);
bool		IsNoteInDoc(CSourcePhrase* pSrcPhrase);
// BEW added next 13Dec10 to support export from documents which, in the original input
// file, did not have any SFMs. (AI puts \p where each newline is, and these need to be
// removed on export.) I wrote these then found out that FormatUnstructuredTextBufferForOutput()
// was written for this purpose and wasn't working because it was not returning the
// modified data, so once I fixed that, it works (with a bit of an additional tweak) and
// so these can be commented out
//bool		IsDocWithParagraphMarkersOnly(SPList* pSrcPhrasesList);
//wxString	RemoveParagraphMarkersOnly(wxString& str);

// BEW 26Aug10, added for Paratext \z feature support
void		ChangeCustomMarkersToParatextPrivates(wxString& buffer);

bool		DetachedNonQuotePunctuationFollows(wxChar* pOld, wxChar* pEnd, 
											   wxChar* pPosAfterMkr, wxString& spacelessPuncts);
int			GetWordCount(wxString& str, wxArrayString* pStrList);
void		FormatMarkerBufferForOutput(wxString& str, enum ExportType expType);
void		FormatUnstructuredTextBufferForOutput(wxString& str, bool bRTFOutput);
int			ParseFootnote(wxChar* pChar, wxChar* pBuffStart, wxChar* pEndChar, 
							enum ParseError& parseError);
int			ParseEndnote(wxChar* pChar, wxChar* pBuffStart, wxChar* pEndChar, 
							enum ParseError& parseError);
int			ParseCrossRef(wxChar* pChar, wxChar* pBuffStart, wxChar* pEndChar, 
							enum ParseError& parseError);
bool		IsACharacterStyle(wxString styleMkr, MapBareMkrToRTFTags& rtfMap);
bool		ProcessAndWriteDestinationText(wxFile& f, wxFontEncoding Encoding, wxString& destStr,
							bool& bIsAtEnd, enum DestinationTextType destTxtType, 
							MapBareMkrToRTFTags& rtfMap, CAdapt_ItDoc* pDoc, 
							enum ParseError& parseError, enum CallerType& callerType, 
							wxString suppliedCaller, bool bSuffixFreeTransToFootnoteBody, 
							wxString& freeAssocStr);
wxString	GetUnfilteredInfoMinusMMarkersAndCrossRefs(CSourcePhrase* pSrcPhrase,
							SPList* pSrcPhrases, wxString filteredInfo_NoXRef,
							wxString collBackTransStr, wxString freeTransStr,
							wxString noteStr, bool bDoCount, bool bCountInTargetText);
wxString	GetUnfilteredCrossRefsAndMMarkers(wxString prefixStr, wxString markersStr, 
							wxString xrefStr, bool bAttachFilteredInfo, bool bAttach_m_markers);
wxString	GetStyleNumberStrFromRTFTagStr(wxString tagStr, int& startPos, int& endPos); // whm added 18Oct05
bool		MarkerIsToBeFilteredFromOutput(wxString bareMarkerForLookup); // whm added 18Nov05
wxString	GetANSIorUnicodeRTFCharsFromString(wxString inStr);
bool		WriteOutputString(wxFile& f, wxFontEncoding Encoding, const wxString& OutStr);
int			ParseMarkerRTF(wxChar* pChar, wxChar* pEndChar); // whm added 22Nov05
bool		IsMarkerRTF(wxChar *pChar, wxChar* pBuffStart); // whm added 22Nov05
int			ParseMarkerAndAnyAssociatedText(wxChar* pChar, wxChar* pBuffStart, 
							wxChar* pEndChar, wxString bareMarkerForLookup, wxString wholeMarker,
							bool parsingRTFText, bool InclCharFormatMkrs); // whm added 11Nov05
wxString	EscapeAnyEmbeddedRTFControlChars(wxString& textStr);
wxString	FormatRTFFootnoteIntoString(wxString callerStr, wxString assocMarkerText,
							wxString noteRefNumStr, wxString fnCallerTags, wxString fnTextTags,
							wxString annotRefTags, wxString annotTextTags, bool addSpBeforeCallerStr);
void		DivideTextForExtentRemaining(wxClientDC& dC, int extentRemaining, wxString inputStr,
							wxString& fitInRowStr,	wxString& spillOverStr);
bool		IsRTFControlWord(wxChar* pChar, wxChar* pEndChar);
int			ParseRTFControlWord(wxChar* pChar, wxChar* pEndChar); // whm added 22Nov05
bool		WriteOutputStringConvertingAngleBrackets(wxFile& f, wxFontEncoding Encoding, wxString& OutStr,wxChar* inptr);
bool		IsCharacterFormatMarker(wxChar* pChar);
void		BuildRTFTagsMap(wxArrayString& StyleDefStrArray, wxArrayString& StyleInDocStrArray,
							wxString OutputFont,MapMkrToColorStr& colorMap,wxString Sltr_precedence);
bool		OutputTextAsBoxedParagraph(wxFile& f, wxString& assocText, wxString bareMkr,
							bool bProcessingTable, enum BoxedParagraphType boxType);
bool		OutputAnyBTorFreeMaterial(wxFile& f, wxFontEncoding WXUNUSED(Encoding), wxString Marker, 
							wxString bareMkr, wxString& assocText, wxString& LastStyle, 
							wxString& LastParaStyle, int& callerRefNumInt, bool& bLastParagraphWasBoxed, 
							enum ParseError& parseError, enum CallerType& callerType, 
							bool bProcessingTable, bool bPlaceFreeTransInRTFText, 
							enum BoxedParagraphType boxType, CAdapt_ItDoc* pDoc);
wxString	BuildColorTableFromUSFMColorAttributes(MapMkrToColorStr& colorMap);
void		DetermineRTFDestinationMarkerFlagsFromBuffer(wxString& textStr,
							bool& bDocHasFootnotes,
							bool& bDocHasEndnotes,
							bool& bDocHasFreeTrans,
							bool& bDocHasBackTrans,
							bool& bDocHasAINotes);
wxString	ChangeMkrs_vn_vt_To_v(wxString text); // BEW created 19May12
wxString	ChangeTildeToNonBreakingSpace(wxString text); // BEW created 19May12
void		CountTotalCurlyBraces(wxString outputStr, int& nOpeningBraces, int& nClosingBraces);
int			ClearBuffer();
wxString	IntToRoman(int n);
int			ParseAnyFollowingChapterLabel(wxChar* pChar, wxChar* pBuffStart, wxChar* pEndChar, 
							wxString& tempLabel);
bool		PunctuationFollowsDestinationText(int itemLen, wxChar* pChar, wxChar* pEnd, bool OutputSrc);
bool		NextMarkerIsFootnoteEndnoteCrossRef(wxChar* pChar, wxChar* pEndChar, int itemLen);
bool		IsBTMaterialHaltingPoint(wxString Marker);
bool		IsFreeMaterialHaltingPoint(wxString Marker);
int			ParseEscapedCharSequence(wxChar *pChar, wxChar *pEndChar);
int			GetMaxMarkerLength(); // whm added 17Oct05
void		BuildRTFStyleTagString(USFMAnalysis* pSfm, wxString& Sdef, wxString& Sindoc,
							int styleSequNum, wxString outputFontStr, 
							wxString colorTblIndxStr, int mkrMaxLength,
							wxString Sltr_precedence); // whm added 17Oct05
void		SortAndResolveStyleIndexRefs(wxArrayString& StyleDefStrArray,
							wxArrayString& StyleInDocStrArray);
void		ProcessIrregularTagsInArrayStrings(wxArrayString& StyleDefStrArray,wxArrayString& StyleInDocStrArray); // whm added 21Oct05
void		AddAnyStylenameColon(wxString& tempStr, USFMAnalysis* pSfm);
void		AddAnyParaAlignment(wxString& tempStr, USFMAnalysis* pSfm);
void		AddAnyParaIndents(wxString& tempStr, USFMAnalysis* pSfm, wxString& save_ri_N_value, wxString& save_li_N_value);
void		AddAnyParaSpacing(wxString& tempStr, USFMAnalysis* pSfm);
void		AddAnyParaKeeps(wxString& tempStr, USFMAnalysis* pSfm);
void		AddAnyRinLin(wxString& tempStr, wxString save_ri_N_value, wxString save_li_N_value);
void		AddAnyCharEnhancements(wxString& tempStr, USFMAnalysis* pSfm);
void		AddAnyFontSizeColor(wxString& tempStr, USFMAnalysis* pSfm, wxString colorTblIndxStr);
void		AddAnyBasedonNext(wxString& tempStr, USFMAnalysis* pSfm);
wxString	RemoveFreeTransWordCountFromStr(wxString freeStr); // whm added 3Dec05
wxString	GetStyleNumberStrAssociatedWithMarker(wxString bareMkr,
							wxArrayString& StyleDefStrArray, int& indx); // whm added 18Oct05
void		RemoveMarkersOfType(enum TextType textType, wxString& text);
int			FindMkrInMarkerInventory(wxString mkr); // BEW added 3Aug11
void		ExcludeCustomMarkersFromExport(); // BEW added 3Aug11
void		ExcludeCustomMarkersAndRemFromExport(); // BEW added 19May12
bool		MakeAndSaveMyCSSFile(wxString path, wxString fname, wxString pathToAIDefaultCSS); // BEW added 13Aug2012
#endif //ExportFunctions_h

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

// The following lists are for DoExportInterlinearRTF
//WX_DECLARE_LIST(wxString, CellxNList); // see list definition macro in .cpp file
//WX_DECLARE_LIST(wxString, CellxNListFree); // see list definition macro in .cpp file
//WX_DECLARE_LIST(wxString, CellxNListBT); // see list definition macro in .cpp file

/// wxList declaration and partial implementation of the SrcList class being
/// a list of pointers to wxString objects
WX_DECLARE_LIST(wxString, SrcList); // see list definition macro in .cpp file

/// wxList declaration and partial implementation of the TgtList class being
/// a list of pointers to wxString objects
WX_DECLARE_LIST(wxString, TgtList); // see list definition macro in .cpp file

/// wxList declaration and partial implementation of the GlsList class being
/// a list of pointers to wxString objects
WX_DECLARE_LIST(wxString, GlsList); // see list definition macro in .cpp file

/// wxList declaration and partial implementation of the NavList class being
/// a list of pointers to wxString objects
WX_DECLARE_LIST(wxString, NavList); // see list definition macro in .cpp file

/// wxList declaration and partial implementation of the FreeTransList class being
/// a list of pointers to wxString objects
WX_DECLARE_LIST(wxString, FreeTransList); // see list definition macro in .cpp file

/// wxList declaration and partial implementation of the BackTransList class being
/// a list of pointers to wxString objects
WX_DECLARE_LIST(wxString, BackTransList); // see list definition macro in .cpp file

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

/* defined in Adapt_It.h
enum ExportType
{
	sourceTextExport,
	targetTextExport,
	glossesTextExport,
	freeTransTextExport
};
*/

// main export functions:
void DoExportSfmText(enum ExportType exportType, bool bForceUTF8Conversion); // BEW created 6Aug09
void DoExportInterlinearRTF();
//void DoExportSrcOrTgtRTF(bool OutputSrc, wxString exportPath, wxString exportName, wxString& Buffer);
void DoExportTextToRTF(enum ExportType exportType, wxString exportPath, wxString exportName, wxString& Buffer);

// below are supporting functions for the above main export functions:
int RebuildSourceText(wxString& srcText);
int RebuildTargetText(wxString& target);
int RebuildGlossesText(wxString& glossText);
int RebuildFreeTransText(wxString& freeTransText);
wxString ApplyOutputFilterToText(wxString& textStr,
		wxArrayString& bareMarkerArray, wxArrayInt& filterFlagsArray,
		bool bRTFOutput);
int CountWordsInFreeTranslationSection(bool bCountInTargetText, SPList* pList, int nAnchorSequNum);
SPList::Node* DoPlacementOfMarkersInRetranslation(SPList::Node* firstPos,SPList* pSrcPhrases,
		wxString& Tstr, wxString& Sstr, wxString& Gstr, wxString& Nstr);
bool DetachedNonQuotePunctuationFollows(wxChar* pOld, wxChar* pEnd, wxChar* pPosAfterMkr, wxString& spacelessPuncts);
int GetWordCount(wxString& str, wxArrayString* pStrList);
void FormatMarkerBufferForOutput(wxString& str);
void FormatUnstructuredTextBufferForOutput(wxString& str, bool bRTFOutput);
int ParseFootnote(wxChar* pChar, wxChar* pBuffStart, wxChar* pEndChar, 
		enum ParseError& parseError);
int ParseEndnote(wxChar* pChar, wxChar* pBuffStart, wxChar* pEndChar, 
		enum ParseError& parseError);
int ParseCrossRef(wxChar* pChar, wxChar* pBuffStart, wxChar* pEndChar, 
		enum ParseError& parseError);
bool IsACharacterStyle(wxString styleMkr, MapBareMkrToRTFTags& rtfMap);
bool ProcessAndWriteDestinationText(wxFile& f, wxFontEncoding Encoding, wxString& destStr,
		bool& bIsAtEnd, enum DestinationTextType destTxtType, 
		MapBareMkrToRTFTags& rtfMap,
		CAdapt_ItDoc* pDoc, enum ParseError& parseError,
		enum CallerType& callerType, wxString suppliedCaller,
		bool bSuffixFreeTransToFootnoteBody, wxString& freeAssocStr);
wxString GetStyleNumberStrFromRTFTagStr(wxString tagStr, int& startPos, int& endPos); // whm added 18Oct05
bool MarkerIsToBeFilteredFromOutput(wxString bareMarkerForLookup); // whm added 18Nov05
wxString GetANSIorUnicodeRTFCharsFromString(wxString inStr);
bool WriteOutputString(wxFile& f, wxFontEncoding Encoding, const wxString& OutStr);
wxString GetStringWithoutMarkerAndAssocText(wxString wholeMkr, wxString wholeEndMkr, 
		wxString inStr, wxString& assocText, wxString& wholeMkrRemoved);
int ParseMarkerRTF(wxChar* pChar, wxChar* pEndChar); // whm added 22Nov05
bool IsMarkerRTF(wxChar *pChar, wxChar* pBuffStart); // whm added 22Nov05
int ParseMarkerAndAnyAssociatedText(wxChar* pChar, wxChar* pBuffStart, 
		wxChar* pEndChar, wxString bareMarkerForLookup, wxString wholeMarker,
		bool parsingRTFText, bool InclCharFormatMkrs); // whm added 11Nov05
wxString EscapeAnyEmbeddedRTFControlChars(wxString& textStr);
wxString FormatRTFFootnoteIntoString(wxString callerStr, wxString assocMarkerText,
		wxString noteRefNumStr, wxString fnCallerTags, wxString fnTextTags,
		wxString annotRefTags, wxString annotTextTags, 
		bool addSpBeforeCallerStr);
void DivideTextForExtentRemaining(wxClientDC& dC, int extentRemaining, wxString inputStr,
		wxString& fitInRowStr,
		wxString& spillOverStr);
bool IsRTFControlWord(wxChar* pChar, wxChar* pEndChar);
int ParseRTFControlWord(wxChar* pChar, wxChar* pEndChar); // whm added 22Nov05
bool WriteOutputStringConvertingAngleBrackets(wxFile& f, wxFontEncoding Encoding, wxString& OutStr,wxChar* inptr);
bool IsCharacterFormatMarker(wxChar* pChar);
void BuildRTFTagsMap(wxArrayString& StyleDefStrArray, wxArrayString& StyleInDocStrArray,
		wxString OutputFont,MapMkrToColorStr& colorMap,wxString Sltr_precedence);
bool OutputTextAsBoxedParagraph(wxFile& f, wxString& assocText,
		wxString bareMkr, bool bProcessingTable,
		enum BoxedParagraphType boxType);
bool OutputAnyBTorFreeMaterial(wxFile& f, wxFontEncoding WXUNUSED(Encoding), wxString Marker, wxString bareMkr,
		wxString& assocText, wxString& LastStyle, wxString& LastParaStyle, 
		int& callerRefNumInt, bool& bLastParagraphWasBoxed, 
		enum ParseError& parseError, enum CallerType& callerType, 
		bool bProcessingTable,
		bool bPlaceFreeTransInRTFText, enum BoxedParagraphType boxType, 
		CAdapt_ItDoc* pDoc);
wxString BuildColorTableFromUSFMColorAttributes(MapMkrToColorStr& colorMap);
void DetermineRTFDestinationMarkerFlagsFromBuffer(wxString& textStr,
		bool& bDocHasFootnotes,
		bool& bDocHasEndnotes,
		bool& bDocHasFreeTrans,
		bool& bDocHasBackTrans,
		bool& bDocHasAINotes);
void CountTotalCurlyBraces(wxString outputStr, int& nOpeningBraces, int& nClosingBraces);
int ClearBuffer();
wxString IntToRoman(int n);
int ParseAnyFollowingChapterLabel(wxChar* pChar, wxChar* pBuffStart, wxChar* pEndChar, 
		wxString& tempLabel);
bool PunctuationFollowsDestinationText(int itemLen, wxChar* pChar, wxChar* pEnd, bool OutputSrc);
bool NextMarkerIsFootnoteEndnoteCrossRef(wxChar* pChar, wxChar* pEndChar, int itemLen);
bool IsBTMaterialHaltingPoint(wxString Marker);
bool IsFreeMaterialHaltingPoint(wxString Marker);
int ParseEscapedCharSequence(wxChar *pChar, wxChar *pEndChar);
int GetMaxMarkerLength(); // whm added 17Oct05
void BuildRTFStyleTagString(USFMAnalysis* pSfm, wxString& Sdef, wxString& Sindoc,
		int styleSequNum, wxString outputFontStr, 
		wxString colorTblIndxStr, int mkrMaxLength,
		wxString Sltr_precedence); // whm added 17Oct05
void SortAndResolveStyleIndexRefs(wxArrayString& StyleDefStrArray,
		wxArrayString& StyleInDocStrArray);
void ProcessIrregularTagsInArrayStrings(wxArrayString& StyleDefStrArray,wxArrayString& StyleInDocStrArray); // whm added 21Oct05
void AddAnyStylenameColon(wxString& tempStr, USFMAnalysis* pSfm);
void AddAnyParaAlignment(wxString& tempStr, USFMAnalysis* pSfm);
void AddAnyParaIndents(wxString& tempStr, USFMAnalysis* pSfm, wxString& save_ri_N_value, wxString& save_li_N_value);
void AddAnyParaSpacing(wxString& tempStr, USFMAnalysis* pSfm);
void AddAnyParaKeeps(wxString& tempStr, USFMAnalysis* pSfm);
void AddAnyRinLin(wxString& tempStr, wxString save_ri_N_value, wxString save_li_N_value);
void AddAnyCharEnhancements(wxString& tempStr, USFMAnalysis* pSfm);
void AddAnyFontSizeColor(wxString& tempStr, USFMAnalysis* pSfm, wxString colorTblIndxStr);
void AddAnyBasedonNext(wxString& tempStr, USFMAnalysis* pSfm);
wxString RemoveFreeTransWordCountFromStr(wxString freeStr); // whm added 3Dec05
wxString GetStyleNumberStrAssociatedWithMarker(wxString bareMkr,
		wxArrayString& StyleDefStrArray, int& indx); // whm added 18Oct05

#endif //ExportFunctions_h

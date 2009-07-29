/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Adapt_It.h
/// \author			Bill Martin
/// \date_created	05 January 2004
/// \date_revised	29 April 2009
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public 
///                 License (see license directory)
/// \description This is the header file for the CAdapt_ItApp class and the AIModalDialog
/// class.  
/// The CAdapt_ItApp class initializes Adapt It's application and gets it running. Most of
/// Adapt It's global enums, structs and variables are declared either as members of the
/// CAdapt_ItApp class or in this source file's global space. The AIModalDialog class
/// provides Adapt It with a modal dialog base class which turns off Idle and UIUpdate
/// processing while the dialog is being shown.
/// \derivation		The CAdapt_ItApp class is derived from wxApp, and inherits its support 
///                 for the document/view framework.
/// The AIModalDialog class is derived from wxDialog.
/////////////////////////////////////////////////////////////////////////////
#ifndef Adapt_It_h
#define Adapt_It_h

class AIPrintout;
// for debugging m_bNoAutoSave not getting preserved across app closure and relaunch...
// comment out when the wxLogDebug() calls are no longer needed
//#define Test_m_bNoAutoSave

/////////////////// MFC to wxWidgets Type Conversions //////////////////////////////////////
// MFC type:					wxWidgets Equivalent:
//	DWORD (unsigned long)			wxUint32
//	COLORREF (unsigned long)		WXCOLORREF (unsigned long)
//	UINT (unsigned int)				wxUint32
//	BYTE (unsigned char)			wxUint8
//	WORD							wxUint16 (or wxInt16 ???)
//	TCHAR							wxChar (Note: wxWidgets pre-defines TCHAR as wchar_t
//											in Unicode builds, char in ANSI builds)
//	LPCTSTR							const wxChar*
//	LPTSTR							wxChar*
//	HBRUSH							wxUint32
/////////////////// MFC to wxWidgets Type Conversions //////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Adapt_It.h"
#endif

// ******************************* my #defines *********************************************

#define VERSION_MAJOR_PART 5
#define VERSION_MINOR_PART 0
#define VERSION_BUILD_PART 1
#define PRE_RELEASE 0  // set to 0 (zero) for normal releases; 1 to indicate "Pre-Release" in About Dialog
#define VERSION_DATE_DAY 29
#define VERSION_DATE_MONTH 7
#define VERSION_DATE_YEAR 2009

//#define _ALT_LAYOUT_ // BEW May09, if defined, it keeps CPile pointer copies out of the
                       // strips, indices used instead; if not defined, CPile* copies are
					   // in strips -- our final design requires the CPile pointers to be
					   // stored in the strips
#define _NEW_LAYOUT // BEW May09, if not #defined, strips are only destroyed & rebuilt, 
                      // never kept & tweaked; if #defined, piles & strips are retained &
                      // tweaked where necessary to update after user editing -- our final
                      // design requires this #define be set

// uncomment to turn on wxLogDebug tracking of gnBeginInsertionsSequNum & gnEndInsertionsSequNum
//#define Highlighting_Bug

// The __WXDEBUG__ and _DEBUG symbols are defined automatically when doing a debug build

// The following define is for testing the wxSplitterWindow on the wx version.
// To add the splitter window uncomment the line below and rebuild. (It may be buggy.
// Leave it, nobody seems to use it.)
//#define _USE_SPLITTER_WINDOW

// The following define causes the SIL Converters specific code to be included in the build. Initially
// this may only work for the Windows port, so I've also conditionally defined the USE_SIL_CONVERTERS
// define to be available only for __WXMSW__
#ifdef __WXMSW__
// Comment out the line below to test the removal of SIL Converters code on Windows builds
#define USE_SIL_CONVERTERS
#endif

// uncomment the define below to output KB I/O benchmarks (in debug mode only)
#define SHOW_KB_I_O_BENCHMARKS
#define SORTKB 1 // change to 0 for output of legacy unsorted KB map entries

#define SHOW_DOC_I_O_BENCHMARKS

#ifdef _UNICODE
#define _RTL_FLAGS  // for the m_bSrcRTL etc flags and supporting code, the Layout menu, etc
#endif

// *************************** end my #defines *********************************************

// The MFC version uses resource.h to store resource symbols, however we are using
// Adapt_It_Resources.h which simply includes the wxDesigner produced C++ resources by
// including "Adapt_It_wdr.h"
#include "Adapt_It_Resources.h"
#include "AdaptitConstants.h"
#include <wx/datetime.h>
#include <wx/file.h>
#include <wx/ffile.h>


// Does wxWidgets recognize/utilize these clipboard defines???
#ifdef _UNICODE
#define CF_CLIPBOARDFORMAT CF_UNICODETEXT
#else
#define CF_CLIPBOARDFORMAT CF_TEXT
#endif

#include "PhraseBox.h"
#include "FindReplace.h"

// forward references (added to wxWidgets version):
class wxSingleInstanceChecker;
class wxConfig;
class wxCmdLineParser;
class wxTextFile;
class CMainFrame; // added for GetMainFrame()
// forward references (from MFC version) below:
class CKB;
class CAdapt_ItView;
class CPhraseBox;
class CAdapt_ItDoc;
class CPunctCorrespPageCommon;
class CUSFMPageCommon;
class CFilterPageCommon;
class CStdioFile;
class CPrintingDlg;
class CStdioFileEx;
class CBString;

// forward references added when moved data structures from the View to the App
class CSourceBundle;
class CPile;
class CCell;
class CEarlierTranslationDlg;
class CFindReplace;
class CNoteDlg;
class CViewFilteredMaterialDlg;
class wxHelpControllerBase;
class wxHtmlHelpController;
class CConsistentChanger;
class wxPropertySheetDialog;

// forward references for refactored view layout support
class CLayout;
class CSourcePhrase;
class SPList;

// The following constants were originally declared in the global space of XML.h. G++ 3.x
// could find them but the g++ 4.x linker can't find them even though XML.h is included
// above, so I've moved them here to the App's header.
// whm CAUTION: Some of the original identifier names/symbols used in the MFC version are
// very short containing only one or two letters, which makes it more likely that they
// might conflict with other defined symbols in the code. Therefore I've prefixed the
// symbols with xml_ in the wx version.

/// For Adapt It document output as XML, and parsing of XML elements.
const char xml_adaptitdoc[] = "AdaptItDoc";
/// For Adapt It document output as XML, and parsing of XML elements.
const char xml_settings[] = "Settings";
/// For Adapt It document output as XML, and parsing of XML elements.
const char xml_scap[] = "S";
/// For Adapt It document output as XML, and parsing of XML elements.
const char xml_mpcap[] = "MP";
/// For Adapt It document output as XML, and parsing of XML elements.
const char xml_mmcap[] = "MM";

// Attribute names for Adapt It XML documents

/// Attribute name used in Adapt It XML documents
const char xml_docversion[] = "docVersion";
/// Attribute name used in Adapt It XML documents
const char xml_sizex[] = "sizex";
/// Attribute name used in Adapt It XML documents
const char xml_sizey[] = "sizey";
/// Attribute name used in Adapt It XML documents
const char xml_specialcolor[] = "specialcolor";
/// Attribute name used in Adapt It XML documents
const char xml_retranscolor[] = "retranscolor";
/// Attribute name used in Adapt It XML documents
const char xml_navcolor[] = "navcolor";
/// Attribute name used in Adapt It XML documents
const char xml_curchap[] = "curchap";
/// Attribute name used in Adapt It XML documents
const char xml_srcname[] = "srcname";
/// Attribute name used in Adapt It XML documents
const char xml_tgtname[] = "tgtname";
/// Attribute name used in Adapt It XML documents
const char xml_others[] = "others";

// next ones are for the sourcephrases themselves

/// Attribute name used in Adapt It XML documents
const char xml_a[] = "a"; // m_adaption
/// Attribute name used in Adapt It XML documents
const char xml_k[] = "k"; // m_key
/// Attribute name used in Adapt It XML documents
const char xml_s[] = "s"; // m_srcPhrase
/// Attribute name used in Adapt It XML documents
const char xml_t[] = "t"; // m_targetStr
/// Attribute name used in Adapt It XML documents
const char xml_g[] = "g"; // m_gloss
/// Attribute name used in Adapt It XML documents
const char xml_f[] = "f"; // flags (32digit number, all 0 or 1)
/// Attribute name used in Adapt It XML documents
const char xml_sn[] = "sn"; // m_nSequNumber
/// Attribute name used in Adapt It XML documents
const char xml_w[] = "w"; // m_nSrcWords
/// Attribute name used in Adapt It XML documents
const char xml_ty[] = "ty"; // m_curTextType
/// Attribute name used in Adapt It XML documents
const char xml_pp[] = "pp"; // m_precPunct
/// Attribute name used in Adapt It XML documents
const char xml_fp[] = "fp"; // m_follPunct
/// Attribute name used in Adapt It XML documents
const char xml_i[] = "i"; // m_inform
/// Attribute name used in Adapt It XML documents
const char xml_c[] = "c"; // m_chapterVerse
/// Attribute name used in Adapt It XML documents
const char xml_m[] = "m"; // m_markers
/// Attribute name used in Adapt It XML documents
const char xml_mp[] = "mp"; // some medial punctuation
/// Attribute name used in Adapt It XML documents
const char xml_mm[] = "mm"; // one or more medial markers (no filtered stuff)

// tag & attribute names for KB i/o

/// Attribute name used in Adapt It XML KB i/o
const char xml_aikb[] = "AdaptItKnowledgeBase";
/// Attribute name used in Adapt It XML KB i/o
const char xml_kb[] = "KB";
/// Attribute name used in Adapt It XML KB i/o
const char xml_gkb[] = "GKB";
/// Attribute name used in Adapt It XML KB i/o
const char xml_map[] = "MAP";
/// Attribute name used in Adapt It XML KB i/o
const char xml_tu[] = "TU";
/// Attribute name used in Adapt It XML KB i/o
const char xml_rs[] = "RS";
/// Attribute name used in Adapt It XML KB i/o
const char xml_srcnm[] = "srcName";
/// Attribute name used in Adapt It XML KB i/o
const char xml_tgtnm[] = "tgtName";
/// Attribute name used in Adapt It XML KB i/o
const char xml_n[] = "n";
/// Attribute name used in Adapt It XML KB i/o
const char xml_max[] = "max";
/// Attribute name used in Adapt It XML KB i/o
const char xml_mn[] = "mn";
/// Attribute name used in Adapt It XML KB i/o
const char xml_xmlns[] = "xmlns";

/// struct for saving top and bottom logical coord offsets for printing pages, stored in
/// m_pagesList Instances of PageOffsets are populated in the PaginateDoc() function in the
/// View.
struct PageOffsets
{
	int nTop; // logical coord offset of first strip on page (from start of the 
			  // logical/virtual document)
	int nBottom; // logical coord offset of last strip on page (from start of the 
				 // logical/virtual document)
	int nFirstStrip; // 0-based index of the first strip to appear on the current page
	int nLastStrip; // 0-based index of the last strip to appear on the current page
};

/// wxList declaration and partial implementation of the POList class being
/// a list of pointers to PageOffsets objects
WX_DECLARE_LIST(PageOffsets, POList); // see list definition macro in .cpp file

/// wxList declaration and partial implementation of the CCellList class being
/// a list of pointers to CCell objects
WX_DECLARE_LIST(CCell, CCellList); // see list definition macro in .cpp file



// globals

/// A struct containing the information relevant to writing a subpart of the free
/// translation in a single rectangle under a single strip. Struct members include:
/// horizExtent and subRect.
struct FreeTrElement 
{
	int horizExtent;
	wxRect subRect;
};

/// An enum for return error-state from GetNewFile()
enum getNewFileState
{
	getNewFile_success,
	getNewFile_error_at_open,
	getNewFile_error_opening_binary,
	getNewFile_error_no_data_read,
	getNewFile_error_unicode_in_ansi
};

/// An enum for specifying the selection extension direction, 
/// either right or left.
enum extendSelDir
{
	right,
	left
};

/// An enum for specifying how the ChooseInterfaceLanguage() function 
/// is called. Struct members include: userInitiated and firstRun.
enum SetInterfaceLanguage
{
	userInitiated,
	firstRun
};

/// An enum for specifying the search case type, either caseSensitive
/// or caseInsensitive.
enum SearchCaseType
{
	caseSensitive,
	caseInsensitive
};

/// An enum for specifying the search string length type, either 
/// subString or exactString.
enum SearchStrLengthType
{
	subString,
	exactString
};

enum StartFromType
{
	fromFirstListPos,
	fromCurrentSelPosToListEnd,
	fromCurrentSelPosCyclingBack
};

/// A struct for specifying time settings. Struct members include: 
/// m_tsDoc, m_tsKB, m_tLastDocSave, and m_tLastKBSave.
struct TIMESETTINGS 
{
	wxTimeSpan	m_tsDoc;
	wxTimeSpan	m_tsKB;
	wxDateTime	m_tLastDocSave;
	wxDateTime	m_tLastKBSave;
};

/// A struct for specifying paired source and target punctuation characters. 
/// Struct members include: charSrc and charTgt.
struct PUNCTPAIR 
{
	wxChar		charSrc;
	wxChar		charTgt;
};

/// A struct for specifying paired source and target two-punctuation characters. Struct
/// members include: twocharSrc[2] and twocharTgt[2].
struct TWOPUNCTPAIR 
{
	wxChar	twocharSrc[2];
	wxChar	twocharTgt[2];
};

/// A struct for use in the DoKBRestore() function, stores key string plus number of words
/// in it. Struct members include: key and count.
struct KeyPlusCount
{
	wxString key;
	int		count;
};

/// A struct for storing font information that is mainly relevant to the MFC version. In
/// the wx version it is mainly used for config file input and output of font data, so that
/// the wx version config files maintain compatibility with the MFC version config files.
/// Struct members include: fLangType, fHeight, fPointSize, fWidth, fEscapement,
/// fOrientation, fWeight, fWeightConfSave, fItalic, fStyle, fUnderline, fStrikeOut,
/// fCharset, fEncoding, fOutPrecision, fClipPrecision, fQuality, fPitchAndFamily, fFamily,
/// and fFaceName.
struct fontInfo 
{
	wxString fLangType;	// = "SourceFont", "TargetFont", or "NavTextFont"
						// Note: those marked *(n) are used in wxWidgets' 
						// wxFont constructor n = order of parameters
	int fHeight;		// = conversion of point size to lfHeight using 
						// PointSizeToNegHeight(); saved in conf
	int fPointSize;		//(1) = conversion of lfHeight to point size using 
						// NegHeightToPointSize(); used by program
	int fWidth;			// = lfWidth (written as 0 but not used in wxWidgets)
	int fEscapement;	// = lfEscapement (written as 0 but not used in wxWidgets)
	int fOrientation;	// = lfOrientation (written as 0 but not used in wxWidgets)
	int fWeight;		//(4) = lfWeight (wxWidgets <-> MFC: wxLIGHT <-> 400, 
						// wxNORMAL <-> 500, and wxBOLD <-> 700)
	int fWeightConfSave;// (4) = lfWeight (raw value read from/saved to conf file)
	bool fItalic;		// = lfItalic (wxWidgets <-> MFC: wxNORMAL <-> 0, 
						// wxSLANT and wxITALIC <-> 1)
	int fStyle;			//(3) = (wxNORMAL, wxSLANT, and wxITALIC in wxWidgets 
						// converted to/from fItalic)
	bool fUnderline;	//(5) = lfUnderline (written as 0 for FALSE, 1 for TRUE)
	bool fStrikeOut;	// = lfStrikeOut (written as 0 for FALSE, 1 for TRUE, 
						// not used in wxWidgets)
	int fCharset;		// = lfCharSet (a compatibility value to maintain 
                        // transferrable config files with the MFC app. This value is
                        // written to config file in as an int value consistent with MFC's
                        // fCharset as used in its LOGFONT structure. When the config file
                        // is read from the config file, the fCharset value is mapped to
                        // the appropriate encoding enum value for the font used in the
                        // wxWidgets' context. This enum value is stored in the fEncoding
                        // struct member below, and is the actual value which is used for
                        // the font during the running of the wx app.
	wxFontEncoding fEncoding;//(7) Not part of MS LOGFONT struct, but used in the wx app 
                        // as the mapped encoding value for the font. We'll use
                        // wxFONTENCODING_DEFAULT regardless of what lfCharSet is stored in
                        // config
	int fOutPrecision;	// = lfOutPrecision (written as 3 but not used in wxWidgets)
	int fClipPrecision;	// = lfClipPrecision (written as 2 but not used in wxWidgets)
	int fQuality;		// = lfQuality (written as 1 but not used in wxWidgets)
	int fPitchAndFamily;// = conversion of family and DEFAULT_PITCH to lfPitchAndFamily 
						// using bit mask; saved in conf
	int fFamily;		//(2) = conversion of lfPitchAndFamily to family using bit mask
	wxString fFaceName;	//(6) = lfFaceName (not to exceed 32 characters in Windows)
};

// the following adapted from PoEdit source code Copyright (C) 2003-2006 Vaclav Slavik

/// A struct for specifying interface language information. Struct members include: 
/// fullName, code and shortName.
struct LangInfo
{
    const wxChar *fullName;
    wxLanguage code;
    const wxChar *shortName;
};

struct CurrLocalizationInfo
{
	int curr_UI_Language;			// for languages known to wxWidgets this is 
									// equivalent to a wxLanguage enum
	wxString curr_shortName;		// the ISO 639 name of the form xx or xx_XX 
									// where xx is the 2-letter language code
	wxString curr_fullName;			// the common descriptive name for the language
	wxString curr_localizationPath;	// the path where the above CurrLocalizationInfo
					// folder (curr_shortName) is located (which contains <appName>.mo)
};

//#if defined(__UNIX__) && !defined(__WXMAC__)
//    #define NEED_CHOOSELANG_UI 0
//#else
//    #define NEED_CHOOSELANG_UI 1
//#endif

/// wxList declaration and partial implementation of the KPlusCList class being
/// a list of pointers to KeyPlusCount objects
WX_DECLARE_LIST(KeyPlusCount, KPlusCList); // see list definition macro in .cpp file

// WX NOTE: The MFC CPtrArray collection class supports arrays of void pointers.
// wxWidgets does not have an exact equivalent, but the docs say it predefines 
// the following standard array classes: wxArrayInt, wxArrayLong, and wxArrayPtrVoid.
// We should be able to get the desired behavior with the wxArrayPtrVoid class.

/// A struct for Bible Book folders support. Struct members include: dirName, 
/// seeName and bookCode.
struct BookNamePair 
{
	wxString dirName;
	wxString seeName;
	wxString bookCode;
};

// whm - the next five were declared static functions in MFC, but not defined that way in
// their implementations. so I've removed "static" here, to avoid gcc warnings
void			SetupSinglesArray(wxArrayPtrVoid* pBooks); // used in default setup, 
														   // if "books.xml" absent
void			SetupBookCodesArray(wxArrayPtrVoid* pBookCodes); // used in default 
													// setup if "books.xml" is absent
BookNamePair*	MakePair(BookNamePair*& pPair, wxChar* dirNm, wxChar* seeNm);
void			SetupDefaultBooksArray(wxArrayPtrVoid* pBookStructs);

// for USFM Filtering support
void SetupDefaultStylesMap();

/// An enum for specifying the type of rebuilding to be done when calling
/// DoUsfmSetChanges(), DoPunctuationChanges() and DoUsfmFilterChanges(), either DoReparse
/// or NoReparse.
enum Reparse
{
	DoReparse,
	NoReparse
};

/// An enum for specifying the type of filter status when calling
/// GetUnknownMarkersFromDoc(), one of the following: setAllUnfiltered, setAllFiltered,
/// useCurrentUnkMkrFilterStatus, or preserveUnkMkrFilterStatusInDoc.
enum SetInitialFilterStatus
{
    setAllUnfiltered,
    setAllFiltered,
    useCurrentUnkMkrFilterStatus,
	preserveUnkMkrFilterStatusInDoc
};

/// An enumb for specifying how to handle the loading of data into the filterPage's list
/// boxes, one of the following: LoadInitialDefaults, UndoEdits,
/// ReloadAndUpdateFromDocList, ReloadAndUpdateFromProjList or
/// ReloadAndUpdateFromFactoryList.
enum ListBoxProcess
{
	LoadInitialDefaults,
	UndoEdits,
	ReloadAndUpdateFromDocList,
	ReloadAndUpdateFromProjList,
	ReloadAndUpdateFromFactoryList
};

/// An enum for specifying which standard format marker set is to be used or is active, one
/// of the following: UsfmOnly, PngOnly, or UsfmAndPng.
enum SfmSet
{
	UsfmOnly,
	PngOnly,
	UsfmAndPng
};

/// An enum for specifying how SetupFilterPageArrays() gets its data for populating its
/// filter flag arrays, can be either useMap or useString.
enum UseStringOrMap
{
	useMap,
	useString
};

/// An enum for specifying how the filter status should be set for unknown markers in the
/// filterPage's unknownMkrStr, either setAsFiltered or setAsUnfiltered.
enum UnkMkrFilterSetting
{
	setAsFiltered,
	setAsUnfiltered
};

/// An enum for specifying how the filter flag in the USFMAnalysis objects should be set in
/// the ResetUSFMFilterStructs() function, either allInSet or onlyThoseInString.
enum resetMarkers
{
	allInSet,
	onlyThoseInString,
};

/// An enum for specifying the footnote caller type when processing and writing destination
/// text, one of the following: auto_number, no_caller, word_literal_caller or
/// supplied_by_parameter.
enum CallerType
{
	auto_number,
	no_caller,
	word_literal_caller,
	supplied_by_parameter
};

/// An enum for specifying the type of error encountered when parsing footnotes, endnotes
/// and cross-references, one of the following: no_error, no_end_marker or
/// premature_buffer_end.
enum ParseError
{
	no_error,
	no_end_marker,
	premature_buffer_end
};

/// An enum for specifying the destination text type in calls to
/// ProcessAndWriteDestinationText(), one of the following: footnoteDest, endnoteDest or
/// crossrefDest.
enum DestinationTextType
{
	footnoteDest,
	endnoteDest,
	crossrefDest
};

/// An enum for specifying the general style type of a standard format marker. Can be one
/// of the following: paragraph, character, table_type, footnote_caller, footnote_text,
/// default_para_font, footerSty, headerSty, horiz_rule, boxed_para ir hidden_note.
enum StyleType
{
	paragraph,
	character,
	table_type,	// whm added 21Oct05 and below
	footnote_caller,
	footnote_text,
	default_para_font,
	footerSty,
	headerSty,
	horiz_rule,
	boxed_para,
	hidden_note
	//note; // usfm.sty has this also as a style type
};

/// An enum for specifying the type of justification to be used in formatting a font's
/// text. Can be one of the following: leading, center, following or justified.
enum Justification
{
	leading,
	center,
	following,
	justified // added for version 3
};

/// An enum for specifying the type of boxed text to be placed around certain types of RTF
/// output. Can be either single_border or double_border.
enum BoxedParagraphType
{
	single_border,
	double_border
};

/// An enum for specifying what to do with edit box contents when the
/// StoreFreeTranslation() function is called. Can be either remove_editbox_contents or
/// retain_editbox_contents.
enum EditBoxContents
{
	remove_editbox_contents,
	retain_editbox_contents
};

/// An enum for specifying how the DrawFreeTranslations() function was called. Can be
/// either call_from_ondraw or call_from_edit.
enum DrawFTCaller
{
	call_from_ondraw,
	call_from_edit
};

/// An enum for specifying which pass is being processed in the call to
/// SetupForSFMSetChange(). Can be either first_pass or second_pass.
enum WhichPass // BEW added 10Jun05 for support of SFM set changes done on the fly
{
	first_pass,
	second_pass
};

/// An enum for specifying the fields of a USFMAnalysis struct, used in
/// ParseAndFillStruct(). Can be one of the following: marker, endMarker, description,
/// usfm, png, filter, userCanSetFilter, inLine, special, bdryOnLast, inform,
/// navigationText, textType, wrap, styleName, styleType, fontSize, color, italic, bold,
/// underline, smallCaps, superScript, justification, spaceAbove, spaceBelow,
/// leadingMargin, followingMargin, firstLineIndent, basedOn, nextStyle, keepTogether or
/// keepWithNext.
enum USFMAnalysisField
{
	// these field name are referenced in ParseAndFillStruct() in Adapt_It.cpp
	marker,
	endMarker,
	description,
	usfm,
	png,
	filter,
	userCanSetFilter,
	inLine,
	special,
	bdryOnLast,
	inform,
	navigationText,
	textType,
	wrap,
	styleName,	// addition for version 3
	styleType,
	fontSize,
	color,
	italic,
	bold,
	underline,
	smallCaps,
	superScript,
	justification,
	spaceAbove,
	spaceBelow,
	leadingMargin,
	followingMargin,
	firstLineIndent,
	basedOn,		// addition for version 3
	nextStyle,		// addition for version 3
	keepTogether,	// addition for version 3
	keepWithNext	// addition for version 3
};

// for USFM Filtering support
// When adding or remove attributes from USFMAnalysis, the following need to be
// checked for consistency:
// 1. This USFMAnalysis struct
// 2. The enum USFMAnalysisField above plus any dependent enum for the added attribute
// 3. The ParseAndFillStruct function in Adapt_It.cpp
// 4. Add any necessary const char defined string literals to beginning of XML.cpp
// 5. Change/add any necessary else if clause to the AtSFMAttr function in XML.cpp
// 6. Change/add any necessary else if clause to the ParseXMLAttribute function in XML.cpp (this
//    would be appropriate for any attribute additions involving enum values)
// 7. Change/add any necessary new USFMAnalysis struct default value for the attribute in AtSFMTag
//    in XML.cpp
// 8. Change/add any necessary > if(comment) dup else '' endif  line in UsfmXml.cct if there is
//    a default form of the attribute which should not be placed in AI_USFM.xml to save space.
// 9. The AI_USFM_full.xml attributes need to be added to the markers defined there, if applicable.
// 10. Using the stand-alone Consistent Changes program with UsfmXml.cct and UsfmXmlTidy.cct,
//    then the resulting AI_USFM.xml file deposited/installed to the Adapt It (Unicode) Work
//    directory.
// 11. The const wxString defaultSFM[] unix-style default strings (defined in Adapt_It.cpp) need to be 
//    re-assembled by moving the updated AI_USFM_full.xml file to the Adapt It (Unicode) Work directory,
//    then uncommenting the //#define Output_Default_Style_Strings symbol near the beginning of the
//    XML.h header file, and compiling/running the program until it reaches the Start Working Wizard. A
//    AI_USFM_full.txt file is generated automatically in the Adapt It (Unicode) Work directory that
//    contains the unix style default strings to be copied over those located in Adapt_It.cpp.

/// A struct for storing the attributes of usfm markers. Structs are created on the heap
/// and their pointers are associated with the standard format marker name in high-speed
/// maps for each of the three possible sfm sets. Struct members include the following:
/// marker, endMarker, description, usfm, png, filter, userCanSetFilter, inLine, special,
/// bdryOnLast, inform, navigationText, textType, wrap, styleName, styleType, fontSize,
/// color, italic, bold, underline, smallCaps, superScript, justification, spaceAbove,
/// spaceBelow, leadingMargin, followingMargin, firstLineIndent, basedOn, nextStyle,
/// keepTogether or keepWithNext.
struct USFMAnalysis
{
	wxString marker;
	wxString endMarker;
	wxString description;
	bool usfm;
	bool png;
	bool filter;
	bool userCanSetFilter;
	bool inLine;
	bool special;
	bool bdryOnLast;
	bool inform;
	wxString navigationText;
	enum TextType textType;
	bool wrap;
	wxString styleName; // addition for version 3
	enum StyleType styleType;
	int fontSize;
	int color;
	bool italic;
	bool bold;
	bool underline;
	bool smallCaps;
	bool superScript;
	enum Justification justification;
	int spaceAbove;
	int spaceBelow;
	float leadingMargin;
	float followingMargin;
	float firstLineIndent;
	wxString basedOn;	// addition for version 3
	wxString nextStyle;	// addition for version 3
	bool keepTogether;	// addition for version 3
	bool keepWithNext;	// addition for version 3
};

/// wxHashMap declaration for the MapSfmToUSFMAnalysisStruct class - a mapped association
/// of sfm marker keys (wxString) with pointers to USFMAnalysis objects.
WX_DECLARE_HASH_MAP( wxString,		// the map key is the sfm marker (bare marker without backslash)
                    USFMAnalysis*,	// the map value is the pointer to the USFMAnalysis struct instance
                    wxStringHash,
                    wxStringEqual,
                    MapSfmToUSFMAnalysisStruct ); // the name of the map class declared by this macro

/// wxHashMap declaration for the MapWholeMkrToFilterStatus class - a mapped association of
/// whole sfm markers including backslash (wxString) mapped with wxString equivalents of
/// zero "0" or one "1".
WX_DECLARE_HASH_MAP( wxString,		// the map key is the whole sfm marker (with backslash)
                    wxString,		// the map value is the string "0" or "1"
                    wxStringHash,
                    wxStringEqual,
                    MapWholeMkrToFilterStatus ); // the name of the map class declared by this macro

enum box_cursor {
	select_all,
	cursor_at_text_end,
	cursor_at_offset
};

enum removeFrom {
	from_source_text,
	from_target_text
};

// The following enums and struct were added by Bruce 12Sep08 for support of Vertical
// Editing. They were located in the global space of CAdapt_ItView.h in the MFC version.

enum ListEnum
{
	adaptationsList,
	glossesList,
	freeTranslationsList,
	notesList
};
enum WhichContextEnum
{
	precedingContext,
	followingContext
};
// BEW added 16Apr08 to support refactored source text editing and vertical editing in
// general
enum EntryPoint
{
	noEntryPoint,
	sourceTextEntryPoint,
	adaptationsEntryPoint,
	glossesEntryPoint,
	freeTranslationsEntryPoint
};

enum EditStep
{
	noEditStep, // the value when no vertical editing is going on currently
	sourceTextStep,
	adaptationsStep,
	glossesStep,
	freeTranslationsStep,
	backTranslationsStep
};

// enums for the action selector for Vertical Edit transition dialog
enum ActionSelector {
	pleaseIgnore = 0,
	nextStep,
	previousStep,
	endNow,
	cancelAllSteps
};

// enums for the GetBar() function
enum VertEditBarType 
{
	Vert_Edit_RemovalsBar,
	Vert_Edit_Bar,				// IDD_VERT_EDIT_BAR
	Vert_Edit_ComposeBar
};

typedef struct
{
    // Note: the "editable span" means the user's selection, except when it encroaches on
    // part of a retranslation, in which case the span is programmatically increased to
    // include the whole of the retranslation in the editable span. (And if the user's edit
    // is accepted, the retranslation is lost automatically and if wanted, would have to be
    // manually reconstituted.)

    // Note 2: after a source text edit, the user will be given opportunity to adapt the
    // new material - when he does this, he will be free to create mergers; hence, some of
    // the indices and the count of the number of CSourcePhrase instances in the editable
    // span after editing was completed, which are defined below, will need to be
    // programmatically decreased accordingly. Additional structs will be used for such
    // lower level entry points to the editing process, so that the user can roll back the
    // total vertical editing process at any stage before it ends. Once it ends, the memory
    // of what was done is deleted and the only rollback strategy is manual editing, or to
    // do a new source text edit at the original location.

    // Note 3: our design for the spans involves 2 spans with associated CObLists
    // containing deep copies of CSourcePhrase instances from m_pSourcePhrases list in the
    // document class: the "cancel span" to be used in restoring the original document (see
    // further comments below) if the user Cancels later on, and a modifications span
    // (coextensive with the cancel span) in which modifications are made to the deep
    // copies of the CSourcePhrase instances, readying them for the user's OK button click.
    // There are also three additional overlapping subspans, defined by starting and ending
    // index pairs using sequence number values, but these subspans do not have separate
    // CObLists associated with them, rather, they are defined on both the cancel span and
    // the modifications span, as those spans are coextensive.
	
    // The first (and minimal length) subspan is the editable span. This is the user's
    // selection, but if the selection overlaps one or more retranslations, the span is
    // widened to include the retranslation/s in its/their entirety.
    // The second subspan is the "free translations span" - it is a span of CSourcePhrase
    // instances where free translation removals need to be done.
    // The third subspan is the "collected back translations span" - it is a span of
    // CSourcePhrase instances where stored back translations have their scope overlapping
    // the information in the editable span (remembering that the editable span may be
    // exended programmatically wider than the user's selection due to the presence of one
    // or more retranslations in the user's selection, or overlapping the user's
    // selection.)
    // These subspans overlap, and involve an inclusion hierarchy: the editable span is
    // equal to or included in the free translation span which is equal to or includeld in
    // the (collected) back translations span.
    //
    // The modifications span is coextensive with the cancel span and contains deep copies
    // of the CSourcePhrase instances (with sequence numbering preserved). The
    // modifications are to make ready for the CSourcePhrase instances to be inserted in
    // the document's m_pSourcePhrases list once the user dismisses the edit source text
    // dialog with an OK button click. These placements involve replacement of the
    // instances in the editable subspan with CSourcePhrase instances created anew from the
    // edited source text (there may be zero, if the user edits out the whole of the
    // contents of the editable source text string seen in the dialog); but for any
    // extended regions preceding and following the editable span up to the bounds of the
    // cancel span, replacements of the CSourcePhrase instances are done in those regions
    // after the OK button click using the modified CSourcePhrase instances from the
    // modifications span. (The modifications involve the removal of notes, free
    // translations and collected back translations in the appropriate subspans,so that the
    // user does not have to look at their distracting (and possibly copious) information
    // while editing the editable span.) (The cancel span is maintained in case the user
    // Cancels out of the whole vertical editing process, it is not needed for cancelling
    // from the Edit Source Text dialog because when the user presses the Cancel button
    // there, no changes have been made to the original document up to that point.)
	
    // Note 4: \bt information might be found to have been stored in the editable subspan,
    // and more than one instance could be stored there; the final one of any such could
    // have a "span" (over which collection originally happened) which extends beyond the
    // editable subspan, and may even extend beyond the end of the free translations
    // subspan too - as far as it takes to get to the next location defined by
    // HaltCurrentCollection(). The nBackTrans_EndingSequNum member tracks the
    // CSourcePhrase instance immediately preceding this halt location.

    // Note 5: Because endmarkers are stored on the CSourcePhrase instance immediately
    // following the span of information to which they logically belong, it is possible for
    // the editable span's bounding CSourcePhrase instances to have (a) endmarkers at the
    // start which belong to the preceding context, and / or (b) endmarkers immediately
    // following the end of the editable span which logically belong within it. So we
    // maintain two CString members to store any such endmarker sequences from either or
    // both locations, so that the user can be shown the most logically meaningful editable
    // text string, and to enable replacement of either or both endmarker strings after the
    // edit is done - if that is an appropriate editing operation for the user to do.

    // Note 6: We also maintain an integer array to store the sequence numbers for the
    // storage locations of any Adapt It Notes which lie within the editable subspan. These
    // location values will be used (and possibly changed, depending on what the user does
    // in his editing - he may remove all source text where they originally were located)
    // in order to programmatically determine suitable locations for rebuilding the notes
    // programmatically once the edit is done. Notes content is not changed - if changes
    // are warranted, the user will have to do them manually after the total vertical edit
    // process ends.
	bool	bGlossingModeOnEntry; // at entry, TRUE if glossing mode is ON, FALSE if 
                // adapting mode is ON, vertical edit is not enabled in any other mode
                // (such as free trans mode). Default is adaptations mode is currently ON,
                // even when it isn't; this is safe because the flag is set or cleared at
                // the start of vertical editing and it is used only for restoring the
                // initial entry state after vertical editing finishes
	bool	bSeeGlossesEnabledOnEntry; // TRUE if See Glosses is ON (ie. gbGlossingEnabled
                // is TRUE) on entry, FALSE if it is OFF on entry (if off, gbIsGlossing is
                // also FALSE; but if on, gbIsGlossing can be TRUE or FALSE). Used for
                // restoring initial entry state after vert edit finishes
	bool	bEditSpanHasAdaptations; // TRUE if adaptations are detected within the
				// editable span
	bool	bEditSpanHasGlosses; //  TRUE if glosses are detected within the editable span
	bool	bEditSpanHasFreeTranslations; // TRUE if free translations are in the editable
                // span (the process looks at CSourcePhrase instances earlier and later
                // than the editable span in order to make sure the editable span is
                // included within an integral number of free translation sections)
	bool	bEditSpanHasBackTranslations; // TRUE if collected backtranslations are in the 
                // editable span. Because each back translation is stored on only one
                // CSourcePhrase instance which is first in the span over which its
                // collection happened, and sets no flag in CSourcePhrase, we need only
                // look for \bt marker content in the editable span, and in any preceding
                // context - going back only as far as the first halt position as
                // determined by the HaltCurrentCollection() function.
	bool	bCollectedFromTargetText; // TRUE if back translations were collected from the 
				// target text line, FALSE if collected from the gloss line

	int		nSaveActiveSequNum; // location of the phrase box at entry 
				// (needed only for a Cancel operation)
	wxString	oldPhraseBoxText; // contents of the phrase box when the edit was invoked
	TextType	nStartingTextType; // value of m_curTextType at the CSourcePhrase with 
				// sequence number nStartingSequNum
	TextType	nEndingTextType; // value of m_curTextType at the CSourcePhrase with 
                // sequence number nEndingSequNum (It is not possible to select across a
                // TextType boundary, and the 'none' TextType never puts a value 6 into the
                // document, instead it propagates the context's TextType through that
                // section, and so we can be confident that the TextType won't change
                // unless the user's edit has changed or introduced SF markers which change
                // the TextType -- in that case we'll need to propagate the new
                // valueforward, potentially past the end of the editable span, as far as
                // the next m_bFirstOfType == TRUE locations, but not including that
                // location.)
	wxArrayString	deletedAdaptationsList; // CStringList	deletedAdaptationsList; this is
                // the last 100 deletions resulting from either source text editing or any
                // adaptations edit operations. This list is maintained for the life of the
                // Adapt It session. A "Deletions List" button in a bar of the GUI will
                // show the list data in a combobox, in adapting mode.
	wxArrayString	deletedGlossesList; // CStringList	deletedGlossesList; // this is 
                //the last 100 deletions resulting from either source text editing or any
                //adaptations edit operations - provided the CSourcePhrase instances which
                //are involved also had glosses stored on them (not necessarily on all of
                //them). This list is maintained for the life of the Adapt It session. A
                //"Deletions List" button in a bar of the GUI will show the list data in a
                //combobox, in glossing mode.
	wxArrayString deletedFreeTranslationsList; // CStringList	deletedFreeTranslationsList; 
                // this isthe last 100 deletions resulting from either source text,
                // adaptations editing, or any use of the Remove Free Translation button in
                // the View Filtered Information dialog, or any use of the Remove button
                // when in Free Translation mode, or editing of an existing free free
                // translation. This list is maintained for the life of the Adapt It
                // session. It's members will be available in the GUI in free translation
                // mode via a combobox, as above.
	wxArrayString storedNotesList; // CStringList	storedNotesList; // the is the text 
                // strings for any Adapt It notes within the span defined by the editable
                // span when doing a source text edit are stored temporarily here, and at
                // the end of the editing process they are automatically put back into the
                // document at approximately the same relative positions, the order of
                // these notes is never scrambled, and if the new source text is very short
                // or empty, they are stored in the immediate context wholely or partly as
                // circumstances require.
	int		nStartingSequNum;	// value of m_nSequenceNum for the start of the editable 
                // span, or for adaptations editing, the start of the span of CSourcePhrase
                // instances that were involved.
	int		nEndingSequNum;	// value of m_nSequenceNum for the last CSourcePhrase of the
                // editable span, or for editing of adaptations, the end of the span of
                // CSourcePhrase instances that were involved
	int		nFreeTrans_StartingSequNum; // sequence number for the CSourcePhrase instance 
                // at the start of the span of removed free translations (it can coincide
                // with the start of the editable span but more commonly is going to be
                // somewhere preceding that location)
	int		nFreeTrans_EndingSequNum; // sequence number for the last CSourcePhrase instance
                // at the end of the span of removed free translations (it can coincide
                // with the end of the editable span but more commonly is going to be
                // somewhere following that location) the span of removed free translations
                // which follow (overlap) the editable span
	int		nBackTrans_StartingSequNum; // sequence number for the CSourcePhrase instance 
                // from which the first (or possibly only) \bt marker and its back
                // translation content were removed, this span (if it exists) has its start
                // at the free translation one's start or even before that, and if no free
                // translation span occurs then it would be at or preceding the start of
                // the editable span
	int		nBackTrans_EndingSequNum; // sequence number for the end of the back 
                // translation span, this span (if it exists) at the end of the editable
                // span, or somewhere following that, depending on where the helper
                // function, HaltCurrentCollection(), determines that collecting back
                // translations needs to end once the editable span has been traversed.
	int		nCancelSpan_StartingSequNum; // sequence number for the first CSourcePhrase
                // in the span delineated for restoring the original document state if the
                // user cancels after a source text edit has been accepted and he is
                // cancelling out of subsequent dependent (vertical) edits.
	int		nCancelSpan_EndingSequNum; // sequence number for the last CSourcePhrase in 
                // the span delineated for restoring the original document state if the
                // user cancels after a source text edit has been accepted and he is
                // cancelling out of subsequent dependent (vertical) edits.
	SPList cancelSpan_SrcPhraseList; // sequence of deep copied unedited CSourcePhrase 
                // instances from the span for of CSourcePhrase instances which get changed
                // in any way (not all need to be changed, the span is widened at either
                // end to include the leftmost and rightmost modified instances.)
	SPList	modificationsSpan_SrcPhraseList; // sequence of deep copied CSourcePhrase 
                // instances, coextensive with the cancel span (but containing fresh deep
                // copies), which will be modified (by having notes, free translations and
                // collected back translations removed) prior to showing the Edit Source
                // Text dialog, reading the information for the possibility that the user
                // will click the OK button rather than the Cancel button. Up until the OK
                // button click, no CSourcePhrase instances in the document's original
                // m_pSourcePhrases list are modified in any way; after the click, what is
                // in the modificationSpan list is used to make the document comply with
                // the user's edit result.
	SPList editableSpan_NewSrcPhraseList; // the sequence of CSourcePhrase instances 
                // resulting from the TokenizeTextString() call with the user's edited
                // source text string as a parameter
	SPList propagationSpan_SrcPhraseList; // stores a deep copy of the CSourcePhrase 
                // instance which is first in the following context (deep copied before
                // sequence numbers are changed), and any of the CSourcePhrase instances
                // which follow it which take part in any propagation of special text and
                // TextType values as a consequence of changes made within the user's
                // edited source text
	int nPropagationSpan_StartingSequNum; // the index for the first CSourcePhrase instance
				// of the following context
	int nPropagationSpan_EndingSequNum; // the index for the last CSourcePhrase in the 
                // following context which was affected by the propagation process
	wxArrayInt arrNotesSequNumbers; //CArray<int,int> arrNotesSequNumbers; // preserve old 
				// location of each removed note in the editable subspan
	int nOldSpanCount; // the original (after any extension) editable span's number of 
				// CSourcePhrase instances
	int	nNewSpanCount; // the final number of CSourcePhrase instances in the editable span 
                // after the user has completed his editing of the source text (or whatever
                // the edit did, eg. removal of a merger - but this additional stuff will
                // only be possible within the wxWidgets versions)
	wxString strInitialEndmarkers; // store here the any endmarkers stripped off the 
                // m_markers member of the editable subspan's first CSourcePhrase member
	wxString strFinalEndmarkers;  // store here the any endmarkers stripped off the 
                // m_markers member of the first CSourcePhrase member following the
                // editable subspan (at the point in the code when we do this storage, the
                // m_markers member has that substring removed, this is done in the
                // pSrcPhrase in the modifications list; and the substring needs to be
                // added to the end of the string of source text and SFM markup which he is
                // to see in the dialog, just prior to making the dialog visible)
	wxString strNewFinalEndmarkers;   // store here the any endmarkers stripped off the 
                // end of the user's edited source text string. His edit could change
                // anything, and so it is conceivable that his new text may end with
                // different endmarkers from those that will be restored to the end of the
                // editable span when he first views the dialog, so we must store the new
                // ones in case we must cancel out and restore the original document's
                // state
	bool bSpecialText; // stores the m_bSpecialText boolean value for the first 
                // CSourcePhrase instance in the editable span; TRUE if it was special
                // text, FALSE if verse or poetry
	SPList follNotesMoveSpanList; // one or more CSourcePhrase pointers from the 
                // context following the edit span, which may have had Notes moved at the
                // Note restoration stage, only used if arrNotesSequNumbers has non-zero
                // content (and the latter determines how many instances without notes on
                // them are stored in follNotesMoveSpanList to ensure safe restoration if
                // the user clicks Cancel)
	SPList precNotesMoveSpanList; // one or more CSourcePhrase pointers from the context
                // preceding the edit span, which may have had Notes moved at the Note
                // restoration stage, only used if arrNotesSequNumbers has non-zero content
                // (and the latter determines how many instances without notes on them are
                // stored in precNotesMoveSpanList to ensure safe restoration if the user
                // clicks Cancel)
	bool bTransferredFilterStuffFromCarrierSrcPhrase; // FALSE, except it is TRUE in the 
                // special circumstance that the user's edit resulted in a single
                // CSourcePhrase with empty key, and m_follPunct empty and m_markers
                // containing filtered information (this can only happen when he edited a
                // misspelled marker and the final marker form matches one designated as
                // "to be filtered out") and the whole, or end, of the user's edit string
                // was then filtered and placed in the m_markers member of the final
                // carrier CSourcePhrase; typically it would be the 'whole' rather than the
                // 'end' because the user normally cannot select across a TextType boundary
                // in the first place)
	bool bDocEndPreventedTransfer; // FALSE by default; TRUE if there was created an empty
                // carrier CSourcePhrase for storing final endmarker(s), or now-filtered
                // information, and there is no following context to which transfer of this
                // information to the m_markers member of a CSourcePhrase there is
                // possible. This flag is ignored, except when the user Cancels or an error
                // causes bail out to be done; it is used as a quick way to inform the
                // bailout function that document restoration requires the document-ending
                // CSourcePhrase instance to be deleted (when TRUE). When there is a
                // following context, transfer will have been possible and this flag will
                // be FALSE, and the earier state of the CSourcePhrase which was initial in
                // the following context will be the one at the start of the propagation
                // span.
	bool bExtendedForFiltering; // FALSE by default. TRUE if marker edit results in the 
                // edited marker being one which should be filtered, AND, the editable span
                // did not include all of the marker's filterable content - so that AI had
                // to extend the editable span by one or more words to get all that
                // material into the new source text string (we may not need to use this
                // BOOL value, but we store it in case it becomes important when I work on
                // the backtracking mechanism for stepping back through steps in the
                // vertical edit process, and/or bailout recovery)

    // next ones are for support of the adaptations update step; but when entry is at the
    // adaptations level (ie. user has changed an established adaptation), free
    // translations etc will need to be removed and a cancelSpan set up, so for that, the
    // cancelSpan supporting members above will be used.
    // The main things to note about the adaptations update step are that: 
    // (1) CSourcePhrases outside the span must not be changed (this can be done by
    // limiting the selections possible),
    // (2) the start of the span is fixed (because no source text changes are done),
    // (3) the end of the span may grow or contract (the former caused by one or more long
    // retranslations, the latter by one or more mergers, (4) extra members needed in the
    // EditRecord are few
	bool bAdaptationStepEntered; // default FALSE, TRUE if this step is set up on the 
                // screen, even if user then jumps to next step (TRUE is a flag which says
                // that the step was entered, even if immediately exitted; so data members
                // here can be expected to perhaps have content, and therefore need to be
                // looked at by processes such as cancel or bailout)
	SPList adaptationStep_SrcPhraseList; // the list of CSourcePhrase instances in the 
                // editable span, as it is at the start of the adaptations update step
                // before the user has had a chance to do anything
	int	nAdaptationStep_StartingSequNum; // the (fixed) sequence number value for the first
				// CSourcePhrase in the span
	int	nAdaptationStep_EndingSequNum; // the (fixed) sequence number value for the last 
				// CSourcePhrase in the span
	int nAdaptationStep_OldSpanCount; // how many instances are in the span when the step 
				// is first entered
	int nAdaptationStep_NewSpanCount; // how many are in the span as the user does his 
                // updating of adaptations - mergers and / or retranslations and / or
                // placeholder insertions can alter the span breadth.
	int nAdaptationStep_ExtrasFromUserEdits; // the final number of extras (can be -ve)
                // due to the cumulative effect of the user's editing work in the
                // adaptationsStep (other steps need this value so as to be able to adjust
                // their spans when control enters them

    // next ones are for the support of the glosses update step; but when entry is at the
    // adaptations level (ie. user has changed an established adaptation), free
    // translations etc will need to be removed and a cancelSpan set up, so for that, the
    // cancelSpan supporting members above will be used.
	// The main things to note about the glosses update step are that: 
    // (1) CSourcePhrases outside the span must not be changed (this can be done by
    // limiting the selections possible),
    // (2) the start of the span is fixed (because no source text changes are done),
    // (3) the end of the span is also fixed - because this mode does not permit mergers
    // etc,
	// (4) extra members needed are equivents for those for adaptationsStep
	bool bGlossStepEntered; // default FALSE, TRUE if this step is set up on the screen, 
                // even if user then jumps to next step (TRUE is a flag which says that the
                // mode was set up, but data members here may or may not be expected to
                // have content, and therefore need to be looked at by processes such as
                // cancel or bailout)
	SPList glossStep_SrcPhraseList; // the list of CSourcePhrase instances in the editable 
                // span, as it is at the start of the glosses update step before the user
                // has had a chance to do anything; so it actually preserves the state of
                // whatever span was the last step
	int	nGlossStep_StartingSequNum; // the (fixed) sequence number value for the first 
				// CSourcePhrase in the span
	int	nGlossStep_EndingSequNum;   // the (fixed) sequence number value for the last 
				// CSourcePhrase in the span
	int nGlossStep_SpanCount; // how many instances are in the span when the step is 
				// first entered (stays constant)

	// next group are unique to the free translations update step
	bool bFreeTranslationStepEntered; // TRUE once control has been in this step once, 
				// even if briefly
	bool bVerseBasedSection; // default FALSE, TRUE if it looks like the section was 
				// created with the radio button "Verse" turned on
	SPList freeTranslationStep_SrcPhraseList; // we only need the initial list, because 
                // the user is unable to return to this step once backTranslationsStep has
                // been entered, and so and so the state of the span when
                // freeTranslationsStep is first entered is actually the final state of the
                // last step - that's what we are storing
	int			nFreeTranslationStep_StartingSequNum;
	int			nFreeTranslationStep_EndingSequNum;
	int 		nFreeTranslationStep_SpanCount;

    // span for collected back translations can be worked out from nBackTrans... starting
    // and ending indices, together with the difference between the old and new span counts
    // in the adaptations Step; backTranslationsStep is not reversible and once there the
    // process is automatically completed without user intervention and the vertical edit
    // process terminates
} EditRecord;

/// The AIModalDialog class is used as the base class for most of Adapt It's modal dialogs.
/// Its primary purpose is to turn off background idle processing while the dialog is being
/// displayed.
/// \derivation The AIModalDialog is derived from wxDialog.
class AIModalDialog : public wxDialog
{
public:
    AIModalDialog(wxWindow *parent, const wxWindowID id, const wxString& title,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            const long style = wxDEFAULT_DIALOG_STYLE);
	~AIModalDialog(); // destructor calls wxIdleEvent::SetMode(wxIDLE_PROCESS_ALL) 
					  // before calling wxDialog::~wxDialog()
	int ShowModal(); // calls wxIdleEvent::SetMode(wxIDLE_PROCESS_SPECIFIED) before 
					 // calling wxDialog::ShowModal()
};

class wxDynamicLibrary;

//////////////////////////////////////////////////////////////////////////////////
/// The CAdapt_ItApp class initializes Adapt It's application and gets it running. Most of
/// Adapt It's global enums, structs and variables are declared either as members of the
/// CAdapt_ItApp class or in this source file's global space.
/// \derivation		The CAdapt_ItApp class is derived from wxApp, and inherits its support
///                 for the document/view framework.
class CAdapt_ItApp : public wxApp
{
  public:
    CAdapt_ItApp();
	virtual ~CAdapt_ItApp(); // whm make all destructors virtual
    CMainFrame* GetMainFrame();

    /// The application's m_pDocManager member is one of the main players in the
    /// document-view framework as implemented in wxWidgets. It is created in OnInit() and
    /// mostly it takes care of itself, but we do use it explicitly to manage the file
    /// history and MRU lists by invoking its FileHistoryUseMenu(), FileHistoryLoad(),
    /// FileHistorySave(), GetFileHistory(), and RemoveFileFromHistory() methods. It is
    /// also referenced in the creation of our pDocTemplate and m_pMainFrame which
    /// participate in doc-view matters. We force our application to be of the "Single
    /// Document Interface (SDI)" type by calling the doc manager's SetMaxDocsOpen(1)
    /// method.
    wxDocManager* m_pDocManager; // for managing the doc/view // made doc manager public 
								 // so wxGetApp() can access it in the View
	
    /// The application's m_pConfig member is used to store and retrieve certain settings
    /// and/or information that needs to persist between application sessions. Since it is
    /// cross-platform it is clever enough to use the Registry on Windows, but uses hidden
    /// plain text configuration files on Linux and the Mac. We could use it to keep track
    /// of most of the basic config file settings, but for the sake of compatibility with
    /// the MFC version we only use it to store the following things: (1) the user
    /// interface language of choice, (2) the recent files used list (file history), and
    /// (3) [to be implemented] any non-default work folder location preferred for the
    /// local machine when the default "Adapt It Work" or "Adapt It Unicode Work" folder
    /// won't do.
	wxConfig* m_pConfig;

	/// The application's m_pConfigSIL member is used mainly to query whether the 
	/// "SOFTWARE\\SIL\\SilEncConverters22" entry exists in the (Windows) registry.
	wxConfig* m_pConfigSIL;

    /// The application's m_pParser member can be used to process command line arguments.
    /// The command line processing in the MFC version was implemented but did not work
    /// correctly. Although available for future use, it has not been implemented fully in
    /// the wxWidgets version.
	wxCmdLineParser* m_pParser;

    /// The application's array of 4 m_pConsistentChanger members are created on demand in
    /// OnToolsDefineCC(). They are used load and process consistent change tables in the
    /// application.
	CConsistentChanger* m_pConsistentChanger[4]; // all four initialized to NULL in Adapt_It.cpp

    /// The list of directory paths that should be searched for Adapt It files (XML
    /// configuration files, localization files, help files, etc.). On Linux this includes
    /// the directory where Adapt It was installed, plus the current user's .audacity-files
    /// directory. Additional directories can be specified using the AUDACITY_PATH
    /// environment variable. On Windows or Mac OS, this will include the directory which
    /// contains the Audacity program.
	wxArrayString adaptitPathList; // TODO: Implement this!

	// a string used for restoring the appropriate phrase box contents after filtering or
	// unfiltering changes have been made to the document. Enables the source text at a
	// possibly new active location in the document determined within the
	// GetSafePhraseBoxLocationUsingList() function to be communicated to a test in the
	// DoUsfmFilterChanges() function
	wxString m_strFiltering_SrcText_AtNewLocation;

private:
    
    /// The application's m_pMainFrame member serves as the backbone for Adapt It's
    /// interface and its document-view framework. It is created in the App's OnInit()
    /// function and is the "parent" window for almost all other parts of Adapt It's
    /// interface including the menuBar, toolBar, controlBar, composeBar, statusBar and
    /// Adapt It's scrolling main client window.
	CMainFrame* m_pMainFrame;
    DECLARE_EVENT_TABLE(); // MFC uses DECLARE_MESSAGE_MAP()

// MFC version code below
public:

    /// This holds the platform specific end-of-line character string for external text
    /// files. On Windows this is \r\n; on Linux it is \n; on Macintosh it is \r. The
    /// appropriate end-of-line character sequence(s) is stored in m_eolStr by a call to
    /// wxTextFile::GetEOL() in the App's OnInit() function.
	wxString m_eolStr;	

    /// The m_strNR string originally held the string "NR " for signifying what was called
    /// the "Non-Roman" version of Adapt It. For version 2.0.6 and onwards, m_strNR holds
    /// _T("Unicode ") because the unicode version is now called "Adapt It Unicode"; and
    /// the old _T("NR ") string is now in m_strOldNR, and this latter one was used within
    /// the function RenameNRtoUnicode( ) which searched for the old "Adapt It NR Work"
    /// folder and if it found it, it silently changed the name permanently to "Adapt It
    /// Unicode Work". This function is not needed or used in the wx version. The only use
    /// for m_strNR now is for using in the wxString::Format() method to add the word
    /// "Unicode " to compose the name of the Unicode version's work folder "Adapt It
    /// Unicode Work".
	wxString m_strNR;
	
    /// The title of the application which is written in the title bar of Adapt It's main
    /// frame, and also serves as the doc template name.
	wxString m_FrameAndDocProgramTitle; // title for main frame and doc template name

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Variable declarations below were moved here from the Doc's public area because the
    // wxWidgets doc/view framework deletes the Doc and recreates it afresh (calling the
    // Doc's constructor)
	
    /// The m_saveList member is a SPList which serves as a place to save m_pSourcePhrases
    /// when printing a selection or a ch/verse range. It is used in the View's
    /// SetupRangePrintOp() and in AIPrintout's OnPreparePrinting() and in its destructor.
	SPList m_saveList;

    /// A pointer to the m_saveList member which serves as a place to save m_pSourcePhrases
    /// when printing a selection or a ch/verse range. It is used in the View's
    /// SetupRangePrintOp() and in AIPrintout's OnPreparePrinting() and in its destructor.
	SPList* m_pSaveList;
	
    /// The m_curOutputFilename member holds the name of a file currently functioning as an
    /// output file. It generally only includes the file name and not the path (which is
    /// stored in m_curOutputPath).
	wxString m_curOutputFilename;

    /// The m_curOutputPath member holds the path of the file currently functioining as an
    /// output file. It generally only represents the path and not the file name itself
    /// (which is stored in m_curOutputFilename).
	wxString m_curOutputPath;

    /// The m_curOutputBackupFilename member holds the name of a file currently functioning
    /// as a backup file. The name of the backup file is of the form *.BAK.xml for the wx
    /// version which only uses the xml files and not binary files.
	wxString m_curOutputBackupFilename; // BEW added 15Aug05 to get a consistent file 
										// and path naming protocol
	
    /// The m_altOutputBackupFilename was used mostly in the MFC version which needed to
    /// juggle the presence of either binary or xml format files and backing them both up.
    /// The wx version only uses xml files, so the need for this variable could be
    /// eliminated
	wxString m_altOutputBackupFilename; // ditto

    /// The m_nInputFileLength member represented in the MFC version the serializable # of
    /// chars in untokenized source text (including terminating null). MFC used DWORD.
	wxUint32 m_nInputFileLength;
	
	// !!! Doxygen commenting up to here!!!
	wxString	m_punctuationSet; // retained in order to keep backwards compatibility in the
								  // doc serialization (with no content), otherwise unused

	// store doc size parameters
	wxSize		m_docSize;	// stores the virtual doc size (in pixels)
	wxSize		m_saveDocSize; // used for restoring original doc size across a printing
							   // operation
	int			m_nAIPrintout_Destructor_ReentrancyCount; // ~AIPrintout() is entered 
                // twice in closing off a Print Preview and parameter cleanup done there is
                // fouling things, so I'll count times reentered and have the function exit
                // immediately if the count gets over 1; the count is initialized to 1 in
                // the AIPrintout() constructor, and in a few other places
	SPList*		m_pSourcePhrases;
	wxString*	m_pBuffer;  // in legacy versions was used to store source (untokenized) 
                // text data on the heap; and still does so while parsing source text, but
                // from late versions 2 the source text was not kept there, not output in
                // the document, and from version 3 the buffer is also used for composing
                // settings information in the preparation of the XML form of the Adapt It
                // document
	wxString	buffer;
	wxString	m_curChapter;

	MapWholeMkrToFilterStatus m_FilterStatusMap;// entries are a wholeMarker (ie. with 
                // backslash) as key with either a "0" or "1" string as its associated
                // value
    // Note: CFilterPage maintains a temporary set of the following variables for
    // displaying the GUI state of unknown markers. When the user accepts any changes to
    // the filtering state of unknown markers, these variables on the app are updated.
	wxArrayString m_unknownMarkers; // array of unknown whole markers -			
									// no end markers stored here
	wxArrayInt	m_filterFlagsUnkMkrs;
	wxString	m_currentUnknownMarkersStr;	// whm added 31May05. Stores the current 
                // active list of any unknown whole markers in a = delimited string (that
                // is, = immediately follows each marker (then 0 or 1). This string is
                // stored in the doc's Buffer member (5th field).
	// next three added by BEW, 10June05 in support of Reconstituting the document after 
	// user changes the SFM set
	wxString	m_filterMarkersBeforeEdit;	// contents of gCurrentFilterMarkers is copied
                // to here when the user invokes the Preferences... command; this material
                // is used only if an SFM set change is requested
	wxString	m_secondPassFilterMarkers;	// the set of filter markers, with any common 
                // ones with what is in m_saveOriginalSfmSetFilterMarkers which are to
                // remain filtered, removed
	enum SfmSet	m_sfmSetBeforeEdit;	// the gCurrentSfmSet value is copied here when the 
                // user invokes the Preferences... command; this is used subsequently only
                // if an SFM set change is requested
	enum SfmSet m_sfmSetAfterEdit; // we use this after the unfiltering pass for the old 
                // SFM set when doing doc rebuild, in order to restore the gCurrentSfmSet
                // value to what is current
	wxString	m_filterMarkersAfterEdit;	//whm added 10Jun05 for Bruce
	bool		m_bWantSourcePhrasesOnly;	// Added by JF.  This is used to allow 
                // load/save operations to be performed _without_ affecting any globals or
                // the current view. This is used during the join operation to load
                // multiple files into memory (one at a time) without closing the current
                // file. This allows us to simply append the source phrases from each file
                // to the current document, rather than having to reconstruct or reload the
                // was-current document after reading the other documents for the join
                // operation. (If TRUE, document loading does nothing except set up the
                // list of CSourcePhrase instances.)

	/////////////////////////////////////////////////////////////////////////////////
    // Variable declarations moved here from the View because the wxWidgets doc/view
    // framework deletes the View and recreates it afresh (calling the View's constructor)
	
	bool m_bECConnected; // whm added for wx version
	bool bECDriverDLLLoaded; // set TRUE or FALSE in OnInit()
	CFindDlg* m_pFindDlg; // whm added to partly replace original 
						  // m_pFindReplaceDlg (non-modal)
	CReplaceDlg* m_pReplaceDlg; // whm added to partly replace original 
								// m_pFindReplaceDlg (non-modal)
	CViewFilteredMaterialDlg* m_pViewFilteredMaterialDlg; // non-modal
	CEarlierTranslationDlg* m_pEarlierTransDlg;

	extendSelDir m_curDirection; // current seln direction, either 'right' or 'left'
	wxPoint m_mouse;			 // last mouse position (log coords) during drag selection
								 // for use in tokenizing retranslations
	static bool bLookAheadMerge; // TRUE when merging a matched multiword phrase
	CPile* m_pActivePile;	// where the phrase box is to be located
	CCell* m_pAnchor;		// anchor element for the selection
	int m_selectionLine;	// index of line which has the selection (0 to 1, -1 
							// if none) Selections typically have line index 0, the 
							// circumstances where line with index 1 (target text) is
							// used as the selection are rare, perhaps not in WX version
	int m_nActiveSequNum;	// sequence number of the srcPhrase at the active 
							// pile location
	bool m_bSelectByArrowKey; // TRUE when user is using ALT + arrow key to extend sel'n
                //next two cannot be removed for refactored layout, because they are needed
                //for backwards compatibility of the config files; retain them, but make no
                //use of them except to put TRUE values in the config files - initialize
                //those in the app's OnInit()
	bool m_bSuppressLast;	// deprecated: (suppress last target text line of display when TRUE)
	bool m_bSuppressFirst;	// deprecated: (suppress first source text line of display when TRUE)
	bool m_bDrafting; // suggested by Bill Martin: a Drafting versus Review mode choice;
                // with the single-step versus automatic insertion choices possible only in
                // drafting mode, while in review mode (m_bDrafting == FALSE) hitting the
                // RETURN key takes user to the immediate next source phrase, whether empty
                // or not, and the Automatic checkbox is disabled until Drafting mode is
                // chosen again. (This review mode is needed so users can review and edit
                // their work, without the phrase box jumping miles ahead trying to find an
                // empty source phrase every time the user finishes editing something &
                // hits RETURN) This flag's functionality is the same for adapting and for
                // glossing.
	bool m_bSingleStep; // when FALSE, application looks ahead to try find a match for
                // source text in one of the 10 knowledge base maps and does not stop to
                // ask the user provided there is only a single match; when TRUE, every
                // match is shown and the app waits for a RETURN key press before
                // proceeding. When glossing is ON, lookahead will be constrained to the
                // next sourcephrase instance only, but otherwise the functionality is the
                // same as when adapting.
	bool m_bAcceptDefaults; // enabled only when its not single step, cons.changes are
                // on, and copy source is on. If TRUE, auto-inserting continues normally
                // until lookup fails to make a match, and then the source word is grabbed
                // and after the source word has been put through the cc table(s), then
                // m_bAutoInsert is turned back on automatically (no user intervention) so
                // that processing continues unhalted. Can halt by a click, but if not
                // done, then processing will continue to file end. Intended as a way to
                // get changes done which rely on cc tables alone, or primarily, without
                // user having to repeatedly hit RETURN key.
	bool m_bRestorePunct; // when TRUE, 4th line visible and user gets asked to manually
				// locate in the target text any medial punction encountered
	bool m_bComposeWndVisible; // TRUE whenever the compose window is potentially
				// visible (it must always be on top)
	bool m_bUseConsistentChanges; // turn on the use of consistent changes when copying 
                // source word or phrase as a default (initial) target word or phrase. For
                // either glossing or adapting.
	bool m_bUseSilConverter; // turn on the use of the configured SilConverter when 
                // copying source word or phrase as a default (initial) target word or
                // phrase. For either glossing or adapting. NOTE: this flag is mutually
                // exclusive with m_bUseConsistentChanges -- only one can be TRUE at a
                // time.
	bool m_bSaveToKB; // a temporary mode, settable for next potential StoreAdaption 
                // only, or StoreGloss only, thereafter reverts to TRUE until explicitly
                // reset; when FALSE, the active src phrase will have its m_bNotInKB flag
                // set TRUE if glossing is not on, but when glossing is on this flag will
                // not be changeable and will be TRUE
	bool m_bForceAsk; // a temporary mode, for next StoreAdaption or StoreGloss only, 
                // it forces the CTargetUnit's m_bAlwaysAsk flag to TRUE, even if there is
                // only one translation, or gloss, so far
	bool m_bCopySource; // when TRUE, if no match, copy the source key as first guess 
                // at the target adaption or gloss, when FALSE, an empty string is used
                // instead
	bool m_bMarkerWrapsStrip; // when TRUE, a st.format marker causes a wrap of the strip
	bool m_bRespectBoundaries; // TRUE by default, if FALSE, user can violate 
	bool m_bHidePunctuation;	// when TRUE, punctuation is not shown in lines 0 & 1, if
                // FALSE (the default value) then punctuation shows in lines 0 and 1
	bool m_bStartViaWizard; // used in OnIdle handler to force text in phrase box 
				// to be selected
	bool m_bUserTypedSomething; // FALSE when phrase box first created, TRUE when 
                // something typed provided it is not an ENTER key, or other control key
	bool m_bUseToolTips;	// shows tooltips when TRUE, none shown when false; default
				// is TRUE
	int m_nTooltipDelay; // amount in milliseconds of time tooltips display before 
                // disappearing if m_bUseToolTips is TRUE the default time is 20000 (20
                // seconds)
	bool m_bExecutingOnXO;  // TRUE if command-line switch -xo is used, FALSE otherwise

	// The following weren't initialized in the view's constructor but moved here from the
	// View for safety.
	int	m_nMaxToDisplay; // max # of words/phrases to display at one time (this has to
                // be retrained in our refactoring for backwards compatibility with
                // configuration files - formerly this was bundle's count of sourcephrases,
                // but we are now saving the m_pSourcePhrases list's m_count value
	// next two retained for backwards compatibility of config files, but we make no use
	// of them - just output values 30 & 40 every time, respectively, to basic config file
	int	m_nPrecedingContext; // minimum # of words/phrases in preceding context
	int	m_nFollowingContext; // ditto, for following context
	int	m_curLeading;	 // the between-strips leading value
	int	m_curLMargin;	 // if user wants a left margin, he can set this
	int	m_curGapWidth;	 // inter-pile gap, measured in pixels (follows a pile)

	// from TEXTMETRICs, heights of source & target text lines & the editbox
	int	m_nSrcHeight;	// line height for source language text
	int	m_nTgtHeight;	// ditto for target lines & the pApp->m_targetBox
	int	m_nNavTextHeight; // for use by CCell when working out where to print the 
                // nav text & the value is set in the App's UpdateTextHeights() function
	int	m_nCurPileMinWidth; // used when laying out, CreatePile uses this value which is
				// set by GetPileWidth() beforehand
	int	m_nSaveActiveSequNum; // a location to save the active sequ number across 
				// an operation
	// window size and position saving & restoring
	wxPoint		m_ptViewTopLeft; // client area
	wxSize		m_szView;

	// for selection (other parameters are also involved besides this one)
	CCellList	m_selection; // list of selected CCell instances

	// BEW added 10Feb09 for refactored view layout support
	CLayout* m_pLayout;	

	// values for members of printing support structures
	wxPageSetupDialogData* pPgSetupDlgData; // for page setup
	// Note: When the page setup dialog has been closed, the app can query the 
	// wxPageSetupDialogData object associated with the dialog. 
	
	wxPrintData* pPrintData; // for print settings
	
	POList	m_pagesList; // MFC uses CPtrList
	int		m_pageWidth;
	int		m_pageLength;
	int		m_marginTop;
	int		m_marginBottom;
	int		m_marginLeft;
	int		m_marginRight;
	bool	m_bIsInches;
	bool	m_bIsPortraitOrientation;
	int		m_pageWidthMM;		// m_pageWidth converted to milimeters
	int		m_pageLengthMM;		// m_pageLength converted to milimeters
	int		m_marginTopMM;		// m_marginTop converted to milimeters
	int		m_marginBottomMM;	// m_marginBottom converted to milimeters
	int		m_marginLeftMM;		// m_marginLeft converted to milimeters
	int		m_marginRightMM;	// m_marginRight converted to milimeters
	int		m_paperSizeCode;	// keep as MFC paper size code enum (internally 
								// we convert to wxPaperSize)

    // although a more exact conversion is 2.54, because of rounding in the dialogs, the
    // values below work better than their more precise values
	float	config_only_thousandthsInchToMillimetres;
	float	config_only_millimetresToThousandthsInch;
	float	thousandthsInchToMillimetres;
	float	millimetresToThousandthsInch;

	// for note support
	CNoteDlg*	m_pNoteDlg; // non-modal

	// attributes in support of the travelling edit box (CPhraseBox)
    // wxWidgets design Note: whm I've changed the MFC m_targetBox to m_pTargetBox and
    // create it only in the View's OnCreate() method. Instead of destroying it and
    // recreating the targetBox repeatedly as the MFC version did, it now lives undestroyed
    // for the life of the View, and will simply be shown, hidden, moved, and/or resized
    // where necessary.
	CPhraseBox*		m_pTargetBox;
	wxString		m_targetPhrase; // the text currently in the m_targetBox
	long			m_nStartChar;   // start of selection in the target box
	long			m_nEndChar;		// end of selection in the target box
	
	bool bUserSelectedFileNew;

	// Single instance checker added as a new feature to wxWidgets version
	wxSingleInstanceChecker* m_pChecker; // used for preventing more than one 
				// program instance by the same user. See OnInit()
	wxString PathSeparator;
	
    // MFC Note: for support of splitter windows - clicks in green wedge of the non-first
    // pane crash because gpApp->GetView() as coded for the legacy app returns the first
    // view found in the doc template, but the one being clicked in won't be that one, and
    // so subsequent code failed. To get around this, I've overrided OnActivateView() and
    // used the new active view to set this m_pCurView member on the app; and then
    // gpApp->GetView() will scan the list of views and return the one that matches what
    // m_pCurView points at
	// whm WX version not initially supporting splitter window (& probably never will, BEW)
	
	// BEW added 06Mar06, to store topleft & bottomright coords of the desktop window
	// (used for error checking on window location & size when reading basic config file)
	wxPoint wndTopLeft;
	wxPoint wndBotRight;

	// BEW added for Bob Eaton 26Apr06; for transliteration mode support
	bool		m_bTransliterationMode;
	
	// for support of user-defined font size for vernacular text in dialogs (version 2.4.0)
	int m_dialogFontSize; // default 12 point size

	// for adding a time delay  for inserting when in Automatic mode
	int			m_nCurDelay;

	// for autosaving
	TIMESETTINGS m_timeSettings;
	bool		 m_bIsDocTimeButton;
	int			 m_nMoves;

	// cc related globals
	bool			m_bCCTableLoaded[4];
	bool			m_bTablesLoaded;
	// paths to cc tables & their names
	wxString		m_tableName[4];
	wxString		m_tableFolderPath[4];
	wxString		m_defaultTablePath;

	// default path to export folder
	wxString		m_lastExportPath;
	// last document path
	wxString		m_lastDocPath; // for config files
	// the next four were added for version 2.0.1, so that each type of export
	// could have its own remembered folder location; and to allow m_lastExportPath
	// to be used ONLY for export of the target text
	wxString		m_lastSrcExportPath; // for export of the source text
	wxString		m_retransReportPath; // for retranslation reports
	wxString		m_rtfExportPath; // for all RTF documents exported
	wxString		m_kbExportPath; // for export of a KB, and place to look for 
									// a KB to import
	bool			m_bSaveAsXML;	// BEW added 04Aug05 for support of saving doc 
									// and KB with XML markup
	// font stuff
	wxFont*		m_pSourceFont;
	wxFont*		m_pTargetFont;
	wxFont*		m_pNavTextFont;

    // wxFontData added for wxWidgets version holds font info with get/set methods for font
    // information such as Get/SetColour(), Get/SetChosenFont() etc. A wxFontData object is
    // the last parameter in a wxFontDialog constructor in order to interoperate with the
    // dialog, receiving the user's font choice data when the dialog is dismissed. We use
    // is mainly for GetChosenFont() and for Get/SetColour().
	wxFontData* m_pSrcFontData;
	wxFontData* m_pTgtFontData;
	wxFontData* m_pNavFontData;
	wxFont*		m_pDlgSrcFont;
	wxFont*		m_pDlgTgtFont;
	wxFont*		m_pDlgGlossFont;
	wxFont*		m_pComposeFont;
	wxFont*		m_pRemovalsFont; // BEW added 11July08, so setting of the font and size can 
                // be done for the removals bar, & the dynamic changing of the other dlg
                // font pointers won't then clobber this one (as would be the case if I
                // used one of them for the removals bar)
	wxFont*		m_pVertEditFont; // BEW added 23July08, so setting of the font and size can
                // be done for the vertical edit message and controls bar, & the dynamic
                // changing of the other dlg font pointers won't then clobber this one (as
                // would be the case if I used one of them for the vertical edit controls
                // bar)
    // whm: In our wxWidgets version Windows API's LOGFONT is not necessary; we're using
    // our own fontInfo struct

	wxColour	m_sourceColor;
	wxColour	m_targetColor;
	wxColour	m_navTextColor;
	wxColour	m_specialTextColor;
	wxColour	m_reTranslnTextColor;
	wxColour	m_AutoInsertionsHighlightColor;
	wxColour	m_freeTransDefaultBackgroundColor; // it will be light pastel green 
				// (set in app constructor)
	wxColour	m_freeTransCurrentSectionBackgroundColor; // it will be light 
				// pastel pink
	wxColour	m_freeTransTextColor; // temporarily set to dark purple (0x640064)
				// -- in app constructor
	wxColour sysColorBtnFace; // color used for read-only text controls displaying
				// static text info button face color
	wxColour sysColorWindow;  // color used for read-only text controls displaying 
				// static text info window background color

	int			m_backgroundMode;	// whm added 6July2006 Set to wxSOLID in App's
                // OnInit because wxDC's SetTextBackground uses by default background mode
                // of wxTRANSPARENT which doesn't show up on screen regardless of what
                // color we set with SetTextBackground(). Each call to SetTextBackground
                // should have a prior call to SetBackgroundMode(m_backgroundMode), in
                // which the m_backgroundMode = wxSOLID.

	// source encoding as in input data, and system encoding as defined by system codepage,
	// and the target encoding (determined by tellenc.cpp 3rd party encoding detector)
	wxFontEncoding	m_srcEncoding;
	wxFontEncoding	m_tgtEncoding;

	// whm added following encoding variables to the wx version
	wxFontEncoding m_navtextFontEncoding;
	wxFontEncoding m_dlgSrcFontEncoding;
	wxFontEncoding m_dlgTgtFontEncoding;
	wxFontEncoding m_dlgGlossFontEncoding;
	wxFontEncoding m_composeFontEncoding;
	wxFontEncoding m_removalsFontEncoding;
	wxFontEncoding m_vertEditFontEncoding;
	wxFontEncoding	m_systemEncoding;	// In OnInit: m_systemEncoding = 
				// wxLocale::GetSystemEncoding();

	bool m_bConfigFileHasFontEncodingInfo;

	// whm added below for cross-platform locale and encoding considerations.
	wxString	m_systemEncodingName; // In OnInit: m_systemEncodingName = 
				// wxLocale::GetSystemEncodingName();
	int			m_systemLanguage;	  // In OnInit: m_systemLanguage = 
				// wxLocale::GetSystemLanguage();
	const wxLanguageInfo* m_languageInfo; // In OnInit: m_languageInfo = 
				// wxLocale::GetLanguageInfo(m_systemLanguage);
	
	CurrLocalizationInfo currLocalizationInfo;

	wxLocale* m_pLocale; // pointer for creating our wxLocale object (using 
                // the non-default constructor which does not require use of
                // wxLocale::Init()) to which we will add localization catalogs, associate
                // catalog lookup paths, and to which we can add wxLanguage values.

	// project-defining attibutes
	wxString	m_sourceName; // name of the source language
	wxString	m_targetName; // name of the target language
	
	bool		m_bExistingAdaption;
	bool		m_bKBReady;
	CKB*		m_pKB; // pointer to the knowledge base
	bool		m_bGlossingKBReady;
	CKB*		m_pGlossingKB; // pointer to the glossing knowledge base

	// autosaving flag
	bool		m_bNoAutoSave;

	// flags and variables for restoring earlier location on launch
	bool		m_bEarlierProjectChosen;
	bool		m_bEarlierDocChosen;
	int			nLastActiveSequNum; // for config files

	// whm For following see block above of other variables I've moved from the View
    // BEW 22Sep05: moved here from the view class, because setting it in the view's
    // creator is a problem when a new view is created - as happens when the user splits
    // the window, and that would otherwise cause OnIdle() to spuriously open the Start
    // Working wizard
	bool		m_bJustLaunched;

#ifdef _RTL_FLAGS
	// flags for RTLReading languages
	bool		m_bSrcRTL; // true when source language reads right to left
	bool		m_bTgtRTL; // ditto, for target language
	bool		m_bNavTextRTL; // ditto, for navigation text
#endif

	bool		m_bZoomed; // is main frame zoomed or not

	// source & target punctuation strings, for adaptation use
	wxString	m_punctuation[2];		// can be unicode
	wxString	m_punctWordBuilding[2]; // can be unicode

	// punctuations correspondences, for adaptation use
	PUNCTPAIR		m_punctPairs[MAXPUNCTPAIRS];
	TWOPUNCTPAIR	m_twopunctPairs[MAXTWOPUNCTPAIRS];

    // for supporting ordinary single-quote as punctuation, or not as punctuation, so that
    // IsOpeningQuote() and IsClosingQuote() can include, or not include, it in the
    // testing; and also for ordinary double-quote (we'll assume user will not ever make
    // curly quotes word-building characters - but if that had to be supported then we'd
    // need four more booleans for those and they'd have to be set/reset in
    // OnEditPunctCorresp() and used in IsOpeningQuote(), IsClosingQuote() in the same way
    // as those below are used there)
	bool		m_bSingleQuoteAsPunct; // default FALSE set in creator's code block
	bool		m_bDoubleQuoteAsPunct; // default TRUE set in creator's code block

	// file i/o and directory structures
	wxString	m_workFolderPath;	// "C:\My Documents\Adapt It Work" or the 
				// Win 2000 equiv path

    // whm added 5Jun09 for alternate "forced" work folder path (forced by use of -wf
    // <path> command-line option)
	wxString	m_wf_forced_workFolderPath; // any path following a -wf 
				// command-line option
	wxString	m_newdoc_forced_newDocPath; // any path following a -newdoc 
				// command-line option
	wxString	m_exports_forced_exportsPath; // any path following a -exports 
				// command-line option

	wxString	m_theWorkFolder;	// "Adapt It Work"
	wxString	m_localPathPrefix;	// the part of the workfolder path before the 
				// m_theWorkFolder part
	wxString	m_adaptionsFolder;	// "Adaptations" folder
	wxString	m_curProjectName;	// <Project Name> in the form 
				// "<SourceLanguageName> to <TargetLanguageName> Adaptations"
	wxString	m_curProjectPath;	// "C:\My Documents\Adapt It Work\<Project Name>"
	wxString	m_curAdaptionsPath;	// "C:\My Documents\Adapt It Work\<Project Name>\Adaptations"
	wxString	m_lastSourceFileFolder;	// path to the folder from which last 
                // source text file was taken; used for config file read and write

	wxString	m_setupFolder;			// whm renamed m_setupFolder to this 
				// and moved to App class 31July06
	wxString	m_executingAppPathName;	// whm added to get the path and file 
				// name of executing app
	
    /// m_appInstallPathOnly stores the path (only the path, not path and name) where the
    /// executable application file is installed on the given platform.
    /// On wxMSW: "C:\Program Files\Adapt It WX\ or C:\Program Files\Adapt It WX Unicode\" 
    /// On wxGTK: "/usr/bin/"
    /// On wxMac: "/Programs/"
	wxString m_appInstallPathOnly;		
	
    /// m_appInstallPathName stores the path and name where the executable application file
    /// is installed on the given platform.
	/// On wxMSW:   "C:\Program Files\Adapt It WX\Adapt_It.exe or 
	///             C:\Program Files\Adapt It WX Unicode\Adapt_It_Unicode.exe"
	/// On wxGTK:   "/usr/bin/adaptit"      [adaptit is the name of the executable, 
	///             not a directory]
	/// On wxMac: "/Programs/AdaptIt.app"
	wxString m_appInstallPathAndName;		
	
    /// m_xmlInstallPath stores the path where the AI_USFM.xml and books.xml files are
    /// installed on the given platform.
	/// On wxMSW:   "C:\Program Files\Adapt It WX\ or 
	///             C:\Program Files\Adapt It WX Unicode\" 
	/// On wxGTK:   "/usr/share/adaptit/"  [adaptit here is the name of a directory]
	/// On wxMac:   "AdaptIt.app/Contents/Resources"  [bundle subdirectory] ??? 
	///             TODO: check this location
	wxString m_xmlInstallPath; // whm added for path where the AI_USFM.xml and 
				// books.xml files are installed
	
    /// m_localizationInstallPath stores the path where the <lang> localization files are
    /// installed on the given platform.
	/// On wxMSW:   "C:\Program Files\Adapt It WX\Languages\ or 
	///             C:\Program Files\Adapt It WX Unicode\Languages\"
	/// On wxGTK:   "/usr/share/locale/"    which then contains multiple 
	///             <lang>/LC_MESSAGES/adaptit.mo
	/// On wxMac:   "AdaptIt.app/Contents/Resources/locale" [bundle subdirectory]
	///             this is where Poedit puts its localization files.
	wxString m_localizationInstallPath;	// whm added for path where top level <lang> 
				// localization directory is installed
	
	/// m_helpInstallPath stores the path where the help files are installed on the given 
	/// platform.
	/// On wxMSW:   "C:\Program Files\Adapt It WX\ or 
	///             C:\Program Files\Adapt It WX Unicode\"
	/// On wxGTK:   "/usr/share/adaptit/help/"  containing: common/.gif and .css
	///             containing: <lang>/  .html .hhp .hhc etc
	/// On wxMac:   "AdaptIt.app/Contents/SharedSupport" [bundle subdirectory] ??? 
	///             TODO: check this location
	wxString m_helpInstallPath;

	/// m_htbHelpFileName stores the actual name of the help file for the given platform.
	/// This name is of the form <appName>.htb and is stored at the m_helpInstallPath.
	/// On wxMSW: Adapt_It.htb
	/// On wxGTK: adaptit.htb
	/// On wxMac: AdaptIt.htb
	wxString m_htbHelpFileName;
	
    /// m_licenseInstallPath stores the path where the license files are installed on the
    /// given platform.
	/// On wxMSW:   "C:\Program Files\Adapt It WX\ or C:\Program Files\Adapt It WX Unicode\"
	/// On wxGTK:   "/usr/share/doc/adaptit/license/"
	/// On wxMac:   "~/Documents"
	wxString m_licenseInstallPath; // whm added for path where license files are installed
	
    /// m_documentsInstallPath stores the path where the Adapt It documents are installed
    /// on the given platform.
	/// On wxMSW:   "C:\Program Files\Adapt It WX\ or C:\Program Files\Adapt It WX Unicode\"
	/// On wxGTK:   "/usr/share/doc/adaptit/docs/"
	/// On wxMac:   "~/Documents"
	wxString m_documentsInstallPath; // whm added for path where documents, reference docs,
				// etc are installed
	
	/// m_desktopInstallPath stores the path where the Adapt It desktop menu configuration
	/// (adaptit.desktop) file is installed on the given platform.
	/// On wxMSW:   n/a
	/// On wxGTK:   "/usr/share/applications/"
	/// On wxMac:   ???
	wxString m_desktopInstallPath; // whm added for path where adaptit.desktop file is 
				// installed

	wxString	m_curKBName;
	wxString	m_curKBPath;
	wxString	m_curKBBackupPath;
	wxString	m_curGlossingKBName;	// all projects use same Glossing.KB name
	wxString	m_curGlossingKBBackupPath;
	wxString	m_curGlossingKBPath;	// same path as m_curKBPath except for 
				// filename difference

    // BEW added 13Aug05, alternative names for the KB files and paths -- required because
    // the KBs might be binary (extensions .KB or .BAK) or XML (extensions .xml or
    // BAK.xml), and the project config file's saved value for the m_bSaveAsXML flag might
    // not be in agreement with the content of the KB files on disk (eg. the user could
    // change the flag value in the interface but then reject saving document and kb with
    // the new value); so we need to preserve the alternative filenames and paths, so that
    // if the one is not present when expected, we can check for the other and load it
    // instead.
    // whm comment: Since wxWidgets version does not read MFC binary file (CArchive) forms,
	// the following may not need to be used. 
    // BEW 25Jul09, they are still needed because they enable the WX versions to read
    // binary doc and kb files from the legacy MFC version even though we only save in XML,
    // any such files have their filename extension converted to .xml in the WX versions,
    // so keep these as they are used in these processes
	wxString	m_altKBName;
	wxString	m_altKBPath;
	wxString	m_altKBBackupPath;
	wxString	m_altGlossingKBName;
	wxString	m_altGlossingKBPath;
	wxString	m_altGlossingKBBackupPath;

	wxArrayString m_acceptedFilesList;	// for use in OnFileRestoreKB function and 
                // elsewhere, such as consistency check, retranslation report, transforming
                // adaptations to glosses
	bool		m_bBackupDocument;
	bool		m_bAutoBackupKB;

	// wizard window suppression
	bool		m_bSuppressWelcome;
	bool		m_bUseStartupWizardOnLaunch; // this always remains TRUE since version 3
				// & although deprecated, we retain it for backwards compatibility in reading
				// configuration files
	bool		m_bSuppressTargetHighlighting;

	// auto insertion
	bool		m_bAutoInsert;  // when TRUE, OnIdle handler tries repetitive matching and
                // inserting provided the view's m_bSingleStep flag is FALSE; when FALSE,
                // no repetitive matching is tried.

    // enable or suppress RTL reading checkboxes in the fontpage, for the Wizard, and
    // preferences (for ANSI version, we want them unseen; for unicode NR version, we want
    // them seen)
	bool		m_bShowRTL_GUI;
    // boolean for the LTR or RTL reading order choice in the layout (see CreateStrip() in
    // the view class)
	bool		m_bRTL_Layout;

    // auto-capitalization support, for version 2.0 and later (also a menagerie of global
    // flags defined in Adapt_ItApp.cpp)
	wxString	m_srcLowerCaseChars; // paired with m_srcUpperCaseChars, equivalences at 
				// same index
	wxString	m_tgtLowerCaseChars; // paired with m_tgtUpperCaseChars, equivalences at 
				// same index
	wxString	m_glossLowerCaseChars; // paired with m_glossUpperCaseChars, equivalences 
				// at same index
	wxString	m_srcUpperCaseChars;
	wxString	m_tgtUpperCaseChars;
	wxString	m_glossUpperCaseChars;

	// support for Bible book folders
	bool		m_bBookMode; // default is FALSE, to get legacy app functionalities only
	wxArrayPtrVoid*	m_pBibleBooks; // MFC uses CPtrArray*
	BookNamePair*	m_pCurrBookNamePair; // currently set ptr to book name pair's struct
	int			m_nBookIndex; // 0-based index into an array of structs for the 
				// (localizable) book folders
	wxString	m_bibleBooksFolderPath; // absolute path to the currently active Bible 
				// book folder
	int			m_nDivSize[5];		// size of each division for the 5 radio buttons
	wxString	m_strDivLabel[5];	// radio button labels for history, wisdom, etc
	int			m_nDefaultBookIndex; // settable from an element in the  books.xml file
	int			m_nLastBookIndex;	// store last one, it is a better default when 
				// turning mode back on
	bool		m_bDisableBookMode; // TRUE if the books.xml file did not parse without 
				// error
	int			m_nTotalBooks;		// total count of all the book folder names in the 
				// books.xml file

	// whm added 19Jan05 AI_USFM.xml file processing and USFM Filtering
	bool		m_bUsingDefaultUsfmStyles;

	// flag for skipping USFM fixed space "!$" 2-character sequence when parsing
	// source text (if TRUE, then skip, if FALSE then don't check for its presence)
	// used in the ParseWord() function in the document class
	bool		m_bChangeFixedSpaceToRegularSpace;

	// BEW added 20 Apr 05 in support of toggling suppression/enabling of copying of
	// source text punctuation on a CSourcePhrase instance at the active location down
	// to the word or phrase in the phrase box
	bool		m_bCopySourcePunctuation;

	// RDE: added 3Apr06 in support of calling SilEncConverters for preprocessing 
	// the target word form (c.f. Consistent Changes)
    // whm: the following two need to be always available in order to interact with MFC
    // produced legacy config files which contain
	wxString	m_strSilEncConverterName;
	bool        m_bSilConverterDirForward;
	int			m_eSilConverterNormalizeOutput;

	enum SfmSet gCurrentSfmSet;	// whm 6May05. Restricted gCurrentSfmSet to indicate 
                //only the current active SfmSet. It is initialized to UsfmOnly, but is
                //assigned whatever value is stored in the project config file. If there is
                //no previous project config file, it is assumed that the user has not used
                //Adapt It previously on this computer, and UsfmOnly will remain the
                //default sfm set used to create any documents. If it is the first use of
                //version 3, but a config file exists indicating the user is upgrading from
                //a previous Adapt It version, and the config file has no UseSFMarkerSet
                //recorded in the config file, the gCurrentSfmSet value and the value for
                //gProjectSfmSetForConfig are now both be set to PngOnly, because all Adapt
                //It documents created prior to version 3 will have been created with a
                //PngOnly inventory of standard format markers. See note below for
                //gProjectSfmSetForConfig.
	enum SfmSet gProjectSfmSetForConfig; // whm added 6May05. At program startup 
                // gProjectSfmSetForConfig is set equal to gCurrentSfmSet. If the project
                // config file is successfully read, gProjectSfmSetForConfig and
                // gCurrentSfmSet are both set to the value last stored in the config file.
                // If, however, the user loads a document that was created with a different
                // set, gCurrentSfmSet will be set equal to the SfmSet that was active when
                // the document was last saved (as stored in the doc's Buffer member). In
                // this case, gProjectSfmSetForConfig will still retain the original config
                // SfmSet value and in any case is now always what is used to store the
                // user's explicitly chosen SfmSet in the project config file. The only
                // time gProjectSfmSetForConfig changes is when the user explicitly changes
                // the current working SfmSet to something else by means of Edit
                // Preferences USFM/Filter tab (forcing a rebuild) or by means of the Start
                // Working... wizard USFM/Filter page (no doc open, no rebuild).
	enum SfmSet gFactorySfmSet;	// will retain the factory default sfm set which is assigned
				// to be UsfmOnly
	MapSfmToUSFMAnalysisStruct*	m_pUsfmStylesMap;  // stores associations of key and ptr to
				// USFMAnalysis instances where the key is a USFM marker without the backslash
	MapSfmToUSFMAnalysisStruct* m_pPngStylesMap;   // "" for PNG styles
	MapSfmToUSFMAnalysisStruct* m_pUsfmAndPngStylesMap;   // "" for combined USFM and PNG 
				// styles (USFM has priority)
	wxArrayPtrVoid*		m_pMappedObjectPointers;// array of all USFMAnalysis struct pointers 
                // on the heap used for ease of deleting them in the destructor
	wxString	UsfmWrapMarkersStr;	// will hold a list of usfm wrap markers
	wxString	PngWrapMarkersStr;	// will hold a list of png wrap markers
	wxString	UsfmAndPngWrapMarkersStr;	// will hold a list of usfm and png wrap markers
	wxString	UsfmSectionHeadMarkersStr;	// will hold a list of usfm sectionHead wrap markers
	wxString	PngSectionHeadMarkersStr;	// will hold a list of png sectionHead wrap markers
	wxString	UsfmAndPngSectionHeadMarkersStr; // will hold a list of usfm and png 
				// sectionHead wrap markers
	wxString	UsfmInLineMarkersStr;	// will hold a list of usfm inLine markers
	wxString	PngInLineMarkersStr;	// will hold a list of png inLine markers
	wxString	UsfmAndPngInLineMarkersStr;	// will hold a list of usfm and png inLine markers
	wxString	UsfmFilterMarkersStr;	// will hold a list of usfm filter markers
	wxString	PngFilterMarkersStr;	// will hold a list of png filter markers
	wxString	UsfmAndPngFilterMarkersStr;	// will hold a list of usfm and png filter markers

	wxString	gFactoryFilterMarkersStr;	// will retain the factory default filter 
                // markers which is defined to be the contents of the UsfmFilterMarkersStr
                // as it was initially built when it was read from the AI_USFM.xml file.

    // The following two are used to keep track of Filter Markers in a similar way that the
    // gCurrentSfmSet and gProjectSfmSetForConfig above are used to keep track of user's
    // sfm set.
	wxString	gCurrentFilterMarkers; // whm added 13May05. Stores the current active list 
                // of whole Filter Markers. This variable is assigned the contents of
                // UsfmFilterMarkersStr, PngFilterMarkersStr, or
                // UsfmAndPngFilterMarkersStr, depending on the currently selected sfm set.
                // If the user saved a document that was created with a modified filter
                // marker list (via changes to USFM/Filtering tab), the string value stored
                // in it will be overridden upon serialization (loading) of the doc by the
                // filter marker list stored in the doc's Buffer member, regardless of the
                // filter marker string stored in the project config file (see the next
                // variable gProjectFilterMarkersForConfig below).
	wxString	gProjectFilterMarkersForConfig; // whm added 13May05. At program startup 
                // gProjectFilterMarkersForConfig is set equal to gCurrentFilterMarkers. If
                // the project config file is successfully read,
                // gProjectFilterMarkersForConfig and gCurrentFilterMarkers are both set to
                // the value last stored in the config file. If, however, the user loads a
                // document that was created with a different filter marker list,
                // gCurrentFilterMarkers will be set equal to the filter marker list that
                // was active when the document was last saved (as stored in the doc's
                // Buffer member). In this case, gProjectFilterMarkersForConfig will still
                // retain the original config filter makers list and in any case is now
                // always what is used to store the user's explicitly chosen list of filter
                // markers in the project config file. The only time
                // gProjectFilterMarkersForConfig changes is when the user explicitly
                // changes the current working set of filter markers to something else by
                // means of Edit Preferences Filter tab or by means of the Start Working...
                // wizard Filter page.

	int m_nSequNumBeingViewed;	// The sequ num of the src phrase whose m_markers is 
				// being viewed in the ViewFilteredMaterial dialog
	
	wxSize	sizeLongestSfm;	// Used to determine the text extent of the largest sfm being 
                // used in the program (using main window - system font) 
    wxSize	sizeSpace; // Used to determine the text extent of a space (using main window -
                // system font)

	// BEW added 21Jun05 for support of free translation mode
	bool	m_bFreeTranslationMode; // TRUE when free translation mode is in effect, 
				// FALSE otherwise
	bool	m_bTargetIsDefaultFreeTrans; // TRUE if the user wants target text to be 
                // composed into the default free translation text for a section and shown
                // in the edit box within the Compose Bar when that particular free
                // translation section is defined, FALSE if not, and default setting is
                // FALSE
	bool	m_bGlossIsDefaultFreeTrans; // TRUE if the user wants glosses text to be 
                // composed into the default free translation text for a section and shown
                // in the edit box within the Compose Bar when that particular free
                // translation section is defined, FALSE if not, and default setting is
                // FALSE
	bool	m_bComposeBarWasAskedForFromViewMenu; // TRUE if the user used the Compose Bar 
                // command on the View menu to open the Compose Bar (which then inhibits
                // being able to turn on free translation mode), FALSE when the Compose Bar
                // gets closed by any command, and FALSE is the default value. The OnUpdate
                // handler for the Free Translation Mode command inspects this bool value
                // to ensure the bar is not already open
	bool	m_bDefineFreeTransByPunctuation; // TRUE by default (gives smaller free 
                // translation sections), if FALSE then sections are defined as the whole
                // verse, or if the phrase box starts off more than 5 locations before the
                // verse end, then up to the verse's end
	bool	m_bNotesExist; // set TRUE by OnIdle() if there are one or more Adapt It notes
				// in the document, else FALSE

	// BEW added 10Jan06
	bool	m_bUnpacking;	// TRUE when Unpack Document... is in progress, else FALSE 
				// (used in SetupDirectories())

	AIPrintout* pAIPrintout;

// Overrides
	public:
    bool	OnInit();// wxApp uses non-virtual OnInit() instead of virtual bool InitInstance()
    int		OnExit();// wxApp uses non-virtual OnExit() instead of virtual int ExitInstance()
    // OnIdle() handler moved to CMainFrame. Having it here in the App was causing File |
    // Exit and x App cancel to become unresponsive 
    // void OnIdle(wxIdleEvent& event);

// Declaration of event handlers

	void OnFileNew(wxCommandEvent& event);

	void OnFileRestoreKb(wxCommandEvent& WXUNUSED(event));
	void OnFileBackupKb(wxCommandEvent& WXUNUSED(event));
	void OnToolsDefineCC(wxCommandEvent& WXUNUSED(event));
	void OnToolsUnloadCcTables(wxCommandEvent& WXUNUSED(event));
	
	void OnFileChangeFolder(wxCommandEvent& event);
	void OnAdvancedBookMode(wxCommandEvent& event);
	void OnAdvancedChangeWorkFolderLocation(wxCommandEvent& event);
	void OnUpdateAdvancedChangeWorkFolderLocation(wxUpdateUIEvent& WXUNUSED(event));
	
	void OnFilePageSetup(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedBookMode(wxUpdateUIEvent& event);
	void OnUpdateFileChangeFolder(wxUpdateUIEvent& event);
	void OnUpdateFileBackupKb(wxUpdateUIEvent& event);
	void OnUpdateFileRestoreKb(wxUpdateUIEvent& event);
	void OnUpdateFileStartupWizard(wxUpdateUIEvent& event);
	void OnUpdateFilePageSetup(wxUpdateUIEvent& event);
	void OnUpdateUnloadCcTables(wxUpdateUIEvent& event);
	void OnUpdateLoadCcTables(wxUpdateUIEvent& event);

	void OnAdvancedTransformAdaptationsIntoGlosses(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedTransformAdaptationsIntoGlosses(wxUpdateUIEvent& event);
	void OnToolsAutoCapitalization(wxCommandEvent& WXUNUSED(event));
	void OnUpdateToolsAutoCapitalization(wxUpdateUIEvent& event);

protected:

	void	AddWedgePunctPair(wxChar wedge);
	void	DoKBRestore(CKB* pKB, int& nCount, int& nTotal, int& nCumulativeTotal);
	bool	DoTransformationsToGlosses(CAdapt_ItDoc* pDoc, wxString& folderPath, 
						wxString& bookFolderName, bool bSuppressStatistics = FALSE);
	void	GetForceAskList(CKB* pKB, KPlusCList* pKeys);
	void	GetValue(const wxString strReadIn, wxString& strValue, wxString& name);
	wxSize	GetExtentOfLongestSfm(wxDC* pDC);
	bool	IsAdaptitProjectDirectory(wxString title);
	void	MakeForeignBasicConfigFileSafe(wxString& configFName,wxString& folderPath);
	CBString MakeKBElementXML(wxString& src,CTargetUnit* pTU,int nTabLevel);
	void	RestoreForceAskSettings(CKB* pKB, KPlusCList* pKeys);
	void	PunctPairsToString(PUNCTPAIR pp[MAXPUNCTPAIRS], wxString& rStr);
	void	SetDefaults();
	void	StringToPunctPairs(PUNCTPAIR pp[MAXPUNCTPAIRS], wxString& rStr);
	void	StringToTwoPunctPairs(TWOPUNCTPAIR pp[MAXTWOPUNCTPAIRS], wxString& rStr);
	void	TwoPunctPairsToString(TWOPUNCTPAIR pp[MAXTWOPUNCTPAIRS], wxString& rStr);


#ifdef _UNICODE
	void	ChangeNRtoUnicode(wxString& s);
	void	PunctPairsToTwoStrings(PUNCTPAIR pp[MAXPUNCTPAIRS], wxString& rSrcStr, wxString& rTgtStr);
	void	TwoPunctPairsToTwoStrings(TWOPUNCTPAIR pp[MAXTWOPUNCTPAIRS], wxString& rSrcStr, wxString& rTgtStr);
	void	TwoStringsToPunctPairs(PUNCTPAIR pp[MAXPUNCTPAIRS], wxString& rSrcStr, wxString& rTgtStr);
	void	TwoStringsToTwoPunctPairs(TWOPUNCTPAIR pp[MAXTWOPUNCTPAIRS], wxString& rStr, wxString& rTgtStr);
#endif

public:

	CurrLocalizationInfo ProcessUILanguageInfoFromConfig();
	bool	LocalizationFilesExist(); 
	// Functions that let the user select/change Adapt It's interface language
	bool	ChooseInterfaceLanguage(enum SetInterfaceLanguage setInterface);
	bool	ReverseOkCancelButtonsForMac(wxDialog* dialog);

	// printing support
	void	DoPrintCleanup();
	bool	CalcPrintableArea_LogicalUnits(int& nPagePrintingWidthLU, int& nPagePrintingLengthLU);

	// miscellaneous (this group in alphabetical order mostly)
	void	ChangeUILanguage();
	bool	FitWithScrolling(wxDialog* dialog, wxScrolledWindow* scrolledWindow, wxSize maxSize);
	wxString GetDefaultPathForLocalizationSubDirectories();
	wxString GetDefaultPathForXMLControlFiles();
	wxString GetDefaultPathForHelpFiles();
	int		GetFirstAvailableLanguageCodeOtherThan(const int codeToAvoid, wxArrayString keyArray);
	void	GetListOfSubDirectories(const wxString initialPath, wxArrayString &arrayStr);
	wxLanguage GetLanguageFromFullLangName(const wxString fullLangName);
	wxLanguage GetLanguageFromDirStr(const wxString dirStr, wxString &fullLangName);
	int		GetMaxIndex(); // BEW added 21Mar09
	int		GetOptimumDlgEditBoxHeight(int pointSize);
	int		GetOptimumDlgFontSize(int pointSize);
	bool	PathHas_mo_LocalizationFile(wxString dirPath, wxString subFolderName);
	void	SaveCurrentUILanguageInfoToConfig();
	void	SaveUserDefinedLanguageInfoStringToConfig(int &wxLangCode, 
				const wxString shortName, const wxString fullName, 
				const wxString localizationPath);
	void	RemoveUserDefinedLanguageInfoStringFromConfig(const wxString shortName, 
				const wxString fullName);
	void	UpdateFontInfoStruct(wxFont* font, fontInfo& fInfo);

	// the following are mostly in alphabetical order, but there are exceptions
	bool	AccessOtherAdaptionProject();
	bool	AreBookFoldersCreated(wxString dirPath);
	void	ClearKB(CAdapt_ItDoc* pDoc);
	bool	ContainsOrdinaryQuote(wxString s, wxChar ch);
	void	CreateBookFolders(wxString dirPath, wxArrayPtrVoid* pFolders);
	void	DoAutoSaveDoc();
	void	DoAutoSaveKB();
	bool	DoUsfmSetChanges(CUSFMPageCommon* pUsfmPageCommon, 
				CFilterPageCommon* pFilterPageCommon, bool& bSetChanged, 
				enum Reparse reparseDoc); // whm added 23May05; BEW added bSetChanged 12Jun05
	bool	DoPunctuationChanges(CPunctCorrespPageCommon* punctPgCommon, enum Reparse reparseDoc);
	void	DoKBBackup();
	void	DoKBSaveAsXML(wxFile& f, bool bIsGlossingKB = FALSE) ;
	void	DoGlossingKBBackup();
	void	DoFileOpen(); // DoFileOpen() calls OnOpenDocument() which is in the Doc
	bool	DoStartWorkingWizard(wxCommandEvent& WXUNUSED(event));
	bool	DoUsfmFilterChanges(CFilterPageCommon* pfilterPageCommon, 
				enum Reparse reparseDoc); // whm revised 23May05
	bool	EnumerateDocFiles(CAdapt_ItDoc* WXUNUSED(pDoc), wxString folderPath, 
				bool bSuppressDialog = FALSE);
	int		FindArrayString(const wxString& findStr, wxArrayString* strArray);
	int		FindListBoxItem(wxListBox* pListBox, wxString searchStr, 
				enum SearchCaseType searchType, enum SearchStrLengthType searchStrLenType);
	int		FindListBoxItem(wxListBox* pListBox, wxString searchStr, 
				enum SearchCaseType searchType, enum SearchStrLengthType searchStrLenType, 
				enum StartFromType startFromType);
	void	FormatMarkerAndDescriptionsStringArray(wxClientDC* pDC, 
				wxArrayString* MkrAndDescrArray, int minNumSpaces, 
				wxArrayInt* pUserCanSetFilterFlags);
	bool	GetBasePointers(CAdapt_ItDoc*& pDoc, CAdapt_ItView*& pView, CPhraseBox*& pBox);
	MapSfmToUSFMAnalysisStruct* GetCurSfmMap(enum SfmSet sfmSet);
	CAdapt_ItDoc* GetDocument();	// convenience function for accessing the Doc
	int		GetPageOrientation();
	void	GetPossibleAdaptionDocuments(wxArrayString* pList, wxString dirPath);
	void	GetPossibleAdaptionProjects(wxArrayString* pList);
	void	GetPunctuationSets(wxString& srcPunctuation, wxString& tgtPunctuation);
	int		GetSafePhraseBoxLocationUsingList(CAdapt_ItView* pView);
	CAdapt_ItView* GetView();		// convenience function for accessing the View
	void	InitializeFonts(); // whm added
	void	InitializePunctuation(); // whm added
	bool	InitializeLanguageLocale(wxString shortLangName, wxString longLangName, 
				wxString pathPrefix);
	bool	IsDirectoryWithin(wxString& dir, wxArrayPtrVoid*& pBooks);
	
    // the following ones were added by BEW to complete JF's implementation of Split, Join
    // and Move functionalities
	bool		IsValidBookID(wxString& id);
	wxString	GetBookIDFromDoc();
	wxString	GetBookID(); // this one handles both book mode on or off
	void		AddBookIDToDoc(SPList* pSrcPhrasesList, wxString id);

	bool	LayoutAndPaginate(int& nPagePrintingWidthLU, int& nPagePrintingLengthLU);
	bool	LoadKB();
	bool	LoadGlossingKB();
	wxFontEncoding MapMFCCharsetToWXFontEncoding(const int Charset);
	int		MapWXFontEncodingToMFCCharset(const wxFontEncoding fontEnc);
	int		MapWXtoMFCPaperSizeCode(wxPaperSize id);
	wxPaperSize MapMFCtoWXPaperSizeCode(int id);
	void	RefreshStatusBarInfo();
	void	RemoveMarkerFromString(wxString& filterMkrStr, wxString wholeMarker);
	wxString MakeExtensionlessName(wxString anyName); // removes .xml if at end of anyName
	void	SetFontAndDirectionalityForDialogControl(wxFont* pFont, wxTextCtrl* pEdit1, 
				wxTextCtrl* pEdit2, wxListBox* pListBox1, wxListBox* pListBox2, 
				wxFont*& pDlgFont, bool bIsRTL = FALSE);

    // rde: version 3.4.1 and up, determining the correct 'encoding=' string to put in an
    // XML file takes some extra thought...
	void	GetEncodingStringForXmlFiles(CBString& aStr);
	wxUint32 m_nCodePage;

	bool	SaveKB(bool bAutoBackup);
	bool	SaveGlossingKB(bool bAutoBackup);
	void	SetDefaultCaseEquivalences();
	void	SetPageOrientation(bool bIsPortrait);
	bool	SetupDirectories();
	void	SetupKBPathsEtc();
	void	SetupMarkerStrings();
	bool	StoreGlossingKB(bool bAutoBackup);
	bool	StoreKB(bool bAutoBackup);
	void	SubstituteKBBackup(bool bDoOnGlossingKB = FALSE);
	void	Terminate();
	void	UpdateTextHeights(CAdapt_ItView* WXUNUSED(pView));
	void	WriteFontConfiguration(const fontInfo fi, wxTextFile* pf);
	void	WriteBasicSettingsConfiguration(wxTextFile* pf);
	void	WriteProjectSettingsConfiguration(wxTextFile* pf);
	bool	WriteConfigurationFile(wxString configFilename, wxString destinationFolder, 
				int selector);

	bool	GetBasicConfigFileSettings(); // whm 20Jan08 changed signature to return bool
    // Note: The three methods below had _UNICODE and non Unicode equivalents in the MFC
    // version wxTextFile is Unicode enabled, so we should only need a single version of
    // these in the wxWidgets code.
	void	GetBasicSettingsConfiguration(wxTextFile* pf);
	bool	GetFontConfiguration(fontInfo& pfi, wxTextFile* pf, bool& bFaceNameFound, 
				wxString& faceName);
	void	GetProjectSettingsConfiguration(wxTextFile* pf);
#ifdef _UNICODE
	void	ConvertAndWrite(wxFontEncoding WXUNUSED(eEncoding),wxFile* pFile,
				wxString& str); // for unicode conversions
	void	Convert8to16(CBString& bstr,wxString& convStr); // converts a CBString to a 
				// wide char CString
	wxString	Convert8to16(CBString& bstr); // BEW added 09Aug05 - overloaded version
				// (it's more useful this way)
	CBString	Convert16to8(const wxString& str); // BEW added 10Aug05, for XML output 
				// in Unicode version
	void	DoInputConversion(wxString& pBuf,const char* pbyteBuff,
				wxFontEncoding eEncoding,bool bHasBOM = FALSE); // for unicode conversions
#endif
	bool	GetConfigurationFile(wxString configFilename, wxString sourceFolder, int selector);
	void	GetSrcAndTgtLanguageNamesFromProjectName(wxString& project, wxString& srcName, 
				wxString& tgtName);
	
	// Getters, setters, and shorthands.  Added by JF.
    // Ultimately, these are best inlined for execution speed. (BEW - but none of these are
    // involved in speed-critical functionalities, so I'd prefer not to inline them.)
	wxString	GetCurrentBookDisplayName();
	wxString	GetCurrentBookFolderPath();
	wxString	GetCurrentBookFolderName();
	wxString	GetAdaptationsFolderDisplayName();
	wxString	GetAdaptationsFolderPath();
	wxString	GetAdaptationsFolderName();
	wxString	GetCurrentDocFileName();
	wxString	GetCurrentDocPath();
	wxString	GetCurrentDocFolderDisplayName();
	wxString	GetCurrentDocFolderPath();
	void		ChangeDocUnderlyingFileNameInPlace(wxString NewFileName);
	void		ChangeDocUnderlyingFileDetailsInPlace(wxString NewFolderPath, 
					wxString NewFileName);
	void		ChangeDocUnderlyingFileDetailsInPlace(wxString NewFileName);
	bool		IsOpenDoc(wxString FolderPath, wxString FileName);
	bool		GetDocHasUnsavedChanges();
	wxString	GetBookIndicatorStringForCurrentBookFolder();
	bool		GetBookMode();
	void		SaveDocChanges();
	void		DiscardDocChanges();
	void		CloseDocDiscardingAnyUnsavedChanges();
	static SPList *LoadSourcePhraseListFromFile(wxString FilePath);
	bool		AppendSourcePhrasesToCurrentDoc(SPList *ol, wxString& curBookID, 
					bool IsLastAppendUsingThisMethodRightNow);
	SPList*		GetSourcePhraseList();
	CSourcePhrase	*GetCurrentSourcePhrase();
	void		SetCurrentSourcePhrase(CSourcePhrase *sp);
	CSourcePhrase	*FindNextChapter(SPList *ol, CSourcePhrase *sp);
	static wxString	ApplyDefaultDocFileExtension(wxString s);
	void	DeleteSourcePhraseListContents(SPList *l); // deletes partner piles too, 
            // one-by-one (this is inefficient, but doing an en masse deletion of the
            // partner piles would require having an overloaded
            // CLayout::DestroyPiles(PileList* list) function which we don't yet have, and
            // we'd need extra code in the SplitDialog.cpp file in order to calculate the
            // sublist list which corresponds to the one passed in to
            // DeleteSourcePhraseListContents(); so I've been lazy and sacrificed some
            // speed to avoid this, and just let the DeleteSingleSrcPhrase() call delete
            // the partner pile each time through the internal loop
	void	CascadeSourcePhraseListChange(bool DoFullScreenUpdate);// If 
            // DoFullScreenUpdate, the Phrase Box is moved. Otherwise, we update internal
            // variables but don't redraw. This is useful if you are going to make a series
            // of changes and want the state to be valid between each change but you don't
            // want the screen to flash with changes as the changes are made. For example,
            // when splitting a document into multiple files, one per chapter, you may wish
            // to call this function multiple times with DoFullScreenUpdate = false, and
            // finally call it with DoFullScreenUpdate = true only after the last change.
	CSourcePhrase *GetSourcePhraseByIndex(int Index);
	void	SetCurrentSourcePhraseByIndex(int Index);

	// whm added 5Jan04 FindAppPath() from suggestion by Julian Smart in wxWidgets 
	// docs re "Writing installers for wxWidgets applications"
	wxString FindAppPath(const wxString& argv0, const wxString& cwd, 
				const wxString& appVariableName); 

private:
	void	EnsureWorkFolderPresent();
public:
	// a couple of members to be used for (hopefully) limiting the CPlaceInternalPunct
	// dialog, at the once location, from being shown twice
	int		m_nPlacePunctDlgCallNumber;
	int		m_nCurSequNum_ForPlacementDialog;
};

DECLARE_APP(CAdapt_ItApp);
// This is used in headers to create a forward declaration of the wxGetApp function It
// creates the declaration className& wxGetApp(void). It is implemented by IMPLEMENT_APP.

#endif /* Adapt_It_h */

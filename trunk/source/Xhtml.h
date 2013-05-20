/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Xhtml.h
/// \author			Bruce Waters
/// \date_created	9 June 2012
/// \rcs_id $Id$
/// \copyright		2012 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	
/// This is the header file for the Xhtml export class. The Xhtml class takes an
/// in-memory plain text (U)SFM marked up text, removes unwanted USFM markers (e.g inline
/// markers like for italics, and material such as book introduction markers and their
/// conntent...) and then builds xhtml output. (Deuterocannon books are not supported.) 
///
/// \derivation		The Xhtml class is derived from wxEvtHandler (it uses some functions
///                 from an unfinished Oxes.h & .cpp module which has been abandoned)
/////////////////////////////////////////////////////////////////////////////

#ifndef Xhtml_h
#define Xhtml_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Xhtml.h"
#endif

#include <wx/dynarray.h>

// forward declarations
class wxObject;
class CAdapt_ItApp;
class CBString;


// next one maps const wxChar* (C-string) USFM keys to XhtmlTagEnum values -- actually
// to int values, and once retrieved we cast them to enum values at the assignment step
// something like this: XhtmlTagEnum aValue = XhtmlTagEnum(<the returned int value>);
WX_DECLARE_STRING_HASH_MAP( int, Usfm2EnumMap );

// we need a hash map of int keys (for enum values cast to int) mapped to wxString values
// - the strings will be the xhtml label strings for the class attribute in <div> and
// <span> tags
WX_DECLARE_HASH_MAP(int, wxString, wxIntegerHash, wxIntegerEqual, Enum2LabelMap);

// we may need a "stack", our CStack would do except that we need to also use the same
// keys for the base and secondary indent maps' keys as well, and I've not found a
// wxWidgets hash map that works properly with variable length char* C-string arrays. So
// I'll use a wxArrayString and make it into a poor man's stack - unfortunately, it can't
// be subclassed, so I'll define Xhtml class methods, Push() and Pop() to push to the array
// end and pop from there. I'll call it m_tagsStack, even though it's really an array.
// BEW 9Jun112 -- I doubt we'll need the stack...

// ========= start of 2012 enums, structs, arrays =====================

enum SpanTypeEnum {
	simple,
	plusClass,
	nested, // for footnote, endnote, cross reference (the bit after the style <span> -- see below)
	chapterNumSpan,
	verseNumSpan,
	footnoteFirstPart,
	targetREF,
	captionREF,
	captionREFempty,
	emptySpan
};
// If more are added to the above, be sure to increase the array size in
//   *****     CBString m_spanTemplate[9];  ******


enum XhtmlTagEnum {
	no_value_,           // use this for 0, since 0 is returned by a failure to match a key in m_pUsfm2IntMap
	title_main_,		 // for \mt or \\mt1
	title_secondary_,    // for \mt2
	title_tertiary_,     // for \mt3

	// ***** NOTE *****
	// The Sena 3 data uses Section_Head as the mapping from \s or \s1; but Greg Trihus's
	// export of a Matthew file uses Section_Head_Minor for ALL the \s (or \s1) in the
	// markup. So I think I'll leave it like the Sena - that way I can distinguish \ms
	// from \s from \s2 or \s3; I can only get a 2-way distinction with Greg's mapping. It
	// shouldn't really matter, as the publisher can set the style anyway - as long as it
	// is used consistenly.
	major_section_,	     // for \ms or \ms1, handled in xhtml without embedding & as inline at same level as \s
	section_,		     // normal section marker, \s or \s1; srcSection & then Section_Head with class= the type
	minor_section_,      // for \s2, for something like "\s2 The Japhethites"; if \s3 TE has nothing so do as \s2

	parallel_passage_ref_, // for \r parallel passage references which aren't auto-generated
	REF_,				 // for the \fr in footnote, or \xo in cross reference
	c_,                  // for chapter \c
	chapter_head_,       // for USFM ?? \cl chapter label (when number isn't used) eg. "Psalm"
	v_,                  // for verse \v
	v_num_in_note_,		 // for \fv in footnote or endnote
	alt_quote_,           // for \fqa in a footnote or endnote (see USFM footnotes) 'footnote quote alternate'
	footnote_,           // for \f ... \f*
	endnote_,            // for \fe ... \fe*
	footnote_text_,		 // USFM \ft  (no style for this, it just gets ignored; same for \xt)
	footnote_quote_,	 // for \fq  (TE wrongly uses 'Alternate_Reading' so I'll use that and for \fqa too) see alt_quote_
	footnote_referenced_text_, // for \fk ... \fk*
	crossref_referenced_text_, // for \xk ... \xk*
	inline_crossReference_, // got USFM inline right-aligned \rq ... \rq* cross references (these have no internal structure)
	crossReference_,     // for \x ... \x*
	crossReference_target_reference_, // for \xt (we just ignore \xt)
	line1_,              // for \q or \q1, poetry line, level 1 
	line2_,              // for \q2 , poetry line, level 2
	line3_,              // for \q3 , poetry line, level 3
	running_hdr_,        // not in xhtml, but probably needed as a defacto book title for scrBookName tag
	id_,                 // for \id, we only get the 3-letter book code, ignore the rest
	p_,                  // for \p paragraph marker
	m_,                  // for \m paragraph continuation marker
	intro_paragraph_,    // for \ip
	intro_section_,      // for \is or \is1 introduction section level 1 (\is2 does not appear to be supported)
	intro_list_item_1_,  // for \ili or \ili1 introduction list item, level 1
	intro_list_item_2_,  // for \ili2 introduction list item, level 2
	stanza_break_,       // for \b  stanza break in poetry
	inscription_paragraph_, // for \pc (USFM paragraph centred)
	inscription_,        // for \sc ... \sc*  inscription  (USFM inline markers)
	emphasis_,           // for \it ... \it*  emphasis (USFM inline markers) (\em \em* is Emphasized_Text)
	emphasized_text_,	 // for \em ... \em* (USFM inline markers)
	list_item_1_,        // for \li or \li1, list item, level 1
	list_item_2_,        // for \li2, list item, level 2
	quoted_text_,		 // for \qt ... \qt*, quoted text (USFM inline markers)
	words_of_christ_,    // for \wj ... \wj*, words of Jesus (USFM inline markers)
	see_glossary_,		 // for \w ... \w*  (USFM inline markers)
	citation_line_1_,    // probably for \qm or \qm1, level 1 (see Oxes 'embedded' tag)
	citation_line_2_,    // probably for \qm2, level 2 (see Oxes 'embedded' tag)
	citation_line_3_,    // probably for \qm3, level 3 (see Oxes 'embedded' tag)
	//no_break_paragraph_, // for \nb, nobreak paragraph marker -- unsupported in TE & xhtml as yet
	remark_,             // for \rem

    // the following are mostly not associated with a particular USFM, but may come in handy for
    // mapping enum values to xhtml "Label" strings to be used as values in the
    // class="Label" attribute - in either <span> or <div> tags; currently there are no wx
	columns_,
	scrBody_,
	scrBook_,
	scrBookCode_,
	scrBookName_,
	scrFootnoteMarker_,  // I wonder if there is a scrEndnoteMarker for endnotes? I guess I'll use scrFootnoteMarker until advised otherwise
	scrIntroSection_,
	scrSection_,
	border_, // no USFM equivalent for this, it's in TE only, so I'll not support it
	crossHYPHENReference, // I'm guessing about the HYPHEN part, because - won't compile in a label name
	figure_, // for \fig DESC|FILE|SIZE|LOC|COPY|CAP|REF\fig* (I've no support for LOC (ref range) or COPY (copyright))
	picture_,
	pictureRight_,
	pictureLeft_,
	pictureCenter_,
	pictureCaption_,
	pictureColumn_,
	picturePage_,

	// add more here....
	nix  // a nothing entry to end off legally
};

// marker constant C-string keys for the m_pUsfm2IntMap hash map (mapping to XhtmlTagEnum
// value cast to int; 56 markers below to be supported, to be equivalent to Text Editor;
// the == operator works with these if one is a wxString and the other one of the constant
// C-strings below
const wxChar unknownMkr[] = _T("\\xxx"); // for anything unmatched when parsing
const wxChar mtMkr[] = _T("\\mt");
const wxChar mt1Mkr[] = _T("\\mt1");
const wxChar mt2Mkr[] = _T("\\mt2");
const wxChar mt3Mkr[] = _T("\\mt3");
const wxChar mrMkr[] = _T("\\mr");   // "major section range: \c 1 \ms BOOK ONE \mr (Psalms 1-41) \s True happiness
const wxChar dMkr[] = _T("\\d");     // "descriptive title (or, Hebrew title): e.g. \d ("For the director of music.")
const wxChar msMkr[] = _T("\\ms");   // "major section: \c 1 \ms BOOK ONE \mr (Psalms 1-41) \s True happiness
const wxChar ms1Mkr[] = _T("\\ms1"); // "major section: \c 1 \ms BOOK ONE \mr (Psalms 1-41) \s True happiness
const wxChar sMkr[] = _T("\\s");     // normal section
const wxChar s1Mkr[] = _T("\\s1");   // normal section
const wxChar s2Mkr[] = _T("\\s2");   // correlates with section_minor_ enum value
const wxChar s3Mkr[] = _T("\\s3");   // ??? nothing in TE, so just conflate with section_minor_ value
const wxChar rMkr[] = _T("\\r");     // parallelPassage reference mkr
//const wxChar s4Mkr[] = _T("\\s4"); //unlikely to ever occur
const wxChar vMkr[] = _T("\\v");     // \v support, (all \vn's will have been converted to \v and all \vt's removed)
const wxChar cMkr[] = _T("\\c");
const wxChar itMkr[] = _T("\\it");	 // for \it ... \it*  (italicized) ie. "Emphasis" style
const wxChar fMkr[] = _T("\\f");     // for footnote
const wxChar ftMkr[] = _T("\\ft");	 // footnote text \ft, also used in endnotes
const wxChar fkMkr[] = _T("\\fk");	 // footnote referenced text \fk, also used in endnotes
const wxChar fqMkr[] = _T("\\fq");   // for "TE alternate reading " USFM \fq 'footnote quote' in a footnote or endnote
const wxChar fqaMkr[] = _T("\\fqa"); // for "alternate reading " USFM \fqa 'footnote quote alternate' in a footnote or endnote
const wxChar fvMkr[] = _T("\\fv");   // for USFM \fv  a verse number within the text of the footnote or endnote
const wxChar frMkr[] = _T("\\fr");   // for \fr in a footnote (ch:vs ref is followed by a space, every time)
const wxChar feMkr[] = _T("\\fe");   // for endnote \fe ... \fe*,
const wxChar rqMkr[] = _T("\\rq");     // for crossReference \rq ... \rq* inline x reference, usually right-aligned in the text
const wxChar xMkr[] = _T("\\x");     // for crossReference \x ... \x* TE does not appear to support these
const wxChar xoMkr[] = _T("\\xo");   // for crossReference, origin reference \xo (handled same as \fr)
const wxChar xtMkr[] = _T("\\xk");   // for crossReference, keyword  \xk marker
const wxChar xkMkr[] = _T("\\xt");   // for crossReference, the list of refs \xt
const wxChar qMkr[] = _T("\\q");     // for poetry, level 1
const wxChar q1Mkr[] = _T("\\q1");   // for poetry, level 1
const wxChar q2Mkr[] = _T("\\q2");   // for poetry, level 2
const wxChar q3Mkr[] = _T("\\q3");   // for poetry, level 3
const wxChar hMkr[] = _T("\\h");     // for running header
const wxChar idMkr[] = _T("\\id");   // for \id
const wxChar nbMkr[] = _T("\\nb");	 // for \nb "no break paragraph"
const wxChar pMkr[] = _T("\\p");	 // for \p "paragraph"
const wxChar mMkr[] = _T("\\m");	 // for \m "paragraph continuation"
const wxChar ipMkr[] = _T("\\ip");	 // for \ip "intro paragraph"
const wxChar liMkr[] = _T("\\li");	 // for \li "list item, level 1"
const wxChar li1Mkr[] = _T("\\li1"); // for \li1 "list item, level 1"
const wxChar li2Mkr[] = _T("\\li2"); // for \li2 "list item, level 2"
const wxChar qtMkr[] = _T("\\qt");	 // for \qt & \qt* "quoted text"
const wxChar wjMkr[] = _T("\\wj");	 // for \wj  "words of christ"
const wxChar wjEndMkr[] = _T("\\wj*"); // need to be able to test for \wj* "words of christ"
const wxChar wMkr[] = _T("\\w");	 // for \w & \w* "word in glossary" xhtml 'See_In_Glossary'
const wxChar clMkr[] = _T("\\cl");	 // for \cl "chapter label"
const wxChar qmMkr[] = _T("\\qm");	 // for \qm "citation line, level 1"
const wxChar qm1Mkr[] = _T("\\qm1"); // for \qml "citation line, level 1"
const wxChar qm2Mkr[] = _T("\\qm2"); // for \qm2 "citation line, level 2"
const wxChar qm3Mkr[] = _T("\\qm3"); // for \qm3 "citation line, level 3"
const wxChar emMkr[] = _T("\\em");   // for \em  \em* "Emphasized_Text"
const wxChar scMkr[] = _T("\\sc");   // for \sc  \sc* "inscription"
const wxChar pcMkr[] = _T("\\pc");   // for \pc "inscription paragraph / USFM paragraph centred"
const wxChar iliMkr[] = _T("\\ili"); // for \ili  "introduction list item, level 1"
const wxChar ili1Mkr[] = _T("\\ili1"); // for \ili1  "introduction list item, level 1"
const wxChar ili2Mkr[] = _T("\\ili2"); // for \ili2  "introduction list item, level 2"
const wxChar bMkr[] = _T("\\b");     // for \b "stanza break" only usable within poetry
const wxChar isMkr[] = _T("\\is");   // for \is "introduction section, level 1"
const wxChar is1Mkr[] = _T("\\is1"); // for \is1 "introduction section, level 1"
const wxChar is2Mkr[] = _T("\\is2"); // for \is2 "introduction section, level 2"
const wxChar remMkr[] = _T("\\rem"); // for \rem "Paratext note"
const wxChar figMkr[] = _T("\\fig"); // for \fig .... \fig* "picture in the text"

class Xhtml : public wxEvtHandler
{

// -----------------CREATORS, DESTRUCTOR, MAIN FUNCTIONS-----------

public:

	Xhtml(); // default constructor
	Xhtml(CAdapt_ItApp* app); // use this one

	virtual ~Xhtml();// destructor

	// DoXhtmlExport() is the public function called to get the job done;
	// Note, we build it as a C-string, and it will be valid UTF-8
	CBString DoXhtmlExport(wxString& buff);
	void SetBookID(wxString& aValidBookCode);

    // not for proper xhtml output, -- an indenter for indenting the Sena or similar xhtml
    // file, using the <div> and </div> tags - to make it more human readable; it gives a
    // file dlg, I select the file, it loads it and does the indents, and then I save it
    // somewhere again using the file dialog
#if defined (_DEBUG)
	void Indent_Etc_XHTML();
	// also not for proper xhtml output, for use by IndentXHTML()
	CBString BuildIndent(CBString& atab, int level);
#endif
private:

// -----------------PRIVATE FUNCTIONS---------------------------

	// setup functions & shutdown functions & initializing functions...
	
	void Initialize();
	bool IsWhiteSpace(wxChar& ch);

// ---------------------------------------------------------------

	// USFM parsing functions...
	
	// a parser which parses over inline (ie. formatting) USFM markers until next halting
	// SF marker is encountered. The function handles PNG marker set's \F or \fe footnote
	// end markers, if present, and the sfm set in operation is the PNG 1998 one.
	// BEw 13Jun12, made a simpler version for xhtml parsing, which halts at every marker
	int ParseMarker_Content_Endmarker(wxString& buffer, wxString& mkr, 
					wxString& endMkr, wxString& dataStr, wxString& nextMkr);
	// The next is for parsing over \f ... \fe*, or \x ... \x*, or \fe ... \fe*, and
	// nextMkr will return whatever marker follows the \f* or \fe* or \x* (unless of
	// course text follows - in which case it will return an empty string). (If I can be
	// bothered, I should also handle PNG marker set's \f ... \F (or \f .... \fe))
	// mkr will return \f or \x or \fe as the case may be; endMkr will return whatever is
	// the endmarker, and dataStr will be everything between these (after leading and
	// trailing whitespace has been trimmed off). Internal processing of the data fields
	// within dataStr is the responsibility of the caller.
	int ParseFXRefFe(wxString& buffer, wxString& mkr, wxString& endMkr, 
					wxString& dataStr, wxString& nextMkr);

// ----------------------DATA STORAGE----------------------------

	CAdapt_ItApp*	m_pApp;	// The app owns this
	wxString m_bookID; // store the 3-letter book code (eg. LUK, GEN, 1CO, etc) here
	int m_version; // initially = 1, later can be 1 or 2
	int m_nPictureNum; // begin at 1 and increase by 1 for each successive picture
	CBString m_strPictureID; // construct the unique id string and store here, for BuildPictureProductions()

	wxString* m_pBuffer; // ptr to the (ex-class) buffer containing the (U)SFM text

	bool m_bContainsFreeTrans;
	bool m_bContainsNotes;
	wxString m_freeMkr; // "\\free"
	wxString m_noteMkr; // "\\note"

public:
	// these public variables are filled by the DoExportAsXhtml() function in ExportFunctions.cpp
	wxString m_languageCode; // the iso639-1 or if not in that standard, then the iso639-3 (3-letter) code
	wxString m_myFilePath; // the path to _TARGET_OUTPUTS where it will be saved
	wxString m_myFilename; // extension removed, and if from collaboration mode, the "Collab_" prefix also removed
	wxString m_directionality; // will be either _T("ltr") or _T("rtl")

private:

	Usfm2EnumMap*    m_pUsfm2IntMap;     // for hooking up Usfm parsing to case blocks in switch
	Enum2LabelMap*   m_pEnum2LabelMap;   // for hooking up the enums to the associated xhtml production, parameterized
	XhtmlTagEnum	m_whichTagEnum; // stores the enum value for the case block to do
									// processing for a given USFM marker's xml productions
	int             m_spanLen;	    // length (in wxChar) of the span of text parsed over by a call
								    // of ParseMarker_Content_Endmarker(), to next mkr or chunk end

	//---------------- tabbing support ------
	CBString m_aTab;        // we'll use 4 single-byte spaces for each tab (set in Initialize())
	int		 m_tabLevel;
	//------------ end tabbing support --------

	CBString m_exporterID;  // set it to the login name of the user account which created the Xhtml export eg.watersb
							// & set it at start of each DoXhtmlExport() call using GetExporterID()
	CBString m_myBookName;  // the bookname, in collab mode usually taken from Paratext book name,
							// otherwise it can be whatever the user specifies in the DoBookName() dialog
	//CBString m_runningHdr;  // extract from the exported USFM text -- it's marker is \h (empty string if undefined)
	CBString m_eolStr;      // platform specific end-of-line string, set from gpApp->m_eolStr,
							// converted to CBString, in Initialize()
	CBString m_chapterNumSpan; // a filled out 
			// <span class=\"Chapter_Number\" lang=\"langAttrCode\">chNumPCDATA</span>
			// is stored here when ParseMarker_Content_Endmarker() encounters a \c marker
	CBString m_verseNumSpan; // a filled out 
			// <span class=\"Verse_Number\" lang=\"langAttrCode\">vNumPCDATA</span>
			// is stored here when ParseMarker_Content_Endmarker() encounters a \v marker
	wxString m_emptyStr;
	wxString m_callerStr; // for the caller character following \f, \fe, or \x markers 
						  // (it's either +, -, or user-defined; my templates use + and the
						  // + gets changed to whatever is in the data, at export time)
	wxString m_beginMkr;
	wxString m_endMkr;
	wxString m_data;    // for returning the content in a USFM marker, 
	                    // found by ParseMarker_Content_Endmarker()
	wxString m_nextMkr; // stores a copy of whatever marker follows when 
						// ParseMarker_Content_Endmarker() returns
	wxString footnoteMkr;
	wxString endnoteMkr;
	wxString xrefMkr;
	wxString inline_xrefMkr; // for \rq (\rq ... \rq* is what TE supports, not \x ... \x*)
	wxArrayString m_endnoteDumpArray; // store the Endnote_General_Paragraph 
									  // productions here, for dump near file end
	wxArrayString m_notePartsArray; // store subproductions for parts of footnotes or endnotes
	bool	 m_bWordsOfChrist;  // TRUE whenever \wj has been processed, but it's 
							    // matching \wj* hasn't been processed yet
	wxString wordsOfChristEndMkr;
	CBString m_convertedData; // for storing m_data after converting to 
							  // UTF-8 and replacing entities
#if defined(_DEBUG)
	//****************** to help with debugging, we don't otherwise use these two
	wxString m_curChapter;
	wxString m_curVerse;	
	//******************
#endif
	bool    m_bFirstIntroSectionHasBeenStarted; // supports \is# in introduction material
	bool    m_bFirstSectionHasBeenStarted; // needed to help get "columns" 
							// <div> tag properly placed, FALSE until first
							// scrSection is done, and "columns" <div> tag
							// must be placed immediately before it (we can't
							// rely on there always being title information -
							// but if present, it follows that stuff)
	bool	m_bMajorSectionJustOpened; // TRUE when \ms or \ms1 has been processed,
							// it will occur a little before a \s quite often, and
							// we don't want the following \s to itself initiate
							// emitting a <div class="scrSection"> tag when the
							// major section has already done so. Set it FALSE at
							// a \p or \v.
	//bool	m_bListItemJustOpened; // TRUE when \li# #=1or 2, has just been opened; 
							// restore to FALSE at end of case dealing with a \p marker;
							// use to prevent a \p from itself starting a newline if a
							// \li1 or \li or \li2 has just opened a newline
					// I don't think this will ever be needed -- paragraphs have some
					// introductory text before a list commences

	// The following function is for normalizing paragraph types.
	// \pmo "opening embedded paragraph" >> \p
	// \pm  "embedded paragraph" >> \p
	// \mi  "indented flush left paragraph" >> \m
	// \pmr "embedded refrain paragraph" >> \p
	// \pmc "embedded centred paragraph" >> \p
	// other paragraph USFM markers (listed in the USFM spec) I'll not bother about, they
	// are rarely used, and I'll only alter this exporter code if someone hollers re them
	// Here now is the function; it should be used early in DoXhtmlExport() before the
	// parsing is done - call once for each change as above, and some others I added
	wxString FindAndReplaceWith(wxString text, wxString searchStr, wxString replaceStr);

    // setup of the xhtml apparatus 
	void SetupXhtmlApparatus(); // call this in the Xhtml() creator, just before it exits

	// functions: encoding converters & for mining of metadata from the exported text
	CBString GetLanguageCode();
	CBString ToUtf8(const wxString& str);
	wxString ToUtf16(CBString& bstr);
	CBString GetExporterID();
	CBString GetDateTime();
	CBString GetMyBookName(); // grab's what is in the app's member, m_bookName_Current
	CBString GetTargetLanguageName();
	//CBString GetRunningHeader(wxString* pBuffer); // pass in m_pBuffer member
	//CBString GetMachineName(); <<-- removed 20May13, this is not used anywhere
	CBString InsertCallerIntoTitleAttr(CBString templateStr);

	// iterator for use with the Enum2LabelMap
	Enum2LabelMap::iterator enumIter;

	// ---------------- helpful BUILDER FUNCTIONS for some productions ----------------

	wxString GetVerseOrChapterString(wxString data);
	CBString ConvertData(wxString data); // does entity replacements and converts to UTF-8
	CBString MakeUUID();
	CBString ConstructPictureID(wxString bookID, int nPictureNum);

	// XHTML production templates
	
	CBString m_spanTemplate[10];
	CBString m_divOpenTemplate;
	CBString m_divCloseTemplate;

	CBString m_imgTemplate;
	CBString m_anchorPcdataTemplate;
	CBString m_picturePcdataTemplate;
	CBString m_picturePcdataEmptyRefTemplate;
	CBString m_metadataTemplate;
	CBString m_footnoteMarkerTemplate;

	// ****************** TODO ****************** don't forget templates for caption ***********************************************************

	// Builders for the xhtml productions....
	CBString GetClassLabel(XhtmlTagEnum key); // map key to stored wxString & convert to utf8 C-string & return it
	CBString BuildDivTag(XhtmlTagEnum key);
	// next builds for simple, or plusClass SpanTypeEnum enum values
	CBString BuildSpan(SpanTypeEnum spanType, XhtmlTagEnum key, CBString param, CBString pcData);
	// next builds for chapterNumSpan, or verseNumSpan SpanTypeEnum values
	CBString BuildCHorV(SpanTypeEnum spanType, CBString langCode, CBString corvStr);
	// next builds for xrefNested SpanTypeEnum enum value
	CBString BuildNested(XhtmlTagEnum key, CBString uuid, CBString langCode, CBString pcData);
	// next, to build the productions for USFM \fig ... \fig* picture info
	CBString BuildPictureProductions(CBString strPictureID, CBString langCode, CBString figureData);
	// next one build the first part of footnote or endnote; the second part(s) of the
	// footnote or endnote is/are built using BuildSpan()'s simple option, then after the
	// last part is done, we must add an extra </span> endtag
	CBString BuildFootnoteInitialPart(XhtmlTagEnum key, CBString uuid);
	// next builds for srcFootnoteMarker class attribute in <span>, precedes footnote,
	// endnote or cross reference -- takes the identical UUID that BuildNested takes, and
	// precedes the production which BuildNested() builds; but the anchor tag will be
	// built by a separate BuildAnchor() function, and that is then passed to this one
	CBString BuildFootnoteMarkerSpan(CBString myHrefAnchor);
	CBString BuildAnchor(CBString myUUID); // see comment above BuildFootnoteMarkerSpan()
	CBString BuildTitleInfo(wxString*& pText);
	CBString BuildFXRefFe();
	CBString FinishOff(CBString& strXhtml);
	// next is for footnotes or endnotes
	CBString BuildFootnoteOrEndnoteParts(XhtmlTagEnum key, CBString uuid, wxString data);
	// next is for cross references of type \x ... \x*
	CBString BuildCrossReferenceParts(XhtmlTagEnum key, CBString uuid, wxString data);
	// next is almost identical to BuildSpan() except I've an obligatory space after
	// chvsREF in the template, and I'll make the class part into boilerplate text
	CBString BuildNoteTgtRefSpan(CBString langCode, CBString chvsREF);
	// next is for the last span, the reference span, in a caption production
	CBString BuildCaptionRefSpan(CBString langCode, CBString chvsREF);
	CBString BuildEmptySpan(CBString langCode);

	// ----------- end of helpful BUILDER FUNCTIONS for some productions ----------------


	DECLARE_EVENT_TABLE()
};


#endif /* Xhtml_h */

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Xhtml.cpp
/// \author			Bruce Waters
/// \date_created	9 June 2012
/// \date_revised	
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// This is the implementation file for the Xhtml export class. The Xhtml class takes an
/// in-memory plain text (U)SFM marked up text, removes unwanted USFM markers (e.g inline
/// markers like for italics, and material such as book introduction markers and their
/// conntent...) and then builds xhtml output. (Deuterocannon books are not supported.) 
///
/// \derivation		The Xhtml class is derived from wxEvtHandler (it uses some functions
///                 from an unfinished Oxes.h & .cpp module which has been abandoned)
//////////////////////////////////////////////////////////////////////////////

// ***************************  NOTE!  ***************************************************
// There are 3 #defines just below. If you want to have, for debugging purposes, an
// indented pretty-formated SECOND file (same filename but with "_IndentedXHTML" appended
// to the filename title) output to the same folder as the one where the xhtml export goes,
// then uncomment out DO_INDENT and XHTML_PRETTY here; and also you MUST do the same at the
// end of the DoXthmlExport() function in ExportFunctions.cpp. You'll then get a second
// file dialog which allows you to chose the exported xhtml file, and the indenting and
// pretty formatting will be done. The pretty formatting vertically lines up <span>
// tags, that's all. If you just choose DO_INDENT, you only get <div> tags lined up
// vertically. If you do not uncomment out those two, but instead uncomment out just
// DO_CLASS_NAMES you'll still see the extra file dialog, you choose the exported xhtml
// file as before, but the output is just a vertical list of all the distinct class
// attribute's stylenames -- such as Section_Head, Line1, Line2, and so forth. Do this if
// you want to get an inventory of such names for the xhtml just exported. Likewise, do the
// same uncommenting out at the end of the DoXthmlExport() function in ExportFunctions.cpp
// to make this work. It's the latter which actually gets the call of the indenting
// function, which does the indenting, or inventorying, as the case may be, done.
// Whether you comment them out again or not, these extra jobs are only done in the debug
// build. So they won't do anything in a release version.
//  **************************************************************************************
//#define DO_CLASS_NAMES
#define DO_INDENT
#define XHTML_PRETTY  // comment out when valid indenting of xhtml is wanted;
					  // but leave #defined if each <span) is to be lined up
					  // at 1 level of indent more than <div> and </div>, for
					  // working at getting the code right and avoiding throwing
					  // important stuff way off the right edge of the screens

#define _IntroOverrun
#undef _IntroOverrun

#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Xhtml.h"
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

#if defined(__VISUALC__) && __VISUALC__ >= 1400
#pragma warning(disable:4428)	// VC 8.0 wrongly issues warning C4428: universal-character-name 
								// encountered in source for a statement like 
								// ellipsis = _T('\u2026');
								// which contains a unicode character \u2026 in a string literal.
								// The MSDN docs for warning C4428 are also misleading!
#endif

#include <wx/arrimpl.cpp>
#include <wx/tokenzr.h>
#include <wx/filesys.h> // for wxFileName

#include "AdaptitConstants.h"
#include "Adapt_It.h"
#include "Adapt_ItDoc.h"
#include "helpers.h"
#include "BString.h"
#include "XML.h"
#include "Xhtml.h"

// ----------------------  SETUP APPARATUS  ---------------------------

void Xhtml::SetupXhtmlApparatus()
{
	// we need map entries for all USFMs which case a halt when parsing with our
	// ParseMarker_Content_Endmarker() function
	
	// for an unknown marker...
	(*m_pUsfm2IntMap)[unknownMkr] = (int)no_value_;  // define unknownMkr to be \xxx
	
	// set up the mapping from USFM markers to XhtmlTagEnum values
	(*m_pUsfm2IntMap)[mtMkr] = (int)title_main_;
	(*m_pUsfm2IntMap)[mt1Mkr] = (int)title_main_;
	(*m_pUsfm2IntMap)[mt2Mkr] = (int)title_secondary_;
	(*m_pUsfm2IntMap)[mt3Mkr] = (int)title_tertiary_;
	
	// "major section: \c 1 \ms BOOK ONE \mr (Psalms 1-41) \s True happiness
	(*m_pUsfm2IntMap)[msMkr] = (int)major_section_;  // not yet supported in xhtml
	// "major section: \c 1 \ms BOOK ONE \mr (Psalms 1-41) \s True happiness
	(*m_pUsfm2IntMap)[ms1Mkr] = (int)major_section_;  // not yet supported in xhtml

	// normal & minor section markers & || passage
	(*m_pUsfm2IntMap)[sMkr] = (int)section_;
	(*m_pUsfm2IntMap)[s1Mkr] = (int)section_;
	(*m_pUsfm2IntMap)[s2Mkr] = (int)minor_section_;
	// \s3 ??? undefined in TE, so conflate with section_minor_ enum value
	(*m_pUsfm2IntMap)[s3Mkr] = (int)minor_section_; // *********** conflated **************
	(*m_pUsfm2IntMap)[rMkr] = (int)parallel_passage_ref_;

	// footnote, endnote and crossReference
	(*m_pUsfm2IntMap)[fMkr] = (int)footnote_;
	(*m_pUsfm2IntMap)[fvMkr] = (int)v_num_in_note_;
	(*m_pUsfm2IntMap)[frMkr] = (int)REF_;
	(*m_pUsfm2IntMap)[xoMkr] = (int)REF_;
	(*m_pUsfm2IntMap)[ftMkr] = (int)footnote_text_;
	(*m_pUsfm2IntMap)[fqMkr] = (int)footnote_quote_; // TE wrongly uses Alternate_Reading for this
	(*m_pUsfm2IntMap)[fqaMkr] = (int)alt_quote_; // TE has Alternate_Reading for \fq (rather than \fqa)
	(*m_pUsfm2IntMap)[fkMkr] = (int)footnote_referenced_text_;
	(*m_pUsfm2IntMap)[feMkr] = (int)endnote_;
	(*m_pUsfm2IntMap)[xMkr] = (int)crossReference_;
	(*m_pUsfm2IntMap)[xkMkr] = (int)crossref_referenced_text_;
	(*m_pUsfm2IntMap)[xtMkr] = (int)crossReference_target_reference_; // for \xt (we just delete \xt)
	(*m_pUsfm2IntMap)[rqMkr] = (int)inline_crossReference_; // for \rq ... \rq*

	// chapter and verse and paragraph ones
	(*m_pUsfm2IntMap)[cMkr] = (int)c_;
	(*m_pUsfm2IntMap)[clMkr] = (int)chapter_head_;
	(*m_pUsfm2IntMap)[vMkr] = (int)v_;
	(*m_pUsfm2IntMap)[pMkr] = (int)p_;
	(*m_pUsfm2IntMap)[mMkr] = (int)m_;
	(*m_pUsfm2IntMap)[fqaMkr] = (int)alt_quote_; // TE's "Alternate_Reading" maps to 
												 // USFM \fqa 'footnote quote alternate'
	// introduction markers
	(*m_pUsfm2IntMap)[ipMkr] = (int)intro_paragraph_;
	(*m_pUsfm2IntMap)[isMkr] = (int)intro_section_;
	(*m_pUsfm2IntMap)[is1Mkr] = (int)intro_section_;
	//(*m_pUsfm2IntMap)[is2Mkr] = (int)intro_section_2_; // <<- not supported yet
	(*m_pUsfm2IntMap)[iliMkr] = (int)intro_list_item_1_;
	(*m_pUsfm2IntMap)[ili1Mkr] = (int)intro_list_item_1_;
	(*m_pUsfm2IntMap)[ili2Mkr] = (int)intro_list_item_2_;

	// for poetry's \q, \q1, \q2, \q3 markers
	(*m_pUsfm2IntMap)[qMkr] = (int)line1_;
	(*m_pUsfm2IntMap)[q1Mkr] = (int)line1_;
	(*m_pUsfm2IntMap)[q2Mkr] = (int)line2_;
	(*m_pUsfm2IntMap)[q3Mkr] = (int)line3_;
	(*m_pUsfm2IntMap)[bMkr] = (int)stanza_break_;

	// for \id and \h -- we pair with some XhtmlTagEnum values, but we don't process these
	// two with any xml production
	(*m_pUsfm2IntMap)[idMkr] = (int)id_;
	(*m_pUsfm2IntMap)[hMkr] = (int)running_hdr_;	
	//(*m_pUsfm2IntMap)[nbMkr] = (int)no_break_paragraph_; // not yet supported in xhtml
	(*m_pUsfm2IntMap)[remMkr] = (int)remark_;

	// inscriptions & other inline-markers -- in xhtml, these each get their own <span> (I
	// think), at least the xhtml will validate; \fig ...\fig* stuff is internally complex
	(*m_pUsfm2IntMap)[pcMkr] = (int)inscription_paragraph_;
	(*m_pUsfm2IntMap)[scMkr] = (int)inscription_;
	(*m_pUsfm2IntMap)[itMkr] = (int)emphasis_;
	(*m_pUsfm2IntMap)[qtMkr] = (int)quoted_text_;
	(*m_pUsfm2IntMap)[wjMkr] = (int)words_of_christ_;
	(*m_pUsfm2IntMap)[wMkr] = (int)see_glossary_;
	(*m_pUsfm2IntMap)[figMkr] = (int)figure_; // for \fig .... \fig*
	
	// lists and citations
	(*m_pUsfm2IntMap)[liMkr] = (int)list_item_1_;
	(*m_pUsfm2IntMap)[li1Mkr] = (int)list_item_1_;
	(*m_pUsfm2IntMap)[li2Mkr] = (int)list_item_2_;
	(*m_pUsfm2IntMap)[qmMkr] = (int)citation_line_1_;
	(*m_pUsfm2IntMap)[qm1Mkr] = (int)citation_line_1_;
	(*m_pUsfm2IntMap)[qm2Mkr] = (int)citation_line_2_;
	(*m_pUsfm2IntMap)[qm3Mkr] = (int)citation_line_3_;
	
// Next, the xhtml label names for the class attribute's string values
	// 5Jul12, I'll use the TE-based style names for now, but where I don't know the
	// appropriate one, I'll use Jim Albright's names from his partially completed
	// documentation

	(*m_pEnum2LabelMap)[(int)no_value_] = _T("Unknown_Marker_xxx"); // in the production, replace xxx with the marker (omit backslash)
	(*m_pEnum2LabelMap)[(int)title_main_] = _T("Title_Main"); // USFM \mt1 or \mt
	(*m_pEnum2LabelMap)[(int)title_secondary_] = _T("Title_Secondary"); // USFM \mt2
	(*m_pEnum2LabelMap)[(int)title_tertiary_] = _T("Title_Tertiary"); // USFM \mt3
	(*m_pEnum2LabelMap)[(int)c_] = _T("Chapter_Number"); // USFM \c 
	(*m_pEnum2LabelMap)[(int)v_] = _T("Verse_Number"); // USFM \v
	(*m_pEnum2LabelMap)[(int)v_num_in_note_] = _T("Verse_Number_In_Note"); // USFM  \fv (a verse num within a footnote or endnote)
	(*m_pEnum2LabelMap)[(int)intro_paragraph_] = _T("Intro_Paragraph"); // USFM \ip
	(*m_pEnum2LabelMap)[(int)intro_section_] = _T("Intro_Section_Head");  // precede with <scrIntroSection> -- see Sena 3 Matthew
	(*m_pEnum2LabelMap)[(int)intro_list_item_1_] = _T("Intro_List_Item1"); // USFM \ili or \ili1
	(*m_pEnum2LabelMap)[(int)intro_list_item_2_] = _T("Intro_List_Item2"); // USFM \ili2
	(*m_pEnum2LabelMap)[(int)line1_] = _T("Line1"); // USFM \q or \q1
	(*m_pEnum2LabelMap)[(int)line2_] = _T("Line2"); // USFM \q2
	(*m_pEnum2LabelMap)[(int)line3_] = _T("Line3"); // USFM \q3
	(*m_pEnum2LabelMap)[(int)list_item_1_] = _T("List_Item1"); // USFM \li or \li1
	(*m_pEnum2LabelMap)[(int)list_item_2_] = _T("List_Item2"); // USFM \li2
	(*m_pEnum2LabelMap)[(int)footnote_] = _T("Note_General_Paragraph"); // precede with scrFootnoteMarker \f |f*
	(*m_pEnum2LabelMap)[(int)REF_] = _T("Note_Target_Reference"); // USFM \fr or \xo
	(*m_pEnum2LabelMap)[(int)footnote_referenced_text_] = _T("Referenced_Text"); // \fk ...\fk*
	(*m_pEnum2LabelMap)[(int)footnote_quote_] = _T("Alternate_Reading"); // TE wrongly assigns\fq 'footnote quote' to this style
	(*m_pEnum2LabelMap)[(int)alt_quote_] = _T("Alternate_Reading"); // \fqa (TE doesn't seem to have it)
	(*m_pEnum2LabelMap)[(int)endnote_] = _T("Endnote_General_Paragraph"); // precede with scrEndnoteMarker? \fe \fe*, but xrefs uses scrFootnoteMarker!
	(*m_pEnum2LabelMap)[(int)inline_crossReference_] = _T("Note_CrossHYPHENReference_Paragraph"); // precede with scrFootnoteMarker (\rq ... \rq*)
	(*m_pEnum2LabelMap)[(int)p_] = _T("Paragraph"); // USFM \p
	(*m_pEnum2LabelMap)[(int)m_] = _T("Paragraph_Continuation"); // USFM \m
	(*m_pEnum2LabelMap)[(int)quoted_text_] = _T("Quoted_Text"); // USFM \qt  \qt*
	(*m_pEnum2LabelMap)[(int)section_] = _T("Section_Head"); // precede with scrSection in <div> tag's class
	(*m_pEnum2LabelMap)[(int)minor_section_] = _T("Section_Head_Minor"); // precede with scrSection in <div> tag's class USFM \s2 or \s3
	(*m_pEnum2LabelMap)[(int)major_section_] = _T("Section_Head_Major"); // precede with scrSection in <div> tag's class USFM \ms or \ms1
	(*m_pEnum2LabelMap)[(int)words_of_christ_] = _T("Words_Of_Christ"); // USFM \wj  \wj*
	(*m_pEnum2LabelMap)[(int)alt_quote_] = _T("Alternate_Reading"); // USFM \fqa 'footnote quote alternate' (within footnote or endnote)
	(*m_pEnum2LabelMap)[(int)chapter_head_] = _T("Chapter_Head"); // USFM \cl
	(*m_pEnum2LabelMap)[(int)citation_line_1_] = _T("Citation_Line1"); // (USFM \qm1 or \qm) within \q1 or \q
	(*m_pEnum2LabelMap)[(int)citation_line_2_] = _T("Citation_Line2"); // (USFM \qm2) within \q2
	(*m_pEnum2LabelMap)[(int)citation_line_3_] = _T("Citation_Line3"); // (USFM \qm3) within \q3
	(*m_pEnum2LabelMap)[(int)emphasis_] = _T("Emphasis"); // occurs within one of those markers, USFM has endmarker \it ... \it*
	(*m_pEnum2LabelMap)[(int)inscription_] = _T("Inscription"); // USFM has endmarker, \sc ... \sc*
	(*m_pEnum2LabelMap)[(int)inscription_paragraph_] = _T("Inscription_Paragraph"); // USFM \pc
	(*m_pEnum2LabelMap)[(int)parallel_passage_ref_] = _T("Parallel_Passage_Reference"); // USFM \r
	(*m_pEnum2LabelMap)[(int)see_glossary_] = _T("See_In_Glossary"); // USFM \w ... \w*
	(*m_pEnum2LabelMap)[(int)stanza_break_] = _T("Stanza_Break"); // USFM \b
	(*m_pEnum2LabelMap)[(int)remark_] = _T("rem"); // USFM \b
	// reference is an attribute,  class="reference" in span at end of a caption

	// the following ones are not associated so directly with markers, e.g. may be
	// associated with a preceding tag like <div> - the class attr's value within such...
	(*m_pEnum2LabelMap)[(int)columns_] = _T("columns");
	(*m_pEnum2LabelMap)[(int)scrBody_] = _T("scrBody");
	(*m_pEnum2LabelMap)[(int)scrBook_] = _T("scrBook");
	(*m_pEnum2LabelMap)[(int)scrBookCode_] = _T("scrBookCode");
	(*m_pEnum2LabelMap)[(int)scrBookName_] = _T("scrBookName");
	(*m_pEnum2LabelMap)[(int)scrFootnoteMarker_] = _T("scrFootnoteMarker");
	(*m_pEnum2LabelMap)[(int)scrIntroSection_] = _T("scrIntroSection");
	(*m_pEnum2LabelMap)[(int)scrSection_] = _T("scrSection");
	/* I ended up not needing to use these, but they document the style names so I'll keep them
	(*m_pEnum2LabelMap)[(int)picture_] = _T("picture");
	(*m_pEnum2LabelMap)[(int)pictureCaption_] = _T("pictureCaption");
	// Note: next two are the only options USFM supports, for options 'col' & 'span' respectively
	(*m_pEnum2LabelMap)[(int)pictureColumn_] = _T("pictureColumn");
	(*m_pEnum2LabelMap)[(int)picturePage_] = _T("picturePage");
	// Note: next three are additional options supported by TE, but these won't appear in
	// a USFM export and so I'll include them here, but they'll be never used I expect
	(*m_pEnum2LabelMap)[(int)pictureRight_] = _T("pictureRight");
	(*m_pEnum2LabelMap)[(int)pictureLeft_] = _T("pictureLeft");
	(*m_pEnum2LabelMap)[(int)pictureCenter_] = _T("pictureCenter");
	*/

	footnoteMkr = _T("\\f");
	endnoteMkr = _T("\\fe");
	xrefMkr = _T("\\x");
	inline_xrefMkr = _T("\\rq");
	wordsOfChristEndMkr = _T("\\wj*"); // needed for testing against m_endMkr

	// set up the xhtml production templates (these are single-byte utf8 encoding, using CBString)
	m_divOpenTemplate = "<div class=\"classAttrLabel\">";
	m_divCloseTemplate = "</div>";
	m_imgTemplate = "<img id=\"idAttrUUID\" class=\"picture\" src=\"srcAttrURL\" alt=\"altAttrURL\" />";
	m_anchorPcdataTemplate = "<a href=\"#hrefAttrUUID\"></a>"; // PCDATA of <span></span> at start of cross reference
	// In the next one, myImgPCDATA has to first be filled out from m_imgTemplate, and
	// then inserted here (picturePlacement can only be one of pictureLeft, pictureCenter, pictureRight)
	m_picturePcdataTemplate = "<div class=\"picturePlacement\">myImgPCDATA<div class=\"pictureCaption\"><span lang=\"langAttrCode\">captionPCDATA</span><span lang=\"langAttrCode\" class=\"reference\">refPCDATA</span></div></div>";
	m_picturePcdataEmptyRefTemplate = "<div class=\"picturePlacement\">myImgPCDATA<div class=\"pictureCaption\"><span lang=\"langAttrCode\">captionPCDATA</span><span lang=\"langAttrCode\" class=\"reference\" /></div></div>";
	// the next is the file-initial metadata, parmeterized
	m_metadataTemplate = "<?xml version=\"1.0\" encoding=\"utf-8\"?>NEWLINE<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">NEWLINE<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"utf-8\" lang=\"utf-8\">NEWLINE<!--NEWLINEThere are no spaces or newlines between <span> elements in this file becauseNEWLINEwhitespace is significant.  We don't want extraneous spaces appearing in theNEWLINEdisplay/printout!NEWLINE      -->NEWLINE<head>NEWLINE<title /><link rel=\"stylesheet\" href=\"myCSSStyleSheet\" type=\"text/css\" /><meta name=\"linkedFilesRootDir\" content=\"myFilePath\" /><meta name=\"description\" content=\"myDescription\" /><meta name=\"filename\" content=\"myFilename\" /></head>NEWLINE<body class=\"scrBody\">NEWLINE";
	m_footnoteMarkerTemplate = "<span class=\"scrFootnoteMarker\">anUuidAnchor</span>";

	m_spanTemplate[simple] = "<span lang=\"langCode\">spanPCDATA</span>";
	m_spanTemplate[plusClass] = "<span class=\"classAttrLabel\" lang=\"langAttrCode\">spanPCDATA</span>";
	// next is the only nested span I've observed, it's in cross reference; UUID must be
	// same as in hrefAttrUUID of m_anchorPCDATATemplate below; this one has 4 variables
	m_spanTemplate[nested] = "<span class=\"classAttrLabel\" id=\"idAttrUUID\" title=\"+\"><span lang=\"langAttrCode\">myPCDATA</span></span>";
	// next one is for chapter numbers (two variables)-- we could use our
	// m_spanTemplate[plusClass] but chapters are so common that saving on one
	// parameter in the function for filling it is worth a separate template
	m_spanTemplate[chapterNumSpan] = "<span class=\"Chapter_Number\" lang=\"langAttrCode\">chNumPCDATA</span>";
	// next one is for verse numbers (two variables)-- we could use our
	// m_spanTemplate[plusClass] but verses are so common that saving on one
	// parameter in the function for filling it is worth a separate template
	m_spanTemplate[verseNumSpan] = "<span class=\"Verse_Number\" lang=\"langAttrCode\">vNumPCDATA</span>";
	// footnotes and endnotes have internal structure which may require several plusClass
	// type of spans, before these is the footnote 'first part' - it's template is the
	// following, note there is no closing </span>. Also, cross references \x .... \x*
	// also use this one
	m_spanTemplate[footnoteFirstPart] = "<span class=\"classAttrLabel\" id=\"idAttrUUID\" title=\"+\">";
	// for the Note_Target_Reference in footnote, endnote and cross reference (the \x ...\x* type)
	m_spanTemplate[targetREF] = "<span class=\"Note_Target_Reference\" lang=\"langAttrCode\">chvsREF </span>"; // note, space follows chvsREF
	// for the caption ref, if chvsREF is an empty string, then make it an empty tag
	m_spanTemplate[captionREF] = "<span lang=\"langAttrCode\" class=\"reference\">chvsREF</span>";
	m_spanTemplate[captionREFempty] = "<span lang=\"langAttrCode\" class=\"reference\" />";
	m_spanTemplate[emptySpan] = "<span lang=\"langAttrCode\" />";
}

// *******************************************************************
// Event handlers
// *******************************************************************

BEGIN_EVENT_TABLE(Xhtml, wxEvtHandler)
// there's no GUI for this export job
END_EVENT_TABLE()

// *******************************************************************
// Construction/Destruction
// *******************************************************************

Xhtml::Xhtml()
{
}

Xhtml::Xhtml(CAdapt_ItApp* app)
{
	wxASSERT(app != NULL);
	m_pApp = app;

	m_pUsfm2IntMap = new Usfm2EnumMap;
	m_pEnum2LabelMap = new Enum2LabelMap;
	Initialize();
}

Xhtml::~Xhtml()
{
	m_pUsfm2IntMap->clear();
	m_pEnum2LabelMap->clear();
	delete m_pUsfm2IntMap;
	delete m_pEnum2LabelMap;
}

void Xhtml::SetBookID(wxString& aValidBookCode)
{
	m_bookID = aValidBookCode;
}

void Xhtml::Initialize()
{
	m_aTab = "    "; // use 4 spaces
	m_eolStr = ToUtf8(m_pApp->m_eolStr);
	m_emptyStr.Empty();

    // do the following only once-only data structure setups here; each time a new xhtml
    // file is to be produced, the stuff specific to any earlier exports will need to be
    // cleared out before the new one's data is added in (using a separate function)
	 
	// set up the maps we need to use, and the xhtml building templates
	SetupXhtmlApparatus();
}

bool Xhtml::IsWhiteSpace(wxChar& ch)
{
	// cloned from the one in CAdapt_ItDoc class, so as to be consistent
	// with the parsers in the rest of the app
#ifdef _UNICODE
	wxChar NBSP = (wxChar)0x00A0; // standard Non-Breaking SPace
#else
	wxChar NBSP = (unsigned char)0xA0;  // standard Non-Breaking SPace
#endif

	// handle common ones first...
	// returns true for tab 0x09, return 0x0D or space 0x20
	// _istspace not recognized by g++ under Linux; the wxIsspace() fn and those it relies
	// on return non-zero if a space type of character is passed in
	if (wxIsspace(ch) != 0 || ch == NBSP)
	{
		return TRUE;
	}
	else
	{
#ifdef _UNICODE
		wxChar WJ = (wxChar)0x2060; // WJ is "Word Joiner"
		if (ch == WJ || ((UInt32)ch >= 0x2000 && (UInt32)ch <= 0x200B))
		{
			return TRUE;
		}
#endif
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
/// \return     the length, in characters, of the parsed marker, it's content, and any
///             endmarker if present - halting at the backslash of the begin marker which 
///             follows, or at the buffer end if no more markers follow
/// \param  buffer  ->  reference to the wxString containing however much of the original
///                     (U)SFM marked up plain text for the adaptation is remaining for
///                     being parsed
/// \param  mkr     <-  The marker which begins this section of data
/// \param  endMkr  <-  The marker which ends this section of data, empty if there is
///                     no endmarker for begin marker, or if the begin marker should
///                     have had an endmarker, but a markup error caused it to be lacking
/// \param  dataStr <-  The contents of the marker's associated data type
/// \param  nextMkr <-  return's the next marker (the one which halted the parse) after
///                     the current marker+content+endmarker has been scanned over, or
///                     if there is no next marker (the end of data was reached), then it
///                     returns an empty string
///                     
///  Side-effects: it produces the needed xhtml productsion for chapter number or verse
///  number each time a \c or \v marker is encountered, and stores the production in
///  m_chapterNumSpan, or m_verseNumSpan, as the case may be, directly - before returning.
///  The code for building the xhtml output file will look at those members, and if
///  non-empty will insert the relevant production where appropriate, and then clear the
///  member concerned. When parsing, the data content for a \v or \c marker will have the
///  verse number of chapter number first in the content string, and so that makes it easy
///  to extract herein and build the needed xhtml production.
///  
/// \remarks
/// On entry, buffer is the whole of the remaining unparsed adaptation's content, and it's
/// first character is a begin marker's backslash; or text with no preceding marker (see
/// change added for 15Sept10 note below). The beginning marker is parsed, being returning
/// in the mkr param, but if there is no beginning marker then the empty string is returned
/// in mkr, and parsing continues across the marker's content, or across the text at the
/// buffer's start as the case may be, returning that in dataStr, with whitespace trimmed
/// off of both ends. (Chapter and/or verse numbers are extracted as well and the relevant
/// xhtml productions formed as a side-effect, and that material removed from the beginning
/// of the parsed content before the content is sent back to the caller. If an endmarker is
/// present then that is parsed and returned in endMkr, and parsing continues beyond the
/// endmarker until the start of the next marker is encountered, or to file end.
/// 
/// Protection is built in so that if a marker's expected endmarker is absent because of a
/// SFM markup error, the function will not return any endmarker but will still return the
/// marker's content and halt at the start of the next marker - thus making it tolerant of
/// the lack of an expected endmarker. (It is not, however, tolerant of a mismatched
/// endmarker, such as if someone typed an endmarker but typed it wrongly so that the
/// actual endmarker turned out to be an unknown endmarker or an endmarker for some other
/// known marker - in either of those scenarios, bEndMarkerError is returned TRUE, and the
/// caller should abort the parse with an appropriate warning to the user.
/// 
/// While an endmarker is returned via the signature, the caller will usually ignore it
/// because it is included within the count of the span of characters delineated by the
/// parse, and that's all that matters.
/// 
/// BEW created 6Sep10 (an older version, for Oxes parsing)
/// BEW 15Sep10, added protocol that if mkr passed in is an empty string, then parsing
/// continues on the text from start of the passed in buffer, until a halt location is reached
/// BEW 24May12, pressed this function back into Oxes service in order to parse the markers
/// and their text content, in the exported USFM text. The idea is that the returned length
/// will be used in its caller in order to consume the material parsed over, until the
/// whole text is consumed. In the process, the xml productions are built with the
/// information returned in the parse.
/// BEW 28May12, added nextMkr to signature
/// BEW 13Jun12, altered to handle parsing for an xhtml export. It needed to be altered so
/// as not to parse over inline markers -- because xhtml creates a span for any such
/// currently supported, and so we must halt at them -- this means a rewrite of the innards
/// was called for - considerably more simple fortunately. Unfortunately, TE, and therefore
/// xhtml, doesn't yet support all inline USFM markers (e.g. \nd ... \nd* Name_Of_Deity
/// isn't supported - that is, no label like Name_Of_Deity is as yet defined, and no style
/// created for display of it; similarly \k .. ]k* keyword, and others. Therefore, in order
/// that encountering such markers doesn't break the export, we need to preprocess the USFM
/// exported file to remove markers we have a reasonable expectation of encountering, to
/// decrease the risk of failure. The altered parser will have fewer params in the
/// signature (only 5 needed I think, rather than the for-Oxes 10); and see the Side-effect
/// note above, which applies only to this xhtml version of the function. We also simplify,
/// we parse from the current location (usually a beginMkr but could be text, eg. after an
/// endmarker such as \wj* there could be text, similarly for other inline endmarkers) to
/// the next marker - and if the next marker is an endmarker, we parse over it and return
/// it, but if not, we halt at that beginmarker, return empty string for the endmarker,
/// and return to caller. So we throw more of the interpretive burden on the case blocks
/// in the switch in the caller.
int Xhtml::ParseMarker_Content_Endmarker(wxString& buffer, wxString& mkr, 
					wxString& endMkr, wxString& dataStr, wxString& nextMkr)
{
	wxString wholeMarker;
	mkr.Empty(); // default to being empty, until set below
	endMkr.Empty(); // default to being empty - we may define an endmarker later below
	dataStr.Empty(); // ensure this starts out empty
	nextMkr.Empty();
	int dataLen = 0;
	int aLength = 0;
	size_t bufflen = buffer.Len();
	const wxChar* pBuffStart = buffer.GetData();
    // remove the const modifier for pBuffStart; however, we don't modify the buffer
    // directly within this function, instead we leave that for the caller to do; all we
    // want here is to be able to get a wxChar* iterator for parsing purposes
	wxChar* ptr = const_cast<wxChar*>(pBuffStart);
	wxChar* pEnd = ptr + bufflen;
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	wxChar* pContentStr = NULL;
	int itemLen = 0;
	int txtLen = 0; // use this to count characters parsed, including those 
					// which belong to markers 

	// do the next block if on entry we are pointing at the backslash of a marker
	if (pDoc->IsMarker(ptr)) 
	{
		// we are pointing at the backslash of a marker
		wholeMarker = pDoc->GetWholeMarker(ptr); // has an initial backslash
		mkr = wholeMarker; // caller needs to know what marker it was
		wxASSERT(!pDoc->IsEndMarker(ptr, pEnd)); // it's not an endmarker
		aLength = wholeMarker.Len();
		ptr += aLength; // point past initial marker
		txtLen += aLength; // count its characters
		dataLen += aLength;

		// parse, and count, the white space following
		itemLen = pDoc->ParseWhiteSpace(ptr);
		ptr += itemLen;
		txtLen += itemLen;
		dataLen += itemLen;
	} // end TRUE block for test: if (pDoc->IsMarker(ptr))

	// mark the starting point for the content to be returned in dataStr
	pContentStr = ptr; // Note, include final white space in contentStr & trim later

	// scan to next marker
	// txtLen must start again from 0, we are counting chars to the next marker (don't
	// forget the debugger doesn't show CR or LF, and one or both will be present before
	// the marker)
	txtLen = 0;
	while (ptr < pEnd && !pDoc->IsMarker(ptr))
	{
		ptr++;
		txtLen++;
	}
	if (ptr == pEnd)
	{
		// we've come to the end of the text, so we've got data but no endmarker
		endMkr.Empty();
		wxString contents(pContentStr,txtLen);
		contents = contents.Trim(); // trim at right end
		dataStr = contents;
		dataLen += txtLen;
		nextMkr.Empty();
	}
	else
	{
		// we've come to another marker, could be a beginmarker or an endmarker
		if (pDoc->IsEndMarker(ptr, pEnd))
		{
			// we don't care what endmarker it is, any endmarker will end the current
			// span, so we'll exit after we set up the params to be returned, and found
			// what the next beginmarker is (which we return in nextMkr)
			wholeMarker = pDoc->GetWholeMarker(ptr); // get the endmarker
			endMkr = wholeMarker;
			wxString contents(pContentStr,txtLen);
			contents = contents.Trim(); // trim at right end
			dataStr = contents;
			dataLen += txtLen;
			// advance over the endmarker -- don't need txtLen for a while so set it to
			// zero
			txtLen = 0; // play safe, just in case
			itemLen = endMkr.Len();
			ptr += itemLen;
			dataLen += itemLen;
			// now parse ahead over whitespace till we come to the next marker, or to
			// text, or to the buffer end
			itemLen = 0; 
			if (ptr < pEnd)
			{
				itemLen = pDoc->ParseWhiteSpace(ptr);
				if (itemLen > 0)
				{
					dataLen += itemLen;
					ptr += itemLen;
				}
				// BEW added 28May12, we should be halted at the "next" marker, so find what it is
				// and return it to the caller in nextMkr param
				if (pDoc->IsMarker(ptr))
				{
					// just in case there are two consecutive endmarkers, we'll test for
					// another endmarker and parse over it - and make it the endmarker we
					// return to the caller since the second would be more important, eg \wj*
					if (pDoc->IsEndMarker(ptr, pEnd))
					{
						wxString anotherEndMkr = pDoc->GetWholeMarker(ptr);
						endMkr = anotherEndMkr;
						itemLen = anotherEndMkr.Len();
						ptr += itemLen;
						dataLen += itemLen;
						// parse over the white space after it (if any)
						itemLen = pDoc->ParseWhiteSpace(ptr);
						if (itemLen > 0)
						{
							dataLen += itemLen;
							ptr += itemLen;
						}
						// what follows could be a marker or text; if text then
						// nextMkr must be set empty
						if (pDoc->IsMarker(ptr) && !pDoc->IsEndMarker(ptr, pEnd))
						{
							// there is a beginmarker following
							nextMkr = pDoc->GetWholeMarker(ptr);
						}
						else
						{
							// if there was a third endmarker (should be impossible), our
							// wxASSERT() above would trip on the next call of this function
							nextMkr.Empty();
						}
					}
					else
					{
						// it's a beginmarker
						nextMkr = pDoc->GetWholeMarker(ptr);
					}
				}
			}
			else
			{
				nextMkr.Empty();
			}
		}
		else
		{
			// it's a beginmarker, so get what it is and return it in nextMkr
			wholeMarker = pDoc->GetWholeMarker(ptr);
			nextMkr = wholeMarker;
			endMkr.Empty();
			wxString contents(pContentStr,txtLen);
			contents = contents.Trim(); // trim at right end
			dataStr = contents;
			dataLen += txtLen;
		}
	}

	// trim any inital white space from the dataStr string
	dataStr = dataStr.Trim(FALSE);

	// if a \c marker, or \v marker was encountered, parse out the chapter or verse number
	// as the case may be, etc
	m_whichTagEnum = (XhtmlTagEnum)(*m_pUsfm2IntMap)[mkr];
	wxString numberStr;
	if (m_whichTagEnum == c_)
	{
		// build the chapter span's production & store in m_chapterNumSpan
		numberStr = GetVerseOrChapterString(dataStr);
#if defined(__WXDEBUG__)
		// next line is for debugging
		m_curChapter = numberStr;
#endif
		if (!numberStr.IsEmpty())
		{
			// bleed off the no longer wanted chapter number & it's following whitespace
			int numLen = numberStr.Len();
			dataStr = dataStr.Mid(numLen);
			dataStr.Trim(FALSE);
			// build the xhtml chapter production & store it
			m_chapterNumSpan = BuildCHorV(chapterNumSpan,GetLanguageCode(),ToUtf8(numberStr));
		}
	}
	else if (m_whichTagEnum == v_)
	{
		// build the verse span's production & store in m_verseNumSpan
		numberStr = GetVerseOrChapterString(dataStr);
#if defined(__WXDEBUG__)
		// next line is for debugging
		m_curVerse = numberStr;
#endif
		if (!numberStr.IsEmpty())
		{
			// bleed off the no longer wanted chapter number & it's following whitespace
			int numLen = numberStr.Len();
			dataStr = dataStr.Mid(numLen);
			dataStr.Trim(FALSE); // this could make it empty. e.g. if \wj followed
			// build the xhtml chapter production & store it
			m_verseNumSpan = BuildCHorV(verseNumSpan,GetLanguageCode(),ToUtf8(numberStr));
		}
	}	
	return dataLen;
}

// The next is for parsing over \f ... \fe*, or \x ... \x*, or \fe ... \fe*, and
// nextMkr will return whatever marker follows the \f* or \fe* or \x* (unless of
// course text follows - in which case it will return an empty string). (If I can be
// bothered, I should also handle PNG marker set's \f ... \F (or \f .... \fe))
// mkr will return \f or \x or \fe as the case may be; endMkr will return whatever is
// the endmarker, and dataStr will be everything between these (after leading and
// trailing whitespace has been trimmed off). Internal processing of the data fields
// within dataStr is the responsibility of the caller.
// 
// Note: TE, and therefore the xhtml standard for export, does not appear to support \x
// ... \x* markup (but AI has to be able to handle it, since published USFM files are
// likely to have such markup in them), but TE does support right-justified \rq...\rq*
// inline crossReference markup - and the latter has no internal structure. The function
// below does not explicitly mention \x or \x*, and therefore does not require
// modification if it is true that the xhtml export doesn't have any tags defined for USFM
// \x ... \x* markup spans; so if such is the case, we'll simply decline to emit any xhtml
// in the export for any \x ... \x* material parsed over.
int Xhtml::ParseFXRefFe(wxString& buffer, wxString& mkr, wxString& endMkr, wxString& dataStr, wxString& nextMkr)
{
	wxString wholeMarker;
	mkr.Empty(); // default to being empty, until set below
	endMkr.Empty(); // default to being empty - we may define an endmarker later below
	dataStr.Empty(); // ensure this starts out empty
	nextMkr.Empty();
	int dataLen = 0;
	int aLength = 0;
	size_t bufflen = buffer.Len();
	const wxChar* pBuffStart = buffer.GetData();
    // remove the const modifier for pBuffStart; however, we don't modify the buffer
    // directly within this function, instead we leave that for the caller to do; all we
    // want here is to be able to get a wxChar* iterator for parsing purposes
	wxChar* ptr = const_cast<wxChar*>(pBuffStart);
	wxChar* pEnd = ptr + bufflen;
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	wxChar* pContentStr = NULL;
	int itemLen = 0;
	int txtLen = 0; // use this to count characters parsed, including those 
					// which belong to markers
	wxString matchingEndMkr;

	// do the next block if on entry we are pointing at the backslash of a marker
	wxASSERT(pDoc->IsMarker(ptr));
	// we are pointing at the backslash of a marker
	wholeMarker = pDoc->GetWholeMarker(ptr);
	wxASSERT(wholeMarker == footnoteMkr || wholeMarker == endnoteMkr || wholeMarker == xrefMkr);

	mkr = wholeMarker; // caller needs to know what marker it was
	wxASSERT(!pDoc->IsEndMarker(ptr, pEnd)); // it's not an endmarker
	aLength = wholeMarker.Len();
	ptr += aLength; // point past initial marker
	txtLen += aLength; // count its characters
	dataLen += aLength;

	// parse, and count, the white space following
	itemLen = pDoc->ParseWhiteSpace(ptr);
	ptr += itemLen;
	txtLen += itemLen;
	dataLen += itemLen;

	// mark the starting point for the content to be returned in dataStr
	pContentStr = ptr; // Note, include final white space in contentStr & trim later

	// scan to the matching endmarker (in the case of PNG 1998 marker set, there is only
	// an endmarker for footnotes, and it is either \F or \fe), no other endmarkers
	// 
	// txtLen must start again from 0, we are counting chars to the next marker (don't
	// forget the debugger doesn't show CR or LF, and one or both will be present before
	// the marker)
	txtLen = 0;
	if (m_pApp->gCurrentSfmSet == PngOnly && mkr == footnoteMkr) 
	{
		matchingEndMkr = _T("\\fe");
		wxString matchingEndMkr2 = _T("\\F");
		// scan till we get to matchingEndMkr
		if (ptr < pEnd)
		{	
			do {
				if (pDoc->IsMarker(ptr))
				{
					wholeMarker = pDoc->GetWholeMarker(ptr);
					if (wholeMarker == matchingEndMkr || wholeMarker == matchingEndMkr2)
					{
						if (wholeMarker == matchingEndMkr)
						{
							endMkr = matchingEndMkr;
						}
						else
						{
							endMkr = matchingEndMkr2;
						}
						break;
					}
					else
					{
						ptr++;
						txtLen++;
					}
				}
				else
				{
					ptr++;
					txtLen++;
				}
			} while (ptr < pEnd);
		}
		if (ptr == pEnd)
		{
			// we've come to the end of the text, so we've got data but no matched
			// endmarker - the data is ill-formed, but we'll handle what we've got as if
			// we'd come to the required endmarker
			endMkr.Empty();
			wxString contents(pContentStr,txtLen);
			contents = contents.Trim(); // trim at right end
			dataStr = contents;
			dataLen += txtLen;
			nextMkr.Empty();
		}
		else
		{
			// we've come to the matching endmarker...
			// endMkr already holds the endmarker so we just want to parse forwards
			// over whitespace, to the text, or next marker if that precedes any text
			// - and if so, put that marker in nextMkr and return dataLen up to that
			// point
			wxString contents(pContentStr,txtLen);
			contents = contents.Trim(); // trim at right end
			dataStr = contents; // what's between the begin & endmarkers, remember, it can have markers in it
			dataLen += txtLen;
			// advance over the endmarker -- clear txtLen to be safe, not needed now
			txtLen = 0;
			itemLen = endMkr.Len();
			ptr += itemLen;
			dataLen += itemLen;
			// now parse ahead over whitespace till we come to the next marker, or to
			// text, or to the buffer end
			itemLen = 0; 
			if (ptr < pEnd)
			{
				itemLen = pDoc->ParseWhiteSpace(ptr);
				if (itemLen > 0)
				{
					dataLen += itemLen;
					ptr += itemLen;
				}
				// for the PNG 1998 marker set, there are no inline markers, and so any
				// following marker will be a beginmarker, and we can store a copy in nextMkr
				if (pDoc->IsMarker(ptr))
				{
					nextMkr = pDoc->GetWholeMarker(ptr);
				}
				else
				{
					// must be text, so return nextMkr empty (it's already
					// empty, but doing it again documents the code better)
					nextMkr.Empty();
				}
			}
		} // end of else block for test: if (ptr == pEnd)
	}
	else
	{
		// assume its usfmOnly -- in which case the matching endmarker is just the begin
		// marker with * added
		matchingEndMkr = mkr + _T('*');
		// scan till we get to matchingEndMkr
		if (ptr < pEnd)
		{	
			do {
				if (pDoc->IsMarker(ptr))
				{
					wholeMarker = pDoc->GetWholeMarker(ptr);
					if (wholeMarker == matchingEndMkr)
					{
						endMkr = matchingEndMkr;
						break;
					}
					else
					{
						ptr++;
						txtLen++;
					}
				}
				else
				{
					ptr++;
					txtLen++;
				}
			} while (ptr < pEnd);
		}
		if (ptr == pEnd)
		{
			// we've come to the end of the text, so we've got data but no matched
			// endmarker - the data is ill-formed, but we'll handle what we've got as if
			// we'd come to the required endmarker
			endMkr.Empty();
			wxString contents(pContentStr,txtLen);
			contents = contents.Trim(); // trim at right end
			dataStr = contents;
			dataLen += txtLen;
			nextMkr.Empty();
		}
		else
		{
			// we've come to the matching endmarker...
			// endMkr already holds the endmarker so we just want to parse forwards
			// over whitespace, to the text, or next marker if that precedes any text
			// - and if so, put that marker in nextMkr and return dataLen up to that
			// point
			wxString contents(pContentStr,txtLen);
			contents = contents.Trim(); // trim at right end
			dataStr = contents; // what's between the begin & endmarkers, remember, it can have markers in it
			dataLen += txtLen;
			// advance over the endmarker -- clear txtLen to be safe, not needed now
			txtLen = 0;
			itemLen = endMkr.Len();
			ptr += itemLen;
			dataLen += itemLen;
			// now parse ahead over whitespace till we come to the next marker, or to
			// text, or to the buffer end
			itemLen = 0; 
			if (ptr < pEnd)
			{
				itemLen = pDoc->ParseWhiteSpace(ptr);
				if (itemLen > 0)
				{
					dataLen += itemLen;
					ptr += itemLen;
				}
				// we can't assume that if a marker follows, it's a begin marker
				// because a footnote may occur preceding \wj*. What do we do?
				// I think the safe thing would be to halt at the \wj* and leave
				// nextMkr empty
				if (pDoc->IsMarker(ptr))
				{
					// just in case there are two consecutive endmarkers...
					if (pDoc->IsEndMarker(ptr, pEnd))
					{
						nextMkr.Empty();
					}
					else
					{
						// it's a beginmarker
						nextMkr = pDoc->GetWholeMarker(ptr);
					}
				}
				else
				{
					// must be text, so return nextMkr empty (it's already
					// empty, but doing it again documents the code better)
					nextMkr.Empty();
				}
			}
		} // end of else block for test: if (ptr == pEnd)
	} // end of else block for test: if (m_pApp->gCurrentSfmSet == PngOnly)
	return dataLen;
}

//A couple of useful member functions cloned from app's Convert16to8() Convert8to16()
CBString Xhtml::ToUtf8(const wxString& str)
{
	// converts UTF-16 strings to UTF-8
#ifdef _UNICODE
	wxCharBuffer tempBuf = str.mb_str(wxConvUTF8);
	return CBString(tempBuf);
#else
	return str;
#endif
}
wxString Xhtml::ToUtf16(CBString& bstr)
{
#ifdef _UNICODE
	wxWCharBuffer buf(wxConvUTF8.cMB2WC(bstr.GetBuffer()));
	if(!buf.data())
		return buf;
	return wxString(buf);
#else
	return bstr;
#endif
}

CBString Xhtml::GetExporterID()
{
	wxString myID = ::wxGetUserId();
	CBString s = ToUtf8(myID);
	ReplaceEntities(s);
	return s;
}
CBString Xhtml::GetLanguageCode()
{
	return ToUtf8(m_languageCode);
}
CBString Xhtml::GetDateTime()
{
	wxString date = GetDateTimeNow(forXHTML);
	return ToUtf8(date);
}
CBString Xhtml::GetTargetLanguageName()
{
	wxString langName = m_pApp->m_targetName;
	wxASSERT(!langName.IsEmpty());
	return ToUtf8(langName);
}
CBString Xhtml::GetRunningHeader(wxString* pBuffer)
{
	CBString emptyStr = "";
	wxString runHdrMkrPlusSpace = _T("\\h ");
	int offset = pBuffer->Find(runHdrMkrPlusSpace);
	if (offset == wxNOT_FOUND)
	{
		// try \h1 instead
		runHdrMkrPlusSpace = _T("\\h1 ");
		offset = pBuffer->Find(runHdrMkrPlusSpace);
		if (offset == wxNOT_FOUND)
		{
			return emptyStr;
		}
	}
	wxString right = pBuffer->Mid(offset + runHdrMkrPlusSpace.Len());
	int count = 0;
	int index;
	wxChar aChar;
	// step over any whitespace before start of running header text
	for (index = 0; index < 20; index++)
	{
		aChar = right[index];
		if (IsWhiteSpace(aChar))
		{
			count++;
		}
		else
		{
			break;
		}
	}
	if (count > 0)
	{
		right = right.Mid(count);
	}
	// we are now pointing at the start of the running header text, or if the marker has
	// no content, at the backslash of the next marker
	if (right[0] == _T('\\'))
	{
		return emptyStr;
	}
	else
	{
		// there is some running header text to collect - do so until next whitespace
		index = 0;
		bool bIsWhite;
		do {
			aChar = right[index];
			bIsWhite = FALSE;
			if (IsWhiteSpace(aChar))
			{
				bIsWhite = TRUE;
				break;
			}
			else
			{
				index++;
			}
		} while (!bIsWhite);
		wxString runningHdr = right.Left(index);
		// we don't expect to ever need to remove entities from a running header, so don't
		// call ReplaceEntities() on the string runningHdr after converting to CBString
		return ToUtf8(runningHdr);
	}
}

CBString Xhtml::GetMachineName()
{
	CBString emptyStr = "";
	wxString mname = ::wxGetHostName();
	if (mname.IsEmpty())
	{
		return emptyStr;
	}
	// to be safe, I'll replace entities here
	CBString s = ToUtf8(mname);
	ReplaceEntities(s);
	return s;
}

// This is a filtering function, used on the wxString of USFM marked up text data, to
// remove substrings (such as markers we don't want), or to change substrings (typically 
// markers we don't support and so they are to be replaced with other markers we do
// support)
wxString Xhtml::FindAndReplaceWith(wxString text, wxString searchStr, wxString replaceStr)
{
	wxString left; left.Empty();
	int length = searchStr.Len();
	int offset;
	offset = text.Find(searchStr);
	if (offset == wxNOT_FOUND)
	{
		return text;
	}
	do {
		offset = text.Find(searchStr);
		if (offset == wxNOT_FOUND)
		{
			left += text;
			return left;
		}
		else
		{
			left += text.Left(offset);
			text = text.Mid(offset + length);
			left += replaceStr;
		}
	} while (offset != wxNOT_FOUND);
	return left;
}

// converts to UTF-8 and does entity transformations (@ -> "&amp;" etc) 
CBString Xhtml::ConvertData(wxString data)
{
	CBString myData = ToUtf8(data);
	InsertEntities(myData);
	return myData;
}

// Pass in the m_data string from a parsing of \v and what follows, the m_data string will
// therefore start with the verse number, verse number bridge, or verse number with a part
// suffix, and so we use GetVerseString() to extract whatever is there up to the first
// whitespace character. This function will work right if there is no verse text, but only
// a verse number.
wxString Xhtml::GetVerseOrChapterString(wxString data)
{
	size_t length = data.Len();
	wxString numStr; numStr.Empty();
	size_t index;
	wxChar aChar;
	for (index = 0; index < length; index++)
	{
		aChar = data.GetChar(index);
		if (IsWhiteSpace(aChar))
		{
			return numStr;
		}
		else
		{
			numStr += aChar;
		}
	}
	return numStr;
}

///////////////////////////////////////////////////////////////////////////////
/// \return           the utf8 string which is the xhtml productions  for all
///                   the title markers (they bunch together) in the title
///                   part of the USFM text for a book; it can't be assumed that
///                   the first one will be a \mt or \mt1 due to natural language
///                   differences. TE processes \mt2 and \mt3 as not initiating
///                   any embedding, and it assumes a srcSection will commence where
///                   title info occurs, but we'll deal with the latter in the caller
/// \param pText  <-> ref to pointer to wxString holding the usfm text being
///                   parsed; when the function returns, pText will have had
///                   the parsed one or more \mt markers and their content
///                   removed from the string, and it will point at the next
///                   USFM marker to be processed
/// \remarks
/// On return, the caller should just append the contents of strTitlesProduction to
/// whatever is in xhtmlStr up to that point; the <div class="Title_Main> tag's PCDATA
/// acts as a container for all the <span> elements for types Title_Secondary and
/// Title_Tertiary, but there is no class attribute in the <span> which carries the PCDATA
/// for the \mt or \mt1 main title info. Strange way to do things, but I didn't dream this
/// up so I have to live with it. Caller will need to put the closing </div> at the start
/// of the next line.
/// On entry, ParseMarker_Content_Endmarker() will have parsed whatever is the first \mt*
/// marker (* is nothing, or 1 or 2 or 3), the content which follows, the endmarker will
/// be empty, and m_nextMkr will contain whatever beginMkr pText is pointing at on return.
/// The parsed marker will be in m_beginMkr, and the title's info in m_data. The function
/// must contain a loop for parsing over any additional markers of type \mt*
/////////////////////////////////////////////////////////////////////////////////
CBString Xhtml::BuildTitleInfo(wxString*& pText)
{
	CBString myxml;
	CBString out; out.Empty();
	myxml.Empty();
	// the start of the title info's xhtml is the same no matter what, so build that much
	out = "<div class=\"Title_Main\">";

	// get the tag enum value for this marker type (note: both \mt and \mt1 map to the one
	// enum value, make the production for whatever it is.
	m_whichTagEnum = (XhtmlTagEnum)(*m_pUsfm2IntMap)[m_beginMkr];
	if (m_whichTagEnum == title_main_)
	{
		myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
		out += myxml;
		myxml.Empty();
	}
	else if (m_whichTagEnum == title_secondary_)
	{
		myxml = BuildSpan(plusClass, title_secondary_, GetLanguageCode(), ConvertData(m_data));
		out += myxml;
		myxml.Empty();
	}
	else if (m_whichTagEnum == title_tertiary_)
	{
		myxml = BuildSpan(plusClass, title_tertiary_, GetLanguageCode(), ConvertData(m_data));
		out += myxml;
		myxml.Empty();
	}
	else
	{
		// if none of those, then what? Treat it as a main title - we've got to do
		// something that won't ruin the output
		myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
		out += myxml;
		myxml.Empty();
	}
	// pText is already pointing at the next marker, process the loop if it is one of the
	// \mt* ones
	m_beginMkr.Empty();
	m_endMkr.Empty();
	if (m_nextMkr == mtMkr || m_nextMkr == mt1Mkr || m_nextMkr == mt2Mkr || m_nextMkr == mt3Mkr)
	{
		do {
			m_nextMkr.Empty();
			m_spanLen = ParseMarker_Content_Endmarker((*pText), m_beginMkr, m_endMkr, m_data, m_nextMkr);
			// bleed out the parsed-over text
			(*pText) = (*pText).Mid(m_spanLen); // m_data has already been trimmed at both ends

			// get the tag enum value for this marker type
			m_whichTagEnum = (XhtmlTagEnum)(*m_pUsfm2IntMap)[m_beginMkr];


			// whichever of the \mt markers we come to first, process it and and any
			// others which follow it in the one function, and return only having
			// parsed beyond them all, and deal with them all
			// 
			if (m_whichTagEnum == title_main_) // for \mt or \mt1
			{
				myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
				out += myxml;
				myxml.Empty();
				m_beginMkr.Empty();
				m_endMkr.Empty();
			}
			else if (m_whichTagEnum == title_secondary_) // for \mt2
			{
				myxml = BuildSpan(plusClass, title_secondary_, GetLanguageCode(), ConvertData(m_data));
				out += myxml;
				myxml.Empty();
				m_beginMkr.Empty();
				m_endMkr.Empty();
			}
			else if (m_whichTagEnum ==  title_tertiary_) // for \mt3
			{
				myxml = BuildSpan(plusClass, title_tertiary_, GetLanguageCode(), ConvertData(m_data));
				out += myxml;
				myxml.Empty();
				m_beginMkr.Empty();
				m_endMkr.Empty();
			}
		// are we at the end of the \mt* markers?
		} while (m_nextMkr == mtMkr || m_nextMkr == mt1Mkr || m_nextMkr == mt2Mkr || m_nextMkr == mt3Mkr);
	}
#ifdef __WXDEBUG__
	wxString s = ToUtf16(out);
	wxLogDebug(_T("BuildTitleInfo():   %s"), s.c_str());
#endif
	return out;
}

// classtype can be footnote_, endnote_ or crossReference_; in the case of building an
// endnote, the Endnote_General_Paragraph production will be stored (as a wxString) in the
// member array m_endnoteDumpArray, and the list of such will be appended just before the
// closing set of </div> endtags. If the file contains more than one book, this dump must
// be done at the end of each book, and the array cleared to prepare for any endnotes
// which may be in the next book within the same file.
// Templates we use here:
// (1) occuring first: 
// m_spanTemplate[ftnoteMkrSpan]= "<span class=\"scrFootnoteMarker\"><a href=\"#idAttrUUID\"></a></span>";
// (2) occuring second; or in the case of endnotes, stored in order of occurrence in m_endnoteDumpArray
//  for dumping out at the end of the book:
// m_spanTemplate[nested] = "<span class=\"classAttrLabel\" id=\"idAttrUUID\" title=\"\"><span lang=\"langAttrCode\">myPCDATA</span></span>";
// Note: these templates don't support footnote-internal markers, just the commonly used
// ones, but they can very easily extended to add support for additional markers.
CBString Xhtml::BuildFXRefFe()
{
	CBString myxml;
	CBString out; out.Empty();
	myxml.Empty();
	CBString myUuid = MakeUUID();
	m_beginMkr.Empty();
	m_endMkr.Empty();

	// parse the information & remove it from m_pBuffer	
	m_nextMkr.Empty();

	m_spanLen = ParseFXRefFe((*m_pBuffer), m_beginMkr, m_endMkr, m_data, m_nextMkr);
	// bleed out the parsed-over text
	(*m_pBuffer) = (*m_pBuffer).Mid(m_spanLen); // m_data has already been trimmed at both ends

	// get the tag enum value for this marker type; will be one of footnote_ 
	// endnote_ or crossReference_
	m_whichTagEnum = (XhtmlTagEnum)(*m_pUsfm2IntMap)[m_beginMkr];
	wxASSERT(m_whichTagEnum == footnote_ || m_whichTagEnum == endnote_ || m_whichTagEnum == crossReference_);
	
	// builders to be used:
	// CBString Xhtml::BuildFootnoteMarkerSpan(CBString uuid)
	// CBString Xhtml::BuildNested(XhtmlTagEnum key, CBString uuid, CBString langCode, CBString pcData)
	// All three types of information take the same initial element, with the uuid in the
	// anchor tag
	out = BuildFootnoteMarkerSpan(myUuid); // also used for cross-references!
	// now build the secondProduction - it depends on what kind of data we are working with
	if (m_whichTagEnum == crossReference_)
	{
		myxml = BuildCrossReferenceParts(m_whichTagEnum, myUuid, m_data);
		out += myxml;
	}
	else
	{
		myxml = BuildFootnoteOrEndnoteParts(m_whichTagEnum, myUuid, m_data);
		// In the event we are handling an endnote, the endnote parts will have been strung
		// together and added as a line for dumping later, in m_endnoteDumpArray -- this is
		// done internally within BuildFootnoteOrEndnoteParts(); however, the srcFootnote part
		// of the production has been returned here (in myxml) for immediate placement
		if (!myxml.IsEmpty())
		{
			if (m_whichTagEnum == endnote_)
			{
                // the returned xhtml was for an endnote, so defer output to end of the
                // content for the book or whatever smaller chunk we are exporting
				m_endnoteDumpArray.Add(ToUtf16(myxml));
				// Note: the first part, however, will still be returned by out
			}
			else
			{
				// the returned xhtml was for a footnote, so it's for immediate placement
				out += myxml;
			}
		}
	}
	return out;
}

///////////////////////////////////////////////////////////////////////////////
/// \return                 the set of <span> elements, (in utf8) made from the passed in
///                         data string (the data string passed in is everything between
///                         the initial \x and the final \x*)
/// \param key              passing in crossReference_ enum value to the internally used
///                         utf8 template
/// \param uuid         ->  a unique string is required  (some software builds
///                         using something like CrossRef_LUK_a, CrossRef_LUK_b, etc; 
///                         we use uuids instead, as, I think, TE does)
/// \param data         ->  the footnote or endnote text  
/// \remarks
/// Call once per \x ... \x* style of cross reference, to collect all the relevant bits of the
/// content string (including for an \xo marker). Internally there is an outer
/// loop, and an inner loop that process text content for a given marker up to the start
/// of the next marker, or end of buffer. The present version supports the current TE
/// styles, but the specification for stylenames is not yet set in concrete, so this
/// function may get some minor changes added later.
/// The last span built herein has to be followed by a (second) closing </span> endtag.
/// If two markers occur in sequence, such as \xt followed by \it, then the \xt would have
/// no content - we have that generate an empty span, <span lang="xxx" /> where xxx is
/// whatever is the relevant 2 or 3-letter language code.
/// 
/// Note: the logic of this function may be extended a bit if someone gets the spec
/// set in concrete some day; we support \xt \xo \xk and \it. Rare markers not supported
/// here because TE has no styles set for them, so we just ignore such markers but keep
/// the text content which follows them.
////////////////////////////////////////////////////////////////////////////////////
CBString Xhtml::BuildCrossReferenceParts(XhtmlTagEnum key, CBString uuid, wxString data)
{
    // I'll not bother with deuterocanon, and some other less often used markers
	wxASSERT(!data.IsEmpty());
	// create wxString markers:
	wxString xtMarker  = _T("\\xt");
	wxString xoMarker  = _T("\\xo");
	wxString xkMarker  = _T("\\xk");
	wxString itMarker  = itMkr;
	// I'm using wxStringBuffer class. This will give me a writable pointer to the
	// original wxString's buffer, PROVIDED wxUSE_STL is 0 (which is the wxWidgets default
	// value) - if it is 1, then there will be a new EMPTY buffer created and no data
	// copied to it, and then this function would fail. (I've checked wxUSE_STL is 0, it
	// is in setup.h, line 207) Although buffer is writable, I'm using it as read-only.
	CBString aSpan; aSpan.Empty(); // use this for each <span lang="xyz"> ... </span> that we build
	CBString production; production.Empty(); // store the xhtml production being built in here
	wxString aSpace = _T(" ");
	data = ChangeWhitespaceToSingleSpace(data); // normalize, so there are no internal CR, LF, etc
	data.Trim(FALSE); // at beginning
	data.Trim(); // at end

	size_t buffLen = data.Len();
	wxStringBuffer myBuffer(data, buffLen);
	wxChar* pBuffStart = (wxChar*)myBuffer;
	wxChar* ptr = pBuffStart; // our parsing pointer
	wxChar* pEnd = ptr + buffLen;

	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	wxString collectStr; collectStr.Empty();
	int contentLen = 0;
	int itemLen = 0;
	wxChar* pContent = NULL; // use this for the start of span of text being parsed over

#if defined(__WXDEBUG__)
	//if (m_curChapter == _T("17") && m_curVerse == _T("35"))
//	if (m_curChapter == _T("1") && m_curVerse == _T("5"))
//	{
//		int breakpoint_here = 1;
//	}
#endif

	// get past white space, store the caller character, get past 
	// any whitespace which follows it
	while (IsWhiteSpace(*ptr) && ptr < pEnd) { ptr++;}
	// get the caller string -- we must set m_callerStr before we call 
	// BuildFootnoteInitialPart() which uses it (The "Footnote" in the name reflects the
	// fact that the xhtml uses srcFootnote style also for cross references, and the
	// initial part of both footnotes and cross references is therefore identical)
	m_callerStr = *ptr;
	// advance over the caller
	ptr++; // whether + or - or user-defined character
	
	// first, Build the initial part of the 2nd production, using template:
	// <span class=\"classAttrLabel\" id=\"idAttrUUID\" title=\"+\"> 
	// I use + as the caller in templates, but the following function will replace that
	// with whatever caller is in the data, which is not in the m_callerStr member
	production = BuildFootnoteInitialPart(key, uuid); // this will be followed by one or more
													  // simple <span> elements
	// advance over any following whitespace
	while (IsWhiteSpace(*ptr) && ptr < pEnd) { ptr++;}
	pContent = ptr; // content possibly starts here
	// If there is an xo field, ptr will now be pointing at the backslash of the \xo
	// marker - we always want this marker's ch:vs ref, but not the marker itself
	if (pDoc->IsMarker(ptr))
	{
		wxString aMarker = pDoc->GetWholeMarker(ptr);
		if (aMarker == xoMarker)
		{
			// jump it
			itemLen = xoMarker.Len();
			ptr += itemLen;
			itemLen = 0;
			// now advance to next marker; but there might be none, and we must get past
			// the ch:vs reference to the start of whatever text then follows
			itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
			itemLen = 0;
			// we are at where content must start to be collected, set the ptr
			pContent = ptr;
			contentLen = 0;
			// parse over the REF chapter & verse string (it may end in colon too)
			while (!IsWhiteSpace(*ptr) && ptr < pEnd) { ptr++; contentLen++;}
			// create the number string
			wxString s(pContent, (size_t)contentLen);
			contentLen = 0;
			// parse over any following white space
			itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
			itemLen = 0;
			// ptr is ready for kick-off of the next iteration of the
			// outer loop
			// Build the span for the verse number
			aSpan = BuildNoteTgtRefSpan(GetLanguageCode(), ConvertData(s));
			production += aSpan;
		}
		pContent = ptr; // default pContent to this location, which is where we may
						// want to start collecting from, adjust below if not so
	}
	// ******************   the outer loop starts here   ***************************

	do {
		pContent = ptr; // default pContent to this location, which is where we may
						// want to start collecting from, adjust below if not so

		// if ptr is not at a marker, advance to the next marker and collect all material
		// parsed over
		if (!pDoc->IsMarker(ptr))
		{
            // there shouldn't be any text at pStart, but rather a marker of some kind; but
            // just in case this assumption is not correct (the user may have declined to
            // use an \xt marker at the start of the cross reference text) we here collect
            // everything up to the next marker, or buffer end if there are no more markers
			wxASSERT(pContent != NULL);
			while (ptr < pEnd && !pDoc->IsMarker(ptr)) {ptr++; itemLen++; contentLen++;}
			wxString s(pContent, (size_t)(ptr - pContent));
			collectStr += s;
			collectStr.Trim(); // trim any space off of the end
			itemLen = 0;
			if (ptr == pEnd)
			{
				if (!s.IsEmpty())
				{
					// make the <span> - it's just a 'simple' one
					if (!collectStr.IsEmpty())
					{
						aSpan = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(collectStr));
						production += aSpan;
						aSpan.Empty();
					}
				}
				else
				{
					// make an empty span
					aSpan = BuildEmptySpan(GetLanguageCode());
					production += aSpan;
					aSpan.Empty();
				}
				break;
			}
			else
			{
				// we've halted at a marker after collecting some footnote text, so
				// iteration of the inner loop is appropriate now -- so prepare
				if (!s.IsEmpty())
				{
					// make the <span> - it's just a 'simple' one
					if (!collectStr.IsEmpty())
					{
						aSpan = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(collectStr));
						production += aSpan;
					}
				}
				pContent = NULL;
				aSpan.Empty();
				contentLen = 0;
				continue; // iterate immediately
			}
		} // end of TRUE block for test: if (!pDoc->IsMarker(ptr))

        // We are at a marker, or end of the x-ref. We count the size of the next marker,
        // but just omit it from the text collection: \xk (keyword), \xt (xref text), and
        // so forth; but we give the text content the appropriate xhtml style label. Some
        // of these markers, in the older USFM markup scheme, have endmarkers which
        // potentially may occur - we'll just parse over any endmarkers we encounter (we
        // count their size though, but we don't bother to check they match the begin
        // marker which precedes); the newer USFM standard doesn't use endmarkers within
        // cross references any more - which makes it safe to do this. Currently, only \xo,
        // \xt, \xk, or \it halt our parse and section the text into a series of <span> ..
        // </span> elements
		do {  // while ptr has not yet reached pEnd

			if (ptr < pEnd && pDoc->IsMarker(ptr))
			{
				wxString aMarker = pDoc->GetWholeMarker(ptr);

				// the USFM data may be marked up in the old style, using endmarkers like
				// \xt*, \xk* and so forth. If so, detect any such marker and
				// skip it, parse over any following whitespace, then iterate the loop
				if (pDoc->IsEndMarker(ptr, pEnd))
				{
					// parse over the endmarker
					itemLen = pDoc->ParseMarker(ptr);
					ptr += itemLen;
					itemLen = 0;
					// parse over following whitespace
					itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
					itemLen = 0;
					pContent = ptr;
					contentLen = 0;
					aMarker.Empty();
					continue; // iterate
				}

				if (aMarker == xtMarker || aMarker == xkMarker || aMarker == xoMarker
					|| aMarker == itMarker)
				{
                    // We've reached the end of a collection span, but we may have only
                    // just begun to collect - we don't expect control to get into the
					// next block, but if it does, just make a simple span of it before
					// continuing with handling the marker we are at and its content
					if (contentLen > 0)
					{
						// we do need to close off for this iteration, so make the <span>
						wxASSERT(pContent != NULL);
						wxString s(pContent, (size_t)(ptr - pContent));
						if (!collectStr.IsEmpty())
						{
							wxChar last = collectStr.GetChar(collectStr.Len() - 1);
							if (last != _T(' '))
							{
								// if collectStr doesn't end in a space, don't append the s
								// string until we've first appended a space 
								collectStr += aSpace;
							}
						}
						collectStr += s;
						collectStr.Trim(); // trim any space off of the end (keep control)

						// now make the <span> - it's just a 'simple' one, as we've no way yet
						// to determine otherwise
						aSpan = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(collectStr));
						production += aSpan;
						contentLen = 0;
						break;
					}
					else
					{
                        // There is no emitable content parsed over yet for this iteration
                        // (contentLen is 0) but ptr is pointing at either \xo or \xt or
                        // \xk or \it etc

						// this is where the footnote-internal markers, including \it for
						// italics (and I can add blocks for \bd etc if necessary) are
						// processed
						if (aMarker == xtMarker)
						{
							// we came to an \xt marker; so parse it's content and stop at
							// the next marker, or at end of the buffer, and build the span
							
							// parse over the \ft marker
							itemLen = pDoc->ParseMarker(ptr);
							ptr += itemLen;
							itemLen = 0;
							// parse over following whitespace
							itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
							itemLen = 0;
							// we are at where content must start to be collected, set the ptr
							pContent = ptr;
							contentLen = 0;
							// parse over the xref text (sub)string; note, an \it (italics)
							// marker may follow - and so the content collected is empty;
							// check for this and when that is the case, make an
							// emptySpan, otherwise make a simple span with content
							while (ptr < pEnd && !pDoc->IsMarker(ptr)) { ptr++; contentLen++;}
							if (contentLen == 0)
							{
								// a second marker follows (usually, italics \it marker),
								// so just make an empty span
								aSpan = BuildEmptySpan(GetLanguageCode());
								production += aSpan;
							}
							else
							{
								// create the string
								wxString s(pContent, (size_t)contentLen);
								contentLen = 0;
								// Build the span for the \xt marker -- this has no explicit style
								aSpan = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(s));
								production += aSpan;
							}
							break;
						}
						else if (aMarker == itMarker)
						{
							// we came to an \it marker; so parse it's content and stop at
							// the next marker, or at end of the buffer, and build the span...
							itemLen = pDoc->ParseMarker(ptr);
							ptr += itemLen;
							itemLen = 0;
							// parse over following whitespace
							itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
							itemLen = 0;
							// we are at where content must start to be collected, set the ptr
							pContent = ptr;
							contentLen = 0;
							// parse over the emphasis text string
							while (ptr < pEnd && !pDoc->IsMarker(ptr)) { ptr++; contentLen++;}
							// create the string
							wxString s(pContent, (size_t)contentLen);
							contentLen = 0;
							// Build the span for the \it marker's content
							aSpan = BuildSpan(plusClass, emphasis_, GetLanguageCode(), ConvertData(s));
							production += aSpan;
							break;
						}
						else if (aMarker == xoMarker)
						{
							// we came to an \xo marker; so parse it's content and stop at
							// the next marker, or at end of the buffer, and build the span
							itemLen = pDoc->ParseMarker(ptr);
							ptr += itemLen;
							itemLen = 0;
							// parse over following whitespace
							itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
							itemLen = 0;
							// we are at where content must start to be collected, set the ptr
							pContent = ptr;
							contentLen = 0;
							// parse over the verse number string
							while (!IsWhiteSpace(*ptr) && ptr < pEnd) { ptr++; contentLen++;}
							// create the number string
							wxString s(pContent, (size_t)contentLen);
							contentLen = 0;
							// parse over any following white space
							itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
							// ptr is ready for kick-off of the next iteration of the
							// outer loop
							// Build the span for the verse number
							aSpan = BuildSpan(plusClass, v_num_in_note_, GetLanguageCode(), ConvertData(s));
							production += aSpan;
							break;
						}
						else if(aMarker == xkMarker)
						{
							// we came to an \xk marker; so parse it's content and stop at
							// the next marker, or at end of the buffer, and build the span...
							itemLen = pDoc->ParseMarker(ptr);
							ptr += itemLen;
							itemLen = 0;
							// parse over following whitespace
							itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
							itemLen = 0;
							// we are at where content must start to be collected, set the ptr
							pContent = ptr;
							contentLen = 0;
							// parse over the quoted text string
							while (ptr < pEnd && !pDoc->IsMarker(ptr)) { ptr++; contentLen++;}
                            // get the content string (TE hasn't a style as far as I know, so
                            // I'll use Jim's one)
							wxString s(pContent, (size_t)contentLen);
							contentLen = 0;
							// Build the span for \xk markup
							aSpan = BuildSpan(plusClass, crossref_referenced_text_, 
												GetLanguageCode(), ConvertData(s));
							production += aSpan;
							break;
						}
						// note -- there may be more class attribute values -- use else if
						// blocks here to add code for processing them, and add a test for
						// each at the top of the block (other markers not in the test
						// will be processed by the else block below)
						// ***************** add more here ****************

					} // end of else block for test: if (contentLen > 0)

				} // end of TRUE block for test: if (aMarker == xtMarker || aMarker == xoMarker etc)
				else
				{
                    // ptr is pointing neither at none of the tested-for xref internal
                    // markers, but at some other marker; parse over the marker and ignore
                    // it, and go on until either we arrive at another marker (in which
                    // case we collect the data and apppend to collectStr & iterate the
                    // inner loop), or to the end of the buffer (in which case, do the same
                    // but then break out)
					itemLen = pDoc->ParseMarker(ptr);
					ptr += itemLen;
					itemLen = 0;
					// parse over following whitespace
					itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
					itemLen = 0;
					// we are at where content must start to be collected, set the ptr
					pContent = ptr;
					contentLen = 0;
					// parse over whatever text is next until either end of buffer, or a
					// marker is reached
					while (ptr < pEnd && !pDoc->IsMarker(ptr)) { ptr++; contentLen++;}
					// create the marker's content substring
					wxString s(pContent, (size_t)contentLen);
					contentLen = 0;
					if (ptr == pEnd)
					{
						// ptr is at the buffer end
						if (!collectStr.IsEmpty())
						{
							wxChar last = collectStr.GetChar(collectStr.Len() - 1);
							if (last != _T(' '))
							{
								// if collectStr doesn't end in a space, don't append the s
								// string until we've first appended a space 
								collectStr += aSpace;
							}
						}
						collectStr += s;
						collectStr.Trim();
						// now make the <span> - it's just a 'simple' one
						aSpan = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(collectStr));
						production += aSpan;
						break;
					}
					else
					{
						// ptr has reached the next marker, finish of this bit of text and
						// iterate the inner loop
						if (!collectStr.IsEmpty())
						{
							wxChar last = collectStr.GetChar(collectStr.Len() - 1);
							if (last != _T(' '))
							{
								// if collectStr doesn't end in a space, don't append the s
								// string until we've first appended a space 
								collectStr += aSpace;
							}
						}
						collectStr += s;
						// prepare for inner loop iteration
						pContent = NULL;
						aSpan.Empty();
						// ptr is ready for kick-off of the next iteration of the inner
						// loop - which we do now - provided ptr < pEnd, which will be
						// the case because we've come to another marker
					}
				} // end of else block for test: if (aMarker == xtMarker || aMarker == xoMarker)

			} // end of TRUE block for test: if (ptr < pEnd && pDoc->IsMarker(ptr))
			else
			{
				// we've reached the end of the cross ref
				wxASSERT(ptr == pEnd);
				if (contentLen > 0)
				{
					// add what is just parsed over to collectStr
					wxASSERT(pContent != NULL);
					wxString s(pContent, (size_t)(ptr - pContent));
					if (!collectStr.IsEmpty())
					{
						wxChar last = collectStr.GetChar(collectStr.Len() - 1);
						if (last != _T(' '))
						{
							// if collectStr doesn't end in a space, don't append the s
							// string until we've first appended a space 
							collectStr += aSpace;
						}
					}
					collectStr += s;
					collectStr.Trim(); // trim any space off of the end (keep control)

					// now make the <span> - it's just a 'simple' one
					aSpan = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(collectStr));
					production += aSpan;
				}
			} // end of else block for test: if (ptr < pEnd && pDoc->IsMarker(ptr))

		} while (ptr < pEnd); // end of <span>-producing loop

		// prepare for next iteration of the outer loop
		aSpan.Empty();
		collectStr.Empty();
		contentLen = 0;
		pContent = NULL;
	} while (ptr < pEnd); // end of outer loop

	// add the required final extra closing endspan
	production += "</span>";

	return production;
}

///////////////////////////////////////////////////////////////////////////////
/// \return                 the set of <span> elements, (in utf8) made from the passed in
///                         data string (the data string passed in is everything between
///                         the initial \f or \fe, and the final \f* or \fe*)
/// \param key              passing in footnote_ or endnote_ enum value to the internally
///                         used utf8 template
/// \param uuid         ->  a unique string is required in two places (some software builds
///                         using something like Footnote_LUK_a, Footnote_LUK_b, etc; we use
///                         uuids instead, as, I think, TE does)
/// \param data         ->  the footnote or endnote text  
/// \remarks
/// Call once per footnote or endnote to collect all the relevant bits of the
/// footnote content string (including for an \fr marker). Internally there is an outer
/// loop, and an inner loop that process text content for a given marker up to the start
/// of the next marker, or end of buffer. The present version supports the current TE
/// styles, but the specification for stylenames is not yet set in concrete, so this
/// function may get some minor changes added later.
/// The last span built herein has to be followed by a (second) closing </span> endtag.
/// If two markers occur in sequence, such as \ft followed by \it, then the \ft would have
/// no content - we have that generate an empty span, <span lang="xxx" /> where xxx is
/// whatever is the relevant 2 or 3-letter language code.
/// 
/// Note 1: the logic of this function may be extended a bit if someone gets the spec
/// set in concrete some day; we support \ft \fr \fv and \fq \fqa \fk and \it.
/// Note 2: In the case of endnotes, the series of spans are stored in a string array, and
/// dumped at the end of the xhtml to give a sort of 'endnote' behaviour. The uuids keep
/// the in-place information connected with the relevant production in the dumped endnote
/// productions at the end.
////////////////////////////////////////////////////////////////////////////////////
CBString Xhtml::BuildFootnoteOrEndnoteParts(XhtmlTagEnum key, CBString uuid, wxString data)
{
    // I'll not bother with deuterocanon, and some other less often markers, so won't
    // search for and remove \fdc ... \fdc*, etc, but just assume such info is not present
    // (if present, the markers will be removed but the text kept, as if it was \ft text)
	wxASSERT(!data.IsEmpty());
	// create wxString markers:
	wxString frMarker  = _T("\\fr");
	wxString fkMarker  = _T("\\fk");
	wxString fqMarker  = _T("\\fq");
	wxString fqaMarker = fqaMkr;
	wxString flMarker  = _T("\\fl");
	wxString fpMarker  = _T("\\fp");
	wxString fvMarker  = fvMkr;
	wxString ftMarker  = ftMkr;
	wxString itMarker  = itMkr;
	wxString fmMarker  = _T("\\fm"); // rare, we'll just omit it if 
									 // encountered, but keep the text
	// I'm using wxStringBuffer class. This will give me a writable pointer to the
	// original wxString's buffer, PROVIDED wxUSE_STL is 0 (which is the wxWidgets default
	// value) - if it is 1, then there will be a new EMPTY buffer created and no data
	// copied to it, and then this function would fail. (I've checked wxUSE_STL is 0, it
	// is in setup.h, line 207) Although buffer is writable, I'm using it as read-only.
	CBString aSpan; aSpan.Empty(); // use this for each <span lang="xyz"> ... </span> that we build
	CBString production; production.Empty(); // store the xhtml production being built in here
	wxString aSpace = _T(" ");
	data = ChangeWhitespaceToSingleSpace(data); // normalize, so there are no internal CR, LF, etc
	data.Trim(FALSE); // at beginning
	data.Trim(); // at end

	size_t buffLen = data.Len();
	wxStringBuffer myBuffer(data, buffLen);
	wxChar* pBuffStart = (wxChar*)myBuffer;
	wxChar* ptr = pBuffStart; // our parsing pointer
	wxChar* pEnd = ptr + buffLen;

	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	wxString collectStr; collectStr.Empty();
	int contentLen = 0;
	int itemLen = 0;
	wxChar* pContent = NULL; // use this for the start of span of text being parsed over

#if defined(__WXDEBUG__)
	//if (m_curChapter == _T("17") && m_curVerse == _T("35"))
//	if (m_curChapter == _T("1") && m_curVerse == _T("5"))
//	{
//		int breakpoint_here = 1;
//	}
#endif

	// get past white space, store the caller character, get past 
	// any whitespace which follows it
	while (IsWhiteSpace(*ptr) && ptr < pEnd) { ptr++;}
	// get the caller string -- we must set m_callerStr before we call 
	// BuildFootnoteInitialPart() which uses it
	m_callerStr = *ptr;
	// advance over the caller
	ptr++; // whether + or - or user-defined character
	
	// first, Build the initial part of the 2nd production, using template:
	// <span class=\"classAttrLabel\" id=\"idAttrUUID\" title=\"+\"> 
	// I use + as the caller in templates, but the following function will replace that
	// with whatever caller is in the data, which is not in the m_callerStr member
	production = BuildFootnoteInitialPart(key, uuid); // this will be followed by one or more
													  // simple <span> elements
	// advance over any following whitespace
	while (IsWhiteSpace(*ptr) && ptr < pEnd) { ptr++;}
	pContent = ptr; // content possibly starts here
	// If there is a \fr field, ptr will now be pointing at the backslash of the \fr
	// marker - we always want this marker's ch:vs ref, but not the marker itself
	if (pDoc->IsMarker(ptr))
	{
		wxString aMarker = pDoc->GetWholeMarker(ptr);
		if (aMarker == frMarker)
		{
			// jump it
			itemLen = frMarker.Len();
			ptr += itemLen;
			itemLen = 0;
			// now advance to next marker; but there might be none, and we must get past
			// the ch:vs reference to the start of whatever text then follows
			itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
			itemLen = 0;
			// we are at where content must start to be collected, set the ptr
			pContent = ptr;
			contentLen = 0;
			// parse over the REF chapter & verse string (it may end in colon too)
			while (!IsWhiteSpace(*ptr) && ptr < pEnd) { ptr++; contentLen++;}
			// create the number string
			wxString s(pContent, (size_t)contentLen);
			contentLen = 0;
			// parse over any following white space
			itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
			itemLen = 0;
			// ptr is ready for kick-off of the next iteration of the
			// outer loop
			// Build the span for the verse number
			aSpan = BuildNoteTgtRefSpan(GetLanguageCode(), ConvertData(s));
			production += aSpan;
		}
		pContent = ptr; // default pContent to this location, which is where we may
						// want to start collecting from, adjust below if not so
	}
	// ******************   the outer loop starts here   ***************************

	do {
		pContent = ptr; // default pContent to this location, which is where we may
						// want to start collecting from, adjust below if not so

		// if ptr is not at a marker, advance to the next marker and collect all material
		// parsed over, assuming it is footnote or endnote text
		if (!pDoc->IsMarker(ptr))
		{
            // there shouldn't be any text at pStart, but rather a marker of some kind; but
            // just in case this assumption is not correct (the user may have declined to
            // use an \ft marker at the start of the footnote text) we here collect
            // everything up to the next marker, or buffer end if there are no more markers
			wxASSERT(pContent != NULL);
			while (ptr < pEnd && !pDoc->IsMarker(ptr)) {ptr++; itemLen++; contentLen++;}
			wxString s(pContent, (size_t)(ptr - pContent));
			collectStr += s;
			collectStr.Trim(); // trim any space off of the end
			itemLen = 0;
			if (ptr == pEnd)
			{
				if (!s.IsEmpty())
				{
					// make the <span> - it's just a 'simple' one
					if (!collectStr.IsEmpty())
					{
						aSpan = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(collectStr));
						production += aSpan;
						aSpan.Empty();
					}
				}
				else
				{
					// make an empty span
					aSpan = BuildEmptySpan(GetLanguageCode());
					production += aSpan;
					aSpan.Empty();
				}
				break;
			}
			else
			{
				// we've halted at a marker after collecting some footnote text, so
				// iteration of the inner loop is appropriate now -- so prepare
				if (!s.IsEmpty())
				{
					// make the <span> - it's just a 'simple' one
					if (!collectStr.IsEmpty())
					{
						aSpan = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(collectStr));
						production += aSpan;
					}
				}
				pContent = NULL;
				aSpan.Empty();
				contentLen = 0;
				continue; // iterate immediately
			}
		} // end of TRUE block for test: if (!pDoc->IsMarker(ptr))

        // We are at a marker, or end of the footnote. We count the size of the next
        // marker, but just omit it from the text collection: \fk (keyword), \fq (footnote
        // translation quotation), \fl (footnote label -- probably rarely used), \fp
        // (footnote paragraph -- it's rare), and \ft (footnote text), and so forth; but we
        // give the text content the appropriate xhtml style label. Some of these markers,
        // in the older USFM markup scheme, have endmarkers which potentially may occur -
        // we'll just parse over any endmarkers we encounter (we count their size though,
        // but we don't bother to check they match the begin marker which precedes); the
        // newer USFM standard doesn't use endmarkers within footnotes any more - which
        // makes it safe to do this. Currently, only \fr, \fv, \ft, \fq, \fqa or \fk halt
        // our parse and section the text into a series of <span> .. </span> elements
		do {  // while ptr has not yet reached pEnd

			if (ptr < pEnd && pDoc->IsMarker(ptr))
			{
				wxString aMarker = pDoc->GetWholeMarker(ptr);

				// the USFM data may be marked up in the old style, using endmarkers like
				// \fq*, \fv*, \ft*, \fqa* and so forth. If so, detect any such marker and
				// skip it, parse over any following whitespace, then iterate the loop
				if (pDoc->IsEndMarker(ptr, pEnd))
				{
					// parse over the endmarker
					itemLen = pDoc->ParseMarker(ptr);
					ptr += itemLen;
					itemLen = 0;
					// parse over following whitespace
					itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
					itemLen = 0;
					pContent = ptr;
					contentLen = 0;
					aMarker.Empty();
					continue; // iterate
				}

				if (aMarker == ftMarker || aMarker == fqaMarker || aMarker == fvMarker
					|| aMarker == itMarker || aMarker == fqMarker || aMarker == fkMarker )
				{
                    // We've reached the end of a collection span, but we may have only
                    // just begun to collect - we don't expect control to get into the
					// next block, but if it does, just make a simple span of it before
					// continuing with handling the marker we are at and its content
					if (contentLen > 0)
					{
						// we do need to close off for this iteration, so make the <span>
						wxASSERT(pContent != NULL);
						wxString s(pContent, (size_t)(ptr - pContent));
						if (!collectStr.IsEmpty())
						{
							wxChar last = collectStr.GetChar(collectStr.Len() - 1);
							if (last != _T(' '))
							{
								// if collectStr doesn't end in a space, don't append the s
								// string until we've first appended a space 
								collectStr += aSpace;
							}
						}
						collectStr += s;
						collectStr.Trim(); // trim any space off of the end (keep control)

						// now make the <span> - it's just a 'simple' one, as we've no way yet
						// to determine otherwise
						aSpan = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(collectStr));
						production += aSpan;
						contentLen = 0;
						break;
					}
					else
					{
                        // There is no emitable content parsed over yet for this iteration
                        // (contentLen is 0) but ptr is pointing at either \fv or \fq or
                        // \fqa or \ft etc

						// this is where the footnote-internal markers, including \it for
						// italics (and I can add blocks for \bd etc if necessary) are
						// processed
						if (aMarker == ftMarker)
						{
							// we came to an \ft marker; so parse it's content and stop at
							// the next marker, or at end of the buffer, and build the span
							
							// parse over the \ft marker
							itemLen = pDoc->ParseMarker(ptr);
							ptr += itemLen;
							itemLen = 0;
							// parse over following whitespace
							itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
							itemLen = 0;
							// we are at where content must start to be collected, set the ptr
							pContent = ptr;
							contentLen = 0;
							// parse over the footnote text (sub)string; note, an \it (italics)
							// marker may follow - and so the content collected is empty;
							// check for this and when that is the case, make an
							// emptySpan, otherwise make a simple span with content
							while (ptr < pEnd && !pDoc->IsMarker(ptr)) { ptr++; contentLen++;}
							if (contentLen == 0)
							{
								// a second marker follows (usually, italics \it marker),
								// so just make an empty span
								aSpan = BuildEmptySpan(GetLanguageCode());
								production += aSpan;
							}
							else
							{
								// create the string
								wxString s(pContent, (size_t)contentLen);
								contentLen = 0;
								// Build the span for the \ft marker -- this has no explicit style
								aSpan = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(s));
								production += aSpan;
							}
							break;
						}
						else if (aMarker == itMarker)
						{
							// we came to an \it marker; so parse it's content and stop at
							// the next marker, or at end of the buffer, and build the span...
							itemLen = pDoc->ParseMarker(ptr);
							ptr += itemLen;
							itemLen = 0;
							// parse over following whitespace
							itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
							itemLen = 0;
							// we are at where content must start to be collected, set the ptr
							pContent = ptr;
							contentLen = 0;
							// parse over the emphasis text string
							while (ptr < pEnd && !pDoc->IsMarker(ptr)) { ptr++; contentLen++;}
							// create the string
							wxString s(pContent, (size_t)contentLen);
							contentLen = 0;
							// Build the span for the \it marker's content
							aSpan = BuildSpan(plusClass, emphasis_, GetLanguageCode(), ConvertData(s));
							production += aSpan;
							break;
						}
						else if (aMarker == fvMarker)
						{
							// we came to a \fv marker; so parse it's content and stop at
							// the next marker, or at end of the buffer, and build the span
							// parse over the \fv marker
							itemLen = pDoc->ParseMarker(ptr);
							ptr += itemLen;
							itemLen = 0;
							// parse over following whitespace
							itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
							itemLen = 0;
							// we are at where content must start to be collected, set the ptr
							pContent = ptr;
							contentLen = 0;
							// parse over the verse number string
							while (!IsWhiteSpace(*ptr) && ptr < pEnd) { ptr++; contentLen++;}
							// create the number string
							wxString s(pContent, (size_t)contentLen);
							contentLen = 0;
							// parse over any following white space
							itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
							// ptr is ready for kick-off of the next iteration of the
							// outer loop
							// Build the span for the verse number
							aSpan = BuildSpan(plusClass, v_num_in_note_, GetLanguageCode(), ConvertData(s));
							production += aSpan;
							break;
						}
						else if(aMarker == fqMarker)
						{
							// we came to an \fq marker; so parse it's content and stop at
							// the next marker, or at end of the buffer, and build the span...
							itemLen = pDoc->ParseMarker(ptr);
							ptr += itemLen;
							itemLen = 0;
							// parse over following whitespace
							itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
							itemLen = 0;
							// we are at where content must start to be collected, set the ptr
							pContent = ptr;
							contentLen = 0;
							// parse over the quoted text string
							while (ptr < pEnd && !pDoc->IsMarker(ptr)) { ptr++; contentLen++;}
                            // get the content string (TE uses Alternate_Reading for its
                            // style, which is certainly wrong; Jim has Footnote_Quotation
                            // but the TE people ignored his specification - so I'll just
                            // use Alternate_Reading for both \fqa and \fq until this
                            // matter is resolved)
							wxString s(pContent, (size_t)contentLen);
							contentLen = 0;
							// Build the span for \fq markup
							aSpan = BuildSpan(plusClass, alt_quote_, GetLanguageCode(), ConvertData(s));
							production += aSpan;
							break;
						}
						else if(aMarker == fqaMarker)
						{
							// we came to an \fqa marker; so parse it's content and stop at
							// the next marker, or at end of the buffer, and build the span...
							itemLen = pDoc->ParseMarker(ptr);
							ptr += itemLen;
							itemLen = 0;
							// parse over following whitespace
							itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
							itemLen = 0;
							// we are at where content must start to be collected, set the ptr
							pContent = ptr;
							contentLen = 0;
							// parse over the alterative quoted text string
							while (ptr < pEnd && !pDoc->IsMarker(ptr)) { ptr++; contentLen++;}
                            // get the content string (TE uses Alternate_Reading for its
                            // style, which may be wrong; Jim has Footnote_Alternate_Reading
                            // but the TE people ignored his specification - so I'll just
                            // use Alternate_Reading for both \fqa and \fq until this
                            // matter is resolved)
							wxString s(pContent, (size_t)contentLen);
							contentLen = 0;
							// Build the span for the \fqa alternate reading span
							aSpan = BuildSpan(plusClass, alt_quote_, GetLanguageCode(), ConvertData(s));
							production += aSpan;
							break;
						}
						else if(aMarker == fkMarker)
						{
							// we came to an \fk marker; so parse it's content and stop at
							// the next marker, or at end of the buffer, and build the span...
							itemLen = pDoc->ParseMarker(ptr);
							ptr += itemLen;
							itemLen = 0;
							// parse over following whitespace
							itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
							itemLen = 0;
							// we are at where content must start to be collected, set the ptr
							pContent = ptr;
							contentLen = 0;
							// parse over the quoted text string
							while (ptr < pEnd && !pDoc->IsMarker(ptr)) { ptr++; contentLen++;}
                            // get the content string (TE uses Alternate_Reading for its
                            // style, which is certainly wrong; Jim has Footnote_Quotation
                            // but the TE people ignored his specification - so I'll just
                            // use Alternate_Reading for both \fqa and \fq until this
                            // matter is resolved)
							wxString s(pContent, (size_t)contentLen);
							contentLen = 0;
							// Build the span for \fk markup
							aSpan = BuildSpan(plusClass, footnote_referenced_text_, 
												GetLanguageCode(), ConvertData(s));
							production += aSpan;
							break;
						}
						// note -- there may be more class attribute values -- use else if
						// blocks here to add code for processing them, and add a test for
						// each at the top of the block (other markers not in the test
						// will be processed by the else block below)
						// ***************** add more here ****************

					} // end of else block for test: if (contentLen > 0)

				} // end of TRUE block for test: if (aMarker == fvMarker || aMarker == fqaMarker etc)
				else
				{
                    // ptr is pointing neither at none of the tested-for footnote internal
                    // markers, but at some other marker; parse over the marker and ignore
                    // it, and go on until either we arrive at another marker (in which
                    // case we collect the data and apppend to collectStr & iterate the
                    // inner loop), or to the end of the buffer (in which case, do the same
                    // but then break out)
					itemLen = pDoc->ParseMarker(ptr);
					ptr += itemLen;
					itemLen = 0;
					// parse over following whitespace
					itemLen = pDoc->ParseOverAndIgnoreWhiteSpace(ptr, pEnd, 0); // updates ptr
					itemLen = 0;
					// we are at where content must start to be collected, set the ptr
					pContent = ptr;
					contentLen = 0;
					// parse over whatever text is next until either end of buffer, or a
					// marker is reached
					while (ptr < pEnd && !pDoc->IsMarker(ptr)) { ptr++; contentLen++;}
					// create the footnote text's content substring
					wxString s(pContent, (size_t)contentLen);
					contentLen = 0;
					if (ptr == pEnd)
					{
						// ptr is at the buffer end
						if (!collectStr.IsEmpty())
						{
							wxChar last = collectStr.GetChar(collectStr.Len() - 1);
							if (last != _T(' '))
							{
								// if collectStr doesn't end in a space, don't append the s
								// string until we've first appended a space 
								collectStr += aSpace;
							}
						}
						collectStr += s;
						collectStr.Trim();
						// now make the <span> - it's just a 'simple' one
						aSpan = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(collectStr));
						production += aSpan;
						break;
					}
					else
					{
						// ptr has reached the next marker, finish of this bit of text and
						// iterate the inner loop
						if (!collectStr.IsEmpty())
						{
							wxChar last = collectStr.GetChar(collectStr.Len() - 1);
							if (last != _T(' '))
							{
								// if collectStr doesn't end in a space, don't append the s
								// string until we've first appended a space 
								collectStr += aSpace;
							}
						}
						collectStr += s;
						// prepare for inner loop iteration
						pContent = NULL;
						aSpan.Empty();
						// ptr is ready for kick-off of the next iteration of the inner
						// loop - which we do now - provided ptr < pEnd, which will be
						// the case because we've come to another marker
					}
				} // end of else block for test: if (aMarker == fvMarker || aMarker == fqaMarker)

			} // end of TRUE block for test: if (ptr < pEnd && pDoc->IsMarker(ptr))
			else
			{
				// we've reached the end of the footnote
				wxASSERT(ptr == pEnd);
				if (contentLen > 0)
				{
					// add what is just parsed over to collectStr
					wxASSERT(pContent != NULL);
					wxString s(pContent, (size_t)(ptr - pContent));
					if (!collectStr.IsEmpty())
					{
						wxChar last = collectStr.GetChar(collectStr.Len() - 1);
						if (last != _T(' '))
						{
							// if collectStr doesn't end in a space, don't append the s
							// string until we've first appended a space 
							collectStr += aSpace;
						}
					}
					collectStr += s;
					collectStr.Trim(); // trim any space off of the end (keep control)

					// now make the <span> - it's just a 'simple' one
					aSpan = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(collectStr));
					production += aSpan;
				}
			} // end of else block for test: if (ptr < pEnd && pDoc->IsMarker(ptr))

		} while (ptr < pEnd); // end of <span>-producing loop

		// prepare for next iteration of the outer loop
		aSpan.Empty();
		collectStr.Empty();
		contentLen = 0;
		pContent = NULL;
	} while (ptr < pEnd); // end of outer loop

	// add the required final extra closing endspan
	production += "</span>";

	return production;
}

// return the xml data if successful, but an empty string if there was an error
CBString Xhtml::DoXhtmlExport(wxString& buff)
{
	m_nPictureNum = 0; // initialize, in case there are pictures (\fig ... \fig*) in the USFM
	CBString xhtmlStr; xhtmlStr.Empty(); // collect the xml productions here
	CBString myxml; myxml.Empty(); // use this for a scratch variable
	// 	myxml += m_eolStr;  use this whenever a newline is needed (only prior to a matched
	// 	closing </div> endtag, and even then, only when the opening <div> was at line start

	m_tabLevel = 0; // the xhtml should not have indenting, but if we do a "pretty indent" 
					// export for debugging purposes, we'll use this member
	m_pBuffer = &buff;
    // The current book's 3-letter code was obtained in the caller, and if not, then
    // DoXhtmlExport() would not have been called, so the code is now in the m_bookID member
    
	m_bWordsOfChrist = FALSE;
	m_bMajorSectionJustOpened = FALSE;

	wxString msg;
	msg = msg.Format(_T("Entered Xhtml component, building initial utf8 xhtml productions,\nfor file: %s in folder: %s"),
		m_myFilename.c_str(), m_myFilePath.c_str());
	m_pApp->LogUserAction(msg); // log where we are

	// more per-export initializations (this should not find any \free ... \free*
	// nor \note ... \note* markers, so each should be FALSE
	m_freeMkr = _T("\\free");
	m_noteMkr = _T("\\note");
	m_bContainsFreeTrans = (m_pBuffer->Find(m_freeMkr) != wxNOT_FOUND) ? TRUE : FALSE; 
	m_bContainsNotes = (m_pBuffer->Find(m_noteMkr) != wxNOT_FOUND) ? TRUE : FALSE;
	wxASSERT(!m_bContainsFreeTrans);
	wxASSERT(!m_bContainsNotes);

	// Do normalizations here.... replacing markers TE doesn't support (yet) with those we
	// do. The USFM may contain \x ... \x* crossReference material, but TE didn't support
	// it (presumably because inserting that stuff wasn't viewed as a scripture authoring
	// task), but Adapt It may be using source text which has been published, and may well
	// have such cross reference info in it. So we'll have to either remove it here
	// (unnecessarily complicated to do so) or when it is parsed below, simply refrain
	// from emitting any xhtml for that information type (that's what I do). TE does
	// support right-justified inline cross references - these have no internal structure,
	// and are wrapped in USFM by \rq ... \rq*, so we do support those in the xhtml.
	wxString text; // temporary, for the normalizations
	wxString emptyStr = _T("");
	text = *m_pBuffer;
	wxString pmoMkr = _T("\\pmo ");
	wxString pMkr = _T("\\p ");
	text = FindAndReplaceWith(text, pmoMkr, pMkr);
	wxString pmMkr = _T("\\pm ");
	text = FindAndReplaceWith(text, pmMkr, pMkr);
	wxString pmrMkr = _T("\\pmr ");
	text = FindAndReplaceWith(text, pmrMkr, pMkr);
	wxString pmcMkr = _T("\\pmc ");
	text = FindAndReplaceWith(text, pmrMkr, pMkr);
	wxString nbMkr = _T("\\nb "); // no break marker
	text = FindAndReplaceWith(text, nbMkr, pMkr);
	wxString miMkr = _T("\\mi ");
	wxString mMkr = _T("\\m ");
	text = FindAndReplaceWith(text, miMkr, mMkr);
	// for \nd 'name of deity' and its endmarker, we'll just replace with nothing
	wxString ndMkr = _T("\\nd ");
	wxString ndendMkr = _T("\\nd*");
	text = FindAndReplaceWith(text, ndMkr, emptyStr);
	text = FindAndReplaceWith(text, ndendMkr, emptyStr);
	// *** \d (descriptive title) and \mr (major section reference range) have no
	// support from TE as yet, and so would break our parse if not somehow supported.
	// Neither takes an endmarker, so the only thing from TE that I can see that would
	// give a reasonable result would be to use \qm1  (level 1 of a citation line)
	wxString dMkr = _T("\\nd ");
	wxString citationMkr = _T("\\qm1");
	text = FindAndReplaceWith(text, dMkr, citationMkr);
	wxString mrMkr = _T("\\mr");
	text = FindAndReplaceWith(text, mrMkr, citationMkr);
	// update m_pBuffer after normalizations are completed
	*m_pBuffer = text;
	text.Empty();

	CBString dateTimeNow = GetDateTime();
	m_exporterID = GetExporterID(); // uses user account name (ie. login name), returns CBString
	m_runningHdr = GetRunningHeader(m_pBuffer); // returns empty string if there is no \h
								// in the m_pBuffer data; else the running header CBString
	// Our USFM parser uses these private members for returning parsed information:
	// m_beginMkr, m_endMkr, m_data, m_nextMkr

	CBString myFilename = ToUtf8(m_myFilename); // lacks extension
	CBString filenameStr = myFilename + ".xhtml";
	CBString cssFilename = myFilename + ".css";
	CBString languageName = GetTargetLanguageName();
	CBString langCode = GetLanguageCode();
	// Build the descriptionStr to be passed to the m_metadataTemplate
	CBString descriptionStr;
	descriptionStr = languageName;
	descriptionStr += " exported from Adapt It by ";
	descriptionStr += m_exporterID + " on ";
	descriptionStr += dateTimeNow;
	// get the path in UTF-8
	CBString pathStr = ToUtf8(m_myFilePath);
	// m_metadataTemplate has the xhtml parameterized string for the productions from
	// initial <?xml down to <body class="scrBody"> & newline at end
	CBString searchStr;
	int length = 0;
	int offset = wxNOT_FOUND;
	CBString left;
	CBString metaStr = m_metadataTemplate; // do our changes on a copy thereof

	searchStr = "myCSSStyleSheet";
	length = searchStr.GetLength();
	offset = metaStr.Find(searchStr); wxASSERT(offset != wxNOT_FOUND);
	left = metaStr.Left(offset);
	myxml += left;
	myxml += cssFilename; // add the filename.css string
	metaStr = metaStr.Mid(offset + length); // bleed off what we've found

	searchStr = "myFilePath";
	length = searchStr.GetLength();
	offset = metaStr.Find(searchStr); wxASSERT(offset != wxNOT_FOUND);
	left = metaStr.Left(offset);
	myxml += left;
	myxml += pathStr; // add the path string (absolute path, no final folder separator)
	metaStr = metaStr.Mid(offset + length); // bleed off what we've found

	searchStr = "myDescription";
	length = searchStr.GetLength();
	offset = metaStr.Find(searchStr); wxASSERT(offset != wxNOT_FOUND);
	left = metaStr.Left(offset);
	myxml += left;
	myxml += descriptionStr; // add the decription information
	metaStr = metaStr.Mid(offset + length); // bleed off what we've found

	searchStr = "myFilename";
	length = searchStr.GetLength();
	offset = metaStr.Find(searchStr); wxASSERT(offset != wxNOT_FOUND);
	left = metaStr.Left(offset);
	myxml += left;
	myxml += filenameStr; // add the filename
	metaStr = metaStr.Mid(offset + length); // bleed off what we've found

	// append the rest & the loop to replace all NEWLINE substrings with the platform
	// native end-of-line character(s) (stored in CBString m_eolStr)
	myxml += metaStr;
	//now put myxml back into metaStr so we can again use myxml as a scratch variable
	metaStr = myxml; myxml.Empty();
	searchStr = "NEWLINE";
	length = searchStr.GetLength();
	do {
		offset = metaStr.Find(searchStr);
		if (offset != wxNOT_FOUND)
		{
			// we have found another instance of the search string "NEWLINE"
			left = metaStr.Left(offset);
			myxml += left;
			myxml += m_eolStr; // add a newline
			metaStr = metaStr.Mid(offset + length); // bleed off what we've found
		}
		else
		{
			// there are no more "NEWLINE" substrings to replace, so just add the rest and
			// exit the loop
			myxml += metaStr;
		}
	} while (offset != wxNOT_FOUND);

	
	// add what we've produced to the string to be returned to the caller
	wxLogDebug(_T("Top Productions:   %s"), (ToUtf16(myxml)).c_str());
	xhtmlStr += myxml;

	// scratch CBString local values for use in passing arguments; for the language code,
	// langCode is already set above
	//CBString classStr;

	// produce <div ... scrBook
	myxml = BuildDivTag(scrBook_);
	xhtmlStr += myxml;

	// produce the scrBookName production, we'll have to use whatever \h holds (or empty
	// string if \h was adapted -- it is by default filtered, so use must remember to
	// unfilter and adapt it
	myxml = BuildSpan(plusClass,scrBookName_,GetLanguageCode(),m_runningHdr);
	xhtmlStr += myxml;

	// produce the scrBookCode production
	myxml = BuildSpan(plusClass,scrBookCode_,GetLanguageCode(),ConvertData(m_bookID));
	xhtmlStr += myxml;

	// indicate that <div class="columns"> has not been entered yet into the file
	m_bFirstSectionHasBeenStarted = FALSE;
	// indicate no introduction section has yet commenced
	m_bFirstIntroSectionHasBeenStarted = FALSE;
	// indicate that no \li \i1 or \li2 'list item' has caused a newline to be opened
	//m_bListItemJustOpened = FALSE; <<-- not needed
	
	CBString myUuid;

	// Only open a new line before srcSection, Paragraph, and any of Line1,
	// Line2, Line3, and <body ..scrBody"> and <div ... srcBook"> and <div...columns">
	do {
		m_nextMkr.Empty();
		m_spanLen = ParseMarker_Content_Endmarker((*m_pBuffer), m_beginMkr, m_endMkr, 
													m_data, m_nextMkr);
		// bleed out the parsed-over text
		(*m_pBuffer) = (*m_pBuffer).Mid(m_spanLen); // m_data has already been trimmed at both ends

		// get the tag enum value for this marker type
		m_whichTagEnum = (XhtmlTagEnum)(*m_pUsfm2IntMap)[m_beginMkr];

		switch (m_whichTagEnum)
		{
		case no_value_: // this applies when text takes off again without an initial marker

			// first test must be to check if the m_endMkr member contains \wj*, if so we
			// must turn off m_bWordsOfChrist after building the span to be built here
			if (m_endMkr == wordsOfChristEndMkr)
			{
				myxml = BuildSpan(plusClass, words_of_christ_, GetLanguageCode(), ConvertData(m_data));
				xhtmlStr += myxml;
				myxml.Empty();
				m_bWordsOfChrist = FALSE;
			}
			if (m_bWordsOfChrist)
			{
				// the \wj marker may have been encountered earlier, and isn't turned off yet
				myxml = BuildSpan(plusClass, words_of_christ_, GetLanguageCode(), ConvertData(m_data));
				xhtmlStr += myxml;
				myxml.Empty();
			}
			else
			{
				// this will just be a simple span, language code, and the text
				myxml = BuildSpan(simple, v_, GetLanguageCode(), ConvertData(m_data));
				xhtmlStr += myxml;
				myxml.Empty();
			}
			// we give \f, \fe, or \x immediate attention - and parse them with a custom
			// parser, because they can't be handled right by our standard parser
			if (m_nextMkr == footnoteMkr || m_nextMkr == xrefMkr || m_nextMkr == endnoteMkr)
			{
				// we've a following footnote, cross reference or endnote, so parse over
				// them and do their productions in the following function
				myxml = BuildFXRefFe();
				if (!myxml.IsEmpty())
				{
					// footnote I'll put where it occurs, ditto for endnode but the first 
					// part wherever it occurs and the second part collected in a string array
					// for dumping at the end like Erik says to do; if \x ... \x* data was
					// in the passed in USFM text, BuildFXRefFe() will just return an
					// empty string - and in that case we emit no xhtml - effectively
					// filtering that unsupported data type out
					xhtmlStr += myxml;
				}
				myxml.Empty();
			}
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case id_:
			// do nothing, the bookID has been extracted before Xhtml::DoXhtmlExport() was entered
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case running_hdr_:
			// do nothing, the running header was extracted before Xhtml::DoXhtmlExport() was entered
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		// whichever of the \mt markers we come to first, process it and and any
		// others which follow it in the one function, and return only having
		// parsed beyond them all, and deal with them all
		case title_main_: // for \mt or \mt1
		case title_secondary_: // for \mt2
		case title_tertiary_: // for \mt3
			myxml = BuildTitleInfo(m_pBuffer);
			xhtmlStr += myxml;
			myxml.Empty(); // clear, for next production
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case c_: // the begin-marker was \c (chapter num production has been build already)
			// nothing to do, except indicate Introduction material is finished with
			if (m_bFirstIntroSectionHasBeenStarted)
			{
				// close the Intro_Section_Head container
				xhtmlStr += m_divCloseTemplate; // </div>
				m_bFirstIntroSectionHasBeenStarted = FALSE;
			}
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case p_: // normal paragraph (always opens a newline and closes off with </div> 
			     // before starting a new <div>
			m_bMajorSectionJustOpened = FALSE;
			if (!m_bFirstSectionHasBeenStarted) // got to a verse without any srcSection being done yet
			{
				// I'm gunna put <div class="columns"> on it's own line - I don't think a
				// newline there will be a problem when rendering
				xhtmlStr += m_eolStr; // open a new line
				xhtmlStr += m_divCloseTemplate; // </div>
				
				// do "columns" <div> now, and then a <div> for scrSection -- then make
				// the flag TRUE and it stays that way until the end of the export 
				myxml = BuildDivTag(columns_);
				xhtmlStr += myxml;
				myxml.Empty();

				// make a scrSection and Section_Head div tags, but the span for the text
				// will be an empty string
				myxml = BuildDivTag(scrSection_); // builds <div class="scrSection">, and is always
						// followed by a <div> of class Section_Head; major and minor likewise
						// have <div with class Section_Head Major or Section_Head_Minor but
						// do not nest and so don't require div of class scrSection is there is
						// already one such opening the section
				xhtmlStr += myxml;
				// add the section's header <div> tag, followed by the span with the text of
				// the subheading (a header doesn't start a new line)
				myxml = BuildDivTag(section_);
				xhtmlStr += myxml;
				myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_emptyStr));
				xhtmlStr += myxml;
				// from now on, to end of the book, this stays TRUE - there's always a
				// "current" section, and each is a container
				m_bFirstSectionHasBeenStarted = TRUE;
			}
			xhtmlStr += m_eolStr; // open a new line
			xhtmlStr += m_divCloseTemplate; // </div>
			// and don't forget that \p can have text after it, so check for m_data
			// non-empty and do a <span> of simple type when that is the case, & no verse
			// num before it
			myxml = BuildDivTag(p_);
			xhtmlStr += myxml;
			if (!m_data.IsEmpty())
			{
				// there is some verse text -- we are at a paragraph which divides a verse
				myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
				xhtmlStr += myxml;
			}
			myxml.Empty();
			// we give \f, \fe, or \x immediate attention - and parse them with a custom
			// parser, because they can't be handled right by our standard parser
			if (m_nextMkr == footnoteMkr || m_nextMkr == xrefMkr || m_nextMkr == endnoteMkr)
			{
				// we've a following footnote, cross reference or endnote, so parse over
				// them and do their productions in the following function
				myxml = BuildFXRefFe();
				if (!myxml.IsEmpty())
				{
					// footnote I'll put where it occurs, ditto for endnode but the first 
					// part wherever it occurs and the second part collected in a string array
					// for dumping at the end like Erik says to do; if \x ... \x* data was
					// in the passed in USFM text, BuildFXRefFe() will just return an
					// empty string - and in that case we emit no xhtml - effectively
					// filtering that unsupported data type out
					xhtmlStr += myxml;
				}
				myxml.Empty();
			}
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case m_: // continuation paragraph (I'm assuming these would open a new line)
			xhtmlStr += m_eolStr; // open a new line
			xhtmlStr += m_divCloseTemplate; // </div>
			// and don't forget that \p can have text after it, so check for m_data
			// non-empty and do a <span> of simple type when that is the case, & no verse
			// num before it
			myxml = BuildDivTag(m_);
			xhtmlStr += myxml;
			if (!m_data.IsEmpty())
			{
				// there is some verse text -- we are at a paragraph which divides a verse
				myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
				xhtmlStr += myxml;
			}
			myxml.Empty();
			// we give \f, \fe, or \x immediate attention - and parse them with a custom
			// parser, because they can't be handled right by our standard parser
			if (m_nextMkr == footnoteMkr || m_nextMkr == xrefMkr || m_nextMkr == endnoteMkr)
			{
				// we've a following footnote, cross reference or endnote, so parse over
				// them and do their productions in the following function
				myxml = BuildFXRefFe();
				if (!myxml.IsEmpty())
				{
					// footnote I'll put where it occurs, ditto for endnode but the first 
					// part wherever it occurs and the second part collected in a string array
					// for dumping at the end like Erik says to do; if \x ... \x* data was
					// in the passed in USFM text, BuildFXRefFe() will just return an
					// empty string - and in that case we emit no xhtml - effectively
					// filtering that unsupported data type out
					xhtmlStr += myxml;
				}
				myxml.Empty();
			}
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case intro_paragraph_: // inscription paragraph (always opens a newline and closes off with </div> 
			     // before starting a new <div>), it's usually centred, so USFM is \pc
			m_bMajorSectionJustOpened = FALSE;
			xhtmlStr += m_eolStr; // open a new line
			xhtmlStr += m_divCloseTemplate; // </div>
			// and don't forget that \ip can have text after it, so check for m_data
			// non-empty and do a <span> of simple type when that is the case, & no verse
			// num before it
			myxml = BuildDivTag(intro_paragraph_);
			xhtmlStr += myxml;
			if (!m_data.IsEmpty())
			{
				// there is some verse text -- we are at a paragraph which divides a verse
				myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
				xhtmlStr += myxml;
			}
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case intro_list_item_1_:
			myxml = BuildSpan(plusClass, intro_list_item_1_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case intro_list_item_2_:
			myxml = BuildSpan(plusClass, intro_list_item_2_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case line1_:
			xhtmlStr += m_eolStr; // open a new line
			xhtmlStr += m_divCloseTemplate; // </div>
			myxml = BuildDivTag(line1_);
			xhtmlStr += myxml;
			if (m_data.IsEmpty())
			{
				// probably another marker followed \q1 or \q, perhaps \wj or \qt, so the
				// case for those can handle the relevant span type
				;
			}
			else
			{
                // There's data to be handled after the \q or \q1, maybe Words_Of_Christ
                // was turned on earlier, so check for that, otherwise it's just a 'simple'
                // span type
                if (m_bWordsOfChrist)
				{
					// there was a \wj encountered preceding a previous span 
					myxml = BuildSpan(plusClass, words_of_christ_, GetLanguageCode(), ConvertData(m_data));
					// check if this span ends with a closing \wj* endmarker, and if so,
					// turn off the flag
					if (m_endMkr == wordsOfChristEndMkr)
					{
						m_bWordsOfChrist = FALSE;
					}
				}
				else
				{
					// "Words_Of_Christ" is not current, so only a simple span is required
					myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
				}
			}
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case line2_:
			xhtmlStr += m_eolStr; // open a new line
			xhtmlStr += m_divCloseTemplate; // </div>
			myxml = BuildDivTag(line2_);
			xhtmlStr += myxml;
			if (m_data.IsEmpty())
			{
				// probably another marker followed \q1 or \q, perhaps \wj or \qt, so the
				// case for those can handle the relevant span type
				;
			}
			else
			{
                // There's data to be handled after the \q or \q1, maybe Words_Of_Christ
                // was turned on earlier, so check for that, otherwise it's just a 'simple'
                // span type
                if (m_bWordsOfChrist)
				{
					// there was a \wj encountered preceding a previous span 
					myxml = BuildSpan(plusClass, words_of_christ_, GetLanguageCode(), ConvertData(m_data));
					// check if this span ends with a closing \wj* endmarker, and if so,
					// turn off the flag
					if (m_endMkr == wordsOfChristEndMkr)
					{
						m_bWordsOfChrist = FALSE;
					}
				}
				else
				{
					// "Words_Of_Christ" is not current, so only a simple span is required
					myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
				}
			}
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case line3_:
			xhtmlStr += m_eolStr; // open a new line
			xhtmlStr += m_divCloseTemplate; // </div>
			myxml = BuildDivTag(line3_);
			xhtmlStr += myxml;
			if (m_data.IsEmpty())
			{
				// probably another marker followed \q1 or \q, perhaps \wj or \qt, so the
				// case for those can handle the relevant span type
				;
			}
			else
			{
                // There's data to be handled after the \q or \q1, maybe Words_Of_Christ
                // was turned on earlier, so check for that, otherwise it's just a 'simple'
                // span type
                if (m_bWordsOfChrist)
				{
					// there was a \wj encountered preceding a previous span 
					myxml = BuildSpan(plusClass, words_of_christ_, GetLanguageCode(), ConvertData(m_data));
					// check if this span ends with a closing \wj* endmarker, and if so,
					// turn off the flag
					if (m_endMkr == wordsOfChristEndMkr)
					{
						m_bWordsOfChrist = FALSE;
					}
				}
				else
				{
					// "Words_Of_Christ" is not current, so only a simple span is required
					myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
				}
			}
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case quoted_text_:
			myxml = BuildSpan(plusClass, quoted_text_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			// THERE IS AN IMPLICIT ASSUMPTION HERE -- that \wj (Words_Of_Christ) and \qt
			// (Quoted_Text) never occur together. This seems to be a reasonable
			// assumption on the basis of scripture. (If they did occur together, we've no
			// way to mark them in this xhtml standard, as the class attribute in a span
			// can only be Words_Of_Christ or Quoted_Text, not both.)
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case inscription_paragraph_: // inscription paragraph (always opens a newline and closes off with </div> 
			     // before starting a new <div>), it's usually centred, so USFM is \pc
			m_bMajorSectionJustOpened = FALSE;
			xhtmlStr += m_eolStr; // open a new line
			xhtmlStr += m_divCloseTemplate; // </div>
			// and don't forget that \p can have text after it, so check for m_data
			// non-empty and do a <span> of simple type when that is the case, & no verse
			// num before it
			myxml = BuildDivTag(inscription_paragraph_);
			xhtmlStr += myxml;
			if (!m_data.IsEmpty())
			{
				// there is some verse text -- we are at a paragraph which divides a verse
				myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
				xhtmlStr += myxml;
			}
			myxml.Empty();
			// we give \f, \fe, or \x immediate attention - and parse them with a custom
			// parser, because they can't be handled right by our standard parser
			if (m_nextMkr == footnoteMkr || m_nextMkr == xrefMkr || m_nextMkr == endnoteMkr)
			{
				// we've a following footnote, cross reference or endnote, so parse over
				// them and do their productions in the following function
				myxml = BuildFXRefFe();
				if (!myxml.IsEmpty())
				{
					// footnote I'll put where it occurs, ditto for endnode but the first 
					// part wherever it occurs and the second part collected in a string array
					// for dumping at the end like Erik says to do; if \x ... \x* data was
					// in the passed in USFM text, BuildFXRefFe() will just return an
					// empty string - and in that case we emit no xhtml - effectively
					// filtering that unsupported data type out
					xhtmlStr += myxml;
				}
				myxml.Empty();
			}
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case inscription_:
			myxml = BuildSpan(plusClass, inscription_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case inline_crossReference_: // for USFM \rq ... \rq*  [but handle like in BuildFXRefFe() ]
			myUuid = MakeUUID();
			myxml = BuildFootnoteMarkerSpan(myUuid);
			xhtmlStr += myxml;
			myxml = BuildNested(m_whichTagEnum, myUuid, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case v_: // the begin-marker was \v (verse num production has been build already)
			m_bMajorSectionJustOpened = FALSE;
			if (!m_bFirstSectionHasBeenStarted) // got to a verse without any srcSection being done yet
			{
				// I'm gunna put <div class="columns"> on it's own line - I don't think a
				// newline there will be a problem when rendering
				xhtmlStr += m_eolStr; // open a new line
				xhtmlStr += m_divCloseTemplate; // </div>
				
				// do "columns" <div> now, and then a <div> for scrSection -- then make
				// the flag TRUE and it stays that way until the end of the export 
				myxml = BuildDivTag(columns_);
				xhtmlStr += myxml;
				myxml.Empty();

				// make a scrSection and Section_Head div tags, but the span for the text
				// will be an empty string
				myxml = BuildDivTag(scrSection_); // builds <div class="scrSection">, and is always
						// followed by a <div> of class Section_Head; major and minor likewise
						// have <div with class Section_Head Major or Section_Head_Minor but
						// do not nest and so don't require div of class scrSection is there is
						// already one such opening the section
				xhtmlStr += myxml;
				// add the section's header <div> tag, followed by the span with the text of
				// the subheading (a header doesn't start a new line)
				myxml = BuildDivTag(section_);
				xhtmlStr += myxml;
				myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_emptyStr));
				xhtmlStr += myxml;
				// from now on, to end of the book, this stays TRUE - there's always a
				// "current" section, and each is a container
				m_bFirstSectionHasBeenStarted = TRUE;
			}
			if (!m_chapterNumSpan.IsEmpty())
			{
				xhtmlStr += m_chapterNumSpan;
				m_chapterNumSpan.Empty();
			}
			if (!m_verseNumSpan.IsEmpty())
			{
				xhtmlStr += m_verseNumSpan;
				m_verseNumSpan.Empty();
			}
			if (!m_data.IsEmpty())
			{
				// there's data to be handled -- we have to check if the \wj marker was
				// encountered at some earlier time and it's flag is still turned on
				if (m_bWordsOfChrist)
				{
					// this span will have class="Words_Of_Christ",the language code, and
					// the text
					myxml = BuildSpan(plusClass, words_of_christ_, GetLanguageCode(), ConvertData(m_data));
					xhtmlStr += myxml;
					myxml.Empty();
				}
				else
				{
					// this will just be a simple span, language code, and the text
					myxml = BuildSpan(simple, v_, GetLanguageCode(), ConvertData(m_data));
					xhtmlStr += myxml;
					myxml.Empty();
				}
			}
			else
			{
				// no data, this could happen if \wj follows a verse marker; anyway, if
				// there's no data, don't make a <span> and just have the parser called
				;
			}
			// we give \f, \fe, or \x immediate attention - and parse them with a custom
			// parser, because they can't be handled right by our standard parser
			if (m_nextMkr == footnoteMkr || m_nextMkr == xrefMkr || m_nextMkr == endnoteMkr)
			{
				// we've a following footnote, cross reference or endnote, so parse over
				// them and do their productions in the following function
				myxml = BuildFXRefFe();
				if (!myxml.IsEmpty())
				{
					// footnote I'll put where it occurs, ditto for endnode but the first 
					// part wherever it occurs and the second part collected in a string array
					// for dumping at the end like Erik says to do; if \x ... \x* data was
					// in the passed in USFM text, BuildFXRefFe() will just return an
					// empty string - and in that case we emit no xhtml - effectively
					// filtering that unsupported data type out
					xhtmlStr += myxml;
				}
				myxml.Empty();
			}
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case major_section_:
			m_bMajorSectionJustOpened = TRUE; // this prevents a following \s or \s1 from
											  // trying to (wrongly) open the section a second time
			// major sections always start a new line, and close off the last <div> tag with </div>
			xhtmlStr += m_eolStr; // open a new line
			xhtmlStr += m_divCloseTemplate; // </div>
			// sections are containers, and so if a section was already opened, then this
			// new section most close it with an extra </div> now
			if (m_bFirstSectionHasBeenStarted)
			{
				xhtmlStr += m_divCloseTemplate; // </div>
			}
			myxml = BuildDivTag(scrSection_); // builds <div class="scrSection">, and is always
                    // followed by a <div> of class Section_Head_Major; minor likewise has
                    // <div with class Section_Head_Minor but does not nest and so doesn't
                    // require div of class scrSection as there is already one such opening
                    // the section; and a norm Section_Head, if it follows, doesn't start
                    // another <div class="scrSection"> because the major section marker
                    // has opened the section, and in that case a following normal section
                    // just builds it's Section_Head and following text span
			m_bFirstSectionHasBeenStarted = TRUE; // is only false at start of parse, and is
					// used for forcing a section to start if one hasn't started by the time
					// a verse marker is encountered
			xhtmlStr += myxml;
			// add the section's header <div> tag, followed by the span with the text of
			// the subheading (a header doesn't start a new line)
			myxml = BuildDivTag(major_section_);
			xhtmlStr += myxml;
			myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case section_:
			// sections always start a new line, and close off the last <div> tag with </div>
			xhtmlStr += m_eolStr; // open a new line
			xhtmlStr += m_divCloseTemplate; // </div>
			// sections are containers, and so if a section was already opened, then this
			// new section most close it with an extra </div> now (but only if a preceding major
			// section has not already done so when opening the current section)
			if (m_bFirstSectionHasBeenStarted && !m_bMajorSectionJustOpened)
			{
				xhtmlStr += m_divCloseTemplate; // </div>
			}
			m_bFirstSectionHasBeenStarted = TRUE; // is only false at start of parse, and is
					// used for forcing a section to start if one hasn't started by the time
					// a verse marker is encountered
			if (!m_bMajorSectionJustOpened)
			{
				// don't do this if a major section, \ms or \ms1, has already made it happen
				myxml = BuildDivTag(scrSection_); // builds <div class="scrSection">, and is always
						// followed by a <div> of class Section_Head; major and minor likewise
						// have <div with class Section_Head Major or Section_Head_Minor but
						// do not nest and so don't require div of class scrSection is there is
						// already one such opening the section
				xhtmlStr += myxml;
				m_bMajorSectionJustOpened = FALSE; // we've prevented a wrong second srcSection
						// from being created, and so this flag can be restored to it's
						// default value, to be ready if there should happen to be more 
						// major sections later somewhere
			}

			// add the section's header <div> tag, followed by the span with the text of
			// the subheading (a header doesn't start a new line)
			myxml = BuildDivTag(section_);
			xhtmlStr += myxml;
			myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case minor_section_:
			// minor sections don't cause a <div class="scrSection"> to be created, but
			// instead they just have a Section_Head_Minor and span with text following;
			// they do start a new line, but do not close off the current section
			xhtmlStr += m_eolStr; // open a new line
			myxml = BuildDivTag(minor_section_);
			xhtmlStr += myxml;
			myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case intro_section_:
			// sections always start a new line, and close off the last <div> tag with </div>
			xhtmlStr += m_eolStr; // open a new line
			xhtmlStr += m_divCloseTemplate; // </div>
            // intro sections are containers, and so if an intro section was already
            // opened, then this new intro section most close it with an extra </div> now
			if (m_bFirstIntroSectionHasBeenStarted)
			{
				xhtmlStr += m_divCloseTemplate; // </div>
			}
			m_bFirstIntroSectionHasBeenStarted = TRUE; // is only false at start of parse
					// of any introductory material, and is used for forcing an extra
					// </div> to be emitted when a new intro section is encountered - the
					// flag stays TRUE only until a \c is encountered
			myxml = BuildDivTag(scrIntroSection_); // builds <div class="scrIntroSection">,
					// and is always followed by a <div> of class Intro_Section_Head
			xhtmlStr += myxml;
			
			// add the intro section's header <div> tag, followed by the span with the
			// text of the introduction subheading (a header doesn't start a new line)
			// Note, the \is or \is1 may, or may not, have no text - if none, the element
			// should still be produced, but with no PCDATA
			myxml = BuildDivTag(intro_section_); // builds <div class="Intro_Section_Head">
			xhtmlStr += myxml;
			myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case list_item_1_:
			xhtmlStr += m_eolStr; // open a new line
			myxml = BuildDivTag(list_item_1_);
			xhtmlStr += myxml;
			myxml = BuildSpan(simple, no_value_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case alt_quote_: // for USFM \fqa 
			// I'm assuming USFM \fqa maps to TE's Alternate_Reading" -- see Oxes v1 documentation
			myxml = BuildSpan(plusClass, alt_quote_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case footnote_quote_: // for USFM \fq
			// Greg Trihus says USFM \fq maps to TE's "Alternate_Reading"
			myxml = BuildSpan(plusClass, footnote_quote_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case v_num_in_note_: // for USFM \fv
			myxml = BuildSpan(plusClass, v_num_in_note_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case chapter_head_: // for USFM \cl 'chapter label' (Oxes documentation wrongly gives this as \cp). Do 'in place', it can be before or after \c
			myxml = BuildSpan(plusClass, chapter_head_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case parallel_passage_ref_:
			myxml = BuildSpan(plusClass, parallel_passage_ref_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case words_of_christ_:
			m_bWordsOfChrist = TRUE;
			myxml = BuildSpan(plusClass, words_of_christ_, GetLanguageCode(), ConvertData(m_data));
			if (m_endMkr == wordsOfChristEndMkr)
			{
				m_bWordsOfChrist = FALSE;
			}
			else
			{
				// the "Words_Of_Christ" \wj* marker did not end off this span, so the
				// boolean stays open in case further words from Jesus follow
				;
			}
			if (m_nextMkr == footnoteMkr || m_nextMkr == xrefMkr || m_nextMkr == endnoteMkr)
			{
				// we've a following footnote, cross reference or endnote, so parse over
				// them and do their productions in the following function
				myxml = BuildFXRefFe();
				if (!myxml.IsEmpty())
				{
					// footnote I'll put where it occurs, ditto for endnode but the first 
					// part wherever it occurs and the second part collected in a string array
					// for dumping at the end like Erik says to do; if \x ... \x* data was
					// in the passed in USFM text, BuildFXRefFe() will just return an
					// empty string - and in that case we emit no xhtml - effectively
					// filtering that unsupported data type out
					xhtmlStr += myxml;
				}
				myxml.Empty();
			}
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case citation_line_1_:
			myxml = BuildSpan(plusClass, citation_line_1_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
            // THERE IS AN IMPLICIT ASSUMPTION HERE -- that \wj (Words_Of_Christ) and \qt
            // (Quoted_Text) and \sc# ... \sc* (Citation_Line#) never occur together. This
            // seems to be a reasonable assumption on the basis of scripture. (If they did
            // occur together, we've no way to mark them in this xhtml standard, as the
            // class attribute in a span can only be Words_Of_Christ or Quoted_Text, or
            // (Citation_Line#) not any two of them.)
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case citation_line_2_:
			myxml = BuildSpan(plusClass, citation_line_2_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
            // THERE IS AN IMPLICIT ASSUMPTION HERE -- that \wj (Words_Of_Christ) and \qt
            // (Quoted_Text) and \sc# ... \sc* (Citation_Line#) never occur together. This
            // seems to be a reasonable assumption on the basis of scripture. (If they did
            // occur together, we've no way to mark them in this xhtml standard, as the
            // class attribute in a span can only be Words_Of_Christ or Quoted_Text, or
            // (Citation_Line#) not any two of them.)
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case citation_line_3_:
			myxml = BuildSpan(plusClass, citation_line_3_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
            // THERE IS AN IMPLICIT ASSUMPTION HERE -- that \wj (Words_Of_Christ) and \qt
            // (Quoted_Text) and \sc# ... \sc* (Citation_Line#) never occur together. This
            // seems to be a reasonable assumption on the basis of scripture. (If they did
            // occur together, we've no way to mark them in this xhtml standard, as the
            // class attribute in a span can only be Words_Of_Christ or Quoted_Text, or
            // (Citation_Line#) not any two of them.)
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case emphasis_:
			// we are assuming here that \it ... \it* will apply only to a short stretch
			// of text in which there were no other markers - if there were, the
			// emphasized span would only apply up to the begin-marker of the embedded
			// markers, which would be a markup glitch, but not break anything in the
			// processing. (In particular, assume no embedded footnote, no embedded \wj
			// ... \wj* subspan.)
			myxml = BuildSpan(plusClass, emphasis_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case emphasized_text_:
			// we are assuming here that \em ... \em* will apply only to a short stretch
			// of text in which there were no other markers - if there were, the
			// emphasized span would only apply up to the begin-marker of the embedded
			// markers, which would be a markup glitch, but not break anything in the
			// processing. (In particular, assume no embedded footnote, no embedded \wj
			// ... \wj* subspan.)
			myxml = BuildSpan(plusClass, emphasized_text_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case remark_:
			myxml = BuildSpan(plusClass, remark_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case stanza_break_:
			myxml = BuildSpan(plusClass, stanza_break_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case see_glossary_:
			myxml = BuildSpan(plusClass, see_glossary_, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		case figure_:
			// increase the picture number (1-based)
			m_nPictureNum++;
			// construct the picture ID (it's unique, of form "Figure-XXX-number" where
			// XXX is the bookID code, eg. LUK, or REV, etc
			m_strPictureID = ConstructPictureID(m_bookID, m_nPictureNum);
			// Build the productions for a picture within the text
			myxml = BuildPictureProductions(m_strPictureID, GetLanguageCode(), ConvertData(m_data));
			xhtmlStr += myxml;
			myxml.Empty();
			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;


// TODO?  more cases go here ***************************************

		default:

			{
			// unsupported markers will go thru here, and do nothing -- but we can give a wxLogDebug()
			// to warn me
			wxString msg;
			msg = msg.Format(_T(" *** Not yet supported Marker (as yet):  %s    ***"), m_beginMkr.c_str());
			wxLogDebug(msg);

			// put something in the text too
			CBString unsupportedStr;
			wxString txtMsg;
			txtMsg = txtMsg.Format(_T("<!-- ******** Not yet supported:  %s  ******** -->"),m_beginMkr.c_str());
			unsupportedStr = ToUtf8(txtMsg);
			xhtmlStr += m_eolStr;
			xhtmlStr += unsupportedStr;
			//xhtmlStr += m_divCloseTemplate;
			}

			m_beginMkr.Empty();
			m_endMkr.Empty();
			break;
		}
		
		// Are we at the end of the book?
        // Our present code will not support multiple books in the one export. If we want
        // to do that, we'll have to close off a book here and start another - and extra
        // code would be needed here for that

	// continue looping so long as something remains in m_pBuffer to be parsed over
	} while (!m_pBuffer->IsEmpty());

	// trim any final spaces; do the endnote dump, and add the final closing tags
	xhtmlStr += FinishOff(3); // should be 3 </div> tags added, then closure for body and html

	return xhtmlStr;
}

CBString Xhtml::ConstructPictureID(wxString bookID, int nPictureNum)
{
	CBString id = "Figure-";
	id += ConvertData(bookID);
	id += '-';
	// whm modified 23Jul12 to use wxItoa() (in helpers)
	CBString numStr; //char numStr[24];
	wxItoa(nPictureNum, numStr); //itoa(nPictureNum, numStr, 10);
	id += numStr;
	return id;
}

// NOTE: BEW created 7Jun12 -- to get the Sena xhtml pretty-printed, or at least <div>
// indented (if XHTML_PRETTY is not #defined). However, this would be a useful function for
// my xhtml export. I can do that export without bothering about indending, and just append
// a m_eolStr to the xhtml text already built when about to add a </div> endtag, to get the
// kind of output Erik sent me for Sena 3.xhtml. Then, if whitespace next to <div> or
// </div> is kosher, I can do run this code on the file and resave, to get a more human
// readable result. (For debugging, use the XHTML_PRETTY #define, and then each <span>
// will be lined up under the last one -- but remember, only when debugging in order to see
// the structure.)
// By using #defines, I can purpose this function for different kinds of analysis of the xhtml
void Xhtml::Indent_Etc_XHTML() // pinched from my commented out encoding converter code 
						 // in OnInit() and repurposed
{
	// BEW 7Jun2012 **** DO NOT REMOVE CODE BELOW **** THIS IS VALUABLE FOR BEW ****
	//
	// Some code to get the sequence of utf-8 bytes for a given utf-16 character
	// in the output window -- run this in the Unicode build, it's useless in the ANSI build
	// It's just a standard file in dialog to get a file, and a standard file out
	// dialog to write to a given folder, and code for doing the indenting in between
	// Input a file of xltml (it's utf8) data and do indents and write to disk

	wxString fullPath = _T("");
	wxString indenter_message = _T("Select XHTML file for indenting");
	wxString default_path = _T("C:\\Users\\bwaters\\My Documents\\Adapt It Unicode Work\\Ikoma to EngRubbish adaptations");
	fullPath = ::wxFileSelector(indenter_message,default_path);
	if (!fullPath.IsEmpty())
	{
		wxFileName fn(fullPath);
		wxString itsFilename = fn.GetFullName();
		wxString itsPath = fn.GetPath();
		wxFile fileIn;
		int level = -1;
		CBString atab = "    ";
		CBString eolStr = m_eolStr; // the platform-specific endofline string
		CBString indentStr; indentStr.Empty();
		bool bOpen = fileIn.Open(fullPath);
		if (bOpen)
		{
			wxFileOffset fileLen = fileIn.Length();
			size_t aSafeLength = (size_t)fileLen;
			aSafeLength = (aSafeLength * 3)/2; // allow for 50% bloat; actual is about 25%
			// need a big enough byte buffer
			char* pBuff = new char[aSafeLength];
			memset(pBuff,0,aSafeLength);
			size_t actualNumBytes = fileIn.Read((void*)pBuff, aSafeLength);
			actualNumBytes = actualNumBytes; // avoid warning
			// make a CBString with a copy of the buffer contents
			CBString inStr(pBuff); // put it into our CBString's buffer as it is UTF-8
			delete pBuff; // no longer needed
			bool bDivOpened = FALSE;
			bool bLastHandledEndtag = FALSE;
			int length = inStr.GetLength();
			char* ptr = inStr.GetBuffer();
			CBString divtag = "<div ";
			CBString divendtag = "</div>";
			char* pDiv = (char*)divtag;
			char* pDivEnd = (char*)divendtag;
			// make a buffer of nulls to accept the indented xml data
			char* pOut = new char[aSafeLength];
			memset(pOut,0,aSafeLength);
			char* pOutStart = pOut; // needed for filing the results out
			// set ptr to end of inStr buffer
			char* pEnd = ptr + (unsigned int)length;
			int indentLen;
			CBString threeEndDivs = "</div></div></div>"; // test for these at end 
				// (4 actually, but three is sufficient for the test's uniqueness)

#if defined( XHTML_PRETTY ) && defined (__WXDEBUG__)
			int indentPrettyLen;
			CBString indentPrettyStr;
			CBString spantag = "<span ";
			char* pSpantag = (char*)spantag;
			CBString metatag = "<meta ";
			char* pMetatag = (char*)metatag;
			int levelPretty;
#endif
#if !defined(DO_INDENT) && !defined(XHTML_PRETTY) && defined(DO_CLASS_NAMES)
			CBString classAttr = "class=\"";
			char* pClassAttr = (char*)classAttr;
#endif
			while (ptr < pEnd)
			{
#if !defined(DO_INDENT) && !defined(XHTML_PRETTY) && defined(DO_CLASS_NAMES)
			while (ptr < pEnd)
			{
				if (strncmp(ptr, pClassAttr, 7) == 0)
				{
					// ptr is pointing at class=" string, the attribute's value follows
					ptr += 7; // get ptr pointing at the attribute's value
					// count distance to next doublequote
					char dquote = '\"';
					char* pAttrEnd = strchr(ptr,(int)dquote);
					int distance = (int)(pAttrEnd - ptr);
					if (distance < 64)
					{
						strncpy(pOut, ptr, distance);
						pOut += distance;
						*pOut = ' '; pOut++;
						*pOut = ' '; pOut++;
						*pOut = ' '; pOut++;
					}
					ptr = ptr + (distance + 1);
				}
				else
				{
					ptr += 1;
				}
			}

#endif

#if defined(DO_INDENT)
				if (strncmp(ptr, pDiv, 5) == 0)
				{
					level++; //we are increasing the indent level (we start from -1)
					bDivOpened = TRUE;
					if (level == 0)
					{
						indentStr.Empty();
						strncpy(pOut, ptr, 5);
						pOut += 5;
						ptr += 5;
						// no indent in this instance
						bLastHandledEndtag = FALSE;
					}
					else
					{
						// compute the new indent (in spaces), add it, and then copy the
						// <div string; but first, add eolStr to pOut to open a new line
						// If the last div was just closed off, then we want the new one
						// to start of at the same indent level, otherwise, we nest the
						// new div in the old one
						int eolLen = eolStr.GetLength();
						strncpy(pOut, eolStr.GetBuffer(), eolLen);
						pOut += eolLen;
						if (bLastHandledEndtag)
						{
							level--; // decrement what we augmented on entry to this block,
							         // we are keeping this <div...> at the same level
							indentStr = BuildIndent(atab, level);
							indentLen = indentStr.GetLength();
							strncpy(pOut, indentStr.GetBuffer(), indentLen);
							pOut += indentLen;
							indentStr.Empty();
							// copy across the "<div " string & iterate
							strncpy(pOut, ptr, 5);
							pOut += 5;
							ptr += 5;
							bLastHandledEndtag = FALSE; // any subsequent one must be at deeper indent
						}
						else
						{
							// we are nesting the <div   > one level deeper
							indentStr = BuildIndent(atab, level);
							indentLen = indentStr.GetLength();
							strncpy(pOut, indentStr.GetBuffer(), indentLen);
							pOut += indentLen;
							indentStr.Empty();
							// copy across the "<div " string & iterate
							strncpy(pOut, ptr, 5);
							pOut += 5;
							ptr += 5;
						}

					}
				}
				else if (strncmp(ptr, pDivEnd, 6) == 0)
				{
					// if bDivOpened is TRUE, we put </div> at the same level as it's
					// opening <div> tag, set the flag false, so that a following
					// additional </div> endtag will then have the level decremented and a
					// lesser indent added before it... we must begin a newline first
					// but not when bDivOpened is still TRUE, because the file already has
					// one - so we do that only when we come to a subsequent </div> before
					// reaching a new <div>
					
					// if we get two 3 consecutive end divs, we know we are at the end-
					// the indenter needs to put an extra newline before the first,
					// otherwise the first </div> is to the right of the end of the last
					// span instead of being properly indented on the next line
					bool bAdjusted = FALSE;
					if (strncmp(ptr, (char*)threeEndDivs, 18) == 0)
					{
						int eolLen = eolStr.GetLength();
						strncpy(pOut, eolStr.GetBuffer(), eolLen);
						pOut += eolLen;
						bAdjusted = TRUE;
					}

					if (bDivOpened)
					{
						bDivOpened = FALSE;
						// the file will have opened a newline already, so we don't need
						// to here
						indentStr = BuildIndent(atab, level);
						indentLen = indentStr.GetLength(); // no level change
						strncpy(pOut, indentStr.GetBuffer(), indentLen);
						pOut += indentLen;
						indentStr.Empty();
						// copy across the "</div>" string & iterate
						strncpy(pOut, ptr, 6);					
						pOut += 6;
						ptr += 6;
						bLastHandledEndtag = TRUE;
					}
					else
					{
						// we've come to a </div> after having closed off an earlier one,
						// and so a level decrement is required; and an end-of-line needs
						// to be added as well (first)
						bLastHandledEndtag = TRUE;
						if (level > 0)
						{
							if (!bAdjusted)
							{
								int eolLen = eolStr.GetLength();
								strncpy(pOut, eolStr.GetBuffer(), eolLen);
								pOut += eolLen;
							}
							level--;
						}
						indentStr = BuildIndent(atab, level);
						indentLen = indentStr.GetLength();
						strncpy(pOut, indentStr.GetBuffer(), indentLen);
						pOut += indentLen;

						indentStr.Empty();
						// copy across the "</div>" string & iterate
						strncpy(pOut, ptr, 6);					
						pOut += 6;
						ptr += 6;
					}
				}
#if !defined( XHTML_PRETTY )
				else
				{
					*pOut = *ptr;
					// for keeping watch on last 128 chars when debugging
/*
#if defined(__WXDEBUG__)
					char* pFurtherIn;
					int aSpan = (int)(pOut - pOutStart);
					if ( (int)((aSpan/128)* 128) == aSpan )
					{
						pFurtherIn = pOutStart + (aSpan - 128);
					}
#endif
*/
					pOut += 1;
					ptr += 1;
				}
#endif
#if defined( XHTML_PRETTY ) && defined (__WXDEBUG__)

				else if (strncmp(ptr, pSpantag, 6) == 0
						 ||
						 strncmp(ptr, pMetatag, 6) == 0)
				{
					// ptr is at the beginning of a <span ... > tag or <meta ...> tag
					int eolLen = eolStr.GetLength();
					strncpy(pOut, eolStr.GetBuffer(), eolLen);
					pOut += eolLen;
					if (level == -1)
					{
						levelPretty = 1;
					}
					else
					{
						levelPretty = level + 1; // always 1 more than current level value
					}
					indentPrettyStr = BuildIndent(atab, levelPretty);
					indentPrettyLen = indentPrettyStr.GetLength();
					strncpy(pOut, indentPrettyStr.GetBuffer(), indentPrettyLen);
					pOut += indentPrettyLen;
					indentPrettyStr.Empty();
					// copy across the "<span " string & iterate
					strncpy(pOut, ptr, 6);
					pOut += 6;
					ptr += 6;
				}
				else
				{
					*pOut = *ptr;
					pOut += 1;
					ptr += 1;
				}
#endif
#endif // for #if defined(DO_INDENT))

			}
			
			CBString outStr(pOutStart);


#if !defined(DO_INDENT) && !defined(XHTML_PRETTY) && defined(DO_CLASS_NAMES)
			wxArrayString myarray;
			wxArrayString uniquesArr;
			wxString s = ToUtf16(outStr);
			bool bStoreEmptyStringsToo = FALSE;
			wxString delimiters = _T(" ");
			long numStrings = SmartTokenize(delimiters, s, myarray, bStoreEmptyStringsToo);
			size_t i;
			for (i = 0; i < (size_t)numStrings; i++)
			{
				wxString str = myarray[i];
				AddUniqueString(&uniquesArr, str);
			}
			// output the set of unique attribute value names for the class attribute, sorted
			uniquesArr.Sort();
			size_t itsCount = uniquesArr.Count();
			outStr.Empty();
			wxString aStr;
			wxString sEOL = ToUtf16(m_eolStr); // CR + LF for Windows, LF for Linux, CR for Mac
			for (i = 0; i < itsCount; i++)
			{	 
				aStr += uniquesArr[i] + sEOL;
			}
			aStr.Trim();
			outStr = ToUtf8(aStr);
#endif
			int itsLength = outStr.GetLength();
			wxASSERT(outStr.GetAt(itsLength) == '\0');
			wxString outfilename = itsPath; // initialize with the path to the folder
			wxString newName = itsFilename;
			int offset = newName.Find(_T('.'));
			if (offset != wxNOT_FOUND)
			{
				newName = newName.Left(offset);
			}
#if !defined(DO_INDENT) && !defined(XHTML_PRETTY) && defined(DO_CLASS_NAMES)
			newName += _T("_Class_Attr_Values_in_XHTML.txt");
#endif
#if defined(DO_INDENT)
			newName += _T("_IndentedXHTML.txt"); // before I used _utf8Str.txt
#endif
			outfilename += _T('\\');
			outfilename += newName; // store in the same folder as the original
			wxFile f;
			bool bOpenedOK = f.Create(outfilename,TRUE); // TRUE means "overwrite"
			if (bOpenedOK)
			{
				size_t writtenCount = f.Write((void*)outStr.GetBuffer(),itsLength);
				writtenCount = writtenCount; // avoid warning
				f.Close(); // load into Notepad++ and see how it looks
			}
			free((void*)pOutStart);
			outStr.Empty();
			wxBell(); // indicate we are done		   
		}
	}
}

CBString Xhtml::BuildIndent(CBString& atab, int level)
{
	CBString strIndent; strIndent.Empty();
	int index;
	for (index = 0; index < level; index++)
	{
		strIndent += atab;
	}
	return strIndent;
}
/////////////////////////////// FOR XHTML //////////////////////////////////

CBString Xhtml::MakeUUID()
{
	wxString myuuid = GetUuid(); // defined in helpers.cpp
	return ToUtf8(myuuid);
}

// lookup m_pEnum2LabelMap hashmap with XhtmlTagEnum key, and return the paired
// classAttrValue CCBString to be inserted in a class="" attribute, in either 
// a div tag or a span tag; returns empty string if the lookup failed
CBString Xhtml::GetClassLabel(XhtmlTagEnum key)
{
	enumIter = m_pEnum2LabelMap->find((int)key);
	CBString label = "";
	if (enumIter != m_pEnum2LabelMap->end())
	{
		label = ToUtf8(enumIter->second);
	}
	return label;
}

// Template string is:  <div class=\"classAttrLabel\">
CBString Xhtml::BuildDivTag(XhtmlTagEnum key)
{
	CBString attr = GetClassLabel(key);
	CBString div = m_divOpenTemplate;
	CBString left; left.Empty();
	CBString paramName = "classAttrLabel";
	int length = paramName.GetLength();
	int offset = div.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
	left = div.Left(offset);
	div = div.Mid(offset + length); // bleed off what we've found
	left += attr;
	left += div;
	wxLogDebug(_T("BuildDivTag():   %s"), (ToUtf16(left)).c_str());
	return left;
}

// ************************   NOTE *********************************
// Don't forget to call ConvertData(wxString data) to replace entities
// & generate the CBString used for input to PCDATA in any production,
// before passing that CBString in to the following Build...() functions
// *****************************************************************

// Next one builds for simple, or plusClass enum values
// The templates for these are, in order:
// <span lang=\"langCode\">spanPCDATA</span>
// <span class=\"classAttrLabel\" lang=\"langAttrCode\">spanPCDATA</span>
// Returns the production, but if xrefNested or chapterNumSpan  or verseNumSpan 
// is passed in for key, then returns an empty string (those other three types have
// their own Build...() functions)
// Note: for a 'simple' spanType, pass in no_value_ as the key since no classAttrValue is required
CBString Xhtml::BuildSpan(SpanTypeEnum spanType, XhtmlTagEnum key, CBString langCode, CBString pcData)  
{
	CBString span = "";
	int length;
	int offset;
	CBString left; left.Empty();
	CBString paramName1;
	CBString paramName2;
	CBString paramName3;
	CBString classAttrValue;
	if (spanType == simple)
	{
		span = m_spanTemplate[simple];
		paramName1 = "langCode";
		length = paramName1.GetLength();
		offset = span.Find(paramName1); wxASSERT(offset != wxNOT_FOUND);
		left = span.Left(offset);
		span = span.Mid(offset + length); // bleed off what we've found
		left += langCode;
		// now do the pcData
		paramName2 = "spanPCDATA";
		length = paramName2.GetLength();
		offset = span.Find(paramName2); wxASSERT(offset != wxNOT_FOUND);
		left += span.Left(offset);
		span = span.Mid(offset + length); // bleed off what we've found
		left += pcData;
		// add whatever's still remaining
		left += span;
	}
	else if (spanType == plusClass)
	{
		span = m_spanTemplate[plusClass];
		paramName1 = "classAttrLabel";
		paramName2 = "langAttrCode";
		paramName3 = "spanPCDATA";
		classAttrValue = GetClassLabel(key);
		length = paramName1.GetLength();
		offset = span.Find(paramName1); wxASSERT(offset != wxNOT_FOUND);
		left = span.Left(offset);
		span = span.Mid(offset + length); // bleed off what we've found
		left += classAttrValue;
		// now do the second parameter
		length = paramName2.GetLength();
		offset = span.Find(paramName2); wxASSERT(offset != wxNOT_FOUND);
		left += span.Left(offset);
		span = span.Mid(offset + length); // bleed off what we've found
		left += langCode;
		// now do the pcData
		length = paramName3.GetLength();
		offset = span.Find(paramName3); wxASSERT(offset != wxNOT_FOUND);
		left += span.Left(offset);
		span = span.Mid(offset + length); // bleed off what we've found
		left += pcData;
		// add whatever's still remaining
		// append what remains
		left += span;
	}
	wxLogDebug(_T("BuildSpan():   %s"), (ToUtf16(left)).c_str());
	return left;
}

// next builds for:
// <span class=\"Note_Target_Reference\" lang=\"langAttrCode\">chvsREF </span>
// (Jim's spec uses "Footnote_Origin_reference", but TE uses the above)
CBString Xhtml::BuildNoteTgtRefSpan(CBString langCode, CBString chvsREF)
{
	CBString span = "";
	int length;
	int offset;
	CBString left; left.Empty();
	CBString paramName1;
	CBString paramName2;
	span = m_spanTemplate[targetREF];
	paramName1 = "langAttrCode";
	length = paramName1.GetLength();
	offset = span.Find(paramName1); wxASSERT(offset != wxNOT_FOUND);
	left = span.Left(offset);
	span = span.Mid(offset + length); // bleed off what we've found
	left += langCode;
	// now do the chvsREF parameter
	paramName2 = "chvsREF";
	length = paramName2.GetLength();
	offset = span.Find(paramName2); wxASSERT(offset != wxNOT_FOUND);
	left += span.Left(offset);
	span = span.Mid(offset + length); // bleed off what we've found
	left += chvsREF;
	// append what remains
	left += span;
	wxLogDebug(_T("BuildNoteTgtRefSpan():   %s"), (ToUtf16(left)).c_str());
	return left;
}

// next builds an empty span production, template is:
// <span lang=\"langAttrCode\" />
CBString Xhtml::BuildEmptySpan(CBString langCode)
{
	CBString span = "";
	int length;
	int offset;
	CBString left; left.Empty();
	CBString paramName1;
	span = m_spanTemplate[emptySpan];
	paramName1 = "langAttrCode";
	length = paramName1.GetLength();
	offset = span.Find(paramName1); wxASSERT(offset != wxNOT_FOUND);
	left = span.Left(offset);
	span = span.Mid(offset + length); // bleed off what we've found
	left += langCode;
	// append what remains
	left += span;
	wxLogDebug(_T("BuildEmptySpan():   %s"), (ToUtf16(left)).c_str());
	return left;
}

// next builds last span of a caption, template is:
// <span lang=\"langAttrCode\" class=\"reference\">chvsREF</span>
// for the caption ref, if chvsREF is an empty string, then make it an empty tag
// <span lang=\"langAttrCode\" class=\"reference\" />
CBString Xhtml::BuildCaptionRefSpan(CBString langCode, CBString chvsREF)
{
	CBString span = "";
	int length;
	int offset;
	CBString left; left.Empty();
	CBString paramName1;
	CBString paramName2;
	if (chvsREF.IsEmpty())
	{
		span = m_spanTemplate[captionREFempty];
	}
	else
	{
		span = m_spanTemplate[captionREF];
	}
	paramName1 = "langAttrCode";
	length = paramName1.GetLength();
	offset = span.Find(paramName1); wxASSERT(offset != wxNOT_FOUND);
	left = span.Left(offset);
	span = span.Mid(offset + length); // bleed off what we've found
	left += langCode;
	if (!chvsREF.IsEmpty())
	{
		// now do the chvsREF parameter
		paramName2 = "chvsREF";
		length = paramName2.GetLength();
		offset = span.Find(paramName2); wxASSERT(offset != wxNOT_FOUND);
		left += span.Left(offset);
		span = span.Mid(offset + length); // bleed off what we've found
		left += chvsREF;
	}
	// append what remains
	left += span;
	wxLogDebug(_T("BuildCaptionRefSpan():   %s"), (ToUtf16(left)).c_str());
	return left;
}

// next builds for chapterNumSpan, or verseNumSpan SpanTypeEnum values
// Templates are, in order:
// <span class=\"Chapter_Number\" lang=\"langAttrCode\">chNumPCDATA</span>
// <span class=\"Verse_Number\" lang=\"langAttrCode\">vNumPCDATA</span>
CBString Xhtml::BuildCHorV(SpanTypeEnum spanType, CBString langCode, CBString corvStr)
{
	CBString span = "";
	int length;
	int offset;
	CBString left; left.Empty();
	CBString paramName1 = "langAttrCode";
	CBString paramName2;
	if (spanType == chapterNumSpan)
	{
		paramName2 = "chNumPCDATA";
		length = paramName1.GetLength();
		span = m_spanTemplate[chapterNumSpan];
		offset = span.Find(paramName1); wxASSERT(offset != wxNOT_FOUND);
		left = span.Left(offset);
		span = span.Mid(offset + length); // bleed off what we've found
		left += langCode;
		// now do the second parameter
		length = paramName2.GetLength();
		offset = span.Find(paramName2); wxASSERT(offset != wxNOT_FOUND);
		left += span.Left(offset);
		span = span.Mid(offset + length); // bleed off what we've found
		left += corvStr;
		// append what remains
		left += span;
	}
	else if (spanType == verseNumSpan)
	{
		paramName2 = "vNumPCDATA";
		length = paramName1.GetLength();
		span = m_spanTemplate[verseNumSpan];
		offset = span.Find(paramName1); wxASSERT(offset != wxNOT_FOUND);
		left = span.Left(offset);
		span = span.Mid(offset + length); // bleed off what we've found
		left += langCode;
		// now do the second parameter
		length = paramName2.GetLength();
		offset = span.Find(paramName2); wxASSERT(offset != wxNOT_FOUND);
		left += span.Left(offset);
		span = span.Mid(offset + length); // bleed off what we've found
		left += corvStr;
		// append what remains
		left += span;
	}
	wxLogDebug(_T("BuildCorV():   %s"), (ToUtf16(left)).c_str());
	return left;
}

// put the data's caller character in the title="+" attribute, replacing +
CBString Xhtml::InsertCallerIntoTitleAttr(CBString templateStr)
{
	// we treat the caller wxChar as a string, because when converted to utf8 it may be a
	// sequence of two or more bytes (e.g. that would be the case for a dagger)
	CBString symbolUtf8Str = ToUtf8(m_callerStr);
	int offset = templateStr.Find("title=\"+"); // 7 chars in from where
							// it was found is where we want to insert it
							// overwriting the + there
	if (offset != wxNOT_FOUND)
	{
		offset += 7;
		CBString left = templateStr.Left(offset);
		CBString right = templateStr.Mid(offset + 1); // starts from char after the +
		templateStr = left;
		templateStr += symbolUtf8Str;
		templateStr += right;
	}
	return templateStr;
}

// the next two build bits for footnotes and endnotes, BuildFootnoteInitialPart()
// builds from the following template:
// <span class=\"classAttrLabel\" id=\"idAttrUUID\" title=\"\">
// (With the current TE-based style names, classAttrLabel will be replaced by:
// Note_General_Paragraph if footnote_ is passed in for key,
// Endnote_General_Paragraph if endnote_ is passed in for key,
// Note_CrossHYPHENRefrence_Paragraph if crossReference_ is passed in for key
CBString Xhtml::BuildFootnoteInitialPart(XhtmlTagEnum key, CBString uuid)
{
	int length;
	int offset;
	CBString left; left.Empty();
	CBString span = m_spanTemplate[footnoteFirstPart];
	// put the caller character into the title attribute
	span = InsertCallerIntoTitleAttr(span);

	CBString classAttrValue = GetClassLabel(key);
	CBString paramName = "classAttrLabel";
	length = paramName.GetLength();
	offset = span.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
	left = span.Left(offset);
	span = span.Mid(offset + length); // bleed off what we've found
	left += classAttrValue;
	// now do the idAttrUUID
	paramName = "idAttrUUID";
	length = paramName.GetLength();
	offset = span.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
	left += span.Left(offset);
	span = span.Mid(offset + length); // bleed off what we've found
	left += uuid;
	// add whatever's still remaining
	left += span;
	return left;
}

// return either a valid production for the picture information, or an empty string
CBString Xhtml::BuildPictureProductions(CBString strPictureID, CBString langCode, CBString figureData)
{
	int length;
	int offset;
	CBString bar = "|";
	CBString production; production.Empty();
	CBString left; CBString right;
	offset = figureData.Find(bar);
	wxASSERT(offset != wxNOT_FOUND);
	CBString pictureFile;
	CBString col = "col";
	CBString strSize;
	CBString strCaption;
	CBString strReference; strReference.Empty();
	// we ignore DESCription information preceding the first bar character
	if (offset >= 0)
	{
		right = figureData.Mid(offset + 1); // start from char following first bar
		wxASSERT(!right.IsEmpty());

		// first field, obligatory, is the picture filename (typically from a standard set)
		offset = right.Find(bar);
		pictureFile = right.Left(offset);
		right = right.Mid(offset + 1); // next will start from char following second bar
		wxASSERT(!right.IsEmpty());

		// next field, obligatory, is the relative size -- USFM only has to options, "col"
		// (which fits in the current column) or "span" which is page-wide across all cols
		offset = right.Find(bar);
		strSize = right.Left(offset);
		right = right.Mid(offset + 1); // next will start from char following third bar (LOCation range info)
		wxASSERT(!right.IsEmpty());
		if (strSize = col)
		{
			// fit picture to current column
			strSize = "pictureColumn";
		}
		else
		{
			// must be a page-wide picture
			strSize = "picturePage";
		}

		// next two fields are LOC (reference range in which picture can be put) and COPY
		// which is for copyright info (I suspect they generate this automatically from
		// the first part of the filename for the picture, anyway, I've no examples of it
		// being used in TE, nor by Greg Trihus in his examples, nor by Erik in Sena 3,
		// nor by Jim Albright in his suggested styles, so I've no way to support them
		// properly - so I'll just ignore them (I don't know what to put in class="")
		offset = right.Find(bar);
		right = right.Mid(offset + 1); // next will start from char following fourth bar (COPyright info)
		wxASSERT(!right.IsEmpty());
		offset = right.Find(bar);
		right = right.Mid(offset + 1); // next will start from char following fifth bar (CAPtion)
		wxASSERT(!right.IsEmpty());

		// now get the caption text
		offset = right.Find(bar);
		strCaption = right.Left(offset);
		right = right.Mid(offset + 1); // next will start from char following sixth bar (REFerence info)

		// right now contains whatever is left, which might be an empty string, or a
		// string with one or more spaces -- only for actual chapter/verse content do we
		// treat the remainder as non-empty
		while (!right.IsEmpty() && right[0] == ' ')
		{
			right = right.Mid(1);
		}
		// if there is anything left, accept it
		if (!right.IsEmpty())
		{
			strReference = right;
			length = strReference.GetLength();
			if (strReference.GetAt(length - 1) != ' ')
			{
				strReference += " "; // append space if one not already there
			}
		}

		// the fields have been parsed, now build the productions with their contents...
		// first, do the <img> element; it's template is:
		// <img id=\"idAttrUUID\" class=\"picture\" src=\"srcAttrURL\" alt=\"altAttrURL\" />
		CBString imgStr = m_imgTemplate;
		int length;
		CBString paramName = "idAttrUUID";
		length = paramName.GetLength();
		offset = imgStr.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
		left = imgStr.Left(offset);
		imgStr = imgStr.Mid(offset + length); // bleed off what we've found
		left += strPictureID; // add the "Figure-XXX-num" ID string
		// now find the substring to where srcAttrURL is, and append it
		paramName = "srcAttrURL";
		length = paramName.GetLength();
		offset = imgStr.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
		left += imgStr.Left(offset);
		imgStr = imgStr.Mid(offset + length); // bleed off what we've found
		left += pictureFile;
		// now do the alt URL
		paramName = "altAttrURL";
		length = paramName.GetLength();
		offset = imgStr.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
		left += imgStr.Left(offset);
		imgStr = imgStr.Mid(offset + length); // bleed off what we've found
		left += pictureFile;
		// now add what's left
		left += imgStr;
		// now that we have the params inserted, reuse imgStr to store the result
		imgStr = left;
		left.Empty();

		// Next do the main part; its template is (splitting a long line into 3 parts):
		// 
		// <div class=\"picturePlacement\">myImgPCDATA<div class=\"pictureCaption\">
		// <span lang=\"langAttrCode\">captionPCDATA</span><span lang=\"langAttrCode\" 
		// class=\"reference\">refPCDATA</span></div></div>
		// 
		// But if there is no reference, then the empty tag should be produced - it is:
		// 
		// <div class=\"picturePlacement\">myImgPCDATA<div class=\"pictureCaption\">
		// <span lang=\"langAttrCode\">captionPCDATA</span><span lang=\"langAttrCode\" 
		// class=\"reference\" /></div></div>
		CBString mainStr;
		if (strReference.IsEmpty())
		{
			// create an empty tag
			mainStr = m_picturePcdataEmptyRefTemplate;
		}
		else
		{
			// there's a reference, so do the full tag
			mainStr = m_picturePcdataTemplate;
		}
		paramName = "picturePlacement";
		length = paramName.GetLength();
		offset = mainStr.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
		left = mainStr.Left(offset);
		mainStr = mainStr.Mid(offset + length); // bleed off what we've found
		left += strSize; // append the "picturePage" or "pictureColumn" strings
		// now look for the "myImgPCDATA"
		paramName = "myImgPCDATA";
		length = paramName.GetLength();
		offset = mainStr.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
		left += mainStr.Left(offset);
		mainStr = mainStr.Mid(offset + length); // bleed off what we've found
		left += imgStr; // append the <img> tag constructed above
		// next look for the langAttrCode parameter, append everything up to that point
		paramName = "langAttrCode";
		length = paramName.GetLength();
		offset = mainStr.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
		left += mainStr.Left(offset);
		mainStr = mainStr.Mid(offset + length); // bleed off what we've found
		left += langCode; // append the langCode passed in
		// look for the param   captionPCDATA  and replace with the caption text
		paramName = "captionPCDATA";
		length = paramName.GetLength();
		offset = mainStr.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
		left += mainStr.Left(offset);
		mainStr = mainStr.Mid(offset + length); // bleed off what we've found
		left += strCaption; // append the caption text
		// next, the language code a second time
		paramName = "langAttrCode";
		length = paramName.GetLength();
		offset = mainStr.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
		left += mainStr.Left(offset);
		mainStr = mainStr.Mid(offset + length); // bleed off what we've found
		left += langCode; // append the langCode passed in
		// Next, if there is no reference string, then append the rest, otherwise do the
		// search for the refPCDATA param string, replace it with strReference, add the
		// end, and we are done
		if (strReference.IsEmpty())
		{
			left += mainStr;
		}
		else
		{
			// there's a reference, so do the full tag
			paramName = "refPCDATA";
			length = paramName.GetLength();
			offset = mainStr.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
			left += mainStr.Left(offset);
			mainStr = mainStr.Mid(offset + length); // bleed off what we've found
			left += strReference; // append the reference string
			// append what's left
			left += mainStr;
		}
		production = left;
	}
	return production;
}

// next builds for nested SpanTypeEnum value; this does the pair of nested spans for class
// attribute values: Note_General_Paragraph (for \f), Endnote_General_Paragraph (for \fe), and
// Note_CrossHYPHENReference_Paragraph (for \rq)      Template is:
// <span class=\"classAttrLabel\" id=\"idAttrUUID\" title=\"\"><span lang=\"langAttrCode\">myPCDATA</span></span>
CBString Xhtml::BuildNested(XhtmlTagEnum key, CBString uuid, CBString langCode, CBString pcData)
{
	int length;
	int offset;
	CBString left; left.Empty();
	CBString span = m_spanTemplate[nested];

	// insert the caller character into the title attribute
	span = InsertCallerIntoTitleAttr(span);

	CBString classAttrValue = GetClassLabel(key);
	CBString paramName = "classAttrLabel";
	length = paramName.GetLength();
	offset = span.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
	left = span.Left(offset);
	span = span.Mid(offset + length); // bleed off what we've found
	left += classAttrValue;
	// now do the idAttrUUID
	paramName = "idAttrUUID";
	length = paramName.GetLength();
	offset = span.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
	left += span.Left(offset);
	span = span.Mid(offset + length); // bleed off what we've found
	left += uuid;
	// now do langAttrCode
	paramName = "langAttrCode";
	length = paramName.GetLength();
	offset = span.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
	left += span.Left(offset);
	span = span.Mid(offset + length); // bleed off what we've found
	left += langCode;
	// now do myPCDATA
	paramName = "myPCDATA";
	length = paramName.GetLength();
	offset = span.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
	left += span.Left(offset);
	span = span.Mid(offset + length); // bleed off what we've found
	left += pcData;
	// add whatever's still remaining
	left += span;
	return left;
}


// This one builds something of the form: 
// <span class="scrFootnoteMarker"><a href="#F987d1d46-f1b1-4e08-9da3-cd6d12b26707"></a></span>
// But it does it in two steps, the anchor <a ... UUID .../a> is built separately by the
// BuildAnchor() function, from a passed in newly generated uuid. The m_uuid stores the
// uuid temporarily, as it is also needed for BuildNested(). Then the anchor projection is
// passed in to BuildFootnoteMarkerSpan(), using the following template:
// <span class=\"scrFootnoteMarker\">myUuidAnchor</span>
CBString Xhtml::BuildFootnoteMarkerSpan(CBString uuid)
{
	CBString myUuidAnchor = BuildAnchor(uuid);
	int length;
	int offset;
	CBString left; left.Empty();
	CBString span = m_footnoteMarkerTemplate; // <span class=\"scrFootnoteMarker\">anUuidAnchor</span>
	CBString paramName = "anUuidAnchor";
	length = paramName.GetLength();
	offset = span.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
	left = span.Left(offset);
	span = span.Mid(offset + length); // bleed off what we've found
	left += myUuidAnchor;
	// add whatever's still remaining
	left += span;
	return left;
}

// This one builds for scrFootnoteMarker, using template: <a href=\"#hrefAttrUUID\"></a>
// The uuid must be generated before it is called and stored in m_uuid because it needs to
// be used also within BuildNested() - the latter always follows the production whose
// class has the value srcFootnoteMarker, so BuildNested picks up the uuid from m_uuid
CBString Xhtml::BuildAnchor(CBString myUUID)
{
	int length;
	int offset;
	CBString left; left.Empty();
	CBString span = m_anchorPcdataTemplate; // <a href=\"#hrefAttrUUID\"></a>
	CBString paramName = "hrefAttrUUID";
	length = paramName.GetLength();
	offset = span.Find(paramName); wxASSERT(offset != wxNOT_FOUND);
	left = span.Left(offset);
	span = span.Mid(offset + length); // bleed off what we've found
	left += myUUID;
	// add whatever's still remaining
	left += span;
	return left;
}

// puts </div></div>..., as many as the input param says, at the end of the data - should
// be 3; also dumps endnotes if there were any squirreled away in m_endnoteDumpArray
CBString Xhtml::FinishOff(int howManyEndDivs)
{
	CBString theEnd; theEnd.Empty();
	int count = 0;
	int i;
	// first, dump any endnotes
	if (!m_endnoteDumpArray.IsEmpty())
	{
		count = m_endnoteDumpArray.Count();
		for (i = 0; i < count; i++)
		{
			wxString endnoteStr = m_endnoteDumpArray.Item(i);
			theEnd += ToUtf8(endnoteStr);
		}
	}
	// finally, dump the </div> endtags, and those for <body> and <html>
	for (i=0; i < howManyEndDivs; i++)
	{
		theEnd += m_divCloseTemplate;
	}
	theEnd += "</body>";
	theEnd += m_eolStr;
	theEnd += "</html>";
	return theEnd;
}




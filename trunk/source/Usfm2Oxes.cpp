/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Usfm2Oxes.h
/// \author			Bruce Waters
/// \date_created	2 Sept 2010
/// \date_revised	
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the header file for the Usfm2Oxes class. 
/// The USFM2Oxes class takes an in-memory plain text (U)SFM marked up text, chunks it
/// according to criteria relevant to oxes support, and then builds oxes xml from each
/// chunk in the appropriate order of chunks. The chunks are stored on structs. 
/// Initially oxes version 1 is supported, later it will also support version 2 when the
/// latter standard is finalized. (Deuterocannon books are not supported explicitly.)
/// \derivation		The Usfm2Oxes class is derived from wxEvtHandler.
/////////////////////////////////////////////////////////////////////////////

#define _IntroOverrun
#undef _IntroOverrun

#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Usfm2Oxes.h"
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

#include "AdaptitConstants.h"
#include "Adapt_It.h"
#include "Adapt_ItDoc.h"
#include "helpers.h"
#include "BString.h"
#include "Usfm2Oxes.h"

// complete the templated array definitions
WX_DEFINE_OBJARRAY(NoteDetailsArray);
WX_DEFINE_OBJARRAY(AIGroupArray);
WX_DEFINE_OBJARRAY(AISectionInfoArray);
WX_DEFINE_OBJARRAY(AISectionPartArray);
WX_DEFINE_OBJARRAY(SectionPartTemplateArray); // four fixed initialized instances 
		// of SectionPart* each has a different partName member, and a different 
		// inventory of possible markers; each SectionPart* in AISEctionPartArray
		// is cloned off of one of these four instances -- these conniptions are
		// forced on us because of the tension created by the WX_DECLARE_OBJARRAY
		// which can only store a single type, and our need for parsing sections
		// to be able to store a sequence of four different types of struct


//WX_DEFINE_OBJARRAY(AISectHdrOrParaArray);


/// This flag is used to indicate that the text being processed is unstructured, i.e.,
/// not containing the standard format markers (such as verse and chapter) that would 
/// otherwise make the document be structured. This global is used to restore paragraphing 
/// in unstructured data, on export of source or target text.
extern bool	gbIsUnstructuredData; 

extern const wxChar* filterMkr; // defined in the Doc
extern const wxChar* filterMkrEnd; // defined in the Doc

extern wxString commonHaltingMarkers; // defined in Adapt_ItView.cpp
extern wxString charFormatMkrs;
extern wxString charFormatEndMkrs;
extern wxString embeddedWholeMkrs;
extern wxString embeddedWholeEndMkrs;

extern CAdapt_ItApp* gpApp;

// *******************************************************************
// Event handlers
// *******************************************************************

BEGIN_EVENT_TABLE(Usfm2Oxes, wxEvtHandler)


END_EVENT_TABLE()

void Usfm2Oxes::SetupOxesTags()
{
    // Build up this function over time... for now, get the data entered as comment lines -
    // I can then probably later pull it out to a file and build a cc table to construct
    // the code I want using this stuff. For each oxes tagname, give the name as a ascii
    // string, a tab, then a for the indent level (indent unit multiplier digit, such as 0
    // or 1 or 2, etc -- the actual indent can then be constructed from a unit of, say, two
    // spaces eg. 4 means 8 spaces for the indent; Wimbish uses 3 spaces for the indent
    // unit). I think I only need the indent multiplies, which I'll store in a hash table
    // with the tagname as key; using CBString; the indent unit decision I can make later
    // 
    // Some tags get variable indenting in the OXES samaples, depending on what they are
    // embedded within. For example, the trGroup tag is at level 6 when it occurs in a
    // titleGroup, 6 when in a trGroup, 7 in a Line production, 7 in a section production;
    // but since it is all arbitrary anyway, I can choose a unique value for each tag and
    // stick with that always, so that's what I'll do.

	/* 
    // this list can be sorted later on, I'm constructing it initially what what I see in
    // the Ikoma OXES file, then I'll look a oxesCore version 1 file, but I've given
    // annotations an extra level of indent (3, not 0, and their embedded tags 3 extra
    // also, so as to fit with para being 4 elsewhere) Inline tags are marked with -1 as a
    // flag that they ignore the indenting level; some inline tags are given an indent -
    // eg. note is inline, but samples give it level 6

oxes	0
oxesText	1
canon	2
book	3
header	2
revisionDesc	3
date	4
para	4
work	3
titleGroup	4
title	5
trGroup	6
tr	7
bt	7
p	5
section	4
sectionHead 5
verseStart	6
verseEnd	6
contributor	4
titlePage	2
chapterStart	6
chapterEnd	6
note	6
annotation	3
created	4
modified	4
resolved	4
notationQuote	4
notationDiscussion	4
notationResponse	4
notationRecommendation	4
notationCategories	4
notationResolution	4
span	5
l	5
parallelPassageHead	6
ordinalNumberEnding	-1
otPassage	-1
nameOfGod	-1
mentioned	-1
label	-1
keyWord	-1
table	5
head	5
row	6
item	7
introduction	2
supplied	-1
stanzaBreak	5
speaker	6
soCalled	-1
seeInGlossary	-1
rights	3
referencedText	-1
glossary	6
gloss	-1
foreign	-1
emphasis	-1

**** not finished ****

	*/
}

// *******************************************************************
// Construction/Destruction
// *******************************************************************

Usfm2Oxes::Usfm2Oxes()
{
}

Usfm2Oxes::Usfm2Oxes(CAdapt_ItApp* app)
{
	wxASSERT(app != NULL);
	m_pApp = app;
	m_pTitleInfo = NULL;
	m_pIntroInfo = NULL;
	m_pCanonInfo = NULL;

	Initialize();
}

Usfm2Oxes::~Usfm2Oxes()
{
	ClearTitleInfo();
	delete m_pTitleInfo;
	ClearIntroInfo();
	delete m_pIntroInfo;
	ClearCanonInfo();
	delete m_pCanonInfo;
	// clear the 6 template instances of SectionPart
	size_t count = m_arrSectionPartTemplate.GetCount();
	size_t index;
	for (index = 0; index < count; index++)
	{
		SectionPart* pSectionPart = m_arrSectionPartTemplate.Item(index);
		ClearAISectionPart(pSectionPart);
	}
	m_arrSectionPartTemplate.Clear();
}

void Usfm2Oxes::SetBookID(wxString& aValidBookCode)
{
	m_bookID = aValidBookCode;
}

void Usfm2Oxes::Initialize()
{
	// do only once-only data structure setups here; each time a new oxes file is to be
	// produced, the stuff specific to any earlier exports will need to be cleared out
	// before the new one's data is added in (using a separate function
	 
	// setup some useful marker strings
	m_chapterMkr = _T("\\c");
	m_idMkr = _T("\\id");
	m_verseMkr = _T("\\v");
	backslash = _T("\\");
	m_majorSectionMkr = _T("\\ms"); // when any of this kind opens a new section, we
		// don't start a new section if any \s marker is found preceding the next verse
		
	// the following are not in the global wxString, charFormatMkrs defined in
	// Adapt_ItView.cpp file, so we define them here and add these to those so that
	// m_specialMrks and m_specialEndMkrs will comply with USFM 2.3 with respect to
	// special markers (but we won't try support them all, but only these command and
	// whichever extras users ask for; we do however need to know them all for parsing
	// purposes) (Note, the last, \vt, has no endmarker, is not a USFM marker, but some
	// people have used it as a marker for 'verse text' - so I'll treat it as an inline
	// marker (so that it is parsed over, and as it would be at the start of the returned
	// dataStr from the parsing function, we'll test for it and if present, just skip over
	// it - in effect, throwing it away)
	m_specialMkrs = _T("\\add \\ord \\sig \\sls \\fig \\ndx \\pro \\w \\wq \\wh \\lit \\pb \\vt ");
	m_specialEndMkrs = _T("\\add* \\ord* \\sig* \\sls* \\fig* \\ndx* \\pro* \\w* \\wq* \\wh* \\lit* \\pb* ");
	m_specialMkrs = charFormatMkrs + m_specialMkrs;
	m_specialEndMkrs = charFormatEndMkrs + m_specialEndMkrs;

	// set the two custom markers that Adapt It recognises
	m_freeMkr = _T("\\free"); // our Adapt It custom marker for OXES backtranslations
	m_noteMkr = _T("\\note"); // our Adapt It custom marker for OXES annotation with 
									 // type 'translatorNote' and category 'adaptationNote'
	m_noteEndMkr = m_noteMkr + _T("*");
	m_freeEndMkr = m_freeMkr + _T("*");

	// remarks marker
	m_remarkMkr = _T("\\rem");

 
    // Make the wxString of 'halting markers' -- utilize Bill's commonHaltingMarkers string
    // and add more (eg \h) but don't add \free or note (commonHaltingMarkers is definied
	// in Adapt_ItView.cpp) We'll handle our two custom ones as special tests, not as part
	// of the USFM standard's markers
	// m_haltingMarkers = commonHaltingMarkers;   use Bill's list but with the introduction
	// markers removed, we'll have those in our own m_intoHaltingMarkers string (see below)
	// Our halting markers list includes title and body text markers, footnotes, endnotes,
	// cross references, tables, running heading, subheadings, and some markers from the
	// 1998 PNG SFM set too; but excludes special markers, character formatting markers,
	// and other inline markers -- see above
	m_haltingMarkers = _T("\\v \\c \\p \\m \\q \\qc \\qm \\qr \\qa \\pi \\mi \\pc \\pt \\ps \\pgi \\cl \\vn \\f \\fe \\x \\gd \\tr \\th \thr \\tc \tcr \\mt \\st \\mte \\div \\ms \\s \\sr \\sp \\d \\di \\hl \\r \\dvrf \\mr \\br \\rr \\pp \\pq \\pm \\pmc \\pmr \\cls \\li \\qh \\gm \\gs \\gd \\gp \\tis \\tpi \\tps \\tir \\pb \\hr \\id \\b ");
	wxString additions = _T("\\h \\vp \\id "); // \vp ... \vp* is for publishing things 
			// like "3b" as a verse number, when what precedes is a range up to "...-3a"
	m_haltingMarkers = additions + m_haltingMarkers;

	// The following are introductory markers and these halt parsing. I've retained the
	// level digits as it's easier to generate the string array having them this way, but
	// we also need the string with them all without the levels, for use in
	// IsHaltingMarker() so those are in m_haltingMarkers_IntroOnly; the
    // PopulateIntroductionPossibleMarkers() function uses m_introHaltingPossibleMarkers to
    // populate the array; but the IsHaltingMarker() function has to have access to all
    // halting markers whether intro ones or not, otherwise GetIntroInfoChunk() will parse
    // beyond the end of, say, the Introduction area because with only the introduction
    // halting markers any marker from the canonical area would look like an inline marker
    // and it would not terminate when the introductory material has been traversed. So, to
    // m_haltingMarkers as it is above we must append m_haltingMarkers_IntroOnly as it is
    // below
	m_introHaltingMarkers = _T("\\imt \\imt1 \\imt2 \\imt3 \\imte \\is \\is1 \\is2 \\is3 \\ip \\ipi \\ipq \\ipr \\iq \\iq1 \\iq2 \\iq3 \\im \\imi \\imq \\io \\io1 \\io2 \\io3 \\iot \\iex \\ib \\ili \\ili1 \\ili2 \\ie ");
	// Introductions can have only a couple of inLine markers - each has an endmarker
	m_introInlineBeginMarkers = _T("\\ior \\iqt ");
	m_introInlineEndMarkers = _T("\\ior* \\iqt* ");
	// The ones which have levels removed are \imt# \is# \iq# (\ili#, # = 1 or 2) and \io#
	m_haltingMarkers_IntroOnly = _T("\\imt \\imte \\is \\ip \\ipi \\ipq \\ipr \\iq \\im \\imi \\imq \\io \\iot \\iex \\ib \\ili \\ie ");

	// combine the halting markers...
	m_haltingMarkers += m_haltingMarkers_IntroOnly; // now IsHaltingMarker() will always 
													// work right in every chunk
													
	// new the fast-access string for the section-defining markers; these will be mostly
	// \s or \s1, but other markers are possible; also, our code will have to check
	// once an \s or \s1, etc, is found because often a \c chapter marker will precede and
	// when that happens the chapter info goes within the Oxes <section>, so we'll have to
	// make sure chapter starts at a section start are indicated in the section chunks  
	m_sectioningMkrs = _T("\\s \\s1 \\s2 \\s3 \\ms \\ms1 \\ms2 \\ms3 "); 

	// next six or more fast-access strings are for parsing sections to obtain arrays of
	// SectionPart pointers...
	// any markers which can start a paragraph or finish one; \nb 'no break' is not
	// included as while USFM lists it with paragraph markers, it indicates "no paragraph
	// break" (occurs after \c ) and so we just ignore it; we'll also not include \cls
	// "(epistle) closure" marker, as the text following belongs in the current paragraph
	// anyway; also ignore \b as it only indicates some extra vertical white space, not a
	// proper paragraph break -- we use this marker set for dividing off successive
	// paragraphs; a complication is that either or both of \free or \note, if present,
	// will occur before any of these which occur in the text (we don't include \free nor
	// \note in these marker lists, but test for them explicitly in the code with a
	// separate test -- it's better this way because we often have to give these separate
	// handling in the protocols for storage etc)
	m_paragraphMkrs = _T("\\p \\m \\pc \\pr \\pmo \\pm \\pmc \\pmr \\pi \\pi1 \\pi2 \\pi3 \\mi \\li \\li1 \\li2 \\ph \\ph1 \\ph2 \\ph3 ");													
	// the following set define the Poetry markers (\qs and \qs*, the "Selah" markers, are
	// not included because these belong in the inline set; similarly for \qac and \qac*,
	// for an acrostic letter within a poetic line (it too is a character style marker);
	// we'll include \q4 even though it is unlikely to ever occur in data
	m_poetryMkrs = gpApp->m_poetryMkrs; // I put these on the app, because 
										// TokenizeText() would like to use them
	m_chapterMkrs = _T("\\c \\ca \\ca* \\cl \\cp \\cd ");
	m_majorOrSeriesMkrs = _T("\\ms \\ms1 \\qa \\ms2 \\ms3 ");
	m_parallelPassageHeadMkrs = _T("\\r ");
	wxString m_rangeOrPsalmMkrs = _T("\\mr \\d ");
	wxString m_normalOrMinorMkrs = _T("\\s \\s1 \\s2 \\s3 \\s4 "); 

	// create the structs: TitleInfo struct, IntroInfo struct
	m_pTitleInfo = new TitleInfo;
	m_pIntroInfo = new IntroductionInfo;
	m_pCanonInfo = new CanonInfo;

	// this stuff is cleared out already, but no harm in ensuring it
	m_pTitleInfo->bChunkExists = FALSE;
	m_pTitleInfo->strChunk.Empty();

	m_pIntroInfo->bChunkExists = FALSE;
	m_pIntroInfo->strChunk.Empty();

	m_pCanonInfo->strChunk.Empty();

	// set up the set-up-only-once array for title info
	m_titleMkrs = _T("\\id \\ide \\h \\h1 \\h2 \\h3 \\mt \\mt1 \\mt2 \\mt3 ");
	PopulatePossibleMarkers(m_titleMkrs, titleChunkType, (void*)m_pTitleInfo);

	// permit Adapt It notes and free translations to be present in the chunk
	// I changed my mind - I'll support the two custom markers in the functions as special
	// cases rather than including them in the marker lists in data structures

    // the wxString members are initially empty so we can leave them, but a separate
    // function, ClearTitleInfo, must clean them out every time a new oxes xml file is
    // being produced

	// for the Introduction material, m_introHaltingMarkers has the markers with their
	// levels, so we can populate m_pIntroInfo->arrPossibleMarkers more easily than above
	// by using a function to do it
	//PopulateIntroductionPossibleMarkers(m_pIntroInfo->arrPossibleMarkers);
	PopulatePossibleMarkers(m_introHaltingMarkers, introductionChunkType, (void*)m_pIntroInfo);

	// (This comment copied from Usfm2Oxes.h & added to a little at the end)
    // For SectionPart structs, which can be one of 4 variant types, we have to set up
    // (using Initialize() the markers which define each type - and so for efficiency we
    // want to set them up only once each. We'll define a (private)
    // SectionPartTemplateArray which will have 4 SectionType instances (actually, pointers
    // to SectionType) - we need only 4, provided we don't find another type of subChunk of
    // a section other than the 4 currently observed, ie. sectionHead, parallelPassageHead,
    // Poetry, Paragraph -- but we can add extra easily if necessary. Then each time we
    // create a SectionPart instance on the heap, we'll clone it using the copy constructor
    // from the relevant one in this template array. Since each arrPossibleMarkers is a
    // wxArrayString, and the latter has an overloaded operator= defined, we'll get a copy
    // of the marker set put into each instance we create from the templated instance - we
    // only then need to set the instance's partName member, and we can then use the newly
    // created instance for delineating the next chunk being parsed. The
    // ParseSectionIntoSectionParts() function will input a section and do the parsing into
    // an object array of SectionPart* pointers - the protocol for doing the parsing will
    // need to be (1) try for a sectionHead parse first, if it succeeds, try for (2) a
    // parallelPassageHead parse, then try for (3) a Poetry parse, and if none of those
    // produced a span of parsed text, try for (4) a Paragraph parse.
    // The template array has this declaration
	// WX_DECLARE_OBJARRAY(SectionPart*, SectionPartTemplateArray);
	// In Usfm2Oxes.h there is a private member defined which uses it:
	// SectionPartTemplateArray m_arrSectionPartTemplate; and we initialize it now to set
	// up the four templated SectionPart* instances on the heap which parsing of sections
	// into an array of SectionPart instances will require, as discussed above.
	SectionPart* pSectionPart = new SectionPart;
	pSectionPart->strChunk.Empty();
	pSectionPart->bChunkExists = FALSE; // default
	pSectionPart->sectionPartType = majorOrSeriesChunkType; // for <sectionHead> tag
	pSectionPart->pSectionInfo = NULL; // this template instance doesn't have a parent
	// populate it's arrPossibleMarkers wxArrayString, from the fast-access string
	PopulatePossibleMarkers(m_majorOrSeriesMkrs, majorOrSeriesChunkType, (void*)pSectionPart);
	// retain, but otherwise ignore any preceding chapter type of marker
	PopulateSkipMarkers(m_chapterMkrs, majorOrSeriesChunkType, (void*)pSectionPart);
	// store this now completed template instance as the first in m_arrSectionPartTemplate
	m_arrSectionPartTemplate.Add(pSectionPart); // the one for major section or series is added

	// Next, the one for the range or psalm types (\mr or \d)
	pSectionPart = new SectionPart;
	pSectionPart->strChunk.Empty();
	pSectionPart->bChunkExists = FALSE; // default
	pSectionPart->sectionPartType = rangeOrPsalmChunkType; // for \mr or \d markers
	pSectionPart->pSectionInfo = NULL; // this template instance doesn't have a parent
	// populate it's arrPossibleMarkers wxArrayString, from the fast-access string
	PopulatePossibleMarkers(m_rangeOrPsalmMkrs, rangeOrPsalmChunkType, (void*)pSectionPart);
	// retain, but otherwise ignore any preceding chapter type of marker
	PopulateSkipMarkers(m_chapterMkrs, rangeOrPsalmChunkType, (void*)pSectionPart);
	// store this now completed template instance as the second in m_arrSectionPartTemplate
	m_arrSectionPartTemplate.Add(pSectionPart); // the one for range or psalm head is added

	// Next, the one for the normal or minor types (\s or \s1 is normal, \s2 is minor, and
	// we'll treat \s3 or \s4 as minor, that is, as if they were \s2 -- but we do this at the
	// xml production building stage, here we just accept \se and \s4 as allowed in the chunk)
	pSectionPart = new SectionPart;
	pSectionPart->strChunk.Empty();
	pSectionPart->bChunkExists = FALSE; // default
	pSectionPart->sectionPartType = normalOrMinorChunkType; // for \s \s1 \s2 etc markers
	pSectionPart->pSectionInfo = NULL; // this template instance doesn't have a parent
	// populate it's arrPossibleMarkers wxArrayString, from the fast-access string
	PopulatePossibleMarkers(m_normalOrMinorMkrs, normalOrMinorChunkType, (void*)pSectionPart);
	// retain, but otherwise ignore any preceding chapter type of marker
	PopulateSkipMarkers(m_chapterMkrs, normalOrMinorChunkType, (void*)pSectionPart);
	// store this now completed template instance as the third in m_arrSectionPartTemplate
	m_arrSectionPartTemplate.Add(pSectionPart);

    // Next, the one for parallelPassageHead type (one marker only, \r) -- we put it here
    // because it typically precedes paragraph and/or poetry chunks and follows the chunks
    // above
	pSectionPart = new SectionPart;
	pSectionPart->strChunk.Empty();
	pSectionPart->bChunkExists = FALSE; // default
	pSectionPart->sectionPartType = parallelPassageHeadChunkType; // for \s \s1 \s2 etc markers
	pSectionPart->pSectionInfo = NULL; // this template instance doesn't have a parent
	// populate it's arrPossibleMarkers wxArrayString, from the fast-access string
	PopulatePossibleMarkers(m_parallelPassageHeadMkrs, parallelPassageHeadChunkType, (void*)pSectionPart);
	// retain, but otherwise ignore any preceding chapter type of marker
	PopulateSkipMarkers(m_chapterMkrs, parallelPassageHeadChunkType, (void*)pSectionPart);
	// store this now completed template instance as the fourth in m_arrSectionPartTemplate
	m_arrSectionPartTemplate.Add(pSectionPart);

    // Next, the one for poetry type (several markers based on \q)
	pSectionPart = new SectionPart;
	pSectionPart->strChunk.Empty();
	pSectionPart->bChunkExists = FALSE; // default
	pSectionPart->sectionPartType = poetryChunkType; // for \q \q1 \q2 etc
	pSectionPart->pSectionInfo = NULL; // this template instance doesn't have a parent
	// populate it's arrPossibleMarkers wxArrayString, from the fast-access string
	PopulatePossibleMarkers(m_poetryMkrs, poetryChunkType, (void*)pSectionPart);
	// retain, but otherwise ignore any preceding chapter type of marker
	PopulateSkipMarkers(m_chapterMkrs, poetryChunkType, (void*)pSectionPart);
	// store this now completed template instance as the fifth in m_arrSectionPartTemplate
	m_arrSectionPartTemplate.Add(pSectionPart);

    // Last, the one for paragraph type (several markers based on \p)
	pSectionPart = new SectionPart;
	pSectionPart->strChunk.Empty();
	pSectionPart->bChunkExists = FALSE; // default
	pSectionPart->sectionPartType = paragraphChunkType; // for \p \m etc
	pSectionPart->pSectionInfo = NULL; // this template instance doesn't have a parent
	// populate it's arrPossibleMarkers wxArrayString, from the fast-access string
	PopulatePossibleMarkers(m_paragraphMkrs, paragraphChunkType, (void*)pSectionPart);
	// retain, but otherwise ignore any preceding chapter type of marker
	PopulateSkipMarkers(m_chapterMkrs, paragraphChunkType, (void*)pSectionPart);
	// store this now completed template instance as the sixth in m_arrSectionPartTemplate
	m_arrSectionPartTemplate.Add(pSectionPart);


	// debug: check things are working to this point -- yep.
	//m_pTitleInfo->idStr = _T("g'day mate");
	//wxLogDebug(_T("Field: idStr =  %s"),m_pTitleInfo->idStr);
}

void Usfm2Oxes::ClearTitleInfo()
{
    // this function cleans out the file-specific data present in the TitleInfo struct from
    // the last-built oxes file, in preparation for building a new oxes file
	m_pTitleInfo->bChunkExists = FALSE;
	m_pTitleInfo->strChunk.Empty();
	if (!m_pTitleInfo->arrPossibleMarkers.IsEmpty())
	{
		m_pTitleInfo->arrPossibleMarkers.Clear();
	}
	if (!m_pTitleInfo->arrSkipMarkers.IsEmpty())
	{
		m_pTitleInfo->arrSkipMarkers.Clear();
	}
	ClearAIGroupArray(m_pTitleInfo->aiGroupArray);
}

void Usfm2Oxes::ClearIntroInfo()
{
    // this function cleans out the file-specific data present in the IntroInfo struct from
    // the last-built oxes file, in preparation for building a new oxes file
	m_pIntroInfo->bChunkExists = FALSE;
	m_pIntroInfo->strChunk.Empty();
	if (!m_pIntroInfo->arrPossibleMarkers.IsEmpty())
	{
		m_pIntroInfo->arrPossibleMarkers.Clear();
	}
	if (!m_pIntroInfo->arrSkipMarkers.IsEmpty())
	{
		m_pIntroInfo->arrSkipMarkers.Clear();
	}
	ClearAIGroupArray(m_pIntroInfo->aiGroupArray);
}

void Usfm2Oxes::ClearAIGroupArray(AIGroupArray& rGrpArray)
{
	if (!rGrpArray.IsEmpty())
	{
		int count = rGrpArray.GetCount();
		int index;
		for (index = 0; index < count; index++)
		{
			aiGroup* pGrp = rGrpArray.Item(index);
			ClearNoteDetails(pGrp->arrNoteDetails);
			delete pGrp;
		}
		rGrpArray.Clear();
	}
}

void Usfm2Oxes::ClearAISectionPartArray(AISectionPartArray& arrSectionParts)
{
	if (!arrSectionParts.IsEmpty())
	{
		int count = arrSectionParts.GetCount();
		int index;
		for (index = 0; index < count; index++)
		{
			SectionPart* pSectionPart = arrSectionParts.Item(index);
			ClearAISectionPart(pSectionPart);
			delete pSectionPart;
		}
		arrSectionParts.Clear();
	}
}

void Usfm2Oxes::ClearAISectionPart(SectionPart* pSectionPart)
{
	pSectionPart->strChunk.Empty();
	if (!pSectionPart->arrPossibleMarkers.IsEmpty())
	{
		pSectionPart->arrPossibleMarkers.Clear();
	}
	if (!pSectionPart->arrSkipMarkers.IsEmpty())
	{
		pSectionPart->arrSkipMarkers.Clear();
	}
	ClearAIGroupArray(pSectionPart->aiGroupArray);
	delete pSectionPart;
}


void Usfm2Oxes::ClearCanonInfo()
{
    // this function cleans out the file-specific data present in the IntroInfo struct from
    // the last-built oxes file, in preparation for building a new oxes file
	m_pCanonInfo->strChunk.Empty();
	ClearAISectionInfoArray(m_pCanonInfo->arrSections);
}

void Usfm2Oxes::ClearAISectionInfoArray(AISectionInfoArray& arrSections)
{
	if (!arrSections.IsEmpty())
	{
		int count = arrSections.GetCount();
		int index;
		for (index = 0; index < count; index++)
		{
			SectionInfo* pSectionInfo = arrSections.Item(index);
			ClearSectionInfo(pSectionInfo); // clears struct then deletes it
		}
		arrSections.Clear();
	}
}

void Usfm2Oxes::ClearSectionInfo(SectionInfo* pSectionInfo)
{
	pSectionInfo->strChunk.Empty();
	ClearAISectionPartArray(pSectionInfo->sectionPartArray); // does nothing if sectionPartArray is empty
	delete pSectionInfo;
}

void Usfm2Oxes::ClearNoteDetails(NoteDetailsArray& rNoteDetailsArray)
{
	if (!rNoteDetailsArray.IsEmpty())
	{
		int count = rNoteDetailsArray.GetCount();
		int index;
		for (index = 0; index < count; index++)
		{
			NoteDetails* pDetails = rNoteDetailsArray.Item(index);
			delete pDetails;
		}
		rNoteDetailsArray.Clear();
	}
}

void Usfm2Oxes::SetOXESVersionNumber(int versionNum)
{
	m_version = versionNum; // set the private member for the version number
}

bool Usfm2Oxes::IsWhiteSpace(wxChar& ch)
{
	if (ch == _T(' ') ||
		ch == _T('\n') ||
		ch == _T('\r') ||
		ch == _T('\t'))
	{
		return TRUE;
	}
	return FALSE;
}

// on entry, pChar must be pointing at a SF marker's backslash
bool Usfm2Oxes::IsSpecialTextStyleMkr(wxChar* pChar)
{
	// Returns TRUE if the marker at pChar is a character formatting marker or
	// a character formatting end marker; these are the markers:
	// \qac \qs \qt \nd \tl \dc \bk \pn \wj \k \no \bd \it \bdit \em \sc and the matching
	// list of endmarkers, and also the extras defined in the creator
	wxChar* ptr = pChar;
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	wxASSERT(pDoc);
	wxASSERT(*ptr == _T('\\')); // we should be pointing at the backslash of a marker

	wxString wholeMkr = pDoc->GetWholeMarker(ptr);
	wholeMkr += _T(' '); // add space
	if (m_specialMkrs.Find(wholeMkr) != wxNOT_FOUND		|| 
		m_specialEndMkrs.Find(wholeMkr) != wxNOT_FOUND)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/* unneeded due to design change
bool Usfm2Oxes::IsMkrAllowedPrecedingParagraphs(wxString& wholeMkr)
{
	if (wholeMkr.Len() < 2)
		return FALSE;
	wxString wholeMkrPlusSpace = wholeMkr + _T(' ');
	size_t offset = m_allowedPreParagraphMkrs.Find(wholeMkrPlusSpace);
	if (offset >= 0)
	{
		return TRUE;
	}
	return FALSE;
}
*/

bool Usfm2Oxes::IsANoteMarker(wxString& wholeMkr)
{
	if (wholeMkr = m_noteMkr)
		return TRUE;
	return FALSE;
}

bool Usfm2Oxes::IsAFreeMarker(wxString& wholeMkr)
{
	if (wholeMkr = m_freeMkr)
		return TRUE;
	return FALSE;
}

/* unneeded due to design change
bool Usfm2Oxes::IsAHaltingMarker_PreFirstParagraph(wxString& wholeMkr)
{
	if (wholeMkr.Len() < 2)
		return FALSE;
	wxString wholeMkrPlusSpace = wholeMkr + _T(' ');
	size_t offset = m_haltingMrks_PreFirstParagraph.Find(wholeMkrPlusSpace);
	if (offset >= 0)
	{
		return TRUE;
	}
	return FALSE;
}
*/

// if str has an exact match in the passed in array, return TRUE, else return FALSE
// BEW 15Sep10, added the protocol that if str is empty, then TRUE is returned immediately
// - so that text without a preceding marker is still considered part of the current
// chunk, as IsOneOf() is used for determining what does or doesn't belong to a chunk
bool Usfm2Oxes::IsOneOf(wxString& str, wxArrayString* pArray, 
				CustomMarkersFT inclOrExclFreeTrans, CustomMarkersN inclOrExclNote, 
				RemarkMarker inclOrExclRemark)
{
	if (str.IsEmpty())
	{
		return TRUE; // the empty string (ie. str is not a marker) indicates membership
					 // in the chunk nevertheless
	}
	size_t count = pArray->GetCount();
	if (count == 0)
		return FALSE;
	size_t index;
	wxString testStr;
	for (index = 0; index < count; index++)
	{
		testStr = pArray->Item(index);
		if (str == testStr)
		{
			return TRUE;
		}
	}
	if (inclOrExclFreeTrans == includeFreeTransInTest)
	{
		// handle \free as an allowed match
		if (str == m_freeMkr)
		{
			return TRUE;
		}
	}
	if (inclOrExclNote == includeNoteInTest)
	{
		// handle \note as an allowed match
		if (str == m_noteMkr)
		{
			return TRUE;
		}
	}
	if (inclOrExclRemark == includeRemarkInTest)
	{
		// handle \rem as an allowed match
		if (str == m_remarkMkr)
		{
			return TRUE;
		}
	}
	return FALSE;
}

// BEW added this function in6Sep10. It is only used in doing an OXES export.
// The commonHaltingMarkers wxString is defined as a global in the
// Adapt_ItView.cpp file, and populated with its markers there at its definition. It's
// original use was in parsing for collecting back translations. The list is copied to a
// private wxString member of the Usfm2Oxes class at initialization, and some extra
// markers added (\h, \note, \free) which are pertinent for OXES parsing of Adapt It
// SFM exported translation text. 
// On entry, pChar can be pointing at the backslash of an SF marker, or at any arbitrary
// character
bool Usfm2Oxes::IsAHaltingMarker(wxChar* pChar, CustomMarkersFT inclOrExclFreeTrans, 
								 CustomMarkersN inclOrExclNote)
{
	// Returns TRUE if the marker at pChar is one which should halt parsing because a
	// marker for a different type of information has unexpectedly been found.
    // The list includes \fe, \f, \x -- no endmarkers are included, as this function looks
    // only for the begining marker, and is a protection against parsing onwards wrongly
    // because the person who did the SFM markup forgot to end content with an endmarker
    // that was expected, such as \f* or \x* etc. Also, no digits are included, so \h2 is
    // represented in the list as \h, \mt3 as \mt, and so forth. So we must exclude digits
    // here from our search string before testing against the list
	wxChar* ptr = pChar;
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	wxASSERT(pDoc);
	if (*ptr != _T('\\'))
	{
		// returning FALSE when not pointing at a marker allows this function to be used
		// in parsing tests
		return FALSE;
	}
	// if control gets here then we must be pointing at a marker - so check if it halts
	// parsing
	wxASSERT(*ptr == _T('\\'));
	wxString wholeMkr = pDoc->GetWholeMarker(ptr);
	// remove final digit if present (we look only for digits 1 to 4 inclusive, USFM
	// doesn't use deeper levels than 4)
	wxChar digits[] = {_T('1'),_T('2'),_T('3'),_T('4')};
	wholeMkr = MakeReverse(wholeMkr);
	wxChar last = wholeMkr[0];
	if (last==digits[0] || last==digits[1] || last==digits[2] || last==digits[3])
	{
		// remove the digit
		wholeMkr = wholeMkr.Mid(1);
	}
	wholeMkr = MakeReverse(wholeMkr);
	wxString wholeMkrPlusSpace = wholeMkr + _T(' '); // add space
	if (m_haltingMarkers.Find(wholeMkrPlusSpace) != wxNOT_FOUND)
	{
		return TRUE;
	}
	if (inclOrExclFreeTrans == includeFreeTransInTest)
	{
		// test for our custom marker \free
		if (wholeMkr == m_freeMkr)
		{
			return TRUE;
		}
	}
	if (inclOrExclNote == includeNoteInTest)
	{
		// test for our custom marker \note
		if (wholeMkr == m_noteMkr)
		{
			return TRUE;
		}
	}
	return FALSE; // no match
}

// a useful overload of the above
bool Usfm2Oxes::IsAHaltingMarker(wxString& buffer, CustomMarkersFT inclOrExclFreeTrans, 
								 CustomMarkersN inclOrExclNote)
{
	const wxChar* pBuffer = buffer.GetData();
	wxChar* pBuff = (wxChar*)pBuffer; // can't be bothered with const_cast<>
	bool bIsHaltingMarker = IsAHaltingMarker(pBuff, inclOrExclFreeTrans, inclOrExclNote);
	return bIsHaltingMarker;
}

/* Unneeded at present time
// This function is only used in doing an OXES export; on entry, pChar must be pointing at
// the backslash of an SF marker
// The embeddedWholeMkrs and embeddedWholeEndMkrs wxStrings are defined as globals in the
// Adapt_ItView.cpp file, and populated with their markers there at their definitions
bool Usfm2Oxes::IsEmbeddedWholeMarker(wxChar* pChar)
{
    // Returns TRUE if the marker at pChar is a an embedded marker or embedded end marker
    // (These are the markers:\fr \fk \fq \fqa \ft \fdc \fv \fm \xo \xt \xk \xq \xdc and
    // the matching list of endmarkers
	wxChar* ptr = pChar;
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	wxASSERT(pDoc);
	wxASSERT(*ptr == _T('\\')); // we should be pointing at the backslash of a marker

	wxString wholeMkr = pDoc->GetWholeMarker(ptr);
	wholeMkr += _T(' '); // add space
	if (embeddedWholeMkrs.Find(wholeMkr) != wxNOT_FOUND		|| 
		embeddedWholeEndMkrs.Find(wholeMkr) != wxNOT_FOUND)
		return TRUE;
	else
		return FALSE;
}
*/

/* deprecated, a generic function, PopulatePossibleMarkers() replaces it
void Usfm2Oxes::PopulateIntroductionPossibleMarkers(wxArrayString& arrMkrs)
{
	arrMkrs.Clear();
	wxStringTokenizer tokens(m_introHaltingMarkers);
	while (tokens.HasMoreTokens())
	{
		wxString mkr = tokens.GetNextToken();
		wxASSERT(mkr[0] == _T('\\'));
		wxASSERT(mkr.Len() >= 3); // all introduction markers are 3 or 
								  // more chars, including backslash
		arrMkrs.Add(mkr);
	}
}
*/
/////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param  strMarkers      ->  space-delimited string of allowed markers for this chunk
///                             (but don't include \note, \free, \rem as these are handled
///                             differently and with more control)
/// \param  chunkType       ->  the chunk type, such as sectionPartChunkType
/// \param  pChunkStruct    ->  the caller's chunk struct, cast to (void*) - we use the
///                             chunkType value in a switch to cast it back to the caller's
///                             passed in struct type before the type is used herein
/// \remarks
/// Used to turn a fast-access string of space delimited USFM markers into a
/// wxArrayString of markers stored within the passed in struct, which the IsOneOf()
/// function will later use for parsing the buffer for this particular type of chunk;
/// sectionPartChunkType actually has 6 subtypes, but this function doesn't need to know
/// that, because the markers that get stored are what the caller provides in param1, and
/// it is the caller that will know what subtype of the struct it is populating
//////////////////////////////////////////////////////////////////////////////////////
void Usfm2Oxes::PopulatePossibleMarkers(wxString& strMarkers, enum ChunkType chunkType, 
										void* pChunkStruct)
{
	switch (chunkType)
	{
	case titleChunkType:
		(static_cast<TitleInfo*>(pChunkStruct))->arrPossibleMarkers.Clear();
		break;
	case introductionChunkType:
		(static_cast<IntroductionInfo*>(pChunkStruct))->arrPossibleMarkers.Clear();
		break;
	case sectionChunkType:
		break; // SectionInfo parsing is handled externally (there's no arrPossibleMarkers
			   // member in a SectionInfo struct
	case sectionPartChunkType:
		// unused
		break;
	case majorOrSeriesChunkType:
		(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers.Clear();
		break;
	case rangeOrPsalmChunkType:
		(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers.Clear();
		break;
	case normalOrMinorChunkType:
		(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers.Clear();
		break;
	case parallelPassageHeadChunkType:
		(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers.Clear();
		break;
	case poetryChunkType:
		(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers.Clear();
		break;
	case paragraphChunkType:
		(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers.Clear();
		break;
	} // end of switch
	// tokenize the fast access string, to make it an array of wholeMarkers
	wxStringTokenizer tokens(strMarkers);
	while (tokens.HasMoreTokens())
	{
		wxString mkr = tokens.GetNextToken();
		wxASSERT(mkr[0] == _T('\\'));
		wxASSERT(mkr.Len() >= 2); // all markers are 2 or more
								  // chars, including backslash
		switch (chunkType)
		{
		case titleChunkType:
			(static_cast<TitleInfo*>(pChunkStruct))->arrPossibleMarkers.Add(mkr);
			break; 
		case introductionChunkType:
			(static_cast<IntroductionInfo*>(pChunkStruct))->arrPossibleMarkers.Add(mkr);
			break;
		case sectionChunkType:
			break; // SectionInfo parsing is handled externally (there's no arrPossibleMarkers
				   // member in a SectionInfo struct
		case sectionPartChunkType:
			// unused
			break;
		case majorOrSeriesChunkType:
			(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers.Add(mkr);
			break;
		case rangeOrPsalmChunkType:
			(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers.Add(mkr);
			break;
		case normalOrMinorChunkType:
			(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers.Add(mkr);
			break;
		case parallelPassageHeadChunkType:
			(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers.Add(mkr);
			break;
		case poetryChunkType:
			(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers.Add(mkr);
			break;
		case paragraphChunkType:
			(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers.Add(mkr);
			break;
		} // end of switch
	} // end of while loop
}

/////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param  strMarkers      ->  space-delimited string of markers for this chunk which occur
///                             before the material of interest, & skipped over -- but are
///                             still to be included in the chunk (e.g. \c before a \ms --
///                             it is the \ms we are interested in, not the \c, at a certain
///                             level of the parsing hierarchy)
///                             (but don't include \note, \free, \rem as these are handled
///                             differently and with more control)
/// \param  chunkType       ->  the chunk type, such as sectionPartChunkType
/// \param  pChunkStruct    ->  the caller's chunk struct, cast to (void*) - we use the
///                             chunkType value in a switch to cast it back to the caller's
///                             passed in struct type before the type is used herein
/// \remarks
/// Used to turn a fast-access string of space delimited USFM markers into a
/// wxArrayString of markers stored within the passed in struct, which the IsOneOf()
/// function will later use for parsing the buffer for this particular type of chunk;
/// markers to be skipped are retained in the chunk, but otherwise just skipped over
/// because the code is interested in markers which follow them for chunking purposes
//////////////////////////////////////////////////////////////////////////////////////
void Usfm2Oxes::PopulateSkipMarkers(wxString& strMarkers, enum ChunkType chunkType, 
										void* pChunkStruct)
{
	switch (chunkType)
	{
	case titleChunkType:
		(static_cast<TitleInfo*>(pChunkStruct))->arrSkipMarkers.Clear();
		break;
	case introductionChunkType:
		(static_cast<IntroductionInfo*>(pChunkStruct))->arrSkipMarkers.Clear();
		break;
	case sectionChunkType:
		break; // SectionInfo parsing is handled externally (there's no arrSkipMarkers
			   // member in a SectionInfo struct
	case sectionPartChunkType:
		// unused
		break;
	case majorOrSeriesChunkType:
		(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers.Clear();
		break;
	case rangeOrPsalmChunkType:
		(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers.Clear();
		break;
	case normalOrMinorChunkType:
		(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers.Clear();
		break;
	case parallelPassageHeadChunkType:
		(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers.Clear();
		break;
	case poetryChunkType:
		(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers.Clear();
		break;
	case paragraphChunkType:
		(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers.Clear();
		break;
	} // end of switch
	// tokenize the fast access string, to make it an array of wholeMarkers
	wxStringTokenizer tokens(strMarkers);
	while (tokens.HasMoreTokens())
	{
		wxString mkr = tokens.GetNextToken();
		wxASSERT(mkr[0] == _T('\\'));
		wxASSERT(mkr.Len() >= 2); // all markers are 2 or more
								  // chars, including backslash
		switch (chunkType)
		{
		case titleChunkType:
			(static_cast<TitleInfo*>(pChunkStruct))->arrSkipMarkers.Add(mkr);
			break;
		case introductionChunkType:
			(static_cast<IntroductionInfo*>(pChunkStruct))->arrSkipMarkers.Add(mkr);
			break;
		case sectionChunkType:
			break; // SectionInfo parsing is handled externally (there's no arrPossibleMarkers
				   // member in a SectionInfo struct
		case sectionPartChunkType:
			// unused
			break;
		case majorOrSeriesChunkType:
			(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers.Add(mkr);
			break;
		case rangeOrPsalmChunkType:
			(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers.Add(mkr);
			break;
		case normalOrMinorChunkType:
			(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers.Add(mkr);
			break;
		case parallelPassageHeadChunkType:
			(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers.Add(mkr);
			break;
		case poetryChunkType:
			(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers.Add(mkr);
			break;
		case paragraphChunkType:
			(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers.Add(mkr);
			break;
		} // end of switch
	} // end of while loop
}

// Chunker has the knowledge of Adapt It's notes, free translations, and PT \rem
// remarks field, these associate with a <trGroup> (which in our parser, is realized
// when chunking as an aiGroup) and precede the text of the group within the input
// data file, so our chunker has to look ahead all the time to work out where the
// chunk ends. 
// Returns TRUE in the bOnlySkippedMaterial if the only material identified and scanned
// over was material stipulated (in pChunkStruct's arrSkipMarkers array member) as to be
// skipped (the caller will need to use this fact to avoid assuming that a certain type of
// chunk was identified and scanned over)
int Usfm2Oxes::Chunker(wxString* pInputBuffer,  enum ChunkType chunkType, 
					   void* pChunkStruct, bool& bOnlySkippedMaterial)
{
	wxASSERT((*pInputBuffer)[0] == _T('\\')); // we must be pointing at a marker
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	bool bEmbeddedSpan = FALSE; // would be true if we parsed a \f, \fe or \x marker
								// but these are unlikely in Title chunks
	bool bHasInlineMarker = FALSE; // needed for \rem parsing -- & is ignored
	int span = 0;
	wxString buff = *pInputBuffer; // work with a copy of the buffer contents until
								   // we are ready to bleed off the title info chunk
	// we need a counter for characters in this particular chunk instance
	int charsDefinitelyInChunk = 0;
	int aCounter = 0; // count using this, when we are not sure that a field will
					  // actually belong in the chunk
	// scratch strings
	wxString wholeEndMkr;
	wxString dataStr;
	bool bMatchedSkipMkr = FALSE;
	bool bMatchedNonSkipMaterial = FALSE;

	// begin...
#ifdef __WXDEBUG__
#ifdef _IntroOverrun
	//if (chunkType == introductionChunkType)
	//{
	//    wxString first2K = buff.Left(2200);
	//    wxLogDebug(_T("****** GetIntroInfoChunk(), FIRST 2,200 characters.\n%s"), first2K.c_str());
	//    wxLogDebug(_T("****** GetIntroInfoChunk(), END OF FIRST 2,200 characters.\n"));
	//}
#endif
#endif

	wxString wholeMkr = pDoc->GetWholeMarker(buff);
	bool bBelongsInChunk = FALSE; // initialize

	// define the skip markers and possible markers arrays
	wxArrayString* pSkipArray = NULL;
	wxArrayString* pPossiblesArray = NULL;
	switch (chunkType)
	{
	case titleChunkType:
		{
			pPossiblesArray = &(static_cast<TitleInfo*>(pChunkStruct))->arrPossibleMarkers;
			pSkipArray = &(static_cast<TitleInfo*>(pChunkStruct))->arrSkipMarkers;
		}
		break;
	case introductionChunkType:
		{
			pPossiblesArray = &(static_cast<IntroductionInfo*>(pChunkStruct))->arrPossibleMarkers; 
			pSkipArray = &(static_cast<IntroductionInfo*>(pChunkStruct))->arrSkipMarkers; 
		}
		break;
	case sectionChunkType:
		break; // SectionInfo chunking is handled externally (there's no
			   // arrPossibleMarkers member in a SectionInfo struct)
	case sectionPartChunkType:
		break; // unused
	case majorOrSeriesChunkType:
	case rangeOrPsalmChunkType:
	case normalOrMinorChunkType:
	case parallelPassageHeadChunkType:
	case poetryChunkType:
	case paragraphChunkType:
		{
			pPossiblesArray = &(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers;
			pSkipArray = &(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers;
		}
		break;
	} // end of switch

    // if there are any markers (and content) to be skipped, but retained in the chunk,
    // test here and do the skips until we come to one which is not to be skipped (we
    // assume there won't be \rem, \free nor \note before any of these -- typically we
    // might want to skip a \c because we are interested in \ms or \s etc which may follow
    // (higher levels of the parse hierarchy should have taken account already of any
    // markers we skip here)
	do {
		bBelongsInChunk = FALSE;
		switch (chunkType)
		{
		case titleChunkType:
			break; // TitleInfo struct doesn't have any skip markers defined
		case introductionChunkType:
			{
				// IntroInfo chunk doesn't have any skip markers defined
				break;
			}
			break;
		case sectionChunkType:
			break; // SectionInfo parsing is handled externally (there's no arrSkipMarkers
				   // member in a SectionInfo struct
		case sectionPartChunkType:
			break; //unused
		case majorOrSeriesChunkType:
		case rangeOrPsalmChunkType:
		case normalOrMinorChunkType:
		case parallelPassageHeadChunkType:
		case poetryChunkType:
		case paragraphChunkType:
			{
				bBelongsInChunk = IsOneOf(wholeMkr, pSkipArray, excludeFreeTransFromTest, 
											excludeNoteFromTest, excludeRemarkFromTest);
			}
			break;
		} // end of switch

		if (bBelongsInChunk)
		{
			if (bBelongsInChunk)
			{
				bMatchedSkipMkr = TRUE; // used later on, further down after loop is finished
			}
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
						bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			charsDefinitelyInChunk += span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
			// get whatever marker is being pointed at now
			wholeMkr = pDoc->GetWholeMarker(buff);
		}
	} while (bBelongsInChunk);

    // we now come to the material this chunk type is really interested in. It may be
    // preceded by one or more \rem fields (data from Paratext can have these), or a \free
    // field, or a \note field - these precede data which we are really wanting to look at,
    // and so we must not commit to counting any of these using the local int
    // charsDefinitelyInChunk until we get to some other field which is found to belong to
    // this chunk; so we keep the tentative count in the other local counter, aCounter
	aCounter = 0;
	do {
		bBelongsInChunk = FALSE; // default, if no marker is found which potentially
								 // belongs in the chunk or definitely belongs, the
								 // loop will terminate
		// NOTE, we know that any \rem, \free...\free*, and/or \note...\note* fields
		// before a marker which is not one of those belongs to this chunk because at the
		// end of the loop we check for loop end condition, jumping over these to find if
		// the first marker which isn't one of these is still in the chunk - if it is, we
		// then iterate the loop and collect those fields, if it isn't, we break out of
		// the loop -- in this way we can be certain we increment charsDefinitelyInChunk
		// only when the material is definitely in the current chunk
		if ( wholeMkr == m_remarkMkr)
		{
            // there could be several in sequence, parse over them all (and later in the
            // parsing operation, at a lower level, we will add each's contents string to
            // the aiGroup's arrRemarks wxArrayString member when we parse to the level of
            // aiGroup structs)
			while (wholeMkr == m_remarkMkr)
			{
				// identify the \rem remarks chunk
				bBelongsInChunk = TRUE;
				span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
							bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
							haltAtRemarkWhenParsing);
				aCounter += span;
				// bleed out the scanned over material
				buff = buff.Mid(span);
				dataStr.Empty();
				// get whatever marker is being pointed at now
				wholeMkr = pDoc->GetWholeMarker(buff);
			}
		}
		// check if we are pointing at a \free marker
		if (m_bContainsFreeTrans && wholeMkr == m_freeMkr)
		{
            // identify the free translation chunk & count its characters; this function
            // always exists with span giving the offset to the marker which halted
            // scanning (or end of buffer)
			bBelongsInChunk = TRUE;
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, 
				dataStr, bEmbeddedSpan, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			aCounter += span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
			// get whatever marker is being pointed at now
			wholeMkr = pDoc->GetWholeMarker(buff);
		}
		else if (m_bContainsNotes && wholeMkr == m_noteMkr)
		{
			bBelongsInChunk = TRUE;
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, 
				dataStr, bEmbeddedSpan, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			aCounter += span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
			// get whatever marker is being pointed at now
			wholeMkr = pDoc->GetWholeMarker(buff);
		}
		else
		{
            // It's possibly some other marker than \free or \note or \rem, (or it might be
            // text - in which case wholeMkr will be empty) so check if it belongs in this
            // type of chunk, if it does, we get charsDefinitelyInChunk updated, get the
            // next wholeMkr, and iterate the loop; if it doesn't belong, break out of the
            // loop. Note, as mentioned above, buff can start with a non-marker, such as
            // the text following a note, and when that happens, we must parse over the
            // text to the next marker and iterate the loop because that text will belong
            // in the chunk if the preceding marker's content belongs in the chunk
			if (!wholeMkr.IsEmpty())
			{
				switch (chunkType)
				{
				case titleChunkType:
					{
						bBelongsInChunk = IsOneOf(wholeMkr, pPossiblesArray, 
							excludeFreeTransFromTest, includeNoteInTest, excludeRemarkFromTest);
					}
					break;
				case introductionChunkType:
					{
						bBelongsInChunk = IsOneOf(wholeMkr, pPossiblesArray,
							excludeFreeTransFromTest, includeNoteInTest, excludeRemarkFromTest);
					}
					break;
				case sectionChunkType:
					break; // SectionInfo chunking is handled externally (there's no
						   // arrPossibleMarkers member in a SectionInfo struct)
				case majorOrSeriesChunkType:
				case rangeOrPsalmChunkType:
				case normalOrMinorChunkType:
				case parallelPassageHeadChunkType:
				case poetryChunkType:
				case paragraphChunkType:
					{
						bBelongsInChunk = IsOneOf(wholeMkr, pPossiblesArray,
							excludeFreeTransFromTest, includeNoteInTest, excludeRemarkFromTest);
					}
					break;
				} // end of switch
				// if it doesn't belong in this chunk then break out of the loop, we've found
				// the start of the next chunk and must parse no further in the current one
				if (!bBelongsInChunk)
				{
					// charsDefinitelyInChunk must not be incremented, just break from loop
					break;
				}
				else
				{
					bMatchedNonSkipMaterial = TRUE;
					// it belongs in the chunk, so accept any of \rem, \free or \note already
					// temporarily parsed over but not yet counted by charsDefinitelyInChunk
					charsDefinitelyInChunk += aCounter;
					aCounter = 0; // re-initialize, for next iteration

					// now parse over the marker, its content and any endmarker, and
					// update buff, to point at the next marker, and get a new wholeMkr
					
					// since there could be a second \note, we parse over any found inline by
					// passing the param ignoreNoteWhenParsing
					span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, 
									dataStr, bEmbeddedSpan, haltAtFreeTransWhenParsing, 
									ignoreNoteWhenParsing, haltAtRemarkWhenParsing);
					charsDefinitelyInChunk += span;
#ifdef __WXDEBUG__
#ifdef _IntroOverrun
					//if (chunkType == introductionChunkType)
					//{
					//	if (wholeMkr == _T("\\io2") && dataStr.Find(_T("B Ghengis")) != wxNOT_FOUND)
					//	{
							int breakPoint_Here = 1;
					//	}
					//}
#endif
#endif
					// bleed out the scanned over material
					buff = buff.Mid(span);
					dataStr.Empty();
					// get whatever marker is being pointed at now
					wholeMkr = pDoc->GetWholeMarker(buff);
					// iterate the loop
				}
			} // end of TRUE block for test: if (!wholeMkr.IsEmpty())
			else
			{
                // buff currently starts with text, not a marker, so accept it so parse to
                // the next marker and iterate the testing loop (we will parse to pEnd if
                // no marker lies ahead, in which case .Find() will return wxNOT_FOUND)
				int myOffset = buff.Find(backslash);
				if (myOffset != wxNOT_FOUND)
				{
					// we found a marker
					charsDefinitelyInChunk += myOffset;
					buff = buff.Mid(myOffset);
					dataStr.Empty();
					wholeMkr = pDoc->GetWholeMarker(buff);
					bBelongsInChunk = TRUE; // iterate the loop
					bMatchedNonSkipMaterial = TRUE;
				}
				else
				{
					// no markers lie ahead, so just accept the rest
					size_t lastStuffLen = buff.Len();
					charsDefinitelyInChunk += lastStuffLen;
					dataStr.Empty();
					bMatchedNonSkipMaterial = TRUE;
					break;
				}
			} // end of else block for test: if (!wholeMkr.IsEmpty())
		} // end of else block for test: else if (m_bContainsNotes && wholeMkr == m_noteMkr)
	} while (bBelongsInChunk); // end of do loop
	if (!bMatchedNonSkipMaterial && bMatchedSkipMkr)
	{
		bOnlySkippedMaterial = TRUE; // tell the caller we scanned only over skippable material
	}
	return charsDefinitelyInChunk;
}

// for poetry chunking; we'll treat a blank line (\b marker) as poetry for chunking
// purposes, but in the OXES productions we are free to call it whatever we like - so
// while a blank is often in extended poetry sections, it can also be between poetry and
// prose - so we'll eventually do whatever the OXES v1 examples do with it
int Usfm2Oxes::PoetryChunker(wxString* pInputBuffer,  enum ChunkType chunkType, 
					void* pChunkStruct, bool& bOnlySkippedMaterial, bool& bBlankLine)
{
	wxASSERT((*pInputBuffer)[0] == _T('\\')); // we must be pointing at a marker
	if (chunkType != poetryChunkType)
	{
		wxMessageBox(_T("PoetryChunker() error: a chunkType other than poetryChunkType was passed in.\nProcessing will continue, but data is likely to be lost from the OXES output."),
						_T(""), wxICON_ERROR);
		return 0;
	}
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	bBlankLine = FALSE; // a \b marker is used for a blank line
	bool bEmbeddedSpan = FALSE;
	bool bHasInlineMarker = FALSE; // needed for \rem parsing -- & is ignored
	int span = 0;
	wxString buff = *pInputBuffer; // work with a copy of the buffer contents
	int charsDefinitelyInChunk = 0;
	int aCounter = 0; // count using this, when we are not sure that a field will
					  // actually belong in the chunk
	// scratch strings
	wxString wholeEndMkr;
	wxString dataStr;
	bool bMatchedSkipMkr = FALSE;
	bool bMatchedNonSkipMaterial = FALSE;

	// define the skip markers and possible markers arrays
	wxArrayString* pSkipArray = NULL;
	wxArrayString* pPossiblesArray = NULL;
	switch (chunkType)
	{
	case titleChunkType:
		{
			pPossiblesArray = &(static_cast<TitleInfo*>(pChunkStruct))->arrPossibleMarkers;
			pSkipArray = &(static_cast<TitleInfo*>(pChunkStruct))->arrSkipMarkers;
		}
		break;
	case introductionChunkType:
		{
			pPossiblesArray = &(static_cast<IntroductionInfo*>(pChunkStruct))->arrPossibleMarkers; 
			pSkipArray = &(static_cast<IntroductionInfo*>(pChunkStruct))->arrSkipMarkers; 
		}
		break;
	case sectionChunkType:
		break; // SectionInfo chunking is handled externally (there's no
			   // arrPossibleMarkers member in a SectionInfo struct)
	case sectionPartChunkType:
		break; // unused
		case majorOrSeriesChunkType:
		case rangeOrPsalmChunkType:
		case normalOrMinorChunkType:
		case parallelPassageHeadChunkType:
		case poetryChunkType:
		case paragraphChunkType:
		{
			pPossiblesArray = &(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers;
			pSkipArray = &(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers;
		}
		break;
	} // end of switch

	// begin...
	wxString wholeMkr = pDoc->GetWholeMarker(buff);
	bool bBelongsInChunk = FALSE; // initialize

    // if there are any markers (and content) to be skipped, but retained in the chunk,
    // test here and do the skips until we come to one which is not to be skipped (we
    // assume there won't be \rem, \free nor \note before any of these -- typically we
    // might want to skip a \c because we are interested in \ms or \s etc which may follow
    // (higher levels of the parse hierarchy should have taken account already of any
    // markers we skip here)
	do {
		bBelongsInChunk = FALSE;
		switch (chunkType)
		{
		case titleChunkType:
			break; // TitleInfo struct doesn't have any skip markers defined
		case introductionChunkType:
			{
				// IntroInfo chunk doesn't have any skip markers defined
				break;
			}
			break;
		case sectionChunkType:
			break; // SectionInfo parsing is handled externally (there's no arrSkipMarkers
				   // member in a SectionInfo struct
		case sectionPartChunkType:
			break; // unused
		case majorOrSeriesChunkType:
		case rangeOrPsalmChunkType:
		case normalOrMinorChunkType:
		case parallelPassageHeadChunkType:
		case poetryChunkType:
		case paragraphChunkType:
			{
				bBelongsInChunk = IsOneOf(wholeMkr, pSkipArray, excludeFreeTransFromTest, 
											excludeNoteFromTest, excludeRemarkFromTest);
			}
			break;
		} // end of switch

		if (bBelongsInChunk)
		{
			if (bBelongsInChunk)
			{
				bMatchedSkipMkr = TRUE;
			}
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
						bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			charsDefinitelyInChunk += span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
			// get whatever marker is being pointed at now
			wholeMkr = pDoc->GetWholeMarker(buff);
		}
	} while (bBelongsInChunk);

    // we now come to the material this chunk type is really interested in. It may be
    // preceded by one or more \rem fields (data from Paratext can have these), or a \free
    // field, or a \note field - these precede data which we are really wanting to look at,
    // and so we must not commit to counting any of these using the local int
    // charsDefinitelyInChunk until we get to some other field which is found to belong to
    // this chunk; so we keep the tentative count in the other local counter, aCounter
	aCounter = 0;
	bool bFoundPoetryMarker = FALSE; // once we've found one, the loop below must no longer
									 // iterate, but instead an inner loop must scan over
									 // all markers until a halting condition is found as 
									 // follows: (1) another poetry marker is found before
									 // coming to a paragraph marker of any kind (any
									 // \rem, \free and or \note before it will belong in 
									 // the NEXT chunk, not this one), or (2) a paragraph
									 // marker is encountered (same comments about \rem,
									 // \free or note apply here too), or (3) buffer end
									 // is encountered
	do { // <<- don't actually need a loop, we match one \q etc, then close off the chunk
		bBelongsInChunk = FALSE; // default, if no marker is found which potentially
								 // belongs in the chunk or definitely belongs, the
								 // loop will terminate
		// NOTE, we know that any \rem, \free...\free*, and/or \note...\note* fields
		// before a marker which is not one of those belongs to this chunk because at the
		// end of the loop we check for loop end condition, jumping over these to find if
		// the first marker which isn't one of these is still in the chunk - if it is, we
		// then iterate the loop and collect those fields, if it isn't, we break out of
		// the loop -- in this way we can be certain we increment charsDefinitelyInChunk
		// only when the material is definitely in the current chunk
		if ( wholeMkr == m_remarkMkr)
		{
            // there could be several in sequence, parse over them all (and later in the
            // parsing operation, at a lower level, we will add each's contents string to
            // the aiGroup's arrRemarks wxArrayString member when we parse to the level of
            // aiGroup structs)
			while (wholeMkr == m_remarkMkr)
			{
				// identify the \rem remarks chunk
				bBelongsInChunk = TRUE;
				span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
							bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
							haltAtRemarkWhenParsing);
				aCounter += span;
				// bleed out the scanned over material
				buff = buff.Mid(span);
				dataStr.Empty();
				// get whatever marker is being pointed at now
				wholeMkr = pDoc->GetWholeMarker(buff);
			}
		}
		// check if we are pointing at a \free marker
		if (m_bContainsFreeTrans && wholeMkr == m_freeMkr)
		{
            // identify the free translation chunk & count its characters; this function
            // always exists with span giving the offset to the marker which halted
            // scanning (or end of buffer)
			bBelongsInChunk = TRUE;
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, 
				dataStr, bEmbeddedSpan, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			aCounter += span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
			// get whatever marker is being pointed at now
			wholeMkr = pDoc->GetWholeMarker(buff);
		}
		else if (m_bContainsNotes && wholeMkr == m_noteMkr)
		{
			bBelongsInChunk = TRUE;
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, 
				dataStr, bEmbeddedSpan, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			aCounter += span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
			// get whatever marker is being pointed at now
			wholeMkr = pDoc->GetWholeMarker(buff);
		}
		else
		{
            // It's possibly some other marker than \free or \note or \rem, (or it might be
            // text - in which case wholeMkr will be empty) so check if it belongs in this
            // type of chunk, if it does, we get charsDefinitelyInChunk updated, get the
            // next wholeMkr, and iterate the loop; if it doesn't belong, break out of the
            // loop. Note, as mentioned above, buff can start with a non-marker, such as
            // the text following a note, and when that happens, we must parse over the
            // text to the next marker and iterate the loop because that text will belong
            // in the chunk if the preceding marker's content belongs in the chunk
			if (!wholeMkr.IsEmpty())
			{
				switch (chunkType)
				{
				case titleChunkType:
					{
						bBelongsInChunk = IsOneOf(wholeMkr, pPossiblesArray, 
							excludeFreeTransFromTest, includeNoteInTest, excludeRemarkFromTest);
					}
					break;
				case introductionChunkType:
					{
						bBelongsInChunk = IsOneOf(wholeMkr, pPossiblesArray,
							excludeFreeTransFromTest, includeNoteInTest, excludeRemarkFromTest);
					}
					break;
				case sectionChunkType:
					break; // SectionInfo chunking is handled externally (there's no
						   // arrPossibleMarkers member in a SectionInfo struct)
				case sectionPartChunkType:
					break; // unused
				case majorOrSeriesChunkType:
				case rangeOrPsalmChunkType:
				case normalOrMinorChunkType:
				case parallelPassageHeadChunkType:
				case poetryChunkType:
				case paragraphChunkType:
					{
						bBelongsInChunk = IsOneOf(wholeMkr, pPossiblesArray,
							excludeFreeTransFromTest, includeNoteInTest, excludeRemarkFromTest);
						if (bBelongsInChunk)
						{
							// we've matched a poetry marker
							bFoundPoetryMarker = TRUE;
						}
					}
					break;
				} // end of switch
				// if it doesn't belong in this chunk then break out of the loop, this
				// isn't a poetry chunk
				if (!bBelongsInChunk && !bFoundPoetryMarker)
				{
					// charsDefinitelyInChunk must not be incremented, just break from
					// loop, because the first non-\rem, non-\free and non-\note wholeMkr 
					// is not one of the poetry markers
					charsDefinitelyInChunk = 0;
					bMatchedNonSkipMaterial = FALSE;
					bOnlySkippedMaterial = FALSE;
					return charsDefinitelyInChunk;
				}
				else
				{
					bBelongsInChunk = FALSE; // this will ensure the loop won't iterate a 2nd time

					bMatchedNonSkipMaterial = TRUE; // we found a poetry marker, so we need 
													// to delineate this marker and its content
													// as the total of this chunk
					if (wholeMkr == _T("\\b") && wholeMkr.Len() == 2)
					{
						// it's a blank line marker, so caller needs to know this so it won't
						// think a zero returned as the character count means that nothing
						// belonging to the poetry marker set was found
						bBlankLine = TRUE; 
					}
					// it belongs in the chunk, so accept any of \rem, \free or \note already
					// temporarily parsed over but not yet counted by charsDefinitelyInChunk
					charsDefinitelyInChunk += aCounter;

                    // now parse forwards, looking for the end of this poetry fragment's
                    // chunk; it will be (1) preceding a following poetry marker provided
                    // no paragraph marker comes first (but beware, prededing \rem \\free
                    // and or \note would belong in the next section, so backparse over
                    // them), or (2) preceding a paragraph marker (same comments re \rem,
                    // etc apply) or (3) end of the data in the section
                    
					int anOffset = wxNOT_FOUND;
					int offset2 = wxNOT_FOUND;
					wxString buff2;
					wxString wholeMkr2; // use this for the marker we hope may end the paragraph 
										// chunk -- ending at it, or preceding \rem, \free,
										// \note fields if present (if they precede, they would
										// belong in next paragraph chunk - so beware)
					wxString wholeMkr2PlusSpace;
					int mkrLen = wholeMkr.Len();
					charsDefinitelyInChunk += mkrLen;
					buff = buff.Mid(mkrLen); // get past the marker
					anOffset = buff.Find(backslash);
					while (anOffset != wxNOT_FOUND && !buff.IsEmpty())
					{
						// found a marker for the potential end location
						buff2 = buff.Mid(anOffset); // buff2 now begins with the backslash
													// where the marker is
						wholeMkr2 = pDoc->GetWholeMarker(buff2);
						wholeMkr2PlusSpace = wholeMkr2 + _T(' ');
						if (pDoc->IsMarker(wholeMkr2))
						{
							// it's a genuine SFM or USFM marker
							offset2 = m_poetryMkrs.Find(wholeMkr2PlusSpace); // is it a poetry marker
							if (offset2 != wxNOT_FOUND)
							{
								// this wholeMkr2 marker is one of the poetry set of
								// markers, so we've found the start (unless \rem, \free
								// and or \note fields precede it - these would belong to
								// information within the next paragraph and so we'd have
								// to parse back over them to find the actual start of
								// that next paragraph - which gives us also the end of
								// the current one)
								int backSpan = BackParseOverNoteFreeRem(buff2, anOffset);
								// there must be something left which isn't \rem or \free or \note data
								wxASSERT( anOffset > backSpan);
								// the end of this paragraph chunk is given by the difference
								charsDefinitelyInChunk += anOffset - backSpan;
								return charsDefinitelyInChunk;
							}
							else
							{
								// wxNOT_FOUND was returned, so wholeMkr2 is not a poetry
								// marker, so check if it's another paragraph marker --
								// and if it is, do the same BackParse...() call as above
								// and for the same reason
								offset2 = m_paragraphMkrs.Find(wholeMkr2PlusSpace); // is it a paragraph marker
								if (offset2 != wxNOT_FOUND)
								{
									// this wholeMkr2 marker is one of the paragraph set of
									// markers, so we've found the start (unless \rem, \free
									// and or \note fields precede it - these would belong to
									// information within the next paragraph and so we'd have
									// to parse back over them to find the actual start of
									// that next paragraph - which gives us also the end of
									// the current one)
									int backSpan = BackParseOverNoteFreeRem(buff2, anOffset);
									// there must be something left which isn't \rem or \free or \note data
									wxASSERT( anOffset > backSpan);
									// the end of this paragraph chunk is given by the difference
									charsDefinitelyInChunk += anOffset - backSpan;
									return charsDefinitelyInChunk;
								}
								else
								{
									// wxNOT_FOUND was returned, so wholeMkr2 is not a paragraph
									// marker nor a poetry marker. Since we found a marker, we know we are
									// not at the end of the section chunk - so wholeMkr2
									// is in the current paragraph chunk, therefore
									// iterate the loop after updating buff and counting
									// the characters in the subspan indicated by anOffset
									buff = buff.Mid(anOffset);
									charsDefinitelyInChunk += anOffset;
									mkrLen = wholeMkr2.Len(); // this one is in the current SectionPart
									buff = buff.Mid(mkrLen); // get past the marker
									charsDefinitelyInChunk += mkrLen;
									anOffset = buff.Find(backslash);
									// iterate loop
								}
							} // end of else block for test: if (offset2 != wxNOT_FOUND),
							  // testing for poetry mkr
						} // end of TRUE block for test: if (pDoc->IsMarker(wholeMkr2))
						else
						{
							// a bogus marker, perhaps a stray backslash -- move over it
							// and continue looping, assume this data is in the current
							// Section Part
							buff = buff.Mid(anOffset + 1);
							charsDefinitelyInChunk += anOffset + 1; // include the backslash
							anOffset = buff.Find(backslash);
							// iterate loop
						}
					} // end of inner loop, while (anOffset != wxNOT_FOUND && !buff.IsEmpty()),
					  // searched for backslash 
					// after the loop, just accept the rest since there are no more backslashes
					int aLength = buff.Len();
					charsDefinitelyInChunk += aLength;
					return charsDefinitelyInChunk;


				} // end of else block for test: if (!bBelongsInChunk && !bFoundPoetryMarker)
			} // end of TRUE block for test: if (!wholeMkr.IsEmpty())
			else
			{
				// buff currently starts with text, not a marker, so don't accept it
				// because poetry must begin with one of the poetry markers
				charsDefinitelyInChunk = 0;
				bMatchedNonSkipMaterial = FALSE;
				bOnlySkippedMaterial = FALSE;
				bBlankLine = FALSE;
				return charsDefinitelyInChunk;
			} // end of else block for test: if (!wholeMkr.IsEmpty())
		} // end of else block for test: else if (m_bContainsNotes && wholeMkr == m_noteMkr)
	} while (bBelongsInChunk); // end of do loop  <<- don't actually need a loop, we match one
										// \q etc only & then close off the chunk

	if (!bMatchedNonSkipMaterial && bMatchedSkipMkr)
	{
		bOnlySkippedMaterial = TRUE; // tell the caller we scanned only over skippable material
	}
	return charsDefinitelyInChunk;
}

// for paragraph chunking (we assume blank lines don't belong in these - so if there are
// any, they'll be picked up as a minimal poetry chunk instead)
int Usfm2Oxes::ParagraphChunker(wxString* pInputBuffer,  enum ChunkType chunkType, 
								void* pChunkStruct, bool& bOnlySkippedMaterial)
{
    // for this chunker, allow buff to start with a marker or text - so don't assert a
    // marker is at its start
	if (chunkType != paragraphChunkType)
	{
		// just a message for the developer
		wxMessageBox(_T("ParagraphChunker() error: a chunkType other than paragraphChunkType was passed in.\nProcessing will continue, but data is likely to be lost from the OXES output."),
						_T(""), wxICON_ERROR);
		return 0;
	}
	if (chunkType != paragraphChunkType)
	{
		// just a message for the developer
		wxMessageBox(_T("ParagraphChunker() error: a chunkType other than paragraphChunkType was passed in.\nProcessing will continue, but data is likely to be lost from the OXES output."),
						_T(""), wxICON_ERROR);
		return 0;
	}
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	bool bEmbeddedSpan = FALSE;
	bool bHasInlineMarker = FALSE; // needed for \rem parsing -- & is ignored
	int span = 0;
	wxString buff = *pInputBuffer; // work with a copy of the buffer contents
	int charsDefinitelyInChunk = 0;
	int aCounter = 0; // count using this, when we are not sure that a field will
					  // actually belong in the chunk
	// scratch strings
	wxString wholeEndMkr;
	wxString dataStr;
	bool bMatchedSkipMkr = FALSE;
	bool bMatchedNonSkipMaterial = FALSE;

	// define the skip markers and possible markers arrays
	wxArrayString* pSkipArray = NULL;
	wxArrayString* pPossiblesArray = NULL;
	switch (chunkType)
	{
	case titleChunkType:
		{
			pPossiblesArray = &(static_cast<TitleInfo*>(pChunkStruct))->arrPossibleMarkers;
			pSkipArray = &(static_cast<TitleInfo*>(pChunkStruct))->arrSkipMarkers;
		}
		break;
	case introductionChunkType:
		{
			pPossiblesArray = &(static_cast<IntroductionInfo*>(pChunkStruct))->arrPossibleMarkers; 
			pSkipArray = &(static_cast<IntroductionInfo*>(pChunkStruct))->arrSkipMarkers; 
		}
		break;
	case sectionChunkType:
		break; // SectionInfo chunking is handled externally (there's no
			   // arrPossibleMarkers member in a SectionInfo struct)
	case sectionPartChunkType:
		break; // unused
		case majorOrSeriesChunkType:
		case rangeOrPsalmChunkType:
		case normalOrMinorChunkType:
		case parallelPassageHeadChunkType:
		case poetryChunkType:
		case paragraphChunkType:
		{
			pPossiblesArray = &(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers;
			pSkipArray = &(static_cast<SectionPart*>(pChunkStruct))->arrSkipMarkers;
		}
		break;
	} // end of switch

	// begin...
	wxString wholeMkr = pDoc->GetWholeMarker(buff);
	bool bBelongsInChunk = FALSE; // initialize

    // if there are any markers (and content) to be skipped, but retained in the chunk,
    // test here and do the skips until we come to one which is not to be skipped (we
    // assume there won't be \rem, \free nor \note before any of these -- typically we
    // might want to skip a \c because we are interested in \ms or \s etc which may follow
    // (higher levels of the parse hierarchy should have taken account already of any
    // markers we skip here)
	do {
		bBelongsInChunk = FALSE;
		switch (chunkType)
		{
		case titleChunkType:
			break; // TitleInfo struct doesn't have any skip markers defined
		case introductionChunkType:
			{
				// IntroInfo chunk doesn't have any skip markers defined
				break;
			}
			break;
		case sectionChunkType:
			break; // SectionInfo parsing is handled externally (there's no arrSkipMarkers
				   // member in a SectionInfo struct
		case sectionPartChunkType:
			break; // unused
		case majorOrSeriesChunkType:
		case rangeOrPsalmChunkType:
		case normalOrMinorChunkType:
		case parallelPassageHeadChunkType:
		case poetryChunkType:
		case paragraphChunkType:
			{
				bBelongsInChunk = IsOneOf(wholeMkr, pSkipArray, excludeFreeTransFromTest, 
											excludeNoteFromTest, excludeRemarkFromTest);
			}
			break;
		} // end of switch

		if (bBelongsInChunk)
		{
			if (bBelongsInChunk)
			{
				bMatchedSkipMkr = TRUE; // used later below, after loop exits
			}
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
						bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			charsDefinitelyInChunk += span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
			// get whatever marker is being pointed at now
			wholeMkr = pDoc->GetWholeMarker(buff);
		}
	} while (bBelongsInChunk);

    // we now come to the material this chunk type is really interested in. It may be
    // preceded by one or more \rem fields (data from Paratext can have these), or a \free
    // field, or a \note field - these precede data which we are really wanting to look at,
    // and so we must not commit to counting any of these using the local int
    // charsDefinitelyInChunk until we get to some other field which is found to belong to
    // this chunk; so we keep the tentative count in the other local counter, aCounter
	aCounter = 0;
	do {
		bBelongsInChunk = FALSE; // default, if no marker is found which potentially
								 // belongs in the chunk or definitely belongs, the
								 // loop will terminate
		// NOTE, we know that any \rem, \free...\free*, and/or \note...\note* fields
		// before a marker which is not one of those belongs to this chunk because at the
		// end of the loop we check for loop end condition, jumping over these to find if
		// the first marker which isn't one of these is still in the chunk - if it is, we
		// then iterate the loop and collect those fields, if it isn't, we break out of
		// the loop -- in this way we can be certain we increment charsDefinitelyInChunk
		// only when the material is definitely in the current chunk
		if ( wholeMkr == m_remarkMkr)
		{
            // there could be several in sequence, parse over them all (and later in the
            // parsing operation, at a lower level, we will add each's contents string to
            // the aiGroup's arrRemarks wxArrayString member when we parse to the level of
            // aiGroup structs)
			while (wholeMkr == m_remarkMkr)
			{
				// identify the \rem remarks chunk
				bBelongsInChunk = TRUE;
				span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
							bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
							haltAtRemarkWhenParsing);
				aCounter += span;
				// bleed out the scanned over material
				buff = buff.Mid(span);
				dataStr.Empty();
				// get whatever marker is being pointed at now
				wholeMkr = pDoc->GetWholeMarker(buff);
			}
		}
		// check if we are pointing at a \free marker
		if (m_bContainsFreeTrans && wholeMkr == m_freeMkr)
		{
            // identify the free translation chunk & count its characters; this function
            // always exists with span giving the offset to the marker which halted
            // scanning (or end of buffer)
			bBelongsInChunk = TRUE;
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, 
				dataStr, bEmbeddedSpan, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			aCounter += span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
			// get whatever marker is being pointed at now
			wholeMkr = pDoc->GetWholeMarker(buff);
		}
		else if (m_bContainsNotes && wholeMkr == m_noteMkr)
		{
			bBelongsInChunk = TRUE;
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, 
				dataStr, bEmbeddedSpan, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			aCounter += span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
			// get whatever marker is being pointed at now
			wholeMkr = pDoc->GetWholeMarker(buff);
		}
		else
		{
            // It's possibly some other marker than \free or \note or \rem, (or it might be
            // text - in which case wholeMkr will be empty) so check if it belongs in this
            // type of chunk, if it does, we get charsDefinitelyInChunk updated, get the
            // next wholeMkr, and iterate the loop; if it doesn't belong, break out of the
            // loop. Note, as mentioned above, buff can start with a non-marker, such as
            // the text following a note, and when that happens, we must parse over the
            // text to the next marker and iterate the loop because that text will belong
            // in the chunk if the preceding marker's content belongs in the chunk
			if (!wholeMkr.IsEmpty())
			{
				switch (chunkType)
				{
				case titleChunkType:
					{
						bBelongsInChunk = IsOneOf(wholeMkr, pPossiblesArray, 
							excludeFreeTransFromTest, includeNoteInTest, excludeRemarkFromTest);
					}
					break;
				case introductionChunkType:
					{
						bBelongsInChunk = IsOneOf(wholeMkr, pPossiblesArray,
							excludeFreeTransFromTest, includeNoteInTest, excludeRemarkFromTest);
					}
					break;
				case sectionChunkType:
					break; // SectionInfo chunking is handled externally (there's no
						   // arrPossibleMarkers member in a SectionInfo struct)
				case sectionPartChunkType:
					break; // unused
					case majorOrSeriesChunkType:
					case rangeOrPsalmChunkType:
					case normalOrMinorChunkType:
					case parallelPassageHeadChunkType:
					case poetryChunkType:
					case paragraphChunkType:
					{
						// here, pPossiblesArray will contain paragraph markers
						bBelongsInChunk = IsOneOf(wholeMkr, pPossiblesArray,
							excludeFreeTransFromTest, includeNoteInTest, excludeRemarkFromTest);
					}
					break;
				} // end of switch
				// ParagraphChunker() will only be called to chunk a section into
				// paragraphs; so we'll allow for a markup error (someone forgot to have a
				// paragraph marker after, say, poetry, or header type info), and accept
				// both flag values for the return from IsOneOf(), and just include what
				// follows to the next paragraph marker, or section end, as a paragraph...
				if (!bBelongsInChunk || bBelongsInChunk)
				{
					bMatchedNonSkipMaterial = TRUE;

					// it belongs in the chunk, so accept any of \rem, \free or \note already
					// temporarily parsed over but not yet counted by charsDefinitelyInChunk
					charsDefinitelyInChunk += aCounter;

					// now parse forwards, looking for the end of this paragraph fragment's
					// chunk; it will be at or immediately preceding one of the following:
					// (1) any of the poetry markers which may lie ahead, provided the poetry
					// marker precedes a paragraph marker of any type
					// (2) the next paragraph marker, of any type
					// (3) end of the data in the section

					int anOffset = wxNOT_FOUND;
					int offset2 = wxNOT_FOUND;
					wxString buff2;
					wxString wholeMkr2; // use this for the marker we hope may end the paragraph 
										// chunk -- ending at it, or preceding \rem, \free,
										// \note fields if present (if they precede, they would
										// belong in next paragraph chunk - so beware)
					wxString wholeMkr2PlusSpace;
					int mkrLen = wholeMkr.Len();
					charsDefinitelyInChunk += mkrLen;
					buff = buff.Mid(mkrLen); // get past the marker
					anOffset = buff.Find(backslash);
					while (anOffset != wxNOT_FOUND && !buff.IsEmpty())
					{
						// found a marker for the potential end location
						buff2 = buff.Mid(anOffset); // buff2 now begins with the backslash
													// where the marker is
						wholeMkr2 = pDoc->GetWholeMarker(buff2);
						wholeMkr2PlusSpace = wholeMkr2 + _T(' ');
						if (pDoc->IsMarker(wholeMkr2))
						{
							// it's a genuine SFM or USFM marker
							offset2 = m_poetryMkrs.Find(wholeMkr2PlusSpace); // is it a poetry marker
							if (offset2 != wxNOT_FOUND)
							{
								// this wholeMkr2 marker is one of the poetry set of
								// markers, so we've found the start (unless \rem, \free
								// and or \note fields precede it - these would belong to
								// information within the next paragraph and so we'd have
								// to parse back over them to find the actual start of
								// that next paragraph - which gives us also the end of
								// the current one)
								int backSpan = BackParseOverNoteFreeRem(buff2, anOffset);
								// there must be something left which isn't \rem or \free or \note data
								wxASSERT( anOffset > backSpan);
								// the end of this paragraph chunk is given by the difference
								charsDefinitelyInChunk += anOffset - backSpan;
								return charsDefinitelyInChunk;
							}
							else
							{
								// wxNOT_FOUND was returned, so wholeMkr2 is not a poetry
								// marker, so check if it's another paragraph marker --
								// and if it is, do the same BackParse...() call as above
								// and for the same reason
								offset2 = m_paragraphMkrs.Find(wholeMkr2PlusSpace); // is it a paragraph marker
								if (offset2 != wxNOT_FOUND)
								{
									// this wholeMkr2 marker is one of the paragraph set of
									// markers, so we've found the start (unless \rem, \free
									// and or \note fields precede it - these would belong to
									// information within the next paragraph and so we'd have
									// to parse back over them to find the actual start of
									// that next paragraph - which gives us also the end of
									// the current one)
									int backSpan = BackParseOverNoteFreeRem(buff2, anOffset);
									// there must be something left which isn't \rem or \free or \note data
									wxASSERT( anOffset > backSpan);
									// the end of this paragraph chunk is given by the difference
									charsDefinitelyInChunk += anOffset - backSpan;
									return charsDefinitelyInChunk;
								}
								else
								{
									// wxNOT_FOUND was returned, so wholeMkr2 is not a paragraph
									// marker nor a poetry marker. Since we found a marker, we know we are
									// not at the end of the section chunk - so wholeMkr2
									// is in the current paragraph chunk, therefore
									// iterate the loop after updating buff and counting
									// the characters in the subspan indicated by anOffset
									buff = buff.Mid(anOffset);
									charsDefinitelyInChunk += anOffset;
									mkrLen = wholeMkr2.Len(); // this one is in the current SectionPart
									buff = buff.Mid(mkrLen); // get past the marker
									charsDefinitelyInChunk += mkrLen;
									anOffset = buff.Find(backslash);
									// iterate loop
								}
							} // end of else block for test: if (offset2 != wxNOT_FOUND),
							  // testing for poetry mkr
						} // end of TRUE block for test: if (pDoc->IsMarker(wholeMkr2))
						else
						{
							// a bogus marker, perhaps a stray backslash -- move over it
							// and continue looping, assume this data is in the current
							// Section Part
							buff = buff.Mid(anOffset + 1);
							charsDefinitelyInChunk += anOffset + 1; // include the backslash
							anOffset = buff.Find(backslash);
							// iterate loop
						}
					} // end of inner loop, while (anOffset != wxNOT_FOUND && !buff.IsEmpty()),
					  // searched for backslash 
					// after the loop, just accept the rest since there are no more backslashes
					int aLength = buff.Len();
					charsDefinitelyInChunk += aLength;
					return charsDefinitelyInChunk;
				} // end of TRUE block for the always succeeding test: 	if (!bBelongsInChunk || bBelongsInChunk)
			} // end of TRUE block for test: if (!wholeMkr.IsEmpty())
			else
			{
				// buff currently starts with text, not a marker, ... we'll treat it
				// as within the current paragraph...so count up to the next backslash and
				// iterate, but if there is none, then count the result of buff contents
				// and return
				int anOffset2 = buff.Find(backslash);
				if (anOffset2 == wxNOT_FOUND)
				{
					// no more markers in the section, so just accept the result
					int aLength = buff.Len();
					charsDefinitelyInChunk += aLength;
					return charsDefinitelyInChunk;
				}
				else
				{
					// found a backslash, so it's probably a marker -- accept characters
					// up to it and iterate
					charsDefinitelyInChunk += anOffset2;
					buff = buff.Mid(anOffset2);
				}
			} // end of else block for test: if (!wholeMkr.IsEmpty())
		} // end of else block for test: else if (m_bContainsNotes && wholeMkr == m_noteMkr)
	} while (!buff.IsEmpty()); // end of do loop
	if (!bMatchedNonSkipMaterial && bMatchedSkipMkr)
	{
		bOnlySkippedMaterial = TRUE; // tell the caller we scanned only over skippable material
	}
	return charsDefinitelyInChunk;
}

// a function to back-parse over any \rem, \free, \note which precedes the match
// location, returning the number of characters back-parsed over; order is significant,
// first over one or more notes (if present), then over a single free translation field
// (if present), and then over one or more remark fields (if present) -- once we are over
// that lot (if present) we are at the end of a chunk; nMatchLocation will be the location
// at which a potential chunking ending marker was found ("potential" because \rem, \free
// and or \note from the following chunk may preceded it -- hence this back-parsing
// function in order to get back to data we are certain is within our chunk)
int Usfm2Oxes::BackParseOverNoteFreeRem(wxString& buff, int nMatchLocation)
{
	wxString strSpan = buff.Left(nMatchLocation);
	int spanLen = strSpan.Len();
	// construct the reversed marker strings
	wxString revNote = _T("eton\\");
	wxString revFree = _T("eerf\\");
	wxString revRem = _T("mer\\");
	// reverse the span in question
	MakeReverse(strSpan);
	int lastCount = 0;
	int offset = FindFromPos(strSpan, revNote, 0);
	// scan back over as many consecutive reversed note markers as there are
	while (offset != wxNOT_FOUND && offset < spanLen)
	{
		lastCount = offset + 5;
		offset = FindFromPos(strSpan, revNote, lastCount);
	}
	// when that loop exits, lastCount has the count to just after the last-matched
	// reversed \note marker; so next we check for a single reversed \free marker
	// immediately following
	offset = FindFromPos(strSpan, revFree, lastCount);
	if (offset != wxNOT_FOUND && offset < spanLen)
	{
		lastCount += offset + 5; // 5 is the size of \free
	}
	// finally, parse over any consecutive reversed \rem fields
	offset = FindFromPos(strSpan, revRem, lastCount);
	while (offset != wxNOT_FOUND && offset < spanLen)
	{
		lastCount = offset + 5;
		offset = FindFromPos(strSpan, revRem, lastCount);
	}
	return lastCount;
}



////////////////////////////////////////////////////////////////////////////////////
/// \return     a pointer to the remainder of the input (U)SFM text after the title info
///             chunk has been removed
/// \param  pInputBuffer    ->  pointer to the wxString which stores the exported (U)SFM
///                             text that is being decomposed in order to form OXES xml
/// \remarks
/// The TitleInfo chunk boundary is determined (by the first marker encountered which does
/// not belong to the TitleInfo chunk), removed from the input buffer string and stored in
/// pTitleInfo, and the shortened pInputBuffer content is returned to the caller -
/// bleeding out this chunk from the input data. The internal decomposition of this chunk
/// is not done here, but in the next function to be called in the caller. The function
/// uses the private struct, m_pTitleInfo
/// BEW created 3Sep10
/////////////////////////////////////////////////////////////////////////////////////
wxString* Usfm2Oxes::GetTitleInfoChunk(wxString* pInputBuffer)
{
	wxString buff = *pInputBuffer; // work with a copy of the buffer contents until
								   // we are ready to bleed off the title info chunk
	// we need a counter for characters in the TitleInfo chunk
	int charsDefinitelyInChunk = 0;

	// delineate the next intoduction chunk
	bool bOnlySkippedMaterial = FALSE;
	charsDefinitelyInChunk = Chunker(&buff, titleChunkType, (void*)m_pTitleInfo,
									bOnlySkippedMaterial);
	// ignore the returned bOnlySkippedMaterial value, we haven't stipulated any skippable
	// markers for a TitleInfo chunk

    // when control gets to here, we've just identified a SF marker which does not belong
    // within the TitleInfo chunk; it such a marker's content had a free translation or
    // note or both defined for it's information then we just throw away the character
    // counts for such info types - so we've nothing to do now other than use the current
    // charsDefinitelyInChunk value to bleed off the chunk from pInputBuffer and store it
	m_pTitleInfo->strChunk = (*pInputBuffer).Left(charsDefinitelyInChunk);
	(*pInputBuffer) = (*pInputBuffer).Mid(charsDefinitelyInChunk);

	if (charsDefinitelyInChunk > 0)
		m_pTitleInfo->bChunkExists = TRUE;

	// check it works? - yes it does
#ifdef __WXDEBUG__
	wxLogDebug(_T("\nGetTitleInfoChunk()  num chars = %d\n%s"),charsDefinitelyInChunk, m_pTitleInfo->strChunk.c_str());
#endif
	return pInputBuffer;
}

// parse from the start of the (shortened) buffer contents, to get any information
// belonging to a book introduction into the IntroInfo struct's strChunk member (but
// parsing of the chunk to get a series of aiGroup structs is done in a separate function)
// and bleed off from the buffer any such information parsed over
wxString* Usfm2Oxes::GetIntroInfoChunk(wxString* pInputBuffer)
{
	wxString buff = *pInputBuffer; // work with a copy of the buffer contents until
						// we are ready to bleed off the introduction info chunk
	// we need a counter for characters in the IntroInfo chunk
	int charsDefinitelyInChunk = 0;
	
	// delineate the next intoduction chunk
	bool bOnlySkipMarkers = FALSE;
	charsDefinitelyInChunk = Chunker(&buff, introductionChunkType, (void*)m_pIntroInfo,
										bOnlySkipMarkers);
	// ignore the returned value of bOnlySkipMarkers because we haven't stipulated any
	// skippable markers for IntroInfo structs

    // when control gets to here, we've just identified a SF marker which does not belong
    // within the introduction chunk; it such a marker's content had a free translation or
    // note or both defined for it's information then we just throw away the character
    // counts for such info types - so we've nothing to do now other than use the current
    // charsDefinitelyInChunk value to bleed off the chunk from pInputBuffer and store it
	m_pIntroInfo->strChunk = (*pInputBuffer).Left(charsDefinitelyInChunk);
	(*pInputBuffer) = (*pInputBuffer).Mid(charsDefinitelyInChunk);

	if (charsDefinitelyInChunk > 0)
		m_pIntroInfo->bChunkExists = TRUE;

	// check it works? - yes it does
#ifdef __WXDEBUG__
	wxLogDebug(_T("\nGetIntroInfoChunk  num chars = %d\n%s"),charsDefinitelyInChunk, m_pIntroInfo->strChunk.c_str());
#endif
	return pInputBuffer;
}

wxString Usfm2Oxes::DoOxesExport(wxString& buff)
{
	m_pBuffer = &buff;

	m_bookID.Empty(); // clear the 3-letter book code

// *** TODO ****
// Get the current book's 3-letter code and store in the class's bookID member

	// more per-OXES-export initializations
	m_bContainsFreeTrans = (m_pBuffer->Find(m_freeMkr) != wxNOT_FOUND) ? TRUE : FALSE; 
	m_bContainsNotes = (m_pBuffer->Find(m_noteMkr) != wxNOT_FOUND) ? TRUE : FALSE; 
	 
	// get the text of USFM marked up translation etc
	wxString mkr; // use this for storing a parsed start marker
	wxString endMkr; // store a parsed end marker here
	wxString dataStr; // store a marker's associated text here

	wxString oxesStr; // collect the xml productions here
	if (m_version == 2)
	{
		// *** TODO *** support Oxes version 2 when the standard stabilizes
		wxString msg;
		msg = msg.Format(_T("Support for OXES version 2 will come in a later version of Adapt It."));
		wxMessageBox(msg,_T("Not yet supported"),wxICON_INFORMATION);
		return *m_pBuffer; // just return the unconverted SF export
	}
	else
	{
		// This block supports Oxes version 1
		 
		// test code -- it works
		//wxString free = _T("\\free");
		//CBString asciiMkr = toUTF8(free);

        // *** DO NOT call the next two -- these are initialized previously and only once
        //     in OnInit(), and calling a clearing function here on them will remove the
        //     markers in the arrays! ***
		// ClearTitleInfo();
		// ClearIntroInfo();

		// make sure the parser starts with a start marker (typically \id) at the
		// beginning of the buffer
		m_pBuffer->Trim(FALSE); // trim white space from the left
		int offset = wxNOT_FOUND;
		offset = m_pBuffer->Find(_T('\\'));
		if (offset == wxNOT_FOUND)
		{
			wxString msg;
			msg = msg.Format(_T("The data does not contain standard format markup. It cannot be converted to the Open XML for Editing Scripture format."));
			wxMessageBox(msg,_T("Data is unsuitable for OXES export"),wxICON_WARNING);
			return *m_pBuffer; // just return the unconverted exported text
		}
		else
		{
			// throw away anything preceding the first backslash
			if (offset > 0)
			{
				m_pBuffer->Mid(offset);
			}
		}
		// Parsing can now begin. Our first task is to get the Title information's chunk
		m_pBuffer = GetTitleInfoChunk(m_pBuffer); // returns input less the chunk
		// and parse it into an Object array of aiGroup structs - one per main marker,
		// with preceding free translation if present, and zero or more embedded notes
		ParseTitleInfoForAIGroupStructs();		

		// Next job is to get the Introduction information's chunk
		m_pBuffer = GetIntroInfoChunk(m_pBuffer); // returns input less the chunk
		// and parse it into an Object array of aiGroup structs - one per main marker,
		// with preceding free translation if present, and zero or more embedded notes
		ParseIntroInfoForAIGroupStructs();

		// what remains in m_pBuffer is canonical information, so it belongs in
		// m_pCanonInfo->strChunk and we'll then parse that to obtain a series of sections
		// - each section will ultimately be a hierarchy of structs, as we parse the
		// contained information to finer levels of granularity
		m_pCanonInfo->strChunk = *m_pBuffer;
		m_pBuffer->Empty();
		// parse the canon into sections
		bool bAllWentWell = ParseCanonIntoSections(m_pCanonInfo);
		if (!bAllWentWell)
		{
			// an error message has been seen already, so just return an empty string
			oxesStr.Empty();
			return oxesStr;
		}




	}
	return oxesStr;
}

// need this utility in order to convert Unicode (UTF-16) to UTF-8
CBString Usfm2Oxes::toUTF8(wxString& str)
{
	wxCharBuffer myBuff(str.utf8_str());
	CBString u8Str(myBuff);
	return u8Str;
}


void Usfm2Oxes::WarnNoEndmarkerFound(wxString endMkr, wxString content)
{
	wxString msg;
	msg = msg.Format(_T("Warning: an end marker for the marker: %s , was not found. Processing will continue. \n(The end marker was absent where the following text occurs: %s)"),
		endMkr.c_str(), content.c_str());
	wxMessageBox(msg, _T(""), wxICON_WARNING);
}

void Usfm2Oxes::WarnAtBufferEndWithoutFindingEndmarker(wxString endMkr)
{
	wxString msg;
	msg = msg.Format(_T("Warning: processing reached the end of the data and an end marker for the marker: %s , was expected but not found.\nProcessing will continue."),
		endMkr.c_str());
	wxMessageBox(msg, _T(""), wxICON_WARNING);
}

void Usfm2Oxes::InitializeSectionInfo(SectionInfo* pSectionInfo)
{
	pSectionInfo->strChunk.Empty();
	pSectionInfo->strChapterNumAtSectionStart.Empty();
	pSectionInfo->strChapterNumAtSectionEnd.Empty();
	pSectionInfo->bChapterChangesMidSection = FALSE;
	pSectionInfo->bChapterStartsAtSectionStart = FALSE;
	pSectionInfo->bChapterEndsAtSectionEnd = FALSE;
}

wxString Usfm2Oxes::GetChapterNumber(wxString& subStr)
{
	wxString str = subStr; // make a local copy (unnecessary but harmless)
	wxStringTokenizer tokens(str);
	wxString chapterNumStr = tokens.GetNextToken();
	wxASSERT(!chapterNumStr.IsEmpty());
	return chapterNumStr;
}

bool Usfm2Oxes::IsNormalSectionMkr(wxString& buffer)
{
	wxASSERT(buffer.Find(backslash) == 0); // backslash must be at buffer start
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	wxString wholeMkr = pDoc->GetWholeMarker(buffer);
	int length = wholeMkr.Len();
	if (wholeMkr.Find(_T("\\s")) == 0)
	{
		// it begins with the two characters \s, but that isn't sufficient
		if (length == 2)
		{
			return TRUE;
		}
		else
		{
			// check for the digits 1 2 or 3 at offset 2
			if (length == 3 && (wholeMkr[2] == _T('1') || 
				wholeMkr[2] == _T('2') || wholeMkr[2] == _T('3')))
			{
				return TRUE;
			}
		}
	}
	return FALSE; // it's not one of \s or \s1 or \s2 or \s3
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
/// \param  bEmbeddedSpan   <-  TRUE if mkr is one or \f, \fe or \x (that is, footnote,
///                             endnote or cross-reference)
/// \param  inclOrExclFreeTrans ->  either includeFreeTrans, or excludeFreeTrans
/// \param  inclOrExclNote ->  either includeNote, or excludeNote
/// \remarks
/// On entry, buffer is the whole of the remaining unparsed adaptation's content, and it's
/// first character is a begin marker's backslash; or text with no preceding marker (see
/// change added for 15Sept10 note below). The beginning marker is parsed, being returning
/// in the mkr param, but if there is not beginning marker then the empty string is
/// returned in mkr, and parsing continues across its content, or across the text at the
/// buffer's start as the case may be, returning that in dataStr. If an endmarker is
/// present then that is parsed and returned in endMkr, and parsing continues beyond the
/// endmarker until the start of the next marker is encountered. Protection is built in so
/// that if a marker's expected endmarker is absent because of a SFM markup error, the
/// function will not return any endmarker but will still return the marker's content and
/// halt at the start of the next marker - thus making it tolerant of the lack of an
/// expected endmarker. (It is not, however, tolerant of a mismatched endmarker, such as if
/// someone typed an endmarker but typed it wrongly so that the actual endmarker turned out
/// to be an unknown endmarker or an endmarker for some other known marker - in either of
/// those scenarios, bEndMarkerError is returned TRUE, and the caller should abort the OXES
/// parse with an appropriate warning to the user.
/// While an endmarker is returned via the signature, the caller will usually ignore it
/// because it is included within the count of the span of characters delineated by the
/// parse, and that's all that usually matters.
/// The custom markers are, of course, \free or \note (OXES support ignores \bt, and any
/// such are removed from the USFM export before this class gets to see the data)
/// This function is based loosely on the ExportFunctions.cpp function:
/// int ParseMarkerAndAnyAssociatedText(wxChar* pChar, wxChar* pBuffStart,
///				wxChar* pEndChar, wxString bareMarkerForLookup, wxString wholeMarker,
///				bool parsingRTFText, bool InclCharFormatMkrs)
/// BEW created 6Sep10
/// BEW 15Sep10, added protocol that if mkr passed in is an empty string, then parsing
/// continues on the text from start of the passed in buffer, until a halt location is reached
int Usfm2Oxes::ParseMarker_Content_Endmarker(wxString& buffer, wxString& mkr, wxString& endMkr, 
						wxString& dataStr, bool& bEmbeddedSpan, 
						CustomMarkersFT inclOrExclFreeTrans, CustomMarkersN inclOrExclNote,
						RemarkMarker inclOrExclRemarks)
{
	wxString wholeMarker;
	mkr.Empty(); // default to being empty, until set below
	endMkr.Empty(); // default to being empty - we may define an endmarker later below
	dataStr.Empty(); // ensure this starts out empty
	bool bNeedsEndMarker = FALSE;
	bEmbeddedSpan = FALSE;
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
	wxString lookedUpEndMkr;

	// do the next block if on entry we are pointing at the backslash of a marker
	//if (*ptr == _T('\\')) 
	if (pDoc->IsMarker(ptr)) 
	{
		// we are pointing at the backslash of a marker
		wholeMarker = pDoc->GetWholeMarker(ptr); // has an initial backslash
		wxString bareMarkerForLookup = pDoc->GetBareMarkerForLookup(ptr); // lacks backslash
		mkr = wholeMarker; // caller needs to know what marker it was

		// use LookupSFM which properly handles \bt... forms as \bt in its lookup (the change
		// only persists for lookup - so we will need to check if South Asia Group markers like
		// \btv etc were matched and skip them - we don't support such markers as OXES has no
		// clue what to do with them (any such will return the USFMAnalysis struct for and
		// Adapt It collected back translation marker, \bt)
		lookedUpEndMkr.Empty();
		USFMAnalysis* pSfm = pDoc->LookupSFM(bareMarkerForLookup); 
		if (pSfm == NULL)
		{
			bNeedsEndMarker = FALSE; // treat unknown markers as those without end markers
		}
		else if (!pSfm->endMarker.IsEmpty())
		{
			// AI_USFM.xml says it needs an end marker
			bNeedsEndMarker = TRUE;
			lookedUpEndMkr = _T("\\");
			lookedUpEndMkr += pSfm->endMarker;
		}
		else
		{
			// pSfm is not NULL and it doesn't have an end marker
			bNeedsEndMarker = FALSE;
		}

		if (ptr == pEnd)
			return 0;
		aLength = wholeMarker.Len();
		ptr += aLength; // point past initial marker
		txtLen += aLength; // count its characters
		dataLen += aLength;

		// check if we are about to parse over an embedded marker span, for \f, \fe, or \x
		// information, that is, footnote, endnote, or cross-reference
		wxString augmentedWholeMarker = wholeMarker + _T(" "); // add a delimiter to prevent
															   // spurious matches
		if (augmentedWholeMarker == _T("\\f ") ||
			augmentedWholeMarker == _T("\\fe ") ||
			augmentedWholeMarker == _T("\\x ") )
		{
			// tell the caller it's either a footnote, endnote or a cross-reference; but we
			// also use this boolean to indicate the presence of inline formatting markers
			bEmbeddedSpan = TRUE; 
		}

		// parse, and count, the white space following
		itemLen = pDoc->ParseWhiteSpace(ptr);
		ptr += itemLen;
		dataLen += itemLen;
	} // end TRUE block for test: if (*ptr == _T('\\'))

	// mark the starting point for the content to be returned in dataStr
	pContentStr = ptr; // Note, include final white space in contentStr

	bool bUnexpectedHalt = FALSE;
	txtLen = 0;
	itemLen = 0;
	if (bNeedsEndMarker)
	{
		// wholeMarker needs an end marker so parse until we either find the end
		// marker or until we get to the next marker capable of halting parsing, or until
		// we reach the end of the buffer (e.g. when the last marker in the document has
		// no endmarker, so that all the remain data is content)
		while (ptr < pEnd && !pDoc->IsCorresEndMarker(wholeMarker, ptr, pEnd))
		{
			// the situations we can have when we get inside this block are:
			// (1) the good situation, we aren't yet at any marker and have yet to come to
			// the matching endmarker;
			// (2) a bad situation, we've come to a marker which halts parsing without
			// having come to the expected endmarker
			// If inclOrExclFreeTrans == includeFreeTransInTest, then \free is included
			// in the halt test; similarly w.r.t. inclOrExclNote for the \note marker
			if (IsAHaltingMarker(ptr, inclOrExclFreeTrans, inclOrExclNote))
			{
				// situation (2), break out with a flag set
				bUnexpectedHalt = TRUE;
				break;
			}
			// situation (1), continue scanning forward
			ptr++;
			txtLen++;
		} // end of scanning loop
		// check for what caused the halt
		if (ptr < pEnd)
		{
			if (bUnexpectedHalt)
			{
                // we didn't find the expected endmarker, but we found some other marker
				// which is deemed to be one which must halt parsing; tell the user an
				// expected endmarker was not found (and which one it is), but let
				// processing continue
				endMkr.Empty();
				wxString contents(pContentStr,txtLen);
				dataStr = contents;
				dataLen += txtLen;
				WarnNoEndmarkerFound(lookedUpEndMkr, contents);

			}
			else
			{
				// we found a corresponding end marker so we need to parse it too, but
				// first compute the dataStr contents (including any string-final space
				// because OXES <tr> element's PCDATA expects it to be included)
				wxString contents(pContentStr,txtLen);
				dataStr = contents;
				dataLen += txtLen;
				
				// parse the endmarker
				itemLen = 0;
				itemLen = pDoc->ParseMarker(ptr);
				wxString theEndMkr(ptr,itemLen);
				endMkr = theEndMkr;
				// update ptr and counts
				ptr += itemLen;
				txtLen += itemLen;
				dataLen += itemLen;
			}
		}
		else // for (ptr < pEnd) test returning FALSE
		{
			// we came to the document end, and so had to halt (and have no endmarker
			// identified) -- we'll warn the user, same as above, but let the OXES export
			// run to completion
			endMkr.Empty();
			wxString contents(pContentStr,txtLen);
			dataStr = contents;
			dataLen += txtLen;
			WarnAtBufferEndWithoutFindingEndmarker(lookedUpEndMkr);
		}
	} // end of TRUE block for test: if (bNeedsEndMarker)
	else
	{
        // wholeMarker doesn't have an end marker, or there was no beginning marker and we
        // are parsing over text at the start of the buffer; so parse until we either
        // encounter another marker that halts scanning forwards, or until the end of the
        // buffer is reached -- and we should continue parsing through any character format
        // markers and their end markers (m_specialMkrs and m_specialEndMkrs, respectively)
		while (ptr < pEnd && !IsAHaltingMarker(ptr, inclOrExclFreeTrans, inclOrExclNote))
		{
			if (pDoc->IsMarker(ptr))
			{
				bool bIsInlineMkr = IsSpecialTextStyleMkr(ptr);
				if (bIsInlineMkr || ((inclOrExclNote == excludeNoteFromTest) && (wholeMarker == _T("\\note")))
								 || ((inclOrExclFreeTrans == excludeFreeTransFromTest) && (wholeMarker == _T("\\free")))
								 || ((inclOrExclRemarks == excludeRemarkFromTest) && (wholeMarker == m_remarkMkr))
				   )
				{
					ptr++;
					txtLen++;
					bEmbeddedSpan = TRUE;
				}
				else
				{
					// we'll halt here anyway
					break;
				}
			}
			else
			{
				ptr++;
				txtLen++;
			}
		} // end of scanning loop
        // we came to a non-inline marker for formatting purposes, or to the buffer end,
        // either way the action wanted here is the same
		wxString contents(pContentStr,txtLen);
		dataStr = contents;
		// update dataLen
		dataLen += txtLen;
	} // end of else block for test: if (bNeedsEndMarker)
	// parse over any following white space, until either the end of the buffer is found
	// or until a backslash is found - add the characters traversed to dataLen, but throw
	// away the whitespace characters
	itemLen = 0;
	if (ptr < pEnd)
	{
		itemLen = pDoc->ParseWhiteSpace(ptr);
		if (itemLen > 0)
		{
			dataLen += itemLen;
		}
	}
	return dataLen;
}

void Usfm2Oxes::ParseTitleInfoForAIGroupStructs()
{
	// a preceding call of ClearTitleInfo() will have cleared all the arrays for the
	// structs stored at each level of the TitleInfo chunk
	
	// This function supports empty information strings, such as \free ... \free* where
	// ... is an empty string (the OXES production will have the xml structure, but the
	// info won't be there in the PCDATA or relevant attribute)
	if (!m_pTitleInfo->bChunkExists)
	{
		// if there is no title information, return without doing anything
		return;
	}
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	bool bHasInlineMarker = FALSE; // would be true if we parsed a \f, \fe or \x marker,
		// or an inline formatting marker, but all these are unlikely in Title chunks
	int span = 0;
	wxString buff = m_pTitleInfo->strChunk; // use a copy of the chunk string, as this
											// parser will consume the copy
	// we are at a parsing level now where SF markers will be identified, their contents
	// grabbed, and the markers themselves abandoned...
	
	// scratch strings
	wxString wholeEndMkr;
	wxString dataStr;
	// pointer to the last NoteDetails struct, since we must delay setting the offsets for
	// any note which precedes the text of this stretch of information
	NoteDetails* pLastNoteDetails = NULL;

	// begin parsing...  (for TitleInfo, each information chunk will begin with an SF
	// marker, and ParseMarker_Content_Endmarker() relies on that being true)
	wxString wholeMkr = pDoc->GetWholeMarker(buff);
	aiGroup* pGroupStruct = new aiGroup;
	pGroupStruct->bHasInlineMarker = FALSE;
	pGroupStruct->usfmBareMarker.Empty(); // never store a "free" marker, OXES 
		// knows nothing about such markers; store only the main marker, or empty string
	do
	{
		// In good USFM markup, if \rem occurs, it occurs at verse start, etc - or before
		// whatever marker type if is a remark for, so if present it should start off an aiGroup
		if ( wholeMkr == m_remarkMkr)
		{
			// there could be several in sequence, parse them all and add each's contents
			// string to the aiGroup's arrRemarks wxArrayString member
			while (wholeMkr == m_remarkMkr)
			{
				// identify the \rem remarks chunk
				span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
							bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
							haltAtRemarkWhenParsing);
				// bleed out the scanned over material
				buff = buff.Mid(span);
				// store the remark
				dataStr.Trim(); // remove whitespace from string's end
				pGroupStruct->arrRemarks.Add(dataStr);
				dataStr.Empty();
				// prepare for next iteration
				wholeMkr = pDoc->GetWholeMarker(buff);
				wholeEndMkr.Empty();
			}
		}
		// check if we are pointing at a \free marker
		else if (m_bContainsFreeTrans && wholeMkr == m_freeMkr)
		{
            // identify the free translation chunk & count its characters; this function
            // always exists with span giving the offset to the marker which halted
            // scanning (or end of buffer)
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
						bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			// bleed out the scanned over material
			buff = buff.Mid(span);
			// remove from dataStr the |@nnn@| and following space
			int anOffset = dataStr.Find(_T("@|"));
			if (anOffset != wxNOT_FOUND)
			{
				dataStr = dataStr.Mid(anOffset + 2);
				dataStr.Trim(FALSE); // trim on left
			}
			// store the free translation
			pGroupStruct->freeTransStr = dataStr;
			dataStr.Empty();
			pLastNoteDetails = NULL; // the last one, if it exists, has been dealt with
		}
		else if (m_bContainsNotes && wholeMkr == m_noteMkr)
		{
			// this block handles an Adapt It note stored on the first word within the
			// textual content for some marker (such as a verse, etc), but it won't handle
			// Adapt It notes embedded within a stretch of text - for those we will need
			// to extract them in the block below
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
						bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			// bleed out the scanned over material
			buff = buff.Mid(span);
			// store this note - the first or perhaps only one in this section of text
			pLastNoteDetails = new NoteDetails;
			SetNoteDetails(pLastNoteDetails, 0, dataStr);
			// beginOffset was passed in as 0 because this note is anchored to the first 
			// word of the text
			pGroupStruct->arrNoteDetails.Add(pLastNoteDetails);
			dataStr.Empty();
		}
		else
		{
            // it's some other marker than \free or \note, so scan over its data and also
            // scan over any embedded Adapt It notes -- we extract those from the returned
            // string and store them, using the ExtractNotesAndStoreText() call further
			// below. At this point we are possibly at a halting marker, but we could be
            // at an inline marker (we'll parse over these as if they were just not there),
            // or even just at the first word of the next bit of text to be parsed - so
            // what we do next will vary (the haltAt... enum values are just for ensuring
            // that \free and \note are included in the test, even though it's other
            // markers we are interested in - until we can parse our way across them to
            // where the text begins)
inner:      if (IsAHaltingMarker(buff, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing))
			{
				// when we get here, we are dealing with not one of the above, and so a
				// remark (\rem) would be a reason to halt, as it would apply to the next
				// aiGroup, so here the last param has the opposite value to above
				span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
							bHasInlineMarker, haltAtFreeTransWhenParsing, ignoreNoteWhenParsing,
							haltAtRemarkWhenParsing);

				// as this function will be used outside of TitleInfo contexts, we have to
				// handle the possibilities that we could have just parsed over a \c and
				// its chapter number, a \v and its verse number or verse number bridge,
				// or other markers such as \p, \q, \q# where # = 1, 2 or 3, possibly
				// others - only for \c or \v would dataStr have content for these types of
				// marker - we proceed in this inner loop using a goto statement, until we
				// get to a non-empty dataStr which does not follow a \c or \v or \vn
				// marker (\vn 'verse number' is not standard USFM, but some people use it)
				if (wholeMkr == _T("\\c"))
				{
					// in this case, wholeEndMkr will be empty, and dataStr a chapter
					// number string
					wxASSERT(wholeEndMkr.IsEmpty());
					pGroupStruct->chapterNumber = dataStr;
					pGroupStruct->arrMarkers.Add(wholeMkr); // this group has a chapter start
					buff = buff.Mid(span); // bleed out this \c and its chapter number
					dataStr.Empty();
					goto inner;
				}
				else if (wholeMkr == _T("\\v") || wholeMkr == _T("\\vn"))
				{
					// it's a verse number we've found (dataStr could have a verse number,
					// or a bridge - we don't care which, we just store the whole whatever
					// it is)
					wxASSERT(wholeEndMkr.IsEmpty());
					bool bIsBridge = IsAVerseBridge(dataStr);
					if (bIsBridge)
					{
						pGroupStruct->verseNumberBridgeStr = dataStr;
					}
					else
					{
						pGroupStruct->verseNumber = dataStr;
					}
					if (wholeMkr = _T("\\vn"))
					{
						wholeMkr = wholeMkr.Left(2); // change it to \v for storage 
													 // purposes for the marker
					}
					pGroupStruct->arrMarkers.Add(wholeMkr); // this group has a verse start
					buff = buff.Mid(span); // bleed out this \v or \vn and its verse info
					dataStr.Empty();
					goto inner;
				}
				else if (wholeEndMkr.IsEmpty() && dataStr.IsEmpty())
				{
					// it's a marker like \q, \q1, \q2, \q3, \m, \p etc - and a halting
					// marker follows it, so we've not arrived at text yet
					pGroupStruct->arrMarkers.Add(wholeMkr);
					buff = buff.Mid(span); // bleed out this \v or \vn and its verse info
					goto inner;
				}

				// store the bare main marker for this aiGroup
				if (!wholeMkr.IsEmpty())
				{
					pGroupStruct->usfmBareMarker = wholeMkr.Mid(1); // OXES will display it in
																	// its marker attribute
				}
				// once control gets to here, we've text to be handled...
				// first, bleed out the scanned over material
				buff = buff.Mid(span);

				// we have to handle the possibility that pLastNoteDetails if it is
				// non-null, needs to be finished off; also we extract any embedded notes and
				// store those in additional NoteDetails stucts, and store the text in the
				// aiGroup's textStr member -- all is done in the next call
				ExtractNotesAndStoreText(pGroupStruct, dataStr);
				dataStr.Empty();

				// store the aiGroup struct's pointer in the TitleInfo's AIGroupArray array
				// now that it is completed
				m_pTitleInfo->aiGroupArray.Add(pGroupStruct);
				pGroupStruct = NULL;
			}
			else
			{
				// assume it is text, with or without embedded notes and with or without
				// inline formatting markers
				ExtractNotesAndStoreText(pGroupStruct, buff);
				buff.Empty(); // the preceding call should consume what remains of this 
							  // aiGroup's text data
			}
		}

		// check next marker, prepare for iteration, and iterate or exit as the case may be
		if (!buff.IsEmpty())
		{
			wholeMkr = pDoc->GetWholeMarker(buff);
			if (pGroupStruct == NULL)
			{
				bHasInlineMarker = FALSE;
				pLastNoteDetails = NULL;
				pGroupStruct = new aiGroup;
				pGroupStruct->bHasInlineMarker = FALSE;
			}
		}
	} while (!buff.IsEmpty()); // end of do loop
	// when control gets to here, we've consumed the chunk copy stored in m_pTitleInfo
	 
	// if the 3-letter Bible book ID is in an initial \id field, we'll find it in the
	// first aiGroup of title info structs -- check for it, and if it's 3 letters, assume
	// it's a valid code and store it in the m_bookID member
	aiGroup* pFirstGroup = NULL;
	if (m_pTitleInfo->bChunkExists)
	{
		pFirstGroup = m_pTitleInfo->aiGroupArray.Item(0);
		if (pFirstGroup->usfmBareMarker == _T("id"))
		{
			wxString contents = pFirstGroup->textStr;
			if (contents.Len() >= 3)
			{
				wxArrayString arr;
				wxString delimiters = _T(" ");
				long numTokens = SmartTokenize(delimiters, contents, arr, FALSE);
				numTokens = numTokens; // avoids a compiler warning
				// the 3-letter Bible book code will be the first token - only accept it
				// if it has a length of 3 characters
				if (!arr.IsEmpty())
				{
					wxString possibleCode = arr.Item(0);
					if (possibleCode.Len() == 3)
					{
						// accept it
						m_bookID = possibleCode;
						m_bookID.Trim(FALSE);
						m_bookID.Trim();
					}
					else
					{
						m_bookID.Empty();
					}
				}
			}
		}
	}

	// check we got the structs filled out correctly
#ifdef __WXDEBUG__
	if (!m_bookID.IsEmpty())
	{
		wxLogDebug(_T(">>>>>    BOOK ID =  %s  <<<<<<"), m_bookID.c_str());
	}
	DisplayAIGroupStructContents(m_pTitleInfo);
#endif
}

void Usfm2Oxes::SetNoteDetails(NoteDetails* pDetails, int beginOffset, wxString& dataStr)
{
	pDetails->usfmMarker = _T("rem");
	pDetails->beginOffset = beginOffset;
	int anOffset = dataStr.Find(_T("@#"));
	wxASSERT(anOffset == 0);
	wxString remainder = dataStr.Mid(2); // remove the @# character pair
	anOffset = remainder.Find(_T(":"));
	wxASSERT(anOffset > 0);
	// get the number substring
	wxString numberStr = remainder.Left(anOffset);
	unsigned long numChars = 0;
	bool bOK = numberStr.ToULong(&numChars); // doesn't include a final space (none present yet)
	wxASSERT(bOK);
	bOK = bOK; // avoid compiler warning
	remainder = remainder.Mid(anOffset + 1); // remove number and trailing colon
	// next info field is the actual phrase plus its punctuation (from m_targetStr)
	anOffset = remainder.Find(_T("#@"));
	wxASSERT(anOffset > 0); // there has to at least be one character there
	pDetails->wordsInSpan = remainder.Left(anOffset);
	pDetails->noteText = remainder.Mid(anOffset + 2); // the note itself follows #@
	pDetails->endOffset = beginOffset + (int)numChars;
}

void Usfm2Oxes::ParseIntroInfoForAIGroupStructs()
{
	// This function supports empty information strings, such as \free ... \free* where
	// ... is an empty string (the OXES production will have the xml structure, but the
	// info won't be there in the PCDATA or relevant attribute)
	if (!m_pIntroInfo->bChunkExists)
	{
		// if there is no introductory information, return without doing anything
		return;
	}
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	bool bHasInlineMarker = FALSE; // would be true if we parsed a \f, \fe or \x marker,
		// or an inline formatting marker
	int span = 0;
	wxString buff = m_pIntroInfo->strChunk; // use a copy of the chunk string, as this
											// parser will consume the copy
	// we are at a parsing level now where SF markers will be identified, their contents
	// grabbed, and the markers themselves abandoned...
	
	// scratch strings
	wxString wholeEndMkr;
	wxString dataStr;
	// pointer to the last NoteDetails struct, since we must delay setting the offsets for
	// any note which precedes the text of this stretch of information
	NoteDetails* pLastNoteDetails = NULL;

    // begin parsing... (for IntroductionInfo, each information chunk will begin with an SF
    // marker, and ParseMarker_Content_Endmarker() relies on that being true)
	wxString wholeMkr = pDoc->GetWholeMarker(buff);
	aiGroup* pGroupStruct = new aiGroup;
	pGroupStruct->bHasInlineMarker = FALSE;
	pGroupStruct->usfmBareMarker.Empty(); // never store a "free" marker, OXES 
		// knows nothing about such markers; store only the main marker, or empty string
	do
	{
#ifdef __WXDEBUG__
#ifdef _IntroOverrun
				wxString initialBuff = buff.Left(50);
				wxLogDebug(_T("ParseIntroInfoForAIGroupStructs(), after do. wholeMkr = %s , buff = %s"),
					wholeMkr.c_str(), initialBuff.c_str());
#endif
#endif
		// In good USFM markup, if \rem occurs, it occurs at verse start, etc - or before
		// whatever marker type if is a remark for, so if present it should start off an aiGroup
		if ( wholeMkr == m_remarkMkr)
		{
			// there could be several in sequence, parse them all and add each's contents
			// string to the aiGroup's arrRemarks wxArrayString member
			while (wholeMkr == m_remarkMkr)
			{
				// identify the \rem remarks chunk, ignore bHasInlineMarker value returned
				span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
							bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
							haltAtRemarkWhenParsing);
				// bleed out the scanned over material
				buff = buff.Mid(span);
				// store the remark
				dataStr.Trim(); // remove whitespace from string's end
				pGroupStruct->arrRemarks.Add(dataStr);
				dataStr.Empty();
				// prepare for next iteration
				wholeMkr = pDoc->GetWholeMarker(buff);
				wholeEndMkr.Empty();
			}
		}
		// check if we are pointing at a \free marker
		if (m_bContainsFreeTrans && wholeMkr == m_freeMkr)
		{
            // identify the free translation chunk & count its characters; this function
            // always exists with span giving the offset to the marker which halted
            // scanning (or end of buffer)
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr, 
					bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			// bleed out the scanned over material
			buff = buff.Mid(span);
			// remove from dataStr the |@nnn@| and following space
			int anOffset = dataStr.Find(_T("@|"));
			if (anOffset != wxNOT_FOUND)
			{
				dataStr = dataStr.Mid(anOffset + 2);
				dataStr.Trim(FALSE); // trim on left
			}
			// store the free translation
			pGroupStruct->freeTransStr = dataStr;
			dataStr.Empty();
			pLastNoteDetails = NULL; // the last one, if it exists, has been dealt with
		}
		else if (m_bContainsNotes && wholeMkr == m_noteMkr)
		{
			// this block handles an Adapt It note stored on the first word within the
			// textual content for some marker (such as a verse, etc), but it won't handle
			// Adapt It notes embedded within a stretch of text - for those we will need
			// to extract them in the block below
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr, 
						bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			// bleed out the scanned over material
			buff = buff.Mid(span);
			// store this note - the first or perhaps only one in this section of text
			pLastNoteDetails = new NoteDetails;
			SetNoteDetails(pLastNoteDetails, 0, dataStr);
			pGroupStruct->arrNoteDetails.Add(pLastNoteDetails);
			dataStr.Empty();
		}
		else
		{
            // it's some other marker than \free or \note, so scan over its data and also
            // scan over any embedded Adapt It notes -- we extract those from the returned
            // string and store them, using the ExtractNotesAndStoreText() call further
			// below. At this point we are possibly at a halting marker, but we could be
            // at an inline marker (we'll parse over these as if they were just not there),
            // or even just at the first word of the next bit of text to be parsed - so
            // what we do next will vary (the haltAt... enum values are just for ensuring
            // that \free and \note are included in the test, even though it's other
            // markers we are interested in - until we can parse our way across them to
            // where the text begins)
inner:      if (IsAHaltingMarker(buff, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing))
			{
				span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr, 
							bHasInlineMarker, haltAtFreeTransWhenParsing, ignoreNoteWhenParsing,
							haltAtRemarkWhenParsing);
				if (wholeEndMkr.IsEmpty() && dataStr.IsEmpty())
				{
					// it's a marker like \q, \q1, \q2, \q3, \m, \p etc - and a halting
					// marker follows it, so we've not arrived at text yet
					pGroupStruct->arrMarkers.Add(wholeMkr);
					buff = buff.Mid(span); // bleed out this \v or \vn and its verse info
					goto inner;
				}
				// store the bare main marker for this aiGroup
				if (!wholeMkr.IsEmpty())
				{
					pGroupStruct->usfmBareMarker = wholeMkr.Mid(1); // OXES will display it in
																	// its marker attribute
				}
				// once control gets to here, we've text to be handled...
				// first, bleed out the scanned over material
				buff = buff.Mid(span);

                // we extract any embedded notes and store those in additional NoteDetails
                // stucts, and store the text in the aiGroup's textStr member -- all is
                // done in the next call
				ExtractNotesAndStoreText(pGroupStruct, dataStr);
				dataStr.Empty();

                // store the aiGroup struct's pointer in the IntroductionInfo's
                // AIGroupArray array now that it is completed
				m_pIntroInfo->aiGroupArray.Add(pGroupStruct);
				pGroupStruct = NULL;
			} // end of TRUE block for test: 
			  // if (IsAHaltingMarker(buff, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing))
			else
			{
				// assume it is text, with or without embedded notes and with or without
				// inline formatting markers; but the chunk may be far from finished, --
				// there could be several text chunks interspersed between notes, so
				// we can't assume there are no more halting markers, nor no more
				// markerless buffer beginning for the current aiGroup - so do the usual
				// parse
				span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr, 
						bHasInlineMarker, haltAtFreeTransWhenParsing, ignoreNoteWhenParsing,
						haltAtRemarkWhenParsing);
#ifdef __WXDEBUG__
#ifdef _IntroOverrun
				wxString buffStarts = buff.Left(80);
				wxLogDebug(_T("ParseIntroInfoForAIGroupStructs(), text block. buff = %s"),buffStarts.c_str());
#endif
#endif
				buff = buff.Mid(span);

				ExtractNotesAndStoreText(pGroupStruct, dataStr);
				dataStr.Empty();

                // store the aiGroup struct's pointer in the IntroductionInfo's
                // AIGroupArray array now that it is completed
				m_pIntroInfo->aiGroupArray.Add(pGroupStruct);
				pGroupStruct = NULL;
			}
		}

		// check next marker, prepare for iteration, and iterate or exit as the case may be
		if (!buff.IsEmpty())
		{
			wholeMkr = pDoc->GetWholeMarker(buff);
			if (pGroupStruct == NULL)
			{
				bHasInlineMarker = FALSE;
				pLastNoteDetails = NULL;
				pGroupStruct = new aiGroup;
				pGroupStruct->bHasInlineMarker = FALSE;
			}
		}
	} while (!buff.IsEmpty());
	// when control gets to here, we've consumed the copy we made from the chunk
	// stored in m_pIntroInfo

	// check we got the structs filled out correctly
#ifdef __WXDEBUG__	
	DisplayAIGroupStructContentsIntro(m_pIntroInfo);
#endif
}


bool Usfm2Oxes::IsAVerseBridge(wxString& data)
{
	// search for some common bridge characters, like 'a' 'b' '-' ','
	// and include '.' as a possibility too, though it may not be standard
	if (	data.Find(_T('-')) != wxNOT_FOUND ||
			data.Find(_T(',')) != wxNOT_FOUND ||
			data.Find(_T('.')) != wxNOT_FOUND ||
			data.Find(_T('a')) != wxNOT_FOUND ||
			data.Find(_T('b')) != wxNOT_FOUND
		)
	{
		return TRUE;
	}
	return FALSE;
}

void Usfm2Oxes::DisplayAIGroupStructContents(TitleInfo* pTitleInfo) // for checking/debugging
{
	if (!pTitleInfo->bChunkExists)
	{
		wxLogDebug(_T("\n*** TitleInfo ***   There is no title information in this export"));
		return;
	}
	else
	{
		int itemCount = pTitleInfo->aiGroupArray.GetCount();
		int index;
		for (index=0; index<itemCount; index++)
		{
			aiGroup* pGrp = pTitleInfo->aiGroupArray.Item(index);
			if (pGrp->bHasInlineMarker)
			{
				wxLogDebug(_T("\n*** TitleInfo ***     aiGroup with index = %d   bHasInlineMarker %s"), index, _T("TRUE"));
			}
			else
			{
				wxLogDebug(_T("\n*** TitleInfo ***     aiGroup with index = %d   bHasInlineMarker %s"), index, _T("FALSE"));
			}

			// display one or more \rem contents, if there was one or more \rem fields present
			size_t remarksCount = pGrp->arrRemarks.Count();
			if (remarksCount == 0)
			{
				wxLogDebug(_T("    *no \\rem field remarks in this group*"));
			}
			else
			{
				size_t remIndex;
				for (remIndex = 0; remIndex < remarksCount; remIndex++)
				{
					wxString aRemark = pGrp->arrRemarks.Item(remIndex);

					wxLogDebug(_T("    remarksIndex = %d , Remark:  %s"), remIndex, aRemark.c_str());
				}
			}

			wxLogDebug(_T("    Usfm bare Mkr  =      %s"),pGrp->usfmBareMarker.c_str());
			wxLogDebug(_T("    freeTransStr   =      %s"),pGrp->freeTransStr.c_str());
			int count2 = pGrp->arrNoteDetails.GetCount();
			if (count2 > 0)
			{
				int index2;
				for (index2=0; index2<count2; index2++)
				{
					NoteDetails* pDetails = pGrp->arrNoteDetails.Item(index2);
					wxLogDebug(_T("    * NoteDetails *     NoteDetail with index2 = %d"), index2);
					wxLogDebug(_T("        noteText = %s"),pDetails->noteText.c_str());
					wxLogDebug(_T("        beginOffset %d , endOffset %d , wordsInSpan = %s , Usfm bare Mkr = %s"),
						pDetails->beginOffset, pDetails->endOffset, pDetails->wordsInSpan.c_str(), pDetails->usfmMarker.c_str());
				}
			}
			int count3 = pGrp->arrMarkers.GetCount();
			if (count3 > 0)
			{
				wxString aSpace = _T(" ");
				wxString markers;
				int index3;
				for (index3=0; index3<count3; index3++)
				{
					markers += pGrp->arrMarkers.Item(index3) + aSpace;
				}
				wxLogDebug(_T("    arrMarkers         %s"),markers.c_str());
			}
			else
			{
				wxLogDebug(_T("    arrMarkers         [empty]"));
			}
			if (!pGrp->chapterNumber.IsEmpty())
			{
				wxLogDebug(_T("    chapterNum         %s"),pGrp->chapterNumber.c_str());
			}
			if (!pGrp->verseNumber.IsEmpty())
			{
				wxLogDebug(_T("    verseNum           %s"),pGrp->verseNumber.c_str());
			}
			if (!pGrp->verseNumberBridgeStr.IsEmpty())
			{
				wxLogDebug(_T("    verseNumBridgeStr  %s"),pGrp->verseNumberBridgeStr.c_str());
			}
			wxLogDebug(_T("    textStr      =       %s"),pGrp->textStr.c_str());
		}
	}
}

void Usfm2Oxes::DisplayAIGroupStructContentsIntro(IntroductionInfo* pIntroInfo) // for checking/debugging
{
	if (!pIntroInfo->bChunkExists)
	{
		wxLogDebug(_T("*** IntroductionInfo ***   There is no introduction information in this export"));
		return;
	}
	else
	{
		int itemCount = pIntroInfo->aiGroupArray.GetCount();
		int index;
		for (index=0; index<itemCount; index++)
		{
			aiGroup* pGrp = pIntroInfo->aiGroupArray.Item(index);
			if (pGrp->bHasInlineMarker)
			{
				wxLogDebug(_T("\n*** IntroductionInfo ***     aiGroup with index = %d   bHasInlineMarker %s"), index, _T("TRUE"));
			}
			else
			{
				wxLogDebug(_T("\n*** IntroductionInfo ***     aiGroup with index = %d   bHasInlineMarker %s"), index, _T("FALSE"));
			}
			wxLogDebug(_T("    Usfm bare Mkr  =      %s"),pGrp->usfmBareMarker.c_str());
			wxLogDebug(_T("    freeTransStr   =      %s"),pGrp->freeTransStr.c_str());
			int count2 = pGrp->arrNoteDetails.GetCount();
			if (count2 > 0)
			{
				int index2;
				for (index2=0; index2<count2; index2++)
				{
					NoteDetails* pDetails = pGrp->arrNoteDetails.Item(index2);
					wxLogDebug(_T("    * NoteDetails *     NoteDetail with index2 = %d"), index2);
					wxLogDebug(_T("        noteText = %s"),pDetails->noteText.c_str());
					wxLogDebug(_T("        beginOffset %d , endOffset %d , wordsInSpan = %s , Usfm bare Mkr = %s"),
						pDetails->beginOffset, pDetails->endOffset, pDetails->wordsInSpan.c_str(), pDetails->usfmMarker.c_str());
				}
			}
			int count3 = pGrp->arrMarkers.GetCount();
			if (count3 > 0)
			{
				wxString aSpace = _T(" ");
				wxString markers;
				int index3;
				for (index3=0; index3<count3; index3++)
				{
					markers += pGrp->arrMarkers.Item(index3) + aSpace;
				}
				wxLogDebug(_T("    arrMarkers         %s"),markers.c_str());
			}
			else
			{
				wxLogDebug(_T("    arrMarkers         [empty]"));
			}
			if (!pGrp->chapterNumber.IsEmpty())
			{
				wxLogDebug(_T("    chapterNum         %s"),pGrp->chapterNumber.c_str());
			}
			if (!pGrp->verseNumber.IsEmpty())
			{
				wxLogDebug(_T("    verseNum           %s"),pGrp->verseNumber.c_str());
			}
			if (!pGrp->verseNumberBridgeStr.IsEmpty())
			{
				wxLogDebug(_T("    verseNumBridgeStr  %s"),pGrp->verseNumberBridgeStr.c_str());
			}
			wxLogDebug(_T("    textStr      =       %s"),pGrp->textStr.c_str());
		}
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param  pCurAIGroup       ->  ptr to the current aiGroup struct for which theText is
///                               the textual content (usually scripture text from a
///                               verse, but not necessarily  - as in Introduction data)
///                               being analysed for storing in its textStr member
/// \param  theText           ->  The text (typically part of a verse or a whole verse)
///                               which is in this aiGroup; this information will end
///                               up within the OXES <tr> element.
/// \remarks
/// On entry, theText is a section of text most or all of which ultimately will end up in an
/// OXES <tr> element. However, the text, as passed in, may still have one or more
/// embedded notes - these have to be extracted by this function, and it may have one or
/// more inline markers for formatting (USFM has a couple of dozen such marker types) -
/// these we leave embedded in the text, because when we later construct the <tr> element,
/// any inline markers can be parsed for and converted, in place, to the appropriate OXES
/// tag, or in the case of an endmarker, an endtag. Eg. <keyWord> ..... </keyWord>.
/// Any notes which we extract here have to be stored in a NoteDetails struct, and its
/// pointer stored in the arrNoteDetails member of pCurAIGroup. OXES places all the notes
/// pertaining to a <trGroup> at its start (after any \rem fields which also end up as
/// annotations), in sequence, and with an attribute giving the offset (in characters) from
/// the start of the text for that group, to the location to which the note applies -- for
/// adapt it, the latter will always be either a word or a phrase (a merger) - from the
/// single CSourcePhrase where the note was stored.
/// BEW created 13Sep10
void Usfm2Oxes::ExtractNotesAndStoreText(aiGroup* pCurAIGroup, wxString& theText)
{
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	// just in case someone has used the non-standard \vt marker to indicate the start of
	// the text, check and jump over it if it is present
	int myOffset = theText.Find(_T("\\vt ")); // <<- this \vt marker is parsed as 'inline' 
											  // even though strictly speaking it isn't
	if (myOffset != wxNOT_FOUND)
	{
		wxASSERT(myOffset == 0); // it has to be at the start, nowhere else
		theText = theText.Mid(4); // remove the marker from contention in all that follows
	}
	theText = theText.Trim(FALSE); // trim any white space from the start
	const wxChar* pBuffStart = theText.GetData();
    // remove the const modifier for pBuffStart
	wxChar* ptr = const_cast<wxChar*>(pBuffStart);
	size_t bufflen = theText.Len();
	wxChar* pEnd = ptr + bufflen;
	if (ptr == pEnd)
	{
		// just return if the passed in theText string is empty
		return;
	}
	wxString noteText;
	wxString aSpace = _T(" ");
	wxString accumStr;
	int itemLen = 0;
	int accumOffset = 0; // character count to a note, excludes inline markers 
					     // (and doesn't count one white space per marker because
					     // we assume that one is a delimiter and will be removed
					     // when the marker is removed)
	// parse across theText string, extracting any \note ... \note* information as we go,
	// storing each instance of note information in its own NoteDetails struct in the
	// array in pCurAiGroup, along with start and ending offsets for the word following,
	// and accumating theText's non-Note text into pCurAIGroup's textStr member. (In the
	// most general case, there may be inline markers, such as formatting markers, in
	// theText - these we parse over and store along with the text - these are changed
	// to oxes elements inline later on, for example: (I think the following is right)...
	// \k Jerusalem\k* becomes "<keyWord>Jerusalem </keyWord> in the Oxes PCDATA
	// and \wj word1 word2 .... becomes <wordsOfJesus>word1 word2.... and the closing \wj*
	// which could be a long long way ahead, becomes  wordn-1 wordn-2 </wordsOfJesus> etc.
	while (ptr < pEnd)
	{
		if (pDoc->IsMarker(ptr))
		{
			// we've encountered a marker, either a \note, or an inLine formatting marker
			// - extract the note if that's what we've found, if it's a formatting marker
			// then don't count its length nor the length of its endmarker when we get to
			// it, but otherwise leave it there
			wxString wholeMkr = pDoc->GetWholeMarker(ptr);
			if (wholeMkr == m_noteMkr)
			{
				// extract the \note, its text content, its endmarker \note* -- the latter
				// will never be absent; note, in our special target text export for oxes
				// purposes, any note string will be prefixed with @#nnn:phrase#@ which
				// contains the character count of the m_targetStr contents, and 'phrase'
				// is the contents of m_targetStr -- we pass these on to the oxes export
				noteText.Empty();
				ptr += 6; // advance 5 characters plus a space (the marker delimiter)
				int aSpan = ptr - pBuffStart;
				wxString copiedStr = theText.Mid(aSpan);
				// at this point, copiedStr should be "@#nnn:phrase#@note_text_string \note*"
				int anOffset = copiedStr.Find(m_noteEndMkr);
				wxASSERT(anOffset != wxNOT_FOUND);
				noteText = copiedStr.Left(anOffset);
				// ensure the note ends with a space
				noteText = noteText.Trim();
				noteText += aSpace;
				// at this point, noteText should contain: "@#nnn:phrase#@note_text_string "

				// create a NoteDetails struct in which to save the note information
				NoteDetails* pLastNoteDetails = new NoteDetails;
				pCurAIGroup->arrNoteDetails.Add(pLastNoteDetails);
				// store in it the information that we know so far
				SetNoteDetails(pLastNoteDetails,accumOffset,noteText);

				// update ptr to point past the \note* marker, at whatever
				// first non-white-space character follows \note*
				ptr += anOffset + 6; // 6 is the length of \note*
				itemLen = pDoc->ParseWhiteSpace(ptr); // don't count this in accumOffset
				ptr += itemLen; // ParseWhiteSpace() will exit if NULL is reached
                // there should usually be at least one word following \note* because we
                // store a note on a CSourcePhrase having at least a word stored in its
                // m_key member, but the corresponding m_adaption could be empty, so we may
                // actually be done processing, and if we are pointing not at the end, we
				// may still be pointing at an inline marker, so handle all these
				// possibilities here
				if(ptr == pEnd)
				{
					// we've gotten to the end of the buffer, and there was no 'following'
					// word; so break out of the loop, there is nothing more to accumulate
					break;
				}
				// iterate...
			}
			else
			{
				//it's some other marker, accumulate it, but don't bump the character
				//counter for the characters in it nor the delimiting space following
				if (IsSpecialTextStyleMkr(ptr))
				{
					pCurAIGroup->bHasInlineMarker = TRUE; // store the fact there is at
														  // least one such in the group
					itemLen = pDoc->ParseMarker(ptr);
					wxString theMarker(ptr,itemLen);
					accumStr += theMarker;
					ptr += itemLen;
					itemLen = pDoc->ParseWhiteSpace(ptr);
					// place the ptr only past the first white space char, treat others as
					// part of the text for accumStr; we place the ptr here because we are
					// making an educated guess that later parsing removal of the marker
					// to form an xml tag or endtag will also remove a single space after
					// the marker which should not be regarded as part of the text - this
					// is relevant to keeping the character count in accumOffset as
					// correct as possible in the event of the presence of embedded inline
					// markers that have to be later removed without altering the count
					if (itemLen >= 1)
					{
						ptr++;
						accumStr += aSpace;
					}
					else
					{
						// itemLen must be zero, which can happen if the text follows an
						// endmarker in USFM, since USFM can treat * as indicating the end
						// of the marker and the following space which normally would be
						// present may not be -- in which case, there is nothing to do as
						// ptr is already pointing at the first non-whitespace character
						// following the endmarker (note, this could be a null at pEnd)
						if (ptr == pEnd)
						{
							break;
						}
					} // if not at pEnd, continue scanning through theText
				}
				else
				{
					// this is a programming/logic error, only inline formatting markers
					// should be able to get through to the enclosing code block, so
					// tell the developer & drop into the debugger? -- Better, give a
					// message and just accumulate the rest including the unknown marker -
					// then inspection of the wxLogDebug output will tell what I did wrong
					wxString aWrongMkr = pDoc->GetWholeMarker(ptr);
					wxString msg;
					msg = msg.Format(_T(
"ExtractNotesAndStoreText(): found non-inline marker %s\nThis marker is not yet handled for parsing to an aiGroup struct."),
					aWrongMkr.c_str());
					wxMessageBox(msg, _T("Logic error in the code"), wxICON_ERROR);
					// in a release build, unless we do something here, the failure to
					// parse the marker will lead to an infinite loop - so just accumulate
					// the rest and return in order to get something fixable produced and output
					wxString theRest(ptr, pEnd);
					accumStr += theRest;
					int aLength = theRest.Len();
					accumOffset += aLength;
					ptr = pEnd;
					//wxASSERT(FALSE);
				}
			}
		}
		else
		{
			accumStr += *ptr;
			accumOffset++;
			ptr++;
		}
	}

	// ensure accumStr ends with a space
	accumStr.Trim();
	accumStr += aSpace;

	// store accumStr in the aiGroup's textStr member
	pCurAIGroup->textStr = accumStr;
}


// This function chunks the canonical information into 'sections' - these have no formal
// existence in USFM nor in other markup schemas. The 'section' is defined as the material
// starting with a section header, up as far as the next section header. Non-linearities
// cause complications at this point, because in correct USFM markup, a \c marker must
// precede any \ms or \s type of section heading markers (ie. \ms \ms1 \ms2, etc) and the
// chapter marker's line needs to associate with the new section about to be created,
// rather than the current one about to be closed off. A further complication is that the
// presence of section divisions does not always coincide with chapter divisions, though it
// very often does. An additional complicating factor is that an \ms type of marker (for
// types of 'major section') may precede a \s type of marker (for a normal subheading for
// the material which follows) and when this is the case, BOTH of these belong in the one
// section. It is difficult to check for and ensure this happens right - our solution is to
// use the fact that proper USFM markup will have either of the following two marker
// sequences: \ms followed by \mr followed by \s (these, of course could be \ms2 \mr \s1,
// or other possible combinations of these two marker types), or, \ms followed by \s. We
// therefore set a counter to 0 when an \ms type is matched, and count iterations of the
// loop - any \s type of marker encountered before the counter reaches the value 3 are
// included in the current new section, rather than being an indicator for closing the
// current section and opening a new one.
// Return TRUE if all was well; but FALSE if there was a problem requiring premature
// termination of the parse - such as a document with no verses in it
bool Usfm2Oxes::ParseCanonIntoSections(CanonInfo* pCanonInfo)
{
	// the markers for sectioning are stored in m_sectioningMkrs
	wxChar aSpace = _T(' ');
	wxASSERT(pCanonInfo != NULL);
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	wxASSERT(pDoc != NULL);
	wxString canonStr = pCanonInfo->strChunk;
	SectionInfo* pCurSection = new SectionInfo;
	InitializeSectionInfo(pCurSection);
	// first section, so get everything preceding the initial verse into the first section
	// before we start looking for the end of the section; since we are at the start of a
	// book, we can assume a chapter starts the section and has a chapter number of 1.
	pCurSection->bChapterStartsAtSectionStart = TRUE;
	pCurSection->strChapterNumAtSectionStart = _T("1");

	int offset = canonStr.Find(m_verseMkr); // this will match USFM markers \v \va \vp, 
						// and also the non-standard \vn (verse number) and \vt
						// (verse text) which are sometimes used in Sth Asia Group;
						// but no matter, because any of these matches is okay here
	if (offset == wxNOT_FOUND)
	{
		wxString msg = _("OXES export terminated prematurely. There are no verses in this document.");
		wxMessageBox(msg, _T(""), wxICON_WARNING);
		return FALSE;
	}
	pCurSection->strChunk = canonStr.Left(offset);
	canonStr = canonStr.Mid(offset); // bleed out the transferred data

	wxString chapterStuff; // temporarily store \c followed by its chapter number here
			// when found in the loop, pending the decision whether it belongs in the
			// current section (if there is no section header at the chapter start) or
			// in the next section (if there is a section header at the chapter start)
	wxString chapterNum; chapterNum.Empty(); // put parsed chapter number string here temporarily

	// Now we must search for section ends in a loop, consuming all canonStr as we
	// delineate successive sections and transfer their data to each successive instance
	// of pCurSection->strChunk. Since many, but probably not all, sections will, in
	// correct USFM markup, start at a chapter break where there is also a sectioning
	// marker present, the \c and its chapter number are also to be included in the next
	// section, rather than at the end of the current one.
	int nIterationCount = -1; // set to 0 when \ms marker type encountered, a following 
							  // \s marker must be encountered before nIterationCount
							  // reaches the value 3 for it to be included in the current
							  // section, rather than starting a new section; once such
							  // a decision has been made, return the value to -1 which
							  // used to indicate that iteration counting is not in effect
	offset = wxNOT_FOUND;
	chapterStuff.Empty();
	wxString wholeMkr;
	wxString wholeMkrPlusSpace;
	wxString precedingStr;
	int offset2;
	bool bIsNormalSectionMkr = FALSE;
	bool bFoundChapterStart = FALSE;
	do {
		if (nIterationCount != -1)
		{
			nIterationCount++; // only values 1 and 2 are useful, after that we turn it off
		}
		if (nIterationCount > 2)
		{
			// turn it off once it reaches 3
			nIterationCount = -1;
		}
		offset = canonStr.Find(backslash);
		if (offset == wxNOT_FOUND)
		{
			// there are no more markers, so the current section gets what remains and
			// then we'll exit the loop after storing the current section's struct
			pCurSection->strChunk += canonStr;
			canonStr.Empty();

			// closing off the last section -- so the ending chapter number will be the one
			// already in strChapterNumAtSectionEnd provided that
			// bChapterChangesMidSection is TRUE, but if that boolean is
			// FALSE, then it will need to be the value in which is stored in 
			// strChapterNumAtSectionStart
            if (pCurSection->bChapterChangesMidSection)
			{
				// nothing to do, except set the flag
				pCurSection->bChapterEndsAtSectionEnd = TRUE;
			}
			else
			{
				wxASSERT(!pCurSection->strChapterNumAtSectionStart.IsEmpty());
				pCurSection->strChapterNumAtSectionEnd = 
									pCurSection->strChapterNumAtSectionStart;
				pCurSection->bChapterEndsAtSectionEnd = TRUE;
			}

			pCanonInfo->arrSections.Add(pCurSection);
		}
		else
		{
			// we've found a marker, determine if it is a chapter marker, and if so, store
			// it and its chapter number temporarily until we can determine where that
			// information belows - the current section, or the next section
			precedingStr = canonStr.Left(offset); // where we store it depends on another test
			canonStr = canonStr.Mid(offset); // shorten, so the marker is initial
			// store the rest (the chapter number) if the previous iteration found a \c
			// marker (Note: our algorithm relies on correct markup for chapter and
			// subheading, the subheading should, if it is present, be immediately
			// following the chapter marker's line. We'll test for this an assert if that
			// is not the case.)
			if (bFoundChapterStart)
			{
				// precedingStr will contain the white space and following chapter number
				// so complete the chapter stuff information by adding it to the \c marker
				// which is already present from the previous iteration
				chapterStuff += precedingStr;
				chapterNum = GetChapterNumber(precedingStr);
				// we clear the bFoundChapterStart flag further below, because we need to
				// use it in further tests
			}
			else
			{
				pCurSection->strChunk += precedingStr;
			}
			wholeMkr = pDoc->GetWholeMarker(canonStr);
			int length = wholeMkr.Len();
			// a chapter marker will have white space following it, test and if it is not
			// so, -- then it is some other marker - in which case check for it being a
			// section-ending marker
			if (wholeMkr == m_chapterMkr && IsWhiteSpace(canonStr[length]))
			{
				// it is a \c marker, so we have come to the start of a new chapter
				bFoundChapterStart = TRUE;
				chapterStuff = wholeMkr;
				canonStr = canonStr.Mid(length);
				offset2 = wxNOT_FOUND;
				precedingStr.Empty();
				continue;
			}
			else
			{
				// it is not a chapter marker, so check if it is one of the section-ending
				// ones
				wholeMkrPlusSpace = wholeMkr + aSpace;
				offset2 = m_sectioningMkrs.Find(wholeMkrPlusSpace);
				if (offset2 == wxNOT_FOUND)
				{
					// the whole marker is not a sectioning one, so continue iterating
					if (bFoundChapterStart)
					{
                        // If this flag is still TRUE, then the last iteration found a
                        // chapter number, and this iteration has just parsed over and
                        // accumulated its chapter number. Also, we've found a marker which
                        // is not a type of subheading marker - in which case there is no
                        // subheading, nor major section heading, at the start of this
                        // chapter and so the chapterStuff string has to be immediately
                        // placed in the current section now, and the chapterStuff string
                        // cleared, before we start grabbing more data from canonStr.
                        // Moreover, we know that this chapter change is neither at the
                        // start of a section nor at its end, and so we can set other
                        // SectionInfo members now to preserve this understanding
						pCurSection->strChunk += chapterStuff;
						chapterStuff.Empty();
						pCurSection->bChapterChangesMidSection = TRUE;
						pCurSection->strChapterNumAtSectionEnd = chapterNum;
						// to get the value of the previous chapter's number, we'll not do
						// arithmetic (we might be handling a test document with chapters
						// omitted) but instead get the ending chapter number of the
						// last-stored SectionInfo struct
						if (!pCanonInfo->arrSections.IsEmpty())
						{
							wxString lastChapterNum = 
								(pCanonInfo->arrSections.Last())->strChapterNumAtSectionEnd;
							wxASSERT(!lastChapterNum.IsEmpty());
							pCurSection->strChapterNumAtSectionStart = lastChapterNum;
						}
						else
						{
							// we can assume the section-starting chapter number would be 1
							// in this case 
							pCurSection->strChapterNumAtSectionStart = _T("1");
						}
					}
					// accumulate the marker we found into the current section and iterate
					pCurSection->strChunk += wholeMkr;
					canonStr = canonStr.Mid(length); // bleed out the marker we've accumulated
					bFoundChapterStart = FALSE; // ensure it is now FALSE
				} // end of TRUE block for test: if (offset2 == wxNOT_FOUND)
				  // (which means 'the found marker is not one of the sectioning ones')
				else
				{
                    // the whole marker is a sectioning one, so check the contents of
                    // chapterStuff to see if it contains \c and a chapter number stored on
                    // the previous iteration - if it does, then that information belongs
                    // in the next section; whatever the case the current section has to be
					// finished off and a new section started; and set the members for
					// tracking the chapter number at start and end of the section, etc.
					// We also much check for the sectioning marker being a majorSection
					// one (ie. one which starts with "\ms") and if that is the case, we
					// must ensure that any subheading (\s type of marker) which may
					// follow the \ms type of marker does not cause a new section to begin
					// but is instead included in the one which has the \ms data.
					bIsNormalSectionMkr = IsNormalSectionMkr(canonStr); // TRUE if \s or \s#
					if (bIsNormalSectionMkr && nIterationCount > 0 && nIterationCount < 3)
					{
						// this section header belongs in the current section
						nIterationCount = -1; // it's done it job, turn it back off
					}
					else
					{
						// a type of \s marker further away than \ms than two iterations,
						// or a sesctioning marker which is not one of the \s ones, must
						// be able to finish of the current section and get a new section
						// commenced
						if (bFoundChapterStart)
						{
							// there is chapter start data (\c nn where nn is a chapter number)
							// which we must deal with - it goes in the next section, which
							// means that the chapter number at the end of the current section
							// will be the one in strChapterNumAtSectionEnd if the boolean
							// bChapterChangesMidSection is TRUE (the former string is already
							// set if that boolean is TRUE), otherwise if the latter flag
							// is FALSE, it is the same as is in strChapterNumAtSectionStart
							if (pCurSection->bChapterChangesMidSection)
							{
								pCurSection->bChapterEndsAtSectionEnd = TRUE;
							}
							else
							{
								pCurSection->bChapterEndsAtSectionEnd = TRUE;
								wxASSERT(!pCurSection->strChapterNumAtSectionStart.IsEmpty());
								pCurSection->strChapterNumAtSectionEnd = 
													pCurSection->strChapterNumAtSectionStart;
							}
	                        
							// store the section in the array for that purpose
							pCanonInfo->arrSections.Add(pCurSection);

							// create a new SectionInfo struct, and set the chapter information
							pCurSection = new SectionInfo;
							InitializeSectionInfo(pCurSection);
							pCurSection->bChapterStartsAtSectionStart = TRUE;
							pCurSection->strChapterNumAtSectionStart = chapterNum;

							// put the chapter start information in the new section's strChunk
							// and clear both chapterStuff and chapterNum strings
							pCurSection->strChunk = chapterStuff;
							chapterStuff.Empty();
							chapterNum.Empty();							
						}
						else
						{
							// a new section is starting but the chapter number remains
							// unchanged because there is no change of chapter at the end of
							// this section -- so the ending chapter number will be the one
							// already in strChapterNumAtSectionEnd provided that
							// bChapterChangesMidSection is TRUE, but if that boolean is
							// FALSE, then it will need to be the value in which is stored in 
							// strChapterNumAtSectionStart
							if (pCurSection->bChapterChangesMidSection)
							{
								// nothing to do
								;
							}
							else
							{
								wxASSERT(!pCurSection->strChapterNumAtSectionStart.IsEmpty());
								pCurSection->strChapterNumAtSectionEnd = 
													pCurSection->strChapterNumAtSectionStart;
							}
							wxString strCarryForwardEndingChapterNum = 
													pCurSection->strChapterNumAtSectionEnd;

							// store the section in the array for that purpose
							pCanonInfo->arrSections.Add(pCurSection);

							// create a new SectionInfo struct
							pCurSection = new SectionInfo;
							InitializeSectionInfo(pCurSection);
							// set the strChapterNumAtSectionStart string, but leave the
							// boolean bChapterStartsAtSectionStart FALSE because the chapter
							// hasn't just been changed
							pCurSection->strChapterNumAtSectionStart = 
													strCarryForwardEndingChapterNum;
							strCarryForwardEndingChapterNum.Empty();
							// ensure the next two are empty (both should already be empty, but
							// it is a good idea to ensure it is so)
							chapterStuff.Empty();
							chapterNum.Empty();
							
						} // end of else block for test: if (bFoundChapterStart)
					} // end of else block for test: 
					  // if (bIsNormalSectionMkr && nIterationCount > 0 && nIterationCount < 3)

					bFoundChapterStart = FALSE;
					offset2 = wxNOT_FOUND;

					// determine if the marker is a type of \ms marker, and if so
					// start iteration counting (in order to determine whether a
					// closely following type of \s marker belongs in this section or not)
					int offset3 = wxNOT_FOUND;
					offset3 = wholeMkr.Find(m_majorSectionMkr);
					if (offset3 != wxNOT_FOUND && nIterationCount == -1)
					{
						nIterationCount = 0;
					}

					// accumulate the sectioning marker in the new current struct,
					// so that the loop will recommence looking for the end of this new
					// section
					pCurSection->strChunk += wholeMkr;
					canonStr = canonStr.Mid(length); // bleed out the marker we've accumulated
				} // end of else block for test: if (offset2 == wxNOT_FOUND)
				  // (i.e. offset2 is >= 0 if a m_sectioningMkrs marker caused section halt)
				  
			} // end of else block for test:  if (wholeMkr == m_chapterMkr && 
			  // IsWhiteSpace(canonStr[length]))
			  
		} // end of else block for test: if (offset == wxNOT_FOUND)

	} while(!canonStr.IsEmpty() && offset != wxNOT_FOUND);

	// verify that the sections are chunked correctly, using a loop and wxLogDebug calls
#ifdef __WXDEBUG__
	size_t count = pCanonInfo->arrSections.GetCount();
	if (count > 0)
	{
		size_t index;
		wxString a,b,c;
		for (index = 0; index < count; index++)
		{
			SectionInfo* pSectionInfo = m_pCanonInfo->arrSections.Item(index);
			a = pSectionInfo->bChapterStartsAtSectionStart?_T("TRUE"):_T("FALSE");
			b = pSectionInfo->bChapterEndsAtSectionEnd?_T("TRUE"):_T("FALSE");
			c = pSectionInfo->bChapterChangesMidSection?_T("TRUE"):_T("FALSE");
			wxLogDebug(_T("\nSection with index = %d\n   bChapterStartsAtSectionStart = %s\n   bChapterEndsAtSectionEnd = %s\n   bChapterChangesMidSection = %s\n      chapterNum at start:  %s\n      chapterNum at end:  %s\n%s"),
				index, a.c_str(), b.c_str(), c.c_str(),
				pSectionInfo->strChapterNumAtSectionStart.c_str(),
				pSectionInfo->strChapterNumAtSectionEnd.c_str(),
				pSectionInfo->strChunk.c_str());
		}
	}
#endif

	// do the next level of parsing, by chunking the set of SectionInfo structs into
	// each's component SectionPart structs
	bool bOK = ParseSectionsIntoSectionParts(&pCanonInfo->arrSections);
	if (!bOK)
	{
		// an error prevented completion of the parse of the Usfm, user has seen a
		// message, so terminate the parese
		return FALSE;
	}
	// all's well
	return TRUE;
}

// loop over each of the SectionInfo structs to call ParseSingleSectionIntoSectionParts()
// on each in order to achieve the next level of parsing. SectionPart structs are of 6
// types, four which are short, if they occur, at the start of a section, and then one or
// more poetry and paragraph chunks until all the section's data is consumed;
// returns TRUE if all was well, FALSE if an error prevented completion of the parse
bool Usfm2Oxes::ParseSectionsIntoSectionParts(AISectionInfoArray* pSectionsArray)
{
	size_t count = pSectionsArray->GetCount();
	if (count > 0)
	{
		size_t index;
		SectionInfo* pSectionInfo = NULL;
		for (index = 0; index < count; index++)
		{
			pSectionInfo = pSectionsArray->Item(index);
#ifdef __WXDEBUG__
			wxLogDebug(_T("\n ***  <<<  Parsing Section with index = %d   >>>  *** \n"), index);
#endif
			bool bParseWorkedRight = ParseSingleSectionIntoSectionParts(pSectionInfo);
			if (!bParseWorkedRight)
			{
				return FALSE; // a message has been seen by the user
			}
		}
	}
	else
	{
		wxBell(); // ring the bell if there are no sections
#ifdef __WXDEBUG__
		wxLogDebug(_T("\n ***  !!!!!!!!!!!  OOPS! No sections defined.  !!!!!!!!!!  ***"));
		wxLogDebug(_T("      Called from the end of ParseCanonIntoSections() at the ParseSectionsIntoSectionParts() call. \n"));
#endif
	}
	return TRUE;
}

// returns TRUE if all was well, FALSE if an error prevented completion of the parse
bool Usfm2Oxes::ParseSingleSectionIntoSectionParts(SectionInfo* pSectionInfo)
{
	wxString buff = pSectionInfo->strChunk; // this will be consumed by the chunking loop
	bool bMatched = FALSE; // TRUE if an attempt to chunk the text for one of the looked-for types of 
						   // SectionPart succeeded in any one pass through the loop
	bool bBlankLine = FALSE;
	int span = 0; // scratch variable, return delimited span's character count here
	bool bOnlySkippedMaterial = FALSE; // will be TRUE only when material (ie. markers
                // and their contents) stipulated as skippable were the only information
                // scanned over in a Chunker() call -- we need this information in order
                // to advance the buff position, without setting up a chunk type which
                // doesn't actually exist in the passed in pSectionInfo's chunk

	// The parsing strategy, since there are 6 different SectionPart chunks which are
	// mutual siblings, is to first look for the 4 which may occur at a section start,
	// doing that in a loop; after that there will only be paragraph SectionParts
	// interspersed with occasional Poetry SectionParts - we look for them in a second
	// loop, until the whole chunk is consumed. If it wasn't for the fact that in Adapt It
	// the user may free translate things like an \ms field, an \s field, and could place
	// notes in them too, it would be easy to just test for markers using the appropriate
	// fast-access strings; but free translations or notes, or even \rem fields (if the
	// data originated from Paratext) could precede the markers diagnostic of each
	// SectionPart, and so we clone each tentative SectionPart to the relevant one from the
	// m_arrSectionPartTemplate array so as to give it its array of possible markers and
	// array of any skip markers, then we can pass it to Chunker() to get the needed
	// chunking done. If Chunker returns a count of zero, then we abandon that instance of
	// SectionPart, produce a new one, and test for that, etc.
    // Note: Chunker() is good only for section parts that contain only the markers unique
    // to that section part plus possibly \rem \note and/or \free; it will prematurely exit
    // if used to try parse over markers not delineated in a struct's arrPossibleMarkers
    // array - but adding such markers doesn't fix things, because then the chunker would
    // not stop at the appropriate places. For this reason, and because we need only two
    // further levels of chunking which Chunker() can't handle, we'll define also
    // PoetryChunker() and ParagraphChunker() - these will be like Chunker() except that
    // they match an instance of the possible markers passed in in the struct, and then
    // internally search ahead using the relevant fast-access string to find the next
    // instance of the relevant marker set, and terminate preceding that (or preceding any
    // of \rem, \note, \free which belong to the next chunk).
	
	// This is the outer loop, it terminates when the content of the passed in section is
	// consumed.
	// NOTE: all of the SectionPart types test for, and parse over, the same skippable
	// information (we've set it to be any type of chapter marker) - but we don't advance
	// buff if there was only skippable information matched and parsed over - instead we
    // try the other types until one of them advances over non-skippable marker information
    // - and in THAT one we acually collect the skipped information along with the rest
    // which we delineated as belonging to that type of chunk as well
	do {
		// This is the first of the inner loops: it looks for section-initial fields like
		// \ms or \ms# or \qa, (# = 1, 2, or 3) then \mr or \d, then \s or \s# (#=1-4),
		// then, finally, \r. If there is a pass through this loop without finding any of
		// these, or any more of these, then the loop terminates and control goes on to
		// the second inner loop below
		do {
			bMatched = FALSE; // initialize for this iteration
			// start by looking for a majorOrSeriesChunkType
			SectionPart* pSectionPart = new SectionPart(*(m_arrSectionPartTemplate.Item(0)));
			// the wxArrayString arrays have to be explicitly copied, fortunately,
			// the WxArrayString class has an assignment operator defined
			//SectionPart* pTemplate = m_arrSectionPartTemplate.Item(0);
			//pSectionPart->arrPossibleMarkers = pTemplate->arrPossibleMarkers;
			//pSectionPart->arrSkipMarkers = pTemplate->arrSkipMarkers;
			pSectionPart->pSectionInfo = pSectionInfo; // set the parent struct
			bOnlySkippedMaterial = FALSE; // initialize
			span = Chunker(&buff, majorOrSeriesChunkType, (void*)pSectionPart,
							bOnlySkippedMaterial);
			if (span == 0 || bOnlySkippedMaterial)
			{
                // it's not a \ms, \ms# nor \qa marker and there was no skippable
                // information skipped over; or we only found skippable information
                // -- so buff remains unchanged, but delete this now unwanted chunk type
                // pSectionPart
				delete pSectionPart; // the internal arrays are automatically deleted 
			}
			else
			{
				bMatched = TRUE; // this iteration found a Section Head Major or 
							// a Section Head Series (there might be skipped material
							// before it as well)
				pSectionPart->bChunkExists = TRUE;
				pSectionPart->strChunk += buff.Left(span);
				buff = buff.Mid(span);
				// add the chunk's struct to the parent's array
				pSectionInfo->sectionPartArray.Add(pSectionPart);
			}
			// if we made a match, iterate the loop; if we didn't, try the next type of
			// SectionPart (this will be a test for a rangeOrPsalmChunkType )
			if (bMatched)
			{
				continue;
			}
			else
			{
				// try looking for a rangeOrPsalmChunkType...
				SectionPart* pSectionPart = new SectionPart(*(m_arrSectionPartTemplate.Item(1)));
				//SectionPart* pTemplate = m_arrSectionPartTemplate.Item(1);
				//pSectionPart->arrPossibleMarkers = pTemplate->arrPossibleMarkers;
				//pSectionPart->arrSkipMarkers = pTemplate->arrSkipMarkers;
				pSectionPart->pSectionInfo = pSectionInfo; // set the parent struct
				bOnlySkippedMaterial = FALSE; // initialize
				span = Chunker(&buff, rangeOrPsalmChunkType, (void*)pSectionPart,
								bOnlySkippedMaterial);
				if (span == 0 || bOnlySkippedMaterial)
				{
                    // it's not a \mr, or \d marker and there was no skippable information
                    // skipped over; or we only found skippable information -- so buff
                    // remains unchanged, but delete this now unwanted chunk type
                    // pSectionPart
					delete pSectionPart; // the internal arrays are automatically deleted 
				}
				else
				{
					bMatched = TRUE; // this iteration found a Section Head Range or 
								// a Section Head Psalm (there might be skipped material
								// before it as well)
					pSectionPart->bChunkExists = TRUE;
					pSectionPart->strChunk += buff.Left(span);
					buff = buff.Mid(span);
					// add the chunk's struct to the parent's array
					pSectionInfo->sectionPartArray.Add(pSectionPart);
				}
				// if we made a match, iterate the loop; if we didn't, try the next type of
				// SectionPart  (this will be a test for a normalOrMinorChunkType )
				if (bMatched)
				{
					continue;
				}
				else
				{
					// try looking for a normalOrMinorChunkType
					SectionPart* pSectionPart = new SectionPart(*(m_arrSectionPartTemplate.Item(2)));
					//SectionPart* pTemplate = m_arrSectionPartTemplate.Item(2);
					//pSectionPart->arrPossibleMarkers = pTemplate->arrPossibleMarkers;
					//pSectionPart->arrSkipMarkers = pTemplate->arrSkipMarkers;
					pSectionPart->pSectionInfo = pSectionInfo; // set the parent struct
					bOnlySkippedMaterial = FALSE; // initialize
					span = Chunker(&buff, normalOrMinorChunkType, (void*)pSectionPart,
									bOnlySkippedMaterial);
					if (span == 0 || bOnlySkippedMaterial)
					{
                        // it's not a \s nor \s# marker and there was no skippable
                        // information skipped over; or we only found skippable information
                        // -- so buff remains unchanged, but delete this now unwanted chunk
                        // type pSectionPart
						delete pSectionPart; // the internal arrays are automatically deleted 
					}
					else
					{
						bMatched = TRUE; // this iteration found a Section Head (normal) or 
									// a Section Head Minor (there might be skipped material
									// before it as well)
						pSectionPart->bChunkExists = TRUE;
						pSectionPart->strChunk += buff.Left(span);
						buff = buff.Mid(span);
						// add the chunk's struct to the parent's array
						pSectionInfo->sectionPartArray.Add(pSectionPart);
					}
					// if we made a match, iterate the loop; if we didn't, try the next type of
					// SectionPart  (this will be a test for a parallelPassageHeaderChunkType )
					if (bMatched)
					{
						continue;
					}
					else
					{
						// try looking for a parallelPassageHeadChunkType
						SectionPart* pSectionPart = new SectionPart(*(m_arrSectionPartTemplate.Item(3)));
						//SectionPart* pTemplate = m_arrSectionPartTemplate.Item(3);
						//pSectionPart->arrPossibleMarkers = pTemplate->arrPossibleMarkers;
						//pSectionPart->arrSkipMarkers = pTemplate->arrSkipMarkers;
						pSectionPart->pSectionInfo = pSectionInfo; // set the parent struct
						bOnlySkippedMaterial = FALSE; // initialize
						span = Chunker(&buff, parallelPassageHeadChunkType, (void*)pSectionPart,
										bOnlySkippedMaterial);
						if (span == 0 || bOnlySkippedMaterial)
						{
                            // it's not an \r marker and there was no skippable information
                            // skipped over; or we only found skippable information -- so
                            // buff remains unchanged, but delete this now unwanted chunk
                            // type pSectionPart
							delete pSectionPart; // the internal arrays are automatically deleted 
						}
						else
						{
							bMatched = TRUE; // this iteration found a Parallel Passage Head 
										// (there might be skipped material before it as well)
							pSectionPart->bChunkExists = TRUE;
							pSectionPart->strChunk += buff.Left(span);
							buff = buff.Mid(span);
							// add the chunk's struct to the parent's array
							pSectionInfo->sectionPartArray.Add(pSectionPart);
						}
					} // end of block for trying parallelPassageHeadChunkType
				} // end of block for trying normalOrMinorChunkType
			} // end of block for rangeOrPsalmChunkType
		}  // end of block for trying majorOrSeriesChunkType; and also the do loop's end
		while (!buff.IsEmpty() && bMatched);

		// This is the second inner loop, it looks for poetry or paragraph SectionParts
		// until the section is exhausted
		if (!buff.IsEmpty())
		{
			do {
//#ifdef __WXDEBUG__
//	int xOffset = buff.Find(_T("\\p"));
//	if (xOffset == 0)
//	{
//			int halt_here = 1;
//	}
//#endif
				bMatched = FALSE; // initialize to FALSE at the start of each iteration
				SectionPart* pSectionPart = new SectionPart(*(m_arrSectionPartTemplate.Item(4)));
				//SectionPart* pTemplate = m_arrSectionPartTemplate.Item(4);
				//pSectionPart->arrPossibleMarkers = pTemplate->arrPossibleMarkers;
				//pSectionPart->arrSkipMarkers = pTemplate->arrSkipMarkers;
				pSectionPart->pSectionInfo = pSectionInfo; // set the parent struct
				bOnlySkippedMaterial = FALSE; // initialize
				span = PoetryChunker(&buff, poetryChunkType, (void*)pSectionPart,
								bOnlySkippedMaterial, bBlankLine);
				if ((span == 0 || bOnlySkippedMaterial) && !bBlankLine)
				{
                    // it's not a poetry marker and not a blank line (which we deem as
                    // poetry here) and there was no skippable information skipped over; or
                    // we only found skippable information and the marker was not a blank
                    // line -- so buff remains unchanged, but delete this now unwanted
                    // chunk type pSectionPart
					delete pSectionPart; // the internal arrays are automatically deleted 
				}
				else
				{
                    // this iteration found a fragment (line) of poetry or a blank line
                    // (there might be skipped material before it as well)
                    bMatched = TRUE;
					pSectionPart->bChunkExists = TRUE;
					pSectionPart->strChunk += buff.Left(span);
					buff = buff.Mid(span);
					// add the chunk's struct to the parent's array
					pSectionInfo->sectionPartArray.Add(pSectionPart);
				}
				// if we made a match, iterate the loop; if we didn't, try the next type of
				// SectionPart  (this will be a test for a paragraphChunkType )
				if (bMatched)
				{
					continue;
				}
				else
				{
                    // try matching a paragraph chunk -- it has to be a paragraph chunk
                    // even if not beginning explicitly with a paragraph type of marker
					SectionPart* pSectionPart = new SectionPart(*(m_arrSectionPartTemplate.Item(5)));
					//SectionPart* pTemplate = m_arrSectionPartTemplate.Item(5);
					//pSectionPart->arrPossibleMarkers = pTemplate->arrPossibleMarkers;
					//pSectionPart->arrSkipMarkers = pTemplate->arrSkipMarkers;
					pSectionPart->pSectionInfo = pSectionInfo; // set the parent struct
					bOnlySkippedMaterial = FALSE; // initialize
					span = ParagraphChunker(&buff, paragraphChunkType, (void*)pSectionPart,
									bOnlySkippedMaterial);
					// the PargraphChunk() MUST succeed if the others failed to advance &
					// consume some of buff's textz
					if (span == 0)
					{
						// this constitues a parser error which leads to an infinite loop,
						// so have to bail out 
						bMatched = FALSE;
						wxString msg;
						int nWhereAt = wxNOT_FOUND;
						nWhereAt = m_pCanonInfo->arrSections.Index(pSectionInfo);
						msg = msg.Format(_("The Usfm parser for OXES version 1 export failed to advance.\nOXES export failed.\n(If not halted immediately, this would cause at infinite loop at section with index = %d)."),
							nWhereAt);
						wxMessageBox(msg,_T("OXES export error (infinite loop)"),wxICON_ERROR);
						return FALSE;
					}
					else
					{
						bMatched = TRUE;
						pSectionPart->bChunkExists = TRUE;
						pSectionPart->strChunk += buff.Left(span);
						buff = buff.Mid(span);
						// add the chunk's struct to the parent's array
						pSectionInfo->sectionPartArray.Add(pSectionPart);
					}
				}
			} while (!buff.IsEmpty() && bMatched);
		}
	} while (!buff.IsEmpty());

	// verify that the section parts are chunked correctly, using a loop and wxLogDebug calls
#ifdef __WXDEBUG__
	size_t count = pSectionInfo->sectionPartArray.GetCount();
	if (count > 0)
	{
		size_t index;
		for (index = 0; index < count; index++)
		{
			SectionPart* pSectionPart = pSectionInfo->sectionPartArray.Item(index);
			wxLogDebug(_T("\nSectionPart with index = %d   enum ChunkType = %d\n   Chunk text (next lines):\n%s"),
				index, pSectionPart->sectionPartType, pSectionPart->strChunk.c_str());
		}
	}
#endif

	// *** TODO *** Here is where we put a function to loop over all the section parts, and parse
	// to the next-lower level of chunks. For 4 of the 6 types of SectionPart, this will
	// just be a single aiGroup - our terminal chunk; for poetry too it will also be a
	// single aiGroup for each chunk; but for paragraph, there could be dozens of aiGroups
	// in an array -- so the next level of parsing will complete the hierarchy
	


	return TRUE;
}






/* first attempt, didn't take siblings of each other in sections into account - unfinished

	// next 3 are unneeded in my new design (removed from Initialize()
	//m_allowedPreParagraphMkrs = _T("\\c \\s \\s1 \\s2 \\s3 \\ms \\mr \\r \\d \\ms1 \\ms2 \\ms3 ");
	// the following set are the ones which possibly may occur immediately following markers
	// from the m_allowedPreParagraphMkrs set; we'll test for this, and if a marker isn't
	// in either set, our Oxes won't export it, but we'll store it and its content in 
	// SectionHeaderOrParagraph.unexpectedMarkers and show that member's contents to the
	// user in the view while parsing, so he can alert the developers that it needs to be
	// handled; we won't repeat all the paragraph markers here, but include them in the
	// tests by testing the marker against m_paragraphMkrs also
	//m_haltingMrks_PreFirstParagraph = _T("\\v \\q \\q1 \\q2 \\q3 \\qr \\qc \\qm \\qm1 \\qm2 \\qm3 ");
	// the allowed set for non-first paragraphs in sections are just two, \c and \nb
	//m_allowedPreParagraphMkrs_NonFirstParagraph = _T("\\c \\nb ");
	

// first attempt at Chunker -- overly complex & doesn't have a skip but keep capability
int Usfm2Oxes::Chunker(wxString* pInputBuffer,  enum ChunkType chunkType, void* pChunkStruct)
{
	wxASSERT((*pInputBuffer)[0] == _T('\\')); // we must be pointing at a marker
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	bool bEmbeddedSpan = FALSE; // would be true if we parsed a \f, \fe or \x marker
								// but these are unlikely in Title chunks
	bool bHasInlineMarker = FALSE; // needed for \rem parsing -- & is ignored
	int span = 0;
	wxString buff = *pInputBuffer; // work with a copy of the buffer contents until
								   // we are ready to bleed off the title info chunk
	// we need a counter for characters in this particular chunk instance
	int charsDefinitelyInChunk = 0;
	// scratch strings
	wxString wholeEndMkr;
	wxString dataStr;

	// begin...
#ifdef __WXDEBUG__
#ifdef _IntroOverrun
	//if (chunkType == introductionChunkType)
	//{
	//    wxString first2K = buff.Left(2200);
	//    wxLogDebug(_T("****** GetIntroInfoChunk(), FIRST 2,200 characters.\n%s"), first2K.c_str());
	//    wxLogDebug(_T("****** GetIntroInfoChunk(), END OF FIRST 2,200 characters.\n"));
	//}
#endif
#endif
	  
	wxString wholeMkr = pDoc->GetWholeMarker(buff);
	bool bBelongsInChunk = FALSE; // initialize
	switch (chunkType)
	{
	case titleChunkType:
		break; // TitleInfo struct is handled externally
	case introductionChunkType:
		{
			bBelongsInChunk = IsOneOf(wholeMkr, 
					(static_cast<IntroductionInfo*>(pChunkStruct))->arrPossibleMarkers, 
					includeFreeTransInTest, includeNoteInTest, includeRemarkInTest);
		}
		break;
	case sectionChunkType:
		break; // SectionInfo parsing is handled externally (there's no arrPossibleMarkers
			   // member in a SectionInfo struct
	case sectionPartChunkType:
		{
			bBelongsInChunk = IsOneOf(wholeMkr, 
					(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers, 
					includeFreeTransInTest, includeNoteInTest, includeRemarkInTest);
		}
		break;
	} // end of switch
	while (bBelongsInChunk)
	{
		// NOTE, we know that any \rem, \free...\free*, and/or \note...\note* fields
		// before a marker which is not one of those belongs to this chunk because at the
		// end of the loop we check for loop end condition, jumping over these to find if
		// the first marker which isn't one of these is still in the chunk - if it is, we
		// then iterate the loop and collect those fields, if it isn't, we break out of
		// the loop -- in this way we can be certain we increment charsDefinitelyInChunk
		// only when the material is definitely in the current chunk
		if ( wholeMkr == m_remarkMkr)
		{
            // there could be several in sequence, parse over them all (and later in the
            // parsing operation, at a lower level, we will add each's contents string to
            // the aiGroup's arrRemarks wxArrayString member when we parse to the level of
            // aiGroup structs)
			while (wholeMkr == m_remarkMkr)
			{
				// identify the \rem remarks chunk
				span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
							bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
							haltAtRemarkWhenParsing);
				charsDefinitelyInChunk += span;
				// bleed out the scanned over material
				buff = buff.Mid(span);
				dataStr.Empty();
				// get whatever marker is being pointed at now
				wholeMkr = pDoc->GetWholeMarker(buff);
			}
		}
		// check if we are pointing at a \free marker
		if (m_bContainsFreeTrans && wholeMkr == m_freeMkr)
		{
            // identify the free translation chunk & count its characters; this function
            // always exists with span giving the offset to the marker which halted
            // scanning (or end of buffer)
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, 
				dataStr, bEmbeddedSpan, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			charsDefinitelyInChunk += span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
		}
		else if (m_bContainsNotes && wholeMkr == m_noteMkr)
		{
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, 
				dataStr, bEmbeddedSpan, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing,
						haltAtRemarkWhenParsing);
			charsDefinitelyInChunk += span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
		}
		else
		{
            // it's some other marker than \free or \note or \rem, and it belongs in this
            // type of chunk, so scan over its data & count that -- then check whether we
            // should iterate or break out
			// Note, buff can start with a non-marker, such as the text following a note,
			// and when that happens, we must parse over the text to the next marker and
			// iterate the loop
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, 
							dataStr, bEmbeddedSpan, haltAtFreeTransWhenParsing, 
							ignoreNoteWhenParsing, haltAtRemarkWhenParsing);
			charsDefinitelyInChunk += span;
#ifdef __WXDEBUG__
#ifdef _IntroOverrun
			//if (chunkType == introductionChunkType)
			//{
			//	if (wholeMkr == _T("\\io2") && dataStr.Find(_T("B Ghengis")) != wxNOT_FOUND)
			//	{
					int breakPoint_Here = 1;
			//	}
			//}
#endif
#endif
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
		}

        // check next marker(s), and iterate or exit as the case may be; we have to be
        // careful with \rem, \free and \note because a remark, free translation or note
        // can be in any of the chunks, and if we match one of these in the following call,
        // it might belong to the chunk which follows - the latter will be the case if the
        // first field which is not one of these and which follows is a marker which is not
        // in the set of markers for the intro chunk - so we have to search ahead in that
        // one of those cases...
		wholeMkr = pDoc->GetWholeMarker(buff);
		wxString aMkr;
		bBelongsInChunk = FALSE; // reinitialize
		if (!wholeMkr.IsEmpty())
		{
			switch (chunkType)
			{
			case titleChunkType:
				break; // TitleInfo struct is handled externally
			case introductionChunkType:
				{
					bBelongsInChunk = IsOneOf(wholeMkr, 
							(static_cast<IntroductionInfo*>(pChunkStruct))->arrPossibleMarkers, 
							includeFreeTransInTest, includeNoteInTest, includeRemarkInTest);
				}
				break;
			case sectionChunkType:
				break; // SectionInfo parsing is handled externally (there's no arrPossibleMarkers
					   // member in a SectionInfo struct
			case sectionPartChunkType:
				{
					bBelongsInChunk = IsOneOf(wholeMkr, 
							(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers, 
							includeFreeTransInTest, includeNoteInTest, includeRemarkInTest);
				}
				break;
			} // end of switch
			// if it doesn't belong in this chunk then break out of the loop, we've found
			// the start of the next chunk and must parse no further in the current one
			if (!bBelongsInChunk)
			{
				break;
			}
			else
			{
				// the marker potentially belongs in the chunk
				if (wholeMkr == m_remarkMkr || wholeMkr == m_freeMkr || wholeMkr == m_noteMkr)
				{
                    // if it is a \rem or \free or \note marker, it may belong in the next
                    // chunk -- check it out
					wxString remainingBuff = buff.Mid(4); // 4 is sizeof(\rem), get past the \rem
														// or most of \free or \note
					size_t offset = 0;
					bool bMarkerIsNotThese = FALSE;
					offset = FindFromPos(remainingBuff, backslash, offset);
					while (offset != wxNOT_FOUND)
					{
						// we've found a marker, find out what it is
						wxString theRest = remainingBuff.Mid(offset); // the marker commences at
																	  // the start of theRest
						aMkr = pDoc->GetWholeMarker(theRest);
						if (aMkr == m_remarkMkr || aMkr == m_freeMkr || aMkr == m_noteMkr
							 || aMkr == m_freeEndMkr || aMkr == m_noteEndMkr)
						{
							// it's another \rem marker or \free or \note or endmarker of
							// the last two, so iterate
							offset += 4; // start from past the just-found marker
							offset = FindFromPos(remainingBuff, backslash, offset);
						}
						else
						{
                            // it's not one of these, so check if it is one which belongs
                            // in the current chunk
							bMarkerIsNotThese = TRUE;
							break;
						}
					} // end of while loop: while (offset != wxNOT_FOUND)
					if (bMarkerIsNotThese)
					{
						// check if aMkr belongs in the current group or not
						bool bItBelongs = FALSE;
						switch (chunkType)
						{
						case titleChunkType:
							break; // TitleInfo struct is handled externally
						case introductionChunkType:
							{
								bItBelongs = IsOneOf(wholeMkr, 
								(static_cast<IntroductionInfo*>(pChunkStruct))->arrPossibleMarkers, 
								excludeFreeTransFromTest, excludeNoteFromTest,
								excludeRemarkFromTest);
							}
							break;
						case sectionChunkType:
							break; // SectionInfo parsing is handled externally (there's no arrPossibleMarkers
								   // member in a SectionInfo struct
						case sectionPartChunkType:
							{
								bItBelongs = IsOneOf(wholeMkr, 
								(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers, 
								excludeFreeTransFromTest, excludeNoteFromTest,
								excludeRemarkFromTest);
							}
							break;
						} // end of switch
						if (bItBelongs)
						{
							continue;
						}
						else
						{
                            // Nope, it doesn't belong, so the one or more fields we
                            // examined belong in the next chunk
							break;
						}
					}
					else
					{
                        // we ran out of markers before finding one belonging to the next
                        // chunk! We never expect this to happen, since in USFM \rem or my
                        // \free or \note fields precede whatever field they are associated
                        // with. I guess we can only include them in the current aiGroup,
                        // which would reorder them earlier - but it's that or throw them
                        // away, and we should not do that. To include it in the group
                        // we've nothing to do here but let the outer loop continue
						continue;
					}
				} // end of TRUE block for test: if (wholeMkr == m_remarkMkr || 
				  // wholeMkr == m_freeMkr || wholeMkr == m_noteMkr)
				else
				{
					// it belongs in the chunk and is not one of \rem, \free or \not, so
					// iterate the loop
					continue;
				}
			} // end of else block for test: if (!bBelongsInChunk) ie. it does belong in chunk
		} // end of TRUE block for test: if (!wholeMkr.IsEmpty())
		else
		{
            // we have some text (we must assume it belongs in the chunk, only certain
            // markers signal the start of the next chunk), so parse to the next marker and
			// iterate the testing loop (we will parse to pEnd if no marker lies ahead, in
			// which case .Find() will return wxNOT_FOUND)
			int myOffset = buff.Find(backslash);
			if (myOffset != wxNOT_FOUND)
			{
				// we found a marker
				charsDefinitelyInChunk += myOffset;
				buff = buff.Mid(myOffset);
				aMkr = pDoc->GetWholeMarker(buff);
				bool bItBelongs = FALSE;
				switch (chunkType)
				{
				case titleChunkType:
					break; // TitleInfo struct is handled externally
				case introductionChunkType:
					{
						bItBelongs = IsOneOf(wholeMkr, 
						(static_cast<IntroductionInfo*>(pChunkStruct))->arrPossibleMarkers, 
						includeFreeTransInTest, includeNoteInTest, includeRemarkInTest);
					}
					break;
				case sectionChunkType:
					break; // SectionInfo parsing is handled externally (there's no arrPossibleMarkers
						   // member in a SectionInfo struct
				case sectionPartChunkType:
					{
						bItBelongs = IsOneOf(wholeMkr, 
						(static_cast<SectionPart*>(pChunkStruct))->arrPossibleMarkers, 
						includeFreeTransInTest, includeNoteInTest, includeRemarkInTest);
					}
					break;
				} // end of switch

				// we'll iterate now, provided bItBelongs is TRUE, but if it is FALSE then
				// we've come to a marker which does not belong in this section - in which
				// case we should exit the loop
				if (!bItBelongs)
				{
					// buff starts with material which is not in this current chunk
					break;
				}
				else
				{
					// we have a marker, so iterate the loop
					bBelongsInChunk = bItBelongs;
					wholeMkr = aMkr;
					aMkr.Empty();
					continue;
				}
			} // end of TRUE block for test: if (myOffset != wxNOT_FOUND)
		} // end of else block for test: if (!wholeMkr.IsEmpty())
	} // end of while loop: while (bBelongsInChunk)

	return charsDefinitelyInChunk;
}

void Usfm2Oxes::ParseSectionsIntoParagraphs(CanonInfo* pCanonInfo)
{
	SectionInfo* pSection = NULL;
	size_t count = pCanonInfo->arrSections.GetCount();
	wxASSERT(count != 0);
	size_t index;
	for (index = 0; index < count; index++)
	{
		pSection = pCanonInfo->arrSections.Item(index);
		wxASSERT(pSection != NULL);
		ParseOneSection(pSection);
	}
}

void Usfm2Oxes::ParseOneSection(SectionInfo* pSection)
{
	wxASSERT(pSection != NULL);
    // get the text to be parsed; sectionText is a copy of the chunk contents and our
    // parsing will consume this copied text
	wxString sectionText = pSection->strChunk;
	wxASSERT(!sectionText.IsEmpty());
	wxASSERT(sectionText[0] == _T('\\')); // the section should start with a marker
	SectionHeaderOrParagraph* pCurPara = new SectionHeaderOrParagraph;
	// set the pointer to the parent section
	pCurPara->pSectionInfo = pSection;
	pCurPara->bHasHeaderInfo = FALSE; // initialize, most paragraphs have no header info
	pCurPara->paragraphOpeningMkr.Empty(); // store the paragraph's opening marker if there
										   // is one (eg. \p or \m or \q etc), but not if
										   // none were present and we halted parsing of
										   // header info for another reason, such as a 
										   // \c without following \nb and no paragraph marker
										   // either, or no paragraph marker but \v was found
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	wxASSERT(pDoc != NULL);
	wxString parsedParagraphData; parsedParagraphData.Empty(); 

    // This parser has two parts; since there may be things like \c or \s or \r etc at the
    // start of the section, we have to parse over any such allowed markers which may
    // preceded the first of any paragraphs in this section - so we parse, checking for one
    // of the allowed ones and so long as we get TRUE we are still within the section
    // header (or pre-Paragraph) part of the first paragraph - we store this preliminary
    // information at the start of the first SectionHeaderOrParagraph struct instance.
    // The second part of the syntax takes over when we get to a paragraph marker or some
    // other marker which is considered to halt the parsing of the preParagraph info - such
    // as \v, \free, \note, \q, etc. A complication is that either or both of \free or
    // \note will occur, if present, preceding a paragraph marker. (There are over a score
    // of different types of paragraph marker in USFM.) We have to store these temporarily
    // and add them back to the data at the start of each new paragraph chunk.
	// Any marker encountered which is not within a paragraph nor is one of the allowed
	// pre-paragraph markers, we store in a special location (m_unexpectedMarkers) and
	// don't include it's data in the Oxes file, but we show its marker and content to the
	// user so that he can alert the developers that we've not handled something and need to.
	wxString aSpace = _T(" ");
	wxString eolStr = _T("\n");
	wxString precedingStr;
	int offset = wxNOT_FOUND;
	int offset1 = wxNOT_FOUND;
	bool bReachedEndOfHdrMaterial = FALSE;
	bool bMatchedOpeningParagraphMkr = FALSE;
	wxString wholeMkr;
	wxString wholeMkr_AtParaOpening; wholeMkr_AtParaOpening.Empty();
	WhichParagraph whichPara = parsingFirstParagraph;
	bool bMatchedNoBreakMkr = FALSE; // TRUE if \nb encountered after a \c marker
	bool bHandledChapterChange = FALSE; // TRUE if a \c marker mid section was handled

	// parse over, and store, the information from where the section header stuff is -
	// this stuff can only be at the start of a section; a section may lack a section
	// header too (e.g. a chapter start which has no \s type of marker); also a section
	// may have a mid-section chapter change
	do {
		if (whichPara == parsingFirstParagraph)
		{
			offset = sectionText.Find(backslash);
			if (offset == wxNOT_FOUND)
			{
				// there are no more markers so the rest belongs in the current
				// paragraph's chunk
				pCurPara->paragraphChunk += sectionText;
				sectionText.Empty();
				break;
			}
			else
			{
				// found a marker
				wxASSERT(offset == 0);
				wholeMkr = pDoc->GetWholeMarker(sectionText);
				int length = wholeMkr.Len();
				if (
					(IsMkrAllowedPrecedingParagraphs(wholeMkr) 
					|| IsAFreeMarker(wholeMkr)
					|| IsANoteMarker(wholeMkr)
				   ))
				{
                    // the marker is one of \c \s \s1 \s2 \s3 \ms \mr \r \d \ms1 \ms2 \ms3,
                    // or even \free or \note, because Adapt It allows the user to attach a
                    // note to any word, even a word of a subheading, and to free translate
                    // these as well of course; it's a note or free translation which
                    // immediately precedes a \p or other paragraph type of marker that we
                    // have to keep with the paragraph information following, rather than
                    // with the subheading information
					pCurPara->bHasHeaderInfo = TRUE;
					ParseHeaderInfoAndFirstParagraph(&sectionText, pCurPara, 
									&bReachedEndOfHdrMaterial, &bMatchedOpeningParagraphMkr, 
									&wholeMkr_AtParaOpening);
					// check if we are at the end of the header material
					if (bReachedEndOfHdrMaterial)
					{
						wxASSERT(!sectionText.IsEmpty());
						// if we halted due to matching a paragraph type of marker, then
						// store it in the struct, and remove it from the paragraph chunk
						// to be parsed over below (if we halted for any other reason,
						// don't store a marker in the struct)
						if (bMatchedOpeningParagraphMkr)
						{
							// store the marker
							pCurPara->paragraphOpeningMkr = wholeMkr_AtParaOpening;
							// now remove the opening paragraph marker from the data for
							// the chunk
							offset1 = sectionText.Find(wholeMkr_AtParaOpening);
							wxASSERT(offset1 >= 0);
							wxString strLeft = sectionText.Left(offset1);
							sectionText = sectionText.Mid(offset1);
							wholeMkr = pDoc->GetWholeMarker(sectionText);
							int length = wholeMkr.Len();
							wxASSERT(length > 1);
							// toss the marker in the garbage
							sectionText = sectionText.Mid(length);
							// join the bits that remain
							if (strLeft.IsEmpty())
							{
								sectionText.Trim(FALSE); // trim white space off of its start
							}
							else
							{
								strLeft.Trim(); // trim white space off of its end
								strLeft += eolStr; // start a new line
								sectionText = strLeft + sectionText; // append the rest
							}
						}
					}
					else
					{
						wxASSERT(sectionText[0] == _T('\\')); // must commence with a marker
						bMatchedOpeningParagraphMkr = FALSE;
						wholeMkr_AtParaOpening.Empty();
					}
				} // end of TRUE block for the test: if ((IsMkrAllowedPrecedingParagraphs(wholeMkr) etc
			} // end of else block for the test: if (offset == wxNOT_FOUND)
		} // end of TRUE block for the test: if (whichPara == parsingFirstParagraph)
		else
		{
			// It's not the first paragraph, so there will not be a section heading of any
			// kind, but there could be a chapter marker  - and it may, or may not, start a new
			// paragraph depending on whether a 'no break' marker, \nb, does not follow it, or
			// does follow it, respectively (we consider \c not followed by \p as equivalent
			// to \c followed by \p, to handle improper markup acceptably). There may be free
			// translation and/or a note present also - see comments above for how we handle
			// them.
			offset = sectionText.Find(backslash);
			if (offset == wxNOT_FOUND)
			{
				// there are no more markers so the rest belongs in the current
				// paragraph's chunk
				pCurPara->paragraphChunk += sectionText;
				sectionText.Empty();
				break;
			}
			else
			{
				// found a marker
				wxASSERT(offset == 0);
				wholeMkr = pDoc->GetWholeMarker(sectionText);
				int length = wholeMkr.Len();
				bMatchedNoBreakMkr = FALSE; // ensure it's set to the default value
				wxString wholeMkrPlusSpace = wholeMkr + aSpace;
				if (
					((m_allowedPreParagraphMkrs_NonFirstParagraph.Find(wholeMkrPlusSpace) >= 0)
					|| IsAFreeMarker(wholeMkr)
					|| IsANoteMarker(wholeMkr)
				   ))
				{
                    // the marker is one of \c or \nb or it's a note or free translation which
                    // immediately precedes a \p or other paragraph type of marker that we
                    // have to keep with the paragraph information following, rather than
					// with the subheading information (actually \nb should never occur
					// without a preceding \c, but just in case someone fouls the markup...)
					pCurPara->bHasHeaderInfo = TRUE;
					ParseNonFirstParagraph(&sectionText, pCurPara, &bHandledChapterChange, 
							&bMatchedOpeningParagraphMkr, &wholeMkr_AtParaOpening, &bMatchedNoBreakMkr);
					// check if we are at the end of the allowed non-paragraph material
					if (bHandledChapterChange)
					{
						wxASSERT(!sectionText.IsEmpty());
						// what we did depended on whether there was \nb after \c, so
						// check it out and process accordingly
						if (bMatchedNoBreakMkr)
						{
							// there is to be no paragraph break here, so keep looking
							// (sectionText should have had both the \c, its chapter
							// number, and the following \nb marker removed (the user
							// should not have a \p marker following a \nb marker, as they
							// conflict, but if he does, the \p marker will be ignored and
							// removed as well)
							wxASSERT(sectionText[0] == _T('\\')); // must be pointing at a mkr
						}
						else
						{
							// if we are at a paragraph type of marker at a paragraph chunk's
							// start, then store it in the struct, and remove it from the
							// paragraph chunk to be parsed over below (if we halted for any
							// other reason, don't store a marker in the struct)
							if (bMatchedOpeningParagraphMkr)
							{
								// store the marker
								pCurPara->paragraphOpeningMkr = wholeMkr_AtParaOpening;
								// now remove the opening paragraph marker from the data for
								// the chunk
								offset1 = sectionText.Find(wholeMkr_AtParaOpening);
								wxASSERT(offset1 >= 0);
								wxString strLeft = sectionText.Left(offset1);
								sectionText = sectionText.Mid(offset1);
								wholeMkr = pDoc->GetWholeMarker(sectionText);
								int length = wholeMkr.Len();
								wxASSERT(length > 1);
								// toss the marker in the garbage
								sectionText = sectionText.Mid(length);
								// join the bits that remain
								if (strLeft.IsEmpty())
								{
									sectionText.Trim(FALSE); // trim white space off of its start
								}
								else
								{
									strLeft.Trim(); // trim white space off of its end
									strLeft += eolStr; // start a new line
									sectionText = strLeft + sectionText; // append the rest
								}
							} // end of TRUE block for test: if (bMatchedParagraphMkrAtEnd)
							else
							{
								// there wan't a paragraph marker -- this shouldn't
								// happen, because there would be no reason to start a paragraph
								;
							} // end of else block for test: if (bMatchedParagraphMkrAtEnd)
						} // end of else block for test: if (bMatchedNoBreakMkr)

					} // end of TRUE block for test: if (bHandledChapterChange)
					else
					{
						wxASSERT(sectionText[0] == _T('\\')); // must commence with a marker
						bMatchedOpeningParagraphMkr = FALSE;
						wholeMkr_AtParaOpening.Empty();
					}
				} // end of TRUE block for the test of wholeMkr being \c or \free or \note
			} // end of else block for the test: if (offset == wxNOT_FOUND)
		} // end of else block for the test: if (whichPara == parsingFirstParagraph)

		if (sectionText.IsEmpty())
		{
			pCurPara->paragraphChunk += parsedParagraphData;
			pSection->paraArray.Add(pCurPara);
			break;
		}

		// get the rest of the paragraph's chunk
		if (whichPara == parsingFirstParagraph)
		{
			if (bMatchedOpeningParagraphMkr)
			{
				// we can proceed with the parsing of the rest of the opening paragraph
				// data because we've gotten past the header information				
				ParseParagraphData(&sectionText, &parsedParagraphData, pCurPara);
				pCurPara->paragraphChunk += parsedParagraphData;
				parsedParagraphData.Empty();

				// when the above function returns, the paragraph chunk is complete, but
				// not stored, so store it and then start a new paragraph if possible
				if (sectionText.IsEmpty())
				{
					// the section is parsed and all its paragraphs created, it just
					// remains to store the last one which we've now completed
					pSection->paraArray.Add(pCurPara);
					break;
				}
				else
				{
					// the paragraph is completed, but there is more in sectionText, so we
					// must continue looping to create more paragraphs
					pSection->paraArray.Add(pCurPara); // store the completed one
					pCurPara = new SectionHeaderOrParagraph; // make a new one
					pCurPara->bHasHeaderInfo = FALSE;
					pCurPara->paragraphOpeningMkr.Empty();
					// set the pointer to the parent section
					pCurPara->pSectionInfo = pSection;
				}
				whichPara = parsingNonFirstParagraph; // next iteration will be a non-first paragraph
				continue;
			}
			else
			{
				// we are in the first paragraph, but have not yet parsed over all the
				// header information, so skip the parse of the paragraph data until we've
				// gotten past the header stuff
				continue;
			}
		} // end of TRUE block for test: if (whichParagraph == parsingFirstParagraph)
		else
		{
			// we are parsing a non-first paragraph...
			
            //  if we got to a chapter marker, \c, and it was not followed by a \nb "no
            //  break" marker, then we must close off this paragraph and start a new one if
            //  possible; but if we did encounter a \nb marker, we just continue the
			//  parsing forwards to find the end of the paragraph, after storing whatever
			//  we've accumulated thus far
            if (bMatchedNoBreakMkr)
			{
				pCurPara->paragraphChunk += parsedParagraphData; // store what we have so far
				parsedParagraphData.Empty();
				
				// proceed to accumulate more parsed data until the paragraph end is reached
				ParseParagraphData(&sectionText, &parsedParagraphData, pCurPara);
				pCurPara->paragraphChunk += parsedParagraphData; // append it to the chunk
				parsedParagraphData.Empty();
				if (sectionText.IsEmpty())
				{
					// the section is parsed and all its paragraphs created, it just
					// remains to store the last one which we've now completed
					pSection->paraArray.Add(pCurPara);
					break;
				}
				else
				{
					// the paragraph is completed, but there is more in sectionText, so we
					// must continue looping to create more paragraphs
					pSection->paraArray.Add(pCurPara); // store the completed one
					pCurPara = new SectionHeaderOrParagraph; // make a new one
					pCurPara->bHasHeaderInfo = FALSE;
					pCurPara->paragraphOpeningMkr.Empty();
					// set the pointer to the parent section
					pCurPara->pSectionInfo = pSection;
					continue;
				}
			}
			else
			{
				// close off the paragraph at his chapter break location, and start a new
				// paragraph
				pCurPara->paragraphChunk += parsedParagraphData; // store what we have so far
				parsedParagraphData.Empty();
				pSection->paraArray.Add(pCurPara); // store the completed one
				pCurPara = new SectionHeaderOrParagraph; // make a new one
				pCurPara->bHasHeaderInfo = FALSE;
				pCurPara->paragraphOpeningMkr.Empty();
				// set the pointer to the parent section
				pCurPara->pSectionInfo = pSection;
				continue;
			}
		}  // end of else block for test: if (whichParagraph == parsingFirstParagraph)
	} while (!sectionText.IsEmpty());

	// when control gets to here, all the paragraph structs for this section are done so
	// this is a place to put wxLogDebug calls to check the parsing is working right
	

	// *** TODO ***
	

}

//////////////////////////////////////////////////////////////////////////////////////////
/// \return         nothing
/// \param  pSectionText                <-> ptr to a copy of the section's text, the
///                                         header information is located at its start
/// \param  pHdrOrPara                  <-  ptr to the (first) SectionHeaderOrParagraph
///                                         struct, where we store parsed information
/// \param  pbReachedEndOfHdrMaterial   <-  TRUE when parsing reaches a marker not in the
///                                         header region, such as \p or \m or \q etc
/// \param  pbMatchedParagraphMkrAtEnd  <-  TRUE if after parsing over any free translation 
///                                         and/or a note, we come not to a marker type which
///                                         belongs to the header information, but one which
///                                         indicates that paragraph data is commencing (such
///                                         as a \p or other paragraph type of marker)
/// \param  pStr_wholeMkrAtOpening      <-  ptr to the wholeMarker which halted the parsing
///                                         of the header information, such as a \p or a \m
///                                         etc. It's the opening paragraph marker usually
/// \remarks
/// A lot of code for a little bit of parsing work!
/// At the start of a new section there could be a marker sequence like this:
/// \c \free \note \ms \mr \free \note \s \r \free \note \d \free \p \v 
/// or as little as:
/// \s \p \v    
/// and in the above, \ms could be any one of \ms, \ms1, \ms2, \ms3, and likewise \s could
/// be any one of \s1, \s2 or \s3. We assume \mr or \r would not contain a note nor be free
/// translated since they only contain scripture references; but \ms and/or \s and/or \d
/// are both free translatable and a user could (though it's highly unlikely) put one or
/// more notes within such a header's content. Our code has to be able to deal with all
/// these possibilities. Moreover, if a free translation and/or a note is present, they
/// will precede the marker and marker content to which they apply - and this is also true
/// when the header information's end is reached (typically at a \p marker) because the
/// following verse text if free translated and/or has a note at its start, the free
/// translation and the note would be, in that order, preceding the \p marker. Hence the
/// final free translation and/or note, in that circumstance, belongs with the verse text
/// which follows, rather than with the header information which precedes.
/// 
/// Our approach to parsing is to parse over three possible information types which
/// constitute a group, in a loop: an optional free translation, an optional note, and 
/// then a header marker of some kind which has text (which could also contain embedded
/// notes) with which the preceding free translation and note, if either or both are
/// present, are associated. What halts parsing of such a group is encountering
/// another marker in the set stored in m_allowedPreParagraphMkrs. The parser is a
/// bleeding parser - it consumes the information parsed over, storing it
/// appropriately within the passed in pHdrOrPara struct - though the actual storage
/// will be done within a further bleeding parser which this present one calls. On return,
/// the passed in sectionText should be shortened (the parsed stuff bled out) and must
/// begin with the next wholeMarker we wish to commence our parsing from in the next
/// iteration of the loop.
/// 
/// Because free translations can be shorter than the stretch of text in a header, and
/// so there may be more than one associated with the text content for a \ms or \s
/// marker, the marker's content may be broken up into smaller substrings, the
/// non-first of which start with no marker at all. Likewise, the presence of notes
/// interposes \note ... \note* substrings within a span of text. So we have to take
/// on board the possibility that the strings we want are not continuous. This problem
/// we encountered earlier in the parsing of title and introductory material, and
/// we'll modify the solution used there for use here as well - by constructing a
/// second parser (mentioned above) which will parse the \ms or \s marker content into
/// one or more aiGroup structs, and store these in pHdrOrPara struct's array members
/// for that purpose.                                 
//////////////////////////////////////////////////////////////////////////////////////////
void Usfm2Oxes::ParseHeaderInfoAndFirstParagraph(wxString* pSectionText, 
		SectionHeaderOrParagraph* pCurPara, bool* pbReachedEndOfHdrMaterial, 
		bool* pbMatchedParagraphMkrAtEnd, wxString* pStr_wholeMkrAtOpening)
{
	int i = 0;
	i = i + 1;




}

//////////////////////////////////////////////////////////////////////////////////////////
/// \return         nothing
/// \param  pSectionText                <-> ptr to a copy of the section's text (there
///                                         could be a chapter change in a non-first
///                                         paragraph chunk within the section's text)
/// \param  pHdrOrPara                  <-  ptr to this instance of SectionHeaderOrParagraph
///                                         struct, where we store parsed information
/// \param  pbHandledChapterChange      <-  TRUE when parsing reaches a \c marker (normally
///                                         a new paragraph will commence, but not if a \nb
///                                         marker follows it)
/// \param  pbMatchedParagraphMkrAtEnd  <-  TRUE if after parsing over any free translation 
///                                         and/or a note, we come to a marker type which 
///                                         indicates that paragraph data is commencing (such
///                                         as a \p or other paragraph type of marker)
/// \param  pStr_wholeMkrAtOpening      <-  ptr to the wholeMarker which halted the parsing
///                                         of the header information, such as a \p or a \m
///                                         etc; its the opening para marker for the paragraph
/// \param  pbMatchedNoBreakMkr         <-  TRUE if a \nb marker follows a \c marker
///                                         (indicating there is no paragraph break at the
///                                         chapter break eg. Luke 7)
/// \remarks
/// This function is like ParseHeaderInfoAndFirstParagraph() except it is internally
/// simpler because in non-initial paragraphs within a section there cannot be any section
/// header information (if there were, a new section would have been commenced there). Our
/// approach to the parsing is described in the comments for the other function mentioned
/// above, and we need similar information returned, plus one extra boolean - hence the
/// signature is almost the same, but internally ParseNonFirstParagraph has much less to
/// deal with, and any free translation and/or note information automatically are
/// associated with the verse text which lies within the paragraph, because nobody could
/// ever free translate a chapter number nor try to attach a note to it. The extra
/// parameter, pbMatchedNoBreakMkr, allows the caller to work out whether or not it needs
/// to initiate a new paragraph when there has been a chapter change. One of the booleans
/// changes its name also because it does a different job, i.e. pbHandledChapterChange,
/// because the caller needs to know that a \c marker was encountered mid-section.
//////////////////////////////////////////////////////////////////////////////////////////
void Usfm2Oxes::ParseNonFirstParagraph(wxString* pSectionText, 
		SectionHeaderOrParagraph* pCurPara, bool* pbHandledChapterChange,
		bool* pbMatchedParagraphMkrAtEnd, wxString* pStr_wholeMkrAtOpening,
		bool* pbMatchedNoBreakMkr)
{
	int i = 0;
	i = i + 1;



}

void Usfm2Oxes::ParseParagraphData(wxString* pSectionText, wxString* pAccumulatedParagraphData,
								   SectionHeaderOrParagraph* pCurPara)
{
	int i = 0;
	i = i + 1;


}

// clear an AISectHdrOrParaArray of its set of SectionHeaderOrParagraph struct instances
void Usfm2Oxes::ClearAISectHdrOrParaArray(AISectHdrOrParaArray& rParagraphArray)
{
	if (!rParagraphArray.IsEmpty())
	{
		int count = rParagraphArray.GetCount();
		int index;
		for (index = 0; index < count; index++)
		{
			SectionHeaderOrParagraph* pParagraph = rParagraphArray.Item(index);
			// temporarily, just delete, it has only wxString members so far, but when we
			// get to the point of having it store an array of substructures, we'll need a
			// prior call to a clearing function here as well
			// *** TODO *** -- any more than the two below???
			ClearAIGroupArray(pParagraph->majorHeaderGroupArray);
			ClearAIGroupArray(pParagraph->sectionHeaderGroupArray);
			delete pParagraph;
		}
		rParagraphArray.Clear(); // remove its stored pointers which are all now hanging
	}
}



*/




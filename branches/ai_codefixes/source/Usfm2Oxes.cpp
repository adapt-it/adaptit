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
#include <wx/tokenzr.h>
#endif

#if defined(__VISUALC__) && __VISUALC__ >= 1400
#pragma warning(disable:4428)	// VC 8.0 wrongly issues warning C4428: universal-character-name 
								// encountered in source for a statement like 
								// ellipsis = _T('\u2026');
								// which contains a unicode character \u2026 in a string literal.
								// The MSDN docs for warning C4428 are also misleading!
#endif

#include <wx/arrimpl.cpp>

#include "AdaptitConstants.h"
#include "Adapt_It.h"
#include "Adapt_ItDoc.h"
#include "helpers.h"
#include "BString.h"
#include "Usfm2Oxes.h"

WX_DEFINE_OBJARRAY(NoteDetailsArray);
WX_DEFINE_OBJARRAY(AIGroupArray);

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

	Initialize();
}

Usfm2Oxes::~Usfm2Oxes()
{
	ClearTitleInfo();
	delete m_pTitleInfo;
	ClearIntroInfo();
	delete m_pIntroInfo;
}

void Usfm2Oxes::SetBookID(wxString& aValidBookCode)
{
	m_bookID = aValidBookCode;
}

void Usfm2Oxes::Initialize()
{
	// do only once-only data structure setups here; each time a new oxes file is to be
	// produced, the stuff specific to any earlier exports will need to be cleared out
	// before the new one's data is added in (using a separate function)
	
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
	m_haltingMarkers = _T("\\v \\c \\p \\m \\q \\qc \\qm \\qr \\qa \\pi \\mi \\pc \\pt \\ps \\pgi \\cl \\vn \\f \\fe \\x \\gd \\tr \\th \thr \\tc \tcr \\mt \\st \\mte \\div \\ms \\s \\sr \\sp \\d \\di \\hl \\r \\dvrf \\mr \\br \\rr \\pp \\pq \\pm \\pmc \\pmr \\cls \\li \\qh \\gm \\gs \\gd \\gp \\tis \\tpi \\tps \\tir \\pb \\hr ");
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
	// create the structs: TitleInfo struct, IntroInfo struct
	m_pTitleInfo = new TitleInfo;
	m_pIntroInfo = new IntroductionInfo;

	// this stuff is cleared out already, but no harm in ensuring it
	m_pTitleInfo->bChunkExists = FALSE;
	m_pTitleInfo->strChunk.Empty();

	m_pIntroInfo->bChunkExists = FALSE;
	m_pIntroInfo->strChunk.Empty();

	// set up the set-up-only-once arrays
	wxString mkr;
	mkr = _T("\\id");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\ide");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\h");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\h1");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\h2");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\h3");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\mt");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\mt1");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\mt2");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\mt3");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\st");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	// permit Adapt It notes and free translations to be present in the chunk
	// I changed my mind - I'll support the two custom markers in the functions as special
	// cases rather than including them in the marker lists in data structures

    // the wxString members are initially empty so we can leave them, but a separate
    // function, ClearTitleInfo, must clean them out every time a new oxes xml file is
    // being produced

	// for the Introduction material, m_introHaltingMarkers has the markers with their
	// levels, so we can populate m_pIntroInfo->arrPossibleMarkers more easily than above
	// by using a function to do it
	PopulateIntroductionPossibleMarkers(m_pIntroInfo->arrPossibleMarkers);



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
	ClearAIGroupArray(m_pTitleInfo->aiGroupArray);
}

void Usfm2Oxes::ClearIntroInfo()
{
    // this function cleans out the file-specific data present in the IntroInfo struct from
    // the last-built oxes file, in preparation for building a new oxes file
	m_pIntroInfo->bChunkExists = FALSE;
	m_pIntroInfo->strChunk.Empty();
	ClearAIGroupArray(m_pIntroInfo->aiGroupArray);
}


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

void Usfm2Oxes::SetOXESVersionNumber(int versionNum)
{
	m_version = versionNum; // set the private member for the version number
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

// if str has an exact match in the passed in array, return TRUE, else return FALSE
// BEW 15Sep10, added the protocol that if str is empty, then TRUE is returned immediately
// - so that text without a preceding marker is still considered part of the current
// chunk, as IsOneOf() is used for determining what does or doesn't belong to a chunk
bool Usfm2Oxes::IsOneOf(wxString& str, wxArrayString& array, 
				CustomMarkersFT inclOrExclFreeTrans, CustomMarkersN inclOrExclNote)
{
	if (str.IsEmpty())
	{
		return TRUE; // the empty string (ie. str is not a marker) indicates membership
					 // in the chunk nevertheless
	}
	size_t count = array.GetCount();
	if (count == 0)
		return FALSE;
	size_t index;
	wxString testStr;
	for (index = 0; index < count; index++)
	{
		testStr = array.Item(index);
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
	wxASSERT((*pInputBuffer)[0] == _T('\\')); // we must be pointing at a marker
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	bool bEmbeddedSpan = FALSE; // would be true if we parsed a \f, \fe or \x marker
								// but these are unlikely in Title chunks
	int span = 0;
	wxString buff = *pInputBuffer; // work with a copy of the buffer contents until
								   // we are ready to bleed off the title info chunk
	// we need a counter for characters in the TitleInfo chunk
	int charsDefinitelyInChunk = 0;
	// and two more for the last free trans and /or note which ultimately may not belong
	// in the span and so will need to have their spans not included in the
	// charsDefinitelyInChunk value when the chunk boundary is finally known
	int lastFreeTransSpan = 0;
	int lastNoteSpan = 0;
	// scratch strings
	wxString wholeEndMkr;
	wxString dataStr;

	// begin...  
	wxString wholeMkr = pDoc->GetWholeMarker(buff);
	bool bBelongsInChunk = IsOneOf(wholeMkr, m_pTitleInfo->arrPossibleMarkers, 
								includeFreeTransInTest, includeNoteInTest);
	while (bBelongsInChunk)
	{
		// check if we are pointing at a \free marker
		if (m_bContainsFreeTrans && wholeMkr == m_freeMkr)
		{
            // identify the free translation chunk & count its characters; this function
            // always exists with span giving the offset to the marker which halted
            // scanning (or end of buffer)
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
						bEmbeddedSpan, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing);
			lastFreeTransSpan = span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
		}
		else if (m_bContainsNotes && wholeMkr == m_noteMkr)
		{
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
						bEmbeddedSpan, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing);
			lastNoteSpan = span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
		}
		else
		{
            // it's some other marker than \free or \note, and it belongs in the TitleInfo
            // chunk, so scan over its data & count that, and add in the counts for
            // preceding free translation and/or note if either or both of these were
			// previously scanned; but the tests within this parse must ignore any
			// embedded notes, so the last param will be ignoreNoteWhenParsing
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
						bEmbeddedSpan, haltAtFreeTransWhenParsing, ignoreNoteWhenParsing);
			charsDefinitelyInChunk += span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();

			// update the count for any free trans and/or note parsed over earlier
			if (lastFreeTransSpan > 0)
			{
				charsDefinitelyInChunk += lastFreeTransSpan;
				lastFreeTransSpan = 0;
			}
			if (lastNoteSpan > 0)
			{
				charsDefinitelyInChunk += lastNoteSpan;
				lastNoteSpan = 0;
			}
		}

		// check next marker, and iterate or exit as the case may be
		wholeMkr = pDoc->GetWholeMarker(buff);
		bBelongsInChunk = IsOneOf(wholeMkr, m_pTitleInfo->arrPossibleMarkers, 
									includeFreeTransInTest, includeNoteInTest);
	}
	// when control gets to here, we've just identified a SF marker which does not belong
	// within the TitleInfo chunk; it such a marker's content had a free translation or note or
	// both defined for it's information then we just throw away the character counts for such
	// info types - so we've nothing to do now other than use the current
	// charsDefinitelyInChunk value to bleed off the chunk from pInputBuffer and store it
	m_pTitleInfo->strChunk = (*pInputBuffer).Left(charsDefinitelyInChunk);
	(*pInputBuffer) = (*pInputBuffer).Mid(charsDefinitelyInChunk);

	if (charsDefinitelyInChunk > 0)
		m_pTitleInfo->bChunkExists = TRUE;

	// check it works - yep
	wxLogDebug(_T("GetTitleInfoChunk  num chars = %d\n%s"),charsDefinitelyInChunk, m_pTitleInfo->strChunk.c_str());
	return pInputBuffer;
}

wxString* Usfm2Oxes::GetIntroInfoChunk(wxString* pInputBuffer)
{
	wxASSERT((*pInputBuffer)[0] == _T('\\')); // we must be pointing at a marker
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	bool bEmbeddedSpan = FALSE; // initialize
	int span = 0;
	wxString buff = *pInputBuffer; // work with a copy of the buffer contents until
						// we are ready to bleed off the introduction info chunk
	// we need a counter for characters in the IntroInfo chunk
	int charsDefinitelyInChunk = 0;
	// and two more for the last free trans and /or note which ultimately may not belong
	// in the span and so will need to have their spans not included in the
	// charsDefinitelyInChunk value when the chunk boundary is finally known
	int lastFreeTransSpan = 0;
	int lastNoteSpan = 0;
	// scratch strings
	wxString wholeEndMkr;
	wxString dataStr;

	// begin...  
	wxString wholeMkr = pDoc->GetWholeMarker(buff);
	bool bBelongsInChunk = IsOneOf(wholeMkr, m_pIntroInfo->arrPossibleMarkers, 
									includeFreeTransInTest, includeNoteInTest);
	while (bBelongsInChunk)
	{
		// check if we are pointing at a \free marker
		if (m_bContainsFreeTrans && wholeMkr == m_freeMkr)
		{
            // identify the free translation chunk & count its characters; this function
            // always exists with span giving the offset to the marker which halted
            // scanning (or end of buffer)
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, 
				dataStr, bEmbeddedSpan, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing);
			lastFreeTransSpan = span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
		}
		else if (m_bContainsNotes && wholeMkr == m_noteMkr)
		{
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, 
				dataStr, bEmbeddedSpan, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing);
			lastNoteSpan = span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
		}
		else
		{
            // it's some other marker than \free or \note, and it belongs in the TitleInfo
            // chunk, so scan over its data & count that, and add in the counts for
            // preceding free translation and/or note if either or both of these were
			// previously scanned; but the tests within this parse must ignore any
			// embedded notes, so the last param will be ignoreNoteWhenParsing
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, 
				dataStr, bEmbeddedSpan, haltAtFreeTransWhenParsing, ignoreNoteWhenParsing);
			charsDefinitelyInChunk += span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();

			// update the count for any free trans and/or note parsed over earlier
			if (lastFreeTransSpan > 0)
			{
				charsDefinitelyInChunk += lastFreeTransSpan;
				lastFreeTransSpan = 0;
			}
			if (lastNoteSpan > 0)
			{
				charsDefinitelyInChunk += lastNoteSpan;
				lastNoteSpan = 0;
			}
		}

		// check next marker, and iterate or exit as the case may be
		wholeMkr = pDoc->GetWholeMarker(buff);
		bBelongsInChunk = IsOneOf(wholeMkr, m_pIntroInfo->arrPossibleMarkers, 
									includeFreeTransInTest, includeNoteInTest);
	}
	// when control gets to here, we've just identified a SF marker which does not belong
	// within the TitleInfo chunk; it such a marker's content had a free translation or note or
	// both defined for it's information then we just throw away the character counts for such
	// info types - so we've nothing to do now other than use the current
	// charsDefinitelyInChunk value to bleed off the chunk from pInputBuffer and store it
	m_pIntroInfo->strChunk = (*pInputBuffer).Left(charsDefinitelyInChunk);
	(*pInputBuffer) = (*pInputBuffer).Mid(charsDefinitelyInChunk);

	if (charsDefinitelyInChunk > 0)
		m_pIntroInfo->bChunkExists = TRUE;

	// check it works - yep
	wxLogDebug(_T("GetIntroInfoChunk  num chars = %d\n%s"),charsDefinitelyInChunk, m_pIntroInfo->strChunk.c_str());
	return pInputBuffer;
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

		// clear structs of any old data
		ClearTitleInfo();
		ClearIntroInfo();

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
/// someone typed an enmarker but typed it wrongly so that the actual endmarker turned out
/// to be an unknown endmarker or an endmarker for some other known marker - in either of
/// those scenarios, bEndMarkerError is returned TRUE, and the caller should abort the OXES
/// parse with an appropriate warning to the user.
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
						CustomMarkersFT inclOrExclFreeTrans, CustomMarkersN inclOrExclNote)
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
	if (*ptr == _T('\\')) 
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
				if (bIsInlineMkr || (inclOrExclNote == excludeNoteFromTest)
								 || (inclOrExclFreeTrans == excludeFreeTransFromTest))
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
		// check if we are pointing at a \free marker
		if (m_bContainsFreeTrans && wholeMkr == m_freeMkr)
		{
            // identify the free translation chunk & count its characters; this function
            // always exists with span giving the offset to the marker which halted
            // scanning (or end of buffer)
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
						bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing);
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
						bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing);
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
				span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
							bHasInlineMarker, haltAtFreeTransWhenParsing, ignoreNoteWhenParsing);

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
	} while (!buff.IsEmpty());
	// when control gets to here, we've consumed the chunk copy stored in m_pTitleInfo

	// check we got the structs filled out correctly
#if defined __WXDEBUG__	
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
		// if there is no title information, return without doing anything
		return;
	}
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	bool bHasInlineMarker = FALSE; // would be true if we parsed a \f, \fe or \x marker,
		// or an inline formatting marker, but all these are unlikely in Title chunks
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

	// begin parsing...  (for TitleInfo, each information chunk will begin with an SF
	// marker, and ParseMarker_Content_Endmarker() relies on that being true)
	wxString wholeMkr = pDoc->GetWholeMarker(buff);
	aiGroup* pGroupStruct = new aiGroup;
	pGroupStruct->bHasInlineMarker = FALSE;
	pGroupStruct->usfmBareMarker.Empty(); // never store a "free" marker, OXES 
		// knows nothing about such markers; store only the main marker, or empty string
	do
	{
		// check if we are pointing at a \free marker
		if (m_bContainsFreeTrans && wholeMkr == m_freeMkr)
		{
            // identify the free translation chunk & count its characters; this function
            // always exists with span giving the offset to the marker which halted
            // scanning (or end of buffer)
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr, 
					bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing);
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
						bHasInlineMarker, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing);
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
							bHasInlineMarker, haltAtFreeTransWhenParsing, ignoreNoteWhenParsing);

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

                // we extract any embedded notes and store those in additional NoteDetails
                // stucts, and store the text in the aiGroup's textStr member -- all is
                // done in the next call
				ExtractNotesAndStoreText(pGroupStruct, dataStr);
				dataStr.Empty();

				// store the aiGroup struct's pointer in the TitleInfo's AIGroupArray array
				// now that it is completed
				m_pIntroInfo->aiGroupArray.Add(pGroupStruct);
				pGroupStruct = NULL;
			} // end of TRUE block for test: 
			  // if (IsAHaltingMarker(buff, haltAtFreeTransWhenParsing, haltAtNoteWhenParsing))
			else
			{
				// assume it is text, with or without embedded notes and with or without
				// inline formatting markers; but the chunk may be far from finished, --
				// there could be several text chunks interspersed between notes, so
				// we can't assume there are no more halting markers not no-more
				// markerless buffer beginning for the current aiGroup - so do the usual
				// parse
				span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr, 
						bHasInlineMarker, haltAtFreeTransWhenParsing, ignoreNoteWhenParsing);

				buff = buff.Mid(span);

				ExtractNotesAndStoreText(pGroupStruct, dataStr);
				dataStr.Empty();

				// store the aiGroup struct's pointer in the TitleInfo's AIGroupArray array
				// now that it is completed
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
	// when control gets to here, we've consumed the chunk copy stored in m_pIntroInfo

	// check we got the structs filled out correctly
#if defined __WXDEBUG__	
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
		wxLogDebug(_T("*** TitleInfo ***   There is no title information in this export"));
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
			wxLogDebug(_T("    Usfm bare Mkr  =      %s"),pGrp->usfmBareMarker);
			wxLogDebug(_T("    freeTransStr   =      %s"),pGrp->freeTransStr);
			int count2 = pGrp->arrNoteDetails.GetCount();
			if (count2 > 0)
			{
				int index2;
				for (index2=0; index2<count2; index2++)
				{
					NoteDetails* pDetails = pGrp->arrNoteDetails.Item(index2);
					wxLogDebug(_T("    * NoteDetails *     NoteDetail with index2 = %d"), index2);
					wxLogDebug(_T("        noteText = %s"),pDetails->noteText);
					wxLogDebug(_T("        beginOffset %d , endOffset %d , wordsInSpan = %s , Usfm bare Mkr = %s"),
						pDetails->beginOffset, pDetails->endOffset, pDetails->wordsInSpan, pDetails->usfmMarker);
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
				wxLogDebug(_T("    arrMarkers         %s"),markers);
			}
			else
			{
				wxLogDebug(_T("    arrMarkers         [empty]"));
			}
			if (!pGrp->chapterNumber.IsEmpty())
			{
				wxLogDebug(_T("    chapterNum         %s"),pGrp->chapterNumber);
			}
			if (!pGrp->verseNumber.IsEmpty())
			{
				wxLogDebug(_T("    verseNum           %s"),pGrp->verseNumber);
			}
			if (!pGrp->verseNumberBridgeStr.IsEmpty())
			{
				wxLogDebug(_T("    verseNumBridgeStr  %s"),pGrp->verseNumberBridgeStr);
			}
			wxLogDebug(_T("    textStr      =       %s"),pGrp->textStr);
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
			wxLogDebug(_T("    Usfm bare Mkr  =      %s"),pGrp->usfmBareMarker);
			wxLogDebug(_T("    freeTransStr   =      %s"),pGrp->freeTransStr);
			int count2 = pGrp->arrNoteDetails.GetCount();
			if (count2 > 0)
			{
				int index2;
				for (index2=0; index2<count2; index2++)
				{
					NoteDetails* pDetails = pGrp->arrNoteDetails.Item(index2);
					wxLogDebug(_T("    * NoteDetails *     NoteDetail with index2 = %d"), index2);
					wxLogDebug(_T("        noteText = %s"),pDetails->noteText);
					wxLogDebug(_T("        beginOffset %d , endOffset %d , wordsInSpan = %s , Usfm bare Mkr = %s"),
						pDetails->beginOffset, pDetails->endOffset, pDetails->wordsInSpan, pDetails->usfmMarker);
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
				wxLogDebug(_T("    arrMarkers         %s"),markers);
			}
			else
			{
				wxLogDebug(_T("    arrMarkers         [empty]"));
			}
			if (!pGrp->chapterNumber.IsEmpty())
			{
				wxLogDebug(_T("    chapterNum         %s"),pGrp->chapterNumber);
			}
			if (!pGrp->verseNumber.IsEmpty())
			{
				wxLogDebug(_T("    verseNum           %s"),pGrp->verseNumber);
			}
			if (!pGrp->verseNumberBridgeStr.IsEmpty())
			{
				wxLogDebug(_T("    verseNumBridgeStr  %s"),pGrp->verseNumberBridgeStr);
			}
			wxLogDebug(_T("    textStr      =       %s"),pGrp->textStr);
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
/// any inline markers can be parsed for an converted, in place, to the appropriate OXES
/// tag, or in the case of an enmarker, an endtag. Eg. <keyWord> ..... </keyWord>.
/// Any notes which we extract here have to be stored in a NoteDetails struct, and its
/// pointer stored in the arrNoteDetails member of pCurAIGroup. OXES places all the notes
/// pertaining to a <trGroup> at its start, in sequence, and with an attribute giving the
/// offset (in characters) from the start of the text for that group, to the location to
/// which the note applies -- for adapt it, the latter will always be either a word or a
/// phrase (a merger) - from the single CSourcePhrase where the note was stored.
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
	// theText - these we parse over and store along with the the text - these are changed
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
					// tell the developer & drop into the debugger
					wxString aWrongMkr = pDoc->GetWholeMarker(ptr);
					wxString msg;
					msg = msg.Format(_T(
"ExtractNotesAndStoreText(): found non-inline marker %s\nExpected an inline formatting marker or endmarker."),
					aWrongMkr.c_str());
					wxMessageBox(msg, _T("Logic error in the code"), wxICON_ERROR);
					wxASSERT(FALSE);
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






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
    #pragma implementation "FreeTrans.h"
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

//#include <wx/object.h>

#include "AdaptitConstants.h"
#include "Adapt_It.h"
#include "Adapt_ItDoc.h"
#include "helpers.h"
#include "BString.h"
#include "Usfm2Oxes.h"


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

	// the following are not in the global wxString, charFormatMkrs defined in
	// Adapt_ItView.cpp file, so we define them here and add these to those so that
	// m_specialMrks and m_specialEndMkrs will comply with USFM 2.3 with respect to
	// special markers (but we won't try support them all, but only these command and
	// whichever extras users ask for; we do however need to know them all for parsing
	// purposes)
	m_specialMkrs = _T("\\add \\ord \\sig \\sls \\fig \\ndx \\pro \\w \\wq \\wh \\lit \\pb ");
	m_specialEndMkrs = _T("\\add \\ord \\sig \\sls \\fig \\ndx \\pro \\w \\wq \\wh \\lit \\pb ");
	m_specialMkrs = charFormatMkrs + m_specialMkrs;
	m_specialEndMkrs = charFormatEndMkrs + m_specialEndMkrs;

	Initialize();
}

Usfm2Oxes::~Usfm2Oxes()
{
	delete m_pTitleInfo;


}


void Usfm2Oxes::Initialize()
{
	// do only once-only data structure setups here; each time a new oxes file is to be
	// produced, the stuff specific to any earlier exports will need to be cleared out
	// before the new one's data is added in (using a separate function)
	
	// set the two custom markers that Adapt It recognises
	m_freeMkr = _T("\\free"); // our Adapt It custom marker for OXES backtranslations
	m_noteMkr = _T("\\note"); // our Adapt It custom marker for OXES annotation with 
									 // type 'translatorNote' and category 'adaptationNote'

 
    // Make the wxString of 'halting markers' -- utilize Bill's commonHaltingMarkers string
    // and add more (eg \h) but don't add \free or note (commonHaltingMarkers is definied
	// in Adapt_ItView.cpp) We'll handle our two custom ones as special tests, not as part
	// of the USFM standard's markers
	m_haltingMarkers = commonHaltingMarkers;
	wxString additions = _T("\\h ");
	m_haltingMarkers = additions + m_haltingMarkers;
	
	// create the TitleInfo struct
	m_pTitleInfo = new TitleInfo;

	// this stuff is cleared out already, but no harm in ensuring it
	m_pTitleInfo->bookCode.Empty();
	m_pTitleInfo->bChunkExists = FALSE;
	m_pTitleInfo->strChunk.Empty();

	m_pTitleInfo->arrMkrSequInData.Clear();
	m_pTitleInfo->arrMkrConvertFrom.Clear();
	m_pTitleInfo->arrMkrConvertTo.Clear();

	// set up the arrays - this is to be done only once;
	// the arrMkrConvertFrom and arrMkrConvertTo arrays contain markers paired by the same
	// index value, hence index = 0 gets "\st" from the From array and "\mt2" from the To
	// array -- meaning that if \st is detected in the chunk, it is to be changed to \mt2
	// before the chunk is processed further (we test for any needed conversions before
	// doing anything else with the chunk)
	wxString mkr;
	mkr = _T("\\id");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\ide");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\h");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	m_pTitleInfo->arrMkrConvertFrom.Add(mkr);
	mkr = _T("\\h1");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	m_pTitleInfo->arrMkrConvertTo.Add(mkr); // we want to always convert a \h to \h1
	mkr = _T("\\h2");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\h3");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\mt");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	m_pTitleInfo->arrMkrConvertFrom.Add(mkr);
	mkr = _T("\\mt1");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	m_pTitleInfo->arrMkrConvertTo.Add(mkr); // always convert \mt to \mt1
	mkr = _T("\\mt2");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	m_pTitleInfo->arrMkrConvertTo.Add(mkr); // always convert PNG 1998 marker \st 
											// "secondary title" to USFM \mt2
	mkr = _T("\\mt3");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	mkr = _T("\\st");
	m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	m_pTitleInfo->arrMkrConvertFrom.Add(mkr);
	// permit Adapt It notes and free translations to be present in the chunk
	// I changed my mind - I'll support the two custom markers in the functions as special
	// cases rather than including them in the marker lists in data structures
	//mkr = _T("\\free");
	//m_pTitleInfo->arrPossibleMarkers.Add(mkr);
	//mkr = _T("\\note");
	//m_pTitleInfo->arrPossibleMarkers.Add(mkr);

    // the wxString members are initially empty so we can leave them, but a separate
    // function, ClearTitleInfo, must clean them out every time a new oxes xml file is
    // being produced

	// debug: check things are working to this point
	//m_pTitleInfo->idStr = _T("g'day mate");
	//wxLogDebug(_T("Field: idStr =  %s"),m_pTitleInfo->idStr);
}

void Usfm2Oxes::ClearTitleInfo()
{
    // this function cleans out the file-specific data present in the TitleInfo struct from
    // the last-built oxes file, in preparation for building a new oxes file
	m_pTitleInfo->bookCode.Empty();
	m_pTitleInfo->bChunkExists = FALSE;
	m_pTitleInfo->strChunk.Empty();

	m_pTitleInfo->arrMkrSequInData.Clear();

	m_pTitleInfo->idStr.Empty();
	m_pTitleInfo->hStr.Empty();
	m_pTitleInfo->h1Str.Empty();
	m_pTitleInfo->h2Str.Empty();
	m_pTitleInfo->h3Str.Empty();
	m_pTitleInfo->mtStr.Empty();
	m_pTitleInfo->mt1Str.Empty();
	m_pTitleInfo->mt2Str.Empty();
	m_pTitleInfo->mt3Str.Empty();
	m_pTitleInfo->stStr.Empty();
	m_pTitleInfo->freeStr.Empty();
	m_pTitleInfo->noteStr.Empty();
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
bool Usfm2Oxes::IsOneOf(wxString& str, wxArrayString& array, CustomMarkers inclOrExcl)
{
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
	if (inclOrExcl == includeCustomMarkers)
	{
		// handle \free or \note as allowed matches
		if (str == m_freeMkr || str == m_noteMkr)
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
// character. If not pointing at a backslash, then return FALSE immediately
bool Usfm2Oxes::IsAHaltingMarker(wxChar* pChar, CustomMarkers inclOrExcl)
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
	if (inclOrExcl == includeCustomMarkers)
	{
		// test our two custom ones - \free & \note
		if (wholeMkr == m_freeMkr || wholeMkr == m_noteMkr)
		{
			return TRUE;
		}
	}
	return FALSE; // no match
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
	bool bFreeTranslationPrecedes = FALSE;
	bool bNotePrecedes = FALSE;
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
	bool bBelongsInChunk = IsOneOf(wholeMkr, m_pTitleInfo->arrPossibleMarkers, includeCustomMarkers);
	while (bBelongsInChunk)
	{
		// check if we are pointing at a \free marker
		if (m_bContainsFreeTrans && wholeMkr == m_freeMkr)
		{
			// set the flag, because the free translation may belong to a marker type
			// which does not itself belong in the TitleInfo chunk, such as the first
			// marker within a following Introduction section, etc
			bFreeTranslationPrecedes = TRUE;
            // identify the free translation chunk & count its characters; this function
            // always exists with span giving the offset to the marker which halted
            // scanning (or end of buffer)
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
												bEmbeddedSpan, includeCustomMarkers);
			lastFreeTransSpan = span;
			// bleed out the scanned over material
			buff = buff.Mid(span);
			dataStr.Empty();
		}
		else if (m_bContainsNotes && wholeMkr == m_noteMkr)
		{
			bNotePrecedes = TRUE;
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
												bEmbeddedSpan, includeCustomMarkers);
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
			// previously scanned
			span = ParseMarker_Content_Endmarker(buff, wholeMkr, wholeEndMkr, dataStr,
												bEmbeddedSpan, includeCustomMarkers);
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
			// re-initialize the booleans for the next iteration
			bFreeTranslationPrecedes = FALSE;
			bNotePrecedes = FALSE;
		}

		// check next marker, and iterate or exit as the case may be
		wholeMkr = pDoc->GetWholeMarker(buff);
		bBelongsInChunk = IsOneOf(wholeMkr, m_pTitleInfo->arrPossibleMarkers, includeCustomMarkers);
	}
	// when control gets to here, we've just identified a SF marker which does not belong
	// within the TitleInfo chunk; it such a marker's content had a free translation or note or
	// both defined for it's information then we just throw away the character counts for such
	// info types - so we've nothing to do now other than use the current
	// charsDefinitelyInChunk value to bleed off the chunk from pInputBuffer and store it
	m_pTitleInfo->strChunk = (*pInputBuffer).Left(charsDefinitelyInChunk);
	(*pInputBuffer) = (*pInputBuffer).Mid(charsDefinitelyInChunk);
	return pInputBuffer;
}

wxString Usfm2Oxes::DoOxesExport(wxString& buff)
{
	m_pBuffer = &buff;

	// more per-OXES-export initializations
	m_bContainsFreeTrans = (m_pBuffer->Find(m_freeMkr) != wxNOT_FOUND) ? TRUE : FALSE; 
	m_bContainsNotes = (m_pBuffer->Find(m_noteMkr) != wxNOT_FOUND) ? TRUE : FALSE; 
	 
	 
	// get the text of USFM marked up translation etc
	bool bEmbeddedSpan = FALSE; // true, for \f, \fe or \x markup when parsing
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
		// Parsing can now begin. Our first task is to parse through any title information
		


// **** TODO ***** 
		// we won't parse this way, first, we'll use GetTitleInfoChunk() to bleed off the chunk,
		// and then we'll parse only within the chunk -- so some of the code below will
		// move to that function...
		
		do {
			// get mkr, endMkr, the dataStr and whether or not embedded info (footnote,
			// endnote or cross reference) was parsed over - return num of chars parsed
			int span = ParseMarker_Content_Endmarker(*m_pBuffer, mkr, endMkr, dataStr,
												bEmbeddedSpan, includeCustomMarkers);
			// does this data belong to the Title info chunk?
			if (IsOneOf(mkr,m_pTitleInfo->arrPossibleMarkers,includeCustomMarkers))
			{
				// it belongs to this chunk, so process it...
				m_pTitleInfo->bChunkExists = TRUE;
				wxString parsedChunk(*m_pBuffer,span); // make a wxString for the characters parsed over
				m_pTitleInfo->strChunk += parsedChunk; // store the parsed over characters in strChunk
				*m_pBuffer = (*m_pBuffer).Mid(span); // consume the parsed over characters
				// record the order in which markers are encountered
				m_pTitleInfo->arrMkrSequInData.Add(mkr);
				// test if it is a \free marker, if so, remove the initial |@nnn@| string
				if (mkr == _T("\\free"))
				{
					int anOffset = dataStr.Find(_T("@|"));
					if (anOffset != wxNOT_FOUND)
					{
						dataStr = dataStr.Mid(anOffset + 2);
						dataStr.Trim(FALSE); // trim on left
					}
					// we can store this one immediately
					m_pTitleInfo->freeStr = dataStr;
				}

			}
			else
			{
				// does not belong to this chunk
				break;
			}
		} while (TRUE);

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
/// \param  inclOrExcl ->  either includeCustomMarkers, or excludeCustomMarkers (no default)
/// \remarks
/// On entry, buffer is the whole of the remaining unparsed adaptation's content, and it's
/// first character is a begin marker's backslash.
/// The beginning marker is parsed, being returning in the mkr param, and parsing
/// continues across its content, returning that in dataStr, and if an endmarker is
/// present then that is parsed and returned in endMkr, and parsing continues beyond the
/// endmarker until the start of the next marker is encountered. Protection is built in so
/// that if a marker's expected endmarker is absent because of a SFM markup error, the
/// function will not return any endmarker but will still return the marker's content and
/// halt at the start of the next marker - thus making it tolerant of the lack of an
/// expected endmarker. (It is not, however, tolerant of a mismatched endmarker, such as
/// if someone typed an enmarker but typed it wrongly so that the actual endmarker turned
/// out to be an unknown endmarker or an endmarker for some other known marker - in either
/// of those scenarios, bEndMarkerError is returned TRUE, and the caller should abort the
/// OXES parse with an appropriate warning to the user.
/// The custom markers are, of course, \free or \note (OXES support ignores \bt, and any
/// such are removed from the USFM export before this class gets to see the data)
/// This function is based loosely on the ExportFunctions.cpp function:
/// int ParseMarkerAndAnyAssociatedText(wxChar* pChar, wxChar* pBuffStart,
///				wxChar* pEndChar, wxString bareMarkerForLookup, wxString wholeMarker,
///				bool parsingRTFText, bool InclCharFormatMkrs)
/// BEW created 6Sep10
int Usfm2Oxes::ParseMarker_Content_Endmarker(wxString& buffer, wxString& mkr, wxString& endMkr,
					 wxString& dataStr, bool& bEmbeddedSpan, CustomMarkers inclOrExcl)
{
	bEmbeddedSpan = FALSE;
	int dataLen = 0;
	int aLength = 0;
	size_t bufflen = buffer.Len();
	const wxChar* pBuffStart = buffer.GetData();
    // remove the const modifier for pBuffStart; however, we don't modify the buffer
    // directly within this function, instead we leave that for the caller to do; all we
    // want here is to be able to get a wxChar* iterator for parsing purposes
	wxChar* ptr = const_cast<wxChar*>(pBuffStart);
	//wxChar* pEnd = const_cast<wxChar*>(pBuffStart) + bufflen;
	wxChar* pEnd = ptr + bufflen;
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	wxASSERT(*ptr == _T('\\')); // we should be pointing at the backslash of a marker

	endMkr.Empty(); // default to being empty - we may define an endmarker later below
	dataStr.Empty(); // ensure this starts out empty
	wxString wholeMarker = pDoc->GetWholeMarker(ptr); // has an initial backslash
	wxString bareMarkerForLookup = pDoc->GetBareMarkerForLookup(ptr); // lacks backslash
	mkr = wholeMarker; // caller needs to know what marker it was

    // use LookupSFM which properly handles \bt... forms as \bt in its lookup (the change
    // only persists for lookup - so we will need to check if South Asia Group markers like
    // \btv etc were matched and skip them - we don't support such markers as OXES has no
	// clue what to do with them (any such will return the USFMAnalysis struct for and
	// Adapt It collected back translation marker, \bt)
	wxString lookedUpEndMkr; lookedUpEndMkr.Empty();
	USFMAnalysis* pSfm = pDoc->LookupSFM(bareMarkerForLookup); 
	bool bNeedsEndMarker;
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
	wxChar* pContentStr = NULL;
	int itemLen = 0;
	int txtLen = 0; // use this to count characters parsed, including those 
					// which belong to markers 
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
		// tell the caller it's either a footnote, endnote or a cross-reference
		bEmbeddedSpan = TRUE; 
	}

	// parse, and count, the white space following
	itemLen = pDoc->ParseWhiteSpace(ptr);
	ptr += itemLen;
	dataLen += itemLen;

	// mark the starting point for the content
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
			// If inclOrExcl == includeCustomMarkers, then \free and \not are included
			// in the halt test
			if (IsAHaltingMarker(ptr, inclOrExcl))
			{
				// situation (2), break out with a flag set
				bUnexpectedHalt = TRUE;
				break;
			}
			// situation (1), continue scanning forward
			ptr++;
			txtLen++;
		}
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
		else // for (ptr < pEnd) test
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
	}
	else
	{
        // wholeMarker doesn't have an end marker, so parse until we either encounter
        // another marker that halts scanning forwards, or until the end of the buffer is
        // reached -- and we should continue parsing through any character format markers
        // and their end markers (m_specialMkrs and m_specialEndMkrs, respectively)
		// If inclOrExcl == includeCustomMarkers, then \free and \not are included in the
		// halt test
		while (ptr < pEnd && !IsAHaltingMarker(ptr, inclOrExcl))
		{
			if (pDoc->IsMarker(ptr))
			{
				bool bIsInlineMkr = IsSpecialTextStyleMkr(ptr);
				if (bIsInlineMkr)
				{
					ptr++;
					txtLen++;
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
		}
        // we came to a non-inline marker for formatting purposes, or to the buffer end,
        // either way the action wanted here is the same
		wxString contents(pContentStr,txtLen);
		dataStr = contents;
		// update dataLen
		dataLen += txtLen;
	}
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




/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Notes.cpp
/// \author			Erik Brommers
/// \date_created	16 Februuary 2010
/// \rcs_id $Id$
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the implementation file for the CNotes class. 
/// The CNotes class presents notes functionality to the user. 
/// The code in the CNotes class was originally contained in
/// the CAdapt_ItView class.
/// \derivation		The CNotes class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "Notes.h"
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

#include <wx/object.h>

#include "AdaptitConstants.h"
#include "Adapt_It.h"
#include "Adapt_ItView.h"
#include "SourcePhrase.h"
#include "Strip.h"
#include "Pile.h"
#include "Cell.h"
#include "Layout.h"
#include "Notes.h"
#include "helpers.h"
#include "MainFrm.h"
#include "Adapt_ItCanvas.h"
#include "Adapt_ItDoc.h"
#include "KB.h"
#include "NoteDlg.h"

///////////////////////////////////////////////////////////////////////////////
// External globals
///////////////////////////////////////////////////////////////////////////////

extern bool	gbIsGlossing;
extern bool gbShowTargetOnly;
extern int	gnSelectionLine;
extern bool gbVerticalEditInProgress;
extern int	gnOldSequNum;
extern wxPoint gptLastClick;

// Define type safe pointer lists
#include "wx/listimpl.cpp"

/// This macro together with the macro list declaration in the .h file
/// complete the definition of a new safe pointer list class called WordList.
WX_DEFINE_LIST(WordList);


// *******************************************************************
// Event handlers
// *******************************************************************

BEGIN_EVENT_TABLE(CNotes, wxEvtHandler)

	EVT_MENU(ID_EDIT_MOVE_NOTE_FORWARD, CNotes::OnEditMoveNoteForward)
	EVT_UPDATE_UI(ID_EDIT_MOVE_NOTE_FORWARD, CNotes::OnUpdateEditMoveNoteForward)
	EVT_MENU(ID_EDIT_MOVE_NOTE_BACKWARD, CNotes::OnEditMoveNoteBackward)
	EVT_UPDATE_UI(ID_EDIT_MOVE_NOTE_BACKWARD, CNotes::OnUpdateEditMoveNoteBackward)

	EVT_TOOL(ID_BUTTON_CREATE_NOTE, CNotes::OnButtonCreateNote)
	EVT_UPDATE_UI(ID_BUTTON_CREATE_NOTE, CNotes::OnUpdateButtonCreateNote)
	EVT_TOOL(ID_BUTTON_PREV_NOTE, CNotes::OnButtonPrevNote)
	EVT_UPDATE_UI(ID_BUTTON_PREV_NOTE, CNotes::OnUpdateButtonPrevNote)
	EVT_TOOL(ID_BUTTON_NEXT_NOTE, CNotes::OnButtonNextNote)
	EVT_UPDATE_UI(ID_BUTTON_NEXT_NOTE, CNotes::OnUpdateButtonNextNote)
	EVT_TOOL(ID_BUTTON_DELETE_ALL_NOTES, CNotes::OnButtonDeleteAllNotes)
	EVT_UPDATE_UI(ID_BUTTON_DELETE_ALL_NOTES, CNotes::OnUpdateButtonDeleteAllNotes)

END_EVENT_TABLE()


///////////////////////////////////////////////////////////////////////////////
// Constructors / destructors
///////////////////////////////////////////////////////////////////////////////

CNotes::CNotes()
{
}

CNotes::CNotes(CAdapt_ItApp* app)
{
	wxASSERT(app != NULL);
	m_pApp = app;
	m_pLayout = m_pApp->GetLayout();
	m_pView = m_pApp->GetView();
}

CNotes::~CNotes()
{
	
}


///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if the Note was successfully created and stored as filtered info, 
///             FALSE if there was failure
/// \param      pSrcPhrases  -> the app's m_pSourcePhrases list
/// \param      nLocationSN  -> the location where the note is to be created
/// \param      strNote      -> the text of the note
/// \remarks
/// Called from: the View's RestoreNotesAfterSourceTextEdit().
/// Creates a filtered note on the CSourcePhrase instance in pSrcPhrases which has location
/// nLocationSN for the sequence number. It is the caller's responsibility to ensure there
/// is no note already present there.
/// 
/// BEW 24Feb10, updated for support of doc version 5
/// BEW 9July10, no changes needed for support of kbVersion 2
/////////////////////////////////////////////////////////////////////////////////
bool CNotes::CreateNoteAtLocation(SPList* pSrcPhrases, int nLocationSN, wxString& strNote)
{
	// refactored 7Apr09 - only needed GetMaxIndex() call
    // pSrcPhrases has to be the m_pSourcePhrases list, or a list of CSourcePhrase
    // instances where the list index stays in synch with the stored m_nSequNumber value in
    // each CSourcePhrase instance of the list, for this function to work right
    // BEW changed 19Jun09, the caller should refrain from calling this if nLocationSN is
    // beyond the end of the document (ie. the user's editing resulted in the end of the
    // document being removed, and a note or so, and so we must assume those are not
    // wanted), but because this might get called in such a circumstance, we should code
    // defensively and return FALSE so that bailout can be done instead
	wxString noteMkr = _T("\\note");
	wxString noteEndMkr = noteMkr + _T('*');

	if (nLocationSN > m_pApp->GetMaxIndex())
		return FALSE;
	bool bHasNote = IsNoteStoredHere(pSrcPhrases, nLocationSN);
	if (bHasNote)
	{
		// this location has a Note already, (the caller should have checked for
		// this and not made the call)
		wxString errStr = _T(
		"There is already a Note at the passed in index, within CreateNoteAtLocation(), so the attempt was abandoned.");
		wxMessageBox(errStr, _T(""), wxICON_EXCLAMATION | wxOK);
		return FALSE;
	}
	else
	{
		// there is no Note at that location, so go ahead and create it there
		CSourcePhrase* pToSrcPhrase = NULL;
		SPList::Node* pos = pSrcPhrases->Item(nLocationSN);
		wxASSERT(pos != NULL);
		pToSrcPhrase = pos->GetData();
		pToSrcPhrase->SetNote(strNote);
		pToSrcPhrase->m_bHasNote = TRUE;
		// mark its owning strip as invalid
		m_pApp->GetDocument()->ResetPartnerPileWidth(pToSrcPhrase);
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      pSrcPhrases  -> the document's m_pSourcePhrases list of CSourcePhrase 
///                             instances
/// \param      pRec         -> the EditRecord for the vertical edit process, which
///                             contains the indices for the start and end of the
///                             CSourcePhrase instances in spans we want to check
/// \remarks
/// Called from: the View's OnEditSourceText().
/// Checks the editable text span, and the preceding and following moved notes span, to
/// make sure that the m_bHasNote flag is set TRUE for every CSourcePhrase instance which
/// contains a \note marker in its m_markers member. If the flag is not set and should be,
/// it sets it.
/// 20June08 created by BEW
/// BEW 24Feb10, updated for support of doc version 5
/// BEW 9July10, no changes needed for support of kbVersion 2
/////////////////////////////////////////////////////////////////////////////////
void CNotes::CheckAndFixNoteFlagInSpans(SPList* pSrcPhrases, EditRecord* pRec)
{
	// first check the editable span
	int nStartAt = pRec->nStartingSequNum;
	int nEndAt = nStartAt + pRec->nNewSpanCount - 1;
    // BEW added 19Jun09, (this error is also in Bill's code base, but it will be merged to
    // this code base for next release so I've not informed him) the span may not exist, if
    // at the end of the document we deleted the source text and the phrase box was located
    // within the deleted section, so check and skip this span if it has gone
	int maxIndex = m_pApp->GetMaxIndex();
	bool bDoEditSpanCheck = TRUE;
    if (nStartAt > maxIndex)
	{
		// old active location is now beyond the end of the document (note: document end
		// and active location have been re-calculated in OnEditSourceText() already prior
		// to CheckAndFixNoteFlagInSpans() having been called - the active location will
		// have been set to the last existing CSourcePhrase in the doc), so we have to
		// skip this edit span check
		bDoEditSpanCheck = FALSE;
	}
	else if (nEndAt >= nStartAt && nEndAt > maxIndex)
	{
		// the span to be checked starts off within the document, but its end is now
		// beyond the document's end, so shorten it to end at the document's end
		nEndAt = maxIndex;
	}
	CSourcePhrase* pSrcPhrase = NULL;
	SPList::Node* pos = NULL;
	if (bDoEditSpanCheck)
	{
		pos = pSrcPhrases->Item(nStartAt);
		wxASSERT(pos != NULL);
		while (pos != NULL)
		{
			pSrcPhrase = pos->GetData();
			pos = pos->GetNext();
			wxASSERT(pSrcPhrase != NULL);
			if (!pSrcPhrase->GetNote().IsEmpty())
			{
				// there is a note stored here
				pSrcPhrase->m_bHasNote = TRUE; // ensure the note is flagged 
				// in case the user edited a typo SF resulting
				// in a Note which got filtered
			}

			// break out of the loop once we've checked the last in the span
			if (pSrcPhrase->m_nSequNumber >= nEndAt)
				break; 
		}
	}
	// next check 5 or as many CSourcePhrase instances there are in the
	// follNotesMoveSpanList and precNotesMoveSpanList (5 because we can't
	// be sure either of those lists has content), doing these checks preceding
	// and following the editable span
	// start with the span following the editable span
	int nNextStartAt = nEndAt + 1;
	int nNextEndAt;
	int delta = 0;
	// don't do it if the following context does not exist
	if (nNextStartAt < maxIndex)
	{
		delta = wxMin(5,pRec->arrNotesSequNumbers.GetCount());
		if (delta < 5) delta = 5;
		nNextEndAt = nNextStartAt + delta - 1;
		if (nNextEndAt > m_pApp->GetMaxIndex())
			nNextEndAt = m_pApp->GetMaxIndex();
		// now do the check of these
		SPList::Node* pos = pSrcPhrases->Item(nNextStartAt);
		wxASSERT(pos != NULL);
		while (pos != NULL)
		{
			pSrcPhrase = pos->GetData();
			pos = pos->GetNext();
			wxASSERT(pSrcPhrase != NULL);
			if (!pSrcPhrase->GetNote().IsEmpty())
			{
				// there is a note stored here
				pSrcPhrase->m_bHasNote = TRUE; // ensure the note is flagged 
				// in case the user edited a typo SF resulting
				// in a Note which got filtered
			}

			// break out of the loop once we've checked the last in the span
			if (pSrcPhrase->m_nSequNumber >= nNextEndAt)
				break; 
		}
	}
	// finally, do the same check and fix for the preceding context, at least 5, etc
	nNextEndAt = nStartAt - 1;
	delta = 0;
	// don't do it if the preceding context does not exist
	if (nNextEndAt > 0)
	{
		delta = wxMin(5,pRec->arrNotesSequNumbers.GetCount());
		if (delta < 5) delta = 5;
		nNextStartAt = nNextEndAt - delta + 1;
		if (nNextStartAt < 0)
			nNextStartAt = 0;
		// now do the check of these
		SPList::Node* pos = pSrcPhrases->Item(nNextStartAt);
		wxASSERT(pos != NULL);
		while (pos != NULL)
		{
			pSrcPhrase = pos->GetData();
			pos = pos->GetNext();
			wxASSERT(pSrcPhrase != NULL);
			if (!pSrcPhrase->GetNote().IsEmpty())
			{
				// there is a note stored here
				pSrcPhrase->m_bHasNote = TRUE; // ensure the note is flagged 
				// in case the user edited a typo SF resulting
				// in a Note which got filtered
			}

			// break out of the loop once we've checked the last in the span
			if (pSrcPhrase->m_nSequNumber >= nNextEndAt)
				break; 
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     void
/// \remarks	Removes all notes.
/// BEW 25Feb10, updated for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
void CNotes::DeleteAllNotes()
{
	SPList* pList = m_pApp->m_pSourcePhrases;
	SPList::Node* pos = pList->GetFirst();
	wxASSERT(pos != NULL);
	CSourcePhrase* pSrcPhrase;
	wxString emptyStr = _T("");
	
	// do the deleting
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		if (pSrcPhrase->m_bHasNote || !pSrcPhrase->GetNote().IsEmpty())
		{
			pSrcPhrase->SetNote(emptyStr);
			pSrcPhrase->m_bHasNote = FALSE;
		}
	}
	m_pView->Invalidate(); // get the view redrawn, so the note icons disappear too
	m_pLayout->PlaceBox();
}

/////////////////////////////////////////////////////////////////////////////////
/// \return             TRUE if the user's typed string matches, FALSE if not
///
///	pSearchList		->	pointer to the list of words in the user's typed string, 
///                     as returned by repeated calls to Tokenize(), using whitespace
///                     characters for delimiters
///	firstWord		->	the first word in pSearchList - the caller will already have 
///	                    matched this one
///	noteStr			->	the full content of the note, as stored on the pSrcPhrase 
///	                    currently being examined (and this string may, or may not, 
///	                    end in a space)
///	nStartOffset	<->	starting offset in noteStr for the already matched part 
///	                    (ie. firstWord)
///	nEndOffset		<-	ending offset (ie. the character immediately after the matched 
///	                    search string) for the match -- used  in conjunction with 
///	                    nStartOffset to enable the CNoteDlg code to later determine how 
///	                    much of noteStr to hightlight
/// \remarks
///    Uses some of the document's text parsing code. For multiword matches, only the first
///    and last of the passed in search words can be other than full-word matchups, the
///    first must at least match suffixally, and the last must at least match prefixally.
///    We don't care if the user has typed white space characters such as newline, carriage
///    return or tabs in noteStr, and only spaces in the search string - such white space
///    differences are ignored (which we expect is what the user would always want). The
///    returned offsets are used by the caller (ie. CNoteDlg) for highlighting purposes
///    when there was a successful match.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
bool CNotes::DoesTheRestMatch(WordList* pSearchList, wxString& firstWord, wxString& noteStr,
									 int& nStartOffset, int& nEndOffset)
{
	// get a str variable containing the rest, beginning at the start 
	// of the matched first word
	wxString str = noteStr.Mid(nStartOffset);
	int len = firstWord.Length();
	
    // if the first word match was just a prefix in a larger word, then we won't be able to
    // match any of the other search words, in which case return FALSE to the caller
	if ( (str[len] != _T(' ')) && (str[len] != _T('\n')) && 
		(str[len] != _T('\r')) && (str[len] != _T('\t')) )
	{
        // there is an alphabetic or numeric character at the next location, so we've just
        // matched a subpart of the word and that subpart does not extend to the word's
        // end, so we cannot match any of the other search words; this constitutes a
        // failure to match the whole search string
		return FALSE;
	}
	
	// we are okay so far, so now try to match the rest
	int buflen = noteStr.Length();
	const wxChar* pBuff = str.GetData();
	wxChar* pBufStart = (wxChar*)pBuff;
	wxChar* ptr = pBufStart + len; // start from where we've matched to so far 
	// (it will be pointing at whitespace)
	wxChar* pEnd = pBufStart + buflen; // get the bound past which we must not go
	wxASSERT(*pEnd == _T('\0')); // whm added
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument(); // we'll use the doc's functions 
	// IsWhiteSpace() and ParseWhiteSpace()
	WordList::Node* pos = pSearchList->GetFirst(); 
	wxString nextWord = *pos->GetData(); // we've already matched this one, 
	// so the loop handles the rest
	pos = pos->GetNext();
	int nHowManyChars;
	int nWordNum = 1;
	int nLastWord = pSearchList->GetCount();
	int nTotalChars = len; // count the total characters spanned in getting the match
	while (pos != NULL)
	{
		// get the next search word, and its length
		nextWord = *pos->GetData();
		pos = pos->GetNext();
		len = nextWord.Length();
		nWordNum++; // count it
		
		// jump over any whitespace
		if (pDoc->IsWhiteSpace(ptr))
		{
			nHowManyChars = pDoc->ParseWhiteSpace(ptr);
			ptr += nHowManyChars;
			nTotalChars += nHowManyChars;
		}
		
        // are we at the end? If so, since we've not yet matched the search word, then we
        // would have failed to get a match and must return, otherwise, continue processing
		if (ptr == pEnd)
		{
			// MFC released noteStr which was error
			return FALSE;
		}
		
		// do we have a match for the contents of nextWord, starting at the ptr location?
		if (wxStrncmp(nextWord,ptr,len) == 0)
		{
			// the strings matched...
			// count what we just matched
			nTotalChars += len;
			ptr += len; // point to the character which 
			// immediately follows the matched word
			
            // if we are not dealing with the last word in the search list, then the match
            // must be a whole word match -- and if it isn't, we have failed & must return
            // FALSE, but if we *are* dealing with the last word, then a prefixal match is
            // sufficient for success
			if (nWordNum < nLastWord)
			{
				// this is not the last word needing to be matched, so check out it is a
				// whole-word match
				if (ptr == pEnd)
				{
					// we've an unmatched word or more to go, so this constitutes a failure
					// MFC released noteStr which was error
					return FALSE;
				}
				else if (!pDoc->IsWhiteSpace(ptr))
				{
                    // there is more of a word at the ptr location, so we only matched an
                    // initial substring, and so no further match is possible
					return FALSE;
				}
				else
				{
					// we had a whole-word match, hence so far so good, 
					// & we iterate the loop
					continue;
				}
			}
			else
			{
				// we are dealing with the last word, so we don't require a whole-word match
				// -- in fact, we are done & we have matched the whole search string typed
				// by the user (except there could be differences in whitespaces)
				break;
			}
		}
		else
		{
			// no match, so the total matchup has failed
			// MFC released noteStr which was error
			return FALSE;
		}
	} // end of word-matching loop
	
	// if we get here, then the whole search string has been matched
	nEndOffset = nStartOffset + nTotalChars; // set nEndOffset which is to be 
	// returned to the caller
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     Return TRUE if there was no error, FALSE for an error (such 
///             as a bounds error) 
/// \param      pList          -> pointer to a list of source phrases
/// \param      nStartLoc      -> starting
/// \param      nFoundAt       <- receives the index value of the next note found, 
///                               or -1 if none found
/// \param      bFindForwards  -> the direction in which to search for notes
/// \remarks
/// Called from: the View's RestoreNotesAfterSourceTextEdit().
/// This function is used in the vertical edit process, when restoring removed notes after
/// a source text edit. It finds the index value in pList at which the next Note is found,
/// either forwards (or at the current location), or backwards, and return it in nFoundAt,
/// or return -1 in nFoundAt if no note was found in the nominated direction.
/// Note: while typically used with pList set to the app's m_pSourcePhrases list, in which
/// the list indices always match the stored m_nSequNumber value in each POSITION's
/// CSourcePhrase instance, the function can be used for arbitrary sublists of
/// CSourcePhrase instances because it returns the stored m_nSequNumber value for the found
/// note in the CSourcePhrase which stores it, not the index value in pList at which that
/// CSourcePhrase was located.
/// BEW 25Feb10, updated for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
bool CNotes::FindNote(SPList* pList, int nStartLoc, int& nFoundAt, bool bFindForwards)
{
	// BEW created 29May08
	wxString errStr;
	// BEW changed 19Jun09, because of the possibility of editing resulting in the loss of
	// data from the end of the document, we need to be smarter than just check for bounds
	// errors using pre-edit index values, instead, we will test and adjust to get the
	// closest valid location and search from there
	int count = (int)pList->GetCount();
	if (nStartLoc < 0)
		nStartLoc = 0;
	if (nStartLoc >= count)
	{
		nStartLoc = count - 1;
	}
	CSourcePhrase* pSrcPhrase = NULL;
	SPList::Node* pos = pList->Item(nStartLoc);
	if (pos == NULL)
	{
		// unexpected error, the location should be findable
		errStr = _T(
		"Error in helper function FindNote(); the POSITION value returned from ");
		errStr += _T(
		"FindIndex() was null. The current operation will be abandoned.");
		wxMessageBox(errStr,_T(""), wxICON_EXCLAMATION | wxOK);
		return FALSE;
	}
	if (bFindForwards)
	{
		// examine the starting location first
		pSrcPhrase = pos->GetData();
		pos = pos->GetNext();
		if (pSrcPhrase->m_bHasNote || !pSrcPhrase->GetNote().IsEmpty())
		{
			// there is one at the current POSITION
			pSrcPhrase->m_bHasNote = TRUE; // ensure the flag is set
			nFoundAt = pSrcPhrase->m_nSequNumber;
			return TRUE;
		}
		while (pos != NULL)
		{
			pSrcPhrase = pos->GetData();
			pos = pos->GetNext();
			if (pSrcPhrase->m_bHasNote || !pSrcPhrase->GetNote().IsEmpty())
			{
				// there is one at the current POSITION
				pSrcPhrase->m_bHasNote = TRUE; // ensure the flag is set
				nFoundAt = pSrcPhrase->m_nSequNumber;
				return TRUE;
			}
		}
	}
	else
	{
		// ignore the starting location
		pSrcPhrase = pos->GetData();
		pos = pos->GetPrevious();
		// search from the preceding location, backwards
		while (pos != NULL)
		{
			pSrcPhrase = pos->GetData();
			pos = pos->GetPrevious();
			if (pSrcPhrase->m_bHasNote || !pSrcPhrase->GetNote().IsEmpty())
			{
				// there is one at the current POSITION
				pSrcPhrase->m_bHasNote = TRUE; // ensure the flag is set
				nFoundAt = pSrcPhrase->m_nSequNumber;
				return TRUE;
			}
		}
	}
	// none was found, so return -1
	nFoundAt = -1;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return	        the sequence number of the pSrcPhase which has a note containing 
///                 the whole string (though white space between words may be different 
///                 in the matching up of the search string with the matched string), 
///                 or -1 if there was no matching string found
///
///	nCurrentlyOpenNote_SequNum	->	sequ num of the note dialog from which the search 
///	                                was initiated
///	searchStr					->	reference to the string the user typed in to be 
///	                                searched for
/// Remarks:
///	If it finds a matching string in a subsequent note, that location's sequence 
///	number is returned to the caller
/// BEW 25Feb10, updated for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
int CNotes::FindNoteSubstring(int nCurrentlyOpenNote_SequNum, WordList*& pSearchList,
									  int numWords, int& nStartOffset, int& nEndOffset)
{
	int sn = nCurrentlyOpenNote_SequNum;
	sn++; // start looking from the next pSrcPhrase in the list
	
	SPList* pList = m_pApp->m_pSourcePhrases;
	SPList::Node* pos;
	CSourcePhrase* pSrcPhrase;
	wxString noteContentStr;
	int nFoundSequNum = -1;
	
	// get the starting POSITION from which to commence the scan
	pos = pList->Item(sn); 
	wxASSERT(pos != NULL);
	
	// loop forward over the pSrcPhrase instances till once containing a note is found
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		sn = pSrcPhrase->m_nSequNumber; // update sn
		if (!pSrcPhrase->m_bHasNote) // support finding empty notes too, so just test the flag
		{
			// this sourcephrase instance contains no filtered note
			continue;
		}
		else
		{
			// this source phrase contains a note, so get content and put it into the
			// string noteContentStr
			noteContentStr = pSrcPhrase->GetNote();
			if (noteContentStr.IsEmpty())
			{
				// no content, so continue looping
				continue;
			}
			else
			{
				// Check out whether the search string is contained in the 
				// content string
				wxString aWord;
				WordList::Node* fpos = pSearchList->GetFirst();
				aWord = *fpos->GetData(); // get the first word in the passed in 
				// search word list
				
                // How we proceed depends on the number of words to be searched for.
                // When searching just for a single word, a match anywhere in
                // noteContentStr constitutes a successful Find(); but when searching
                // for two or more words, the first word must match all the way up to
                // the end of a content string's word, and then subsequent non-final
                // search words must match whole words exactly, and the final search
                // word must match the next content word from its beginning (but does
                // not necessarily have to match all the character in the word) --
                // since the multi-word match is complex, we will do it in a function
                // which will return the offsets to the matched substring as well when
                // the match succeeded
				int nFound;
				if (numWords == 1)
				{
					// do a simple Find() for the word
					nFound = noteContentStr.Find(aWord);
					if (nFound == -1)
					{
						// no match, so continue iterating the pSrcPhrase loop
						continue;
					}
					else
					{
						// it matched, so set the offsets to start and end locations
						int len = aWord.Length();
						nStartOffset = nFound;
						nEndOffset = nStartOffset + len;
						
						// get the sequ number for this pSrcPhrase which we need to 
						// return to the caller
						nFoundSequNum = sn;
						break;
					}
				}
				else
				{
                    // we have a multiword match requested... find matches for the
                    // first of the search word - (there could be more than one
                    // matching location)
					nFound = 0;
					while ( (nFound = FindFromPos(noteContentStr,aWord,nFound)) != -1)
					{
						// we found a match for the first word
						nStartOffset = nFound;
						
						// find out if the rest of the search string matches at 
						// this location
						bool bItMatches = DoesTheRestMatch(pSearchList,aWord,
											noteContentStr,nStartOffset,nEndOffset);
						if (!bItMatches)
						{
							// keep iterating when the whole search string was not
							// matched
							nFound++; // start from next character after 
							// start of last match
							nStartOffset = -1;
							nEndOffset = -1;
							continue; // iterate within this inner loop, to find
							// another  matching location for the first
							// word of the search string
						}
						else
						{
                            // the whole lot matches, so we can break out after
                            // determining the sequence number for this location
							nFoundSequNum = sn;
							return nFoundSequNum;
						}
					}
					
					// there was no match for the whole search string
					nFoundSequNum = wxNOT_FOUND; // equals -1
					continue; // iterate the outer loop which 
							  // scans pSrcPhrase instances
				} // end block for testing for a multi-word match
			} // end block for processing a non-empty noteContentStr
		} // end block for pSrcPhrase contains a note
	} // end loop for scanning over the pSrcPhrase instances in m_pSourcePhrases
	return nFoundSequNum;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if there were no errors, FALSE if there was an error
/// \param      pSrcPhrases -> pointer to the app's m_pSourcePhrases list, after the
///                            user's edit source text has been made to replace the
///                            original selection and m_nSequNumber values in all
///                            CSourcePhrase instances in pSrcPhrases have been made
///                            sequential from 0 at the list's start
/// \param      pRec        -> pointer to the EditRecord for the vertical edit process
/// \param      context     -> a WhichContextEnum enum value which is either
///                            precedingContext (0) or followingContext (1)
/// \remarks
/// Called from: the View's OnEditSourceText().
/// The passed in EditRecord contains the information about the source text edit which has
/// just been completed, and it includes an array, arrNotesSequNumbers, which stores in
/// normal order the sequence number indices for each Note that was removed from the
/// original editable span (this span will have been extended beyond the user's selection
/// to include all of any retranslation(s) it overlaps, but if there were no such overlaps
/// then it was the same as the user's selection). The array is used to assist in
/// relocating the removed notes. GetMovedNotesSpan is called after the edited source text
/// has been incorporated into the document list, but immediately prior to the attempt to
/// recreate any temporarily removed Notes from the editable span (possibly extended, see
/// previous paragraph). The Note restoration process, if there are many notes, or the user
/// removed source text where Notes were stored, may need to move unremoved notes in the
/// preceding context leftwards, or in the following context rightwards, in order to make
/// gaps for re-placing the temporarily removed Notes. Any such Note movements would
/// invalidate the Note placements in the cancel span, so that if the user asks for a
/// Cancel, or there is an error requiring the original state of the document to be
/// rebuilt, the Notes could end up duplicated in nearby locations, or worse. The solution
/// to this connundrum is to work out the maximum number of possible moves that the
/// algorithms for Note replacement might request in order to form gaps, at both preceding
/// and following context (with respect to the edit span) and make deep copies of that many
/// CSourcePhrase instances in one or the other context (depending on the passed in context
/// value) prior to the Note restoration being initiated. If a Cancel or bail it is later
/// requested, then the first thing to be done is to restore these preceding and following
/// small contextual sublists, so that the pre-Note-moves state of the pSrcPhrases list is
/// restored, and then the cancel span will be able to be used without error in the rest of
/// the document restoration process. The EditRecord stores these two sublists in its
/// follNotesMoveSpanList and precNotesMoveSpanList members.
/// BEW 26May08	function created as part of refactoring the Edit Source Text functionality
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/// BEW 9July10, no changes needed for support of kbVersion 2
/////////////////////////////////////////////////////////////////////////////////
bool CNotes::GetMovedNotesSpan(SPList* pSrcPhrases, EditRecord* pRec, WhichContextEnum context)
{
	wxString errStr;
	int nEditSpanStartingSN = pRec->nStartingSequNum;
	int nEditSpanEndingSN;
	if (pRec->nNewSpanCount == 0)
	{
		nEditSpanEndingSN = pRec->nStartingSequNum;
	}
	else
	{
		nEditSpanEndingSN = pRec->nStartingSequNum + pRec->nNewSpanCount - 1;
	}
	int nMaxMoves = pRec->arrNotesSequNumbers.GetCount();
	// if the array has no members, the span does not need to be created
	if (nMaxMoves == 0)
		return TRUE;
    // there is at least one temporarily removed Note, so there is the possibility that a
    // non-removed Note may need to be moved at the Note restoration step; so we need to go
    // ahead and work collect the CSourcePhrase instances which would potentially be
    // involved in any such moves and copy those and store in pRec
	int nGapCount = 0;	// count how many CSourcePhrase instances without a Note 
	// stored there are traversed as we scan across them, leftwards in top block,
	// rightwards in the else block; when nGapCount == nMaxMoves, we've collected
	// enough instances; don't count any CSourcePhrase which has a stored Note in
	// it
	int nStartAt;
	CSourcePhrase* pSrcPhrase = NULL;
	SPList::Node* pos = NULL; //POSITION pos = NULL;
	CSourcePhrase* pNewOne = NULL; // for creation of deep copies on the heap
	if (context == precedingContext)
	{
		// at this invocation, collect from the context preceding the edit span
		nStartAt = nEditSpanStartingSN - 1;
		if (nStartAt < 0)
		{
			// there is no preceding context, so return
			return TRUE;
		}
		pos = pSrcPhrases->Item(nStartAt); // initialize pos
		if (pos == NULL)
		{	
			errStr = 
			_T("FindIndex() failed in GetMovedNotesSpan(), preceding context, pos value is NULL.");
			errStr += 
			_T(" Abandoning the edit process. Will attempt to restore original document state.");
			wxMessageBox( errStr, _T(""), wxICON_EXCLAMATION | wxOK);
			return FALSE;
		}
		while (pos != NULL)
		{
			// count this CSourcePhrase instance if it has no Note stored in it
			pSrcPhrase = pos->GetData();
			pos = pos->GetPrevious();
			if (!pSrcPhrase->m_bHasNote)
				nGapCount++;
			// make a deep copy, and store it in the EditRecord
			pNewOne = new CSourcePhrase(*pSrcPhrase);
			pNewOne->DeepCopy();
			// In wxList Insert() without a position parameter always adds to the 
			// head of the list
			SPList::Node* posInsert = pRec->precNotesMoveSpanList.Insert(pNewOne);
			posInsert = posInsert; // to avoid warning in release build
			wxASSERT(posInsert != NULL);
			// check if the break out criterion has been met
			if (nGapCount == nMaxMoves)
			{
                // we've gotten sufficient preceding context for a safe restoration of the
                // sublist section which might have one or more moved Notes done within it
				break;
			}
		}
        // NOTE, the sublist in pRec will contain more than nGapCount CSourcePhrase
        // instances if the above while loop scanned across instances which contain a Note
        // already; these are not counted, but must be included in the sublist
	}
	else
	{
		// at this invocation, collect from the context following the edit span
		if (pRec->nNewSpanCount == 0)
			nStartAt = nEditSpanEndingSN;
		else
			nStartAt = nEditSpanEndingSN + 1;
		if (nStartAt > m_pApp->GetMaxIndex())
		{
			// there is no following context, so return
			return TRUE;
		}
		pos = pSrcPhrases->Item(nStartAt); // initialize pos
		if (pos == NULL)
		{	
			errStr = 
			_T("FindIndex() failed in GetMovedNotesSpan(), following context, pos value is NULL.");
			errStr += 
			_T(" Abandoning the edit process. Will attempt to restore original document state.");
			wxMessageBox( errStr, _T(""), wxICON_EXCLAMATION | wxOK);
			return FALSE;
		}
		while (pos != NULL)
		{
			// count this CSourcePhrase instance if it has no Note stored in it
			pSrcPhrase = pos->GetData();
			pos = pos->GetNext();
			if (!pSrcPhrase->m_bHasNote)
				nGapCount++;
			// make a deep copy, and store it in the EditRecord
			pNewOne = new CSourcePhrase(*pSrcPhrase);
			pNewOne->DeepCopy();
			SPList::Node* posInsert = pRec->follNotesMoveSpanList.Append(pNewOne);
			posInsert = posInsert; // to avoid warning in release build
			wxASSERT(posInsert != NULL);
			// check if the break out criterion has been met
			if (nGapCount == nMaxMoves)
			{
				// we've gotten sufficient following context for a safe restoration of the
				// sublist section which might have one or more moved Notes done within it
				break;
			}
		}
        // NOTE, the sublist in pRec will contain more than nGapCount CSourcePhrase
        // instances if the above while loop scanned across instances which contain a Note
        // already; these are not counted, but must be included in the sublist
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if a note exists there, FALSE if not
/// \param      pSrcPhrases  -> the app's m_pSourcePhrases list
/// \param      nNoteSN      -> the sequence number index for the CSourcePhrase 
///                             which we want to know whether or not it stores a Note
/// \remarks
/// Called from: the View's RestoreNotesAfterSourceTextEdit().
/// Determines is a Note is stored at nNoteSN location in the pSrcPhrases list.
/// 
/// BEW 24Feb10 changed for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
bool CNotes::IsNoteStoredHere(SPList* pSrcPhrases, int nNoteSN)
{
    // BEW added 30May08 in support of the source text editing step of the 
    // vertical editing process
    // BEW changed 19Jun09, because editing may have removed some of doc's end, including
    // one or more notes, and we can't just take the note's old sequence number and assume
    // that still corresponds to a position within the edited document; so I think the
    // thing to do is to test for nNoteSN beyond doc end, and just return FALSE - the
    // caller should not call IsNoteStoredHere if nNoteSN is beyond the doc end, but we
    // better allow for it and code defensively
	int maxIndex = pSrcPhrases->GetCount() -1;
	if (nNoteSN > maxIndex)
		return FALSE; // beyond document's end, so certainly can't store a note there!!
	SPList::Node* pos = pSrcPhrases->Item(nNoteSN);
	CSourcePhrase* pSrcPhrase = pos->GetData();
	return (!pSrcPhrase->GetNote().IsEmpty() || pSrcPhrase->m_bHasNote);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     void
/// \param      nJumpOffSequNum ->  the sequence number of the CSoucePhrase instance which
///                                 stores the currently open Note, or if no Note is open,
///                                 the sequence number of the phrase box location
/// \remarks	
/// Implements the command to jump backwards in the document to the first note in the
/// preceding context. Don't jump if there is no such Note in existence.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::JumpBackwardToNote_CoreCode(int nJumpOffSequNum)
{
	CMainFrame* pFrame;
	wxTextCtrl* pEdit;
	
	SPList::Node* pos;
	SPList* pList = m_pApp->m_pSourcePhrases;
	pos = pList->Item(nJumpOffSequNum);
	CSourcePhrase* pSrcPhrase = NULL;
	int nNoteSequNum = nJumpOffSequNum; // for iterating back from the jump origin
	int nBoxSequNum;
	
	// remove any selection to be safe from unwanted selection-related side effects
	m_pView->RemoveSelection();
	
	// jump only if there is a possibility of a note being earlier
	if (nNoteSequNum > 0)
	{
		// loop until the previous note's pSrcPhrase is found
		while (pos != NULL)
		{
			pSrcPhrase = (CSourcePhrase*)pos->GetData();
			pos = pos->GetPrevious();
			if (pSrcPhrase->m_bHasNote)
				break;
			// if pos is NULL, no earlier one was found
			if (pos == NULL)
				return;
		}
		
		// store the adaptation in the KB before moving the phrase box
		wxASSERT(m_pApp->m_pActivePile); // the old value is still valid, and 
		// it's pile has the old sourcephrase
		bool bOK;
		bOK = m_pView->StoreBeforeProceeding(m_pApp->m_pActivePile->GetSrcPhrase());
		bOK = bOK; // avoid warning TODO: check for failures? (BEW 3Jan12, No, even if
				   // the store were to fail, we'd still want to do the jump and continue
				   // processing
        // Otherwise, we have found one, so it can be opened. However, we have to exercise
        // care with the phrase box - if the note is in a retranslation while adaptation
        // mode is turned on, then we can't place the phrase box at the note location -
        // the best we can do is place it at a safe location outside the retranslation.
        // We implement these protocols below.
		
		// get the new value of nNoteSequNum from the located sourcephrase
		nNoteSequNum = pSrcPhrase->m_nSequNumber;
		nBoxSequNum = nNoteSequNum; // default, at least until we know it is not safe 
		// and adjust it
		if (m_pApp->m_bFreeTranslationMode)
		{
            // free translation mode is on, which limits the phrase box locations so
            // disallow the jump
			pFrame = m_pApp->GetMainFrame();
			wxASSERT(pFrame);
			wxASSERT(&pFrame->m_pComposeBar);
			pEdit = (wxTextCtrl*)pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
			
            // NOTE: the Update handler will prevent OnButtonNextNote() being entered when
            // in Free Translation mode, so control should never get here. But in case
            // someone changes things, moving to the next (or previous) note when in Fr.
            // Trans mode is bad news - because in that mode the phrase box location is
            // limited to the start of existing free trans sections, and in the part of the
            // doc which does not have free trans sections yet, to jump to a note location
            // would result in a free trans section being defined there - which is unlikely
            // to be what is wanted because notes can be anywhere and are unlikely to just
            // be at locations where we'd want a free translation to start.
			
			// since we are in free translation mode, we want the focus to be in the
			// Compose Bar's edit box
			if (m_pApp->m_bFreeTranslationMode)
			{
				pEdit->SetFocus();
			}
			return; // do nothing else
		}
		else if (pSrcPhrase->m_bRetranslation && !gbIsGlossing)
		{
            // we are in adapting mode and the sourcephrase with the note lies within a
            // retranslation so the box location will end up different than the note
            // location, so set the note location (ie. its sequence number) since we
            // already know it
			m_pApp->m_nSequNumBeingViewed = nBoxSequNum; // the note dialog needs 
														   // this value to be correct
            // now work out where the active location (ie. phrase box location) should
            // be
			m_pApp->m_nActiveSequNum = nNoteSequNum;
			m_pApp->m_pActivePile = m_pView->GetPile(nNoteSequNum);
			wxASSERT(m_pApp->m_pActivePile);
			
            // this 'active' location is invalid, because the phrase box can't be put in
            // a retranslation, so we'll try put the box after the retranslation, if not,
            // then before it
			CSourcePhrase* pSafeSrcPhrase = m_pView->GetFollSafeSrcPhrase(pSrcPhrase);
			if (pSafeSrcPhrase == NULL)
			{
				// not a safe place, so try earlier than the retranslation
				pSafeSrcPhrase = m_pView->GetPrevSafeSrcPhrase(pSrcPhrase);
				if (pSafeSrcPhrase == NULL)
				{
					// horrors! nowhere is safe - if so, just open it 
					// within the retranslation
					goto a;
				}
			}
			pSrcPhrase = pSafeSrcPhrase;
			m_pApp->m_nActiveSequNum = pSafeSrcPhrase->m_nSequNumber;
			m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);
			wxASSERT(m_pApp->m_pActivePile);
			
			// now do the stuff common to all three of these possibilities
			goto a;
		}
		else
		{
            // there should be no restraint against us placing the box
            // at the same location as the note
			m_pApp->m_nActiveSequNum = nNoteSequNum;
			m_pApp->m_pActivePile = m_pView->GetPile(nNoteSequNum);
			wxASSERT(m_pApp->m_pActivePile);
			m_pApp->m_nSequNumBeingViewed = m_pApp->m_nActiveSequNum; // the note
												// dialog needs this value to be correct
			// now do the stuff common to each of these above three possibilities
		} // end of block for not in a retranslation and not in free translation mode
	} // end of block for testing that a jump is possible
	
    // get the phrase box contents appropriate for the new location & handle the
    // possibility that the new active location might be a "<Not In KB>" one
a:	if (!pSrcPhrase->m_bHasKBEntry && pSrcPhrase->m_bNotInKB)
	{
		// this ensures user has to explicitly type into the box and explicitly check
		// the checkbox if he wants to override the "not in kb" earlier setting at this
		// location
		m_pApp->m_bSaveToKB = FALSE;
		m_pApp->m_targetPhrase.Empty();
		m_pApp->m_pTargetBox->m_bAbandonable = TRUE;
	}
	else
	{
		if (!pSrcPhrase->m_adaption.IsEmpty())
		{
			m_pApp->m_targetPhrase = pSrcPhrase->m_adaption;
			m_pApp->m_pTargetBox->m_bAbandonable = FALSE;
		}
		else
		{
			m_pApp->m_pTargetBox->m_bAbandonable = TRUE;
			if (m_pApp->m_bCopySource)
			{
				m_pApp->m_targetPhrase = m_pView->CopySourceKey(pSrcPhrase,
													 m_pApp->m_bUseConsistentChanges);
			}
			else
			{
				m_pApp->m_targetPhrase.Empty();
			}
		}
	}

	// remove the text from the KB or GlossingKB, if refString is not null
	wxString emptyStr = _T("");
	if (gbIsGlossing)
		m_pApp->m_pGlossingKB->GetAndRemoveRefString(pSrcPhrase,emptyStr,useGlossOrAdaptationForLookup);
	else
		m_pApp->m_pKB->GetAndRemoveRefString(pSrcPhrase,emptyStr,useGlossOrAdaptationForLookup);

	// recalculate the layout
#ifdef _NEW_LAYOUT
	m_pLayout->RecalcLayout(pList, keep_strips_keep_piles);
#else
	m_pLayout->RecalcLayout(pList, create_strips_keep_piles);
#endif
	
	// recalculate the active pile
	m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);
	
	// scroll the active location into view, if necessary
	m_pApp->GetMainFrame()->canvas->ScrollIntoView(m_pApp->m_nActiveSequNum);
	
	m_pLayout->m_docEditOperationType = default_op;
	
	// restore default button image, and m_bCopySourcePunctuation to TRUE
	wxCommandEvent event;
	if (!m_pApp->m_bCopySourcePunctuation)
	{
		m_pView->OnToggleEnablePunctuationCopy(event);
	}
	m_pView->Invalidate();
	m_pLayout->PlaceBox();
	
	// now put up the note dialog at the m_nSequNumBeingViewed location
	wxASSERT(m_pApp->m_pNoteDlg == NULL);
	m_pApp->m_pNoteDlg = new CNoteDlg(m_pApp->GetMainFrame());
	// wx version: we don't need the Create() call for modeless notes dialog
	m_pView->AdjustDialogPosition(m_pApp->m_pNoteDlg); // show it lower, not at top left
	m_pApp->m_pNoteDlg->Show(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     void
/// \param      nJumpOffSequNum ->  the sequence number of the CSoucePhrase instance which
///                                 stores the currently open Note, or if no Note is open,
///                                 the sequence number of the phrase box location
/// \remarks	
/// Implements the command to jump forwards in the document to the first note in the
/// following context. Don't jump if there is no such Note in existence.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::JumpForwardToNote_CoreCode(int nJumpOffSequNum)
{
	CMainFrame* pFrame;
	wxTextCtrl* pEdit;
	
	// find the first note
	SPList::Node* pos; 
	SPList* pList = m_pApp->m_pSourcePhrases;
	pos = pList->Item(nJumpOffSequNum); 
	CSourcePhrase* pSrcPhrase = NULL;
	int nNoteSequNum = nJumpOffSequNum; // for iterating forward from the jump origin
	int nBoxSequNum;
	
	// remove any selection to be safe from unwanted selection-related side effects
	m_pView->RemoveSelection();
	
	// jump only if there is a possibility of a note being ahead
	if (nNoteSequNum <= m_pApp->GetMaxIndex())
	{
		// loop until the next note's pSrcPhrase is found
		while (pos != NULL)
		{
			pSrcPhrase = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();
			if (pSrcPhrase->m_bHasNote)
				break;
			// return if at doc end without finding a note
			if (pos == NULL)
				return;
		}
		
		// store the adaptation in the KB before moving the phrase box
		wxASSERT(m_pApp->m_pActivePile); // the old value is still valid, and it's 
		// pile has the old sourcephrase
		bool bOK;
		bOK = m_pView->StoreBeforeProceeding(m_pApp->m_pActivePile->GetSrcPhrase());
		bOK = bOK; // avoid warning TODO: Check for failures? (BEW 3Jan12, No, even
				   // if the store were to fail, we'd still want to do the jump and
				   // continue processing
        // Otherwise, we have found one, so it can be opened. However, we have to exercise
        // care with the phrase box - if the note is in a retranslation while adaptation
        // mode is turned on, then we can't place the phrase box at the note location - the
        // best we can do is place it at a safe location outside the retranslation. We
        // implement these protocols below.
		
		// get the new value of nNoteSequNum from the located sourcephrase
		nNoteSequNum = pSrcPhrase->m_nSequNumber;
		nBoxSequNum = nNoteSequNum; // default, at least until we know it is not safe 
		// and adjust it
		if (m_pApp->m_bFreeTranslationMode)
		{
            // free translation mode is on, which limits the phrase box locations so
            // disallow the jump
			pFrame = m_pApp->GetMainFrame();
			wxASSERT(pFrame);
			wxASSERT(&pFrame->m_pComposeBar);
			pEdit = (wxTextCtrl*)pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
			
            // NOTE: the Update handler will prevent OnButtonNextNote() being entered when
            // in Free Translation mode, so control should never get here. But in case
            // someone changes things, moving to the next (or previous) note when in Fr.
            // Trans mode is bad news - because in that mode the phrase box location is
            // limited to the start of existing free trans sections, and in the part of the
            // doc which does not have free trans sections yet, to jump to a note location
            // would result in a free trans section being defined there - which is unlikely
            // to be what is wanted because notes can be anywhere and are unlikely to just
            // be at locations where we'd want a free translation to start. BEW comment
            // 18Oct06; actually control WILL get to here when the Find Next button is used
            // when in Free Trans mode, so here we trap it and prevent any mess from
            // happening
			
			// since we are in free translation mode, we want the focus to be in the
			// Compose Bar's edit box
			if (m_pApp->m_bFreeTranslationMode)
			{
				pEdit->SetFocus();
			}
			return; // do nothing else
		}
		else if (pSrcPhrase->m_bRetranslation && !gbIsGlossing)
		{
            // we are in adapting mode and the sourcephrase with the note lies within a
            // retranslation so the box location will end up different than the note
            // location, so set the note location (ie. its sequence number) since we
            // already know it
			m_pApp->m_nSequNumBeingViewed = nBoxSequNum; // the note dialog needs this 
			// value to be correct
            // now work out where the active location (ie. phrase box) should be
			m_pApp->m_nActiveSequNum = nNoteSequNum;
			m_pApp->m_pActivePile = m_pView->GetPile(nNoteSequNum);
			wxASSERT(m_pApp->m_pActivePile);
			
            // this 'active' location is invalid, because the phrase box can't be put in a
            // retranslation, so we'll try put the box before the retranslation, if not,
            // then after it
			CSourcePhrase* pSafeSrcPhrase = m_pView->GetPrevSafeSrcPhrase(pSrcPhrase);
			if (pSafeSrcPhrase == NULL)
			{
				// not a safe place, so try after the retranslation
				pSafeSrcPhrase = m_pView->GetFollSafeSrcPhrase(pSrcPhrase);
				if (pSafeSrcPhrase == NULL)
				{
					// horrors! nowhere is safe - if so, just open it within 
					// the retranslation
					goto a;
				}
			}
			pSrcPhrase = pSafeSrcPhrase;
			m_pApp->m_nActiveSequNum = pSafeSrcPhrase->m_nSequNumber;
			m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);
			wxASSERT(m_pApp->m_pActivePile);
			
			// now do the stuff common to all three of these possibilities
			goto a;
		}
		else
		{
            // there should be no restraint against us placing the box at the same location
            // as the note
			m_pApp->m_nActiveSequNum = nNoteSequNum;
			m_pApp->m_pActivePile = m_pView->GetPile(nNoteSequNum);
			wxASSERT(m_pApp->m_pActivePile);
			m_pApp->m_nSequNumBeingViewed = m_pApp->m_nActiveSequNum; // note dialog needs this
			// value be to correct
			// now do the stuff common to each of these above three possibilities
		} // end of block for not in a retranslation and not in free translation mode
	} // end of block for testing that a jump is possible
	
    // get the phrase box contents appropriate for the new location & handle the
    // possibility that the new active location might be a "<Not In KB>" one
a:	if (!pSrcPhrase->m_bHasKBEntry && pSrcPhrase->m_bNotInKB)
	{
		// this ensures user has to explicitly type into the box and explicitly check the
		// checkbox if he wants to override the "not in kb" earlier setting at this
		// location
		m_pApp->m_bSaveToKB = FALSE;
		m_pApp->m_targetPhrase.Empty();
		m_pApp->m_pTargetBox->m_bAbandonable = TRUE;
	}
	else
	{
		if (!pSrcPhrase->m_adaption.IsEmpty())
		{
			m_pApp->m_targetPhrase = pSrcPhrase->m_adaption;
			m_pApp->m_pTargetBox->m_bAbandonable = FALSE;
		}
		else
		{
			m_pApp->m_pTargetBox->m_bAbandonable = TRUE;
			if (m_pApp->m_bCopySource)
			{
				m_pApp->m_targetPhrase = 
				m_pView->CopySourceKey(pSrcPhrase,m_pApp->m_bUseConsistentChanges);
			}
			else
			{
				m_pApp->m_targetPhrase.Empty();
			}
		}
	}

	// remove the text from the KB, if refString is not null
	wxString emptyStr = _T("");
	if (gbIsGlossing)
		m_pApp->m_pGlossingKB->GetAndRemoveRefString(pSrcPhrase,emptyStr,useGlossOrAdaptationForLookup);
	else
		m_pApp->m_pKB->GetAndRemoveRefString(pSrcPhrase,emptyStr,useGlossOrAdaptationForLookup);

	// recalculate the layout
#ifdef _NEW_LAYOUT
	m_pLayout->RecalcLayout(pList, keep_strips_keep_piles);
#else
	m_pLayout->RecalcLayout(pList, create_strips_keep_piles);
#endif

	// recalculate the active pile
	m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);

	// scroll the active location into view, if necessary
	m_pApp->GetMainFrame()->canvas->ScrollIntoView(m_pApp->m_nActiveSequNum);

	m_pLayout->m_docEditOperationType = default_op;

	// restore default button image, and m_bCopySourcePunctuation to TRUE
	wxCommandEvent event;
	if (!m_pApp->m_bCopySourcePunctuation)
	{
		m_pView->OnToggleEnablePunctuationCopy(event);
	}

	m_pView->Invalidate(); // get Draw() done & phrase box shown
	m_pLayout->PlaceBox();

	// now put up the note dialog at the m_nSequNumBeingViewed location
	wxASSERT(m_pApp->m_pNoteDlg == NULL);
	m_pApp->m_pNoteDlg = new CNoteDlg(m_pApp->GetMainFrame());
	// wx version: we don't need the Create() call for modeless notes dialog
	m_pView->AdjustDialogPosition(m_pApp->m_pNoteDlg); // show it lower, not at top left
	m_pApp->m_pNoteDlg->Show(TRUE);
}
	
/////////////////////////////////////////////////////////////////////////////////
/// \return     void
/// \param      pFromSrcPhrase  ->  the instance where the Note currently is
/// \param      pToSrcPhrase    ->  the instance where the Note will be put
/// \remarks	
// it is the caller's responsibility to determine which sourcephrase is to receive the
// note, and it must exist, and the sourcephrase passed in as pFromSrcPhrase must have a
// note (caller must bleed out any situations where this is not the case)
/// BEW 25Feb10, updated for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
void CNotes::MoveNote(CSourcePhrase* pFromSrcPhrase, CSourcePhrase* pToSrcPhrase)
{
	wxString noteStr;
	wxString emptyStr = _T("");
	if ( pFromSrcPhrase->m_bHasNote && pFromSrcPhrase->GetNote().IsEmpty())
	{
		// this sourcephrase instance contains no filtered note, so just return
		return;
	}
	else
	{
		// this source phrase contains a note (it may be empty)...
		// first determine that the target sourcephrase has no note,
		// since it is invalid to move a note to such a one
		if (pToSrcPhrase->m_bHasNote || !pToSrcPhrase->GetNote().IsEmpty())
		{
            // it has a note, so do nothing (no message here, the GUI button's
            // handler has a test for this and disables the command if necessary,
			// but for programatic use of MoveNote, we want a silent return and
			// auto-fixing of the location)
            if (!pToSrcPhrase->m_bHasNote)
			{
				// reset the flag, since m_note must be non-empty
				pToSrcPhrase->m_bHasNote = TRUE;
			}
			return;
		}
		
		// get the note's content
		noteStr = pFromSrcPhrase->GetNote();
		pFromSrcPhrase->m_bHasNote = FALSE; // ensure the flag is cleared 
											// on the old location
		pFromSrcPhrase->SetNote(emptyStr); // clear old note
		
		// now create the note on the pToSrcPhrase instance
		pToSrcPhrase->SetNote(noteStr);
		pToSrcPhrase->m_bHasNote = TRUE;
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     void
/// \remarks	Moves to the first note and opens it.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::MoveToAndOpenFirstNote()
{
	// is a note dialog open, if so - close it (and invoke the OK button's handler) it's
    // location defines the starting sequence number from which we look forward for the
    // next one -- but if the dialog is not open, then the phrase box's location is where
    // we start looking from
	int nJumpOffSequNum = 0;
	if (m_pApp->m_pNoteDlg != NULL)
	{
		// the note dialog is still open, so save the note and close the dialog
		wxCommandEvent oevent(wxID_OK);
		m_pApp->m_pNoteDlg->OnOK(oevent);
		m_pApp->m_pNoteDlg = NULL;
	}
	JumpForwardToNote_CoreCode(nJumpOffSequNum);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     void
/// \remarks	Moves to the last note and opens it.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::MoveToAndOpenLastNote()
{
	// is a note dialog open, if so - close it (and invoke the OK button's handler) it's
    // location defines the starting sequence number from which we look backward for the
    // last one -- but if the dialog is not open, then the phrase box's location is where
    // we start looking from
	int nJumpOffSequNum = m_pApp->GetMaxIndex();
	if (m_pApp->m_pNoteDlg != NULL)
	{
		// the note dialog is still open, so save the note and close the dialog
		wxCommandEvent event(wxID_OK);
		m_pApp->m_pNoteDlg->OnOK(event);
		m_pApp->m_pNoteDlg = NULL;
	}
	JumpBackwardToNote_CoreCode(nJumpOffSequNum);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if successful, FALSE if the move could not be done
/// \param      pLocationsList  -> an array of consecutive sequence numbers where 
///                                (squeezed) Notes will be reconstituted (because they
///                                would not fit in the new edit span)
/// \param      nLeftBoundSN    -> enables us to detect when leftwards movement is 
///                                no longer possible, and FALSE is returned
/// \remarks
/// Called from: the View's RestoreNotesAfterSourceTextEdit().
/// Moves a note to the left one place.
/// pLocationsList is an array of consecutive sequence numbers where (squeezed) Notes will
/// be reconstituted (because they would not fit in the new edit span), and the caller has
/// determined that the next consecutive location in the document would be a right bound
/// (that is, a non-removed note already is there) and so the location about to be added to
/// the squeezed array can't be used. In this circumstances, we need to call this function
/// to move all the locations in the pLocationsList one place leftwards (ie. decrease the
/// stored indices by one) in order to open a gap between the last stored index and the
/// right bound location so that the next note location can be added as the gap location.
/// This works, as often as necessary, unless the stored index values come to the left
/// bound (which is either the start of the document, or an unremoved note's location lying
/// to the left of the edit span). The nLeftBoundSN enables us to detect when leftwards
/// movement is no longer possible, and FALSE is returned. A successful leftwards movement
/// returns TRUE.
/// (If all the locations between left and right bounds are used up and still there are
/// notes to be placed, the caller will attempt to move the note which is the right bound
/// to the right to create the needed gaps.)
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/// BEW 9July10, no changes needed for support of kbVersion 2
/// BEW 27Sep10, trying to remove a big slab of source text near end of the document in
/// which there were many Notes, pLocationsList was an empty lis, and so the test for
/// (*pLocationsList)[0] failed. The appropriate behaviour is to test for an empty list
/// and return FALSE if so - then the caller will simply abandon those notes because they
/// can't be placed anywhere, and tell the user so.
/////////////////////////////////////////////////////////////////////////////////
bool CNotes::MoveNoteLocationsLeftwardsOnce(wxArrayInt* pLocationsList, int nLeftBoundSN)
{
	if (pLocationsList->IsEmpty())
		return FALSE; // abandon any unplaced notes which remain

	// BEW added 30May08 in support of the source text editing step of the 
	// vertical editing process
	int aSequNum;
	// get the first sequence number index from the list
	aSequNum = (*pLocationsList)[0];
	// if there is a gap between it and the left bound, then shift all the values one
	// place to the left and return TRUE, otherwise do not shift and return FALSE
	if (nLeftBoundSN + 1 < aSequNum)
	{
		// a gap exists, so leftshift
		int count = pLocationsList->GetCount();
		int i;
		int value;
		for (i=0;i<count;i++)
		{
			value = (*pLocationsList)[i];
			(*pLocationsList)[i] = value - 1;
		}
		return TRUE;
	}
	// the attempt was unsuccessful
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if there were no errors, FALSE if there was an error
/// \param      pSrcPhrases ->  pointer to the document's m_pSourcePhrases list (the
///                             Note(s) will be restored to instances of CSourcePhrase in
///                             this list)
/// \param      pRec        ->  pointer to the EditRecord for the vertical edit process
/// \remarks
/// Called from: the View's OnEditSourceText().
/// Restores notes to the source text after having been removed during source text editing.
/// The passed in EditRecord contains the information about the source text edit which has
/// just been completed, and it includes an array, arrNotesSequNumbers, which stores in
/// normal order the sequence number indices for each Note that was removed from the
/// original editable span (this span will have been extended beyond the user's selection
/// to include all of any retranslation(s) it overlaps, but if there were no such overlaps
/// then it was the same as the user's selection). The array is used to assist in
/// relocating the removed notes. This is a difficult process because the user may have
/// done a minor edit, or a major one, or removed his entire selection from the document.
/// The best the function can do is to try relocate the notes in approximately the same
/// locations as much as possible; without reordering any, and within the bounds formed by
/// unremoved notes in the preceding and following contexts, or the document's start or
/// end. For an edit which results in the same number of CSourcePhrase instances in the
/// final edit span, or more, this is easy to do; when fewer instances result, some
/// squeezing of note locations may be required, and possibly even relocating some notes,
/// or all, in the following context - and if there is insufficient following context for
/// that, left-shifting some or all in order to create empty locations for the
/// unreplaceable ones to be replaced. The whole of this process is encapsulated in this
/// function; unfortunately it isn't trivial to do, and if there are too many Notes to be
/// replaced near the end of the document so that not all can be replaced, then the
/// unreplaceable ones are simply lost - but the user is given a message saying so.
/// BEW 26May08	function created as part of refactoring the Edit Source Text functionality
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/// BEW 9July10, no changes needed for support of kbVersion 2
/////////////////////////////////////////////////////////////////////////////////
bool CNotes::RestoreNotesAfterSourceTextEdit(SPList* pSrcPhrases, EditRecord* pRec)
{
	wxArrayInt arrUnsqueezedLocations; // for those locations for which the old sequence
	// numbers are still valid, and in the edit span (in its final form)
	wxArrayInt arrSqueezedLocations; // for those locations for which the old sequence
	// numbers are not valid and therefore are squeezed leftwards
	
	// notes were removed, but this happens only for those in the editable span, and in
	// left to right order; so get the removals' minimum and maximum sequence numbers
	int nNumRemoved = pRec->arrNotesSequNumbers.GetCount();
	int nNumUnsqueezedLocations = -1;
	int nEditSpanStartingSN = pRec->nStartingSequNum; // this could be the start of the
					// 'following context' if the user's edit was to remove everything in
					// the edit span
	int nEditSpanEndingSN; // for the same reason this once can also be at the start
					// of the former following context
	if (pRec->nNewSpanCount == 0)
	{
		nEditSpanEndingSN = pRec->nStartingSequNum;
	}
	else
	{
		nEditSpanEndingSN = pRec->nStartingSequNum + pRec->nNewSpanCount - 1;
	}
	
    // find where there are existing unremoved notes, earlier and later than the edit span,
    // which will act as bounds for our note relocations - we can't transgress these bounds
    // without causing a reordering of at least a couple of notes, and reordering is
    // forbidden
	int nLeftBound;
	int nRightBound;
	bool bNoError;
	if (nEditSpanStartingSN - 1 < 0)
	{
		// the edit span commenced at the start of the document, so the edit span's start
		// has to be the left bound
		nLeftBound = 0;
	}
	else
	{
		// FALSE in the last parameter of the next call means 'seach backwards'
		// (when searching backwards, the initial location is ignored; but not when
		// searching forward, so pass in the edit span's starting SN value)
		bNoError = FindNote(pSrcPhrases, nEditSpanStartingSN, nLeftBound, FALSE);
		if (!bNoError)
		{
			// an error message has been seen already, so return FALSE to the
			// caller to make the vertical edit process BailOut() function be called
			return FALSE;
		}
		if (nLeftBound == -1)
		{
			// no leftwards note was found, so the left bound is the start of the
			// document (otherwise, it is the value returned in nLeftBound)
			nLeftBound = 0;
		}
	}
	// Now nRightBound, no final BOOL parameter in the next call means 'seach forwards'
	bNoError = FindNote(pSrcPhrases, nEditSpanEndingSN, nRightBound); 
	if (!bNoError)
	{
        // an error message has been seen already, so return FALSE to the caller to make
        // the vertical edit process BailOut() function be called
		return FALSE;
	}
	if (nRightBound == -1)
	{
		// no forwards note was found, so the right bound is the end of the document
		// (otherwise, it is the value returned in nRightBound)
		nRightBound = m_pApp->GetMaxIndex();
	}
	
    // loop to handle the cases where note replacement can occur without location changes
    // (but there is no guarantee that any one note will be on the same word as formerly,
    // nor even that the notes' contents are appropriate for the new meaning at the places
    // where they will be put); these locations we will store in the unsqueezed array;
    // we'll put the remainders in the squeezed array; but we'll put all in the squeezed
    // array if the user's edit was to delete the entire editable source text string
	int index;
	int aNoteSN;
	int nCurrentSN = nEditSpanStartingSN; // potential storage location's index
	if (pRec->nNewSpanCount == 0)
	{
        // the user deleted all the source text shown him in the dialog, so notes removed
        // have nowhere to go, so we'll just bunch them up in consecutive CSourcePhrase
        // instances in the old following context; but moving them left to create gaps if
        // necessary
		bool bShiftingRightWorked;
		for (index = 0; index < nNumRemoved; index++)
		{
			if (!IsNoteStoredHere(pSrcPhrases, nCurrentSN))
			{
				// no note at this location, so we can restore a note at this location
				if (index+1 > (int)arrSqueezedLocations.GetCount())
					arrSqueezedLocations.SetCount(index+1); // any added elements to the 
				// array are assigned int(0) by default
				arrSqueezedLocations[index] = nCurrentSN;
				nCurrentSN++; // get the next potential storage location's index
			}
			else
			{
                // we've come to a bound, that is, a note already existing at this location
                // so we'll move it, and as many consecutive following ones as there are,
                // one place to the right, to create a gap for restoring the present one.
                // If we don't succeed in creating a gap this way (the only way to prevent
                // it would be to have come to the end of the document with a series of
                // consecutive notes), we'll next try left-shifting the already stored
                // locations (remember, these are locations without notes by definition, so
                // we are just decrementing stored index values) to create a gap that way -
                // this will work so long as we don't go bump into the nLeftBound value,
                // but if that unthinkable thing should happen, the only recourse is to
                // insert the remaining Notes' text into the removed free translations
                // list, and tell the user he can get it/them from there by entering Free
                // Translation Mode and using the combobox there to have any visible Note
                // in the list sent to the Clipboard - after which he can create an empty
                // note (it can be done in Free Translation Mode) somewhere and paste the
                // old Note text into it.
				bShiftingRightWorked = 
				ShiftASeriesOfConsecutiveNotesRightwardsOnce(pSrcPhrases, nCurrentSN);
				if (bShiftingRightWorked)
				{
					// exploit the gap we created by the above call
					if (index+1 > (int)arrSqueezedLocations.GetCount())
						arrSqueezedLocations.SetCount(index+1); // any added elements to 
					// the array are assigned int(0) by default
					arrSqueezedLocations[index] = nCurrentSN;
					nCurrentSN++; // get the next potential storage location's index
				}
				else
				{
                    // there are more to replace, but no more document space at the end on
                    // which to store them, so now try leftshifting as explained above
					bool bLeftwardsOK = MoveNoteLocationsLeftwardsOnce(
											&arrSqueezedLocations, nLeftBound);
					if (bLeftwardsOK)
					{
						// exploit the gap we created by the above call
						nCurrentSN--; // the gap is at the end of the valid locations 
						// in the array, but we above had incremented nCurrentSN to
						// point past that location, so we must decrement it so it
						// indexes the gap we created
						if (index+1 > (int)arrSqueezedLocations.GetCount())
							arrSqueezedLocations.SetCount(index+1); // any added elements to 
						// the array are assigned int(0) by default
						arrSqueezedLocations[index] = nCurrentSN;
						nCurrentSN++; // get the next potential storage location's index
					}
					else
					{
                        // left shifting indices bumped up against the left bound - we will
                        // call it quits and just abandon the rest, and tell the user
						//
                        // BEW changed 2Sep08, I want user's click on an item in the
                        // Removed list to send the clicked string to the phrase box or
                        // free translation Compose box, directly. If I let potentially
                        // huge Notes be stored in the free translations list, this could
                        // result in unwieldly large strings having to be dealt with in the
                        // Compose bar's edit box. Since an unreplaceable Note is, in
                        // practical terms, almost an impossibility, I'll just instead
                        // delete unreplaceable ones, but still warn the user what has
                        // happened.
						
						// delete any which remain
						pRec->storedNotesList.Clear();
						
						// warn the user about what has happened -- it is extremely unlikely
						// that space would be so tight that we'd have to abandon a note
						int nRemainder = nNumRemoved - index;
						wxString aStr; 
						// IDS_UNPLACEABLE_NOTES
						aStr = aStr.Format(
_("Some temporarily removed Notes could not be restored to the document due to lack of space, so they have been discarded. Number of notes discarded: %d"),
						nRemainder);
						wxMessageBox(aStr, _T(""), wxICON_INFORMATION | wxOK);
						break; // break out of the loop and let the rest of the 
						// function do the replacements of those that were successfully
						// relocated and stored in arrSqueezedLocations
					}
				}
			}
		}
	}
	else
	{
		// there is some new edited text, so the possibility of reconstituting at least some
		// of the removed notes on CSourcePhrase instances within the new edit span remains
		for (index = 0; index < nNumRemoved; index++)
		{
			aNoteSN = pRec->arrNotesSequNumbers[index]; // get the next note's location
			if (aNoteSN >= nEditSpanStartingSN && aNoteSN <= nEditSpanEndingSN)
			{
				// the note index falls within the edit span, store it unchanged in value
				// in the "unsqueezed" array
				arrUnsqueezedLocations.Add(aNoteSN);
			}
			else
			{
                // the note index falls later than the end of the edit span, so just store
                // it unchanged for the present in the "squeezed" array
				arrSqueezedLocations.Add(aNoteSN);
			}
		}
		if (arrSqueezedLocations.GetCount() == 0)
		{
            // all the removed ones were placed within the edit span, so go to en(d) to
            // have the relocated notes reconstituted in the document
			goto en;
		}
		else
		{
            // some would not fit in the edit span, so try fit the other ones within it at
            // its end, failing that, create gaps by left shifting and try fit at the end
			bool bRelocatedThemAll = BunchUpUnsqueezedLocationsLeftwardsFromEndByOnePlace(
									  pRec->nStartingSequNum, pRec->nNewSpanCount,
									  &arrUnsqueezedLocations, &arrSqueezedLocations, 
									  nRightBound);
			if (!bRelocatedThemAll)
			{
                // not all were successfully re-located, the remainder which are as yet
                // unrelocated are in arrSqueezedLocations; we will locate them in the
                // following context for the edit span, and move existing unremoved notes
                // rightwards to create gaps as far as possible, this will work except when
                // there are a lot of notes yet to be placed and we are at or near the
                // document's end and there can't be a sufficient number of note movements
                // to the right in order to accomodate those that remain in the locations
                // vacated -- if that happens, the last unrelocated ones will just be
                // stored at the top of the removed free translations list, so as to be
                // accessible (though only when in free translation mode) rather than lost
                // entirely
				int nRemainderCount = arrSqueezedLocations.GetCount();
				wxASSERT(nRemainderCount > 0);
				int nStartAt = pRec->nStartingSequNum + pRec->nNewSpanCount; // the first 
				// location following the edit span
				// check this is a valid index within the document's list
				if (nStartAt <= m_pApp->GetMaxIndex())
				{
                    // the location is within the document; so we check for the presence of
                    // an existing Note, and if there is none, we use the location as a
                    // location for recreating the next Note, and remove the latter's old
                    // SN index from the start of the arrSqueezedLocations array. If we
                    // bump against an already existing Note, we'll move it (and any
                    // consecutive ones following it) a location rightwards, to create the
                    // gap we need, etc.
					nNumUnsqueezedLocations = arrUnsqueezedLocations.GetCount(); // make 
					// sure it's value is current
					for (index = 0; index < nRemainderCount; index++)
					{
						// iterate over all those remaining unrelocated, relocating each...
						aNoteSN = nStartAt + index;
						bool bHasNote = IsNoteStoredHere(pSrcPhrases, aNoteSN);
						if (bHasNote)
						{
                            // we've bumped against an already existing Note still in the
                            // document, so we have to move it righwards (any any
                            // consecutives which follow it) if we can...
							bool bMoveRightWorked = 
							ShiftASeriesOfConsecutiveNotesRightwardsOnce(pSrcPhrases, aNoteSN);
							if (bMoveRightWorked)
							{
                                // we've created a gap at aNoteSN location, so set this
                                // location up as one for Note relocation in the code later
                                // below
								arrSqueezedLocations.RemoveAt(0);
                                // whm note: wxArrayInt's Insert method reverses the
                                // parameters! Caution: wx docs also says of
                                // wxArray::Insert() "Insert the given number of copies of
                                // the item into the array before the existing item n. This
                                // resulted in incorrect ordering of source phrases, so we
                                // use array[] = assignment notation instead. Bruce's note
                                // indicates that it is going to "insert at the array's
                                // end", so to be safe we ensure that the array has at
                                // least nNumInUnsqueezedArray elements by calling
                                // SetCount()
								if (nNumUnsqueezedLocations + 1 > 
									(int)arrUnsqueezedLocations.GetCount())
									arrUnsqueezedLocations.SetCount(nNumUnsqueezedLocations+1);
								arrUnsqueezedLocations[nNumUnsqueezedLocations] = aNoteSN;
								nNumUnsqueezedLocations = arrUnsqueezedLocations.GetCount();
							}
							else
							{
                                // the move rightwards by one location did not succeed
                                // (probably we got to the end of the document); so the
                                // only recourse we have is to store the remaining stored
                                // Note text strings at the head of the removed free
                                // translations list (to make them accessible in Free
                                // Translation mode)
								int nRemainder = nRemainderCount - index;
								int nNewIndex;
								wxArrayString* pRemList = new wxArrayString;
								wxASSERT(pRemList != NULL);
								wxString aNote;
								int lastIndexPos;
								lastIndexPos = pRec->storedNotesList.GetCount() -1;
								for (nNewIndex = index; nNewIndex < nRemainder; nNewIndex++)
								{
									aNote = pRec->storedNotesList.Item(lastIndexPos);
									lastIndexPos--;
									pRemList->Insert(aNote,0);
									if (lastIndexPos < 0)
										break;
								}
								bool bResult = m_pView->InsertSublistAtHeadOfList(pRemList, 
																freeTranslationsList, pRec);
								if (!bResult)
								{
									// there was an error (an unknown list was requested in
									// the switch) - message not localizable (not likely
									// to be ever seen)
									wxString errStr = 
									_T("InsertSublistAtHeadOfList(), for storing un-restorable ");
									errStr += 
									_T("Notes in the free translations list (just some from tail), failed. ");
									errStr += 
									_T("Edit process abandoned. Document restored to pre-edit state.");
									wxMessageBox(errStr,_T(""), wxICON_EXCLAMATION | wxOK);
									pRemList->Clear();
									if (pRemList != NULL) // whm 11Jun12 added NULL test
										delete pRemList;
									return FALSE;
								}
								
								if (pRemList != NULL) // whm 11Jun12 added NULL test
									delete pRemList; // the free translations list now 
								// manages these strings, so don't do any Remove()
								// before doing this deletion
								// warn the user about what has happened ?? Nah
								break; // break out of the loop and let the rest of 
								// the function do the replacements of those that
								// were successfully relocated and stored in
								// arrSqueezedLocations
							} // end of else block for test bMoveRightWorked == TRUE
						}
						else
						{
							// the aNoteSN index has no Note at this location, 
							// so we can create one there
							arrSqueezedLocations.RemoveAt(0);
                            // whm note: wxArrayInt's Insert method reverses the
                            // parameters! Caution: wx docs also says of wxArray::Insert()
                            // "Insert the given number of copies of the item into the
                            // array before the existing item n. This resulted in incorrect
                            // ordering of source phrases, so we use array[] = assignment
                            // notation instead. Bruce's note indicates that it is going to
                            // "insert at the array's end", so to be safe we ensure that
                            // the array has at least nNumInUnsqueezedArray elements by
                            // calling SetCount()
							if (nNumUnsqueezedLocations + 1 > 
								(int)arrUnsqueezedLocations.GetCount())
								arrUnsqueezedLocations.SetCount(nNumUnsqueezedLocations+1);
							arrUnsqueezedLocations[nNumUnsqueezedLocations] = aNoteSN;
							nNumUnsqueezedLocations = arrUnsqueezedLocations.GetCount();
						}
					} // end of for loop for relocating all those we couldn't fit in 
					// edit span at the start of the context following the edit span 
					// - rightshifting notes if necessary
				} // end of TRUE block for test nStartAt <= GetMaxIndex()
				else
				{
                    // we are already at the end of the document, so the remainders have to
                    // be put into the removed free translation list; we'll remove them in
                    // a loop from the tail of the CStringList in pRec, and insert each
                    // such one at the start of a new CStringList which we will then insert
                    // at the start of the removed free translations list
					arrSqueezedLocations.Clear(); // clear these stored SN indices, 
					// so that the Note creations done below won't wrongly
					// grab the locations stored in arrSqueezedLocations
					int nNewIndex;
					wxArrayString* pRemList = new wxArrayString;
					wxASSERT(pRemList != NULL);
					wxString aNote;
					int lastIndexPos;
					lastIndexPos = pRec->storedNotesList.GetCount() -1;
					for (nNewIndex = 0; nNewIndex < nRemainderCount; nNewIndex++)
					{
						aNote = pRec->storedNotesList.Item(lastIndexPos);
						lastIndexPos--;
						pRemList->Insert(aNote,0);
					}
					bool bResult = m_pView->InsertSublistAtHeadOfList(pRemList, freeTranslationsList, pRec);
					if (!bResult)
					{
						// there was an error (an unknown list was requested in
						// the switch)
						wxString errStr = 
						_T("InsertSublistAtHeadOfList(), for storing un-restorable ");
						errStr += 
						_T("Notes in the free translations list (all the removed ones), failed. ");
						errStr += 
						_T("Edit process abandoned. Document restored to pre-edit state.");
						wxMessageBox(errStr,_T(""), wxICON_EXCLAMATION | wxOK);
						pRemList->Clear();
						if (pRemList != NULL) // whm 11Jun12 added NULL test
							delete pRemList;
						return FALSE;
					}
					
					if (pRemList != NULL) // whm 11Jun12 added NULL test
						delete pRemList; // the free translations list now manages 
					// these strings, so don't do any Remove() before doing this
					// deletion
					// warn the user about what has happened ?? No
				}
			}
		}
	}
	
    // build the notes at the required locations, using the stored new sequence numbers,
    // after making the unsqueezed and squeezed arrays into one; we don't need to keep them
    // separate anymore, because any values stored in the squeezed array will be at higher
    // sequence numbers than any in the unsqueezed array
en:	;
	// In wx we need to add the elements of another array manually
	int arrCt;
	for (arrCt = 0; arrCt < (int)arrSqueezedLocations.GetCount(); arrCt++)
	{
		arrUnsqueezedLocations.Add(arrSqueezedLocations.Item(arrCt));
	}
	
	//return FALSE; // uncomment out in order to test the document restoration code
	
	wxString strNoteText;
	int nNumToReplace = arrUnsqueezedLocations.GetCount();
	for (index = 0; index < nNumToReplace; index++)
	{
        // get each Note string and create its Note on the appropriate CSourcePhrase
        // instance, there should not be any Notes already present at these locations, but
        // we'll check and if there is, we've an error state which must cause the vertical
        // edit process to be abandoned and the earlier doc state rebuilt
		// BEW added 19Jun09, if end of doc edited away, including one or more notes,
		// we'll assume they are unwanted and can be abandoned; so test for index values
		// beyond the doc end, and if so, just continue to next iteration without
		// recreating the note
		aNoteSN = arrUnsqueezedLocations[index]; // get the next note's location
		if (aNoteSN > m_pApp->GetMaxIndex())
			continue;
		strNoteText = pRec->storedNotesList.Item(index); // get the next note's text
		if (IsNoteStoredHere(pSrcPhrases, aNoteSN))
		{
			// error, so cause bailout after showing the developer a message
			wxString errStr = _T(
			"Error in RestoreNotesAfterSourceTextEdit(), attempted to restore ");
			errStr += _T(
			"a Note at a location where there was supposed to be no Note already stored. ");
			errStr += _T("Edit process abandoned. Document restored to pre-edit state.");
			wxMessageBox(errStr,_T(""), wxICON_EXCLAMATION | wxOK);
			return FALSE;
		}
		else
		{
			// these is no note at this location, so create it there now
			bool bAllsWell = CreateNoteAtLocation(pSrcPhrases, aNoteSN, strNoteText);
			if (!bAllsWell)
			{
				// there was an unexpected error - either a bounds error, 
				// or a note already present
				wxString errStr = _T(
				"Error in RestoreNotesAfterSourceTextEdit(), the CreateNoteAtLocation() ");
				errStr += _T(
				"function returned FALSE, either because of a bounds error (past doc end) or ");
				errStr += _T(
				"there really was a Note already stored at this location when all the many ");
				errStr += _T("previous checks said there wasn't!! ");
				errStr += _T(
				"Edit process abandoned. Document restored to pre-edit state.");
				wxMessageBox(errStr,_T(""), wxICON_EXCLAMATION | wxOK);
				return FALSE;
			}
		}
	}
	
	// Test the error handling code by returning FALSE, comment out next line when
	// the error handling code is working right, and instead return TRUE
	//return FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if successful, FALSE if the move could not be done
/// \param      pSrcPhrases  -> the app's m_pSourcePhrases list
/// \param      nNoteSN      -> the sequence number index for the CSourcePhrase 
///                             which stores the Note which we want to move 
///                             rightwards to the next CSourcePhrase
/// \remarks
/// Called from: the View's ShiftASeriesOfConsecutiveNotesRightwardsOnce().
/// Moves a note to the right one place.
/// The move can be done only if not at the end of the document, and provided the next 
/// CSourcePhrase does not already store a different Note.
/// BEW 25Feb10, updated for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
bool CNotes::ShiftANoteRightwardsOnce(SPList* pSrcPhrases, int nNoteSN)
{
	// BEW added 30May08 in support of the source text editing step of the 
	// vertical editing process
	SPList::Node* pos = pSrcPhrases->Item(nNoteSN);
	CSourcePhrase* pOriginalSrcPhrase = pos->GetData();
	pos = pos->GetNext();
	// check the original src phrase actually has a note
	bool bOriginalHasANote = IsNoteStoredHere(pSrcPhrases, nNoteSN);
	if (!bOriginalHasANote)
	{
		// no note at the passed in location
		return FALSE;
	}
	// check there is a following CSourcePhrase instance 
	if (pos == NULL)
	{
		// we are at the end of the document, so no destination CSourcePhrase exists
		return FALSE;
	}
	// check there is no note on the following CSourcePhrase instance, 
	// if there is, we can't shift the note to this instance
	CSourcePhrase* pDestSrcPhrase = pos->GetData(); // MFC used GetAt(pos);
	wxASSERT(pDestSrcPhrase != NULL);
	if (pDestSrcPhrase->m_bHasNote || !pDestSrcPhrase->GetNote().IsEmpty())
	{
		// it contains a note already, so we can't move another to here
		return FALSE;
	}
	// the shift is possible, so do it
	MoveNote(pOriginalSrcPhrase,pDestSrcPhrase);
	// mark the one or both owning strips invalid
	m_pApp->GetDocument()->ResetPartnerPileWidth(pOriginalSrcPhrase); // mark its strip invalid
	m_pApp->GetDocument()->ResetPartnerPileWidth(pDestSrcPhrase); // mark its strip invalid
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if successful, FALSE if the moves could not be done
/// \param      pSrcPhrases  -> the app's m_pSourcePhrases list
/// \param      nFirstNoteSN -> the sequence number index for the CSourcePhrase which stores 
///                             the first Note of the consecutive series which we want to 
///                             move rightwards one location
/// \remarks
/// Called from: the View's RestoreNotesAfterSourceTextEdit().
/// Moves a series of consecutive notes to the right during source text editing.
/// The move can be done only if not at the end of the document, and provided there is a 
/// CSourcePhrase without a Note after the consecutive series ends. The function can be 
/// used even when the location passed in is the only one which has a stored Note.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/// BEW 9July10, no changes needed for support of kbVersion 2
/////////////////////////////////////////////////////////////////////////////////
bool CNotes::ShiftASeriesOfConsecutiveNotesRightwardsOnce(SPList* pSrcPhrases, int nFirstNoteSN)
{
	// refactored 7Apr09 - only needed GetMaxIndex() call
    // BEW added 30May08 in support of the source text editing step of the vertical editing
    // process first compile an array of consecutive note locations which need to be
    // shifted right and return FALSE if we come to the end of the document without finding
    // a CSourcePhrase instance which lacks a note (and which could otherwise have accepted
    // a moved Note)
	wxArrayInt locationsArr;
	int locIndex = nFirstNoteSN;
	bool bHasNote = FALSE;
	int anArrayIndex = -1;
	while (TRUE)
	{
		anArrayIndex++;
		if (locIndex > m_pApp->GetMaxIndex())
		{
			// we've passed the end of the document without finding a location
			// that does not have a note, so we cannot succeed
			return FALSE;
		}
		bHasNote = IsNoteStoredHere(pSrcPhrases,locIndex);
		if (bHasNote)
		{
			// insert into the array of noted locations and iterate
			//
			//locationsArr.SetAtGrow(anArrayIndex,locIndex);
            // whm Note: wxArrayInt doesn't have MFC's SetAtGrow() method, but we can
            // accomplish the same thing. We can use the ::SetCount() method of wxArray to
            // ensure the array has at least anArrayIndex + 1 elements, then assign
            // locIndex to element anArrayIndex. We only call SetCount() if the array is
            // too small. The MFC docs for CAtlArray::SetAtGrow say SetAtGrow does the same
            // thing, "If iElement is larger than the current size of the array, the array
            // is automatically increased using a call to CAtrArray::SetCount."
			if (anArrayIndex+1 > (int)locationsArr.GetCount())
				locationsArr.SetCount(anArrayIndex+1); // any added elements to the array 
			// are assigned int(0) by default
			locationsArr[anArrayIndex] = locIndex;
			locIndex++;
		}
		else
		{
			// the CSourcePhrase at this locIndex value does not have a Note
			// so we have found an instance that will permit right-shifting all
			// the consecutive noted locations in the array
			break;
		}
	}
	// now iterate backwards across the array of stored locations, moving the
	// note on each one rightwards once
	int nHowMany = locationsArr.GetCount();
	int aSequNum;
	bool bShiftedOK;
	for (anArrayIndex = nHowMany - 1; anArrayIndex >= 0; anArrayIndex--)
	{
		aSequNum = locationsArr[anArrayIndex];
		bShiftedOK = ShiftANoteRightwardsOnce(pSrcPhrases, aSequNum);
		wxASSERT(bShiftedOK); // this should not have failed
		bShiftedOK = bShiftedOK; // avoid compile warning (retain this line as is)
	}
	// success, the passed in nFirstNoteSN location is not a 'gap' as far
	// as stored Notes are concerned
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if all Notes were relocated, FALSE if not
/// \param      nStartOfEditSpan -> 
/// \param      nEditSpanCount   -> 
/// \param      pUnsqueezedArr   <- 
/// \param      pSqueezedArr     <- 
/// \param      nRightBound      -> is used to make sure that when locating notes
///                                 consecutively, we don't transgress the bound and so
///                                 cause a note reordering
/// \remarks
/// Called from: the View's RestoreNotesAfterSourceTextEdit().
/// This is sort of like a leftwards version of
/// ShiftASeriesOfConsecutiveNotesRightwardsOnce() but with some important differences. 
/// 1. It doesn't relocate notes, it just decrements note locations stored in the passed in
/// pLocationsList.
/// 2. The locations it operates on are not necessarily consecutive, because this function
/// is intended to work with the "unsqueezed" array (ie. as many note locations as possible
/// kept the same as they were before the edit of source text was commenced), and the idea
/// is to start with locations at the end of the array and try to create a gap by moving
/// the last leftwards one location, creating a gap. This iterative decrementing of the
/// final location index may eventually bump up against a previous note's location also
/// stored in the array, and if that is the case, then both those locations get moved left
/// one location, creating a gap. Ultimately, if there are enough removed Notes to be
/// replaced, all the stored locations might have been closed up leftwards to be
/// consecutive after the first one - if that happens, and more gaps are needed, then the
/// whole lot are decremented by one, -- that process can happen only so long as the first
/// index in the list is greater in value than the passed in nStartOfEditSpan index value.
/// We won't move the locations to precede the final edit span, but if we get to the point
/// where we still need gaps, we'll try relocating the remainder in the following context,
/// and if necessary, they can be created by moving real notes in the following context
/// rightwards. If not all could be relocated, we will return FALSE to the caller and with
/// the pSqueezedArr still containing unlocated stored indices for the Notes unable to be
/// relocated by this function. The caller can then try moving unremoved Notes rightwards
/// to make more gaps, and if that can't get enough, the remainder of the Notes text's will
/// be stored in the top of the removed free translations list. So, return TRUE if all were
/// relocated, FALSE if not. A second scenario is that there are no entries in the
/// "unsqueezed" array, so that all the entries are in the squeezed array, with unchanged
/// values, put there by the caller. When this is the case, the function will relocate them
/// consecutively from the start of the following context to the start of the edit span;
/// because we want removals (which are all done from within the old edit span) to be
/// reconstituted within the new bounds of the edit span after the user's edit is done. The
/// nRightBound parameter is the location of the first unremoved note in the following
/// context, or if there are none, then the location of the last CSourcePhrase
/// in the document. The nRightBound value is used to make sure that when locating notes
/// consecutively, we don't transgress the bound and so cause a note reordering. If we come
/// to this bound, we'll return to the caller to let the above algorithm for placing the
/// remainder do its job.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/// BEW 9July10, no changes needed for support of kbVersion 2
/////////////////////////////////////////////////////////////////////////////////
bool CNotes::BunchUpUnsqueezedLocationsLeftwardsFromEndByOnePlace(int nStartOfEditSpan, 
									int nEditSpanCount, wxArrayInt* pUnsqueezedArr, 
									wxArrayInt* pSqueezedArr, int WXUNUSED(nRightBound))
{
	// BEW added 30May08 in support of the source text editing step of the 
	// vertical editing process
    // **** Note: in this function, the capital letter pair SN used in variable naming is
    //     an acronym for 'sequence number', that is, the variable for which it is a part
    //     of its name is an index value into the document's list, m_pSourcePhrases, which
    //     stores CSourcePhrase instances ****

	wxASSERT(nEditSpanCount > 0); // == 0 case should have been dealt with in the caller
	int nNumInSqueezedArray = pSqueezedArr->GetCount();
	wxASSERT(nNumInSqueezedArray != 0); // the function should never be called if there is
									    // nothing needing to be relocated by it
	int aNoteSN = 0; // whm: I've initialized it here to prevent warning, 
					 // but see its use below
	int index;
	int nEditSpanEndLoc = nStartOfEditSpan + nEditSpanCount - 1;
	int nLastLocInUnsqueezedArr;
	int nNumInUnsqueezedArray = pUnsqueezedArr->GetCount();
	if (nNumInUnsqueezedArray == 0)
	{
        // nothing was relocated within the edit span, so the location indices are in the
        // "squeezed" array and unlocated as yet ('unlocated' means they have their old,
        // invalid, sequence numbers unchanged as yet, but are at least in the squeezed
        // array); and the new edit span has at least one CSourcePhrase instance in it.
        // We'll just locate them consecutively within the edit span, rightwards so that
        // the last to be relocated is at the end of the edit span - if if necessary
        // starting the sequence from the start of the edit span, but no earlier; so if any
        // are still are unrelocated at that stage, we return FALSE to the caller so that
        // the caller can handle those that remain (which didn't fit within the span)
		if (nNumInSqueezedArray > nEditSpanCount)
		{
			// not all will fit, so relocate starting from nStartOfEditSpan index value,
			//as many as will fit
			for (index = 0; index < nEditSpanCount; index++)
			{
				pSqueezedArr->RemoveAt(0);
				// create an alternative location for that location just removed
				aNoteSN = nStartOfEditSpan + index;
                // whm note: wxArrayInt's Insert method reverses the parameters! Caution:
                // wx docs also says of wxArray::Insert() "Insert the given number of
                // copies of the item into the array before the existing item n. This
                // resulted in incorrect ordering of source phrases, so we use array[] =
                // assignment notation instead. Bruce's note indicates that it is going to
                // "insert at the array's end", so to be safe we ensure that the array has
                // at least nNumInUnsqueezedArray elements by calling SetCount()
				if (nNumInUnsqueezedArray+1 > (int)pUnsqueezedArr->GetCount())
					pUnsqueezedArr->SetCount(nNumInUnsqueezedArray+1);
				(*pUnsqueezedArr)[nNumInUnsqueezedArray] = aNoteSN; // insert at the array's end
				nNumInUnsqueezedArray = pUnsqueezedArr->GetCount(); // update its value
				nNumInSqueezedArray = pSqueezedArr->GetCount(); // update its value
			}
		}
		else
		{
            // all will fit, so relocate starting from wherever in the span will result in
            // them all being bunched up at the end of the span
			int nBeginAt = nEditSpanEndLoc - nNumInSqueezedArray + 1;
			int nHowManyToDo = nNumInSqueezedArray;
			for (index = 0; index < nHowManyToDo; index++)
			{
				pSqueezedArr->RemoveAt(0);
				// create an alternative location for that location just removed
				aNoteSN = nBeginAt + index;
				// whm: See note in block above
				if (nNumInUnsqueezedArray+1 > (int)pUnsqueezedArr->GetCount())
					pUnsqueezedArr->SetCount(nNumInUnsqueezedArray+1);
				(*pUnsqueezedArr)[nNumInUnsqueezedArray] = aNoteSN; // insert at the array's end
				nNumInUnsqueezedArray = pUnsqueezedArr->GetCount(); // update its value
				nNumInSqueezedArray = pSqueezedArr->GetCount(); // update its value
				if (nNumInSqueezedArray == 0)
				{
					return TRUE; // success, all are relocated now within the final edit span
				}
			}
		}
        // if control gets here, then there is still at least one unrelocated index
        // remaining in pSqueezedArr, in which case we'll return FALSE and let the caller
        // deal with what remains
		return FALSE;
	}
	else
	{
        // there is at least one replacement location within the edit span; this suggests
        // that the edit span may be large enough for the ones which didn't fit within the
        // span resulting from the user's edit to be squeezed in there at its end, so now
        // we attempt to do that, leftshifting as necessary.
		nLastLocInUnsqueezedArr = (*pUnsqueezedArr)[nNumInUnsqueezedArray - 1];

        // find the number of locations in the gap betwen the last in the unsqueezed array,
        // and the end of the edit span - if we can place all of the ones in the squeezed
        // array there using successive locations, then do so; otherwise, we'll need to
        // uses a left-shifting strategy to open up gaps; anything we successfully relocate
        // will be added at the tail of the unsqueezed array and removed from the squeezed
        // array, so that the final content of the squeezed array is what remains to be
        // relocated somewhere
		int nNumberPossibleAtEnd = nEditSpanEndLoc - nLastLocInUnsqueezedArr; // size of 
																		// the gap there
		// if an end gap exists try to fill it to whatever extent is possible
		if (nNumberPossibleAtEnd > 0 )
		{
            // do as many as possible by filling the gap, and then exit the loop to use a
            // left-shifting strategy for as many of the remainders as possible - unless
            // there are no remainders in which case we are done
			for (index = 0; index < nNumberPossibleAtEnd; index++)
			{
				// remove the first of those remaining in the squeezed list
				pSqueezedArr->RemoveAt(0);
				// create an alternative location for that location just removed
				aNoteSN = nLastLocInUnsqueezedArr + 1 + index;
                // store it at the end of the unsqueezed array - (forming one or more
                // consecutive locations there, depending on how many iterations this loop
                // can do before it exits)
                // whm note: wxArrayInt's Insert method reverses the parameters! Caution:
                // wx docs also says of wxArray::Insert() "Insert the given number of
                // copies of the item into the array before the existing item n. This
                // resulted in incorrect ordering of source phrases, so we use array[] =
                // assignment notation instead. Bruce's note indicates that it is going to
                // "insert at the array's end", so to be safe we ensure that the array has
                // at least nNumInUnsqueezedArray elements by calling SetCount()
				if (nNumInUnsqueezedArray+1 > (int)pUnsqueezedArr->GetCount())
					pUnsqueezedArr->SetCount(nNumInUnsqueezedArray+1);
				(*pUnsqueezedArr)[nNumInUnsqueezedArray] = aNoteSN;
				nNumInUnsqueezedArray = pUnsqueezedArr->GetCount(); // update its value
				nNumInSqueezedArray = pSqueezedArr->GetCount(); // update its value
				if (nNumInSqueezedArray == 0)
				{
					return TRUE; // success, all are relocated now within the final edit span
				}
			}
		}
        // If control gets here, we've more to locate, so use left-shifts to do it, at
        // least until the leftshifts bring the replacement locations to the start of the
        // final edit span - and if that happens we won't let leftshifting go any further,
        // but rather we exit and let the caller relocate those remaining in the squeezed
        // array by a filling strategy for the immediate context of CSourcePhrase instances
        // which follow the end of the final edit span
		wxASSERT(nNumInSqueezedArray > 0);
		int nNumberToRelocate = nNumInSqueezedArray;
		int nPotentialGapSN;
		int nBackIndex;
		int anUnsqueezedArrIndex;
		while(nNumberToRelocate > 0) // loop for how many times we have to try
		{
            // inner loop iterates backwards over the relocation SN values stored in the
            // unsqueezed array now, looking for gaps and leftshifting rightmost stored
            // values as necessary to create gaps
			for (anUnsqueezedArrIndex = 0; anUnsqueezedArrIndex < nNumInUnsqueezedArray; 
				anUnsqueezedArrIndex++)
			{
				nBackIndex = nNumInUnsqueezedArray - 1 - anUnsqueezedArrIndex; // indexing 
															// from the end of the array
				// find next gap
				nPotentialGapSN = (*pUnsqueezedArr)[nBackIndex] - 1; // subtract 1 from 
																	 // the stored value
				if (nPotentialGapSN < nStartOfEditSpan)
				{
					// we can't go back that far in SN values, so break out of loop, 
					// the caller must finish it
					return FALSE;
				}
                // we have not gone past the bounding nStartOfEditSpan sequ number value,
                // so the nPotentialGapSN value is still a potential gap; it won't be a gap
                // if the previously stored SN value in the pUnsqeezedArr has the same
                // value as nPotentialGapSN, in which case we keep iterating back over
                // stored values in the unsqueezed array; but if there is no previous
                // entry, we have come to the gap region (it could be empty) from the
                // nStartOfEditSpan value (inclusive) to the SN value immediately preceding
                // the first stored SN value in the pUnsqueezedArr array - where we can
                // shift left as often as we like provided no shifted stored SN value
                // becomes less than the nStartOfEditSpan. So we have to test for these
                // conditions etc.
  				if (nBackIndex == 0)
				{
                    // there isn't any earlier stored SN entry, so we've come to the region
                    // we potentially multiple leftshifts can be done, to create multiple
                    // gaps at the end of the list of leftshifted stored SN values; either
                    // we can do enough leftshifts in this code block to relocate those
                    // that remain in pSqueezedArr and return TRUE, or we do as many as
                    // possible here and return FALSE, so that the caller can handle the
                    // remainder -- either we control will return to the caller at the end
                    // of this code block
					int nVacantLocations = (*pUnsqueezedArr)[nBackIndex] - nStartOfEditSpan;
					if (nVacantLocations > 0 )
					{
                        // do as many as possible by leftshifting, and fill the gaps
                        // created at the end and then return TRUE or FALSE depending on
                        // whether or not all were handled
						int index3;
						int nDecrementBy;
						nNumInSqueezedArray = pSqueezedArr->GetCount(); // make sure the 
																		// value is uptodate
						if (nNumInSqueezedArray <= nVacantLocations)
						{
							// we can handle all of those that remain
							nDecrementBy = nNumInSqueezedArray;
							pSqueezedArr->Clear(); // abandon these, as we'll calculate new 
                                // loc'n values decrement the pUnsqueezedArr stored values
                                // so as to leftshift into the gap
							for (index3 = 0; index3 < nNumInUnsqueezedArray; index3++)
							{
								// decrement the unsqueezed array stored SN values, 
								// creating a gap at the end
								aNoteSN = (*pUnsqueezedArr)[index3]; // get next
								aNoteSN -= nDecrementBy; // decrement its stored value 
														 // by nDecrementBy
								(*pUnsqueezedArr)[index3] = aNoteSN; // restore the new value
																	 // at same index
							}
                            // the aNoteSN value on exit of the preceding loop is the last
                            // stored SN value, in pUnsqueezedArr, and so there are
                            // nDecrementBy locations available for creating the new
                            // consecutive entries required for handling the rest of the
                            // needed replacement locations not yet assigned
							int nNewSN;
							int nItsLocation;
							for (index3 = 0; index3 < nDecrementBy; index3++)
							{
                                // store the new ones at the end of the unsqueezed array
                                // whm Note: aNoteSN here is "potentially uninitialized
                                // local variable" I've initialized it at the top of this
                                // function to 0, but the logic should be checked. TODO:
								nNewSN = aNoteSN + 1 + index3; // the SN value to be stored
								nItsLocation = nNumInUnsqueezedArray + index3;
								// whm: See notes above on MFC's InsertAt vs wx Insert
								if (nItsLocation+1 > (int)pUnsqueezedArr->GetCount())
									pUnsqueezedArr->SetCount(nItsLocation+1);
								(*pUnsqueezedArr)[nItsLocation] = nNewSN;
							}
							return TRUE;
						}
						else
						{
							// we can handle only some of those that remain
							nDecrementBy = nVacantLocations;
                            // whm Note: the STL erase doesn't have a second parameter for
                            // number of removals, so we'll do it in a for loop
							int ct;
							for (ct = 0; ct < nDecrementBy; ct++)
								pSqueezedArr->RemoveAt(0);
                            // this many new loc'n values by decrementing all the
                            // pUnsqueezedArr stored values so as to leftshift into the
                            // this gap, and the remainder will be the caller's job
							for (index3 = 0; index3 < nNumInUnsqueezedArray; index3++)
							{
								// decrement the unsqueezed array stored SN values, 
								// creating a gap at the end
								aNoteSN = (*pUnsqueezedArr)[index3]; // get next
								aNoteSN -= nDecrementBy; // decrement its stored value 
														 // by nDecrementBy
								(*pUnsqueezedArr)[index3] = aNoteSN; // restore the 
														// new value at same index
							}
							// create and store the required new SN indices at the end 
							// of the edit span
							int nNewSN;
							int nItsLocation;
							for (index3 = 0; index3 < nDecrementBy; index3++)
							{
								// store the new ones at the end of the unsqueezed array
								nNewSN = aNoteSN + 1 + index3; // the SN value to be stored
								nItsLocation = nNumInUnsqueezedArray + index3;
								// whm: See notes above on MFC's InsertAt vs wx Insert
								if (nItsLocation+1 > (int)pUnsqueezedArr->GetCount())
									pUnsqueezedArr->SetCount(nItsLocation+1);
								(*pUnsqueezedArr)[nItsLocation] = nNewSN;
							}						
						}
						return FALSE;
					}
					else
					{
                        // the stored SN value at nBackIndex == 0 is already the value
                        // nStartOfEditSpan and so we can't relocate any more, so hand it
                        // back to the caller to do
						return FALSE;
					}
				} // end of block for loop end condition being satisfied, that is,
				  // nBackIndex having reached 0 with at least one more not yet relocated
				else
				{
                    // there is at least one earlier stored SN entry, so get it's value so
                    // we can compare it with the nPotentialGapSN value (reuse the aNoteSN
                    // variable for this purpose)
					aNoteSN = (*pUnsqueezedArr)[nBackIndex - 1];
					if (aNoteSN < nPotentialGapSN)
					{
                        // nPotentialGapSN is a genuine gap, so we can leftshift entry
                        // values by one to fill this gap, and then we can fill the opened
                        // gap at the nEditSpanEndLoc SN value by removing the next first
                        // element from the pSqueezedArr array, and storing a
                        // nEditSpanEndLoc as the new relation value for it in the tail of
                        // the pUnsqueezedArr array; then adjust the appropriate values to
                        // comply, and then iterate the outer loop
						int index2;
						for (index2 = nNumInUnsqueezedArray - 1; index2 >= nBackIndex; index2--)
						{
                            // decrement by 1 the stored values at the end of the
                            // unsqueezed array (aNoteSN can be reused here for this too
							aNoteSN = (*pUnsqueezedArr)[index2];
							aNoteSN--; // decrement it, leftshifting thereby by 1
							(*pUnsqueezedArr)[index2] = aNoteSN; // overwrite with the 
																 // decremented SN value
						}
                        // now we have a 'gap' at the sequence number nEditSpanEndLoc which
                        // we can use for the next so-far-unrelocated Note index, so do the
                        // relocation etc
						pSqueezedArr->RemoveAt(0); // chuck this one
                        // whm note: wxArrayInt's Insert method reverses the parameters!
                        // Caution: wx docs also says of wxArray::Insert() "Insert the
                        // given number of copies of the item into the array before the
                        // existing item n. This resulted in incorrect ordering of source
                        // phrases, so we use array[] = assignment notation instead.
                        // Bruce's note indicates that it is going to "insert at the
                        // array's end", so to be safe we ensure that the array has at
                        // least nNumInUnsqueezedArray elements by calling SetCount()
						if (nNumInUnsqueezedArray+1 > (int)pUnsqueezedArr->GetCount())
							pUnsqueezedArr->SetCount(nNumInUnsqueezedArray+1);
						(*pUnsqueezedArr)[nNumInUnsqueezedArray] = nEditSpanEndLoc; // store 
																// the relocation sequ number
						nNumInUnsqueezedArray = pUnsqueezedArr->GetCount(); // update size
						nNumInSqueezedArray = pSqueezedArr->GetCount(); // update size
						nNumberToRelocate--; // decrement the count of how many remain to be
											 // handled by the outer loop
						break; // iterate in the outer loop
					}
					else
					{
                        // aNoteSN must equal nPotentialGapSN, so these entries are
                        // consecutive, so keep iterating the inner loop to look for a gap
                        // into which we can leftshift
						continue;
					}
				} // end of the test for whether or not there is an earlier stored 
				  // SN value preceding the currently accessed one
			} // end of inner loop
		} // end of outer loop
	}
	return FALSE; // we didn't manage to relocate them all, caller can do the rest
}
	
///////////////////////////////////////////////////////////////////////////////
// Event handlers
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks	Handler for the Create Note button pressed event.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::OnButtonCreateNote(wxCommandEvent& WXUNUSED(event))
{
	CPile* pPile = NULL;
	CSourcePhrase* pSrcPhrase = NULL;
	gnOldSequNum = m_pApp->m_nActiveSequNum; // save it, to be safe
	
    // create the note attached to the first sourcephrase of a selection if there is one,
    // else do it at the active location
	int nSequNum = -1;
	CCellList* pCellList;
	if (m_pApp->m_selectionLine != -1)
	{
		// we have a selection, the pile we want is that of 
		// the selection list's first element
		pCellList = &m_pApp->m_selection;
		CCellList::Node* fpos = pCellList->GetFirst();
		pPile = fpos->GetData()->GetPile();
		if (pPile == NULL)
		{
			// unlikely, so an English message will do
			wxMessageBox(_T(
			"A zero pile pointer was returned, the note dialog cannot be put up."),
			_T(""), wxICON_EXCLAMATION | wxOK);
			return;
		}
		wxASSERT(pPile != NULL);
		m_pView->RemoveSelection(); // need Invalidate() to be called later on
	}
	else
	{
        // no selection, so just attach the note to whatever pile the caller sets up,
        // usually wherever the phraseBox currently is located; but in the case of free
        // translation mode being current, or a note in a retranslation, the caller may
        // calculate a different pile than the active one
		pPile = m_pApp->m_pActivePile;
		if (pPile == NULL)
		{
			// unlikely, so an English message will do
			wxMessageBox(_T(
			"A zero pile pointer was returned, the note dialog cannot be put up."),
			_T(""), wxICON_EXCLAMATION | wxOK);
			return;
		}
	}
	pSrcPhrase = pPile->GetSrcPhrase(); // get the sourcephrase
	nSequNum = pSrcPhrase->m_nSequNumber; // get its sequence number
	wxASSERT(nSequNum >= 0);
	
    // set m_nSequNumBeingViewed, as the note dialog also uses it, not just the View
    // Filtered Material dialog (both dialogs cannot be open at the one time and so we can
    // safely use the one variable for the same purpose in the two functionalities)
	m_pApp->m_nSequNumBeingViewed = nSequNum;
	m_pView->Invalidate();
	m_pLayout->PlaceBox();
	
	// open the dialog so the user can type in a note
	wxASSERT(m_pApp->m_pNoteDlg == NULL);
	m_pApp->m_pNoteDlg = new CNoteDlg(m_pApp->GetMainFrame()); 
    // whm using the ...ByClick form of the function here doesn't make sense to me unless
    // the user purposely clicks near the phrase box location of the note and avoids
    // scrolling afterwards positioning the phrasebox. AdjustDialogPositionByClick doesn't
    // appear to avoid the phrasebox location very well which I think is more important, so
    // I'm changing the call below to use AdjustDialogPosition() which better avoids the
    // phrasebox even with scrolling. (Bill didn't do what he said. I'll leave it.)
	m_pView->AdjustDialogPositionByClick(m_pApp->m_pNoteDlg,gptLastClick); // avoid click location
	m_pApp->m_pNoteDlg->Show(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever idle processing is enabled. If any
/// of the following conditions are TRUE this handler disables the "Open A Note dialog"
/// toolBar button: The application is only showing the target text, the active pile
/// pointer is NULL, a Note dialog is already open (the m_pNoteDlg is not NULL), if there
/// already is a note on the first source phrase of any selection, or if the targetBox is
/// not shown. Otherwise the toolbar button is enabled.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::OnUpdateButtonCreateNote(wxUpdateUIEvent& event)
{
	// whm added 26Mar12 for read-only mode
	if (m_pApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}
	
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pNoteDlg != NULL)
	{
		// there already is a note dialog open, 
		// so we can't open another until it is closed
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_selectionLine != -1)
	{
		// Having a current selection and then clicking app closure button generates a
		// crash with control entering here to try update the menus, but the
		// CSourcePhrases list and layout are clobbered -- so filter out this state here,
		// otherwise piles and so forth are attempted to be accessed after their memory
		// has been freed.
		if (m_pApp->GetLayout()->GetPileList() == NULL ||
			m_pApp->GetLayout()->GetPileList()->IsEmpty())
		{
			// There is no layout, so we are being called at app closure, so just disable
			// the menu item & return
			event.Enable(FALSE);
			return;
		}

        // if the first sourcephrase in the selection does not have a note,
        // enable the button, but if it does then disable the button
		CCellList::Node* pos = m_pApp->m_selection.GetFirst();
		while (pos != NULL)
		{
			CPile* pPile = pos->GetData()->GetPile();
			pos = pos->GetNext();
			CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
			if (pSrcPhrase->m_bHasNote)
			{
                // has a note already, so clicking this button is not the way
                // to open it - do it with the note icon in the layout instead
				event.Enable(FALSE);
				return;
			}
			else
			{
				// it's enabled only if the sourcephrase does not already have a note
				event.Enable(TRUE);
				return;
			}
		}
		event.Enable(TRUE);
		return;
	}
	if (m_pApp->m_pTargetBox->GetHandle() != NULL && m_pApp->m_pTargetBox->IsShown())
	{
		// if the phrase box is visible, then enable
		event.Enable(TRUE);
		return;
	}
	event.Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks	Handler for the Previous Note button click event.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::OnButtonPrevNote(wxCommandEvent& WXUNUSED(event))
{
    // is the note dialog open, if so - close it (and invoke the OK button's handler) it's
    // location defines the starting sequence number from which we look forward for the
    // next one -- but if the dialog is not open, then the phrase box's location is where
    // we start looking from
	int nJumpOffSequNum = m_pApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber;
	if (m_pApp->m_pNoteDlg != NULL)
	{
        // the note dialog is still open, so save the note and close the dialog and reset
        // the jump off value to the pSrcPhase where the note was attached
		nJumpOffSequNum = m_pApp->m_nSequNumBeingViewed;
		wxCommandEvent oevent(wxID_OK);
		m_pApp->m_pNoteDlg->OnOK(oevent);
		m_pApp->m_pNoteDlg = NULL;
	}
	
	// find the previous note
	if (nJumpOffSequNum == 0)
		return; // can't go back if already at the start of the doc
	nJumpOffSequNum--; // jump origin
	JumpBackwardToNote_CoreCode(nJumpOffSequNum);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle 
///                        handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever idle processing is enabled. If any
/// of the following conditions are TRUE this handler disables the "Jump To The Previous
/// Note" toolBar button: The App's m_bNotesExist flag is FALSE (there are no Notes to jump
/// to), The application is only showing the target text, the application is in free
/// translation mode, there is a selection current, or the targetBox is not being shown.
/// Otherwise the toolbar button is enabled.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::OnUpdateButtonPrevNote(wxUpdateUIEvent& event)
{
	// whm added 26Mar12 for read-only mode
	if (m_pApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}
	
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (!m_pApp->m_bNotesExist)
	{
		event.Enable(FALSE);
		return;
	}
	if (gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	// Protect against idle time update menu handler action at app shutdown 
	// time, when piles no longer exist - we must prevent pile access then
	if (m_pApp->GetLayout()->GetPileList() == NULL ||
		m_pApp->GetLayout()->GetPileList()->IsEmpty())
	{
		event.Enable(FALSE);
		return;
	}
	// Protect against idle time update menu handler action at app shutdown 
	// time, when piles no longer exist - we must prevent pile access then
	if (m_pApp->GetLayout()->GetPileList() == NULL ||
		m_pApp->GetLayout()->GetPileList()->IsEmpty())
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_selectionLine != -1)
	{
        // if there is a selection, then disable the button (doing the jump
        // and ignoring a selection might be confusing to some users)
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pNoteDlg != NULL)
	{
		// there is a note dialog open, but the button handler 
		// will close it before jumping
		event.Enable(TRUE);
	}
	if (m_pApp->m_pTargetBox != NULL && m_pApp->m_pTargetBox->IsShown()
		&& m_pApp->m_bNotesExist)
	{
		// if the phrase box is visible and there are notes 
		// in the document, then enable
		event.Enable(TRUE);
		return;
	}
	// otherwise disable
	event.Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks	Handler for the Next Note button click event.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::OnButtonNextNote(wxCommandEvent& WXUNUSED(event))
{
    // is the note dialog open, if so - close it (and invoke the OK button's handler) it's
    // location defines the starting sequence number from which we look forward for the
    // next one -- but if the dialog is not open, then the phrase box's location is where
    // we start looking from
	int nJumpOffSequNum = m_pApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber;
	if (m_pApp->m_pNoteDlg != NULL)
	{
		// the note dialog is still open, so save the note and close the dialog
		// and reset the jump off value to the pSrcPhase where the note was attached
		nJumpOffSequNum = m_pApp->m_nSequNumBeingViewed;
		wxCommandEvent oevent(wxID_OK);
		m_pApp->m_pNoteDlg->OnOK(oevent);
		m_pApp->m_pNoteDlg = NULL;
	}
	
	// find the next note
	nJumpOffSequNum++;
	JumpForwardToNote_CoreCode(nJumpOffSequNum);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever idle processing is enabled. If any
/// of the following conditions are TRUE this handler disables the "Jump To The Next Note"
/// toolBar button: The App's m_bNotesExist flag is FALSE (there are no Notes to jump to),
/// The application is only showing the target text, the application is in free translation
/// mode, there is a selection current, or the targetBox is not being shown. Otherwise the
/// toolbar button is enabled.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::OnUpdateButtonNextNote(wxUpdateUIEvent& event)
{
	// whm added 26Mar12 for read-only mode
	if (m_pApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}
	
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (!m_pApp->m_bNotesExist)
	{
		event.Enable(FALSE);
		return;
	}
	if (gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	// Protect against idle time update menu handler action at app shutdown 
	// time, when piles no longer exist - we must prevent pile access then
	if (m_pApp->GetLayout()->GetPileList() == NULL ||
		m_pApp->GetLayout()->GetPileList()->IsEmpty())
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_selectionLine != -1)
	{
		// if there is a selection, then disable the button (doing the jump
		// and ignoring a selection might be confusing to some users)
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pNoteDlg != NULL)
	{
		// there is a note dialog open, but the button handler 
		// will close it before jumping
		event.Enable(TRUE);
	}
	if (m_pApp->m_pTargetBox != NULL && m_pApp->m_pTargetBox->IsShown()
		&& m_pApp->m_bNotesExist)
	{
		// if the phrase box is visible and there are notes 
		// in the document, then enable
		event.Enable(TRUE);
		return;
	}
	// otherwise disable
	event.Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks	Handler for the Delete All Notes button click event.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/// BEW 12Nov10, needed to change the syntax here to conform to that in the wxWidgets
/// documentation. The "Yes" button click was doing nothing. I also had to add a Redraw()
/// the layout command, otherwise the note icons remain on the screen until the next redraw
/// happens.
/////////////////////////////////////////////////////////////////////////////////
void CNotes::OnButtonDeleteAllNotes(wxCommandEvent& WXUNUSED(event))
{
	int answer = wxMessageBox(_(
"You are about to cause all the notes in this document to be irreversibly deleted. Are you sure you want to do this?"),
	_T(""),wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT);
	if (answer == wxYES)
	{
		m_pView->RemoveSelection();
		
		// close any open note
		if (m_pApp->m_pNoteDlg)
		{
			wxCommandEvent cevent(wxID_CANCEL);
			m_pApp->m_pNoteDlg->OnCancel(cevent);
			m_pApp->m_pNoteDlg = NULL;
		}
		
		// delete them all
		DeleteAllNotes(); // calls Invalidate() and PlaceBox() internally
		m_pApp->GetDocument()->Modify(TRUE);

		m_pLayout->Redraw();
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever idle processing is enabled. If any
/// of the following conditions are TRUE this handler disables the "Delete All Notes"
/// toolBar button: The App's m_bNotesExist flag is FALSE (there are no Notes to jump to),
/// The application is only showing the target text, the active pile pointer is NULL.
/// Otherwise, if the targetBox is showing the toolbar button is enabled.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::OnUpdateButtonDeleteAllNotes(wxUpdateUIEvent& event)
{
	// whm added 26Mar12 for read-only mode
	if (m_pApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}
	
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (!m_pApp->m_bNotesExist)
	{
		event.Enable(FALSE);
		return;
	}
	if (gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pTargetBox->GetHandle() != NULL && m_pApp->m_pTargetBox->IsShown()
		&& m_pApp->m_bNotesExist)
	{
		// if the phrase box is visible and there are notes 
		// in the document, then enable
		event.Enable(TRUE);
		return;
	}
	event.Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::OnEditMoveNoteForward(wxCommandEvent& WXUNUSED(event))
{
	// whm added 15Mar12 for read-only mode
	if (m_pApp->m_bReadOnlyAccess)
	{
		::wxBell();
		return;
	}
	
	// Since the Move Note Forward menu item has an accelerator table hot key (CTRL-3 see
    // CMainFrame) and wxWidgets accelerator keys call menu and toolbar handlers even when
    // they are disabled, we must check for a disabled button and return if disabled.
    // On Windows, the accelerator key doesn't appear to call the handler for a disabled
    // menu item, but I'll leave the following code here in case it works differently on
    // other platforms.
	m_pApp->LogUserAction(_T("Initiated OnEditMoveNoteForward()"));
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	wxMenuBar* pMenuBar = pFrame->GetMenuBar();
	wxASSERT(pMenuBar != NULL);
	if (!pMenuBar->IsEnabled(ID_EDIT_MOVE_NOTE_FORWARD))
	{
		::wxBell();
		m_pApp->LogUserAction(_T("Move note forward menu item disabled"));
		return;
	}
	
	CPile* pPile = NULL;
	SPList* pList = m_pApp->m_pSourcePhrases;
	CSourcePhrase* pSrcPhrase = NULL;
	
	// determine which pSrcPhrase has the note - if unable, return, doing nothing
	int nSequNum = -1;
	CCellList* pCellList;
	CCellList::Node* cpos;
	CCell* pCell;
	if (m_pApp->m_selectionLine != -1)
	{
		// we have a selection, the pile we want is that of 
		// the selection list's first element
		pCellList = &m_pApp->m_selection;
		cpos = pCellList->GetFirst();
		pCell = cpos->GetData();
		pPile = pCell->GetPile();
		if (pPile == NULL)
		{
			// unlikely, so an English message will do
		a:	wxMessageBox(_T(
			"A zero pile pointer was returned, the sourcephrase with the note is indeterminate."),
			_T(""), wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(_T("A zero pile pointer was returned, the sourcephrase with the note is indeterminate."));
			return;
		}
		wxASSERT(pPile != NULL);
		m_pView->RemoveSelection(); // need Invalidate() to be called later on
	}
	else
	{
		// no selection, so just assume the note is on the sourcephrase where 
		// the phrase box is
		pPile = m_pApp->m_pActivePile;
		if (pPile == NULL)
		{
			goto a;
		}
	}
	pSrcPhrase = pPile->GetSrcPhrase(); // get the sourcephrase
	nSequNum = pSrcPhrase->m_nSequNumber; // get its sequence number
	wxASSERT(nSequNum >= 0);
	
    // the update handler will enable the menu item only if there was an unopen note stored
    // on the first of any selected sourcephrases, or if no selection, then at the active
    // location, so we can here assume pSrcPhrase has a note - so next we need to remove it
    // (after storing its content), determine what the target sourcephrase is, then
    // re-establish the note there. We do this using a function call, because there may be
    // other situations where we'd want to reestablish the note at some other location
    // other than the immediate next sourcephrase, doing so under program control (eg.
    // moving the note several sourcephrases forward or back, so that a merger could be
    // done at what would otherwise have been a place where filtered material would be
    // merger-internal which is illegal).
	SPList::Node* tgtPos = pList->Item(nSequNum);
	wxASSERT(tgtPos != NULL);
	CSourcePhrase* pTgtSrcPhrase = (CSourcePhrase*)tgtPos->GetData(); // abandon this one
	tgtPos = tgtPos->GetNext();
	if (tgtPos == NULL)
	{
        // the update handler should disable the button to prevent this possibility, but
        // just in case we'll check for it and return silently if we can't move it forward
		return;
	}
	else
	{
        // the next sourcephrase exists, so put the note there - provided there is not
        // already a note there -- and if there is, then return without doing anything as
        // the update handler will have disabled the command anyway so we shouldn't ever
        // get into this handler in that case
		pTgtSrcPhrase = (CSourcePhrase*)tgtPos->GetData();
		tgtPos = tgtPos->GetNext();
		wxASSERT(pTgtSrcPhrase != NULL); 
		if (!pTgtSrcPhrase->m_bHasNote)
		{
			// there is no note there yet
			MoveNote(pSrcPhrase,pTgtSrcPhrase);
			
            // BEW added 19Dec07: establish a selection at the new location in case the
            // user wishes to use accelerator key combination in order to move the note
            // further forward
			if (!gbShowTargetOnly)
			{
                // can do it only if showing source text and nav text whiteboard, (but
                // update handler blocks otherwise, but having this test documents things
                // better so we'll use it, though unnecessary)
				wxASSERT(m_pApp->m_selection.IsEmpty());
				int nNewSequNum = nSequNum + 1;
				CPile* pNewPile = m_pView->GetPile(nNewSequNum);
				if (pNewPile == NULL)
				{
                    // this should not happen, but just in case, we will exit and not
                    // bother with trying to make a selection, as presumably the move has
                    // succeeded, so we'll let the user try again manually
					return;
				}
				else
				{
                    // the pile pointer is known, we can assume it is valid; we'll create
                    // the selection in the cell with index = 0
					m_pApp->m_selectionLine = gnSelectionLine = 0;
					CCell* pCell = pNewPile->GetCell(0);
					m_pApp->m_selection.Insert(pCell);
					m_pApp->m_pAnchor = pCell;
					m_pApp->m_curDirection = right;
					m_pApp->m_bSelectByArrowKey = FALSE;
					
					// draw the background yellow for the CCell we want shown selected
					wxClientDC aDC(m_pApp->GetMainFrame()->canvas); // get a temporary 
					// client device context for this view window
					aDC.SetBackgroundMode(m_pApp->m_backgroundMode);
					aDC.SetTextBackground(wxColour(255,255,0)); // yellow
					pCell->DrawCell(&aDC, m_pLayout->GetSrcColor());
					pCell->SetSelected(TRUE);
				}
			}
#ifdef _NEW_LAYOUT
			m_pLayout->RecalcLayout(pList, keep_strips_keep_piles);
#else
			m_pLayout->RecalcLayout(pList, create_strips_keep_piles);
#endif
			m_pView->Invalidate();
			m_pLayout->PlaceBox();
		}
		else
		{
			// shouldn't happen because update handler should disable
			// the command if there is already a note on pSrcPhrase
			;
		}
	}

}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the Edit Menu 
///                        is about to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed. The "Move Note Forward" item on the Edit menu is
/// disabled if any of the following conditions is TRUE: the application is showing only
/// the target text, the active pile pointer is NULL, a Note dialog is currently open (must
/// be closed first), the first source phrase of any selection already has a Note.
/// Otherwise, if there is a Note at the active location and there is an eligible source
/// phrase ahead to move to, the menu item is enabled, otherwise the menu item is disabled.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::OnUpdateEditMoveNoteForward(wxUpdateUIEvent& event)
{
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	SPList* pList = m_pApp->m_pSourcePhrases;
	SPList::Node* tgtPos; 
	CSourcePhrase* pTgtSrcPhrase;
	if (gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pNoteDlg != NULL)
	{
		// there already is a note dialog open, 
		// so we can't move one until it is closed
		event.Enable(FALSE);
		return;
	}
	// Protect against idle time update menu handler action at app shutdown 
	// time, when piles no longer exist - we must prevent pile access then
	if (m_pApp->GetLayout()->GetPileList() == NULL ||
		m_pApp->GetLayout()->GetPileList()->IsEmpty())
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_selectionLine != -1)
	{
        // if the first sourcephrase in the selection does not have a note,
        // enable the button, but if it does then disable the button
		CCellList::Node* pos = m_pApp->m_selection.GetFirst();
		while (pos != NULL)
		{
			CPile* pPile = ((CCell*)pos->GetData())->GetPile();
			pos = pos->GetNext();
			CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
			if (pSrcPhrase->m_bHasNote)
			{
                // has a note, so it can be moved forward, provided there is something
                // forward to receive it and it does not already contain a note
				if (pSrcPhrase->m_nSequNumber < m_pApp->GetMaxIndex())
				{
					tgtPos = pList->Item(pSrcPhrase->m_nSequNumber); 
					pTgtSrcPhrase = (CSourcePhrase*)tgtPos->GetData(); // abandon this one
					tgtPos = tgtPos->GetNext();
					pTgtSrcPhrase = (CSourcePhrase*)tgtPos->GetData(); // this is the next
					tgtPos = tgtPos->GetNext();
					if (pTgtSrcPhrase->m_bHasNote)
					{
						// the next one already has a note, so disable the button
						event.Enable(FALSE);
					}
					else
					{
						// there is no note on the next one, so it can be a target instance
						event.Enable(TRUE); // more of the doc is ahead
					}
				}
				else
					event.Enable(FALSE); // at the end of the doc, so nothing is ahead
				return;
			}
			else
			{
				// doesn't have a note, so no move is possible here
				event.Enable(FALSE);
				return;
			}
		}
	}
	else
	{
		// check out if there is a note at the active location
		
		if (m_pApp->m_pTargetBox != NULL && m_pApp->m_pTargetBox->IsShown())
		{
            // enable the button only if there is a note at the active location and there
            // is at least one sourcephrase ahead to form the target one for the move
			CSourcePhrase* pSrcPhrase = m_pApp->m_pActivePile->GetSrcPhrase();
			if (pSrcPhrase->m_bHasNote)
			{
				if (pSrcPhrase->m_nSequNumber < m_pApp->GetMaxIndex())
				{
					// there is an instance ahead
					tgtPos = pList->Item(pSrcPhrase->m_nSequNumber); 
					pTgtSrcPhrase = (CSourcePhrase*)tgtPos->GetData(); // abandon this one
					tgtPos = tgtPos->GetNext();
					pTgtSrcPhrase = (CSourcePhrase*)tgtPos->GetData(); // this is the next
					tgtPos = tgtPos->GetNext();
					if (pTgtSrcPhrase->m_bHasNote)
					{
						// the next one already has a note, so disable the button
						event.Enable(FALSE);
					}
					else
					{
						// there is no note on the next one, so it can be a target instance
						event.Enable(TRUE); // more of the doc is ahead
					}
				}
				else
					event.Enable(FALSE); // at the end of the doc, so nothing is ahead
				return;
			}
			else
			{
				event.Enable(FALSE);
				return;
			}
		}
	}
	event.Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::OnEditMoveNoteBackward(wxCommandEvent& WXUNUSED(event))
{
	// whm added 15Mar12 for read-only mode
	if (m_pApp->m_bReadOnlyAccess)
	{
		::wxBell();
		return;
	}
	
    // Since the Move Note Backward menu item has an accelerator table hot key (CTRL-2 see
    // CMainFrame) and wxWidgets accelerator keys call menu and toolbar handlers even when
    // they are disabled, we must check for a disabled button and return if disabled.
    // On Windows, the accelerator key doesn't appear to call the handler for a disabled
    // menu item, but I'll leave the following code here in case it works differently on
    // other platforms.
	m_pApp->LogUserAction(_T("Initiated OnEditMoveNoteBackward"));
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	wxMenuBar* pMenuBar = pFrame->GetMenuBar();
	wxASSERT(pMenuBar != NULL);
	if (!pMenuBar->IsEnabled(ID_EDIT_MOVE_NOTE_BACKWARD))
	{
		::wxBell();
		m_pApp->LogUserAction(_T("Move note backward menu item disabled"));
		return;
	}
	
	CPile* pPile = NULL;
	SPList* pList = m_pApp->m_pSourcePhrases;
	CSourcePhrase* pSrcPhrase = NULL;
	
	// determine which pSrcPhrase has the note - if unable, return, doing nothing
	int nSequNum = -1;
	CCellList* pCellList;
	CCellList::Node* cpos;
	if (m_pApp->m_selectionLine != -1)
	{
		// we have a selection, the pile we want is that of the selection list's 
		// first element
		pCellList = &m_pApp->m_selection;
		cpos = pCellList->GetFirst();
		pPile = ((CCell*)cpos->GetData())->GetPile();
		if (pPile == NULL)
		{
			// unlikely, so an English message will do
		a:	wxMessageBox(_T(
			"A zero pile pointer was returned, the sourcephrase with the note is indeterminate."),
			_T(""), wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(_T("A zero pile pointer was returned, the sourcephrase with the note is indeterminate."));
			return;
		}
		wxASSERT(pPile != NULL);
		m_pView->RemoveSelection(); // need Invalidate() to be called later on
	}
	else
	{
		// no selection, so just assume the note is on the sourcephrase
		// where the phrase box is
		pPile = m_pApp->m_pActivePile;
		if (pPile == NULL)
		{
			goto a;
		}
	}
	pSrcPhrase = pPile->GetSrcPhrase(); // get the sourcephrase
	nSequNum = pSrcPhrase->m_nSequNumber; // get its sequence number
	wxASSERT(nSequNum >= 0);
	
    // the update handler will enable the menu item only if there was an unopen note stored
    // on the first of any selected sourcephrases, or if no selection, then at the active
    // location, so we can here assume pSrcPhrase has a note - so next we need to remove it
    // (after storing its content), determine what the target sourcephrase is, then
    // re-establish the note there. We do this using a function call, because there may be
    // other situations where we'd want to reestablish the note at some other location
    // other than the immediate previous sourcephrase, doing so under program control (eg.
    // moving the note several sourcephrases back, so that a merger could be done at what
    // would otherwise have been a place where filtered material would be merger-internal
    // which is illegal).
	SPList::Node* tgtPos = pList->Item(nSequNum); 
	wxASSERT(tgtPos != NULL);
	CSourcePhrase* pTgtSrcPhrase = (CSourcePhrase*)tgtPos->GetData(); // abandon this one
	tgtPos = tgtPos->GetPrevious();
	if (tgtPos == NULL)
	{
        // the update handler should disable the button to prevent this possibility, but
        // just in case we'll check for it and return silently if we can't move it backward
		return;
	}
	else
	{
        // the next sourcephrase exists, so put the note there - provided there is not
        // already a note there -- and if there is, then tell the user to first move that
        // note & return without doing anything
		pTgtSrcPhrase = (CSourcePhrase*)tgtPos->GetData();
		tgtPos = tgtPos->GetPrevious();
		wxASSERT(pTgtSrcPhrase);
		if (!pTgtSrcPhrase->m_bHasNote)
		{
			// there is no note there yet
			MoveNote(pSrcPhrase,pTgtSrcPhrase);
			
            // BEW added 19Dec07: establish a selection at the new location in case the
            // user wishes to use accelerator key combination in order to move the note
            // further backward
			if (!gbShowTargetOnly)
			{
                // can do it only if showing source text and nav text whiteboard, (but
                // update handler blocks otherwise, but having this test documents things
                // better so we'll use it, though unnecessary)
				wxASSERT(m_pApp->m_selection.IsEmpty());
				int nNewSequNum = nSequNum - 1;
				CPile* pNewPile = m_pView->GetPile(nNewSequNum);
				if (pNewPile == NULL)
				{
                    // this should not happen, but just in case, we will exit and not
                    // bother with trying to make a selection, as presumably the move has
                    // succeeded, so we'll let the user try again manually
					return;
				}
				else
				{
                    // the pile pointer is known, we can assume it is valid; we'll create
                    // the selection in the cell with index = 0 
					m_pApp->m_selectionLine = gnSelectionLine = 0;
					CCell* pCell = pNewPile->GetCell(0);
					m_pApp->m_selection.Insert(pCell);
					m_pApp->m_pAnchor = pCell;
					m_pApp->m_curDirection = left;
					m_pApp->m_bSelectByArrowKey = FALSE;
					
					// draw the background yellow for the CCell we want shown selected
					wxClientDC aDC(m_pApp->GetMainFrame()->canvas); // get a temporary client 
					// device context for this view window
					aDC.SetBackgroundMode(m_pApp->m_backgroundMode);
					aDC.SetTextBackground(wxColour(255,255,0)); // yellow
					pCell->DrawCell(&aDC, m_pLayout->GetTgtColor());
					pCell->SetSelected(TRUE);
				}
			}
#ifdef _NEW_LAYOUT
			m_pLayout->RecalcLayout(pList, keep_strips_keep_piles);
#else
			m_pLayout->RecalcLayout(pList, create_strips_keep_piles);
#endif
			m_pView->Invalidate();
			m_pLayout->PlaceBox();
		}
		else
		{
            // shouldn't happen because update handler should disable
            // the command if there is already a note on pSrcPhrase
			;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the Edit Menu
///                        is about to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed.
/// The "Move Note Backward" item on the Edit menu is disabled if any of the following
/// conditions is TRUE: the application is showing only the target text, the active pile
/// pointer is NULL, a Note dialog is currently open (must be closed first), the first
/// source phrase of any selection already has a Note. Otherwise, if there is a Note at the
/// active location and there is an eligible source phrase previous to the current location
/// to move to, the menu item is enabled, otherwise the menu item is disabled.
/// BEW 25Feb10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CNotes::OnUpdateEditMoveNoteBackward(wxUpdateUIEvent& event)
{
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	SPList* pList = m_pApp->m_pSourcePhrases;
	SPList::Node* tgtPos;
	CSourcePhrase* pTgtSrcPhrase;
	if (gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pNoteDlg != NULL)
	{
		// there already is a note dialog open, so we can't move one until it is closed
		event.Enable(FALSE);
		return;
	}
	// Protect against idle time update menu handler action at app shutdown 
	// time, when piles no longer exist - we must prevent pile access then
	if (m_pApp->GetLayout()->GetPileList() == NULL ||
		m_pApp->GetLayout()->GetPileList()->IsEmpty())
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_selectionLine != -1)
	{
        // if the first sourcephrase in the selection does not have a note, enable the
        // button, but if it does then disable the button
		CCellList::Node* pos = m_pApp->m_selection.GetFirst();
		while (pos != NULL)
		{
			CPile* pPile = ((CCell*)pos->GetData())->GetPile();
			pos = pos->GetNext();
			CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
			if (pSrcPhrase->m_bHasNote)
			{
                // has a note, so it can be moved backwards, provided there is a
                // sourcephrase earlier to receive it and it does not already contain a
                // note
				if (pSrcPhrase->m_nSequNumber > 0)
				{
					tgtPos = pList->Item(pSrcPhrase->m_nSequNumber); 
					pTgtSrcPhrase = (CSourcePhrase*)tgtPos->GetData(); // abandon this one
					tgtPos = tgtPos->GetPrevious();
					pTgtSrcPhrase = (CSourcePhrase*)tgtPos->GetData(); // the previous one
					tgtPos = tgtPos->GetPrevious();
					if (pTgtSrcPhrase->m_bHasNote)
					{
						// the previous one already has a note, so disable the button
						event.Enable(FALSE);
					}
					else
					{
						// there is no note on the previous one, so it can be a 
						// target instance
						event.Enable(TRUE); // some of the doc is earlier
					}
				}
				else
					event.Enable(FALSE); // at the start of the doc, so nothing is earlier
				return;
			}
			else
			{
				// doesn't have a note, so no move is possible here
				event.Enable(FALSE);
				return;
			}
		}
	}
	else
	{
		// check out if there is a note at the active location
		
		if (m_pApp->m_pTargetBox->GetHandle() != NULL && m_pApp->m_pTargetBox->IsShown())
		{
            // enable the button only if there is a note at the active location and there
            // is at least one sourcephrase earlier to form the target one for the move
			CSourcePhrase* pSrcPhrase = m_pApp->m_pActivePile->GetSrcPhrase();
			if (pSrcPhrase->m_bHasNote)
			{
				if (pSrcPhrase->m_nSequNumber > 0)
				{
					// there is an instance backwards
					tgtPos = pList->Item(pSrcPhrase->m_nSequNumber);
					pTgtSrcPhrase = (CSourcePhrase*)tgtPos->GetData(); // abandon this one
					tgtPos = tgtPos->GetPrevious();
					pTgtSrcPhrase = (CSourcePhrase*)tgtPos->GetData(); // the earlier one
					tgtPos = tgtPos->GetPrevious();
					if (pTgtSrcPhrase->m_bHasNote)
					{
						// the previous one already has a note, so disable the button
						event.Enable(FALSE);
					}
					else
					{
						// there is no note on the previous one, so it can be a 
						// target instance
						event.Enable(TRUE); // some of the doc is earlier
					}
				}
				else
					event.Enable(FALSE); // at the start of the doc, 
				// so nothing is earlier
				return;
			}
			else
			{
				event.Enable(FALSE);
				return;
			}
		}
	}
	event.Enable(FALSE);
}


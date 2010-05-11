/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Placeholder.cpp
/// \author			Erik Brommers
/// \date_created	02 April 2010
/// \date_revised	02 April 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the implementation file for the CPlaceholder class. 
/// The CPlaceholder class contains methods for working with placeholder 
/// elements within the translated text.
/// \derivation		The CPlaceholder class is derived from wxEvtHandler.
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "Placeholder.h"
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

/////////
#include "Adapt_It_Resources.h"
#include "Adapt_It.h"
#include "Adapt_ItView.h"
#include "SourcePhrase.h"
#include "Strip.h"
#include "Pile.h"
#include "Cell.h"
#include "Layout.h"
#include "MainFrm.h"
#include "Adapt_ItCanvas.h"
#include "Adapt_ItDoc.h"
#include "KB.h"
#include "Placeholder.h"
//////////



///////////////////////////////////////////////////////////////////////////////
// External globals
///////////////////////////////////////////////////////////////////////////////
extern int gnOldSequNum;
extern bool gbVerticalEditInProgress;
extern bool gbInhibitLine4StrCall;
extern bool gbIsGlossing;
extern bool gbShowTargetOnly;
extern bool gbDummyAddedTemporarily;
extern EditRecord gEditRecord;
extern wxChar gSFescapechar;
extern const wxChar* filterMkr;			// defined in Adapt_ItDoc

///////////////////////////////////////////////////////////////////////////////
// Event Table
///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CPlaceholder, wxEvtHandler)
EVT_TOOL(ID_BUTTON_REMOVE_NULL_SRCPHRASE, CPlaceholder::OnButtonRemoveNullSrcPhrase)
EVT_UPDATE_UI(ID_BUTTON_REMOVE_NULL_SRCPHRASE, CPlaceholder::OnUpdateButtonRemoveNullSrcPhrase)
EVT_TOOL(ID_BUTTON_NULL_SRC, CPlaceholder::OnButtonNullSrc)
EVT_UPDATE_UI(ID_BUTTON_NULL_SRC, CPlaceholder::OnUpdateButtonNullSrc)
END_EVENT_TABLE()


///////////////////////////////////////////////////////////////////////////////
// Constructors / destructors
///////////////////////////////////////////////////////////////////////////////

CPlaceholder::CPlaceholder()
{
}

CPlaceholder::CPlaceholder(CAdapt_ItApp* app)
{
	wxASSERT(app != NULL);
	m_pApp = app;
	m_pLayout = m_pApp->GetLayout();
	m_pView = m_pApp->GetView();
}

CPlaceholder::~CPlaceholder()
{
	
}


///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

// BEW 18Feb10 updated for doc version 5 support (no changes needed)
void CPlaceholder::InsertNullSrcPhraseBefore() 
{
	// first save old sequ num for active location
	gnOldSequNum = m_pApp->m_nActiveSequNum;
	
    // find the pile preceding which to do the insertion - it will either be preceding the
    // first selected pile, if there is a selection current, or preceding the active
    // location if no selection is current
	CPile* pInsertLocPile;
	CAdapt_ItDoc* pDoc = m_pView->GetDocument();
	wxASSERT(pDoc != NULL);
	int nCount = 1;
	int nSequNum = -1;
	if (m_pApp->m_selectionLine != -1)
	{
		// we have a selection, the pile we want is that of the selection list's 
		// first element
		CCellList* pCellList = &m_pApp->m_selection; 
		CCellList::Node* fpos = pCellList->GetFirst();
		pInsertLocPile = fpos->GetData()->GetPile(); 
		if (pInsertLocPile == NULL)
		{
			wxMessageBox(_T(
							"Sorry, a zero pointer was returned, the insertion cannot be done."),
						 _T(""), wxICON_EXCLAMATION);
			return;
		}
		nSequNum = pInsertLocPile->GetSrcPhrase()->m_nSequNumber;
		wxASSERT(pInsertLocPile != NULL);
		m_pView->RemoveSelection(); // Invalidate will be called in InsertNullSourcePhrase()
	}
	else
	{
		// no selection, so just insert preceding wherever the phraseBox 
		// currently is located
		pInsertLocPile = m_pApp->m_pActivePile;
		if (pInsertLocPile == NULL)
		{
			wxMessageBox(_T(
							"Sorry, a zero pointer was returned, the insertion cannot be done."),
						 _T(""), wxICON_EXCLAMATION);
			return;
		}
		nSequNum = pInsertLocPile->GetSrcPhrase()->m_nSequNumber;
	}
	wxASSERT(nSequNum >= 0);
	
	// check we are not in a retranslation - we can't insert there!
	if(pInsertLocPile->GetSrcPhrase()->m_bRetranslation)
	{
		CPile* pPile = m_pView->GetPrevPile(pInsertLocPile);
		if (pPile == NULL || pPile->GetSrcPhrase()->m_bRetranslation)
		{
			// IDS_NO_INSERT_IN_RETRANS
			wxMessageBox(_(
						   "Sorry, you cannot insert a placeholder within a retranslation. The command has been ignored."),
						 _T(""), wxICON_EXCLAMATION);
			m_pView->RemoveSelection();
			m_pView->Invalidate();
			m_pLayout->PlaceBox();
			return;
		}
	}
	
	// ensure the contents of the phrase box are saved to the KB
	// & make the punctuated target string
	if (m_pApp->m_pTargetBox != NULL && m_pApp->m_pTargetBox->IsShown())
	{
		m_pView->MakeLineFourString(m_pApp->m_pActivePile->GetSrcPhrase(), m_pApp->m_targetPhrase);
		
        // we are about to leave the current phrase box location, so we must try to store
        // what is now in the box, if the relevant flags allow it
		m_pView->RemovePunctuation(pDoc, &m_pApp->m_targetPhrase, from_target_text);
		gbInhibitLine4StrCall = TRUE;
		bool bOK;
		bOK = m_pView->StoreText(m_pApp->m_pKB, m_pApp->m_pActivePile->GetSrcPhrase(), 
						m_pApp->m_targetPhrase);
		gbInhibitLine4StrCall = FALSE;
	}
	
	InsertNullSourcePhrase(pDoc, pInsertLocPile, nCount);
	
	// jump to it (can't use old pile pointers, the recalcLayout call will have 
	// clobbered them)
	CPile* pPile = m_pView->GetPile(nSequNum);
	m_pView->Jump(m_pApp, pPile->GetSrcPhrase());
}

// BEW 18Feb10 updated for doc version 5 support (no changes needed)
void CPlaceholder::InsertNullSrcPhraseAfter() 
{
	// this function is public (called in PhraseBox.cpp)
	int nSequNum;
	int nCount;
	CAdapt_ItDoc* pDoc = m_pView->GetDocument();
	
	// CTRL key is down, so an "insert after" is wanted
	// first save old sequ num for active location
	gnOldSequNum = m_pApp->m_nActiveSequNum;
	
	// find the pile after which to do the insertion - it will either be after the
	// last selected pile if there is a selection current, or after the active location
	// if no selection is current - beware the case when active location is a doc end!
	CPile* pInsertLocPile;
	nCount = 1; // the button or shortcut can only insert one
	nSequNum = -1;
	if (m_pApp->m_selectionLine != -1)
	{
		// we have a selection, the pile we want is that of the selection list's last element
		CCellList* pCellList = &m_pApp->m_selection;
		CCellList::Node* cpos = pCellList->GetLast();
		pInsertLocPile = cpos->GetData()->GetPile(); 
		if (pInsertLocPile == NULL)
		{
			wxMessageBox(_T(
							"Sorry, a zero pointer was returned, the insertion cannot be done."),
						 _T(""), wxICON_EXCLAMATION);
			return;
		}
		nSequNum = pInsertLocPile->GetSrcPhrase()->m_nSequNumber;
		wxASSERT(pInsertLocPile != NULL);
		m_pView->RemoveSelection(); // Invalidate will be called in InsertNullSourcePhrase()
	}
	else
	{
		// no selection, so just insert after wherever the phraseBox currently is located
		pInsertLocPile = m_pApp->m_pActivePile;
		if (pInsertLocPile == NULL)
		{
			wxMessageBox(_T(
							"Sorry, a zero pointer was returned, the insertion cannot be done."),
						 _T(""), wxICON_EXCLAMATION);
			return;
		}
		nSequNum = pInsertLocPile->GetSrcPhrase()->m_nSequNumber;
	}
	wxASSERT(nSequNum >= 0);
	
    // check we are not in a retranslation - we can't insert there! We can be at the end of
    // a retranslation since we are then inserting after it, but if we are not at the end,
    // there will be at least one more retranslation sourcephrase to the right, and so we
    // must check if this is the case & abort the operation if so
	if(pInsertLocPile->GetSrcPhrase()->m_bRetranslation)
	{
		CPile* pPile = m_pView->GetNextPile(pInsertLocPile);
		if (pPile == NULL || pPile->GetSrcPhrase()->m_bRetranslation)
		{
			// IDS_NO_INSERT_IN_RETRANS
			wxMessageBox(_(
						   "Sorry, you cannot insert a placeholder within a retranslation. The command has been ignored."),
						 _T(""), wxICON_EXCLAMATION);
			m_pView->RemoveSelection();
			m_pView->Invalidate();
			m_pLayout->PlaceBox();
			return;
		}
	}
	
	// ensure the contents of the phrase box are saved to the KB
	// & make the punctuated target string
	if (m_pApp->m_pTargetBox != NULL && m_pApp->m_pTargetBox->IsShown())
	{
		m_pView->MakeLineFourString(m_pApp->m_pActivePile->GetSrcPhrase(), m_pApp->m_targetPhrase);
		
        // we are about to leave the current phrase box location, so we must try to store
        // what is now in the box, if the relevant flags allow it
		m_pView->RemovePunctuation(pDoc, &m_pApp->m_targetPhrase, from_target_text);
		gbInhibitLine4StrCall = TRUE;
		bool bOK;
		bOK = m_pView->StoreText(m_pApp->m_pKB, m_pApp->m_pActivePile->GetSrcPhrase(), m_pApp->m_targetPhrase);
		gbInhibitLine4StrCall = FALSE;
	}
	
    // at this point, we need to increment the pInsertLocPile pointer to the next pile, and
    // the nSequNum to it's sequence number, since InsertNullSourcePhrase() always inserts
    // "before" the pInsertLocPile; however, if the selection end, or active location if
    // there is no selection, is at the very end of the document (ie. last sourcephrase),
    // there is no following source phrase instance to insert before. If this is the case,
    // we have to append a dummy sourcephrase at the end of the document, do the insertion,
    // and then remove it again; and we will also have to set (and later clear) the global
    // gbDummyAddedTemporarily because this is used in the function in order to force a
    // leftwards association only (and hence the user does not have to be asked whether to
    // associate right or left, if there is final punctuation)
	SPList* pSrcPhrases = m_pApp->m_pSourcePhrases;
	CSourcePhrase* pDummySrcPhrase = NULL; // whm initialized to NULL
	if (nSequNum == m_pApp->GetMaxIndex())
	{
		// a dummy is temporarily required
		gbDummyAddedTemporarily = TRUE;
		
		// do the append
		pDummySrcPhrase = new CSourcePhrase;
		wxASSERT(pDummySrcPhrase != NULL);
		pDummySrcPhrase->m_srcPhrase = _T("dummy"); // something needed, so a pile width can
		// be computed
		pDummySrcPhrase->m_key = pDummySrcPhrase->m_srcPhrase;
		pDummySrcPhrase->m_nSequNumber = m_pApp->GetMaxIndex() + 1;
		SPList::Node* posTail;
		posTail = pSrcPhrases->Append(pDummySrcPhrase);
		
		// create a partner pile for this dummy CSourcePhrase instance
		pDoc->CreatePartnerPile(pDummySrcPhrase);
		
		// we need a valid layout which includes the new dummy element on its own pile
		m_pApp->m_nActiveSequNum = m_pApp->GetMaxIndex();
#ifdef _NEW_LAYOUT
		m_pLayout->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
		m_pLayout->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
		m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->GetMaxIndex()); // temporary active 
		// location, at the dummy one
		// now we can do the insertion
		pInsertLocPile = m_pApp->m_pActivePile;
		nSequNum = m_pApp->GetMaxIndex();
	}
	else
	{
		// the 'next' pile is not beyond the document's end, so make it the insert
		// location
		CPile* pPile = m_pView->GetNextPile(pInsertLocPile);
		wxASSERT(pPile != NULL);
		pInsertLocPile = pPile;
		m_pApp->m_pActivePile = pInsertLocPile; // ensure it is up to date
		nSequNum++; // make the sequence number agree
	}
	
	InsertNullSourcePhrase(pDoc,pInsertLocPile,nCount,TRUE,FALSE,FALSE); // here, never for
	// Retransln if we inserted a dummy, now get rid of it and clear the global flag
	if (gbDummyAddedTemporarily)
	{
		gbDummyAddedTemporarily = FALSE;
		
		// now remove the dummy element, and make sure memory is not leaked!
		// for refactored code, first remove the partner pile for the dummy, then the dummy
		pDoc->DeletePartnerPile(pDummySrcPhrase);
		// now the dummy CSourcePhrase instance
		delete pDummySrcPhrase->m_pSavedWords;
		pDummySrcPhrase->m_pSavedWords = (SPList*)NULL;
		delete pDummySrcPhrase->m_pMedialMarkers;
		pDummySrcPhrase->m_pMedialMarkers = (wxArrayString*)NULL;
		delete pDummySrcPhrase->m_pMedialPuncts;
		pDummySrcPhrase->m_pMedialPuncts = (wxArrayString*)NULL;
		SPList::Node *pLast = pSrcPhrases->GetLast();
		pSrcPhrases->DeleteNode(pLast);
		delete pDummySrcPhrase;
		pDummySrcPhrase = (CSourcePhrase*)NULL;
		
		// get another valid layout
		m_pApp->m_nActiveSequNum = m_pApp->GetMaxIndex();
#ifdef _NEW_LAYOUT
		m_pLayout->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
		m_pLayout->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
		m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->GetMaxIndex()); // temporarily at the end, 
		// caller will fix
		nSequNum = m_pApp->GetMaxIndex();
	}
	
	// jump to it (can't use old pile pointers, the recalcLayout call 
	// will have clobbered them)
	CPile* pPile = m_pView->GetPile(nSequNum);
	m_pView->Jump(m_pApp, pPile->GetSrcPhrase());
}

// BEW additions 22Jul05 for support of free translations when placeholder insertions are 
// done
// BEW additions 17Feb10 in support of doc version 5 (a previous, or at end of retranslation,
// CSourcePhrase with a non-empty m_endMarkers must move to the final placeholder in the
// retranslation, or in the case of manual placeholder insertion, must move to the
// placeholder if there is a leftward association stipulated when the message box asks the
// user 
void CPlaceholder::InsertNullSourcePhrase(CAdapt_ItDoc* pDoc,
										   CPile* pInsertLocPile,const int nCount,
										   bool bRestoreTargetBox,bool bForRetranslation,
										   bool bInsertBefore)
{
	bool bAssociatingRightwards = FALSE;
	CSourcePhrase* pSrcPhrase = NULL; // whm initialized to NULL
	CSourcePhrase* pPrevSrcPhrase = NULL; // whm initialized to NULL
	CPile* pPrevPile;
	CPile* pPile = pInsertLocPile;
	int nStartingSequNum = pPile->GetSrcPhrase()->m_nSequNumber;
	// whm 2Aug06 added following test to prevent insertion of placeholder 
	// in front of \id marker
	// BEW added !bForRetranslation on 6Aug09, because Bob Buss found the test succeeded
	// when here was the following information at the source text file start:
	// \id MAT English-US: New Living Translation 1996 
	// \ide 65001 - UNICODE(UTF-8)
	// \rem For any ... holder
	// \h MAT
	// \mt Matthew
	// \c 1
	// \s The Record of Jesus' Ancestors
	// where the first non-filtered info was "Matthew" after the stuff in the \id line,
	// and he had selected from English-US as far as the "1996" to make his retranslation
	// and his retranslation was longer than the 5 words he selected. What happened was
	// that the following test (without my addition) succeeds because the CSourcePhrase
	// storing "Matthew" contains the m_markers member with marker \ide in it. The test
	// for finding "\id" then succeeds, and so the bell rings and the needed null source
	// phrases are not inserted - but in the caller the extra words are inserted into the
	// list assuming the null source phrases had been put into the list beforehand,
	// resulting in an error state - that is, a malformed document, but the app doesn't
	// crash  - the extra words in the retranslation wipe out adaptations from Matthew
	// onwards until all the extra words are consumed.
	// Since this test only applies to an attempt by the user to insert a placeholder
	// preceding the very first CSourcePhrase, and that can't happen when doing a
	// retranslation because there have to be selectable CSourcePhrase instances before
	// that, and that can't happen before the one bearing an \id marker, all we need do
	// here is add the additional condition !bForRetranslation and this bug is history
	if (pPile->GetSrcPhrase()->m_markers.Find(_T("\\id")) != -1 && !bForRetranslation)
	{
        // user is attempting to insert placeholder before a \id marker which should not be
        // allowed rather than a message, we'll just beep and return
		::wxBell();
		return;
	}
	SPList* pList = m_pApp->m_pSourcePhrases;
	SPList::Node* insertPos	= pList->Item(nStartingSequNum); // the position before
	// which we will make the insertion

    // BEW added to, 17Feb10, to support doc version 5 -- if the final instance's
    // m_endMarkers member has content, then that content has to be cleared out and
    // transferred to the last of the inserted placeholders - this is true in
    // retranslations, and also for non-retranslation placeholder insertion following a
    // CSourcePhrase with m_endMarkers with content and the association is leftwards (if
    // rightwards, the endmarkers stay put)
	wxString endmarkersToTransfer = _T("");
	wxString emptyStr = _T("");
	bool bTransferEndMarkers = FALSE;
	CSourcePhrase* pOldLastSrcPhrase; // the sourcephrase which lies before the first 
	// inserted ellipsis, we have to check this one for a m_bEndFreeTrans == TRUE flag
	// and move that BOOL value to the end of the insertions
	bool bMoveEndOfFreeTrans = FALSE; // moved outside of if block below
	if (nStartingSequNum > 0) // whm added to prevent assert and unneeded test for 
		// m_bEndFreeTrans when sequ num is zero
	{
		SPList::Node* earlierPos = pList->Item(nStartingSequNum - 1);
		pOldLastSrcPhrase = (CSourcePhrase*)earlierPos->GetData();
		if (pOldLastSrcPhrase->m_bEndFreeTrans)
		{
			pOldLastSrcPhrase->m_bEndFreeTrans = FALSE;
			bMoveEndOfFreeTrans = TRUE;
		}

		if (!pOldLastSrcPhrase->GetEndMarkers().IsEmpty())
		{
			endmarkersToTransfer = pOldLastSrcPhrase->GetEndMarkers();
			if (bForRetranslation)
			{
				bTransferEndMarkers = TRUE;
				pOldLastSrcPhrase->SetEndMarkers(emptyStr);
			}
			else
			{
                // when it is not for padding of a long retranslation, we will defer the
                // decision about removal of the marker content and setting of the flag
                // until further below, when the code deals with inserting a placeholder by
                // user-choice within the document - for that scenario we have to consider
                // whether we are associating leftwards or rightwards, and rightwards
                // associations don't require the moving of the endmarkers
				;
			}
		}
	}
	
	// get the sequ num for the insertion location 
	// (it could be quite diff from active sequ num)
	CSourcePhrase* pSrcPhraseInsLoc = pInsertLocPile->GetSrcPhrase();
	int	nSequNumInsLoc = pSrcPhraseInsLoc->m_nSequNumber;
	wxASSERT(nSequNumInsLoc >= 0 && nSequNumInsLoc <= m_pApp->GetMaxIndex());
	
	wxASSERT(insertPos != NULL);
	int nActiveSequNum = m_pApp->m_nActiveSequNum; // save, so we can restore later on, 
	// since the call to RecalcLayout() will clobber pointers
	
    // we may be inserting in the context of a footnote, either within it, or next to it -
    // so we will need to do some checks to determine whether or not we will have to set
    // the TextType as 'footnote'. A sufficient condition for being 'within' a footnote is
    // m_bFootnote = FALSE but m_curTextType == footnote; the insertion before the start of
    // a footnote, or after the end of a footnote, will be dealt with further down in the
    // current function.
	if (!bForRetranslation)
	{
        // retranslation case is taken care of by handlers OnButtonRetranslation and
        // OnButtonEditRetranslation, so only the normal null srcphrase insertion needs to
        // be considered here
		if (pSrcPhraseInsLoc->m_curTextType == footnote && 
			pSrcPhraseInsLoc->m_bFootnote == FALSE)
			m_pApp->GetRetranslation()->SetIsInsertingWithinFootnote(TRUE);
	}
	
    // create the needed null source phrases and insert them in the list; preserve pointers
    // to the first and last for use below
	CSourcePhrase* pFirstOne = NULL; // whm initialized to NULL
	CSourcePhrase* pLastOne = NULL; // whm initialized to NULL
	for (int i = 0; i<nCount; i++)
	{
		CSourcePhrase* pSrcPhrasePH = new CSourcePhrase; // PH means 'PlaceHolder'
		if (i == 0)
		{
			pFirstOne = pSrcPhrasePH;
			pSrcPhrase = pSrcPhrasePH;
		}
		if (i == nCount-1)
			pLastOne = pSrcPhrasePH;
		pSrcPhrasePH->m_bNullSourcePhrase = TRUE;
		pSrcPhrasePH->m_srcPhrase = _T("...");
		pSrcPhrasePH->m_key = _T("...");
		pSrcPhrasePH->m_nSequNumber = nStartingSequNum + i; // ensures the
		// UpdateSequNumbers() call works
		if (bForRetranslation)
		{
            // if we are calling the function as part of rendering a retranslation, then we
            // will want to set the appropriate flags for each of the null source phrases
			pSrcPhrasePH->m_bRetranslation = TRUE;
			pSrcPhrasePH->m_bNotInKB = TRUE;
			
            // BEW added 22Jul05 for support of free translations. We assume placeholders
            // within a free translation section in a retranslation belong to the free
            // translation section - provided that section exists at the last sourcephrase
            // instance before the first placeholder, and if the last sourcephrase before
            // the first placeholder was the end of the free translation section, then we
            // move the end of the section to the last placeholder as well
			if (pSrcPhraseInsLoc->m_bHasFreeTrans)
				pSrcPhrasePH->m_bHasFreeTrans = TRUE; // handles the 'within' case
			if ((i == nCount - 1) && bMoveEndOfFreeTrans)
			{
                // move the end of the free translation to this last one (flag on old
                // location is already cleared above in anticipation of this)
				wxASSERT(pLastOne);
				pLastOne->m_bEndFreeTrans = TRUE;
			}

			if ((i == nCount - 1) && bTransferEndMarkers)
			{
                // move the endMarkers to this last one (old m_endMarkers
                // location is already cleared above in anticipation of this)
				wxASSERT(pLastOne);
				pLastOne->SetEndMarkers(endmarkersToTransfer);
			}
		} // end of TRUE block for test:  if (bForRetranslation)
		
        // set the footnote TextType if the flag is TRUE; the flag can be set TRUE within
        // the handlers for retranslation, edit of a retranslation, edit of source text,
        // and in the InsertNullSourcePhrase function itself. We have to get the type
        // right, because the user might output interlinear RTF with footnote suppression
        // wanted, so we have to ensure that these null source phrases have the footnote
        // TextType set so that the suppression will work properly
		if (m_pApp->GetRetranslation()->GetIsInsertingWithinFootnote())
		{
			pSrcPhrasePH->m_curTextType = footnote;
			if (!pSrcPhrasePH->m_bRetranslation)
				pSrcPhrasePH->m_bSpecialText = TRUE; // want it to have special text colour
		}
		
		pList->Insert(insertPos,pSrcPhrasePH);
		
		// BEW added 13Mar09 for refactored layout
		pDoc->CreatePartnerPile(pSrcPhrasePH);
	}
	
	// fix the sequ num for the old insert location's source phrase
	nSequNumInsLoc += nCount;
	
    // calculate the new active sequ number - it could be anywhere, but all we need to know
    // is whether or not the insertion was done preceding the former active sequ number's
    // location
	if (nStartingSequNum <= nActiveSequNum)
		m_pApp->m_nActiveSequNum = nActiveSequNum + nCount;
	
	// update the sequence numbers, starting from the first one inserted
	m_pView->UpdateSequNumbers(nStartingSequNum);
	
    // we must check if there is preceding punctuation on the following source phrase, or a
    // marker; or following punctuation on the preceding source phrase. If one of these
    // conditions (or several conditions) obtain, then we must ask the user whether the
    // placeholder associates with the end of the previous one, or with the following one;
    // and do the appropriate transfer of punctuation and or marker (ie. a non-endMarker)
    // to the first placeholder (if association is to the right), or the last placeholder
    // (if the association is to the left) - but all of this only provided we are not doing
    // rendering of a retranslation. If inserting before the first pile, we don't have to
    // worry about any preceding context.
    // From 17Feb10 we have to consider association direction when there is also a non-empty
	// m_endMarkers member preceding the inserted placeholder; if association is to the
	// left, then we must move the content on to the inserted placeholder; if to the
	// right, then we can leave it untouched; a copy of the endMarkers on the preceding
	// CSourcePhrase has already been obtained above, it is in the local variable
	// endmarkersToTransfer (as of docVersion = 5)
	bool bAssociationRequired = FALSE; // it will be set true if the message box for 
	// associating to the left or right is required at punctuation, or a
	// preceding non-empty m_endMarkers member (the latter for docVersion = 5 or above)
	bool bPreviousFollPunct = FALSE;
	bool bFollowingPrecPunct = FALSE;
	bool bFollowingMarkers = FALSE;
	bool bEndMarkersPrecede = FALSE; // set true when preceding srcPhrase has endmarkers
	int nPrevSequNum = nStartingSequNum - 1; // remember, this could be -ve
	if (!bForRetranslation)
	{
		if (nPrevSequNum != -1)
		{
			pPrevPile = m_pView->GetPrevPile(pInsertLocPile); // old pointers are still valid
			wxASSERT(pPrevPile != NULL);
			pPrevSrcPhrase = pPrevPile->GetSrcPhrase();
			if (!pPrevSrcPhrase->m_follPunct.IsEmpty())
				bPreviousFollPunct = TRUE;
			if (!pPrevSrcPhrase->GetEndMarkers().IsEmpty())
				bEndMarkersPrecede = TRUE;
		}
		else
		{
			// make sure these pointers are null if we are inserting at the doc beginning
			pPrevPile = NULL;
			pPrevSrcPhrase = NULL;
		}
		// now check the following source phrase for any preceding punct or a marker
		if (!pSrcPhraseInsLoc->m_precPunct.IsEmpty())
			bFollowingPrecPunct = TRUE;
		if (!pSrcPhraseInsLoc->m_markers.IsEmpty())
		{
            // BEW added 23Jul05, so that a marker which has TextType none does not trigger
            // the message box, and if the insertion is before its endmarker then the
            // endmarker gets moved to the placeholder (making the insertion be 'outside'
            // the text delimited by the marker & endmarker pair)
			wxString markersStr = pSrcPhraseInsLoc->m_markers;
			int curPos = markersStr.Find(filterMkr);
			if (curPos >= 0)
			{
				// it's a \~FILTER marker, so not one of the TextType == none ones
				bFollowingMarkers = TRUE;
			}
			else
			{
				// check out what it actually is
				curPos = markersStr.Find(gSFescapechar);
				if (curPos == -1)
				{
					// no marker present, so don't set the flag
					;
				}
				else
				{
                    // wx version note: Since we require a read-only buffer we use GetData()
                    // which just returns a const wxChar* to the data in the string.
					const wxChar* pBuff = markersStr.GetData(); 
					wxChar* pBufStart = (wxChar*)pBuff;
					wxChar* pEnd;
					pEnd = pBufStart + markersStr.Length(); // whm added
					wxASSERT(*pEnd == _T('\0')); // whm added
					wxChar* ptr = pBufStart + curPos;
					wxASSERT(*ptr == gSFescapechar);
					wxString bareMkr = pDoc->GetBareMarkerForLookup(ptr);
					wxString wholeMkrLessBackslash = pDoc->GetMarkerWithoutBackslash(ptr);
					USFMAnalysis* pAnalysis = pDoc->LookupSFM(ptr);
					if (pAnalysis == NULL)
					{
						// its an unknown marker, so one which should trigger the message box
						bFollowingMarkers = TRUE;
					}
					else
					{
						// it's a known marker

						// for docVersion = 5, we'll allow any startmarker, including
						// those with textType = none, to be fronted to the placeholder
						bFollowingMarkers = TRUE;
					}
				}
			}
		}
		
		// if one of the flags is true, ask the user for the direction of association
		if (bFollowingMarkers || bFollowingPrecPunct || bPreviousFollPunct || bEndMarkersPrecede)
		{
			// association leftwards or rightwards will be required, so set the flag
			// for this
			bAssociationRequired = TRUE;
			
			// bleed off the case when we are inserting before a temporary dummy srcphrase,
			// since in this situation association always can only be leftwards
			if (!bInsertBefore && gbDummyAddedTemporarily)
				goto a;
			
			// any other situation, we need to let the user make the choice
			// IDS_PUNCT_OR_MARKERS
			if (wxMessageBox(_(
							   "Adapt It does not know whether the inserted placeholder is the end of the preceding text, or the beginning of what follows. Is it the start of what follows?"),
							 _T(""),wxYES_NO) == wxYES)
			{
				bAssociatingRightwards = TRUE;
				
				// the association is to the text which follows, so transfer from there
				// to the first in the list - but not if the first sourcephrase of a
				// retranslation follows, if it does, then silently ignore user's choice
				if (bFollowingMarkers)
				{
					if (pSrcPhraseInsLoc->m_bBeginRetranslation)
						// can't right associate into a retranslation, so skip the block
						goto m;
					wxASSERT(pFirstOne != NULL); // whm added
					pFirstOne->m_markers = pSrcPhraseInsLoc->m_markers; // transfer markers
					pSrcPhraseInsLoc->m_markers.Empty();
					
					// right association to the beginning of a footnote makes the insertion
					// also part of the footnote, so deal with this possibility
					if (pSrcPhraseInsLoc->m_curTextType == 
						footnote && pSrcPhraseInsLoc->m_bFootnote)
					{
						pFirstOne->m_curTextType = footnote;
						// note m_bFootnote flag is handled in next block rather than here
					}
					
					
					// have to also copy various members, such as m_inform, so navigation
					// text works right; but we don't want to copy everything - for
					// instance, we don't want to incorporate it into a retranslation; so
					// just get the essentials
					pFirstOne->m_inform = pSrcPhraseInsLoc->m_inform;
					pFirstOne->m_chapterVerse = pSrcPhraseInsLoc->m_chapterVerse;
					pFirstOne->m_bVerse = pSrcPhraseInsLoc->m_bVerse;
					pFirstOne->m_bParagraph = pSrcPhraseInsLoc->m_bParagraph;
					pFirstOne->m_bChapter = pSrcPhraseInsLoc->m_bChapter;
					pFirstOne->m_bSpecialText = pSrcPhraseInsLoc->m_bSpecialText;
					pFirstOne->m_bFootnote = pSrcPhraseInsLoc->m_bFootnote;
					pFirstOne->m_bFirstOfType = pSrcPhraseInsLoc->m_bFirstOfType;
					pFirstOne->m_curTextType = pSrcPhraseInsLoc->m_curTextType;
					
					// copying the m_markers member means we must transfer the flag values
					// for the 3 booleans which could be there due to a note and/or free
					// translation
					pFirstOne->m_bHasNote = pSrcPhraseInsLoc->m_bHasNote;
					pFirstOne->m_bHasFreeTrans = pSrcPhraseInsLoc->m_bHasFreeTrans;
					pFirstOne->m_bStartFreeTrans = pSrcPhraseInsLoc->m_bStartFreeTrans;
					// if we inserted before the end of a free translation, the
					// m_bEndFreeTrans boolean does not move
					pSrcPhraseInsLoc->m_bHasNote = FALSE;
					pSrcPhraseInsLoc->m_bStartFreeTrans = FALSE;
					
					// clear the others which were moved
					pSrcPhraseInsLoc->m_inform.Empty();
					pSrcPhraseInsLoc->m_chapterVerse.Empty();
					pSrcPhraseInsLoc->m_bFirstOfType = FALSE;
					pSrcPhraseInsLoc->m_bVerse = FALSE;
					pSrcPhraseInsLoc->m_bParagraph = FALSE;
					pSrcPhraseInsLoc->m_bChapter = FALSE;
					pSrcPhraseInsLoc->m_bFootnote = FALSE;
					pSrcPhraseInsLoc->m_bFootnote = FALSE;
				}
				if (bFollowingPrecPunct)
				{
					pFirstOne->m_precPunct = pSrcPhraseInsLoc->m_precPunct; // transfer 
					// the preceding punctuation
					pSrcPhraseInsLoc->m_precPunct.Empty();
					
					// do an adjustment of the m_targetStr member, simplest solution is to
					// make it same as the m_adaption member
					pSrcPhraseInsLoc->m_targetStr = pSrcPhraseInsLoc->m_adaption;
					pFirstOne->m_bFirstOfType = pSrcPhraseInsLoc->m_bFirstOfType;
				}
			}
			else
			{
				// the association is to the text which precedes, so transfer from there
				// to the last in the list
a:				bAssociatingRightwards = FALSE;
				if (bEndMarkersPrecede)
				{
					// these have to be moved
					pPrevSrcPhrase->SetEndMarkers(emptyStr);
					pSrcPhrase->SetEndMarkers(endmarkersToTransfer);
					goto a;
				}
				if (bPreviousFollPunct)
				{
					pLastOne->m_follPunct = pPrevSrcPhrase->m_follPunct; // transfer 
					// the following punctuation
					pPrevSrcPhrase->m_follPunct.Empty();
					
					// left association when the text to the left is a footnote makes the
					// inserted text part of the footnote; so get the TextType set
					// correctly
					if (pPrevSrcPhrase->m_curTextType == footnote)
					{
						pLastOne->m_bSpecialText = TRUE; // want it to have special text colour
						pLastOne->m_curTextType = footnote;
						// note: m_bFootnoteEnd is dealt with below
					}
					
					// do an adjustment of the m_targetStr member, simplest solution is to
					// make it same as the m_adaption member; then transfer the other
					// member's values which are pertinent to the leftwards association
					pPrevSrcPhrase->m_targetStr = pPrevSrcPhrase->m_adaption;
					pLastOne->m_bFootnoteEnd = pPrevSrcPhrase->m_bFootnoteEnd;
					pPrevSrcPhrase->m_bFootnoteEnd = FALSE;
					pLastOne->m_bBoundary = pPrevSrcPhrase->m_bBoundary;
					pPrevSrcPhrase->m_bBoundary = FALSE;
				}
			}
		}
	} // end of TRUE block for test: if (!bForRetranslation)
	else 
	{
        // we are inserting to pad out a retranslation, so if the last of the selected
        // source phrases has following punctuation, we need to move it to the last
        // placeholder inserted (note, the case of moving free-translation-supporting BOOL
        // values is done above, as also is the moving of the content of a final non-empty
        // m_endMarkers member to the last placeholder)
		CSourcePhrase* pPrevSrcPhrase2 = NULL;	// whm initialized to NULL
		if (nPrevSequNum != -1)
		{
			CPile* pPrevPile = m_pView->GetPrevPile(pInsertLocPile); // old pointers are still valid
			wxASSERT(pPrevPile != NULL);
			pPrevSrcPhrase2 = pPrevPile->GetSrcPhrase();
			if (!pPrevSrcPhrase2->m_follPunct.IsEmpty())
				bPreviousFollPunct = TRUE;
		}
		
		if (bPreviousFollPunct)
		{
			// the association is to the text which precedes, so transfer from there
			// to the last in the list
			wxASSERT(pPrevSrcPhrase2 != NULL); // whm added
			pLastOne->m_follPunct = pPrevSrcPhrase2->m_follPunct; // transfer following punct
			pPrevSrcPhrase2->m_follPunct.Empty();
		}
	}
	
    // handle any adjustments required because the insertion was done where there is one or
    // more free translation sections defined in the viscinity of the inserted
    // sourcephrase.
	if (!bForRetranslation)
	{
        // we've done the retranslation placeholder additions case in a block previously,
        // so here we are interested in the single placeholder inserted not in any
        // retranslation
		if (pPrevSrcPhrase == NULL)
		{
            // the insertion was at the start of the document, so we must assume a
            // rightwards association for the inserted sourcephrase
			if (pSrcPhraseInsLoc->m_bStartFreeTrans)
			{
				pSrcPhraseInsLoc->m_bStartFreeTrans = FALSE;
				pSrcPhrase->m_bStartFreeTrans = TRUE;
				pSrcPhrase->m_bHasFreeTrans = TRUE;
				// we assume punctuation has already been handled above, if necessary
			}
		}
		else
		{
            // the insertion was done not at the doc beginning, and it may have been done
            // before a temporary sourcephrase added to the doc end until the insertion has
            // been completed, but typically it is some general location in the doc - and
            // what we do will typically depend on whether there was associating left or
            // right, or no associating at all (which means the insertion was done where
            // punctuation was absent immediately before or after) -- but these potential
            // locations are independent (not completely, but enough to complicate matters)
            // of where free translation sections begin and end, so there are a couple of
            // dozen possibilities to be considered for the three consecutive sourcephrase
            // pointers pPrevSrcPhrase, pSrcPhrase, and pSrcPhraseInsLoc - pSrcPhrase being
            // the placeholder which was just inserted.
			if (pPrevSrcPhrase->m_bHasFreeTrans)
			{
				// the previous sourcephrase has a free translation section defined on it
				if (pPrevSrcPhrase->m_bStartFreeTrans)
				{
					// the previous sourcephrase is where a section of free translation starts
					if (pPrevSrcPhrase->m_bEndFreeTrans)
					{
                        // the previous sourcephrase is both the start and end of a free
                        // translation section (ie. a rare short section with one
                        // sourcephrase in it)
						if (pSrcPhraseInsLoc->m_bHasFreeTrans)
						{
                            // the sourcephrase following the placeholder also has a
                            // different free translation section defined on it - what
                            // happens depends on whether there was a left or right
                            // association, or none
							if (pSrcPhraseInsLoc->m_bStartFreeTrans)
							{
                                // its the start of a new section - which we know must be
                                // the case anyway 
                                if (bAssociationRequired && bAssociatingRightwards)
								{
                                    // we have to make the placeholder the new start of the
                                    // section on the right m_markers should have been
                                    // moved already, so just do the flags
									pSrcPhraseInsLoc->m_bStartFreeTrans = FALSE;
									pSrcPhrase->m_bStartFreeTrans = TRUE;
									pSrcPhrase->m_bHasFreeTrans = TRUE;
								}
								else if (bAssociationRequired && !bAssociatingRightwards)
								{
									// we have to make the placeholder the end of the section
									// on the left
									pPrevSrcPhrase->m_bEndFreeTrans = FALSE;
									pSrcPhrase->m_bEndFreeTrans = TRUE;
									pSrcPhrase->m_bHasFreeTrans = TRUE;
								}
								else if (!bAssociationRequired)
								{
                                    // no punctuation in the context, so the user was not
                                    // asked for an association choice - so we must do so
                                    // now 
                                    // IDS_ASSOC_WITH_FREE_TRANS
									if (wxMessageBox(_(
													   "Do you want the inserted placeholder to be considered as belonging to the free translation section which begins immediately following it?")
													 ,_T(""), wxYES_NO) == wxYES)
									{
                                        // user wants to associate it to the right - make
                                        // the placeholder the new start location, and move
                                        // m_markers to it since it won't have been done
                                        // previously
										pSrcPhraseInsLoc->m_bStartFreeTrans = FALSE;
										pSrcPhrase->m_bStartFreeTrans = TRUE;
										pSrcPhrase->m_bHasFreeTrans = TRUE;
										pSrcPhrase->m_markers = pSrcPhraseInsLoc->m_markers;
										pSrcPhraseInsLoc->m_markers.Empty();
									}
									else
									{
										// user wants to associate it to the left
										pPrevSrcPhrase->m_bEndFreeTrans = FALSE;
										pSrcPhrase->m_bEndFreeTrans = TRUE;
										pSrcPhrase->m_bHasFreeTrans = TRUE;
									}
								}
							}
							else
							{
                                // impossible situation (pPrevSrcPhrase is the end of an
                                // earlier free translation section, so pSrcPhraseInsLoc
                                // must be the starting one of a new section if it is in a
                                // free translation section - so do nothing
								;
							}
						}
					}
					else
					{
                        // the previous sourcephrase starts a free translation section, and
                        // because it is not also the end of the section, the section must
                        // extend past the inserted placeholder - so the placeholder is
                        // within the section, so we only need set one flag
						pSrcPhrase->m_bHasFreeTrans = TRUE;
					}
				}
				else if (pPrevSrcPhrase->m_bEndFreeTrans)
				{
                    // the previous sourcephrase does not start a free translation section,
                    // but it is the end of a section
					if (pSrcPhraseInsLoc->m_bHasFreeTrans)
					{
                        // the sourcephrase following the inserted placeholder is involved
                        // in a free translation section - it has to be the start of a new
                        // section, or the only one in a new section
						if (pSrcPhraseInsLoc->m_bStartFreeTrans)
						{
                            // the sourcephrase following the inserted placeholder is the
                            // start of a new free translation section - what we do now
                            // depends on whether or not there was an association to left
                            // or right due to punctuation, or no association was done
							if (bAssociationRequired && bAssociatingRightwards)
							{
                                // associate to the right, so move the right section's
                                // start to the inserted placeholder, m_markers should
                                // already have been moved
								pSrcPhraseInsLoc->m_bStartFreeTrans = FALSE;
								pSrcPhrase->m_bStartFreeTrans = TRUE;
								pSrcPhrase->m_bHasFreeTrans = TRUE;
							}
							else if (bAssociationRequired && !bAssociatingRightwards)
							{
                                // associate to the left, so move the end of the previous
                                // section to the inserted placeholder
								pPrevSrcPhrase->m_bEndFreeTrans = FALSE;
								pSrcPhrase->m_bEndFreeTrans = TRUE;
								pSrcPhrase->m_bHasFreeTrans = TRUE;
							}
							else if (!bAssociationRequired)
							{
                                // no association is defined, so we must ask the user for
                                // what to do 
                                // IDS_ASSOC_WITH_FREE_TRANS
								if (wxMessageBox(_(
												   "Do you want the inserted placeholder to be considered as belonging to the free translation section which begins immediately following it?"),
												 _T(""),wxYES_NO) == wxYES)
								{
									// associate rightwards, which means we'll need to move
									// m_markers too
									pSrcPhraseInsLoc->m_bStartFreeTrans = FALSE;
									pSrcPhrase->m_bStartFreeTrans = TRUE;
									pSrcPhrase->m_bHasFreeTrans = TRUE;
									pSrcPhrase->m_markers = pSrcPhraseInsLoc->m_markers;
									pSrcPhraseInsLoc->m_markers.Empty();
								}
								else
								{
									// associate leftwards
									pPrevSrcPhrase->m_bEndFreeTrans = FALSE;
									pSrcPhrase->m_bEndFreeTrans = TRUE;
									pSrcPhrase->m_bHasFreeTrans = TRUE;
								}
							}
						}
						else
						{
							// impossible situation
							; // do nothing
						}
					}
					else
					{
                        // the sourcephrase following the inserted placeholder has no free
                        // translation defined on it - so we need consider only leftwards
                        // associations or no association
						if (bAssociationRequired && !bAssociatingRightwards)
						{
                            // leftwards association was already requested, so make the
                            // placeholder the new end of the previous section of free
                            // translation
							pPrevSrcPhrase->m_bEndFreeTrans = FALSE;
							pSrcPhrase->m_bEndFreeTrans = TRUE;
							pSrcPhrase->m_bHasFreeTrans = TRUE;
						}
						else if (bAssociationRequired && bAssociatingRightwards)
						{
							// nothing to be done, there is no free translation to the
							// immediate right
							;
						}
						else if (!bAssociationRequired)
						{
							// no association known as yet, so ask the user
							// IDS_ASSOC_LEFT_FREE_TRANS
							if (wxMessageBox(_(
											   "Do you want the inserted placeholder to be considered as belonging to the free translation section which immediately precedes it?"),
											 _T(""),wxYES_NO) == wxYES)
							{
                                // user wants the inserted placeholder to be associated
                                // with the free translation lying to the left - so make
                                // the placeholder be the new end of the section
								pPrevSrcPhrase->m_bEndFreeTrans = FALSE;
								pSrcPhrase->m_bEndFreeTrans = TRUE;
								pSrcPhrase->m_bHasFreeTrans = TRUE;
							}
							else
							{
                                // there is no association, and since no free translation
                                // lies to the immediate right of the placeholder, we don't
                                // have to do anything
								;
							}
						}
					}
				}
				else
				{
                    // the previous sourcephrase neither starts nor ends a free translation
                    // section, so it is within a free translation section - we just need
                    // to set the one flag
					pSrcPhrase->m_bHasFreeTrans = TRUE;
				}
			}
			else
			{
				// pPrevSrcPhrase has no free translation defined on it
				if (pSrcPhraseInsLoc->m_bHasFreeTrans)
				{
                    // the only possibilities are that pSrcPhraseInsLoc is the first or
                    // only free translation in a section lying to the right of the
                    // placeholder. What we do will depend on the associativity, which may
                    // be leftwards, rightwards, or none. (A placeholder inserted before a
                    // temporary sourcephrase at the document end always associates
                    // leftwards. bAssociatingRightwards == FALSE handles that case
                    // automatically.)
					if (pSrcPhraseInsLoc->m_bStartFreeTrans)
					{
						// its the start of a free translation section
						if (bAssociationRequired && !bAssociatingRightwards)
						{
							// leftwards association - so nothing is to be done
							;
						}
						else if (bAssociationRequired && bAssociatingRightwards)
						{
							// rightwards association - we must move the start to the 
							// placeholder
							pSrcPhraseInsLoc->m_bStartFreeTrans = FALSE;
							pSrcPhrase->m_bStartFreeTrans = TRUE;
							pSrcPhrase->m_bHasFreeTrans = TRUE;
							// punctuation should have been handled already, and 
							// also m_markers
						}
						else if (!bAssociationRequired)
						{
                            // no association in either direction (ie. insertion not at
                            // punctuation) so we have no user action to guide us - so we
                            // must ask the user for guidance
							// IDS_ASSOC_WITH_FREE_TRANS
							if (wxMessageBox(_(
											   "Do you want the inserted placeholder to be considered as belonging to the free translation section which begins immediately following it?"),
											 _T(""),wxYES_NO) == wxYES)
							{
                                // rightwards association - move the start to the
                                // placeholder, but in this case no m_markers movement will
                                // have been done, so we must move that as well
								pSrcPhraseInsLoc->m_bStartFreeTrans = FALSE;
								pSrcPhrase->m_bStartFreeTrans = TRUE;
								pSrcPhrase->m_bHasFreeTrans = TRUE;
								pSrcPhrase->m_markers = pSrcPhraseInsLoc->m_markers;
								pSrcPhraseInsLoc->m_markers.Empty();
							}
							else
							{
								// leftwards association, so do nothing
								;
							}
						}
					}
					else
					{
                        // impossible situation (a free trans section starting without
                        // m_bStartFreeTrans having the value TRUE)
						;
					}
					
				}
				else
				{
                    // pSrcPhraseInsLoc has no free translation defined on it, so nothing
                    // needs to be done to the default flag values (all 3 FALSE) for free
                    // translation support in pSrcPhrase
					;
				}
			}
		}
	}
	
	// recalculate the layout
#ifdef _NEW_LAYOUT
m:	m_pLayout->RecalcLayout(pList, keep_strips_keep_piles);
#else
m:	m_pLayout->RecalcLayout(pList, create_strips_keep_piles);
#endif
	m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);
	wxASSERT(m_pApp->m_pActivePile);
	
	// don't draw the layout and the phrase box when this function is called
	// as part of a larger inclusive procedure
	if (bRestoreTargetBox)
	{
		// restore focus, and selection if any
		m_pApp->m_pTargetBox->SetFocus();
		if (m_pApp->m_nStartChar != m_pApp->m_nEndChar)
		{
			m_pApp->m_pTargetBox->SetSelection(m_pApp->m_nStartChar, m_pApp->m_nEndChar); 
		}
		
		// scroll into view, just in case a lot were inserted
		m_pApp->GetMainFrame()->canvas->ScrollIntoView(m_pApp->m_nActiveSequNum);
		
		m_pView->Invalidate();
		m_pLayout->PlaceBox();
	}
	m_pApp->GetRetranslation()->SetIsInsertingWithinFootnote(FALSE);
}

// returns a pointer to the inserted single null source phrase which was inserted
CSourcePhrase* CPlaceholder::ReDoInsertNullSrcPhrase(SPList* pList,SPList::Node*& insertPos,
													  bool bForRetranslation)
{
	wxASSERT(insertPos != NULL);
	wxASSERT(pList);
	
	CSourcePhrase* pSrcPhr = (CSourcePhrase*)insertPos->GetData();
	wxASSERT(pSrcPhr);
	int nCurSequNum = pSrcPhr->m_nSequNumber; // this will be used for 
	// the UpdateSequNumbers() call
	// create a new null source phrase
	CSourcePhrase* pSrcPhrase = new CSourcePhrase; // this should never fail, it's small
	wxASSERT(pSrcPhrase != NULL);
	pSrcPhrase->m_bNullSourcePhrase = TRUE;
	pSrcPhrase->m_srcPhrase = _T("...");
	pSrcPhrase->m_key = _T("...");
	pSrcPhrase->m_nSequNumber = nCurSequNum; // ensures the UpdateSequNumbers() call works
	
	if (bForRetranslation)
	{
        // if we are calling the function as part of re-rendering a retranslation, then we
        // will want to set the appropriate flags for this situation
		pSrcPhrase->m_bRetranslation = TRUE;
		pSrcPhrase->m_bNotInKB = TRUE;
	}
	
    // now insert it in the list, and update the end index (the latter is probably not
    // needed here) Note: wxList::Insert places the item before the given item and the
    // inserted item then has the insertPos node position.
	pList->Insert(insertPos,pSrcPhrase); 
	
	// update the sequence numbers starting from the newly inserted null source phrase
	m_pView->UpdateSequNumbers(nCurSequNum);
	
    // do any copying to m_inform, and other source phrase members in the caller, so just
    // return a pointer to the new null source phrase
	return pSrcPhrase;
}

// BEW 18Feb10 updated for doc version 5 support (added code to restore endmarkers that
// were moved to the placeholder when it was inserted, ie. moved from the preceding
// CSourcePhrase instance)
void CPlaceholder::RemoveNullSourcePhrase(CPile* pRemoveLocPile,const int nCount)
{
	// while this function can handle nCount > 1, in actual fact we use it for creating
	// (manually) only a single placeholder, and so nCount is always 1 on entry
	CPile* pPile			= pRemoveLocPile;
	int nStartingSequNum	= pPile->GetSrcPhrase()->m_nSequNumber;
	SPList* pList			= m_pApp->m_pSourcePhrases;
	SPList::Node* removePos = pList->Item(nStartingSequNum); // the position at
	// which we will do the removal
	SPList::Node* savePos = removePos; // we will alter removePos & need to restore it
	wxASSERT(removePos != NULL);
	int nActiveSequNum = m_pApp->m_nActiveSequNum; // save, so we can restore later on, 
	// since the call to RecalcLayout will clobber some pointers
	
    // we may be removing the m_pActivePile, so get parameters useful for setting up a
    // temporary active pile for the RecalcLayout() call below
	int nRemovedPileIndex = pRemoveLocPile->GetSrcPhrase()->m_nSequNumber;
	
    // get the preceding source phrase, if it exists, whether null or not - we may have to
    // transfer punctuation to it
	CSourcePhrase* pPrevSrcPhrase = NULL;
	if (nStartingSequNum > 0)
	{
		// there is a preceding one, so get it
		CPile* pPile = m_pView->GetPrevPile(pRemoveLocPile);
		wxASSERT(pPile != NULL);
		pPrevSrcPhrase = pPile->GetSrcPhrase();
		wxASSERT(pPrevSrcPhrase != NULL);
	}
	
	// ensure that there are nCount null source phrases which can be removed from this location
	int count = 0;
	CSourcePhrase* pFirstOne = NULL; // whm initialized to NULL
	CSourcePhrase* pLastOne = NULL; // whm initialized to NULL
	while( removePos != 0 && count < nCount)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)removePos->GetData();
		removePos = removePos->GetNext();
		wxASSERT(pSrcPhrase != NULL);
		if (count == 0)
			pFirstOne = pSrcPhrase;
		pLastOne = pSrcPhrase;
		count++;
		if (!pSrcPhrase->m_bNullSourcePhrase)
		{
			//IDS_TOO_MANY_NULL_SRCPHRASES
			wxMessageBox(_T(
							"Warning: you are trying to remove more empty source phrases than exist at that location: the command will be ignored."),
						 _T(""),wxICON_EXCLAMATION);
			if (m_pApp->m_selectionLine != -1)
				m_pView->RemoveSelection();
			m_pView->Invalidate();
			m_pLayout->PlaceBox();
			return;
		}
	}
	
    // a null source phrase can (as of version 1.3.0) be last in the list, so we can no
    // longer assume there will be a non-null one following, if we are at the end we must
    // restore the active location to an earlier sourcephrase, otherwise, to a following
    // one
	bool bNoneFollows = FALSE;
	CSourcePhrase* pSrcPhraseFollowing = 0;
	if (nStartingSequNum + nCount > m_pApp->GetMaxIndex())
	{
		// we are at the very end, or wanting to remove more at the end than is possible
		bNoneFollows = TRUE; // flag this condition
	}
	
	if (bNoneFollows)
		pSrcPhraseFollowing = 0;
	else
	{
		pSrcPhraseFollowing = (CSourcePhrase*)removePos->GetData();
		removePos = removePos->GetNext();
		wxASSERT(pSrcPhraseFollowing != NULL);
	}
	
	wxASSERT(pFirstOne != NULL); // whm added; note: if pFirstOne can ever be NULL there
	// should be more code added here to deal with it
    // if the null source phrase(s) carry any punctuation or markers, then these have to be
    // first transferred to the appropriate normal source phrase in the context, whether
    // forwards or backwards, depending on what is stored.

	// docVersion 5 has endmarkers stored on the CSourcePhrase they are pertinent to, in
	// m_endMarkers, so we search for these on pFirst, and if the member is non-empty,
	// transfer its contents to pPrevSrcPhrase (if the markers were automatically
	// transferred to the placement earlier, then pPrevSrcPhrase is guaranteed to exist)
	if (!pFirstOne->GetEndMarkers().IsEmpty() && pPrevSrcPhrase != NULL)
	{
		wxString emptyStr = _T("");
		pPrevSrcPhrase->SetEndMarkers(pFirstOne->GetEndMarkers());
		pFirstOne->SetEndMarkers(emptyStr);
	}
	if (!pFirstOne->m_markers.IsEmpty() && !bNoneFollows)
	{
        // BEW comment 25Jul05, if a TextType == none endmarker was initial in m_markers,
        // it will have been moved to the placeholder; so the next line handles other
        // situations as well as moving an endmarker back on to the following sourcephrase
        // which formerly owned it
		pSrcPhraseFollowing->m_markers = pFirstOne->m_markers; // don't clear original
		
		// now all the other things which depend on markers
		pSrcPhraseFollowing->m_inform = pFirstOne->m_inform;
		pSrcPhraseFollowing->m_chapterVerse = pFirstOne->m_chapterVerse;
		pSrcPhraseFollowing->m_bVerse = pFirstOne->m_bVerse;
		pSrcPhraseFollowing->m_bParagraph = pFirstOne->m_bParagraph;
		pSrcPhraseFollowing->m_bChapter = pFirstOne->m_bChapter;
		pSrcPhraseFollowing->m_bSpecialText = pFirstOne->m_bSpecialText;
		pSrcPhraseFollowing->m_bFootnote = pFirstOne->m_bFootnote;
		pSrcPhraseFollowing->m_bFirstOfType = pFirstOne->m_bFirstOfType;
		pSrcPhraseFollowing->m_curTextType = pFirstOne->m_curTextType;
		
		// BEW 05Jan06 if there was a moved note we must ensure that the following
		// sourcephrase gets the note flag set (it might already be TRUE anyway)
		pSrcPhraseFollowing->m_bHasNote = pFirstOne->m_bHasNote;
	}
	// block ammended by BEW 25Jul05
	if (!pFirstOne->m_precPunct.IsEmpty() && !bNoneFollows)
	{
		pSrcPhraseFollowing->m_precPunct = pFirstOne->m_precPunct;
		
		// fix the m_targetStr member (we are just fixing punctuation, so no store needed)
		m_pView->MakeLineFourString(pSrcPhraseFollowing,pSrcPhraseFollowing->m_targetStr);
		
		// anything else
		pSrcPhraseFollowing->m_bFirstOfType = pFirstOne->m_bFirstOfType;
	}
	// BEW added 25Jul05
    // a m_bHasFreeTrans = TRUE value can be ignored provided m_bStartFreeTrans value is
    // FALSE, if the latter is TRUE, then we must move the value to the following
    // sourcephrase
	if (pFirstOne->m_bStartFreeTrans && !bNoneFollows)
	{
		pSrcPhraseFollowing->m_bStartFreeTrans = TRUE;
		pSrcPhraseFollowing->m_bHasFreeTrans = TRUE;
	}
	wxASSERT(pLastOne != NULL); // whm added; note: if pLastOne can ever be NULL there
	// should be more code added here to deal with it
	if (!pLastOne->m_follPunct.IsEmpty() && nStartingSequNum > 0)
	{
		pPrevSrcPhrase->m_follPunct = pLastOne->m_follPunct;
		
		// now the other stuff
		pPrevSrcPhrase->m_bFootnoteEnd = pLastOne->m_bFootnoteEnd;
		pPrevSrcPhrase->m_bBoundary = pLastOne->m_bBoundary;
		
		// fix the m_targetStr member (we are just fixing punctuation, so no store needed)
		m_pView->MakeLineFourString(pPrevSrcPhrase,pPrevSrcPhrase->m_targetStr);
	}
	// BEW added 25Jul05...
    // a m_bHasFreeTrans = TRUE value can be ignored provided m_bEndFreeTrans value is
    // FALSE, if the latter is TRUE, then we must move the value to the preceding
    // sourcephrase
	if (pLastOne->m_bEndFreeTrans && nStartingSequNum > 0)
	{
		pPrevSrcPhrase->m_bEndFreeTrans = TRUE;
		pPrevSrcPhrase->m_bHasFreeTrans = TRUE;
	}
	
	// remove the null source phrases from the list, after removing their 
	// translations from the KB
	removePos = savePos;
	count = 0;
	wxString emptyStr = _T("");
	while (removePos != NULL && count < nCount)
	{
		SPList::Node* pos2 = removePos; // save current position for RemoveAt call
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)removePos->GetData();
		
		// BEW added 13Mar09 for refactored layout
		m_pView->GetDocument()->DeletePartnerPile(pSrcPhrase);
		removePos = removePos->GetNext();
		wxASSERT(pSrcPhrase != NULL);
		// last param being FALSE means do lookup with m_adaption, not the phrase box
		// contents (the KB pointer can be m_pKB as here, or m_pGlossingKB) and the
		// first FALSE is the value for gbIsGlossing here (placeholders support is not
		// a glossing mode feature)
		m_pApp->m_pKB->GetAndRemoveRefString(FALSE,pSrcPhrase,emptyStr,FALSE);

		count++;
		delete pSrcPhrase;
		pList->DeleteNode(pos2); 
	}
	
    // calculate the new active sequ number - it could be anywhere, but all we need to know
    // is whether or not the last removal of the sequence was done preceding the former
    // active sequ number's location
	if (nStartingSequNum + nCount < nActiveSequNum)
		m_pApp->m_nActiveSequNum = nActiveSequNum - nCount;
	else
	{
		if (bNoneFollows)
			m_pApp->m_nActiveSequNum = nStartingSequNum - 1;
		else
			m_pApp->m_nActiveSequNum = nStartingSequNum;
	}
	
    // update the sequence numbers, starting from the location of the first one removed;
    // but if we removed at the end, no update is needed
	if (!bNoneFollows)
		m_pView->UpdateSequNumbers(nStartingSequNum);
	
	// for getting over the hump of the call below to RecalcLayout() we only need a temporary
	// reasonably accurate active pile pointer set, and only if the removal was done at the
	// active location - if it wasn't, then RecalcLayout() will set things up correctly
	// without a failure
	if (nRemovedPileIndex == nActiveSequNum)
	{
		// set a temporary one, we'll use the pile which is now at nRemovedPileIndex
		// location, which typically is the one immediately following the placeholder's
		// old location; but if deleted from the doc's end, we'll use the last valid
		// document location
		int nMaxDocIndex = m_pApp->GetMaxIndex();
		if (nRemovedPileIndex > nMaxDocIndex)
			m_pApp->m_pActivePile = m_pView->GetPile(nMaxDocIndex);
		else
			m_pApp->m_pActivePile = m_pView->GetPile(nRemovedPileIndex);
	}
	
    // in case the active location is going to be a retranslation, check and if so, advance
    // past it; but if at the end, then back up to a valid preceding location
	CSourcePhrase* pSP = m_pView->GetSrcPhrase(m_pApp->m_nActiveSequNum);
	CPile* pNewPile;
	if (pSP->m_bRetranslation)
	{
		CPile* pPile = m_pView->GetPile(m_pApp->m_nActiveSequNum);
		do {
			pNewPile = m_pView->GetNextPile(pPile);
			if (pNewPile == NULL)
			{
				// move backwards instead, and find a suitable location
				pPile = m_pView->GetPile(m_pApp->m_nActiveSequNum);
				do {
					pNewPile = m_pView->GetPrevPile(pPile);
					pPile = pNewPile;
				} while (pNewPile->GetSrcPhrase()->m_bRetranslation);
				goto b;
			}
			pPile = pNewPile;
		} while (pNewPile->GetSrcPhrase()->m_bRetranslation);
	b:		m_pApp->m_pActivePile = pNewPile;
		m_pApp->m_nActiveSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
	}
	
    // we need to set m_targetPhrase to what it will be at the new active location, else if
    // the old string was real long, the CalcPileWidth() call will compute enormous and
    // wrong box width at the new location
	pSP = m_pView->GetSrcPhrase(m_pApp->m_nActiveSequNum);
	if (!m_pApp->m_bHidePunctuation) // BEW 8Aug09, removed deprecated m_bSuppressLast from test
		m_pApp->m_targetPhrase = pSP->m_targetStr;
	else
		m_pApp->m_targetPhrase = pSP->m_adaption;
	
	// recalculate the layout
#ifdef _NEW_LAYOUT
	m_pLayout->RecalcLayout(pList, keep_strips_keep_piles);
#else
	m_pLayout->RecalcLayout(pList, create_strips_keep_piles);
#endif
	
	// get a new (valid) active pile pointer, now that the layout is recalculated
	m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);
	wxASSERT(m_pApp->m_pActivePile);
	
	// create the phraseBox at the active pile, do it using PlacePhraseBox()...
	CSourcePhrase* pSrcPhrase = m_pApp->m_pActivePile->GetSrcPhrase();
	wxASSERT(pSrcPhrase != NULL);
	
    // renumber its sequ number, as its now in a new location because of the deletion (else
    // the PlacePhraseBox call below will get the wrong number when it reads its
    // m_nSequNumber attribute)
	pSrcPhrase->m_nSequNumber = m_pApp->m_nActiveSequNum;
	m_pView->UpdateSequNumbers(m_pApp->m_nActiveSequNum);
	
	// set m_targetPhrase to the appropriate string
	if (!pSrcPhrase->m_adaption.IsEmpty())
	{
		if (!m_pApp->m_bHidePunctuation) // BEW 8Aug09, removed deprecated m_bSuppressLast from test
			m_pApp->m_targetPhrase = pSrcPhrase->m_targetStr;
		else
			m_pApp->m_targetPhrase = pSrcPhrase->m_adaption;
	}
	else
	{
		m_pApp->m_targetPhrase.Empty(); // empty string will have to do
	}
	
    // we must remove the source phrase's translation from the KB as if we
    // had clicked here (otherwise PlacePhraseBox will assert)...
	
    // last param being FALSE means do lookup with m_adaption, not the phrase box
    // contents (the KB pointer can be m_pKB as here, or m_pGlossingKB) and the
    // first FALSE is the value for gbIsGlossing here (placeholders support is not
    // a glossing mode feature)
	m_pApp->m_pKB->GetAndRemoveRefString(FALSE,pSrcPhrase,emptyStr,FALSE);

    // save old sequ number in case required for toolbar's Back button - but since it
    // probably has been lost (being the null source phrase location), to be safe we must
    // set it to the current active location
	gnOldSequNum = m_pApp->m_nActiveSequNum;
	
	// scroll into view, just in case a lot were inserted
	m_pApp->GetMainFrame()->canvas->ScrollIntoView(m_pApp->m_nActiveSequNum);
	m_pView->Invalidate();
	// now place the box
	m_pLayout->PlaceBox();
}

// pList is the sublist of (formerly) selected source phrase instances, pSrcPhrases is the
// document's list (the whole lot), nCount is the count of elements in pList (it will be
// reduced as each null source phrase is eliminated), bActiveLocAfterSelection is a flag in
// the caller, nSaveActiveSequNum is the caller's saved value for the active sequence
// number
// BEW updated 17Feb10 for support of doc version 5 (no changes were needed)
void CPlaceholder::RemoveNullSrcPhraseFromLists(SPList*& pList,SPList*& pSrcPhrases,
												  int& nCount,int& nEndSequNum,bool bActiveLocAfterSelection,
												  int& nSaveActiveSequNum)
{
	// refactored 16Apr09
	// find the null source phrase in the sublist
	SPList::Node* pos = pList->GetFirst();
	while (pos != NULL)
	{
		SPList::Node* savePos = pos;
		CSourcePhrase* pSrcPhraseCopy = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext(); 
		wxASSERT(pSrcPhraseCopy != NULL);
		if (pSrcPhraseCopy->m_bNullSourcePhrase)
		{
            // we've found a null source phrase in the sublist, so get rid of its KB
            // presence, then delete it from the (temporary) sublist, and its instance from
            // the heap
			wxString emptyStr = _T("");
            // last param being FALSE means do lookup with m_adaption, not the phrase box
            // contents (the KB pointer can be m_pKB as here, or m_pGlossingKB) and the
            // first FALSE is the value for gbIsGlossing here (placeholders support is not
            // a glossing mode feature)
			m_pApp->m_pKB->GetAndRemoveRefString(FALSE,pSrcPhraseCopy,emptyStr,FALSE);

			delete pSrcPhraseCopy;
			pSrcPhraseCopy = (CSourcePhrase*)NULL;
			pList->DeleteNode(savePos); 
			
            // the main list on the app still stores the (now hanging) pointer, so find
            // where it is and remove it from that list too
			SPList::Node* mainPos = pSrcPhrases->GetFirst();
			wxASSERT(mainPos != 0);
			mainPos = pSrcPhrases->Find(pSrcPhraseCopy); // search from the beginning
			wxASSERT(mainPos != NULL); // it must be there somewhere
			
			// BEW added 13Mar09 for refactor of layout; delete its partner pile too 
			m_pView->GetDocument()->DeletePartnerPile(pSrcPhraseCopy);
			pSrcPhrases->DeleteNode(mainPos); 
			
			nCount -= 1; // since there is one less source phrase in the selection now
			nEndSequNum -= 1;
			if (bActiveLocAfterSelection)
				nSaveActiveSequNum -= 1;
			
            // now we have to renumber the source phrases' sequence number values - since
            // the temp sublist list has pointer copies, we need only do this in the main
            // list using a call to UpdateSequNumbers, and so the value of nSaveSequNum
            // will still be correct for the first element in the sublist - even if there
            // were deletions at the start of the sublist
			m_pView->UpdateSequNumbers(0); // start from the very first in the list to be safe
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Event Handlers
///////////////////////////////////////////////////////////////////////////////

// BEW 18Feb10 updated for doc version 5 support (no changes needed)
void CPlaceholder::OnButtonRemoveNullSrcPhrase(wxCommandEvent& WXUNUSED(event))
{
    // Since the Remove Placeholder toolbar button has an accelerator table hot key (CTRL-D
    // see CMainFrame) and wxWidgets accelerator keys call menu and toolbar handlers even
    // when they are disabled, we must check for a disabled button and return if disabled.
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	wxToolBarBase* pToolBar = pFrame->GetToolBar();
	wxASSERT(pToolBar != NULL);
	if (!pToolBar->GetToolEnabled(ID_BUTTON_REMOVE_NULL_SRCPHRASE))
	{
		::wxBell();
		return;
	}
	
	// If glossing is ON we don't allow removal (nor insertion) of null source phrases,
	// so warn the user and return.
	if (gbIsGlossing)
	{
		//IDS_NOT_WHEN_GLOSSING
		wxMessageBox(_("This particular operation is not available when you are glossing."),
					 _T(""),wxICON_INFORMATION);
		return;
	}
	
    // find the pile containing the null src phrase to be deleted - it will either be the
    // first selected pile, if there is a selection current, or the active location if no
    // selection is current; the button removes only one at a time.
	CPile* pRemoveLocPile;
	int nCount = 1;
	if (m_pApp->m_selectionLine != -1)
	{
		// we have a selection, the pile we want is that of the selection list's first element
		CCellList* pCellList = &m_pApp->m_selection;
		CCellList::Node* cpos = pCellList->GetFirst();
		CCell* pCell;
		pCell = cpos->GetData();
		pRemoveLocPile = cpos->GetData()->GetPile();
		wxASSERT(pRemoveLocPile != NULL);
		if (pRemoveLocPile->GetSrcPhrase()->m_bNullSourcePhrase != TRUE)
		{
			::wxBell();
			m_pView->RemoveSelection();
			return;
		}
		if (pRemoveLocPile->GetSrcPhrase()->m_bRetranslation == TRUE)
		{
			::wxBell(); 
			m_pView->RemoveSelection();
			return;
		}
		m_pView->RemoveSelection(); // Invalidate() will be called in RemoveNullSourcePhrase()
	}
	else
	{
		// no selection, so just remove at wherever the phraseBox currently is located
		pRemoveLocPile = m_pApp->m_pActivePile;
		wxASSERT(pRemoveLocPile != NULL);
		if (pRemoveLocPile->GetSrcPhrase()->m_bNullSourcePhrase == FALSE ||
			pRemoveLocPile->GetSrcPhrase()->m_bRetranslation == TRUE)
		{
			::wxBell();
			return;
		}
	}
    // remove the placeholder - note, this will clobber the m_pActivePile pointer, and a
    // new partner pile will not be created, so we have to provide a temporary valid
    // m_pActivePile after this next call returns, so that our RecalcLayout() code which
    // needs to find the active strip will find a pile in a strip at or about the right
    // place in the document (so the layout gets set up right) -- what we need to ensure is
    // that any access to CPile::m_pOwningStrip using the GetStrip() call, will not have an
    // m_pOwningStrip value set to NULL (or to a non-NULL value in the debugger which then
    // points at arbitrary memory)
	RemoveNullSourcePhrase(pRemoveLocPile, nCount);
	
    // we don't do the next block at a deeper level because removing a retranslation which
    // is long uses lower level functions to do automated placeholder removals, and we
    // don't want them counted a second time; so we do it here only, in the handler for a
    // user's manual click of the command bar button to remove a placeholder
	if (gbVerticalEditInProgress)
	{
		// update the relevant parts of the adaptationsStep parameters in gEditRecord
		gEditRecord.nAdaptationStep_ExtrasFromUserEdits -= 1;
		gEditRecord.nAdaptationStep_NewSpanCount -= 1;
		gEditRecord.nAdaptationStep_EndingSequNum -= 1;
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever idle processing is enabled. If any
/// of the following conditions are TRUE, this handler disables the "Remove A Placeholder"
/// toolbar item and returns immediately: The application is in glossing mode, the target
/// text only is showing in the main window, the m_pActivePile pointer is NULL, or the
/// application is in Free Translation mode. It enables the toolbar button if there is a
/// selection which is on a null source phrase which is not a retranslation, or if the
/// active pile is a null source phrase which is not a retranslation. The selection, if
/// there is one, takes priority, if its pile is different from the active pile..
/////////////////////////////////////////////////////////////////////////////////
void CPlaceholder::OnUpdateButtonRemoveNullSrcPhrase(wxUpdateUIEvent& event)
{
	if (gbIsGlossing || gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	bool bCanDelete = FALSE;
	if (m_pApp->m_pTargetBox->GetHandle() != NULL)
	{
        // set the flag true either if there is a selection and which is on a null source
        // phrase which is not a retranslation, or if the active pile is a null source
        // phrase which is not a retranslation. The selection, if there is one, takes
        // priority, if its pile is different from the active pile.
		if (m_pApp->m_selectionLine != -1)
		{
			CCellList::Node* cpos = m_pApp->m_selection.GetFirst();
			CCell* pCell = cpos->GetData();
			if (pCell->GetPile()->GetSrcPhrase()->m_bNullSourcePhrase
				&& !pCell->GetPile()->GetSrcPhrase()->m_bRetranslation)
			{
				bCanDelete = TRUE;
			}
		}
		else
		{
			wxWindow *focus = wxWindow::FindFocus();
			if (m_pApp->m_pTargetBox == focus) // don't use GetHandle() on m_pTargetBox here !!!
			{
				if (m_pApp->m_pTargetBox->IsShown()
					&& m_pApp->m_pActivePile->GetSrcPhrase()->m_bNullSourcePhrase
					&& !m_pApp->m_pActivePile->GetSrcPhrase()->m_bRetranslation)
					bCanDelete = TRUE;
			}
		}
	}
	event.Enable(bCanDelete);
}

// BEW 18Feb10 updated for doc version 5 support (no changes needed)
void CPlaceholder::OnButtonNullSrc(wxCommandEvent& WXUNUSED(event))
{
    // Since the Add placeholder toolbar button has an accelerator table hot key (CTRL-I
    // see CMainFrame) and wxWidgets accelerator keys call menu and toolbar handlers even
    // when they are disabled, we must check for a disabled button and return if disabled.
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument(); //GetDocument();
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	wxToolBarBase* pToolBar = pFrame->GetToolBar();
	wxASSERT(pToolBar != NULL);
	if (!pToolBar->GetToolEnabled(ID_BUTTON_NULL_SRC))
	{
		::wxBell();
		return;
	}
	
	if (gbIsGlossing)
	{
		//IDS_NOT_WHEN_GLOSSING
		wxMessageBox(_(
					   "This particular operation is not available when you are glossing."),
					 _T(""),wxICON_INFORMATION);
		return;
	}
	int nSequNum;
	int nCount;
	
	// Bill wanted the behaviour modified, so that if the box's m_bAbandonable flag is TRUE
	// (ie. a copy of source text was done and nothing typed yet) then the current pile
	// would have the box contents abandoned, nothing put in the KB, and then the placeholder
	// inserion - the advantage of this is that if the placeholder is inserted immediately
	// before the phrasebox's location, then after the placeholder text is typed and the user
	// hits ENTER to continue looking ahead, the former box location will get the box and the
	// copy of the source redone, rather than the user usually having to edit out an unwanted
	// copy from the KB, or remember to clear the box manually. A sufficient thing to do here
	// is just to clear the box's contents.
	if (m_pApp->m_pTargetBox->m_bAbandonable)
	{
		m_pApp->m_targetPhrase.Empty();
		if (m_pApp->m_pTargetBox->GetHandle() != NULL && m_pApp->m_pTargetBox->IsShown())
		{
			m_pApp->m_pTargetBox->ChangeValue(_T(""));
		}
	}
	
	if (wxGetKeyState(WXK_CONTROL))
	{
		// CTRL key is down, so an "insert after" is wanted
		// first save old sequ num for active location
		gnOldSequNum = m_pApp->m_nActiveSequNum;
		
		// find the pile after which to do the insertion - it will either be after the
		// last selected pile if there is a selection current, or after the active location
		// if no selection is current - beware the case when active location is a doc end!
		CPile* pInsertLocPile;
		nCount = 1; // the button or shortcut can only insert one
		nSequNum = -1;
		if (m_pApp->m_selectionLine != -1)
		{
			// we have a selection, the pile we want is that of the selection 
			// list's last element
			CCellList* pCellList = &m_pApp->m_selection;
			CCellList::Node* cpos = pCellList->GetLast();
			pInsertLocPile = cpos->GetData()->GetPile();
			if (pInsertLocPile == NULL)
			{
				wxMessageBox(_T(
								"A zero pointer was returned, the insertion cannot be done."),
							 _T(""), wxICON_EXCLAMATION);
				return;
			}
			nSequNum = pInsertLocPile->GetSrcPhrase()->m_nSequNumber;
			wxASSERT(pInsertLocPile != NULL);
			m_pView->RemoveSelection(); // Invalidate will be called in InsertNullSourcePhrase()
		}
		else
		{
			// no selection, so just insert after wherever the phraseBox currently is located
			pInsertLocPile = m_pApp->m_pActivePile;
			if (pInsertLocPile == NULL)
			{
				wxMessageBox(_T(
								"A zero pointer was returned, the insertion cannot be done."),
							 _T(""), wxICON_EXCLAMATION);
				return;
			}
			nSequNum = pInsertLocPile->GetSrcPhrase()->m_nSequNumber;
		}
		wxASSERT(nSequNum >= 0);
		
        // check we are not in a retranslation - we can't insert there! We can be at the
        // end of a retranslation since we are then inserting after it, but if we are not
        // at the end, there will be at least one more retranslation sourcephrase to the
        // right, and so we must check if this is the case & abort the operation if so
		if(pInsertLocPile->GetSrcPhrase()->m_bRetranslation)
		{
			CPile* pPile = m_pView->GetNextPile(pInsertLocPile);
			if (pPile == NULL || pPile->GetSrcPhrase()->m_bRetranslation)
			{
				// IDS_NO_INSERT_IN_RETRANS
				wxMessageBox(_(
							   "You cannot insert a placeholder within a retranslation. The command has been ignored.")
							 ,_T(""), wxICON_EXCLAMATION);
				m_pView->RemoveSelection();
				m_pView->Invalidate();
				m_pLayout->PlaceBox();
				return;
			}
		}
		
		// ensure the contents of the phrase box are saved to the KB
		// & make the punctuated target string
		if (m_pApp->m_pTargetBox->GetHandle() != NULL && m_pApp->m_pTargetBox->IsShown())
		{
			m_pView->MakeLineFourString(m_pApp->m_pActivePile->GetSrcPhrase(), m_pApp->m_targetPhrase);
			
            // we are about to leave the current phrase box location, so we must try to
            // store what is now in the box, if the relevant flags allow it
			m_pView->RemovePunctuation(pDoc,&m_pApp->m_targetPhrase,from_target_text);
			gbInhibitLine4StrCall = TRUE;
			bool bOK;
			bOK = m_pView->StoreText(m_pApp->m_pKB, m_pApp->m_pActivePile->GetSrcPhrase(), 
							m_pApp->m_targetPhrase);
			gbInhibitLine4StrCall = FALSE;
		}
		
        // at this point, we need to increment the pInsertLocPile pointer to the next pile,
        // and the nSequNum to it's sequence number, since InsertNullSourcePhrase() always
        // inserts "before" the pInsertLocPile; however, if the selection end, or active
        // location if there is no selection, is at the very end of the document (ie. last
        // sourcephrase), there is no following source phrase instance to insert before. If
        // this is the case, we have to append a dummy sourcephrase at the end of the
        // document, do the insertion, and then remove it again; and we will also have to
        // set (and later clear) the global gbDummyAddedTemporarily because this is used in
        // the function in order to force a leftwards association only (and hence the user
        // does not have to be asked whether to associate right or left, if there is final
        // punctuation)
		SPList* pSrcPhrases = m_pApp->m_pSourcePhrases;
		CSourcePhrase* pDummySrcPhrase = NULL; // whm initialized to NULL
		if (nSequNum == m_pApp->GetMaxIndex())
		{
			// a dummy is temporarily required
			gbDummyAddedTemporarily = TRUE;
			
			// do the append
			pDummySrcPhrase = new CSourcePhrase;
			pDummySrcPhrase->m_srcPhrase = _T("dummy"); // something needed, so a 
			// pile width can be computed
			pDummySrcPhrase->m_key = pDummySrcPhrase->m_srcPhrase;
			pDummySrcPhrase->m_nSequNumber = m_pApp->GetMaxIndex() + 1;
			SPList::Node* posTail;
			posTail = pSrcPhrases->Append(pDummySrcPhrase);
			
			// now we need to add a partner pile for it in CLayout::m_pileList
			pDoc->CreatePartnerPile(pDummySrcPhrase);
			
			// we need a valid layout which includes the new dummy element on 
			// its own pile
			m_pApp->m_nActiveSequNum = m_pApp->GetMaxIndex();
#ifdef _NEW_LAYOUT
			m_pLayout->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
			m_pLayout->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
			m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->GetMaxIndex()); // temporary 
			// active location, at the dummy one
			// now we can do the insertion
			pInsertLocPile = m_pApp->m_pActivePile;
			nSequNum = m_pApp->GetMaxIndex();
		}
		else
		{
			// the 'next' pile is not beyond the document's end, so make it the insert
			// location
			CPile* pPile = m_pView->GetNextPile(pInsertLocPile);
			wxASSERT(pPile != NULL);
			pInsertLocPile = pPile;
			m_pApp->m_pActivePile = pInsertLocPile; // ensure it is up to date
			nSequNum++; // make the sequence number agree
		}
		
		InsertNullSourcePhrase(pDoc,pInsertLocPile,nCount,TRUE,FALSE,FALSE); // here, never
		// for Retransln
		// if we inserted a dummy, now get rid of it and clear the global flag
		if (gbDummyAddedTemporarily)
		{
			gbDummyAddedTemporarily = FALSE;
			
			// first, remove the temporary partner pile
			pDoc->DeletePartnerPile(pDummySrcPhrase);
			
			// now remove the dummy element, and make sure memory is not leaked!
			delete pDummySrcPhrase->m_pSavedWords;
			delete pDummySrcPhrase->m_pMedialMarkers;
			delete pDummySrcPhrase->m_pMedialPuncts;
			bool deleteOK;
			deleteOK = pSrcPhrases->DeleteNode(pSrcPhrases->GetLast());
			wxASSERT(deleteOK);
			delete pDummySrcPhrase;
			
			// get another valid layout
			m_pApp->m_nActiveSequNum = m_pApp->GetMaxIndex();
#ifdef _NEW_LAYOUT
			m_pLayout->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
			m_pLayout->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
			m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->GetMaxIndex()); // temporarily at the end, 
			// caller will fix
			nSequNum = m_pApp->GetMaxIndex();
		}
		
		// BEW added 10Sep08 in support of Vertical Edit mode
		if (gbVerticalEditInProgress)
		{
            // update the relevant parts of the gEditRecord (all spans are affected
            // equally, except the source text edit section is unchanged)
			gEditRecord.nAdaptationStep_ExtrasFromUserEdits += 1;
			gEditRecord.nAdaptationStep_NewSpanCount += 1;
			gEditRecord.nAdaptationStep_EndingSequNum += 1;
		}
		
		// jump to it (can't use old pile pointers, the recalcLayout call 
		// will have clobbered them)
		CPile* pPile = m_pView->GetPile(nSequNum);
		m_pView->Jump(m_pApp,pPile->GetSrcPhrase());
	}
	else // not inserting after the selection's end or active location, but before
	{
		// first save old sequ num for active location
		gnOldSequNum = m_pApp->m_nActiveSequNum;
		
        // find the pile preceding which to do the insertion - it will either be preceding
        // the first selected pile, if there is a selection current, or preceding the
        // active location if no selection is current
		CPile* pInsertLocPile;
		nCount = 1; // the button or shortcut can only insert one
		nSequNum = -1;
		if (m_pApp->m_selectionLine != -1)
		{
			// we have a selection, the pile we want is that of the selection 
			// list's first element
			CCellList* pCellList = &m_pApp->m_selection;
			CCellList::Node* cpos = pCellList->GetFirst();
			pInsertLocPile = cpos->GetData()->GetPile();
			if (pInsertLocPile == NULL)
			{
				wxMessageBox(_T(
								"A zero pointer was returned, the insertion cannot be done."),
							 _T(""), wxICON_EXCLAMATION);
				return;
			}
			nSequNum = pInsertLocPile->GetSrcPhrase()->m_nSequNumber;
			wxASSERT(pInsertLocPile != NULL);
			m_pView->RemoveSelection(); // Invalidate will be called in InsertNullSourcePhrase()
		}
		else
		{
			// no selection, so just insert preceding wherever the phraseBox 
			// currently is located
			pInsertLocPile = m_pApp->m_pActivePile;
			if (pInsertLocPile == NULL)
			{
				wxMessageBox(_T(
								"A zero pointer was returned, the insertion cannot be done."),
							 _T(""), wxICON_EXCLAMATION);
				return;
			}
			nSequNum = pInsertLocPile->GetSrcPhrase()->m_nSequNumber;
		}
		wxASSERT(nSequNum >= 0);
		
		
		// check we are not in a retranslation - we can't insert there! (only need to check
		// previous one, if it is still a retranslation, we must abort the operation)
		if(pInsertLocPile->GetSrcPhrase()->m_bRetranslation)
		{
			CPile* pPile = m_pView->GetPrevPile(pInsertLocPile);
			if (pPile == NULL || pPile->GetSrcPhrase()->m_bRetranslation)
			{
				// IDS_NO_INSERT_IN_RETRANS
				wxMessageBox(_(
							   "Sorry, you cannot insert a placeholder within a retranslation. The command has been ignored."),
							 _T(""), wxICON_EXCLAMATION);
				m_pView->RemoveSelection();
				m_pView->Invalidate();
				m_pLayout->PlaceBox();
				return;
			}
		}
		
		// ensure the contents of the phrase box are saved to the KB
		// & make the punctuated target string
		if (m_pApp->m_pTargetBox->GetHandle() != NULL && m_pApp->m_pTargetBox->IsShown())
		{
			m_pView->MakeLineFourString(m_pApp->m_pActivePile->GetSrcPhrase(), m_pApp->m_targetPhrase);
			
            // we are about to leave the current phrase box location, so we must try to
            // store what is now in the box, if the relevant flags allow it
			m_pView->RemovePunctuation(pDoc,&m_pApp->m_targetPhrase,from_target_text);
			gbInhibitLine4StrCall = TRUE;
			bool bOK;
			bOK = m_pView->StoreText(m_pApp->m_pKB, m_pApp->m_pActivePile->GetSrcPhrase(), 
							m_pApp->m_targetPhrase);
			gbInhibitLine4StrCall = FALSE;
		}
		
		InsertNullSourcePhrase(pDoc, pInsertLocPile, nCount);
		
		// BEW added 10Sep08 in support of Vertical Edit mode
		if (gbVerticalEditInProgress)
		{
            // update the relevant parts of the gEditRecord (all spans are affected
            // equally, except the source text edit section is unchanged)
			gEditRecord.nAdaptationStep_ExtrasFromUserEdits += 1;
			gEditRecord.nAdaptationStep_NewSpanCount += 1;
			gEditRecord.nAdaptationStep_EndingSequNum += 1;
		}
		
		// jump to it (can't use old pile pointers, the recalcLayout call 
		// will have clobbered them)
		CPile* pPile = m_pView->GetPile(nSequNum);
		m_pView->Jump(m_pApp, pPile->GetSrcPhrase());
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever idle processing is enabled. If any
/// of the following conditions are TRUE, this handler disables the "Insert A Placeholder"
/// toolbar item and returns immediately: The application is in glossing mode, the target
/// text only is showing in the main window, the m_pActivePile pointer is NULL, or the
/// application is in Free Translation mode. It enables the toolbar button if there is a
/// valid selection or the targetBox is showing, and the targetBox is the window in focus.
/////////////////////////////////////////////////////////////////////////////////
void CPlaceholder::OnUpdateButtonNullSrc(wxUpdateUIEvent& event)
{
	if (gbIsGlossing || gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	bool bCanInsert = FALSE;
	if (m_pApp->m_pTargetBox != NULL)
	{
		if (m_pApp->m_selectionLine != -1 || (m_pApp->m_pTargetBox->IsShown()
											&& (m_pApp->m_pTargetBox == wxWindow::FindFocus())))
			bCanInsert = TRUE;
	}
	event.Enable(bCanInsert);
}

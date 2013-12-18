/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Placeholder.cpp
/// \author			Erik Brommers
/// \date_created	02 April 2010
/// \rcs_id $Id$
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
#include "helpers.h"
#include "Placeholder.h"
//////////



///////////////////////////////////////////////////////////////////////////////
// External globals
///////////////////////////////////////////////////////////////////////////////
extern int gnOldSequNum;
extern bool gbVerticalEditInProgress;
extern bool gbInhibitMakeTargetStringCall;
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
						 _T(""), wxICON_EXCLAMATION | wxOK);
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
						 _T(""), wxICON_EXCLAMATION | wxOK);
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
						 _T(""), wxICON_EXCLAMATION | wxOK);
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
		m_pView->MakeTargetStringIncludingPunctuation(m_pApp->m_pActivePile->GetSrcPhrase(), m_pApp->m_targetPhrase);
		
        // we are about to leave the current phrase box location, so we must try to store
        // what is now in the box, if the relevant flags allow it
		m_pView->RemovePunctuation(pDoc, &m_pApp->m_targetPhrase, from_target_text);
		gbInhibitMakeTargetStringCall = TRUE;
		bool bOK;
		bOK = m_pApp->m_pKB->StoreText(m_pApp->m_pActivePile->GetSrcPhrase(), 
						m_pApp->m_targetPhrase);
		bOK = bOK; // avoid warning
		gbInhibitMakeTargetStringCall = FALSE;
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
						 _T(""), wxICON_EXCLAMATION | wxOK);
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
						 _T(""), wxICON_EXCLAMATION | wxOK);
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
						 _T(""), wxICON_EXCLAMATION | wxOK);
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
		m_pView->MakeTargetStringIncludingPunctuation(m_pApp->m_pActivePile->GetSrcPhrase(), m_pApp->m_targetPhrase);
		
        // we are about to leave the current phrase box location, so we must try to store
        // what is now in the box, if the relevant flags allow it
		m_pView->RemovePunctuation(pDoc, &m_pApp->m_targetPhrase, from_target_text);
		gbInhibitMakeTargetStringCall = TRUE;
		bool bOK;
		bOK = m_pApp->m_pKB->StoreText(m_pApp->m_pActivePile->GetSrcPhrase(), m_pApp->m_targetPhrase);
		bOK = bOK; // avoid warning
		gbInhibitMakeTargetStringCall = FALSE;
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
		posTail = posTail; // avoid warning
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
		if (pDummySrcPhrase->m_pSavedWords != NULL) // whm 11Jun12 added NULL test
			delete pDummySrcPhrase->m_pSavedWords;
		pDummySrcPhrase->m_pSavedWords = (SPList*)NULL;
		if (pDummySrcPhrase->m_pMedialMarkers != NULL) // whm 11Jun12 added NULL test
			delete pDummySrcPhrase->m_pMedialMarkers;
		pDummySrcPhrase->m_pMedialMarkers = (wxArrayString*)NULL;
		if (pDummySrcPhrase->m_pMedialPuncts != NULL) // whm 11Jun12 added NULL test
			delete pDummySrcPhrase->m_pMedialPuncts;
		pDummySrcPhrase->m_pMedialPuncts = (wxArrayString*)NULL;
		SPList::Node *pLast = pSrcPhrases->GetLast();
		pSrcPhrases->DeleteNode(pLast);
		if (pDummySrcPhrase != NULL) // whm 11Jun12 added NULL test
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

CSourcePhrase*  CPlaceholder::CreateBasicPlaceholder()
{
	CSourcePhrase* pSP = new CSourcePhrase();
	pSP->m_bNullSourcePhrase = TRUE;
	pSP->m_key = _T("...");
	pSP->m_srcPhrase = pSP->m_key;
	return pSP;
}

// BEW additions 22Jul05 for support of free translations when placeholder insertions are 
// done
// BEW additions 17Feb10 in support of doc version 5 (a previous, or at end of retranslation,
// CSourcePhrase with a non-empty m_endMarkers must move to the final placeholder in the
// retranslation, or in the case of manual placeholder insertion, must move to the
// placeholder if there is a leftward association stipulated when the message box asks the
// user
// BEW 24Sep10, more docVersion 5 changes, and improvements to code and comments, and
// elimination of the a: goto label
// BEW 11Oct10, more docVersion 5 changes, addition of m_follOuterPunct, and the four
// wxString members for storage of inline binding & non-binding begin- and end-markers;
// also prevent left association or right association if the CSourcePhrase being
// associated to has a USFM fixed space marker ~ conjoining two words (our doc model does
// not support moving markers or puncts to/from such an instance)
void CPlaceholder::InsertNullSourcePhrase(CAdapt_ItDoc* pDoc,
										   CPile* pInsertLocPile,const int nCount,
										   bool bRestoreTargetBox,bool bForRetranslation,
										   bool bInsertBefore)
{
	bool bAssociatingRightwards = FALSE;
	CSourcePhrase* pSrcPhrase = NULL; // whm initialized to NULL
	CSourcePhrase* pPrevSrcPhrase = NULL; // the sourcephrase which lies before the first 
	// inserted ellipsis, in a retranslation we have to check this one for an 
	// m_bEndFreeTrans being TRUE flag and move that BOOL value to the end of the
	// insertions, and so forth...
	//CPile* pPrevPile; // set but not used
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

    // BEW added to, 17Feb10, to support doc version 5 -- if the final non-placeholder
    // instance's m_endMarkers member has content, then that content has to be cleared out
    // and transferred to the last of the inserted placeholders - this is true in
    // retranslations, and also for non-retranslation placeholder insertion following a
    // CSourcePhrase with m_endMarkers with content and the association is leftwards (if
    // rightwards, the endmarkers stay put). So in the next section of code, deal with the
    // 'in retranslation' case; the left-association case has to be delayed until later
    // since we don't yet know which way the association will go
	wxString endmarkersToTransfer = _T("");
	wxString inlineNonbindingEndmarkersToTransfer = _T("");
	wxString inlineBindingEndmarkersToTransfer = _T("");
	wxString emptyStr = _T("");
	bool bTransferEndMarkers = FALSE; // true if any endmarkers transferred, of the 3 locations
	bool bMoveEndOfFreeTrans = FALSE; // moved declaration outside of if block below
	// BEW 23Sep10, store the sequence number to the pPrevSrcPhrase, so we can get a
	// pile pointer from it whenever needed (all the changes will occur at higher sequence
	// number values, so this value won't change throughout the placeholder insertion
	// process)
	int nPrevSequNum = -1; // it gets set when we need it, later below
	if (nStartingSequNum > 0) // whm added to prevent assert and unneeded test for 
		// m_bEndFreeTrans when sequ num is zero
	{
		SPList::Node* earlierPos = pList->Item(nStartingSequNum - 1);
		pPrevSrcPhrase = (CSourcePhrase*)earlierPos->GetData();
		if (pPrevSrcPhrase->m_bEndFreeTrans)
		{
			if (bForRetranslation)
			{
				pPrevSrcPhrase->m_bEndFreeTrans = FALSE;
				// only code in a "for retranslation" block will look at the
				// bMoveEndOfFreeTrans boolean
				bMoveEndOfFreeTrans = TRUE;
			}
			else
			{
				// when it is not for padding of a retranslation, we defer the decision
				// about removal of the endmarkers and tranferring them to the placeholder
				// until later - since it could be the case that the user may choose
				// rightwards association and so no endmarker transfer would be needed
				;
			}
		}

		if (!pPrevSrcPhrase->GetEndMarkers().IsEmpty() || 
			!pPrevSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty() ||
			!pPrevSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
		{
			if (bForRetranslation && !pPrevSrcPhrase->GetEndMarkers().IsEmpty())
			{
				// only code in a "for retranslation" block will look at the
				// bTransferEndMarkers boolean
				endmarkersToTransfer = pPrevSrcPhrase->GetEndMarkers();
				bTransferEndMarkers = TRUE;
				pPrevSrcPhrase->SetEndMarkers(emptyStr);
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
			if (bForRetranslation && !pPrevSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
			{
				// only code in a "for retranslation" block will look at the
				// bTransferEndMarkers boolean
				inlineNonbindingEndmarkersToTransfer = pPrevSrcPhrase->GetInlineNonbindingEndMarkers();
				bTransferEndMarkers = TRUE;
				pPrevSrcPhrase->SetInlineNonbindingEndMarkers(emptyStr);
			}
			if (bForRetranslation && !pPrevSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
			{
				// only code in a "for retranslation" block will look at the
				// bTransferEndMarkers boolean
				inlineBindingEndmarkersToTransfer = pPrevSrcPhrase->GetInlineBindingEndMarkers();
				bTransferEndMarkers = TRUE;
				pPrevSrcPhrase->SetInlineBindingEndMarkers(emptyStr);
			}
		}
	} // end of TRUE block for test: if (nStartingSequNum > 0)
	
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
		{
			pLastOne = pSrcPhrasePH;
		}
		pSrcPhrasePH->m_bNullSourcePhrase = TRUE;
		pSrcPhrasePH->m_srcPhrase = _T("...");
		pSrcPhrasePH->m_key = _T("...");
		pSrcPhrasePH->m_nSequNumber = nStartingSequNum + i; // ensures the
								// UpdateSequNumbers() call starts with correct value
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
			if (pPrevSrcPhrase->m_bHasFreeTrans)
				pSrcPhrasePH->m_bHasFreeTrans = TRUE; // handles the 'within' case
			if ((i == nCount - 1) && bMoveEndOfFreeTrans)
			{
                // move the end of the free translation to this last one (flag on old
                // location is already cleared above in anticipation of this)
				wxASSERT(pLastOne);
				pLastOne->m_bEndFreeTrans = TRUE;
			}
			// do the move of endmarkers to be transferred to the last placeholder in the
			// retranslation
			if ((i == nCount - 1) && bTransferEndMarkers)
			{
                // move the endMarkers to this last one (old m_endMarkers
                // location is already cleared above in anticipation of this)
				wxASSERT(pLastOne);
				pLastOne->SetEndMarkers(endmarkersToTransfer);
				pLastOne->SetInlineNonbindingEndMarkers(inlineNonbindingEndmarkersToTransfer);
				pLastOne->SetInlineBindingEndMarkers(inlineBindingEndmarkersToTransfer);
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
			{
				pSrcPhrasePH->m_bSpecialText = TRUE; // want it to have special text colour
				// retranslations can be done within a footnote, but we don't have to
				// here worry about text colour because the retranslation gets its own
				// text colour
			}
		}
		
		pList->Insert(insertPos,pSrcPhrasePH);
		
		// BEW added 13Mar09 for refactored layout
		pDoc->CreatePartnerPile(pSrcPhrasePH);
	} // end of for loop for inserting one or more placeholders
    // BEW 11Oct10, NOTE: transfer, for insertions in a retranslation span, of m_follPunct
    // data and m_follOuterPunct data could have been done above; for no good reason though
    // they are done below instead in the else block of the next major test
	
	// fix the sequ num for the old insert location's source phrase
	nSequNumInsLoc += nCount;
	
    // calculate the new active sequ number - it could be anywhere, but all we need to know
    // is whether or not the insertion was done preceding the former active sequ number's
    // location
	if (nStartingSequNum <= nActiveSequNum)
		m_pApp->m_nActiveSequNum = nActiveSequNum + nCount;
	
    // update the sequence numbers, starting from the first one inserted (although the
    // layout of the view is no longer valid, the relationships between piles,
    // CSourcePhrases and sequence numbers will be correct after the following call, and so
    // we can, for example, get the correct pile pointer for a given sequence number and
    // from the pointer, get the correct CSourcePhrase)
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
	// BEW 11Oct10, the above comments about beginmarkers apply to the new members
	// m_inlineNonbindingMarkers and m_inlineBindingMarkers, and the above comments about
	// endmarkers apply to the new m_inlineNonbindingEndMarkers and
	// m_inlineBindingEndMarkers and comments about following punctuation apply also to
	// the new member m_follOuterPunct - so changes related to all these are made below.
	bool bAssociationRequired = FALSE; // it will be set true if the message box for 
	// associating to the left or right is required at punctuation, or a
	// preceding non-empty m_endMarkers member (the latter for docVersion = 5 or above)
	// 
	// The following 4 booleans are only looked at by code in "not-for-retranslation"
	// blocks
	bool bPreviousFollPunct = FALSE; // true now if one or both of m_follPunct and
									 // m_follOuterPunct have content
	bool bFollowingPrecPunct = FALSE;
	bool bFollowingMarkers = FALSE;
	bool bEndMarkersPrecede = FALSE; // set true when preceding srcPhrase has endmarkers
									 // from any or all of the 3 endmarker string members
	nPrevSequNum = nStartingSequNum - 1; // remember, this could be -ve (ie. -1)

	// This is a major block for when inserting manually (ie. not in a retranslation), and
	// manual insertions are always only a single placeholder per insertion command by the
	// user, and so nCount in here will only have the value 1
	if (!bForRetranslation)
	{
		if (nPrevSequNum != -1)
		{
			if (!pPrevSrcPhrase->m_follPunct.IsEmpty())
				bPreviousFollPunct = TRUE;
			if (!pPrevSrcPhrase->GetFollowingOuterPunct().IsEmpty())
				bPreviousFollPunct = TRUE;

			if (!pPrevSrcPhrase->GetEndMarkers().IsEmpty())
				bEndMarkersPrecede = TRUE;
			if (!pPrevSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
				bEndMarkersPrecede = TRUE;
			if (!pPrevSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
				bEndMarkersPrecede = TRUE;
		}
		else
		{
			// make sure these pointers are null if we are inserting at the doc beginning
			//pPrevPile = NULL;
			pPrevSrcPhrase = NULL;
		}
		// now check the following source phrase for any preceding punct or a marker in
		// one of the three marker storage locations: m_markers, m_inlineNonbindingMarkers,
		// and m_inlineBindingMarkers
		if (!pSrcPhraseInsLoc->m_precPunct.IsEmpty())
			bFollowingPrecPunct = TRUE;

        // BEW 23Sep10, the function has not been fully converted for docVersion 5, and
        // here is another place. The legacy code only had to look at m_markers because
        // markers and filtered stuff was all in there together; but docVersion 5 has to
        // look, for the possibility of user specifying right association, at all markers
        // or information-beginnings for info within m_markers, m_endMarkers,
        // m_collectedBackTrans, or m_freeTrans (including markers for info with TextType
        // none) and the inline marker stores. There is no reason though to consider a
        // note, as placeholder insertion shouldn't move it from where it is presumably
        // most relevant. So the block below has to be replaced by more tests -- since we
        // are interested only in setting or clearing bFollowingMarkers, it would be best
        // to replace the block below with a function call to do the job --
        // IsRightAssociationTransferPossible() is it
		bFollowingMarkers = IsRightAssociationTransferPossible(pSrcPhraseInsLoc);		
		
		// if one of the flags is true, ask the user for the direction of association
		// (bAssociatingRightwards is default FALSE when control reaches here)
		if (bFollowingMarkers || bFollowingPrecPunct || bPreviousFollPunct || bEndMarkersPrecede)
		{
			// association leftwards or rightwards will be required,
			// so set the flag for this
			bAssociationRequired = TRUE;
			
			// bleed off the case when we are inserting before a temporary dummy srcphrase,
			// since in this situation association always can only be leftwards
			if (!bInsertBefore && gbDummyAddedTemporarily)
			{
				bAssociatingRightwards = FALSE;
			}
			else
			{
				// any other situation, we need to let the user make the choice
				if (wxMessageBox(_(
"Adapt It does not know whether the inserted placeholder is the end of the preceding text, or the beginning of what follows.\nIs it the start of what follows?"),
				_T(""),wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT) == wxYES)
				{
					bAssociatingRightwards = TRUE;
				}
			}
			if (bAssociatingRightwards)
			{
				// BEW 11Oct10, warn right association is not permitted when a word pair
				// joined by USFM ~ fixed space follows
				if (IsFixedSpaceSymbolWithin(pSrcPhraseInsLoc))
				{
					wxMessageBox(_(
"Two words are joined with fixed-space marker ( ~ ) following the placeholder.\nForwards association is not possible when this happens."),
_T("Warning: Unacceptable Forwards Association"),wxICON_EXCLAMATION | wxOK);
						goto m; // make no further adjustments, just jump to the 
								// RecalcLayout() call near the end
				}
				// the association is to the text which follows, so transfer from there
				// to the first in the list - but not if the first sourcephrase of a
				// retranslation follows, if it does, then tell the user what to do
				if (bFollowingMarkers)
				{
					if (pSrcPhraseInsLoc->m_bBeginRetranslation)
					{
						// can't right associate into a retranslation, so skip the block;
						// we don't allow placeholders in retranslations except as
						// end-padders; the way to fix the retranslation is not to try
						// associate a preceding placeholder to it, but to edit the
						// retranslation itself
						wxMessageBox(_(
"Trying to associate the inserted placeholder with the start of the retranslation text which follows it does not work.\nIt will be better if you now delete the placeholder and instead edit the retranslation itself."),
_T("Warning: Unacceptable Forwards Association"),wxICON_EXCLAMATION | wxOK);
						goto m; // make no further adjustments, just jump to the 
								// RecalcLayout() call near the end
					}
					// Note: although we use pFirstOne, there is only one anyway for a manual
					// insertion; doing it this way, if we ever allowed a list to be
					// inserted (but we won't), this function would work without any changes
					wxASSERT(pFirstOne != NULL); // whm added
					pFirstOne->m_markers = pSrcPhraseInsLoc->m_markers; // transfer markers
					pSrcPhraseInsLoc->m_markers.Empty();
					pFirstOne->SetInlineNonbindingMarkers(pSrcPhraseInsLoc->GetInlineNonbindingMarkers());
					pSrcPhraseInsLoc->SetInlineNonbindingMarkers(emptyStr);
					pFirstOne->SetInlineBindingMarkers(pSrcPhraseInsLoc->GetInlineBindingMarkers());
					pSrcPhraseInsLoc->SetInlineBindingMarkers(emptyStr);
					
					// right association to the beginning of a footnote makes the insertion
					// also part of the footnote, so deal with this possibility
					if (pSrcPhraseInsLoc->m_curTextType == 
						footnote && pSrcPhraseInsLoc->m_bFootnote)
					{
						pFirstOne->m_curTextType = footnote;
						// Note: moving m_bFootnote flag value is handled in next block
					}
					
					
					// have to also copy various members, such as m_inform, so navigation
					// text works right; but we don't want to copy everything - for
					// instance, we don't want to incorporate it into a retranslation; so
					// just get the essentials
					pFirstOne->m_inform = pSrcPhraseInsLoc->m_inform;
					pFirstOne->m_chapterVerse = pSrcPhraseInsLoc->m_chapterVerse;
					pFirstOne->m_bVerse = pSrcPhraseInsLoc->m_bVerse;
					// BEW 8Oct10, repurposed m_bParagraph as m_bUnused
					//pFirstOne->m_bParagraph = pSrcPhraseInsLoc->m_bParagraph;
					pFirstOne->m_bChapter = pSrcPhraseInsLoc->m_bChapter;
					pFirstOne->m_bSpecialText = pSrcPhraseInsLoc->m_bSpecialText;
					pFirstOne->m_bFootnote = pSrcPhraseInsLoc->m_bFootnote;
					pFirstOne->m_bFirstOfType = pSrcPhraseInsLoc->m_bFirstOfType;
					pFirstOne->m_curTextType = pSrcPhraseInsLoc->m_curTextType;
					
					// clear the old locations which had their values moved
					pSrcPhraseInsLoc->m_inform.Empty();
					pSrcPhraseInsLoc->m_chapterVerse.Empty();
					pSrcPhraseInsLoc->m_bFirstOfType = FALSE;
					pSrcPhraseInsLoc->m_bVerse = FALSE;
					pSrcPhraseInsLoc->m_bChapter = FALSE;
					pSrcPhraseInsLoc->m_bFootnote = FALSE;

                    // copying the m_markers member means we must transfer the flag values
                    // for the 2 booleans which could be there due to a free translation
                    // (we'll leave a note unmoved though, it's on its own m_note member if
                    // there at all, so we can just ignore it; but take care, the user may
                    // manually later move the note to the placeholder, so if we remove the
                    // placeholder at a later time with a button click in the interface
                    // then we have to check for a note on it and move it to where it
                    // should be after the placeholder has gone)
					pFirstOne->m_bHasNote = FALSE;
					pFirstOne->m_bHasFreeTrans = pSrcPhraseInsLoc->m_bHasFreeTrans;
					pFirstOne->m_bStartFreeTrans = pSrcPhraseInsLoc->m_bStartFreeTrans;

					// & if we inserted before the end of a free translation, the
					// m_bEndFreeTrans boolean's value does not move; so just clear the
					// indicator for the start of a free translation
					pSrcPhraseInsLoc->m_bStartFreeTrans = FALSE;

					// move the text of a free translation, and of a collected back
					// translation if there is one (but leave a note unmoved)
					pFirstOne->SetFreeTrans(pSrcPhraseInsLoc->GetFreeTrans());
					pFirstOne->SetCollectedBackTrans(pSrcPhraseInsLoc->GetCollectedBackTrans());
					pSrcPhraseInsLoc->SetFreeTrans(emptyStr);
					pSrcPhraseInsLoc->SetCollectedBackTrans(emptyStr);
				}
				if (bFollowingPrecPunct)
				{
					pFirstOne->m_precPunct = pSrcPhraseInsLoc->m_precPunct; // transfer 
															// the preceding punctuation
					pSrcPhraseInsLoc->m_precPunct.Empty();

					// do an adjustment of the m_targetStr member, because it will have just
					// lost its preceding punctuation (tranferred to placeholder): the simplest
					// solution is to make it same as the m_adaption member
					pSrcPhraseInsLoc->m_targetStr = pSrcPhraseInsLoc->m_adaption;
				}	
			}
			else // next block for Left-Association effects
			{
				// BEW 11Oct10, warn left association is not permitted when a word pair
				// joined by USFM ~ fixed space precedes
				if (IsFixedSpaceSymbolWithin(pPrevSrcPhrase))
				{
					wxMessageBox(_(
"Two words are joined with fixed-space marker ( ~ ) preceding the placeholder.\nBackwards association is not possible when this happens."),
_T("Warning: Unacceptable Backwards Association"),wxICON_EXCLAMATION | wxOK);
						goto m; // make no further adjustments, just jump to the 
								// RecalcLayout() call near the end
				}
				// the association is to the text which precedes, so transfer from there
				// to the last in the list
				bAssociatingRightwards = FALSE;
				if (bEndMarkersPrecede)
				{
					// these have to be moved to the placeholder, pLastOne
					endmarkersToTransfer = pPrevSrcPhrase->GetEndMarkers();
					pPrevSrcPhrase->SetEndMarkers(emptyStr);
					pLastOne->SetEndMarkers(endmarkersToTransfer);

					inlineNonbindingEndmarkersToTransfer = pPrevSrcPhrase->GetInlineNonbindingEndMarkers();
					pPrevSrcPhrase->SetInlineNonbindingEndMarkers(emptyStr);
					pLastOne->SetInlineNonbindingEndMarkers(inlineNonbindingEndmarkersToTransfer);

					inlineBindingEndmarkersToTransfer = pPrevSrcPhrase->GetInlineBindingEndMarkers();
					pPrevSrcPhrase->SetInlineBindingEndMarkers(emptyStr);
					pLastOne->SetInlineBindingEndMarkers(inlineBindingEndmarkersToTransfer);
				}
				if (bPreviousFollPunct)
				{
					// transfer the following punctuation
					pLastOne->m_follPunct = pPrevSrcPhrase->m_follPunct; 
					pPrevSrcPhrase->m_follPunct.Empty();

					pLastOne->SetFollowingOuterPunct(pPrevSrcPhrase->GetFollowingOuterPunct());
					pPrevSrcPhrase->SetFollowingOuterPunct(emptyStr);

                    // do an adjustment of the m_targetStr member (because it has just lost
                    // its following punctuation to the placeholder), simplest solution is
                    // to make it same as the m_adaption member
					pPrevSrcPhrase->m_targetStr = pPrevSrcPhrase->m_adaption;
				}	
                // left association when the text to the left is a footnote makes the
                // inserted text part of the footnote; so get the TextType set correctly
				if (pPrevSrcPhrase->m_curTextType == footnote)
				{
					pLastOne->m_bSpecialText = TRUE; // want it to have special text colour
					pLastOne->m_curTextType = footnote;
					// note: m_bFootnoteEnd is dealt with below
				}
				
                // transfer the other member's values which are pertinent to the leftwards
                // association
				pLastOne->m_bFootnoteEnd = pPrevSrcPhrase->m_bFootnoteEnd;
				pPrevSrcPhrase->m_bFootnoteEnd = FALSE;
				pLastOne->m_bBoundary = pPrevSrcPhrase->m_bBoundary;
				pPrevSrcPhrase->m_bBoundary = FALSE;

				// BEW 8Oct10, added next 2 lines because m_bParagraph has been
				// repurposed as m_bUnused
				pLastOne->m_bUnused = pPrevSrcPhrase->m_bUnused;
				pPrevSrcPhrase->m_bUnused = FALSE;
			}
		} // end of TRUE block for test: if (bFollowingMarkers || bFollowingPrecPunct || 
		  //                                 bPreviousFollPunct || bEndMarkersPrecede)
	} // end of TRUE block for test: if (!bForRetranslation)
	else // next block is for Retranslation situation
	{
        // BEW 24Sep10, simplified & corrected a logic error here that caused final
        // punctuation to not be moved to the last placeholder inserted...; and as of
        // 11Oct10 we also have to move what is in m_follOuterPunct.
        // We are inserting to pad out a retranslation, so if the last of the selected
        // source phrases has following punctuation, we need to move it to the last
        // placeholder inserted (note, the case of moving free-translation-supporting BOOL
        // values is done above, as also is the moving of the content of a final non-empty
		// m_endMarkers member, and m_inlineNonbindingEndMarkers and
		// m_inlineBindingEndMarkers to the last placeholder, in the insertion loop itself)
		if (nPrevSequNum != -1)
		{
			if (!pPrevSrcPhrase->m_follPunct.IsEmpty() || 
				!pPrevSrcPhrase->GetFollowingOuterPunct().IsEmpty())
			{
				bPreviousFollPunct = TRUE;
				// the association is to the text which precedes, so transfer from there
				// to the last in the list
				wxASSERT(pLastOne != NULL);

				// transfer following punct & clear original location
				pLastOne->m_follPunct = pPrevSrcPhrase->m_follPunct; 
				pPrevSrcPhrase->m_follPunct.Empty();
				// and also transfer following outer punct & clear original location
				pLastOne->SetFollowingOuterPunct(pPrevSrcPhrase->GetFollowingOuterPunct()); 
				pPrevSrcPhrase->SetFollowingOuterPunct(emptyStr);

				// BEW 8Oct10, added next 2 lines because m_bParagraph has been
				// repurposed as m_bUnused
				pLastOne->m_bUnused = pPrevSrcPhrase->m_bUnused;
				pPrevSrcPhrase->m_bUnused = FALSE;

				// BEW 11Oct10, if m_bBoundary is true on pPrevSrcPhrase, it needs to also
				// transfer to the end of the placeholders & be cleared at its old location
				if (pPrevSrcPhrase->m_bBoundary)
				{
					pLastOne->m_bBoundary = pPrevSrcPhrase->m_bBoundary;
					pPrevSrcPhrase->m_bBoundary = FALSE;
				}
			}
		}
	} // end of else block for test: if (!bForRetranslation)
	
    // handle any adjustments required because the insertion was done where there is one or
    // more free translation sections defined in the viscinity of the inserted
    // sourcephrase.
	// BEW 24Sep10 -- the next huge nested block is needed, it's for handling all the
	// possible interactions of where free translations may start and end, and the various
	// flags etc.
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
            // the placeholder which was just inserted (it was set where pFirstOne was set)
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
									,_T(""), wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT) == wxYES)
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
							else // the pSrcPhraseInsertLoc is not the start of a new fr tr section
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
								_T(""),wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT) == wxYES)
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
							_T(""),wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT) == wxYES)
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
							_T(""),wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT) == wxYES)
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

// A private utility function for checking for the presence of any information which has
// its start at the passed in pSrcPhrase, and which therefore potentially can have that
// information's start extended forwards to a right-associated placeholder. At the time
// this function is called it is not known if the user will request right association, but
// this function returns TRUE if one or more things should have such transfer done in the
// event that the user decides to right-associate a placeholder he's just requested be
// inserted into the document. The things to check are 1. begin-markers, 2. free
// translation, 3. collected back translation, 4. any filtered information, 5. an inline
// non-binding marker, 6. an inline binding marker; and as soon as we get a TRUE value for
// any check we return immediately, return FALSE if 1. the pSrcPhrase has ~ conjoining, or
// 2. there are no candidates; also there is no  compelling reason to transfer a note forward 
// - so ignore it
// BEW 11Oct10 updated for adding tests for the new inline marker stores, for docV5
// BEW 11Oct10 updated to prevent right association if the instance to the right is a word
// pair conjoined with USFM fixed space marker ~
bool CPlaceholder::IsRightAssociationTransferPossible(CSourcePhrase* pSrcPhrase)
{
	// prevent right association when ~ is in the CSourcePhrase instance
	if (IsFixedSpaceSymbolWithin(pSrcPhrase))
		return FALSE;
	wxString s = pSrcPhrase->m_markers;
	wxString subStr;
	int offset = wxNOT_FOUND;
	if (!s.IsEmpty())
	{
		offset = s.Find(gSFescapechar);
        // for docVersion 5 no endmarkers are now stored in m_markers, so the presence of a
        // backslash is a sufficient indication of a begin-marker, and that is potentially
        // transferrable forwards (this covers known markers, and unknown markers)
		if (offset != wxNOT_FOUND)
		{
			return TRUE;
		}
	}
	// next, check for a free translation - we'd want to make the placeholder be the start
	// of it if the placeholder gets to be right associated
	if (!pSrcPhrase->GetFreeTrans().IsEmpty() && pSrcPhrase->m_bStartFreeTrans)
	{
		return TRUE;
	}
	// next, check for filtered information
	if (!pSrcPhrase->GetFilteredInfo().IsEmpty())
	{
		return TRUE;
	}
	// check for a collected back translation (we store these at the beginning of
	// verses and so there would be a \v in m_markers anyway which should have cause a
	// return of TRUE before this, but just in case.... 
	if (!pSrcPhrase->GetCollectedBackTrans().IsEmpty())
	{
		return TRUE;
	}
	// check for an inline non-binding marker - there are only 5 of these & if we are
	// storing one it is in its own member on CSourcePhrase
	if (!pSrcPhrase->GetInlineNonbindingMarkers().IsEmpty())
	{
		return TRUE;
	}
	// check for an inline non-binding marker - there are about a score of these, we store
	// them in m_inlineBindingMarkers, a space character following each
	if (!pSrcPhrase->GetInlineBindingMarkers().IsEmpty())
	{
		return TRUE;
	}
	
    // we don't check here for content in the m_endMarkers member; because any endmarkers
    // can only transfer to a placeholder which the user left-associates to the end of the
    // text which precedes
	return FALSE;
}

/* BEW 11Oct10, this is not called anywhere, so I've commented it out
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
*/
bool CPlaceholder::NeedsTransferBackwards(CSourcePhrase* pPlaceholderSrcPhrase)
{
	if (!pPlaceholderSrcPhrase->m_bNullSourcePhrase)
		return FALSE; // it's not a placeholder
	if (!pPlaceholderSrcPhrase->m_follPunct.IsEmpty())
		return TRUE;
	if (!pPlaceholderSrcPhrase->GetEndMarkers().IsEmpty())
		return TRUE;
	if (!pPlaceholderSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
		return TRUE;
	if (!pPlaceholderSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
		return TRUE;
	if (!pPlaceholderSrcPhrase->GetFollowingOuterPunct().IsEmpty())
		return TRUE;
	return FALSE;
}

bool CPlaceholder::NeedsTransferForwards(CSourcePhrase* pPlaceholderSrcPhrase)
{
	if (!pPlaceholderSrcPhrase->m_bNullSourcePhrase)
		return FALSE; // it's not a placeholder
	if (!pPlaceholderSrcPhrase->m_precPunct.IsEmpty())
		return TRUE;
	if (!pPlaceholderSrcPhrase->m_markers.IsEmpty())
		return TRUE;
	if (!pPlaceholderSrcPhrase->GetInlineNonbindingMarkers().IsEmpty())
		return TRUE;
	if (!pPlaceholderSrcPhrase->GetInlineBindingMarkers().IsEmpty())
		return TRUE;
	return FALSE;
}

bool CPlaceholder::IsPlaceholderInSublist(SPList* pSublist)
{
	if (pSublist->IsEmpty())
		return FALSE;
	CSourcePhrase* pSP = NULL;
	SPList::Node* pos = pSublist->GetFirst();
	while (pos != NULL)
	{
		pSP = pos->GetData();
		pos = pos->GetNext();
		if (pSP->m_bNullSourcePhrase)
			return TRUE;
	}
	return FALSE;
}

// BEW added 11Oct10 for better support of docV5 within OnButtonRetranslation()
// params -- we untransfer the transfers etc, before we permit a further function to
// delete the placeholders that are within the retranslation's selection span
//   pSrcPhrase  ->  pointer to a placeholder CSourcePhrase instance which may or
//                   may not have some puncts, markers or flag changes due to
//                   transfers made because it was inserted in a context which required
//                   left or right association
void CPlaceholder::UntransferTransferredMarkersAndPuncts(SPList* pSrcPhraseList,
														 CSourcePhrase* pSrcPhrase)
{
	// in the blocks below, we first handle all the possibilities for having to transfer
	// information or span end to the preceding context; then we deal with having to
	// transfer information or span beginning to the following context in the blocks after
	// that
	wxString emptyStr = _T("");
	CSourcePhrase* pPrevSrcPhrase = NULL;
	CSourcePhrase* pSrcPhraseFollowing = NULL;
	SPList::Node* pos = NULL;
	pos = pSrcPhraseList->Find(pSrcPhrase);
	SPList::Node* savePos = pos;
	pos = pos->GetPrevious();
	if (pos != NULL)
	{
		pPrevSrcPhrase = pos->GetData();
	}
	pos = savePos;
	pos = pos->GetNext();
	if (pos != NULL)
	{
		pSrcPhraseFollowing = pos->GetData();
	}
	// untransfer for transfers made in a former left association
	if (pPrevSrcPhrase != NULL)
	{
		if (!pSrcPhrase->GetEndMarkers().IsEmpty() ||
			 !pSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty() ||
			 !pSrcPhrase->GetInlineBindingEndMarkers().IsEmpty() )
		{
			pPrevSrcPhrase->SetEndMarkers(pSrcPhrase->GetEndMarkers());
			pSrcPhrase->SetEndMarkers(emptyStr);
			pPrevSrcPhrase->SetInlineNonbindingEndMarkers(pSrcPhrase->GetInlineNonbindingEndMarkers());
			pSrcPhrase->SetInlineNonbindingEndMarkers(emptyStr);
			pPrevSrcPhrase->SetInlineBindingEndMarkers(pSrcPhrase->GetInlineBindingEndMarkers());
			pSrcPhrase->SetInlineBindingEndMarkers(emptyStr);
		}
		if (!pSrcPhrase->m_follPunct.IsEmpty() || !pSrcPhrase->GetFollowingOuterPunct().IsEmpty())
		{
			// following punction was earlier transferred, so now tranfer it back
			pPrevSrcPhrase->m_follPunct = pSrcPhrase->m_follPunct;
			pPrevSrcPhrase->SetFollowingOuterPunct(pSrcPhrase->GetFollowingOuterPunct());
			
			// now footnote span end and punctuation boundary, we can unilaterally copy
			// because if these were not changed then pPrevSrcPhrase won't originally have had
			// them set anyway, so we'd be copying FALSE over FALSE, which does no harm
			pPrevSrcPhrase->m_bFootnoteEnd = pSrcPhrase->m_bFootnoteEnd;
			pPrevSrcPhrase->m_bBoundary = pSrcPhrase->m_bBoundary;
			
			// fix the m_targetStr member (we are just fixing punctuation on the m_targetStr
			// member of pPrevSrcPhrase, so no store needed) the puncts are restored by the
			// above lines, so give it the m_adaption string and let the stored puncts get
			// converted and put in the appropriate places
			m_pView->MakeTargetStringIncludingPunctuation(pPrevSrcPhrase, pPrevSrcPhrase->m_adaption);
		}
		if (pSrcPhrase->m_bEndFreeTrans)
		{
			pPrevSrcPhrase->m_bEndFreeTrans = TRUE;
			pPrevSrcPhrase->m_bHasFreeTrans = TRUE;
		}

		if (pSrcPhrase->m_bEndFreeTrans)
		{
			pPrevSrcPhrase->m_bEndFreeTrans = TRUE;
			pPrevSrcPhrase->m_bHasFreeTrans = TRUE;
		}
	} // end TRUE block for test: if (pPrevSrcPhrase != NULL) 
	
	// now the transfers from the first, to the first of the following context

	if (pSrcPhraseFollowing != NULL)
	{
		if ((!pSrcPhrase->m_markers.IsEmpty() || !pSrcPhrase->GetInlineNonbindingMarkers().IsEmpty()
			 || !pSrcPhrase->GetInlineBindingMarkers().IsEmpty() ))
		{
			// BEW 27Sep10, for docVersion 5, m_markers stores only markers and chapter or
			// verse number, so if non-empty transfer its contents & ditto for the inline ones
			pSrcPhraseFollowing->m_markers = pSrcPhrase->m_markers;
			pSrcPhraseFollowing->SetInlineNonbindingMarkers(pSrcPhrase->GetInlineNonbindingMarkers());
			pSrcPhraseFollowing->SetInlineBindingMarkers(pSrcPhrase->GetInlineBindingMarkers());
			
			// now all the other things which depend on markers
			pSrcPhraseFollowing->m_inform = pSrcPhrase->m_inform;
			pSrcPhraseFollowing->m_chapterVerse = pSrcPhrase->m_chapterVerse;
			pSrcPhraseFollowing->m_bVerse = pSrcPhrase->m_bVerse;
			// BEW 8Oct10, repurposed m_bParagraph as m_bUnused
			//pSrcPhraseFollowing->m_bParagraph = pSrcPhrase->m_bParagraph;
			pSrcPhraseFollowing->m_bUnused = pSrcPhrase->m_bUnused;
			pSrcPhraseFollowing->m_bChapter = pSrcPhrase->m_bChapter;
			pSrcPhraseFollowing->m_bSpecialText = pSrcPhrase->m_bSpecialText;
			pSrcPhraseFollowing->m_bFootnote = pSrcPhrase->m_bFootnote;
			pSrcPhraseFollowing->m_bFirstOfType = pSrcPhrase->m_bFirstOfType;
			pSrcPhraseFollowing->m_curTextType = pSrcPhrase->m_curTextType;
		}
		// block ammended by BEW 25Jul05; preceding punctuation earlier moved forward to a
		// right-associated placeholder now must be moved back (its presence flags the fact
		// that it was transferred earlier)
		if (!pSrcPhrase->m_precPunct.IsEmpty())
		{
			pSrcPhraseFollowing->m_precPunct = pSrcPhrase->m_precPunct;
			
			// fix the m_targetStr member (we are just fixing punctuation, so no store needed)
			m_pView->MakeTargetStringIncludingPunctuation(pSrcPhraseFollowing,
														pSrcPhraseFollowing->m_adaption);
		}
		// BEW added 25Jul05 a m_bHasFreeTrans = TRUE value can be ignored provided
		// m_bStartFreeTrans value is FALSE, if the latter is TRUE, then we must move the value
		// to the following sourcephrase
		if (pSrcPhrase->m_bStartFreeTrans)
		{
			pSrcPhraseFollowing->m_bStartFreeTrans = TRUE;
			pSrcPhraseFollowing->m_bHasFreeTrans = TRUE;

			// and transfer the free translation text iteself
			pSrcPhraseFollowing->SetFreeTrans(pSrcPhrase->GetFreeTrans());
		}
		// we don't now transfer notes, so we don't have to check for them and move them, but
		// we do have to transfer any collected backtranslation (the free translation case was
		// handled above already)
		if (!pSrcPhrase->GetCollectedBackTrans().IsEmpty())
		{
			pSrcPhraseFollowing->SetCollectedBackTrans(pSrcPhrase->GetCollectedBackTrans());
		}

		// finally, information in m_filteredInfo moved forward when a placeholder was right
		// associated has to be transferred back again
		if (!pSrcPhrase->GetFilteredInfo().IsEmpty())
		{
			pSrcPhraseFollowing->SetFilteredInfo(pSrcPhrase->GetFilteredInfo());
		}
	} // end of TRUE block for test: if (pSrcPhraseFollowing != NULL)
}

// It's the caller's responsibility to ensure that any placeholders in pSublist, if they
// require information transfer to undo a previous left or right association, are not at
// the whichever end of the list would prevent that from happening.
// Called from ScanSpanDoingSourceTextReconstruction() which is in turn called from 
// OnEditSourceText()
// BEW created 11Oct10
bool CPlaceholder::RemovePlaceholdersFromSublist(SPList*& pSublist)
{
	SPList::Node* pos = pSublist->GetFirst();
	SPList::Node* savePos = NULL;
	if (pos == NULL)
		return FALSE; // something's wrong, make the caller abort the operation
	CSourcePhrase* pSrcPhrase = NULL;
	bool bHasPlaceholders = FALSE;
	// the first loop just does the untransfers that are needed; because the caller
	// ensures we don't have to do transfers to non-existing pSrcPhrase instances at
	// either end, the loop is uncomplicated
	while (pos != NULL)
	{
		pSrcPhrase = pos->GetData();
		pos = pos->GetNext();
		// BEW 2Dec13 remove placeholders, but leave free translation section wideners
		if (IsFreeTransWidener(pSrcPhrase))
		{
			// It's a widener, so skip it
			continue;
		}
		if (pSrcPhrase->m_bNullSourcePhrase)
		{
			// It's a placeholder, so transfer dataa and then 
			bHasPlaceholders = TRUE;
			UntransferTransferredMarkersAndPuncts(pSublist, pSrcPhrase);
		}
	} // end of while loop
	// now loop again, removing the placeholders, in reverse order, if any exist in the
	// sublist 
	if (bHasPlaceholders)
	{
		pos = pSublist->GetLast();
		while (pos != NULL)
		{
			savePos = pos;
			pSrcPhrase = pos->GetData();
			pos = pos->GetPrevious();
			if (IsFreeTransWidener(pSrcPhrase))
			{
				// It's a widener, so don't remove it
				continue;
			}
			if (pSrcPhrase->m_bNullSourcePhrase)
			{
				m_pApp->GetDocument()->DeleteSingleSrcPhrase(pSrcPhrase, FALSE);
				pSublist->DeleteNode(savePos);
			}
		}
	}
	return TRUE;
}


// BEW 18Feb10 updated for doc version 5 support (added code to restore endmarkers that
// were moved to the placeholder when it was inserted, ie. moved from the preceding
// CSourcePhrase instance)
// BEW 27Sep10, more changes needed for supporting docVersion 5, and some logic errors
// fixed.
// BEW 11Oct10, added docversion 5 support for m_follOuterPunct, and the 4 wxString
// members for the inline binding and non-binding begin and end markers
// BEW 2Dec13 changed so that wideners are not included in the removal
void CPlaceholder::RemoveNullSourcePhrase(CPile* pRemoveLocPile,const int nCount)
{
    // while this function used to be able to handle nCount > 1, in actual fact we have
    // only always used it for removing a manually created (single) placeholder, and so
    // nCount is always 1 on entry; 
    // 11Oct10, now I've made a change which explicitly assumes nCount is 1.
	CPile* pPile			= pRemoveLocPile;
	int nStartingSequNum	= pPile->GetSrcPhrase()->m_nSequNumber;
	SPList* pList			= m_pApp->m_pSourcePhrases;
	SPList::Node* removePos = pList->Item(nStartingSequNum); // the position at
													// which we will do the removal
	wxString emptyStr = _T("");
	SPList::Node* savePos = removePos; // we will alter removePos & need to restore it
	wxASSERT(removePos != NULL);
	int nActiveSequNum = m_pApp->m_nActiveSequNum; // save, so we can restore later on, 
	// since the call to RecalcLayout will clobber some pointers
	
    // we may be removing the m_pActivePile, so get parameters useful for setting up a
    // temporary active pile for the RecalcLayout() call below
	//int nRemovedPileIndex = pRemoveLocPile->GetSrcPhrase()->m_nSequNumber;
	int nRemovedPileIndex = nStartingSequNum;
	
    // get the preceding source phrase, if it exists, whether null or not - we may have to
    // transfer punctuation and/or an endmarker/endmarkers to it
	CSourcePhrase* pPrevSrcPhrase = NULL;
	if (nStartingSequNum > 0)
	{
		// there is a preceding one, so get it
		CPile* pPile = m_pView->GetPrevPile(pRemoveLocPile);
		wxASSERT(pPile != NULL);
		pPrevSrcPhrase = pPile->GetSrcPhrase();
		wxASSERT(pPrevSrcPhrase != NULL);
	}
	
	// ensure that there are nCount null source phrases which can be removed from this
	// location, this loop removes nothing, it does the check and also sets pointers to
	// the first and last placeholder CSourcePhrase instances to be removed
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

		// BEW 11Oct10, I've decided to give this a better behaviour. If the count value
		// goes over 1, then we'll just silently refrain from removing more than one. If
		// the count value is one, and pSrcPhrase has m_bNullSourcePhrase FALSE, we'd not
		// enter this function anyway - the Update handler would disable the command. So
		// we can eliminate the message and premature exit
		if (count > 1)
			break;
		/*
		if (!pSrcPhrase->m_bNullSourcePhrase)
		{
			//IDS_TOO_MANY_NULL_SRCPHRASES
			wxMessageBox(_T(
"Warning: you are trying to remove more placeholders than exist at the selection location: the command will be ignored."),
			_T(""),wxICON_EXCLAMATION | wxOK);
			if (m_pApp->m_selectionLine != -1)
				m_pView->RemoveSelection();
			m_pView->Invalidate();
			m_pLayout->PlaceBox();
			return;
		}
		*/
	}

	// BEW 2Dec13 added support for "free translation widener" removal
	bool bWidener = FALSE;
	if (IsFreeTransWidener(pLastOne))
	{
		bWidener = TRUE;
	}
	
    // a null source phrase can (as of version 1.3.0) be last in the document, so we can no
    // longer assume there will be a non-null one following, if we are at the end of the
    // document we must restore the active location to an earlier sourcephrase, otherwise,
    // to a following one
	bool bNoneFollows = FALSE;
	CSourcePhrase* pSrcPhraseFollowing = NULL;
	if (nStartingSequNum + nCount > m_pApp->GetMaxIndex())
	{
		// we are at the very end, or wanting to remove more at the end than is possible
		bNoneFollows = TRUE; // flag this condition
	}
	
	if (bNoneFollows)
		pSrcPhraseFollowing = NULL;
	else
	{
		// removePos is already pointing at the Node we want
		pSrcPhraseFollowing = (CSourcePhrase*)removePos->GetData();
		//removePos = removePos->GetNext(); // BEW 27Sep10, can't see a reason for this line
		wxASSERT(pSrcPhraseFollowing != NULL);
	}
	
	wxASSERT(pFirstOne != NULL); // whm added; note: if pFirstOne can ever be NULL there
	// should be more code added here to deal with it
    // if the null source phrase(s) carry any punctuation or markers, then these have to be
    // first transferred to the appropriate normal source phrase in the context, whether
    // forwards or backwards, depending on what is stored.

	// in the blocks below, we first handle all the possibilities for having to transfer
	// information or span end to the preceding context; then we deal with having to
	// transfer information or span beginning to the following context in the blocks after
	// that
	
    // docVersion 5 has endmarkers stored on the CSourcePhrase they are pertinent to, in
    // m_endMarkers, and from 11Oct10 onwards, in m_inlineNonbindingEndMarkers and possibly
    // in m_inlineBindingEndMarkers, so we search for these on pLastOne (pLastOne and
    // pFirstOne are the same pointer if nCount is 1, but if nCount is > 1 (but it will
    // never happen), we should expect that the last will carry the transferred
    // endmarker(s)), and if the member is non-empty, transfer its contents to
    // pPrevSrcPhrase (if the markers were automatically transferred to the placement
    // earlier, then pPrevSrcPhrase is guaranteed to exist)
	//if (!pFirstOne->GetEndMarkers().IsEmpty() && pPrevSrcPhrase != NULL)
	wxASSERT(pLastOne != NULL);
	if (!bWidener)
	{
		// Do the following only if it is not a widener
		if ((!pLastOne->GetEndMarkers().IsEmpty() ||
			 !pLastOne->GetInlineNonbindingEndMarkers().IsEmpty() ||
			 !pLastOne->GetInlineBindingEndMarkers().IsEmpty() )
			 && pPrevSrcPhrase != NULL)
		{
			pPrevSrcPhrase->SetEndMarkers(pLastOne->GetEndMarkers());
			pFirstOne->SetEndMarkers(emptyStr);
			pPrevSrcPhrase->SetInlineNonbindingEndMarkers(pLastOne->GetInlineNonbindingEndMarkers());
			pFirstOne->SetInlineNonbindingEndMarkers(emptyStr);
			pPrevSrcPhrase->SetInlineBindingEndMarkers(pLastOne->GetInlineBindingEndMarkers());
			pFirstOne->SetInlineBindingEndMarkers(emptyStr);
		}
		if ((!pLastOne->m_follPunct.IsEmpty() || !pLastOne->GetFollowingOuterPunct().IsEmpty())
			&& nStartingSequNum > 0 && pPrevSrcPhrase != NULL)
		{
			// following punction was earlier transferred, so now tranfer it back
			pPrevSrcPhrase->m_follPunct = pLastOne->m_follPunct;
			pPrevSrcPhrase->SetFollowingOuterPunct(pLastOne->GetFollowingOuterPunct());
			
			// now footnote span end and punctuation boundary, we can unilaterally copy
			// because if these were not changed then pPrevSrcPhrase won't originally have had
			// them set anyway, so we'd be copying FALSE over FALSE, which does no harm
			pPrevSrcPhrase->m_bFootnoteEnd = pLastOne->m_bFootnoteEnd;
			pPrevSrcPhrase->m_bBoundary = pLastOne->m_bBoundary;
			
			// fix the m_targetStr member (we are just fixing punctuation on the m_targetStr
			// member of pPrevSrcPhrase, so no store needed) the puncts are restored by the
			// above lines, so give it the m_adaption string and let the stored puncts get
			// converted and put in the appropriate places
			m_pView->MakeTargetStringIncludingPunctuation(pPrevSrcPhrase, pPrevSrcPhrase->m_adaption);
		}
		// BEW added 25Jul05, a m_bHasFreeTrans = TRUE value can be ignored provided the
		// m_bEndFreeTrans value is FALSE because pPrevSrcPhrase will have the value set
		// already; but if the latter is TRUE, then we must move the value to the preceding
		// sourcephrase
		if (pLastOne->m_bEndFreeTrans && pPrevSrcPhrase != NULL)
		{
			pPrevSrcPhrase->m_bEndFreeTrans = TRUE;
			pPrevSrcPhrase->m_bHasFreeTrans = TRUE;
		}

		// now the transfers from the first, to the first of the following context

		if ((!pFirstOne->m_markers.IsEmpty() || !pFirstOne->GetInlineNonbindingMarkers().IsEmpty()
			 || !pFirstOne->GetInlineBindingMarkers().IsEmpty() ) && !bNoneFollows)
		{
			// BEW 27Sep10, for docVersion 5, m_markers stores only markers and chapter or
			// verse number, so if non-empty transfer its contents & ditto for the inline ones
			pSrcPhraseFollowing->m_markers = pFirstOne->m_markers;
			pSrcPhraseFollowing->SetInlineNonbindingMarkers(pFirstOne->GetInlineNonbindingMarkers());
			pSrcPhraseFollowing->SetInlineBindingMarkers(pFirstOne->GetInlineBindingMarkers());
			
			// now all the other things which depend on markers
			pSrcPhraseFollowing->m_inform = pFirstOne->m_inform;
			pSrcPhraseFollowing->m_chapterVerse = pFirstOne->m_chapterVerse;
			pSrcPhraseFollowing->m_bVerse = pFirstOne->m_bVerse;
			// BEW 8Oct10, repurposed m_bParagraph as m_bUnused
			//pSrcPhraseFollowing->m_bParagraph = pFirstOne->m_bParagraph;
			pSrcPhraseFollowing->m_bUnused = pFirstOne->m_bUnused;
			pSrcPhraseFollowing->m_bChapter = pFirstOne->m_bChapter;
			pSrcPhraseFollowing->m_bSpecialText = pFirstOne->m_bSpecialText;
			pSrcPhraseFollowing->m_bFootnote = pFirstOne->m_bFootnote;
			pSrcPhraseFollowing->m_bFirstOfType = pFirstOne->m_bFirstOfType;
			pSrcPhraseFollowing->m_curTextType = pFirstOne->m_curTextType;
		}
		// block ammended by BEW 25Jul05; preceding punctuation earlier moved forward to a
		// right-associated placeholder now must be moved back (its presence flags the fact
		// that it was transferred earlier)
		if (!pFirstOne->m_precPunct.IsEmpty() && !bNoneFollows)
		{
			pSrcPhraseFollowing->m_precPunct = pFirstOne->m_precPunct;
			
			// fix the m_targetStr member (we are just fixing punctuation, so no store needed)
			m_pView->MakeTargetStringIncludingPunctuation(pSrcPhraseFollowing,
														pSrcPhraseFollowing->m_adaption);
		}
		// BEW added 25Jul05 a m_bHasFreeTrans = TRUE value can be ignored provided
		// m_bStartFreeTrans value is FALSE, if the latter is TRUE, then we must move the value
		// to the following sourcephrase
		if (pFirstOne->m_bStartFreeTrans && !bNoneFollows)
		{
			pSrcPhraseFollowing->m_bStartFreeTrans = TRUE;
			pSrcPhraseFollowing->m_bHasFreeTrans = TRUE;

			// and transfer the free translation text iteself
			pSrcPhraseFollowing->SetFreeTrans(pFirstOne->GetFreeTrans());
		}
		// we don't now transfer notes, so we don't have to check for them and move them, but
		// we do have to transfer any collected backtranslation (the free translation case was
		// handled above already)
		if (!pFirstOne->GetCollectedBackTrans().IsEmpty() && !bNoneFollows)
		{
			pSrcPhraseFollowing->SetCollectedBackTrans(pFirstOne->GetCollectedBackTrans());
		}

		// finally, information in m_filteredInfo moved forward when a placeholder was right
		// associated has to be transferred back again
		if (!pFirstOne->GetFilteredInfo().IsEmpty() && !bNoneFollows)
		{
			pSrcPhraseFollowing->SetFilteredInfo(pFirstOne->GetFilteredInfo());
		}
	} // end of TRUE block for test: if (!bWidener)
	else
	{
		// Since we only can remove a placeholder or widener one at a time, we don't need
		// a loop here. A widener will have received the m_bEndFreeTrans value as TRUE, so
		// we have to set that boolean on the previous CSourcePhrase instance so the
		// section remains valid after the widener is removed
		if (pPrevSrcPhrase != NULL)
		{
			pPrevSrcPhrase->m_bEndFreeTrans = TRUE;
		}
	}

	// remove the null source phrases from the list, after removing their 
	// translations from the KB
	removePos = savePos;
	count = 0;
	while (removePos != NULL && count < nCount)
	{
		SPList::Node* pos2 = removePos; // save current position for RemoveAt call
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)removePos->GetData();
		
		// BEW added 13Mar09 for refactored layout
		m_pView->GetDocument()->DeletePartnerPile(pSrcPhrase);
		removePos = removePos->GetNext();
		wxASSERT(pSrcPhrase != NULL);
		if (!bWidener)
		{
			m_pApp->m_pKB->GetAndRemoveRefString(pSrcPhrase,emptyStr,useGlossOrAdaptationForLookup);
		}
		count++;
        // in the next call, FALSE means 'don't delete the partner pile' (no need to
        // because we already deleted it a few lines above)
		m_pView->GetDocument()->DeleteSingleSrcPhrase(pSrcPhrase,FALSE);
		pList->DeleteNode(pos2);

		// BEW 11Oct10, limit the loop to one iteration, consistent with earlier changes
		if (count > 1)
			break;
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
	
	m_pApp->m_pKB->GetAndRemoveRefString(pSrcPhrase,emptyStr,useGlossOrAdaptationForLookup);

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
// BEW 11Oct10, updated to remove a bug (pSrcPhraseCopy deleted and then cleared to NULL
// and then searched for in the main m_pSrcPhrases list. Not a good idea to search for NULL!)
// This function is called only for removing placeholders from a retranslation - called
// only in OnButtonRetranslation() to remove any earlier placeholders in the selection
void CPlaceholder::RemoveNullSrcPhraseFromLists(SPList*& pList,SPList*& pSrcPhrases,
									int& nCount,int& nEndSequNum,
									bool bActiveLocAfterSelection,
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
		// BEW 2Dec, don't delete wideners (these have five dots, ....., as well as
		// m_bNullSrcPhrase TRUE)
		if (IsNormalPlaceholderNotWidener(pSrcPhraseCopy))
		{
            // we've found a null source phrase in the sublist, so get rid of its KB
            // presence, then delete it from the (temporary) sublist, and its instance from
            // the heap
			wxString emptyStr = _T("");
			m_pApp->m_pKB->GetAndRemoveRefString(pSrcPhraseCopy,emptyStr,
												useGlossOrAdaptationForLookup);
			delete pSrcPhraseCopy;
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
			pSrcPhraseCopy = (CSourcePhrase*)NULL; // unneeded, but harmless

			
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
	wxASSERT(pFrame != NULL);
	wxAuiToolBarItem *tbi;
	tbi = pFrame->m_auiToolbar->FindTool(ID_BUTTON_REMOVE_NULL_SRCPHRASE);
	// Return if the toolbar item is hidden
	if (tbi == NULL)
	{
		return;
	}
	// Return if this toolbar item is disabled
	if (!pFrame->m_auiToolbar->GetToolEnabled(ID_BUTTON_REMOVE_NULL_SRCPHRASE))
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
					 _T(""),wxICON_INFORMATION | wxOK);
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
		//CCell* pCell; // set but not used
		//pCell = cpos->GetData();
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

	// BEW 2Dec13 With the advent of "Free Translation (section) Wideners" we allow
	// removal of these with this button too, but we must warn the user that doing so is
	// not recommended (the free translation will then get shown truncated if he does so)
	// We'll also no allow removal if in free translation mode (otherwise we could muck
	// up a vertical edit)
	CSourcePhrase* pTheSrcPhrase = pRemoveLocPile->GetSrcPhrase();
	if (IsFreeTransWidener(pTheSrcPhrase) && !m_pApp->m_bFreeTranslationMode)
	{
		wxString title = _("Attempting removal of a free translation widener");
		wxString msg = _("Removing a free translation widener is permitted, but not recommended. If you do so, its section of free translation would be displayed truncated. Do you wish to go ahead with the removal?");
		int value = wxMessageBox(msg, title, wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT);
		if ((value == wxNO))
			return;
	}
	// Don't need another block for when it is free trans mode, because removal in 
	// free trans mode is impossible as a) we cannot put the phrasebox at the widener,
	// and b) we can't select it's source text while free trans mode is active
	//else if (IsFreeTransWidener(pTheSrcPhrase) && m_pApp->m_bFreeTranslationMode)
	//{
	//	wxString title = _("Not permitted");
	//	wxString msg = _("Removing a free translation widener is not permitted while free translation mode is active.");
	//	wxMessageBox(msg, title, wxICON_WARNING | wxOK);
	//	return;
	//}
	
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
/// of the following conditions are TRUE, this handler disables the "Remove a Placeholder,
/// or Remove a Free Translation Widener" toolbar item and returns immediately: The
/// application is in glossing mode, the target text only is showing in the main window,
/// the m_pActivePile pointer is NULL, or the application is in Free Translation mode. It
/// enables the toolbar button if there is a selection which is on a null source phrase
/// which is not a retranslation, or if the active pile is a null source phrase which is
/// not a retranslation. The selection, if there is one, takes priority, if its pile is
/// different from the active pile..
/// BEW 2Dec13, added ", or Remove a Free Translation Widener" above
/// and made similar change (also A to a in "Remove A Placeholder" in Erik's functions -
/// see CMainFrame() creator at approx line 1654 (two places), and Adapt_It.cpp in
/// ConfigureToolBarForUserProfile() at approx line 8917
/////////////////////////////////////////////////////////////////////////////////
void CPlaceholder::OnUpdateButtonRemoveNullSrcPhrase(wxUpdateUIEvent& event)
{
	// whm added 26Mar12 for read-only mode
	if (m_pApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}
	
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
		// BEW 2Dec13 This also disables the button so that a Free Translation Widener
		// cannot be removed while free translation is active - we want this to be the case
		event.Enable(FALSE);
		return;
	}
	bool bCanDelete = FALSE;
    // BEW 10Jan13, added the && !m_pApp->m_bClosingDown part of the test, to ensure
    // this block's code isn't entered when closing down and there are no CSourcePhrase
    // instances in existence (else, get a crash)
	// BEW 24Jan13, added test for a non-empty selection, because m_selectionLine can be 0
	// even when no selection is defined (e.g. doing ALT+leftarrow after a message box has
	// removed focus from the phrasebox)
	if (m_pApp->m_pTargetBox->GetHandle() != NULL && !m_pApp->m_bClosingDown)
	{
        // set the flag true either if there is a selection and which is on a null source
        // phrase which is not a retranslation, or if the active pile is a null source
        // phrase which is not a retranslation. The selection, if there is one, takes
        // priority, if its pile is different from the active pile.
        // BEW 24Jan13 added first subtest here, to make the if robust
		if (!m_pApp->m_selection.IsEmpty() && m_pApp->m_selectionLine != -1)
		{
			// First, protect against idle time update menu handler at app shutdown time, when
			// piles no longer exist
 			if (m_pApp->GetLayout()->GetPileList() == NULL ||
				m_pApp->GetLayout()->GetPileList()->IsEmpty())
			{
				event.Enable(FALSE);
				return;
			}
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
			if (focus != NULL && m_pApp->m_pTargetBox == focus) // don't use GetHandle() on m_pTargetBox here !!!
			{
				if (m_pApp->m_pActivePile != NULL && m_pApp->m_pActivePile->GetSrcPhrase() != NULL)
				{
					if (m_pApp->m_pTargetBox->IsShown()
						&& m_pApp->m_pActivePile->GetSrcPhrase()->m_bNullSourcePhrase
						&& !m_pApp->m_pActivePile->GetSrcPhrase()->m_bRetranslation)
						bCanDelete = TRUE;
				}
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
	wxASSERT(pFrame != NULL);

	wxAuiToolBarItem *tbi;
	tbi = pFrame->m_auiToolbar->FindTool(ID_BUTTON_NULL_SRC);
	// Return if the toolbar item is hidden or disabled
	if (tbi == NULL)
	{
		return;
	}
	if (!pFrame->m_auiToolbar->GetToolEnabled(ID_BUTTON_NULL_SRC))
	{
		::wxBell();
		return;
	}
	
	if (gbIsGlossing)
	{
		//IDS_NOT_WHEN_GLOSSING
		wxMessageBox(_(
					   "This particular operation is not available when you are glossing."),
					 _T(""),wxICON_INFORMATION | wxOK);
		return;
	}
	int nSequNum;
	int nCount;
	
	// Bill wanted the behaviour modified, so that if the box's m_bAbandonable flag is TRUE
	// (ie. a copy of source text was done and nothing typed yet) then the current pile
	// would have the box contents abandoned, nothing put in the KB, and then the placeholder
	// insertion - the advantage of this is that if the placeholder is inserted immediately
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
							 _T(""), wxICON_EXCLAMATION | wxOK);
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
							 _T(""), wxICON_EXCLAMATION | wxOK);
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
							 ,_T(""), wxICON_EXCLAMATION | wxOK);
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
			m_pView->MakeTargetStringIncludingPunctuation(m_pApp->m_pActivePile->GetSrcPhrase(), m_pApp->m_targetPhrase);
			
            // we are about to leave the current phrase box location, so we must try to
            // store what is now in the box, if the relevant flags allow it
			m_pView->RemovePunctuation(pDoc,&m_pApp->m_targetPhrase,from_target_text);
			gbInhibitMakeTargetStringCall = TRUE;
			bool bOK;
			bOK = m_pApp->m_pKB->StoreText(m_pApp->m_pActivePile->GetSrcPhrase(), 
											m_pApp->m_targetPhrase);
			bOK = bOK; // avoid warning
			gbInhibitMakeTargetStringCall = FALSE;
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
			posTail = posTail; // avoid warning
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
			if (pDummySrcPhrase->m_pSavedWords != NULL) // whm 11Jun12 added NULL test
				delete pDummySrcPhrase->m_pSavedWords;
			if (pDummySrcPhrase->m_pMedialMarkers != NULL) // whm 11Jun12 added NULL test
				delete pDummySrcPhrase->m_pMedialMarkers;
			if (pDummySrcPhrase->m_pMedialPuncts != NULL) // whm 11Jun12 added NULL test
				delete pDummySrcPhrase->m_pMedialPuncts;
			bool deleteOK;
			deleteOK = pSrcPhrases->DeleteNode(pSrcPhrases->GetLast());
			wxASSERT(deleteOK);
			deleteOK = deleteOK; // avoid warning (BEW 3Jan12, leave it as is, a leak is unlikely)
			if (pDummySrcPhrase != NULL) // whm 11Jun12 added NULL test
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
							 _T(""), wxICON_EXCLAMATION | wxOK);
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
							 _T(""), wxICON_EXCLAMATION | wxOK);
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
							 _T(""), wxICON_EXCLAMATION | wxOK);
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
			m_pView->MakeTargetStringIncludingPunctuation(m_pApp->m_pActivePile->GetSrcPhrase(), m_pApp->m_targetPhrase);
			
            // we are about to leave the current phrase box location, so we must try to
            // store what is now in the box, if the relevant flags allow it
			m_pView->RemovePunctuation(pDoc,&m_pApp->m_targetPhrase,from_target_text);
			gbInhibitMakeTargetStringCall = TRUE;
			bool bOK;
			bOK = m_pApp->m_pKB->StoreText(m_pApp->m_pActivePile->GetSrcPhrase(), 
											m_pApp->m_targetPhrase);
			bOK = bOK; // avoid warning
			gbInhibitMakeTargetStringCall = FALSE;
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
	// whm added 26Mar12 for read-only mode
	if (m_pApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}
	
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
	// Protect against idle time update menu handler at app shutdown time, when
	// piles no longer exist
	if (m_pApp->GetLayout()->GetPileList() == NULL ||
		m_pApp->GetLayout()->GetPileList()->IsEmpty())
	{
		event.Enable(FALSE);
		return;
	}
	bool bCanInsert = FALSE;
	if (m_pApp->m_pTargetBox != NULL)
	{
		// BEW changed 24Jan13, to make the test more robust
		if ((!m_pApp->m_selection.IsEmpty() && m_pApp->m_selectionLine != -1 ) || 
			(m_pApp->m_pTargetBox->IsShown() && (m_pApp->m_pTargetBox == wxWindow::FindFocus())))
		{
			bCanInsert = TRUE;
		}
	}
	event.Enable(bCanInsert);
}

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

#include <wx/docview.h>

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
#include "ChooseTranslation.h"
//////////

///////////////////////////////////////////////////////////////////////////////
// External globals
///////////////////////////////////////////////////////////////////////////////
extern bool gbVerticalEditInProgress;
extern bool gbIsGlossing;
extern bool gbShowTargetOnly;
extern EditRecord gEditRecord;
extern wxChar gSFescapechar;
/*
// The following CPlaceholderInsertDlg class was not needed after making the
// insertion of placeholders directional to left/right of selection/phrasebox
BEGIN_EVENT_TABLE(CPlaceholderInsertDlg, AIModalDialog)
    EVT_INIT_DIALOG(CPlaceholderInsertDlg::InitDialog)
    EVT_BUTTON(wxID_YES, CPlaceholderInsertDlg::OnButtonYes)
    EVT_BUTTON(wxID_NO, CPlaceholderInsertDlg::OnButtonNo)
    EVT_BUTTON(wxID_CANCEL, CPlaceholderInsertDlg::OnButtonCancel)
    EVT_KEY_DOWN(CPlaceholderInsertDlg::OnKeyDown)
    EVT_CHAR(CPlaceholderInsertDlg::OnKeyDownChar)
    EVT_CLOSE(CPlaceholderInsertDlg::OnClose)
END_EVENT_TABLE()


// a helper class for CPlaceholder
CPlaceholderInsertDlg::CPlaceholderInsertDlg(wxWindow* parent)
    : AIModalDialog(parent, -1, _("Placeholder Question"),
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    PlaceholderInsertDlgFunc(this, TRUE, TRUE); // wxDesigner dialog function

    // whm 14Mar2020 Note: The PlaceholderInsertDlgFunc() dialog only has a "YES" (wxID_OK) button
    // and a "NO" button . The "YES" button is the default button.
    // aligned right at the bottom of the dialog. We need not use wxStdDialogButtonSizer, nor
    // the ReverseOkCancelButtonsForMac() function in this case.

    pYesBtn = (wxButton*)FindWindowById(wxID_YES);
    wxASSERT(pYesBtn != NULL);
    pNoBtn = (wxButton*)FindWindowById(wxID_NO);
    wxASSERT(pNoBtn != NULL);
    pApp = &wxGetApp();
}

CPlaceholderInsertDlg::~CPlaceholderInsertDlg() // destructor
{
}

void CPlaceholderInsertDlg::InitDialog(wxInitDialogEvent & WXUNUSED(event))
{
    pYesBtn->SetFocus();
}

void CPlaceholderInsertDlg::OnClose(wxCloseEvent& event)
{
    event.Skip();
}

void CPlaceholderInsertDlg::OnButtonCancel(wxCommandEvent & WXUNUSED(event))
{
    EndModal(wxID_CANCEL);
}

void CPlaceholderInsertDlg::OnButtonYes(wxCommandEvent& event)
{
    //pApp->b_Spurious_Enter_Tab_Propagated = TRUE;
    event.Skip();
    EndModal(wxID_YES); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void CPlaceholderInsertDlg::OnButtonNo(wxCommandEvent& event)
{

    //pApp->b_Spurious_Enter_Tab_Propagated = TRUE;
    event.Skip();
    EndModal(wxID_NO); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void CPlaceholderInsertDlg::OnKeyDown(wxKeyEvent & event)
{
    // Why does this handler never get triggered???
    int keycode = event.GetKeyCode();
    if (keycode == WXK_RETURN
        || keycode == WXK_NUMPAD_ENTER)
    {
        event.StopPropagation();
    }
    event.Skip();
}

void CPlaceholderInsertDlg::OnKeyDownChar(wxKeyEvent & event)
{
    // Why does this handler never get triggered???
    int keycode = event.GetKeyCode();
    if (keycode == WXK_RETURN
        || keycode == WXK_NUMPAD_ENTER)
    {
        event.StopPropagation();
    }
    event.Skip();
}
*/

///////////////////////////////////////////////////////////////////////////////
// Event Table
///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CPlaceholder, wxEvtHandler)
EVT_TOOL(ID_BUTTON_REMOVE_NULL_SRCPHRASE, CPlaceholder::OnButtonRemoveNullSrcPhrase)
EVT_UPDATE_UI(ID_BUTTON_REMOVE_NULL_SRCPHRASE, CPlaceholder::OnUpdateButtonRemoveNullSrcPhrase)
EVT_UPDATE_UI(ID_BUTTON_NULL_SRC_RIGHT, CPlaceholder::OnUpdateButtonNullSrc)
EVT_UPDATE_UI(ID_BUTTON_NULL_SRC_LEFT, CPlaceholder::OnUpdateButtonNullSrc)
//#if defined(_PHRefactor)
EVT_TOOL(ID_BUTTON_NULL_SRC_LEFT, CPlaceholder::OnButtonNullSrcLeft)
EVT_TOOL(ID_BUTTON_NULL_SRC_RIGHT, CPlaceholder::OnButtonNullSrcRight) 
//#else
//EVT_TOOL(ID_BUTTON_NULL_SRC_LEFT, CPlaceholder::OnButtonNullSrc)
//EVT_TOOL(ID_BUTTON_NULL_SRC_RIGHT, CPlaceholder::OnButtonNullSrc) // call the same OnButtonNullSrc function used for ID_...LEFT above
//#endif
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

    // whm 21Feb2018 moved and renamed gbDummyAddedTemporarily to become a private member
    // of CPlaceholder, initialized here.
    // TRUE if an null sourcephrase is to be inserted
    // after the sel'n or after the active location, when the either of those are at
    // the GetMaxIndex() location (we use InsertNullSrcPhrase() which always inserts
    // before a location, so we have to add a dummy at the end until the insert is
    // done, and then remove it.
    m_bDummyAddedTemporarily = FALSE;
}

CPlaceholder::~CPlaceholder()
{
	
}


///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

// BEW 18Feb10 updated for doc version 5 support (no changes needed)
// BEW 21Jul14, no change needed for support of ZWSP etc (internally it calls
// InsertNullSourcePhrase() which has the refactored code for support of ZWSP)
// The InsertNullSrcPhraseBefore() function is only called from CPhraseBox::OnSysKeyUp()
// to handle the SHIFT+ALT+LeftArrow combination key command to insert a placeholder
// BEFORE a selection/phrasebox location.
void CPlaceholder::InsertNullSrcPhraseBefore()
{
	// first save old sequ num for active location
    m_pApp->m_nOldSequNum = m_pApp->m_nActiveSequNum;
	
    // find the pile preceding which to do the insertion - it will either be preceding the
    // first selected pile, if there is a selection current, or preceding the active
    // location if no selection is current
	CPile* pInsertLocPile;
	CAdapt_ItDoc* pDoc = m_pView->GetDocument();
	wxASSERT(pDoc != NULL);
	int nCount = 1;
	int nSequNum = -1;
	wxString SelOrNoSel; SelOrNoSel.Empty();
	if (m_pApp->m_selectionLine != -1)
	{
		// we have a selection, the pile we want is that of the selection list's 
		// first element
		SelOrNoSel = _T("Selection");
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
		// whm 30Dec2024 commented out the RemoveSelection() call here. Instead, call it
		// later within the InsertNullSrcPhrase() function right after the dialog is show.
		// It is helpful to retain the selection when that dialog pops up.
		//m_pView->RemoveSelection(); // Invalidate will be called in InsertNullSourcePhrase()
	}
	else
	{
		// no selection, so just insert preceding wherever the phraseBox 
		// currently is located
		SelOrNoSel = _T("NoSelection");
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

            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;

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
        m_pApp->m_bInhibitMakeTargetStringCall = TRUE;
		bool bOK = TRUE;
		bOK = m_pApp->m_pKB->StoreText(m_pApp->m_pActivePile->GetSrcPhrase(), 
						m_pApp->m_targetPhrase);
		bOK = bOK; // avoid gcc set but not used warning
        m_pApp->m_bInhibitMakeTargetStringCall = FALSE;
	}
	
    m_pApp->m_bMovingToDifferentPile = TRUE; // whm 22Mar2018 added
	InsertNullSourcePhrase(pDoc, pInsertLocPile, nCount, SelOrNoSel); // whm 30Dec2024 added SelOrNoSel paramter
    m_pApp->m_bMovingToDifferentPile = FALSE; // whm 22Mar2018 added

	// jump to it (can't use old pile pointers, the recalcLayout call will have 
	// clobbered them)
	CPile* pPile = m_pView->GetPile(nSequNum);
	m_pView->Jump(m_pApp, pPile->GetSrcPhrase());
}

// BEW 18Feb10 updated for doc version 5 support (no changes needed)
// BEW 21Jul14, no change needed for support of ZWSP etc (internally it calls
// InsertNullSourcePhrase() which has the refactored code for support of ZWSP)
// The InsertNullSrcPhraseAfter() function is only called from CPhraseBox::OnSysKeyUp()
// to handle the SHIFT+ALTRightArrow combination key command to insert a placeholder
// AFTER a selection/phrasebox location.
void CPlaceholder::InsertNullSrcPhraseAfter() 
{
	// this function is public (called in PhraseBox.cpp)
	int nSequNum;
	int nCount;
	CAdapt_ItDoc* pDoc = m_pView->GetDocument();
	
	// CTRL key is down, so an "insert after" is wanted
	// first save old sequ num for active location
    m_pApp->m_nOldSequNum = m_pApp->m_nActiveSequNum;
	
	// find the pile after which to do the insertion - it will either be after the
	// last selected pile if there is a selection current, or after the active location
	// if no selection is current - beware the case when active location is a doc end!
	CPile* pInsertLocPile;
	nCount = 1; // the button or shortcut can only insert one
	nSequNum = -1;
	wxString SelOrNoSel; SelOrNoSel.Empty();
	if (m_pApp->m_selectionLine != -1)
	{
		// we have a selection, the pile we want is that of the selection list's last element
		SelOrNoSel = _T("Selection");
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
		// whm 30Dec2024 commented out the RemoveSelection() call here. Instead, call it
		// later within the InsertNullSrcPhrase() function right after the dialog is show.
		// It is helpful to retain the selection when that dialog pops up.
		//m_pView->RemoveSelection(); // Invalidate will be called in InsertNullSourcePhrase()
	}
	else
	{
		// no selection, so just insert after wherever the phraseBox currently is located
		SelOrNoSel = _T("NoSelection");
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

            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;

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
        m_pApp->m_bInhibitMakeTargetStringCall = TRUE;
		bool bOK = TRUE;
		bOK = m_pApp->m_pKB->StoreText(m_pApp->m_pActivePile->GetSrcPhrase(), m_pApp->m_targetPhrase);
		bOK = bOK; // avoid gcc set but not used warning
		m_pApp->m_bInhibitMakeTargetStringCall = FALSE;
	}
	
    // at this point, we need to increment the pInsertLocPile pointer to the next pile, and
    // the nSequNum to it's sequence number, since InsertNullSourcePhrase() always inserts
    // "before" the pInsertLocPile; however, if the selection end, or active location if
    // there is no selection, is at the very end of the document (ie. last sourcephrase),
    // there is no following source phrase instance to insert before. If this is the case,
    // we have to append a dummy sourcephrase at the end of the document, do the insertion,
    // and then remove it again; and we will also have to set (and later clear)  
    // m_bDummyAddedTemporarily because this is used in the function in order to force a
    // leftwards association only (and hence the user does not have to be asked whether to
    // associate right or left, if there is final punctuation)
	SPList* pSrcPhrases = m_pApp->m_pSourcePhrases;
	CSourcePhrase* pDummySrcPhrase = NULL; // whm initialized to NULL
	if (nSequNum == m_pApp->GetMaxIndex())
	{
		// a dummy is temporarily required
		m_bDummyAddedTemporarily = TRUE;
		
		// do the append
		pDummySrcPhrase = new CSourcePhrase;
		wxASSERT(pDummySrcPhrase != NULL);
		pDummySrcPhrase->m_srcPhrase = _T("dummy"); // something needed, so a pile width can
		// be computed
		pDummySrcPhrase->m_key = pDummySrcPhrase->m_srcPhrase;
		pDummySrcPhrase->m_nSequNumber = m_pApp->GetMaxIndex() + 1;
		SPList::Node* posTail = NULL;
		posTail = pSrcPhrases->Append(pDummySrcPhrase);
		posTail = posTail; // avoid gcc set but not used warning
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
	
    m_pApp->m_bMovingToDifferentPile = TRUE; // whm 22Mar2018 added
	InsertNullSourcePhrase(pDoc,pInsertLocPile,nCount,SelOrNoSel, // whm 30Dec2024 added SelOrNoSel parameter
		TRUE,FALSE,FALSE); // here, never for
	// Retransln if we inserted a dummy, now get rid of it and clear the global flag
    m_pApp->m_bMovingToDifferentPile = FALSE; // whm 22Mar2018 added
	if (m_bDummyAddedTemporarily)
	{
        m_bDummyAddedTemporarily = FALSE;
		
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
// BEW 21Jul14, refactored for support of ZWSP etc
// This InsertNullSourcePhrase() function is called from:
//   CPlaceholder::OnButtonNullSrc() handler - two places
//   CPlaceholder::InsertNullSrcPhraseBefore()
//   CPlaceholder::InsertNullSourcePhraseAfter()
//   CRetranslation::PadWithNullSourcePhrasesAtEnd() - two places
void CPlaceholder::InsertNullSourcePhrase(CAdapt_ItDoc* pDoc,
										   CPile* pInsertLocPile,const int nCount,
										   wxString SelOrNoSel, // whm 30Dec2024 added
										   bool bRestoreTargetBox,bool bForRetranslation,
										   bool bInsertBefore)
{
	bool bAssociatingRightwards = FALSE;
	CSourcePhrase* pSrcPhrase = NULL; // whm initialized to NULL
	CSourcePhrase* pPrevSrcPhrase = NULL; // the sourcephrase which lies before the first 
	// inserted ellipsis, in a retranslation we have to check this one for an 
	// m_bEndFreeTrans being TRUE flag and move that bool value to the end of the
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
		if (SelOrNoSel == _T("Selection"))
		{
			m_pView->RemoveSelection(); // whm 30Dec2024 added
		}
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

	// BEW 21Jul14, Set a default src text wordbreak string for the placeholder
	// - get it from the previous instance
	bool bWordBreakDifference = FALSE; // default
	wxString defaultWordBreak = _T(" "); // a space, in case there is no previous CSourcePhrase
	if (pPrevSrcPhrase != NULL)
	{
		defaultWordBreak = pPrevSrcPhrase->GetSrcWordBreak();
	}

	// get the sequ num for the insertion location 
	// (it could be quite diff from active sequ num)
	CSourcePhrase* pSrcPhraseInsLoc = pInsertLocPile->GetSrcPhrase();
	int	nSequNumInsLoc = pSrcPhraseInsLoc->m_nSequNumber;
	wxASSERT(nSequNumInsLoc >= 0 && nSequNumInsLoc <= m_pApp->GetMaxIndex());
	// BEW 21Jul14 get the pile after the insert loc, and its CSourcePhrase so we can do
	// transfers to the m_srcWordBreak contents in a right-association context
	CPile* pPostInsertLocPile = m_pView->GetNextPile(pInsertLocPile); // will return NULL if
									// the pInsertLocPile is at document's end
	CSourcePhrase* pPostInsertLocSrcPhrase = NULL;
	if (pPostInsertLocPile != NULL)
	{
		pPostInsertLocSrcPhrase = pPostInsertLocPile->GetSrcPhrase();
	}
	
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

		// BEW 21Jul14, preceding word-delimiter for each CSourcePhrase is an issue.
		// Normally it is a space. However, for some SE Asian scripts it may be a space on
		// the first instance, followed by a series of words with ZWSP delimitation. There
		// may also quite likely be no marker, punctuation, or other information on the
		// first following instance that triggers right association; however, the latin space on
		// preceding the first instance if the second an subsequent instances have
		// non-latin-space delimitation may require right association of the latin space
		// to the placeholder, and copying forward of the next-to-first's (different)
		// word break to the first instance. But if left associated, we don't do that,
		// we'd have to transfer a copy of the preceding source phrase's word break
		// forwards to the inserted placeholder. So we need an extra test and a boolean to
		// save the results of the test. Then we'll include the boolean into the left or
		// right association blocks below. But first we must give the placeholder a default
		// m_srcWordBreak - we can't assume it is a space, so instead, since default
		// association is to associate leftwards, we'll copy the previous CSourcePhrase's
		// m_srcWordBreak to the placeholder. Then, our test will be to check if what's
		// the placeholder's wordbreak is different than what follows. If so, we have to
		// ask the user to define which way the association is to be - left or right.
		wxString nextWordBreak = _T(" "); // default space in case pSrcPhraseInsLoc is NULL
		if (pSrcPhraseInsLoc != NULL)
		{
			nextWordBreak = pSrcPhraseInsLoc->GetSrcWordBreak();
		}
		// Make our test:
		if (defaultWordBreak != nextWordBreak)
		{
			bWordBreakDifference = TRUE;
		}
        // Okay, given the above explanation, for right association the m_srcWordBreak on
        // pSrcPhraseInsLoc has to be copied to the placeholder - it could be a space being
        // transferred, and what follows that instance may be ZWSP delimiters - so we can't
        // just leave the pSrcPhraseInsLoc's m_srcWordBreak unchanged - what we have to do
        // is to copy the next instance's (pPostInsertLocSrcPhrase's) to pSrcPhraseInsLoc's
        // m_srcWordBreak to keep any potential chain of ZWSP delimiters intact.
		// But for left association, we copy the previous instance's delimiter forwards to
		// the placeholder, and change nothing in the delimiters which follow the placeholder

		// If one of the flags is true, ask the user for the direction of association
		// (bAssociatingRightwards is default FALSE when control reaches here)
		if (bFollowingMarkers || bFollowingPrecPunct || bPreviousFollPunct || bEndMarkersPrecede
			|| bWordBreakDifference)
		{
			// association leftwards or rightwards will be required,
			// so set the flag for this
			bAssociationRequired = TRUE;
			
			// bleed off the case when we are inserting before a temporary dummy srcphrase,
			// since in this situation association always can only be leftwards
			if (!bInsertBefore && m_bDummyAddedTemporarily)
			{
				bAssociatingRightwards = FALSE;
			}
            else
            {
                // whm 24Mar2020 PROBLEM identified: If the user clicks the default YES button with the
                // mouse to associate rightwards, the phrasebox ends up being successfully placed
                // at the location of the placeholder. However, if the user presses Enter/Tab to
                // accept the default value of YES, the Enter/Tab key event gets propagated to the
                // OnKeyUp() handler in CPhraseBox, causing the phrasebox to move forward to the 
                // next location (in Review mode, or some further hole location in Drafting mode.
                // Simply using wxMessageDialog() instead of wxMessageBox() doesn't always stop
                // the spurious Enter/Tab key event from propagating to OnKeyUp().
                // Also using a custom dialog class does not prevent the propagation of a spuriouus
                // Enter key event. 
                // I also tried using a bool value on the App to detect when a spurious Enter key 
                // might get propagated that when the default Yes is made by Enter key press, so that
                // the bool can be tested within the OnKeyUp() handler's Enter/Tab code block.
                // This can be made to work for first instance, but then there is no place that I
                // could find to reset the bool value back to FALSE, resulting in the necessity of
                // pressing Enter twice for the next move of the phrasebox.
                // Therefore, I've added back the wxMessageDialog with the likelyhood that the
                // user who uses the keyboard to accept Yes via Enter key press will experience the
                // nuisance moving along of the phrasebox after responding to the message prompt.
                //
                // any other situation, we need to let the user make the choice
                // whm 20Mar2020 modified. Due to above mentioned problems the team decided to provide
                // two tool bar buttons (along with the two hot key short-cuts) to allow the user to
                // specify the left/right location of insertions in relation to the selection/phrasebox location.
                // For now, since the query of user below seemed to always appear when the user wants the
                // inserted placeholder to go to the left of the selection/phrasebox - i.e., "rightwards association",
                // I will remove the query below and for this block just set the bAssociatingRightwards = TRUE.
                // Note also that the use of the App's b_Spurious_Enter_Tab_Propagated global flag is no longer
                // present.
                // Note: a different parent such as m_pApp->m_pTargetBox doesn't prevent propagation of the Enter key event
                // Can't make the m_pTargetBox the parent with dlg.Center() call below places the dialog directly over the
                // phraseBox!
				/*
                CPlaceholderInsertDlg dlg(m_pApp->GetMainFrame());
                dlg.Center();
                int result;
                result = dlg.ShowModal();
                if (result == wxID_YES)
                {
                    wxLogDebug(_T("User says bAssociatingRightwards is TRUE..."));
                    bAssociatingRightwards = TRUE;
                    //m_pApp->b_Spurious_Enter_Tab_Propagated = TRUE;
                }
                else if (result == wxID_NO)
                {
                    wxLogDebug(_T("User says bAssociatingRightwards is FALSE..."));
                    wxASSERT(bAssociatingRightwards == FALSE);
                }
                else // result == wxID_CANCEL
                {
                    wxASSERT(result == wxID_CANCEL);
                    // User wants to abort the process of inserting the placeholder.
                    // TODO: BEW needs to review this and undo anything that needs to roll back that
                    // may have been done above such as the addition of a dummy source phrase, when
                    // m_bDummyAddedTemporarily is TRUE
                    return;
                }
				*/

// BEW 25Mar20 refactor && prevent spurious phrasebox jump ahead


                // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                m_pApp->m_bUserDlgOrMessageRequested = TRUE;
				// whm revised dialog text below to be what BEW suggested in email of 30Dec2024, but with a substitution 
				// of either "selection" or "phrasebox" which depends on the incoming paramter value SelOrNoSel.
				wxString msg;
				if (SelOrNoSel == _T("Selection"))
					msg = _("Adapt It will put the new placeholder in one of two places. You must choose where you want it to be. Do you want it to be at the start of the words which follow the selection? If you do, press Yes. If you press No, it will be put after the selection.");
				else // SelOrNoSel is "NoSelection" or empty string
					msg = _("Adapt It will put the new placeholder in one of two places. You must choose where you want it to be. Do you want it to be at the start of the words which follow the phrasebox? If you do, press Yes. If you press No, it will be put after the phrasebox.");

                wxMessageDialog dlg(NULL,msg,
                    _("Adapt It needs more information"),wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);
                if (dlg.ShowModal() == wxID_YES)
                {
                    wxLogDebug(_T("User says bAssociatingRightwards is TRUE..."));
                    bAssociatingRightwards = TRUE;
                    //m_pApp->b_Spurious_Enter_Tab_Propagated = TRUE;

/* ** test ** */	
                }
                else
                {
                    wxLogDebug(_T("User says bAssociatingRightwards is FALSE..."));
                }

                // whm Note: the wxMessageBox() prompt below was the original coding for prompting the user.
//				if (wxMessageBox(_(
//"Adapt It does not know whether the inserted placeholder is the end of the preceding text, or the beginning of what follows.\nIs it the start of what follows?"),
//				_T(""),wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT) == wxYES)
//				{
//					bAssociatingRightwards = TRUE;
//				}
			}
			if (bAssociatingRightwards)
			{
				// BEW 11Oct10, warn right association is not permitted when a word pair
				// joined by USFM ~ fixed space follows
				if (IsFixedSpaceSymbolWithin(pSrcPhraseInsLoc))
				{

                    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                    m_pApp->m_bUserDlgOrMessageRequested = TRUE;

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

                        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                        m_pApp->m_bUserDlgOrMessageRequested = TRUE;

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
					// BEW 8Oct10, repurposed m_bParagraph as m_bUnused <-- BEW Mar20, it's now repurposed for TRUE saying m_punctsPattern
					// having USFM3 squirreled away attributes metadata substring
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
				// BEW 21Jul14, Do any needed right association for wordbreaks
				wxString aWordBreak = _T("");
				aWordBreak = pSrcPhraseInsLoc->GetSrcWordBreak();
				pFirstOne->SetSrcWordBreak(aWordBreak);
				// now copy the following one forward as explained above
				aWordBreak = pPostInsertLocSrcPhrase->GetSrcWordBreak();
				pSrcPhraseInsLoc->SetSrcWordBreak(aWordBreak);
			}
			else // next block for Left-Association effects
			{
				// BEW 11Oct10, warn left association is not permitted when a word pair
				// joined by USFM ~ fixed space precedes
				if (IsFixedSpaceSymbolWithin(pPrevSrcPhrase))
				{

                    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                    m_pApp->m_bUserDlgOrMessageRequested = TRUE;

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

                // BEW 21Jul14, for left association, the m_srcWordBreak on pPrevSrcPhrase
                // has to be copied onwards to the placeholder (pLastOne)
				wxString aWordBreak = _T("");
				aWordBreak = pPrevSrcPhrase->GetSrcWordBreak();
				pLastOne->SetSrcWordBreak(aWordBreak);
			}
		} // end of TRUE block for test: if (bFollowingMarkers || bFollowingPrecPunct || 
		  //                                 bPreviousFollPunct || bEndMarkersPrecede)
		else
		{
			// Associated neither left nor right, so give the placeholder the default
			// wordbreak set near top
			pLastOne->SetSrcWordBreak(defaultWordBreak);
		}

	} // end of TRUE block for test: if (!bForRetranslation)
	else // next block is for Retranslation situation
	{
        // BEW 24Sep10, simplified & corrected a logic error here that caused final
        // punctuation to not be moved to the last placeholder inserted...; and as of
        // 11Oct10 we also have to move what is in m_follOuterPunct.
        // We are inserting to pad out a retranslation, so if the last of the selected
        // source phrases has following punctuation, we need to move it to the last
        // placeholder inserted (note, the case of moving free-translation-supporting bool
        // values is done above, as also is the moving of the content of a final non-empty
		// m_endMarkers member, and m_inlineNonbindingEndMarkers and
		// m_inlineBindingEndMarkers to the last placeholder, in the insertion loop itself)
		// 
		// BEW 21Jul14, in this situation, no moving of m_srcWordBreak contents is
        // involved here - because the padding instances get the last CSourcePhrase's
        // m_scrWordBreak contents copied to them in the caller when the padding is done.
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

                                    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                                    m_pApp->m_bUserDlgOrMessageRequested = TRUE;

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

                                // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                                m_pApp->m_bUserDlgOrMessageRequested = TRUE;

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

                            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                            m_pApp->m_bUserDlgOrMessageRequested = TRUE;

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

                            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                            m_pApp->m_bUserDlgOrMessageRequested = TRUE;

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

	// whm 30Dec2024 added. We delayed the RemoveSelection() call until after any 
	// display of the dialog asking "Yes" or "No" for placeholder insertion.
	if (SelOrNoSel == _T("Selection"))
	{
		m_pView->RemoveSelection();
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
        // whm 13Aug2018 Note: The SetFocus() correctly precedes the 
        // SetSelection(m_pApp->m_nStartChar, m_pApp->m_nEndChar) call below it.
		m_pApp->m_pTargetBox->GetTextCtrl()->SetFocus();
        // whm 3Aug2018 Note: The following SetSelection restores any previous
        // selection, so no adjustment for 'Select Copied Source' needed here.
		m_pApp->m_pTargetBox->GetTextCtrl()->SetSelection(m_pApp->m_nStartChar, m_pApp->m_nEndChar); 
		
		// scroll into view, just in case a lot were inserted
		m_pApp->GetMainFrame()->canvas->ScrollIntoView(m_pApp->m_nActiveSequNum);

		m_pView->Invalidate();
		m_pLayout->PlaceBox();
	}

    // whm 21May2020 added - make the doc dirty so it can be samed after insertion of placeholder
    pDoc->Modify(TRUE);

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
// BEW 21Jul14, no change needed for ZWSP support - right association issues in the context
// of ZWSP wordbreaks are handled by either RemoveNullSourcePhrase() or 
// InsertNullSourcePhrase() internally, so we don't need to do anything here
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
        // for docVersion 5 no endmarkers are not stored in m_markers, so the presence of a
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
	SPList::Node* pos_pSubList = pSublist->GetFirst();
	while (pos_pSubList != NULL)
	{
		pSP = pos_pSubList->GetData();
		pos_pSubList = pos_pSubList->GetNext();
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
// BEW 21Jul14, refactored for support of ZWSP
void CPlaceholder::UntransferTransferredMarkersAndPuncts(SPList* pSrcPhraseList,
														 CSourcePhrase* pSrcPhrase)
{
	// in the blocks below, we first handle all the possibilities for having to transfer
	// information or span end to the preceding context; then we deal with having to
	// transfer information or span beginning to the following context in the blocks after
	// that
	// BEW 21Jul14, if there was left association of the placeholder, a ZWSP may have been
	// copied to the placeholder. Since the placeholder is going to be deleted, nothing
	// needs to be done to pPrevSrcPhrase's m_srcWordBreak, it's already correct. In the
	// case of right association, however, there may have been a chain of ZWSP
	// delimitations with a space delimitation at the start, and the space transferred to
	// the placeholder being right-associated. So we can't ignore this possibility. We
	// have to transfer the placeholder's m_srcWordBreak value to pSrcPhraseFolling, just
	// in case it is a different type of space than the other following ones.
	wxString emptyStr = _T("");
	CSourcePhrase* pPrevSrcPhrase = NULL;
	CSourcePhrase* pSrcPhraseFollowing = NULL;
	SPList::Node* pos_pSPList = NULL;
	pos_pSPList = pSrcPhraseList->Find(pSrcPhrase);
	SPList::Node* savePos = pos_pSPList;
	pos_pSPList = pos_pSPList->GetPrevious();
	if (pos_pSPList != NULL)
	{
		pPrevSrcPhrase = pos_pSPList->GetData();
	}
	pos_pSPList = savePos;
	pos_pSPList = pos_pSPList->GetNext();
	if (pos_pSPList != NULL)
	{
		pSrcPhraseFollowing = pos_pSPList->GetData();
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
	// BEW 21Jul14, it's only here that we need to transfer m_srcWordBreak, and only forward -
	// but we do the transfer of forwards only if the placeholder's m_srcWordBreak is not
	// equal to whatever is pSrcPhraseFollowing's m_srcWordBreak

	if (pSrcPhraseFollowing != NULL)
	{
		// BEW 21Jul14 - first do any ZWSP supporting transfer that may be needed; we
		// do it here because we can't assume that other right-associating factors (than a
		// difference in the wordbreak character) were present
		if (pSrcPhrase->GetSrcWordBreak() != pSrcPhraseFollowing->GetSrcWordBreak())
		{
			pSrcPhraseFollowing->SetSrcWordBreak(pSrcPhrase->GetSrcWordBreak());
		}

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
// BEW 21Jul14, no change need for ZWSP support - the internal call of Untransfer...()
// has been refactored for support of ZWSP untransfers etc.
bool CPlaceholder::RemovePlaceholdersFromSublist(SPList*& pSublist)
{
	SPList::Node* pos_pSubList = pSublist->GetFirst();
	SPList::Node* savePos = NULL;
	if (pos_pSubList == NULL)
		return FALSE; // something's wrong, make the caller abort the operation
	CSourcePhrase* pSrcPhrase = NULL;
	bool bHasPlaceholders = FALSE;
	// the first loop just does the untransfers that are needed; because the caller
	// ensures we don't have to do transfers to non-existing pSrcPhrase instances at
	// either end, the loop is uncomplicated
	while (pos_pSubList != NULL)
	{
		pSrcPhrase = pos_pSubList->GetData();
		pos_pSubList = pos_pSubList->GetNext();
		if (pSrcPhrase->m_bNullSourcePhrase)
		{
			// It's a placeholder, so transfer data & set flag for the block which follows 
			bHasPlaceholders = TRUE;
			UntransferTransferredMarkersAndPuncts(pSublist, pSrcPhrase);
		}
	} // end of while loop
	// now loop again, removing the placeholders, in reverse order, if any exist in the
	// sublist 
	if (bHasPlaceholders)
	{
		pos_pSubList = pSublist->GetLast();
		while (pos_pSubList != NULL)
		{
			savePos = pos_pSubList;
			pSrcPhrase = pos_pSubList->GetData();
			pos_pSubList = pos_pSubList->GetPrevious();
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
// BEW 21Jul14, refactored for support of ZWSP and other exotic wordbreaks
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

	// BEW 21Jul14, unilaterally check for the wordbreak on the placeholder being different
	// than the one on pSrcPhraseFollowing, if different, then transfer the placeholder's
	// wordbreak to pSrcPhraseFollowing
	if (pSrcPhraseFollowing != NULL && 
		(pFirstOne->GetSrcWordBreak() != pSrcPhraseFollowing->GetSrcWordBreak()))
	{
		pSrcPhraseFollowing->SetSrcWordBreak(pFirstOne->GetSrcWordBreak());
	}

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

	// remove the null source phrases from the list, after removing their 
	// translations from the KB
	removePos = savePos;
	count = 0;
	while (removePos != NULL && count < nCount)
	{
		SPList::Node* pos_toRemove = removePos; // save current position for RemoveAt call
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)removePos->GetData();
		
		// BEW added 13Mar09 for refactored layout
		m_pView->GetDocument()->DeletePartnerPile(pSrcPhrase);
		removePos = removePos->GetNext();
		wxASSERT(pSrcPhrase != NULL);
		m_pApp->m_pKB->GetAndRemoveRefString(pSrcPhrase,emptyStr,useGlossOrAdaptationForLookup);
		count++;
        // in the next call, FALSE means 'don't delete the partner pile' (no need to
        // because we already deleted it a few lines above)
		m_pView->GetDocument()->DeleteSingleSrcPhrase(pSrcPhrase,FALSE);
		pList->DeleteNode(pos_toRemove);

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
    m_pApp->m_nOldSequNum = m_pApp->m_nActiveSequNum;
	
	// scroll into view, just in case a lot were inserted
	m_pApp->GetMainFrame()->canvas->ScrollIntoView(m_pApp->m_nActiveSequNum);
	m_pView->Invalidate();
	// now place the box
	m_pLayout->PlaceBox();

    // whm 21May2020 added - make the doc dirty so it can be samed after insertion of placeholder
    m_pApp->GetDocument()->Modify(TRUE);

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
// only in OnButtonRetranslation() to remove any earlier placeholders in the selection.
// Untransferring any transferred info from the context will have taken place first, by
// a prior call of UntransferTransferredMarkersAndPuncts() in the caller.
// BEW refactored 21Jul14, for ZWSP support - no change, provided 
// UntransferTransferredMarkersAndPuncts() is called before this is called, which it is 
void CPlaceholder::RemoveNullSrcPhraseFromLists(SPList*& pList,SPList*& pSrcPhrases,
									int& nCount,int& nEndSequNum,
									bool bActiveLocAfterSelection,
									int& nSaveActiveSequNum)
{
	// refactored 16Apr09
	// find the null source phrase in the sublist
	SPList::Node* pos_pList = pList->GetFirst();
	while (pos_pList != NULL)
	{
		SPList::Node* savePos = pos_pList;
		CSourcePhrase* pSrcPhraseCopy = (CSourcePhrase*)pos_pList->GetData();
		pos_pList = pos_pList->GetNext();
		wxASSERT(pSrcPhraseCopy != NULL);
		if (pSrcPhraseCopy->m_bNullSourcePhrase)
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

        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;

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
    // remove the placeholder - note, this will clobber the m_pActivePile pointer, and a
    // new partner pile will not be created, so we have to provide a temporary valid
    // m_pActivePile after this next call returns, so that our RecalcLayout() code which
    // needs to find the active strip will find a pile in a strip at or about the right
    // place in the document (so the layout gets set up right) -- what we need to ensure is
    // that any access to CPile::m_pOwningStrip using the GetStrip() call, will not have an
    // m_pOwningStrip value set to NULL (or to a non-NULL value in the debugger which then
    // points at arbitrary memory)
	RemoveNullSourcePhrase(pRemoveLocPile, nCount);
	
	// whm 30Dec2024 removal of a placeholder can leave a very large gap between some piles
	// in the display. Best to do a m_pApp->GetMainFrame()->SendSizeEvent() call here to get
	// the display back in shape.
	m_pApp->GetMainFrame()->SendSizeEvent();

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
			if (focus != NULL && m_pApp->m_pTargetBox->GetTextCtrl() == focus) // don't use GetHandle() on m_pTargetBox here !!! // whm 12Jul2018 added ->GetTextCtrl() part
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
// whm 20Mar2020 Note: This OnButtonNullSrc() handler is called by EVT_TOOL handler for 
// the Insert A Placeholder LEFT, and the EVT_TOOL handler for the Insert A Placeholder RIGHT. 
// It is also called by the CPhraseBox::OnSysKeyUp() handler for the CTRL+I hot key (on Linux and Mac).
// OnButtonNullSrc() internally calls the InsertNullSourcePhrase() function which in turn calls
// either InsertNullSrcPhraseBefore() or InsertNullSrcPhraseAfter().
/* BEW deprecated 28Apr20
void CPlaceholder::OnButtonNullSrc(wxCommandEvent& event)
{
    // Since the Add placeholder toolbar button has an accelerator table hot key (CTRL-I
    // see CMainFrame) and wxWidgets accelerator keys call menu and toolbar handlers even
    // when they are disabled, we must check for a disabled button and return if disabled.
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument(); //GetDocument();
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	wxASSERT(pFrame != NULL);

    bool bInsertAfter = FALSE;
    if (event.GetId() == ID_BUTTON_NULL_SRC_RIGHT  || wxGetKeyState(WXK_CONTROL))
    {
        // The accelerator CTRL+I is defined in the CMainFrame constructor to use the id ID_BUTTON_NULL_SRC_RIGHT
        // so this test will detect a direct mouse click on the "Insert A Placeholder To Right..." as well as
        // whenever the CTRL+I accelerator key combination is pressed. The wxGetKeyState(WXK_CONTROL) test
        // will hopefully detect when CTRL+I is detected on Linux/Mac within CPhraseBox::OnSysKeyUp() where the
        // CTRL+I handling code there calls OnButtonNullSrc() explicitly with a dummyevent.
        bInsertAfter = TRUE;
    }
    else if (event.GetId() == ID_BUTTON_NULL_SRC_LEFT)
    {
        // The event ID is ID_BUTTON_NULL_SRC_LEFT
        bInsertAfter = FALSE;
    }

	wxAuiToolBarItem *tbi;
	tbi = pFrame->m_auiToolbar->FindTool(ID_BUTTON_NULL_SRC_LEFT); // CTRL-I is only associated with ID_...LEFT
	// Return if the toolbar item is hidden or disabled
	if (tbi == NULL)
	{
		return;
	}
    // whm 20Mar2020 modified test below to detect whether the insert placeholder to left button is disabled, if so abort insertion
    if (!pFrame->m_auiToolbar->GetToolEnabled(ID_BUTTON_NULL_SRC_LEFT))
    {
        ::wxBell();
        return;
    }
    // whm 20Mar2020 modified test below to detect whether the insert placeholder to right button is disabled, if so abort insertion
    if (!pFrame->m_auiToolbar->GetToolEnabled(ID_BUTTON_NULL_SRC_RIGHT))
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
			m_pApp->m_pTargetBox->GetTextCtrl()->ChangeValue(_T(""));
		}
	}

	if (bInsertAfter) //(wxGetKeyState(WXK_CONTROL))
	{
		// An "insert after" is wanted
		// first save old sequ num for active location
        m_pApp->m_nOldSequNum = m_pApp->m_nActiveSequNum;
		
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
            m_pApp->m_bInhibitMakeTargetStringCall = TRUE;
			bool bOK;
			bOK = m_pApp->m_pKB->StoreText(m_pApp->m_pActivePile->GetSrcPhrase(), 
											m_pApp->m_targetPhrase);
			bOK = bOK; // avoid warning
            m_pApp->m_bInhibitMakeTargetStringCall = FALSE;
		}
		
        // at this point, we need to increment the pInsertLocPile pointer to the next pile,
        // and the nSequNum to it's sequence number, since InsertNullSourcePhrase() always
        // inserts "before" the pInsertLocPile; however, if the selection end, or active
        // location if there is no selection, is at the very end of the document (ie. last
        // sourcephrase), there is no following source phrase instance to insert before. If
        // this is the case, we have to append a dummy sourcephrase at the end of the
        // document, do the insertion, and then remove it again; and we will also have to
        // set (and later clear) m_bDummyAddedTemporarily because this is used in
        // the function in order to force a leftwards association only (and hence the user
        // does not have to be asked whether to associate right or left, if there is final
        // punctuation)
		SPList* pSrcPhrases = m_pApp->m_pSourcePhrases;
		CSourcePhrase* pDummySrcPhrase = NULL; // whm initialized to NULL
		if (nSequNum == m_pApp->GetMaxIndex())
		{
			// a dummy is temporarily required
            m_bDummyAddedTemporarily = TRUE;
			
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
		
        m_pApp->m_bMovingToDifferentPile = TRUE; // whm 22Mar2018 added
		InsertNullSourcePhrase(pDoc,pInsertLocPile,nCount,TRUE,FALSE,FALSE); // here, never
		// for Retransln
        m_pApp->m_bMovingToDifferentPile = FALSE; // whm 22Mar2018 added
        // if we inserted a dummy, now get rid of it and clear the global flag
		if (m_bDummyAddedTemporarily)
		{
            m_bDummyAddedTemporarily = FALSE;
			
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
		m_pView->Jump(m_pApp,pPile->GetSrcPhrase()); // whm note: calls ScrollIntoView(), PlacePhraseBox(2), Invalidate() and PlaceBox()
	}
	else // not inserting after the selection's end or active location, but before
	{
		// first save old sequ num for active location
        m_pApp->m_nOldSequNum = m_pApp->m_nActiveSequNum;
		
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
            m_pApp->m_bInhibitMakeTargetStringCall = TRUE;
			bool bOK;
			bOK = m_pApp->m_pKB->StoreText(m_pApp->m_pActivePile->GetSrcPhrase(), 
											m_pApp->m_targetPhrase);
			bOK = bOK; // avoid warning
            m_pApp->m_bInhibitMakeTargetStringCall = FALSE;
		}
		
        m_pApp->m_bMovingToDifferentPile = TRUE; // whm 22Mar2018 added
        InsertNullSourcePhrase(pDoc, pInsertLocPile, nCount);
        m_pApp->m_bMovingToDifferentPile = FALSE; // whm 22Mar2018 added

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
*/

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
	bool bCanInsert = TRUE; // initialise
	if (m_pApp->m_pTargetBox != NULL)
	{
		// BEW changed 24Jan13, to make the test more robust
		// BEW added more, 9Apr20, to disable if active location is within a span
		// of footnote (\f ... \f*), extended footnote (\ef .... \ef*), 
		// cross-reference (\x ... \x*) or extended cross-reference (\ex ... \ex*)
		CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
		bool bWithin = FALSE; // initialise
		
		// TODO try caching soln that repeatedly provides the bWithin value until
		// the phrasebox moves
		if (m_pApp->m_entryCount_updateHandler == 0)
		{
			// call the function, and cache it's return value, augment entry count
			bWithin = pDoc->IsWithinSpanProhibitingPlaceholderInsertion(m_pApp->m_pActivePile->GetSrcPhrase());
			m_pApp->m_bIsWithin_ProhibSpan = bWithin; // cached now
			// cache the active pile's ptr
			m_pApp->m_pCachedActivePile = m_pApp->m_pActivePile; // what we compare against
			m_pApp->m_entryCount_updateHandler++; // augment entry counter
		}
		else
		{
			// counter is > 0, so check for movement of the active pile
			if (m_pApp->m_pActivePile != m_pApp->m_pCachedActivePile)
			{
				// There was movement of the active location, so gotta make
				// the function call again, and re-cache the values, and
				// reset entry count to zero
				m_pApp->m_entryCount_updateHandler = 0;
				bWithin = pDoc->IsWithinSpanProhibitingPlaceholderInsertion(m_pApp->m_pActivePile->GetSrcPhrase());
				m_pApp->m_bIsWithin_ProhibSpan = bWithin; // cached now
				// cache the active pile's ptr
				m_pApp->m_pCachedActivePile = m_pApp->m_pActivePile; // what we compare against
			}
			else
			{
				// Active pile location has not changed, so set bWithin
				// to the cached value
				bWithin = m_pApp->m_bIsWithin_ProhibSpan; // saves a time-consuming func call
				// Well... we don't really need to augment the counter here because
				// it will stay at 1 until the active location moves again - so I'll
				// refrain from augmenting entry counter here
			}
		}
		if (bWithin)
		{
			bCanInsert = FALSE;
			event.Enable(bCanInsert);
			return;
		}
		// Next, if the target box is shown, but the window with focus is not that one,
		// then disable
		if ((m_pApp->m_pTargetBox->IsShown() && (m_pApp->m_pTargetBox->GetTextCtrl() != wxWindow::FindFocus())))
		{
			// whm 12Jul2018 added ->GetTextCtrl() part
			bCanInsert = FALSE;
			event.Enable(bCanInsert);
			return;
		}
		
		// If there is a selection that is not well formed, disable
		if ((!m_pApp->m_selection.IsEmpty() && m_pApp->m_selectionLine == -1) || 
			(m_pApp->m_selection.IsEmpty() && m_pApp->m_selectionLine != -1))
		{
			bCanInsert = FALSE;
			event.Enable(bCanInsert);
			return;
		}

		// Finally, check iF PlacePhraseBox() has requested disabled 
		// insert null src phrase buttons
		if (bCanInsert)
		{
			if (m_pApp->m_bDisablePlaceholderInsertionButtons)
			{
				// BEW addition 9Apr20 handler is called on leaving and on landing - we
				// only what this disable when landing at a new location; clear the flag
				// when 'leaving'. See PlacePhraseBox() in view file.
				bCanInsert = FALSE;
			}
		}
	}
	event.Enable(bCanInsert);
}

#if defined (_PHRefactor)

void CPlaceholder::DoInsertPlaceholder(CAdapt_ItDoc* pDoc, // needed here & there
	CPile* pInsertLocPile,	// ptr to the pSrcPhrase which is the current active location, before
							// or after which the placeholder is to be inserted
	const int nCount,		// how many placeholders to sequentially insert
	bool bRestoreTargetBox,	// TRUE if restoration wanted, FALSE if not; no default
	bool bForRetranslation,	// TRUE if in a Retranslation, 1 or more may need appending, no default
	bool bInsertBefore,		// TRUE to insert before pInsertLocPile, FALSE to insert after pInsertLocPile, no default
	bool bAssociateLeftwards // TRUE to associate with left located text, FALSE to associate with right
							 // located text, no default. However, if bInsertBefore is TRUE we will
							 // hard code bAssocateLeftwards to FALSE. If bInsertBefore is FALSE we will
							 // hard code bAssociateLeftwards to TRUE. The parameter is included in
							 // the signature in case sometime we with to vary the behaviour.
	)
{
	// Bill, in the legacy code wanted that if the pInsertLocPile's box's m_bAbandonable flag
	// is TRUE (ie. a copy of source text was done and nothing typed yet) then the current pile
	// would have the box contents abandoned, nothing put in the KB, and then the placeholder
	// insertion - the advantage of this is that if the placeholder is inserted immediately
	// before the phrasebox's location, then after the placeholder text is typed and the user
	// hits ENTER to continue looking ahead, the former box location will get the box and the
	// copy of the source redone, rather than the user usually having to edit out an unwanted
	// copy from the KB, or remember to clear the box manually. A sufficient thing to do here
	// is just to clear the box's contents. This applies to 'insert before' and to 'insert after'
	// code blocks
	if (pInsertLocPile == NULL)
		return;
	if (m_pApp->m_pTargetBox->m_bAbandonable)
	{
		m_pApp->m_targetPhrase.Empty(); // clear the string
		if (m_pApp->m_pTargetBox->GetHandle() != NULL && m_pApp->m_pTargetBox->IsShown())
		{
			// empty the box
			m_pApp->m_pTargetBox->GetTextCtrl()->ChangeValue(_T(""));
		}
	}
	CSourcePhrase* pCurrentSrcPhrase = NULL; // at the passed in pInsertLocPile location
	pCurrentSrcPhrase = pInsertLocPile->GetSrcPhrase(); // in case we need it (we do need it)
	
	// The next two will be dynamically determined, as where they are previous or following
	// to will depend on which button was pressed in the GUI, or where this function gets called
	CSourcePhrase* pPrevSrcPhrase = NULL; // initialize
	CSourcePhrase* pFollSrcPhrase = NULL; // initialize
	SPList* pList = m_pApp->m_pSourcePhrases;
	int placeholderLoc = pCurrentSrcPhrase->m_nSequNumber; // initialize to a close value

	/*  Before block Begins */

	// Distinguish between the button choices - whether to insert before, or after
	if (bInsertBefore)
	{
		wxASSERT(bForRetranslation == FALSE); // the 'insert before' option is never used 
											  //for padding a retranslation end
		wxASSERT(nCount == 1); // the button or shortcut can only insert one

		int nSequNumInsLoc = pCurrentSrcPhrase->m_nSequNumber; // this will increase after PHolder insertion

		// first save old sequ num for active location
		m_pApp->m_nOldSequNum = m_pApp->m_nActiveSequNum;

		// find the pile preceding which to do the insertion - it will either be preceding
		// the first selected pile, if there is a selection current, or preceding the
		// active location if no selection is current
		CPile* pInsertLocPile2 = NULL; // initialize, allow for selection location 
									   // to be different from passed in pInsertLocPile
		//nCount = 1; 
		int nSequNum = -1;
		if (m_pApp->m_selectionLine != -1)
		{
			// we have a selection, the pile we want is that of the selection 
			// list's first element
			CCellList* pCellList = &m_pApp->m_selection;
			CCellList::Node* cpos = pCellList->GetFirst();
			pInsertLocPile2 = cpos->GetData()->GetPile();
			if (pInsertLocPile2 == NULL)
			{
				wxMessageBox(_T(
					"A zero CPile pointer was returned from selection, the insertion cannot be done."),
					_T(""), wxICON_EXCLAMATION | wxOK);
				return;
			}
			nSequNum = pInsertLocPile2->GetSrcPhrase()->m_nSequNumber;
			wxASSERT(pInsertLocPile2 != NULL);
			m_pView->RemoveSelection(); // Invalidate will be called in InsertNullSourcePhrase()
		}
		else
		{
			// no selection, so just insert preceding wherever the phraseBox 
			// currently is located
			pInsertLocPile2 = pInsertLocPile; // set from signature's value passed in
			if (pInsertLocPile2 == NULL)
			{
				wxMessageBox(_T(
					"A zero CPile pointer was set from signature, the insertion cannot be done."),
					_T(""), wxICON_EXCLAMATION | wxOK);
				return;
			}
			nSequNum = pInsertLocPile2->GetSrcPhrase()->m_nSequNumber;
		}
		wxASSERT(nSequNum >= 0);
		nSequNumInsLoc = nSequNum; // make sure they agree

		// check we are not in a retranslation - we can't insert there!
		// if it is still a retranslation, we must abort the operation)
		if (pInsertLocPile2->GetSrcPhrase()->m_bRetranslation)
		{

            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;

            wxMessageBox(_(
				"Sorry, you cannot insert a placeholder within a retranslation. The command has been ignored."),
				_T(""), wxICON_EXCLAMATION | wxOK);
			m_pView->RemoveSelection();
			m_pView->Invalidate();
			m_pLayout->PlaceBox();
			return;
		}

		// ensure the contents of the phrase box are saved to the KB, because after insertion
		// the placeholder will be the current location - Note: BEW 30March20, we no longer
		// allow placeholders to be stored in the KB, but the insert location's pSrcPhrase will
		// be moved there, and so the passed in pInsertLocPile needs it's translation to be
		// stored before moving
		if (m_pApp->m_pTargetBox->GetHandle() != NULL && m_pApp->m_pTargetBox->IsShown())
		{
			m_pView->MakeTargetStringIncludingPunctuation(pInsertLocPile->GetSrcPhrase(), m_pApp->m_targetPhrase);

			// we are about to leave the current phrase box location, so we must try to
			// store what is now in the box, if the relevant flags allow it
			m_pView->RemovePunctuation(pDoc, &m_pApp->m_targetPhrase, from_target_text);
			m_pApp->m_bInhibitMakeTargetStringCall = TRUE;
			bool bOK = FALSE;
			bOK = m_pApp->m_pKB->StoreText(pInsertLocPile->GetSrcPhrase(), m_pApp->m_targetPhrase);
			bOK = bOK; // avoid gcc set but not used warning
			m_pApp->m_bInhibitMakeTargetStringCall = FALSE;
		}

		m_pApp->m_bMovingToDifferentPile = TRUE; // whm 22Mar2018 added
		bool bAssociatingRightwards = bAssociateLeftwards ? FALSE : TRUE; // easier to work with 

		// Pull code from legacy InsertNullSourcePhrase(pDoc, pInsertLocPile, nCount);
		// and tweak, - use MS Code to help 

		// from above pInsertLocPile2 is where the active pile is - either from passed in
		// pInsertLocPile, or the first pile in a user selection - which could be far away
		// from the current phrasebox location
		int nStartingSequNum = pInsertLocPile2->GetSrcPhrase()->m_nSequNumber; // starts off with 
					// same value as nSequNum above, and also nSequNumInsLoc above

		// whm 2Aug06 added following test to prevent insertion of placeholder 
		// in front of \id marker
		if ((pInsertLocPile2->GetSrcPhrase()->m_markers.Find(_T("\\id")) != wxNOT_FOUND && !bForRetranslation) || (nSequNum == 0))
		{
			// user is attempting to insert placeholder before a \id marker which should not be
			// allowed rather than a message, we'll just beep and return; similarly disallow
			// insertion at doc start when there is no markup
			::wxBell();
			return;
		}
		SPList::Node* insertPos = pList->Item(nStartingSequNum); // the position before which
				// we will make the insertion - insertions in wxList always precede the insertPos
		wxString emptyStr = _T("");

		// BEW 23Sep10, store the sequence number to the pPrevSrcPhrase, so we can get a
		// pile pointer from it whenever needed (all the changes will occur at higher sequence
		// number values, so this value won't change throughout the placeholder insertion
		// process)
		int nPrevSequNum = -1; // initialize
		if (nStartingSequNum > 0) // whm added to prevent assert and unneeded test for 
								  // m_bEndFreeTrans when sequ num is zero
		{
			SPList::Node* earlierPos = pList->Item(nStartingSequNum - 1);
			pPrevSrcPhrase = (CSourcePhrase*)earlierPos->GetData();

		}
		// end of TRUE block for test: if (nStartingSequNum > 0)
		// BEW 21Jul14, Set a default src text wordbreak string for the placeholder
		// - get it from the previous instance
		bool bWordBreakDifference = FALSE; // default
		wxString defaultWordBreak = _T(" "); // a space, in case there is no previous CSourcePhrase
		if (pPrevSrcPhrase != NULL)
		{
			defaultWordBreak = pPrevSrcPhrase->GetSrcWordBreak();
		}
		// get the sequ num for the insertion location 
		// (it could be quite diff from active sequ num)
		CSourcePhrase* pSrcPhraseInsLoc = pInsertLocPile->GetSrcPhrase();
		//int	nSequNumInsLoc = pSrcPhraseInsLoc->m_nSequNumber; // got it above
		wxASSERT(nSequNumInsLoc >= 0 && nSequNumInsLoc <= m_pApp->GetMaxIndex());

		// BEW 21Jul14 get the pile after the insert loc, and its CSourcePhrase so we can do
		// transfers to the m_srcWordBreak contents in a right-association context
		CPile* pPostInsertLocPile = m_pView->GetNextPile(pInsertLocPile); // will return NULL
											// if the pInsertLocPile is at document's end
		CSourcePhrase* pPostInsertLocSrcPhrase = NULL;
		if (pPostInsertLocPile != NULL)
		{
			pPostInsertLocSrcPhrase = pPostInsertLocPile->GetSrcPhrase();
		}
		wxASSERT(insertPos != NULL);
		int nActiveSequNum = m_pApp->m_nActiveSequNum; // save, so we can restore later on 
													   
		// Insertion of a placeholder is no longer permitted within a retranslation,
		// nor within a footnote or cross reference type of span. But inserting "before"
		// a span is acceptable because the placeholder cannot possibly be considered
		// as a candidate for being within the span. When the insert location is after
		// a span, association is now obligatorily rightwards, and so the placeholder in
		// that circumstance cannot be considered as being part of the span.
		if (!bForRetranslation)
		{
			// The retranslation case is taken care of by handlers OnButtonRetranslation and
			// OnButtonEditRetranslation, so only the normal null srcphrase insertion needs to
			// be considered here
			m_pApp->GetRetranslation()->SetIsInsertingWithinFootnote(FALSE);
			// The Retranslation boolean, m_bInsertingWithinFootnote occurs elsewhere
			// so we need to force it to FALSE because such placeholder insertions never
			// now can happen
		}

		// create the needed null source phrases and insert them in the list;
		// preserve pointers to the first and last for use below
		CSourcePhrase* pFirstOne = NULL; // whm initialized to NULL
		CSourcePhrase* pLastOne = NULL; // whm initialized to NULL
		CSourcePhrase* pSrcPhrase = NULL; // the first (and only) one's CSourcePhrase
			// (strictly speaking its redundant, but as I'm copying & tweaking legacy
			// code, it can be retained - it's same as pFirstOne, which is same as pLastOne
			// when nCount is 1, which it is)
		for (int i = 0; i<nCount; i++)
		{
			CSourcePhrase* pSrcPhrasePH = new CSourcePhrase; // PH means 'PlaceHolder'
			if (i == 0)
			{
				pFirstOne = pSrcPhrasePH;
				pSrcPhrase = pSrcPhrasePH;
			}
			if (i == nCount - 1)
			{
				pLastOne = pSrcPhrasePH;
			}
            // whm 20May2020 Note: It turns out to be inappropriate to make the new placeholder source phrase
            // have a m_bHasKBEntry flag set TRUE - the code within the GetNextEmptyPile() considers it to be
            // an error and resets it to FALSE. The place to deal with the problem of a placeholder with an 
            // adaptation being considered as a 'hole', is within the do...while loop in the View's 
            // GetNextEmptyPile() function - adding a 4th test there: 
            // (pPile->GetSrcPhrase()->m_bNullSourcePhrase && !pPile->GetSrcPhrase()->m_adaption.IsEmpty()) ||
			pSrcPhrasePH->m_bNullSourcePhrase = TRUE;
			pSrcPhrasePH->m_srcPhrase = _T("...");
			pSrcPhrasePH->m_key = _T("...");
			pSrcPhrasePH->m_nSequNumber = nStartingSequNum + i; // ensures the
						// UpdateSequNumbers() call starts with correct value

			pList->Insert(insertPos, pSrcPhrasePH);

			// BEW added 13Mar09 for refactored layout
			pDoc->CreatePartnerPile(pSrcPhrasePH);
		} // end of for loop for inserting a placeholder

		// fix the sequ num for the old insert location's source phrase
		nSequNumInsLoc += nCount;

		// calculate the new active sequ number - it could be anywhere, but all 
		// we need to know is whether or not the insertion was done preceding 
		// the former active sequ number's location
		if (nStartingSequNum <= nActiveSequNum)
			m_pApp->m_nActiveSequNum = nActiveSequNum + nCount;

		// update the sequence numbers, starting from the first one inserted (although
		// the layout of the view is no longer valid, the relationships between piles,
		// CSourcePhrases and sequence numbers will be correct after the following call,
		// and so we can, for example, get the correct pile pointer for a given sequence
		// number and from the pointer, get the correct CSourcePhrase)
		m_pView->UpdateSequNumbers(nStartingSequNum);

		placeholderLoc = pFirstOne->m_nSequNumber;

		// We must check if there is preceding punctuation on the following source phrase, or a
		// begin-marker or both; accociation is obligatorily to the right, so stuff
		// at the end of the previous CSourcePhrase instance is ignored. 
		// If the insertion location's instance has initial punct(s) or begin marker(s),
		// then the placeholder associates to the following instance; and so do the
		// appropriate transfer of initial punctuation and / or markers to the first
		// placeholder (because association is to the right when inserting "before").
		bool bAssociationRequired = FALSE; // it will be set true if  
			// associating to the right is required for initial punctuation or stored
			// begin-marker
			// 
			// The following 2 booleans are only looked at by code in 
			// "not-for-retranslation" blocks
		bool bFollowingPrecPunct = FALSE;
		bool bFollowingMarkers = FALSE;
		nPrevSequNum = nStartingSequNum - 1; // remember, this could be -ve (ie. -1)

		// This is a major block for when inserting manually (ie. not in a retranslation), and
		// manual insertions are always only a single placeholder per insertion command by the
		// user, and so nCount in here will only have the value 1
		if (!bForRetranslation)
		{
			if (nPrevSequNum == -1)
			{
				// make sure these pointers are null if we are inserting 
				// at the doc beginning
				pPrevSrcPhrase = NULL;
			}
			// now check the following source phrase for any preceding punct or a marker in
			// one of the three marker storage locations: m_markers, m_inlineNonbindingMarkers,
			// and m_inlineBindingMarkers
			if (!pSrcPhraseInsLoc->m_precPunct.IsEmpty())
				bFollowingPrecPunct = TRUE;

			// BEW 25Appr20,  for right association, look at all markers
			// or information-beginnings for info within m_markers, m_endMarkers,
			// m_collectedBackTrans, or m_freeTrans (including markers for info with TextType
			// none) and the inline marker stores. There is no reason though to consider a
			// note, as placeholder insertion shouldn't move it from where it is presumably
			// most relevant. We just want to set TRUE or make FALSE the bool
			// bFollowingMarkers, so this function does the job
			bFollowingMarkers = IsRightAssociationTransferPossible(pSrcPhraseInsLoc);

			// BEW 21Jul14, preceding word-delimiter for each CSourcePhrase is an issue.
			// Normally it is a space. However, for some SE Asian scripts it may be a space on
			// the first instance, followed by a series of words with ZWSP delimitation. There
			// may also quite likely be no marker, punctuation, or other information on the
			// first following instance that triggers right association; however, the latin space on
			// preceding the first instance if the second an subsequent instances have
			// non-latin-space delimitation may require right association of the latin space
			// to the placeholder, and copying forward of the next-to-first's (different)
			// word break to the first instance. So we need an extra test and a boolean to
			// save the results of the test. Then we'll include the boolean into the 
			// right association blocks below. But first we must give the placeholder a default
			// m_srcWordBreak - we can't assume it is a space. 
			
			// Since we associate rightwards, we'll copy the insert Loc's CSourcePhrase's
			// m_srcWordBreak to the placeholder. Then, our test will be to check if
			// the placeholder's wordbreak is different than what follows. If so, we have to
			// associate to the right, which means moving the right's wordbreak back to the
			//start of the placeholder.
			wxString prevWordBreak = _T(" "); // default space in case pSrcPhraseInsLoc is NULL
			if (pSrcPhraseInsLoc != NULL)
			{
				prevWordBreak = pSrcPhraseInsLoc->GetSrcWordBreak();
			}
			// Make our test:
			if (defaultWordBreak != prevWordBreak)
			{
				bWordBreakDifference = TRUE;
			}
			// Okay, given the above explanation, for right association the m_srcWordBreak on
			// pSrcPhraseInsLoc has to be copied to the placeholder - it could be a space being
			// transferred, and what follows that instance may be ZWSP delimiters - so we can't
			// just leave the pSrcPhraseInsLoc's m_srcWordBreak unchanged - what we have to do
			// is to copy the next instance's (pSrcPhraseInsLoc's) to the placeholder's
			// m_srcWordBreak to keep any potential chain of ZWSP delimiters intact.

			// If one of the flags is true, ask the user for the direction of association
			// (bAssociatingRightwards is TRUE when control reaches here)
			if (bFollowingMarkers || bFollowingPrecPunct || bWordBreakDifference)
			{
				// association will be required, so set the flag for this
				bAssociationRequired = TRUE;

				if (bAssociatingRightwards)
				{
					/* No longer correct, it's okay to insert "before" a conjoining
					// BEW 11Oct10, warn right association is not permitted when a word pair
					// joined by USFM ~ fixed space follows
					if (IsFixedSpaceSymbolWithin(pSrcPhraseInsLoc))
					{
						wxMessageBox(_(
							"Two words are joined with fixed-space marker ( ~ ) following the placeholder.\nForwards association is not possible when this happens."),
							_T("Warning: Unacceptable Forwards Association"), wxICON_EXCLAMATION | wxOK);
						goto m; // make no further adjustments, just jump to the 
								// RecalcLayout() call near the end
					}
					*/

					// the association is to the text which follows, so transfer from there
					// to the first in the list - but not if the first sourcephrase of a
					// retranslation follows, ... which can't happen anyway because we can't
					// put the phrase at the retranslation's first pile in order to insert 
					// before it
					if (bFollowingMarkers)
					{
						
						if (pSrcPhraseInsLoc->m_bBeginRetranslation)
						{
							// Can't happen; the way to fix the retranslation is not to try
							// associate a preceding placeholder to it, but to edit the
							// retranslation itself. We can dispense with the warning and
							// just beep & return - doing nothing
							//wxMessageBox(_(
							//	"Trying to associate the inserted placeholder with the start of the retranslation text which follows it does not work.\nIt will be better if you now delete the placeholder and instead edit the retranslation itself."),
							//	_T("Warning: Unacceptable Forwards Association"), wxICON_EXCLAMATION | wxOK);
							wxBell();
							goto m; // make no further adjustments, just jump to the 
									// RecalcLayout() call near the end
						}
						
						// Note: although we use pFirstOne, there is only one anyway for a manual
						// insertion; but this could be generalized safely if we want (but we don't want)
						wxASSERT(pFirstOne != NULL); // whm added
						pFirstOne->m_markers = pSrcPhraseInsLoc->m_markers; // transfer markers
						pSrcPhraseInsLoc->m_markers.Empty();
						pFirstOne->SetInlineNonbindingMarkers(pSrcPhraseInsLoc->GetInlineNonbindingMarkers());
						pSrcPhraseInsLoc->SetInlineNonbindingMarkers(emptyStr);
						pFirstOne->SetInlineBindingMarkers(pSrcPhraseInsLoc->GetInlineBindingMarkers());
						pSrcPhraseInsLoc->SetInlineBindingMarkers(emptyStr);

						// have to also copy various members, such as m_inform, so navigation
						// text works right; but we don't want to copy everything - for
						// instance, we don't want to incorporate it into a retranslation; so
						// just get the essentials
						pFirstOne->m_inform = pSrcPhraseInsLoc->m_inform;
						pFirstOne->m_chapterVerse = pSrcPhraseInsLoc->m_chapterVerse;
						pFirstOne->m_bVerse = pSrcPhraseInsLoc->m_bVerse;
						// BEW 8Oct10, repurposed m_bParagraph as m_bUnused <-- BEW Mar20, it's 
						// now repurposed for TRUE saying thata m_punctsPattern
						// has USFM3 attributes metadata substring stored at the
						// pSrcPhraseInsLoc- and since that is tied to where an endmarker
						// must be, we must NOT move m_bUnused forwards to the placeholder
						pFirstOne->m_bChapter = pSrcPhraseInsLoc->m_bChapter;
						pFirstOne->m_bSpecialText = pSrcPhraseInsLoc->m_bSpecialText;
						pFirstOne->m_bFirstOfType = pSrcPhraseInsLoc->m_bFirstOfType;
						pFirstOne->m_curTextType = pSrcPhraseInsLoc->m_curTextType;

						// clear the old locations which had their values moved
						pSrcPhraseInsLoc->m_inform.Empty();
						pSrcPhraseInsLoc->m_chapterVerse.Empty();
						pSrcPhraseInsLoc->m_bFirstOfType = FALSE;
						pSrcPhraseInsLoc->m_bVerse = FALSE;
						pSrcPhraseInsLoc->m_bChapter = FALSE;

						// A note stays put, and start of free translation does not move either
						pFirstOne->m_bHasNote = FALSE;

						// move the text of a collected back translation if there is one
						pFirstOne->SetCollectedBackTrans(pSrcPhraseInsLoc->GetCollectedBackTrans());
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
					// BEW 21Jul14, Do any needed right association for wordbreaks
					if (bWordBreakDifference)
					{
						wxString aWordBreak = _T(""); // initialise
						aWordBreak = pSrcPhraseInsLoc->GetSrcWordBreak();
						pFirstOne->SetSrcWordBreak(aWordBreak);
						// now copy the following one forward as explained above
						aWordBreak = pPostInsertLocSrcPhrase->GetSrcWordBreak();
						pSrcPhraseInsLoc->SetSrcWordBreak(aWordBreak);
					}
				}
			} // end of TRUE block for test: if (bFollowingMarkers || bFollowingPrecPunct
			  // || bWordBreakDifference)
			else
			{
				// Associated neither left nor right, so give the placeholder the
				// wordbreak of the insert location
				pLastOne->SetSrcWordBreak(pSrcPhraseInsLoc->GetSrcWordBreak());
			}
		} // end of TRUE block for test: if (!bForRetranslation)

		// handle any adjustments required because the insertion was done where
		// there is one or more free translation sections defined in the at the
		// insertion location; the inserted placeholder immediatey precedes
		// and assocation is obligatorily to the right (so start of a free
		// translation should most probably go to the inserted placeholder)
		// BEW 24Sep10 -- the next huge nested block is needed, it's for 
		// handling all the possible interactions of where free translations
		// may start and end, and the various flags etc.
		if (!bForRetranslation)
		{
			// here we are interested in the single placeholder inserted not in any
			// retranslation
			if (!(pPrevSrcPhrase == NULL))
			{
				// the insertion was done not at the doc beginning, and typically it is some 
				// general location in the doc - and we are associating right, or no associating
				// at all (which means the insertion was done where initial punctuation was absent
				// and initiala begin markers also absent. So there are some possibilities
				// to consider for the three consecutive sourcephrase pointers pPrevSrcPhrase, 
				// pSrcPhrase, and pSrcPhraseInsLoc - pSrcPhrase being the placeholder which 
				// was just inserted (it was set where pFirstOne was set) and pSrcPhraseInsLoc
				// is the one where the phrasebox was located originally.

				if (!pPrevSrcPhrase->m_bHasFreeTrans)
				{
					// pPrevSrcPhrase cannot have a free translation defined on it, because
					// the update handler will have disabled the GUI button
					if (pSrcPhraseInsLoc->m_bHasFreeTrans)
					{
						// This is the one where the box was located originally.
						// The only possibilities are that pSrcPhraseInsLoc is the first or
						// only free translation in a section lying to the right of the
						// placeholder. What we do will depend on the associativity, which is
						// rightwards, or none.
						if (pSrcPhraseInsLoc->m_bStartFreeTrans)
						{
							// its the start of a free translation section
							if (bAssociationRequired && bAssociatingRightwards)
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
								// no association in either direction, but rightwards
								// is required - so move the start to the placeholder
								// but in this case no m_markers movement will
								// have been done, so we must move that as well
								pSrcPhraseInsLoc->m_bStartFreeTrans = FALSE;
								pSrcPhrase->m_bStartFreeTrans = TRUE;
								pSrcPhrase->m_bHasFreeTrans = TRUE;
								pSrcPhrase->m_markers = pSrcPhraseInsLoc->m_markers;
								pSrcPhraseInsLoc->m_markers.Empty();
							}
						}
					} // end of TRUE block for test:if (pSrcPhraseInsLoc->m_bHasFreeTrans)
					else
					{
						// pSrcPhraseInsLoc has no free translation defined on it, so nothing
						// needs to be done to the default flag values (all 3 FALSE) for free
						// translation support in pSrcPhrase
						;
					}
				} // end of TRUE block for test: if (!pPrevSrcPhrase->m_bHasFreeTrans)

			} // end of TRUE block for test: if (!(pPrevSrcPhrase == NULL))
		} // end of the TRUE block for test: if (!bForRetranslation)

		// BEW added 10Sep08 in support of Vertical Edit mode
		if (gbVerticalEditInProgress)
		{
			// update the relevant parts of the gEditRecord (all spans are affected
			// equally, except the source text edit section is unchanged)
			gEditRecord.nAdaptationStep_ExtrasFromUserEdits += 1;
			gEditRecord.nAdaptationStep_NewSpanCount += 1;
			gEditRecord.nAdaptationStep_EndingSequNum += 1;
		}

		// The following boolean is important, it is used (by wrapping with
		// TRUE and doing a call then clearing to FALSE after the call exits,
		//  e.g PlacePhraseBox() call where PlaceBox() is going to be called
		// later anyway. So leave as is.
		m_pApp->m_bMovingToDifferentPile = FALSE; // whm 22Mar2018 added

		m_pApp->m_nActiveSequNum = placeholderLoc;
		
	} // End of TRUE block for test: if (bInsertBefore)

 /*  Before block ends */

/*  After block begins */

	else // What is wanted is an "insert after" the pInsertLocPile location (and if at
		 // doc end we append a dummy and insert before it, and then remove the dummy)
	{
		// This is the only block which, besides inserting a single placeholder, can
		// insert several to complete a long Retranslation, etc

		// The following long block of code is taken from the "insert after" block
		// of the OnInsertNullSrc() handler - and simplified because the "after" choice
		// associates leftwards (for RTL lenguages, leftwards in the code, but rightwards
		// in the GUI) - it's now hard coded

		// first save old sequ num for active location
		m_pApp->m_nOldSequNum = m_pApp->m_nActiveSequNum;

		// find the pile after which to do the insertion - it will either be after the
		// last selected pile if there is a selection current, or after the active location
		// if no selection is current - beware the case when active location is a doc end!
		CPile* pInsertLocPile2 = NULL; // initialize
		wxASSERT(nCount >= 1); // the button or shortcut can only insert one; but
							   // padding the end of a retranslation, etc, can insert many
		int nSequNum = -1;

		if (m_pApp->m_selectionLine != -1)
		{
			// we have a selection, the pile we want is that of the selection 
			// list's last element
			CCellList* pCellList = &m_pApp->m_selection;
			CCellList::Node* cpos = pCellList->GetLast();
			// whm 19Aug2021 modified line below: edb noted a bug in following assignment when last source pile is selected and user tries
			// to insert a placeholder AFTER that location. In this case the assignment statement should be assigning to pInsertLocPile2 
			// instead of the passed in param pInsertLocPile. Compare with the if (bInsertingBefore) block at about lines 3246-3277 above.
			pInsertLocPile2 = cpos->GetData()->GetPile();
			if (pInsertLocPile2 == NULL)
			{
				wxMessageBox(_T(
					"A zero CPile pointer was set from selection, the insertion cannot be done."),
					_T(""), wxICON_EXCLAMATION | wxOK);
				return;
			}
			nSequNum = pInsertLocPile2->GetSrcPhrase()->m_nSequNumber;
			wxASSERT(pInsertLocPile2 != NULL);
			m_pView->RemoveSelection(); // Invalidate will be called in InsertNullSourcePhrase()
		}
		else
		{
			// no selection, so just insert after wherever the phraseBox currently is located
			pInsertLocPile2 = pInsertLocPile; // using the signature's passed in location
			if (pInsertLocPile2 == NULL)
			{
				wxMessageBox(_T(
					"A zero CPile pointer was set from signature, the insertion cannot be done."),
					_T(""), wxICON_EXCLAMATION | wxOK);
				return;
			}
			nSequNum = pInsertLocPile2->GetSrcPhrase()->m_nSequNumber;
		}
		wxASSERT(nSequNum >= 0);

		// check we are not in a retranslation, for manual inserts - we can't insert there! 
		// But when padding a long retranslation, the insertion of the extras relies on
		// the insertion being an "insert after", and that means that the insert location
		// necessarily must be at the last selected pile's location. So, use the 
		// pointer to the retranslation object (on the app) to grab the public boolean
		// m_bIsRetranslationCurrent (with value TRUE) to tweak the next block to only
		// warn when necessary
		CSourcePhrase* pInsSrcPhr = pInsertLocPile2->GetSrcPhrase();
		if (pInsSrcPhr->m_bRetranslation)
		{
			CRetranslation* pRetrans = m_pApp->GetRetranslation();
			bool bIsRetranslating = pRetrans->m_bIsRetranslationCurrent;
			if (!bIsRetranslating)
			{

                // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                m_pApp->m_bUserDlgOrMessageRequested = TRUE;

                // Disallow insertion when we are not within a retranslation
				wxMessageBox(_(
					"You cannot insert a placeholder within a retranslation. The command has been ignored.")
					, _T(""), wxICON_EXCLAMATION | wxOK);
				m_pView->RemoveSelection();
				m_pView->Invalidate();
				m_pLayout->PlaceBox();
				return;
			}
		}

		// ensure the contents of the phrase box are saved to the KB
		// & make the punctuated target string - the active location will shift to
		// the placeholder, so where it currently is (at passed in pInsertLocPile) has to 
		// have it's contents saved to the KB before the move is done
		if (m_pApp->m_pTargetBox->GetHandle() != NULL && m_pApp->m_pTargetBox->IsShown())
		{
			m_pView->MakeTargetStringIncludingPunctuation(pInsertLocPile->GetSrcPhrase(), m_pApp->m_targetPhrase);

			// we are about to leave the current phrase box location, so we must try to
			// store what is now in the box, if the relevant flags allow it
			m_pView->RemovePunctuation(pDoc, &m_pApp->m_targetPhrase, from_target_text);
			m_pApp->m_bInhibitMakeTargetStringCall = TRUE;
			bool bOK = TRUE;
			bOK = m_pApp->m_pKB->StoreText(pInsertLocPile->GetSrcPhrase(), m_pApp->m_targetPhrase);
			bOK = bOK; // avoid gcc set but not used warning
			m_pApp->m_bInhibitMakeTargetStringCall = FALSE;
		}
		wxASSERT(bAssociateLeftwards == TRUE);

		// In the legacy code, insertion at the doc end required a dummy CSourcePhrase
		// be appended, before which left-insertion was done.
		// Likewise, since we are inserting into a wxList, and list insertions are always to the left
		// of the Node*, we need a pInsertLocAfter which is the next one after pInsertLoc2, and we
		// will insert before it. But if at doc end (MaxIndex()) then there is no such pile 
		// available - in which case we must simulate an Append() to the list instead by appending
		// a dummy, inserting before it, then removing the dummy
		bool bDoAppend = FALSE;
		if (nSequNum == m_pApp->GetMaxIndex()) // note: nSequNum is based on pInsertLoc2 above
		{
			bDoAppend = TRUE;
		}
		int nStartingSequNum = nSequNum;
		int nFollowingSequNum = nSequNum + 1;

		// Get local strings in case we need to transfer markers to the end of
		// the inserted or placeholder
		wxString endmarkersToTransfer = _T("");
		wxString inlineNonbindingEndmarkersToTransfer = _T("");
		wxString inlineBindingEndmarkersToTransfer = _T("");
		wxString emptyStr = _T("");

		bool bTransferEndMarkers = FALSE; // true if any endmarkers transferred, of the 3 locations
		bool bMoveEndOfFreeTrans = FALSE; // TRUE if endmarkers need to be transferred to the end
										  // of a lengthened (by placeholder insertion) free translation span
		CSourcePhrase* pDummySrcPhrase = NULL; // whm initialized to NULL
		m_bDummyAddedTemporarily = FALSE; // initialise
		if (bDoAppend)
		{
			// We have to appear to be appending the new instance of the placeholder or placeholders; 
			// and it or they will associate leftwards. But at doc end we need a CSourcePhrase*
			// which we can call pFollSrcPhrase before which we can do wxList insert, so instead
			// of working out how to do the list append append and cover all the bases below, we
			// will create a dummy CSourcePhrase at doc end, to be the needed pFollSrcPhrase, do
			// the list insertion before it, and then remove the dummy later on. So here's where
			// we do this stuff
			if (nSequNum == m_pApp->GetMaxIndex())
			{
				// a dummy is temporarily required
				m_bDummyAddedTemporarily = TRUE;

				// do the append
				pDummySrcPhrase = new CSourcePhrase;
				pDummySrcPhrase->m_srcPhrase = _T("dummy"); // something needed, so a 
															// pile width can be computed
				pDummySrcPhrase->m_key = pDummySrcPhrase->m_srcPhrase;
				pDummySrcPhrase->m_nSequNumber = m_pApp->GetMaxIndex() + 1;
				wxASSERT(pDummySrcPhrase->m_nSequNumber == nFollowingSequNum);
				SPList::Node* posTail;
				posTail = pList->Append(pDummySrcPhrase);
				wxUnusedVar(posTail); // avoid compiler warning

				// now we need to add a partner pile for it in CLayout::m_pileList
				pDoc->CreatePartnerPile(pDummySrcPhrase);

				// we need a valid layout which includes the new dummy element on 
				// its own pile; and the active location being the new max index value
				m_pApp->m_nActiveSequNum = m_pApp->GetMaxIndex();
#ifdef _NEW_LAYOUT
				m_pLayout->RecalcLayout(pList, keep_strips_keep_piles);
#else
				m_pLayout->RecalcLayout(pList, create_strips_keep_piles);
#endif
				m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->GetMaxIndex()); // temporary 
						// active location, at the dummy one, now we can do the insertion

				pInsertLocPile2 = m_pApp->m_pActivePile; // where our pFollSrcPhrase is located for the insert
				nSequNum = m_pApp->GetMaxIndex(); // the augmented max index value
			}
		} // end of TRUE block for test: if (bDoAppend)

		// There now is a following instance before which we can insert, and it
		// will associate leftwards
		SPList::Node* insertPos = pList->Item(nFollowingSequNum); // the position before which
			// we will make the insertion - insertion in wxList is always preceding insertPos
		pFollSrcPhrase = insertPos->GetData(); // it will be the dummy one if we want insertion after doc end

		// BEW 24Apr20 We need to set  pSrcPhraseInsLoc (and pPostInsertLocSrcPhrase?) here, 
		// transferred legacy code to work. Otherwise they are undefined, but only the first
		// is needed for a leftwards association when inserting "after"
		CSourcePhrase* pPostInsertLocSrcPhrase = pFollSrcPhrase;
		wxUnusedVar(pPostInsertLocSrcPhrase); // left-assocation doesn't need it
		// And the previous one - where we'll get word-final stuff if it exists, to move rightwards
		SPList::Node* insertPosBefore = pList->Item(nFollowingSequNum - 1);
		CSourcePhrase* pSrcPhraseInsLoc = insertPosBefore->GetData();

		// BEW added to, 17Feb10, to support doc version 5 -- if the final non-placeholder
		// instance's m_endMarkers member has content, then that content has to be cleared out
		// and transferred to the last of the inserted placeholders - this is true in
		// retranslations, and also for non-retranslation placeholder insertion following a
		// CSourcePhrase with m_endMarkers with content and the association is leftwards. 
		// So in the next section of code, deal with the 'in retranslation' case; which is a
		// left-association, and then any free translation mode case, which is also 
		// left associating

		// Since the list insertion will occur before nFollowingSequNum, the nStartingSequNum
		// has become the "previous" sequence number - so we'll need to preserve that and
		// get it's CSourcePhrase instance too - that's where we'll be looking for endmarkers etc
		int nPrevSequNum = nStartingSequNum; // see line about 80 lines above
		if (nPrevSequNum > 0)
		{
			SPList::Node* prevPos = pList->Item(nPrevSequNum); // the position we'll examine for
																// endmarker or final puncts transfers
			// Get its CSourcePhrase instance
			pPrevSrcPhrase = prevPos->GetData(); // this is what we examine

			// Test for the m_bEndFreeTrans boolean being TRUE, if it is, moving
			// the boolean is required when dealing with a retranslation
			if (pPrevSrcPhrase->m_bEndFreeTrans)
			{
				if (bForRetranslation)
				{
					pPrevSrcPhrase->m_bEndFreeTrans = FALSE; // we'll make it TRUE at placeholder
					// only code in a "for retranslation" block will look at the
					// bMoveEndOfFreeTrans boolean
					bMoveEndOfFreeTrans = TRUE;
				}
			}
			// Next, test for endmarker or endmarkers needing transfer - set
			if (!pPrevSrcPhrase->GetEndMarkers().IsEmpty() ||
				!pPrevSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty() ||
				!pPrevSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
			{
				bTransferEndMarkers = TRUE;
				if (!pPrevSrcPhrase->GetEndMarkers().IsEmpty())
				{
					endmarkersToTransfer = pPrevSrcPhrase->GetEndMarkers();
					pPrevSrcPhrase->SetEndMarkers(emptyStr); // empty, we set them on the following srcPhrase
				}
				if (!pPrevSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
				{
					inlineNonbindingEndmarkersToTransfer = pPrevSrcPhrase->GetInlineNonbindingEndMarkers();
					pPrevSrcPhrase->SetInlineNonbindingEndMarkers(emptyStr); // empty, we set them on the following srcPhrase
				}
				if (!pPrevSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
				{
					inlineBindingEndmarkersToTransfer = pPrevSrcPhrase->GetInlineBindingEndMarkers();
					pPrevSrcPhrase->SetInlineBindingEndMarkers(emptyStr); // empty, we set them on the following srcPhrase
				}
			} // end of TRUE block for an endmarker or endmarkers to move to the placeholder

		} // end of TRUE block for test: if (nPrevSequNum > 0)

		// BEW 21Jul14, Set a default src text wordbreak string for the placeholder
		// - get it from the previous instance
		wxString defaultWordBreak = _T(" "); // a space, in case there is no previous CSourcePhrase
		if (pPrevSrcPhrase != NULL)
		{
			defaultWordBreak = pPrevSrcPhrase->GetSrcWordBreak();
		}
		// get the sequ num for the wxList's insertion location  -  
		// (it could be quite diff from active sequ num)
		int	nSequNumInsLoc = pFollSrcPhrase->m_nSequNumber;
		wxASSERT(nSequNumInsLoc >= 0 && nSequNumInsLoc <= m_pApp->GetMaxIndex());
		wxASSERT(insertPos != NULL); // the wxList's Node's position for inserting - see above

		// save the old active sequ num location, so we can restore later on, 
		// since the call to RecalcLayout() will clobber pointers
		int nActiveSequNum = m_pApp->m_nOldSequNum; // RHS set above at beginning of the 'after' block

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
			if (pFollSrcPhrase->m_curTextType == footnote &&
				pFollSrcPhrase->m_bFootnote == FALSE)
				m_pApp->GetRetranslation()->SetIsInsertingWithinFootnote(TRUE);
		}

		// create the needed null source phrases and insert them in the list; 
		// preserve pointers to the first and last for use further below
		CSourcePhrase* pFirstOne = NULL; // whm initialized to NULL
		CSourcePhrase* pLastOne = NULL; // whm initialized to NULL

		for (int i = 0; i < nCount; i++)
		{
			CSourcePhrase* pSrcPhrasePH = new CSourcePhrase; // PH means 'PlaceHolder'
			if (i == 0)
			{
				pFirstOne = pSrcPhrasePH;
				pFollSrcPhrase = pSrcPhrasePH; // BEW 1Apr20, LHS was pSrcPhrase (marked
					// as unknown), I changed to pFollSrcPhrase, as that was  the
					// insert location before which placeholder(s) get inserted, so will
					// take its sequence number in the updating of the order  <<--  I think this is the correct thinking ****
			}
			if (i == nCount - 1)
			{
				pLastOne = pSrcPhrasePH;
			}
			pSrcPhrasePH->m_bNullSourcePhrase = TRUE;
			pSrcPhrasePH->m_srcPhrase = _T("...");
			pSrcPhrasePH->m_key = _T("...");
			pSrcPhrasePH->m_nSequNumber = nStartingSequNum + i; // ensures the
					// UpdateSequNumbers() call starts with correct value
			// Handle retranslation end padding...
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
				// and in the this function itself. We have to get the type right, because
				// the user might output interlinear RTF with footnote suppression wanted,
				// so we have to ensure that these placeholders have the footnote TextType set 
				// so that the suppression will work properly
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

			pList->Insert(insertPos, pSrcPhrasePH); // insertPos (line 3326) set above at pFollSrcPhrase

			// BEW added 13Mar09 for refactored layout
			pDoc->CreatePartnerPile(pSrcPhrasePH);

		} // end of for loop for inserting one or more placeholders
			// BEW 11Oct10, NOTE: transfer, for insertions in a retranslation span, of m_follPunct
			// data and m_follOuterPunct data could have been done above; for no good reason though
			// they are done below instead - in the else block of the next major test

		// fix the sequ num for the insert location's source phrase (i.e. it was created
		// from pFollSrcPhrase's m_nSequNumber value above, in this "insert after" block),
		// and that was augmented by 1 to allow a wxList insert to be done from pFollSrecPhrase;
		// so here we need to remove the + 1; and then add the nCount. Next line written that
		// way to document why we don't just add nCount, nor nCount - 1.
		nSequNumInsLoc = (nSequNumInsLoc - 1) + nCount;

		// calculate the new active sequ number - it could be anywhere, but all we need to know
		// is whether or not the insertion was done preceding the former active sequ number's
		// location, or not
		if (nStartingSequNum <= nActiveSequNum)
		{
			// This is where the new active location should be moved to, if possible
			m_pApp->m_nActiveSequNum = nActiveSequNum + nCount;
		}
		// update the sequence numbers, starting from the first one inserted (although the
		// layout of the view is no longer valid, the relationships between piles,
		// CSourcePhrases and sequence numbers will be correct after the following call, and so
		// we can, for example, get the correct pile pointer for a given sequence number and
		// from the pointer, get the correct CSourcePhrase)
		m_pView->UpdateSequNumbers(nStartingSequNum);  // starts the renumbering at nStartingSequNum

		// Now tease apart the legacy association direction code. We here are in the block for
		// "insert ... after active location" and so we want the associating leftwards code.
		// In our refactored code, there is always an association direction in place... so
		bool bAssociationRequired = FALSE;  // initialize this to FALSE
		//wxUnusedVar(bAssociationRequired);
		// BEW 17Feb10 we have to consider association direction when there is a non-empty
		// m_endMarkers member preceding the inserted placeholder; because association is
		// to the left, then we must move the m_endMarkers content on to the inserted placeholder.
		// In fact, our richer document model means we have to look for inline binding end markers,
		// and inline non-binding endmarkers, and move them likewise.
		// A a copy of the endMarkers on the preceding CSourcePhrase has already been obtained above, 
		// it is in the local variable endmarkersToTransfer (as of docVersion = 5). Likewise, the local
		// string variables: inlineNonbindingEndmarkersToTransfer and inlineBindingEndmarkersToTransfer

		// Set or clear bAssociationRequired;
		if (!endmarkersToTransfer.IsEmpty())
			bAssociationRequired = TRUE;
		if (!inlineNonbindingEndmarkersToTransfer.IsEmpty())
			bAssociationRequired = TRUE;
		if (!inlineBindingEndmarkersToTransfer.IsEmpty())
			bAssociationRequired = TRUE;
		// Is there final punctuation needing to be moved?
		wxString finalPuncts = pPrevSrcPhrase->m_follPunct;
		wxString finalPunctsOuter = pPrevSrcPhrase->GetFollowingOuterPunct();
		if (!bAssociationRequired)
		{
			// Not yet required, but check the final punctuation strings for content
			if (!finalPuncts.IsEmpty())
				bAssociationRequired = TRUE;
			if (!finalPunctsOuter.IsEmpty())
				bAssociationRequired = TRUE;
		}

		// The following 2 booleans are only looked at by code for "not-for-retranslation" blocks

		bool bPreviousFollPunct = FALSE; // true if the leftwards-of-placeholder 
				//  CSourcePhrase has one or both of m_follPunct and m_follOuterPunct having content
		bool bEndMarkersPrecede = bTransferEndMarkers; // set true when the leftwards-of-placeholder
				// srcPhrase has endmarkers from any or all of the 3 endmarker storage members -
				// m_endMarkers, m_inlineBindingEndMarkers, m_inlineNonbindingEndMarkers
		if (!pCurrentSrcPhrase->m_follPunct.IsEmpty())
		{
			bPreviousFollPunct = TRUE;
		}
		if (!pCurrentSrcPhrase->GetFollowingOuterPunct().IsEmpty())
		{
			bPreviousFollPunct = TRUE;
		}

		// This is a major block for when inserting manually (ie. not in a retranslation), and
		// manual insertions are always only a single placeholder per insertion command by the
		// user, and so nCount in here will only have the value 1. (Retranslations handled above)
		if (!bForRetranslation)
		{
			// First, handle sourcetext word-break (swbk). The inserted placeholder will be given
			// a default word-break of a single Latin space, or is just simply empty as yet. In 
			// Asian languages, since we are left associating here, the text to the left might be 
			// delimiting words with a zero-width space (ZWSP), or there may be a newline. If its
			// a newline then a space will suffice before the placeholder, but otherwise we need 
			// to make sure that the placeholder's CSourcePhrase stores a word delimitation value
			// that is the same as what precedes the word (or phrase) at pInsertLocPile
			wxString defaultWordBreak = _T(" ");
			pLastOne->SetSrcWordBreak(defaultWordBreak); // give the placeholder a safe default
			wxString previousWordBreak = wxEmptyString;
			wxString strNewline = _T('\n');
			if (pCurrentSrcPhrase != NULL)
			{
				previousWordBreak = pCurrentSrcPhrase->GetSrcWordBreak(); // could be space, newline, ZWSP, ...
			}
			if (defaultWordBreak != previousWordBreak)
			{
				if (previousWordBreak.IsEmpty())
				{
					// Space will have to do
					pLastOne->SetSrcWordBreak(defaultWordBreak);
				}
				else // it's not empty
				{
					// Is it a newline?
					if (previousWordBreak == strNewline)
					{
						pLastOne->SetSrcWordBreak(defaultWordBreak);
					}
					else
					{
						// Give it the left-of-Placeholder's CSourcePhrase value for the word-break string
						pLastOne->SetSrcWordBreak(previousWordBreak);
					}
				}
			} // end of TRUE block for test: if (defaultWordBreak != previousWordBreak)

			// BEW 11Oct10, warn left association is not permitted when a word pair
			// joined by the USFM ~ (fixed-space indicator symbol) precedes
			if (IsFixedSpaceSymbolWithin(pCurrentSrcPhrase))
			{

                // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                m_pApp->m_bUserDlgOrMessageRequested = TRUE;

                wxMessageBox(_(
					"Two words are joined with fixed-space marker ( ~ ) preceding the placeholder.\nAssociation with the two words is not possible."),
					_("Warning: Unacceptable Backwards Association"), wxICON_EXCLAMATION | wxOK);
				goto m; // make no further adjustments, just jump to the RecalcLayout() call near the end
			}
			// the association is to the text which precedes, so transfer from there
			// to the last in the list, or the only one if nCount is 1

			if (bEndMarkersPrecede)
			{
				// these have to be moved to the placeholder, pLastOne; the 3 strings were
				// set further up in the function, and their pCurrentSrcPhrase members for
				// these were cleared above too
				pLastOne->SetEndMarkers(endmarkersToTransfer);
				pLastOne->SetInlineNonbindingEndMarkers(inlineNonbindingEndmarkersToTransfer);
				pLastOne->SetInlineBindingEndMarkers(inlineBindingEndmarkersToTransfer);
			}
			if (bPreviousFollPunct)
			{
				// transfer the following punctuation
				pLastOne->m_follPunct = pCurrentSrcPhrase->m_follPunct;
				pCurrentSrcPhrase->m_follPunct.Empty();

				pLastOne->SetFollowingOuterPunct(pCurrentSrcPhrase->GetFollowingOuterPunct());
				pCurrentSrcPhrase->SetFollowingOuterPunct(emptyStr);

				// do an adjustment of the m_targetStr member (because it has just lost
				// its following punctuation to the placeholder), simplest solution is
				// to make it same as the m_adaption member
				pCurrentSrcPhrase->m_targetStr = pCurrentSrcPhrase->m_adaption;
			}
			// BEW 21Jul14, Do any needed right association for wordbreaks
			wxString aWordBreak = _T("");
			aWordBreak = pSrcPhraseInsLoc->GetSrcWordBreak();
			pFirstOne->SetSrcWordBreak(aWordBreak);
			// now copy the following one forward as explained above
			//aWordBreak = pPostInsertLocSrcPhrase->GetSrcWordBreak();
			//pSrcPhraseInsLoc->SetSrcWordBreak(aWordBreak);

			// Now handle Left-Association effects
			// BEW 11Oct10, warn left association is not permitted when a word pair
			// joined by USFM ~ fixed space precedes
			if (IsFixedSpaceSymbolWithin(pPrevSrcPhrase))
			{

                // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                m_pApp->m_bUserDlgOrMessageRequested = TRUE;

                wxMessageBox(_(
					"Two words are joined with fixed-space marker ( ~ ) preceding the placeholder.\nBackwards association is not possible when this happens."),
					_T("Warning: Unacceptable Backwards Association"), wxICON_EXCLAMATION | wxOK);
				goto m; // make no further adjustments, just jump to the 
						// RecalcLayout() call near the end
			}

		} // end  of TRUE block for test: if (!bForRetranslation)

		else // next block is for Retranslation situation
		{
			// BEW 24Sep10, simplified & corrected a logic error here that caused final
			// punctuation to not be moved to the last placeholder inserted...; and as of
			// 11Oct10 we also have to move what is in m_follOuterPunct.
			// We are inserting to pad out a retranslation, so if the last of the selected
			// source phrases has following punctuation, we need to move it to the last
			// placeholder inserted (note, the case of moving free-translation-supporting bool
			// values is done above, as also is the moving of the content of a final non-empty
			// m_endMarkers member, and m_inlineNonbindingEndMarkers and
			// m_inlineBindingEndMarkers to the last placeholder, in the insertion loop itself)
			// 
			// BEW 21Jul14, in this situation, no moving of m_srcWordBreak contents is
			// involved here - because the padding instances get the last CSourcePhrase's
			// m_scrWordBreak contents copied to them in the caller when the padding is done.
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
			//BEW 24Apr20, pSrcPhrase is used in this copied legacy code; pPrevSrcPhrase
			// is still  defined here, but pSrcPhrase is not. It's clear from the
			// code that pSrcPhrase is to be identified as pFollSourcePhrase, so define it
			CSourcePhrase* pSrcPhrase = pLastOne;

			// we've done the retranslation placeholder additions case in a block previously,
			// so here we are interested in the single placeholder inserted not in any
			// retranslation
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
								if (bAssociationRequired && bAssociateLeftwards)
								{
									// we have to make the placeholder the end of the section
									// on the left
									pPrevSrcPhrase->m_bEndFreeTrans = FALSE;
									pSrcPhrase->m_bEndFreeTrans = TRUE;
									pSrcPhrase->m_bHasFreeTrans = TRUE;
								}
								else if (!bAssociationRequired)
								{
									// no punctuation in the context,  
									// we will associate it to the left
									pPrevSrcPhrase->m_bEndFreeTrans = FALSE;
									pSrcPhrase->m_bEndFreeTrans = TRUE;
									pSrcPhrase->m_bHasFreeTrans = TRUE;
								}
							} // end of TRUE block for test: if (pSrcPhraseInsLoc->m_bStartFreeTrans)
							else // the pSrcPhraseInsertLoc is not the start of a new fr tr section
							{
								// impossible situation (pPrevSrcPhrase is the end of an
								// earlier free translation section, so pSrcPhraseInsLoc
								// must be the starting one of a new section if it is in a
								// free translation section - so do nothing
								;
							}
						} // end of TRUE block for test: if (pSrcPhraseInsLoc->m_bHasFreeTrans)
					} // end of TRUE block for test: if (pPrevSrcPhrase->m_bEndFreeTrans)
					else
					{
						// the previous sourcephrase starts a free translation section, and
						// because it is not also the end of the section, the section must
						// extend past the inserted placeholder - so the placeholder is
						// within the section, so we only need set one flag
						pSrcPhrase->m_bHasFreeTrans = TRUE;
					}
				} // end of TRUE block for test: if (pPrevSrcPhrase->m_bStartFreeTrans)
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
							if (bAssociationRequired && bAssociateLeftwards)
							{
								// associate to the left, so move the end of the previous
								// section to the inserted placeholder
								pPrevSrcPhrase->m_bEndFreeTrans = FALSE;
								pSrcPhrase->m_bEndFreeTrans = TRUE;
								pSrcPhrase->m_bHasFreeTrans = TRUE;
							}
							else if (!bAssociationRequired)
							{
								// associate leftwards - give use no choice
								pPrevSrcPhrase->m_bEndFreeTrans = FALSE;
								pSrcPhrase->m_bEndFreeTrans = TRUE;
								pSrcPhrase->m_bHasFreeTrans = TRUE;
								
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
						if (bAssociationRequired && bAssociateLeftwards)
						{
							// leftwards association was already requested, so make the
							// placeholder the new end of the previous section of free
							// translation
							pPrevSrcPhrase->m_bEndFreeTrans = FALSE;
							pSrcPhrase->m_bEndFreeTrans = TRUE;
							pSrcPhrase->m_bHasFreeTrans = TRUE;
						}
						else if (!bAssociationRequired)
						{
								pPrevSrcPhrase->m_bEndFreeTrans = FALSE;
								pSrcPhrase->m_bEndFreeTrans = TRUE;
								pSrcPhrase->m_bHasFreeTrans = TRUE;
						}
					}
				} // end of TRUE block for test: else if (pPrevSrcPhrase->m_bEndFreeTrans)
				else
				{
					// the previous sourcephrase neither starts nor ends a free translation
					// section, so it is within a free translation section - we just need
					// to set the one flag
					pSrcPhrase->m_bHasFreeTrans = TRUE;
				}
			}   // end of TRUE block for test: if (pPrevSrcPhrase->m_bHasFreeTrans)
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
						if (bAssociationRequired && bAssociateLeftwards)
						{
							// leftwards association - so nothing is to be done
							;
						}
					}
					else
					{
						// impossible situation (a free trans section starting without
						// m_bStartFreeTrans having the value TRUE)
						;
					}

				} //end of TRUE block for test: if (pSrcPhraseInsLoc->m_bHasFreeTrans)
				else
				{
					// pSrcPhraseInsLoc has no free translation defined on it, so nothing
					// needs to be done to the default flag values (all 3 FALSE) for free
					// translation support in pSrcPhrase
					;
				}
			} // end of else block for test: if (pPrevSrcPhrase->m_bHasFreeTrans)

		} // end of TRUE block for test: if (!bForRetranslation)



	}  // end of else block for test: if (bInsertBefore) i.e. ends "inserting after"

	/*  After block ends */

	// recalculate the layout
#ifdef _NEW_LAYOUT
	m : m_pLayout->RecalcLayout(pList, keep_strips_keep_piles);
#else
	m:	m_pLayout->RecalcLayout(pList, create_strips_keep_piles);
#endif
	m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);
	wxASSERT(m_pApp->m_pActivePile);

	// don't draw the layout and the phrase box when this function is called
	// as part of a larger inclusive procedure
	if (bRestoreTargetBox)
	{
		/* BEW 25Apr20 - the phrasebox, at the placeholder, must always come up empty
		// whm 13Aug2018 Note: The SetFocus() correctly precedes the 
		// SetSelection(m_pApp->m_nStartChar, m_pApp->m_nEndChar) call below it.
		m_pApp->m_pTargetBox->GetTextCtrl()->SetFocus();
		// whm 3Aug2018 Note: The following SetSelection restores any previous
		// selection, so no adjustment for 'Select Copied Source' needed here.
		m_pApp->m_pTargetBox->GetTextCtrl()->SetSelection(m_pApp->m_nStartChar, m_pApp->m_nEndChar);
		*/
		m_pApp->m_targetPhrase.Empty(); // clear the string
		m_pApp->m_pTargetBox->GetTextCtrl()->Clear();
		m_pApp->m_pTargetBox->GetTextCtrl()->SetFocus();
		m_pApp->m_pTargetBox->GetTextCtrl()->SetSelection(0, 0);

		// scroll into view, just in case a lot were inserted
		m_pApp->GetMainFrame()->canvas->ScrollIntoView(m_pApp->m_nActiveSequNum);

		m_pView->Invalidate();
		m_pLayout->PlaceBox();
	}
	m_pApp->GetRetranslation()->SetIsInsertingWithinFootnote(FALSE);
}

void CPlaceholder::OnButtonNullSrcLeft(wxCommandEvent& event)
{
	CAdapt_ItApp* pApp = m_pApp;
	//CAdapt_ItDoc* pDoc = pApp->GetDocument();
	wxUnusedVar(event);

	CMainFrame* pFrame = m_pApp->GetMainFrame();
	wxASSERT(pFrame != NULL);
	wxAuiToolBarItem *tbi;
	tbi = pFrame->m_auiToolbar->FindTool(ID_BUTTON_NULL_SRC_LEFT);
	// Return if the toolbar item is hidden or disabled															   
	if (tbi == NULL)
	{
		::wxBell();
		return;
	}
	// whm 20Mar2020 modified test below to detect whether the insert placeholder 
	// to left button is disabled, if so abort insertion
    // edb 19Aug2021: event order is different on the Mac (M1 in particular); the placeholder button
    // was getting the focus before our test here, which caused the "is the current focus the target"
    // test in OnUpdateButtonNullSrc() to fail... causing the toolbar button to become disabled.
    // The solution here is to bypass the button enabled check -- I'm not seeing disabled buttons triggering
    // button click events on the Mac.
#ifndef __WXMAC__
    if (!pFrame->m_auiToolbar->GetToolEnabled(ID_BUTTON_NULL_SRC_LEFT))
    {
        ::wxBell();
        return;
    }
#endif
	if (gbIsGlossing)
	{
		//IDS_NOT_WHEN_GLOSSING

        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;

        wxMessageBox(_(
			"This particular operation is not available when you are glossing."),
			_T(""), wxICON_INFORMATION | wxOK);
		return;
	}

	// whm 30Dec2024 remove the following call of DoInsertPlaceholder() with the coding used in
	// PhraseBox.cpp's OnSysKeyUp() handler detecting Shift+Alt+LeftArrow hotkey
	/*
	CPile* pInsertLocPile = pApp->m_pActivePile;
	int nCount = 1; // The button or shortcut can only insert one
	bool bRestoreTargetBox = TRUE;
	bool bForRetranslation = FALSE;
	bool bInsertBefore = TRUE;
	bool bAssociateLeftwards = FALSE; // we want rightwards association

	DoInsertPlaceholder(pDoc, pInsertLocPile, nCount, bRestoreTargetBox,
		bForRetranslation, bInsertBefore, bAssociateLeftwards);
	*/
	// whm 30Dec2024 the following coding is more robust than the coding commented out above.
	// Insert of null sourcephrase but first save old sequ number in case required for toolbar's
	// Back button

	// Bill wanted the behaviour modified, so that if the box's m_bAbandonable flag is TRUE
	// (ie. a copy of source text was done and nothing typed yet) then the current pile
	// would have the box contents abandoned, nothing put in the KB, and then the placeholder
	// inserion - the advantage of this is that if the placeholder is inserted immediately
	// before the phrasebox's location, then after the placeholder text is typed and the user
	// hits ENTER to continue looking ahead, the former box location will get the box and the
	// copy of the source redone, rather than the user usually having to edit out an unwanted
	// copy from the KB, or remember to clear the box manually. A sufficient thing to do here
	// is just to clear the box's contents.
	if (pApp->m_pTargetBox->m_bAbandonable)
	{
		pApp->m_targetPhrase.Empty();
		if (pApp->m_pTargetBox != NULL
			&& (pApp->m_pTargetBox->IsShown()))
		{
			pApp->m_pTargetBox->GetTextCtrl()->ChangeValue(_T(""));
		}
	}

	// now do the 'insert before'
	pApp->m_nOldSequNum = pApp->m_nActiveSequNum;
	pApp->GetPlaceholder()->InsertNullSrcPhraseBefore();
	// whm 30Dec2024 Insertion of of a placeholder can potentially disrupt some of the display.
	// Best to do a m_pApp->GetMainFrame()->SendSizeEvent() call here to get the display back 
	// in shape.
	pApp->GetMainFrame()->SendSizeEvent();


	return;

}

void CPlaceholder::OnButtonNullSrcRight(wxCommandEvent& event)
{
	CAdapt_ItApp* pApp = m_pApp;
	//CAdapt_ItDoc* pDoc = pApp->GetDocument();
	wxUnusedVar(event);

	CMainFrame* pFrame = m_pApp->GetMainFrame();
	wxASSERT(pFrame != NULL);
	wxAuiToolBarItem *tbi;
	tbi = pFrame->m_auiToolbar->FindTool(ID_BUTTON_NULL_SRC_RIGHT);
	// Return if the toolbar item is hidden or disabled															   
	if (tbi == NULL)
	{
		::wxBell();
		return;
	}
	// whm 20Mar2020 modified test below to detect whether the insert placeholder 
	// to right button is disabled, if so abort insertion
    // edb 19Aug2021: event order is different on the Mac (M1 in particular); the placeholder button
    // was getting the focus before our test here, which caused the "is the current focus the target"
    // test in OnUpdateButtonNullSrc() to fail... causing the toolbar button to become disabled.
    // The solution here is to bypass the button enabled check -- I'm not seeing disabled buttons triggering
    // button click events on the Mac.
#ifndef __WXMAC__
	if (!pFrame->m_auiToolbar->GetToolEnabled(ID_BUTTON_NULL_SRC_RIGHT))
	{
		::wxBell();
		return;
	}
#endif
    
	if (gbIsGlossing)
	{
		//IDS_NOT_WHEN_GLOSSING

        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;

        wxMessageBox(_(
			"This particular operation is not available when you are glossing."),
			_T(""), wxICON_INFORMATION | wxOK);
		return;
	}

	/*
	CPile* pInsertLocPile = pApp->m_pActivePile;
	int nCount = 1;  // The button or shortcut can only insert one
	bool bRestoreTargetBox = TRUE;
	bool bForRetranslation = FALSE;
	bool bInsertBefore = FALSE;
	bool bAssociateLeftwards = TRUE; // we want leftwards association

	DoInsertPlaceholder(pDoc, pInsertLocPile, nCount, bRestoreTargetBox,
		bForRetranslation, bInsertBefore, bAssociateLeftwards);
	*/
	// whm 30Dec2024 the following coding is more robust than the coding commented out above.
	// Insert of null sourcephrase but first save old sequ number in case required for toolbar's
	// Back button.
	// first save old sequ number in case required for toolbar's Back button
	// If glossing is ON, we don't allow the insertion, and just return instead
	pApp->m_nOldSequNum = pApp->m_nActiveSequNum;
	pApp->GetPlaceholder()->InsertNullSrcPhraseAfter();
	// whm 30Dec2024 Insertion of of a placeholder can potentially disrupt some of the display.
	// Best to do a m_pApp->GetMainFrame()->SendSizeEvent() call here to get the display back 
	// in shape.
	pApp->GetMainFrame()->SendSizeEvent();
	return;

}


#endif

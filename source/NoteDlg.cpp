/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			NoteDlg.cpp
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CNoteDlg class. 
/// The CNoteDlg class provides a dialog for creating, finding, editing, deleting,  
/// and navigating through Adapt It notes (those prefixed by \note).
/// The CNoteDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// \derivation		The CNoteDlg class is derived from wxScrollingDialog when built 
/// with wxWidgets prior to version 2.9.x, but derived from wxDialog for version 2.9.x 
/// and later.

/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "NoteDlg.h"
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

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator

#include <wx/tokenzr.h>
#include "Adapt_It.h"
#include "Pile.h"
#include "Layout.h"
#include "NoteDlg.h"
#include "Adapt_ItView.h"
#include "helpers.h"
#include "Adapt_ItDoc.h"
#include "Notes.h"

/// This global is defined in Adapt_It.cpp.
extern CPile* gpNotePile;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

extern const wxChar* filterMkr; // defined in the Doc, used here in OnLButtonDown() & free translation code, etc
extern const wxChar* filterMkrEnd; // defined in the Doc, used in free translation code, etc

/// This global is defined in Adapt_ItView.cpp.
extern bool	 gbShowTargetOnly;

int	   gnStartOffset = -1; // offset to beginning of matched note substring when searching
int    gnEndOffset = -1; // ending offset to the matched note substring
wxString gSearchStr; // a place to store the search string so it can be restored when a match was made in searching

// event handler table
// whm 14Jun12 modified to use wxDialog for wxWidgets 2.9.x and later; wxScrollingDialog for pre-2.9.x
#if wxCHECK_VERSION(2,9,0)
BEGIN_EVENT_TABLE(CNoteDlg, wxDialog)
#else
BEGIN_EVENT_TABLE(CNoteDlg, wxScrollingDialog)
#endif
	EVT_INIT_DIALOG(CNoteDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CNoteDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, CNoteDlg::OnCancel)
	EVT_BUTTON(IDC_NEXT_BTN, CNoteDlg::OnBnClickedNextBtn)
	EVT_BUTTON(IDC_DELETE_BTN, CNoteDlg::OnBnClickedDeleteBtn)
	EVT_BUTTON(IDC_PREV_BTN, CNoteDlg::OnBnClickedPrevBtn)
	EVT_BUTTON(IDC_FIRST_BTN, CNoteDlg::OnBnClickedFirstBtn)
	EVT_BUTTON(IDC_LAST_BTN, CNoteDlg::OnBnClickedLastBtn)
	EVT_BUTTON(IDC_FIND_NEXT_BTN, CNoteDlg::OnBnClickedFindNextBtn)
	EVT_TEXT(IDC_EDIT_FIND_TEXT, CNoteDlg::OnEnChangeEditBoxSearchText)
	EVT_IDLE(CNoteDlg::OnIdle)
END_EVENT_TABLE()


CNoteDlg::CNoteDlg(wxWindow* parent) // dialog constructor
// whm 14Jun12 modified to use wxDialog for wxWidgets 2.9.x and later; wxScrollingDialog for pre-2.9.x
#if wxCHECK_VERSION(2,9,0)
	: wxDialog(parent, -1, _("Note"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
#else
	: wxScrollingDialog(parent, -1, _("Note"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
#endif
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	NoteDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	pEditNote = (wxTextCtrl*)FindWindowById(IDC_EDIT_NOTE); // whm moved here to constructor
	wxASSERT(pEditNote != NULL);

	pEditSearch = (wxTextCtrl*)FindWindowById(IDC_EDIT_FIND_TEXT); // whm added
	wxASSERT(pEditSearch != NULL);
	
	pFindNextBtn = (wxButton*)FindWindowById(IDC_FIND_NEXT_BTN);
	wxASSERT(pFindNextBtn != NULL);

	pDeleteBtn = (wxButton*)FindWindowById(IDC_DELETE_BTN);
	wxASSERT(pDeleteBtn != NULL);

	wxButton* pOKButton;
	pOKButton = (wxButton*)FindWindow(wxID_OK);
	wxASSERT(pOKButton != NULL);
	pOKButton = pOKButton; // avoid warning

	m_strNote = _T("");
	m_searchStr = _T("");

	// other attribute initializations
}

CNoteDlg::~CNoteDlg() // destructor
{
	
}

// BEW added 18Oct06 in support of making either the Find Next button or the OK button
// the default button, depending on whether the m_editSearch CEdit box has text in it
// or not, respectively
// BEW 25Feb10, no change for doc version 5
void CNoteDlg::OnEnChangeEditBoxSearchText(wxCommandEvent& WXUNUSED(event))
{
	wxString boxTextStr = _T("");
	boxTextStr = pEditSearch->GetValue();
	if (boxTextStr.IsEmpty())
	{
		wxButton* pOKButton = (wxButton*)FindWindow(wxID_OK);
		wxASSERT(pOKButton != NULL);
		// nothing in the box, so make OK button the default button
		pOKButton->SetDefault();
	}
	else
	{
		// has some text, so make the Find Next button the default button
		pFindNextBtn->SetDefault();
	}
}

// BEW added 18Oct06 in support of making either the Find Next button or the OK button
// the default button, depending on whether the m_editSearch CEdit box has text in it
// or not, respectively
// whm 18Oct06 wx version uses OnIdle() instead of detecting focus event as MFC's 
// OnEnSetFocusEditBoxNote()
// BEW 25Feb10, no change for doc version 5
void CNoteDlg::OnIdle(wxIdleEvent& WXUNUSED(event))
{
	wxString boxTextStr = _T("");
	boxTextStr = pEditSearch->GetValue();
	if (boxTextStr.IsEmpty())
	{
		wxButton* pOKButton = (wxButton*)FindWindow(wxID_OK);
		wxASSERT(pOKButton != NULL);
		// nothing in the box, so make OK button the default button
		pOKButton->SetDefault();
	}
	else
	{
		// has some text, so make the Find Next button the default button
		pFindNextBtn->SetDefault();
		int len = boxTextStr.Length();
		long nFrom,nTo;
		pEditSearch->GetSelection(&nFrom,&nTo);
		if (nFrom != len && nTo != len) // only call SetSelection if needed to prevent flicker
		{
			pEditSearch->SetSelection(len,len);
		}
		// BEW added 6Mar08, it may have been the case that the current Note dialog instance
		// was opened as a result of a success search and match of a substring in the note
		// text's CEdit. If that was the case, then globals gnStartOffset and gnEndOffset
		// will still preserve the offsets for the required selection - so test and if 
		// successful, set up the selection
		if (!m_strNote.IsEmpty() && gnStartOffset > -1 && gnEndOffset > -1 && (gnEndOffset - gnStartOffset) > 0)
		{
			pEditNote->SetSelection(gnStartOffset,gnEndOffset);
		}

		gnStartOffset = -1;
		gnEndOffset = -1;
	}
}

// BEW 25Feb10, updated for support of doc version 5. Also, for docVersion 5 there is no need, when
// extracting the note text for showing in the dialog, to remove the text from the
// CSourcePhrase instance; rather, if the user subsequently clicks OK, then the text
// (possibly edited) just overwrites what is present, and if empty, the note is abandoned.
// BEW 25Feb10, updated for support of doc version 5
void CNoteDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	wxASSERT(this == gpApp->m_pNoteDlg);
	// get pointers to dialog controls
	pEditNote = (wxTextCtrl*)gpApp->m_pNoteDlg->FindWindowById(IDC_EDIT_NOTE); // whm moved to constructor
	pEditSearch = (wxTextCtrl*)gpApp->m_pNoteDlg->FindWindowById(IDC_EDIT_FIND_TEXT); // whm added

	//wxWindow* pOldFocusWnd = NULL; // BEW 5Mar08 added // set but unused

	// set up the nav text font in user's desired point size
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, pEditNote, pEditSearch,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, pEditNote, pEditSearch, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif

	// Locate the source phrase whose m_markers member contains the note text to be
	// displayed/edited. Its m_nSequNumber is stored in the m_nSequNumBeingViewed global
	CAdapt_ItView* pView = gpApp->GetView();
	CSourcePhrase* pSrcPhrase;
	pSrcPhrase = pView->GetSrcPhrase(gpApp->m_nSequNumBeingViewed);

	gpNotePile = pView->GetPile(gpApp->m_nSequNumBeingViewed);

	// if opening on an existing note, then get it (docVersion 5) from m_note member
	m_bPreExisting = TRUE;
	if (pSrcPhrase->m_bHasNote)
	{
		// get the existing Note text
		m_strNote = pSrcPhrase->GetNote();
		m_saveText = m_strNote;
	}
	else
	{
		m_bPreExisting = FALSE;
		m_strNote.Empty();
		m_saveText = m_strNote;
		gSearchStr.Empty(); // BEW added 6Mar08 to ensure a new note never starts with a search string defined
	}

	// wx note: we need to initialize dialog data since we aren't using UpdateData(FASLE) above
	// whm changed 1Apr09 SetValue() to ChangeValue() below so that is doesn't generate the wxEVT_COMMAND_TEXT_UPDATED
	// event, which now deprecated SetValue() generates.
	pEditNote->ChangeValue(m_strNote); // whm added 4Jul06
	pEditSearch->ChangeValue(m_searchStr); // whm added 4Jul06

	// whm added 29Mar12. Disable the pMkrTextEdit box and the "Remove ..." button
	// when in read-only mode
	if (gpApp->m_bReadOnlyAccess)
	{
		pEditNote->Disable();
		pDeleteBtn->Disable();
	}

	int len = m_strNote.Length();
	bool bSearchBoxHasFocus = FALSE; // BEW added 6Mar08
	int srchLen = 0; // ditto
	if (m_bPreExisting)
	{
		// there are two possibilities - the dialog was opened by searching and finding
		// a matching substring - in which case we want the substring shown hilighted, or
		// the user opened it some other way - in which case we'll put the cursor at the
		// end of the text. The test for the former is that gnStartOffset and gnEndOffset
		// are not -1
		if (gnStartOffset > -1 && gnEndOffset > -1 && (gnEndOffset - gnStartOffset) > 0)
		{
			// BEW removed 6Mar08, because setting the selection in note text's CEdit is
			// pointless when the code a bit further down in this block will put the focus
			// in the search box's CEdit and the cursor after the search text. (We instead
			// keep the gnStartOffset and gnEndOffset values alive until user clicks on
			// the note CEdit & OnEnSetFocusEditBoxNote() then gets its chance to use them,
			// or they are clobbered at the start of a new search in OnBnClickedFindNextBtn()
			//pEditNote->SetSelection(gnStartOffset,gnEndOffset);
			bSearchBoxHasFocus = TRUE;

		}
		else
		{
			// opened by any other criteria - such as user clicked the note icon
			// BEW changed 6MarO8, to permit a preexisting search string to be retained until
			// the user explicitly deletes it; in this block, since we are opening from a non-search
			// opening action, we just restore the string but otherwise ignore it
			pEditNote->SetSelection(len,len); // put the cursor at the end
			//gSearchStr.Empty(); // ensure it is empty
		}

		// restore any pre-existing search string
		m_searchStr = gSearchStr;

		// get the search string text visible
		// whm changed 1Apr09 SetValue() to ChangeValue() below so that is doesn't generate the wxEVT_COMMAND_TEXT_UPDATED
		// event, which now deprecated SetValue() generates.
		pEditSearch->ChangeValue(m_searchStr);

		// BEW added 6Mar08 so that user can give successive Enter keypresses to
		// step through several Notes without clobbering the matched text each time
		// in the pNote CEdit; we put input focus defaulting to the search string
		// CEdit, with cursor at the end of the search string
		srchLen = m_searchStr.Length();
	}
	else
	{
		pEditNote->SetSelection(0,len); // I've decided against 'instruction text' - as Cancel button's
							  // behaviour then gets more complicated
	}

	// get the button pointers
	pNextNoteBtn = (wxButton*)FindWindow(IDC_NEXT_BTN);
	pPrevNoteBtn = (wxButton*)FindWindow(IDC_PREV_BTN);
	pFirstNoteBtn = (wxButton*)FindWindow(IDC_FIRST_BTN);
	pLastNoteBtn = (wxButton*)FindWindow(IDC_LAST_BTN);;
	if (gpApp->m_bFreeTranslationMode || gbShowTargetOnly)
	{
		pNextNoteBtn->Enable(FALSE);
		pPrevNoteBtn->Enable(FALSE);
		pFirstNoteBtn->Enable(FALSE);
		pLastNoteBtn->Enable(FALSE);
	}

	// give the note CEdit the focus
	//pEditNote->SetFocus();
	// BEW altered 6Mar08, give the note CEdit the focus, but the search text's CEdit if 
	// the note was opened from a successful search
	if (bSearchBoxHasFocus)
	{
		// whm Note: the wxWindow::SetFocus() method does not return a handle to the previous window so
		// the following commented out MFC code line won't work:
		//pOldFocusWnd = pSearchBox->SetFocus();
		// instead we call FindFocus first the call SetFocus
		//pOldFocusWnd = FindFocus();
		pEditSearch->SetFocus();

		// also make the cursor be after the search text
		pEditSearch->SetSelection(srchLen,srchLen);
	}
	else
	{
		//pOldFocusWnd = FindFocus();
		pEditNote->SetFocus();
	}
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CNoteDlg::OnOK(wxCommandEvent& WXUNUSED(event)) 
{
	CAdapt_ItView* pView = gpApp->GetView();
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	if (gpApp->m_pNoteDlg != NULL)
	{
		m_strNote = pEditNote->GetValue(); // whm added 4Jul06
		m_searchStr = pEditSearch->GetValue(); // whm added
	}

	// mark the doc as dirty, so that Save command becomes enabled
	pDoc->Modify(TRUE);

	// insert the note, or if this is its first creation, construct the filtered
	// string and insert it - fortunately we have functions for doing these things
	wxString mkr = _T("\\note");
	wxString endMkr = _T("\\note*");

	SPList* pList = gpApp->m_pSourcePhrases;
	SPList::Node* pos = pList->Item(gpApp->m_nSequNumBeingViewed);		
	wxASSERT(pos != NULL);
	CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();		
	wxASSERT(pSrcPhrase);

    // we prefer not to have empty notes (we can't categorically prevent them because the
    // user may use the View Filtered Material dlg to clear out a note's content)...
	// *** Comment out the next 14 lines if we decide to allow empty notes (the rest of the
	// notes code in CNotes is done in such a way that they are supported if we allow them
	// to exist here)***
	if (m_strNote.IsEmpty())
	{
		pSrcPhrase->SetNote(m_strNote); // make m_note contain nothing
		pSrcPhrase->m_bHasNote = FALSE; // this makes the note non-existing, rather than just empty
		Destroy(); // the wxWidgets call to destroy the top level window (ie. the dialog)
		if (gpApp->m_pNoteDlg != NULL) // whm 11Jun12 added NULL test
			delete gpApp->m_pNoteDlg; // yes, it is required in wx to prevent crashes while navigating
		gpApp->m_pNoteDlg = NULL; // allow the View Filtered Material dialog to be opened
		gpNotePile = NULL;
		pView->RemoveSelection(); // in case a selection was used to indicate the note location
		gpApp->m_pLayout->Redraw(); // needed in order to get rid of the selection
		pView->Invalidate(); // so the note icon and green wedge show for the new note
		gpApp->m_pLayout->PlaceBox();
		return;
	}
	pSrcPhrase->SetNote(m_strNote); // overwrite whatever is in m_note already, if anything
	pSrcPhrase->m_bHasNote = TRUE;
	Destroy(); // destroy the dialog window (see comments at end for why)

	// wx version note: Compare to the ViewFilteredMaterialDlg where it is sometimes also necessary to
	// create a new dialog while the current one is open. In the case of ViewFilteredMaterialDlg, the
	// code first called its OnCancel() handler (because it was assumed there that the user wouldn't
	// want to save any changes to the edit box), before Destroy() on the old one and creating a new 
	// dialog with the new operator.
	// Here with the note dialog a similar thing happens, with dialogs needing to be programattically
	// closed and new ones created with the new operator. In this case, however, it is assumed that 
	// the user would want to save any Note edits, so OnOK() is called before Destroy() is called on 
	// the old one and a new one created with the new operator. With the case of the ViewFilteredMaterialDlg
	// I found it necessary to call delete on the App's dialog pointer, only in OnCancel. I assume
	// that delete should be called on the App's Note dialog pointer here in OnOK().
	if (gpApp->m_pNoteDlg != NULL) // whm 11Jun12 added NULL test
		delete gpApp->m_pNoteDlg; // yes, it is required in wx to prevent crashes while navigating
	gpApp->m_pNoteDlg = NULL; // allow the View Filtered Material dialog to be opened
	gpNotePile = NULL;
	pView->RemoveSelection(); // in case a selection was used to indicate the note location
	gpApp->m_pLayout->Redraw(); // needed in order to get rid of the selection
	pView->Invalidate();
	gpApp->m_pLayout->PlaceBox();
	//AIModalDialog::OnOK(event); // we are running modeless so don't call the base method
}


// other class methods
void CNoteDlg::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	// Allow the original state to remain - so just remove the dialog and refresh the view
	CAdapt_ItView* pView = gpApp->GetView();
	pView->RemoveSelection(); // in case a selection was used to indicate the note location
	//pView->Invalidate(); // so the note icon and green wedge show for the new note
	// Redraw has to be called after selection removal otherwise the selection remains
	// intact despite the Invalidate() call below
	gpApp->m_pLayout->Redraw(); // better than calling Invalidate() here, see previous line

	Destroy();
	gpApp->m_pNoteDlg = NULL; // allow the View Filtered Material dialog to be opened
	gpNotePile = NULL;
	pView->Invalidate();
	gpApp->m_pLayout->PlaceBox();
}

// ******************************  handlers   **********************************

void CNoteDlg::OnBnClickedNextBtn(wxCommandEvent& event)
{
	CAdapt_ItView* pView = gpApp->GetView();

	// mark the doc as dirty, so that Save command becomes enabled
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	pDoc->Modify(TRUE);

	if (gpApp->m_selectionLine != -1)
	{
		// if there is a selection, then remove the selection too
		pView->RemoveSelection();
	}
	// the rest of the possible scenarios should be okay
	gpApp->GetNotes()->OnButtonNextNote(event);
}

void CNoteDlg::OnBnClickedDeleteBtn(wxCommandEvent& event)
{
	m_strNote.Empty();
	if (gpApp->m_pNoteDlg != NULL)
	{
        // whm changed 1Apr09 SetValue() to ChangeValue() below so that is doesn't generate
        // the wxEVT_COMMAND_TEXT_UPDATED event, which now deprecated SetValue() generates.
		pEditNote->ChangeValue(m_strNote); // whm added 4Jul06
		pEditSearch->ChangeValue(m_searchStr); // whm added 4Jul06
	}
	OnOK(event); //OnBnClickedOk(event);
}

void CNoteDlg::OnBnClickedPrevBtn(wxCommandEvent& event)
{
	CAdapt_ItView* pView = gpApp->GetView();

	// mark the doc as dirty, so that Save command becomes enabled
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	pDoc->Modify(TRUE);

	if (gpApp->m_selectionLine != -1)
	{
		// if there is a selection, then remove the selection too
		pView->RemoveSelection();
	}
	// the rest of the possible scenarios should be okay
	gpApp->GetNotes()->OnButtonPrevNote(event);
}

void CNoteDlg::OnBnClickedFirstBtn(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItView* pView = gpApp->GetView();

	// mark the doc as dirty, so that Save command becomes enabled
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	pDoc->Modify(TRUE);

	if (gpApp->m_selectionLine != -1)
	{
		// if there is a selection, then remove the selection too
		pView->RemoveSelection();
	}
	// the rest of the possible scenarios should be okay
	gpApp->GetNotes()->MoveToAndOpenFirstNote();
}

void CNoteDlg::OnBnClickedLastBtn(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItView* pView = gpApp->GetView();

	// mark the doc as dirty, so that Save command becomes enabled
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	pDoc->Modify(TRUE);

	if (gpApp->m_selectionLine != -1)
	{
		// if there is a selection, then remove the selection too
		pView->RemoveSelection();
	}
	// the rest of the possible scenarios should be okay
	gpApp->GetNotes()->MoveToAndOpenLastNote();
}

void CNoteDlg::OnBnClickedFindNextBtn(wxCommandEvent& event)
{
	// BEW added 6Mar08, ensure invalid offsets to prevent a failed match from resulting in
	// a spurious substring selection
	gnStartOffset = -1;
	gnEndOffset = -1;

	if (gpApp->m_pNoteDlg != NULL)
	{
		m_strNote = pEditNote->GetValue(); // whm added 4Jul06
		m_searchStr = pEditSearch->GetValue(); // whm added
	}

	if (m_searchStr.IsEmpty())
	{
		::wxBell(); // alert the user something is wrong
		return; // there is nothing to search for
	}

	// mark the doc as dirty, so that Save command becomes enabled
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	pDoc->Modify(TRUE);

	WordList* pWordList = new WordList; // to hold the word strings returned by the Tokenize() loop
	int numWords; // to hold the number of words which the Tokenize() loop returns
	wxString aWord;
	// delimiters for words can be spaces, tabs, newlines or CRs
	wxStringTokenizer tkz(m_searchStr,_T(" \n\r\t")); 
	while (tkz.HasMoreTokens())
	{
		aWord = tkz.GetNextToken();
		// add it to the list
		pWordList->Append(&aWord);
	}
	// determine the number of words
	numWords = pWordList->GetCount();
	wxASSERT(numWords > 0);

	// determine if another note lying ahead in the doc has the string
	// (beware, the notes may have tabs, carriage returns and newlines, so we cannot
	// assume only a single space between words - we'll instead try to match words, the
	// first and last of which might be subparts of a word)
	int nStartOffset;
	int nEndOffset;
	int nFoundSequNum = gpApp->GetNotes()->FindNoteSubstring(gpApp->m_nSequNumBeingViewed, 
										pWordList, numWords, nStartOffset, nEndOffset);
	if (nFoundSequNum == -1)
	{
		// the string was not found in any subsequent note - so tell this to the user
		//IDS_NO_MATCHING_NOTE
		wxMessageBox(
		_("Searching forward did not find a note with text matching that which you typed into the box."),
		_T(""), wxICON_INFORMATION | wxOK);
		if (pWordList != NULL) // whm 11Jun12 added NULL test
			delete pWordList;
		gnStartOffset = gnEndOffset = -1; // ensure the 'no match' condition is restored
		// BEW changed 6Mar08, so that search string is retained until user explicitly deletes it
		//gSearchStr.Empty(); // ensure it is empty
	}
	else
	{
		// the string was found, so go there
		int nJumpOffSequNum = nFoundSequNum;

		// enable the OnInitDialog() handler for the note dialog to display the matched
		// substring selected, and the search string to be restored
		gnStartOffset = nStartOffset;
		gnEndOffset = nEndOffset;
		gSearchStr = m_searchStr;

		// close the current note dialog which is still open, saving its note
		OnOK(event); //OnBnClickedOk(event);
		gpApp->m_pNoteDlg = NULL;
		if (pWordList != NULL) // whm 11Jun12 added NULL test
			delete pWordList;
	
		// jump to the found note and open it
		gpApp->GetNotes()->JumpForwardToNote_CoreCode(nJumpOffSequNum);
		
		// clear the offsets so subsequent openings of the dialog don't wrongly select
		// a spurious substring
		// BEW removed 6Mar08, to have it at the start of the function, so that the match
		// values will be available if the user clicks on the note text's CEdit after a search
		// which succeeds, so that OnEnSetFocusEditBoxNote() notification handler can set the
		// selection for the match before setting these values to -1 there
		//gnStartOffset = -1;
		//gnEndOffset = -1;
	}
}



/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			NoteDlg.cpp
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CNoteDlg class. 
/// The CNoteDlg class provides a dialog for creating, finding, editing, deleting,  
/// and navigating through Adapt It notes (those prefixed by \note).
/// The CNoteDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// \derivation		The CNoteDlg class is derived from wxDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in NoteDlg.cpp (in order of importance): (search for "TODO")
// 1. Clicking on First or Last (and other navigation) buttons, or on another note while current 
//    note is open results in a blank (phantom) note being displayed and a crash when trying to 
//    click Cancel, OK or any other button.
//
// Unanswered questions: (search for "???")
// 1. 
// 
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
#include "NoteDlg.h"
#include "Adapt_ItView.h"
#include "helpers.h"
#include "Adapt_ItDoc.h"

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
BEGIN_EVENT_TABLE(CNoteDlg, wxDialog)
	EVT_INIT_DIALOG(CNoteDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
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
	: wxDialog(parent, -1, _("Note"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	NoteDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);

	pEditNote = (wxTextCtrl*)FindWindowById(IDC_EDIT_NOTE); // whm moved here to constructor
	wxASSERT(pEditNote != NULL);

	pEditSearch = (wxTextCtrl*)FindWindowById(IDC_EDIT_FIND_TEXT); // whm added
	wxASSERT(pEditSearch != NULL);
	
	pFindNextBtn = (wxButton*)FindWindowById(IDC_FIND_NEXT_BTN);
	wxASSERT(pFindNextBtn != NULL);

	wxButton* pOKButton;
	pOKButton = (wxButton*)FindWindow(wxID_OK);
	wxASSERT(pOKButton != NULL);

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
// whm 18Oct06 wx version uses OnIdle() instead of detecting focus event as MFC's OnEnSetFocusEditBoxNote()
void CNoteDlg::OnIdle(wxIdleEvent& WXUNUSED(event)) // wx version uses OnIdle() instead of detecting focus event as MFC's OnEnSetFocusEditBoxNote()
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
		// BEW added 6Mar08, it may have been the case that the current Note dialog instance was opened as a result
		// of a success search and match of a substring in the note text's CEdit. If that was the case, then
		// globals gnStartOffset and gnEndOffset will still preserve the offsets for the required selection - so test
		// and if successful, set up the selection
		if (!m_strNote.IsEmpty() && gnStartOffset > -1 && gnEndOffset > -1 && (gnEndOffset - gnStartOffset) > 0)
		{
			pEditNote->SetSelection(gnStartOffset,gnEndOffset);
		}

		gnStartOffset = -1;
		gnEndOffset = -1;
	}
}


void CNoteDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
   
	wxASSERT(this == gpApp->m_pNoteDlg);
	// get pointers to dialog controls
	pEditNote = (wxTextCtrl*)gpApp->m_pNoteDlg->FindWindowById(IDC_EDIT_NOTE); // whm moved to constructor
	pEditSearch = (wxTextCtrl*)gpApp->m_pNoteDlg->FindWindowById(IDC_EDIT_FIND_TEXT); // whm added

	wxWindow* pOldFocusWnd = NULL; // BEW 5Mar08 added

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

	// if opening on an existing note, then extract the note text from the filtered stuff in m_markers
	m_bPreExisting = TRUE;
	if (pSrcPhrase->m_bHasNote)
	{
		// extract the existing Note text (the view variable m_note will store the string)
		// (extraction removes the note contents and trailing space from between the \note
		// and \note* markers, so that when we exit the dialog, we can copy in the text
		// (which might have been edited) -- Note: if there is no text left in the note when
		// the user has finished dealing with it, we will remove the note, markers, and 
		// filter markers from m_markers when we exit the dialog
		wxString mkr = _T("\\note");
		wxString endMkr = _T("\\note*");
		wxString markers = pSrcPhrase->m_markers;
		int nFound = markers.Find(mkr);
		if (nFound == -1)
		{
			// there is no \note marker in the markers string, and so we have nothing to
			// extract -- because of this, the m_bHasNote flag must be cleared
			pSrcPhrase->m_bHasNote = FALSE;
		}
		else
		{
			// the marker is present, so extract the note content, setting its offset and length,
			// and then clear its space in m_markers
			m_strNote = pView->GetExistingMarkerContent(mkr,endMkr,pSrcPhrase,m_noteOffset,m_noteLen);
			m_saveText = m_strNote;
			markers.Remove(m_noteOffset,m_noteLen);
			pSrcPhrase->m_markers = markers; // update it
		}
	}
	else
	{
		m_bPreExisting = FALSE;
		m_strNote.Empty();
		m_saveText = m_strNote;
		gSearchStr.Empty(); // BEW added 6Mar08 to ensure a new note never starts with a search string defined
	}

	// wx note: we need to initialize dialog data since we aren't using UpdateData(FASLE) above
	pEditNote->SetValue(m_strNote); // whm added 4Jul06
	pEditSearch->SetValue(m_searchStr); // whm added 4Jul06

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
		pEditSearch->SetValue(m_searchStr);

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
		pOldFocusWnd = FindFocus();
		pEditSearch->SetFocus();

		// also make the cursor be after the search text
		pEditSearch->SetSelection(srchLen,srchLen);
	}
	else
	{
		pOldFocusWnd = FindFocus();
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

	// do a reality check on m_markers - I noticed that the test document had sometimes an
	// m_markers string ending with a SF marker, and no trailing space. The space should be
	// present - so if we detect this, we can do an automatic silent correction now
	int markersLen = pSrcPhrase->m_markers.Length();
	if (markersLen >= 2)
	{
		// at least a backslash and a single character should be present, otherwise don't bother
		if (pSrcPhrase->m_markers.GetChar(markersLen - 1) != _T(' '))
		{
			pSrcPhrase->m_markers += _T(' ');
		}
	}

	wxString markers = pSrcPhrase->m_markers;

	int nFound = markers.Find(mkr);
	bool bInsertContentOnly;
	if (nFound == -1)
	{
		// there is no \note marker in the markers string, and so we are working
		// with a just-created note -- insert it provided it is not empty
		if (m_strNote.IsEmpty())
		{
			// we prefer not to have empty notes (we can't categorically prevent them
			// because the user may use the View Filtered Material dlg to clear out a
			// note's content)
			pSrcPhrase->m_bHasNote = FALSE;
			goto a;
		}
		else
		{
			// there is some text in the note, so construct filter markers and note markers and content
			// and insert it at the proper location
			bInsertContentOnly = FALSE;

			// we may have other content in m_markers, so we have to first find the proper insertion location
			m_noteOffset = pView->FindFilteredInsertionLocation(pSrcPhrase->m_markers,mkr);

			// ensure the note string ends with a single space, we'll leave internal tabs, carriage returns
			// and newlines - after all, this is only a note, not text for parsing for adaptation
			//m_strNote.Trim(_T(" \r\n\t")); //m_strNote.Trim(_T(" \r\n\t"));
			// Under wx .Trim appears to only remove spaces, so I'll insure the other whitespace chars are also
			// removed if present on the right end of m_strNote.
			while (m_strNote.Length() > 0 && (m_strNote[m_strNote.Length() -1] == _T(' ')
				|| m_strNote[m_strNote.Length() -1] == _T('\r')
				|| m_strNote[m_strNote.Length() -1] == _T('\n')
				|| m_strNote[m_strNote.Length() -1] == _T('\t')))
			{
				m_strNote.Remove(m_strNote.Length() -1,1);
			}
			m_strNote += _T(' '); // append the final single space

			// now build the total string and insert it all (final BOOL being FALSE accomplishes the
			// build of \~FILTER \note <contents of m_strNote> \note* \~FILTER* followed by a single space,
			// which is done internally
			pView->InsertFilteredMaterial(mkr,endMkr,m_strNote,pSrcPhrase,m_noteOffset,bInsertContentOnly);
			pSrcPhrase->m_bHasNote = TRUE;
		}
	}
	else
	{
		// we are dealing with a prexisting note, so we only need to save the new (or same) content
		bInsertContentOnly = TRUE;
		if (m_strNote.IsEmpty())
		{
			pView->RemoveContentWrappers(pSrcPhrase,mkr,m_noteOffset); // doesn't clear m_bHasNote
			pSrcPhrase->m_bHasNote = FALSE;
		}
		else
		{
			// do the insertion
			pView->InsertFilteredMaterial(mkr,endMkr,m_strNote,pSrcPhrase,m_noteOffset,bInsertContentOnly);
		}
	}
	pView->RemoveSelection(); // in case one was used to indicate the note location
	pView->Invalidate(); // so the note icon and green wedge show for the new note
	
	//OnOK(); // MFC commented out the base class call
	//wxDialog::OnOK(event); // not virtual in wxDialog
a:	Destroy();
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
	delete gpApp->m_pNoteDlg; // yes, it is required in wx to prevent crashes while navigating
	gpApp->m_pNoteDlg = NULL; // allow the View Filtered Material dialog to be opened
	gpNotePile = NULL;
	pView->Invalidate();

	//wxDialog::OnOK(event); // we are running modeless so don't call the base method
}


// other class methods
void CNoteDlg::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	// put back the original text if there was some, but if not, remove the note entirely
	CAdapt_ItView* pView = gpApp->GetView();
	if (gpApp->m_pNoteDlg != NULL)
	{
		m_strNote = pEditNote->GetValue(); // whm added 4Jul06
		m_searchStr = pEditSearch->GetValue(); // whm added
	}

	// insert the note, or if these is its first creation, construct the filtered
	// string and insert it - fortunately we have functions for doing these things
	wxString mkr = _T("\\note");
	wxString endMkr = _T("\\note*");

	SPList* pList = gpApp->m_pSourcePhrases;
	SPList::Node* pos = pList->Item(gpApp->m_nSequNumBeingViewed);		
	wxASSERT(pos != NULL);
	CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();		
	wxASSERT(pSrcPhrase);

	// do a reality check on m_markers - I noticed that the test document had sometimes an
	// m_markers string ending with a SF marker, and no trailing space. The space should be
	// present - so if we detect this, we can do an automatic silent correction now
	int markersLen = pSrcPhrase->m_markers.Length();
	if (markersLen >= 2)
	{
		// at least a backslash and a single character should be present, otherwise don't bother
		if (pSrcPhrase->m_markers.GetChar(markersLen - 1) != _T(' '))
		{
			pSrcPhrase->m_markers += _T(' ');
		}
	}

	wxString markers = pSrcPhrase->m_markers;

	int nFound = markers.Find(mkr);
	if (nFound == -1)
	{
		// there is no \note marker in the markers string, and so we are cancelling
		// a just-created note -- so whether empty or not, just remove it entirely
		pSrcPhrase->m_bHasNote = FALSE;
	}
	else
	{
		// we are dealing with a prexisting note, so we need to restore the original content
		bool bInsertContentOnly = TRUE; // unused
		m_strNote = m_saveText;
		
		// do the re-insertion of the original content
		pView->InsertFilteredMaterial(mkr,endMkr,m_strNote,pSrcPhrase,m_noteOffset,bInsertContentOnly);
	}
	pView->RemoveSelection(); // in case one was used to indicate the note location
	pView->Invalidate(); // so the note icon and green wedge show for the new note


	Destroy();
	gpApp->m_pNoteDlg = NULL; // allow the View Filtered Material dialog to be opened
	gpNotePile = NULL;
	pView->Invalidate();

	//wxDialog::OnCancel(event); //don't call base class because we are modeless
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
	pView->OnButtonNextNote(event);
}

void CNoteDlg::OnBnClickedDeleteBtn(wxCommandEvent& event)
{
	m_strNote.Empty();
	if (gpApp->m_pNoteDlg != NULL)
	{
		pEditNote->SetValue(m_strNote); // whm added 4Jul06
		pEditSearch->SetValue(m_searchStr); // whm added 4Jul06
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
	pView->OnButtonPrevNote(event);
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
	pView->MoveToAndOpenFirstNote();
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
	pView->MoveToAndOpenLastNote();
}

void CNoteDlg::OnBnClickedFindNextBtn(wxCommandEvent& event)
{
	// BEW added 6Mar08, ensure invalid offsets to prevent a failed match from resulting in
	// a spurious substring selection
	gnStartOffset = -1;
	gnEndOffset = -1;

	// get the search string updated
	CAdapt_ItView* pView = gpApp->GetView();
	if (gpApp->m_pNoteDlg != NULL)
	{
		m_strNote = pEditNote->GetValue(); // whm added 4Jul06
		m_searchStr = pEditSearch->GetValue(); // whm added
	}
	int nJumpOffSequNum;

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
	int nFoundSequNum = pView->FindNoteSubstring(gpApp->m_nSequNumBeingViewed, pWordList, numWords,
												nStartOffset, nEndOffset);
	if (nFoundSequNum == -1)
	{
		// the string was not found in any subsequent note - so tell this to the user
		//IDS_NO_MATCHING_NOTE
		wxMessageBox(_("Searching forward did not find a note with text matching that which you typed into the box."), _T(""), wxICON_INFORMATION);
		delete pWordList;
		gnStartOffset = gnEndOffset = -1; // ensure the 'no match' condition is restored
		// BEW changed 6Mar08, so that search string is retained until user explicitly deletes it
		//gSearchStr.Empty(); // ensure it is empty
	}
	else
	{
		// the string was found, so go there
		nJumpOffSequNum = nFoundSequNum;

		// enable the OnInitDialog() handler for the note dialog to display the matched
		// substring selected, and the search string to be restored
		gnStartOffset = nStartOffset;
		gnEndOffset = nEndOffset;
		gSearchStr = m_searchStr;

		// close the current note dialog which is still open, saving its note
		OnOK(event); //OnBnClickedOk(event);
		gpApp->m_pNoteDlg = NULL;
		delete pWordList;
	
		// jump to the found note and open it
		pView->JumpForwardToNote_CoreCode(nJumpOffSequNum);

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



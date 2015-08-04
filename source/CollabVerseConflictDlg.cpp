/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CollabVerseConflictDlg.cpp
/// \author			Bill Martin
/// \date_created	10 July 2015
/// \rcs_id $Id$
/// \copyright		2015 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CCollabVerseConflictDlg class. 
/// The CCollabVerseConflictDlg class provides the user with a dialog that is
/// used to choose the best version to send to the external editor when
/// conflicts have been detected at save time during collaboration.
/// \derivation		The CCollabVerseConflictDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "CollabVerseConflictDlg.h"
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
#include "Adapt_It.h"
#include "CollabUtilities.h"
#include "CollabVerseConflictDlg.h"

extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CCollabVerseConflictDlg, AIModalDialog)
	EVT_INIT_DIALOG(CCollabVerseConflictDlg::InitDialog)
	// Samples:
	EVT_CHECKLISTBOX(ID_CHECKLISTBOX_VERSE_REFS, CCollabVerseConflictDlg::OnCheckListBoxTickChange)
	EVT_LISTBOX(ID_CHECKLISTBOX_VERSE_REFS, CCollabVerseConflictDlg::OnListBoxSelChange)

	EVT_RADIOBUTTON(ID_RADIOBUTTON_USE_AI_VERSION, CCollabVerseConflictDlg::OnRadioUseAIVersion)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_RETAIN_PT_VERSION, CCollabVerseConflictDlg::OnRadioRetainPTVersion)

	EVT_BUTTON(ID_BUTTON_SELECT_ALL_VS, CCollabVerseConflictDlg::OnSelectAllVersesButton)
	EVT_BUTTON(ID_BUTTON_UNSELECT_ALL_VS, CCollabVerseConflictDlg::OnUnSelectAllVersesButton)
	EVT_BUTTON(wxID_OK, CCollabVerseConflictDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, CCollabVerseConflictDlg::OnCancel)
	//EVT_LEFT_DOWN(CCollabVerseConflictDlg::OnLeftBtnDown)
	EVT_TEXT(ID_TEXTCTRL_EDITABLE_PT_VERSION, CCollabVerseConflictDlg::OnPTorBEtextUpdated)
	EVT_BUTTON(ID_BUTTON_RESTORE, CCollabVerseConflictDlg::OnRestoreBtn)

	// ... other menu, button or control events
END_EVENT_TABLE()

CCollabVerseConflictDlg::CCollabVerseConflictDlg(wxWindow* parent, wxArrayPtrVoid* pConfArr) // dialog constructor
	: AIModalDialog(parent, -1, _("Choose The Best Verses To Transfer To Paratext"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pConflictDlgTopSizer = AI_PT_ConflictingVersesFunc(this, TRUE, TRUE);
	// The declaration is: AI_PT_ConflictingVersesFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	wxUnusedVar(pConflictDlgTopSizer);
	m_pApp = gpApp;  // was (CAdapt_ItApp*)&wxGetApp();
	pConflictsArray = pConfArr;

	// Setup dialog box control pointers below:
	pCheckListBoxVerseRefs = (wxCheckListBox*)FindWindowById(ID_CHECKLISTBOX_VERSE_REFS);
	wxASSERT(pCheckListBoxVerseRefs != NULL);

	pTextCtrlSourceText = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_READONLY_SOURCE_TEXT);
	wxASSERT(pTextCtrlSourceText != NULL);
	pTextCtrlSourceText->SetBackgroundColour(m_pApp->sysColorBtnFace);

	pTextCtrlAITargetVersion = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_READONLY_AI_VERSION);
	wxASSERT(pTextCtrlAITargetVersion != NULL);
	pTextCtrlAITargetVersion->SetBackgroundColour(m_pApp->sysColorBtnFace);

	pTextCtrlPTTargetVersion = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_EDITABLE_PT_VERSION);
	wxASSERT(pTextCtrlPTTargetVersion != NULL);
	pTextCtrlPTTargetVersion->SetBackgroundColour(m_pApp->sysColorBtnFace);

	pBtnSelectAllVerses = (wxButton*)FindWindowById(ID_BUTTON_SELECT_ALL_VS);
	wxASSERT(pBtnSelectAllVerses != NULL);

	pBtnUnSelectAllVerses = (wxButton*)FindWindowById(ID_BUTTON_UNSELECT_ALL_VS);
	wxASSERT(pBtnUnSelectAllVerses != NULL);

	pBtnTransferSelectedVerses = (wxButton*)FindWindowById(wxID_OK); // Note: this button acts as wxID_OK
	wxASSERT(pBtnTransferSelectedVerses != NULL);

	pBtnCancel = (wxButton*)FindWindowById(wxID_CANCEL);
	wxASSERT(pBtnCancel != NULL);

	pRadioUseAIVersion = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_USE_AI_VERSION);
	wxASSERT(pRadioUseAIVersion != NULL);

	pRadioRetainPTVersion = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_RETAIN_PT_VERSION);
	wxASSERT(pRadioRetainPTVersion != NULL);

	pStaticTextCtrlTopInfoBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_READONLY_TOP); // for substituting Paratext/Bibledit into %s and %s
	wxASSERT(pStaticTextCtrlTopInfoBox != NULL);
	pStaticTextCtrlTopInfoBox->SetBackgroundColour(m_pApp->sysColorBtnFace);

	pStaticInfoLine1 = (wxStaticText*)FindWindowById(ID_TEXT_INFO_1); 
	wxASSERT(pStaticInfoLine1 != NULL);

	pStaticInfoLine2 = (wxStaticText*)FindWindowById(ID_TEXT_INFO_2); // for substituting Paratext/Bibledit into %s
	wxASSERT(pStaticInfoLine2 != NULL);

	pStaticInfoLine3 = (wxStaticText*)FindWindowById(ID_TEXT_INFO_3); // for substituting Paratext/Bibledit into %s
	wxASSERT(pStaticInfoLine3 != NULL);

	pStaticInfoLine4 = (wxStaticText*)FindWindowById(ID_TEXT_INFO_4); // for substituting Paratext/Bibledit into %s
	wxASSERT(pStaticInfoLine4 != NULL);

	pStaticPTVsTitle = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_PT_VS_TITLE);
	wxASSERT(pStaticPTVsTitle != NULL);

	CurrentListBoxHighlightedIndex = 0;
	lastIndex = 0;

	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	wxUnusedVar(bOK); // avoid warning

	pTextCtrlPTTargetVersion->SetEditable(TRUE); // ensure it is editable
}

CCollabVerseConflictDlg::~CCollabVerseConflictDlg() // destructor
{
}

void CCollabVerseConflictDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	wxASSERT(pConflictsArray != NULL);
	
	size_t count = pConflictsArray->GetCount();
	size_t index;
	ConflictRes* pCR = NULL;
	for (index = 0; index < count; index++)
	{
		pCR = (ConflictRes*)pConflictsArray->Item(index);
		wxASSERT(pCR != NULL);
		verseRefsArray.Add(MakeVerseReference(pCR));
		sourceTextVsArray.Add(pCR->srcText);
		aiTargetTextVsArray.Add(pCR->AIText);
		ptTargetTextVsArray.Add(pCR->PTorBEText_original);
		ptTargetTextVsEditedArray.Add(pCR->PTorBEText_edited);
	}

	// Set font and directionality for the three edit boxes
	// For the "Source text of verse selected at left" edit box:
	#ifdef _RTL_FLAGS
	m_pApp->SetFontAndDirectionalityForDialogControl(m_pApp->m_pSourceFont, pTextCtrlSourceText, NULL,
													NULL, NULL, m_pApp->m_pDlgSrcFont, m_pApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	m_pApp->SetFontAndDirectionalityForDialogControl(m_pApp->m_pSourceFont, pTextCtrlSourceText, NULL, 
													NULL, NULL, m_pApp->m_pDlgSrcFont);
	#endif

	// For the "Translation of verse in Adapt It" edit box:
	#ifdef _RTL_FLAGS
	m_pApp->SetFontAndDirectionalityForDialogControl(m_pApp->m_pTargetFont, pTextCtrlAITargetVersion, NULL,
													NULL, NULL, m_pApp->m_pDlgTgtFont, m_pApp->m_bTgtRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	m_pApp->SetFontAndDirectionalityForDialogControl(m_pApp->m_pTargetFont, pTextCtrlAITargetVersion, NULL, 
													NULL, NULL, m_pApp->m_pDlgTgtFont);
	#endif

	// For the "Translation of verse in Paratext" edit box:
	#ifdef _RTL_FLAGS
	m_pApp->SetFontAndDirectionalityForDialogControl(m_pApp->m_pTargetFont, pTextCtrlPTTargetVersion, NULL,
													NULL, NULL, m_pApp->m_pDlgTgtFont, m_pApp->m_bTgtRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	m_pApp->SetFontAndDirectionalityForDialogControl(m_pApp->m_pTargetFont, pTextCtrlPTTargetVersion, NULL, 
													NULL, NULL, m_pApp->m_pDlgTgtFont);
	#endif

	// Set the static strings in controls that need to distinguish external editor string "Paratext" or "Bibledit"
	wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
	wxASSERT(m_pApp->m_collaborationEditor == _T("Paratext") ||m_pApp->m_collaborationEditor == _T("Bibledit"));
	// The wxDesigner resource already has "Paratext" in its string resources,
	// we need only change those to "Bibledit" if we're using Bibledit
	if (m_pApp->m_collaborationEditor == _T("Bibledit"))
	{
		wxString tempStr;
		tempStr = pStaticTextCtrlTopInfoBox->GetValue();
		tempStr.Replace(_T("Paratext"),_T("Bibledit"));
		pStaticTextCtrlTopInfoBox->ChangeValue(tempStr);

		tempStr = pStaticInfoLine2->GetLabel();
		tempStr.Replace(_T("Paratext"),_T("Bibledit"));
		pStaticInfoLine2->SetLabel(tempStr);

		tempStr = pStaticInfoLine3->GetLabel();
		tempStr.Replace(_T("Paratext"),_T("Bibledit"));
		pStaticInfoLine3->SetLabel(tempStr);

		tempStr = pStaticPTVsTitle->GetLabel();
		tempStr.Replace(_T("Paratext"),_T("Bibledit"));
		pStaticPTVsTitle->SetLabel(tempStr);

		tempStr = pRadioRetainPTVersion->GetLabel();
		tempStr.Replace(_T("Paratext"),_T("Bibledit"));
		pRadioRetainPTVersion->SetLabel(tempStr);

		tempStr = this->GetTitle(); // get the dialog title string
		tempStr.Replace(_T("Paratext"),_T("Bibledit"));
		this->SetTitle(tempStr);

	}

	// Load the test arrays into the appropriate dialog controls
	pCheckListBoxVerseRefs->Clear();
	pCheckListBoxVerseRefs->Append(verseRefsArray);

	// Set focus on the first item in the verse reference list; the
	// CurrentListBoxHighlightedIndex will be 0
	pCheckListBoxVerseRefs->SetFocus();
	if (pCheckListBoxVerseRefs->GetCount() > 0)
	{
		CurrentListBoxHighlightedIndex = 0;
		pCheckListBoxVerseRefs->SetSelection(CurrentListBoxHighlightedIndex);
	}
	FillEditBoxesWithVerseTextForHighlightedItem();
	SyncRadioButtonsWithHighlightedItemTickState();

}

void CCollabVerseConflictDlg::SyncRadioButtonsWithHighlightedItemTickState()
{
	// The radio buttons should always stay in sync with whatever line in the
	// list is currently highlighted,
	int nSel;
	nSel = pCheckListBoxVerseRefs->GetSelection();
	if (pCheckListBoxVerseRefs->IsChecked(nSel))
	{
		pRadioUseAIVersion->SetValue(TRUE);
		pRadioRetainPTVersion->SetValue(FALSE);
	}
	else
	{
		pRadioUseAIVersion->SetValue(FALSE);
		pRadioRetainPTVersion->SetValue(TRUE);
	}
}

void CCollabVerseConflictDlg::FillEditBoxesWithVerseTextForHighlightedItem()
{
	pTextCtrlSourceText->ChangeValue(sourceTextVsArray.Item(CurrentListBoxHighlightedIndex));
	pTextCtrlAITargetVersion->ChangeValue(aiTargetTextVsArray.Item(CurrentListBoxHighlightedIndex));
	// We use the ptTargetTextVsArray for getting the string value whenever that value is the
	// same as what we'd get from ptTargetTextVsEditedArray; but if the latter is different than
	// the former, it's because of user edits and so the user will want to see his edited version
	// anytime he clicks back at this verse reference in the list - so in that case we use
	// the ptTargetTextVsEditedArray verse version in the PT or BE text ctrl box
	wxString original = ptTargetTextVsArray.Item(CurrentListBoxHighlightedIndex);
	wxString edited = ptTargetTextVsEditedArray.Item(CurrentListBoxHighlightedIndex);
	if (original == edited)
	{
		pTextCtrlPTTargetVersion->ChangeValue(ptTargetTextVsArray.Item(CurrentListBoxHighlightedIndex));
	}
	else
	{
		pTextCtrlPTTargetVersion->ChangeValue(ptTargetTextVsEditedArray.Item(CurrentListBoxHighlightedIndex));
	}
}

void CCollabVerseConflictDlg::OnCheckListBoxTickChange(wxCommandEvent& event)
{
	// This handler is entered when a check box has been ticked or unticked
	// Note: Calling GetSelection() on the event gives the index of the check
	// box that was ticked or unticked. Calling GetSelection() on the
	// pCheckListBoxVerseRefs check list box control itself give the index of
	// the list item that is highlighted/selected. 
	// Since highlighting and ticking of check boxes can happen independently
	// it would be good to make ticking or unticking of a check box force the
	// selection to change to that line in the list. That way a user can see
	// the verse contents of source and targets that he is affecting by the
	// ticking or unticking action. We don't keep the highlight and ticking
	// action in sync going the other direction, that is, when a user changes
	// the highlighted item (using up/down arrow keys or clicking on the line
	// item, but away from the check box) we don't change any ticks in any
	// check boxes.
	int nTickSel;
	nTickSel = event.GetSelection(); // GetSelection here gets the index of the check box item in which the tick was made
	// Change the highlight selection to the tick change item
	CurrentListBoxHighlightedIndex = nTickSel; // keep the CurrentListBoxHighlightedIndex up to date
	pCheckListBoxVerseRefs->SetSelection(CurrentListBoxHighlightedIndex); // sync the highlighted to a newly ticked/unticked box - no command events emitted
	FillEditBoxesWithVerseTextForHighlightedItem();
	SyncRadioButtonsWithHighlightedItemTickState(); // sync the radio buttons to agree with the highlight
}

void CCollabVerseConflictDlg::OnListBoxSelChange(wxCommandEvent& WXUNUSED(event))
{
	// This handler is entered when a list box selection has changed (user clicks
	// on a line item in the list box, but not on the check box itself).
	//wxMessageBox(_T("List box selection changed"));
	lastIndex = CurrentListBoxHighlightedIndex; // remember, user can click a second time on same list item, so check

	// Make the radio buttons agree with the current check box selection
	int nSel;
	nSel = pCheckListBoxVerseRefs->GetSelection();

	// If we have moved to a different line, then update the old location's text
	// in the struct, for the PT or BE edited version first. If we didn't move
	// to a new ch/verse ref, then there is nothing to do
	if (nSel != lastIndex)
	{
		// Get the update done for the old index's stored (possibly edited) PT or BE text;
		// this code is also called at the start of the Transfer... button handler, since
		// the user will probably go from working with whatever is the final list item he
		// wants to deal with straight to the Transfer The Listed Verses button - and so
		// without a copy of this code in that button's handler, the final update would not
		// get done, because the update here would not get called
		wxASSERT(lastIndex >= 0 && lastIndex < (int)pConflictsArray->GetCount()); // no bounds error
		if (!ptTargetTextVsEditedArray.IsEmpty())
		{
			UpdatePTorBEtext(lastIndex, &ptTargetTextVsEditedArray, pTextCtrlPTTargetVersion);
		}

		CurrentListBoxHighlightedIndex = nSel; // keep the CurrentListBoxHighlightedIndex up to date
		// Change the displayed text in the 3 edit boxes to match the
		// selection/highlight index. Use the wxTextCtrl::ChangeValue() method rather 
		// than the deprecated SetValue() method. SetValue() generates an undesirable
		// wxEVT_COMMAND_TEXT_UPDATED event whereas ChangeValue() does not trigger
		// that event.
		FillEditBoxesWithVerseTextForHighlightedItem();
		SyncRadioButtonsWithHighlightedItemTickState(); // keep the radio buttons in sync
	}
}

void CCollabVerseConflictDlg::OnRadioUseAIVersion(wxCommandEvent& WXUNUSED(event))
{
	// This handler is entered when user clicks on the "Use this Adapt It
	// version" radio button.
	// This action should add a tick on the current highlighted item 
	// in the verse references check list box.
	pCheckListBoxVerseRefs->Check(CurrentListBoxHighlightedIndex,TRUE);
	// Set the "Use this Adapt It version" radio button and unset the "Retain
	// this Paratext version" radio button
	SyncRadioButtonsWithHighlightedItemTickState();
}

void CCollabVerseConflictDlg::OnRadioRetainPTVersion(wxCommandEvent& WXUNUSED(event))
{
	// This handler is entered whent the user clicks on the "Retain this
	// Paratext version" radio button
	// This action should remove any tick on the current item in the verse
	// references check list box.
	pCheckListBoxVerseRefs->Check(CurrentListBoxHighlightedIndex,FALSE);
	// Unset the "Use this Adapt It version" radio button and set the "Retain
	// this Paratext version" radio button
	SyncRadioButtonsWithHighlightedItemTickState();
}

void CCollabVerseConflictDlg::OnSelectAllVersesButton(wxCommandEvent& WXUNUSED(event))
{
	// When Selecting All the check boxes in the list, we should keep the
	// current selection the same.
	int nItemCount;
	nItemCount = pCheckListBoxVerseRefs->GetCount();
	int i;
	for (i = 0; i < nItemCount; i++)
	{
		pCheckListBoxVerseRefs->Check(i,TRUE); // TRUE ticks item
	}
	// The tick state of the currently highlighted item may have changed so
	// update the radio buttons if needed.
	SyncRadioButtonsWithHighlightedItemTickState();
}

void CCollabVerseConflictDlg::OnUnSelectAllVersesButton(wxCommandEvent& WXUNUSED(event))
{
	// When Selecting All the check boxes in the list, we should keep the
	// current selection the same.
	int nItemCount;
	nItemCount = pCheckListBoxVerseRefs->GetCount();
	int i;
	for (i = 0; i < nItemCount; i++)
	{
		pCheckListBoxVerseRefs->Check(i,FALSE); // FALSE unticks item
	}
	SyncRadioButtonsWithHighlightedItemTickState();
}

void CCollabVerseConflictDlg::OnPTorBEtextUpdated(wxCommandEvent& event)
{
	// We get here for every edit (per keystroke!) done by the user in the PTorBE box
	// so we have to update the stored string in the edited array each time
	wxASSERT(CurrentListBoxHighlightedIndex >= 0 && CurrentListBoxHighlightedIndex < (int)pConflictsArray->GetCount()); // no bounds error
	if (!ptTargetTextVsEditedArray.IsEmpty())
	{
		UpdatePTorBEtext(CurrentListBoxHighlightedIndex, &ptTargetTextVsEditedArray, pTextCtrlPTTargetVersion);
#if defined(_DEBUG)
		wxLogDebug(_T("OnPTorBEtextUpdated() at index %d: original:  %s  <>  edited:  %s"),
			CurrentListBoxHighlightedIndex, ptTargetTextVsArray.Item(CurrentListBoxHighlightedIndex).c_str(), 
					ptTargetTextVsEditedArray.Item(CurrentListBoxHighlightedIndex).c_str());
#endif
	}
	event.Skip();
}

void CCollabVerseConflictDlg::OnOK(wxCommandEvent& event)
{
	//lastIndex = CurrentListBoxHighlightedIndex;
	//wxASSERT(lastIndex >= 0 && lastIndex < (int)pConflictsArray->GetCount()); // no bounds error
	if (!ptTargetTextVsEditedArray.IsEmpty())
	{
		// We don't need this call, the 'edited' array is kept continuously uptodate
		//UpdatePTorBEtext(lastIndex, &ptTargetTextVsEditedArray, pTextCtrlPTTargetVersion);
#if defined(_DEBUG)
		wxLogDebug(_T("OnOK() at index %d: original:  %s  <>  edited:  %s"),
			CurrentListBoxHighlightedIndex, ptTargetTextVsArray.Item(CurrentListBoxHighlightedIndex).c_str(),
			ptTargetTextVsEditedArray.Item(CurrentListBoxHighlightedIndex).c_str());
#endif
	}
	event.Skip();
}

void CCollabVerseConflictDlg::OnCancel(wxCommandEvent& event)
{
	// When the user cancels, we should restore the original PT or BE versions
	// to the RHS text box, and do the "safe" legacy thing, which is to retain
	// the PT or BE versions when conflicts arise
	lastIndex = CurrentListBoxHighlightedIndex;

	event.Skip();
}

void CCollabVerseConflictDlg::UpdatePTorBEtext(int index, wxArrayString* ptTargetTextVsEditedArrayPtr, wxTextCtrl* pTxtCtrl)
{
	if (ptTargetTextVsEditedArrayPtr->IsEmpty())
	{
		gpApp->LogUserAction(_T("UpdatePTorBEtext() error, ptTargetTextVsEditedArrayPtr passed in was Empty"));
		return;
	}
	else
	{
		// Non empty, so go ahead
		wxString currentStr = pTxtCtrl->GetValue(); // it may, or may not, have been edited by the user
		ptTargetTextVsEditedArrayPtr->RemoveAt(index);
		ptTargetTextVsEditedArrayPtr->Insert(currentStr, index);
	}
}

void CCollabVerseConflictDlg::OnRestoreBtn(wxCommandEvent& WXUNUSED(event))
{	
	// Restore the original PT or BE text value to the text box. We use SetValue()
	// because we want to generate an update event that will get the restored
	// original value back into the ptTargetTextVsEditedArray at the correct
	// location
	wxString original = ptTargetTextVsArray.Item(CurrentListBoxHighlightedIndex);
	pTextCtrlPTTargetVersion->SetValue(original);
}


/* unneeded
void CCollabVerseConflictDlg::OnLeftBtnDown(wxMouseEvent& event)
{
	if (event.GetButton() == wxID_OK)
	{
		; // nothing to do yet
	}
	event.Skip();
}
*/
// other class methods
wxString CCollabVerseConflictDlg::MakeVerseReference(ConflictRes* p)
{
	wxString aVsRef = _T(" ");
	wxASSERT(p != NULL);
	aVsRef = p->bookCodeStr + _T(" ");
	aVsRef += p->chapterRefStr + _T(":");
	aVsRef += p->verseRefStr;
	return aVsRef;
}


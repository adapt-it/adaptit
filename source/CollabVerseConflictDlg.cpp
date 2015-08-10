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

// Uncomment to turn on the July 2015 wxLogDebug calls, for code fixes and conflict
// resolution etc; same #define is in CollabUtilities.cpp near top
//#define JUL15

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
	EVT_TEXT(ID_TEXTCTRL_EDITABLE_PT_VERSION, CCollabVerseConflictDlg::OnPTorBEtextUpdated)
	EVT_BUTTON(ID_BUTTON_RESTORE, CCollabVerseConflictDlg::OnRestoreBtn)
	EVT_CHECKBOX(ID_CHECKBOX_MAKE_SOLIDUS_VISIBLE, CCollabVerseConflictDlg::OnCheckboxShowSlashes)

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

	pCheckboxShowSolidi = (wxCheckBox*)FindWindowById(ID_CHECKBOX_MAKE_SOLIDUS_VISIBLE);
	wxASSERT(pCheckboxShowSolidi != NULL);

	CurrentListBoxHighlightedIndex = 0;
	lastIndex = 0;
	bIsShowingSlashes = FALSE;

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

	// The RHS textbox is always editable, but when the app boolean m_bFwdSlashDelimiter
	// is TRUE, we are dealing with an Asian language which uses delimitation by ZWSP
	// but switches to solidus (a forward slash) when user editability is to be had. If
	// TRUE we need to support this in BOTH AI version and PTorBE version text boxes 
	// (because showing or hiding solidus are two modes, and the modes must be consistent
	// across those two wxTextCtrls because the user may copy from the read only left box
	// to paste into the right box, and so both boxes must be displaying / or both boxes
	// must be displaying ZWSP; we must not allow mixing, otherwise our functions for
	// switching the modes will leave part of the text in the other mode. We'll risk
	// not supporting these two modes for the source text box, assuming the user will not
	// want to copy from there to the RH box; and so for source text we'll show the ZWSP
	// delimited wordforms regardless of what the lower two boxes are showing.
	// ZWSP delimitation is easier for the user to read the text, but harder for support
	// of editing; solidi visible make editing easier, but reading harder. So we allow
	// the user to easily switch between these two modes, with a checkbox 
	// "Show Slashes ( / ) at the bottom right of the dialog. It is a hidden checkbox,
	// only being shown when the app (via a Preferences choice) has been put into the
	// solidus support mode by a checkbox in View tab (I think it's there), which sets
	// or clears m_bFwdSlashDelimiter. The conflict res dlg is populated from exports, and
	// they have ZWSP restored (solidi removed from visibility), and so the checkbox
	// in the conflict res dlg should default to being unticked when the dlg is built.
	// For both the Transfer... and Cancel buttons, our handlers must interrogate the
	// state of this checkbox, and if the current mode is to Show Slashes being ON, then
	// each handler must, before anything else, restore all text strings to have only
	// ZWSP word delimitations, and then the handlers can do their thing.
	// Note: the state cannot be changed within the dialog. The m_bFwdSlashDelimiter
	// flag must be set from Preferences before a document is loaded or created.
	if (gpApp->m_bFwdSlashDelimiter)
	{
		// Solidus support is turned ON, so the app must support switching the view
		// between ZWSP word delimitation, and (for checkbox ticked) Solidus delimitation
		pCheckboxShowSolidi->Show(TRUE);
	}
	else
	{
		// Solidus support is not wanted. Text with ZWSP will be displayed with that
		// delimitation, and/or latin spaces as latin spaces, where present, so in
		// these cases the checkbox would be confusing, so we hide it. This is also
		// the default.
		pCheckboxShowSolidi->Show(FALSE);
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
	nTickSel = event.GetSelection(); // GetSelection here gets the index of the 
									 // check box item in which the tick was made
	// Change the highlight selection to the tick change item
	CurrentListBoxHighlightedIndex = nTickSel; // keep the CurrentListBoxHighlightedIndex up to date
	pCheckListBoxVerseRefs->SetSelection(CurrentListBoxHighlightedIndex); // sync the
		// highlighted item to a just ticked or unticked box - no command events emitted
	FillEditBoxesWithVerseTextForHighlightedItem();
	// sync the radio buttons to agree with the highlight
	SyncRadioButtonsWithHighlightedItemTickState(); 
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
#if defined(_DEBUG) && defined(JUL15)
		wxLogDebug(_T("OnPTorBEtextUpdated() at index %d: original:  %s  <>  edited:  %s"),
			CurrentListBoxHighlightedIndex, ptTargetTextVsArray.Item(CurrentListBoxHighlightedIndex).c_str(), 
					ptTargetTextVsEditedArray.Item(CurrentListBoxHighlightedIndex).c_str());
#endif
	}
	event.Skip();
}

void CCollabVerseConflictDlg::OnOK(wxCommandEvent& event)
{
	if (gpApp->m_bFwdSlashDelimiter)
	{
		// We are in the mode in which ZWSP and solidus ( / ) are interchangeable, check
		// the flag to see if the wxTextCtrl pair for AI and PTorBE are displaying 
		// words delimited with /, if so we must change back to ZWSP delimitation before
		// going any further - the default text state for collaborating is to use ZWSP
		// for this mode
		if (bIsShowingSlashes)
		{
			ShowZWSPsNotSlashes();
		}
	}
	
	// Next, we now have wxArrayString arrays, and in the ptTargetTextVsEditedArray there
	// may be user edits. We must, for each ConflictRes struct (ptr) in conflictsArr, set
	// or clear the bUserWantsAIverse boolean, and when it's value is FALSE, we must test
	// for the edited PTorBE text being different from the original PTorBE text, and when
	// it is, we must update the ConflictRes struct's PTorBEText_edited member too
	unsigned int count = (unsigned int)pConflictsArray->GetCount();
	unsigned int index;
	ConflictRes* pCR = NULL; // pointer to ConflictRes struct
	for (index = 0; index < count; index++)
	{
		pCR = (ConflictRes*)pConflictsArray->Item((size_t)index);
		pCR->bUserWantsAIverse = pCheckListBoxVerseRefs->IsChecked(index) ? TRUE : FALSE;
		if (!pCR->bUserWantsAIverse)
		{
			// PT or BE version of the verse is wanted. Check for user edits & update
			// accordingly
			wxString oldversion = ptTargetTextVsArray.Item(index);
			wxString newversion = ptTargetTextVsEditedArray.Item(index);
			if (oldversion != newversion)
			{
				// Give PT or BE the user's edited version
				pCR->PTorBEText_edited = newversion;
			}
		}
	}
	// Moving the data in the pConflictsArray items back, where needed, into the items of
	// the parent CollabActionsArr array, for the newText building loop to use, will be
	// done in the caller when this dialog is dismissed
	event.Skip();
}

void CCollabVerseConflictDlg::ShowSlashesNotZWSP()
{
	//#if defined(FWD_SLASH_DELIM)
	wxString str;

	// Handle left wxTextCtrl
	str = pTextCtrlAITargetVersion->GetValue();
	str = ZWSPtoFwdSlash(str);
	str = DoFwdSlashConsistentChanges(insertAtPunctuation, str);
	pTextCtrlAITargetVersion->ChangeValue(str);

	// Handle left wxTextCtrl
	str = pTextCtrlPTTargetVersion->GetValue();
	str = ZWSPtoFwdSlash(str);
	str = DoFwdSlashConsistentChanges(insertAtPunctuation, str);
	pTextCtrlPTTargetVersion->ChangeValue(str);

	// In a loop, change the AI text version, the original PTorBE version,
	// and the edited PTorBE version, in the relevant wxArrayString arrays
	unsigned int count = (unsigned int)aiTargetTextVsArray.GetCount();
	unsigned int index;
	wxString aiText;
	wxString ptTextOriginal;
	wxString ptTextEdited;
	for (index = 0; index < count; index++)
	{
		// Do the AI target text version of the verse
		aiText = aiTargetTextVsArray.Item(index);
		aiText = ZWSPtoFwdSlash(aiText);
		aiText = DoFwdSlashConsistentChanges(insertAtPunctuation, aiText);
		aiTargetTextVsArray.RemoveAt(index);
		aiTargetTextVsArray.Insert(aiText, index);

		// Do the PT original target text version of the verse
		ptTextOriginal = ptTargetTextVsArray.Item(index);
		ptTextOriginal = ZWSPtoFwdSlash(ptTextOriginal);
		ptTextOriginal = DoFwdSlashConsistentChanges(insertAtPunctuation, ptTextOriginal);
		ptTargetTextVsArray.RemoveAt(index);
		ptTargetTextVsArray.Insert(ptTextOriginal, index);

		// Do the user-edited PT original target text version of the verse
		ptTextEdited = ptTargetTextVsEditedArray.Item(index);
		ptTextEdited = ZWSPtoFwdSlash(ptTextEdited);
		ptTextEdited = DoFwdSlashConsistentChanges(insertAtPunctuation, ptTextEdited);
		ptTargetTextVsEditedArray.RemoveAt(index);
		ptTargetTextVsEditedArray.Insert(ptTextEdited, index);
	}
	//#endif
}

void CCollabVerseConflictDlg::ShowZWSPsNotSlashes()
{
	//#if defined(FWD_SLASH_DELIM)
	wxString str;

	// Handle left wxTextCtrl
	str = pTextCtrlAITargetVersion->GetValue();
	str = DoFwdSlashConsistentChanges(removeAtPunctuation, str);
	str = FwdSlashtoZWSP(str);
	pTextCtrlAITargetVersion->ChangeValue(str);

	// Handle left wxTextCtrl
	str = pTextCtrlPTTargetVersion->GetValue();
	str = DoFwdSlashConsistentChanges(removeAtPunctuation, str);
	str = FwdSlashtoZWSP(str);
	pTextCtrlPTTargetVersion->ChangeValue(str);

	// In a loop, change the AI text version, the original PTorBE version,
	// and the edited PTorBE version, in the relevant wxArrayString arrays
	unsigned int count = (unsigned int)aiTargetTextVsArray.GetCount();
	unsigned int index;
	wxString aiText;
	wxString ptTextOriginal;
	wxString ptTextEdited;
	for (index = 0; index < count; index++)
	{
		// Do the AI target text version of the verse
		aiText = aiTargetTextVsArray.Item(index);
		aiText = DoFwdSlashConsistentChanges(removeAtPunctuation, aiText);
		aiText = FwdSlashtoZWSP(aiText);
		aiTargetTextVsArray.RemoveAt(index);
		aiTargetTextVsArray.Insert(aiText, index);

		// Do the PT original target text version of the verse
		ptTextOriginal = ptTargetTextVsArray.Item(index);
		ptTextOriginal = DoFwdSlashConsistentChanges(removeAtPunctuation, ptTextOriginal);
		ptTextOriginal = FwdSlashtoZWSP(ptTextOriginal);
		ptTargetTextVsArray.RemoveAt(index);
		ptTargetTextVsArray.Insert(ptTextOriginal, index);

		// Do the user-edited PT original target text version of the verse
		ptTextEdited = ptTargetTextVsEditedArray.Item(index);
		ptTextEdited = DoFwdSlashConsistentChanges(removeAtPunctuation, ptTextEdited);
		ptTextEdited = FwdSlashtoZWSP(ptTextEdited);
		ptTargetTextVsEditedArray.RemoveAt(index);
		ptTargetTextVsEditedArray.Insert(ptTextEdited, index);
	}
	//#endif
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

void CCollabVerseConflictDlg::OnCancel(wxCommandEvent& event)
{
	// When the user cancels, we should restore the original PT or BE versions
	// to the RHS text box, and do the "safe" legacy thing, which is to retain
	// the PT or BE versions when conflicts arise
	lastIndex = CurrentListBoxHighlightedIndex;

	if (gpApp->m_bFwdSlashDelimiter)
	{
		// We are in the mode in which ZWSP and solidus ( / ) are interchangeable, check
		// the flag to see if the wxTextCtrl pair for AI and PTorBE are displaying 
		// words delimited with /, if so we must change back to ZWSP delimitation before
		// going any further - the default text state for collaborating is to use ZWSP
		// for this mode
		if (bIsShowingSlashes)
		{
			ShowZWSPsNotSlashes();
		}
	}
	// The caller is responsible for melding the "keep PTorBE version" for all conflicts
	// into the various conflict-bearing structs in the CollabActionsArr
	event.Skip();
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

void CCollabVerseConflictDlg::OnCheckboxShowSlashes(wxCommandEvent& WXUNUSED(event))
{
	if (gpApp->m_bFwdSlashDelimiter)
	{
		// Set the member boolean to reflect the checkbox's new value
		// (gets the checkbox value as it is after the click has changed its state)
		bIsShowingSlashes = pCheckboxShowSolidi->IsChecked();

		if (bIsShowingSlashes)
		{
			// User has just requested that slashes be made visible (ie. all ZWSP replaced by /,
			// and / inserted at punctuation locations)
			ShowSlashesNotZWSP();
		}
		else
		{
			// User has just requested that slashes be made invisible (ie. all / replaced by ZWSP,
			// and / removed at punctuation locations)
			ShowZWSPsNotSlashes();
		}
	}
	// Else, do nothing if solidus support is not enabled in View tab of Preferences
}

wxString CCollabVerseConflictDlg::MakeVerseReference(ConflictRes* p)
{
	wxString aVsRef = _T(" ");
	wxASSERT(p != NULL);
	aVsRef = p->bookCodeStr + _T(" ");
	aVsRef += p->chapterRefStr + _T(":");
	aVsRef += p->verseRefStr;
	return aVsRef;
}


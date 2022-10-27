/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GoToDlg.cpp
/// \author			Bill Martin
/// \date_created	1 July 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CGoToDlg class. 
/// The CGoToDlg class provides a simple dialog in which the user can enter a reference
/// to jump to in the document.
/// \derivation		The CGoToDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in GoToDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "GoToDlg.h"
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

#include "Adapt_It.h"
#include "GoToDlg.h"
#include "Adapt_ItView.h"
#include "helpers.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CGoToDlg, AIModalDialog)
	EVT_INIT_DIALOG(CGoToDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CGoToDlg::OnOK)
	EVT_BUTTON(ID_BUTTON_GO_BACK_TO, CGoToDlg::OnButtonGoBackTo)
	EVT_COMBOBOX(ID_COMBO_GO_BACK_TO, CGoToDlg::OnComboBox)
	EVT_TEXT(ID_COMBO_GO_BACK_TO, CGoToDlg::OnComboTextChanged)
	EVT_SET_FOCUS(CGoToDlg::OnSetFocus)// This focus event handler doesn't appear to be triggered for some reason
END_EVENT_TABLE()


CGoToDlg::CGoToDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Go To"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	m_nChapter = 0;
	m_nVerse = 1;

	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	GoToDlgFunc(this, TRUE, TRUE);
	// The declaration is: GoToDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

    // whm 5Mar2019 Note: The GoToDlgFunc() now uses the wxStdDialogButtonSizer, so it is no
    // longer necessary to call the ReverseOkCancelButtonsForMac() function below.
	//bool bOK;
	//bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	//bOK = bOK; // avoid warning
	m_pSpinCtrlChapter = (wxSpinCtrl*)FindWindowById(IDC_EDIT_CHAPTER);
	m_pSpinCtrlVerse = (wxSpinCtrl*)FindWindowById(IDC_EDIT_VERSE);
	m_pComboBoxGoBackTo = (wxComboBox*)FindWindowById(ID_COMBO_GO_BACK_TO);
	m_pButtonGoBackTo = (wxButton*)FindWindowById(ID_BUTTON_GO_BACK_TO);
	m_pComboBoxGoBackTo = (wxComboBox*)FindWindowById(ID_COMBO_GO_BACK_TO);
}

CGoToDlg::~CGoToDlg() // destructor
{
	
}

void CGoToDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	m_pSpinCtrlChapter->SetRange(0,150);
	m_pSpinCtrlVerse->SetRange(0,176);

	m_bComboTextChanged = FALSE;

	// whm 25Oct2022 Note: BEW suggested that the new Go To dialog's chapter and verse number should be
	// initialized to the chapter and version of the current location of the phrasebox instead of 1:1
	wxString ChVsStr;
	ChVsStr = GetChVsRefFromActiveLocation(); // usally of the form ch:vs, but may be of form ch:vs1-vs2, or ch:vs1, vs2
	wxString ChStr;
	wxString VsStr;
	// parse the ch and vs out of the ch:vs string
	ParseChVsFromReference(ChVsStr, ChStr, VsStr);
	// whm Note: The VsStr may be a bridged or comma delimited verse, i.e., 12-16 or 12,16
	// so modify the code here to allow for that possibility and just take the first verse 
	// of the bridge or comma delimited group.
	ChVsStr = NormalizeChVsRefToInitialVsOfAnyBridge(ChVsStr);
	// Convert TestCh and TestVs from wxString to int.
	m_nChapter = wxAtoi(ChStr);
	m_nVerse = wxAtoi(VsStr);
	// initialize to the reference at the current phrasebox
	m_pSpinCtrlChapter->SetValue(m_nChapter);
	m_pSpinCtrlVerse->SetValue(m_nVerse);
	m_pSpinCtrlChapter->SetFocus();

	if (gpApp->m_prevVisitedChVsReferences.GetCount() == 0)
	{
		m_pButtonGoBackTo->Disable();
		m_pComboBoxGoBackTo->Disable();
	}
	else // there is at least 1 reference string in the m_prevVisitedChVsReferences wsArrayString
	{
		// Enable the Go Back To button and combo list
		m_pButtonGoBackTo->Enable(TRUE);
		m_pComboBoxGoBackTo->Enable(TRUE);
		// populate the m_pComboBoxGoBackTo with content from the App's wxArrayString m_prevVisitedChVsReferences
		m_pComboBoxGoBackTo->Append(gpApp->m_prevVisitedChVsReferences);
		// Make first reference value appear in the combobox's edit box.
		wxString initialComboBoxStringValue;
		initialComboBoxStringValue = m_pComboBoxGoBackTo->GetString(0);
		m_pComboBoxGoBackTo->ChangeValue(initialComboBoxStringValue);
		m_pComboBoxGoBackTo->SetSelection(0);
	}

}

// whm 25Oct2022 added for revised Go To dialog
void CGoToDlg::OnButtonGoBackTo(wxCommandEvent& WXUNUSED(event))
{
	// Note: This OnButtonGoBackTo() handler will get triggered only if the button is enabled. The button is
	// enabled only if there is a previous ch:vs reference available and showing in the drop down combobox.
	// 
	// Note that the code to execute the actual jump of the phrasebox is located in the Go To dialog's caller 
	// within its "if (dlg.ShowModal() == wxID_OK)" block in Adapt_ItView.cpp. Just before the jump is made
	// the code there stores the current location ch:vs reference as the first element in the App's 
	// m_prevVisitedChVsReferences wxArrayString, so that on the next invocation of the Go To dialog, that
	// last reference will be displayed as the top element in the combobox below the "Go Back To:" button.
	// 
	// The "Go Back To:" button was clicked, and the phrasebox should now jump to the ch:vs location showing
	// in the edit box of the combobox below this button. 
	// If the user hasn't explicitly selected an item in the combobox dropdown list, but simply wants to go 
	// to the last location showing in the combobox's edit box, and clicks the "Go Back To:" button, we need
	// to update the Chapter and Verse spin controls to have the location reference that is in the combobox's
	// edit box before doing the jump via the OnOK() call below. We can do this by invoking the OnComboBox()
	// handler
	wxCommandEvent noEvent;
	OnComboBox(noEvent);

	
	// Clicking this "Go Back To:" button should effectively do the same as the OK button, so we can just
	// invoke the OK button here followed by EndModal(wxID_OK).
	wxCommandEvent dummyEvent = wxID_OK;
	OnOK(dummyEvent);
	EndModal(wxID_OK);
}

// whm 25Oct2022 added for revised Go To dialog
void CGoToDlg::OnComboBox(wxCommandEvent& WXUNUSED(event))
{
	// The user selected a previous ch:vs reference item in the drop down combobox.
	// Parse and put the chapter number in the Chapter spin control and the verse
	// number in the Verse spin control. The combobox automatically puts the selection's
	// string in its own edit box.
	// The user can then click on either the OK button or the "Go Back To:" button 
	// to initiate the jump to the new location reference.
	wxString strSel = m_pComboBoxGoBackTo->GetStringSelection();
	if (!strSel.IsEmpty())
	{
		wxString ChVsStr;
		wxString ChStr;
		wxString VsStr;
		// parse the ch and vs out of the ch:vs string
		ParseChVsFromReference(strSel, ChStr, VsStr);
		// whm Note: The VsStr may be a bridged or comma delimited verse, i.e., 12-16 or 12,16
		// so modify the code here to allow for that possibility and just take the first verse 
		// of the bridge or comma delimited group.
		strSel = NormalizeChVsRefToInitialVsOfAnyBridge(strSel);
		// Convert TestCh and TestVs from wxString to int.
		m_nChapter = wxAtoi(ChStr);
		m_nVerse = wxAtoi(VsStr);
		// initialize to the reference at the current phrasebox
		m_pSpinCtrlChapter->SetValue(m_nChapter);
		m_pSpinCtrlVerse->SetValue(m_nVerse);
		m_pSpinCtrlChapter->SetFocus();
	}
}

void CGoToDlg::OnComboTextChanged(wxCommandEvent& WXUNUSED(event))
{
	m_bComboTextChanged = TRUE;
	//wxString text = m_pComboBoxGoBackTo->GetValue();
	//wxString msg = _T("Text is %s");
	//msg = msg.Format(msg, text.c_str());
	//wxLogDebug(msg);
}

// The focus event handler doesn't appear to be triggered for some reason
void CGoToDlg::OnSetFocus(wxFocusEvent& event)
{
	if (event.GetWindow() == m_pSpinCtrlChapter)// IDC_EDIT_CHAPTER)
	{
		m_pSpinCtrlChapter->SetFocus();
	}
	else if (event.GetId() == IDC_EDIT_VERSE)
	{
		m_pSpinCtrlVerse->SetFocus();
	}

}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CGoToDlg::OnOK(wxCommandEvent& event) 
{
	if (m_pComboBoxGoBackTo->HasFocus() && m_bComboTextChanged)
	{
		// The user hit Enter while focus is in the edit box of the dropdown combobox
		// In this case the user may have entered a reference that is different from
		// the the values set in the chapter:verse spin controls, and different from any previous
		// visited reference within the combobox list items. If that is the case then
		// we go ahead and parse the reference entered and set the Chapter:Verse spin controls 
		// to that typed-in reference and proceed. If the reference typed is invalid or
		// out of range, the usual prompts will be shown by the View's OnGoTo() routines.
		wxString textStr;
		wxString ChStr;
		wxString VsStr;
		textStr = m_pComboBoxGoBackTo->GetValue();
		ParseChVsFromReference(textStr, ChStr, VsStr);
		textStr = NormalizeChVsRefToInitialVsOfAnyBridge(textStr);
		// Convert TestCh and TestVs from wxString to int.
		m_nChapter = wxAtoi(ChStr);
		m_nVerse = wxAtoi(VsStr);
		// initialize to the reference at the current phrasebox
		m_pSpinCtrlChapter->SetValue(m_nChapter);
		m_pSpinCtrlVerse->SetValue(m_nVerse);

		m_chapterVerse = ChStr + _T(":") + VsStr;
		m_verse = VsStr;
	}
	else
	{
		wxString str1;
		m_nChapter = m_pSpinCtrlChapter->GetValue();
		str1 << m_nChapter;

		wxString str2;
		m_nVerse = m_pSpinCtrlVerse->GetValue();
		str2 << m_nVerse;

		m_chapterVerse = str1 + _T(":") + str2;
		m_verse = str2;
	}
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GoToDlg.cpp
/// \author			Bill Martin
/// \date_created	1 July 2006
/// \date_revised	15 January 2008
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

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CGoToDlg, AIModalDialog)
	EVT_INIT_DIALOG(CGoToDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_OK, CGoToDlg::OnOK)
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
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	
	m_pSpinCtrlChapter = (wxSpinCtrl*)FindWindowById(IDC_EDIT_CHAPTER);
	m_pSpinCtrlVerse = (wxSpinCtrl*)FindWindowById(IDC_EDIT_VERSE);
}

CGoToDlg::~CGoToDlg() // destructor
{
	
}

void CGoToDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	m_pSpinCtrlChapter->SetRange(0,150);
	m_pSpinCtrlVerse->SetRange(0,176);

	// initialize to safe values ie. 1:1
	m_nChapter = 1;
	m_nVerse = 1;
	m_pSpinCtrlChapter->SetValue(m_nChapter);
	m_pSpinCtrlVerse->SetValue(m_nVerse);
	m_pSpinCtrlChapter->SetFocus();
	m_pSpinCtrlChapter->SetSelection(-1,-1);
	m_pSpinCtrlVerse->SetSelection(-1,-1);
}

// The focus event handler doesn't appear to be triggered for some reason
void CGoToDlg::OnSetFocus(wxFocusEvent& event)
{
	if (event.GetWindow() == m_pSpinCtrlChapter)// IDC_EDIT_CHAPTER)
	{
		m_pSpinCtrlChapter->SetFocus();
		m_pSpinCtrlChapter->SetSelection(-1,-1);
	}
	else if (event.GetId() == IDC_EDIT_VERSE)
	{
		m_pSpinCtrlVerse->SetFocus();
		m_pSpinCtrlVerse->SetSelection(-1,-1);
	}

}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CGoToDlg::OnOK(wxCommandEvent& event) 
{
	wxString str1;
	m_nChapter = m_pSpinCtrlChapter->GetValue();
	str1 << m_nChapter;

	wxString str2;
	m_nVerse = m_pSpinCtrlVerse->GetValue();
	str2 << m_nVerse;

	m_chapterVerse = str1 + _T(":") + str2;
	m_verse = str2;

	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


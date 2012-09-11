/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			BookNameDlg.h
/// \author			Bruce Waters
/// \date_created	7 August 2012
/// \date_revised	
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CBookName class. 
/// The CBookName class provides a handler for the "Change Book Name..." menu item. It is
/// also called at other times, not just due to an explicit menu item click - such as when
/// creating a new document, or opening a document which has no book name defined but does
/// have a valid bookID code within it. This handler class lets the user get an appropriate
/// book name defined, given a valid bookID code -- it supports the Paratext list of 123
/// book ids and full book name strings (the latter are localizable). A book name is
/// needed for exports of xhtml or to Pathway, so this dialog provides the functionality
/// for associating a book name with every document which contains a valid bookID.
/// \derivation		The CBookName class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "BookNameDlg.h"
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
#include "Adapt_It.h" // need this for AIModalDialog definition
#include "Adapt_It_wdr.h"
#include "BookNameDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CBookName, AIModalDialog)
	EVT_INIT_DIALOG(CBookName::InitDialog)
	EVT_BUTTON(wxID_OK, CBookName::OnOK)
	EVT_BUTTON(wxID_CANCEL, CBookName::OnCancel)
	EVT_RADIOBUTTON(ID_RADIO_SUGGESTED_BOOKNAME_ACCEPTABLE, CBookName::OnRadioSuggestedName)
	EVT_RADIOBUTTON(ID_RADIO_BOOKNAME_IS_INAPPROPRIATE, CBookName::OnRadioInappropriateBookName)
	EVT_RADIOBUTTON(ID_RADIO_USE_LAST_BOOKNAME, CBookName::OnRadioUseCurrentBookName)
	EVT_RADIOBUTTON(ID_RADIO_DIFFERENT_BOOKNAME, CBookName::OnRadioUseDifferentBookName)
END_EVENT_TABLE()

CBookName::CBookName(
		wxWindow* parent,
		wxString* title,
		wxString* pstrBookCode, 
		bool      bShowCentered) : AIModalDialog(parent, -1, *title, wxDefaultPosition,
					wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	m_pBookNameDlgSizer = BookNameDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	m_bShowItCentered = bShowCentered;
	m_bookCode = *pstrBookCode;

	// set up pointers to the buttons etc
	m_pRadioUseCurrent = (wxRadioButton*)FindWindowById(ID_RADIO_USE_LAST_BOOKNAME);
	m_pRadioInappropriateName = (wxRadioButton*)FindWindowById(ID_RADIO_BOOKNAME_IS_INAPPROPRIATE);
	m_pRadioSuggestedName = (wxRadioButton*)FindWindowById(ID_RADIO_SUGGESTED_BOOKNAME_ACCEPTABLE);
	m_pRadioTypeMyOwn = (wxRadioButton*)FindWindowById(ID_RADIO_DIFFERENT_BOOKNAME);
	m_pTextCtrl_CurrentBookName = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_OLD_BOOKNAME);
	m_pTextCtrl_TypeNewBookName = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_BOOKNAME);

}

CBookName::~CBookName() // destructor
{
}

void CBookName::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	if (m_bShowItCentered)
	{
		this->Centre(wxHORIZONTAL);
	}
	// get and show the current book name; what we show depends on whether collaboration
	// is on
	if (gpApp->m_bCollaboratingWithParatext || gpApp->m_bCollaboratingWithBibledit)
	{
		if (gpApp->m_bookName_Current.IsEmpty())
		{
			// if m_bookName_Current has no name in it, show what's in m_CollabBookSelected
			m_currentBookName = gpApp->m_CollabBookSelected;
		}
		else
		{
			// if m_bookName_Current has a name in it, we'll honour it even in collab mode
			m_currentBookName = gpApp->m_bookName_Current;
		}
	}
	else
	{
		m_currentBookName = gpApp->m_bookName_Current;
	}
	wxString showThisName = m_currentBookName;
	if (m_currentBookName.IsEmpty())
	{
		// we don't want the user to see an empty box, so show  <not named>
		showThisName = _("<not named>");
	}
	m_pTextCtrl_CurrentBookName->ChangeValue(showThisName);
	m_pTextCtrl_CurrentBookName->SetEditable(FALSE); // it's now read-only


	// get a book name from the list of Paratext "full names" based on the passed in bookID
	m_suggestedBookName = gpApp->GetBookNameFromBookCode(m_bookCode);
	// get the radio button label string
	m_radioLabelStr = m_pRadioSuggestedName->GetLabel();
	// insert the suggested book name into the label
	wxString s;
	s = s.Format(m_radioLabelStr, m_suggestedBookName.c_str());
	m_radioLabelStr = s;
	// update the label
	m_pRadioSuggestedName->SetLabel(m_radioLabelStr);

	// set initial radio button values & put the current bookname in the box at the bottom
	// and the value in m_newBookName as well; radio buttons set up accordingly too
	wxCommandEvent dummyEvent;
	OnRadioUseCurrentBookName(dummyEvent);
	/*
	m_pRadioUseCurrent->SetValue(TRUE);
	m_pRadioInappropriateName->SetValue(FALSE);
	m_pRadioSuggestedName->SetValue(FALSE);
	m_pRadioTypeMyOwn->SetValue(FALSE);

	// m_newBookName starts out empty
	m_newBookName.Empty();
	m_pTextCtrl_TypeNewBookName->Clear();
	*/
}

void CBookName::OnOK(wxCommandEvent& event)
{
	// update the app's book name member value (there are four possibilities, depending on
	// what the user did in the dialog):
	// 1. he asked that the old book name be used unchanged (the default action)
	// 2. he asked that the old book name be cleared, and left cleared
	// 3. he accepted the (English) book name suggested from the Paratext list on the
	// basis of a match using the document's bookID code (such as MAT, or 1CO, etc)
	// 4. he typed some custom name of his own choosing, possibly in his own language
	m_newBookName = m_pTextCtrl_TypeNewBookName->GetValue();
	gpApp->m_bookName_Current = m_newBookName; // this could be an empty string
	event.Skip();
}

void CBookName::OnCancel(wxCommandEvent& event)
{
	event.Skip();
}

void CBookName::OnRadioSuggestedName(wxCommandEvent& WXUNUSED(event))
{
	// copy the suggested name to the text box at the bottom, and put the name in the
	// m_newBookName member variable; and set the radio button values accordingly
	m_pRadioUseCurrent->SetValue(FALSE);
	m_pRadioInappropriateName->SetValue(FALSE);
	m_pRadioSuggestedName->SetValue(TRUE);
	m_pRadioTypeMyOwn->SetValue(FALSE);

	m_pTextCtrl_TypeNewBookName->ChangeValue(m_suggestedBookName);
	m_newBookName = m_suggestedBookName;
}

void CBookName::OnRadioInappropriateBookName(wxCommandEvent& WXUNUSED(event))
{
	// clear the bottom text ctrl, empty m_newBookName, and set the radio buttons accordingly
	m_pRadioUseCurrent->SetValue(FALSE);
	m_pRadioInappropriateName->SetValue(TRUE);
	m_pRadioSuggestedName->SetValue(FALSE);
	m_pRadioTypeMyOwn->SetValue(FALSE);

	m_pTextCtrl_TypeNewBookName->Clear();
	m_newBookName.Empty();
}

void CBookName::OnRadioUseCurrentBookName(wxCommandEvent& WXUNUSED(event))
{
    // copy the old name to the text box at the bottom, and put that name also in the
    // m_newBookName member variable; and set the radio button values accordingly
	m_pRadioUseCurrent->SetValue(TRUE);
	m_pRadioInappropriateName->SetValue(FALSE);
	m_pRadioSuggestedName->SetValue(FALSE);
	m_pRadioTypeMyOwn->SetValue(FALSE);

	m_pTextCtrl_TypeNewBookName->ChangeValue(m_currentBookName);
	m_newBookName = m_currentBookName;
}

void CBookName::OnRadioUseDifferentBookName(wxCommandEvent& WXUNUSED(event))
{
    // clear the text box at the bottom, and empty m_newBookName and set 
	// the radio button values accordingly -- grab the value in the text box in the OnOK()
	// handler
	m_pRadioUseCurrent->SetValue(FALSE);
	m_pRadioInappropriateName->SetValue(FALSE);
	m_pRadioSuggestedName->SetValue(FALSE);
	m_pRadioTypeMyOwn->SetValue(TRUE);

	m_pTextCtrl_TypeNewBookName->Clear();
	m_newBookName.Empty();
	m_pTextCtrl_TypeNewBookName->SetFocus();
	m_pTextCtrl_TypeNewBookName->SetInsertionPoint(0L);
}





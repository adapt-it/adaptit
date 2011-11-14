/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			WhichFilesDlg.cpp
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CWhichFilesDlg class. 
/// The CWhichFilesDlg class presents the user with a 
/// dialog in which various files can be selected or deselected and by moving them 
/// right or left into different lists (the will be used list, and the will not be
/// used list), by pressing a large check mark button (to include), or a large X 
/// button (to exclude). The interface resources for the CWhichFilesDlg are defined 
/// in WhichFilesDlgFunc() which was developed and is maintained by wxDesigner.
/// \derivation		The CWhichFilesDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////


// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "WhichFilesDlg.h"
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
#include "WhichFilesDlg.h"
#include "helpers.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CWhichFilesDlg, AIModalDialog)
	EVT_INIT_DIALOG(CWhichFilesDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CWhichFilesDlg::OnOK)
	EVT_BUTTON(IDC_BUTTON_REJECT, CWhichFilesDlg::OnButtonReject)
	EVT_BUTTON(IDC_BUTTON_ACCEPT, CWhichFilesDlg::OnButtonAccept)
	EVT_BUTTON(ID_BUTTON_REJECT_ALL_FILES, CWhichFilesDlg::OnButtonRejectAllFiles)
	EVT_BUTTON(ID_BUTTON_ACCEPT_ALL_FILES, CWhichFilesDlg::OnButtonAcceptAllFiles)
	EVT_LISTBOX(IDC_LIST_REJECTED, CWhichFilesDlg::OnSelchangeListRejected)
	EVT_LISTBOX(IDC_LIST_ACCEPTED, CWhichFilesDlg::OnSelchangeListAccepted)
END_EVENT_TABLE()


CWhichFilesDlg::CWhichFilesDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Remove Unwanted Files From The List"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pWhichFilesDlgSizer = WhichFilesDlgFunc(this, TRUE, TRUE);
	// The declaration is: WhichFilesDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	
	// use wxValidator for simple dialog data transfer
	m_pListBoxAccepted = (wxListBox*)FindWindowById(IDC_LIST_ACCEPTED);
	m_pListBoxRejected = (wxListBox*)FindWindowById(IDC_LIST_REJECTED);

	// other attribute initializations
}

CWhichFilesDlg::~CWhichFilesDlg() // destructor
{
	
}

// event handling functions

void CWhichFilesDlg::OnSelchangeListRejected(wxCommandEvent& WXUNUSED(event)) 
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	//if (m_pListBoxRejected->GetCount() == 0)
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxRejected))
	{
		::wxBell();
		return;
	}

	int nSel;
	nSel = m_pListBoxRejected->GetSelection();
	//if (nSel == -1)
	//{
	//	::wxBell();
	//	return;
	//}
	wxString file;
	file = m_pListBoxRejected->GetString(nSel);
	wxASSERT(!file.IsEmpty());
}

void CWhichFilesDlg::OnSelchangeListAccepted(wxCommandEvent& WXUNUSED(event)) 
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	//if (m_pListBoxAccepted->GetCount() == 0)
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxAccepted))
	{
		::wxBell();
		return;
	}

	int nSel;
	nSel = m_pListBoxAccepted->GetSelection();
	//if (nSel == -1)
	//{
	//	::wxBell();
	//	return;
	//}
	wxString file;
	file = m_pListBoxAccepted->GetString(nSel);
	wxASSERT(!file.IsEmpty());
}

void CWhichFilesDlg::OnButtonRejectAllFiles(wxCommandEvent& WXUNUSED(event))
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxAccepted))
	{
		::wxBell();
		return;
	}
	unsigned int countLeft = m_pListBoxAccepted->GetCount();
	unsigned int index;
	wxString file;
	if (countLeft > 0)
	{
		for (index = 0; index < countLeft; index++)
		{
			// copy all to the right first
			m_pListBoxAccepted->SetSelection(index);
			file = m_pListBoxAccepted->GetString(index);
			wxASSERT(!file.IsEmpty());
			m_pListBoxRejected->Append(file); // not interested in returned index
		}
		// clear the left list box
		m_pListBoxAccepted->Clear();
		// set no selection on the left box
		m_pListBoxAccepted->SetSelection(wxNOT_FOUND);
		// set first item as default selection on the right list box
		if (m_pListBoxRejected->GetCount() > 0)
		{
			m_pListBoxRejected->SetSelection(0);
		}
	}
}

void CWhichFilesDlg::OnButtonAcceptAllFiles(wxCommandEvent& WXUNUSED(event))
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxRejected))
	{
		::wxBell();
		return;
	}
	unsigned int countRight = m_pListBoxRejected->GetCount();
	unsigned int index;
	wxString file;
	if (countRight > 0)
	{
		for (index = 0; index < countRight; index++)
		{
			// copy all to the right first
			m_pListBoxRejected->SetSelection(index);
			file = m_pListBoxRejected->GetString(index);
			wxASSERT(!file.IsEmpty());
			m_pListBoxAccepted->Append(file); // not interested in returned index
		}
		// clear the right list box
		m_pListBoxRejected->Clear();
		// set no selection on the right box
		m_pListBoxRejected->SetSelection(wxNOT_FOUND);
		// set first item as default selection on the left list box
		if (m_pListBoxAccepted->GetCount() > 0)
		{
			m_pListBoxAccepted->SetSelection(0);
		}
	}
}


void CWhichFilesDlg::OnButtonReject(wxCommandEvent& WXUNUSED(event)) 
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxAccepted))
	{
		::wxBell();
		return;
	}
	int nSel;
	nSel = m_pListBoxAccepted->GetSelection();
	//if (nSel == -1)
	//{
	//	::wxBell();
	//	return;
	//}
	wxString file;
	file = m_pListBoxAccepted->GetString(nSel);
	wxASSERT(!file.IsEmpty());
	m_pListBoxAccepted->Delete(nSel);
	m_pListBoxRejected->Append(file); 

	// put the selection on the next at the same place
	if (m_pListBoxAccepted->GetCount() > 0)
	{
		// there is at least one item left in Accepted list
		if (nSel -1 >= 0)
		{
			// the selected item was not the first item in list so select the previous item
			m_pListBoxAccepted->SetSelection(nSel-1,TRUE); //m_listBoxAccepted.SetCurSel(nSel);
		}
		else
		{
			// the selected item was first in list, so just select the new first item in list
			m_pListBoxAccepted->SetSelection(0,TRUE);
		}
	}

	// if there are now no more items in the "Accepted" list, set the focus to the
	// "rejected" list and set the selection to the first item in the list.
	if (m_pListBoxAccepted->GetCount() == 0 && m_pListBoxRejected->GetCount() > 0)
	{
		m_pListBoxRejected->SetFocus();
		m_pListBoxRejected->SetSelection(0,TRUE);
	}
}

void CWhichFilesDlg::OnButtonAccept(wxCommandEvent& WXUNUSED(event)) 
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxRejected))
	{
		::wxBell();
		return;
	}

	int nSel;
	nSel = m_pListBoxRejected->GetSelection();
	//if (nSel == -1)
	//{
	//	::wxBell();
	//	return;
	//}
	wxString file;
	file = m_pListBoxRejected->GetString(nSel);
	wxASSERT(!file.IsEmpty());
	m_pListBoxRejected->Delete(nSel);
	m_pListBoxAccepted->Append(file);

	// put the selection on the next at the same place
	if (m_pListBoxRejected->GetCount() > 0)
	{
		if (nSel -1 >= 0)
		{
			// the selected item was not the first item in list so select the previous item
			m_pListBoxRejected->SetSelection(nSel-1,TRUE);
		}
		else
		{
			// the selected item was first in list, so just select the new first item in list
			m_pListBoxRejected->SetSelection(0,TRUE);
		}
	}

	// if there are now no more items in the "Rejected" list, set the focus to the
	// "Accepted" list and set the selection to the first item in the list.
	if (m_pListBoxRejected->GetCount() == 0 && m_pListBoxAccepted->GetCount() > 0)
	{
		m_pListBoxAccepted->SetFocus();
		m_pListBoxAccepted->SetSelection(0,TRUE);
	}
}

void CWhichFilesDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) 
{
	//CDialog::OnInitDialog();
	
	// set up the bitmap buttons
	// wx note: button faces set in resources

	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp);

	// make the fonts show user's desired point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL,
								m_pListBoxAccepted, m_pListBoxRejected, gpApp->m_pDlgSrcFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL, 
								m_pListBoxAccepted, m_pListBoxRejected, gpApp->m_pDlgSrcFont);
	#endif

	// In wxArrayString the "head position" should be Item zero (0)
	// We'll iterate through the array with a for loop
	for (int i = 0; i < (int)pApp->m_acceptedFilesList.GetCount(); i++)
	{
		wxString str = pApp->m_acceptedFilesList[i];
		m_pListBoxAccepted->Append(str);
	}
	// make the top one be selected
	if (m_pListBoxAccepted->GetCount() > 0)
	{
		m_pListBoxAccepted->SetSelection(0,TRUE); //m_listBoxAccepted.SetCurSel(0);
	}

	// make the list boxes scrollable
	// whm note: wxDesigner has the listbox styles set for wxLB_HSCROLL which creates a horizontal scrollbar
	// if contents are too wide (Windows only)
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CWhichFilesDlg::OnOK(wxCommandEvent& event) 
{
	// update the app's m_acceptedFilesList using the contents of the dialog's list
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp);
	int count = m_pListBoxAccepted->GetCount();
	wxASSERT(count != -1);
	if (count == 0)
	{
		// IDS_NO_DOCS_ERR
		wxMessageBox(_("Sorry, the left list must not be empty. Doing that makes it impossible to restore the knowledge base."),_T(""), wxICON_INFORMATION);
		return;
	}
	// We don't need to use a validator to transfer data since below we
	// add the accepted files to our global wxArrayString m_acceptedFilesList
	pApp->m_acceptedFilesList.Clear();
	for (int i=0; i<count; i++)
	{
		wxString str;
		str = m_pListBoxAccepted->GetString(i);
		pApp->m_acceptedFilesList.Add(str);
	}

	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

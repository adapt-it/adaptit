/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ServDisc_KBserversDlg.cpp
/// \author			Bruce Waters
/// \date_created	12 January 2016
/// \rcs_id $Id$
/// \copyright		2016 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CServDisc_KBserversDlg class. 
/// The CServDisc_KBserversDlg class provides a dialog in which the user can choose 
/// one of two or more URLs, each URL being the location on the LAN for a currently
/// running KBserver instance, discovered by the service discovery module 
/// encapulated in the CAdapt_ItApp::DoServiceDiscovery() function. Each has an
/// associated hostname, which defaults to kbserver if the person who setup the 
/// multicasting KBerver did not supply a hostname at the appropriate time when
/// the setup script was running. Otherwise, the name he entered is used.
/// \derivation		The CServDisc_KBserversDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ServDisc_KBserversDlg.h"
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
//#include "MainFrm.h"
#include "helpers.h"
#include "ServDisc_KBserversDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast


// event handler table
BEGIN_EVENT_TABLE(CServDisc_KBserversDlg, AIModalDialog)
EVT_INIT_DIALOG(CServDisc_KBserversDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CServDisc_KBserversDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, CServDisc_KBserversDlg::OnCancel)
	EVT_BUTTON(ID_BUTTON_MORE_INFORMATION, CServDisc_KBserversDlg::OnButtonMoreInformation)
	EVT_LISTBOX(ID_LISTBOX_URLS, CServDisc_KBserversDlg::OnSelchangeListboxUrls)
	EVT_LISTBOX_DCLICK(ID_LISTBOX_URLS, CServDisc_KBserversDlg::OnDblclkListboxUrls)
END_EVENT_TABLE()


CServDisc_KBserversDlg::CServDisc_KBserversDlg(wxWindow* parent, wxArrayString* pUrls, wxArrayString* pHostnames) // dialog constructor
	: AIModalDialog(parent, -1, _("Which KBserver?"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	pServDisc_KBserversDlgSizer = ServDisc_KBserversDlg(this, TRUE, TRUE);
    // The declaration is: ChooseTranslationDlgFunc( wxWindow *parent, bool call_fit, bool
    // set_sizer );

	m_pListBoxUrls = (wxListBox*)FindWindowById(ID_LISTBOX_URLS);
    m_pListBoxUrls->SetFocus();
	m_urlsArr = wxArrayString(*pUrls); // copy the urls from the array in DoServiceDiscovery()
	m_hostnamesArr = wxArrayString(*pHostnames); // copy the associated hostnames
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	wxUnusedVar(bOK); // avoid warning

	m_bUserCancelled = FALSE; // Set TRUE if user clicks Cancel button
}

CServDisc_KBserversDlg::~CServDisc_KBserversDlg() // destructor
{

}

void CServDisc_KBserversDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{

	// Set the list box contents to discovered urls
	size_t count = m_urlsArr.GetCount();
	size_t index;
	wxString url;
	wxString hostname;
	for (index = 0; index < count; index++)
	{
		url = m_urlsArr.Item(index);
		hostname = m_hostnamesArr.Item(index);

		m_pListBoxUrls->Append(url);

		// TODO  --- add the hostname, once I start supporting them in the data sent to CServiceDiscovery - we'll need a wxListCtrl then

		// ********************************************************************************************************************** TODO
	}

	// Select the first url string in the listbox by default
	if (!m_pListBoxUrls->IsEmpty())
	{
        m_pListBoxUrls->SetSelection(0);
        m_urlSelected = m_pListBoxUrls->GetStringSelection();
	}
	m_pListBoxUrls->SetFocus();
}

void CServDisc_KBserversDlg::OnSelchangeListboxUrls(wxCommandEvent& WXUNUSED(event))
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxUrls))
		return;

	nSel = m_pListBoxUrls->GetSelection();
	m_urlSelected = m_pListBoxUrls->GetString(nSel);
}

void CServDisc_KBserversDlg::OnDblclkListboxUrls(wxCommandEvent& WXUNUSED(event))
{
    // whm Note: Sinced this is a "double-click" handler we want the behavior to be
    // essentially equivalent to calling both the OnSelchangeListBoxTranslations(),
    // followed by whatever OnOK() does. Testing shows that when making a double-click on a
    // list box the OnSelchangeListBoxTranslations() is called first, then this
    // OnDblclkListboxTranslations() handler is called.

	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxUrls))
	{
		wxMessageBox(_("List box error when double-clicking. Instead, try this: Click once to select a url, then click OK."),
		_T(""), wxICON_EXCLAMATION | wxOK);
		return;
	}
	nSel = m_pListBoxUrls->GetSelection();
	m_urlSelected = m_pListBoxUrls->GetString(nSel);
    EndModal(wxID_OK); //EndDialog(IDOK);
}

void CServDisc_KBserversDlg::OnButtonMoreInformation(wxCommandEvent& WXUNUSED(event))
{
	wxString title = _("More Information");
	wxString msg = _("You can connect to a KBserver using only one of these URLs. The KBserver at the chosen URL must contain the knowledge base defined by the source and target text language codes for your translation project.  If you try to connect using the wrong URL, the connection will not succeed and you will see an error mesage.  Nothing bad will happen if your attempt fails. Just try again, and choose a different URL.  Remember, each URL is the address of a different KBserver, and each KBserver will have its own password.  You will need to know the correct password associated with the URL you try to connect with.");
	wxMessageBox(msg, title, wxICON_INFORMATION | wxOK);
}

void CServDisc_KBserversDlg::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	// don't need to do anything except
	m_bUserCancelled = TRUE;
	EndModal(wxID_CANCEL);
}

void CServDisc_KBserversDlg::OnOK(wxCommandEvent& event)
{
	m_bUserCancelled = FALSE;
	// The caller will read the m_urlSelected value after the dialog is dismissed,
	// but before it's class instance is destroyed
	event.Skip(); // dismiss the dialog
}

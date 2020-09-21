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
/// one of two or more ipAddresses, each ipAdr being the location on the LAN for a currently
/// running KBserver instance, discovered by the service discovery module.
/// Each has an associated hostname, which defaults to kbserver if the person who setup the
/// multicasting KBerver did not supply a hostname at the appropriate time when
/// the setup script was running. Otherwise, the name he entered is used.
/// The dialog shows what's available, and optionally allows selecting one for
/// a connection to be attempted by a different handler.
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

#if defined(_KBSERVER)

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
	EVT_BUTTON(ID_BUTTON_REMOVE_KBSERVER_SELECTION, CServDisc_KBserversDlg::OnRemoveSelection)
	EVT_BUTTON(ID_BUTTON_REMOVE_SEL_ENTRY, CServDisc_KBserversDlg::OnRemoveSelectedEntry)
	EVT_LIST_ITEM_SELECTED(ID_LISTCTRL_URLS, CServDisc_KBserversDlg::OnIpAddrSelection)
	EVT_LIST_ITEM_DESELECTED(ID_LISTCTRL_URLS, CServDisc_KBserversDlg::OnIpAddrDeselection)
	END_EVENT_TABLE()

// Dialog constructor
CServDisc_KBserversDlg::CServDisc_KBserversDlg(wxWindow* parent, wxArrayString* pIpAddrs, wxArrayString* pHostnames)
	: AIModalDialog(parent, -1, _("Which KBserver?"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	pServDisc_KBserversDlgSizer = ServDisc_KBserversDlg(this, TRUE, TRUE);
    // The declaration is: ChooseTranslationDlgFunc( wxWindow *parent, bool call_fit, bool
    // set_sizer );

    // whm 5Mar2019 Note: The ServDisc_KBserversDlg() dialog now has the OK and Cancel
    // buttons set/controlled by a wxStdDialogButtonSizer at the bottom of the dialog,
    // so there is now no need to call the ReverseOkCancelButtonsForMac() function below.

	m_pListCtrlIpAddrs = (wxListView*)FindWindowById(ID_LISTCTRL_URLS);
	m_pBtnRemoveSelection = (wxButton*)FindWindowById(ID_BUTTON_REMOVE_KBSERVER_SELECTION);

	m_pListCtrlIpAddrs->SetFocus(); // input focus should start off in the list control
	m_ipAddrsArr = wxArrayString(*pIpAddrs); // copy the urls from the array in ConnectUsingDiscoveryResults()
	m_hostnamesArr = wxArrayString(*pHostnames); // copy the associated hostnames
	//bool bOK;
	//bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	//wxUnusedVar(bOK); // avoid warning

	m_bUserCancelled = FALSE; // Set TRUE if user clicks Cancel button
	count = pIpAddrs->GetCount();
	m_ipAddrSelected.Empty();
	m_hostnameSelected.Empty();
}

CServDisc_KBserversDlg::~CServDisc_KBserversDlg() // destructor
{

}

void CServDisc_KBserversDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	CAdapt_ItApp* pApp = &wxGetApp();
	// Set the list control contents to discovered and or typed urls,
	size_t index;
	wxString ipAddr;
	wxString hostname;
	m_compositeArr.Clear();
	wxString at3 = _T("@@@");

	m_pBottomMessageBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_MSG_BOTTOM);
	if (pApp->m_bShownFromServiceDiscoveryAttempt)
	{
		m_pBottomMessageBox->ChangeValue(pApp->m_SD_Message_For_Discovery);
	}
	else
	{
		m_pBottomMessageBox->ChangeValue(pApp->m_SD_Message_For_Connect);
	}
	// BEW 23July20 m_ipAddrsArr now only stores ip addresses, Leon's solution is direct to the
	// mariaDB service on the LAN or PAN, and we don't support connectivity remotely any more;
	// the https:// protocol prefix was commented out at line 2,882 in MainFrm.cpp
	wxLogDebug(_T("CServDisc_KBserversDlg::InitDialog() list ip addresses and their hostnames"));
	// Make and store the composite strings
	for (index = 0; index < count; index++)
	{
		ipAddr = m_ipAddrsArr.Item(index);
		hostname = m_hostnamesArr.Item(index);
#if defined(_DEBUG)
		wxLogDebug(_T("m_ipAddrsArr, at index %d,  ipAddr = %s  hostname = %s"), index, ipAddr.c_str(), hostname.c_str());
#endif
		wxString s = ipAddr + at3 + hostname;
		m_compositeArr.Add(s);
	}

	// Setup columns
	size_t lineCount = count; // count was set in ctor
	m_pListCtrlIpAddrs->ClearAll();
	wxListItem column[2]; // first is the url, second is its user-defined name
	// Column 0
	column[0].SetId(0L);
	column[0].SetText(_("ipAddress"));
	column[0].SetWidth(320);
	m_pListCtrlIpAddrs->InsertColumn(0, column[0]);
	// Column 1
	column[1].SetId(1L);
	column[1].SetText(_("Name"));
	column[1].SetWidth(280);
	m_pListCtrlIpAddrs->InsertColumn(1, column[1]);
	// can recover each columns string from the user's selected line
	for (index = 0; index < lineCount; index++)
	{
		wxListItem item;
		item.SetId(index);
		m_pListCtrlIpAddrs->InsertItem(item); // put this line item (still empty) into the list
		ipAddr = m_ipAddrsArr.Item(index);
		m_pListCtrlIpAddrs->SetItem(index, 0, ipAddr); // set first column string to the ip address
		hostname = m_hostnamesArr.Item(index);

		m_pListCtrlIpAddrs->SetItem(index, 1, hostname); // set second column string to the user's typed-in name
		m_pListCtrlIpAddrs->SetItemFont(index, *pApp->m_pDlgTgtFont); // use target text, 12 pt size, for each line
	}
}

void CServDisc_KBserversDlg::OnIpAddrSelection(wxListEvent& event)
{
	event.Skip();

	long anIndex = event.GetIndex();
	wxString at3 = _T("@@@");

	// The composite string, of ipAddress and Name, is stored in parallel with the list lines
	// so anIndex also indexes into the m_compositeArr, so I can recover the ipAddress and
	// (host)name directly from that array
	size_t i = (size_t)anIndex; // loss of precision is no issue here, the max index
								// value is unlikely to ever exceed about a dozen
	wxString aComposite = m_compositeArr.Item(i);
	nSel = i;

	int offset = aComposite.Find(at3);
	if (offset != wxNOT_FOUND)
	{
		// Get the user's choice of KBserver URL and its Name
		m_ipAddrSelected = aComposite.Left(offset);
		m_hostnameSelected = aComposite.Mid(offset + 3);
#if defined(_DEBUG)
		wxLogDebug(_T("OnIpAddrSelection() chosen ipAddr = %s   chosen hostname = %s"), m_ipAddrSelected.c_str(), m_hostnameSelected.c_str());
#endif
	}
	else
	{
		m_ipAddrSelected.Empty();
		m_hostnameSelected.Empty();
	}
}

void CServDisc_KBserversDlg::OnRemoveSelection(wxCommandEvent& WXUNUSED(event))
{
	if (nSel != (size_t)-1)
	{
		// A selection is current - so unselect it
		long selectionLineIndex = (long)nSel;
		m_pListCtrlIpAddrs->SetItemState(selectionLineIndex, 0, wxLIST_STATE_SELECTED);
		m_pListCtrlIpAddrs->Refresh();
		m_ipAddrSelected.Empty();
		m_hostnameSelected.Empty();
	}
}

void CServDisc_KBserversDlg::OnRemoveSelectedEntry(wxCommandEvent& WXUNUSED(event))
{
	if (nSel != (size_t)-1)
	{
		CAdapt_ItApp* pApp = &wxGetApp();

		// A selection is current - so remove it and the data in arrays for this entry
		long selectionLineIndex = (long)nSel;
		m_pListCtrlIpAddrs->DeleteItem(selectionLineIndex); // Beware, nSel becomes -1  after this call

		// Set the list control contents to discovered and or typed ipAdresses, 
		size_t index = (size_t)selectionLineIndex;
		wxString ipAddr = m_ipAddrsArr.Item(index);;
		wxString hostname = m_hostnamesArr.Item(index);
		wxString at3 = _T("@@@");
		wxString selectedComposite = ipAddr + at3 + hostname;
		wxLogDebug(_T("CServDisc_KBserversDlg::OnRemoveSelectedEntry() the constructed composite: %s from index: %d"),
			selectedComposite.c_str(), index);

		// Search in m_compositeArr for the composite string, and remove it
		int whereAt = m_compositeArr.Index(selectedComposite);
		if (whereAt != wxNOT_FOUND)
		{
			m_compositeArr.RemoveAt((size_t)whereAt);
		}

		// Search for selectedComposite also in CAdapt_ItApp::m_ipAddrs_Hostnames,
		// and remove it
#if defined (_DEBUG)
		int aCount = pApp->m_ipAddrs_Hostnames.GetCount();
		int anIndex;
		for (anIndex = 0; anIndex < aCount; anIndex++)
		{
			wxString anEntry = wxEmptyString;
			anEntry = pApp->m_ipAddrs_Hostnames.Item(anIndex);
			wxLogDebug(_T("CServDisc_KBserversDlg::OnRemoveSelectedEntry(), pApp->m_ipAddrs_Hostnames entry: %s at index: %d"),
					anEntry.c_str(), anIndex);
		}
#endif
		whereAt = pApp->m_ipAddrs_Hostnames.Index(selectedComposite);
		if (whereAt != wxNOT_FOUND)
		{
			pApp->m_ipAddrs_Hostnames.RemoveAt((size_t)whereAt);
		}
		// Clean up
		m_pListCtrlIpAddrs->Refresh();
		m_ipAddrSelected.Empty();
		m_hostnameSelected.Empty();
	}
}

void CServDisc_KBserversDlg::OnIpAddrDeselection(wxListEvent& event)
{
	event.Skip();

	long anIndex = event.GetIndex();
	wxUnusedVar(anIndex);
	m_ipAddrSelected.Empty();
	m_hostnameSelected.Empty();
	nSel = (size_t)-1;
}

void CServDisc_KBserversDlg::OnButtonMoreInformation(wxCommandEvent& WXUNUSED(event))
{
	wxString title = _("More Information");
	wxString msg = _("Click only one KBserver ipAddress. The KBserver at that ipAddress must contain the knowledge base defined by the 'source language' and 'target language' names for your translation project (see 'Preferences...' > 'Backups and Misc' tab).  A 'Knowledge Base Sharing Manager' tool, available from the hidden password-protected Administrator menu, can get this job done for you by someone who knows what to do, or by you - following documented instructions. If you try to connect using the wrong ipAddress for your project, the connection will fail and show an error message; but nothing bad will happen. Just try again, and choose a different ipAddress.  Remember, each ipAddress refers to a different KBserver, and each KBserver will have its own user access password.  You will need to know the correct password for your ipAdddress And username to get a successful connection.");
	wxMessageBox(msg, title, wxICON_INFORMATION | wxOK);
}

void CServDisc_KBserversDlg::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = &wxGetApp();
	// Turn the following flag off - otherwise the Discover KBservers menu command remains disabled
	pApp->m_bServDiscSingleRunIsCurrent = FALSE;
	// don't need to do anything except
	m_bUserCancelled = TRUE;
	EndModal(wxID_CANCEL);
}

void CServDisc_KBserversDlg::OnOK(wxCommandEvent& event)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	m_bUserCancelled = FALSE;

	// BEW added 10Jul17, because successful discovery when the user has not yet
	// made the project a KB Sharing one gives no feedback to the GUI that anything
	// useful has happened, or that there is one or more running KBservers to which
	// a connection can be made. Connection to a glossing kbserver is rare or very
	// unlikely, so we'll just ignore that possibility - it would complicate the 
	// tests done here
	if (!(pApp->m_bIsKBServerProject || pApp->m_bIsKBServerProject_FromConfigFile)
		&& count > 0)
	{
		// Either this adapting project has not yet been set up to be a KB Sharing
		// one, or no earlier setup of this project as a KB sharing one was 
		// recovered from the project config file. Without such a setup, while 
		// service discovery can succeed, a connection to any running KBserver is
		// impossible until the project becomes a KB Sharing one. Tell the user.
		wxString warningStr = _("One or more KBservers may be available. But your project is not yet set up for Knowledge Base Sharing.\nUntil you do this, a connection to any available KBserver is impossible.\nUse the command Setup Or Remove Knowledge Base Sharing, on the Advanced menu.");
		wxString titleStr = _("Cannot connect yet");
		wxMessageBox(warningStr, titleStr, wxICON_INFORMATION | wxOK);
	}


	// Turn the following flag off - otherwise the Discover KBservers menu command remains disabled
	pApp->m_bServDiscSingleRunIsCurrent = FALSE;
	// The caller will read the m_urlSelected value after the dialog is dismissed,
	// but before it's class instance is destroyed
	event.Skip(); // dismiss the dialog
}

#endif // _KBSERVER

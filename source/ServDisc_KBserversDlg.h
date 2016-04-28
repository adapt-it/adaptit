/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ServDisc_KBserversDlg.h
/// \author			Bruce Waters
/// \date_created	12 January 2016
/// \rcs_id $Id$
/// \copyright		2016 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the ServDisc_KBserversDlg class. 
/// The ServDisc_KBserversDlg class provides a dialog in which the user can choose 
/// one of two or more URLs, each URL being the location on the LAN for a currently
/// running KBserver instance, discovered by the service discovery module 
/// encapulated in the CAdapt_ItApp::ConnectUsingDiscoveryResults() function.
/// \derivation		The ServDisc_KBserversDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ServDisc_KBserversDlg_h
#define ServDisc_KBserversDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ServDisc_KBserversDlg.h"
#endif

class CServDisc_KBserversDlg : public AIModalDialog
{
public:
	CServDisc_KBserversDlg(wxWindow* parent, wxArrayString* pUrls, wxArrayString* pHostnames); // constructor
	virtual ~CServDisc_KBserversDlg(void); // destructor

	wxSizer*	pServDisc_KBserversDlgSizer;
	wxListBox*	m_pListBoxUrls;
	wxString	m_urlSelected;
	wxString	m_hostnameSelected;
	wxArrayString m_urlsArr;
	wxArrayString m_hostnamesArr;
	bool m_bUserCancelled;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& WXUNUSED(event));
	void OnButtonMoreInformation(wxCommandEvent& WXUNUSED(event));
	void OnSelchangeListboxUrls(wxCommandEvent& WXUNUSED(event));
	void OnDblclkListboxUrls(wxCommandEvent& WXUNUSED(event));

private:
	wxString strComposite; // url, spaces, hostname <--- temporary
	int nSel; // index of selected URL
	DECLARE_EVENT_TABLE()
};
#endif

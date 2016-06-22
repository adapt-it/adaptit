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
/// one of one or more URLs, each URL being the location on the LAN for a currently
/// running KBserver instance, discovered by the service discovery module 
/// encapulated in the CAdapt_ItApp::ConnectUsingDiscoveryResults() function.
/// The dialog shows what's available, and optionally allows selecting one for
/// a connection to be attempted by a different handler.
/// \derivation		The ServDisc_KBserversDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ServDisc_KBserversDlg_h
#define ServDisc_KBserversDlg_h


// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ServDisc_KBserversDlg.h"
#endif

#if defined(_KBSERVER)

class CServDisc_KBserversDlg : public AIModalDialog
{
public:	
	CServDisc_KBserversDlg(wxWindow* parent, wxArrayString* pUrls, wxArrayString* pHostnames); // constructor
	virtual ~CServDisc_KBserversDlg(void); // destructor

	wxSizer*	pServDisc_KBserversDlgSizer;
	wxString	m_urlSelected;
	wxString	m_hostnameSelected;
	wxArrayString m_urlsArr;
	wxArrayString m_hostnamesArr;
	bool		m_bUserCancelled;
	wxArrayString m_compositeArr; // put both strings into one, separated by @@@, recover
						// the url & name substrings, using the selection's index
protected:
	void		InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void		OnOK(wxCommandEvent& event);
	void		OnCancel(wxCommandEvent& WXUNUSED(event));
	void		OnButtonMoreInformation(wxCommandEvent& WXUNUSED(event));
	void        CServDisc_KBserversDlg::OnRemoveSelection(wxCommandEvent& WXUNUSED(event));
	void		OnURLSelection(wxListEvent& event);
	void		OnURLDeselection(wxListEvent& event);

private:
	wxButton*   m_pBtnRemoveSelection;
	wxTextCtrl* m_pBottomMessageBox;
	wxListView* m_pListCtrlUrls;
	wxString    strComposite; // url, spaces, hostname <--- temporary
	size_t      nSel; // index of selected URL line
	size_t		count; // count of how many items in pUrls wxArrayString* passed in in ctor
	DECLARE_EVENT_TABLE()
};
#endif // _KBSERVER
#endif

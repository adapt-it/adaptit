/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharingSetup.h
/// \author			Bruce Waters
/// \date_created	15 January 2013
/// \rcs_id $Id: KBSharingSetup.h 3028 2013-01-15 11:38:00Z jmarsden6@gmail.com $
/// \copyright		2013 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the KBSharing class.
/// The KBSharing class provides a dialog for the turning on or off KB Sharing, and for
/// controlling the non-automatic functionalities within the KB sharing feature.
/// \derivation		The KBSharing class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef KBSharingSetupDlg_h
#define KBSharingSetupDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KBSharingSetupDlg.h"
#endif

#if defined(_KBSERVER)

class KBSharingSetupDlg : public AIModalDialog
{
public:
	KBSharingSetupDlg(wxWindow* parent); // constructor
	virtual ~KBSharingSetupDlg(void); // destructor

	// other methods

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);

	//void OnBtnGetAll(wxCommandEvent& WXUNUSED(event));

	// Note: I've stored the to-be-typed-just-once kb server password in the CMainFrame
	// instance, and the dialog for getting the user to type it in is there too, and a
	// private member wxString, m_kbserverPassword which keeps the value for the session's
	// duration. When either the adapting or glossing KbServer instance needs the password
	// it can get it from there with the accessor: GetKBSvrPassword()

	DECLARE_EVENT_TABLE()
};

#endif

#endif /* KBSharingSetupDlg_h */

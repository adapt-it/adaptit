/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharingMgrTabbedDlg.h
/// \author			Bruce Waters
/// \date_created	02 July 2013
/// \rcs_id $Id: KBSharingMgrTabbedDlg.h 2883 2012-11-12 03:58:57Z adaptit.bruce@gmail.com $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the KBSharingMgrTabbedDlg class.
/// The KBSharingMgrTabbedDlg class provides a dialog with tabbed pages in which an
/// appropriately authenticated user/manager of a remote KBserver installation may add,
/// edit or remove users stored in the user table of the mysql server, and/or add or
/// remove knowledge base definitions stored in the kb table of the mysql server.
/// \derivation		The KBSharingMgrTabbedDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef KBSharingMgrTabbedDlg_h
#define KBSharingMgrTabbedDlg_h
#endif

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KBSharingMgrTabbedDlg.h"
#endif
//#if defined(_KBSERVER)
// needed for the KbServerUser and KbServerKb structures
#include "KbServer.h"

// forward declarations
class UsersList;

class KBSharingMgrTabbedDlg : public AIModalDialog
{
public:
	KBSharingMgrTabbedDlg(wxWindow* parent); // constructor
	virtual ~KBSharingMgrTabbedDlg(void); // destructor

	KbServer* GetKbServer(); // gets whatever m_pKbServer is pointing at
	KbServer* m_pKbServer;   // we'll assign the 'foreign' one to this pointer
	bool m_bAllow;
protected:
	bool		   m_bLegacyEntry; // Set this TRUE for a re-entry, or when within
						// the current Mgr session and normal entry to the legagu TRUE
						// block for the test at line 546 is wanted
	wxNotebook*    m_pKBSharingMgrTabbedDlg;
	wxListBox*     m_pUsersListBox;
	//wxTextCtrl*    m_pTheConnectedIpAddr;
	wxTextCtrl*	   m_pConnectedTo;
	wxTextCtrl*    m_pTheUsername;
	wxTextCtrl*    m_pEditInformalUsername;
	wxTextCtrl*    m_pEditPersonalPassword;
	wxTextCtrl*    m_pEditPasswordTwo;
	wxCheckBox*    m_pCheckUserAdmin;
	wxTextCtrl*	   m_pEditShowPasswordBox;

	wxButton*      m_pBtnUsersClearControls;
	wxButton*      m_pBtnUsersAddUser;

	// local copies of globals on the App, for the person using the Manager dialog
	bool           m_bKbAdmin;   // for m_kbserver_kbadmin - BEW 27Aug20, retain, 
								 // and set TRUE always so anyone entering manager
								 // can make a new KB
	bool		   m_bUserAdmin; // for m_kbserver_useradmin

	int m_nCurPage;
#ifdef __WXGTK__
	bool  m_bUsersListBoxBeingCleared; // BEW 11Jan21 is this still needed for Linux build?
	//bool  m_bSourceKbsListBoxBeingCleared;
	//bool  m_bTargetKbsListBoxBeingCleared;
	//bool  m_bGlossKbsListBoxBeingCleared;
#endif

	void  InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void  OnOK(wxCommandEvent& event);
	void  OnCancel(wxCommandEvent& event);
public:
	void  LoadDataForPage(int pageNumSelected);
	void  FillUserList(CAdapt_ItApp* pApp);

	// BEW Created 21Dec20 - extract a field (wxString value) from the matrix
	//wxString GetFieldAtIndex(wxArrayString& arr, int index);

protected:
	void  OnTabPageChanged(wxNotebookEvent& event);

	// Functions needed by the Users page
	bool  CheckThatPasswordsMatch(wxString password1, wxString password2); // BEW 11Jan21 this needed

	// event handlers - Users page
	void  OnButtonUserPageClearControls(wxCommandEvent& WXUNUSED(event)); // BEW 29Aug20 updated

	// BEW 29Aug20 updated -- TODO, legacy code commented out  -- see 1372 .cpp
	void  OnButtonUserPageAddUser(wxCommandEvent& WXUNUSED(event));
	void  OnButtonShowPassword(wxCommandEvent& WXUNUSED(event)); // BEW added 20Nov20
	void  ClearCurPwdBox();
	void  OnButtonUserPageChangePermission(wxCommandEvent& WXUNUSED(event)); // BEW 31Aug20
	void  OnButtonUserPageChangeFullname(wxCommandEvent& WXUNUSED(event)); // BEW 9Dec20
	void  OnButtonUserPageChangePassword(wxCommandEvent& WXUNUSED(event)); // BEW 11Jan21

	void  OnSelchangeUsersList(wxCommandEvent& WXUNUSED(event)); // BEW 29Aug20  updated - no kbserver accesses
	void  OnCheckboxUseradmin(wxCommandEvent& WXUNUSED(event)); // BEW 29Aug20  updated, with wxMessage() to
																// click the 'Change Permission' button
private:
	// All the lists, users, kbs and custom language definitions, are SORTED.
	CAdapt_ItApp*  m_pApp;

	DECLARE_EVENT_TABLE()
};

//#endif /* KBSharingMgrTabbedDlg_h */

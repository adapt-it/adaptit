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
/// appropriately authenticated user/manager of a remote kbserver installation may add,
/// edit or remove users stored in the user table of the mysql server, and/or add or
/// remove knowledge base definitions stored in the kb table of the mysql server.
/// \derivation		The KBSharingMgrTabbedDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef KBSharingMgrTabbedDlg_h
#define KBSharingMgrTabbedDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KBSharingMgrTabbedDlg.h"
#endif

// needed for the KbServerUser and KbServerKb structures
#include "KbServer.h"

// forward declarations
class UsersList;

class KBSharingMgrTabbedDlg : public AIModalDialog
{
public:
	KBSharingMgrTabbedDlg(wxWindow* parent); // constructor
	virtual ~KBSharingMgrTabbedDlg(void); // destructor

	// Setter for the stateless instance of KbServer created by KBSharingSetupDlg's creator
	// (that KbServer instance will internally have it's m_bStateless member set to TRUE)
	void      SetStatelessKbServerPtr(KbServer* pKbServer);
	KbServer* GetKbServer();

protected:
	wxNotebook*    m_pKBSharingMgrTabbedDlg;
	wxListBox*     m_pUsersListBox;

	wxTextCtrl*    m_pTheUsername; 
	wxTextCtrl*    m_pEditInformalUsername;
	wxTextCtrl*    m_pEditPersonalPassword;
	wxTextCtrl*    m_pEditPasswordTwo;
	wxCheckBox*    m_pCheckUserAdmin;
	wxCheckBox*    m_pCheckKbAdmin;

	wxButton*      m_pBtnUsersClearControls;
	wxButton*      m_pBtnUsersAddUser;
	wxButton*      m_pBtnUsersEditUser;
	wxButton*      m_pBtnUsersRemoveUser;


	// For Create KB Definitions page
	// 
	wxRadioButton* m_pRadioKBType1;
	wxRadioButton* m_pRadioKBType2;
	wxListBox*     m_pSourceKbsListBox;
	wxListBox*     m_pNonSourceKbsListBox;
	wxStaticText*  m_pNonSrcLabel;

	//wxTextCtrl*    m_pSrcText;
	wxTextCtrl*    m_pEditSourceCode;
	wxTextCtrl*    m_pEditNonSourceCode;
	wxButton*	   m_pBtnUsingRFC5646Codes;
	wxButton*      m_pBtnAddKbDefinition;
	wxButton*      m_pBtnClearBothLangCodeBoxes;
	wxButton*      m_pBtnLookupLanguageCodes;
	wxButton*      m_pBtnRemoveSelectedKBDefinition;



	// local copies of globals on the App, for the person using the Manager dialog
	bool           m_bKbAdmin;   // for m_kbserver_kbadmin
	bool		   m_bUserAdmin; // for m_kbserver_useradmin

	wxString rfc5654guidelines; // read in from "RFC56554_guidelines.txt in adaptit\docs\ folder,
								// & is a versioned file
	int m_nCurPage;
#ifdef __WXGTK__
	bool			m_bUsersListBoxBeingCleared;
	bool			m_bSourceKbsListBoxBeingCleared;
	bool			m_bTargetKbsListBoxBeingCleared;
	bool			m_bGlossKbsListBoxBeingCleared;
#endif

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void LoadDataForPage(int pageNumSel);
	void OnTabPageChanging(wxNotebookEvent& event);

protected:
	// Functions needed by the Users page
	KbServerUser* GetUserStructFromList(UsersList* pUsersList, size_t index);
	void          LoadUsersListBox(wxListBox* pListBox, size_t count, UsersList* pUsersList);
	void		  CopyUsersList(UsersList* pSrcList, UsersList* pDestList);
	KbServerUser* CloneACopyOfKbServerUserStruct(KbServerUser* pExistingStruct);
	void		  DeleteClonedKbServerUserStruct();
	bool		  CheckThatPasswordsMatch(wxString password1, wxString password2);
	bool		  AreBothPasswordsEmpty(wxString password1, wxString password2);
	wxString	  GetEarliestUseradmin(UsersList* pUsersList);
	KbServerUser* GetThisUsersStructPtr(wxString& username, UsersList* pUsersList);

	// event handlers - Users page
	void		  OnButtonUserPageClearControls(wxCommandEvent& WXUNUSED(event));
	void		  OnButtonUserPageAddUser(wxCommandEvent& WXUNUSED(event));
	void		  OnButtonUserPageRemoveUser(wxCommandEvent& WXUNUSED(event));
	void		  OnButtonUserPageEditUser(wxCommandEvent& WXUNUSED(event));
	void		  OnSelchangeUsersList(wxCommandEvent& WXUNUSED(event));
	void		  OnCheckboxUseradmin(wxCommandEvent& WXUNUSED(event));
	void		  OnCheckboxKbadmin(wxCommandEvent& WXUNUSED(event));

	// event handlers - Create KB Definitions page
	void		  OnRadioButton1CreateKbsPageType1(wxCommandEvent& WXUNUSED(event));
	void		  OnRadioButton2CreateKbsPageType2(wxCommandEvent& WXUNUSED(event));
	void		  OnBtnCreatePageLookupCodes(wxCommandEvent& WXUNUSED(event));
	void		  OnBtnCreatePageRFC5646Codes(wxCommandEvent& WXUNUSED(event));

private:
	// All the lists, users or kbs, are SORTED.
	CAdapt_ItApp*     m_pApp;
	int				  m_nSel; // index value (0 based) for selection in the users listbox,
							  // and has value wxNOT_FOUND when nothing is selected
	wxString		  m_earliestUseradmin; // this person cannot be deleted or demoted
	UsersList*        m_pUsersList; // initialize in InitDialog() as the KbServer instance has the list
	size_t            m_nUsersListCount; // stores how many entries are in the m_pUsersList
	UsersList*        m_pOriginalUsersList; // store copies of KbServerUser structs at
									        // entry, for comparison with final list
											// after the edits, removals and additions
											// are done
	KbServer*         m_pKbServer; // we'll assign the stateless one to this pointer
	KbServerUser*     m_pUserStruct; // scratch variable to get at returned values
								     // for a user entry's fields
	KbServerUser*     m_pOriginalUserStruct; // scratch variable to get at returned values
								     // for a user entry's fields, this one stores the
								     // struct immediately after the user's click on the
									 // user item in the listbox, freeing up the
									 // m_pUserStruct to be given values as edited by
									 // the user
	// Next members are additional ones needed for the Create KB definitions page (and
	// some will be also used in the 3rd page for editing KB definitions)
	bool m_bKBisType1; // TRUE for adaptations KB definition, FALSE for a glosses KB definition
	wxString m_tgtLanguageCodeLabel; // InitDialog() sets it to "Target language code"
	wxString m_glossesLanguageCodeLabel; // InitDialog() sets it to "Glosses language code"
	wxString m_sourceLangCode;
	wxString m_targetLangCode;
	wxString m_glossLangCode;

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

#endif /* KBSharingMgrTabbedDlg_h */

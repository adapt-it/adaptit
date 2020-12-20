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

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KBSharingMgrTabbedDlg.h"
#endif
#if defined(_KBSERVER)
// needed for the KbServerUser and KbServerKb structures
#include "KbServer.h"

// forward declarations
class UsersList;
class CodesList;
class LanguagesList;

class KBSharingMgrTabbedDlg : public AIModalDialog
{
public:
	KBSharingMgrTabbedDlg(wxWindow* parent); // constructor
	virtual ~KBSharingMgrTabbedDlg(void); // destructor

	KbServer* GetKbServer(); // gets whatever m_pKbServer is pointing at
	KbServer* m_pKbServer;   // we'll assign the 'foreign' one to this pointer
	bool m_bAllow;


protected:
	int	 m_nSel; // index value (0 based) for selection in the the listbox of one
				 // of the pages, and has value wxNOT_FOUND when nothing is selected

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
	//wxCheckBox*    m_pCheckKbAdmin; // BEW 27Aug20 removed

	wxButton*      m_pBtnUsersClearControls;
	wxButton*      m_pBtnUsersAddUser;
	// Dont need next two pointers, the event table accesss directly
	//wxButton*      m_pBtnUsersChangePermission; // BEW 27Aug20 new button (toggles useradmin)
	//wxButton*      m_pBtnUsersChangeFullname; // BEW 9Dec20 new button (replaces old fullname)
	wxButton*      m_pBtnUsersRemoveUser;


	// For Create KB Definitions page

	wxRadioButton* m_pRadioKBType1;
	wxRadioButton* m_pRadioKBType2;
	wxListBox*     m_pKbsListBox;
	wxStaticText*  m_pNonSrcLabel;
	wxStaticText*  m_pAboveListBoxLabel;

	wxTextCtrl*    m_pEditSourceCode; // BEW 27Aug20 'Code' is now to be the 'Language Name'
	wxTextCtrl*    m_pEditNonSourceCode;// BEW 27Aug20 'Code' is now to be the 'Language Name'

	wxTextCtrl*    m_pKbDefinitionCreator; // this one is read-only
	//wxButton*	   m_pBtnUsingRFC5646Codes; // BEW 27Aug20 reemoved
	wxButton*      m_pBtnAddKbDefinition;
	wxButton*      m_pBtnClearBothLangCodeBoxes;// BEW 27Aug20 'Code' is now to be the 'Language Name'
	//wxButton*      m_pBtnLookupLanguageCodes;// BEW 27Aug20 removed
	wxButton*      m_pBtnRemoveSelectedKBDefinition;
	wxButton*	   m_pBtnClearListSelection;
	// local copies of globals on the App, for the person using the Manager dialog
	bool           m_bKbAdmin;   // for m_kbserver_kbadmin - BEW 27Aug20, retain, 
								 // and set TRUE always so anyone entering manager
								 // can make a new KB
	bool		   m_bUserAdmin; // for m_kbserver_useradmin

	int m_nCurPage;
#ifdef __WXGTK__
	bool  m_bUsersListBoxBeingCleared;
	bool  m_bSourceKbsListBoxBeingCleared;
	bool  m_bTargetKbsListBoxBeingCleared;
	bool  m_bGlossKbsListBoxBeingCleared;
#endif

	void  InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void  OnOK(wxCommandEvent& event);
	void  OnCancel(wxCommandEvent& event);
public:
	void  LoadDataForPage(int pageNumSelected);
	void  FillUserList(CAdapt_ItApp* pApp);

protected:
	void  OnTabPageChanged(wxNotebookEvent& event);

	// Functions needed by the Users page
	KbServerUserForeign* GetUserStructFromList(UsersListForeign* pUsersListForeign, size_t index); // BEW 28Aug20 updated
	void  LoadUsersListBox(wxListBox* pListBox, size_t count, UsersListForeign* pUsersListForeign);  // BEW 28Aug20 updated
	void  CopyUsersList(UsersListForeign* pSrcList, UsersListForeign* pDestList); // BEW 28Aug20 updated
	KbServerUserForeign* CloneACopyOfKbServerUserStruct(KbServerUserForeign* pExistingStruct); // BEW 28Aug20 updated
	void  DeleteClonedKbServerUserStruct(); // BEW 29Aug20 no change needed
	bool  CheckThatPasswordsMatch(wxString password1, wxString password2); // BEW 29Aug20 no change needed
	bool  AreBothPasswordsEmpty(wxString password1, wxString password2); // BEW 29Aug20 no change needed
	wxString GetEarliestUseradmin(UsersListForeign* pUsersListForeign); // BEW 29Aug20 updated, returns two names, comma-sep
	KbServerUserForeign* GetThisUsersStructPtr(wxString& username2, UsersListForeign* pUsersListForeign); // BEW 28Aug updated
	// BEW 16Dec20 next one, to get the m_pUsersListForeign and m_pOriginalUsersList arrays of
	// UserListForeign pointers updated to match changes made by the button handlers after a
	// new state for the user2 has been produced by the do_list_users.exe call after configuring
	// for the commandLine for list_users case ( = 3), to achieve syncing without resort to
	// another call of LoadDataForPage(0) -- internally it reads the returned results file from
	// the kbserver's user table, and borrows existing calls to get the syncing done
	void  UpdateUserPage(CAdapt_ItApp* appPtr, wxString execPath, wxString resultFile, wxArrayString* pArrLines);

	// event handlers - Users page
	void  OnButtonUserPageClearControls(wxCommandEvent& WXUNUSED(event)); // BEW 29Aug20 updated

	// BEW 29Aug20 updated -- TODO, legacy code commented out  -- see 1372 .cpp
	void  OnButtonUserPageAddUser(wxCommandEvent& WXUNUSED(event));
	void  OnButtonShowPassword(wxCommandEvent& WXUNUSED(event)); // BEW added 20Nov20
	void  ClearCurPwdBox();
	void  OnButtonUserPageChangePermission(wxCommandEvent& WXUNUSED(event)); // BEW 31Aug20
	void  OnButtonUserPageChangeFullname(wxCommandEvent& WXUNUSED(event)); // BEW 9Dec20

	void  OnSelchangeUsersList(wxCommandEvent& WXUNUSED(event)); // BEW 29Aug20  updated - no kbserver accesses
	void  OnCheckboxUseradmin(wxCommandEvent& WXUNUSED(event)); // BEW 29Aug20  updated, with wxMessage() to
																		// click the 'Change Permission' button
	// Functions needed by the Kb Definitions page  --- TODO deprecate this soon
	KbServerKb*	  CloneACopyOfKbServerKbStruct(KbServerKb* pExistingStruct);  // BEW 28Aug20 updated
	void		  CopyKbsList(KbsList* pSrcList, KbsList* pDestList); // BEW 31Aug20 updated
	//void		  DeleteClonedKbServerKbStruct(); // BEW 31Aug20, no change needed
	bool		  IsThisKBDefinitionInSessionList(KbServerKb* pKbDefToTest, KbsList* pKbsList); // BEW 31Aug20 updated
	void		  LoadLanguageNamePairsInListBox_KbsPage(bool bKBTypeIsSrcTgt, KbsList* pSrcTgtKbsList,
							 KbsList* pSrcGlsKbsList, wxListBox* pListBox); // BEW 31Aug20 updated
	bool		  MatchExistingKBDefinition(wxListBox* pKbsList,
							wxString& srcLangNameStr, wxString& nonsrcLangNameStr);  // BEW 31Aug20 minor updated
	void		  SeparateKbServerKbStructsByType(KbsList* pAllKbStructsList,
							KbsList* pKbStructs_TgtList, KbsList* pKbStructs_GlsList); // BEW 31Aug20 no changes needed
	bool		  IsAKbDefinitionAltered(wxArrayString* pBeforeArr, wxArrayString* pAfterArr); // BEW 31Aug20 no changes

	// event handlers - Create KB Definitions page (for name/name kb table entries)
	void		  OnRadioButton1KbsPageType1(wxCommandEvent& WXUNUSED(event)); // BEW 31Aug20 updated (minor tweaks)
	void		  OnRadioButton2KbsPageType2(wxCommandEvent& WXUNUSED(event)); // BEW 31Aug20 updated (minor tweaks)
	void		  OnButtonKbsPageClearListSelection(wxCommandEvent& WXUNUSED(event));
	void		  OnButtonKbsPageClearBoxes(wxCommandEvent& WXUNUSED(event));
	void		  OnButtonKbsPageAddKBDefinition(wxCommandEvent& WXUNUSED(event));
	void		  OnSelchangeKBsList(wxCommandEvent& WXUNUSED(event));
public:
	UsersListForeign* m_pUsersListForeign; // initialize in InitDialog() as the KbServer 
										   // instance has the list
	UsersListForeign* m_pOriginalUsersList; // store copies of KbServerUser structs at
											// entry, for comparison with final list
											// after the edits, removals and additions
											// are done
private:
	// All the lists, users, kbs and custom language definitions, are SORTED.
	CAdapt_ItApp*  m_pApp;
	wxString  m_earliestUseradmin; // these two username records cannot be deleted or 
										   // their privilege level changed (they are comma separated)
	KbsList*  m_pKbsList; // initialize in InitDialog() as the KbServer instance has the list
	KbsList*  m_pOriginalKbsList; // store copies of KbServerKb structs at
										  // entry, for comparison with final list after
										  // edits or additions are done
	KbsList*  m_pKbsAddedInSession; // store KbKbserverKb struct ptrs for those added
	// NOTE: the next two are required so we can separate out the Type1 KBs (adaptation
	// ones) from the Type2 KBs (glossing ones) into separate lists - and when we populate
	// these lists, we'll do so with deep copies of the structs, so that we can call
	// ClearKbsList() on these as we do on the m_pOriginalKbsList and m_pKbsList
	KbsList*		  m_pKbsList_Tgt;
	KbsList*		  m_pKbsList_Gls;

	KbServerUserForeign*  m_pUserStructForeign; // scratch variable to get at returned values
								     // for a user entry's fields
	KbServerUserForeign*  m_pOriginalUserStruct; // scratch variable to get at returned values
								     // for a user entry's fields, this one stores the
								     // struct immediately after the user's click on the
									 // user item in the listbox, freeing up the
									 // m_pUserStructForeign to be given values as edited by
									 // the user
	KbServerKb*  m_pKbStruct; // scratch variable to hold returned values for
								   // a kb entry's pair of language names, etc
	KbServerKb*  m_pOriginalKbStruct; // performs the same service for m_pKbStruct that
									 // m_pOriginalUserStruct does for m_pUserStruct

	// Next members are additional ones needed for the kbs page  TODO deprecate soon
	bool		m_bKBisType1; // TRUE for adaptations KB definition, FALSE for a glosses KB definition
	wxString	m_tgtLanguageNameLabel; // InitDialog() sets it to "Target language name:"
	wxString	m_glossesLanguageNameLabel; // InitDialog() sets it to "Glossing language names:"
	wxString	m_tgtListLabel; // InitDialog() sets it to
					// "Existing shared databases (as   source,target   comma delimited language name pairs):"
	wxString	m_glsListLabel; // InitDialog() sets it to
					// "Existing shared databases (as   source,glossing   comma delimited language name pairs):"

	wxString	m_sourceLangName;
	wxString	m_targetLangName;
	wxString	m_glossLangName;

	// Support for showing informative message when user attempts to alter one or both
	// language codes for a KB definition which is parent to entries in the entry table -
	// the attmept will fail without giving any feedback to the administrator unless we
	// compare the before and after values for the codes being changed, and give the
	// helpful message when they have not changed as expected.
	bool			m_bUpdateTried;
	wxArrayString	m_listBeforeUpdate;
	wxArrayString	m_listAfterUpdate;

	// Support for prevention of the Manager user trying to remove a kb definition for the
	// project which is currently the active project and it is set up to be sharing to the
	// same remote kb and kbtype! (Clearly, to allow entries to flow in while entries are
	// being actively removed would be crazy - so we check for this and advise the Manager
	// user to first remove the sharing setup with the kb definition which is to be removed.
	// (In the app class there are also:
	// 	wxString		m_srcLangCodeOfCurrentRemoval;  and
	//  wxString		m_nonsrcLangCodeOfCurrentRemoval;
	//  which preserve the code values which a deletion is in progress, so we can ensure
	//  that nonone sets up a sharing to the old kb definition while the removal is in
	//  progress.)
	wxString		m_srcLangNameOfDeletion;
	wxString		m_nonsrcLangNameOfDeletion;
	int				m_kbTypeOfDeletion; //1, 2, or undefined (-1)

	DECLARE_EVENT_TABLE()
};
#endif
#endif /* KBSharingMgrTabbedDlg_h */

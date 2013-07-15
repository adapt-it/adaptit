/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharingMgrTabbedDlg.cpp
/// \author			Bruce Waters
/// \date_created	02 July 2013
/// \rcs_id $Id: KBSharingMgrTabbedDlg.cpp 3310 2013-06-19 07:14:50Z adaptit.bruce@gmail.com $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the KBSharingMgrTabbedDlg class.
/// The KBSharingMgrTabbedDlg class provides a dialog with tabbed pages in which an
/// appropriately authenticated user/manager of a remote kbserver installation may add,
/// edit or remove users stored in the user table of the mysql server, and/or add or 
/// remove knowledge base definitions stored in the kb table of the mysql server.
/// \derivation		The KBSharingMgrTabbedDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KBSharingMgrTabbedDlg.h"
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
//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
#include <wx/filesys.h> // for wxFileName
//#include <wx/dir.h> // for wxDir
//#include <wx/choicdlg.h> // for wxGetSingleChoiceIndex

#include "Adapt_It.h"
#include "helpers.h"
#include "KbServer.h"
#include "KBSharingMgrTabbedDlg.h"

/// Length of the byte-order-mark (BOM) which consists of the three bytes 0xEF, 0xBB and 0xBF
/// in UTF-8 encoding.
//#define nBOMLen 3

/// Length of the byte-order-mark (BOM) which consists of the two bytes 0xFF and 0xFE in
/// in UTF-16 encoding.
//#define nU16BOMLen 2

//static wxUint8 szBOM[nBOMLen] = {0xEF, 0xBB, 0xBF};
//static wxUint8 szU16BOM[nU16BOMLen] = {0xFF, 0xFE};

// event handler table for the KBSharingMgrTabbedDlg class
BEGIN_EVENT_TABLE(KBSharingMgrTabbedDlg, AIModalDialog)
	EVT_INIT_DIALOG(KBSharingMgrTabbedDlg::InitDialog)
	EVT_NOTEBOOK_PAGE_CHANGED(-1, KBSharingMgrTabbedDlg::OnTabPageChanging) // value from EVT_NOTEBOOK_PAGE_CHANGING are inconsistent across platforms - better to use ..._CHANGED
	EVT_BUTTON(ID_BUTTON_CLEAR_CONTROLS, KBSharingMgrTabbedDlg::OnButtonUserPageClearControls)
	EVT_LISTBOX(ID_LISTBOX_CUR_USERS, KBSharingMgrTabbedDlg::OnSelchangeUsersList)
	EVT_BUTTON(ID_BUTTON_ADD_USER, KBSharingMgrTabbedDlg::OnButtonUserPageAddUser)
	EVT_BUTTON(ID_BUTTON_REMOVE_USER, KBSharingMgrTabbedDlg::OnButtonUserPageRemoveUser)
	EVT_BUTTON(ID_BUTTON_EDIT_USER, KBSharingMgrTabbedDlg::OnButtonUserPageEditUser)
	EVT_CHECKBOX(ID_CHECKBOX_USERADMIN, KBSharingMgrTabbedDlg::OnCheckboxUseradmin)
	EVT_CHECKBOX(ID_CHECKBOX_KBADMIN, KBSharingMgrTabbedDlg::OnCheckboxKbadmin)
	EVT_BUTTON(wxID_OK, KBSharingMgrTabbedDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, KBSharingMgrTabbedDlg::OnCancel)

END_EVENT_TABLE()

KBSharingMgrTabbedDlg::KBSharingMgrTabbedDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Knowledge Base Sharing Manager"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	m_pApp = &wxGetApp();

	// wx Note: Since InsertPage also calls SetSelection (which in turn activates our OnTabSelChange
	// handler, we need to initialize some variables before CCTabbedNotebookFunc is called below.
	// Specifically m_nCurPage and pKB needs to be initialized - so no harm in putting all vars
	// here before the dialog controls are created via KBEditorDlgFunc.
	m_nCurPage = 0; // default to first page (Users)
#ifdef __WXGTK__
	m_bUsersListBoxBeingCleared = FALSE;
	m_bSourceKbsListBoxBeingCleared = FALSE;
	m_bTargetKbsListBoxBeingCleared = FALSE;
	m_bGlossKbsListBoxBeingCleared = FALSE;
#endif

	SharedKBManagerNotebookFunc(this, TRUE, TRUE);
	// The declaration is: SharedKBManagerNotebookFunc( wxWindow *parent, bool call_fit, bool set_sizer );

	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	// pointers to the controls common to each page (most of them) are obtained within
	// the LoadDataForPage() function

	// the following pointer to the KBSharingMgrTabbedDlg control is a single instance;
	// it can only be associated with a pointer after the SharedKBManagerNotebookFunc call above
	m_pKBSharingMgrTabbedDlg = (wxNotebook*)FindWindowById(ID_SHAREDKB_MANAGER_DLG);
	wxASSERT(m_pKBSharingMgrTabbedDlg != NULL);
}

KBSharingMgrTabbedDlg::~KBSharingMgrTabbedDlg()
{
}

void KBSharingMgrTabbedDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	// Initdialog is called once before the dialog is shown.

	// Get pointers to the controls created in the two pages, using FindWindowById(),
	// which is acceptable because the controls on each page have different id values
	// Listboxes
	m_pUsersListBox = (wxListBox*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_LISTBOX_CUR_USERS);
	wxASSERT(m_pUsersListBox != NULL);
	m_pSourceKbsListBox = (wxListBox*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_LISTBOX_SRC_LANG_CODE);
	wxASSERT(m_pSourceKbsListBox != NULL);
	m_pNonSourceKbsListBox = (wxListBox*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_LISTBOX_TGT_LANG_CODE);
	wxASSERT(m_pNonSourceKbsListBox != NULL);
	// Radio buttons
	m_pRadioButton_Type1KB = (wxRadioButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_RADIOBUTTON_TYPE1_KB);
	m_pRadioButton_Type2KB = (wxRadioButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_RADIOBUTTON_TYPE2_KB);
	// wxTextCtrls
	m_pEditUsername = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_USERNAME);
	wxASSERT(m_pEditUsername != NULL);
	m_pEditInformalUsername = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_INFORMAL_NAME);
	wxASSERT(m_pEditInformalUsername != NULL);
	m_pEditPersonalPassword = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_PASSWORD);
	wxASSERT(m_pEditPersonalPassword != NULL);
	m_pEditPasswordTwo = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_PASSWORD_TWO);
	wxASSERT(m_pEditPasswordTwo != NULL);
	m_pEditSourceCode = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_SRC);
	wxASSERT(m_pEditSourceCode != NULL);
	m_pEditNonSourceCode = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_TGT);
	wxASSERT(m_pEditNonSourceCode != NULL);
	// Checkboxes
	m_pCheckUserAdmin = (wxCheckBox*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_CHECKBOX_USERADMIN);
	wxASSERT(m_pCheckUserAdmin != NULL);
	m_pCheckKbAdmin = (wxCheckBox*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_CHECKBOX_KBADMIN);
	wxASSERT(m_pCheckKbAdmin != NULL);
	// Buttons
	m_pBtnUsersClearControls = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_CLEAR_CONTROLS);
	wxASSERT(m_pBtnUsersClearControls != NULL);
	m_pBtnUsersAddUser = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_ADD_USER);
	wxASSERT(m_pBtnUsersAddUser != NULL);
	m_pBtnUsersEditUser = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_EDIT_USER);
	wxASSERT(m_pBtnUsersEditUser != NULL);
	m_pBtnUsersRemoveUser = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_REMOVE_USER);
	wxASSERT(m_pBtnUsersRemoveUser != NULL);
	m_pBtnUsingRFC5654Codes = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_RFC5654);
	wxASSERT(m_pBtnUsingRFC5654Codes != NULL);
	m_pBtnAddKbDefinition = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_ADD_DEFINITION);
	wxASSERT(m_pBtnAddKbDefinition != NULL);
	m_pBtnClearBothLangCodeBoxes = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_CLEAR_BOXES);
	wxASSERT(m_pBtnClearBothLangCodeBoxes != NULL);
	m_pBtnLookupLanguageCodes = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_LOOKUP_THE_CODES);
	wxASSERT(m_pBtnLookupLanguageCodes != NULL);
	m_pBtnRemoveSelectedKBDefinition = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_REMOVE_SELECTED_DEFINITION);
	wxASSERT(m_pBtnRemoveSelectedKBDefinition != NULL);

	// For an instantiated KbServer class instance to use, we'll use the stateless one created within
	// KBSharingSetupDlg's creator function; and assign it to
	// KBSharingMgrTabbedDlg::m_pKbServer by the setter SetStatelessKbServerPtr() after
	// KBSsharingMgrTabbedDlg is instantiated
	m_pKbServer = NULL; // temporarily

	// Initialize the User page's checkboxes to OFF
	m_pCheckUserAdmin->SetValue(FALSE);
	m_pCheckKbAdmin->SetValue(FALSE);
/* temporary
	// Hook up to the m_usersList member of the adaptations KbServer instance
	m_pUsersList = m_pKbServer->GetUsersList();
	m_nUsersListCount = 0;
*/
	m_pUsersList = NULL; // remove both these when Jonathan restores my access to kbserver
	m_nUsersListCount = 0;

	// Create the 2nd UsersList to store original copies before user's edits etc
	// (destroy it in OnOK() and OnCancel())
	m_pOriginalUsersList = new UsersList;

	// Get the administrator's kbadmin and useradmin values from the app members for same
	m_bKbAdmin = m_pApp->m_kbserver_kbadmin;
	m_bUserAdmin = m_pApp->m_kbserver_useradmin;

	// Start m_pOriginalUserStruct off with a value of NULL. Each time this is updated,
	// the function which does so first deletes the struct stored in it (it's on the
	// heap), and then deep copies the one resulting from the user's listbox click and
	// stores it here. When deleted, this pointer should be reset to NULL. And the
	// OnCancel() and OnOK() functions must check here for a non-null value, and if found,
	// delete it from the heap before they return (to avoid a memory leak)
	m_pOriginalUserStruct = NULL;

	m_nCurPage = 0;
/* temporary
	LoadDataForPage(m_nCurPage); // start off showing the Users page (for now)
*/
}

// Setter for the stateless instance of KbServer created by KBSharingSetupDlg's creator
// (that KbServer instance will internally have it's m_bStateless member set to TRUE)
void KBSharingMgrTabbedDlg::SetStatelessKbServerPtr(KbServer* pKbServer)
{
	m_pKbServer = pKbServer;
}


KbServerUser* KBSharingMgrTabbedDlg::CloneACopyOfKbServerUserStruct(KbServerUser* pExistingStruct)
{
	KbServerUser* pClone = new KbServerUser;
	pClone->id = pExistingStruct->id;
	pClone->username = pExistingStruct->username;
	pClone->fullname = pExistingStruct->fullname;
	pClone->useradmin = pExistingStruct->useradmin;
	pClone->kbadmin = pExistingStruct->kbadmin;
	pClone->timestamp = pExistingStruct->timestamp;
	return pClone;
}

void KBSharingMgrTabbedDlg::DeleteClonedKbServerUserStruct()
{
	if (m_pOriginalUserStruct != NULL)
	{
		delete m_pOriginalUserStruct;
		m_pOriginalUserStruct = NULL;
	}
}

// This is called each time the page to be viewed is switched to; note: removed user
// entries are actually only pseudo-removed, their deleted field is set to 1;  these are
// included in the ListUsers() call, but we do not put them into the Users page's list box
void KBSharingMgrTabbedDlg::LoadDataForPage(int pageNumSelected)
{
	if (pageNumSelected == 0) // Users page
	{
		// Clear out anything from a previous button press done on this page
		// ==================================================================
		m_pUsersListBox->Clear();
		wxCommandEvent dummy;
		OnButtonUserPageClearControls(dummy);
		DeleteClonedKbServerUserStruct(); // ensure it's freed & ptr is NULL
		m_pKbServer->ClearUsersList(m_pOriginalUsersList);
		m_nUsersListCount = 0;
		// The m_pUsersList will be cleared within the ListUsers() call done below, so it
		// is unnecessary to do it here now
		// ==================================================================

		// Get the users data from the server, store in the list of KbServerUser structs, 
		// the call will clear the list first before storing what the server returns
		wxString username = m_pKbServer->GetKBServerUsername();
		wxString password = m_pKbServer->GetKBServerPassword();
		CURLcode result = CURLE_OK;
		if (!username.IsEmpty() && !password.IsEmpty())
		{
/* Temporarily disable until Jonathan restores my kbserver access
			result = (CURLcode)m_pKbServer->ListUsers(username, password);
			if (result == CURLE_OK)
			{
				// Set the counter for the m_usersList's length (it could be greater than
				// the number of list entries shown to the user, due to the presence of
				// pseudo-deleted "Remove User" entries)
				if (m_pUsersList->empty())
				{
					// Don't expect an empty list, so an English message will suffice
					wxMessageBox(_T("LoadDataForPage() error, the returned list of users was empty. Fix this."),
					_T("KB Sharing Manager error"), wxICON_WARNING | wxOK);
					m_pKbServer->ClearUsersList(m_pUsersList);
					return;
				}
				m_nUsersListCount = m_pUsersList->size();

				// Copy the list, before user gets a chance to modify anything
				CopyUsersList(m_pUsersList, m_pOriginalUsersList); // param 1 is src list, param2 is dest list

                // Load the username strings into m_pUsersListBox, the ListBox is sorted;
                // Note: the client data for each username string added to the list box is
                // the ptr to the KbServerUser struct itself which supplied the username
                // string, the struct coming from m_pUsersList
				LoadUsersListBox(m_pUsersListBox, m_nUsersListCount, m_pUsersList);
			}
			else
			{
				// If there was a cURL error, it will have been generated from within
				// ListUsers() already, so that will suffice
				;
			}
*/
		}
		else
		{
			// Don't expect this error, so an English message will do
			wxMessageBox(_T("LoadDataForPage() unable to call ListUsers() because password or username is empty, or both. The Manager won't work until this is fixed."),
				_T("KB Sharing Manager error"), wxICON_WARNING | wxOK);
		}
	}
	else // must be first Kbs page
	{
		m_pSourceKbsListBox->Clear();
		m_pNonSourceKbsListBox->Clear();


// TODO  the rest of it
	}
}

// Returns nothing. The ListBox is in sorted order, so Append() returns the index of where
// it actually ended up in the list, but we don't need it because the client data can be
// set within the Append() call
void KBSharingMgrTabbedDlg::LoadUsersListBox(wxListBox* pListBox, size_t count, UsersList* pUsersList)
{
	wxASSERT(count != 0); wxASSERT(pListBox); wxASSERT(pUsersList);
	if (pUsersList->empty())
		return;
	UsersList::iterator iter;
	UsersList::compatibility_iterator c_iter;
	int anIndex = -1;
	for (iter = pUsersList->begin(); iter != pUsersList->end(); ++iter)
	{
		// Assign the KbServerUser struct's pointer to pEntry
		anIndex++;
		c_iter = pUsersList->Item((size_t)anIndex);
		KbServerUser* pEntry = c_iter->GetData();
		// Make pEntry the client data, and the pListBox visible string be pEntry's
		// username member
		pListBox->Append(pEntry->username, (void*)pEntry); // param 2 is clientData ptr
	}
}

KbServerUser* KBSharingMgrTabbedDlg::GetUserStructFromList(UsersList* pUsersList, size_t index)
{
	wxASSERT(!pUsersList->empty());
	KbServerUser* pStruct = NULL;
	UsersList::compatibility_iterator c_iter;
	c_iter = pUsersList->Item(index);
	wxASSERT(c_iter);
	pStruct = c_iter->GetData();
	return pStruct;
}

// Make deep copies of the KbServerUser struct pointers in pSrcList and save them to pDestList
void KBSharingMgrTabbedDlg::CopyUsersList(UsersList* pSrcList, UsersList* pDestList)
{
	if (pSrcList->empty())
		return;
	UsersList::iterator iter;
	UsersList::compatibility_iterator c_iter;
	int anIndex = -1;
	for (iter = pSrcList->begin(); iter != pSrcList->end(); ++iter)
	{
		anIndex++;
		c_iter = pSrcList->Item((size_t)anIndex);
		KbServerUser* pEntry = c_iter->GetData();
		KbServerUser* pNew = new KbServerUser;
		pNew->id = pEntry->id;
		pNew->username = pEntry->username;
		pNew->fullname = pEntry->fullname;
		pNew->useradmin = pEntry->useradmin;
		pNew->kbadmin = pEntry->kbadmin;
		pNew->timestamp = pEntry->timestamp;
		pDestList->Append(pNew);
	}
}

void KBSharingMgrTabbedDlg::OnTabPageChanging(wxNotebookEvent& event)
{
	// OnTabPageChanging is called whenever any tab is selected
	int pageNumSelected = event.GetSelection();
	if (pageNumSelected == m_nCurPage)
	{
		// user selected same page, so just return
		return;
	}
	// if we get to here user selected a different page
	m_nCurPage = pageNumSelected;
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void KBSharingMgrTabbedDlg::OnOK(wxCommandEvent& event)
{
	// copy local values to the globals on the App -- if needed


	// Tidy up
	m_pKbServer->ClearUsersList(m_pOriginalUsersList); // this one is local to this
	delete m_pOriginalUsersList;
	m_pKbServer->ClearUsersList(m_pUsersList); // this one is in the adaptations 
									// KbServer instance & don't delete this one
	DeleteClonedKbServerUserStruct();

	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void KBSharingMgrTabbedDlg::OnCancel(wxCommandEvent& event)
{	
	// Tidy up
	m_pKbServer->ClearUsersList(m_pOriginalUsersList); // this one is local to this
	delete m_pOriginalUsersList;
	m_pKbServer->ClearUsersList(m_pUsersList); // this one is in the adaptations 
									// KbServer instance & don't delete this one
	DeleteClonedKbServerUserStruct();

	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void KBSharingMgrTabbedDlg::OnButtonUserPageClearControls(wxCommandEvent& WXUNUSED(event))
{
	wxString emptyStr = _T("");
	m_pUsersListBox->SetSelection(wxNOT_FOUND);
	m_pEditUsername->ChangeValue(emptyStr);
	m_pEditInformalUsername->ChangeValue(emptyStr);
	m_pEditPersonalPassword->ChangeValue(emptyStr);
	m_pEditPasswordTwo->ChangeValue(emptyStr);
	m_pCheckUserAdmin->SetValue(FALSE);
	m_pCheckKbAdmin->SetValue(FALSE);
}

bool KBSharingMgrTabbedDlg::AreBothPasswordsEmpty(wxString password1, wxString password2)
{
	if (password1.IsEmpty() && password2.IsEmpty())
	{
		return TRUE;
	}
	return FALSE;
}


// Return TRUE if the passwords match and are non-empty (both boxes empty is handled by
// the function AreBothPasswordsEmpty(), not this function), FALSE if mismatched
bool KBSharingMgrTabbedDlg::CheckThatPasswordsMatch(wxString password1, wxString password2)
{
	if (	(password1.IsEmpty() && !password2.IsEmpty()) ||
			(!password1.IsEmpty() && password2.IsEmpty()) ||
			(password1 != password2))
	{
		// The passwords are not matching, tell the user and force him to retype
		wxString msg = _("The passwords in the two boxes are not identical. Try again.");
		wxString title = _("Error: different passwords");
		wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
		// Clear the two password boxes
		wxString emptyStr = _T("");
		m_pEditPersonalPassword->ChangeValue(emptyStr);
		m_pEditPasswordTwo->ChangeValue(emptyStr);
		return FALSE;
	}
	else
		return TRUE;
}

void KBSharingMgrTabbedDlg::OnButtonUserPageAddUser(wxCommandEvent& WXUNUSED(event))
{
	// Username box with a value, informal username box with a value, password box with a
	// value, are mandatory. Test for these and if one is not set, abort the button press
	// and inform the user why. Also, a selection in the list is irrelevant, and would be
	// confusing, so if there is one, just remove it before proceeding
	m_nSel = m_pUsersListBox->GetSelection();
	if (m_nSel != wxNOT_FOUND)
	{
		m_pUsersListBox->SetSelection(wxNOT_FOUND);
	}
	wxString strUsername = m_pEditUsername->GetValue();
	wxString strFullname = m_pEditInformalUsername->GetValue();
	wxString strPassword = m_pEditPersonalPassword->GetValue();
	wxString strPasswordTwo = m_pEditPasswordTwo->GetValue();
	bool bKbadmin = FALSE; // initialize to default value
	bool bUseradmin = FALSE; // initialize to default value
	if (strUsername.IsEmpty() || strFullname.IsEmpty() || strPassword.IsEmpty() || strPasswordTwo.IsEmpty())
	{
		wxString msg = _("One or more of the text boxes: Username, Informal username, or one or both password boxes, are empty.\nEach of these must have appropriate text typed into them before an Add User request will be honoured. Do so now.\nIf you want to give this new user more than minimal privileges, use the checkboxes also.");
		wxString title = _("Warning: Incomplete user definition");
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		return;
	}
	else
	{
		// Test the passwords match (if they don't match, or a box is empty, then the call
		// will show an exclamation message from within it, and clear both password text boxes)
		bool bMatchedPasswords = CheckThatPasswordsMatch(strPassword, strPasswordTwo);
		if (!bMatchedPasswords)
		{
			// Unmatched passwords, the user has been told to try again
			return;
		}

		// Get the checkbox values
		bKbadmin = m_pCheckKbAdmin->GetValue();
		bUseradmin = m_pCheckUserAdmin->GetValue();

		// Create the new entry in the kbserver's user table
		CURLcode result = CURLE_OK;
		result = (CURLcode)m_pKbServer->CreateUser(strUsername, strFullname, strPassword, bKbadmin, bUseradmin);
		// Update the page if we had success, if no success, just clear the controls
		if (result == CURLE_OK)
		{
			LoadDataForPage(m_nCurPage);
		}
		else
		{
			// The creation did not succeed -- an error message will have been shown
			// from within the above CreateUser() call
			wxCommandEvent dummy;
			OnButtonUserPageClearControls(dummy);
		}
	}
	DeleteClonedKbServerUserStruct();
}

// When I checked the kbserver at kbserver.jmarsden.org, I found that the earliest user
// (using timestamp values) was not Jonathan as expected, but me - and I didn't set up
// that server instance, he did. So preventing the earliest user from being deleted may
// not be a robust way to ensure someone with useradmin = true privilege level is always
// retained in the user table. My fallback, unless Jonathan gives me some other clue, will
// be to simply prevent any user with useradmin = true privilege level from being deleted.
void KBSharingMgrTabbedDlg::OnButtonUserPageRemoveUser(wxCommandEvent& WXUNUSED(event))
{
	// Get the ID value, if we can't get it, return
	if (m_pOriginalUserStruct == NULL)
	{
		wxBell();
		return;
	}
	else
	{
		// Prevent removal if the user is a useradmin == true one, and tell the user
		// that's why his attempt was rejected
		if (m_pOriginalUserStruct->useradmin)
		{
			// This guy must not be deleted
			wxBell();
			wxString msg = _("This user has user administrator privilege level. Deleting this user may leave the server with nobody having permission to add, change or remove users. So we do not allow such removals.");
			wxString title = _("Warning: Illegal user deletion attempt");
			wxMessageBox(msg, title, wxICON_WARNING | wxOK);
			return;


		}
		int nID = (int)m_pOriginalUserStruct->id;
		// Remove the selected user from the kbserver's user table
		CURLcode result = CURLE_OK;
		result = (CURLcode)m_pKbServer->RemoveUser(nID);	
		// Update the page if we had success, if no success, just clear the controls
		if (result == CURLE_OK)
		{
			LoadDataForPage(m_nCurPage);
		}
		else
		{
			// The removal did not succeed -- an error message will have been shown
			// from within the above RemoveUser() call
			wxCommandEvent dummy;
			OnButtonUserPageClearControls(dummy);
		}
		DeleteClonedKbServerUserStruct();
	}
}

// The box state will have already been changed by the time control enters the handler's body
void KBSharingMgrTabbedDlg::OnCheckboxUseradmin(wxCommandEvent& WXUNUSED(event))
{
	bool bCurrentUseradminValue = m_pCheckUserAdmin->GetValue();
	// We set kbadmin value to TRUE if the Useradmin checkbox has just been ticked, but if
	// unticked, we leave the kbadmin value untouched. We must avoid having useradmin TRUE
	// but kbadmin FALSE
	if (bCurrentUseradminValue)
	{
		m_pCheckKbAdmin->SetValue(TRUE);
	}
}

// The box state will have already been changed by the time control enters the handler's body
void KBSharingMgrTabbedDlg::OnCheckboxKbadmin(wxCommandEvent& WXUNUSED(event))
{
	bool bCurrentKbadminValue = m_pCheckKbAdmin->GetValue();
	// We set useradmin value to FALSE if the Kbadmin checkbox has just been unticked, but if
	// ticked, we leave the useradmin value untouched. We must avoid having useradmin TRUE
	// but kbadmin FALSE
	if (bCurrentKbadminValue == FALSE)
	{
		m_pCheckUserAdmin->SetValue(FALSE);
	}
}


// The API call to kbserver should only generate a json field for those fields which have
// been changed; in the case of a password, leave the two edit boxes empty to retain the
// use of the old password, but if a password change is required, both boxes must have the
// same new password. The function works out what's changed by comparing the old values
// (obtained when the administrator clicks the user's entry in the list) with the values
// in the controls when the Edit User button is pressed.
void KBSharingMgrTabbedDlg::OnButtonUserPageEditUser(wxCommandEvent& WXUNUSED(event))
{
	// The way to do this is like in the PseudoDeleteOrUndeleteEntry() function, except
	// that more fields are potentially editable - only the ID and timestamp are not editable.

    // A selection in the list is mandatory because only if an existing user has been
    // selected is it possible to be editing an existing user. So check there is a
    // selection current. Note: if an existing user is a parent of entries in the entry
    // table, then his username cannot be edited and attempting to do so will generate an
    // sql error. When such an error happens, the administrator should be told to give this
    // user a whole new entry and leave the old one untouched.
	m_nSel = m_pUsersListBox->GetSelection();
	if (m_nSel == wxNOT_FOUND)
	{
		wxBell();
		wxString msg = _("No user in the list is selected. The only way to edit an existing user's properties is to first select that user's entry in the list box. Do so now, and try again.");
		wxString title = _("Warning: No selection");
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		return;
		
	}
	// Who's logged in? If I'm logged in, it would be an error to try change my username.
	// In the same vein, if I'm logged in and I've just added user XXX, then I can't
	// possibly be logged in as XXX - so changing XXX's username would be permissable,
	// provided XXX is not yet parent of any records. So the crucial test here is for
	// whether or not I'm trying to edit myself or not -- if I am, I must reject any
	// attempt at a username change
	wxString loggedInUser = m_pApp->GetKbServer(1)->GetKBServerUsername();
	wxString originalUsername = m_pOriginalUserStruct->username;
	bool bIAmEditingMyself = loggedInUser == originalUsername ? TRUE : FALSE;

	// Get the administrator's edited values - get every field, and check against the
	// original values (already stored in m_pOriginalUserStruct which was populated when
	// the administrator clicked on the list box's entry) to find which have changed. We
	// will use a bool to track which ones are to be used for constructing the appropriate
	// json string to be PUT to the server's entry for this user.
	wxString strUsername = m_pEditUsername->GetValue();
	wxString strFullname = m_pEditInformalUsername->GetValue();
	wxString strPassword = m_pEditPersonalPassword->GetValue();
	wxString strPasswordTwo = m_pEditPasswordTwo->GetValue();
	wxString emptyStr = _T("");

	// First check, prevent me editing my username if I'm the one currently logged in
	if (bIAmEditingMyself)
	{
		// If my 'before' and 'after' username strings are different, I'm about to create
		// chaos, so prevent it  -- and automatically restore my earlier username to the
		// box; but if I'm not changing my username, then control can progress and I can
		// legally change one or any or all of my password, fullname, useradmin & kbadmin
		// fields
		if (strUsername != originalUsername)
		{
			// Oops, prevent this!!
			wxBell();
			wxString msg = _("You are the currently logged in user, so you are not permitted to change your username. Your original username will be restored to the text box.\nIf you really want to have a different username, Add yourself under your new name and with a new password, and also give yourself appropriate privileges using the checkboxes, and then leave the project and re-enter it - authenticating with your new credentials.");
			wxString title = _("Warning: Illegal username change");
			wxMessageBox(msg, title, wxICON_WARNING | wxOK);
			m_pEditUsername->ChangeValue(originalUsername);
			return;

		}
	}

	// Deal with the passwords first. Acceptable situations are:
	// 1. Both password boxes empty -- in this case, retain the old password
	// 2. Both password boxes non-empty, and the password strings are identical - in
	//    this case the user can be safely assigned the new password
	// 3. Any variation from 1 or 2 is an error, warn the user to fix it first
	bool bBothEmpty = AreBothPasswordsEmpty(strPassword, strPasswordTwo);
	if (!bBothEmpty)
	{
		bool bMatched = CheckThatPasswordsMatch(strPassword, strPasswordTwo);
		if (!bMatched)
		{
			// A message has been seen, and the two password boxes cleared, so just return
			return;
		}
	}

	// Get the checkbox values; if the useradmin one has been ticked, the
	// kbadmin one will have been forced to be ticked as well (but the administrator is
	// free to untick the latter which will force the former to also be unticked)
	bool bUseradmin = m_pCheckUserAdmin->GetValue();
	bool bKbadmin = m_pCheckKbAdmin->GetValue();
	// Pass the values to the API call via a KbServerUser struct, store it in
	//  m_pUserStruct after cleaning out what's currently there
	m_pKbServer->ClearUserStruct(); // clears m_userStruct member, it's in stack frame
	// our m_pUserStruct member points that m_userStruct, so set it's contents below
	
	// We are ready to roll...
	int nID = (int)m_pOriginalUserStruct->id;  // this value never changes
	CURLcode result = CURLE_OK;
	// Initialize the booleans to safe defaults
	bool bUpdateUsername = FALSE;
	bool bUpdateFullName = FALSE;
	bool bUpdatePassword = FALSE;
	bool bUpdateKbadmin = FALSE;
	bool bUpdateUseradmin = FALSE;
    // Set m_pUserStruct to the current values, some or all of which may have been
    // edited; we pass this in to UpdateUser() below
	m_pUserStruct->id = nID;
	m_pUserStruct->username = strUsername;
	m_pUserStruct->fullname = strFullname;
	m_pUserStruct->kbadmin = bKbadmin;
	m_pUserStruct->useradmin = bUseradmin;
	// Set the booleans... 
    // Note: in the case of a successful attempt to update the password of the person who
    // is currently logged in, on return from the UpdateUser() call, and provided it
    // returned CURLE_OK and there was no HTTP error, then subsequent accesses of kbserver
    // will fail unless the password change is immediately propagated to the KbServers'
    // private m_kbServerPassword member for both adapting and glossing KbServer instances.
    // So we must check for these conditions being in effect, and make the changes with the
	// help of the bLoggedInUserJustChangedThePassword boolean
    bool bLoggedInUserJustChangedThePassword = FALSE;
	if (!strPassword.IsEmpty())
	{
		bUpdatePassword = TRUE;

		// Control can only get here if the logged in user, even if editing his own user
		// entry, isn't trying to change his username, so we can just rely below on the
		// value of bLoggedInUserJustChangedThePassword to guide what happens 
		bLoggedInUserJustChangedThePassword = TRUE;
	}
	if (bBothEmpty)
	{
		strPassword = emptyStr;
		bUpdatePassword = FALSE;
	}
	bUpdateUsername = m_pUserStruct->username == m_pOriginalUserStruct->username ? FALSE : TRUE;
	bUpdateFullName = m_pUserStruct->fullname == m_pOriginalUserStruct->fullname ? FALSE : TRUE;
	bUpdateUseradmin = m_pUserStruct->useradmin == m_pOriginalUserStruct->useradmin ? FALSE : TRUE;
	bUpdateKbadmin = m_pUserStruct->kbadmin == m_pOriginalUserStruct->kbadmin ? FALSE : TRUE;

	// We must prevent an attempt to edit the username, as most likely it owns some
	// records in the entry table. If we allow the attempt, nothing happens anyway, curl
	// returns CURLE_OK, an HTTP's 204 No Content is returned, so it's not treated as an
	// error, even thought sql has rejected the request. The problem with leaving it
	// happen is that the user gets no feedback that attempting an edit of the username
	// for one which is parent to entries, will fail. We can't unilaterally prevent it
	// either, because editing a username which has no entries yet because it's just been
	// created should succeed - and we'd want that; likewise we'd want to be able to have
	// Remove User succeed in such a case. So here we will test for bUpdateUsername being
	// TRUE, and if so give an information message to explain to the user that if the edit
	// hasn't succeeded, there's a good reason and tell him what it is.
	if (bUpdateUsername)
	{
		wxBell();
		wxString title = _("This edit attempt might fail...");
		wxString msg = _("Warning: editing the username for an existing entry may not succeed. The reason for success or failure is given below.\n\nIf the old username already 'owns' stored KB entries, no change to the user entry will be made - including no change to other parameters you may have edited (but no harm is done by you trying to do so).\nHowever, if the old username does not 'own' any stored KB entries yet, your attempt to change the username will succeed - and if you are changing other parameters as well in this attempt, those changes will succeed too.");
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
	}
	
	// Remove the selected user from the kbserver's user table
	result = (CURLcode)m_pKbServer->UpdateUser(nID, bUpdateUsername, bUpdateFullName, 
										bUpdatePassword, bUpdateKbadmin, bUpdateUseradmin, 
										m_pUserStruct, strPassword);	
	if (result == CURLE_OK)
	{
		// see what we've done and be ready for a new administrator action; but
		// LoadDataForPage() does a ListUsers() call, and if the password has been
		// changed, using the old password will give a failure - so made the needed
		// password updates in the rest of the application before calling it
		if (bLoggedInUserJustChangedThePassword)
		{
			// strPassword is the new password which has just been made the current one -
			// this is just the password string itself, not a digest (the digest form of
			// it was created at the point of need, within the UpdateUser) call itself)
			m_pApp->GetKbServer(1)->SetKBServerPassword(strPassword);
			m_pApp->GetKbServer(2)->SetKBServerPassword(strPassword);
		}
		LoadDataForPage(m_nCurPage); // calls OnButtonUserPageClearControls() internally
	}
	else
	{
        // The UpdateUser() call did not succeed -- an error message will have been shown
        // from within it; and since KbServer's m_userStruct member has the tweaked values
        // for the edit, rather than the original, and the list's selected entry's
        // ClientData ptr is pointing at the current contents of m_userStruct, we also have
        // to restore the original values to that pointer's contents in memory
		wxCommandEvent dummy;
		OnButtonUserPageClearControls(dummy); // clears selection too
		//Restore the contents of the entry's ClientData as explained above
		m_pUserStruct->id = m_pOriginalUserStruct->id;
		m_pUserStruct->username = m_pOriginalUserStruct->username;
		m_pUserStruct->fullname = m_pOriginalUserStruct->fullname;
		m_pUserStruct->useradmin = m_pOriginalUserStruct->useradmin;
		m_pUserStruct->kbadmin = m_pOriginalUserStruct->kbadmin;
        // The old password is still in effect; it is only changed in the storage within
        // the two KbServer instances m_pKbServer[0] and m_pKbServer[1] if a new one was
        // typed and the UpdateUser() call suceeded
	}
	DeleteClonedKbServerUserStruct();
}

void KBSharingMgrTabbedDlg::OnSelchangeUsersList(wxCommandEvent& WXUNUSED(event))
{
	// some minimal sanity checks - can't use Bill's helpers.cpp function
	// ListBoxSanityCheck() because it clobbers the user's selection just made
	if (m_pUsersListBox == NULL)
	{
		m_nSel = wxNOT_FOUND; // -1
		return;
	}
	if (m_pUsersListBox->IsEmpty())
	{
		m_nSel = wxNOT_FOUND; // -1
		return;
	}
	m_nSel = m_pUsersListBox->GetSelection();
	// Get the entry's KbServerUser struct which is its associated client data (no need to
	// also get the username string from the ListBox because the struct's username field
	// contains the same string)
	m_pUserStruct = (KbServerUser*)m_pUsersListBox->GetClientData(m_nSel);

	// Put a copy in m_pOriginalUserStruct, in case the administrator clicks the
	// Edit User button - the latter would use what's in this variable to compare
	// the user's edits to see what has changed, and proceed accordingly. The
	// Remove User button also uses the struct in m_pOriginalUserStruct to get
	// at the ID value for the user to be removed
	DeleteClonedKbServerUserStruct();
	m_pOriginalUserStruct = CloneACopyOfKbServerUserStruct(m_pUserStruct);

	// Use the struct to fill the Users page's controls with their required data
	m_pEditUsername->ChangeValue(m_pUserStruct->username);
	m_pEditInformalUsername->ChangeValue(m_pUserStruct->fullname);
	m_pEditPersonalPassword->ChangeValue(_T("")); // we can't recover it, so user must
					// look up his records if he can't remember what it was, too bad if he
					// didn't make any records! In that case, he has to set a new password
					// and inform the user of it about the change, or just refrain from
					// clicking the Edit User button if setting a password was all he
					// wanted to do (in which case the old one remains in effect)
	m_pCheckUserAdmin->SetValue(m_pUserStruct->useradmin);
	m_pCheckKbAdmin->SetValue(m_pUserStruct->kbadmin);
	
	// Note 1: selecting in the list does NOT make any change to the user table in the mysql
	// database, nor does editing any of the control values. The user table is only
	// affected when the user clicks one of the buttons Add User, Edit User, or Remove
	// User, and it is at that time that the controls' values are grabbed and used.

	// Note 2: Remove User does a real deletion; the entry is not retained in the user
	// table
}






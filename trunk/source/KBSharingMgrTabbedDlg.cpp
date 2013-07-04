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
	m_pTargetKbsListBox = (wxListBox*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_LISTBOX_TGT_LANG_CODE);
	wxASSERT(m_pTargetKbsListBox != NULL);
	m_pGlossKbsListBox = (wxListBox*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_LISTBOX_GLOSS_LANG_CODE);
	wxASSERT(m_pGlossKbsListBox != NULL);
	// wxTextCtrls
	m_pEditUsername = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_USERNAME);
	wxASSERT(m_pEditUsername != NULL);
	m_pEditInformalUsername = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_INFORMAL_NAME);
	wxASSERT(m_pEditInformalUsername != NULL);
	m_pEditPersonalPassword = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_PASSWORD);
	wxASSERT(m_pEditPersonalPassword != NULL);
	m_pEditSourceCode = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_SRC);
	wxASSERT(m_pEditSourceCode != NULL);
	m_pEditTargetCode = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_TGT);
	wxASSERT(m_pEditTargetCode != NULL);
	m_pEditGlossCode = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_GLOSSES);
	wxASSERT(m_pEditGlossCode != NULL);
	// Checkboxes
	m_pCheckUserAdmin = (wxCheckBox*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_CHECK_USERADMIN);
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
	m_pBtnTargetListNoSelection = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_NO_UPPER_SELECTION);
	wxASSERT(m_pBtnTargetListNoSelection != NULL);
	m_pBtnGlossListNoSelection = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_NO_LOWER_SELECTION);
	wxASSERT(m_pBtnGlossListNoSelection != NULL);
	m_pBtnUsingRFC5654Codes = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_RFC5654);
	wxASSERT(m_pBtnUsingRFC5654Codes != NULL);
	m_pBtnAddKbDefinition = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_ADD_DEFINITION);
	wxASSERT(m_pBtnAddKbDefinition != NULL);
	m_pBtnUpdateKbDefinition = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_UPDATE_DEFINITION);
	wxASSERT(m_pBtnUpdateKbDefinition != NULL);
	m_pBtnLookupLanguageCodes = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_LOOKUP_THE_CODES);
	wxASSERT(m_pBtnLookupLanguageCodes != NULL);

	// For an instantiated KbServer class instance to use, we'll use the one for
	// adaptations, and ignore the one for glossing. Be sure there is one available! The
	// menu item should be disabled if the project is not yet a kb sharing one.
	m_pKbServer = m_pApp->GetKbServer(1); // 1 is adaptations KbServer instance, 2 is  glosses KbServer instance
	wxASSERT(m_pKbServer != NULL);

	// Initialize the User page's checkboxes to OFF
	m_pCheckUserAdmin->SetValue(FALSE);
	m_pCheckKbAdmin->SetValue(FALSE);

	// Hook up to the m_usersList member of the adaptations KbServer instance
	m_pUsersList = m_pKbServer->GetUsersList();
	m_nUsersListCount = 0;

	// Create the 2nd UsersList to store original copies before user's edits etc
	// (destroy it in OnOK() and OnCancel())
	m_pOriginalUsersList = new UsersList;

	// Get the administrator's kbadmin and useradmin values from the app members for same
	m_bKbAdmin = m_pApp->m_kbserver_kbadmin;
	m_bUserAdmin = m_pApp->m_kbserver_useradmin;


	m_nCurPage = 0;
	LoadDataForPage(m_nCurPage); // start off showing the Users page (for now)
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

// TODO -- any more?
			}
			else
			{
				// If there was a cURL error, it will have been generated from within
				// ListUsers() already, so that will suffice
				;
			}
		}
		else
		{
			// Don't expect this error, so an English message will do
			wxMessageBox(_T("LoadDataForPage() unable to call ListUsers() because password or username is empty, or both. The Manager won't work until this is fixed."),
				_T("KB Sharing Manager error"), wxICON_WARNING | wxOK);
		}
	}
	else // must be Kbs page
	{
		m_pSourceKbsListBox->Clear();
		m_pTargetKbsListBox->Clear();
		m_pGlossKbsListBox->Clear();


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

	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void KBSharingMgrTabbedDlg::OnCancel(wxCommandEvent& event)
{	
	// Tidy up
	m_pKbServer->ClearUsersList(m_pOriginalUsersList); // this one is local to this
	delete m_pOriginalUsersList;
	m_pKbServer->ClearUsersList(m_pUsersList); // this one is in the adaptations 
									// KbServer instance & don't delete this one


	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void KBSharingMgrTabbedDlg::OnButtonUserPageClearControls(wxCommandEvent& WXUNUSED(event))
{
	wxString emptyStr = _T("");
	m_pUsersListBox->SetSelection(wxNOT_FOUND);
	m_pEditUsername->ChangeValue(emptyStr);
	m_pEditInformalUsername->ChangeValue(emptyStr);
	m_pEditPersonalPassword->ChangeValue(emptyStr);
	m_pCheckUserAdmin->SetValue(FALSE);
	m_pCheckKbAdmin->SetValue(FALSE);
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
	bool bKbadmin = FALSE; // initialize to default value
	bool bUseradmin = FALSE; // initialize to default value
	if (strUsername.IsEmpty() || strFullname.IsEmpty() || strPassword.IsEmpty())
	{
		wxString msg = _("One or more of the text boxes: Username, Informal username, and password, are empty.\nEach of these must have appropriate text typed into them before an Add User request will be honoured. Do so now.");
		wxString title = _("Warning: Incomplete user definition");
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		return;
	}
	else
	{
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






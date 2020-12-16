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
/// appropriately authenticated user/manager of a remote KBserver installation may add,
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

#define _WANT_DEBUGLOG // comment out to suppress wxLogDebug() calls

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
#include <wx/filesys.h> // for wxFileName
//#include <wx/dir.h> // for wxDir
//#include <wx/choicdlg.h> // for wxGetSingleChoiceIndex
#include <wx/html/htmlwin.h> // for display of the "Help for Administrators.htm" file from the Administrator menu
#include <wx/textfile.h>

#if defined(_KBSERVER)

#include "Adapt_It.h"
#include "helpers.h"
#include "KbServer.h"
#include "LanguageCodesDlg.h"
#include "LanguageCodesDlg_Single.h"
#include "Adapt_ItView.h"
#include "KBSharingMgrTabbedDlg.h"
#include "HtmlFileViewer.h"
#include "MainFrm.h"
#include "StatusBar.h"

extern bool gbIsGlossing;

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
	// Note: value from EVT_NOTEBOOK_PAGE_CHANGING are inconsistent across platforms - so
	// it's better to use EVT_NOTEBOOK_PAGE_CHANGED
	EVT_NOTEBOOK_PAGE_CHANGED(-1, KBSharingMgrTabbedDlg::OnTabPageChanged)
	// For page 1: event handlers for managing Users of the shared databases
	EVT_BUTTON(ID_BUTTON_CLEAR_CONTROLS, KBSharingMgrTabbedDlg::OnButtonUserPageClearControls)
	EVT_LISTBOX(ID_LISTBOX_CUR_USERS, KBSharingMgrTabbedDlg::OnSelchangeUsersList)
	EVT_BUTTON(ID_BUTTON_ADD_USER, KBSharingMgrTabbedDlg::OnButtonUserPageAddUser)
	EVT_BUTTON(ID_BUTTON_SHOW_PASSWORD, KBSharingMgrTabbedDlg::OnButtonShowPassword)
	//EVT_BUTTON(ID_BUTTON_EDIT_USER, KBSharingMgrTabbedDlg::OnButtonUserPageEditUser)
	EVT_CHECKBOX(ID_CHECKBOX_USERADMIN, KBSharingMgrTabbedDlg::OnCheckboxUseradmin)
	//EVT_CHECKBOX(ID_CHECKBOX_KBADMIN, KBSharingMgrTabbedDlg::OnCheckboxKbadmin)
	EVT_BUTTON(ID_BUTTON_CHANGE_PERMISSION, KBSharingMgrTabbedDlg::OnButtonUserPageChangePermission)
	EVT_BUTTON(ID_BUTTON_CHANGE_FULLNAME, KBSharingMgrTabbedDlg::OnButtonUserPageChangeFullname)
	EVT_BUTTON(wxID_OK, KBSharingMgrTabbedDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, KBSharingMgrTabbedDlg::OnCancel)
	// For page 2: KB Shared database (definitions) page
	EVT_RADIOBUTTON(ID_RADIOBUTTON_TYPE1_KB, KBSharingMgrTabbedDlg::OnRadioButton1KbsPageType1)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_TYPE2_KB, KBSharingMgrTabbedDlg::OnRadioButton2KbsPageType2)
	//EVT_BUTTON(ID_BUTTON_LOOKUP_THE_CODES, KBSharingMgrTabbedDlg::OnBtnKbsPageLookupLanguageNames)
	//EVT_BUTTON(ID_BUTTON_RFC5646, KBSharingMgrTabbedDlg::OnBtnKbsPageRFC5646Codes)
	EVT_BUTTON(ID_BUTTON_CLEAR_LIST_SELECTION, KBSharingMgrTabbedDlg::OnButtonKbsPageClearListSelection)
	EVT_BUTTON(ID_BUTTON_CLEAR_BOXES, KBSharingMgrTabbedDlg::OnButtonKbsPageClearBoxes)
	EVT_BUTTON(ID_BUTTON_ADD_DEFINITION, KBSharingMgrTabbedDlg::OnButtonKbsPageAddKBDefinition)
	EVT_LISTBOX(ID_LISTBOX_KB_CODE_PAIRS, KBSharingMgrTabbedDlg::OnSelchangeKBsList)
	END_EVENT_TABLE()

KBSharingMgrTabbedDlg::KBSharingMgrTabbedDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Shared Database Server Manager"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
#if defined  (_DEBUG)
	wxLogDebug(_T("\n\n********  KBSharingMgrTabbedDlg() line %d: ENTERING  **********"),__LINE__);
#endif

	m_pApp = &wxGetApp();
	m_pApp->m_bDoingChangeFullname = FALSE; // initialize
	m_pApp->m_bUseForeignOption = TRUE; // we use this to say "The KB Sharing Mgr is where control is"
	m_pApp->m_bWithinMgr = FALSE; // control is not inside yet, InitDialog() will set it TRUE
								  // if the access is granted to the user page

	// Entry was happened, use the TRUE value in ConfigureMovedDatFile() for case lookup_user
	// to ensure the constraint, user1 == user2 == m_strUserID is satisfied for the lookup_user
	// case just a bit below, as refactoring I've removed the need to have Authenticate2Dlg
	// called in order to open the Mgr. Looking at other usernames than m_strUserID from the
	// currently open shared project, is a job for the internals of the Mgr, once opened
	m_pApp->m_bKBSharingMgrEntered = TRUE;

	// set the m_bUser1IsUser2 boolean to TRUE, because we are contraining entry to a
	// lookup_user case ( = 2) in which user1 == user2 == app's m_strUserID (the project's user)
	m_pApp->m_bUser1IsUser2 = TRUE;
	bool bExecutedOK = FALSE;
	if (!m_pApp->m_bKBSharingMgrEntered)
	{
		// BEW 16Dec20 Two accesses of the user table are needed. The first, here, LookupUser() 
		// determines whether or not user2 exists in the user table, and what that user's 
		// useradmin permission level is. This check is not needed again if the user has gained 
		// access to the manager, since within the manager the list of users may be altered
		// and require a new call of ListUsers() be made from there.
		// (Only call LoadDataForPage(0) once. When entering to the manager. Thereafter,
		// keep the kbserver user table in sync with the user page by a function, which reads 
		// the results file of do_list_users.exe and updates the m_pOriginalUsersList and 
		// m_pUsersListForeign 'in place' with an Update....() function

#if defined  (_DEBUG)
		wxLogDebug(_T("ConfigureDATfile(lookup_user = %d) line %d: entering, in Mgr creator"), 
			lookup_user, __LINE__);
#endif
		bool bReady = m_pApp->ConfigureDATfile(lookup_user); // case = 2
		if (bReady)
		{
			// The input .dat file is now set up ready for do_user_lookup.exe
			wxString execFileName = _T("do_user_lookup.exe");
			wxString execPath = m_pApp->execPath;
			wxString resultFile = _T("lookup_user_return_results.dat");
#if defined  (_DEBUG)
			wxLogDebug(_T("CallExecute(lookup_user = %d) line %d: entering, in Mgr creator"),
				lookup_user, __LINE__);
#endif
			bExecutedOK = m_pApp->CallExecute(lookup_user, execFileName, execPath, resultFile, 32, 33);
			// In above call, last param, bReportResult, is default FALSE therefore omitted;
			// wait msg 32 is _("Authentication succeeded."), and msg 33 is _("Authentication failed.")

			// m_bHasUseradminPermission has now been set, if LookupUser() succeeded, so
			// get what its access permission level is. TRUE allows access to the manager.
			// (in the entry table, '1' value for useradmin)
		}
		else
		{
			// logging is done, so just return -1 if there was a configure error
			return;
		}
	}

	// Disallow entry, if user has insufficient permission level
	if (bExecutedOK && !m_pApp->m_bHasUseradminPermission)
	{
		wxString msg = _("Access to the Knowledge Base Sharing Manager denied. Insufficient permission level, 0. Choose a different user having level 1.");
		wxString title = _("Warning: permission too low");
		wxMessageBox(msg, title, wxICON_WARNING & wxOK);
		return;
	}

	// wx Note: Since InsertPage also calls SetSelection (which in turn activates our OnTabSelChange
	// handler, we need to initialize some variables before CCTabbedNotebookFunc is called below.
	// Specifically m_nCurPage and pKB needs to be initialized - so no harm in putting all vars
	// here before the dialog controls are created via KBEditorDlgFunc.
	m_nCurPage = 0; // default to first page (Users)

	SharedKBManagerNotebookFunc2(this, TRUE, TRUE);
	// The declaration is: SharedKBManagerNotebookFunc2( wxWindow *parent, bool call_fit, bool set_sizer );

    // whm 5Mar2019 Note: The SharedKBManagerNotebookFunc2() tabbed dialog now
    // uses the wxStdDialogButtonSizer, and so we need not call the 
    // ReverseOkCancelButtonsForMac() function below.
	//bool bOK;
	//bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	//bOK = bOK; // avoid warning
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
	wxASSERT(m_pApp->m_bKBSharingEnabled);
	// Entry has been effected, but once in, we need user1 to be different from
	// user2, so here set app's m_bWithinMgr to TRUE.
	m_pApp->m_bWithinMgr = TRUE;

#if defined  (_DEBUG)
	wxLogDebug(_T("Init_Dialog(for Sharing Mgr) line %d: entering"), __LINE__);
#endif

	// Initdialog is called once before the dialog is shown.
	m_bUpdateTried = FALSE;
	// In the KB Sharing Manager, m_bUserAuthenticating should be FALSE,
	// because it should be TRUE only when using the Authenticate2Dlg.cpp
	// outside of the KB Sharing Manager - and in that circumstance, "Normal"
	// storage variables apply; but different ones for the Manager.

	// Get pointers to the controls created in the two pages, using FindWindowById(),
	// which is acceptable because the controls on each page have different id values
	// Listboxes
	m_pUsersListBox = (wxListBox*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_LISTBOX_CUR_USERS);
	wxASSERT(m_pUsersListBox != NULL);

	m_pConnectedTo = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXT_CONNECTED_TO);
	wxASSERT(m_pConnectedTo != NULL);
	m_pTheUsername = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_THE_USERNAME);
	wxASSERT(m_pTheUsername != NULL);
	m_pEditInformalUsername = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_INFORMAL_NAME);
	wxASSERT(m_pEditInformalUsername != NULL);
	m_pEditPersonalPassword = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_PASSWORD);
	wxASSERT(m_pEditPersonalPassword != NULL);
	m_pEditPasswordTwo = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_PASSWORD_TWO);
	wxASSERT(m_pEditPasswordTwo != NULL);
	m_pEditShowPasswordBox = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_SEL_USER_PWD);
	wxASSERT(m_pEditShowPasswordBox != NULL);

	// Checkboxes (only the users page has them)
	m_pCheckUserAdmin = (wxCheckBox*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_CHECKBOX_USERADMIN);
	wxASSERT(m_pCheckUserAdmin != NULL);

	// Buttons
	m_pBtnUsersClearControls = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_CLEAR_CONTROLS);
	wxASSERT(m_pBtnUsersClearControls != NULL);
	m_pBtnUsersAddUser = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_ADD_USER);
	wxASSERT(m_pBtnUsersAddUser != NULL);

	// Set an appropriate 'ForManager' KbServer instance which we'll need for the
	// services it provides regarding access to the KBserver
	// We need to use app's m_pKbServer_ForManager as the pointer, but it should be NULL
	// when the Manager is created, and SetKbServer() should create the needed 'ForManager'
	// instance and assign it to our m_pKbServer in-class pointer. So check for the NULL;
	// if there is an instance currently, we'll delete that and recreate a new one on heap
	// to do that
	wxASSERT(m_pApp->m_pKbServer_ForManager == NULL); // catch non-NULL in debug build
	// Rather than have a SetKbServer() function which is not self documenting about what
	// m_pKbServer is pointing at, make the app point explicit in this module
	if (m_pApp->m_pKbServer_ForManager == NULL)
	{
		m_pApp->m_pKbServer_ForManager = new KbServer(1, TRUE); // TRUE is bForManager
        // That created and set app's m_pKbServer_ForManager to a 'ForManager' KbServer
        // instance and now we assign that to the Manager's m_pKbServer pointer
        m_pKbServer = m_pApp->m_pKbServer_ForManager;
        // (Re-vectoring m_pKbServer to the app's m_pKbServer_Persistent KbServer*
        // instance should only be possible to do in the handler for deleting a remote
        // kb. Everywhere else in this module, m_pKbServer_ForManager is used exclusively)
        m_pKbServer->SetKB(m_pApp->m_pKB); // set ptr to local target text CKB* for local 
										   // KB access as needed
	}
	else
	{
		// Kill the one that somehow escaped death earlier, and make a new one
		delete m_pApp->m_pKbServer_ForManager; // We must kill it, because the user
				// might have just switched to a different adaptation project and
				// so the old source and non-source language codes may now be incorrect
				// for the project which we are currently in
		m_pApp->m_pKbServer_ForManager = new KbServer(1, TRUE); // TRUE is bForManager
        m_pKbServer = m_pApp->m_pKbServer_ForManager;
        m_pKbServer->SetKB(m_pApp->m_pKB); // set ptr to local target text CKB* for local 
										   // KB access as needed
	}
	// These are reasonable defaults for initialization purposes
	m_pApp->m_pKbServer_ForManager->SetKBServerIpAddr(m_pApp->m_strKbServerIpAddr);
	m_pApp->m_pKbServer_ForManager->SetKBServerUsername(m_pApp->m_curNormalUsername);
	m_pApp->m_pKbServer_ForManager->SetKBServerPassword(m_pApp->m_curNormalPassword);

	// Initialize the User page's checkboxes to OFF
	m_pCheckUserAdmin->SetValue(FALSE);
	//m_pCheckKbAdmin->SetValue(FALSE);

	// Hook up to the m_usersList member of the KbServer instance
	m_pUsersListForeign = m_pKbServer->GetUsersListForeign(); // an accessor for m_usersListForeign

	// Add the kbserver's ipAddr to the static text 2nd from top of the tabbed dialog
	wxString myStaticText = m_pConnectedTo->GetLabel();
	myStaticText += _T("  "); // two spaces, for ease in reading the value

	wxString theIpAddr = m_pKbServer->GetKBServerIpAddr();

	myStaticText += theIpAddr; // an accessor for m_kbServerIpAddrBase
	m_pConnectedTo->SetLabel(myStaticText);

	// Create the 2nd UsersList to store original copies before user's edits etc
	// (destroy it in OnOK() and OnCancel())
	m_pOriginalUsersList = new UsersListForeign;

	// BEW 13Nov20 changed, 
	m_bKbAdmin = TRUE;  // was m_pApp->m_kbserver_kbadmin; // BEW always TRUE now as of 28Aug20
	m_bUserAdmin = TRUE; // was m_pApp->m_kbserver_useradmin; // has to be TRUE for anyone getting access

	// Start m_pOriginalUserStruct off with a value of NULL. Each time this is updated,
	// the function which does so first deletes the struct stored in it (it's on the
	// heap), and then deep copies the one resulting from the user's listbox click and
	// stores it here. When deleted, this pointer should be reset to NULL. And the
	// OnCancel() and OnOK() functions must check here for a non-null value, and if found,
	// delete it from the heap before they return (to avoid a memory leak)
	m_pOriginalUserStruct = NULL;
	// Ditto for the one for the Create Kbs page and the Edit Kbs page
	//m_pOriginalKbStruct = NULL; // BEW deprecated 13Nov20

	// Next 8 for Create Kbs page and Edit Kbs page
	m_bKBisType1 = TRUE; // initialize, and the m_pRadioKBType1 button on kbs page is
						 // preset (in wxDesigner) to be ON on first entry to that page
						 // so it isn't necessary to do anything more here
	// Start by showing Users page
	m_nCurPage = 0;
#if defined  (_DEBUG)
	wxLogDebug(_T("LoadDataForPage(for Sharing Mgr) line %d: entering"), __LINE__);
#endif
	LoadDataForPage(m_nCurPage); // start off showing the Users page (for now)
#if defined  (_DEBUG)
	wxLogDebug(_T("LoadDataForPage(for Sharing Mgr) line %d: just exited"), __LINE__);
#endif
	m_pApp->GetView()->PositionDlgNearTop(this);

#if defined  (_DEBUG)
	wxLogDebug(_T("Init_Dialog(for Sharing Mgr) line %d: exiting"), __LINE__);
#endif
}

KbServer*KBSharingMgrTabbedDlg::GetKbServer()
{
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	//wxLogDebug(_T("KBSharingMgrTabbedDlg::GetKbServer(): m_bForManager = %d"),m_pKbServer->m_bForManager ? 1 : 0);
#endif
   return m_pKbServer;
}
// BEW 14Nov20 updated
KbServerUserForeign* KBSharingMgrTabbedDlg::CloneACopyOfKbServerUserStruct(KbServerUserForeign* pExistingStruct)
{
	KbServerUserForeign* pClone = new KbServerUserForeign;
	//pClone->id = pExistingStruct->id;
	pClone->username = pExistingStruct->username;
	pClone->fullname = pExistingStruct->fullname;
	pClone->password = pExistingStruct->password;
	pClone->useradmin = pExistingStruct->useradmin;
	//pClone->timestamp = pExistingStruct->timestamp;
	return pClone;
}

// BEW 28Aug20 updated
KbServerKb* KBSharingMgrTabbedDlg::CloneACopyOfKbServerKbStruct(KbServerKb* pExistingStruct)
{
	wxUnusedVar(pExistingStruct);
	/*
	KbServerKb* pClone = new KbServerKb;
	//pClone->id = pExistingStruct->id;
	pClone->sourceLanguageName = pExistingStruct->sourceLanguageName;
	pClone->targetLanguageName = pExistingStruct->targetLanguageName;
	pClone->kbType = pExistingStruct->kbType;
	pClone->username = pExistingStruct->username;
	//pClone->timestamp = pExistingStruct->timestamp;
	//pClone->deleted = pExistingStruct->deleted;
	return pClone;
	*/
	return (KbServerKb*)NULL;
}

void KBSharingMgrTabbedDlg::DeleteClonedKbServerUserStruct()
{
	if (m_pOriginalUserStruct != NULL)
	{
		delete m_pOriginalUserStruct;
		m_pOriginalUserStruct = NULL;
	}
}
/* BEW 3Nov20 deprecated
void KBSharingMgrTabbedDlg::DeleteClonedKbServerKbStruct()
{
	if (m_pOriginalKbStruct != NULL)
	{
		delete m_pOriginalKbStruct;
		m_pOriginalKbStruct = NULL;
	}
}
*/
// BEW 29Aug20 updated, keep 'earliest' in the name, but change the protocol. Now we
// allow deletions of all but the following user table entries: the one for username 
// kbadmin, and the one with username matching the current pApp->m_strUserID value
// (of course, if the user changes m_strUserID, it makes *any* username potentially
// deletable - we'll accept the risk, it's unlikely to trip any naive user up)
// The function should return a comma-separated string containing the names of the 
// two non-removable users; or _T("0") for any which are absent
wxString KBSharingMgrTabbedDlg::GetEarliestUseradmin(UsersListForeign* pUsersListForeign) 
{
	size_t count = pUsersListForeign->size();
	wxString comma(_T(','));
	wxString aUser;
	wxString strBoth = wxEmptyString;
	//int nEarliestID = 99999; // initialize
	//int anID = 0;
	KbServerUserForeign* pUserStructForeign = NULL;
	size_t index;
	wxString user1 = _T("kbadmin"); // not to ever be removed
	wxString user2 = m_pApp->m_strUserID;  // nor is this one to be removed, whatever it is
	int counter = 0;
	// Verify these are in the list, return _T("0") for any which
	// are not present
	for (index = 0; index < count; index++)
	{
		pUserStructForeign = GetUserStructFromList(pUsersListForeign, index); // <<-- already updated BEW 28Aug20
		aUser = pUserStructForeign->username;

		// BEW 29Aug20 new code - see comment above definition
		if (aUser == user1)
		{
			counter++;
			if (strBoth.IsEmpty())
			{
				strBoth = user1;

			}
			else
			{
				strBoth += comma + user1;
			}
		}
		if (aUser == user2)
		{
			counter++;
			if (strBoth.IsEmpty())
			{
				strBoth = user2;
			}
			else
			{
				strBoth += comma + user2;
			}
		}
	}
	if (counter == 0)
	{
		strBoth = _T("0") + comma + _T("0");
		return strBoth;
	}
	else if (counter == 1)
	{
		strBoth += comma + _T("0");
		return strBoth;
	}
	// counter must be 2, so both strings are present
	return strBoth;
}

// This is called each time the page to be viewed is switched to
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
		m_pKbServer->ClearUsersListForeign(m_pOriginalUsersList);
		// The m_pUsersList will be cleared within the ListUsers() call done below, so it
		// is unnecessary to do it here now
		// ==================================================================

		// Get the users data from the server, store in the list of KbServerUser structs,
		// the call will clear the list first before storing what the server returns
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("KBSharingMgrTabbedDlg::LoadDataForPage(): m_pKbServer = %p , m_bForManager = %d  , page is Users page"),
		m_pKbServer, (int)m_pKbServer->m_bForManager ? 1 : 0);
#endif
		// The following values come from Authenticate2Dlg() where Manager access
		// was requested, and checking user2 for existing in the user table
		
		wxString password = _T(""); // initialize
		wxString username = m_pApp->m_curNormalUsername; // most likely correct one
		wxString user2 = m_pApp->m_Username2; // most likely correct one

		if (m_pApp->m_Username2 != m_pApp->m_strUserID)
		{
			// authenticating user1 is not same user as user2, so we
			// need to authenticate using user1's password
			password = m_pApp->m_curNormalPassword;
		}
		else
		{
			// user1 and user2 are same
			// These could be project's user, or some other listed user being
			// used for authenticating in
			if (m_pApp->m_Username2 == m_pApp->m_strUserID)
			{
				// identical, and the current project's user so use user1's
				password = m_pApp->m_curNormalPassword;
			}
			else
			{
				password = m_pApp->m_curAuthPassword; // the Mgr one, hopefully it's been set
			}
		}

		wxString ipAddr = m_pApp->m_curIpAddr;
		if (m_pApp->m_bDoingChangeFullname)
		{
			// Is changing the fullname associated with m_strUserID? Or with
			// some other listed user which is not the one for the running project?

			if (m_pApp->m_curAuthUsername == m_pApp->m_strUserID)
			{
				// we can use m_curNormalUsername
				username = m_pApp->m_curNormalUsername;
			}
			else
			{
				username = m_pApp->m_curAuthUsername;
			}



		wxASSERT(!m_pApp->m_strChangeFullname_User2.IsEmpty());
			username = m_pApp->m_strChangeFullname_User2;
			m_pApp->m_Username2 = m_pApp->m_strChangeFullname_User2;
			// insurance for next two ?? 
			if (!m_pApp->m_strChangeFullname.IsEmpty())
			{
				m_pApp->m_curNormalFullname = m_pApp->m_strChangeFullname;
				m_pApp->UpdateCurFullname(m_pApp->m_strChangeFullname);
			}
			
		}
		else
		{
			// If m_curAuthUsername has a value, test to see if it or
			// or m_curNormalUsername should be used to set username,
			// if username was not set to something above
			if (!m_pApp->m_curAuthUsername.IsEmpty() && username.IsEmpty())
			{
				if (m_pApp->m_curAuthUsername == m_pApp->m_strUserID)
				{
					// we can use m_curNormalUsername
					username = m_pApp->m_curNormalUsername;
				}
				else
				{
					username = m_pApp->m_curAuthUsername;
				}
			}
			// now user2
			user2 = m_pApp->m_Username2;
		}

		int result = 0;

		// Tell the app's status variables what page we are on -- DoEntireKbDeletion
		// may need this, if a KB deletion is later asked for
		m_pApp->m_bKbPageIsCurrent = FALSE;

		if (!username.IsEmpty() && !password.IsEmpty())
		{
			// LoadDataForPage has to look at list_users_return_results.dat file, 
			// to construct the list; and for a 2nd or subsequent run, check the
			// input .dat file has correct values
			/* BEW 14Dec20 removed, 
			// I couldn't get access to the textctrl - but SOMEWHERE 1st run does
			// get access and shows it, its in SharedKBManagerNotebookFunc2
			wxString shownIpAddr = m_pTheConnectedIpAddr->GetValue();
			if (shownIpAddr.IsEmpty())
			{
				m_pTheConnectedIpAddr->ChangeValue(m_pApp->m_strKbServerIpAddr);
			}
			*/
			result = m_pKbServer->ListUsers(ipAddr,username,password,user2);
			if (result == 0)
			{
				// BEW 14Nov20, the ListUsers() call will have populated an
				// Adapt_It.h class member function called m_arrLines(), with
				// the lines returned from a "success" run of do_list_users.exe,
				// calling app's member DatFile2StringArray(execPath, resultFile, m_arrLines).
				// (KbServer.cpp also has an identical DatFile2StringArray() function,
				// from which the app one was cloned. I need both. Bit ugly, but easier.

				// Since app's m_arrLines wxArrayString is populated with comma-separated
				// lines from the do_list_user.exe functions returned list_users_return_results.dat
				// file, I can now hook up to the legacy code by calling the function
				// ConvertLinesToUserStructs() here
				if (m_pUsersListForeign != NULL)
				{
					if (!m_pUsersListForeign->IsEmpty())
					{
						m_pKbServer->ClearUsersListForeign(m_pUsersListForeign);
						// Now it's empty and ready for new set of foreign user structs
					}
				}
				m_pKbServer->ConvertLinesToUserStructs(m_pApp->m_arrLines, m_pUsersListForeign);

				#if defined (_DEBUG)
				wxLogDebug(_T("KBSharingMgrTabbedDlg::LoadDataForPage(): line = %d:  number of user struct lines = %d"),
					__LINE__, m_pUsersListForeign->GetCount());
				#endif
				// Copy the list, before user gets a chance to modify anything
				// param 1 is src list, param2 is dest list
				CopyUsersList(m_pUsersListForeign, m_pOriginalUsersList); 

                // Load the username strings into m_pUsersListBox, the ListBox is sorted;
                // Note: the client data for each username string added to the list box is
                // the ptr to the KbServerUser struct itself which supplied the username
                // string, the struct coming from m_pUsersList

				LoadUsersListBox(m_pUsersListBox, m_pUsersListForeign->size(), m_pUsersListForeign);
			}
			else
			{
				// If there was an ipAddress error, it will have been generated from within
				// ListUsers() already, so that will suffice
				;
			}
		}
		else
		{
			// Don't expect this error, so an English message will do
			wxString msg = _T("LoadDataForPage() unable to call ListUsers() because password or username is empty, or both. The Manager won't work until this is fixed.");
			wxMessageBox(msg, _T("KB Sharing Manager error"), wxICON_WARNING | wxOK);
			m_pApp->LogUserAction(msg);
		}
	}
}

// Return TRUE if at least one KB definition differs from what it was earlier, or if the
// order is different (that indicates there was a change somewhere because the list is
// sorted), else return FALSE if nothing got changed and order is unchanged
bool KBSharingMgrTabbedDlg::IsAKbDefinitionAltered(wxArrayString* pBeforeArr, wxArrayString* pAfterArr)
{
	unsigned int count = pBeforeArr->GetCount();
	size_t index;
	for (index = 0; index < (size_t)count; index++)
	{
		wxString before = pBeforeArr->Item(index);
		wxString after = pAfterArr->Item(index);
		if (before != after)
		{
			return TRUE;
		}
	}
	return FALSE; // no change was found in content or order
}

/* BEW 5Sep20 removed
void KBSharingMgrTabbedDlg::DisplayRFC5646Message()
{
	// Display the RFC5646message.htm file in the platform's web browser
	// The "RFC5646message.htm" file should go into the m_helpInstallPath
	// for each platform, which is determined by the GetDefaultPathForHelpFiles() call.
	wxString helpFilePath = m_pApp->GetDefaultPathForHelpFiles() + m_pApp->PathSeparator + m_pApp->m_rfc5646MessageFileName;

	bool bSuccess = TRUE;

	wxLogNull nogNo;
	bSuccess = wxLaunchDefaultBrowser(helpFilePath,wxBROWSER_NEW_WINDOW); // result of
				// using wxBROWSER_NEW_WINDOW depends on browser's settings for tabs, etc.

	if (!bSuccess)
	{
		wxString msg = _(
		"Could not launch the default browser to open the HTML file's URL at:\n\n%s\n\nYou may need to set your system's settings to open the .htm file type in your default browser.\n\nDo you want Adapt It to show the Help file in its own HTML viewer window instead?");
		msg = msg.Format(msg, helpFilePath.c_str());
		int response = wxMessageBox(msg,_("Browser launch error"),wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);
		m_pApp->LogUserAction(msg);
		if (response == wxYES)
		{
			wxString title = _("RFC 5646 Guidelines");
			m_pApp->m_pHtmlFileViewer = new CHtmlFileViewer(this,&title,&helpFilePath);
			m_pApp->m_pHtmlFileViewer->Show(TRUE);
			m_pApp->LogUserAction(_T("Launched RFC5646message.htm in HTML Viewer"));
		}
	}
	else
	{
		m_pApp->LogUserAction(_T("Launched RFC5646message.htm in browser"));
	}
}
*/

// Return the pointer to the struct - this should never fail, but we'll return NULL if it does
KbServerUserForeign* KBSharingMgrTabbedDlg::GetThisUsersStructPtr(wxString& username2, UsersListForeign* pUsersListForeign)
{
	wxASSERT(pUsersListForeign);
	KbServerUserForeign* pEntry = NULL;
	if (pUsersListForeign->IsEmpty())
		return (KbServerUserForeign*)NULL;
	UsersListForeign::iterator iter;
	for (iter = pUsersListForeign->begin(); iter != pUsersListForeign->end(); ++iter)
	{
		// Assign the KbServerUserForeign struct's pointer to pEntry
		pEntry = *iter;
		if (pEntry->username == username2)
		{
			// We found the struct, so return it
			return pEntry;
		}
	}
	// control should not get here, but if it does, return NULL
	wxString msg = _T("KB Sharing Manager: GetThisUsersStructPtr() returned NULL; this should never happen.");
	m_pApp->LogUserAction(msg);
	return (KbServerUserForeign*)NULL;
}
/* BEW 5Sep20 removed
// Return the pointer to the struct - this should never fail, but we'll return NULL if it does
KbServerLanguage* KBSharingMgrTabbedDlg::GetThisLanguageStructPtr(wxString& customCode, LanguagesList* pLanguagesList)
{
	wxASSERT(pLanguagesList);
	KbServerLanguage* pEntry = NULL;
	if (pLanguagesList->empty())
		return (KbServerLanguage*)NULL;
	LanguagesList::iterator iter;
	for (iter = pLanguagesList->begin(); iter != pLanguagesList->end(); ++iter)
	{
		// Assign the KbServerLanguage struct's pointer to pEntry
		pEntry = *iter;
		if (pEntry->code == customCode)
		{
			// We found the struct, so return it
			return pEntry;
		}
	}
	// control should not get here, but if it does, return NULL
	wxString msg = _T("KB Sharing Manager: GetThisLanguageStructPtr() returned NULL; this should never happen.");
	m_pApp->LogUserAction(msg);
	return (KbServerLanguage*)NULL;
}
*/
// Returns nothing. The ListBox is in sorted order by the usernames, usinf wxSortedArrayString,
// and the contributing KbServerUserForeign struct's point is added to the relevant user line
// in the list, as it's client data (from it we can get useradmin etc)
// BEW 28Aug20  updated for Leon's code
void KBSharingMgrTabbedDlg::LoadUsersListBox(wxListBox* pListBox, size_t count, UsersListForeign* pUsersListForeign)
{
	wxASSERT(count != 0); wxASSERT(pListBox); wxASSERT(pUsersListForeign);
	if (pUsersListForeign->empty())
		return;

	// Get the usernames into sorted order, by adding them to a wxSortedArrayString
	wxSortedArrayString sorted_arrUsernames;

	UsersListForeign::iterator iter;
	wxString username;
	for (iter = pUsersListForeign->begin(); iter != pUsersListForeign->end(); ++iter)
	{
		KbServerUserForeign* pEntry = *iter;
		username = pEntry->username;
		sorted_arrUsernames.Add(username);
	}
	// Now match the original struct to the username and store the struct in
	// the list's client data member
	int index;
	for (index = 0; index < (int)count; index++)
	{
		wxString username2 = sorted_arrUsernames.Item(index);
		KbServerUserForeign* pClientData = GetThisUsersStructPtr(username2, pUsersListForeign);
		wxASSERT(pClientData != NULL);
		pListBox->Append(username2, pClientData);
	}
}

void KBSharingMgrTabbedDlg::OnButtonUserPageChangePermission(wxCommandEvent& WXUNUSED(event))
{
	bool bReady = m_pApp->ConfigureDATfile(change_permission); // arg is const int, value 10
	if (bReady)
	{
		// The input .dat file is now set up ready for do_change_permission.exe
		wxString execFileName = _T("do_change_permission.exe"); 
		wxString execPath = m_pApp->execPath;
		wxString resultFile = _T("change_permission_return_results.dat");
		bool bExecutedOK = m_pApp->CallExecute(list_users, execFileName, execPath, resultFile, 99, 99);
		if (!bExecutedOK)
		{
			// error in the call, inform user, and put entry in LogUserAction() - English will do
			wxString msg = _T("Line %d: CallExecute for enum: change_permission, failed - perhaps input parameters (and/or password) did not match any entry in the user table; Adapt It will continue working ");
			msg = msg.Format(msg, __LINE__);
			wxString title = _T("Probable do_change_permission.exe error");
			wxMessageBox(msg, title, wxICON_WARNING | wxOK);
			m_pApp->LogUserAction(msg);
		}
	}

	// At this point, the user table is altered, so it just remains to
	// call LoadDataForPage(int pageNumSelected)
	LoadDataForPage(0);
}

void KBSharingMgrTabbedDlg::OnButtonUserPageChangeFullname(wxCommandEvent& WXUNUSED(event))
{
	// Get the new fullname value, not the one in the m_pUserStructForeign - the latter is old one
	wxString newFullname = m_pEditInformalUsername->GetValue();
	m_pApp->m_bDoingChangeFullname = TRUE;
	m_pApp->m_strChangeFullname = newFullname; // make sure app knows it
	m_pApp->m_strChangeFullname_User2 = m_pApp->m_Username2; // so LoadDataForPage(0) can grab it

	bool bReady = m_pApp->ConfigureDATfile(change_fullname); // arg is const int, value 11
	if (bReady)
	{
		// The input .dat file is now set up ready for do_change_fullname.exe
		wxString execFileName = _T("do_change_fullname.exe");
		wxString execPath = m_pApp->execPath;
		wxString resultFile = _T("change_fullname_return_results.dat");


		bool bExecutedOK = FALSE;
		bExecutedOK = m_pApp->CallExecute(change_fullname, execFileName, execPath, resultFile, 99, 99);
		if (!bExecutedOK)
		{
			// error in the call, inform user, and put entry in LogUserAction() - English will do
			wxString msg = _T("Line %d: CallExecute for enum: change_fullname, failed - perhaps input parameters (and/or password) did not match any entry in the user table; Adapt It will continue working ");
			msg = msg.Format(msg, __LINE__);
			wxString title = _T("Probable do_change_fullname.exe error");
			wxMessageBox(msg, title, wxICON_WARNING | wxOK);
			m_pApp->LogUserAction(msg);
			m_pApp->m_bDoingChangeFullname = FALSE; // restore default value
		}
	}

	// At this point, the user table is altered, so it just remains to
	// call LoadDataForPage(int pageNumSelected)
	LoadDataForPage(0);

	if (m_pApp->m_bDoingChangeFullname)
	{
		// Turn it off, to default FALSE
		m_pApp->m_bDoingChangeFullname = FALSE;
		// And clear these, until such time as another change of fullname is done
		m_pApp->m_strChangeFullname.Clear();
		m_pApp->m_strChangeFullname_User2.Clear();
	}
}

/*
// The wxDesigner listbox is not sorted. Any sorting we'll take care of ourselves, prior to populating the list
// We do the filtering earlier than here (at the JSON decode step in ListLanguages()) & here we put the
// customCode strings into a wxSortedArrayString, and then load the list box from that, using the 
// GetThisLanguageStructPtr() function to grab the correct KbServerLanguage* to load as client
// data into the list row being constructed
void KBSharingMgrTabbedDlg::LoadLanguagesListBox(wxListBox* pListBox, LanguagesList* pLanguagesList)
{
	wxASSERT(pListBox); wxASSERT(pLanguagesList);
	if (pLanguagesList == NULL || pLanguagesList->empty())
		return;

	// First, filter out any ISO639 codes (we've not filtered in the SQL, so all have been
	// sent), and put a copy of each custom code in a wxSortedArrayString, to to establish
	// an alphabetical ascending order of the codes to be displayed in the list box
	wxSortedArrayString sorted_arrCustomCodes;
	LanguagesList::iterator iter;
	LanguagesList::compatibility_iterator c_iter;
	int anIndex = -1;
	for (iter = pLanguagesList->begin(); iter != pLanguagesList->end(); ++iter)
	{
		anIndex++;
		c_iter = pLanguagesList->Item((size_t)anIndex);
		KbServerLanguage* pEntry = c_iter->GetData();
		if (pEntry != NULL)
		{
			if (pEntry->code.Find(_T("-x-")) != wxNOT_FOUND)
			{
				// It's a custom code, so retain it -- this one will get displayed in the list
				sorted_arrCustomCodes.Add(pEntry->code);
			}
			else
			{
				// It's a non-editable ISO639 code,, so throw it away - we don't display it
				delete pEntry; // frees its memory block
			}
		}
	}
	// Now iterate over all the sorted custom code values, adding them to the list box's rows,
	// and for each, storing its originating ptr to KbServerLanguage struct as client data
	size_t count = sorted_arrCustomCodes.Count();
	if (count == 0)
		return; // there are no custom codes in the list as yet
	size_t i;
	KbServerLanguage* pLanguage = NULL; // initialize
	wxString customCode;
	for (i = 0; i < count; i++)
	{
		customCode = sorted_arrCustomCodes.Item(i);
		pLanguage = GetThisLanguageStructPtr(customCode, pLanguagesList);
		if (pLanguage != NULL)
		{
			pListBox->Append(customCode, pLanguage);
		}
	}
}
*/
KbServerUserForeign* KBSharingMgrTabbedDlg::GetUserStructFromList(UsersListForeign* pUsersListForeign, size_t index)
{
	wxASSERT(!pUsersListForeign->empty());
	KbServerUserForeign* pStruct = NULL;
	UsersListForeign::compatibility_iterator c_iter;
	c_iter = pUsersListForeign->Item(index);
	wxASSERT(c_iter);
	pStruct = c_iter->GetData();
	return pStruct;
}
/* BEW 28Aug20 deprecated, not needed any more
KbServerLanguage* KBSharingMgrTabbedDlg::GetLanguageStructFromList(LanguagesList* pLanguagesList, size_t index)
{
	wxASSERT(!pLanguagesList->empty());
	KbServerLanguage* pStruct = NULL;
	LanguagesList::compatibility_iterator c_iter;
	c_iter = pLanguagesList->Item(index);
	wxASSERT(c_iter);
	pStruct = c_iter->GetData();
	return pStruct;
}
*/
// Make deep copies of the KbServerUser struct pointers in pSrcList and save them to pDestList
// BEW 14Nov20 updated for Leon's solution
void KBSharingMgrTabbedDlg::CopyUsersList(UsersListForeign* pSrcList, UsersListForeign* pDestList)
{
	if (pSrcList->empty())
		return;
	UsersListForeign::iterator iter;
	UsersListForeign::compatibility_iterator c_iter;
	int anIndex = -1;
	for (iter = pSrcList->begin(); iter != pSrcList->end(); ++iter)
	{
		anIndex++;
		c_iter = pSrcList->Item((size_t)anIndex);
		KbServerUserForeign* pEntry = c_iter->GetData();
		KbServerUserForeign* pNew = new KbServerUserForeign;
		//pNew->id = pEntry->id;
		pNew->username = pEntry->username;
		pNew->fullname = pEntry->fullname;
		pNew->password = pEntry->password;
		pNew->useradmin = pEntry->useradmin;
		//pNew->timestamp = pEntry->timestamp;
		pDestList->Append(pNew);
	}
}

// Make deep copies of the KbServerKb struct pointers in pSrcList and save 
// them to pDestList
// BEW 31Aug20 updated for Leon's solution
void KBSharingMgrTabbedDlg::CopyKbsList(KbsList* pSrcList, KbsList* pDestList)
{
	if (pSrcList->empty())
		return;
	KbsList::iterator iter;
	KbsList::compatibility_iterator c_iter;
	int anIndex = -1;
	for (iter = pSrcList->begin(); iter != pSrcList->end(); ++iter)
	{
		anIndex++;
		c_iter = pSrcList->Item((size_t)anIndex);
		KbServerKb* pEntry = c_iter->GetData();
		KbServerKb* pNew = new KbServerKb; // we make deep copies on the heap
		//pNew->id = pEntry->id;
		pNew->sourceLanguageName = pEntry->sourceLanguageName;
		pNew->targetLanguageName = pEntry->targetLanguageName;
		pNew->kbType = pEntry->kbType;
		pNew->username = pEntry->username;
		//pNew->timestamp = pEntry->timestamp;
		//pNew->deleted = pEntry->deleted;
		pDestList->Append(pNew);
	}
}

void KBSharingMgrTabbedDlg::SeparateKbServerKbStructsByType(KbsList* pAllKbStructsList,
								KbsList* pKbStructs_TgtList, KbsList* pKbStructs_GlsList)
{
	// For params 2 and 3, pass in members m_pKbsList_Tgt and m_pKbsList_Gls; for param 1,
	// pass in the list of KbServerKb structs obtained from the ListKbs() call of the
	// 'foreign' KbServer (temporary) instance
	//m_pKbServer->ClearKbsList(pKbStructs_TgtList);
	//m_pKbServer->ClearKbsList(pKbStructs_GlsList);
	if (pAllKbStructsList->empty())
		return;
	KbsList::iterator iter;
	KbsList::compatibility_iterator c_iter;
	int anIndex = -1;
	for (iter = pAllKbStructsList->begin(); iter != pAllKbStructsList->end(); ++iter)
	{
		anIndex++;
		c_iter = pAllKbStructsList->Item((size_t)anIndex);
		KbServerKb* pEntry = c_iter->GetData();
		// just store ptr copies, that's enough for populating the list box
		if (pEntry->kbType == (int)1)
		{
			pKbStructs_TgtList->Append(pEntry);
		}
		else
		{
			pKbStructs_GlsList->Append(pEntry);
		}
	}
	// Note, we cannot assume everyone will create a glossing KB definition to match each
	// created adapting KB definition. For instance, Jonathan didn't. If a glossing one is
	// lacking, then glosses in that project's glossing mode won't get shared, but nothing
	// will break
}

void KBSharingMgrTabbedDlg::OnTabPageChanged(wxNotebookEvent& event)
{
	// OnTabPageChanged is called whenever any tab is selected
	int pageNumSelected = event.GetSelection();
	if (pageNumSelected == m_nCurPage)
	{
		// user selected same page, so just return
		return;
	}
	// If we get to here user selected a different page
	m_nCurPage = pageNumSelected;

	// We want each entry to the kbs page to have the adapting radio button preselected to
	// be ON, rather than the old button-pair's selection to be remembered, so check here
	// and reset the adapting one to be on if the glossing one is currently on
	if (m_nCurPage == 1)
	{
		// We are entering the kbs page
		bool bRadioBtnAdaptingIsOn = m_pRadioKBType1->GetValue();
		if (!bRadioBtnAdaptingIsOn)
		{
			m_pRadioKBType2->SetValue(FALSE);
			m_pRadioKBType1->SetValue(TRUE);
			m_bKBisType1 = TRUE;
		}
	}

	//Set up new page data by populating list boxes and controls
	LoadDataForPage(m_nCurPage);
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void KBSharingMgrTabbedDlg::OnOK(wxCommandEvent& event)
{
	// Tidy up
	m_pKbServer->ClearUsersListForeign(m_pOriginalUsersList); // this one is local to this
	delete m_pOriginalUsersList;
	m_pKbServer->ClearUsersListForeign(m_pUsersListForeign); // this one is in the stateless
									// KbServer instance & don't delete this one
	DeleteClonedKbServerUserStruct();

	// Delete the KbServer instance we've been using, (it will be the app's
	// m_pKbServer_ForManager instance, a stateless one for authentications, and most
	// of the Manager's functionalities) & then set the ptr to NULL
	delete m_pKbServer;
	m_pApp->m_pKbServer_ForManager = NULL; // we could use m_pKbServer = NULL, but
				// this way it reminds the app maintainer what is happening here
	m_pApp->m_pKbServer_Persistent = NULL;

	m_pApp->m_pKBSharingMgrTabbedDlg = (KBSharingMgrTabbedDlg*)NULL;
	m_pApp->m_bKBSharingMgrEntered = FALSE; // reset default
	m_pApp->m_bHasUseradminPermission = FALSE; // reset default
	m_pApp->m_bUseForeignOption = FALSE; // reset default
	m_pApp->m_bWithinMgr = FALSE; // control is no longer within the Mgr

	event.Skip();
	// Remember, the Manager is closing, but there may still be a running detached
	// thread working to delete a kb definition from the kb table in KBserver (it first
	// has to delete all entries for this definition from the entry table before the
	// kb definition can itself be removed); and the KbServer instance supplying 
	// resources for that deletion task is the app's m_pKbServer_Persistent instance
}

void KBSharingMgrTabbedDlg::OnCancel(wxCommandEvent& event)
{
	// Tidy up
	m_pKbServer->ClearUsersListForeign(m_pOriginalUsersList); // this one is local to ptr 'this'
	delete m_pOriginalUsersList;
	m_pKbServer->ClearUsersListForeign(m_pUsersListForeign); // this one is in the adaptations
									// KbServer instance & don't delete this one
	DeleteClonedKbServerUserStruct();

	// Delete the stateless KbServer instance we've been using
	delete m_pKbServer;
	m_pApp->m_pKbServer_ForManager = NULL; // see OnOK() for additional 
										   // explatory comments that apply here
	m_pApp->m_pKBSharingMgrTabbedDlg = (KBSharingMgrTabbedDlg*)NULL;
	m_pApp->m_bKBSharingMgrEntered = FALSE; // reset default
	m_pApp->m_bHasUseradminPermission = FALSE; // reset default
	m_pApp->m_bUseForeignOption = FALSE; // reset default
	m_pApp->m_bWithinMgr = FALSE; // control is no longer within the Mgr

	event.Skip();
}

// BEW 29Aug20 updated
void KBSharingMgrTabbedDlg::OnButtonUserPageClearControls(wxCommandEvent& WXUNUSED(event))
{
	wxString emptyStr = _T("");
	m_pUsersListBox->SetSelection(wxNOT_FOUND);
	m_pTheUsername->ChangeValue(emptyStr);
	m_pEditInformalUsername->ChangeValue(emptyStr);
	m_pEditPersonalPassword->ChangeValue(emptyStr);
	m_pEditPasswordTwo->ChangeValue(emptyStr);
	m_pCheckUserAdmin->SetValue(FALSE);
//	m_pCheckKbAdmin->SetValue(FALSE);
}

void KBSharingMgrTabbedDlg::OnButtonKbsPageClearListSelection(wxCommandEvent& WXUNUSED(event))
{
	m_pKbsListBox->SetSelection(wxNOT_FOUND);
	// also clear the "Original Creator:" read-only text box
	wxString emptyStr = _T("");
	m_pKbDefinitionCreator->ChangeValue(emptyStr);
	m_pBtnRemoveSelectedKBDefinition->Enable(FALSE); // Enable it by selecting a kb definition
}
/*
void KBSharingMgrTabbedDlg::OnButtonLanguagesPageClearListSelection(wxCommandEvent& WXUNUSED(event))
{
	m_pCustomCodesListBox->SetSelection(wxNOT_FOUND);
	// also clear the "Original Creator:" read-only text box
	wxString emptyStr = _T("");
	m_pCustomCodeDefinitionCreator->ChangeValue(emptyStr);
}
*/
void KBSharingMgrTabbedDlg::OnButtonKbsPageClearBoxes(wxCommandEvent& WXUNUSED(event))
{
	wxString emptyStr = _T("");
	m_pEditSourceCode->ChangeValue(emptyStr);
	m_pEditNonSourceCode->ChangeValue(emptyStr);
}
/*
void KBSharingMgrTabbedDlg::OnButtonLanguagesPageClearBoxes(wxCommandEvent& WXUNUSED(event))
{
	wxString emptyStr = _T("");
	m_pCustomCodesListBox->SetSelection(wxNOT_FOUND);
	m_pEditCustomCode->ChangeValue(emptyStr);
	m_pEditDescription->ChangeValue(emptyStr);
	m_pCustomCodeDefinitionCreator->ChangeValue(emptyStr);
}
*/
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

void KBSharingMgrTabbedDlg::OnButtonShowPassword(wxCommandEvent& WXUNUSED(event))
{
	ClearCurPwdBox();
	// Sanity checks
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
	// The GetSelection() call returns -1 if there is no selection current, so check for
	// this and return (with a beep) if that's what got returned
	m_nSel = m_pUsersListBox->GetSelection();
	if (m_nSel == wxNOT_FOUND)
	{
		wxBell();
		return;
	}
	// now get the user struct, which has the pwd, for the selected user
	m_pUserStructForeign = (KbServerUserForeign*)m_pUsersListBox->GetClientData(m_nSel);
	if (m_pUserStructForeign != NULL)
	{
		// get the password
		wxString password = m_pUserStructForeign->password;
		// show it in the box
		m_pEditShowPasswordBox->ChangeValue(password);
	}
}
void KBSharingMgrTabbedDlg::ClearCurPwdBox()
{
	m_pEditShowPasswordBox->Clear();
}

// BEW 29Aug20 updated -- TODO, legacy code commented out
void KBSharingMgrTabbedDlg::OnButtonUserPageAddUser(wxCommandEvent& WXUNUSED(event))
{
	m_pApp->m_bUseForeignOption = TRUE;

	// mediating AI.cpp wxString variables to be set: m_temp_username, m_temp_fullname,
	// m_temp_password, m_temp_useradmin_flag. Set these from within this handler.
	// Then our ConfigureMovedDatFile() can grab it's needed data from these.

	// Legacy comments - still relevant...
	// Username box with a value, informal username box with a value, password box with a
	// value, and the checkbox for useradmin flag, with or without a tick, are mandatory. 
	// Test for these and if one is not set, abort the button press
	// and inform the user why. Also, a selection in the list is irrelevant, and it may
	// mean that the logged in person is about to try to add the selected user a second
	// time - which is illegal, so test for this and if so, warn, and remove the
	// selection, and clear the controls & return. In fact, ensure no duplication of username
	wxString strUsername = m_pTheUsername->GetValue();
	wxString strFullname = m_pEditInformalUsername->GetValue();
	wxString strPassword = m_pEditPersonalPassword->GetValue();
	wxString strPasswordTwo = m_pEditPasswordTwo->GetValue();
	bool bUseradmin = m_pCheckUserAdmin->GetValue();
	wxString strUseradmin = bUseradmin ? _T("1") : _T("0");

	// First, test all the textboxes that should have a value in them, actually have something
	if (strUsername.IsEmpty() || strFullname.IsEmpty() || strPassword.IsEmpty() || strPasswordTwo.IsEmpty())
	{
		wxString msg = _("One or more of the text boxes: Username, Informal username, or one or both password boxes, are empty.\nEach of these must have appropriate text typed into them before an Add User request will be honoured. Do so now.\nIf you want this new user to have the privilege to add other users, tick the checkbox also.");
		wxString title = _("Warning: Incomplete user definition");
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		return;
	}
	else
	{
        // BEW 27Jan16 refactored, to test strUsername against *all* currently listed
        // usernames, to ensure this is not an attempt to create a duplicate - that's a
        // fatal error
		int count = m_pUsersListBox->GetCount();
		int index;
		for (index = 0; index < count; index++)
		{
			m_pUsersListBox->SetSelection(index);
			wxString selectedUser = m_pUsersListBox->GetString(index);
			if (selectedUser == strUsername)
			{
				// Oops, he's trying to add someone who is already there. Warn, and probably
				// best to clear the controls to force him to start over
				wxBell();
				wxString title = _("Warning: this user already exists");
				wxString msg = _(
				"You are trying to add a username which already exists in the user table. This is illegal.");
				wxMessageBox(msg, title, wxICON_WARNING | wxOK);
				wxCommandEvent dummy;
				OnButtonUserPageClearControls(dummy);
				return;
			}
		}

		// Test the passwords match in the two boxes for them,(if they don't match, 
		// or a box is empty, then the call will show an exclamation message from 
		// within it, and clear both password text boxes)
		bool bMatchedPasswords = CheckThatPasswordsMatch(strPassword, strPasswordTwo);
		if (!bMatchedPasswords)
		{
			// Unmatched passwords, the user has been told to try again
			return;
		}

		// Create the new entry in the KBserver's user table
		int result = -1; // initialize

		// First, copy the strings needed to the temp variables above
		m_pApp->m_temp_username = strUsername;
		m_pApp->m_temp_fullname = strFullname;
		m_pApp->m_temp_password = strPassword;
		m_pApp->m_temp_useradmin_flag = strUseradmin;
		wxString ipAddr = m_pApp->m_chosenIpAddr;
		wxString datFilename = _T("credentials_for_user.dat");

		bool bCredsOK = Credentials_For_User(&ipAddr, &strUsername, &strFullname,
			&strPassword, bUseradmin, datFilename);
		if (bCredsOK)
		{
			bool bOK = m_pApp->ConfigureDATfile(credentials_for_user);
			if (bOK)
			{
				wxString execFileName = _T("do_add_KBUsers.exe");
				wxString resultFile = _T("credentials_for_user_return_results.dat");
				result = m_pApp->CallExecute(credentials_for_user, execFileName,
						m_pApp->m_curExecPath, resultFile, 99, 99);
			}
		}
		// Update the page if we had success, if no success, just clear the controls
		if (result == 0)
		{
			LoadDataForPage(0);
		}
		else
		{
			// The creation did not succeed -- an error message will have been shown
			// from within the above CreateUser() call
			wxString msg = _T("KB Sharing Manager, line %d: OnButtonUserPageAddUser() failed.");
			msg = msg.Format(msg, __LINE__);
			m_pApp->LogUserAction(msg);
			wxCommandEvent dummy;
			OnButtonUserPageClearControls(dummy);
		}
	}
	DeleteClonedKbServerUserStruct(); // ***** THIS CAN BE REMOVED - no longer need it

	// restore default for following flag
	m_pApp->m_bUseForeignOption = FALSE;
}

void KBSharingMgrTabbedDlg::OnButtonKbsPageAddKBDefinition(wxCommandEvent& WXUNUSED(event))
{
    // Source language code box with a value, target or gloss language code box with a
    // value, are mandatory. Test for these and if one is not set, abort the button press
    // and inform the user why. Also, a selection in the list is irrelevant, and it may
    // mean that the logged in person is about to try to add the selected kb definition a
    // second time - which is illegal, so test for this and if so, warn, and remove the
    // selection, and clear the controls & return
	m_nSel =  m_pKbsListBox->GetSelection(); // get selection from list box (note:
				// there might be no selection in the list, which would be normal; we only
				// need the two wxTextCtrl contents for the CreateKb() call; we are just
				// checking for administrator or user confusion here...)
	wxString textCtrl_SrcLangName = m_pEditSourceCode->GetValue(); // source lang name wxTextCtrl
	wxString textCtrl_NonSrcLangName = m_pEditNonSourceCode->GetValue(); // non-source lang name wxTextCtrl

	// The first test is to make sure the administrator is not trying to re-create a
	// shared kb which is currently in the process of being removed. The app's 
	// m_pKbServer_Persistent (which supports the current deletion thread) stores
	// values for src code, non-src code and kbType on it. Check something differs. 
	// The flag: bool m_bKbSvrMgr_DeleteAllIsInProgress will be TRUE if such a removal
	// is going on.
	if (m_pApp->m_bKbSvrMgr_DeleteAllIsInProgress)
	{
		// A removal is happening. Check the proposed new kb definition addition has at
		// least one code different, or a different kbType, than what is being removed.
		int kbTypeInManagerDlg = m_bKBisType1 ? 1 : 2;
		wxASSERT(m_pApp->m_pKbServer_Persistent != NULL);
		if (m_pApp->m_pKbServer_Persistent->GetKBServerType() == kbTypeInManagerDlg)
		{
			// The type to be added is of the same type as the one being removed, so check
			// further - see if there is identity in the source, and non-source, language
			// codes?
			if (textCtrl_SrcLangName == m_pApp->m_pKbServer_Persistent->GetSourceLanguageCode()) // ************
			{
				// source lang code of the pair chosen for removal is the same as the
				// source lang code of the pair about to be added - so there's still the
				// potential for a clash - so check further
				if (textCtrl_NonSrcLangName == m_pApp->m_pKbServer_Persistent->GetTargetLanguageCode()) // ********
				{
					// ALARM!! User of the Manager GUI is attempting to re-create the
					// kb definition of a remote kb database currently in the process of
					// being removed. Prevent this, and warn accordingly.
					wxBell();
					wxString title = _("Warning: Adding what is being removed");
					wxString msg = _("You are trying to add a shared database to the server, but that database is currently being removed from the same server.\nThis is illegal. The removal must be allowed to run to completion.\nIt is unlikely that you should attempt to add the database back to this server - because someone has decided this database is no longer needed.\nAsk for clarification from the server administrator.");
					wxMessageBox(msg, title, wxICON_WARNING | wxOK);
					wxCommandEvent dummy;
					OnButtonKbsPageClearListSelection(dummy);
					OnButtonKbsPageClearBoxes(dummy);
					return;
				}
			}
		}
	}
	// If control gets to here, the new pair of codes may be valid for creating a new definition -
	// so continue checks etc...
	// First check the two editboxes' contents don't match any of the listed code pairs,
	// (i.e. one of the src code - non-src code pairs) -- if they do match, disallow & warn user
	bool bPairMatchesAListedKB = MatchExistingKBDefinition(m_pKbsListBox, textCtrl_SrcLangName, 
															textCtrl_NonSrcLangName);
	if (bPairMatchesAListedKB)
	{
		// Oops, he's trying to add a KB which is already there. Warn, and probably
		// best to clear the controls to force him to start over
		wxBell();
		wxString title = _("Already exists");
		wxString msg = _("Warning: you are trying to add a shared database which already exists in the server. This is illegal, each pair of codes must be unique.\nTo add a new definition, one or both of the codes in the text entry boxes must be different in some way from every listed language name pair.\nEditing of language names in existing knowledge base definitions is illegal.\nTo change a knowledge base definition, first remove the old definition, then create a new one with corrected language names, then click the Add Definition button.)");
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		wxCommandEvent dummy;
		OnButtonKbsPageClearListSelection(dummy);
		OnButtonKbsPageClearBoxes(dummy);
		return;
	}
	// Clear the listbox selection, it can play no further part 
	if (m_nSel != wxNOT_FOUND)
	{
		m_pKbsListBox->SetSelection(wxNOT_FOUND);
	}

	// Test that neither wxTextCtrl has an empty string in it, and that each string is at
	// least 2 chars in length
	int srcCodeLength = textCtrl_SrcLangName.Len();
	int nonsrcCodeLength = textCtrl_NonSrcLangName.Len();
	bool bSrcLangNameExistsInDB = FALSE;
	bool bNonSrcLangNameExistsInDB = FALSE;
	if ((srcCodeLength < 2) || (nonsrcCodeLength < 2))
	{
		wxString msg;
		msg = _("One or more of the two text boxes contain too short a name, or no name.\nEach box must have a language name before the create request will be honoured. Fix that now, then try again.");
		wxString title = _("Warning: Language name too short or empty");
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		return; // back to parent dialog so user can edit the deviant name or names
	}
	else
	{
		// Create the new KB definition in the KBserver's kb table.

		wxString username = m_pKbServer->GetKBServerUsername(); // for authentication
		wxString password = m_pKbServer->GetKBServerPassword(); // for authentication
		wxString ipAddr = m_pKbServer->GetKBServerIpAddr(); // for authentication to this server
		int result = 0;
		bSrcLangNameExistsInDB = FALSE;
		bNonSrcLangNameExistsInDB = FALSE;
/* no longer relevant
		if (!username.IsEmpty() && !password.IsEmpty())
		{
			// Check first for the source language code being already present in the language table
			result = (CURLcode)m_pKbServer->ReadLanguage(url,  username, password, textCtrl_SrcLangCode);
			if (result == CURLE_OK)
			{
				// Check that the passed in language code is identical to what got returned in the JSON
				wxString thecode =  m_pKbServer->GetLanguageStruct().code; // RHS has just been filled out from the returned JSON
				if (thecode != textCtrl_SrcLangCode)
				{
					// Don't expect an inequality, so an English message will suffice
					wxString msg = _T("ReadLanguage() data error, in create KBs page, the returned source language code does not match the one passed in for the ReadLanguage() call. Fix this.");
					wxMessageBox(msg, _T("KB Sharing Manager error"), wxICON_WARNING | wxOK);
					m_pApp->LogUserAction(msg);
					return;
				}
				// The source language code exists in the language table. 
				// Check next for the non-source code also existing in the language table
				bSrcLangCodeExistsInDB = TRUE;
			}
			else
			{
				// Error, which we'll assume is a 404 error - in which case, this
				// language code needs to be created & stored in the language table,
				// so tell the user and return  (this message must be localizable)
				wxString aMsg = _("The source language code does not yet exist in the server's language database. First use the Create Or Delete Custom Language Codes page to create the needed code, then return to this page and try again.");
				// developers need an English message
				wxString aMsgEnglish = _T("The source language code does not yet exist in the server's language database. First use the Create Or Delete Custom Language Codes page to create the needed code, then return to this page and try again.");
				m_pApp->LogUserAction(aMsgEnglish);
				wxMessageBox(aMsg, _("KB Sharing Manager error"), wxICON_EXCLAMATION | wxOK);
				return;
			}
			// Now check for the non-source language code being already present in the language table
			m_pKbServer->ClearLanguageStruct();
			result = (CURLcode)m_pKbServer->ReadLanguage(url, username, password, textCtrl_NonSrcLangCode);
			if (result == CURLE_OK)
			{
				// Check that the passed in language code is identical to what got returned in the JSON
				wxString thecode = m_pKbServer->GetLanguageStruct().code; // RHS has just be filled out from the returned JSON
				if (thecode != textCtrl_NonSrcLangCode)
				{
					// Don't expect an inequality, so an English message will suffice
					wxString msg = _T("ReadLanguage() data error, in create KBs page, the returned non-source language code does not match the one passed in for the ReadLanguage() call. Fix this.");
					m_pApp->LogUserAction(msg);
					wxMessageBox(msg, _T("KB Sharing Manager error"), wxICON_WARNING | wxOK);
					return;
				}

				// The source language code exists in the language table. Check next for the non-source code also existing in the language table
				bNonSrcLangCodeExistsInDB = TRUE;
			}
			else
			{
				// Error, which we'll assume is a 404 error - in which case, this
				// language code needs to be created & stored in the language table,
				// so tell the user and return  (this message must be localizable)
				wxString aMsg = _("The adaptation (or gloss) language code does not yet exist in the server's language database. First use the Create Or Delete Custom Language Codes page to create the needed code, then return to this page and try again.");
				// developers need an English message
				wxString aMsgEnglish = _T("The adaptation (or gloss) language code does not yet exist in the server's language database. First use the Create Or Delete Custom Language Codes page to create the needed code, then return to this page and try again.");
				m_pApp->LogUserAction(aMsgEnglish);
				wxMessageBox(aMsg, _("KB Sharing Manager error"), wxICON_EXCLAMATION | wxOK);
				return;
			}
			
		}
		else
		{
			// Don't expect username or password to be empty, so an English message will suffice
			wxString msg = _T("ReadLanguage() authentication error, at create KBs page, either the password or username string was empty. Fix this.");
			m_pApp->LogUserAction(msg);
			wxMessageBox(msg, _T("ReadLanguage() athentication error"), wxICON_WARNING | wxOK);
			return;
		}
		m_pKbServer->ClearLanguageStruct();
*/
		// If control gets to here, we've verified the needed source and non-source language names 
		// are not in  the kb table already, so we can progress to the CreateKb() attempt. <- ********* true?
		if (bSrcLangNameExistsInDB && bNonSrcLangNameExistsInDB)
		{
			// temp credentials BEW 15Sep20 - next 3 lines, to get a clean compile
			wxString ipAddr = m_pApp->m_curIpAddr;
			wxString username = m_pApp->m_strUserID;
			wxString pwd = m_pApp->GetMainFrame()->GetKBSvrPassword();

//			result = (int)m_pKbServer->CreateKb(ipAddr, username, pwd,
//				textCtrl_SrcLangName, textCtrl_NonSrcLangName, m_bKBisType1);

			// Update the page if we had success, if no success, just clear the controls
			if (result == 0)
			{
				// Add a KbServerKb struct to m_pKbsAddedInSession so that we can test for
				// which ones have been added in the table, OnKbsPageRemoveKb() handler will
				// use this list
				KbServerKb* pAddedKbDef = new KbServerKb;
				pAddedKbDef->sourceLanguageName = textCtrl_SrcLangName;
				pAddedKbDef->targetLanguageName = textCtrl_NonSrcLangName;
				int itsType = m_bKBisType1 ? 1 : 2;
				pAddedKbDef->kbType = itsType;
				m_pKbsAddedInSession->Append(pAddedKbDef);

				// Update the page to show what has happened
				LoadDataForPage(m_nCurPage);
			}
			else
			{
				// The creation did not succeed -- an error message will have been shown
				// from within the above CreateKb() call
				wxString msg = _T("KB Sharing Manager: OnButtonKbsPageAddKBDefinition() failed at the CreateKb() call.");
				m_pApp->LogUserAction(msg);
				wxCommandEvent dummy;
				OnButtonKbsPageClearListSelection(dummy);
				OnButtonKbsPageClearBoxes(dummy);
			}
		}
		else
		{
			wxBell(); // control should never get here, so a bell will suffice
		}
	}
	//DeleteClonedKbServerKbStruct();
}
/*
void KBSharingMgrTabbedDlg::OnButtonLanguagesPageCreateCustomCode(wxCommandEvent& WXUNUSED(event))
{
	wxString code = m_pEditCustomCode->GetValue();
	wxString description = m_pEditDescription->GetValue();
	wxString strX = _T("-x-");
	wxString title = _("Error");
	wxString msg_code = _("The custom code text box must not be empty");
	wxString msg_desc = _("The description text box must not be empty");
	// Don't need one for the definition creator, since that is auto-filled from login username
	if (code.IsEmpty())
	{
        wxMessageBox(msg_code, title, wxICON_WARNING | wxOK);
        return;
	}
	if (description.IsEmpty())
	{
        wxMessageBox(msg_desc, title, wxICON_WARNING | wxOK);
        return;
	}
	// Test for substring "-x-" as it must be present for a valid custom definition;
	// then test that something precedes and something follows the -x-
	int offset = wxNOT_FOUND;
	offset = code.Find(strX);
	if (offset == wxNOT_FOUND)
	{
        // There is no substring "-x-" within the custom code, it's structure is invalid
        wxString msg_absent = _("The three characters -x- are not present, this custom code is invalid.");
        wxMessageBox(msg_absent, title, wxICON_WARNING | wxOK);
        return;
    }
    else
    {
        // Here test for at least a character after the -x- and at least two before it
        wxString strRight = code.Right(offset + 3L);
        if (offset < 2 || strRight.Len() < 1)
        {
            wxString msg_context =
            _("There must be at least two characters preceding -x- and at least one character following it, this custom code is invalid.");
            wxMessageBox(msg_context, title, wxICON_WARNING | wxOK);
            return;
        }
   }
    // The custom code is probably a valid one, so continue processing... The next check it
    // to ensure that it is not already listed in the custom codes list box
    KbServerLanguage* pLanguageStruct = NULL;
    wxString aCode;
    unsigned int count = m_pCustomCodesListBox->GetCount();
    unsigned int i;
    for (i = 0; i < count; i++)
    {
        pLanguageStruct = (KbServerLanguage*)m_pCustomCodesListBox->GetClientData(i);
        aCode = pLanguageStruct->code;
        if (code == aCode)
        {
            // There is a duplicate of this custom code, so this one is invalid
            wxString msg_dup =_("This custom code is already in the list. A duplicate cannot be accepted because each code must be unique.");
            wxMessageBox(msg_dup, title, wxICON_WARNING | wxOK);
            return;
        }
    }
	// If control gets to here, we can go ahead with creating the new custom code entry
	// in the remote server's language table. (We'll assume that no other user has sneeked a
	// competing identical custom code into the language table in the brief interval between
	// the populating of the wxListBox here, and the typing of the code and subsequent button
	// press here to have it added to the language table in the remote server.)
	//
	// Note: KB Sharing Manager uses a stateless setup of the KbServer class. That means that
	// the Manager's class instance's m_pKbServer instance points at a stateless instance, and that
	// the url, username and password stored within it are separate from any used by the user for
	// a KBserver access as stored in the project config file; so the following calls will not clobber
	// any of the user's authentication credentials. Any person with relevant permissions and valid
	// credentials can use the KB Sharing Manager from anyone's computer, with compromising the
	// integrity of the normal user's KBserver settings.
	wxString username = m_pKbServer->GetKBServerUsername(); // for authentication & same is used for
												// the creator of this particular custom language code
	wxString password = m_pKbServer->GetKBServerPassword(); // for authentication
	wxString url = m_pKbServer->GetKBServerIpAddr(); // for authentication to this server NOTE <<-- url
	CURLcode result = CURLE_OK;
	result = (CURLcode)m_pKbServer->CreateLanguage(url, username, password, code, description);
	if (result != CURLE_OK)
	{
		// Don't expect an error of this kind, but probably a good idea to make it localizable
		// but the developers need an English message
		wxString msg;
		msg = msg.Format(_T("Creating the custom code definition in the server failed, for custom language code: %s with description: %s and username: %s"),
			code.c_str(), description.c_str(), username.c_str());
		wxString msgEnglish;
		msgEnglish = msgEnglish.Format(_T("CreateLanguage() call failed, for custom language code: %s with description: %s and username: %s  curlCode: %d"),
			code.c_str(), description.c_str(), username.c_str(), result);
		m_pApp->LogUserAction(msgEnglish);
		wxMessageBox(msg, _T("KB Sharing Manager error"), wxICON_WARNING | wxOK);
		return;
	}
	else
	{
		// Success, re-populate the list, and clear the text controls; after first
		// clearing out the KbServerLanguage struct pointers being managed for the
		// last population of the list box.
		m_pKbServer->ClearLanguagesList(m_pKbServer->GetLanguagesList());
		LoadDataForPage(m_nCurPage);

		wxCommandEvent dummy;
		OnButtonLanguagesPageClearBoxes(dummy);
	}
}
*/
/*
void KBSharingMgrTabbedDlg::OnButtonLanguagesPageDeleteCustomCode(wxCommandEvent& WXUNUSED(event))
{
	m_pKbServer->ClearKbsList(m_pKbServer->GetKbsList()); // we need to re-get them below
	wxString code = m_pEditCustomCode->GetValue(); // the code we are trying to delete
	wxASSERT(!code.IsEmpty());
	wxArrayString kbDefsArray; // store in here all representations of any kb definitions
							   // that use the code intended for deletion
	wxString compositeDefsStr; // construct a list of using definitions here, in format [src:tgt],[src:tgt]...
	// Deleting a custom language code is not simply a matter of removing it from the list of 
	// language codes. It cannot be deleted if the code is part of the definition of one or
	// more shared knowledge base definitions - whether as the source language, or as the target
	// or gloss language. Therefore, all the shared kb definitions would have to be deleted first,
	// and each of those cannot be deleted without first deleting any kb entries stored in each
	// such shared kbs' entry tables. So a custom language code should be carefully defined, to
	// avoid having to try change it after a lot of user work has been done using it.
	wxArrayString kbDefsList; // populate with "[srccode:nonsrccode]" structured strings
	// Out approach here is just to get all the KbServerKb structs, and find as many as have the
	// code to be deleted in either their source language part of the definition, or the non-source
	// part of the definition, and count however many there are that are detected. Then we'll 
	// message the user to tell him to find those using the tabbed dialog's 2nd page, and delete
	// each first. They can be listed in format [srccode1:nonsrccode1],[srccode2:tgtcode2],etc...
	// Then he can come to the languages page and delete the unwanted or misspelled
	// language definition, and if it's a mispelling issue, after that he can recreate it with
	// a correct spelling. Of course, if no shared kb definition uses a given custom code, it
	// can be deleted immediately without any fuss.
	wxString title = _("Error");
	wxString msg = _("There was an error in the https transmission. Perhaps try again later.");
	// Next one for the developers... if the user log is sent to them
	wxString msg_Eng = _T("There was an error in the https transmission for ListKbs() in OnButtonLanguagesPageDeleteCustomCode(), error returned will be 22 regardless of the actual error.");
	bool bNoUsingDefinitions = TRUE; // initialize to there being no shared kb
									 // definitions using this particular custom code
	// List the current definitions
	CURLcode result = (CURLcode)m_pKbServer->ListKbs(m_pKbServer->GetKBServerUsername(),
										   m_pKbServer->GetKBServerPassword());
	if (result > CURLE_OK) // CURLE_OK is 0
	{
		// There was an error in the http transmission; let the user try again
		// later on, so warn him and leave the page unchanged
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		m_pApp->LogUserAction(msg_Eng);
		return;
	}
	else
	{
		// No error - the current list of shared kb definitions is in m_kbsList.
		// These are in the form of KbServerKb struct pointers on the heap, so
		// we must be sure to delete them after we've finished with them.
		wxString aDefinition;
		KbsList* pKbsList = NULL;
		pKbsList = m_pKbServer->GetKbsList();
		// There may be no kb definitions done yet, or there could be one or more
		wxASSERT(pKbsList != NULL && pKbsList->GetCount() >= 0);
		size_t count = pKbsList->GetCount();
		if (count == 0)
		{
			// No kb definitions yet, so the custom code can be deleted
			;
		}
		else
		{
			// There is at least a single kb definition, so more processing
			// is required - get the ones which contain the code into a
			// string list; if the result is an empty list, then the code
			// has not yet been used in a shared kb definition, in which case
			// we can go ahead and delete it further below.
			KbServerKb* pItem = NULL;
			KbsList::iterator iter;
			KbsList::compatibility_iterator c_iter;
			wxString srccode;
			wxString nonsrccode;
			int indx = -1;
			for (iter = pKbsList->begin(); iter != pKbsList->end(); ++iter)
			{
				indx++;
				c_iter = pKbsList->Item((size_t)indx);
				pItem = c_iter->GetData();
				srccode = pItem->sourceLanguageCode;
				nonsrccode = pItem->targetLanguageCode;
				if (code == srccode || code == nonsrccode)
				{
					// Collect this definition, as it contains the code we want to delete
					aDefinition.Empty();
					aDefinition << _T('[');
					aDefinition << srccode;
					aDefinition << _T(':');
					aDefinition << nonsrccode;
					aDefinition << _T(']');
					kbDefsArray.Add(aDefinition);
					bNoUsingDefinitions = FALSE;
				}
			}
		}
	}
	// TEMPORARY, for testing the code for the warning which follows
	//bNoUsingDefinitions = FALSE;
	if (!bNoUsingDefinitions)
	{
		// There is at least one shared kb definition which uses the code
		// which is to be deleted, so we cannot permit deletion (until all
		// such shared kb definitions, and their kb entries also, are
		// deleted; because MySQL will not allow the code (custom or not) to
		// be removed while something depending on it exists. We have to tell
		// this to the user, and clear the text boxes and the selection. 
		// Also, tell the user which shared definition(s) are the offending
		// ones.
		// Compute compositeDefsStr from the array of stored using definitions
		compositeDefsStr.Empty();
		size_t mySize = kbDefsArray.size();
		size_t i;
		for (i = 0; i < mySize; i++)
		{
			wxString aDef = kbDefsArray.Item(i);
			if (!compositeDefsStr.IsEmpty())
			{
				compositeDefsStr << _T(',');
			}
			compositeDefsStr << aDef;
		}
		wxString myMsg;
		if (mySize > 1)
		{
			myMsg = myMsg.Format(_("Knowledge bases are depending on the code %s which you want to delete.\nYou first need to delete the shared knowledge bases which use it (which also deletes all their entries from the server).\n These ones use the code: %s"),
				code.c_str(), compositeDefsStr.c_str());
		}
		else
		{
			myMsg = myMsg.Format(_("A knowledge base is depending on the code %s which you want to delete.\nYou first need to delete the shared knowledge base which uses it (which also deletes all its entries from the server).\n This one uses the code: %s"),
				code.c_str(), compositeDefsStr.c_str());
		}
		wxString title = _("Warning: Unable to delete");
		wxMessageBox(myMsg, title, wxICON_WARNING | wxOK);

		// We must clear the array of constructed defs, otherwise memory will leak
		for (i = 0; i < mySize; i++)
		{
			(kbDefsArray.Item(i)).Clear();
		}
		kbDefsArray.Clear();
		return;
	}
	else
	{
		// Go ahead and do the deletion of the custom language code...
		// Note: KB Sharing Manager uses a stateless setup of the KbServer class. That means that
		// the Manager's class instance's m_pKbServer instance points at a stateless instance, and that
		// the url, username and password stored within it are separate from any used by the user for
		// a KBserver access as stored in the project config file; so the following calls will not clobber
		// any of the user's authentication credentials. Any person with relevant permissions and valid
		// credentials can use the KB Sharing Manager from anyone's computer, with complete safety.
		CURLcode result = CURLE_OK;
		result = (CURLcode)m_pKbServer->RemoveCustomLanguage(code);
		if (result != CURLE_OK)
		{
			// Don't expect an error of this kind, but probably a good idea to make it localizable
			// but the developers need an English message
			wxString msg;
			msg = msg.Format(_("Deleting the custom code definition in the server failed, for custom language code: %s"), code.c_str());
			wxString msgEnglish;
			msgEnglish = msgEnglish.Format(_T("Deleting the custom code definition in the server failed, for custom language code: %s CURLcode %d"),
				code.c_str(), (unsigned int)result);
			m_pApp->LogUserAction(msgEnglish);
			wxMessageBox(msg, _T("KB Sharing Manager error"), wxICON_WARNING | wxOK);
			return;
		}
		else
		{
			// Success, re-populate the list, and clear the text controls, after
			// first removing from the heap the existing KbServerLanguage structs
			// stored as Data() members for the listbox entries
			m_pKbServer->ClearLanguagesList(m_pKbServer->GetLanguagesList());
			LoadDataForPage(m_nCurPage);

			wxCommandEvent dummy;
			OnButtonLanguagesPageClearBoxes(dummy);
		}
	}
	m_pKbServer->ClearKbsList(m_pKbServer->GetKbsList());
}
*/
// Return TRUE if the KB definition defined by pKbDefToTest matches one of those stored
// in pKbsList, FALSE otherwise
// BEW 31Aug20 updated, changed 'Code' to 'Name' in the internal vars - for leon's solution
bool KBSharingMgrTabbedDlg::IsThisKBDefinitionInSessionList(KbServerKb* pKbDefToTest, KbsList* pKbsList)
{
	if (pKbsList == NULL || pKbsList->empty())
		return FALSE;
	KbsList::iterator iter;
	KbsList::compatibility_iterator c_iter;
	int anIndex = -1;
	for (iter = pKbsList->begin(); iter != pKbsList->end(); ++iter)
	{
		anIndex++;
		c_iter = pKbsList->Item((size_t)anIndex);
		KbServerKb* pEntry = c_iter->GetData();
		bool bsrcNameIsSame = pKbDefToTest->sourceLanguageName == pEntry->sourceLanguageName;
		bool bnonsrcNameIsSame = pKbDefToTest->targetLanguageName == pEntry->targetLanguageName;
		bool bTypeIsAMatch = pKbDefToTest->kbType == pEntry->kbType;
		if (bsrcNameIsSame && bnonsrcNameIsSame && bTypeIsAMatch)
		{
			return TRUE;
		}
	}
	// If control gets to here, there was no match
	return FALSE;
}

// When I checked the KBserver at kbserver.jmarsden.org, I found that the earliest user
// (using timestamp values) was not Jonathan as expected, but me - and I didn't set up
// that server instance, he did. So preventing the earliest user from being deleted may
// not be a robust way to ensure someone with useradmin = true privilege level is always
// retained in the user table. So Jonathan said the way to do it is to look at all the
// users with useradmin privilege, and protect the one with the lowest ID value from being
// deleted, which is the way I've chosen to go
// BEW 29Aug20 pending update
/* deprecated 16Nov20 -- too dangerous to let a user do this
void KBSharingMgrTabbedDlg::OnButtonUserPageRemoveUser(wxCommandEvent& WXUNUSED(event))
{

	// TODO  add code for Leon's solution


	//  legacy code,  calls  m_pKbServer->RemoveUser(nID)
	// Get the ID value, if we can't get it, return
	if (m_pOriginalUserStruct == NULL)
	{
		wxBell();
		wxString msg = _T("KB Sharing Manager: bell ring, because m_pOriginalUserStruct was NULL in OnButtonUserPageRemoveUser(), so returned prematurely.");
		m_pApp->LogUserAction(msg);
		return;
	}
	else
	{
		// Prevent removal if the user is the useradmin == true one which is the earliest
		// (ie. the lowest ID value), and tell the user that's why his attempt was rejected
		if (m_pOriginalUserStruct->username == m_earliestUseradmin)
		{
			// This guy must not be deleted
			wxBell();
			wxString msg = _("This user is the earliest user with administrator privilege level. This user cannot be removed.");
			wxString msg_Eng = _T("This user is the earliest user with administrator privilege level. This user cannot be removed.");
			m_pApp->LogUserAction(msg_Eng);
			wxString title = _("Warning: Illegal user deletion attempt");
			wxMessageBox(msg, title, wxICON_WARNING | wxOK);
			return;
		}
		int nID = (int)m_pOriginalUserStruct->id;
		// Remove the selected user from the KBserver's user table
		CURLcode result = CURLE_OK;
		result = (CURLcode)m_pKbServer->RemoveUser(nID);
		// Update the page if we had success, if no success, just clear the controls
		if (result == CURLE_OK)
		{
			LoadDataForPage(m_nCurPage);
		}
		else
		{
            // The removal did not succeed- probably because it is parent to entries in the
            // entry table -- an error message will have been shown from within the above
            // RemoveUser() call
			wxString msg_Eng = _T("KB Sharing Manager: the RemoveUser() call failed in OnButtonUserPageRemoveUser().");
			m_pApp->LogUserAction(msg_Eng);
			wxCommandEvent dummy;
			OnButtonUserPageClearControls(dummy);
		}
		DeleteClonedKbServerUserStruct();
	}

}
*/

/* BEW 21Oct20 unneeded

void KBSharingMgrTabbedDlg::OnButtonKbsPageRemoveKb(wxCommandEvent& WXUNUSED(event))
{
	// Is the user of the Manager authorized to use this button?
	if (!m_bKbAdmin)
	{
		wxString msg = _("Sorry, you are not a kb administrator.\nYou are not authorized to use this button.");
		wxString title = _("Authorization lacking");
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		return;
	}
	wxASSERT(m_pApp->m_pKbServer_ForManager != NULL);
	KbServer* pOldKbServer = m_pApp->m_pKbServer_ForManager;

	// Get the ID value, if we can't get it, return
	if (m_pOriginalKbStruct != NULL)
	{
		wxString msg = _("This shared database was not created in this session, so a background deletion of the database entries will be done.\n This may take several hours if the KBserver is located on the web; or minutes if located on the local area network.\nYou can shut the machine down safely anytime, and later repeat the removal to get rid of any that remained undeleted when you shut down.\nYou can safely close the Knowledge Base Sharing Manager while the deletions are in progress, and even work in a different project if you wish.\nThe background deletions will continue for as long as the Adapt It session is active, or until the deletions are finished.\nWhen all entries in the selected database are deleted, the language codes for it are automatically removed from the list as the last step.\nAt that time you can be certain the entire database no longer exists on the server to which you are currently connected.\nDatabases not chosen for deletion will, of course, remain intact on the server.");
		wxString title = _("Warning: a lengthy background deletion is happening");

		// Test for the selected definition not being in the list of KB definitions added
		// in this session of the KB Sharing Manager gui; (comment out this test and
		// the TRUE block which follows, to have the deletion tried unilaterally)
		// If not in the session list, then it was added at some earlier session and probably
		// now has entries stored in it - in which case we need to get rid of its remote
		// row entries in the MySql entry table, before we can remove the KB definition from
		// the kb table. If it is in the session list, then there has been no chance yet for
		// any entries to have been added to the remote KBserver for this KB, so we can
		// immediately delete it from the kb table without having to do prior entry removal
		if (!IsThisKBDefinitionInSessionList(m_pOriginalKbStruct, m_pKbsAddedInSession))
		{
			// If an "all entries" emptying as part of a removal is already in progress,
			// don't start another.... (the next line must be commented out when the
			// code is compiled for a release)
			//m_pApp->m_bKbSvrMgr_DeleteAllIsInProgress = TRUE; // uncomment out to test showing
																// of the information message
			if (m_pApp->m_bKbSvrMgr_DeleteAllIsInProgress)
			{
				wxString msg2 = _("Emptying of only one database at a time is allowed.\nAn emptying is already in progress, and it may take a long time.\nWhen the one requested for deletion no longer displays its pair of language codes in the list,\nthe Manager dialog is then ready for you to request deletion of another one.");
				wxString title2 = _("Only one at a time");
				wxMessageBox(msg2, title2, wxICON_INFORMATION | wxOK);
				return;
			}
			// There is no removal of a dependent kb definition currently in progress, so the
			// next check is to ensure we are not trying to remove a kb which is currently
			// actively being shared by the open Adapt It project -- if we are, tell the
			// administrator to first remove that sharing setup ( using menu Advance menu,
			// Setup Or Remove Knowledge Base Sharing) and then return to do the Removal
			m_srcLangNameOfDeletion = m_pOriginalKbStruct->sourceLanguageName;
			m_nonsrcLangNameOfDeletion = m_pOriginalKbStruct->targetLanguageName; // stores non-src code
			m_kbTypeOfDeletion = m_pOriginalKbStruct->kbType;
			bool bDeletingAdaptionKB = m_kbTypeOfDeletion == 1 ? TRUE : FALSE;
			
			// There has to be an active project (and it has to have a local KB of same type and
			// language codes the same as we want to delete; but sharing must be off -- check for
			// these conditions in the blocks which follow
			if (m_pApp->m_bKBReady && m_pApp->m_bGlossingKBReady)
			{
				bool bTheNamesMatch = FALSE;
				if (bDeletingAdaptionKB) // trying to delete a remote adaptation KB from KBserver
				{
					if ((m_srcLangNameOfDeletion == m_pApp->m_sourceLanguageName) &&
						(m_nonsrcLangNameOfDeletion == m_pApp->m_targetLanguageName))
					{
						bTheNamesMatch = TRUE;
					}
				}
				else // trying to delete a remote glossing KB from KBserver
				{
					if ((m_srcLangNameOfDeletion == m_pApp->m_sourceLanguageName) &&
						(m_nonsrcLangNameOfDeletion == m_pApp->m_glossesLanguageName))
					{
						bTheNamesMatch = TRUE;
					}
				}
				// If the source & nonsource codes don't match those of the remote KB
				// we want to delete, then tell user and bail out
				if (!bTheNamesMatch)
				{
					m_nonsrcLangNameOfDeletion.Empty();
					wxCommandEvent dummy;
					OnButtonKbsPageClearListSelection(dummy);
					OnButtonKbsPageClearBoxes(dummy);
					wxString msgAD = _(
"You are currently in an adaptation project which cannot share the knowledge base you are trying to delete.\nYou are trying to delete an adaptations knowledge base with names [ %s , %s ]\nbut your local knowledge base of the same type has different names [ %s , %s].\nClose this Knowledge Base Sharing Manager now, then close the project and then enter the correct project. Then try to delete from there.");
					wxString msgGL = _(
"You are currently in an adaptation project which cannot share the knowledge base you are trying to delete.\nYou are trying to delete a glosses knowledge base with names [ %s , %s ]\nbut your local knowledge base of the same type has different names [ %s , %s].\nClose this Knowledge Base Sharing Manager now, then close the project and then enter the correct project. Then try to delete from there.");
					wxString title3 = _("Wrong adaptation project");
					if (bDeletingAdaptionKB)
					{
						msgAD = msgAD.Format(msgAD, m_srcLangNameOfDeletion.c_str(), m_nonsrcLangNameOfDeletion.c_str(),
							m_pApp->m_sourceLanguageName.c_str(), m_pApp->m_targetLanguageName.c_str());
						wxMessageBox(msgAD, title3, wxICON_WARNING | wxOK);
					}
					else
					{
						msgGL = msgGL.Format(msgGL, m_srcLangNameOfDeletion.c_str(), m_nonsrcLangNameOfDeletion.c_str(),
							m_pApp->m_sourceLanguageCode.c_str(), m_pApp->m_glossesLanguageCode.c_str());
						wxMessageBox(msgGL, title3, wxICON_WARNING | wxOK);
					}
					return;
				}

				// Our next check is for whether or not the remote KB to be deleted is
				// actually being shared presently, or not. If it is being shared, we won't go
				// ahead immediately. If not being shared, we can allow removal to go ahead.
				// See next comment for more detail
				if ((m_pApp->m_bIsKBServerProject && bDeletingAdaptionKB) ||
					(m_pApp->m_bIsGlossingKBServerProject && !bDeletingAdaptionKB))
				{
					// Oops, we are trying to deleted a remote KB that this current AI project
					// is actively sharing! Letting the deletion happen would be crazy, we would
					// be fighting against ourselves. There are two options. Just warn the user
					// and here turn of all sharing, then let the deletion go ahead. Or. Warn the
					// user, and not do the deletion. The latter option is safer, deletion is
					// a radical action, so if he has to do some extra steps to make it happen
					// then that would be wise -> he needs to get a warning, be told to exit
					// the Manager dialog, then turn of sharing to at least the CKB type that
					// he is trying to delete, then reenter the Manager, and try again. 
					m_srcLangNameOfDeletion.Empty();
					m_nonsrcLangNameOfDeletion.Empty();
					wxCommandEvent dummy;
					OnButtonKbsPageClearListSelection(dummy);
					OnButtonKbsPageClearBoxes(dummy);
					wxString msg3 = _(
"The knowledge base you selected for removal is a shared knowledge base within the current adaptation project.\nIt makes no sense to remove the contents of a knowledge base that is still able to accept new entries.\nClose this Knowledge Base Sharing Manager now, then use the Setup Or Remove Knowledge Base Sharing command in the Advanced menu to make the knowledge base no longer shared.\nThen reopen the Knowledge Base Sharing Manager and retry the removal. It should then succeed.\nBut first make sure no users anywhere are sharing to the remote knowledge base you want to remove from the KBserver.\n(Removing a remote knowledge base from a KBserver does not affect the contents of the local knowledge base.)");
					wxString title3 = _("Illegal Removal Attempt");
					wxMessageBox(msg3, title3, wxICON_INFORMATION | wxOK);
					return;
				}

				// It's okay, we can go ahead with the removal request
				m_pApp->m_bKbSvrMgr_DeleteAllIsInProgress = TRUE;

                // Put the two codes in the stateless Occasional KbServer , so we can check
                // for and prevent new attempts on this machine to recreate the sharing
                // with those old code values while the deletion of entry rows is
                // happening; and put the kbtype there too
				m_pKbServer->SetSourceLanguageName(m_srcLangNameOfDeletion);
				m_pKbServer->SetTargetLanguageName(m_nonsrcLangNameOfDeletion);
				m_pKbServer->SetKBServerType(m_kbTypeOfDeletion);


				// The remote kb definition can be deleted, but it owns database entries, & so
				// may take a long time because a https transmission is done for each entry
				// deleted, and in a high latency environment this may turn a job that should
				// only take minutes into one that may take a day or more -- but it is on a
				// thread and the database won't break if the thread is trashed before
				// completion (e.g. by the user shutting down the machine to go home) - he can
				// just get the deletion going again in a later session and there will be
				// fewer entries to delete each such successive attempt made
				wxMessageBox(msg, title, wxICON_WARNING | wxOK);
							
				// This module's m_pKbServer pointer is set currently to the the app's
				// m_pKbServer_ForManager - created at end of InitDialog(); and the code
				// above used the target CKB instance for the testing. The code below
				// does not require CKB access, only access to the KBserver, and to 
				// a persistent KbServer instance which remains no longer needed
				wxASSERT(m_pApp->m_pKbServer_Persistent == NULL); // required

				// From this point on, we need to create a stateless persistent KbServer instance
				// to assign to app's m_pKbServer_Persistent pointer, which can persist
				// after the Manager closes if necessary. It needs no ptr to either of the local
				// CKB instances - so it's m_pKB member we set to NULL
				m_pApp->m_pKbServer_Persistent = new KbServer(1, TRUE); // TRUE is bool bStateless
				// From now to the end of this OnButtonKbsPageRemoveKb() function, we want
				// our m_pKbServer member variable to point to this persistent KbServer instance
				m_pKbServer = m_pApp->m_pKbServer_Persistent;
                // Note: we instantiate the persistent one only when
                // m_pApp->m_bKbSvrMgr_DeleteAllIsInProgress has already been set to TRUE,
                // which is the case here. 
                // m_pKbServer now points at the tgt KbServer instance in app's
                // m_pKbServer_Persistent member variable; we need no local CKB so pass NULL
                // for it's m_pKB member variable
				m_pKbServer->SetKB(NULL);

				// Most of m_pKbServer_Persistent members are as yet undefined, so populate them
				// from the KB Sharing Manager's stateless KbServer Occasional instance -
				// as that was used for authenticating when logging in to the Manager, and from
				// what the user did in the controls on the kbs page of the Manager
				wxString anIpAddr = pOldKbServer->GetKBServerIpAddr();
				m_pKbServer->SetKBServerIpAddr(anIpAddr);

// TODO fix this bit 5Sep20
				wxString str = pOldKbServer->GetSourceLanguageName();
				m_pKbServer->SetSourceLanguageName(str);
				if (bDeletingAdaptionKB)
				{
					str = pOldKbServer->GetTargetLanguageName();
					m_pKbServer->SetTargetLanguageName(str);
				}
				else
				{
					str = pOldKbServer->GetGlossLanguageName();
					m_pKbServer->SetGlossLanguageName(str);
				}
				m_pKbServer->SetKBServerType(m_kbTypeOfDeletion);

				// Which do we want to delete?
//				long nID = (int)m_pOriginalKbStruct->id; // this is the one we selected in the listbox
				// Put a copy in the app instance, since the Manager GUI might be deleted long
				// before the background deletion (thread) of the KB entries has completed,
				// and we'll need to get the ID then for deleting that kb language code pair
//				m_pApp->kbID_OfDefinitionForDeletion = nID;
				wxString theUsername = pOldKbServer->GetKBServerUsername();
				m_pKbServer->SetKBServerUsername(theUsername);
				wxString thePassword = pOldKbServer->GetKBServerPassword();
				m_pKbServer->SetKBServerPassword(thePassword);
				// The above are all that the ChangedSince_Queued() call needs, except for the
				// timestamp, which we do next; and the DeleteSingleKbEntry() calls just need
				// the above, and an ID for the entry - each ID we get from the data returned
				// from the ChangedSince_Queued() call

				// Get all the selected database's entries
				int rv = 0; // rv is "return value", initialize it
				wxString timestamp;
				timestamp = _T("1920-01-01 00:00:00"); // earlier than everything, so downloads the lot
				m_pKbServer->GetDownloadsQueue()->Clear();
				rv = m_pKbServer->ChangedSince_Queued(timestamp, FALSE);
				// in above call, FALSE is value of the 2nd param, bDoTimestampUpdate
				// Check for error
				if (rv != 0)
				{
					// The download of all owned entries in the selected kb server definition
					// did not succeed; the user probably needs to know, so make it localizable;
					// and since we didn't succeed, just clear the list selection and the text boxes
					wxCommandEvent dummy;
					OnButtonKbsPageClearListSelection(dummy);
					OnButtonKbsPageClearBoxes(dummy);
					wxBell();
					m_pKbServer->GetDownloadsQueue()->clear();
					wxString msg = _("The request to the remote server to download all the entries owned\nby the knowledge base selected for removal, failed unexpectedly.\nNothing has been changed, so you might try again later.");
					wxString title = _("Error: downloading database failed");
					m_pApp->LogUserAction(_T("OnButtonKbsPageRemoveKb() failed at the ChangedSince_Queued() download call"));
					wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);

					// Tidy up
					goto tidyup;
				}
				else
				{
					// All's well, continue processing... remember m_pKbServer is
					// now pointing at app's m_pKbServer_Persistent member which
					// has the pointer to the persisting stateless KbServer instance
					// used (only) for the kb deletion process
					// How many entries have to be deleted?
					m_pApp->m_nQueueSize = m_pKbServer->GetDownloadsQueue()->size();

#if defined (_DEBUG) && defined(_WANT_DEBUGLOG)
					wxLogDebug(_T("OnButtonKbsPageRemoveKb: m_nQueueSize  %d"), m_pApp->m_nQueueSize);
#endif
					if (m_pApp->m_nQueueSize == 0)
					{
						// We don't expect this, but if we get nothing back, we don't need to
						// call the thread; instead, just do the kb definition deletion, and
						// clean up
//	Leon not use id					int nID = (int)m_pOriginalKbStruct->id;
						// Remove the selected kb definition from the KBserver's kb table
						CURLcode result = CURLE_OK;
//	Leon no use id					result = (CURLcode)m_pKbServer->RemoveKb(nID);
						// Update the page if we had success, if no success, just clear the controls
						if (result == CURLE_OK)
						{
							LoadDataForPage(m_nCurPage);
						}
						else
						{
							// The removal did not succeed- this is quite unexpected, an English error
							// message will do
							wxCommandEvent dummy;
							OnButtonKbsPageClearListSelection(dummy);
							OnButtonKbsPageClearBoxes(dummy);
							wxBell();
							msg = _T("OnButtonKbsPageRemoveKb(): unexpected failure to remove a shared knowledge base (definition) which owns no entries.\nThe application will continue safely. Perhaps try the removal again later.");
							title = _T("Error: could not remove the database");
							m_pApp->LogUserAction(msg);
							wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
						}

						// If the new kb has not been used yet, but we are in a new session of the
						// KB sharing manager, then control will go through here. Check for the
						// deletion server being deleted already, and if not deleted & it's pointer
						// set to NULL, delete it here and clear the app members for storing the type
						// and language codes
						goto tidyup;
					}

					CStatusBar* pStatusBar = NULL;
					pStatusBar = (CStatusBar*)m_pApp->GetMainFrame()->m_pStatusBar;
					pStatusBar->StartProgress(_("Delete KB"), _("Deleting..."), m_pApp->m_nQueueSize);

//BEW 7Sep20 --> change, it's a simple SQL pair of commands	// Do the deletions synchronously
//					int rv = m_pApp->m_pKbServer_Persistent->DoEntireKbDeletion(
//																m_pApp->m_pKbServer_Persistent, nID);
					wxUnusedVar(rv);

					pStatusBar->FinishProgress(_("Delete KB"));
				} // end of else block for test: if (rv != 0)

			} // end of TRUE block for test: if (m_pApp->m_bKBReady && m_pApp->m_bGlossingKBReady)
			else
			{
				wxBell(); // we don't expect local KBs to not be loaded, so just a bell will do
			}

			// Tidy up
			goto tidyup;

		} // end of TRUE block for test: if (!IsThisKBDefinitionInSessionList(m_pOriginalKbStruct, m_pKbsAddedInSession))
		else
		{
			// It was created in this session, so we can safely remove it - it can't
			// possibly own any KB entries yet
//			int nID = (int)m_pOriginalKbStruct->id;
			// Remove the selected kb definition from the KBserver's kb table
			CURLcode result = CURLE_OK;
//			result = (CURLcode)m_pKbServer->RemoveKb(nID);
			// Update the page if we had success, if no success, just clear the controls
			if (result == CURLE_OK)
			{
				LoadDataForPage(m_nCurPage);
			}
			else
			{
				// The removal did not succeed- this is quite unexpected, an English error
				// message will do
				wxCommandEvent dummy;
				OnButtonKbsPageClearListSelection(dummy);
				OnButtonKbsPageClearBoxes(dummy);
				wxBell();
				msg = _T("OnButtonKbsPageRemoveKb(): unexpected failure to remove a shared KB (definition) created earlier in this session. The removal attempt will be ignored and processing will continue.");
				title = _T("Error: could not remove the database");
				m_pApp->LogUserAction(msg);
				wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);

				// Tidy up
				goto tidyup;
			}
			//DeleteClonedKbServerKbStruct();

		} // end of else block for test: if (!IsThisKBDefinitionInSessionList(m_pOriginalKbStruct, m_pKbsAddedInSession))

	} // end of TRUE block for test: if (m_pOriginalKbStruct != NULL)
	else
	{
		wxBell();
		wxString msg = _T("KB Sharing Manager: bell ring, because m_pOriginalKbStruct was NULL in OnButtonKbsPageRemoveKb(), so returned prematurely.");
		m_pApp->LogUserAction(msg);
		return;
	}

	// Tidy up -- note, the thread could still be running and it has a member which points
	// at the m_pKbServer_Persistent KbServer instance which has the embedded queue
	// within it which holds the KbServerEntry struct pointers which define the entries
	// to be deleted, and control will typically reach here well before all the entries
	// have been removed, so... test for m_pApp->m_bKbSvrMgr_DeleteAllIsInProgress being
	// TRUE still, and if so, don't tidy up here. If the thread completes while the AI
	// session is still running, the thread will do the tidy up. If the session completes
	// we can not bother, since all the app's heap memory gets reclaimed at that time
tidyup:	if (!m_pApp->m_bKbSvrMgr_DeleteAllIsInProgress)
	{
		// Deletion is not in progess, so can clean up here
		if (m_pApp->m_pKbServer_Persistent != NULL)
		{
			if (!m_pApp->m_pKbServer_Persistent->IsQueueEmpty())
			{
				// Queue is not empty, so delete the KbServerEntry structs that are 
				// on the heap
				m_pApp->m_pKbServer_Persistent->DeleteDownloadsQueueEntries();
			}
			delete m_pApp->m_pKbServer_Persistent;
			m_pApp->m_pKbServer_Persistent = (KbServer*)NULL;

			// Restore the correct KbServer pointer
			m_pKbServer = pOldKbServer; // this makes m_pKbServer point back 
										// again at m_pApp->m_pKbServer_ForManager;

			m_pApp->m_bKbSvrMgr_DeleteAllIsInProgress = FALSE;
		}
		
		DeleteClonedKbServerKbStruct();
		m_pApp->StatusBar_EndProgressOfKbDeletion();
		m_pApp->RefreshStatusBarInfo();
	}
}
*/
// The box state will have already been changed by the time control enters the handler's body
void KBSharingMgrTabbedDlg::OnCheckboxUseradmin(wxCommandEvent& WXUNUSED(event))
{
	// The value in the checkbox has toggled by the time control
	// enters this handler, so code accordingly
	bool bCurrentUseradminValue = m_pCheckUserAdmin->GetValue();
    // kbadmin value is auto-set to 1 (TRUE) always, because users have to be 
	// able to create entries in kb table

	wxString title = _("Permission level: Set or Change");
	wxString msg = _("Tick the box, but after that do not click 'Change Permission',\nif you are giving the permission 'Can add other users'\n to a new user you are creating.\nTick the box and then click the 'Change Permission' button,\nif you want to change the permission level for an existing user.\nUn-tick the box to give a new user no permission to add other users,\nor to remove an existing user's permission to add new users.");
	wxMessageBox(msg, title, wxICON_INFORMATION | wxOK);

	if (bCurrentUseradminValue)
	{
		// This click was to promote the user administrator privilege level, so that
		// the user can add other users. The box is now ticked, but the kbserver
		// knows nothing of this change. So after this handler exits, the button
		// "Change Permission" needs to be clicked.
		// But if adding a new user is being done, tick the box but do not click
		// the Change Permission button below it
		;
	}
	else
	{
		// The click was to demote from user administrator privilege level...
		//
		// So test, to make sure this constrait is not violated.
		wxString strNonChangeablePair = GetEarliestUseradmin(m_pUsersListForeign); // signature var is private
		wxString strFirst = m_pApp->m_strUserID;
		wxString strSecond = _T("kbadmin");
		//int offset1 = wxNOT_FOUND;
		//int offset2 = wxNOT_FOUND;
		//offset1 = strNonChangeablePair.Find(strFirst);
		//offset2 = strNonChangeablePair.Find(strSecond);
		
		
		// Get the privilege level value as it was when the user was clicked in the listbox
		if ((m_pOriginalUserStruct->username != strFirst) && (m_pOriginalUserStruct->username != strSecond))
		{
			// This guy (on the left of each inequality test) can be safely demoted, because
			// he's not in the list of taboo useradmin persons for permission change
			m_pCheckUserAdmin->SetValue(FALSE);
		}
		else
		{
			// One of them was matched, so this guy has to retain his useradmin == TRUE permission level
			wxString title = _("Illegal permission change");
			wxString msg = _("Warning: you are not permitted to change the permission value for either of these users: %s");
			msg = msg.Format(msg, m_earliestUseradmin.c_str());
			wxMessageBox(msg, title, wxICON_WARNING | wxOK);

			m_pCheckUserAdmin->SetValue(TRUE); // restore the ticked state of the useradmin checkbox
		}
	}
}
/* deprecated  BEW 28Aug20
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
*/ 
/* deprecate, an Edit User button is dangerous. Just have remove or add
// The API call to a KBserver should only generate a json field for those fields which have
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
    // selection current. Note: if an existing user is an owner of entries in the entry
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
	// provided XXX is not yet the owner of any records. So the crucial test here is for
	// whether or not I'm trying to edit myself or not -- if I am, I must reject any
	// attempt at a username change
	wxString loggedInUser = m_pKbServer->GetKBServerUsername(); // it will have been set to m_statelessUsername
	wxString originalUsername = m_pOriginalUserStruct->username;
	bool bIAmEditingMyself = loggedInUser == originalUsername ? TRUE : FALSE;

	// Get the administrator's edited values - get every field, and check against the
	// original values (already stored in m_pOriginalUserStruct which was populated when
	// the administrator clicked on the list box's entry) to find which have changed. We
	// will use a bool to track which ones are to be used for constructing the appropriate
	// json string to be PUT to the server's entry for this user.
	wxString strUsername = m_pTheUsername->GetValue();
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
			m_pTheUsername->ChangeValue(originalUsername);
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

    // Get the checkbox values; if the useradmin one has been ticked, the kbadmin one will
    // have been forced to be ticked as well (but the administrator is free to untick the
    // latter which will force the former to also be unticked)
	bool bUseradmin = m_pCheckUserAdmin->GetValue();
	bool bKbadmin = m_pCheckKbAdmin->GetValue();
	// Pass the values to the API call via a KbServerUser struct, store it in
	//  m_pUserStruct after cleaning out what's currently there
	m_pKbServer->ClearUserStruct(); // clears m_userStruct member, it's in stack frame
	// our m_pUserStruct member points to that m_userStruct, so set it's contents below

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
    // returned CURLE_OK and there was no HTTP error, then subsequent accesses of KBserver
    // will fail unless the password change is immediately propagated to the KbServers'
    // private m_kbServerPassword member for both adapting and glossing KbServer instances.
    // So we must check for these conditions being in effect, and make the changes with the
	// help of the bLoggedInUserJustChangedThePassword boolean
    bool bLoggedInUserJustChangedThePassword = FALSE;
	if (!strPassword.IsEmpty())
	{
		bUpdatePassword = TRUE;
		if (bIAmEditingMyself)
		{
			// Control can only get here if the logged in user, even if editing his own user
			// entry, isn't trying to change his username, so we can just rely below on the
			// value of bLoggedInUserJustChangedThePassword to guide what happens
			bLoggedInUserJustChangedThePassword = TRUE;
		}
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

	// We could prevent an attempt to edit the username, as most likely it owns some
	// records in the entry table. But if we allow the attempt, nothing happens anyway, curl
	// returns CURLE_OK, an HTTP's 204 No Content is returned, so it's not treated as an
	// error, even thought sql has rejected the request. The problem with leaving it
	// happen is that the user gets no feedback that attempting an edit of the username
	// for one which is the owner of entries, will fail. We can't unilaterally prevent it
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

	// If nothing is to be changed, return, after telling the user
	if ((bUpdateUsername == FALSE) && (bUpdateFullName == FALSE) && (bUpdatePassword == FALSE) &&
		(bUpdateKbadmin == FALSE) && (bUpdateUseradmin == FALSE))
	{
		wxBell();
		wxString title = _("There is nothing to do");
		wxString msg = _("Warning: you have not made any changes. Make at least one change, then click Edit User again.\nIf you no longer want to edit this user, click Clear Controls.");
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		return;
	}

	// Update the user's details in the KBserver's user table
	result = (CURLcode)m_pKbServer->UpdateUser(nID, bUpdateUsername, bUpdateFullName,
										bUpdatePassword, bUpdateKbadmin, bUpdateUseradmin,
										m_pUserStruct, strPassword);
	if (result == CURLE_OK)
	{
		// See what we've done and be ready for a new administrator action; but
		// LoadDataForPage() does a ListUsers() call, and if the password has been
		// changed, using the old password will give a failure - so make the needed
		// password updates in the rest of the application before calling it
		if (bLoggedInUserJustChangedThePassword)
		{
			// strPassword is the new password which has just been made the current one -
			// this is just the password string itself, not a digest (the digest form of
			// it was created at the point of need, within the UpdateUser) call itself)
			m_pKbServer->SetKBServerPassword(strPassword);
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
	}
	DeleteClonedKbServerUserStruct();
}
*/

// Return TRUE if the code pair (srcLangCodeStr,nonsrcLangCodeStr) match the pair in any
// row of the listbox pKbsList. FALSE if there is at least one difference
bool KBSharingMgrTabbedDlg::MatchExistingKBDefinition(wxListBox* pKbsList,
							wxString& srcLangNameStr, wxString& nonsrcLangNameStr)
{
	unsigned int count = pKbsList->GetCount();
	wxString srcCodeVal;
	wxChar comma = _T(',');
	wxString nonsrcCodeVal;
	wxString compositeStr;
	int offset = wxNOT_FOUND;
	unsigned int index;
	for (index = 0; index < count; index++)
	{
		compositeStr = pKbsList->GetString(index);
		wxASSERT(!compositeStr.IsEmpty());
		offset = compositeStr.find(comma);
		wxASSERT(offset != wxNOT_FOUND);
		srcCodeVal = compositeStr.Left(offset);
		nonsrcCodeVal = compositeStr.Mid(offset + 1);
		wxASSERT(!nonsrcCodeVal.IsEmpty());
		if ( (srcCodeVal == srcLangNameStr) && (nonsrcCodeVal == nonsrcLangNameStr) )
		{
			return TRUE;
		}
	}
	// If control gets to here, there was no match
	return FALSE;
}

// When the administrator clicks on a kb definition line in the listbox of the Kbs page of
// the KB Sharing Manager, follow through on populating the various structs etc, even though
// it is only the final contents of the two text boxes that the CreateKb() call relies on
void KBSharingMgrTabbedDlg::OnSelchangeKBsList(wxCommandEvent& WXUNUSED(event))
{
    if (m_pKbsListBox == NULL)
	{
		m_nSel = wxNOT_FOUND; // -1
		m_pBtnRemoveSelectedKBDefinition->Enable(FALSE); // Enable it by selecting a kb definition
		return;
	}
	if (m_pKbsListBox->IsEmpty())
	{
		m_nSel = wxNOT_FOUND; // -1
		m_pBtnRemoveSelectedKBDefinition->Enable(FALSE); // Enable it by selecting a kb definition
		return;
	}
    // The GetSelection() call returns -1 if there is no selection current. The CreateKb()
    // call doesn't rely on a selection having been made, since at least one of the
    // language codes for the newly created KB definition will need to be different from
    // the definition selected. However the selection will populate KbServerKb structs, and
    // the administrator may want to spin off a new KB definition by editing value(s)
    // obtained from such a struct
	m_nSel = m_pKbsListBox->GetSelection();
	wxString compositeStr;
	wxString comma = _T(',');
	int offset = wxNOT_FOUND;
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
    wxLogDebug(_T("OnSelchangeKBsList(): selection index m_nSel = %d"), m_nSel);
#endif
	// Get the entry's KbServerKb struct which is its associated client data
	m_pKbStruct = (KbServerKb*)m_pKbsListBox->GetClientData(m_nSel);
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnSelchangeKBsList(): ptr to client data... m_pKbStruct = %p"), m_pKbStruct);
#endif

	// Get the source language code & non-source language names
	wxString theSrcLangName;
	compositeStr = m_pKbsListBox->GetString(m_nSel);
	offset = compositeStr.find(comma);
	wxASSERT(offset != wxNOT_FOUND);
	theSrcLangName = compositeStr.Left(offset);
	wxASSERT(theSrcLangName == m_pKbStruct->sourceLanguageName);
	wxString nonSrcLangName = compositeStr.Mid(offset + 1);
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
    wxLogDebug(_T("OnSelchangeKBsList(): from list: source_langName = %s  non-source_langName = %s  , kbType %d (1 or 2)"),
		theSrcLangName.c_str(), nonSrcLangName.c_str(), m_pKbStruct->kbType);
#endif
	wxASSERT(nonSrcLangName == m_pKbStruct->targetLanguageName); // BEW 7Sep20 the struct is updated to 'Name's

	// Put the username for "who created the definition originally"
	// in the "Original Creator" read-only text box
	m_pKbDefinitionCreator->ChangeValue(m_pKbStruct->username);

	// Put a copy in m_pOriginalKbStruct
	//DeleteClonedKbServerKbStruct();
	m_pOriginalKbStruct = CloneACopyOfKbServerKbStruct(m_pKbStruct);

	// Fill the KB definition page's controls with their required data; the
	// logged in user can then edit the parameters
	m_pEditSourceCode->ChangeValue(theSrcLangName); // harmlessly retaining old box names
	m_pEditNonSourceCode->ChangeValue(nonSrcLangName); // ditto

	m_pBtnRemoveSelectedKBDefinition->Enable(); // Enable it - since there is a KB definition selected
}
/*
// When the administrator clicks on a custom code line in the listbox of the Languages page of
// the Manager, fill the page's widgets with the selected entry's data from the client data
// stored at the selected line
void KBSharingMgrTabbedDlg::OnSelchangeLanguagesList(wxCommandEvent& WXUNUSED(event))
{
	if (m_pCustomCodesListBox == NULL)
	{
		m_nSel = wxNOT_FOUND; // -1
		return;
	}
	if (m_pCustomCodesListBox->IsEmpty())
	{
		m_nSel = wxNOT_FOUND; // -1
		return;
	}
	// The GetSelection() call returns -1 if there is no selection current. The CreateLanguage()
	// call doesn't rely on a selection having been made, since if a language code is to be
	// created, it will need to be different from the definition selected. However the selection
	// will populate KbServerLanguage struct, and the administrator may want to spin off a new
	// custom language code definition by editing value(s) obtained from such a struct
	m_nSel = m_pCustomCodesListBox->GetSelection();
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnSelchangeLanguagesList(): selection index m_nSel = %d"), m_nSel);
#endif
	// Get the entry's KbServerKb struct which is its associated client data
	m_pLanguageStruct = (KbServerLanguage*)m_pCustomCodesListBox->GetClientData(m_nSel);
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnSelchangeLanguagesList(): ptr to client data... m_pLanguageStruct = %p"), m_pLanguageStruct);
#endif

	// Get the custom code & its description string, also that code's creator
	wxString customCode;
	wxString description;
	wxString creator;
	customCode = m_pCustomCodesListBox->GetString(m_nSel);
	description = m_pLanguageStruct->description;
	creator = m_pLanguageStruct->username;
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnSelchangeLanguagesList(): from list... customCode = %s  description = %s  creator = %s"),
		customCode.c_str(), description.c_str(), creator.c_str());
#endif
	wxASSERT(!customCode.IsEmpty());

	// Put the username for who created it in the  "Creator of the selected code"
	// read-only text box
	m_pCustomCodeDefinitionCreator->ChangeValue(creator);

	// Fill the KB definition page's controls with their required data; the
	// logged in user can then edit the parameters
	m_pEditCustomCode->ChangeValue(customCode);
	m_pEditDescription->ChangeValue(description);
}
*/
// The user clicked on a different line of the listbox for usernames
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
	// The GetSelection() call returns -1 if there is no selection current, so check for
	// this and return (with a beep) if that's what got returned
	m_nSel = m_pUsersListBox->GetSelection();
	if (m_nSel == wxNOT_FOUND)
	{
		wxBell();
		return;
	}

#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
    wxLogDebug(_T("OnSelchangeUsersList(): selection index m_nSel = %d"), m_nSel);
#endif
	// Get the username - the one clicked in the list and its client data's stored struct
	wxString theUsername = m_pUsersListBox->GetString(m_nSel);
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
    wxLogDebug(_T("OnSelchangeUsersList(): from list: username = %s"), theUsername.c_str());
#endif
	// Get the entry's KbServerUserForeign struct which is its associated client data
	m_pUserStructForeign = (KbServerUserForeign*)m_pUsersListBox->GetClientData(m_nSel);
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnSelchangeUsersList(): ptr to client data... m_pUserStructForeign = %p"), 
			m_pUserStructForeign);
#endif
	// check they match
	wxASSERT(theUsername == m_pUserStructForeign->username);

	// BEW 20Nov20 every time a user is clicked on in the list, we need that user
	// to be stored on the app in the member variable m_strChangePermission_User2
	// because if a change of the user permission is wanted, the ConfigureMovedDatFile()
	// app function will need that user as it's user2 value (user1 is for authenticating
	// to mariaDB/kbserver so is not appropriate). The aforementioned function needs to
	// create the correct comandLine for the ::wxExecute() call in app function CallExecute()
	// and it can't do so without getting the user name selected in the Manager user list
	m_pApp->m_strChangePermission_User2 = m_pUserStructForeign->username;
	// BEW 9Dec20 fullname changing needs it's own app storage wxStrings for commandLine building
	m_pApp->m_strChangeFullname_User2 = m_pUserStructForeign->username;
	m_pApp->m_strChangeFullname = m_pUserStructForeign->fullname;
	wxASSERT(!m_pUserStructForeign->fullname.IsEmpty()); // make sure it's not empty

	// BEW 20Nov20 a click to select some other user must clear the curr password box
	// so that it does not display the pwd for any user other than the selected user
	ClearCurPwdBox();

	// Put a copy in m_pOriginalUserStruct. The Remove User button uses the struct in 
	// m_pOriginalUserStruct to get at the user to be removed
	DeleteClonedKbServerUserStruct(); // BEW 29Aug20 <<-- no change needed for this one
	m_pOriginalUserStruct = CloneACopyOfKbServerUserStruct(m_pUserStructForeign); // BEW 28Aug20 updated
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnSelchangeUsersList(): ptr to client data... m_pOriginalUserStruct = %p"), m_pOriginalUserStruct);
    wxLogDebug(_T("OnSelchangeUsersList(): m_pUserStructForeign->username = %s  ,  useradmin = %d , kbadmin = %d"),
		m_pUserStructForeign->username.c_str(), m_pUserStructForeign->useradmin, 1);
#endif

	// Use the struct to fill the Users page's controls with their required data; the
	// logged in user can then edit the parameters (m_pTheUsername is the ptr to the
	// username wxTextCtrl)
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
    wxLogDebug(_T("OnSelchangeUsersList(): username box BEFORE contains: %s"), m_pTheUsername->GetValue().c_str());
#endif
	m_pTheUsername->ChangeValue(theUsername);
	// BEW 16Dec20 also put the value into app's m_Username2 string using
	// it's Update function UpdateUsername2(wxString str), so that
	// user page buttons which work with whatever is the selected user,
	// can grab its value and use for building commandLine for
	// ConfigureMovedDatFile() - so the correct sql change can be effected
	m_pApp->UpdateUsername2(theUsername);

#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
    wxLogDebug(_T("OnSelchangeUsersList(): username box AFTER contains: %s"), m_pTheUsername->GetValue().c_str());
#endif

	m_pEditInformalUsername->ChangeValue(m_pUserStructForeign->fullname);
	m_pEditPersonalPassword->ChangeValue(_T("")); // we can't recover it, so user must
			// look up his records if he can't remember what it was, too bad if he
			// didn't make any records! In that case, he has to set a new password
			// and inform the user of that new one about the change, after removing
			// the user and the recreating with a new password
	m_pCheckUserAdmin->SetValue(m_pUserStructForeign->useradmin);
	//m_pCheckKbAdmin->SetValue(m_pUserStruct->kbadmin);

	// Note 1: selecting in the list does NOT make any change to the user table in the mysql
	// database, nor does editing any of the control values. The user table is only
	// affected when the user clicks one of the buttons Add User, Remove User, or Change Permission,
	// and it is at that time that the controls' values are grabbed and used.

	// Note 2: Remove User does a real deletion; the entry is not retained in the user
	// table. But if the user owns entries in the KB database, then the attempt to delete
	// the user will fail - and a message will be seen. In this situation, just leave the
	// entry table alone - removing entries clobbers other people's work; and instead, 
	// just create a similar user with a new password and use that from then on.)
}

// BEW 16Dec20 next one, to get the m_pUsersListForeign and m_pOriginalUsersList arrays of
// UserListForeign pointers updated to match changes made by the button handlers after a
// new state for the user2 has been produced by the do_list_users.exe call after configuring
// for the commandLine for list_users case ( = 3), to achieve syncing without resort to
// another call of LoadDataForPage(0) -- internally it reads the returned results file from
// the kbserver's user table, and borrows existing calls to get the syncing done
void KBSharingMgrTabbedDlg::UpdateUserPage(CAdapt_ItApp* appPtr, wxString execPath, 
								wxString resultFile, wxArrayString* pArrLines)
{
	CAdapt_ItApp* pApp = appPtr;

	wxArrayString arrLines = *pArrLines;
		
	arrLines.Empty(); // empty contents, leaving it ready for reuse of a new set of line strings
	wxString pathToResults = execPath + resultFile; // absolute path to the Adapt It executable
	bool bResultsFileExists = ::FileExists(pathToResults);
	if (bResultsFileExists)
	{
		wxTextFile f(pathToResults);
		bool bOpened = f.Open();
		int lineIndex = 0; // ignore this one, it has "success in it" etc
		int lineCount = f.GetLineCount();

		wxString strCurLine = wxEmptyString;
		if (bOpened)
		{
			for (lineIndex = 1; lineIndex < lineCount; lineIndex++)
			{
				strCurLine = f.GetLine(lineIndex);
				arrLines.Add(strCurLine);
			}
		}
	}
	else
	{
		// results file does not exist, log the error
		wxBell();
		wxString msg = _T("UpdateUserPage(): .dat resultFile does not exist in the executable's folder: %s");
		msg = msg.Format(msg, execPath.c_str());
		pApp->LogUserAction(msg);
		return;
	}
	// Okay, the rows from the user table are in arrLines, as wxString vars.
	// Now clobber the contents of m_UsersListForeign to prepare for refilling
	// with altered contents after the user table was altered in some way
	pApp->m_pKbServer_ForManager->ClearUsersListForeign(m_pUsersListForeign); // empties
			// the contents, and frees their memory, but leaves m_pUsersListForeign
			// existing ready for new content to be added
	pApp->m_pKbServer_ForManager->ClearUsersListForeign(m_pOriginalUsersList); // empties
			// the contents, and frees their memory, but leaves m_pUsersListForeign
			// existing ready for new content to be added (m_pOriginalUsersList is on
			// the heap. OnOK() or OnCancel() will clear and delete it.
	// Now access the lines in arrLines above, turning them into a list of foreign user structs
	pApp->m_pKbServer_ForManager->ConvertLinesToUserStructs(arrLines, m_pUsersListForeign);
	// Copy the list, before user gets a chance to modify anything
	// param 1 is src list, param2 is dest list
	CopyUsersList(m_pUsersListForeign, m_pOriginalUsersList); // comparisons are tested against
					// whatever is in m_pOriginalUsersList after user does something in the
					// users page GUI of the KB Sharing Manager
	// Load the username strings into m_pUsersListBox, the ListBox is sorted;
	// Note: the client data for each username string added to the list box is
	// the ptr to the KbServerUser struct itself which supplied the username
	// string, the struct coming from m_pUsersList
	LoadUsersListBox(m_pUsersListBox, m_pUsersListForeign->size(), m_pUsersListForeign);

	// Finished. The users list in the GUI for the users page, should now be
	// in sync with what m_pUsersListForeign is now containing
}


// BEW 31Aug20 updated
void KBSharingMgrTabbedDlg::OnRadioButton1KbsPageType1(wxCommandEvent& WXUNUSED(event))
{
	// The user's click has already changed the value held by the radio button
	m_pRadioKBType1->SetValue(TRUE);
	m_pRadioKBType2->SetValue(FALSE);

	// Tell the app what value we have chosen
	m_pApp->m_bAdaptingKbIsCurrent = TRUE;

	// Set the appropriate label for above the listbox
	m_pAboveListBoxLabel->SetLabel(m_tgtListLabel);

	// Set the appropriate label text for the second text control
	m_pNonSrcLabel->SetLabel(m_tgtLanguageNameLabel);

	// Record which type of KB we are defining
	m_bKBisType1 = TRUE;

	// Get it's data displayed (each such call "wastes" one of the sublists, we only
	// display the sublist wanted, and a new ListKbs() call is done each time -- not
	// optimal for efficiency, but it greatly simplifies our code)
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnRadioButton1KbsPageType1(): This page # (m_nCurPage + 1) = %d"), m_nCurPage + 1);
#endif
	LoadDataForPage(m_nCurPage);
}

void KBSharingMgrTabbedDlg::OnRadioButton2KbsPageType2(wxCommandEvent& WXUNUSED(event))
{
	// The user's click has already changed the value held by the radio button
	m_pRadioKBType2->SetValue(TRUE);
	m_pRadioKBType1->SetValue(FALSE);

	// Tell the app what value we have chosen - Thread_DoEntireKbDeletion may
	// need this value
	m_pApp->m_bAdaptingKbIsCurrent = FALSE;

	// Set the appropriate label for above the listbox
	m_pAboveListBoxLabel->SetLabel(m_glsListLabel);

	// Set the appropriate label text for the second text control
	m_pNonSrcLabel->SetLabel(m_glossesLanguageNameLabel);

	// Record which type of KB we are defining
	m_bKBisType1 = FALSE;

	// Get it's data displayed (each such call "wastes" one of the sublists, we only
	// display the sublist wanted, and a new ListKbs() call is done each time -- not
	// optimal for efficiency, but it greatly simplifies our code)
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnRadioButton2KbsPageType2(): This page # (m_nCurPage + 1) = %d"), m_nCurPage + 1);
#endif
	LoadDataForPage(m_nCurPage);
}

/* BEW 31Aug20 remove from app
void KBSharingMgrTabbedDlg::OnBtnLanguagesPageLookupCode(wxCommandEvent& WXUNUSED(event))
{
	// Call up CLanguageCodesDlg_Single here so the user can choose the ISO639 language code
	// which is the first part of the custom code definition which he is about to create
	CLanguageCodesDlg_Single lcDlg(this);
	lcDlg.Center();

	lcDlg.m_langCode = m_pEditCustomCode->GetValue();

	int returnValue = lcDlg.ShowModal();
	if (returnValue == wxID_CANCEL)
	{
		// user cancelled
		return;
	}
	// user pressed OK so update the edit box & string members in the custom codes page
	m_customLangCode = lcDlg.m_langCode;
	m_pEditCustomCode->SetValue(m_customLangCode);
}
*/
/* BEW 31Aug20 remove, the KBs page no longer has a lookup for this, instead, remove it - we get data into the boxes directly
void KBSharingMgrTabbedDlg::OnBtnKbsPageLookupLanguageCodes(wxCommandEvent& WXUNUSED(event))
{
	// Call up CLanguageCodesDlg here so the user can enter language names for
	// the source and target languages, or for the source and glossing languages,
	// according to the LangNamesChoice modality.
	LangNamesChoice whichPairType = source_and_target_only;
	if (m_bKBisType1 != TRUE)
	{
		// It's not source-to-target that we are dealing with, but glossing mode
		// - that is, source-to-glosses, so choose the other enum value to pass in
		whichPairType = source_and_glosses_only;
	}
	CLanguageCodesDlg lcDlg(this, whichPairType); // make the parent be
								// SharedKBManager_CreateUsersPageFunc
	lcDlg.Center();

	if (whichPairType == source_and_target_only)
	{
		lcDlg.m_sourceLangCode = m_pEditSourceCode->GetValue();
		lcDlg.m_targetLangCode = m_pEditNonSourceCode->GetValue();
	}
	else if (whichPairType == source_and_glosses_only)
	{
		lcDlg.m_sourceLangCode = m_pEditSourceCode->GetValue();
		lcDlg.m_glossLangCode  = m_pEditNonSourceCode->GetValue();
	}
	else
	{
		// Hopefully, this block should never be entered
		m_pEditSourceCode->Clear();
		lcDlg.m_sourceLangCode.Empty();
		lcDlg.m_targetLangCode.Empty();
		m_pEditNonSourceCode->Clear();
		lcDlg.m_glossLangCode.Empty();
	}
	lcDlg.m_freeTransLangCode.Empty(); // not used in KB Sharing Manager gui

	int returnValue = lcDlg.ShowModal();
	if (returnValue == wxID_CANCEL)
	{
		// user cancelled
		return;
	}
	// user pressed OK so update the edit boxes & string members
	if (whichPairType == source_and_target_only)
	{
		m_sourceLangName = lcDlg.m_sourceLangCode;
		m_targetLangName = lcDlg.m_targetLangCode;
		m_pEditSourceCode->ChangeValue(m_sourceLangName);
		m_pEditNonSourceCode->ChangeValue(m_targetLangName);
	}
	else if (whichPairType == source_and_glosses_only)
	{
		m_sourceLangName = lcDlg.m_sourceLangCode;
		m_glossLangName = lcDlg.m_glossLangCode;
		m_pEditSourceCode->ChangeValue(m_sourceLangName);
		m_pEditNonSourceCode->ChangeValue(m_glossLangName);
	}
	else
	{
		// Indeterminate result, so just clear the boxes
		wxBell();
		m_pEditSourceCode->ChangeValue(_T(""));
		m_pEditNonSourceCode->ChangeValue(_T(""));
	}
}
*/
// Next function needs no changes to work with Page 2 or 3 of the GUI, only the input
// params change
// BEW 31Aug20 changed name from having 'Code' to 'Name' & some internal changes
void KBSharingMgrTabbedDlg::LoadLanguageNamePairsInListBox_KbsPage(bool bKBTypeIsSrcTgt,
				KbsList* pSrcTgtKbsList, KbsList* pSrcGlsKbsList, wxListBox* pListBox)
{
	KbsList* pKbsList = NULL;
	if (bKBTypeIsSrcTgt)
	{
		pKbsList = pSrcTgtKbsList;
	}
	else
	{
		pKbsList = pSrcGlsKbsList;
	}
	pListBox->Clear();
    // Make the listbox sorted, put a composite string in it, and store the KbServerKb
    // struct pointers as client data; then make it unsorted, and scan over it and read the
    // client data and make the string be just the source language code, and use the client
    // data to construct the right hand list of non-source language codes
	long mystyle = pListBox->GetWindowStyleFlag();
	// Make this listbox be sorted (we didn't make it be sorted in wxDesigner)
	mystyle = mystyle | wxLB_SORT | wxLB_SINGLE;
	pListBox->SetWindowStyleFlag(mystyle);

    // Now scan the list of KbServerKb struct pointers, construct an "entry" which is the
    // source language code followed by a comma followed by the target code (or gloss code,
    // as the case may be), and add the struct ptr as pClientData each time. This gets all
    // the data into the order we want the user to see.
 	KbsList::iterator iter;
	KbsList::compatibility_iterator c_iter;
	int anIndex = -1;
	for (iter = pKbsList->begin(); iter != pKbsList->end(); ++iter)
	{
		anIndex++;
		c_iter = pKbsList->Item((size_t)anIndex);
		KbServerKb* pEntry = c_iter->GetData();
		if (pEntry != NULL)
		{
			wxString compositeStr = pEntry->sourceLanguageName;
			compositeStr += _T(",");
			compositeStr += pEntry->targetLanguageName; // don't be misled: according to
					// bKBTypeIsSrcTgt's value, this might be the genuine tgt lang name, or
					// instead, the glossing language's name (the same variable name is
					// used for either in the KbServerKb struct)
			pListBox->Append(compositeStr, pEntry); // pEntry is not seen by user
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
    wxLogDebug(_T("LoadLanguageNamePairsInListBox(): compositeStr = %s , KbServerKb* pEntry = %p"),
		compositeStr.c_str(), pEntry);
#endif
		}
	}
 #if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	unsigned int count;
	count = pListBox->GetCount();
    wxLogDebug(_T("LoadLanguageNamePairsInListBox):  count for pListBox = %d"), count);
#endif
    // The list box is now populated, set no selection
	pListBox->SetSelection(wxNOT_FOUND);
}
/*
void KBSharingMgrTabbedDlg::OnBtnLanguagesPageRFC5646Codes(wxCommandEvent& WXUNUSED(event))
{
	// Display the RFC5646message.htm file in the platform's web browser
	DisplayRFC5646Message();
}

void KBSharingMgrTabbedDlg::OnBtnKbsPageRFC5646Codes(wxCommandEvent& WXUNUSED(event))
{
	// Display the RFC5646message.htm file in the platform's web browser
	DisplayRFC5646Message();
}
*/
#endif

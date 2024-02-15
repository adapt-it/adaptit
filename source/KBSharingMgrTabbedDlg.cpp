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

//#if defined(_KBSERVER)

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

//#include "C:\Program Files (x86)\MariaDB 10.5\include\mysql\mysql.h"
//#include "C:\Program Files (x86)\MariaDB 10.5\lib\"

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
	EVT_CHECKBOX(ID_CHECKBOX_USERADMIN, KBSharingMgrTabbedDlg::OnCheckboxUseradmin)
	EVT_BUTTON(ID_BUTTON_CHANGE_PERMISSION, KBSharingMgrTabbedDlg::OnButtonUserPageChangePermission)
	EVT_BUTTON(ID_BUTTON_CHANGE_FULLNAME, KBSharingMgrTabbedDlg::OnButtonUserPageChangeFullname)
	EVT_BUTTON(ID_BUTTON_CHANGE_PASSWORD, KBSharingMgrTabbedDlg::OnButtonUserPageChangePassword)
	EVT_BUTTON(wxID_OK, KBSharingMgrTabbedDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, KBSharingMgrTabbedDlg::OnCancel)

	END_EVENT_TABLE()

KBSharingMgrTabbedDlg::KBSharingMgrTabbedDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Shared Database Server Manager"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
#if defined  (_DEBUG)
	wxLogDebug(_T("\n\n********  KBSharingMgrTabbedDlg() line %d: ENTERING  **********"),__LINE__);
#endif

	m_pApp = &wxGetApp();
	if (m_pApp->m_bConfigMovedDatFileError == TRUE)
	{
		wxString msg = _("InitDialog() failed due to a problem creating the commandLine for \'lookup_user\'.\nOpening the KB Sharing Manager will not work until this problem is fixed.");
		wxString title = _("Looking up user - warning");
		wxMessageBox(msg, title, wxICON_INFORMATION | wxOK);
		m_pApp->LogUserAction(msg);
		return;
	}

	// wx Note: Since InsertPage also calls SetSelection (which in turn activates our OnTabSelChange
	// handler, we need to initialize some variables before CCTabbedNotebookFunc is called below.
	// Specifically m_nCurPage and pKB needs to be initialized - so no harm in putting all vars
	// here before the dialog controls are created via KBEditorDlgFunc.
	m_nCurPage = 0; // default to first page (i.e. Users page)

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
	m_pApp->m_bDoingChangeFullname = FALSE; // initialise -- needed by 
					// LoadDataForPage(), TRUE if change is requested
	m_bLegacyEntry = FALSE;  // re-entry to the Mgr in InitDialog will cause
							 // this to be made TRUE, and it will remain
							 // TRUE until the session ends, except when
							 // temporarily set FALSE when an operating functionality
							 // needs to avoid OnSelChange...() and m_mgr... arrays
							 // being accessed, in the legacy code in LoadDataForPage(0)
#if defined  (_DEBUG)
	wxLogDebug(_T("Init_Dialog(for Sharing Mgr) line %d: entering"), __LINE__);
#endif
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

	// whm 31Aug2021 modified 2 lines below to use the AutoCorrectTextCtrl class which is now
	// used as a custom control in wxDesigner's RetranslationDlgFunc() dialog.
	//m_pTheUsername = (AutoCorrectTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_THE_USERNAME); // BEW 17Nov21 temp remove the cast
	m_pTheUsername = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_THE_USERNAME);
#if (_DEBUG)
	wxTextCtrl* usernameCtrlPtr = m_pTheUsername; // temp
	wxString aUserNm = usernameCtrlPtr->GetValue();
#endif
	wxASSERT(m_pTheUsername != NULL);
	// m_pEditInformalUsername = (AutoCorrectTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_INFORMAL_NAME); // BEW 17Nov21 temp remove the cast
	m_pEditInformalUsername = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_INFORMAL_NAME);
#if (_DEBUG)
	wxTextCtrl* informal_usernameCtrlPtr = m_pEditInformalUsername; // temp
	wxString anInformalNm = informal_usernameCtrlPtr->GetValue();
#endif

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
	m_pBtnChangePermission = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_CHANGE_PERMISSION);
	wxASSERT(m_pBtnChangePermission != NULL);
	// Set an appropriate 'ForManager' KbServer instance which we'll need for the
	// services it provides regarding access to the KBserver
	// We need to use app's m_pKbServer_ForManager as the pointer, but it should be NULL
	// when the Manager is created, and SetKbServer() should create the needed 'ForManager'
	// instance and assign it to our m_pKbServer in-class pointer. So check for the NULL;
	// if there is an instance currently, we'll delete that and recreate a new one on heap
	// to do that
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

	// BEW 17Dec20 Need initial call of ListUsers() so that the m_pUsersListBox can get
	// populated before the GUI shows the Mgr to the user.
	// NOTE: InitDialog() is called BEFORE the creator for the Mgr class. So the code
	// for testing if user2 (the user looked up) has useradmin permision == 1 has to
	// be here; and so the rejection of unauthorized users has to be done here

	//First, set these app booleans, so that ConfigureMovedDatFile() accesses the needed
	// block for setting commandLine for this situation
	// set the m_bUser1IsUser2 boolean to TRUE, because we are contraining entry to a
	// lookup_user case ( = 2) in which user1 == user2 == app's m_strUserID (the project's user)
	m_pApp->m_bUser1IsUser2 = TRUE;	// (ConfigureMovedDatFile() asserts if it is FALSE)
	m_pApp->m_bWithinMgr = FALSE; // we aren't in the GUI yet
	m_pApp->m_bKBSharingMgrEntered = TRUE;
	m_pApp->m_bHasUseradminPermission = TRUE; // must be TRUE, otherwise ListUsers() aborts
	m_pApp->m_bConfigMovedDatFileError = FALSE; // initialize to "no ConfigureMovedDatFile() error"

	// BEW 16Dec20 Two accesses of the user table are needed. The first, here, LookupUser() 
	// determines whether or not user2 exists in the user table, and what that user's 
	// useradmin permission level is. This check is not needed again if the user has gained 
	// access to the manager, since within the manager the list of users may be altered
	// and require a new call of ListUsers() be made from there.
	// (Only call LoadDataForPage(0) once. When entering to the manager. Thereafter,
	// keep the kbserver user table in sync with the user page by a function, which reads 
	// the results file of do_list_users.exe and updates the m_pOriginalUsersList and 
	// m_pUsersListForeign 'in place' with an Update....() function

/*
#if defined  (_DEBUG)
	wxLogDebug(_T("ConfigureDATfile(lookup_user = %d) line %d: entering, in Mgr InitDialog()"),
			lookup_user, __LINE__);
#endif
	bool bExecutedOK = FALSE;
	bool bReady = m_pApp->ConfigureDATfile(lookup_user); // case = 2
	if (bReady)
	{
		// The input .dat file is now set up ready for do_lookup_user.exe
		wxString execFileName = _T("do_lookup_user.exe");
		wxString execPath = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator; // whm 22Feb2021 changed execPath to m_appInstallPathOnly + PathSeparator
		wxString resultFile = _T("lookup_user_results.dat");
#if defined  (_DEBUG)
		wxLogDebug(_T("CallExecute(lookup_user = %d) line %d: entering, in Mgr creator"),
			lookup_user, __LINE__);
#endif
		bExecutedOK = m_pApp->CallExecute(lookup_user, execFileName, execPath, resultFile, 32, 33, TRUE);
		// In above call, last param, bReportResult, is default FALSE therefore omitted;
		// wait msg 32 is _("Authentication succeeded."), and msg 33 is _("Authentication failed.")
#if defined (_DEBUG)
		bool bPermission = m_pApp->m_bHasUseradminPermission;
		wxUnusedVar(bPermission);
#endif
		// Disallow entry, if user has insufficient permission level
		if (bExecutedOK && !m_pApp->m_bHasUseradminPermission)
		{
			wxString msg = _("Access to the Knowledge Base Sharing Manager denied. Insufficient permission level, 0. Choose a different user having level 1.");
			wxString title = _("Warning: permission too low");
			wxMessageBox(msg, title, wxICON_WARNING & wxOK);
			m_bAllow = FALSE; // enables caller to cause exit of the Mgr handler
			return;
		}
		else
		{
			if (bExecutedOK)
			{
				m_pApp->m_bHasUseradminPermission = TRUE;
			}
		}
		// m_bHasUseradminPermission has now been set, if LookupUser() succeeded, so
		// get what its access permission level is. TRUE allows access to the manager.
		// (in the entry table, '1' value for useradmin)
		m_pApp->m_bConfigMovedDatFileError = FALSE; // "no error"

	} // end of TRUE block for test: if (bReady)
	else
	{
		// return TRUE for the bool below, caller will warn user, and caller's
		// handler will abort the attempt to create the mgr
		m_pApp->m_bConfigMovedDatFileError = TRUE;
		return;
	}
*/

	// BEW 9Jan24 updated comment: First, before dealing with the attempt to list users, which is
	// relevant to authorizing to enter the KB Sharing Manager, to it's LoadDataForPage(0) pane; 
	// we must find out whether the user trying to log in is credentialed to
	// be able to add other users. This depends on that user's  useradmin value, in the user table.
	// A '1' value will allow adding others; a '0' value will allow access to the user page, but 
	// disallow two things:  the Add User button, and the Change Permission button. 
	// (But a 0-valued user can still change password or fullname, and can work under a different
	// username in different AI project (i.e. different language names - or at least one different name)
	// provided sharing on the same LAN, and that username can access to kbserver (ie. name is listed in user table)

	m_pApp->RemoveDatFileAndEXE(lookup_user); // BEW 11May22 added, must precede call of ConfigureDATfile()
	bool bUserLookupSucceeded = m_pApp->ConfigureDATfile(lookup_user);
	if (bUserLookupSucceeded)
	{
		// The input .dat file is now set up ready for do_lookup_user.exe
		m_pApp->m_server_username_looked_up = m_pApp->m_strUserID;
		m_pApp->m_server_fullname_looked_up = m_pApp->m_strFullname;

		// open the lookup_user_results.dat file, get its one line contents into a wxString

		wxString resultFile = _T("lookup_user_results.dat");
		wxString datPath = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator + resultFile;
		bool bExists = ::FileExists(datPath);
		wxTextFile f;
		wxString myResults = wxEmptyString; // initialise
		if (bExists)
		{
			bool bOpened = f.Open(datPath);
			if (bOpened)
			{
				myResults = f.GetFirstLine();
				myResults = MakeReverse(myResults);
				// myResults now should be either (a) ",1,.....") or (b) ",0,...."
				// Remove the initial comma, if present
				wxString commaStr = _T(",");
				// BEW 29Oct22 protect call of Get Char(0)
				wxChar firstChar = (wxChar)0;
				if (!myResults.IsEmpty())
				{
					firstChar = myResults.GetChar(0);

					if ((firstChar != _T('\0')) && firstChar == commaStr)
					{
						myResults = myResults.Mid(1);
					}
					// Okay, '0' or '1' is next - which is it?
					wxChar permissionChar = myResults.GetChar(0);


					// Now store it where it will be available for the list_users case
					f.Close();
					// save the permission value where the list_users case (here) can access it below
					m_pApp->m_server_useradmin_looked_up = permissionChar;  // saves '1' or '0' as wxChar
					// and on the app too
					m_pApp->m_bHasUseradminPermission = permissionChar == _T('1') ? TRUE : FALSE;

					// disallow, if user does not have useradmin == 1 permission
					if (bUserLookupSucceeded && !m_pApp->m_bHasUseradminPermission)
					{
						/* BEW comment out 9Jan24 - we now let such users in, but disable adding users or changing permission to such users
						wxString msg = _("Access to the Knowledge Base Sharing Manager denied. Insufficient permission level, 0. Choose a different user having level 1.");
						wxString title = _("Warning: permission too low");
						wxMessageBox(msg, title, wxICON_WARNING & wxOK);
						m_pApp->LogUserAction(msg);
						m_bAllow = FALSE; // enables caller to cause exit of the Mgr handler
						return;
						*/
						m_pBtnUsersAddUser->Enable(FALSE);
						m_pBtnChangePermission->Enable(FALSE);
					}
					else
					{
						m_pApp->m_bHasUseradminPermission = TRUE;
						m_pBtnUsersAddUser->Enable();
						m_pBtnChangePermission->Enable();
					}
				} // end of TRUE block for test: if (!myResults.IsEmpty())
				else
				{
					// Default to no user permission
					m_pApp->m_bHasUseradminPermission = FALSE;
					wxString msg = _("Looking up user for the Tabbed Manager, line %d, failed - the string myResults was empty for user: %s");
					msg = msg.Format(msg, __LINE__, m_pApp->m_strUserID.c_str());
					m_pApp->LogUserAction(msg);
				}
			} // end of TRUE block for test: if (bOpened)
			else
			{
				// Unable to open file
				m_pApp->m_server_username_looked_up = m_pApp->m_strUserID;
				m_pApp->m_server_fullname_looked_up = m_pApp->m_strFullname;
				m_pApp->m_server_useradmin_looked_up = _T('0');  // '1' or '0' as wxChar
				wxString title = _("Unable to open file");
				wxString msg = _("Looking up user \"%s\" failed when attempting to get useradmin value (0 or 1), in support of ListUsers");
				msg = msg.Format(msg, m_pApp->m_strUserID.c_str());
				m_pApp->LogUserAction(msg);
			}
		}
		else
		{
			// Unlikely to fail, if it does, take the 'play safe' option below
			m_pApp->m_server_username_looked_up = m_pApp->m_strUserID;
			m_pApp->m_server_fullname_looked_up = m_pApp->m_strFullname;
			m_pApp->m_server_useradmin_looked_up = _T('0');  // '1' or '0' as wxChar
			wxString title = _("User not found in the KBserver user table");
			wxString msg = _("Looking up user \"%s\" failed when attempting to get useradmin value (0 or 1), in support of ListUsers");
			msg = msg.Format(msg, m_pApp->m_strUserID.c_str());
			m_pApp->LogUserAction(msg);
			m_pBtnUsersAddUser->Enable(FALSE);
			m_pBtnChangePermission->Enable(FALSE);
			m_pApp->m_bHasUseradminPermission = FALSE;

		}
	}
	else
	{
		// Play safe. Disallow access to the KB Sharing Manager
		m_pApp->m_server_username_looked_up = m_pApp->m_strUserID;
		m_pApp->m_server_fullname_looked_up = m_pApp->m_strFullname;
		m_pApp->m_server_useradmin_looked_up = _T('0');  // '1' or '0' as wxChar
		wxString title = _("Lookup of username failed");
		wxString msg = _("Looking up user \"%s\" failed when attempting to get useradmin value (0 or 1), in support of ListUsers");
		msg = msg.Format(msg, m_pApp->m_strUserID.c_str());
		m_pApp->LogUserAction(msg);
	}
// REPLACE END

	m_pApp->m_bDoingChangeFullname = FALSE; // initialize
	m_pApp->m_bUseForeignOption = TRUE; // we use this to say "The KB Sharing Mgr is where control is"
	m_pApp->m_bWithinMgr = FALSE; // control is not inside yet, InitDialog() will set it TRUE
								  // if the access is granted to the user page
	// Entry has happened, use the TRUE value in ConfigureMovedDatFile() for case lookup_user
	// to ensure the constraint, user1 == user2 == m_strUserID is satisfied for the lookup_user
	// case just a bit below, as refactoring I've removed the need to have Authenticate2Dlg
	// called in order to open the Mgr. Looking at other usernames than m_strUserID from the
	// currently open shared project, is a job for the internals of the Mgr, once opened
	m_pApp->m_bKBSharingMgrEntered = TRUE;


	// These are reasonable defaults for initialization purposes
	if (m_pApp->m_strKbServerIpAddr != m_pApp->m_chosenIpAddr)
	{
		m_pApp->m_pKbServer_ForManager->SetKBServerIpAddr(m_pApp->m_chosenIpAddr);
	}
	else
	{
		m_pApp->m_pKbServer_ForManager->SetKBServerIpAddr(m_pApp->m_strKbServerIpAddr);
	}
	if (m_pApp->m_curNormalUsername != m_pApp->m_strUserID)
	{
		m_pApp->m_pKbServer_ForManager->SetKBServerUsername(m_pApp->m_strUserID);
		m_pApp->m_DB_username = m_pApp->m_strUserID; // pass this to do_list_users.dat
	}
	else
	{
		m_pApp->m_pKbServer_ForManager->SetKBServerUsername(m_pApp->m_curNormalUsername);
		m_pApp->m_DB_username = m_pApp->m_curNormalUsername; // pass this to do_list_users.dat
	}

	m_pApp->m_pKbServer_ForManager->SetKBServerPassword(m_pApp->m_curAuthPassword);
	if (m_pApp->m_DB_password.IsEmpty())
	{
		m_pApp->m_DB_password = m_pApp->m_curAuthPassword; // pass this to do_list_users.dat
	}


	// Get the list of users into the returned list_users_results.dat file
	int rv = m_pApp->m_pKbServer_ForManager->ListUsers(m_pApp->m_strKbServerIpAddr, m_pApp->m_DB_username, m_pApp->m_DB_password);
	wxUnusedVar(rv);
	wxASSERT(rv == 0);

	// Initialize the User page's checkboxes to OFF
	m_pCheckUserAdmin->SetValue(FALSE);

	// Hook up to the m_usersList member of the KbServer instance
	//m_pUsersListForeign = m_pKbServer->GetUsersListForeign(); // an accessor for m_usersListForeign

	// Add the kbserver's ipAddr to the static text 2nd from top of the tabbed dialog
	wxString myStaticText = m_pConnectedTo->GetLabel();
	myStaticText += _T("  "); // two spaces, for ease in reading the value

	wxString theIpAddr = m_pKbServer->GetKBServerIpAddr();

	myStaticText += theIpAddr; // an accessor for m_kbServerIpAddrBase
	m_pConnectedTo->SetLabel(myStaticText);

	// BEW 13Nov20 changed, 
	m_bKbAdmin = TRUE;  // was m_pApp->m_kbserver_kbadmin; // BEW always TRUE now as of 28Aug20
	m_bUserAdmin = TRUE; // was m_pApp->m_kbserver_useradmin; // has to be TRUE for anyone getting access

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

// This is called each time the page to be viewed is switched to
void KBSharingMgrTabbedDlg::LoadDataForPage(int pageNumSelected)
{
	if (pageNumSelected == 0) // Users page
	{
		// BEW 21Dec23 the users page has buttons for changing fulname, pwd or useradmin. When one
		// of these buttons is pressed, the index for which field to change in the users' page four
		// arrays, for user, or fullname, or pwd, or useradmin will range between [1 and 3], but
		// the index for which user was clicked is given by app's m_nMgrSel value - which could be
		// must more than 3. So we need to set a correct array index for each of the 4 arrays, here
		// (Note, changing the clicked user's pwd is not allowed, if that user is the authenticated one)
		m_nIndexOfChangedField = -1; // init
		if (m_pApp->m_bDoingChangeFullname)
		{
			m_nIndexOfChangedField = 1;
		}
		else if (m_pApp->m_bDoingChangePassword)
		{
			m_nIndexOfChangedField = 2;
		}
		else if (m_pApp->m_bChangingPermission)
		{
			// There's no m_bChangingPermission in AI.h, is at line 4071
			m_nIndexOfChangedField = 3;
		}
			
		// Setting m_pApp->m_bChangePermission_DifferentUser to TRUE is
		// incompatible with an operation different than changing some
		// other parameter, so check and hack it to FALSE before making
		// the test following
		if (m_pApp->m_bDoingChangeFullname)
		{
			// The following assures that LoadDataForPage will not 
			// unnecessarily reset the useradmin checkbox further below
			m_pApp->m_bChangingPermission = FALSE;
			m_pApp->m_bChangePermission_DifferentUser = FALSE;
		}
#if defined (_DEBUG)
		wxLogDebug(_T("/n%s::%s(), line %d : PreTest: m_bDoingChangeFullname %d , m_bChangingPermission %d , m_bChangePermission_DifferentUser %d"),
			__FILE__, __FUNCTION__, __LINE__, (int)m_pApp->m_bDoingChangeFullname, 
			(int)m_pApp->m_bChangingPermission, (int)m_pApp->m_bChangePermission_DifferentUser);
#endif
		// BEW 8Jan21 NOTE the test carefully, if user is re-entering the KB Sharing Mgr
		// more than once, each time m_pApp->m_nMgrSel must have previously been set to
		// -1 (wxNOT_FOUND), so that control enters the TRUE block of the compound test
		// below. Failure to reset to -1 will send control to the else block where the
		// there is no attempt to access the arrays for username, fullname and password,
		// with the result that the manager would open with an EMPTY LIST - and not be
		// able to do a thing. So, OnOK() and OnCancel() reinitialize m_nMgrSel to -1
#if defined (_DEBUG)
		wxLogDebug(_T("%s::%s(), line %d : PreTest: m_pApp->m_nMgrSel = %d  <<-- IT MUST BE -1 HERE"),
			__FILE__, __FUNCTION__, __LINE__, m_pApp->m_nMgrSel);
#endif
		if (m_pApp->m_nMgrSel == wxNOT_FOUND)
		{
			// The manager is being re-entered
			m_bLegacyEntry = TRUE;
		}
		else
		{
			// m_nMgrSel is not -1, so...
			// LoadDataForPage is being called during a Manager session, this can
			// happen multiple times, and each time m_bLegacyEntry should be TRUE
			// except when "Change Permission" is the functionality currently
			// in effect. Implement this protocol in this else block. The end of
			// LoadDataForPage() will reinstate m_bLegacyEntry reset to TRUE

			// Which one(s) require no legacy code access? Bleed those out in the
			// TRUE block here; the else block then sets every other functionality
			// to have legacy access to the full works
			if (m_pApp->m_bChangingPermission == TRUE)
			{
				// So far, this is the only one requiring no access to the legacy code
				m_bLegacyEntry = FALSE;
			}
			else
			{
				// The other functionalities will want legacy code access
				m_bLegacyEntry = TRUE;
			}
		}
		// The TRUE block for this test has control do the full works, setting the
		// commandLine for wxExecute() call, getting the returned .dat file's data
		// extracted into app's m_mgr.... set of arrays, calling OnSelchangeUsersList()
		// writing the various string fields to their GUI wxTextCtrl members, and then
		// calling LoadDataForPage() to get the Mgr user page updated	
		if ( m_bLegacyEntry == TRUE)
		{
			// Clear out anything from a previous button press done on this page
			// ==================================================================
			m_pUsersListBox->Clear();
			wxCommandEvent dummy;
			//OnButtonUserPageClearControls(dummy);

			m_pApp->EmptyMgrArraySet(TRUE); // bNormalSet is TRUE  for, m_mgrUsernameArr etc
			m_pApp->EmptyMgrArraySet(FALSE); // bNormalSet is FALSE  for, m_mgrSavedUsernameArr etc

			// Adding a new user, need to add the user line here (manually)
			if (!m_pApp->m_strNewUserLine.IsEmpty())
			{
				m_pApp->m_arrLines.Add(m_pApp->m_strNewUserLine);
			}
#if defined (_DEBUG)
			wxLogDebug(_T("%s::%s() line %d: m_arrLines line count = %d"), 
				__FILE__,__FUNCTION__,__LINE__,m_pApp->m_arrLines.GetCount());
#endif
			m_pApp->ConvertLinesToMgrArrays(m_pApp->m_arrLines); // data may require tweaking before logging

			if (m_pApp->m_arrLines.IsEmpty())
			{
				m_pApp->m_nMgrSel = wxNOT_FOUND; // no selection index could be valid
												 // and don't try to FillUserList()
			}
			else
			{
				// There are lines to get and use for filling the list,
				// and m_nMgrSel will have a non negative value
				FillUserList(m_pApp);
			}

			// Each LoadDataForPage() has to update the useradmin checkbox value, 
			// and fullname value, and username value
			if (m_pApp->m_nMgrSel != wxNOT_FOUND)
			{
				// Get the username
				wxString username;
				wxString fullname;
				int theUseradmin = -1;
				if (!m_pApp->m_mgrUsernameArr.IsEmpty())
				{
					username = m_pApp->m_mgrUsernameArr.Item(m_pApp->m_nMgrSel);
					// select the username in the list
					int seln = m_pUsersListBox->FindString(username);
					// Make sure app's member agrees
					wxASSERT(m_pApp->m_nMgrSel == seln);
					m_pApp->m_nMgrSel = seln;
					// select the list item
					m_pUsersListBox->SetSelection(seln);

					// Put the selected username into the m_pTheUsername wxTextCrl box
					m_pTheUsername->ChangeValue(username);
				}

				// Put the associated fullname value into it's box, or if change of
				// fullname was requested, it will already by reset - in which case
				// check it's correct
				if (m_pApp->m_bDoingChangeFullname)
				{
					// Get the wxTextCtrl's updated value
					wxString strFullname = m_pEditInformalUsername->GetValue();
					wxASSERT(m_pApp->m_nMgrSel != wxNOT_FOUND);
					//m_pApp->m_mgrPasswordArr.RemoveAt(m_pApp->m_nMgrSel); // BEW 21Dec23 if Change Fullname btn was
					// pressed, there's no reason for removing that user's password from m_mgrPasswordArr - because doing
					// so loses the user's pwd value, and reduces the AI.h/s m_mgrPasswordArr's count by 1, leading to
					// an app crash if the clicked user's Show Password button is clicked, due to: nIndex  < m_nCount.

					// BEW 21Dec23 a second error to the first just above is the next line. strFullname has been correctly
					// set, but doing insert (e.g. for selection = 2 in a 3 line user array) pushes the earlier fullname 
					// value to the right, and so adds an extra line to the m_mgrFullnameArr array, making it a 4-line array.
					// Fortunately, that pushed-right value would not get accessed, but its a logic error. We need to replace
					// it with the new strFullname value. So fix that here
					//m_pApp->m_mgrFullnameArr.Insert(strFullname, m_pApp->m_nMgrSel);
					m_pApp->m_mgrFullnameArr.RemoveAt(m_nIndexOfChangedField,1);
					m_pApp->m_mgrFullnameArr.Insert(strFullname, m_nIndexOfChangedField, 1);
				}
				else
				{
					fullname = m_pApp->m_mgrFullnameArr.Item(m_pApp->m_nMgrSel);
					m_pEditInformalUsername->ChangeValue(fullname);
				}

				if (m_pApp->m_bDoingChangePassword && (m_pApp->m_nMgrSel != -1))
				{
					m_pEditShowPasswordBox->Clear();
					wxString strPwd = m_pApp->m_ChangePassword_NewPassword; // stored here 
																// after wxExecute() done
					// We'll not show it changed, but require a "Show Password" click
					// to get the new value displayed. But to display it, it first
					// has to be lodged in the relevant m_mgr....Array of AI.h
					wxASSERT(m_pApp->m_nMgrSel != wxNOT_FOUND);
					// BEW 21Dec23, next two lines wrong, replacement requires index of 2 for a pwd replacement
					// but m_nMgrSel could be a value larger than 2, or might be 1. m_nIndexOfChangedField is 
					// set at top of LoadDataForPage(0)
					//m_pApp->m_mgrPasswordArr.RemoveAt(m_pApp->m_nMgrSel);
					//m_pApp->m_mgrPasswordArr.Insert(strPwd, m_pApp->m_nMgrSel);
					m_pApp->m_mgrPasswordArr.RemoveAt(m_nIndexOfChangedField, 1);
					m_pApp->m_mgrPasswordArr.Insert(strPwd, m_nIndexOfChangedField, 1);

				}

				// Set the useradmin checkbox's value to what it should be, if
				// it needs changing - it's a toggle, so simpler
				if (m_pApp->m_bChangingPermission && m_pApp->m_bChangePermission_DifferentUser)
				{
					theUseradmin = m_pApp->m_mgrUseradminArr.Item(m_pApp->m_nMgrSel);
					bool bUsrAdmin = theUseradmin == 1 ? FALSE : TRUE; // flip the bool value
					m_pCheckUserAdmin->SetValue(bUsrAdmin);
				}

#if defined (_DEBUG)
				wxString dumpedUsers = m_pApp->DumpManagerArray(m_pApp->m_mgrUsernameArr);
				wxString dumpedFullnames = m_pApp->DumpManagerArray(m_pApp->m_mgrFullnameArr);
				wxString dumpedPasswords = m_pApp->DumpManagerArray(m_pApp->m_mgrPasswordArr);
				wxString dumpedUseradmins = m_pApp->DumpManagerUseradmins(m_pApp->m_mgrUseradminArr);

				wxLogDebug(_T("User_Names_Page, line %d  usernames: %s"), __LINE__, dumpedUsers.c_str());
				wxLogDebug(_T("User_Names_Page, line % d  fullnames : % s"), __LINE__, dumpedFullnames.c_str());
				wxLogDebug(_T("User_Names_Page, line %d  passwords: %s"), __LINE__, dumpedPasswords.c_str());
				wxLogDebug(_T("User_Names_Page, line %d  useradmins: %s"), __LINE__, dumpedUseradmins.c_str());

				// BEW 21Dec23 counts for each of the arrays
				wxLogDebug(_T("User_Names_Page, line %d  usernames m_nCount: %d "), __LINE__, m_pApp->m_mgrUsernameArr.GetCount());
				wxLogDebug(_T("User_Names_Page, line %d  fullnames  m_nCount: %d "), __LINE__, m_pApp->m_mgrFullnameArr.GetCount());
				wxLogDebug(_T("User_Names_Page, line %d  passwords m_nCount: %d "), __LINE__, m_pApp->m_mgrPasswordArr.GetCount());
				wxLogDebug(_T("User_Names_Page, line %d  useradmins m_nCount: %d "), __LINE__, m_pApp->m_mgrUseradminArr.GetCount());
				// And for the "saved" set
				wxLogDebug(_T("User_Names_Page, line %d  SAVED usernames m_nCount: %d "), __LINE__, m_pApp->m_mgrSavedUsernameArr.GetCount());
				wxLogDebug(_T("User_Names_Page, line %d  SAVED fullnames  m_nCount: %d "), __LINE__, m_pApp->m_mgrSavedFullnameArr.GetCount());
				wxLogDebug(_T("User_Names_Page, line %d  SAVED passwords m_nCount: %d "), __LINE__, m_pApp->m_mgrSavedPasswordArr.GetCount());
				wxLogDebug(_T("User_Names_Page, line %d  SAVED useradmins m_nCount: %d "), __LINE__, m_pApp->m_mgrSavedUseradminArr.GetCount());

#endif

			} // end TRUE block for test: if (m_pApp->m_nMgrSel != wxNOT_FOUND)

			// If a new user was added manually above, we need to empty the
			// line added to m_arrLines, and clear out the two password
			// wxTextCtrl boxes in the dialog's centre column
			if (!m_pApp->m_strNewUserLine.IsEmpty())
			{
				wxString empty = wxEmptyString;
				m_pApp->m_strNewUserLine.Empty();
				m_pEditPersonalPassword->ChangeValue(empty);
				m_pEditPasswordTwo->ChangeValue(empty);
			}
		} // end of TRUE block for test: if ( m_bLegacyEntry == TRUE)
		else
		{
#if defined (_DEBUG)		
			wxLogDebug(_T("/n%s::%s(), line %d : ELSE block, entry here when not doing 'change permission' is a BUG"),
				__FILE__, __FUNCTION__,__LINE__);
#endif
			// When the before and after .py execution's user2 values are the same,
			// nothing much needs to be done, because the checkbox in the GUI has
			// already been flipped to the new value. But that value has not gotten
			// into app's store of the useradmin values, so we do it here
			bool bCurrentValue = m_pCheckUserAdmin->GetValue(); // whether TRUE or FALSE
			// Update the m_mgrUseradminArr to have this value
			int nCurrentValue = bCurrentValue == TRUE ? 1 : 0;
			if (m_pApp->m_nMgrSel != -1)
			{
				// Remove the old value stored
				m_pApp->m_mgrUseradminArr.RemoveAt(m_pApp->m_nMgrSel);
				// Insert nNewValue at the same location
				m_pApp->m_mgrUseradminArr.Insert(nCurrentValue, m_pApp->m_nMgrSel);
#if defined (_DEBUG)
				wxString dumpedUseradmins = m_pApp->DumpManagerUseradmins(m_pApp->m_mgrUseradminArr);
				wxLogDebug(_T("%s::%s(), line %d  useradmins: %s"), __FILE__, __FUNCTION__, 
					__LINE__, dumpedUseradmins.c_str());
#endif
			}
			else
			{
				wxBell();
				// we don't expect this sync error, so just put a message in LogUserAction(), and
				// things should get right eventually, especially if use shuts down and reopens, 
				// and tries again
				wxString msg = _T("LoadDataForPage() line %s: m_nMgrSel value is wrongly -1 in KB Sharing Manager");
				msg = msg.Format(msg, __LINE__);
				m_pApp->LogUserAction(msg);
			}
		} // end of else block for test: if ( m_bLegacyEntry == TRUE)

		if (m_bClearUseradminCheckbox == TRUE)
		{
			// clear the Useradmin checkbox
			bool bChecked = m_pCheckUserAdmin->IsChecked();
			if (bChecked)
			{
				m_pCheckUserAdmin->SetValue(FALSE);
				m_bClearUseradminCheckbox = FALSE; // ready for another change_permission attempt
			}
		}
	} // end of TRUE block for test: if (pageNumSelected == 0)

	m_pApp->m_bDoingChangeFullname = FALSE; // re-initialise pending a new request for fullname change
	m_pApp->m_bDoingChangePassword = FALSE; // restore default
	m_pApp->m_bChangingPermission = FALSE; // restore default
	m_bLegacyEntry = TRUE; // reset default value
}

void KBSharingMgrTabbedDlg::OnButtonUserPageChangePermission(wxCommandEvent& WXUNUSED(event))
{
	m_pApp->m_bChangingPermission = TRUE; // BEW 7Jan21 added
	// Get the selected (in list) username that was clicked e.g. glenys@Unit2
	m_pApp->m_Username2 = m_pTheUsername->GetValue();

	// BEW 29Dec23, we should disallow changing the permission value when m_Username2 is the same
	// user as the currenly active user (ie. when same as m_pApp->m_strUserID), because if that user
	// has useradmin value of 1 (usually would be so, but could be 0) then the active user would be
	// unable to do anything in the KB Sharing Manager other than look at the users list
	if (m_pApp->m_Username2 == m_pApp->m_strUserID)
	{
		wxString msg = _("You are trying to change the permission value for the active user. This is not allowed for active user: %s");
		msg = msg.Format(msg, m_pApp->m_strUserID.c_str());
		wxString title = _("Illegal permission change attempt");
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		m_pApp->m_bChangingPermission = FALSE;
		wxCommandEvent dummyevent;
		OnButtonUserPageClearControls(dummyevent);
		return;
	}

	// we don't need the useradmin value for that user, if we did, it's in the storage arrays in pApp
	int selIndex = m_pApp->m_nMgrSel; // index in the list to the selected user for which changes are to be made
	wxUnusedVar(selIndex); // don't think I need it for this handler
	//int User2UseradminFlag = m_pApp->m_mgrSavedUseradminArr.Item(selIndex); // see AI.h line 2446 No, get it from the user table for that user
	// Now build the commandLine in a local wxString, and save to m_pApp->m_commandLine_forManager when it's done
	wxString comma = _T(",");
	localCmdLineStr = wxEmptyString; // build commandLine here...
	localCmdLineStr = m_pApp->m_chosenIpAddr + comma;

	wxString tempStr;
	tempStr = m_pApp->m_strUserID;
	if (m_pApp->m_curNormalUsername.IsEmpty())
	{
		m_pApp->m_curNormalUsername = tempStr;
	}
	localCmdLineStr += tempStr + comma; // has any embedded ' escaped
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	if (m_pApp->m_curNormalPassword.IsEmpty())
	{
		m_pApp->m_curNormalPassword = pFrame->GetKBSvrPassword(); // gets m_kbserverPassword string
	}
	localCmdLineStr += m_pApp->m_curNormalPassword + comma;
	// Finally add the 2nd user - the one clicked
	localCmdLineStr += m_pApp->m_Username2 + comma;
	// Note, the useradmin flag is integer, not _T('0') or _T('1'), and it's present value gets
	// grabbed from the second user's entry in the user table when do_change_permission.exe runs

	// Put a copy of this commandLine on the app, where it can be accessed by the various processing
	// functions which are on the app
	m_pApp->m_commandLine_forManager = localCmdLineStr;

	m_pApp->RemoveDatFileAndEXE(change_permission); // BEW 11May22 added, must precede call of ConfigureDATfile()
	bool bReady = m_pApp->ConfigureDATfile(change_permission); // arg is const int, value 10
	if (bReady)
	{
		// The input .dat file is now set up ready for do_change_permission.exe
		wxString execFileName = _T("do_change_permission.exe");
		wxString execPath = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator; // whm 22Feb2021 changed execPath to m_appInstallPathOnly + PathSeparator
		wxString resultFile = _T("change_permission_results.dat");
		bool bExecutedOK = m_pApp->CallExecute(change_permission, execFileName, execPath, resultFile, 99, 99, FALSE); // bReportResult is FALSE
		if (!bExecutedOK)
		{
			// error in the call, inform user, and put entry in LogUserAction() - English will do
			wxString msg = _T("Line %d: CallExecute for enum: change_permission, failed - perhaps input parameters (and/or password) did not match any entry in the user table; Adapt It will continue working ");
			msg = msg.Format(msg, __LINE__);
			wxString title = _T("Probable do_change_permission.exe error");
			wxMessageBox(msg, title, wxICON_WARNING | wxOK);
			m_pApp->LogUserAction(msg);
		}

		// At this point, the user table is altered, so it just remains to
		// use LoadDataForPage() and make the GUI conform to what was done - well almost, see below
		//LoadDataForPage(0); <<-- put it later, at it's end clear the Useradmin checkbox if permission was reset to 0
		
		// BEW 28Dec23 a successful CallExecute() to change a user's permission from 1 to 0, leaves the
		// UserAdministrator checkbox ticked (ie. suggesting useradmin is 1, when it's actually been changed
		// to 0. So I need to detect this, and clear the checkbox. I can get the useradmin value from the
		// results.dat file, which will end in either "0," or "1,", provided the line starts with "success".
		m_bClearUseradminCheckbox = FALSE; // init, public member added by BEW 28Dec23
		wxString resultsPath = execPath + resultFile;
		bool bResultsFileExists = ::FileExists(resultsPath);
		wxString firstLineStr = wxEmptyString; // init
		if (bResultsFileExists)
		{
			// The file exists, so open it and check for an empty initial line
			wxTextFile f(resultsPath);
			bool bOpened = f.Open();
			if (bOpened)
			{
				// It was successfully opened, so get it's line count
				size_t count = f.GetLineCount();
				if (count > 0)
				{
					// There is at least one line, - get it
					firstLineStr = f.GetFirstLine(); // may be empty
				}
			}
			f.Close();
		}
		int offset = -1;
		wxString strSuccess = _T("success");
		offset = firstLineStr.Find(strSuccess);
		if (offset >= 0)
		{
			// It was a successful run. Now we want to know if the substring _T("0,")
			// is present - if so, then set m_bClearUseradminCheckbox to TRUE, for LoadDataForPage(0)
			// at its end to clear the checkbox
			offset = -1;
			offset = firstLineStr.Find(_T("0,"));
			if (offset > 0)
			{
				m_bClearUseradminCheckbox = TRUE; // Load DataForPage(0) will use this
			}
		}

		LoadDataForPage(0);
	}
}
void KBSharingMgrTabbedDlg::OnButtonUserPageChangeFullname(wxCommandEvent& WXUNUSED(event))
{
	// Get the new fullname value, from the value put by user in editbox m_pEditInformalUsername, e.g. GLenys Lee (was: Glenys Waters)
	wxString newFullname = m_pEditInformalUsername->GetValue();
	// Get the selected (in list) username matching the fullname value: e.g. glenys@Unit2
	m_pApp->m_Username2 = m_pTheUsername->GetValue();

	// Get the original values for the clicked username, including fullname, and password - do_change_fullname.py  or permission etc, needs them
	int selIndex = m_pApp->m_nMgrSel; // index in the list to the selected user for which changes are to be made
	wxString strClickedUser2 = m_pApp->m_mgrSavedUsernameArr.Item(selIndex); //see AI.h line 2444
	wxString strUser2Fullname = m_pApp->m_mgrSavedFullnameArr.Item(selIndex); //see AI.h line 2445
	wxString strUser2Pwd = m_pApp->m_mgrSavedPasswordArr.Item(selIndex); //see AI.h line 2445
	int User2UseradminFlag = m_pApp->m_mgrSavedUseradminArr.Item(selIndex); // see AI.h line 2446
#if defined (_DEBUG)
	wxLogDebug(_T("\nOnButtonUserPageChangeFullname() line %d, orig params: user2= %s, user2Fullname= %s user2Pwd= %s user2UseradminFlag= %d"),
		__LINE__, strClickedUser2.c_str(), strUser2Fullname.c_str(), strUser2Pwd.c_str(), (int)User2UseradminFlag);
#endif
	m_pApp->m_bDoingChangeFullname = TRUE;
	m_pApp->m_strChangeFullname = newFullname; // make sure app knows it
	m_pApp->m_strChangeFullname_User2 = m_pApp->m_Username2; // so LoadDataForPage(0) can grab it

#if defined (_DEBUG)
	wxLogDebug(_T("OnButtonUserPageChangeFullname() line %d: before ConfigDATfile(change_fullname): newFullname= %s selected username= %s"),
		__LINE__, newFullname.c_str(), strClickedUser2.c_str());
#endif
	wxString comma = _T(",");
	localCmdLineStr = wxEmptyString; // build commandLine here...
	localCmdLineStr = m_pApp->m_chosenIpAddr + comma;

	wxString tempStr;
	tempStr = m_pApp->m_strUserID;
	if (m_pApp->m_curNormalUsername.IsEmpty())
	{
		m_pApp->m_curNormalUsername = tempStr;
	}
	localCmdLineStr += tempStr + comma; // has any embedded ' escaped
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	if (m_pApp->m_curNormalPassword.IsEmpty())
	{
		m_pApp->m_curNormalPassword = pFrame->GetKBSvrPassword(); // gets m_kbserverPassword string
	}
	localCmdLineStr += m_pApp->m_curNormalPassword + comma;

	// Now add the next 4 fields, selected username, old fullname, user2's password, then the new fullname
	localCmdLineStr += strClickedUser2 + comma;
	localCmdLineStr += strUser2Pwd + comma;
	localCmdLineStr += strUser2Fullname + comma;
	localCmdLineStr += newFullname + comma;

	// Put a copy of this commandLine on the app
	m_pApp->m_commandLine_forManager = localCmdLineStr;

// TODO where does ConfigureMovedDatFile() go?

	m_pApp->RemoveDatFileAndEXE(change_fullname); // BEW 11May22 added, must precede call of ConfigureDATfile()
#if defined (_DEBUG)
	wxLogDebug(_T("App value of m_bDoingChangeFullname: %d, at line %d, in OnButtonUserPageChangeFullname()"), 
		(int)m_pApp->m_bDoingChangeFullname, __LINE__ );
#endif
	bool bReady = m_pApp->ConfigureDATfile(change_fullname); // arg is const int, value 11
	if (bReady)
	{
#if defined (_DEBUG)
		wxLogDebug(_T("App value of m_bDoingChangeFullname: %d, at line %d, in OnButtonUserPageChangeFullname()"),
			(int)m_pApp->m_bDoingChangeFullname, __LINE__);
#endif
		// The input .dat file is now set up ready for do_change_fullname.exe
		wxString execFileName = _T("do_change_fullname.exe");
		wxString execPath = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator; // whm 22Feb2021 changed execPath to m_appInstallPathOnly + PathSeparator
		wxString resultFile = _T("change_fullname_results.dat");

		bool bExecutedOK = FALSE; // return from CallExecute() the value of bool bSuccess
		bExecutedOK	= m_pApp->CallExecute(change_fullname, execFileName, execPath, resultFile, 99, 99);
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

		// At this point, the user table is altered, so it just remains to
		// Update...() the two lists of structs to achieve synced state
		// and load in the new state to the m_pUsersListBox
		//UpdateUserPage(m_pApp, m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator, resultFile, &m_pApp->m_arrLines); deprecated 22Dec20
		LoadDataForPage(0);

		if (m_pApp->m_bDoingChangeFullname)
		{
			// Turn it off, to default FALSE
			m_pApp->m_bDoingChangeFullname = FALSE;
			// And clear these, until such time as another change of fullname is done
			m_pApp->m_strChangeFullname.Clear();
			m_pApp->m_strChangeFullname_User2.Clear();
		}
	} // end of TRUE block for test: if (bReady)
}

void KBSharingMgrTabbedDlg::OnButtonUserPageChangePassword(wxCommandEvent& WXUNUSED(event))
{
	// Get the new password value
	//wxString newPassword = m_pEditShowPasswordBox->GetValue(); // user should have typed the new one
	wxString newPassword = wxEmptyString; // init
	if (m_pEditPersonalPassword->GetValue().IsEmpty())
	{
		wxString msgTitle = _("Type new password");
		wxString msgGuide = _("Type your new password in each of the password text boxes. The two must match.");
		wxMessageBox(msgGuide, msgTitle, wxICON_INFORMATION | wxOK);
		return;
	}
	wxString pwd1 = m_pEditPersonalPassword->GetValue();
	wxString pwd2 = m_pEditPasswordTwo->GetValue();
	if (pwd1.IsEmpty())
	{
		// warn user that no new password has been typed in the first box, middle column of KB Sharing Manager
		wxString title = _("No new password was typed");
		wxString msg = _("Changing password error: no new password was typed in the text box under: A password for this user");
		wxMessageBox(msg, title, wxICON_ERROR | wxOK);
		return;
	}
	else
	{
		newPassword = pwd1;
	}
	if (pwd2.IsEmpty() || pwd2 != pwd1)
	{
		wxString titleBad = _("Typed passwords do not match");
		wxString msgBad = _("Either the second password box is empty, or the two typed passwords do not match. Try again.");
		wxMessageBox(msgBad, titleBad, wxICON_ERROR | wxOK);
		// Clear the password boxes so the user can retry in both
		m_pEditPersonalPassword->SetValue(wxEmptyString);
		m_pEditPasswordTwo->SetValue(wxEmptyString);
		return;
	}
	else
	{
		newPassword = pwd1;
	}

	// Get the original values for the clicked username, including fullname, and password - do_change_password.py  or permission etc, needs them
	int selIndex = m_pApp->m_nMgrSel; // index in the list to the selected user for which changes are to be made
	wxString strClickedUser2 = m_pApp->m_mgrSavedUsernameArr.Item(selIndex); //see AI.h line 2444
	wxString strUser2Fullname = m_pApp->m_mgrSavedFullnameArr.Item(selIndex); //see AI.h line 2445
	wxString strUser2Pwd = m_pApp->m_mgrSavedPasswordArr.Item(selIndex); //see AI.h line 2445
	int User2UseradminFlag = m_pApp->m_mgrSavedUseradminArr.Item(selIndex); // see AI.h line 2446
#if defined (_DEBUG)
	wxLogDebug(_T("\nOnButtonUserPageChangePassword() line %d, orig params: user2= %s, user2Fullname= %s user2Pwd= %s user2UseradminFlag= %d"),
		__LINE__, strClickedUser2.c_str(), strUser2Fullname.c_str(), strUser2Pwd.c_str(), (int)User2UseradminFlag);
#endif
	m_pApp->m_bDoingChangePassword = TRUE;
	m_pApp->m_ChangePassword_NewPassword = newPassword; // make sure app knows it
	m_pApp->m_strChangePassword_User2 = m_pApp->m_Username2; // so LoadDataForPage(0) can grab it

	// Test, it's same as what's in gui text ctrl
	wxString guiUser2 = m_pTheUsername->GetValue(); // use this if the assert trips
	wxASSERT(guiUser2 == m_pApp->m_Username2); 
//#if defined (_DEBUG)
//	wxLogDebug(_T("OnButtonUserPageChangePassword() line %d: before ConfigureDATfile(change_password): newPassword: %s (old)m_Username2: %s , guiUser2: %s"),
//		__LINE__, newPassword.c_str(), m_pApp->m_Username2.c_str(), guiUser2.c_str());
//#endif
#if defined (_DEBUG)
	wxLogDebug(_T("OnButtonUserPageChangePassword() line %d: before ConfigDATfile(change_password): newPassword= %s selected username= %s"),
		__LINE__, newPassword.c_str(), strClickedUser2.c_str());
#endif

	wxString comma = _T(",");
	localCmdLineStr = wxEmptyString; // build commandLine here...
	localCmdLineStr = m_pApp->m_chosenIpAddr + comma;

	wxString tempStr;
	tempStr = m_pApp->m_strUserID;
	if (m_pApp->m_curNormalUsername.IsEmpty())
	{
		m_pApp->m_curNormalUsername = tempStr;
	}
	localCmdLineStr += tempStr + comma; // has any embedded ' escaped
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	if (m_pApp->m_curNormalPassword.IsEmpty())
	{
		m_pApp->m_curNormalPassword = pFrame->GetKBSvrPassword(); // gets m_kbserverPassword string
	}
	localCmdLineStr += m_pApp->m_curNormalPassword + comma;

	// Now add the next 4 fields, selected username, old fullname, user2's password, then the new password
	localCmdLineStr += strClickedUser2 + comma;
	localCmdLineStr += strUser2Pwd + comma;
	localCmdLineStr += strUser2Fullname + comma;
	localCmdLineStr += newPassword + comma;

	// Put a copy of this commandLine on the app
	m_pApp->m_commandLine_forManager = localCmdLineStr;

	m_pApp->RemoveDatFileAndEXE(change_password); // BEW 11May22 added, must precede call of ConfigureDATfile()
	bool bReady = m_pApp->ConfigureDATfile(change_password); // arg is const int, value 12
	if (bReady)
	{
		// The input .dat file is now set up ready for do_change_fullname.exe
		wxString execFileName = _T("do_change_password.exe");
		wxString execPath = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator; // whm 22Feb2021 changed execPath to m_appInstallPathOnly + PathSeparator
		wxString resultFile = _T("change_password_results.dat");

		bool bExecutedOK = FALSE;
		bExecutedOK = m_pApp->CallExecute(change_password, execFileName, execPath, resultFile, 99, 99);
		if (!bExecutedOK)
		{
			// error in the call, inform user, and put entry in LogUserAction() - English will do
			wxString msg = _T("Line %d: CallExecute for enum: change_password, failed - perhaps input parameters (and/or password) did not match any entry in the user table; Adapt It will continue working ");
			msg = msg.Format(msg, __LINE__);
			wxString title = _T("Probable do_change_password.exe error");
			wxMessageBox(msg, title, wxICON_WARNING | wxOK);
			m_pApp->LogUserAction(msg);
		}
		// At this point, the user table is altered, so it just remains to
		// load in the new state to the m_pUsersListBox
		LoadDataForPage(0);

		// Clear these, until such time as another change of password is done
		m_pApp->m_ChangePassword_NewPassword.Clear();
		m_pApp->m_strChangePassword_User2.Clear();
		
	} // end of TRUE block for test: if (bReady)
}

// BEW 11Jan21 leave it, but with no support for any page but page zero
void KBSharingMgrTabbedDlg::OnTabPageChanged(wxNotebookEvent& event)
{
	// OnTabPageChanged is called whenever any tab is selected
	int pageNumSelected = event.GetSelection();
	if (pageNumSelected == m_nCurPage)
	{
		// user selected same page, so just return
		return;
	}
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void KBSharingMgrTabbedDlg::OnOK(wxCommandEvent& event)
{
	m_pUsersListBox->Clear();
	ClearCurPwdBox(); 
	m_pApp->m_nMgrSel = -1; // reinitialize, in case user reenters the Mngr later
	event.Skip();

	delete m_pKBSharingMgrTabbedDlg;
}

void KBSharingMgrTabbedDlg::OnCancel(wxCommandEvent& event)
{
	m_pUsersListBox->Clear();
	ClearCurPwdBox();
	m_pApp->m_nMgrSel = -1; // reinitialize, in case user reenters the Mngr later
	event.Skip();

	delete m_pKBSharingMgrTabbedDlg;
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
	ClearCurPwdBox();
	m_pApp->m_bSelectionChange = FALSE;
	m_pApp->m_bHasUseradminPermission = TRUE;
	m_pBtnUsersAddUser->Enable();
	m_pBtnChangePermission->Enable();

}

// BEW 19Dec20 uses the mgr arrays defined in AI.h at 2217 to 2225 to
// extract values from these arrays for use in the users page of the GUI.
// This function takes (the already filled arrays) and extracts the
// usernames, unsorted, and adds them to the m_pUsersListBox for viewing
void KBSharingMgrTabbedDlg::FillUserList(CAdapt_ItApp* pApp)
{
	// Sanity check - make sure the m_mgrUsernameArr is not empty
	if (pApp->m_mgrUsernameArr.IsEmpty())
	{
#if defined (_DEBUG)
		wxLogDebug(_T("%s::%s(), line %d: pApp->m_mgrUsernameArr is EMPTY"),
			__FILE__, __FUNCTION__, __LINE__);
#endif
		wxBell();
		return;
	}
	int locn = -1;
    locn = locn; // avoid "set but not used" gcc warning in release builds
	int count = pApp->m_mgrUsernameArr.GetCount();
	wxString aUsername;
	int i;
	for (i = 0; i < count; i++)
	{
		aUsername = pApp->m_mgrUsernameArr.Item(i);
		/* BEW remove, must take it out of usrs table, otherwise, adding new users becomes impossible
		// BEW 11Jan24 hide kbadmin and kbauth, as these are dedicated username and pwd for 
		// accessing the MariaDB to do something within - send data, get data, or change data.
		// These two are now built-in values, and so must not appear in the user list - they are
		// not changeable; other values do not give access to the MariaDB server's kbserver
		if (aUsername == pApp->m_strThingieID)
		{
			continue;
		}
		*/
		locn = m_pUsersListBox->Append(aUsername);
#if defined (_DEBUG)
		wxLogDebug(_T("%s::%s(), line %d: list location - where appended = %d , aUsername = %s"), 
			__FILE__, __FUNCTION__, __LINE__, locn, aUsername.c_str());
#endif
	}
}

// Return TRUE if the passwords match and are non-empty , FALSE if mismatched
// BEW 11Jan21 This one is needed, for we still check for a match
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
		m_pApp->m_nMgrSel = wxNOT_FOUND; // -1
		return;
	}
	if (m_pUsersListBox->IsEmpty())
	{
		m_pApp->m_nMgrSel = wxNOT_FOUND; // -1
		return;
	}
	// The GetSelection() call returns -1 if there is no selection current, so check for
	// this and return (with a beep) if that's what got returned
	m_pApp->m_nMgrSel = m_pUsersListBox->GetSelection();
	if (m_pApp->m_nMgrSel == wxNOT_FOUND)
	{
		wxBell();
		return;
	}
#if defined (_DEBUG)
	// BEW 21Dec23 counts for each of the arrays
	wxLogDebug(_T("User_Names_Page in ShowPassword, line %d  usernames m_nCount: %d "), __LINE__, m_pApp->m_mgrUsernameArr.GetCount());
	wxLogDebug(_T("User_Names_Page in ShowPassword, line %d  fullnames  m_nCount: %d "), __LINE__, m_pApp->m_mgrFullnameArr.GetCount());
	wxLogDebug(_T("User_Names_Page in ShowPassword, line %d  passwords m_nCount: %d "), __LINE__, m_pApp->m_mgrPasswordArr.GetCount());
	wxLogDebug(_T("User_Names_Page in ShowPassword, line %d  useradmins m_nCount: %d "), __LINE__, m_pApp->m_mgrUseradminArr.GetCount());
	// And for the "saved" set
	wxLogDebug(_T("User_Names_Page in ShowPassword, line %d  SAVED usernames m_nCount: %d "), __LINE__, m_pApp->m_mgrSavedUsernameArr.GetCount());
	wxLogDebug(_T("User_Names_Page in ShowPassword, line %d  SAVED fullnames  m_nCount: %d "), __LINE__, m_pApp->m_mgrSavedFullnameArr.GetCount());
	wxLogDebug(_T("User_Names_Page in ShowPassword, line %d  SAVED passwords m_nCount: %d "), __LINE__, m_pApp->m_mgrSavedPasswordArr.GetCount());
	wxLogDebug(_T("User_Names_Page in ShowPassword, line %d  SAVED useradmins m_nCount: %d "), __LINE__, m_pApp->m_mgrSavedUseradminArr.GetCount());

#endif


	// Get the password from the matrix
	wxString the_password = m_pApp->GetFieldAtIndex(m_pApp->m_mgrPasswordArr, m_pApp->m_nMgrSel);
	// show it in the box
	m_pEditShowPasswordBox->ChangeValue(the_password);
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

	// BEW 30Dec23 I discovered logic flaws when trying to use the Add User button.
	// (1) If I want to add a new user with 0 permission - checkbox unticked (i.e. can't add other users), the current
	// code drops into a test's TRUE block where a message is shown and then control drops back into the Mgr's user
	// page having done nothing. So the present code won't allow a new user with permission zero.
	// (2) Long ago I made a mad decision - requiring a user with permission 1 be clicked (which gets the checkbox
	// ticked) as a prerequisite to being able to have Add User button do its job. This was crazy, why would a 
	// naive user think he should click some other user first as a way to get a new one added - and clicking the
	// other user puts that user's values into the wxTextCtrls - the very controls that we want the naive user to
	// consider to be the ones where he should type the wanted new user's credentials: username, fullname, new pwd,
	// and use the checkbox to set permission either 0 or 1?
	// So, the Add User button should work ONLY provided no user in the user list is clicked to be active. And all
	// the textboxes must be empty, and the checkbox unticked. Clicking Clear Controls button will accomplish this,
	// so I think I should programmatically get Clear Controls's handler called, as the first thing to do when the
	// user clicks OnButtonUserPageAddUser. (Remember there are pApp wxArrayString's which store username, fullname,
	// current password, and useradmin value, already with valid param values for these, for active user m_strUserID,
	// by the time that a user can get the User page of the Manager open, so a sensible fix of the protocols will work.)

	// BEW 1Jan24 first, check if there currently is a user in the list that is selected. If so, it must be unselected
	// and a message box given to the user about what the condition are for a successfulu Add User button press, and
	// with that programmatically remove the selection, clear all the controls, and return to UserPage(0) for the user
	// to have a second try.
	if (m_pApp->m_nMgrSel != wxNOT_FOUND)
	{
		// A username in the Mgr's listBox is selected. This selection must be removed so that no user is selected, to
		// clear the way for setting the values for a new user (and ticking the checkbox if the new user is to have
		// permission to create other users). Inform the active user, and then clear the controls and checkbox and
		// return to the userpage without progressing further.
		wxString msg = _("The Add User button only works when no user is selected in the list.\nThe controls will be cleared when you close this message,\nthen you can type in the values for a new username, its fullname, and its password.\nYou can see this password by clicking the Show Password button.\nTo give your new user the privilege to add other users\ntick the User Administrator checkbox also.");
		wxString title = _("Clear list selection");
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		wxCommandEvent dummyEvent;
		OnButtonUserPageClearControls(dummyEvent);
		return;
	}

	// Test that the logged in person is not trying to add an existing listed username a second
	// time - which is illegal. If so, warn, then clear the controls & return without doing anything.
	// BEW 11Jan24, listing the special MariaDB accessing username is illegal, so disallow "kbadmin"
	// being set as strUsername 

	// The following lines get the new user's name, fullname, password, and useradmin value (if any of the
	// required fields are empty, warn, (don't clear), just return so the active user can remedy the empty fields)
	wxString strUsername = m_pTheUsername->GetValue();
	wxString strFullname = m_pEditInformalUsername->GetValue();
	wxString strPassword = m_pEditPersonalPassword->GetValue(); // variable needs to be defined, even if empty
	wxString strPasswordTwo = m_pEditPasswordTwo->GetValue();  //  ditto
	bool bUseradmin = m_pCheckUserAdmin->GetValue();
	wxString strUseradmin = bUseradmin ? _T("1") : _T("0");

	if (strUsername == m_pApp->m_strThingieID)
	{
		wxString msg = _("Your typed user in the Username box is not available for Add User button. Please type something else.");
		wxString title = _("Warning: Wrong user definition");
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		m_pTheUsername->SetValue(wxEmptyString);
		return;
	}

	// Test now that the relevant wxTextCtrl values have enabling values in them (don't test the checkbox, because
	// if it is unticked, then the new user will have a 0 value, which is a legal value meaning "this new user can't 
	// add more users")
	if (strUsername.IsEmpty() || strFullname.IsEmpty() || strPassword.IsEmpty() || strPasswordTwo.IsEmpty())
	{
		wxString msg = _("One or more of the text boxes: Username, Fullname, or one or both password boxes, are empty.\nEach of these must have appropriate values typed into them. Do so now.");
		wxString title = _("Warning: Incomplete new user definition");
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		return;
	}

	// turn this else block into a scoping block, where the only task is to discover an illegal
	// attempt to add an already existing user to the list a second time - and prevent it
	//else
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
					"You are trying to add a username which already exists in the user table. This is illegal. Each username must be unique.");
				wxMessageBox(msg, title, wxICON_WARNING | wxOK);
				wxCommandEvent dummy;
				OnButtonUserPageClearControls(dummy);
				return;
			}
		}
		m_pUsersListBox->SetSelection(-1); // don't leave last of the for loop showing as selected
	}

	// Test the passwords match in the two boxes for them,(if they don't match, 
	// or a box is empty, then the call will show an exclamation message from 
	// within it, and clear both password text boxes). Leave other values as they are.
	// We just want to give the opportunity to retype the new pwd identically in each box
	bool bMatchedPasswords = CheckThatPasswordsMatch(strPassword, strPasswordTwo);
	if (!bMatchedPasswords)
	{
		// Unmatched passwords, the user has been told to try again
		return;
	}

	// Create the new entry in the KBserver's user table
	// First, copy the strings needed to the temp variables above
	// - these are the foreign ones, when taking values from user page of Manager
	m_pApp->m_temp_username = strUsername;
	m_pApp->m_temp_fullname = strFullname;
	m_pApp->m_temp_password = strPassword;
	m_pApp->m_temp_useradmin_flag = strUseradmin;
	wxString ipAddr = m_pApp->m_chosenIpAddr;
	wxString datFilename = _T("add_foreign_users.dat");
	//wxString datFilename = _T("add_foreign_KBUsers.dat"); use only 1 name for GUI or Mgr

	wxString resultFile = _T("add_foreign_users_results.dat");

	m_pApp->m_bAddUser2UserTable = FALSE; // BEW 15Feb24, this bool is for differentiating between
		// adding a user by the GUI menu choice (if TRUE), or the Mgr's Add User button (FALSE). There's
		// no need to differentiate with two .dat files which use the a commandLine of one structure
	bool result = FALSE; // initialize

	wxString DBUsername = m_pApp->m_strUserID;
	wxString DBPassword = m_pApp->m_curNormalPassword;

	//bool bCredsOK = Credentials_For_User(&ipAddr, &strUsername, &strFullname, &strPassword, bUseradmin, datFilename);
	bool bCredsOK = DoAddForeignUser(&ipAddr, &DBUsername, &DBPassword, &DBUsername, &DBPassword,
		&strUsername, &strFullname, &strPassword, &strUseradmin, datFilename);

	if (bCredsOK)
	{
		m_pApp->RemoveDatFileAndEXE(credentials_for_user); // BEW 11May22 added, must precede call of ConfigureDATfile() - call does nothing here, yet
		bool bOK = m_pApp->ConfigureDATfile(credentials_for_user);
		if (bOK)
		{
			wxString execFileName = _T("do_add_foreign_kbusers.exe");
			result = m_pApp->CallExecute(credentials_for_user, execFileName, m_pApp->m_curExecPath, resultFile, 99, 99);
		}
	}
	// Update the page if we had success, if no success, just clear the controls
	if (result == TRUE)
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
	//}
}

// The box state will have already been changed by the time control enters the handler's body
void KBSharingMgrTabbedDlg::OnCheckboxUseradmin(wxCommandEvent& WXUNUSED(event))
{
	// The value in the checkbox has toggled by the time control
	// enters this handler, so code accordingly
	bool bCurrentUseradminValue = m_pCheckUserAdmin->GetValue();
    // kbadmin value is auto-set to 1 (TRUE) always, because users have to be 
	// able to create entries in kb table
	// BEW 3Jan24 added test to not show the message if the current user is wanting to set
	// the permission level to '1' for an Add User button press. If not suppressed, the message
	// would recommend ticking the checkbox to effect a toggle of an already added user from 1 to 0,
	// or from 0 to 1.
	if (m_pApp->m_bSelectionChange == TRUE)
	{
		wxString title = _("Permission level: Set or Change");
		//wxString msg = _("Tick the box, but after that do not click 'Change Permission',\nif you are giving the permission 'Can add other users'\n to a new user you are creating.\nTick the box and then click the 'Change Permission' button,\nif you want to change the permission level for an existing user.\nUn-tick the box to give a new user no permission to add other users,\nor to remove an existing user's permission to add new users.");
		wxString msg = _("If you want to change the permission level for an existing user,\ntick the checkbox and then click the 'Change Permission' button.\n(Note: trying to turn off the active user's permission to add new users\nwill not be allowed.)");
		wxMessageBox(msg, title, wxICON_INFORMATION | wxOK);
	}
	wxString strAuthenticatedUser;
	wxString strSelectedUsername;

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
		// Test, to make sure this constrait is not violated for the authenticated username
		// (that username is that of app's m_strUserID, for whatever project is current)		
		strAuthenticatedUser = m_pApp->m_strUserID;
		// Get the selected username from the list in the manager's GUI: for this we need
		// to use app's m_mgrSavedUsernameArr array, which has just been filled with the
		// list's username strings; and we also need the selection index for which
		// username got clicked - that value is in app's m_nMgrSel public member
		strSelectedUsername = m_pApp->m_mgrSavedUsernameArr.Item(m_pApp->m_nMgrSel);

		// Check these are different usernames, if not, return control to the GUI and
		// reset the checkbox back to ticked (ie. useradmin is reset t0 '1')
		if (strSelectedUsername != strAuthenticatedUser)
		{
			// This guy (on the left) can be safely demoted, because he's
			// not the currently authenticated username for the project
			m_pCheckUserAdmin->SetValue(FALSE);
		}
		else
		{
			// Both usenames are matched, so this selected guy has to retain his 
			// useradmin == TRUE permission level
			wxString title = _("Illegal permission change");
			wxString msg = _("Warning: you are not permitted to change the permission value for the authenticated user: %s");
			msg = msg.Format(msg, strAuthenticatedUser.c_str());
			wxMessageBox(msg, title, wxICON_WARNING | wxOK);

			m_pCheckUserAdmin->SetValue(TRUE); // restore the ticked state of the useradmin checkbox
		}
	}
}

// The user clicked on a different line of the listbox for usernames
void KBSharingMgrTabbedDlg::OnSelchangeUsersList(wxCommandEvent& WXUNUSED(event))
{
	// some minimal sanity checks - can't use Bill's helpers.cpp function
	// ListBoxSanityCheck() because it clobbers the user's selection just made
	if (m_pUsersListBox == NULL)
	{
		m_pApp->m_nMgrSel = wxNOT_FOUND; // -1
		return;
	}
	if (m_pUsersListBox->IsEmpty())
	{
		m_pApp->m_nMgrSel = wxNOT_FOUND; // -1
		return;
	}
	// The GetSelection() call returns -1 if there is no selection current, so check for
	// this and return (with a beep) if that's what got returned
	m_pApp->m_nMgrSel = m_pUsersListBox->GetSelection();
	if (m_pApp->m_nMgrSel == wxNOT_FOUND)
	{
		wxBell();
		return;
	}

#if defined(_DEBUG) //&& defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnSelchangeUsersList(): selection index m_nMgrSel = %d"), m_pApp->m_nMgrSel);
#endif
	// BEW 3Jan24, m_bSelectionChange needs to be set TRUE. The checkbox handler needs to know when 
	// it's true or false. When false, the user needs to have the handler for the checkbox suppress
	// showing the message - because otherwise ticking the checkbox for a Add User call, would show
	// a confusing message.
	m_pApp->m_bSelectionChange = TRUE;

	// Get the username - the one clicked in the list - to compare with whatever
	// was the existing username before the Change Permission request. Why?
	// We need a different processing path if the user clicks the checkbox and asks 
	// for "Change Permission" without altering the selected user in the list. Both
	// processing paths, however, need to call do_change_permission.exe, which
	// has the python code for toggling the useradmin permission to the opposite value
	wxString theUsername = m_pUsersListBox->GetString(m_pApp->m_nMgrSel);

	// BEW 12Jan24 the kbadmin user is special, it's the only one that can be used for accessing
	// the MariaDB/MySQL kbserver. We don't want clicks on it to select it, as a protection for
	// users in the Manager who might think is of equal status to other listed users. So disallow
	// an attempt to select it.
	if (theUsername == m_pApp->m_strThingieID)
	{
		wxString msg = _("Do not select this user. It is important for kbserver database access.");
		wxLogDebug(_T("OnSelchangeUsersList(): msg = %s"), msg.c_str());
		return;
	}
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnSelchangeUsersList(): from list: username = %s"), theUsername.c_str());
#endif
	wxString userReturned = m_pApp->GetFieldAtIndex(m_pApp->m_mgrUsernameArr, m_pApp->m_nMgrSel);

	// BEW 20Nov20 every time a user is clicked on in the list, we need that user
	// to be stored on the app in the member variable m_strChangePermission_User2
	// because if a change of the user permission is wanted, the ConfigureMovedDatFile()
	// app function will need that user as it's user2 value (user1 is for authenticating
	// to mariaDB/kbserver so is not appropriate). The aforementioned function needs to
	// create the correct comandLine for the ::wxExecute() call in app function CallExecute()
	// and it can't do so without getting the user name selected in the Manager user list
	m_pApp->m_strChangePermission_User2 = userReturned;

	//* BEW 19Dec23 removed this block?? unsure if need - keep it for now
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnSelchangeUsersList(): Old username = %s"),
		m_pApp->m_ChangePermission_OldUser.c_str());
#endif
	if (theUsername != m_pApp->m_ChangePermission_OldUser)
	{
		m_pApp->m_bChangePermission_DifferentUser = TRUE;
	}
	else
	{
		m_pApp->m_bChangePermission_DifferentUser = FALSE; // user didn't select 
				// a different user than the one currently selected in the list
	}
	//*/
	// BEW 9Dec20 fullname changing needs it's own app storage wxStrings for commandLine building
	m_pApp->m_strChangeFullname_User2 = userReturned;
	wxString fullnameReturned = m_pApp->GetFieldAtIndex(m_pApp->m_mgrFullnameArr, m_pApp->m_nMgrSel);
	m_pApp->m_strChangeFullname = fullnameReturned;
	wxASSERT(!fullnameReturned.IsEmpty()); // make sure it's not empty

	// BEW 20Nov20 a click to select some other user must clear the curr password box
	// so that it does not display the pwd for any user other than the selected user
	ClearCurPwdBox();

	// Copy the whole matrix of mgr arrays to the 'saved set' -- only useradmin is unset so get it
	int useradminReturned = m_pApp->GetIntAtIndex(m_pApp->m_mgrUseradminArr, m_pApp->m_nMgrSel);
	wxUnusedVar(useradminReturned);
	bool bNormalToSaved = TRUE; // empty 'saved set' and refill with 'normal set', 
								// as 'normal set' may change below
	m_pApp->MgrCopyFromSet2DestSet(bNormalToSaved); // facilitates checking for differences below

#if defined(_DEBUG) //&& defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnSelchangeUsersList(): username = %s , fullname = %s,  useradmin = %d"),
		userReturned.c_str(), fullnameReturned.c_str(), useradminReturned);
#endif

	// Fill the User page's (central vertical list of wxTextCtrl) controls with their 
	// required data; the logged in user can then edit the these parameters (m_pTheUsername is 
	// the ptr to the username wxTextCtrl)
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnSelchangeUsersList(): username box BEFORE contains: %s"), m_pTheUsername->GetValue().c_str());
#endif
	// BEW 22Dec20  Changing the value of the username (but keeping rest of row unchanged)
	// is not permitted in our refactored design. It makes no sense. If a different username
	// is wanted, it has to be handled as a CreateUser() scenario - with different fullname and
	// password and it's own useradmin value. The menu item on the Administrator menu
	// can do this job, but we can support it in the Mngr too.	
	m_pTheUsername->ChangeValue(theUsername);

	// BEW 16Dec20 also put the value into app's m_Username2 string using
	// it's Update function UpdateUsername2(wxString str), so that
	// user page buttons which work with whatever is the selected user,
	// can grab its value and use for building commandLine for
	// ConfigureMovedDatFile() - so the correct sql change can be effected
	m_pApp->UpdateUsername2(theUsername);

#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnSelchangeUsersList(): username box AFTER contains: %s"), m_pTheUsername->GetValue().c_str());
	// it should not have changed at this point
#endif

	// Show the fullname associated with theUsername
	wxString aFullname = m_pApp->GetFieldAtIndex(m_pApp->m_mgrFullnameArr, m_pApp->m_nMgrSel);
	m_pEditInformalUsername->ChangeValue(aFullname);

	// Show the checkbox value for the useradmin flag
	int flag = m_pApp->GetIntAtIndex(m_pApp->m_mgrUseradminArr, m_pApp->m_nMgrSel);
	bool bValue = flag == 1 ? TRUE : FALSE;
	m_pCheckUserAdmin->SetValue(bValue);

	// Show the password in central boxes? No, but there is a "Show Password" button for
	// for the box with ptr m_pEditShowPasswordBox - which is empty till "Show Password"
	// button is pressed. So....
	m_pEditShowPasswordBox->ChangeValue(_T("")); // don't display pwd, until asked to
			// When OnSelchangeUsersList() is invoked, the two central password boxes
			// should stay empty. So..
	m_pEditPersonalPassword->ChangeValue(_T(""));
	m_pEditPasswordTwo->ChangeValue(_T(""));

	// Note 1: selecting in the list does NOT in itself cause changes in the mysql
	// database, nor does editing any of the control values, or clearing them. The 
	// the mysql user table is only affected when the user clicks one of the buttons
	// Add User, or Change Permission, or Change Fullname is invoked - because only
	// those three are implemented with .c wrapped to be .exe functions, one for each.

	// Note 2: Remove User is not permitted - it would make an unacceptable mess of
	// the mysql entry table, so we have no support for such a button. Do you want
	// to change a username - the way to do it is to make a whole new user.
}

//#endif

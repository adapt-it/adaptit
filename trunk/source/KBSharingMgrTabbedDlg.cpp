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

#define _WANT_DEBUGLOG // comment out to suppress wxLogDebug() calls

// other includes
//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
#include <wx/filesys.h> // for wxFileName
//#include <wx/dir.h> // for wxDir
//#include <wx/choicdlg.h> // for wxGetSingleChoiceIndex

#include "Adapt_It.h"
#include "helpers.h"
#include "KbServer.h"
#include "LanguageCodesDlg.h"
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
	// For page 2: Create KB Definitions page
	EVT_RADIOBUTTON(ID_RADIOBUTTON_TYPE1_KB, KBSharingMgrTabbedDlg::OnRadioButton1CreateKbsPageType1)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_TYPE2_KB, KBSharingMgrTabbedDlg::OnRadioButton2CreateKbsPageType2)
	EVT_BUTTON(ID_BUTTON_LOOKUP_THE_CODES, KBSharingMgrTabbedDlg::OnBtnCreatePageLookupCodes) // whm added 10May10
	EVT_BUTTON(ID_BUTTON_RFC5646, KBSharingMgrTabbedDlg::OnBtnCreatePageRFC5646Codes)
	EVT_BUTTON(ID_BUTTON_CLEAR_BOXES, KBSharingMgrTabbedDlg::OnButtonCreateKbsPageClearControls)

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
	m_pNonSourceKbsListBox = (wxListBox*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_LISTBOX_NONSRC_LANG_CODE);
	wxASSERT(m_pNonSourceKbsListBox != NULL);
	// wxTextCtrls
	m_pTheUsername = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_THE_USERNAME);
	wxASSERT(m_pTheUsername != NULL);
	m_pEditInformalUsername = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_INFORMAL_NAME);
	wxASSERT(m_pEditInformalUsername != NULL);
	m_pEditPersonalPassword = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_PASSWORD);
	wxASSERT(m_pEditPersonalPassword != NULL);
	m_pEditPasswordTwo = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_PASSWORD_TWO);
	wxASSERT(m_pEditPasswordTwo != NULL);
	m_pEditSourceCode = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_SRC); // src lang code on Create Kbs page
	wxASSERT(m_pEditSourceCode != NULL);
	m_pEditNonSourceCode = (wxTextCtrl*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXTCTRL_NONSRC); // non-src lang code on Create Kbs page
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
	m_pBtnUsingRFC5646Codes = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_RFC5646);
	wxASSERT(m_pBtnUsingRFC5646Codes != NULL);
	m_pBtnAddKbDefinition = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_ADD_DEFINITION);
	wxASSERT(m_pBtnAddKbDefinition != NULL);
	m_pBtnClearBothLangCodeBoxes = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_CLEAR_BOXES);
	wxASSERT(m_pBtnClearBothLangCodeBoxes != NULL);
	m_pBtnLookupLanguageCodes = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_LOOKUP_THE_CODES);
	wxASSERT(m_pBtnLookupLanguageCodes != NULL);
	m_pBtnRemoveSelectedKBDefinition = (wxButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_BUTTON_REMOVE_SELECTED_DEFINITION);
	wxASSERT(m_pBtnRemoveSelectedKBDefinition != NULL);

	// Radiobuttons & wxStaticText
	m_pRadioKBType1 = (wxRadioButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_RADIOBUTTON_TYPE1_KB);
	m_pRadioKBType2 = (wxRadioButton*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_RADIOBUTTON_TYPE2_KB);
	m_pNonSrcLabel  = (wxStaticText*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXT_TGT_LANG_CODE);
	m_pNonSrcCorrespCodesListLabel = (wxStaticText*)m_pKBSharingMgrTabbedDlg->FindWindowById(ID_TEXT_NONSOURCE_CODES_LABEL);

    // For an instantiated KbServer class instance to use, we use the stateless one created
    // within KBSharingSetupDlg's creator function; and it has been assigned to
    // KBSharingMgrTabbedDlg::m_pKbServer by the setter SetStatelessKbServerPtr() after
    // KBSsharingMgrTabbedDlg was instantiated

	// Initialize the User page's checkboxes to OFF
	m_pCheckUserAdmin->SetValue(FALSE);
	m_pCheckKbAdmin->SetValue(FALSE);

	// Hook up to the m_usersList member of the stateless KbServer instance
	m_pUsersList = m_pKbServer->GetUsersList();
	// Hook up to the m_kbsList member of the stateless KbServer instance
	m_pKbsList = m_pKbServer->GetKbsList(); // this list contains both Type1 and Type2 KB definitions

	// Create the 2nd UsersList to store original copies before user's edits etc
	// (destroy it in OnOK() and OnCancel())
	m_pOriginalUsersList = new UsersList;
	// Ditto, for the KbsList
	m_pOriginalKbsList = new KbsList;  // destroy it in OnOK() and OnCancel()
	// We also need two more lists, for the KbServerKb struct clones, separated by type
	// (i.e. adapting KB versus glossing KB definitions)
	m_pKbsList_Tgt = new KbsList; // destroy it in OnOK() and OnCancel()
	m_pKbsList_Gls = new KbsList; // destroy it in OnOK() and OnCancel()

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

	m_bKBisType1 = TRUE; // initialize
	m_tgtLanguageCodeLabel = _("Target language code:");
	m_glossesLanguageCodeLabel = _("Glosses language code:");
	m_correspTgtLanguageCodesLabel = _T("Corresponding target language codes:");
	m_correspGlsLanguageCodesLabel = _T("Corresponding glossing language codes:");

	m_sourceLangCode.Empty();
	m_targetLangCode.Empty();
	m_glossLangCode.Empty();


	// Start by showing Users page
	m_nCurPage = 0;
	LoadDataForPage(m_nCurPage); // start off showing the Users page (for now)
}

// Setter for the stateless instance of KbServer created by KBSharingSetupDlg's creator
// (that KbServer instance will internally have it's m_bStateless member set to TRUE)
void KBSharingMgrTabbedDlg::SetStatelessKbServerPtr(KbServer* pKbServer)
{
	m_pKbServer = pKbServer;
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("KBSharingMgrTabbedDlg::SetStatelessKbServerPtr(): settting ptr = %x , m_bStateless = %d"),
		pKbServer, (int)pKbServer->m_bStateless);
#endif
}

KbServer*KBSharingMgrTabbedDlg::GetKbServer()
{
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("KBSharingMgrTabbedDlg::GetKbServer(): gettting ptr = %x , m_bStateless = %d"),
		m_pKbServer, (int)m_pKbServer->m_bStateless);
#endif
   return m_pKbServer;
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

KbServerKb* KBSharingMgrTabbedDlg::CloneACopyOfKbServerKbStruct(KbServerKb* pExistingStruct)
{
	KbServerKb* pClone = new KbServerKb;
	pClone->id = pExistingStruct->id;
	pClone->sourceLanguageCode = pExistingStruct->sourceLanguageCode;
	pClone->targetLanguageCode = pExistingStruct->targetLanguageCode;
	pClone->kbType = pExistingStruct->kbType;
	pClone->username = pExistingStruct->username;
	pClone->timestamp = pExistingStruct->timestamp;
	pClone->deleted = pExistingStruct->deleted;
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

void KBSharingMgrTabbedDlg::DeleteClonedKbServerKbStruct()
{
	if (m_pOriginalKbStruct != NULL)
	{
		delete m_pOriginalKbStruct;
		m_pOriginalKbStruct = NULL;
	}
}

wxString KBSharingMgrTabbedDlg::GetEarliestUseradmin(UsersList* pUsersList)
{
	size_t count = pUsersList->size();
	wxString earliestUseradmin;
	wxString aUser;
	int nEarliestID = 99999; // initialize
	int anID = 0;
	KbServerUser* pUserStruct = NULL;
	size_t index;
	for (index = 0; index < count; index++)
	{
		pUserStruct = GetUserStructFromList(pUsersList, index);
		aUser = pUserStruct->username;
		anID = pUserStruct->id;
		if (anID < nEarliestID)
		{
			nEarliestID = anID;
			earliestUseradmin = aUser;
		}
	}
	return earliestUseradmin;
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
		// The m_pUsersList will be cleared within the ListUsers() call done below, so it
		// is unnecessary to do it here now
		// ==================================================================

		// Get the users data from the server, store in the list of KbServerUser structs,
		// the call will clear the list first before storing what the server returns
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("KBSharingMgrTabbedDlg::LoadDataForPage(): m_pKbServer = %x , m_bStateless = %d"),
		m_pKbServer, (int)m_pKbServer->m_bStateless);
#endif
		wxString username = m_pKbServer->GetKBServerUsername();
		wxString password = m_pKbServer->GetKBServerPassword();
		CURLcode result = CURLE_OK;
		if (!username.IsEmpty() && !password.IsEmpty())
		{
			result = (CURLcode)m_pKbServer->ListUsers(username, password);
			if (result == CURLE_OK)
			{
				// Return if there's nothing
				if (m_pUsersList->empty())
				{
					// Don't expect an empty list, so an English message will suffice
					wxMessageBox(_T("LoadDataForPage() error, the returned list of users was empty. Fix this."),
					_T("KB Sharing Manager error"), wxICON_WARNING | wxOK);
					m_pKbServer->ClearUsersList(m_pUsersList);
					return;
				}

				// BEW 21Jul13, find the user who is useradmin = 1, and has the lowest ID.
				// This user must then have his username stored in m_earliestUseradmin,
				// and the Delete User button must disallow deleting this one, and the
				// Edit User button must disallow demoting this user's privileges. This is
				// so there is always at least one user who is able to add, edit or remove
				// users.
				m_earliestUseradmin = GetEarliestUseradmin(m_pUsersList);
#if defined (_DEBUG) && defined(_WANT_DEBUGLOG)
				wxLogDebug(_T("LoadUsersListBox(): earliest (by ID value) username who is a useradmin = %s"),
				m_earliestUseradmin.c_str());
#endif
				// Copy the list, before user gets a chance to modify anything
				CopyUsersList(m_pUsersList, m_pOriginalUsersList); // param 1 is src list, param2 is dest list

                // Load the username strings into m_pUsersListBox, the ListBox is sorted;
                // Note: the client data for each username string added to the list box is
                // the ptr to the KbServerUser struct itself which supplied the username
                // string, the struct coming from m_pUsersList
				LoadUsersListBox(m_pUsersListBox, m_pUsersList->size(), m_pUsersList);
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
	else if (pageNumSelected == 1)// must be first Kbs page -- the one for creating new KB definitions
	{
		// To keep the code simpler, we'll call LoadDataForPage() for this page, every
		// time the radio button choice is changed. Strictly speaking, one listing of the
		// KB definitions gets all the adapting ones, and all the glossing ones, in one
		// hit - so once called for, say, to see the adapting KB definitions, we don't
		// need to make another call of ListKbs() to the kbserver to get the glossing ones
		// - we already have those in a separate list. But it convolutes the code if we
		// want to inhibit a second ListKbs() call when, say, the glossing KB definitions
		// radio button is clicked. Much easier just to recall it, and separate into
		// types, and show the wanted set of KB definitions (ignoring the ones not
		// requested). 
		
		// Clear out anything from a previous button press done on this page
		// ==================================================================
		m_pSourceKbsListBox->Clear();
		m_pNonSourceKbsListBox->Clear();
		wxCommandEvent dummy;
		OnButtonCreateKbsPageClearControls(dummy);
		DeleteClonedKbServerKbStruct(); // ensure it's freed & ptr is NULL
		m_pKbServer->ClearKbsList(m_pOriginalKbsList);
		if (!m_pKbsList_Tgt->empty())
		{
			m_pKbsList_Tgt->clear(); // remove the ptr copies
		}
		if (!m_pKbsList_Gls->empty())
		{
			m_pKbsList_Gls->clear(); // remove the ptr copies
		}
		// The m_pKbsList will be cleared within the ListKbs() call done below, so it
		// is unnecessary to do it here now
		// ==================================================================

		// Get the kbs data from the server, store in the list of KbServerKb structs,
		// the call will clear the list first before storing what the server returns;
		// then separate the structs into two lists, by type (adapting versus glossing)
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("KBSharingMgrTabbedDlg::LoadDataForPage(): m_pKbServer = %x , m_bStateless = %d, page (1-based) = %d"),
		m_pKbServer, (int)m_pKbServer->m_bStateless, pageNumSelected + 1);
#endif
		wxString username = m_pKbServer->GetKBServerUsername(); // for authentication, and for the username 
																// field in the KB definition
		wxString password = m_pKbServer->GetKBServerPassword(); // for authentication
		CURLcode result = CURLE_OK;
		if (!username.IsEmpty() && !password.IsEmpty())
		{
			result = (CURLcode)m_pKbServer->ListKbs(username, password);
			if (result == CURLE_OK)
			{
				if (m_pKbsList->empty())
				{
					// Don't expect an empty list, so an English message will suffice
					wxMessageBox(_T("LoadDataForPage() error, create KBs page, the returned list of KBs was empty. Fix this."),
					_T("KB Sharing Manager error"), wxICON_WARNING | wxOK);
					m_pKbServer->ClearKbsList(m_pKbsList);
					return;
				}

				// Copy the list, before user gets a chance to modify anything
				CopyKbsList(m_pKbsList, m_pOriginalKbsList); // param 1 is src list, param2 is dest list

				// Separate into lists of adapting versus glossing KB definition structs
				SeparateKbServerKbStructsByType(m_pKbsList, m_pKbsList_Tgt, m_pKbsList_Gls);

				// Load according to the page's state: either adapting KB definitions, or
				// glossing KB definitions - as determined by the state of the radio
				// buttons.
                // Load the source code strings into m_pSourceKbsListBox, and the
                // m_pNonSourceKbsListBox; neither of these listboxes is sorted, but the
                // codes in the left one are in sorted order by src code followed by space
                // then the non-source code - so the right hand list shows sorting by the
                // non-source code, whenever there are more than one source codes which are
                // the same. We do that by preloading them into a sorted wxListBox
                // temporary instance created internally, and then having gotten the
                // required order, we scan this internal temporary list box and create the
                // entries in the two visible list boxes in the top down (sorted) order
                // that we want the user to see. This enables us to easily keep the
                // correspondences between src-tgt, or src-gloss pairs for the two list
                // boxes. The corresponding tgt or glosses codes are listed in the
                // "Corresponding..." list - that one is on the right of the two.
                // Note 1: the client data for each KB source code string added to the left
                // list box is the ptr to the KbServerKb struct itself which supplied the
                // KB source code string, the struct coming from m_pKbsList.
				// Note 2: the first param, m_bKBisType1 selects which of the following
				// params are used internally: if TRUE is passed in, then the two lists
				// are populated using the contents of m_pKbsList_Tgt list. If FALSE is
				// passed in, then instead the two lists are populated by the
				// m_pKbsList_Gls list
				LoadLanguageCodePairsInListBoxes_CreatePage(m_bKBisType1, m_pKbsList_Tgt, 
							m_pKbsList_Gls, m_pSourceKbsListBox, m_pNonSourceKbsListBox);
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
			wxMessageBox(_T("LoadDataForPage() unable to call ListKbs(), for create KB definitions page, because password or username is empty, or both. This Manager page won't work properly until this is fixed."),
				_T("KB Sharing Manager error"), wxICON_WARNING | wxOK);
		}
	}
	else // must be second Kbs page -- the one for editing existing KB definitions
	{

// TODO ***************  this block


	}
}

// Return the pointer to the struct - this should never fail, but we'll return NULL if it does
KbServerUser* KBSharingMgrTabbedDlg::GetThisUsersStructPtr(wxString& username, UsersList* pUsersList)
{
	wxASSERT(pUsersList);
	KbServerUser* pEntry = NULL;
	if (pUsersList->empty())
		return (KbServerUser*)NULL;
	UsersList::iterator iter;
	UsersList::compatibility_iterator c_iter;
	int anIndex = -1;
	for (iter = pUsersList->begin(); iter != pUsersList->end(); ++iter)
	{
		// Assign the KbServerUser struct's pointer to pEntry
		anIndex++;
		c_iter = pUsersList->Item((size_t)anIndex);
		pEntry = c_iter->GetData();
		if (pEntry->username == username)
		{
			// We found the struct, so return it
			return pEntry;
		}
	}
	// control should not get here, but if it does, return NULL
	return (KbServerUser*)NULL;
}

// Returns nothing. The ListBox is in sorted order by the usernames. This is achieved by
// having the wxListBox be unsorted, and pre-storing the usernames in a
// wxSortedArrayString, and then adding the ID values at the time the array entries are
// added to the wxLixBox. The KbServerUser struct pointer is added, for each username,
// after the lines are added to the list
void KBSharingMgrTabbedDlg::LoadUsersListBox(wxListBox* pListBox, size_t count, UsersList* pUsersList)
{
	wxASSERT(count != 0); wxASSERT(pListBox); wxASSERT(pUsersList);
	if (pUsersList->empty())
		return;

	// Get the usernames into sorted order, by adding them to a wxSortedArrayString
	wxSortedArrayString sorted_arrUsernames;
	UsersList::iterator iter;
	UsersList::compatibility_iterator c_iter;
	int anIndex = -1;
	int maxID = 0;
	wxString strID;
	wxString tab = _T('\t');
	wxString username;
	for (iter = pUsersList->begin(); iter != pUsersList->end(); ++iter)
	{
		// Get the max ID value
		anIndex++;
		c_iter = pUsersList->Item((size_t)anIndex);
		KbServerUser* pEntry = c_iter->GetData();
// Check if there are any NULL ptrs here
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
		wxLogDebug(_T("LoadUsersListBox(): index = %d , pEntry = %x , username = %s , ID = %d"),
			anIndex, (void*)pEntry, pEntry == NULL ? _T("null pointer") : pEntry->username.c_str(), pEntry->id);
#endif
		if (pEntry->id > maxID)
		{
			maxID = pEntry->id;
		}
	}
	int maxChars = 0;
	if (maxID <= 9)          {maxChars = 1;}
	else if (maxID < 99)     {maxChars = 2;}
	else if (maxID < 999)    {maxChars = 3;}
	else if (maxID < 9999)   {maxChars = 4;}
	else if (maxID < 99999)  {maxChars = 5;}
	else if (maxID < 999999) {maxChars = 6;}
	else if (maxID < 9999999){maxChars = 7;}
	else                     {maxChars = 8;}	
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
		wxLogDebug(_T("LoadUsersListBox(): maxID = %d , number of characters in maxID = %d"), maxID, maxChars);
#endif

	anIndex = -1;
	for (iter = pUsersList->begin(); iter != pUsersList->end(); ++iter)
	{
		// Assign the KbServerUser struct's pointer to pEntry, add the ID as a string at
		// the end of the username with TAB character as a delimiter (we'll later move it
		// to the front of the string to be inserted into the listbox)
		anIndex++;
		c_iter = pUsersList->Item((size_t)anIndex);
		KbServerUser* pEntry = c_iter->GetData();
		username = pEntry->username;
		username += tab;
		wxItoa(pEntry->id, strID);
        // Add preceding spaces, so that there are a const number, maxChars, of characters
        // up to the end of each ID, and then 3 spaces following each (assume IDs can go up
        // to any ID value, actually, < 99,999,999)
		int idLen = strID.Len();
		int spacesToAdd = maxChars - idLen;
		wxString spaces = _T("");
		if (spacesToAdd > 0)
		{
			int i;
			for (i = 0; i<spacesToAdd; i++) { spaces += _T(' ');}
			strID = spaces + strID;
		}
		username += strID;
		username += _T("   "); // 3 spaces (will be the delimiter between the 
							   // ID and the username which follows
		// Now add the composite string to the sorted array
		sorted_arrUsernames.Add(username);
	}
	// Now load the list box, the ID values will appear first, followed by 3 spaces, then
	// the usernames
	int index;
	for (index = 0; index < (int)count; index++)
	{
		wxString usernamePlusID = sorted_arrUsernames.Item(index);
		int offset = usernamePlusID.Find(tab);
		wxASSERT(offset != wxNOT_FOUND);
		username = usernamePlusID.Left(offset);
		wxString prefix = usernamePlusID.Mid(offset + 1);
		KbServerUser* pClientData = GetThisUsersStructPtr(username, pUsersList);
		wxASSERT(pClientData != NULL);
		pListBox->Append(prefix + username, pClientData);
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

// Make deep copies of the KbServerKb struct pointers in pSrcList and save them to pDestList
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
		pNew->id = pEntry->id;
		pNew->sourceLanguageCode = pEntry->sourceLanguageCode;
		pNew->targetLanguageCode = pEntry->targetLanguageCode;
		pNew->kbType = pEntry->kbType;
		pNew->username = pEntry->username;
		pNew->timestamp = pEntry->timestamp;
		pNew->deleted = pEntry->deleted;
		pDestList->Append(pNew);
	}
}


void KBSharingMgrTabbedDlg::SeparateKbServerKbStructsByType(KbsList* pAllKbStructsList, 
								KbsList* pKbStructs_TgtList, KbsList* pKbStructs_GlsList)
{
	// For params 2 and 3, pass in members m_pKbsList_Tgt and m_pKbsList_Gls; for param 1,
	// pass in the list of KbServerKb structs obtained from the ListKbs() call of the
	// stateless KbServer (temporary) instance
	m_pKbServer->ClearKbsList(pKbStructs_TgtList);
	m_pKbServer->ClearKbsList(pKbStructs_GlsList);
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
		/* try just doing shallow copies
		KbServerKb* pNew = new KbServerKb; // we make deep copies on the heap
		pNew->id = pEntry->id;
		pNew->sourceLanguageCode = pEntry->sourceLanguageCode;
		pNew->targetLanguageCode = pEntry->targetLanguageCode;
		pNew->kbType = pEntry->kbType;
		pNew->username = pEntry->username;
		pNew->timestamp = pEntry->timestamp;
		pNew->deleted = pEntry->deleted;
		*/
		if (pEntry->kbType == (int)1)
		{
			//pKbStructs_TgtList->Append(pNew);
			pKbStructs_TgtList->Append(pEntry); // shallow copy
		}
		else
		{
			//pKbStructs_GlsList->Append(pNew);
			pKbStructs_GlsList->Append(pEntry); // shallow copy
		}
	}
	// Note, we cannot assume everyone will create a glossing KB definition to match each
	// created adapting KB definition. For instance, Jonathan didn't. If a glossing one is
	// lacking, then glosses in that project's glossing mode won't get shared, but nothing
	// should break (we hope)
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
	// If we get to here user selected a different page
	m_nCurPage = pageNumSelected;

	//Set up new page data by populating list boxes and controls
	LoadDataForPage(m_nCurPage);
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void KBSharingMgrTabbedDlg::OnOK(wxCommandEvent& event)
{
    // This is a stateless dialog - nothing we did is communicated back to the application
    // (it's the remote kbserver instance that has it all, if we did anything that is)

	// Tidy up
	m_pKbServer->ClearUsersList(m_pOriginalUsersList); // this one is local to this
	delete m_pOriginalUsersList;
	m_pKbServer->ClearUsersList(m_pUsersList); // this one is in the stateless
									// KbServer instance & don't delete this one
	DeleteClonedKbServerUserStruct();

	// Tidy up for the Create KB Definitions page
	m_pKbServer->ClearKbsList(m_pOriginalKbsList); // this one is local to this
	delete m_pOriginalKbsList;
	if (!m_pKbsList_Tgt->empty())
	{
		m_pKbsList_Tgt->clear();
		delete m_pKbsList_Tgt;
	}
	if (!m_pKbsList_Gls->empty())
	{
		m_pKbsList_Gls->clear();
		delete m_pKbsList_Gls;
	}
	m_pKbServer->ClearKbsList(m_pKbsList); // this one is in the stateless
									// KbServer instance & don't delete this one
	DeleteClonedKbServerKbStruct();

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

	// Tidy up for the Create KB Definitions page
	m_pKbServer->ClearKbsList(m_pOriginalKbsList); // this one is local to this
	delete m_pOriginalKbsList;
	if (!m_pKbsList_Tgt->empty())
	{
		m_pKbsList_Tgt->clear();
		delete m_pKbsList_Tgt;
	}
	if (!m_pKbsList_Gls->empty())
	{
		m_pKbsList_Gls->clear();
		delete m_pKbsList_Gls;
	}
	m_pKbServer->ClearKbsList(m_pKbsList); // this one is in the stateless
									// KbServer instance & don't delete this one
	DeleteClonedKbServerKbStruct();

	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void KBSharingMgrTabbedDlg::OnButtonUserPageClearControls(wxCommandEvent& WXUNUSED(event))
{
	wxString emptyStr = _T("");
	m_pUsersListBox->SetSelection(wxNOT_FOUND);
	m_pTheUsername->ChangeValue(emptyStr);
	m_pEditInformalUsername->ChangeValue(emptyStr);
	m_pEditPersonalPassword->ChangeValue(emptyStr);
	m_pEditPasswordTwo->ChangeValue(emptyStr);
	m_pCheckUserAdmin->SetValue(FALSE);
	m_pCheckKbAdmin->SetValue(FALSE);
}

void KBSharingMgrTabbedDlg::OnButtonCreateKbsPageClearControls(wxCommandEvent& WXUNUSED(event))
{
	wxString emptyStr = _T("");
	m_pSourceKbsListBox->SetSelection(wxNOT_FOUND);
	m_pNonSourceKbsListBox->SetSelection(wxNOT_FOUND);

	m_pEditSourceCode->ChangeValue(emptyStr);
	m_pEditNonSourceCode->ChangeValue(emptyStr);
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
	// and inform the user why. Also, a selection in the list is irrelevant, and it may
	// mean that the logged in person is about to try to add the selected user a second
	// time - which is illegal, so test for this and if so, warn, and remove the
	// selection, and clear the controls & return
	m_nSel = m_pUsersListBox->GetSelection();
	wxString strUsername = m_pTheUsername->GetValue();
	if (m_nSel != wxNOT_FOUND)
	{
		m_nSel = m_pUsersListBox->GetSelection();
		wxString selectedUser = m_pUsersListBox->GetString(m_nSel);
		if (selectedUser == strUsername)
		{
			// Oops, he's trying to add someone who is already there. Warn, and probably
			// best to clear the controls to force him to start over
			wxBell();
			wxString title = _("This user already exists");
			wxString msg = _("Warning: you are trying to add an entry which already exists in the server. This is illegal, each username must be unique.\n To add a new user, do not make any selection in the list; just use the text boxes, the checkboxes too if appropriate, and the Add User button.");
			wxMessageBox(msg, title, wxICON_WARNING | wxOK);
			wxCommandEvent dummy;
			OnButtonUserPageClearControls(dummy);
			//m_pUsersListBox->SetSelection(wxNOT_FOUND);
			return;
		}
	}
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
		// Prevent removal if the user is the useradmin == true one which is the earliest
		// (ie. the lowest ID value), and tell the user that's why his attempt was rejected
		if (m_pOriginalUserStruct->username == m_earliestUseradmin)
		{
			// This guy must not be deleted
			wxBell();
			wxString msg = _("This user is the earliest user with administrator privilege level. This user cannot be removed.");
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
            // The removal did not succeed- probably because it is parent to entries in the
            // entry table -- an error message will have been shown from within the above
            // RemoveUser() call
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
    // unticked (i.e. it was ticked earlier and now isn't) then we only allow the privilege
    // demotion to apply to a user who is not the earliest one (ie. has lowest ID value)
    // who also has administrator privilege level. Why? We must ensure there is always at
    // least one user who can add, edit or remove users. This particular special user is
    // the one in the m_earliestUsername member variable. If that's not the one we are
    // demoting we leave the kbadmin value untouched.
	// 
	// Also, we must avoid having useradmin TRUE but kbadmin FALSE, for any one username.
	if (bCurrentUseradminValue)
	{
		// This click was a promotion to user administrator privilege level, so he must
		// also be given kb privilege level as well
		m_pCheckKbAdmin->SetValue(TRUE);
	}
	else
	{
		// The click was to demote from user administrator privilege level...
		//
        // But it's not permitted to demote the user who is the earliest with useradmin
        // privilege level to either just kbadmin privilege or no privilege. 
        // Check, and if that is being attempted. If so, warn the user and return having
        // restored useradmin privilege
        
		// Get the privilege level value as it was when the user was clicked in the listbox
		if (m_pOriginalUserStruct->username != m_earliestUseradmin)
		{
			// This guy (on the left of the inequality test) can be safely demoted, because
			// he's not the lowest ID value'd useradmin person
			m_pCheckUserAdmin->SetValue(FALSE);
		}
		else
		{
			// The click was on the useradmin == TRUE user who has lowest ID value - this
			// guy has to retain his useradmin == TRUE permission level
			wxString title = _("Illegal demotion");
			wxString msg = _("Warning: you are not permitted to demote whoever was first added with user administrator permission. That user is: %s\n\n(There must be at least one user with user administrator privilege, and this protection ensures that this condition is satisfied.)");
			msg = msg.Format(msg, m_earliestUseradmin.c_str());
			wxMessageBox(msg, title, wxICON_WARNING | wxOK);

			m_pCheckUserAdmin->SetValue(TRUE); // restore the ticked state of the useradmin checkbox			
		}
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
			//m_pEditUsername->ChangeValue(originalUsername);
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

	// Update the user's details in the kbserver's user table
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
	// Get the username - the one clicked in the list is not helpful because we'd
	// need to remove the ID and spaces preceding the username, so better to get it
	// from the client data's stored struct
	wxString theUsername = m_pUsersListBox->GetString(m_nSel);
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
    wxLogDebug(_T("OnSelchangeUsersList(): from list... ID & username = %s"), theUsername.c_str());
#endif
	// Try putting theUsername into a 2nd textbox after the first -- this was necessary on
	// Linux to get past a bug in which Linux stubbornly refused to display the text in
	// it, even though it accepted it being inserted. After that I removed the pesky top
	// edit box and just retained the newer one, and Linux then behaved itself

	// Get the entry's KbServerUser struct which is its associated client data
	m_pUserStruct = (KbServerUser*)m_pUsersListBox->GetClientData(m_nSel);
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
    wxLogDebug(_T("OnSelchangeUsersList(): ptr to client data... m_pUserStruct = %x"), (void*)m_pUserStruct);
#endif
	// Set the username from the struct, it doesn't have the ID prefixed to it
	theUsername = m_pUserStruct->username;

	// Put a copy in m_pOriginalUserStruct, in case the administrator clicks the
	// Edit User button - the latter would use what's in this variable to compare
	// the user's edits to see what has changed, and proceed accordingly. The
	// Remove User button also uses the struct in m_pOriginalUserStruct to get
	// at the ID value for the user to be removed
	DeleteClonedKbServerUserStruct();
	m_pOriginalUserStruct = CloneACopyOfKbServerUserStruct(m_pUserStruct);
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
    wxLogDebug(_T("OnSelchangeUsersList(): ptr to client data... m_pOriginalUserStruct = %x"), (void*)m_pOriginalUserStruct);
    wxLogDebug(_T("OnSelchangeUsersList(): m_pUserStruct->username = %s  ,  useradmin = %d , kbadmin = %d"), 
		m_pUserStruct->username.c_str(), m_pUserStruct->useradmin, m_pUserStruct->kbadmin);
#endif

	// Use the struct to fill the Users page's controls with their required data; the
	// logged in user can then edit the parameters (m_pTheUsername is the ptr to the
	// username wxTextCtrl)
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
    wxLogDebug(_T("OnSelchangeUsersList(): username box BEFORE contains: %s"), m_pTheUsername->GetValue().c_str());
#endif
	m_pTheUsername->ChangeValue(theUsername);

#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
    wxLogDebug(_T("OnSelchangeUsersList(): username box AFTER contains: %s"), m_pTheUsername->GetValue().c_str());
#endif

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


//*******************************************************************************************
//
//************************** Functions for Create KB Definitions page ***********************
//
//*******************************************************************************************


void KBSharingMgrTabbedDlg::OnRadioButton1CreateKbsPageType1(wxCommandEvent& WXUNUSED(event))
{
	// The user's click has already changed the value held by the radio button
	m_pRadioKBType1->SetValue(TRUE);
	m_pRadioKBType2->SetValue(FALSE);

	// Set the appropriate label for the non-source listbox
	m_pNonSrcCorrespCodesListLabel->SetLabel(m_correspTgtLanguageCodesLabel);

	// Set the appropriate label text for the second text control
	m_pNonSrcLabel->SetLabel(m_tgtLanguageCodeLabel);

	// Record which type of KB we are defining
	m_bKBisType1 = TRUE;

	// Get it's data displayed (each such call "wastes" one of the sublists, we only
	// display the sublist wanted, and a new ListKbs() call is done each time -- not
	// optimal for efficiency, but it greatly simplifies our code)
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnRadioButton1CreateKbsPageType1(): This page # (m_nCurPage + 1) = %d"), m_nCurPage + 1);
#endif
	LoadDataForPage(m_nCurPage);
}

void KBSharingMgrTabbedDlg::OnRadioButton2CreateKbsPageType2(wxCommandEvent& WXUNUSED(event))
{
	// The user's click has already changed the value held by the radio button
	m_pRadioKBType2->SetValue(TRUE);
	m_pRadioKBType1->SetValue(FALSE);

	// Set the appropriate label for the non-source listbox
	m_pNonSrcCorrespCodesListLabel->SetLabel(m_correspGlsLanguageCodesLabel);

	// Set the appropriate label text for the second text control
	m_pNonSrcLabel->SetLabel(m_glossesLanguageCodeLabel);

	// Record which type of KB we are defining
	m_bKBisType1 = FALSE;

	// Get it's data displayed (each such call "wastes" one of the sublists, we only
	// display the sublist wanted, and a new ListKbs() call is done each time -- not
	// optimal for efficiency, but it greatly simplifies our code)
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnRadioButton2CreateKbsPageType2(): This page # (m_nCurPage + 1) = %d"), m_nCurPage + 1);
#endif
	LoadDataForPage(m_nCurPage);
}

void KBSharingMgrTabbedDlg::OnBtnCreatePageLookupCodes(wxCommandEvent& WXUNUSED(event))
{
	// Call up CLanguageCodesDlg here so the user can enter language codes for
	// the source and target languages, or for the source and glossing languages, accoring
	// to the LangCodesChoice modality.
	LangCodesChoice whichPairType = source_and_target_only; 
	if (m_bKBisType1 != TRUE)
	{
		// It's not source-to-target that we are dealing with, but glossing mode - that
		// is, source-to-glosses, so choose the other enum value to pass in
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
		m_sourceLangCode = lcDlg.m_sourceLangCode;
		m_targetLangCode = lcDlg.m_targetLangCode;
		m_pEditSourceCode->ChangeValue(m_sourceLangCode);
		m_pEditNonSourceCode->ChangeValue(m_targetLangCode);
	}
	else if (whichPairType == source_and_glosses_only)
	{
		m_sourceLangCode = lcDlg.m_sourceLangCode;
		m_glossLangCode = lcDlg.m_glossLangCode;
		m_pEditSourceCode->ChangeValue(m_sourceLangCode);
		m_pEditNonSourceCode->ChangeValue(m_glossLangCode);
	}
	else
	{
		// We've wrongly called the constructor for not KB Sharing Manager instance so
		// just clear the boxes
		wxBell();
		m_pEditSourceCode->ChangeValue(_T(""));
		m_pEditNonSourceCode->ChangeValue(_T(""));
	}
}

void KBSharingMgrTabbedDlg::LoadLanguageCodePairsInListBoxes_CreatePage(bool bKBTypeIsSrcTgt,
							KbsList* pSrcTgtKbsList, KbsList* pSrcGlsKbsList,
							wxListBox* pSrcCodeListBox, wxListBox* pNonSrcCodeListBox)
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
	pSrcCodeListBox->Clear();
	pNonSrcCodeListBox->Clear();
	// Make the left listbox sorted, put a composite string in it, and store the
	// KbServerKb struct pointers as client data; then make unsorted, and scan over it and read the client
	// data and make the string be just the source language code, and use the client data
	// to construct the right hand list of non-source language codes
	long mystyle = pSrcCodeListBox->GetWindowStyleFlag();
	// Make this listbox be temporarily sorted while we populate it
	mystyle = mystyle | wxLB_SORT | wxLB_SINGLE;
	pSrcCodeListBox->SetWindowStyleFlag(mystyle);
	
    // Now scan the list of KbServerKb struct pointers, construct an "entry" which is the
    // source language code followed by a space followed by the target code (or gloss code,
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
			wxString compositeStr = pEntry->sourceLanguageCode;
			compositeStr += _T(" ");
			compositeStr += pEntry->targetLanguageCode; // don't be misled: according to 
					// bKBTypeIsSrcTgt's value, this might be the genuine tgt lang code, or
					// instead, the glossing language's code (one variable name used for
					// either in the KbServerKb struct)
			pSrcCodeListBox->Append(compositeStr, (void*)pEntry);
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
    wxLogDebug(_T("LoadLanguageCodePairsInListBoxes(): compositeStr = %s , KbServerKb* pEntry = %x"), compositeStr.c_str(), (void*)pEntry);
#endif
		}
	}
    // Now we have the sorted order we want. Grab the entries in top-down order, and
    // re-create the strings of the rows to just be the source language code (on the left)
    // and for the pNonSrcCodeListBox (on the right) which the user will get to see; use
    // the the client data to get the non-src code and populate the matching rows of the
    // right list
	unsigned int count;
	count = pSrcCodeListBox->GetCount();
 #if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
    wxLogDebug(_T("LoadLanguageCodePairsInListBoxes():  count for pSrcCodeListBox = %d"), count);
#endif
   
	// Now make the left list, pSrcCodeListBox, be unsorted, so that the rows won't be
	// rearranged as we change the text within them
    mystyle = 0L;
	mystyle = mystyle | wxLB_SINGLE;
	pSrcCodeListBox->SetWindowStyleFlag(mystyle);
    // ensure pNonSrcCodeListBox is also unsorted (for some reason the right hand side one
    // keeps sorting everything appended -- so try a couple of calls)
	//pNonSrcCodeListBox->SetWindowStyleFlag(0L);
	pNonSrcCodeListBox->SetWindowStyleFlag(mystyle);

	// loop over the rows in the listbox
	unsigned int index;
	for (index = 0; index < count; index++)
	{
		// Get the client data's struct ptr
		KbServerKb* pKbServerKbStruct = (KbServerKb*)pSrcCodeListBox->GetClientData(index);
		// Get the source language code
		wxString srcLangCode = pKbServerKbStruct->sourceLanguageCode;
		// Get the non-source language code (either target lang code, or glossing lang code)
		wxString nonsrcLangCode = pKbServerKbStruct->targetLanguageCode;
		// Reset the left list box's composite str to the source language code
		pSrcCodeListBox->SetString(index, srcLangCode);
		// Append target language code, or gloss language code, to the right list
		pNonSrcCodeListBox->Append(nonsrcLangCode);
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
		wxLogDebug(_T("LoadLanguageCodePairsInListBoxes():  appending nonsrc:  %s  [to pNonSrcCodeListBox]"), nonsrcLangCode.c_str());
#endif
	}

    // The visible list boxes are now populated, set no selection in either (note, when
    // later selecting in one, we have to compute the equivalent selection in the other,
    // and make both selections visible and horizontally lined up)
	pSrcCodeListBox->SetSelection(wxNOT_FOUND);
	pNonSrcCodeListBox->SetSelection(wxNOT_FOUND);
}

void KBSharingMgrTabbedDlg::OnBtnCreatePageRFC5646Codes(wxCommandEvent& WXUNUSED(event))
{
	// Display the RFC5646message.htm file in the platform's web browser

}








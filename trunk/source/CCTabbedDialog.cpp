/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CCTabbedDialog.cpp
/// \author			Bill Martin
/// \date_created	19 June 2007
/// \date_revised	2 August 2011
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CCCTabbedDialog class.
/// The CCCTabbedDialog class provides a dialog with tabbed pages in which the user can load up to four
/// consistent changes tables for use in Adapt It. Each tabbed page has controls that enable the user to
/// browse, select, create and/or edit consistent change (.cct) tables.
/// \derivation		The CCCTabbedDialog class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "CCTabbedDialog.h"
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
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
#include <wx/filesys.h> // for wxFileName
#include <wx/dir.h> // for wxDir
#include <wx/choicdlg.h> // for wxGetSingleChoiceIndex

#include "Adapt_It.h"
#include "helpers.h"
#include "CCTabbedDialog.h"
#include "CCTableEditDlg.h"
#include "CCTableNameDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// Length of the byte-order-mark (BOM) which consists of the three bytes 0xEF, 0xBB and 0xBF
/// in UTF-8 encoding.
#define nBOMLen 3

/// Length of the byte-order-mark (BOM) which consists of the two bytes 0xFF and 0xFE in
/// in UTF-16 encoding.
#define nU16BOMLen 2

//static wxUint8 szBOM[nBOMLen] = {0xEF, 0xBB, 0xBF};
//static wxUint8 szU16BOM[nU16BOMLen] = {0xFF, 0xFE};

// event handler table for the CCCTabbedDialog class
BEGIN_EVENT_TABLE(CCCTabbedDialog, AIModalDialog)
	EVT_INIT_DIALOG(CCCTabbedDialog::InitDialog)
	EVT_NOTEBOOK_PAGE_CHANGED(-1, CCCTabbedDialog::OnTabPageChanging) // value from EVT_NOTEBOOK_PAGE_CHANGING are inconsistent across platforms - better to use ..._CHANGED
	EVT_BUTTON(IDC_BUTTON_BROWSE, CCCTabbedDialog::OnButtonBrowse)
	EVT_LISTBOX(IDC_LIST_CCTABLES, CCCTabbedDialog::OnSelchangeListCctables)
	EVT_LISTBOX_DCLICK(IDC_LIST_CCTABLES, CCCTabbedDialog::OnDblclkListCctables)
	EVT_BUTTON(IDC_BUTTON_EDIT_CCT, CCCTabbedDialog::OnButtonEditCct)
	EVT_BUTTON(IDC_BUTTON_SELECT_NONE, CCCTabbedDialog::OnButtonSelectNone)
	EVT_BUTTON(IDC_BUTTON_CREATE_CCT, CCCTabbedDialog::OnButtonCreateCct)
END_EVENT_TABLE()

CCCTabbedDialog::CCCTabbedDialog(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Get One Or More Consistent Change Tables"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{

	// wx Note: Since InsertPage also calls SetSelection (which in turn activates our OnTabSelChange
	// handler, we need to initialize some variables before CCTabbedNotebookFunc is called below.
	// Specifically m_nCurPage and pKB needs to be initialized - so no harm in putting all vars
	// here before the dialog controls are created via KBEditorDlgFunc.
	m_nCurPage = 0; // default to first page (1 Word)
	m_nPrevSelection = -1;
#ifdef __WXGTK__
	m_bListBoxBeingCleared = FALSE;
#endif
	// whm Note: If m_bUseConsistentChanges is TRUE when the Tools | Load Consistent Changes... menu
	// item is called, we want to populate the CCTabbledDialog with data reflecting the currently
	// selected changes tables in the Table tabs of the dialog.
/*
	bool			m_bCCTableLoaded[4];
	bool			m_bTablesLoaded;
	// paths to cc tables & their names
	wxString		m_tableName[4];
	wxString		m_tableFolderPath[4];
	wxString		m_lastCcTablePath;
*/
	int ct;
	for (ct = 0; ct < 4; ct++)
	{
		m_tblName[ct] = gpApp->m_tableName[ct];
		m_folderPath[ct] = gpApp->m_tableFolderPath[ct];
		m_bTableLoaded[ct] = gpApp->m_bCCTableLoaded[ct];
	}

	// The following strings are localizable and used to mark .cct file names in the m_pListBox to
	// indicate in which Table tab they are being used.
	usedInTableMarker[0] = _(" - [USED in Table 1]");
	usedInTableMarker[1] = _(" - [USED in Table 2]");
	usedInTableMarker[2] = _(" - [USED in Table 3]");
	usedInTableMarker[3] = _(" - [USED in Table 4]");

	CCTabbedNotebookFunc(this, TRUE, TRUE);
	// The declaration is: CCTabbedNotebookFunc( wxWindow *parent, bool call_fit, bool set_sizer );

	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	// pointers to the controls common to each page (most of them) are obtained within
	// the LoadDataForPage() function

	// the following pointer to the CC wxNotebook control is a single instance;
	// it can only be associated with a pointer after the CCTabbedNotebookFunc above call
	m_pCCTabbedNotebook = (wxNotebook*)FindWindowById(ID_CC_TABBED_NOTEBOOK);
	wxASSERT(m_pCCTabbedNotebook != NULL);
}

CCCTabbedDialog::~CCCTabbedDialog()
{
}

void CCCTabbedDialog::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	// Initdialog is called once before the dialog is shown.
	LoadDataForPage(0); // start with first item selected for new tab page - it calls UpdateButtons()
	ShowUsageOfListBoxItems();
	UpdateButtons();
	m_pListBox->SetFocus();
}

void CCCTabbedDialog::LoadDataForPage(int pageNumSel)
{
	// In the wx vesion wxDesigner has created identical sets of controls for each
	// page (nbPage) in our wxNotebook. We need to associate the pointers with the correct
	// controls here within LoadDataForPage() which is called initially and for each
	// tab page selected. The pointers to controls will differ for each page, hence
	// they need to be reassociated for each page. Note the nbPage-> pefix on
	// FindWindow(). I've chosen not to use Validators here because of the complications
	// of having pointers to controls differ on each page of the notebook; instead we
	// manually transfer data between dialog controls and their variables.

	// set the page selection and get pointer to the page in wxNotebook
	m_pCCTabbedNotebook->SetSelection(pageNumSel);
	wxNotebookPage* nbPage = m_pCCTabbedNotebook->GetPage(pageNumSel);

	// Get pointers to the controls created in CCTablePageFunc() by wxDesigner.
	// Use FindWindow here because there may be up to 4 pages created each having controls by the
	// same ID. FindWindow() does not search other windows for IDs as does FindWindowById().
	m_pListBox = (wxListBox*)nbPage->FindWindow(IDC_LIST_CCTABLES);
	wxASSERT(m_pListBox != NULL);

	m_pEditSelectedTableName = (wxTextCtrl*)nbPage->FindWindow(IDC_EDIT_SELECTED_TABLE);
	wxASSERT(m_pEditSelectedTableName != NULL);

	m_pEditFolderPath = (wxTextCtrl*)nbPage->FindWindow(IDC_EDIT_FOLDER_PATH);
	wxASSERT(m_pEditFolderPath != NULL);


	// below are pointers to the buttons on this nbPage page
	m_pBtnBrowse = (wxButton*)nbPage->FindWindow(IDC_BUTTON_BROWSE);
	wxASSERT(m_pBtnBrowse != NULL);
	m_pBtnCreateCct = (wxButton*)nbPage->FindWindow(IDC_BUTTON_CREATE_CCT);
	wxASSERT(m_pBtnCreateCct != NULL);
	m_pBtnEditCct = (wxButton*)nbPage->FindWindow(IDC_BUTTON_EDIT_CCT);
	wxASSERT(m_pBtnEditCct != NULL);
	m_pBtnSelectNone = (wxButton*)nbPage->FindWindow(IDC_BUTTON_SELECT_NONE);
	wxASSERT(m_pBtnSelectNone != NULL);

	// make the fonts show user's desired point size in the dialog
	//
	// whm Note: Why do we set a special font for the m_pEditFolderPath's edit box? It results in a
	// larger than normal (12 point) font size which generally makes it extend beyond the right end of the
	// edit box often hiding the actual name of the cct table file. The result is that the user has
	// to use the supplied horizontal scroll bar to see the name of the most important part of the path.
	// whm 11Jul11 commented out the following calls, so the m_pEditfolderPath, m_pListBox, and
	// m_pEditSelectedTableName will use the 9 point default dialog font. Previously the font was too
	// big to fit in the multiline edit control. Paths and file names don't need special font treatment.
	//#ifdef _RTL_FLAGS
	//gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, m_pEditFolderPath, NULL,
	//							NULL, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bNavTextRTL);
	//#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	//gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, m_pEditFolderPath, NULL,
	//							NULL, NULL, gpApp->m_pDlgTgtFont);
	//#endif

	//#ifdef _RTL_FLAGS
	//gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pEditSelectedTableName, NULL,
	//							m_pListBox, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bNavTextRTL);
	//#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	//gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pEditSelectedTableName, NULL,
	//							m_pListBox, NULL, gpApp->m_pDlgSrcFont);
	//#endif

	// The following App globals have local equivalents here in CCTabbedDialog:
	//         App Globals:                         CCTabbedDialog Equivalents:
	// ------------------------------------------------------------------------
	// bool            m_bCCTableLoaded[4];            bool m_bTableLoaded[4];
	// wxString        m_tblName[4];                   wxString m_tblName[4];
	// wxString        m_tableFolderPath[4];           wxString m_folderPath[4];
	// bool            m_bTablesLoaded;                        n/a
	// wxString        m_lastCcTablePath;                     n/a

	m_pListBox->Clear();

    // MFC Note: negotiate to get the best possible initial folder path - if a table has been found
    // previously, then that table's folder path would be better than the initial one supplied on launch
    // of the property sheet (and will be obtainable from the app's m_lastCcTablePath variable which is
    // updated in each Browse... button handler).
    //
    // whm Note: On subsequent invocations of the property sheet MFC's OnInitDialog() always sets the
    // current local m_folderPath to the m_lastCcTablePath on the App. That is OK if the user only ever
    // uses one .cct table on one path. But, I don't think this is desirable if the user loads two or
    // more .cct tables in different Table tabs - one or more coming from different "browsed to" paths.
    // For an initial invocation of the property sheet the "Folder path" edit box will show any
    // different paths used for the given .cct tables, but on subsequent invocations (while CC is
    // active) the edit box will only show the last browsed to path.
    //
    // In redesigning for clarity, I think we want subsequent invocations to accurately show the data on
    // the tab pages (including folder path) so it is consistent with what was previously loaded. In
    // particular, we want the "Folder Path" editbox to show the actual path for any .cct file loaded in
    // a given Table tab page.
    //
    // whm revised 6Aug11.
	// Check whether navigation protection is in effect for _CCTABLE_INPUTS_OUTPUTS,
	// and whether the App's m_lastCcTablePath is empty or has a valid path,
	// and set the appropriate m_folderPath[] for the CC table accordingly.
	if (gpApp->m_bProtectCCTableInputsAndOutputsFolder)
	{
		// Navigation protection is ON, so force the use of the special protected
		// folder.
		m_folderPath[m_nCurPage] = gpApp->m_ccTableInputsAndOutputsFolderPath;
	}
	else if (gpApp->m_lastCcTablePath.IsEmpty()
		|| (!gpApp->m_lastCcTablePath.IsEmpty() && !::wxDirExists(gpApp->m_lastCcTablePath)))
	{
		// Navigation protection is OFF so we set the flag to allow the wxFileDialog
		// to appear. But the m_lastCcTablePath is either empty or, if not empty,
		// it points to an invalid path, so we initialize the defaultDir to point to
		// the special protected folder _CCTABLE_INPUTS_OUTPUTS, even though Navigation
		// protection is not ON. In this case, the user could point the export path
		// elsewhere using the wxFileDialog that will appear.
		m_folderPath[m_nCurPage] = gpApp->m_ccTableInputsAndOutputsFolderPath;
	}
	else
	{
		// Navigation protection is OFF and we have a valid path in m_lastCcTablePath,
		// so we initialize the defaultDir to point to the m_lastCcTablePath for the
		// location of the export. The user could still point the export path elsewhere
		// in the wxFileDialog that will appear.
		m_folderPath[m_nCurPage] = gpApp->m_lastCcTablePath; // this is where we will create it
	}

	if (!m_folderPath[m_nCurPage].IsEmpty())
	{
		// generate a wxArrayString of all the possible *.cct table filenames
		wxArrayString possibleTables;
		GetPossibleCCTables(&possibleTables); // uses current m_folderPath value

		// fill the list box with the c table filename strings
		int aCt;
		for (aCt = 0; aCt < (int)possibleTables.GetCount(); aCt++)
		{
			wxString str = possibleTables.Item(aCt);
			m_pListBox->Append(str);
		}

        // MFC Note: hilight the current one if possible, but if none is current then highlight the
        // first in the list
        //
        // whm modified 10Apr08 from MFC version behavior. I think the MFC version behavior is confusing
        // because when the property page comes up and there is more than one .cct table file in the
        // folder path, all four "Table n" tab pages in the dialog have the first .cct table in the list
        // selected and all four start with the same .cct table already in "The CC Table you have chosen
        // is:" edit box. For users that happen to have more than one .cct table in the remembered
        // folder path, this is not really a good initialization of the dialog, because, if the user
        // intends to only use one .cct table, and selects the desired .cct table from the list for
        // Table 1, then checks the other Table tabs, it will appear to him that Adapt It has already
        // selected the first .cct table from the list (which may or may not be the same cct table he
        // selected for Table 1) and is going to use it for Table 2, Table 3 and Table 4 making it
        // necessary for him to click "Select None" on all the Table 2, Table 3 and Table 4 tabs, when
        // he only wants to use the one .cct table he selected for Table 1 (likely the most common
        // situation). The MFC version seems to lead the user to choose the same .cct table for two,
        // three or all four of the Table tabs, which is not appropriate.
        //
        // Here is how I think the "Get One Or More Consistent Change Tables" dialog should work:
        // 1. If the remembered (default) folder path has only one .cct table in that folder, and
        // m_tblName is empty, then I think it is appropriate for Adapt It to automatically select that
        // one .cct table and put it into "The CC Table you have chosen is:" edit box. The user can then
        // just hit OK to use that .cct table, and the "Use Consistent Changes" menu item in the Tools
        // menu will also be automatically checked". Since we are not saving the last-use state (whether
        // we were using CC or not-using CC the last time we ran Adapt It) in the config file (perhaps
        // we should?), it seems the most likely scenario that the user would want to use the only .cct
        // table located in the remembered folder, and selecting it for him results in less work for the
        // user who wants to continue doing work in Adapt It that includes using Consistent Changes.
        // Moreover, if the user happens to click on the Table 2 tab, or the Table 3 tab, or the Table 4
        // tab, before clicking on OK, it should not look like Adapt It will also use that same .cct
        // table for the other Table tabs (as it does in the MFC version). The .cct table file should,
        // once chosen for a different Table tab, be shown in the list box, but marked as "[ALREADY
        // USED]" and it should not be automatically entered into "The CC Table you have chosen is:"
        // edit box in the other table tabs - as the MFC version currently does. Of course, if the user
        // wishes to find a different path for Table 2 he can then click on the Browse button and do so.
        // The list box should behave the same for that Table tab as I've described above for the Table
        // 1 tab.
        //
        // 2. If the remembered folder path (or any path browsed to) contains more than one .cct table,
        // and if m_tblName is empty, then I think it is inappropriate for Adapt It to automatically
        // enter the first .cct file into "The CC Table you have chosen is:" edit box. If a given .cct
        // table file has already been selected for use in another Table tab (and grayed out), then it
        // would, I think be OK for Adapt It to assume that the first non-grayed out .cct file in the
        // list could be at least highlighted, but I'm not sure if it should automatically be selected
        // into "The CC Table you have chosen is:" edit box. Perhaps it would be OK, because the use of
        // two tables is probably going to be much rarer than one table, three or four even less common,
        // so once one .cct table is chosen in Table 1, then it would be more likely that the remaining
        // table (that is not grayed out) would be the one the user would select for Table 2, and so on
        // for any Table 3 or Table 4.

        // We are loading the list box of .cct table files from OnInitDialog() or the
        // OnTabPageChanging() handler. The OnInitDialog() handler will be called each time the user
        // invokes the "Load Consistent Changes..." item from the Tools menu, even when "Use Consistent
        // Changes" is active. In any case, the user may or may not have already selected a .cct table
        // in the m_nCurPage, or in a different Table tab page. If a .cct table item was previously
        // selected/loaded, the list box item needs to be suffixed with " - [USED...]" to indicate that
        // it was selected previously. If the user selected a given .cct file in a different Table tab,
        // we want to mark it as " - [USED...]" in the current tab's listbox, but we don't want to
        // highlight/select it in the current list because it wasn't selected in the current Table tab.
        // However, if the user previously selected the given .cct file from this current tab page
        // (which might be the case if we're returning to this tab after looking at other tabs), we want
        // to both mark it as "[USED]", and also automatically highlight it because it IS the .cct item
        // the user previously selected on the current page.
        //
        // Note: At this point in LoadDataForPage, the listbox only contains .cct files - none of which
        // would have any " - [USED...]" suffixes. Later such suffixes will be added to any appropriate
		// listbox items when ShowUsageOfListBoxItems() is called. All list items are also only found
		// on the current path as stored in m_folderPath[m_nCurPage]. A different tab page may have already
		// been loaded with a .cct from a different path, but they won't be listed in the current Table
		// tab's listbox.
		if (!m_tblName[m_nCurPage].IsEmpty())
		{
			bool bFileFound = FALSE;
			int nReturned = m_pListBox->FindString(m_tblName[m_nCurPage]);
			if (nReturned != -1)
			{
				// path and file names match
				bFileFound = TRUE;
			}

			if (bFileFound)
			{
				// we found m_tblName in m_pListBox, so select it
				m_pListBox->SetSelection(nReturned);
				m_nPrevSelection = nReturned;
			}
			else
			{
				// We didn't find m_tblName in m_pListBox on the current m_folderPath[m_nCurPage] path, so select
				// instead the first unallocated item in the list.
				int aCt;
				for (aCt = 0; aCt < (int)m_pListBox->GetCount(); aCt++)
				{
					wxString listBoxStr;
			 		listBoxStr = m_pListBox->GetString(aCt);
					int nTableNum = -1;
					if (!IsSelectedInAnyTable(listBoxStr,nTableNum))
					{
						// This item has not been allocated in a table so we can select it as the first
						// unallocated item in the list.
						m_pListBox->SetSelection(aCt);
						m_nPrevSelection = aCt;
						break; // we must stop looking since we want only the "first" unallocated item
					}
				}
			}
		}
	}
	if (!m_tblName[m_nCurPage].IsEmpty())
	{
		m_pEditSelectedTableName->SetValue(m_tblName[m_nCurPage]);
	}
	m_pEditFolderPath->SetValue(m_folderPath[m_nCurPage]);
}

void CCCTabbedDialog::UpdateButtons()
{
    // The enabled state of the buttons could be set with an update UI idle handler, but we'll
    // enable/disable the buttons manually by calling UpdateButtons() from the appropriate handlers.
    //
    // Note: The "Browse to Find CC Table..." button and the "Create CC Table..." button stay enabled at
    // all times.
    //
    // The "Edit CC Table..." button should only be enabled when there is an active selection of a .cct
    // file in the listbox.
	if (m_pListBox->GetSelection() != -1)
	{
		m_pBtnEditCct->Enable(TRUE);
	}
	else
	{
		m_pBtnEditCct->Enable(FALSE);
	}

    // The "Select None..." button should be enabled only when a path + file name is displayed in the
    // ...chosen edit box above the buttons.
	if (m_pEditSelectedTableName->GetValue() != _T(""))
	{
		m_pBtnSelectNone->Enable(TRUE);
	}
	else
	{
		m_pBtnSelectNone->Enable(FALSE);
	}
}

wxString CCCTabbedDialog::GetListItemWithoutUsedString(wxString inStr)
{
	if (inStr.Find(_T(" - [")) != -1)
	{
		inStr = inStr.Mid(0,inStr.Find(_T(" - [")));
	}
	return inStr;
}

void CCCTabbedDialog::OnSelchangeListCctables(wxCommandEvent& WXUNUSED(event))
{
    // wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a
    // call to Clear() so we must check to see if the listbox contains no items and if so
    // return immediately.
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBox))
	{
		return;
	}

	int nSel;
	nSel = m_pListBox->GetSelection();

	if (nSel != -1) //== m_nPrevSelection)
	{
		wxString listBoxStr = m_pListBox->GetString(nSel);
		int nTableNum = -1;
		if (IsSelectedInCurrentTable(listBoxStr))
		{
			// no change in current selection was made, i.e., either there is no selection or the user
			// just clicked on the current selection.
			return;
		}
		else if (IsSelectedInAnyTable(listBoxStr,nTableNum) && nTableNum != m_nCurPage)
		{
			// this cc table was already selected in another table
			listBoxStr = GetListItemWithoutUsedString(listBoxStr);
			wxString msg;
			msg = msg.Format(_("The %s changes table was already assigned in Table tab %d"),listBoxStr.c_str(),nTableNum+1);
			wxMessageBox(msg,_T("Consistent Changes"),wxICON_WARNING);
			m_pListBox->SetSelection(m_nPrevSelection);
		}
	}
	if (nSel == -1)
	{
		::wxBell();
		m_tblName[m_nCurPage] = _T("");
		m_bTableLoaded[m_nCurPage] = FALSE;
		m_pEditSelectedTableName->SetValue(m_tblName[m_nCurPage]);
		m_nPrevSelection = -1;
		return;
	}
	m_nPrevSelection = nSel;
	wxString tempStr = m_pListBox->GetString(nSel);
	tempStr = GetListItemWithoutUsedString(tempStr);
	m_tblName[m_nCurPage] = tempStr;
	m_bTableLoaded[m_nCurPage] = TRUE; // set our local copy (global is only set in OnOK()
	m_pEditSelectedTableName->SetValue(m_tblName[m_nCurPage]);
	m_pEditFolderPath->SetValue(m_folderPath[m_nCurPage]);
	ShowUsageOfListBoxItems();
	UpdateButtons();
	m_pListBox->SetFocus();
}

void CCCTabbedDialog::OnTabPageChanging(wxNotebookEvent& event)
{
	// OnTabPageChanging is called whenever any Table tab is selected ("Table 1", "Table 2",
	// "Table 3", or "Table 4").
	int pageNumSelected = event.GetSelection();
	if (pageNumSelected == m_nCurPage)
	{
		// user selected same page, so just return
		return;
	}
	// if we get to here user selected a different page
	m_nCurPage = pageNumSelected;

	// When a Table tab changes there is no previous selection for the new tab page, so set
	// m_nPrevSelection to -1
	m_nPrevSelection = -1;

	// Each Table tab is capable of having a different Folder Path, so before loading the new tab
	// page's data and displaying it, we need to

	//Set up new page data by populating list boxes and controls
	LoadDataForPage(pageNumSelected); // start with first item selected for new tab page - calls UpdateButtons
	ShowUsageOfListBoxItems();
	UpdateButtons();
	m_pListBox->SetFocus();
}

void CCCTabbedDialog::ShowUsageOfListBoxItems()
{
    // For each <file>.cct item in the current list box, that has already been associated with a Table
    // tab, indicate that association by suffixing the appropriate usedInTableMarker[] literal strings
    // to the right end of the list item. For example, if test1.cct is associated with Table 3, we want
    // to suffix test1.cct with usedInTableMarker[3] so that it shows "test1.cct - [USED in Table 3]" in
	// the list box. This function does not change the m_tblName[], m_folderPath[], mtblPathPlusName[],
	// or m_bTableLoaded[] arrays; it only uses the information from the m_tblName[]array to add or
	// remove " - [USED ...]" suffixes from the list box items of m_pListBox.
	wxString prevTblName;
	prevTblName = m_tblName[m_nCurPage];
	int listBoxItemIndex;
	for (listBoxItemIndex = 0; listBoxItemIndex < (int)m_pListBox->GetCount(); listBoxItemIndex++)
	{
		wxString listBoxStr;
		listBoxStr = m_pListBox->GetString(listBoxItemIndex);
		listBoxStr = GetListItemWithoutUsedString(listBoxStr);
		int nTableNum = -1; // a suitable default indication
		if (IsSelectedInAnyTable(listBoxStr,nTableNum))
		{
			// Add the " - [USED ...]" suffix to the existing string item and set the suffixed
			// string back into the same list item.
			listBoxStr += usedInTableMarker[nTableNum];
			m_pListBox->SetString(listBoxItemIndex,listBoxStr);
		}
		else
		{
			// just set the string (minus any suffix it may have had originally) back into the same
			// list item.
			m_pListBox->SetString(listBoxItemIndex,listBoxStr);
		}
	}
}

bool CCCTabbedDialog::IsSelectedInAnyTable(wxString cctFile, int & nTableNum)
{
	// nTableNum is unchanged as reference parameter unless actual table number is found, in which case
	// nTableNum is the Table tab number where the table is found.
	bool bSelected = FALSE;
	int ct;
	cctFile = GetListItemWithoutUsedString(cctFile);
	for (ct = 0; ct < 4; ct++)
	{
        // The incoming cctFile name will always be associated with the path stored in
        // m_folderPath[m_nCurPage]. Only if m_folderPath[ct] is the same path do we actually have a
        // match.
		if (cctFile == m_tblName[ct] && m_folderPath[m_nCurPage] == m_folderPath[ct])
		{
			bSelected = TRUE;
			nTableNum = ct;
			break;
		}
	}
	return bSelected;
}

bool CCCTabbedDialog::IsSelectedInCurrentTable(wxString cctFile)
{
	// nTableNum is unchanged as reference parameter unless actual table number is found
	bool bSelected = FALSE;
	int ct;
	for (ct = 0; ct < 4; ct++)
	{
		if (cctFile == m_tblName[ct] && ct == m_nCurPage)
		{
			bSelected = TRUE;
			break;
		}
	}
	return bSelected;
}

void CCCTabbedDialog::OnButtonBrowse(wxCommandEvent& WXUNUSED(event))
{
	// whm 11Jan12 modified. If this CCCTabbedDialog is summoned without
	// a project being open (as can happen if Cancel is pressed at the
	// wizard/collab dialog), browsing will be directed depending on any
	// whether a project is open and whether nav protection in effect
	// for the App's m_ccTableInputsAndOutputsFolderPath. If nav protection
	// is ON for _CCTABLE_INPUTS_OUTPUTS, that folder is the one that any
	// contained cc tables are shown for selection. Otherwise we look for
	// cc table files located in the App's m_lastCcTablePath if it has a
	// valid/existing path, or in m_curProjectPath when a project is open,
	// or when no project is open we look in either the m_workFolderPath
	// or m_customWorkFolderPath as appropriate.
	CAdapt_ItApp* pApp = &wxGetApp();

	// whm revised 6Aug11 in support of protecting inputs/outputs folder navigation
	bool bBypassFileDialog_ProtectedNavigation = FALSE;
	wxString defaultDir;
	if (gpApp->m_bProtectCCTableInputsAndOutputsFolder)
	{
		// Navigation protection is ON, so set the flag to bypass the wxFileDialog
		// and force the use of the special protected folder for the export.
		bBypassFileDialog_ProtectedNavigation = TRUE;
		defaultDir = gpApp->m_ccTableInputsAndOutputsFolderPath;
	}
	else if (gpApp->m_lastCcTablePath.IsEmpty()
		|| (!gpApp->m_lastCcTablePath.IsEmpty() && !::wxDirExists(gpApp->m_lastCcTablePath)))
	{
		// Navigation protection is OFF so we set the flag to allow the wxFileDialog
		// to appear. But the m_lastCcTablePath is either empty or, if not empty,
		// it points to an invalid path, so we initialize the defaultDir to point to
		// the special protected folder (_CCTABLE_INPUTS_OUTPUTS), even though
		// Navigation protection is not ON. In this case, the user could point the
		// export path elsewhere using the wxFileDialog that will appear.
		bBypassFileDialog_ProtectedNavigation = FALSE;
		defaultDir = gpApp->m_ccTableInputsAndOutputsFolderPath;
	}
	else
	{
		// Navigation protection is OFF and we have a valid path in m_lastCcTablePath,
		// so we initialize the defaultDir to point to the m_lastCcTablePath for the
		// location of the export. The user could still point the export path elsewhere
		// in the wxFileDialog that will appear.
		bBypassFileDialog_ProtectedNavigation = FALSE;
		defaultDir = gpApp->m_lastCcTablePath;
	}

	// whm modified 7Jul11 to bypass the wxFileDialog when the export is protected from
	// navigation.
	wxString ccTableFilename = _("ccTable");
	wxString tablePath;
	if (!bBypassFileDialog_ProtectedNavigation)
	{
		wxString filter;
		filter = _("Consistent Changes Tables (*.cct)|*.cct||");
		wxFileDialog fileDlg(
			(wxWindow*)wxGetApp().GetMainFrame(), // MainFrame is parent window for file dialog
			_("Locate The Consistent Change Table"),
			defaultDir,
			ccTableFilename,
			filter,
			wxFD_OPEN);	// a "File Open" dialog
						// GDLC wxOPEN deprecated in 2.8
		fileDlg.Centre();

		// open as modal dialog
		int returnValue = fileDlg.ShowModal();
		if (returnValue == wxID_CANCEL)
		{
			return; // user cancelled
		}
		else // must be wxID_OK
		{
			// whm revision 11Jan12 Note: We set the App's m_lastCcTablePath variable
			// with the path part of the selected path. We do this even when navigation
			// protection is on (see below), so that the special folders would be the
			// initial path suggested if the administrator were to switch Navigation
			// Protection OFF.
			wxString path, fname, ext;
			// whm Note: wxFileDialog::GetPath() returns the full path (directory and filename)
			wxFileName::SplitPath(fileDlg.GetPath(), &path, &fname, &ext);
			m_tblName[m_nCurPage] = fileDlg.GetFilename(); // this has just the file name
			m_folderPath[m_nCurPage] = path; // has just the path part

			// update the app's m_lastCcTablePath variable
			pApp->m_lastCcTablePath = m_folderPath[m_nCurPage];

			LoadDataForPage(m_nCurPage);
			ShowUsageOfListBoxItems();
			UpdateButtons();
			m_pListBox->SetFocus();
		}
	}
	else
	{
		// CC table folder (_CCTABLE_INPUTS_OUTPUTS) is protected
		wxArrayString ccTableFilesIncludingPaths,ccTableFilesNamesOnly;
		// get an array list of .cct files
		tablePath = pApp->m_ccTableInputsAndOutputsFolderPath;
		wxDir::GetAllFiles(tablePath,&ccTableFilesIncludingPaths,_T("*.cct"),wxDIR_FILES);
		int totFiles = (int)ccTableFilesIncludingPaths.GetCount();
		if (totFiles > 0)
		{
			int ct;
			for (ct = 0; ct < totFiles; ct++)
			{
				wxFileName fn(ccTableFilesIncludingPaths.Item(ct));
				wxString fNameOnly = fn.GetFullName();
				ccTableFilesNamesOnly.Add(fNameOnly);
			}
		}
		wxString message = _("Choose a Consistent Changes table from the following list:\n(from the location: %s):");
		message = message.Format(message,tablePath.c_str());
		wxString myCaption = _T("");
		int returnValue = wxGetSingleChoiceIndex(message,myCaption,
			ccTableFilesNamesOnly,(wxWindow*)pApp->GetMainFrame(),-1,-1,true,250,100);
		if (returnValue == -1)
			return; // user pressed Cancel or OK with nothing selected (list empty)

		m_tblName[m_nCurPage] = ccTableFilesNamesOnly.Item(returnValue); // this has just the file name
		m_folderPath[m_nCurPage] = tablePath;

		// update the app's m_lastCcTablePath variable
		pApp->m_lastCcTablePath = m_folderPath[m_nCurPage];

		LoadDataForPage(m_nCurPage);
		ShowUsageOfListBoxItems();
		UpdateButtons();
		m_pListBox->SetFocus();
	}
}


void CCCTabbedDialog::OnDblclkListCctables(wxCommandEvent& WXUNUSED(event))
{
	// whm note: A double click also activates the OnSelchangeListCctables() event handler so I've
	// commented out the code from this handler
	/*
	int nSel;
	nSel = m_pListBox->GetSelection();
	if (nSel == -1)
	{
		::wxBell();
		m_tblName[m_nCurPage] = _T("");
		m_pEditSelectedTableName->SetValue(m_tblName[m_nCurPage]); //TransferDataToWindow();
		return;
	}
	m_tblName[m_nCurPage] = m_pListBox->GetString(nSel);
	m_bTableLoaded[m_nCurPage] = TRUE;
	m_pEditSelectedTableName->SetValue(m_tblName[m_nCurPage]);
	m_pEditFolderPath->SetValue(m_folderPath[m_nCurPage]); //TransferDataToWindow();
	OnSelchangeListCctables(event);
	*/
}

void CCCTabbedDialog::GetPossibleCCTables(wxArrayString* pList)
{
	wxString dirPath = m_folderPath[m_nCurPage];
	wxDir finder;

	bool bOK = (finder.Open(dirPath)); // wxDir must call .Open() before enumerating files!
	if (!bOK)
	{
		// we might be working on a different machine, so the path on the current machine cannot
		// be assumed to be valid; so allow the user to try again
		wxMessageBox(_("Sorry, the default consistent changes table path is not valid. Adapt It has substituted the path to the project folder. Use the Browse button to locate the changes table files(s)."),_T(""), wxICON_INFORMATION);
		return;
	}
	else
	{
		// Must call wxDir::Open() before calling GetFirst() - see above
		wxString str = _T("");
		bool bWorking = finder.GetFirst(&str,wxEmptyString,wxDIR_FILES);
		// whm note: wxDIR_FILES finds only files; it ignores directories, and . and ..
		// second parameter wxEmptyString iterates all files, of any or no extension
		while (bWorking)
		{
			wxFileName fn(str);
			wxString e = fn.GetExt(); // GetExt() returns the extension NOT including the dot
			e = e.MakeUpper();
			if (e.CmpNoCase(_T("cct")) == 0)
				pList->Add(str); // it's a cc table file, so put in listbox
			bWorking = finder.GetNext(&str);
		}
	}
}

void CCCTabbedDialog::OnButtonEditCct(wxCommandEvent& WXUNUSED(event))
{
	CCCTableEditDlg editor((wxWindow*)gpApp->GetMainFrame());

	if (m_tblName[m_nCurPage].IsEmpty())
	{
		wxMessageBox(_("Sorry, you have not yet selected a consistent changes table from the list box. Do so and then try the editing button again."),_T(""), wxICON_INFORMATION);
		return;
	}

	// make the file's path
	wxString path = m_folderPath[m_nCurPage] + gpApp->PathSeparator + m_tblName[m_nCurPage];
	wxASSERT(!path.IsEmpty());

	DoEditor(editor,path);
	m_pListBox->SetFocus();
}

void CCCTabbedDialog::OnButtonCreateCct(wxCommandEvent& WXUNUSED(event))
{
	// whm 14Jul11 modified. The creation of a cc table is somewhat of a
	// technical task and not likely to be done by novice users. Moreover
	// the tabbed dialog does not utilize a wxFileDialog, but makes use of
	// the CCCTableNameDlg instead for getting the name of a cc table file
	// to create. Hence whenever nav protection is in force for cc table
	// storage, this routine utilizes the _CCTABLE_INPUTS_OUTPUTS which is
	// a child folder of the m_workFolderPath or m_customWorkFolderPath as
	// appropriate. The folder is located there because the CCCTabbedDialog
	// can be summoned without a project being open (as can happen of Cancel
	// is pressed at the wizard/collab dialog).
	CCCTableNameDlg nameDlg((wxWindow*)gpApp->GetMainFrame());
	if (nameDlg.ShowModal() == wxID_OK)
	{
		// get the user-supplied table name and make it a filename
		m_tblName[m_nCurPage] = nameDlg.m_tableName;
		m_tblName[m_nCurPage] += _T(".cct"); // append .cct extension

		bool bBypassFileDialog_ProtectedNavigation = FALSE;
		bBypassFileDialog_ProtectedNavigation = bBypassFileDialog_ProtectedNavigation; // avoid compiler warning
		// Determine the value for m_folderPath[m_nCurPage]
		if (gpApp->m_bProtectCCTableInputsAndOutputsFolder)
		{
			// Navigation protection is ON, so set the flag to bypass the wxFileDialog
			// and force the use of the special protected folder for the export.
			bBypassFileDialog_ProtectedNavigation = TRUE;
			m_folderPath[m_nCurPage] = gpApp->m_ccTableInputsAndOutputsFolderPath;
		}
		else if (gpApp->m_lastCcTablePath.IsEmpty()
			|| (!gpApp->m_lastCcTablePath.IsEmpty() && !::wxDirExists(gpApp->m_lastCcTablePath)))
		{
			// Navigation protection is OFF so we set the flag to allow the wxFileDialog
			// to appear. But the m_lastKbOutputPath is either empty or, if not empty,
			// it points to an invalid path, so we initialize the defaultDir to point to
			// the special protected folder, even though Navigation protection is not ON.
			// In this case, the user could point the export path elsewhere using the
			// wxFileDialog that will appear.
			bBypassFileDialog_ProtectedNavigation = FALSE;
			m_folderPath[m_nCurPage] = gpApp->m_ccTableInputsAndOutputsFolderPath;
		}
		else
		{
			// Navigation protection is OFF and we have a valid path in m_lastKbOutputPath,
			// so we initialize the defaultDir to point to the m_lastKbOutputPath for the
			// location of the export. The user could still point the export path elsewhere
			// in the wxFileDialog that will appear.
			bBypassFileDialog_ProtectedNavigation = FALSE;
			m_folderPath[m_nCurPage] = gpApp->m_lastCcTablePath;
		}

		// go with the full path specification
		gpApp->m_lastCcTablePath = m_folderPath[m_nCurPage]; // set the default Table path to the same place too
		wxString path = m_folderPath[m_nCurPage] + gpApp->PathSeparator + m_tblName[m_nCurPage]; // this defines the file we wish to create

		wxFile f;
		if (::wxFileExists(path))
		{
			if (!f.Open(path,wxFile::read_write))
			{
				wxMessageBox(_("Error opening the file. Cannot edit the file. Aborting."), _T(""), wxICON_EXCLAMATION);
				return;
			}
		}
		else
		{
			// file doesn't yet exist, so create it
			if (!f.Open(path,wxFile::write))
			{
				wxMessageBox(_("Error creating the file. Aborting."), _T(""), wxICON_EXCLAMATION);
				return;
			}
		}
		// if we get here, f is open
		// close the file - the DoEditor() function will reopen it for editing
		f.Close();

		CCCTableEditDlg editor((wxWindow*)gpApp->GetMainFrame());
		DoEditor(editor,path); // add content in the editor

		LoadDataForPage(m_nCurPage);
		ShowUsageOfListBoxItems();
		UpdateButtons();
		m_pListBox->SetFocus();

		/*
		// on dismissal of the editor, make the list box show the tables available, and the new one hilited
		// whm: but do we automatically use the edited table? probably not, just highlight it.

		// remove any existing filenames in the list
		m_pListBox->Clear();

		// fill the list box with any *.cct filenames in the folder
		wxArrayString possibleTables;
		GetPossibleCCTables(&possibleTables);

		// fill the list box with the c table filename strings
		int aCt;
		for (aCt = 0; aCt < (int)possibleTables.GetCount(); aCt++)
		{
			wxString str = possibleTables.Item(aCt);
			m_pListBox->Append(str);
		}

		// hilight the current one if possible, but if none is current then highlight
		// the first in the list
		if (m_tblName[m_nCurPage].IsEmpty())
		{
			if (m_pListBox->GetCount() > 0)
			{
				m_pListBox->SetSelection(0);
				m_nPrevSelection = 0;
			}
		}
		else
		{
			int nReturned = m_pListBox->FindString(m_tblName[m_nCurPage]);
			if (nReturned != -1)
			{
				// we found m_tblName in m_pListBox, so select it
				m_pListBox->SetSelection(nReturned);
				m_nPrevSelection = nReturned;
			}
			else
			{
				// we didn't find m_tblName in m_pListBox, so select the first item provided GetCount() > 0
				if (m_pListBox->GetCount() > 0)
				{
					m_pListBox->SetSelection(0);
					m_nPrevSelection = 0;
				}
			}
		}

		if (m_pListBox->GetSelection() != -1)
		{
			OnSelchangeListCctables(event);
		}
		//show the new page data
		ShowUsageOfListBoxItems();
		UpdateButtons();
		m_pListBox->SetFocus();
		*/
	}
	else
		return; // user cancelled
}

void CCCTabbedDialog::DoEditor(CCCTableEditDlg& editor,wxString& path)
{
	// check that the file is not too large for a CEdit (max 30,000 chars)
	wxFile f;

	if (::wxFileExists(path))
	{
		if (!f.Open(path,wxFile::read_write))
		{
			wxMessageBox(_("Error opening the file. Cannot edit the file. Aborting."), _T(""), wxICON_EXCLAMATION);
			return;
		}
	}
	else
	{
		// file doesn't yet exist, so create it
		if (!f.Open(path,wxFile::write))
		{
			wxMessageBox(_("Error creating the file. Aborting."), _T(""), wxICON_EXCLAMATION);
			return;
		}
	}

	// file is now open, so find its logical length
	wxUint32 nLogLen = f.Length(); // finds length in bytes

	//this is where unicode support is required; we'll allow the table to be encoded in UTF-16, not just the
	// expected UTF-8, to give a bit more flexibility
#ifndef _UNICODE // ANSI version, no unicode support

	// create the required buffer and then read in the file (no conversions needed)
	// whm note: I'm borrowing the routines from GetNewFile() in the Doc which use malloc
	// rather than alloca; it won't hurt to allocate memory from the heap rather than the stack frame
	wxChar* pBuf = (wxChar*)malloc(nLogLen + 1); // allow for terminating null byte
	memset(pBuf,0,nLogLen + 1); // set all to zero
	wxUint32 numRead = f.Read(pBuf,(wxUint32)nLogLen);
	pBuf[numRead] = '\0'; // add terminating null
	nLogLen += 1; // allow for terminating null (sets m_nInputFileLength in the caller)
	editor.m_ccTable = pBuf; // copy to the caller's wxString (on the heap) before malloc
							// buffer is destroyed
	free((void*)pBuf);
#else
	wxUint32 nNumRead;
//	bool bHasBOM = FALSE;
	wxUint32 nBuffLen = (wxUint32)nLogLen + sizeof(wxChar);
	char* pbyteBuff = (char*)malloc(nBuffLen);
	memset(pbyteBuff,0,nBuffLen);
	nNumRead = f.Read(pbyteBuff,nLogLen);
	nLogLen = nNumRead + sizeof(wxChar);

	// now we have to find out what kind of encoding the data is in, and set the encoding
	// and we convert to UTF-16 in the DoInputConversion() function
	// check for UTF-16 first; we allow it, but don't expect it (and we assume it would have a BOM)
//	if (!memcmp(pbyteBuff,szU16BOM,nU16BOMLen))
//	{
//		// it's UTF-16
//		gpApp->m_srcEncoding = wxFONTENCODING_UTF16; //eUTF16;
//		bHasBOM = TRUE;
//	}
//	else
//	{
//		// see if it is UTF-8, whether with or without a BOM; if so,
//		if (!memcmp(pbyteBuff,szBOM,nBOMLen))
//		{
//			// the UTF-8 BOM is present
//			gpApp->m_srcEncoding = wxFONTENCODING_UTF8; //eUTF8;
//			bHasBOM = TRUE;
//		}
//		else
//		{
//			// whm: I think it is safest to just try using the default system encoding
//			// BEW 26July10, the default is not the system encoding as assumed.
//			// wxFONTENCODING_DEFAULT has a value of 0, and if passed to wxCSConv()
//			// conversion function (which happens within DoInputConversion() below), then
//			// wxWidgets asserts. So either use wxFONTENCODING_SYSTEM explicitly, or
//			// wxFONTENCODING_UTF8 -- and in the Unicode build, the latter is always best
//			//gpApp->m_srcEncoding = wxFONTENCODING_DEFAULT;
//			gpApp->m_srcEncoding = wxFONTENCODING_UTF8;
//		}
//	}

	// do the converting and transfer the converted data to editor's wxString member for the table,
	// (which then persists while table editor lives)
	// GDLC 16Sep11 Last parameter no longer needed
	// gpApp->DoInputConversion(editor.m_ccTable,pbyteBuff,gpApp->m_srcEncoding,bHasBOM);
	// GDLC 7Dec11 Changed for revised return parameters of DoInputConversion
//	wxString* pTemp;
//	gpApp->DoInputConversion(pTemp, pbyteBuff, gpApp->m_srcEncoding);
//	editor.m_ccTable = *pTemp;
	wxChar* pTemp;		// DoInputConversion() creates the wxChar buffer
	wxUint32 lenTemp;
	//	GDLC 13Jan12 Use UTF8 as the fallback default font encoding without changing the app's
	// record of the source text encoding; also provide DoInputConversion() with the correct
	// bte buffer length
	gpApp->DoInputConversion(pTemp, lenTemp, pbyteBuff, wxFONTENCODING_UTF8, nLogLen);
	//	gpApp->DoInputConversion(pTemp, lenTemp, pbyteBuff, gpApp->m_srcEncoding);
	editor.m_ccTable = *(new wxString(pTemp, lenTemp));
//	GDLC 7Dec11 Free the temporary wxChar buffer created by DoInputConversion()
	free((void*)pTemp);

	// free the original read in (const) char data's chunk
	free((void*)pbyteBuff);

#endif // for _UNICODE

	f.Close();

	if (editor.ShowModal() == wxID_OK)
	{
		if (!f.Open(path, wxFile::write))
		{
				wxString message;
				message = message.Format(_("Error opening cc table file for writing with path %s."),path.c_str());
				message += _T("  The table file was not updated.");
				wxMessageBox(message, _T(""), wxICON_EXCLAMATION);
				return;
		}

#ifdef _UNICODE // unicode version

		// Conversion back to UTF-8 because we always write cc tables out as UTF-8
		wxCharBuffer tempBuf = editor.m_ccTable.mb_str(wxConvUTF8);
		size_t nLen = strlen(tempBuf);


		// allow for a terminating null byte, since it's a byte string
		// whm note: No, doing so embeds a null byte in the text file, so
		// I've commented out the next line in the wx version
		//nLen += 1;

		// write it out
		f.Write(tempBuf,nLen);

#else // ANSI version

		wxUint32 len = editor.m_ccTable.Length() + 1;
		wxASSERT(len > 0);
		const char* p = editor.m_ccTable;
		f.Write((const void*)p,len); // .Write() is byte oriented

#endif // for !_UNICODE

		f.Close();
	}
}

void CCCTabbedDialog::OnButtonSelectNone(wxCommandEvent& WXUNUSED(event))
{
	m_pListBox->SetSelection(-1); // remove selections (returns LB_ERR too) same in wx
	m_nPrevSelection = -1;
	m_tblName[m_nCurPage] = _T("");
	m_bTableLoaded[m_nCurPage] = FALSE; // set our local copy (global is only set in OnOK()
	m_pEditSelectedTableName->SetValue(m_tblName[m_nCurPage]); //TransferDataToWindow();
	ShowUsageOfListBoxItems();
	UpdateButtons();
	m_pListBox->SetFocus();
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CCCTabbedDialog::OnOK(wxCommandEvent& event) // unused in CKBEditor
{
	// copy local values to the globals on the App
	int ct;
	for (ct = 0; ct < 4; ct++)
	{
		gpApp->m_tableName[ct] = m_tblName[ct];
		gpApp->m_tableFolderPath[ct] = m_folderPath[ct];
		gpApp->m_bCCTableLoaded[ct] = m_bTableLoaded[ct];
	}
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

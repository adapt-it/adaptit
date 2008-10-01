/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			CCTabbedDialog.h
/// \author			Bill Martin
/// \date_created	19 June 2007
/// \date_revised	14 April 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CCCTabbedDialog class. 
/// The CCCTabbedDialog class provides a dialog with tabbed pages in which the user can load up to four
/// consistent changes tables for use in Adapt It. Each tabbed page has controls that enable the user to 
/// browse, select, create and/or edit consistent change (.cct) tables.
/// \derivation		The CCCTabbedDialog class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef CCTabbedDialog_h
#define CCTabbedDialog_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "CCTabbedDialog.h"
#endif

// forward declarations
class CCCTableEditDlg;

/// The CCCTabbedDialog class provides a dialog with tabbed pages in which the user can load up to four
/// consistent changes tables for use in pre-processing the Adapt It source text before it is copied to
/// the phrase box. The contents of the tab pages come from instances of the CCCTablePageFunc, a resource
/// (created by wxDesigner) which has controls that enable the user to browse, select, create and/or edit 
/// consistent change (.cct) tables.
/// \derivation		The CCCTabbedDialog class is derived from wxPropertySheetDialog.
class CCCTabbedDialog : public AIModalDialog
{
public:
	CCCTabbedDialog(wxWindow* parent); // constructor
	virtual ~CCCTabbedDialog(void); // destructor
	
	wxNotebook* m_pCCTabbedNotebook;
	wxListBox* m_pListBox;
	wxTextCtrl* m_pEditSelectedTableName;
	wxTextCtrl* m_pEditFolderPath;
	wxButton* m_pBtnBrowse;
	wxButton* m_pBtnCreateCct;
	wxButton* m_pBtnEditCct;
	wxButton* m_pBtnSelectNone;
	
	// local copies of globals on the App
	wxString m_tblName[4];
	wxString m_folderPath[4];
	bool m_bTableLoaded[4];
	
	wxString usedInTableMarker[4];
	
	int m_nCurPage;
	int m_nPrevSelection;
#ifdef __WXGTK__
	bool			m_bListBoxBeingCleared;
#endif
	
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void LoadDataForPage(int pageNumSel);
	void UpdateButtons();
	void ShowUsageOfListBoxItems();
	bool IsSelectedInAnyTable(wxString cctFile, int & nTableNum);
	bool IsSelectedInCurrentTable(wxString cctFile);
	void OnTabPageChanging(wxNotebookEvent& event);
	wxString GetListItemWithoutUsedString(wxString inStr);

protected:
	void DoEditor(CCCTableEditDlg& editor,wxString& path);
	void GetPossibleCCTables(wxArrayString* pList);
	void OnButtonBrowse(wxCommandEvent& WXUNUSED(event));
	void OnDblclkListCctables(wxCommandEvent& WXUNUSED(event));
	void OnButtonEditCct(wxCommandEvent& WXUNUSED(event));
	void OnSelchangeListCctables(wxCommandEvent& WXUNUSED(event));
	void OnButtonSelectNone(wxCommandEvent& WXUNUSED(event));
	void OnButtonCreateCct(wxCommandEvent& WXUNUSED(event));

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

#endif /* CCTabbedDialog_h */

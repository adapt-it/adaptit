/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBEditor.h
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \date_revised	4 February 2009
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CKBEditor class. 
/// The CKBEditor class provides a tabbed dialog (one tab for each of one to
/// ten word source phrases stored in the knowledge base). It allows the user
/// to examine, edit, add, or delete the translations that have been stored for
/// a given source phrase. The user can also move up a given translation in the
/// list to be shown automatically in the phrasebox by the ChooseTranslationDlg.
/// A user can also toggle the "Force Choice..." flag or add <no adaptation> for
/// a given source phrase using this dialog.
/// \derivation		The CKBEditor class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef KBEditor_h
#define KBEditor_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KBEditor.h"
#endif

/// The CKBEditor class provides a tabbed dialog (one tab for each of one to
/// ten word source phrases stored in the knowledge base). It allows the user
/// to examine, edit, add, or delete the translations that have been stored for
/// a given source phrase. The user can also move up a given translation in the
/// list to be shown automatically in the phrasebox by the ChooseTranslationDlg.
/// A user can also toggle the "Force Choice..." flag or add <no adaptation> for
/// a given source phrase using this dialog.
/// \derivation		The CKBEditor class is derived from AIModalDialog.
class MapKeyStringToTgtUnit;

class CKBEditor : public AIModalDialog
{
	// friend functions (definitions are in helpers.cpp)
	friend void PopulateTranslationsListBox(CKB* pKB, CTargetUnit* pTgtUnit, 
											CKBEditor* pKBEditor);

public:
	CKBEditor(wxWindow* parent); // constructor
	virtual ~CKBEditor(void); // destructor
	// other methods
	
	//enum { IDD = IDD_EDITKB_PAGE };
	wxTextCtrl*		m_pFlagBox;
	wxTextCtrl*		m_pTypeSourceBox;
	wxTextCtrl*		m_pEditOrAddTranslationBox;
	wxTextCtrl*		m_pEditRefCount;
	wxTextCtrl*		m_pEditSearches;
	wxComboBox*		m_pComboOldSearches; // BEW added 22Jan10
	wxStaticText*	m_pStaticCount;
	wxStaticText*	m_pStaticWhichKB; // BEW added 13Nov10
	wxListBox*		m_pListBoxExistingTranslations;
	wxListBox*		m_pListBoxKeys;
	wxNotebook*		m_pKBEditorNotebook;
	wxStaticText*	m_pStaticSelectATab;
	wxButton*		m_pBtnUpdate;
	wxButton*		m_pBtnAdd;
	wxButton*		m_pBtnRemove;
	wxButton*		m_pBtnMoveUp;
	wxButton*		m_pBtnMoveDown;
	wxButton*		m_pBtnAddNoAdaptation;
	wxButton*		m_pBtnToggleTheSetting;
	// BEW added 22Jan10, next 3 buttons
	wxButton*		m_pBtnGo;
	wxButton*		m_pBtnEraseAllLines;
	
	wxString		m_edTransStr;
	wxString		m_srcKeyStr;
	int				m_refCount;
	wxString		m_refCountStr;
	wxString		m_flagSetting;
	wxString		m_entryCountStr;
	wxString		m_ON;
	wxString		m_OFF;
	int				m_nWordsSelected; // whm eliminated gnWordsInPhrase global; now only this m_nWordsSelected is needed
	wxString		m_TheSelectedKey; // if multiple keys selected, take only the first

	wxString		m_curKey;
	int				m_nCurPage; // whm added for wx version

	/// Indicates the count of entries in each of the 10 MapKeyStringToTgtUnit maps in the KB. Used for
	/// support of preallocation of storage prior to opening KB editor with list controls of many entries.
	int				m_nMapLength[10]; // whm eliminated gnMapLength[10] global by using an int array local to CKBEditor
	
	CAdapt_ItApp*	pApp;
	CKB*			pKB;
	CTargetUnit*	pCurTgtUnit;
	CRefString*		pCurRefString;
	MapKeyStringToTgtUnit* pMap;
	bool m_bRemindUserToDoAConsistencyCheck;

	void LoadDataForPage(int pageNumSel,int nStartingSelection);
	bool IsInListBox(wxListBox* listBox, wxString str);

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	virtual void OnCancel(wxCommandEvent& WXUNUSED(event));
	bool AddRefString(CTargetUnit* pTargetUnit, wxString& translationStr);
	void UpdateButtons();

	void OnAddNoAdaptation(wxCommandEvent& event);
	void OnTabSelChange(wxNotebookEvent& event);
	void OnSelchangeListSrcKeys(wxCommandEvent& WXUNUSED(event));
	void OnSelchangeListExistingTranslations(wxCommandEvent& WXUNUSED(event));
	void OnDblclkListExistingTranslations(wxCommandEvent& event);
	void OnUpdateEditSrcKey(wxCommandEvent& event);
	void OnUpdateEditOrAdd(wxCommandEvent& WXUNUSED(event));
	void OnButtonUpdate(wxCommandEvent& WXUNUSED(event));
	void OnButtonAdd(wxCommandEvent& event);
	void OnButtonRemove(wxCommandEvent& WXUNUSED(event));
	void OnButtonMoveUp(wxCommandEvent& WXUNUSED(event));
	void OnButtonMoveDown(wxCommandEvent& event);
	void OnButtonFlagToggle(wxCommandEvent& WXUNUSED(event));

	void OnButtonGo(wxCommandEvent& WXUNUSED(event));
	void OnButtonEraseAllLines(wxCommandEvent& WXUNUSED(event));
	void OnComboItemSelected(wxCommandEvent& event);

private:
	void DoRetain();
	void UpdateComboInEachPage();
	void DoRestoreSearchStrings();
	bool bKBEntryTemporarilyAddedForLookup;
	void MessageForConsistencyCheck();
	//void PopulateTranslationsListBox(CTargetUnit* pTgtUnit); // not needed
	//void EraseTranslationsListBox(); // not needed

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* KBEditor_h */

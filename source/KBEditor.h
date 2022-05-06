/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBEditor.h
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \rcs_id $Id$
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
	wxButton*		m_pBtnRemoveSomeTgtEntries;
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
	wxString		m_entryCountLabel; // BEW added 16Jan13, so we don't put a formatted string
									   // into m_entryCountStr thereby removing the %d format specifier
	wxString		m_ON;
	wxString		m_OFF;
	int				m_nWordsSelected; // whm eliminated gnWordsInPhrase global; now only this m_nWordsSelected is needed
	wxString		m_TheSelectedKey; // if multiple keys selected, take only the first

	wxString		m_currentKey; // whm 24Feb2018 renamed from m_curKey to m_currentKey to distinguish from m_CurKey (note: capital C) in CPhraseBox 
	int				m_nCurPage; // whm added for wx version
	// BEW 1Sep15 added next two so that the message shown at the Update button does not get shown more than 3 times per KB editor session
	int             m_messageCount;
	int             m_maxMessageCount;

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

    // whm 15Sep2018 added for use by caller View's OnToolsKbEditor to restore phrasebox content after PlaceBox() call there.
    wxString m_originalPhraseBoxContent; 
    bool m_bBoxContent_present_but_deleted;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	virtual void OnCancel(wxCommandEvent& WXUNUSED(event));
	bool AddRefString(CTargetUnit* pTargetUnit, wxString& translationStr);
	//void UpdateButtons();
	void OnAddNoAdaptation(wxCommandEvent& event);
	void OnUpdateAddNoAdaptation(wxUpdateUIEvent& event);
	void OnTabSelChange(wxNotebookEvent& event);
	void OnSelchangeListSrcKeys(wxCommandEvent& WXUNUSED(event));
	void OnSelchangeListExistingTranslations(wxCommandEvent& WXUNUSED(event));
	void OnDblclkListExistingTranslations(wxCommandEvent& event);
	void OnButtonSourceFindGo(wxCommandEvent& event);
	//void OnUpdateEditOrAdd(wxCommandEvent& WXUNUSED(event));
	void OnButtonUpdate(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonUpdate(wxUpdateUIEvent& event);
	void OnButtonAdd(wxCommandEvent& event);
	void OnUpdateButtonAdd(wxUpdateUIEvent& event);
	void OnButtonRemove(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonRemove(wxUpdateUIEvent& event);
	void OnButtonRemoveSomeTgtEntries(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonRemoveSomeTgtEntries(wxUpdateUIEvent& event);
	void OnButtonMoveUp(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonMoveUp(wxUpdateUIEvent& event);
	void OnButtonMoveDown(wxCommandEvent& event);
	void OnUpdateButtonMoveDown(wxUpdateUIEvent& event);
	void OnButtonFlagToggle(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonFlagToggle(wxUpdateUIEvent& event);

	void OnButtonGo(wxCommandEvent& WXUNUSED(event));
	void OnButtonEraseAllLines(wxCommandEvent& WXUNUSED(event));
	void OnComboItemSelected(wxCommandEvent& event);
	//void OnIdle(wxIdleEvent& WXUNUSED(event));

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

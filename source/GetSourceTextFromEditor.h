/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GetSourceTextFromEditorDlg.h
/// \author			Bill Martin
/// \date_created	10 April 2011
/// \date_revised	30 June 2011
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CGetSourceTextFromEditorDlg class. 
/// The CGetSourceTextFromEditorDlg class represents a dialog in which a user can obtain a source text
/// for adaptation from an external editor such as Paratext or Bibledit. 
/// \derivation		The CGetSourceTextFromEditorDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef GetSourceTextFromEditorDlg_h
#define GetSourceTextFromEditorDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "GetSourceTextFromEditorDlg.h"
#endif

class CGetSourceTextFromEditorDlg : public AIModalDialog
{
public:
	CGetSourceTextFromEditorDlg(wxWindow* parent); // constructor
	virtual ~CGetSourceTextFromEditorDlg(void); // destructor
	// other methods
	wxComboBox* pComboSourceProjectName;
	wxComboBox* pComboTargetProjectName;
	wxComboBox* pComboFreeTransProjectName;
	wxComboBox* pComboAiProjects;
	wxRadioBox* pRadioBoxChapterOrBook;
	wxListBox* pListBoxBookNames;
	wxListView* pListCtrlChapterNumberAndStatus; // wxListBox* pListCtrlChapterNumberAndStatus;
	wxListItem* pTheFirstColumn; // has to be on heap
	wxListItem* pTheSecondColumn; // has to be on heap
	wxTextCtrl* pStaticTextCtrlNote;
	wxStaticText* pStaticSelectAChapter;
	wxStaticText* pStaticGetSrcFromThisProj;
	wxStaticText* pStaticTransTgtToThisProj;
	wxStaticText* pStaticTransFtToThisProj;
	wxTextCtrl* pTextCtrlAsStaticNewAIProjName;
	wxTextCtrl* pTextCtrlSourceLanguageName;
	wxTextCtrl* pTextCtrlTargetLanguageName;
	wxStaticText* pStaticTextEnterLangNames;
	wxStaticText* pStaticTextNewAIProjName;
	wxStaticText* pStaticTextSrcLangName;
	wxStaticText* pStaticTextTgtLangName;
	wxStaticText* pStaticTextDoNotChangeNote;
	wxStaticText* pStaticTextUseDropDown;
	wxStaticText* pStaticTextSelectSuitableAIProj;
	wxStaticLine* pStaticLine2;
	wxButton* pBtnNoFreeTrans;
	wxButton* pBtnChangeProjects;
	wxButton* pBtnCancel;
	wxButton* pBtnOK;
	wxSizer* pGetSourceTextFromEditorSizer;
	wxSize m_computedDlgSize; // stores the computed size of the dialog's sizer - accounting for its current layout state

	// The following m_Temp... variables are used while the dialog is active until
	// the user clicks on OK. In the OnOK() handler the Apps persistent values are
	// assigned according to these m_Temp... variables. Hence, if the "Cancel" button
	// is clicked, no changes are made to the App's persistent values.
	wxString m_TempCollabProjectForSourceInputs;
	wxString m_TempCollabProjectForTargetExports;
	wxString m_TempCollabProjectForFreeTransExports;
	wxString m_TempCollabAIProjectName;
	wxString m_TempCollabSourceProjLangName;
	wxString m_TempCollabTargetProjLangName;
	wxString m_TempCollabBookSelected;
	bool m_bTempCollabByChapterOnly; // FALSE means the "whole book" option
	wxString m_TempCollabChapterSelected;
	wxString m_bareChapterSelected;
	bool m_bTempCollaborationExpectsFreeTrans; // whm added 6Jul11
	wxArrayString projList;

	wxString sourceWholeBookBuffer;
	wxString targetWholeBookBuffer;
	wxString freeTransWholeBookBuffer;
	wxString sourceChapterBuffer;
	wxString targetChapterBuffer;
	wxString freeTransChapterBuffer;
	wxArrayString SourceTextUsfmStructureAndExtentArray;
	wxArrayString TargetTextUsfmStructureAndExtentArray;
	wxArrayString FreeTransTextUsfmStructureAndExtentArray;
	wxArrayString SourceChapterUsfmStructureAndExtentArray;
	wxArrayString TargetChapterUsfmStructureAndExtentArray;
	wxArrayString FreeTransChapterUsfmStructureAndExtentArray;

	wxString m_rdwrtp7PathAndFileName;
	wxString m_bibledit_rdwrtPathAndFileName;
	
	wxString m_collabEditorName;

	wxArrayString m_staticBoxDescriptionArray;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnNoFreeTrans(wxCommandEvent& WXUNUSED(event));
	void OnBtnShowOrHideChangeProjects(wxCommandEvent& WXUNUSED(event));
	void OnComboBoxSelectSourceProject(wxCommandEvent& WXUNUSED(event));
	void OnComboBoxSelectTargetProject(wxCommandEvent& WXUNUSED(event));
	void OnComboBoxSelectFreeTransProject(wxCommandEvent& WXUNUSED(event));
	void OnLBBookSelected(wxCommandEvent& WXUNUSED(event));
	void OnLBChapterSelected(wxListEvent& WXUNUSED(event));
	void OnLBDblClickChapterSelected(wxCommandEvent& WXUNUSED(event));
	void OnRadioBoxSelected(wxCommandEvent& WXUNUSED(event));
	void OnComboBoxSelectAiProject(wxCommandEvent& WXUNUSED(event));
	void OnEnChangeSrcLangName(wxCommandEvent& WXUNUSED(event));
	void OnEnChangeTgtLangName(wxCommandEvent& WXUNUSED(event));

	bool CollabProjectsExistAsAIProject(wxString LanguageNameSrc, 
								wxString LanguageNameTgt, wxString& aiProjectFolderName,
								wxString& aiProjectFolderPath);
	bool EmptyVerseRangeIncludesAllVersesOfChapter(wxString emptyVersesStr);
	void RecordArrayDataForLastUsfm();
	void GetChapterListAndVerseStatusFromTargetBook(wxString targetBookFullName, 
								wxArrayString& chapterList, wxArrayString& statusList);
	wxString GetStatusOfChapter(const wxArrayString &TargetArray,int indexOfChItem,
								wxString targetBookFullName,wxString& nonDraftedVerses);
	wxString AbbreviateColonSeparatedVerses(const wxString str);
	void LoadBookNamesIntoList();
	void ExtractVerseNumbersFromBridgedVerse(wxString tempStr,int& nLowerNumber,
								int& nUpperNumber);
	wxString GetBareChFromLBChSelection(wxString lbChapterSelected);
	EthnologueCodePair* MatchAIProjectUsingEthnologueCodes(wxString& editorSrcLangCode,
								wxString& editorTgtLangCode);

private:
	bool m_bTextOrPunctsChanged;
	bool m_bUsfmStructureChanged;
	bool m_bProjOptionsShowing;
	CAdapt_ItApp* m_pApp;
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* GetSourceTextFromEditorDlg_h */

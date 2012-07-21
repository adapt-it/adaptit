/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GetSourceTextFromEditorDlg.h
/// \author			Bill Martin
/// \date_created	10 April 2011
/// \date_revised	17 November 2011
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for two friend classes: the CGetSourceTextFromEditorDlg and
/// the CChangeCollabProjectsDlg class. 
/// The CGetSourceTextFromEditorDlg class represents a dialog in which a user can obtain a source text
/// for adaptation from an external editor such as Paratext or Bibledit.
/// \derivation		The CGetSourceTextFromEditorDlg is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef GetSourceTextFromEditorDlg_h
#define GetSourceTextFromEditorDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "GetSourceTextFromEditorDlg.h"
#endif

class CGetSourceTextFromEditorDlg : public AIModalDialog
{
	//friend class CChangeCollabProjectsDlg;
public:
	CGetSourceTextFromEditorDlg(wxWindow* parent); // constructor
	virtual ~CGetSourceTextFromEditorDlg(void); // destructor
	
	// other methods
	//wxRadioBox* pRadioBoxChapterOrBook; // whm removed 28Jan12
	wxListBox* pListBoxBookNames;
	wxListView* pListCtrlChapterNumberAndStatus; // wxListBox* pListCtrlChapterNumberAndStatus;
	wxListItem* pTheFirstColumn; // has to be on heap
	wxListItem* pTheSecondColumn; // has to be on heap
	wxTextCtrl* pStaticTextCtrlNote;
	wxStaticText* pStaticSelectAChapter;
	wxStaticText* pSrcProj;
	wxStaticText* pTgtProj;
	wxStaticText* pFreeTransProj;
	wxStaticBox* pStaticBoxUsingTheseProjects;
	wxStaticText* pUsingAIProjectName;
	//wxButton* pBtnChangeProjects;
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

	// These are temporary m_Save... values for holding the App's original
	// collaboration settings upon entry to the GetSourceTextFromEditor dialog. 
	// They are used to restore those App values before Canceling the dialog. This
	// is designed to provide a safety net to help prevent the unintended 
	// saving of bogus collaboration settings as a result of actions taken in
	// this dialog that require adjusting the value of the App's collaboration
	// settings before callling WriteConfigurationFile(szProjectConfiguration....
	wxString m_SaveCollabProjectForSourceInputs;
	wxString m_SaveCollabProjectForTargetExports;
	wxString m_SaveCollabProjectForFreeTransExports;
	wxString m_SaveCollabAIProjectName;
	wxString m_SaveCollabSourceProjLangName;
	wxString m_SaveCollabTargetProjLangName;
	wxString m_SaveCollabBookSelected;
	bool m_bSaveCollabByChapterOnly; // FALSE means the "whole book" option
	wxString m_SaveCollabChapterSelected;
	bool m_bSaveCollaborationExpectsFreeTrans;

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

	wxArrayString m_staticBoxSourceDescriptionArray;
	wxArrayString m_staticBoxTargetDescriptionArray;
	wxArrayString m_staticBoxFreeTransDescriptionArray;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	//void OnBtnChangeProjects(wxCommandEvent& WXUNUSED(event)); // calls DoChangProjects()
	void OnLBBookSelected(wxCommandEvent& WXUNUSED(event));
	void OnLBChapterSelected(wxListEvent& WXUNUSED(event));
	void OnLBDblClickChapterSelected(wxCommandEvent& WXUNUSED(event));
	//void OnRadioBoxSelected(wxCommandEvent& WXUNUSED(event));

	//bool DoChangeProjects();
	//bool CollabProjectsExistAsAIProject(wxString LanguageNameSrc, 
	//							wxString LanguageNameTgt, wxString& aiProjectFolderName,
	//							wxString& aiProjectFolderPath);
	void RecordArrayDataForLastUsfm();
	void GetChapterListAndVerseStatusFromTargetBook(wxString targetBookFullName, 
								wxArrayString& chapterList, wxArrayString& statusList);
	void GetChapterListAndVerseStatusFromFreeTransBook(wxString freeTransBookFullName, 
								wxArrayString& chapterList, wxArrayString& statusList);
	void LoadBookNamesIntoList();
	wxString GetBareChFromLBChSelection(wxString lbChapterSelected);
	EthnologueCodePair* MatchAIProjectUsingEthnologueCodes(wxString& editorSrcLangCode,
								wxString& editorTgtLangCode);

private:
	bool m_bTextOrPunctsChanged;
	bool m_bUsfmStructureChanged;
	CAdapt_ItApp* m_pApp;
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
	
#endif /* GetSourceTextFromEditorDlg_h */

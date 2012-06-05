/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SetupEditorCollaboration.h
/// \author			Bill Martin
/// \date_created	8 April 2011
/// \date_revised	28 February 2012
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CSetupEditorCollaboration class. 
/// The CSetupEditorCollaboration class represents a dialog in which an administrator can set up Adapt It to
/// collaborate with an external editor such as Paratext or Bibledit. Once set up Adapt It will use projects
/// under the control of the external editor; obtaining its input (source) texts from one or more of the
/// editor's projects, and transferring its translation (target) texts to one of the editor's projects.
/// \derivation		The CSetupEditorCollaboration class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef SetupEditorCollaboration_h
#define SetupEditorCollaboration_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "SetupEditorCollaboration.h"
#endif

class CSetupEditorCollaboration : public AIModalDialog
{
public:
	CSetupEditorCollaboration(wxWindow* parent); // constructor
	virtual ~CSetupEditorCollaboration(void); // destructor
	// other methods
	
	// These are temporary values for holding settings until user clicks OK.
	// If user clicks Cancel, these values are ignored and not copied to the
	// corresponding settings on the App.
	bool m_bTempCollaboratingWithParatext;
	bool m_bTempCollaboratingWithBibledit;
	wxString m_TempCollabProjectForSourceInputs;
	wxString m_TempCollabProjectForTargetExports;
	wxString m_TempCollabProjectForFreeTransExports;
	wxString m_TempCollabAIProjectName;
	wxString m_TempCollaborationEditor;
	wxString m_TempCollabSourceProjLangName;
	wxString m_TempCollabTargetProjLangName;
	bool m_bTempCollabByChapterOnly; // FALSE means the "whole book" option
	bool m_bTempCollaborationExpectsFreeTrans;
	wxString m_TempCollabBookSelected;
	wxString m_TempCollabChapterSelected;
	
	// These are temporary m_Save... values for holding the App's original
	// collaboration settings upon entry to the SetupEditorCollaboration dialog. 
	// They are used to restore those App values before closing the dialog. This
	// is designed to provide a safety net to help prevent the unintended 
	// saving of bogus collaboration settings as a result of actions taken in
	// this dialog that require adjusting the value of the App's collaboration
	// settings before callling WriteConfigurationFile(szProjectConfiguration....
	bool m_bSaveCollaboratingWithParatext;
	bool m_bSaveCollaboratingWithBibledit;
	wxString m_SaveCollabProjectForSourceInputs;
	wxString m_SaveCollabProjectForTargetExports;
	wxString m_SaveCollabProjectForFreeTransExports;
	wxString m_SaveCollabAIProjectName;
	wxString m_SaveCollaborationEditor;
	wxString m_SaveCollabSourceProjLangName;
	wxString m_SaveCollabTargetProjLangName;
	bool m_bSaveCollabByChapterOnly; // FALSE means the "whole book" option
	bool m_bSaveCollaborationExpectsFreeTrans;
	wxString m_SaveCollabBookSelected;
	wxString m_SaveCollabChapterSelected;
	
	wxSizer* pSetupEditorCollabSizer;
	wxSize m_computedDlgSize; // stores the computed size of the dialog's sizer - accounting for its current layout state

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void DoInit();
	void OnCancel(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event); // void OnOK(wxCommandEvent& event);
	void OnBtnSelectFromListSourceProj(wxCommandEvent& WXUNUSED(event));
	void OnBtnSelectFromListTargetProj(wxCommandEvent& WXUNUSED(event));
	void OnBtnSelectFromListFreeTransProj(wxCommandEvent& WXUNUSED(event));
	void OnComboBoxSelectAiProject(wxCommandEvent& WXUNUSED(event));
	void DoSetControlsFromConfigFileCollabData(bool bCreatingNewProject);
	void OnNoFreeTrans(wxCommandEvent& WXUNUSED(event));
	void OnRadioBtnByChapterOnly(wxCommandEvent& WXUNUSED(event));
	void OnRadioBtnByWholeBook(wxCommandEvent& WXUNUSED(event));
	void OnCreateNewAIProject(wxCommandEvent& WXUNUSED(event)); // whm added 23Feb12
	void OnSaveSetupForThisProjNow(wxCommandEvent& WXUNUSED(event)); // whm added 23Feb12
	bool DoSaveSetupForThisProject();
	void OnRemoveThisAIProjectFromCollab(wxCommandEvent& WXUNUSED(event)); // whm added 23Feb12
	void SetStateOfRemovalButton();
	void SetPTorBEsubStringsInControls();
	void OnRadioBoxSelectBtn(wxCommandEvent& WXUNUSED(event)); // whm added 4Apr12

private:

	CAdapt_ItApp* m_pApp;
	wxTextCtrl* pTextCtrlAsStaticTopNote;
	wxTextCtrl* pTextCtrlAsStaticSelectedSourceProj;
	wxTextCtrl* pTextCtrlAsStaticSelectedTargetProj;
	wxTextCtrl* pTextCtrlAsStaticSelectedFreeTransProj;
	wxStaticText* pStaticTextUseThisDialog;
	wxStaticText* pStaticTextListOfProjects;
	wxStaticText* pStaticTextSelectWhichProj;
	wxStaticText* pStaticTextSrcTextFromThisProj;
	wxStaticText* pStaticTextTgtTextToThisProj;
	wxStaticText* pStaticTextFtTextToThisProj;
	//wxCheckBox* pCheckPwdProtectCollabSwitching;
	//wxButton* pBtnSetPwdForCollabSwitching;
	wxRadioBox* pRadioBoxScriptureEditor;
	wxListBox* pListOfProjects;
	wxComboBox* pComboAiProjects;
	wxButton* pBtnSelectFmListSourceProj;
	wxButton* pBtnSelectFmListTargetProj;
	wxButton* pBtnSelectFmListFreeTransProj;
	wxButton* pBtnNoFreeTrans;
	wxRadioButton* pRadioBtnByChapterOnly;
	wxRadioButton* pRadioBtnByWholeBook;
	wxButton* pBtnRemoveProjFromCollab;
	wxButton* pBtnClose;
	bool m_bCollabChangedThisDlgSession;
	wxArrayString projList;

	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* SetupEditorCollaboration_h */

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SetupEditorCollaboration.h
/// \author			Bill Martin
/// \date_created	8 April 2011
/// \rcs_id $Id$
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

enum KosherForCollab
{
	ignoredoc,
	nonkosher,
	kosher_bychapter,
	kosher_wholebook
};

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
    wxString m_TempCollabEditorVersion; // whm added 24June2016
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
    wxString m_SaveCollabEditorVersion; // whm added 24June2016
	wxString m_SaveCollabSourceProjLangName;
	wxString m_SaveCollabTargetProjLangName;
	bool m_bSaveCollabByChapterOnly; // keep input value, so we can test if user changes it in the dialog
	bool m_bSaveCollaborationExpectsFreeTrans;
	wxString m_SaveCollabBookSelected;
	wxString m_SaveCollabChapterSelected;
	
	wxSizer* pSetupEditorCollabSizer;
	wxSize m_computedDlgSize; // stores the computed size of the dialog's sizer - accounting for its current layout state

	// BEW 7Sep16, for support of anytime change of collaboration type 
	// (by chapter only <-> whole book), the following 5 booleans need
	// to be TRUE at the click of the "Accept this setup and prepare for another" 
	// button; and if the user tries to change the type when one of these has
	// changed, immediately return FALSE so that nothing is change - but give
	// him an information message to explain why his attempt was rejected
	bool m_bSameCollaborationEditor;
	bool m_bSameAIProjectName;
	bool m_bSameCollabSourceProjectLangName;  // such as TPK
	bool m_bSameCollabTargetProjectLangName;  // such as TPU
	bool m_bSameCollabEditorVersion; // don't allow changing of the collab type at 
					// the same time as the editor is being changed - eg. PT7 -> PT8

	// BEW 7Sep16, the following boolean tracks whether or not the user or
	// administrator requested a collaboration type change, such as 'whole book'
	// becoming 'by chapter only', or vise versa. A TRUE value here will initiate
	// the relevant splitting or joining of the collaborating document set; but only
	// provided the five bSame... booleans just above are each TRUE!
	bool m_bDifferentCollabType;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void DoInit(bool bPrompt);
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

	/////////////////////////////////////////////////////////////////////////////////////
	///     Functions for support of changing, on the fly, the collaboration type (ie. between 
	///		whole-book versus by-chapter-only)
	/////////////////////////////////////////////////////////////////////////////////////

	KosherForCollab TriageDocFile(wxString& doc);
	void SplitIntoChapters_ForCollaboration(wxString& docFolderPath, wxString& docFilenameBase,
		SPList* pTempSourcePhrasesList);

	// Function for splitting to chapter documents, or joining to whole book documents, if the
	// user changes the value of the app's m_bCollabByChapterOnly flag
	// BEW added 23Aug16 so users can change the value on the fly at any time
	bool AdjustForCollaborationTypeChange(bool bCurrentCollabByChapterOnly);


	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* SetupEditorCollaboration_h */

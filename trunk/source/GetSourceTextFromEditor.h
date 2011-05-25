/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GetSourceTextFromEditorDlg.h
/// \author			Bill Martin
/// \date_created	10 April 2011
/// \date_revised	10 April 2011
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
	wxComboBox* pComboDestinationProjectName;
	wxRadioBox* pRadioBoxWholeBookOrChapter;
	wxListBox* pListBoxBookNames;
	wxListBox* pListBoxChapterNumberAndStatus;
	wxTextCtrl* pStaticTextCtrlNote;
	wxStaticText* pStaticSelectAChapter;
	wxButton* pBtnCancel;

	wxString m_TempPTProjectForSourceInputs;
	wxString m_TempPTProjectForTargetExports;
	wxString m_TempPTBookSelected;
	wxString m_TempPTChapterSelected;
	wxArrayString projList;

	wxString sourceFileBuffer;
	wxString targetFileBuffer;
	wxString sourceChapterBuffer;
	wxString targetChapterBuffer;
	wxArrayString SourceTextUsfmStructureAndExtentArray;
	wxArrayString TargetTextUsfmStructureAndExtentArray;

	wxString m_rdwrtp7PathAndFileName;

	wxArrayString m_staticBoxDescriptionArray;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnComboBoxSelectSourceProject(wxCommandEvent& WXUNUSED(event));
	void OnComboBoxSelectDestinationProject(wxCommandEvent& WXUNUSED(event));
	void OnLBBookSelected(wxCommandEvent& WXUNUSED(event));
	void OnLBChapterSelected(wxCommandEvent& WXUNUSED(event));
	void OnLBDblClickChapterSelected(wxCommandEvent& WXUNUSED(event));
	void OnRadioBoxSelected(wxCommandEvent& WXUNUSED(event));

	bool PTProjectIsEditable(wxString projShortName);
	bool PTProjectsExistAsAIProject(wxString shortProjNameSrc, wxString shortProjNameTgt);
	bool EmptyVerseRangeIncludesAllVersesOfChapter(wxString emptyVersesStr);
	wxString GetShortNameFromLBProjectItem(wxString LBProjItem);
	void RecordArrayDataForLastUsfm();
	wxArrayString GetChapterListAndVerseStatusFromTargetBook(wxString targetBookFullName);
	wxString GetStatusOfChapter(const wxArrayString &TargetArray,int indexOfChItem,
									wxString targetBookFullName,wxString& nonDraftedVerses);
	wxString AbbreviateColonSeparatedVerses(const wxString str);
	void LoadBookNamesIntoList();
	void ExtractVerseNumbersFromBridgedVerse(wxString tempStr,int& nLowerNumber,int& nUpperNumber);

private:
	CAdapt_ItApp* m_pApp;
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* GetSourceTextFromEditorDlg_h */

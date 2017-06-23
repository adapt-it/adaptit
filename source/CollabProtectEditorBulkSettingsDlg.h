/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CollabProtectEditorBulkSettingsDlg.h
/// \author			Bill Martin
/// \date_created	18 April 2017
/// \rcs_id $Id$
/// \copyright		2017 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CCollabProtectEditorBulkSettingsDlg class. 
/// The CCollabProtectEditorBulkSettingsDlg class creates a dialog that an administrator can use to indicate
/// which books and/or chapters of an AI project should that should no longer transfer changes to Paratext/Bibledit.
/// Changes are still allowed to be made locally, but won't be transferred to the external editor for books and/or
/// chapters that are ticked within this dialog.
/// \derivation		The CCollabProtectEditorBulkSettingsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef CollabProtectEditorBulkSettingsDlg_h
#define CollabProtectEditorBulkSettingsDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "CollabProtectEditorBulkSettingsDlg.h"
#endif

class CCollabProtectEditorBulkSettingsDlg : public AIModalDialog
{
public:
    CCollabProtectEditorBulkSettingsDlg(wxWindow* parent); // constructor
    virtual ~CCollabProtectEditorBulkSettingsDlg(void); // destructor
    // The following are temporary values for holding settings until user clicks OK.
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
    wxString m_TempCollabBooksProtectedFromSavingToEditor;
    bool m_TempCollabDoNotShowMigrationDialogForPT7toPT8;

    bool m_bChangesMade;
    wxSizer* pCollabProtectEditorDlgSizer;
    wxString Step2StaticText;
    wxString Step3StaticText;
    wxString Step2StaticTextBookOnly;
    wxString Step3StaticTextBookOnly;


protected:
    void InitDialog(wxInitDialogEvent& WXUNUSED(event));
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnLBSelchangeListBooks(wxCommandEvent& WXUNUSED(event));
    void OnCheckLBToggleListBooks(wxCommandEvent& event);
    void OnLBSelchangeListChapters(wxCommandEvent& WXUNUSED(event));
    void OnCheckLBToggleListChapters(wxCommandEvent& event);
    void OnComboBoxSelectAiProject(wxCommandEvent& WXUNUSED(event));
    void OnBtnAllBooks(wxCommandEvent& WXUNUSED(event));
    void OnBtnDeselectAllBooks(wxCommandEvent& WXUNUSED(event));
    void OnBtnNTBooks(wxCommandEvent& WXUNUSED(event));
    void OnBtnOTBooks(wxCommandEvent& WXUNUSED(event));
    void OnBtnDCBooks(wxCommandEvent& WXUNUSED(event));
    void OnBtnAllChapters(wxCommandEvent& WXUNUSED(event));
    void OnBtnDeselectAllChapters(wxCommandEvent& WXUNUSED(event));

    void DoSetControlsFromConfigFileCollabData();
    void ParseProtectedDataIntoArrays(wxString& inputStr, wxArrayString& allBookIDsInCurrentVersification, wxArrayString& selectedWholeBookIDs, wxArrayString& selectedBookIDs, wxArrayString& selectedChs);
    void LoadChaptersListBox(int nBookSel);
    int LoadBooksListBox();
    int GetBookArrayIndexFromVrsBookID(wxString bookCode);
    bool ProtectedDataHasChanged();
    //int GetBookArrayIndexFromVrsBookName(wxString bookName); // unused
    wxString GetProtectedChsForBookSelection(int nBookSel);
    wxString GetProtectedChsStringFromChListBoxSelections();
    wxString GetProtectedStringForAllBooksAndChaptersForSaving();

private:
    // class attributes
    CAdapt_ItApp* m_pApp;
    wxCheckListBox* pCheckListOfBooks;
    wxListBox* pListOfBooks;
    wxCheckListBox* pListOfChapters;
    wxComboBox* pComboAiProjects;
    wxArrayString projList;
    wxArrayString vrsBookIDs;
    wxArrayString vrsBookIDsForListBox;
    wxArrayString vrsBookChsForListBox;
    wxArrayString allBookIDsInCurrentVersification;

    // The next 6 arrays represent whole book IDs, chapter-only book IDs, and chapters strings correspponding to 
    // the chapter-only books respectively

    // selectedWholeBookIDs is an array that contains bookIDs of whole books that are selected to be "protected"
    // Books that are not selected remain as empty strings in the array.
    // The array is always parallel to the vrsBookIDs, as well as the Books List control, and the other selected...
    // arrays implemented here.
    // Note: selectedWholeBookIDs is independent of the next two arrays, since whole books can be present alongside book:ch selections
    wxArrayString selectedWholeBookIDs;

    // selectedBookIDs is an array that contains the bookIDs of chapter-only docs (in parallel with selectedChs) that 
    // have one or more chapters selected to be "protected". The array is always parallel to the vrsBookIDs, the
    // vrsBookIDsForListBox, the vrsBookChsForListBox, as well as the Chapters List control and the other selected...
    // arrays implemented here.
    // not selected/ticked books remain as empty strings in the array.
    wxArrayString selectedBookIDs;

    // selectedChs is an array that contains just the chapters (of the parallel selectedBookIDs) that are selected 
    // to be "protected". Chapters that are not selected do not have a presence in the strings in the array.
    // Note: If an element of selectedBookIDs is an empty string, so will the parallel element of selectedChs be empty
    wxArrayString selectedChs;

    // saveSelectedWholeBookIDs is an array that saves the state of the selectedWholeBookIDs array at the point 
    // the project config file has been read in and the initial state of protected books/chapters has been 
    // determined by the call to ParseProtectedDataIntoArrays(). This save... array can then be used to
    // determine whether changes have been made during the use of the dialog.
    wxArrayString saveSelectedWholeBookIDs;

    // saveSelectedBookIDs is an array that saves the state of the selectedBookIDs array at the point 
    // the project config file has been read in and the initial state of protected books/chapters has been 
    // determined by the call to ParseProtectedDataIntoArrays(). This save... array can then be used to
    // determine whether changes have been made during the use of the dialog..
    wxArrayString saveSelectedBookIDs;

    // saveSelectedChs is an array that saves the state of the selectedChs array at the point 
    // the project config file has been read in and the initial state of protected books/chapters has been 
    // determined by the call to ParseProtectedDataIntoArrays(). This save... array can then be used to
    // determine whether changes have been made during the use of the dialog.
    wxArrayString saveSelectedChs;

    wxStaticText* pStaticTextDialogTopicLine;
    wxStaticText* pStaticTextStep2;
    wxStaticText* pStaticTextStep3;
    wxStaticText* pStaticTextCollabEditor;
    wxStaticText* pStaticTextProjSrc;
    wxStaticText* pStaticTextProjTgt;
    wxStaticText* pStaticTextWholeBookOrCh;
    wxStaticText* pStaticTextVersificationName;
    wxButton* pBtnAllBooks;
    wxButton* pBtnDeselectAllBooks;
    wxButton* pBtnNTBooks;
    wxButton* pBtnOTBooks;
    wxButton* pBtnDCBooks;
    wxButton* pBtnAllChapters;
    wxButton* pBtnDeselectAllChapters;
    // other class attributes

    DECLARE_EVENT_TABLE()
};
#endif /* CollabProtectEditorBulkSettingsDlg_h */

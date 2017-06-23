/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CollabProtectEditorBulkSettingsDlg.cpp
/// \author			Bill Martin
/// \date_created	18 April 2017
/// \rcs_id $Id$
/// \copyright		2017 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CCollabProtectEditorBulkSettingsDlg class. 
/// The CCollabProtectEditorBulkSettingsDlg class creates a dialog that an administrator can use to indicate
/// which books and/or chapters of an AI project should that should no longer transfer changes to Paratext/Bibledit.
/// Changes are still allowed to be made locally, but won't be transferred to the external editor for books and/or
/// chapters that are ticked within this dialog.
/// \derivation		The CCollabProtectEditorBulkSettingsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "CollabProtectEditorBulkSettingsDlg.h"
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
#include <wx/textfile.h>
#include <wx/tokenzr.h>

#include "Adapt_It.h"
#include "CollabProtectEditorBulkSettingsDlg.h"
#include "CollabUtilities.h"

extern wxString szCollabProjectForSourceInputs;
extern wxString szCollabProjectForTargetExports;
extern wxString szCollabProjectForFreeTransExports;
extern wxString szCollabAIProjectName;
extern wxString szCollaborationEditor;
extern wxString szParatextVersionForProject; // whm added 24June2016 - the old obsolete project config file label
extern wxString szCollabParatextVersionForProject; // whm added 24June2016 the new project config file label
extern wxString szCollabExpectsFreeTrans;
extern wxString szCollabBookSelected;
extern wxString szCollabByChapterOnly;
extern wxString szCollabChapterSelected;
extern wxString szCollabSourceLangName;
extern wxString szCollabTargetLangName;
extern wxString szCollabBooksProtectedFromSavingToEditor; // whm added 2February2017
extern wxString szProjectConfiguration;
extern wxString szCollabDoNotShowMigrationDialogForPT7toPT8;
extern wxString AllBookIds[]; 


// event handler table
BEGIN_EVENT_TABLE(CCollabProtectEditorBulkSettingsDlg, AIModalDialog)
EVT_INIT_DIALOG(CCollabProtectEditorBulkSettingsDlg::InitDialog)
EVT_BUTTON(wxID_OK, CCollabProtectEditorBulkSettingsDlg::OnOK)
EVT_BUTTON(wxID_CANCEL, CCollabProtectEditorBulkSettingsDlg::OnCancel)
EVT_LISTBOX(ID_LISTBOX_BOOKS, CCollabProtectEditorBulkSettingsDlg::OnLBSelchangeListBooks) // handler for list item selection changed
EVT_CHECKLISTBOX(ID_LISTBOX_BOOKS, CCollabProtectEditorBulkSettingsDlg::OnCheckLBToggleListBooks) // handler for check box changed
EVT_LISTBOX(ID_CHECKLISTBOX_BOOKS, CCollabProtectEditorBulkSettingsDlg::OnLBSelchangeListBooks) // handler for list item selection changed
EVT_CHECKLISTBOX(ID_CHECKLISTBOX_BOOKS, CCollabProtectEditorBulkSettingsDlg::OnCheckLBToggleListBooks) // handler for check box changed
EVT_LISTBOX(ID_CHECKLISTBOX_CHAPTERS, CCollabProtectEditorBulkSettingsDlg::OnLBSelchangeListChapters) // handler for list item selection changed
EVT_CHECKLISTBOX(ID_CHECKLISTBOX_CHAPTERS, CCollabProtectEditorBulkSettingsDlg::OnCheckLBToggleListChapters) // handler for check box changed
EVT_COMBOBOX(ID_COMBO_AI_PROJECTS, CCollabProtectEditorBulkSettingsDlg::OnComboBoxSelectAiProject)
EVT_BUTTON(ID_BUTTON_BOOKS_SELECT_ALL, CCollabProtectEditorBulkSettingsDlg::OnBtnAllBooks)
EVT_BUTTON(ID_BUTTON_BOOKS_DE_SELECT_ALL, CCollabProtectEditorBulkSettingsDlg::OnBtnDeselectAllBooks)
EVT_BUTTON(ID_BUTTON_BOOKS_SELECT_NT, CCollabProtectEditorBulkSettingsDlg::OnBtnNTBooks)
EVT_BUTTON(ID_BUTTON_BOOKS_SELECT_OT, CCollabProtectEditorBulkSettingsDlg::OnBtnOTBooks)
EVT_BUTTON(ID_BUTTON_BOOKS_SELECT_DC, CCollabProtectEditorBulkSettingsDlg::OnBtnDCBooks)
EVT_BUTTON(ID_BUTTON_CHAPTERS_SELECT_ALL, CCollabProtectEditorBulkSettingsDlg::OnBtnAllChapters)
EVT_BUTTON(ID_BUTTON_CHAPTERS_DE_SELECT_ALL, CCollabProtectEditorBulkSettingsDlg::OnBtnDeselectAllChapters)
END_EVENT_TABLE()

CCollabProtectEditorBulkSettingsDlg::CCollabProtectEditorBulkSettingsDlg(wxWindow* parent) // dialog constructor
    : AIModalDialog(parent, -1, _("Manage Data Transfer Protections for Collaboration Editor"),
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // This dialog function below is generated in wxDesigner, and defines the controls and sizers
    // for the dialog. The first parameter is the parent which should normally be "this".
    // The second and third parameters should both be TRUE to utilize the sizers and create the right
    // size dialog.
    pCollabProtectEditorDlgSizer = CollabProtectEditorBulkSettingsDlgFunc(this, FALSE, TRUE);
    // The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

    m_pApp = (CAdapt_ItApp*)&wxGetApp();
    wxASSERT(m_pApp != NULL);

    pCheckListOfBooks = (wxCheckListBox*)FindWindowById(ID_CHECKLISTBOX_BOOKS);
    wxASSERT(pCheckListOfBooks != NULL);
    pListOfBooks = (wxListBox*)FindWindowById(ID_LISTBOX_BOOKS);
    wxASSERT(pListOfBooks != NULL);
    pListOfChapters = (wxCheckListBox*)FindWindowById(ID_CHECKLISTBOX_CHAPTERS);
    wxASSERT(pListOfChapters != NULL);
    pComboAiProjects = (wxComboBox*)FindWindowById(ID_COMBO_AI_PROJECTS);
    wxASSERT(pComboAiProjects != NULL);
    pStaticTextCollabEditor = (wxStaticText*)FindWindowById(ID_TEXT_COLLAB_EDITOR);
    wxASSERT(pStaticTextCollabEditor != NULL);
    pStaticTextProjSrc = (wxStaticText*)FindWindowById(ID_TEXT_EDITOR_PROJ_SOURCE_NAME);
    wxASSERT(pStaticTextProjSrc != NULL);
    pStaticTextProjTgt = (wxStaticText*)FindWindowById(ID_TEXT_EDITOR_PROJ_TARGET_NAME);
    wxASSERT(pStaticTextProjTgt != NULL);
    pStaticTextWholeBookOrCh = ( wxStaticText*)FindWindowById(ID_TEXT_BY_WHOLE_BOOK_OR_CHAPTER);
    wxASSERT(pStaticTextWholeBookOrCh != NULL);
    pStaticTextVersificationName = (wxStaticText*)FindWindowById(ID_TEXT_VERSIFICATION_NAME);
    wxASSERT(pStaticTextVersificationName != NULL);
    pStaticTextDialogTopicLine = (wxStaticText*)FindWindowById(ID_TEXT_DIALOG_TOPIC_LINE);
    wxASSERT(pStaticTextDialogTopicLine != NULL);
    pStaticTextStep2 = (wxStaticText*)FindWindowById(ID_TEXT_STEP2);
    wxASSERT(pStaticTextStep2 != NULL);
    pStaticTextStep3 = (wxStaticText*)FindWindowById(ID_TEXT_STEP3);
    wxASSERT(pStaticTextStep3 != NULL);

    pBtnAllBooks = (wxButton*)FindWindowById(ID_BUTTON_BOOKS_SELECT_ALL);
    wxASSERT(pBtnAllBooks != NULL);
    pBtnDeselectAllBooks = (wxButton*)FindWindowById(ID_BUTTON_BOOKS_DE_SELECT_ALL);
    wxASSERT(pBtnDeselectAllBooks != NULL);
    pBtnNTBooks = (wxButton*)FindWindowById(ID_BUTTON_BOOKS_SELECT_NT);
    wxASSERT(pBtnNTBooks != NULL);
    pBtnOTBooks = (wxButton*)FindWindowById(ID_BUTTON_BOOKS_SELECT_OT);
    wxASSERT(pBtnOTBooks != NULL);
    pBtnDCBooks = (wxButton*)FindWindowById(ID_BUTTON_BOOKS_SELECT_DC);
    wxASSERT(pBtnDCBooks != NULL);
    pBtnAllChapters = (wxButton*)FindWindowById(ID_BUTTON_CHAPTERS_SELECT_ALL);
    wxASSERT(pBtnAllChapters != NULL);
    pBtnDeselectAllChapters = (wxButton*)FindWindowById(ID_BUTTON_CHAPTERS_DE_SELECT_ALL);
    wxASSERT(pBtnDeselectAllChapters != NULL);

    // other attribute initializations
}

CCollabProtectEditorBulkSettingsDlg::~CCollabProtectEditorBulkSettingsDlg() // destructor
{

}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_INIT_DIALOG() event handler to initialize the dialog
/// by calling the InitDialog() function.
/// In this dialog the InitDialog() handler initializes some of the default static text strings,
/// calls GetPossibleAdaptionCollabProjects() and loads the possible AI collab projects into the
/// dialog's combobox, makes the pListOfBooks visible as the default listbox, and hides the 
/// pCheckListOfBooks (may be changed by the DoSetControlsFromConfigFile() function), and adjusts
/// the dialog size for its initial contents.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
    //InitDialog() is not virtual, no call needed to a base class

    // The Step2StaticText and Step3StaticText below should be identical to the strings used in the wxDesigner resource for
    // the CollabProtectEditorBulkSettingsDlgFunc() function, in order to appear only once in the localization po files.
    // Note: The following localizable texts override those in the wxDesigner CollabProtectEditorBulkSettingsDlgFunc() function!
    Step2StaticText = _("2. Select a book from the 'Books List' below. Its chapters will display in 'Chapters List'.");
    Step3StaticText = _("3. Tick chapters in the 'Chapters List' to keep changes in those chapters from Paratext.");
    // The next two strings are substituted at run time for a selected AI project that uses "Whole Book" collab mode.
    Step2StaticTextBookOnly = _("2. Select a book from the 'Books List' below.");
    Step3StaticTextBookOnly = _("3. Tick books in the 'Books List' to keep changes in those books from Paratext.");

    m_bChangesMade = FALSE;
    // The main work of InitDialog() is to fill the pComboAiProjects combo list with AI collab projects
    // If there are no AI collaboration projects we let the user know that fact and that this dialog can
    // only serve them for existing collab projects.
    // Get a potential list/array of AI projects for the pComboAiProjects combo box.
    wxArrayString aiCollabProjectNamesArray;
    m_pApp->GetPossibleAdaptionCollabProjects(&aiCollabProjectNamesArray);

    aiCollabProjectNamesArray.Sort();
    // Clear the combo box and load the sorted list of ai projects into it.
    pComboAiProjects->Clear();
    int ct;
    for (ct = 0; ct < (int)aiCollabProjectNamesArray.GetCount(); ct++)
    {
        pComboAiProjects->Append(aiCollabProjectNamesArray.Item(ct));
    }

    // Change the editor name "Paratext" to "Bibledit" if needed in first dialog line and step 3
    if (m_TempCollaborationEditor == _T("Bibledit"))
    {
        wxString tmpStr;
        tmpStr = pStaticTextDialogTopicLine->GetLabel();
        tmpStr.Replace(_T("Paratext"), _T("Bibledit"));
        pStaticTextDialogTopicLine->SetLabel(tmpStr);

        tmpStr = Step3StaticText;
        tmpStr.Replace(_T("Paratext"), _T("Bibledit"));
        pStaticTextStep3->SetLabel(tmpStr);
    }

    // Disable all buttons - except Cancel - until a project is selected from the combo box
    pBtnAllBooks->Disable();
    pBtnDeselectAllBooks->Disable();
    pBtnNTBooks->Disable();
    pBtnOTBooks->Disable();
    pBtnDCBooks->Disable();
    pBtnAllChapters->Disable();
    pBtnDeselectAllChapters->Disable();
    pListOfChapters->Disable();
    pListOfBooks->Disable();

    // Here in InitDialog, Make the pListOfBooks visible, and hide the pCheckListOfBooks
    pCheckListOfBooks->Hide();

    pCollabProtectEditorDlgSizer->Layout(); 
    // The hiding of the list box requires that we resize the dialog to fit its new contents. 
    // Note: The constructor's call of CollabProtectEditorBulkSettingsDlgFunc(this, FALSE, TRUE)
    // has its second parameter as FALSE to allow this resize here in InitDialog().
    wxSize dlgSize;
    dlgSize = pCollabProtectEditorDlgSizer->ComputeFittingWindowSize(this);
    this->SetSize(dlgSize);
    this->CenterOnParent();

    // Nothing more to do here in InitDialog. When the administrator selects an AI project from the
    // combo drop down list, the "Books List" list will get populated with the books contained in
    // the source project's versification, and if there are any protected documents, the "Chapters List"
    // gets populated-when the project is a Chapter Only mode of collaboration.
}

// event handling functions

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_BUTTON() event handler for the "OK" button.
/// In this dialog the OnOK() handler examines the selected... arrays and
/// compares them to the saveSelected... arrays to determine if a change
/// has been made for protected books/chapters in this dialog. If no change
/// is detected it simply returns to close the dialog. If a change was detected,
/// it calls GetProtectedStringForAllBooksAndChaptersForSaving() to generate a
/// new string of protected books/chapters and saves those along with all the
/// other collaboration settings (unchanged) to the project's config file.
/// In the background:
/// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
/// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
/// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
/// if the dialog is modeless.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::OnOK(wxCommandEvent& event)
{
    // Use the three save... arrays and compare them with the parallel selected... arrays to 
    // determine if changes have been made in the dialog session. The three compareisons are:
    //    saveSelectedWholeBookIDs compared with selectedWholeBookIDs
    //    saveSelectedBookIDs compared with selectedBookIDs
    //    saveSelectedChs compared with selectedChs
    // Non-empty strings in the array indicate a protection state exists for that item.
    wxString buildProtectedString = _T("");
    bool bDataChanged = FALSE;
    bDataChanged = ProtectedDataHasChanged();
    if (bDataChanged)
    {
        // Changes were made, so call GetProtectedChsStringFromChListBoxSelections() to gather 
        // the books/chapters into a composite string for saving to the project config file's 
        // CollabBooksProtectedFromSavingToEditor field. 
        buildProtectedString = GetProtectedStringForAllBooksAndChaptersForSaving();

        // Remove the final colon (':') from the buildProtectedString before saving
        if (!buildProtectedString.IsEmpty() && buildProtectedString.Last() == _T(':'))
        {
            buildProtectedString.Remove(buildProtectedString.Length() - 1);
        }

        // Using the m_Temp... local variables, update the App's values for this project for 
        // saving in the project config file. This is necessary since a project may not be open
        // and in such cases, the App's members won't have values for this project to use in 
        // saving the project config file.
        m_pApp->m_bCollaboratingWithParatext = m_bTempCollaboratingWithParatext;
        m_pApp->m_bCollaboratingWithBibledit =  m_bTempCollaboratingWithBibledit;
        m_pApp->m_CollabProjectForSourceInputs = m_TempCollabProjectForSourceInputs;
        m_pApp->m_CollabProjectForTargetExports = m_TempCollabProjectForTargetExports;
        m_pApp->m_CollabProjectForFreeTransExports = m_TempCollabProjectForFreeTransExports;
        m_pApp->m_CollabAIProjectName = m_TempCollabAIProjectName;
        m_pApp->m_collaborationEditor = m_TempCollaborationEditor;
        m_pApp->m_ParatextVersionForProject = m_TempCollabEditorVersion; // whm added 24June2016
        m_pApp->m_CollabSourceLangName = m_TempCollabSourceProjLangName;
        m_pApp->m_CollabTargetLangName = m_TempCollabTargetProjLangName;
        m_pApp->m_bCollabByChapterOnly = m_bTempCollabByChapterOnly; // FALSE means the "whole book" option
        m_pApp->m_bCollaborationExpectsFreeTrans = m_bTempCollaborationExpectsFreeTrans;
        m_pApp->m_CollabBookSelected = m_TempCollabBookSelected;
        m_pApp->m_CollabChapterSelected = m_TempCollabChapterSelected;

        m_pApp->m_CollabBooksProtectedFromSavingToEditor = buildProtectedString; // <<< This is the only collab setting that can change for this project

        m_pApp->m_bCollabDoNotShowMigrationDialogForPT7toPT8 = m_TempCollabDoNotShowMigrationDialogForPT7toPT8; // whm added 6April2017

        // Write any change (of protected docs) to collab settings immediately before exiting this handler. 
        // We can't use the App's m_curProjectPath because this dialog may be opened without
        // a project being open, so we must compose the current project path based on the selected
        // project's AI project name stored in m_TempCollabAIProjectName.

        wxString projectName = m_TempCollabAIProjectName;
        wxString curProjPath;
        if (!m_pApp->m_customWorkFolderPath.IsEmpty() && m_pApp->m_bUseCustomWorkFolderPath)
        {
            curProjPath = m_pApp->m_customWorkFolderPath + m_pApp->PathSeparator
                + projectName; // use the incoming project name
        }
        else
        {
            curProjPath = m_pApp->m_workFolderPath + m_pApp->PathSeparator
                + projectName; // use the incoming project name
        }

        bool bOK;
        bOK = m_pApp->WriteConfigurationFile(szProjectConfiguration, curProjPath, projectConfigFile);
        if (!bOK)
        {
            // Not likely to fail, so just log error
            m_pApp->LogUserAction(_T("Failed to write proj config file changes in CCollabProtectEditorBulkSettingsDlg::OnOK() handler."));
        }
    }
    event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_BUTTON() event handler for the "Cancel" button.
/// If the user selects the Cancel button, the ProtectedDataHasChanged() function
/// is called to determine if any changes were made to the protection settings
/// during the dialog session. If so, this handler notifies the user that changes
/// were made and confirms whether those changes are to be abandoned with a Yes/No
/// message box (No is the default selected button). If the user responds Yes, the 
/// dialog closes without saving the changes in the project config file. 
/// If the user responds No, the Cancel is ignored and the dialog remains open for
/// further interaction on the part of the user.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::OnCancel(wxCommandEvent & event)
{
    // Check if changes have been made and allow the user to abort the Cancel event
    int result = wxNO;
    if (ProtectedDataHasChanged())
    {
        wxString msg = _("You made changes to the Data Transfer Protections for the \"%s\" collaboration project.\n\nDo you want to abandon the changes you made?");
        msg = msg.Format(msg, m_TempCollabAIProjectName.c_str());
        wxString title = _("Changes made to the project settings");
        result = wxMessageBox(msg, title, wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT);
        if (result == wxNO)
        {
            // return without proceeding to the event.Skip() default call
            return;
        }
    }
    event.Skip();
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_LISTBOX() event handler for the "Books List"
/// combobox.
/// This handler fires only when the list item selection/highlight has changed. 
/// It doesn't fire when only the check box of the list item has changed (ticked/unticked).
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::OnLBSelchangeListBooks(wxCommandEvent & WXUNUSED(event))
{
    if (m_bTempCollabByChapterOnly)
    {
        if (!ListBoxPassesSanityCheck((wxControlWithItems*)pListOfBooks))
            return;

        int nBookSel;
        nBookSel = pListOfBooks->GetSelection();
        // When a particular book is selected/highlighted, this should trigger a repopulation of the
        // Chapter list box with the chapters listed for the book that is selected/highlighted
        // For each selection event the chapter list should be refreshed with a new list of chapters.
        // Only fill the "Chapters List" when the project is set to work by Chapter Only mode.
        // The LoadChaptersListBox() function retrieves the string of chapters for the selected book 
        // from the vrsBookChsForListBox array which is always kept in parallel with the items in the
        // pListOfChapters control.
        this->LoadChaptersListBox(nBookSel); // this loads Chapters List where appropriate
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_CHECKLISTBOX() event handler for the "Books List"
/// combobox.
/// This function fires when a checkbox has been ticked/unticked in the pCheckListOfBooks. 
/// This handler fires only when the check box of the list item has changed (ticked/unticked), 
/// and not when the list item selection/highlight has changed.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::OnCheckLBToggleListBooks(wxCommandEvent & event)
{
    // When the check box is ticked/unticked, make the active selection move to that list item
    // if it is not already at that item.
    int nItem = event.GetInt(); // get the index of the list box item whose box has changed
    pCheckListOfBooks->SetSelection(nItem); // ensure that the selection highlights the line having the clicked box
    
    // Update the selectedWholeBookIDs array for the change in state of the check box for this item (nItem)
    // The vrsBookIDs array is parallel to the pCheckListOfBooks control and contains the pure bookIDs, so
    // use it to insert the ticked whole book item into the selectedWholeBookIDs array.
    selectedWholeBookIDs.RemoveAt(nItem);
    selectedWholeBookIDs.Insert(vrsBookIDs.Item(nItem), nItem);
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_LISTBOX() event handler for the "Chapters List"
/// combobox.
/// This handler fires only when the list item selection/highlight has changed. 
/// It doesn't fire when only the check box of the list item has changed (ticked/unticked).
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::OnLBSelchangeListChapters(wxCommandEvent & WXUNUSED(event))
{
    if (!ListBoxPassesSanityCheck((wxControlWithItems*)pListOfChapters))
        return;
    // Do nothing in this handler - see the OnCheckLBToggleListChapters() handler below
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_CHECKLISTBOX() event handler for the "Chapters List"
/// combobox.
/// This function fires when a checkbox has been ticked/unticked in the pListOfChapters. 
/// This handler fires only when the check box of the list item has changed (ticked/unticked), 
/// and not when the list item selection/highlight has changed.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::OnCheckLBToggleListChapters(wxCommandEvent & event)
{
    // When the check box is ticked/unticked, make the active selection move to that list item
    // if it is not already at that item.
    int nItem = event.GetInt(); // get the index of the list box item whose box has changed
    pListOfChapters->SetSelection(nItem); // ensure that the selection highlights the line having the clicked box

    // Update the composite chapter string in selectedChs. 
    // Note: The index for storage in selectedChs is NOT the nItem from this event (chapter list toggle event), 
    // but is the index of the selected item in the pListOfBooks, whose chapters are being displayed.
    int nBookSel = pListOfBooks->GetSelection();
    wxASSERT(nBookSel != -1);
    // Update the selectedChs array for the change in state of the check box for this item (nItem);
    // get the updated composite chapters list to store in the selectedChs array.
    wxString protectedChsFromListBox = GetProtectedChsStringFromChListBoxSelections();
    // Update the selectedChs array
    selectedChs.RemoveAt(nBookSel);
    selectedChs.Insert(protectedChsFromListBox, nBookSel);
    // Make sure that the bookID is also updated for this change in chapter selection
    if (protectedChsFromListBox.IsEmpty())
    { 
        // With the current change/toggle state, no chapters are currently selected so we remove the 
        // bookID from selectedBookIDs by storing an empty string there.
        selectedBookIDs.RemoveAt(nBookSel);
        selectedBookIDs.Insert(wxEmptyString, nBookSel);
    }
    else
    {
        // There is at least one chapter selected. Update the selectedChs array,
        // and also ensure that the bookID also is present in selectBookIDs (in case this
        // change of toggle state is changing from no selected chapters to at least
        // one selected chapters).
        wxString bookIDofThisChapter = vrsBookIDs.Item(nBookSel);
        selectedBookIDs.RemoveAt(nBookSel);
        selectedBookIDs.Insert(bookIDofThisChapter, nBookSel);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_COMBOBOX() event handler for the "Select AI collaboration project"
/// combobox.
/// This function fires when a selection has been made in the combobox. 
/// The function calls the DoSetControlsFromConfigFileCollabData() function.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::OnComboBoxSelectAiProject(wxCommandEvent & WXUNUSED(event))
{
    DoSetControlsFromConfigFileCollabData(); // Sets all Temp collab values as read from proj config file 
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_BUTTON() event handler for the "ALL Books" button.
/// This function selects all book items. It is only callable when the project is 
/// set to "Whole Book" collaboration mode.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::OnBtnAllBooks(wxCommandEvent & WXUNUSED(event))
{
    wxASSERT(!this->m_bTempCollabByChapterOnly);
    // The "All Books" button is only enabled when a project is set to "Whole Book" mode
    // and the active list box is pCheckListOfBooks.
    // The vrsBookIDs is in parallel with pCheckListOfBooks, and has all the pure bookIDs
    // for the project's versification.
    int ct;
    int tot;
    tot = pCheckListOfBooks->GetCount();
    for (ct = 0; ct < tot; ct++)
    {
        pCheckListOfBooks->Check(ct);
        selectedWholeBookIDs.RemoveAt(ct);
        selectedWholeBookIDs.Insert(vrsBookIDs.Item(ct), ct);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_BUTTON() event handler for the "Deselect ALL Books" button.
/// This function deselects all book items. It is only callable when the project is 
/// set to "Whole Book" collaboration mode.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::OnBtnDeselectAllBooks(wxCommandEvent & WXUNUSED(event))
{
    wxASSERT(!this->m_bTempCollabByChapterOnly);
    // The "Deselect All Books" button is only enabled when a project is set to "Whole Book" mode
    // and the active list box is pCheckListOfBooks. We unselect all books by ensuring each
    // element of the selectedWholeBookIDs array contains an empty string.
    int ct;
    int tot;
    tot = pCheckListOfBooks->GetCount();
    for (ct = 0; ct < tot; ct++)
    {
        pCheckListOfBooks->Check(ct, FALSE);
        selectedWholeBookIDs.RemoveAt(ct);
        selectedWholeBookIDs.Insert(wxEmptyString, ct); // replace string at element ct with empty string
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_BUTTON() event handler for the "NT Books" button.
/// This function selects all New Testament (NT) book items. It is only callable when 
/// the project is set to "Whole Book" collaboration mode.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::OnBtnNTBooks(wxCommandEvent & WXUNUSED(event))
{
    // The "NT Books" button is only enabled when a project is set to "Whole Book" mode
    // and the active list box is pCheckListOfBooks.
    // Note: The "NT Books" button in Paratext's "Books to Create" dialog simply adds all the
    // NT Books to any previous selections that might have been made. We emulate that dialog.
    // The AllBooksCanonType[] 123-element array on the App contains all books classified as
    // "OT", "NT", "DC", or "" (empty string)
    // The AllBooksCanonType[] however is not parallel with the selected... arrays, so we
    // have to get the canon type by calling the App's GetCanonTypeFromBookCode() on the 
    // bookCode retrieved from the parallel vrsBookIDs array.

    // Go through the pCheckListOfBooks and for each element look up the bookID and
    // call the App's GetCanonTypeFromBookCode() function. If the book list item is the
    // correct canonType we check/tick its check box, and update the selectedWholeBookIds
    // array. The Chapters List is in a disabled state.
    int ct;
    int tot;
    wxString bookCode;
    wxString canonType;
    // Note: we need to use the bookCode from the parallel vrsBookIDs array instead of 
    // the contents of the pCheckListOfBooks control since pCheckListOfBooks has the 
    // (bookID) suffixed to the end of each element.
    tot = pCheckListOfBooks->GetCount();
    for (ct = 0; ct < tot; ct++)
    {
        bookCode = vrsBookIDs.Item(ct);
        canonType = m_pApp->GetCanonTypeFromBookCode(bookCode);
        if (canonType == _T("NT"))
        {
            pCheckListOfBooks->Check(ct);
            selectedWholeBookIDs.RemoveAt(ct);
            selectedWholeBookIDs.Insert(vrsBookIDs.Item(ct), ct);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_BUTTON() event handler for the "OT Books" button.
/// This function selects all Old Testament (OT) book items. It is only callable when 
/// the project is set to "Whole Book" collaboration mode.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::OnBtnOTBooks(wxCommandEvent & WXUNUSED(event))
{
    // The "OT Books" button is only enabled when a project is set to "Whole Book" mode
    // and the active list box is pCheckListOfBooks.
    // Note: The "OT Books" button in Paratext's "Books to Create" dialog simply adds all the
    // OT Books to any previous selections that might have been made.
    // The AllBooksCanonType[] 123-element array on the App contains all books classified as
    // "OT", "NT", "DC", or "" (empty string)
    // The AllBooksCanonType[] however is not parallel with the selected... arrays, so we
    // have to get the canon type by calling the App's GetCanonTypeFromBookCode() on the 
    // bookCode retrieved from the parallel vrsBookIDs array.

    // Go through the pCheckListOfBooks's parallel array vrsBookIDs and for each element 
    // look up the bookID and call the App's GetCanonTypeFromBookCode() function. If the 
    // book list item is the correct canonType we check/tick its pCheckListOfBooks's 
    // check box, and update the selectedWholeBookIds array. The Chapters List is in a 
    // disabled state.
    int ct;
    int tot;
    wxString bookCode;
    wxString canonType;
    // Note: we need to use the bookCode from the parallel vrsBookIDs array instead of 
    // the contents of the pCheckListOfBooks control since pCheckListOfBooks has the 
    // (bookID) suffixed to the end of each element.
    tot = vrsBookIDs.GetCount();
    for (ct = 0; ct < tot; ct++)
    {
        bookCode = vrsBookIDs.Item(ct);
        canonType = m_pApp->GetCanonTypeFromBookCode(bookCode);
        if (canonType == _T("OT"))
        {
            pCheckListOfBooks->Check(ct);
            selectedWholeBookIDs.RemoveAt(ct);
            selectedWholeBookIDs.Insert(vrsBookIDs.Item(ct), ct);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_BUTTON() event handler for the "DC Books" button.
/// This function selects all Deutero-canonical book items. It is only callable when 
/// the project is set to "Whole Book" collaboration mode.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::OnBtnDCBooks(wxCommandEvent & WXUNUSED(event))
{
    // The "DC Books" button is only enabled when a project is set to "Whole Book" mode
    // and the active list box is pCheckListOfBooks.
    // Note: The "DC Books" button in Paratext's "Books to Create" dialog simply adds all the
    // DC Books to any previous selections that might have been made.
    // The AllBooksCanonType[] 123-element array on the App contains all books classified as
    // "OT", "NT", "DC", or "" (empty string)
    // The AllBooksCanonType[] however is not parallel with the selected... arrays, so we
    // have to get the canon type by calling the App's GetCanonTypeFromBookCode() on the 
    // bookCode retrieved from the parallel vrsBookIDs array.

    // Go through the pCheckListOfBooks's parallel array vrsBookIDs and for each element 
    // look up the bookID and call the App's GetCanonTypeFromBookCode() function. If the 
    // book list item is the correct canonType we check/tick its pCheckListOfBooks's 
    // check box, and update the selectedWholeBookIds array.  The Chapters List is in a 
    // disabled state.
    int ct;
    int tot;
    wxString bookCode;
    wxString canonType;
    // Note: we need to use the bookCode from the parallel vrsBookIDs array instead of 
    // the contents of the pCheckListOfBooks control since pCheckListOfBooks has the 
    // (bookID) suffixed to the end of each element.
    tot = vrsBookIDs.GetCount();
    for (ct = 0; ct < tot; ct++)
    {
        bookCode = vrsBookIDs.Item(ct);
        canonType = m_pApp->GetCanonTypeFromBookCode(bookCode);
        if (canonType == _T("DC"))
        {
            pCheckListOfBooks->Check(ct);
            selectedWholeBookIDs.RemoveAt(ct);
            selectedWholeBookIDs.Insert(vrsBookIDs.Item(ct), ct);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_BUTTON() event handler for the "All Chapters of Book" button.
/// This function selects all Chapters List items. It is only callable when the project
/// is set to "Chapter Only" collaboration mode.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::OnBtnAllChapters(wxCommandEvent & WXUNUSED(event))
{
    // The "All Chapters" button is only enabled when a project is set to "Chapter Only" mode
    // and the active list box is pListOfChapters.

    // If Chapters List is not loaded, notify user and return
    if (pListOfChapters->GetCount() == 0)
    {
        wxMessageBox(_("Select a book from the \"Books List\"."), _("No book is selected."), wxICON_EXCLAMATION | wxOK);
        return;
    }
    int ct;
    int tot;
    tot = pListOfChapters->GetCount();
    for (ct = 0; ct < tot; ct++)
    {
        pListOfChapters->Check(ct);
    }
    // Get the book that is selected - the book that has its chapters displaying in PListOfChapters
    // nBookSel represents the index into the these "selection" arrays:
    //    selectedBookIDs
    //    selectedChs - Array containing just the chapters (of the parallel selectedBookIDs) that are selected to be "protected" - not selected books remain as empty strings in the array
    int nBookSel = pListOfBooks->GetSelection();
    wxString protectedChsFromListBox = GetProtectedChsStringFromChListBoxSelections();
    // The protectedChsFromListBox string should not be empty!
    wxASSERT(!protectedChsFromListBox.IsEmpty());
    // All chapters should be selected. Update the selectedChs array,
    // and also ensure that the bookID also is present in selectBookIDs (in case this
    // change of toggle state is changing from no selected chapters to all chapters
    // selected).
    wxString bookIDofThisChapter = vrsBookIDs.Item(nBookSel);
    selectedBookIDs.RemoveAt(nBookSel);
    selectedBookIDs.Insert(bookIDofThisChapter, nBookSel);
    selectedChs.RemoveAt(nBookSel);
    selectedChs.Insert(protectedChsFromListBox, nBookSel);
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: The EVT_BUTTON() event handler for the "Deselect All Chapters of Book" button.
/// This function deselects all Chapters List items. It is only callable when the project
/// is set to "Chapter Only" collaboration mode.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::OnBtnDeselectAllChapters(wxCommandEvent & WXUNUSED(event))
{
    // The "Deselect All Chapters" button is only enabled when a project is set to 
    // "Chapter Only" mode and the active list box is pListOfChapters.

    // If Chapters List is not loaded, notify user and return
    if (pListOfChapters->GetCount() == 0)
    {
        wxMessageBox(_("Select a book from the \"Books List\"."), _("No book is selected."), wxICON_EXCLAMATION | wxOK);
        return;
    }
    int ct;
    int tot;
    tot = pListOfChapters->GetCount();
    for (ct = 0; ct < tot; ct++)
    {
        pListOfChapters->Check(ct, FALSE);
    }

    // Get the book that is selected - the book that has its chapters displaying in PListOfChapters
    // nBookSel represents the index into the these "selection" arrays:
    //    selectedBookIDs
    //    selectedChs - Array containing just the chapters (of the parallel selectedBookIDs) that are selected to be "protected" - not selected books remain as empty strings in the array
    int nBookSel = pListOfBooks->GetSelection();
    wxString protectedChsFromListBox = GetProtectedChsStringFromChListBoxSelections();
    // The protectedChsFromListBox string should be empty!
    wxASSERT(protectedChsFromListBox.IsEmpty());
    // With the current change/toggle state, no chapters are currently selected so we remove the 
    // bookID from selectedBookIDs.
    selectedBookIDs.RemoveAt(nBookSel);
    selectedBookIDs.Insert(wxEmptyString, nBookSel);
    selectedChs.RemoveAt(nBookSel);
    selectedChs.Insert(protectedChsFromListBox, nBookSel);
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    (none)
/// \remarks
/// Called from: OnComboBoxSelectAiProject() handler.
/// This function gets the Collab... settings and their values from the project by calling the
/// App's GetCollaborationSettingsOfAIProject() function. It stores the retrieved values in the
/// Dialog's m_Temp... and m_bTemp... local variables. It also sets the dialog's controls 
/// appropriately from the config file's collab data.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::DoSetControlsFromConfigFileCollabData()
{
    int nSel = pComboAiProjects->GetSelection();
    wxString selStr = pComboAiProjects->GetStringSelection();
    if (nSel == wxNOT_FOUND)
    {
        ::wxBell();
        return;
    }
    // Get the AI project's collab settings if any. If the selected project already has
    // collab settings put them into the m_Temp... variables and into the dialog's controls, 
    // so the administrator can see what settings if any have already been made
    // for that AI project. If the selected project has no collab settings yet, parse the three
    // m_Temp... ones from the selected project's name (m_TempCollabAIProjectName,
    // m_TempCollabSourceProjLangName, and m_TempCollabTargetProjLangName), and supply defaults
    // for the others.
    wxArrayString collabSettingsArray;
    wxArrayString collabLabelsArray;

    m_pApp->GetCollaborationSettingsOfAIProject(selStr, collabLabelsArray, collabSettingsArray);
    wxASSERT(collabLabelsArray.GetCount() == collabSettingsArray.GetCount());
    // Assign the m_Temp... and m_bTemp... local variables for the selected project.
    // These collab settings must be preserved accurately so that nothing gets lost or 
    // inadvertently changed when the OnOK() handler saves any changes to the protected 
    // document settings. The m_TempCollabBooksProtectedFromSavingToEditor holds the only
    // collaboration setting that can be changed using this dialog.
    if (collabSettingsArray.GetCount() > 0)
    {
        // The AI project has collaboration settings
        int ct;
        int tot = (int)collabSettingsArray.GetCount();
        wxString collabLabelStr;
        wxString collabItemStr;
        wxString saveCollabEditor = m_TempCollaborationEditor;
        // Scan through the two arrays in parallel and assign values to the m_Temp... and m_bTemp... variables
        for (ct = 0; ct < tot; ct++)
        {
            // Note: The main m_Temp... and m_bTemp values used by the SetupEditorCollaboration
            // dialog are:
            //    m_TempCollabProjectForSourceInputs
            //    m_TempCollabProjectForTargetExports
            //    m_TempCollabProjectForFreeTransExports
            //    m_TempCollabAIProjectName
            //    m_TempCollaborationEditor
            //    m_TempCollabEditorVersion // whm added 25June2016
            //    m_bTempCollabByChapterOnly
            // These m_Temp... and m_bTemp... values are derived from others or are unused
            //      m_bTempCollaborationExpectsFreeTrans
            //      m_TempCollabBookSelected
            //      m_TempCollabChapterSelected
            //      m_TempCollabSourceProjLangName
            //      m_TempCollabTargetProjLangName
            //      m_TempCollabBooksProtectedFromSavingToEditor // whm added 2February2017
            //      m_TempCollabDoNotShowMigrationDialogForPT7toPT8 // whm added 6April2017
            collabLabelStr = collabLabelsArray.Item(ct);
            collabItemStr = collabSettingsArray.Item(ct);
            if (collabLabelStr == szCollabProjectForSourceInputs)
                this->m_TempCollabProjectForSourceInputs = collabItemStr;
            else if (collabLabelStr == szCollabProjectForTargetExports)
                this->m_TempCollabProjectForTargetExports = collabItemStr;
            else if (collabLabelStr == szCollabProjectForFreeTransExports)
                this->m_TempCollabProjectForFreeTransExports = collabItemStr;
            else if (collabLabelStr == szCollabAIProjectName)
                this->m_TempCollabAIProjectName = collabItemStr;
            else if (collabLabelStr == szCollaborationEditor)
            {
                this->m_TempCollaborationEditor = collabItemStr;
            }
            else if (collabLabelStr == szParatextVersionForProject) // the old proj config file label in case it exists (only used in a pre-release version to developers)
            {
                this->m_TempCollabEditorVersion = collabItemStr; // whm added 24June2016
            }
            else if (collabLabelStr == szCollabParatextVersionForProject) // the new proj config file label for ongoing use
            {
                this->m_TempCollabEditorVersion = collabItemStr; // whm added 24June2016
            }
            else if (collabLabelStr == szCollabExpectsFreeTrans)
            {
                if (collabItemStr == _T("1"))
                    this->m_bTempCollaborationExpectsFreeTrans = TRUE;
                else
                    this->m_bTempCollaborationExpectsFreeTrans = FALSE;
            }
            else if (collabLabelStr == szCollabBookSelected)
                this->m_TempCollabBookSelected = collabItemStr;
            else if (collabLabelStr == szCollabByChapterOnly)
            {
                if (collabItemStr == _T("0"))
                    this->m_bTempCollabByChapterOnly = FALSE;
                else
                    this->m_bTempCollabByChapterOnly = TRUE;
            }
            else if (collabLabelStr == szCollabChapterSelected)
                this->m_TempCollabChapterSelected = collabItemStr;
            else if (collabLabelStr == szCollabSourceLangName)
                this->m_TempCollabSourceProjLangName = collabItemStr;
            else if (collabLabelStr == szCollabTargetLangName)
                this->m_TempCollabTargetProjLangName = collabItemStr;
            else if (collabLabelStr == szCollabBooksProtectedFromSavingToEditor)
                this->m_TempCollabBooksProtectedFromSavingToEditor = collabItemStr;
            else if (collabLabelStr == szCollabDoNotShowMigrationDialogForPT7toPT8)
            {
                if (collabItemStr == _T("0"))
                    this->m_TempCollabDoNotShowMigrationDialogForPT7toPT8 = FALSE;
                else
                    this->m_TempCollabDoNotShowMigrationDialogForPT7toPT8 = TRUE;
            }
        }
    }
    // The collaboration settings are now read in from the project selected in the combo box of AI projects
    // InitDialog() has only populated the combo control with collab projects, so we don't have to verify
    // those projects here. Here we adjust the static text data describing the collab project, and then
    // populate the books list.
    // Since the m_TempCollaborationEditor may have changed above we need to get a fresh
    // list of editor projects.

    wxString editorProjectsPath;
    projList.Clear();
    if (m_TempCollaborationEditor == _T("Paratext"))
    {
        if (m_TempCollabEditorVersion == _T("PTVersion7"))
        {
            projList = m_pApp->GetListOfPTProjects(_T("PTVersion7")); // as a side effect, it populates the App's m_pArrayOfCollabProjects
            editorProjectsPath = m_pApp->GetParatextProjectsDirPath(_T("PTVersion7"));
        }
        else if (m_TempCollabEditorVersion == _T("PTVersion8"))
        {
            projList = m_pApp->GetListOfPTProjects(_T("PTVersion8")); // as a side effect, it populates the App's m_pArrayOfCollabProjects
            editorProjectsPath = m_pApp->GetParatextProjectsDirPath(_T("PTVersion8"));
        }
        else if (m_TempCollabEditorVersion == _T("PTLinuxVersion7"))
        {
            projList = m_pApp->GetListOfPTProjects(_T("PTLinuxVersion7")); // as a side effect, it populates the App's m_pArrayOfCollabProjects
            editorProjectsPath = m_pApp->GetParatextProjectsDirPath(_T("PTLinuxVersion7"));
        }
        else if (m_TempCollabEditorVersion == _T("PTLinuxVersion8"))
        {
            projList = m_pApp->GetListOfPTProjects(_T("PTLinuxVersion8")); // as a side effect, it populates the App's m_pArrayOfCollabProjects
            editorProjectsPath = m_pApp->GetParatextProjectsDirPath(_T("PTLinuxVersion8"));
        }
    }
    else if (m_TempCollaborationEditor == _T("Bibledit"))
    {
        projList = m_pApp->GetListOfBEProjects(); // as a side effect, it populates the App's m_pArrayOfCollabProjects
        
        // Versification info for Bibledit:
        // Inspecting bibledit-gtk version 4.9, it stores its versification information in xml files located in the 
        // Linux system at: /usr/share/bibledit-gtk/. 
        // The xml files are named as follows:
        //    versification_dutch_traditional.xml
        //    versification_english.xml
        //    versification_original.xml
        //    versification_roman_catholic_1.xml
        //    versification_russian_canonical.xml
        //    versification_russian_orthodox.xml
        //    versification_russian_protestant.xml
        //    versification_septuagint.xml
        //    versification_spanish.xml
        //    versification_staten_bible.xml
        //    versification_vulgate.xml
        // The external name of the xml file is the only way to know what versification is represented by the content,
        // so versification_english.xml has the "English" versification, versification_russian_protestant.xml has the
        // "Russian Protestant" versification, etc.
        // Bibledit divides versification groupings differently than Paratext. 
        // The xml structure of the above files is:
        //
        // <bibledit - versification - system>
        //    <triad>
        //    <book>Genesis</book>
        //    <chapter>1</chapter>
        //    <verse>31</verse>
        //    </triad>
        //    <triad>
        //    <book>Genesis</book>
        //    <chapter>2</chapter>
        //    <verse>25</verse>
        //    </triad>
        //    ...
        //    <triad>
        //    <book>Revelation</book>
        //    <chapter>22</chapter>
        //    <verse>21</verse>
        //    </triad>
        // </bibledit-versification-system>
        //
        // where <book> <chapter> <verse> tag "triad"s enumerate the (usual English) name of the <book>,
        // the <chapter> tag indicates the chapter number, and the <verse> tag indicates the number of
        // verses that exist in the chapter.
        // Parsing this xml file, we only need to parse out the <book> name and the <chapter> numbers for
        // our whole book / chapter only purposes.

        // In light of the above, we'll hard code the path to the Bibledit versification files.
#if defined(_DEBUG) && defined(__WXMSW__)
        // Testing Bibledit on Windows
        // Note: For this test to work there must be a valid .bibledit folder in the Windows Docments folder (C:\Users\Bill\Documents\.bibledit)
        //       AND, the versification xml files located in the path: C:\Users\Bill\Documents\.bibledit\projects:
        //    versification_dutch_traditional.xml
        //    versification_english.xml
        //    versification_original.xml
        //    versification_roman_catholic_1.xml
        //    versification_russian_canonical.xml
        //    versification_russian_orthodox.xml
        //    versification_russian_protestant.xml
        //    versification_septuagint.xml
        //    versification_spanish.xml
        //    versification_staten_bible.xml
        //    versification_vulgate.xml
        editorProjectsPath = m_pApp->GetBibleditProjectsDirPath();
#else
        editorProjectsPath = _T("/usr/share/bibledit-gtk");
#endif
    }

    // Get the project's <Versification/> value and parse the appropriate .vrs file for Paratext,
    // or the appropriate versification_<name>.xml file for Bibledit.
    // Then use the books and chapter content specified in the .vrs/.xml file to populate the
    // "Select Book" list box. The data gleaned from the .vrs/.xml file will also be used to
    // populate the "Select Chapter" list items that are available for when a Book is 
    // selected.
    int tot = (int)m_pApp->m_pArrayOfCollabProjects->GetCount();
    wxString versificationStr = _T("");
    wxString collabProjShortName;
    wxString vrsFileName;
    wxString versificationName;
    wxString versificationNamePreserveCase;
    collabProjShortName = GetShortNameFromProjectName(m_TempCollabProjectForSourceInputs); // m_TempCollabProjectForSourceInputs is th composite name of the source project
    Collab_Project_Info_Struct* pArrayItem;
    pArrayItem = (Collab_Project_Info_Struct*)NULL;
    int ct;
    for (ct = 0; ct < tot; ct++)
    {
        pArrayItem = (Collab_Project_Info_Struct*)(*m_pApp->m_pArrayOfCollabProjects)[ct];
        wxASSERT(pArrayItem != NULL);
        if (pArrayItem != NULL && pArrayItem->shortName == collabProjShortName)
        {
            versificationStr = pArrayItem->versification;
            break;
        }
    }
    if (m_TempCollaborationEditor == _T("Paratext"))
    {
        // map to Paratext versification info scheme
        int versificationInt;
        versificationInt = wxAtoi(versificationStr);
        vrsFileName = m_pApp->GetVersificationFileNameFromEnumVal(versificationInt);
        versificationName = m_pApp->GetVersificationNameFromEnumVal(versificationInt);
        versificationNamePreserveCase = versificationName;
        editorProjectsPath = editorProjectsPath + m_pApp->PathSeparator + vrsFileName;
    }
    else if (m_TempCollaborationEditor == _T("Bibledit"))
    {
        // map to Bibledit versification info scheme
        versificationName = versificationStr;
        versificationNamePreserveCase = versificationName;
        versificationName.MakeLower(); // the filenames use lower case
        versificationName.Replace(_T(" "), _T("_")); // replace spaces with underscores
        vrsFileName = _T("versification_") + versificationName + _T(".xml");
        editorProjectsPath = editorProjectsPath + m_pApp->PathSeparator + vrsFileName;
    }

    // Fill dialog controls with collab settings.
    // Note: The combo box selection in step 1 was selected by the administrator so something should be selected!
    wxASSERT(pComboAiProjects->GetSelection() != -1);
    if (m_TempCollaborationEditor == _T("Paratext"))
    {
        wxString PTversionNum;
        if (m_TempCollabEditorVersion == _T("PTVersion7"))
            PTversionNum = _T("7");
        else if (m_TempCollabEditorVersion == _T("PTVersion8"))
            PTversionNum = _T("8");
        else if (m_TempCollabEditorVersion == _T("PTLinuxVersion7"))
            PTversionNum = _T("7 (Linux)");
        else if (m_TempCollabEditorVersion == _T("PTLinuxVersion8"))
            PTversionNum = _T("8 (Linux)");

        pStaticTextCollabEditor->SetLabel(m_TempCollaborationEditor + _T(" ") + PTversionNum);
    }
    else
        pStaticTextCollabEditor->SetLabel(m_TempCollaborationEditor);
    pStaticTextProjSrc->SetLabel(m_TempCollabProjectForSourceInputs);
    pStaticTextProjTgt->SetLabel(m_TempCollabProjectForTargetExports);
    if (m_bTempCollabByChapterOnly)
        pStaticTextWholeBookOrCh->SetLabel(_("Chapter Only Documents"));
    else 
        pStaticTextWholeBookOrCh->SetLabel(_("Whole Book Documents"));
    pStaticTextVersificationName->SetLabel(versificationNamePreserveCase);

    // Locate and parse the vrs file, filling arrays/structs with the book and chapter data
    // The editorProjectsPath string contains the currently selected project's path.
    // Tom H mentions there could be a custom.vrs file in a PT project directory. From what I can
    // see this is a file that can be used to "fine tune" an established versification scheme, and
    // not replace the established one. For example, there is a custom.vrs file in the PT7
    // tpi2008 project (Tok Pisin). The custom.vrs file says it was prepared by Michael Johnson. 
    // What it does it tweaks the tpi2008.ssf's <Versification>4</Versification> (English) 
    // versification scheme, by having the following content in custom.vrs:
    // - - - - - - - - - - - - - - -
    //# custom.vrs
    //# prepared by Michael Johnson for tpi2008 project
    //# amend total quantity of verses in chapter
    //2CO 13:13
    //TOB 10 : 12
    //ESG 8 : 41
    //SIR 20 : 31
    //SIR 33 : 33
    //SIR 35 : 24
    //SIR 36 : 27
    //SIR 41 : 27
    //BAR 3 : 38
    //BAR 6 : 0
    //            
    //# deliberately missing verses
    //#-2CO 13:14
    // - - - - - - - - - - - - - - -
    // What the above tpi2008 custom.vrs file does is simply inform PT about an intentional
    // departure of the number of verses for some indicated books/chapters. The 2CO 13:13 entry
    // (on first uncommented line) informs Paratext that 2CO chapter 13 intentionally has just 
    // 13 verses (instead of 14 verses according to the 2CO 13:14 that exists in the eng.vrs 
    // versification scheme. The #-2CO 13:14 explicitly (with a prefixed minus) indicates that 
    // verse 14 of 2CO chapter 13 is left out.
    // Although it is hard to be certain, it seems that a custom.vrs file is likely never used 
    // to alter the number of chapters specified within one of the standard *.vrs files, but 
    // only the inventory of verses. So, since we aren't worried about the total number of 
    // verses contained within chapters, I think we can safely ignore any custom.vrs file that 
    // may be present within the project directory, and focus only on generating a list of
    // books and chapters. 
    if (!::wxFileExists(editorProjectsPath))
    {
        wxString msg = _("Cannot find the versification file at:\n\n   %s\n\nThe \"Manage Data Transfer Protections for Collaboration Editor\" dialog cannot be used because Adapt It cannot list all of the books and chapters for the %s versification. Instead of using this dialog, you can still use these menu items on the Advanced menu:\n\n   Prevent Paratext/Bibledit from getting changes to this document\n   Allow Paratext/Bibledit to get changes to this document\n\nto adjust the protections for a collaboration document that you have opened in Adapt It. Please Cancel this dialog now.");
        msg = msg.Format(msg, editorProjectsPath.c_str(), versificationName.c_str());
        wxString title = _("The %s versification definition file was not found");
        title = title.Format(title, versificationName.c_str());
        m_pApp->LogUserAction(msg);
        wxMessageBox(msg, title, wxICON_WARNING | wxOK);
        return;
    }
    // if we get here the versification file was found, so open it and parse its content
    // for loading the data into the ListBoxes.
    bool bOpenedOK;

    // These wxArrayString arrarys are parallel with the parsed vrs file, but not parallel with
    // the AllBookIDs[] array of all possible book IDs. Most of the vrs files won't have all possible
    // book IDs, but will have a substantial subset of them.
    vrsBookIDs.Clear();
    vrsBookIDsForListBox.Clear();
    vrsBookChsForListBox.Clear();

    wxTextFile f;
    bOpenedOK = f.Open(editorProjectsPath);
    if (bOpenedOK)
    {
        // The vrsFileName file is now in memory and accessible line-by-line through textFile.
        wxString lineStr = _T("");
        wxString bookId = _T("");
        wxString bookFullName = _T("");
        wxString prevBookFullName = _T("");
        wxString bookNameAndId = _T("");
        wxString buildChStr = _T("");
        wxString chvsDataForBook = _T("");
        wxString tokenStr = _T("");
        wxString chPart = _T("");
        if (m_TempCollaborationEditor == _T("Paratext"))
        {
            for (lineStr = f.GetFirstLine(); !f.Eof(); lineStr = f.GetNextLine())
            {
                // Skip the line if it is a comment line
                lineStr.Trim(FALSE); // trim any whitespace at left end
                if (lineStr.Find(_T("#")) != 0)
                {
                    // line is not a # commented line
                    // Is the first word on line a recognizable book code?
                    if (lineStr.Length() > 3 && lineStr.Find(_T(" ")) == 3 && lineStr.Find(_T("=")) == wxNOT_FOUND) // the 4th char on line is a space and the line is not a mapping line with '=' in it
                    {
                        bookId = lineStr.Mid(0, 3);
                        chvsDataForBook = lineStr.Mid(4);
                        if (m_pApp->IsValidBookID(bookId))
                        {
                            // Add the bookID to the vrsBookIDs and vrsBookIDsForListBox arrays
                            vrsBookIDs.Add(bookId);
                            bookNameAndId = m_pApp->GetBookNameFromBookCode(bookId, _T("Paratext")) + _T(" (") + bookId + _T(")");
                            vrsBookIDsForListBox.Add(bookNameAndId);
                            //wxLogDebug(bookNameAndId);
                            buildChStr = _T("");
                            // Parse out the chapter numbers of the chvsDataForBook string, discarding the verse numbers
                            wxStringTokenizer tkz(chvsDataForBook, _T(" "));
                            while (tkz.HasMoreTokens())
                            {
                                // add the tokenStr to fitPartStr if it doesn't make fitPartStr get longer than extentRemaining
                                tokenStr = tkz.GetNextToken();
                                // each tokenStr will be of the form n:nn, we only want the chapter part before the colon
                                chPart = tokenStr.Mid(0, tokenStr.Find(_T(":")));
                                buildChStr = buildChStr + chPart + _T(":"); // buildChStr will always terminate with a colon ':'.
                            }
                            vrsBookChsForListBox.Add(buildChStr);
                            //wxLogDebug(_T("  ") + buildChStr);
                        }
                    }
                }
            } // end of for (lineStr = f.GetFirstLine(); !f.Eof(); lineStr = f.GetNextLine())
        }
        else if (m_TempCollaborationEditor == _T("Bibledit"))
        {
            for (lineStr = f.GetFirstLine(); !f.Eof(); lineStr = f.GetNextLine())
            {
                // The Bibledit file is an xml file (see structure described above). It is fairly simple xml structure, so
                // we'll just parse it directly using this wxTextFile line-by-line processing, rather than using a formal 
                // xml parser/reader.
                // Remove leading white space on lineStr
                int posOpenAngleBracket = lineStr.Find(_T("<"));
                if (posOpenAngleBracket != wxNOT_FOUND)
                {
                    lineStr = lineStr.Mid(posOpenAngleBracket);
                    // We only need to process lines that begin with '<book>' and '<chapter>'
                    if (lineStr.Find(_T("<book>")) != wxNOT_FOUND)
                    {
                        bookFullName = m_pApp->GetStringBetweenXMLTags(&f, lineStr, _T("<book>"), _T("</book>"));
                        if (bookFullName != prevBookFullName)
                        {
                            // It is a new book full name
                            // Add the bookID to the vrsBookIDs and vrsBookIDsForListBox arrays
                            bookId = m_pApp->GetBookCodeFromBookName(bookFullName);
                            vrsBookIDs.Add(bookId);
                            bookNameAndId = bookFullName + _T(" (") + bookId + _T(")");
                            vrsBookIDsForListBox.Add(bookNameAndId);
                            if (buildChStr != _T(""))
                            {
                                vrsBookChsForListBox.Add(buildChStr);
                                buildChStr = _T(""); // 
                            }
                            prevBookFullName = bookFullName;
                        }
                    }
                    else if (lineStr.Find(_T("<chapter>")) != wxNOT_FOUND)
                    {
                        chPart = m_pApp->GetStringBetweenXMLTags(&f, lineStr, _T("<chapter>"), _T("</chapter>"));
                        buildChStr = buildChStr + chPart + _T(":"); // buildChStr will always terminate with a colon ':'.
                    }
                }
            } // end of for (lineStr = f.GetFirstLine(); !f.Eof(); lineStr = f.GetNextLine())
            // We've processed to end of file
            // Process the last book's chapter data:
            // Just add buildChStr to vrsBookChsForListBox array
            vrsBookChsForListBox.Add(buildChStr);
        } // end of else if (m_TempCollaborationEditor == _T("Bibledit"))
    }
    f.Close();

    // Clear out the list boxes
    pListOfBooks->Clear();
    pCheckListOfBooks->Clear();
    pListOfChapters->Clear();
    // Load book data into the appropriate list box
    // When this->m_bTempCollabByChapterOnly is TRUE, then we load the vrsBookIDsForListBox
    // into the pListOfBooks list box, otherwise, we load the vrsBookIDsForListBox into
    // the pCheckListOfBooks. Show/Hide and Enable/Disable elements appropriately.
    if (this->m_bTempCollabByChapterOnly)
    {
        // Project is set to Chapter Only document mode
        // Hide the pCheckListOfBooks and make the pListOfBooks visible
        pCheckListOfBooks->Hide();
        pListOfBooks->Enable(TRUE);
        pListOfBooks->Show(TRUE);
        // Note: The pListOfBooks->InsertItems(vrsBookIDsForListBox, 0) call is done in the LoadBooksListBox() function
        // Enable the pListOfChapters
        pListOfChapters->Enable(TRUE);
        pBtnAllChapters->Enable(TRUE);
        pBtnDeselectAllChapters->Enable(TRUE);
        // Disable the Books range selection buttons
        pBtnAllBooks->Disable();
        pBtnDeselectAllBooks->Disable();
        pBtnNTBooks->Disable();
        pBtnOTBooks->Disable();
        pBtnDCBooks->Disable();
        wxString tempStr;
        tempStr = Step2StaticText; // _("2. Select a book from the 'Books List' below. Its chapters will display in 'Chapters List'.");
        pStaticTextStep2->SetLabel(tempStr);
        tempStr = Step3StaticText; // _("3. Tick chapters in the 'Chapters List' to keep changes in those chapters from Paratext.")
        pStaticTextStep3->SetLabel(tempStr);
        if (m_TempCollaborationEditor == _T("Bibledit"))
        {
            tempStr = pStaticTextDialogTopicLine->GetLabel();
            tempStr.Replace(_T("Paratext"), _T("Bibledit"));
            pStaticTextDialogTopicLine->SetLabel(tempStr); // becomes: _("To prevent changes in collaboration documents from transferring to Bibledit")

            tempStr = Step3StaticText;
            tempStr.Replace(_T("Paratext"), _T("Bibledit"));
            pStaticTextStep3->SetLabel(tempStr); // becomes: _("3. Tick chapters in the 'Chapters List' to keep changes in those chapters from Bibledit.")
        }
    }
    else
    {
        // Project is set to Whole Book document mode
        // Hide the pCheckListOfBooks and make the pListOfBooks visible
        pListOfBooks->Disable();
        pListOfBooks->Hide();
        pCheckListOfBooks->Show(TRUE);
        // Note: The pCheckListOfBooks->InsertItems(vrsBookIDsForListBox, 0) call is done in the LoadBooksListBox() function
        // Put "[Not used in 'Whole Book Documents' mode]" message in the Chapters List
        pListOfChapters->Insert(_("[Not used in 'Whole Book Documents' mode]"),0);
        //pListOfChapters->Check(0, TRUE);
        // Disable the pListOfChapters, and the "All Chapters" and "Deselect All Chapters" buttons
        pListOfChapters->Disable();
        pBtnAllChapters->Disable();
        pBtnDeselectAllChapters->Disable();
        // Enable the Books range selection buttons
        pBtnAllBooks->Enable(TRUE);
        pBtnDeselectAllBooks->Enable(TRUE);
        pBtnNTBooks->Enable(TRUE);
        pBtnOTBooks->Enable(TRUE);
        pBtnDCBooks->Enable(TRUE);
        wxString tempStr;
        tempStr = Step2StaticTextBookOnly; // _("2. Select a book from the 'Books List' below.");
        pStaticTextStep2->SetLabel(tempStr);
        tempStr = Step3StaticTextBookOnly; // _("3. Tick books in the 'Books List' to keep changes in those books from Paratext.")
        pStaticTextStep3->SetLabel(tempStr);
        if (m_TempCollaborationEditor == _T("Bibledit"))
        {
            tempStr = pStaticTextDialogTopicLine->GetLabel();
            tempStr.Replace(_T("Paratext"), _T("Bibledit"));
            pStaticTextDialogTopicLine->SetLabel(tempStr); // becomes: _("To prevent changes in collaboration documents from transferring to Bibledit")

            tempStr = Step3StaticTextBookOnly;
            tempStr.Replace(_T("Paratext"), _T("Bibledit"));
            pStaticTextStep3->SetLabel(tempStr); // becomes: _("3. Tick books in the 'Books List' to keep Adapt It from transferring changes in those books to Bibledit.")
        }

        // Note: The "Select Chapter" list boxes is filled with the data in the OnLBSelchangeListBooks() 
        // handler when a book within the "Select Book" list is selected.
    }

    pCollabProtectEditorDlgSizer->Layout();
    // The hiding of one of the list boxes requires that we resize the dialog to fit its new contents. 
    // Note: The constructor's call of CollabProtectEditorBulkSettingsDlgFunc(this, FALSE, TRUE)
    // has its second parameter as FALSE to allow this resize here in DoSetControlsFromConfigFileCollabData().
    wxSize dlgSize;
    dlgSize = pCollabProtectEditorDlgSizer->ComputeFittingWindowSize(this);
    this->SetSize(dlgSize);
    this->CenterOnParent();

    wxString inputStr = this->m_TempCollabBooksProtectedFromSavingToEditor;
    // The following protection string is for testing only!!!
    //wxString inputStr = _T("MAT MRK MRK:1:2:16 LUK ROM 1CO:4:6");

    // The ParseProtectedDataIntoArrays() function takes in an existing formatted string, inputStr, 
    // which is in the bookID and/or bookID:ch:ch... form of "MAT MRK MRK:1:3 ACT ACT:1:2:5:7", from 
    // the project config file and parses that string into the selected... arrays for the initial state 
    // protected books/chapters represented in this dialog. This function is the inverse of the 
    // GetProtectedStringForAllBooksAndChaptersForSaving() function, which uses the data in the selected... 
    // arrays to create the formatted string of protected books/chapters to save in the project config file.
    // First, this function initializes the four wxArrayString arrays referenced in the parameters. 
    // It initializes the allBookIDsInCurrentVersification array filling it with the BookIds of the
    // current project's versification scheme. 
    // Then it initializes the three selected... arrays all with the same number of empty strings.
    // Then it parses the m_TempCollabBooksProtectedFromSavingToEditor string which is sourced
    // from the CollabBooksProtectedFromSavingToEditor field, using the model array 
    // allBookIDsInCurrentVersification, into 3 arrays:
    //     selectedWholeBookIDs // initialize to have the same number of empty wxString elements as the model array
    //     selectedBookIDs // initialize to have the same number of empty wxString elements as the model array
    //     selectedChs // initialize to have the same number of empty wxString elements as the model array
    // and the above 3 arrays are always parallel to the model array allBookIDsInCurrentVersification which 
    // contains all book IDs from the currently selected project's versification scheme.
    // ParseProtectedDataIntoArrays() is called from: DoSetControlsFromConfigFileCollabData() handler (above),
    // so it gets called once for each project selected using the dialog's drop down AI Project combo box.
    // All parameters are reference parameters. The inputStr is a reference parameter, not to return a different 
    // value but because it may be a fairly large wxString, and to avoid constructing it as a value parameter.
    ParseProtectedDataIntoArrays(inputStr, allBookIDsInCurrentVersification, selectedWholeBookIDs, selectedBookIDs, selectedChs);

    // Load the Books List and Chapters List boxes 
    int firstBookIndex;
    firstBookIndex = LoadBooksListBox(); // Loads Books List and also ticks any protected whole books if project is Whole Book Only mode.
                                         // LoadBooksListBox() selects/highlights and returns the firstBookIndex of any protected book.
    LoadChaptersListBox(firstBookIndex); // Loads Chapters List box and also ticks any protected chapters if project is Chapters Only mode.
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    inputStr <-> a wxString containing the protected string of books/chs from the project config file
/// \param    allBookIDsInCurrentVersification <-> a wxArrayString
/// \param    selectedWholeBookIDs <-> a wxArrayString containing selected whole bookIDs
/// \param    selectedBookIDs <-> a wxArrayString containing selected bookIDs
/// \param    selectedChs <-> a wxArrayString containing selected chapters (in ch:ch:ch: form)
/// \remarks
/// Called from: DoSetControlsFromConfigFile().
/// The ParseProtectedDataIntoArrays() function takes in an existing formatted string, inputStr, 
/// which is in the bookID and/or bookID:ch:ch... form of "MAT MRK MRK:1:3 ACT ACT:1:2:5:7", from 
/// the project config file and parses that string into the selected... arrays for the initial state 
/// protected books/chapters represented in this dialog. This function is the inverse of the 
/// GetProtectedStringForAllBooksAndChaptersForSaving() function, which uses the data in the selected... 
/// arrays to create the formatted string of protected books/chapters to save in the project config file.
/// First, this function initializes the four wxArrayString arrays referenced in the parameters. 
/// It initializes the allBookIDsInCurrentVersification array filling it with the BookIds of the
/// current project's versification scheme. 
/// Then it initializes the three selected... arrays all with the same number of empty strings.
/// Then it parses the m_TempCollabBooksProtectedFromSavingToEditor string which is sourced
/// from the CollabBooksProtectedFromSavingToEditor field, using the model array 
/// allBookIDsInCurrentVersification, into 3 arrays:
///     selectedWholeBookIDs // initialize to have the same number of empty wxString elements as the model array
///     selectedBookIDs // initialize to have the same number of empty wxString elements as the model array
///     selectedChs // initialize to have the same number of empty wxString elements as the model array
/// and the above 3 arrays are always parallel to the model array allBookIDsInCurrentVersification which 
/// contains all book IDs from the currently selected project's versification scheme.
/// ParseProtectedDataIntoArrays() is called from: DoSetControlsFromConfigFileCollabData() handler (above),
/// so it gets called once for each project selected using the dialog's drop down AI Project combo box.
/// All parameters are reference parameters. The inputStr is a reference parameter, not to return a different 
/// value but because it may be a fairly large wxString, and to avoid constructing it as a value parameter.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::ParseProtectedDataIntoArrays(wxString & inputStr, wxArrayString& allBookIDsInCurrentVersification,
    wxArrayString & selectedWholeBookIDs, wxArrayString & selectedBookIDs, wxArrayString & selectedChs)
{
    // initialize arrays
    allBookIDsInCurrentVersification.Clear();
    selectedWholeBookIDs.Clear();
    selectedBookIDs.Clear();
    selectedChs.Clear();
    int ct;
    // The vrsBookIDs array has the pure book IDs that are parallel to the Books List control.
    int tot = (int)vrsBookIDs.GetCount();
    for (ct = 0; ct < tot; ct++)
    {
        allBookIDsInCurrentVersification.Add(vrsBookIDs.Item(ct));
        // The following 3 arrays remain in parallel to the allBookIDsInCurrentVersification model array above
        // They also remain in parallel to the contents of the corresponding list box controls.
        selectedWholeBookIDs.Add(wxEmptyString); // initialize to have the same number of empty wxString elements as the model array
        selectedBookIDs.Add(wxEmptyString); // initialize to have the same number of empty wxString elements as the model array
        selectedChs.Add(wxEmptyString); // initialize to have the same number of empty wxString elements as the model array
        
        // Also initialize the save... arrays
        // These save... arrays also remain in parallel to the contents of the corresponding list box controls.
        saveSelectedWholeBookIDs.Add(wxEmptyString); // initialize to have the same number of empty wxString elements as the model array
        saveSelectedBookIDs.Add(wxEmptyString); // initialize to have the same number of empty wxString elements as the model array
        saveSelectedChs.Add(wxEmptyString); // initialize to have the same number of empty wxString elements as the model array
    }

    // The inputStr coming from the project's configuration file's CollabBooksProtectedFromSavingToEditor field, 
    // might look something like:
    // "MAT MRK MRK:1:2:16 LUK ROM 1CO:4:6"
    // where whole books have stand-alone 3-letter codes, and chapter-only documents have a preceeding 3-letter code followed
    // by a list of protected chapters delimited by colons. A whole book code can co-exist with book:ch content for the same book
    // code, as the above book MRK illustrates. This might happen if a project switches between whole book and chapter only
    // collaboration modes and protects books and/or chapters in each mode. The dialog preserves the whole book protection
    // info for a chapter-only project, and preserves info for a chapter-only project for a whole book project - in case the
    // project later switches its mode of collaboration. The dialog, however, can only adjust the protections for the currently
    // set project mode.

    wxArrayString tokenArray;
    wxArrayString tempTokenArray;
    wxString tokenStr;
    // Tokenize the inputStr into chunks composed possibly of whole books, and book:ch strings.
    wxStringTokenizer tkz(inputStr, _T(" ")); // books/chunks are delimited by spaces
    while (tkz.HasMoreTokens())
    {
        tokenStr = tkz.GetNextToken();
        // do comparisons with forced uppercase in this function
        tokenStr.MakeUpper();
        tokenStr.Trim(TRUE);
        tokenStr.Trim(FALSE);
        tokenArray.Add(tokenStr);
    }
    
    // Go through inputStr's string tokens and assign protected element data to the select... arrays.
    wxString tempString;
    int tokenCt;
    int tokenTot = (int)tokenArray.GetCount();
    for (tokenCt = 0; tokenCt < tokenTot; tokenCt++)
    {
        // Need to clear out the tempTokenArray for each pass through the loop
        tempTokenArray.Clear();
        tempString.Empty(); // build a new string to return at end of processing

        wxString bookIDorCh = tokenArray.Item(tokenCt);
        int posCol = bookIDorCh.Find(_T(":"));
        wxString bkIDtoFind;
        bkIDtoFind.MakeUpper();
        wxString chaptersToFind = _T("");
        if (posCol != wxNOT_FOUND)
        {
            // a colon is present in bookIDorCh
            // Store the 3-letter bookID part
            bkIDtoFind = bookIDorCh.Mid(0, posCol);
            int index;
            index = GetBookArrayIndexFromVrsBookID(bkIDtoFind);
            wxString bkStr = allBookIDsInCurrentVersification.Item(index);
            selectedBookIDs.RemoveAt(index);
            selectedBookIDs.Insert(bkStr, index);
            // Store the chapters string in selectedChs array at the same parallel index as the bkStr above
            chaptersToFind = bookIDorCh.AfterFirst(_T(':'));
            // To make later parsing easier, append a final ':' to the chaptersToFind string
            chaptersToFind += _T(":");
            selectedChs.RemoveAt(index);
            selectedChs.Insert(chaptersToFind, index);
        }
        else
        {
            // Store the 3-letter bookID at the appropriate element in the selectedWholeBookIDs array
            bkIDtoFind = bookIDorCh.Mid(0, 3);
            size_t index;
            index = (size_t)GetBookArrayIndexFromVrsBookID(bkIDtoFind);
            wxString bkStr = allBookIDsInCurrentVersification.Item(index);
            selectedWholeBookIDs.RemoveAt(index);
            selectedWholeBookIDs.Insert(bkStr, index);
        }

    } // end of for (tokenCt = 0; tokenCt < tokenTot; tokenCt++)

    // Save the initial protection state of the newly parsed project. This is done only once 
    // here in the ParseProtectedDataIntoArrasy() to preserve the initial state before making
    // any changes in the dialog.
    for (ct = 0; ct < tot; ct++)
    {
        // The following arrays are always in parallel
        saveSelectedWholeBookIDs.Item(ct) = selectedWholeBookIDs.Item(ct);
        saveSelectedBookIDs.Item(ct) = selectedBookIDs.Item(ct);
        saveSelectedChs.Item(ct) = selectedChs.Item(ct);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \return   nothing
/// \param    nBookSel -> an int that represents the index of the book whose chapters are to be displayed
/// \remarks
/// Called from: OnLBSelchangeListBooks(), DoSetControlsFromConfigFile().
/// LoadChaptersListBox() gets a composite string of chapters from vrsBookChsForListBox array for the 
/// nBookSel, tokenizes that string into a lbChArray, then loads the chapters into the pListOfChapters 
/// dialog control. It then gets the protected chapters by calling GetProtectedChsForBookSelection() 
/// and ticks the check boxes that should be protected in pListOfChapters.
///////////////////////////////////////////////////////////////////////////////
void CCollabProtectEditorBulkSettingsDlg::LoadChaptersListBox(int nBookSel)
{
    // If nBookSel is -1 then there is no book selection and the Chapters list remains
    // empty
    if (nBookSel == -1)
        return;

    // If the project is set to work by whole books, don't load the Chapters list
    if (!this->m_bTempCollabByChapterOnly)
        return;

    // The list of chapters is a composite string at the nBookSel index in the vrsBookChsForListBox array
    // Get the bookID of selected book from the vrsBookID, and the composite string from vrsBookChsForListBox.
    wxString bookID = vrsBookIDs.Item(nBookSel);
    wxString allChsStr;
    allChsStr = vrsBookChsForListBox.Item(nBookSel); // vrsBookChsForListBox ch strings end with a colon

    // Tokenize the chapters out of the allChsStr string which has ':' delimiters (and ends with ':')
    // storing the chapters (formatted for list box) in an lbChArray.
    wxString chStr;
    wxArrayString lbChArray;
    wxStringTokenizer tkz(allChsStr, _T(":")); // chapters are delimited by colons
    wxString chapterLocal = _("Chapter");
    wxString space = _T(" ");
    while (tkz.HasMoreTokens())
    {
        chStr = tkz.GetNextToken();
        chStr.Trim(TRUE);
        chStr.Trim(FALSE);
        // Add the "Chapter " prefix to the chapter number string for the list box
        chStr = chapterLocal + space + chStr;
        lbChArray.Add(chStr);
    }

    // Fill the pListOfChapters with the lbChArray
    pListOfChapters->Clear();
    pListOfChapters->InsertItems(lbChArray, 0);

    // Get the protected chapters so we can tick the check boxes that should be protected 
    // in pListOfChapters based on any chapters present in the protected string for the nBookSel book
    wxString protectedChsStrForBook;
    protectedChsStrForBook = GetProtectedChsForBookSelection(nBookSel);
    // protectedChsStrForBook is a string that contains a list of protected chapters in the form "1:2:3:" 
    // without the bookID prefix, but with a final colon on the list of chapters so we can easily tokenize 
    // the string into chapters.
    wxString tokenStr;
    wxStringTokenizer tkz2(protectedChsStrForBook, _T(":")); // chapters are delimited by colons
    while (tkz2.HasMoreTokens())
    {
        tokenStr = tkz2.GetNextToken();
        tokenStr.Trim(TRUE);
        tokenStr.Trim(FALSE);
        // The listbox of chapters will have a localizable "Chapter " prefix, so we add that here to make it easier
        // to compare the lists
        tokenStr = chapterLocal + space + tokenStr;
        // Now find this formatted string item in the 'Chapter List' items stored in pListOfChapters
        int itemIndex;
        itemIndex = pListOfChapters->FindString(tokenStr);
        if (itemIndex != wxNOT_FOUND)
        {
            // Found it, so check/tick the checkbox of this item
            pListOfChapters->Check(itemIndex);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \return     an int representing the index of any pre-selected list box element.
/// \param     (none)
/// \remarks
/// Called from: DoSetControlsFromConfigFile().
/// LoadBooksListBox() loads the appropriate books list box depending on whether the project
/// utilizes whole book mode or chapter only mode: pListOfBooks for chapter only mode, and
/// pCheckListOfBooks for whole book mode.
/// For Chapter Only Mode, it selects the first whole book of any books that have chapters 
/// that should be protected and returns that first element's index so that the actual 
/// checking/ticking of Chapter Only items is done in the LoadChaptersListBox() function.
/// For Whole Book mode, it checks/ticks any boxes for whole books that should be marked 
/// as 'protected'.
/// The function returns the index of the first book that it finds is in the protected 
/// list of books, or -1 if no books are yet marked as projected for the project.
///////////////////////////////////////////////////////////////////////////////
int CCollabProtectEditorBulkSettingsDlg::LoadBooksListBox()
{
    int nFirstBookIndex = -1;
    // Load the appropriate Books List box as determined by the m_bTempCollabByChapterOnly
    // We use the array data that was set by a prior call to ParseProtectedDataIntoArrays()
    if (this->m_bTempCollabByChapterOnly)
    {
        // When in chapter only mode, the 'Books List' uses the pListOfBooks (wxListBox)
        // instead of the pCheckListOfBooks (wxCheckListBox). The pListOfBooks does not have
        // any checkboxes on each list item. The 'Chapters List' in chapter only mode is 
        // enabled - and filled by the LoadChaptersListBox() function.
        //
        // Fill the pListOfBooks with the possible book IDs using vrsBookIDsForListBox
        pListOfBooks->Clear();
        pListOfBooks->InsertItems(vrsBookIDsForListBox, 0);

        // When project is Chapter Only Mode, the books list box has no checkboxes, just a
        // list of books that can be selected to display chapters for the selected book in
        // the Chapters List box. If any chapters were previously marked as protected, we
        // can determine that from examining the selectedBookIDs array. If no book/chapters
        // were previously protected for this project, the selectedBookIDs array will only
        // contain empty strings. If a book has at least one protected chapter, that book's
        // bookID will be present in selectedBookIDs. We scan through selectedBookIDs, and
        // the first bookID we find there we return its index (nFirstBookIndex) to the caller.
        // The caller then passes that nFirstBookIndex on to the LoadChaptersListBox() function
        // to fill the appropriate chapters in the Chapter List.
        int selectedBookIndex;
        int selectedBookTotal;
        wxString bkid;
        wxString chid;
        wxString tempStr;
        int nFirstBookIndex = -1;
        selectedBookTotal = (int)selectedBookIDs.GetCount();
        for (selectedBookIndex = 0; selectedBookIndex < selectedBookTotal; selectedBookIndex++)
        {
            bkid = selectedBookIDs.Item(selectedBookIndex);
            if (bkid != _T(""))
            {
                // We found a BookID from selectedBookIDs that should be selected in the pListOfBooks.
                // We record the first instance index in nFirstBookIndex for passing to the LoadChaptersListBox()
                // function so it can display the chapters for this selected book.
                if (nFirstBookIndex == -1)
                {
                    nFirstBookIndex = selectedBookIndex;
                    pListOfBooks->SetSelection(nFirstBookIndex);
                    // The LoadChaptersListBox() will use nFirstBookIndex to display the appropriate chapters in its Chapters List
                    return nFirstBookIndex; 
                }
            }
        }
        // If no non-empty bookID was found in selectedBookIDs, the list won't have a selected item
        // and nFirstBookIndex will be passed as -1 to the LoadChaptersListBox() call - which will
        // leave that Chapters List empty - until the user actually selects a book in the Books List.
    }
    else
    {
        // When in whole book mode, the 'Books List' uses the pCheckListOfBooks (wxCheckListBox)
        // instead of the pListOfBooks (wxListBox). The pCheckListOfBooks has checkboxes on each
        // list item. The 'Chapters List' in this whole book mode is disabled, and displays the
        // message "[Not used in 'Whole Book Documents' mode]".
        //
        // Fill the pCheckListOfBooks with the possible book IDs using vrsBookIDsForListBox
        pCheckListOfBooks->Clear();
        pCheckListOfBooks->InsertItems(vrsBookIDsForListBox, 0);

        // Set the checkboxes of any protected whole books.
        //
        // When project is Whole Book Mode, the books list box has check boxes, and the chapters 
        // list is disabled. The pCheckListOfBooks should be set according to the data in
        // the arrays populated in a prior ParseProtectedDataIntoArrays() function call.
        // We can also select the first book item in the books list pCheckListOfBooks that is
        // protected.
        // The array with the config file's selection data is selectedWholeBookIDs.

        // The selectedWholeBookIDs is parallel with the pCheckListOfBooks control.
        // We need to go through the selectedWholeBookIDs, and for each array item that has a non-empty 
        // string in the form of a BookID, set the pCheckListOfBooks control for the item at that same 
        // index.
        int selectedBookIndex;
        int selectedBookTotal;
        wxString bkid;
        wxString chid;
        wxString tempStr;
        int nFirstBookIndex = -1;
        selectedBookTotal = (int)selectedWholeBookIDs.GetCount();
        for (selectedBookIndex = 0; selectedBookIndex < selectedBookTotal; selectedBookIndex++)
        {
            bkid = selectedWholeBookIDs.Item(selectedBookIndex);
            if (bkid != _T(""))
            {
                // Found a match. At this count index, we can record the first instance index
                // in nFirstBookIndex (for the SetSelection() call below), and tick the check 
                // box for the whole book.
                if (nFirstBookIndex == -1)
                {
                    nFirstBookIndex = selectedBookIndex;
                }
                // We found a BookID that should be ticked in the pCheckListOfBooks.
                pCheckListOfBooks->Check(selectedBookIndex);
            }
        }
        pCheckListOfBooks->SetSelection(nFirstBookIndex);
    }
    return nFirstBookIndex;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString composed of the protected chapters, if any, from the project's
///             m_TempCollabBooksProtectedFromSavingToEditor string for the selected
///             book.
/// \param      nBookSel -> an int representing the currently selected book in the Books List
/// \remarks
/// Called from: LoadChaptersListBox().
/// Parses the m_TempCollabBooksProtectedFromSavingToEditor string to get a string list of 
/// protected chapters, if any, in the form of a protected chapters string for the selected 
/// book. The function returns the protected chapter data in the form "1:3:4:" without the 
/// bookID, and ensures there is a colon at the end of the chapter data.
/// If there no chapter data for the nBookSel, this function returns an empty string.
///////////////////////////////////////////////////////////////////////////////
wxString CCollabProtectEditorBulkSettingsDlg::GetProtectedChsForBookSelection(int nBookSel)
{
    wxString returnStr = _T("");
    wxString tempStr = m_TempCollabBooksProtectedFromSavingToEditor;
    tempStr.Trim(FALSE);
    tempStr.Trim(TRUE);
    if (tempStr.IsEmpty())
    {
        return returnStr;
    }
    wxString chStr = _T("");
    if (!this->m_bTempCollabByChapterOnly)
    {
        // Whole book mode so we don't return any chapter data
        return returnStr;
    }
    else
    {
        wxString bookID;
        // Get the bookID represented by nBookSel in the vrsBookIDs array.
        bookID = vrsBookIDs.Item(nBookSel);
        // Parse the tempStr string and retrieve the protected chapters string for the bookID if
        // they exist. Since the tempStr (from m_TempCollabBooksProtectedFromSavingToEditor) may
        // have a combination of whole book IDs without chapter refs, as well as whole book IDs with 
        // chapter refs, it is best to use wxStringTokenizer if there is at least one space in the 
        // protected string to detect situations like "MRK MRK:1:2:3" etc, where there could be a
        // whole book and chapter only mix.
        chStr = tempStr;
        if (chStr.Find(_T(" ")) != wxNOT_FOUND)
        {
            // It has a space, so we can tokenize separate books, and check for a book:ch match
            wxString tokenStr;
            wxStringTokenizer tkz(chStr, _T(" ")); // books are delimited by spaces
            while (tkz.HasMoreTokens())
            {
                tokenStr = tkz.GetNextToken();
                tokenStr.Trim(TRUE);
                tokenStr.Trim(FALSE);
                if (tokenStr.Find(bookID) != wxNOT_FOUND && tokenStr.Find(_T(":")) != wxNOT_FOUND)
                {
                    // There is the bookID and a ':' in the token so we have a match
                    // Remove the BookID and intervening colon and just return the ch data
                    tokenStr = tokenStr.Mid(tokenStr.Find(bookID) + 4);
                    if (!tokenStr.IsEmpty() && tokenStr.GetChar(tokenStr.Length() - 1) != _T(':'))
                    {
                        tokenStr += _T(":"); // ensure the string ends with a colon
                    }
                    return tokenStr;
                }
            }
        }
        else
        {
            // The string has no spaces so we can check for the bookID and ':' in the string
            if (chStr.Find(bookID) != wxNOT_FOUND && chStr.Find(_T(":")) != wxNOT_FOUND)
            {
                // There is the bookID and a ':' in the token so we have a match
                // Remove the BookID and intervening colon and just return the ch data
                chStr = chStr.Mid(chStr.Find(bookID) + 4); // also removes colon following the bookID
                if (!chStr.IsEmpty() && chStr.GetChar(chStr.Length() - 1) != _T(':'))
                {
                    chStr += _T(":"); // ensure the string ends with a colon
                }
                return chStr;
            }
            else
            {
                // either the bookID is not present, or there is no colon present. There
                // could be simply a single whole book string like "MRK", which has no colon(s) 
                // in the string, but this isn't a chapter only situation so return an empty string.
                return wxEmptyString;
            }
        }
    }
    return returnStr;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		    a wxString composed of the chapter elements that currently selected
///                 in the pListOfChapters checklist box control.
/// \param (none)
/// \remarks
/// Called from: OnCheckLBToggleListChapters(), OnBtnAllChapters(), and OnBtnDeselectAllChapters().
/// This function gets a string in the form "1:2:5:7:" that represents the chapters that are
/// currently selected in the pListOfChapters checklist box control. This string can then be
/// stored in the selectedChs array index corresponding to the index of the currently selected 
/// book. 
///////////////////////////////////////////////////////////////////////////////
wxString CCollabProtectEditorBulkSettingsDlg::GetProtectedChsStringFromChListBoxSelections()
{
    wxString tempStr;
    wxString workStr;
    int ct;
    int tot = (int)pListOfChapters->GetCount();
    for (ct = 0; ct < tot; ct++)
    {
        if (pListOfChapters->IsChecked(ct))
        {
            workStr = pListOfChapters->GetString(ct);
            if (!workStr.IsEmpty())
            {
                // remove the _("Chapter") and space _T(" ") part from workstring. 
                // Since a localization for "Chapter" might have a space, we find the position of the space from the right end
                workStr = workStr.Mid(workStr.Find(_T(' '), TRUE) + 1);
                tempStr = tempStr + workStr + _T(":");
            }
        }
    }
    return tempStr;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		    a wxString composed of all whole books and/or book:ch elements that
///                 represent all protected books/chapters currently selected
/// \param (none)
/// \remarks
/// Called from: The CCollabProtectEditorBulkSettingsDlg::OnOK() handler.
/// This function examines the selected... arrays to compose a string in the bookID and/or bookID:ch:ch... 
/// form of "MAT MRK MRK:1:3 ACT ACT:1:2:5:7" that represents all protected whole books and/or book:ch 
/// instances that are currently selected in the pListOfBooks and/or pListOfChapters checklist box controls. 
/// This string is formatted to store in the project's config file in its 
/// CollabBooksProtectedFromSavingToEditor field. This function is the inverse of the
/// ParseProtectedDataIntoArrays() function. This function creates the formatted string to
/// save in the project config file, whereas the ParseProtectedDataIntoArrays() function
/// takes in an existing formatted string from the project config file and parses that string
/// into the selected... arrays for the initial state of this dialog.
///////////////////////////////////////////////////////////////////////////////
wxString CCollabProtectEditorBulkSettingsDlg::GetProtectedStringForAllBooksAndChaptersForSaving()
{
    // The data for building a string are contained in the three selected... arrays:
    //   selectedWholeBookIDs
    //   selectedBookIDs
    //   selectedChs
    // We need only process the array elements that are non-empty strings
    
    wxString workStr = _T("");
    int ct;
    int tot = (int)selectedWholeBookIDs.GetCount();
    for (ct = 0; ct < tot; ct++)
    {
        if (selectedWholeBookIDs.Item(ct) != wxEmptyString)
        {
            // There is a whole book item to add to the string
            if (!workStr.IsEmpty())
            {
                // workStr already has some content so we need an intervening space to delimin a new whole book
                workStr = workStr + _T(" ") + selectedWholeBookIDs.Item(ct);
            }
            else
            {
                // workStr is empty so we don't need a delimiting space
                workStr = selectedWholeBookIDs.Item(ct);
            }
        }
        // We process whole books and the corresponding book:ch data together in the worsStr
        if (selectedBookIDs.Item(ct) != wxEmptyString)
        {
            // There is a book ID that has protected chapter content stored at the same ct in selectedChs
            wxASSERT(selectedChs.Item(ct) != wxEmptyString);
            // the composite ch: string in selectedChs ends with a colon. For saving to the project config
            // file we don't want colons at the end of a composite ch:ch:ch: stringt
            wxString compositeChhStr = selectedChs.Item(ct);
            if (!compositeChhStr.IsEmpty() && compositeChhStr.Last() == _T(':'))
                compositeChhStr.Remove(compositeChhStr.Length() - 1);
            if (!workStr.IsEmpty())
            {
                // workStr already has some content so we need an intervening space to delimin a new book:ch item
                workStr = workStr + _T(" ") + selectedBookIDs.Item(ct) + _T(":") + compositeChhStr;
            }
            else
            {
                // workStr is empty so we don't need a delimiting space
                workStr = selectedBookIDs.Item(ct) + _T(":") + compositeChhStr;
            }
        }
    }
    return workStr;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		       the int index of the bookID in the vrsBookIDs array
/// \param bookCode -> a wxStirng being the bookID code as used in AllBookIds[] array
/// \remarks
/// Called from: ParseProtectedDataIntoArrays().
/// Finds the index of the bookCode in the vrsBookIDs array and returns it,
/// or -1 if the bookCode was not found.
///////////////////////////////////////////////////////////////////////////////
int CCollabProtectEditorBulkSettingsDlg::GetBookArrayIndexFromVrsBookID(wxString bookCode)
{
    wxString bkCode = bookCode.MakeUpper();
    int indx = -1;
    int arrayCt = (int)vrsBookIDs.GetCount();
    for (indx = 0; indx < arrayCt; indx++)
    {
        if (vrsBookIDs.Item(indx) == bkCode)
        {
            return indx;
        }
    }
    return indx;
}

///////////////////////////////////////////////////////////////////////////////
/// \return    TRUE if protected doc selection data has changed otherwise FALSE
/// \param     (none)
/// \remarks
/// Called from: OnOK(), OnCancel().
/// Compares the data in the selected... array to the data in the saveSelected...
/// arrays. If a difference is found it returns TRUE, otherwise returns FALSE
///////////////////////////////////////////////////////////////////////////////
bool CCollabProtectEditorBulkSettingsDlg::ProtectedDataHasChanged()
{
    // Use the three save... arrays and compare them with the parallel selected... arrays to 
    // determine if changes have been made in the dialog session. The three compareisons are:
    //    saveSelectedWholeBookIDs compared with selectedWholeBookIDs
    //    saveSelectedBookIDs compared with selectedBookIDs
    //    saveSelectedChs compared with selectedChs
    // Non-empty strings in the array indicate a protection state exists for that item.
    int ct;
    int tot = (int)saveSelectedWholeBookIDs.GetCount();
    bool bDataChanged = FALSE;
    for (ct = 0; ct < tot; ct++)
    {
        if (saveSelectedWholeBookIDs.Item(ct) != selectedWholeBookIDs.Item(ct))
        {
            bDataChanged = TRUE;
            break;
        }
        if (saveSelectedBookIDs.Item(ct) != selectedBookIDs.Item(ct) || saveSelectedChs.Item(ct) != selectedChs.Item(ct))
        {
            bDataChanged = TRUE;
            break;
        }
    }
    return bDataChanged;
}

/* // currently unused
///////////////////////////////////////////////////////////////////////////////
/// \return		       the int index of the bookName in the vrsBookIDsForListBox array
/// \param bookName -> a wxStirng being the English book name as used in AllBookNames[] array
/// \remarks
/// Called from:
/// Finds the index of the bookName in the vrsBookIDsForListBox array and returns it,
/// or -1 if the bookName was not found.
/// Note: The incoming bookName cannot be the decorated bookName that is taken from the
/// Books List wxListBox which has the bookID suffixed in parentheses, for example "Genesis (GEN)"
///////////////////////////////////////////////////////////////////////////////
int CCollabProtectEditorBulkSettingsDlg::GetBookArrayIndexFromVrsBookName(wxString bookName)
{
    wxString bkName = bookName;
    int indx = -1;
    int arrayCt = (int)vrsBookIDsForListBox.GetCount();
    for (indx = 0; indx < arrayCt; indx++)
    {
        if (vrsBookIDsForListBox.Item(indx) == bkName)
        {
            return indx;
        }
    }
    return indx;
}
*/

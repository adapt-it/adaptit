/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			MoveDialog.cpp
/// \author			Jonathan Field; modified by Bill Martin for the WX version
/// \date_created	10 November 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CMoveDialog class. 
/// The CMoveDialog class provides a dialog interface in which the user can
/// move documents from the Adaptations folder to the current book folder or
/// from the current book folder to the Adaptations folder. It also allows
/// the user to view the documents in the "other" folder and/or rename documents.
/// \derivation		The CMoveDialog class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in MoveDialog.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "MoveDialog.h"
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
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
#include "Adapt_It.h"
#include "helpers.h"
#include "Mover.h"
#include "ListDocumentsInOtherFolderDialog.h"
#include "MoveDialog.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

// JF Tue 5-Apr-2005 : Leftoff in spec at paragraph beginning "Checking should be done 
// before a document is moved from the Adaptations folder to a book folder; but no check 
// need be done to go from a book folder to the Adaptations folder ...".

// event handler table
BEGIN_EVENT_TABLE(CMoveDialog, AIModalDialog)
	EVT_INIT_DIALOG(CMoveDialog::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(IDC_MOVE_NOW, CMoveDialog::OnBnClickedMoveNow)
	EVT_BUTTON(IDC_BUTTON_RENAME_DOC, CMoveDialog::OnBnClickedButtonRenameDoc)
	EVT_BUTTON(IDC_VIEW_OTHER, CMoveDialog::OnBnClickedViewOther)
	EVT_RADIOBUTTON(IDC_RADIO_TO_BOOK_FOLDER, CMoveDialog::OnBnClickedRadioToBookFolder)
	EVT_RADIOBUTTON(IDC_RADIO_FROM_BOOK_FOLDER, CMoveDialog::OnBnClickedRadioFromBookFolder)
	EVT_BUTTON(wxID_OK, CMoveDialog::OnOK)
END_EVENT_TABLE()

CMoveDialog::CMoveDialog(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Move A Document"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	MoveDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
}

CMoveDialog::~CMoveDialog() // destructor
{
	
}

void CMoveDialog::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	pSourceFolderDocumentListBox = (wxListBox*)FindWindowById(IDC_LIST_SOURCE_FOLDER_DOCS);
	wxASSERT(pSourceFolderDocumentListBox != NULL);
	pToBookFolderRadioButton = (wxRadioButton*)FindWindowById(IDC_RADIO_TO_BOOK_FOLDER);
	wxASSERT(pToBookFolderRadioButton != NULL);
	pFromBookFolderRadioButton = (wxRadioButton*)FindWindowById(IDC_RADIO_FROM_BOOK_FOLDER);
	wxASSERT(pFromBookFolderRadioButton != NULL);
	pDocumentsInTheFolderLabel = (wxStaticText*)FindWindowById(IDC_STATIC_DOCS_IN_FOLDER);
	wxASSERT(pDocumentsInTheFolderLabel != NULL);

#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL,
				pSourceFolderDocumentListBox, NULL, gpApp->m_pDlgGlossFont, gpApp->m_bNavTextRTL);
#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL, 
				pSourceFolderDocumentListBox, NULL, gpApp->m_pDlgGlossFont);
#endif

	// Set default values and stuff.
	pToBookFolderRadioButton->SetValue(TRUE); //SetCheck(1);
	bFromBookFolder = false;
	MoveDirectionChanged();

	gpApp->RefreshStatusBarInfo();
}

// event handling functions

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CMoveDialog::OnOK(wxCommandEvent& event) 
{
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


// MoveDialog message handlers

void CMoveDialog::OnBnClickedMoveNow(wxCommandEvent& WXUNUSED(event))
{
	Mover m;
	int Result;
	wxString FileNameSansPath;
	bool MoveDeeper;

	if (pSourceFolderDocumentListBox->GetSelection() == -1)
	{
		wxMessageBox(_("Please select the file you wish to move."),_T(""), wxICON_WARNING); //IDS_SELECT_MOVE_FILE
		return;
	}

	// construct the long message string, in case it is needed
	wxString msg;
	wxString str;
	str = _("You currently have unsaved changes for the document you are trying to move.");
	msg = str;
	str = _("Do you want to save those changes now and move the file (in which case click Yes), or discard those changes and just move the last-saved version of the file (in which case click No)? To bail out, click Cancel.");//IDS_SAVE_DISCARD_BAILOUT
	msg += str;
	str = _("WARNING : If you click 'No', YOU WILL LOSE THE CURRENT UNSAVED CHANGES.");
	msg += str;

	FileNameSansPath = pSourceFolderDocumentListBox->GetString(pSourceFolderDocumentListBox->GetSelection());
	MoveDeeper = (pToBookFolderRadioButton->GetValue() == TRUE);
	m.BeginMove(FileNameSansPath, MoveDeeper);
	int nAnswer;
	do {
		Result = m.FinishMove();
		switch (Result) 
		{
			case DOCUMENTMOVER_SUCCESS :
				UpdateFileList();
				wxMessageBox(_("Moving the document was successful."),_T(""),wxICON_INFORMATION); //IDS_SUCCESSFUL_MOVE
				break;
			case DOCUMENTMOVER_USERINTERVENTIONREQUIRED_PROMPTSAVECHANGES :
				nAnswer = wxMessageBox(msg, _T(""), wxYES_NO | wxCANCEL);
				switch (nAnswer)
				{
					case wxYES: //case +1 :
						m.SaveChanges = true;
						break;
					case wxNO: //case -1 :
						m.DiscardChanges = true;
						break;
					case wxCANCEL: //0 :
						m.CancelMove();
						Result = 0;
						break;
				}
				break;
			case DOCUMENTMOVER_USERINTERVENTIONREQUIRED_PROMPTOVERWRITEEXISTINGFILE :
				nAnswer = wxMessageBox(_("A file with this name already exists in the destination folder.  Do you want to overwrite it?"), _T(""), wxYES | wxCANCEL);
				if (nAnswer == wxYES) //IDS_THIS_FILE_EXISTS_ALREADY
				{
					m.OverwriteExistingFile = true;
				} 
				else 
				{
					m.CancelMove();
					Result = 0;
				}
				break;
			case DOCUMENTMOVER_ERROR_BOOKVIOLATION :
				wxMessageBox(_("Sorry, the file you are trying to move belongs to a different book folder than the current one."),_T(""), wxICON_WARNING); //IDS_NOT_FOR_CURRENT_BOOK_FOLDER
				break;
			default :
				m.CancelMove();
				wxMessageBox(_("An unexpected error occured while trying to do the move.  The move has been aborted."),_T(""), wxICON_WARNING); //IDS_UNEXPECTED_MOVE_ERROR
				break;
		}

		gpApp->RefreshStatusBarInfo();

	} while (Result > 0);
}

void CMoveDialog::OnBnClickedButtonRenameDoc(wxCommandEvent& WXUNUSED(event))
{
	wxString FolderPath;
	wxString OldFileName;
	wxString NewFileName;

	if (pSourceFolderDocumentListBox->GetSelection() == -1) 
	{
		wxMessageBox(_("Please select the file you wish to rename."),_T(""),wxICON_WARNING); //IDS_SELECT_FILE_TO_RENAME
		return;
	}
	if (bFromBookFolder) 
	{
		FolderPath = gpApp->GetCurrentBookFolderPath();
	} else {
		FolderPath = gpApp->GetAdaptationsFolderPath();
	}
	OldFileName = pSourceFolderDocumentListBox->GetString(pSourceFolderDocumentListBox->GetSelection());
	bool SelectedFileIsOpen = gpApp->IsOpenDoc(FolderPath, OldFileName);
	bool OpenFileHasUnsavedChanges = gpApp->GetDocHasUnsavedChanges();
	bool SelectedFileIsOpenWithUnsavedChanges = SelectedFileIsOpen && OpenFileHasUnsavedChanges;

	// construct the long message string, in case it is needed
	wxString msg;
	wxString str;
	str = _("You currently have unsaved changes for the document you are trying to rename.  Do you want to save those changes or discard them?  ('Yes' to save; 'No' to discard.)  To bail out, click Cancel."); //bool bOK = str.LoadString(IDS_UNSAVED_CHANGES_IN_RENAME);
	msg = str;
	str = _("WARNING : If you click 'No', YOU WILL LOSE THE CURRENT UNSAVED CHANGES."); //IDS_WARN_WILL_LOSE_CHANGES
	msg += str;

	if (SelectedFileIsOpenWithUnsavedChanges) {
		int nAnswer = wxMessageBox(msg, _T(""), wxYES_NO | wxCANCEL);
		switch (nAnswer)
		{
			case wxYES: //case +1 :
				gpApp->SaveDocChanges();
				break;
			case wxNO: //case -1 :
				gpApp->DiscardDocChanges();
				break;
			case wxCANCEL: //case 0 :
				return;
		}
	}
	wxString caption;
	caption = _("Enter A New Document Name"); //IDS_DOC_NAME_CAPTION
	wxString prompt;
	prompt = _("Please enter the new name for the document: "); //IDS_DOC_NAME_PROMPT
	NewFileName = ::wxGetTextFromUser(prompt,caption);
	if (!NewFileName.IsEmpty())
	{
		// the user may have supplied just a name, which may contain a period or two, and it may not
		// have an extension, so we must check and supply an extension if needed.
		wxString noExtnName = gpApp->MakeExtensionlessName(NewFileName);

		// we'll now put on it what the extension should be, according to the doc
		// type we have elected to save
		wxString extn;
		// wx version only uses .xml extensions on documents
		//if (gpApp->m_bSaveAsXML)
			extn = _T(".xml");
		//else
		//	extn = _T(".adt");
		NewFileName = noExtnName + extn;

		// wx version note: The MFC version's MoveFile() cannot move files/directories across diferent
		// volumes. wx doesn't have a single equivalent of MoveFile, but we can use ::wxCopyFile and then
		// ::wxRemoveFile, as long as we don't need to move whole directories and contents which doesn't
		// appear to be the case for MoveDialog which is used to move Adapt It documents between the
		// Adaptations folder and the current book folder (all on the same volume). Here MoveFile() was
		// used in MFC version to rename the file, not move it to a different location.
		if (::wxRenameFile(ConcatenatePathBits(FolderPath, OldFileName), ConcatenatePathBits(FolderPath, NewFileName)))
		{
			
			// At this point, the rename has been successful.  Now tidy up.

			if (SelectedFileIsOpen) {
				gpApp->ChangeDocUnderlyingFileDetailsInPlace(gpApp->GetCurrentDocFolderPath(), NewFileName);
			}
			
			// Unfortunately, there doesn't seem to be any way to update the text of the one item 
			// in the listbox that has changed, so we have to refresh the entire listbox instead. 
			// Very not nice, especially since this loses the user's current scroll position and current 
			// selection in the list.
			UpdateFileList();

		} else 
		{
			wxMessageBox(_("Sorry, an unexpected error occured while trying to rename the document."),_T(""),wxICON_WARNING); //TellUser(IDS_UNEXPECTED_ERROR_WHEN_RENAMING);
		}
	}

	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL,
				pSourceFolderDocumentListBox, NULL, gpApp->m_pDlgGlossFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL, 
				pSourceFolderDocumentListBox, NULL, gpApp->m_pDlgGlossFont);
	#endif
	
	gpApp->RefreshStatusBarInfo();
}

void CMoveDialog::OnBnClickedViewOther(wxCommandEvent& WXUNUSED(event))
{
	CListDocumentsInOtherFolderDialog d(this);
	if (bFromBookFolder) {
		d.FolderDisplayName = gpApp->GetAdaptationsFolderDisplayName();
		d.FolderPath = gpApp->GetAdaptationsFolderPath();
	} else {
		d.FolderDisplayName = gpApp->GetCurrentBookDisplayName();
		d.FolderPath = gpApp->GetCurrentBookFolderPath();
	}
	d.ShowModal(); //d.DoModal();

	// the dlg font settings will be clobbered now for the parent dialog,
	// so we must re-establish them
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL,
					pSourceFolderDocumentListBox, NULL, gpApp->m_pDlgGlossFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL, 
					pSourceFolderDocumentListBox, NULL, gpApp->m_pDlgGlossFont);
	#endif
}

void CMoveDialog::OnBnClickedRadioToBookFolder(wxCommandEvent& WXUNUSED(event))
{
	if (bFromBookFolder) {
		bFromBookFolder = false;
		MoveDirectionChanged();
	}

	gpApp->RefreshStatusBarInfo();
}

void CMoveDialog::OnBnClickedRadioFromBookFolder(wxCommandEvent& WXUNUSED(event))
{
	if (!bFromBookFolder) {
		bFromBookFolder = true;
		MoveDirectionChanged();
	}
	
	gpApp->RefreshStatusBarInfo();
}

// Call this whenever a change between From Book Folder and To Book Folder occurs, 
// or when the dialog is initially opened.
// This function takes care of setting the label above the listbox, and filling the listbox.
void CMoveDialog::MoveDirectionChanged()
{
	wxString Message;
	wxString s;
	wxString FolderName;

	// Set the label above the listbox.
	s = _("Documents in the folder %s"); //IDS_DOCS_IN_FOLDER
	if (bFromBookFolder) {
		FolderName = gpApp->GetCurrentBookDisplayName();
	} else {
		FolderName = gpApp->GetAdaptationsFolderDisplayName();
	}
	Message = Message.Format(s, FolderName.c_str());
	this->pDocumentsInTheFolderLabel->SetLabel(Message);

	// Fill the listbox.
	UpdateFileList();

}

void CMoveDialog::UpdateFileList()
{
	wxString FolderPath;

	if (bFromBookFolder) {
		FolderPath = gpApp->GetCurrentBookFolderPath();
	} else {
		FolderPath = gpApp->GetAdaptationsFolderPath();
	}
	//this->FillListBoxWithListOfFilesInPath(pSourceFolderDocumentListBox, FolderPath);
	// whm note: Code below from JF's DialogBase::FillListBoxWithListOfFilesInPath(CListBox& lb, CString Path, BOOL bShowAllFiles)
	// In wx version we're not implementing JF's DialogBase class, but using the code from its function here.
	//wxString FolderPathAndFilespecWildcards = FolderPath + PathSeparator + _T("*.*");
	pSourceFolderDocumentListBox->Clear(); // clear out all entries from the listbox
	wxArrayString files;
	// enumerate the document files in the Adaptations folder or the current book folder; and
	// note that internally GetPossibleAdaptionDocuments excludes any files with names of the
	// form *.BAK.xml (these are backup XML document files, and for each there will be present
	// an *.xml file which has identical content -- it is the latter we enumerate) and also note
	// the result could be an empty m_acceptedFilesList, but have the caller of EnumerateDocFiles
	// check it for no entries in the list
	gpApp->GetPossibleAdaptionDocuments(&files, FolderPath);
	wxString nextDoc;
	int ct;
	for (ct = 0; ct < (int)files.GetCount(); ct++)
	{
		nextDoc = files.Item(ct);
		pSourceFolderDocumentListBox->Append(nextDoc);
	}

}


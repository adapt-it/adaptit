/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			JoinDialog.cpp
/// \author			Jonathan Field; modified by Bill Martin for the WX version
/// \date_created	10 November 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CJoinDialog class. 
/// The CJoinDialog class provides a dialog interface for the user to be able
/// to combine Adapt It documents into larger documents.
/// \derivation		The CJoinDialog class is derived from AIModalDialog.
/// BEW 12Apr10, all changes for supporting doc version 5 are done for this file
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "JoinDialog.h"
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
#include "JoinDialog.h"
#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"
#include "helpers.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

extern bool gbDoingSplitOrJoin;


// event handler table
BEGIN_EVENT_TABLE(CJoinDialog, AIModalDialog)
	EVT_INIT_DIALOG(CJoinDialog::InitDialog)
	EVT_BUTTON(wxID_OK, CJoinDialog::OnOK)
	EVT_BUTTON(ID_JOIN_NOW, CJoinDialog::OnBnClickedJoinNow)
	EVT_BUTTON(IDC_BUTTON_MOVE_ALL_LEFT, CJoinDialog::OnBnClickedButtonMoveAllLeft)
	EVT_BUTTON(IDC_BUTTON_MOVE_ALL_RIGHT, CJoinDialog::OnBnClickedButtonMoveAllRight)
	EVT_BUTTON(IDC_BUTTON_ACCEPT, CJoinDialog::OnBnClickedButtonAccept)
	EVT_BUTTON(IDC_BUTTON_REJECT, CJoinDialog::OnBnClickedButtonReject)
	EVT_LISTBOX_DCLICK(IDC_LIST_ACCEPTED, CJoinDialog::OnLbnDblclkListAccepted)
	EVT_LISTBOX_DCLICK(IDC_LIST_REJECTED, CJoinDialog::OnLbnDblclkListRejected)
	EVT_LISTBOX(IDC_LIST_ACCEPTED, CJoinDialog::OnLbnSelchangeListAccepted)
	EVT_LISTBOX(IDC_LIST_REJECTED, CJoinDialog::OnLbnSelchangeListRejected)
	EVT_BUTTON(IDC_BUTTON_MOVE_DOWN, CJoinDialog::OnBnClickedButtonMoveDown)
	EVT_BUTTON(IDC_BUTTON_MOVE_UP, CJoinDialog::OnBnClickedButtonMoveUp)
END_EVENT_TABLE()

CJoinDialog::CJoinDialog(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Join Documents By Appending To The Open Document"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	JoinDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
}

CJoinDialog::~CJoinDialog() // destructor
{
	
}

void CJoinDialog::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	
	pNewFileName = (wxTextCtrl*)FindWindowById(IDC_EDIT_NEW_FILENAME);
	wxASSERT(pNewFileName != NULL);
	pAcceptedFiles = (wxListBox*)FindWindowById(IDC_LIST_ACCEPTED);
	wxASSERT(pAcceptedFiles != NULL);
	pRejectedFiles = (wxListBox*)FindWindowById(IDC_LIST_REJECTED);
	wxASSERT(pRejectedFiles != NULL);
	pMoveAllRight = (wxButton*)FindWindowById(IDC_BUTTON_MOVE_ALL_RIGHT);
	wxASSERT(pMoveAllRight != NULL);
	pMoveAllLeft = (wxButton*)FindWindowById(IDC_BUTTON_MOVE_ALL_LEFT);
	wxASSERT(pMoveAllLeft != NULL);
	pJoinNow = (wxButton*)FindWindowById(ID_JOIN_NOW);
	wxASSERT(pJoinNow != NULL);
	pClose = (wxButton*)FindWindowById(wxID_OK);
	wxASSERT(pClose != NULL);
	pMoveUp = (wxButton*)FindWindowById(IDC_BUTTON_MOVE_UP);
	wxASSERT(pMoveUp != NULL);
	pMoveDown = (wxButton*)FindWindowById(IDC_BUTTON_MOVE_DOWN);
	wxASSERT(pMoveDown != NULL);
	pJoiningWait = (wxStaticText*)FindWindowById(IDC_STATIC_JOINING_WAIT);
	wxASSERT(pJoiningWait != NULL);
	pReject = (wxBitmapButton*)FindWindowById(IDC_BUTTON_REJECT);
	wxASSERT(pReject != NULL);
	pAccept = (wxBitmapButton*)FindWindowById(IDC_BUTTON_ACCEPT);
	wxASSERT(pAccept != NULL);

	wxTextCtrl* pTextCtrlAsStaticJoin1 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_JOIN1);
	wxASSERT(pTextCtrlAsStaticJoin1 != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	pTextCtrlAsStaticJoin1->SetBackgroundColour(backgrndColor);

	wxTextCtrl* pTextCtrlAsStaticJoin2 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_JOIN2);
	wxASSERT(pTextCtrlAsStaticJoin2 != NULL);
	pTextCtrlAsStaticJoin2->SetBackgroundColour(backgrndColor);

	wxTextCtrl* pTextCtrlAsStaticJoin3 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_JOIN3);
	wxASSERT(pTextCtrlAsStaticJoin3 != NULL);
	pTextCtrlAsStaticJoin3->SetBackgroundColour(backgrndColor);

	wxTextCtrl* pTextCtrlAsStaticJoin4 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_JOIN4);
	wxASSERT(pTextCtrlAsStaticJoin4 != NULL);
	pTextCtrlAsStaticJoin4->SetBackgroundColour(backgrndColor);

	//CAdapt_ItApp* pApp;
	//pApp = (CAdapt_ItApp*)&wxGetApp();
	//wxASSERT(pApp != NULL);

	this->pJoiningWait->Show(FALSE);

	// make the font show the user's desired dialog font point size
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, pNewFileName, NULL,
					pAcceptedFiles, pRejectedFiles, gpApp->m_pDlgGlossFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, pNewFileName, NULL, 
					pAcceptedFiles, pRejectedFiles, gpApp->m_pDlgGlossFont);
	#endif

	InitialiseLists();

	// get the book ID code from the currently open document (in the case of book folders
	// mode being on currently, it is gotten instead from the book folder's struct, an
	// BookNamePair pointer, pertaining to the currently active Bible book folder). We
	// use this returned ID as follows: if the document to be joined has a book ID different
	// than the currently open document, then the join is aborted and a message shown to the
	// user; but if they match, then the CSourcePhrase instance which stores the ID in the
	// document to be appended (it will be the first in the list) is automatically removed
	// before the join is done -- because there can be only one \id marker per document
	bookID = gpApp->GetBookID();

	// select the top item as default
	int index = 0;
	if (pAcceptedFiles->GetCount() > 0)
	{
		pAcceptedFiles->SetSelection(index);
		ListContentsOrSelectionChanged();
	}

	gpApp->RefreshStatusBarInfo();

}

// event handling functions

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CJoinDialog::OnOK(wxCommandEvent& event) 
{
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


void CJoinDialog::OnBnClickedJoinNow(wxCommandEvent& WXUNUSED(event))
{
	gpApp->LogUserAction(_T("Initiated OnBnClockedJoinNow()"));
	// BEW added 23Jun07
	gbDoingSplitOrJoin = TRUE; // use to suppress document backups generation during the joining

	bool MergeIntoCurrentDoc;
	wxString MergeFileName;
	wxString MergeFilePath;
	wxString OldCurrentDocumentFileName;
	int i;
	wxString FileName;
	wxString FilePath;

	if (pAcceptedFiles->GetCount() == 0) 
	{
		wxMessageBox(_("There are no files listed for joining to the current document."),_T(""),wxICON_EXCLAMATION | wxOK); //TellUser(IDS_NONE_FOR_JOINING);
		gbDoingSplitOrJoin = FALSE; // restore default
		return;
	}

	// Show the "Joining... Please wait." message
	this->pJoiningWait->Show(TRUE);
	this->pJoiningWait->Update();


#ifdef __WXDEBUG__
	// check the reordering user does gets reflected in the AcceptedFiles list (it does)
	int cnt = pAcceptedFiles->GetCount();
	int ii;
	wxString s;
	for (ii=0;ii<cnt;ii++)
	{
		s = pAcceptedFiles->GetString(ii);
	}
#endif

	OldCurrentDocumentFileName = gpApp->GetCurrentDocFileName();

	// Determine merge destination filename.
	MergeFileName = pNewFileName->GetValue();
	MergeFileName.Trim(FALSE); // trim left end
	MergeFileName.Trim(TRUE); // trim right end
	MergeIntoCurrentDoc = MergeFileName.IsEmpty();
	if (!MergeIntoCurrentDoc) 
	{
		MergeFileName = CAdapt_ItApp::ApplyDefaultDocFileExtension(MergeFileName);
		if (MergeFileName.CmpNoCase(OldCurrentDocumentFileName) == 0) 
		{
			MergeIntoCurrentDoc = true;
			MergeFileName = OldCurrentDocumentFileName;
		}
	}
	if (MergeIntoCurrentDoc) 
	{
		MergeFileName = OldCurrentDocumentFileName;
	}
	MergeFilePath = ConcatenatePathBits(gpApp->GetCurrentDocFolderPath(), MergeFileName);
	
	// Ensure merge destination filename is valid.
	if (!MergeIntoCurrentDoc) 
	{
		if (!IsValidFileName(MergeFileName)) 
		{
			wxMessageBox(_("The new file name you supplied contains characters that are not permitted in a file name. Please edit the file name."),_T(""),wxICON_EXCLAMATION | wxOK); //TellUser(IDS_BAD_FNAME_FOR_JOIN);
			this->pJoiningWait->Show(FALSE);
			gbDoingSplitOrJoin = FALSE; // restore default
			return;
		}
	}

	// Ensure merge destination filename does not exist, or that if it does, 
	// it is one of the merge source filenames.
	if (!MergeIntoCurrentDoc) {
		bool CantOverwrite = false;
		if (FileExists(MergeFilePath)) {
			CantOverwrite = true;
			for (i = 0; i < (int)pAcceptedFiles->GetCount(); ++i) 
			{
				FileName = pAcceptedFiles->GetString(i);
				if (FileName.CmpNoCase(MergeFileName) == 0) 
				{
					CantOverwrite = false;
					break;
				}
			}
			if (CantOverwrite) 
			{
				wxMessageBox(_("The resulting document's file name must be unique or must be the same as one of the input files."),_T(""),wxICON_EXCLAMATION | wxOK); //TellUser(IDS_UNIQUE_OR_SAME_FOR_JOIN);
				this->pJoiningWait->Show(FALSE);
				gbDoingSplitOrJoin = FALSE; // restore default
				return;
			}
		}
	}

	// In this loop we do the actual joining.
	bool bNoIDMismatch = TRUE;
	for (i = 0; i < (int)pAcceptedFiles->GetCount(); ++i) {

		// General algorithm: (BEW modified 07Nov05) added bookID to signature
		//   For each source document:
		//     Read the source phrase list and append that list to the current source phrase list;
		//		but abort the joining operation if the bookID code does not match the bookID passed in
		//		for the current source phrase list.

		FileName = pAcceptedFiles->GetString(i);
		FilePath = ConcatenatePathBits(gpApp->GetCurrentDocFolderPath(), FileName);

		SPList* ol = gpApp->LoadSourcePhraseListFromFile(FilePath);
		bNoIDMismatch = gpApp->AppendSourcePhrasesToCurrentDoc(ol, bookID, i == (int)pAcceptedFiles->GetCount() - 1);
		if (!bNoIDMismatch)
		{
			// tell the user of the book ID mismatch, and then abort the joining of this and subsequent documents,
			// if any -- the user message should indicate which doc (using FileName) is the offending one
			wxString msg;
			//IDS_MISMATCHED_BOOK_IDS
			msg = msg.Format(_("The book ID in the document with filename %s does not match the book ID for the currently open document."),FileName.c_str());
			wxMessageBox(msg,_T(""),wxICON_EXCLAMATION | wxOK);
			break;
		}
		ol->Clear();
		if (ol != NULL) // whm 11Jun12 added NULL test
			delete ol;

	}
	// set up safe indices range for the bundle; refresh the document view if the parameter passed in
	// is true, skip the update if false is passed in
	gpApp->CascadeSourcePhraseListChange(false);
	
	// Change underlying filename if applicable.
	if (!MergeIntoCurrentDoc) {
		gpApp->ChangeDocUnderlyingFileDetailsInPlace(gpApp->GetCurrentDocFolderPath(), MergeFileName);
	}

	// Save.
	gpApp->SaveDocChanges();

	// Delete source files, except current document, although even delete the old current document if 
	// we've changed filename and thus have a new current document.
	// POTENTIAL IMPROVEMENT : It would be good, when deleting a file, we also delete any associated MRU 
	// ("most recently used files") entry.
	// BEW note 08Nov05: I thought of doing that, but it would reduce the functionality too much. Adapt It
	// currently can open a file, even when the file is an XML one, from a currently non-open project via
	// the MRU list, and also get the project switched over silently in doing so. If we were to delete
	// MRU listed files, we really would want to only have to examine the current project's folders to do so;
	// otherwise checking the file is not in any folder in any project as a condition for deletion would
	// be rather timeconsuming and minimally useful. Instead, if the file does not exist (and is an XML
	// one), I've coded so that the user is told the file probably no longer exists and the Start Working
	// wizard gets automatically opened to enable him to get into a project and get a file open that way.
	if (bNoIDMismatch)
	{
		// no book ID mismatch occurred, so all the files in the list were processed
		for (i = 0; i < (int)pAcceptedFiles->GetCount(); ++i) 
		{
			FileName = pAcceptedFiles->GetString(i);
			if (FileName.CmpNoCase(MergeFileName) != 0) 
			{
				::wxRemoveFile(ConcatenatePathBits(gpApp->GetCurrentDocFolderPath(), FileName));
			}
		}
		if (OldCurrentDocumentFileName.CmpNoCase(MergeFileName) != 0) 
		{
			::wxRemoveFile(ConcatenatePathBits(gpApp->GetCurrentDocFolderPath(), OldCurrentDocumentFileName));
		}
	}
	else
	{
		// a book ID mismatch was detected, so not all the files in the list were processed. The i index
		// value is was left equal to the index of the first filename in the list which was not processed,
		// so only delete the files which were processed.
		int j;
		for (j = 0; j < i; ++j) 
		{
			FileName = pAcceptedFiles->GetString(j);
			if (FileName.CmpNoCase(MergeFileName) != 0) 
			{
				::wxRemoveFile(ConcatenatePathBits(gpApp->GetCurrentDocFolderPath(), FileName));
			}
		}
		if (OldCurrentDocumentFileName.CmpNoCase(MergeFileName) != 0) 
		{
			::wxRemoveFile(ConcatenatePathBits(gpApp->GetCurrentDocFolderPath(), OldCurrentDocumentFileName));
		}
	}

	// Trigger a re-layout/re-render of the current document.
	gpApp->CascadeSourcePhraseListChange(true);

	InitialiseLists();

	this->pJoiningWait->Show(FALSE);

	// update Title bar (BEW added 14Aug09)
	CAdapt_ItDoc* d = gpApp->GetDocument();
	wxString strUserTyped = gpApp->m_curOutputFilename;
	d->SetDocumentWindowTitle(strUserTyped, strUserTyped);

	if (bNoIDMismatch)
	{
		wxMessageBox(_("Joining to the current document was successful."),_T(""),wxICON_INFORMATION | wxOK);// IDS_JOIN_SUCCESSFUL
		gpApp->LogUserAction(_T("Joining to the current document was successful."));
	}
	else
	{
		wxMessageBox(_("Joining documents exited prematurely because of a mismatched book ID code."),_T(""),wxICON_EXCLAMATION | wxOK); //IDS_BAD_JOIN_FROM_MISMATCH
		gpApp->LogUserAction(_T("Joining documents exited prematurely because of a mismatched book ID code."));
	}

	gpApp->RefreshStatusBarInfo();
	gbDoingSplitOrJoin = FALSE; // restore default so document backups can happen again
}

void CJoinDialog::OnBnClickedButtonMoveAllLeft(wxCommandEvent& WXUNUSED(event))
{
	MoveAllItems(*pRejectedFiles, *pAcceptedFiles);
	ListContentsOrSelectionChanged();
}

void CJoinDialog::OnBnClickedButtonMoveAllRight(wxCommandEvent& WXUNUSED(event))
{
	MoveAllItems(*pAcceptedFiles, *pRejectedFiles);
	ListContentsOrSelectionChanged();
}

void CJoinDialog::OnBnClickedButtonAccept(wxCommandEvent& WXUNUSED(event))
{
	MoveSelectedItems(*pRejectedFiles, *pAcceptedFiles);
	ListContentsOrSelectionChanged();
}

void CJoinDialog::OnBnClickedButtonReject(wxCommandEvent& WXUNUSED(event))
{
	MoveSelectedItems(*pAcceptedFiles, *pRejectedFiles);
	ListContentsOrSelectionChanged();
}

void CJoinDialog::OnLbnDblclkListAccepted(wxCommandEvent& WXUNUSED(event))
{
	MoveSelectedItems(*pAcceptedFiles, *pRejectedFiles);
	ListContentsOrSelectionChanged();
}

void CJoinDialog::OnLbnDblclkListRejected(wxCommandEvent& WXUNUSED(event))
{
	MoveSelectedItems(*pRejectedFiles, *pAcceptedFiles);
	ListContentsOrSelectionChanged();
}

void CJoinDialog::InitialiseLists()
{
	int i;
	wxString FileName;
	wxString CurrentDocFileName;

	// BEW modified 02Nov05, to only show document files (the FALSE parameter effects this)
	//FillListBoxWithListOfFilesInPath(pAcceptedFiles, gpApp->GetCurrentDocFolderPath(), FALSE);
	// whm note: Code below from JF's DialogBase::FillListBoxWithListOfFilesInPath(CListBox& lb, CString Path, BOOL bShowAllFiles)
	// In wx version we're not implementing JF's DialogBase class, but using the code from its function here.
	pAcceptedFiles->Clear(); // clear out all entries from the listbox
	wxArrayString files;
	// enumerate the document files in the Adaptations folder or the current book folder; and
	// note that internally GetPossibleAdaptionDocuments excludes any files with names of the
	// form *.BAK (these are backup XML document files, and for each there will be present
	// an *.xml file which has identical content -- it is the latter we enumerate) and also note
	// the result could be an empty m_acceptedFilesList, but have the caller of EnumerateDocFiles
	// check it for no entries in the list
	gpApp->GetPossibleAdaptionDocuments(&files, gpApp->GetCurrentDocFolderPath());
	wxString nextDoc;
	int ct;
	for (ct = 0; ct < (int)files.GetCount(); ct++)
	{
		nextDoc = files.Item(ct);
		pAcceptedFiles->Append(nextDoc);
	}
	
	if (true) 
	{ // Remove the current document from the file list.
		CurrentDocFileName = gpApp->GetCurrentDocFileName();
		for (i = 0; i < (int)pAcceptedFiles->GetCount(); ++i) {
			FileName = pAcceptedFiles->GetString(i);
			if (FileName.CmpNoCase(CurrentDocFileName) == 0) {
				pAcceptedFiles->Delete(i);
				break;
			}
		}
	}
	pRejectedFiles->Clear();

	ListContentsOrSelectionChanged();
}

void CJoinDialog::ListContentsOrSelectionChanged()
{
	if ((int)pAcceptedFiles->GetCount() > 0) 
	{
		if (pAcceptedFiles->GetSelection() >= 0)
			pReject->Enable(TRUE);
		else
			pReject->Enable(FALSE);
		pMoveAllRight->Enable(TRUE);
		pJoinNow->Enable(TRUE);
		pMoveUp->Enable(TRUE);
		pMoveDown->Enable(TRUE);
	} 
	else 
	{
		pReject->Disable();
		pMoveAllRight->Disable();
		pJoinNow->Disable();
		pMoveUp->Disable();
		pMoveDown->Disable();
	}
	if ((int)pRejectedFiles->GetCount() > 0) 
	{
		if (pRejectedFiles->GetSelection() >= 0)
			pAccept->Enable(TRUE);
		else
			pAccept->Enable(FALSE);
		pMoveAllLeft->Enable(TRUE);
	} 
	else 
	{
		pAccept->Disable();
		pMoveAllLeft->Disable();
	}
}

void CJoinDialog::OnLbnSelchangeListAccepted(wxCommandEvent& WXUNUSED(event))
{
	ListContentsOrSelectionChanged();
}

void CJoinDialog::OnLbnSelchangeListRejected(wxCommandEvent& WXUNUSED(event))
{
	ListContentsOrSelectionChanged();
}

void CJoinDialog::OnBnClickedButtonMoveDown(wxCommandEvent& WXUNUSED(event))
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pAcceptedFiles))
	{
		wxMessageBox(_("Error when getting the current selection, Move Down button has been ignored"), 
						_T(""),wxICON_EXCLAMATION | wxOK);
		return;
	}
	int nSel;
	nSel = pAcceptedFiles->GetSelection();
	//if (nSel == -1) // LB_ERR
	//{
	//	wxMessageBox(_("Error when getting the current selection, Move Down button has been ignored"), 
	//					_T(""),wxICON_EXCLAMATION | wxOK);
	//	return;
	//}
	int nOldSel = nSel; // save old selection index

	// change the order of the string in the list box
	int count = pAcceptedFiles->GetCount();
	wxASSERT(nSel < count);
	if (nSel < count-1)
	{
		nSel++;
		wxString tempStr;
		tempStr = pAcceptedFiles->GetString(nOldSel);
		pAcceptedFiles->Delete(nOldSel);
		pAcceptedFiles->Insert(tempStr,nSel);
		pAcceptedFiles->SetSelection(nSel);
	}
	else
	{
		return; // impossible to move the list element further down!
	}
	TransferDataToWindow();
}

void CJoinDialog::OnBnClickedButtonMoveUp(wxCommandEvent& WXUNUSED(event))
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pAcceptedFiles))
	{
		wxMessageBox(_("Error when getting the current selection, Move Up button has been ignored"),
						_T(""), wxICON_EXCLAMATION | wxOK);
		return;
	}
	int nSel;
	nSel = pAcceptedFiles->GetSelection();
	//if (nSel == -1) // LB_ERR
	//{
	//	wxMessageBox(_("Error when getting the current selection, Move Up button has been ignored"),
	//					_T(""), wxICON_EXCLAMATION | wxOK);
	//	return;
	//}
	int nOldSel = nSel; // save old selection index

	// change the order of the string in the list box
	int count;
	count = pAcceptedFiles->GetCount();
	wxASSERT(nSel < count);
	count = count; // avoid warning
	if (nSel > 0)
	{
		nSel--;
		wxString tempStr;
		tempStr = pAcceptedFiles->GetString(nOldSel);
		pAcceptedFiles->Delete(nOldSel);
		pAcceptedFiles->Insert(tempStr,nSel);
		//wxASSERT(nLocation != LB_ERR);
		pAcceptedFiles->SetSelection(nSel);
	}
	else
	{
		return; // impossible to move up the first element in the list!
	}
	TransferDataToWindow();
}

void CJoinDialog::MoveSelectedItems(wxListBox& From, wxListBox& To)
{
	int i;
	wxString s;

	if (From.GetSelection() == -1) 
	{
		wxMessageBox(_("Nothing selected."),_T(""),wxICON_EXCLAMATION | wxOK);
	}
	for (i = 0; i < (int)From.GetCount(); ++i) 
	{
		if (From.IsSelected(i))
		{
			s = From.GetString(i);
			To.Append(s);
		}
	}
	for (i = (int)From.GetCount() - 1; i >= 0; --i) 
	{
		if (From.IsSelected(i)) 
		{
			From.Delete(i);
		}
	}
}

void CJoinDialog::MoveAllItems(wxListBox& From, wxListBox& To)
{
	int i;
	wxString s;

	for (i = 0; i < (int)From.GetCount(); ++i) 
	{
		s = From.GetString(i);
		To.Append(s);
	}
	From.Clear();
}


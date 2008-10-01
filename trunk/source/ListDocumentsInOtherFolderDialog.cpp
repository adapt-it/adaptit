/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			ListDocumentsInOtherFolderDialog.cpp
/// \author			Bill Martin
/// \date_created	10 November 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the CListDocumentsInOtherFolderDialog class. 
/// The CListDocumentsInOtherFolderDialog class is called from the CMoveDialog class to provide
/// a sorted list of documents in a different folder.
/// \derivation		The CListDocumentsInOtherFolderDialog class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ListDocumentsInOtherFolderDialog.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ListDocumentsInOtherFolderDialog.h"
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
#include "ListDocumentsInOtherFolderDialog.h"
#include "helpers.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

// event handler table
BEGIN_EVENT_TABLE(CListDocumentsInOtherFolderDialog, AIModalDialog)
	EVT_INIT_DIALOG(CListDocumentsInOtherFolderDialog::InitDialog)// not strictly necessary for dialogs based on wxDialog
END_EVENT_TABLE()

CListDocumentsInOtherFolderDialog::CListDocumentsInOtherFolderDialog(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Documents In Other Folder"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	ListDocInOtherFolderDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	pLabel = (wxStaticText*)FindWindowById(IDC_STATIC_DOCS_IN_FOLDER);
	wxASSERT(pLabel != NULL);
	pListBox = (wxListBox*)FindWindowById(IDC_LIST_SOURCE_FOLDER_DOCS);
	wxASSERT(pListBox != NULL);
}

CListDocumentsInOtherFolderDialog::~CListDocumentsInOtherFolderDialog() // destructor
{
	
}

void CListDocumentsInOtherFolderDialog::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	wxString Message;
	wxString s;

	// make the font show user's desired point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL,
								pListBox, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL, 
								pListBox, NULL, gpApp->m_pDlgSrcFont);
	#endif

	// Set the label above the listbox.
	s = _("Documents in the folder %s"); //.LoadString(IDS_DOCS_IN_FOLDER);
	// wx note: FolderPath and FolderDiaplayName are set in the caller (CMoveDialog) before
	// dialog is shown
	Message = Message.Format(s, FolderDisplayName.c_str());
	pLabel->SetLabel(Message);

	// Fill the listbox.
	wxArrayString docs;
	pListBox->Clear();
	// enumerate the document files in the Adaptations folder or the current book folder; and
	// note that internally GetPossibleAdaptionDocuments excludes any files with names of the
	// form *.BAK.xml (these are backup XML document files, and for each there will be present
	// an *.xml file which has identical content -- it is the latter we enumerate) and also note
	// the result could be an empty m_acceptedFilesList, but have the caller of EnumerateDocFiles
	// check it for no entries in the list
	gpApp->GetPossibleAdaptionDocuments(&docs, FolderPath);
	wxString nextDoc;
	int ct,nTot;
	nTot = docs.GetCount();
	for (ct = 0; ct < nTot; ct++)
	{
		nextDoc = docs.Item(ct);
		pListBox->Append(nextDoc);
	}

	// make the list boxes scrollable
	// wx version horiz scroll set in resources by wxDesigner (only for Win32)
}

// event handling functions

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CListDocumentsInOtherFolderDialog::OnOK(wxCommandEvent& event) 
{
	
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods


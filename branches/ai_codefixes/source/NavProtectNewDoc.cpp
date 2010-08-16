/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			NavProtectNewDoc.cpp
/// \author			Bruce Waters
/// \date_created	9 August 2010
/// \date_revised
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the NavProtectNewDoc class.
/// The NavProtectNewDoc class provides a dialog interface for the user to create a New
/// Document from a list of loadable source text (typically USFM marked up) plain text
/// files, obtained from a folder called 'Source Data' which is a child of the currently
/// open project folder. (If this Source Data folder exists, and has at least one loadable
/// file in it - where loadable is defined as "returning TRUE from being tested by the
/// helpers.cpp function IsLoadableFile()", then the application auto-configures to only
/// show the user files from this folder for which no document of the same name has been
/// created. This class is in support of the "Navigation Protection" feature, to hide from
/// the user the ability to do file and/or folder navigation using the standard file input
/// browser which is shown when <New Document> is clicked, or the File / New... command is
/// invoked, in legacy Adapt It versions. The latter is still what happens if the Source
/// Data folder does not exists, or it exists but the filter tests above return a FALSE
/// result.
/// \derivation		The NavProtectNewDoc class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "NavProtectNewDoc.h"
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
#include <wx/filename.h>
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
#include "Adapt_It.h"
#include "helpers.h" // it has the Get... functions for getting list of files, folders
					 // and optionally sorting
#include "Adapt_It_wdr.h" // needed for the AIMainFrameIcons(index) function which returns
						  // for index values 10 and 11 the folder icon and file icon
						  // respectively, which we use in the two wxListCtrl instances to
						  // distinguish folder names from filenames; the function returns
						  // the relevant wxBitmap*
#include "NavProtectNewDoc.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

// Event handler table (The first version of this code called the left and right panes the
// source and destination panes, respectively. I've switched to 'left' and 'right', in the
// code, but I've left the wxDesigner resource labels unchanged, and they have the words
// SOURCE and DESTINATION within them -- so just read them as LEFT and RIGHT, respectively,
// and it will make sense)
BEGIN_EVENT_TABLE(NavProtectNewDoc, AIModalDialog)

	EVT_INIT_DIALOG(NavProtectNewDoc::InitDialog)
	EVT_BUTTON(wxID_OK, NavProtectNewDoc::OnInputFileButton)	
	EVT_BUTTON(wxID_CANCEL, NavProtectNewDoc::OnBnClickedCancel)
	EVT_LISTBOX(ID_LISTBOX_LOADABLES_FILENAMES, NavProtectNewDoc::OnItemSelected)
	EVT_LISTBOX_DCLICK(ID_LISTBOX_LOADABLES_FILENAMES, NavProtectNewDoc::OnDoubleClick)

END_EVENT_TABLE()

NavProtectNewDoc::NavProtectNewDoc(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Input Text File For Adaptation"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // This dialog function below is generated in wxDesigner, and defines the controls and
    // sizers for the dialog. The first parameter is the parent which should normally be
    // "this".
    // The second and third parameters should both be TRUE to utilize the sizers and create
    // the right size dialog.
	NewDocFromSourceDataFolderFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(m_pApp != NULL);

}

NavProtectNewDoc::~NavProtectNewDoc() // destructor
{
}

///////////////////////////////////////////////////////////////////////////////////
///
///    START OF GUI FUNCTIONS 
///
///////////////////////////////////////////////////////////////////////////////////

// InitDialog is method of wxWindow
void NavProtectNewDoc::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_pInputFileButton = (wxButton*)FindWindowById(wxID_OK);
	m_pCancelButton = (wxButton*)FindWindowById(wxID_CANCEL);
	m_pMonoclineListOfFiles = (wxListBox*)FindWindowById(ID_LISTBOX_LOADABLES_FILENAMES);
	m_pTopMessageStaticCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_TOP_MSG); // multiline, read-only
	m_pInstructionsStaticCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_INSTRUCTIONS); // multiline, read-only

	wxString topmsg = _(
"Please note: because this project owns a 'Source Data' folder, Adapt It is limited to creating new documents from the files stored in that folder only.\nAny loadable file with a filename the same as an existing document filename is not shown in the list.\n(If you first use File / Save As... to rename the document, then the file from which it was created will appear again in the list - allowing you to create a second document from that source text file. But normally you should never need to do so.)");
	m_pTopMessageStaticCtrl->ChangeValue(topmsg);
	wxString leftmsg = _(
"Instructions\n\n1. To create a new adaptation document, do either 2. or 3.\n2. Click on a listed filename to select it, then click the 'Input file' button.\n3. Double-click one of the listed files.\n\nIs the list empty? If so then either (a) or (b) is true:\n(a) There are no source text files in the 'Source Data' folder.\n(b) An adaptation document has already been created for each of the source text files in that folder.\nIn either case, please inform your administrator that no more documents can be created.");
	m_pInstructionsStaticCtrl->ChangeValue(leftmsg);

	// make the message text controls read-only
	m_pTopMessageStaticCtrl->SetEditable(FALSE);
	m_pInstructionsStaticCtrl->SetEditable(FALSE);

	m_pMonoclineListOfFiles->Clear();
	if (!m_pApp->m_sortedLoadableFiles.IsEmpty())
	{
		// copy the filenames into the listbox
		size_t limit = m_pApp->m_sortedLoadableFiles.GetCount();
		if (limit > 0)
		{
			size_t index;
			for (index = 0; index < limit; index++)
			{
				wxString filename = m_pApp->m_sortedLoadableFiles.Item(index);
				wxASSERT(!filename.IsEmpty());
				m_pMonoclineListOfFiles->Append(filename);
			}
		}
	}
}

void NavProtectNewDoc::OnInputFileButton(wxCommandEvent& event) 
{
	// test that the user has a selection, and get the selected filename into the private
	// m_userFilename member. The caller of this class can then access the value using the
	// public getter, GetUserFilename()
	wxASSERT(m_pMonoclineListOfFiles);
	int selIndex = m_pMonoclineListOfFiles->GetSelection();
	if (selIndex >= 0 && selIndex < (int)m_pMonoclineListOfFiles->GetCount())
	{
		// end the dialog
		event.Skip();
	}
	else
	{
		// stay in the dialog, and just beep
		wxBell();
	}
}

void NavProtectNewDoc::OnBnClickedCancel(wxCommandEvent& event) 
{
	m_userFilename = _T(""); // set it to an empty string
	event.Skip();
}

void NavProtectNewDoc::OnItemSelected(wxCommandEvent& WXUNUSED(event))
{
	wxASSERT(m_pMonoclineListOfFiles);
	int selIndex = m_pMonoclineListOfFiles->GetSelection();
	wxASSERT(selIndex != wxNOT_FOUND && selIndex < (int)m_pMonoclineListOfFiles->GetCount());
	if (selIndex >= 0 && selIndex < (int)m_pMonoclineListOfFiles->GetCount())
	{
		m_userFilename = m_pMonoclineListOfFiles->GetString(selIndex);
	}
	else
	{
		m_userFilename.Empty();
	}
}

void NavProtectNewDoc::OnDoubleClick(wxCommandEvent& WXUNUSED(event))
{
	wxASSERT(m_pMonoclineListOfFiles);
	int selIndex = m_pMonoclineListOfFiles->GetSelection();
	wxASSERT(selIndex != wxNOT_FOUND && selIndex < (int)m_pMonoclineListOfFiles->GetCount());
	if (selIndex >= 0 && selIndex < (int)m_pMonoclineListOfFiles->GetCount())
	{
		m_userFilename = m_pMonoclineListOfFiles->GetString(selIndex);
	}
	else
	{
		m_userFilename.Empty();
	}
	TransferDataToWindow();
    EndModal(wxID_OK);
}

// access function
wxString NavProtectNewDoc::GetUserFileName()
{
	return m_userFilename;
}




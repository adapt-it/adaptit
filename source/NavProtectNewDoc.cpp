/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			NavProtectNewDoc.cpp
/// \author			Bruce Waters
/// \date_created	9 August 2010
/// \rcs_id $Id$
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the NavProtectNewDoc class.
/// The NavProtectNewDoc class provides a dialog interface for the user to create a New
/// Document from a list of loadable source text (typically USFM marked up) plain text
/// files, obtained from a folder called '__SOURCE_INPUTS' which is a child of the currently
/// open project folder. (This __SOURCE_INPUTS folder is now routinely created in the
/// project folder when a project is created. When the AssignLocationsForInputsAndOutputs
/// dialog is set to protect source text inputs from navigation, the application only
/// shows the user files from this folder for which no document of the same name has been
/// created. This class is in support of the "Navigation Protection" feature, to hide from
/// the user the ability to do file and/or folder navigation using the standard file input
/// browser which is shown when <New Document> is clicked, or the File / New... command is
/// invoked, in legacy Adapt It versions. The latter is still what happens if the App's
/// m_bProtectSourceOutputsFolder is FALSE.
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

	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
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
	m_pInstructionsStaticCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_INSTRUCTIONS); // multiline, read-only
	wxString leftmsg;
	
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
			leftmsg = _(
"To create a new adaptation document:\n\nClick on a listed filename to select it, then click the 'Input file' button.\n\nOr, double-click one of the listed filenames.");
			m_pInstructionsStaticCtrl->ChangeValue(leftmsg);
		}
	}
	else
	{
		// no source text files are available (typically because they've all been used
		// to create documents, and so have bled the list to the point of it being
		// empty, so give an alternative message in this circumstance
		leftmsg = _(
"All of the formerly listed filenames have been used to create adaptation documents, so the list is now empty. You have not made an error.\n\nPlease inform your administrator that no more documents can be created.");
			m_pInstructionsStaticCtrl->ChangeValue(leftmsg);

			// hide the "Input file" button, since there is nothing in the list to select,
			// leave just the Cancel button showing
			m_pInputFileButton->Hide();
	}
	// make the message text control read-only
	m_pInstructionsStaticCtrl->SetEditable(FALSE);
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
	//wxASSERT(selIndex != wxNOT_FOUND && selIndex < (int)m_pMonoclineListOfFiles->GetCount());
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




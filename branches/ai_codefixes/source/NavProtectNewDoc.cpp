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
	EVT_BUTTON(wxID_OK, NavProtectNewDoc::OnCreateNewDocButton)	
	EVT_BUTTON(wxID_CANCEL, NavProtectNewDoc::OnBnClickedCancel)

END_EVENT_TABLE()

NavProtectNewDoc::NavProtectNewDoc(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Create a new document"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // This dialog function below is generated in wxDesigner, and defines the controls and
    // sizers for the dialog. The first parameter is the parent which should normally be
    // "this".
    // The second and third parameters should both be TRUE to utilize the sizers and create
    // the right size dialog.
	NewDocFromSourceDataFolderFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );


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
	CAdapt_ItApp* pApp;
	pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp != NULL);
}

void NavProtectNewDoc::OnCreateNewDocButton(wxCommandEvent& event) 
{
	event.Skip();
}

void NavProtectNewDoc::OnBnClickedCancel(wxCommandEvent& event) 
{
	event.Skip();
}



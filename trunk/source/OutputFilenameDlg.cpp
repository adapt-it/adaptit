/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			OutputFilenameDlg.cpp
/// \author			Bill Martin
/// \date_created	3 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the COutputFilenameDlg class. 
/// The COutputFilenameDlg class works together with the GetOutputFilenameDlgFunc()
/// dialog which was created and is maintained by wxDesigner. Together they 
/// implement the dialog used for getting a suitable name for the source data 
/// (title only, no extension).
/// \derivation		The COutputFilenameDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in OutputFilenameDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "OutputFilenameDlg.h"
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

// wxWidgets includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/filename.h>

// other includes
#include "Adapt_It.h"
#include "OutputFilenameDlg.h"
#include <wx/valgen.h>

// extern globals

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

extern fontInfo NavFInfo;

// event table
BEGIN_EVENT_TABLE(COutputFilenameDlg, AIModalDialog)
	EVT_INIT_DIALOG(COutputFilenameDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
END_EVENT_TABLE()

/////////////////////////////////////////////////////////////////////////////
// Dialog construction and attribute initialization

COutputFilenameDlg::COutputFilenameDlg(wxWindow* parent)
	: AIModalDialog(parent, -1, _("Type a name for this data"), // suggested title by Jim Henderson
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	GetOutputFilenameDlgFunc(this, TRUE, TRUE);
	// This dialog function is generated in wxDesigner, and defines the controls and sizers
	// for the dialog.
	// The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	// The declaration is: GetOutputFilenameDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	// initialize attributes

	m_strFilename = _T("");

	// use wxValidator for simple dialog data transfer
	wxTextCtrl* pEdit;
	pEdit = (wxTextCtrl*)FindWindowById(IDC_EDIT_FILENAME);
	pEdit->SetValidator(wxGenericValidator(&m_strFilename));

}

/////////////////////////////////////////////////////////////////////////////
// COutputFilenameDlg dialog initialization

void COutputFilenameDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) 
{
	//InitDialog is not virtual
		
	wxTextCtrl* pEdit = (wxTextCtrl*)FindWindowById(IDC_EDIT_FILENAME);

	// make the font show the user's desired point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, pEdit, NULL,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, pEdit, NULL, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif

	wxFileName fn(m_strFilename);
	pEdit->SetValue(fn.GetName());
	pEdit->SetFocus();
	pEdit->SetSelection(-1,-1); // -1,-1 selects all
}

/////////////////////////////////////////////////////////////////////////////
// COutputFilenameDlg message handlers



/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			OutputFilenameDlg.cpp
/// \author			Bill Martin
/// \date_created	3 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
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
#include "Adapt_ItDoc.h"
#include "OutputFilenameDlg.h"
#include "helpers.h"
#include <wx/valgen.h>

// extern globals

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

extern fontInfo NavFInfo;

// event table
BEGIN_EVENT_TABLE(COutputFilenameDlg, AIModalDialog)
	EVT_INIT_DIALOG(COutputFilenameDlg::InitDialog)
	EVT_BUTTON(wxID_OK, COutputFilenameDlg::OnOK)
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
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);

	m_strFilename = _T("");

	// use wxValidator for simple dialog data transfer
	pEdit = (wxTextCtrl*)FindWindowById(IDC_EDIT_FILENAME);
	wxASSERT(pEdit != NULL);
	//pEdit->SetValidator(wxGenericValidator(&m_strFilename));

	pStaticTextInvalidCharacters = (wxStaticText*)FindWindowById(ID_TEXT_INVALID_CHARACTERS);
	wxASSERT(pStaticTextInvalidCharacters != NULL);
	pStaticTextInvalidCharacters->SetLabel(_T(" ") + wxFileName::GetForbiddenChars()); 
}

/////////////////////////////////////////////////////////////////////////////
// COutputFilenameDlg dialog initialization

void COutputFilenameDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) 
{
	//InitDialog is not virtual
		
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

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void COutputFilenameDlg::OnOK(wxCommandEvent& event) 
{
	//  ensure there are no illegal characters in the filename
	//TransferDataFromWindow(); // whm removed 21Nov11
	m_strFilename = pEdit->GetValue(); // whm added 21Nov11

	wxString fn = m_strFilename;
	wxString illegals = wxFileName::GetForbiddenChars(); //_T(":?*\"\\/|<>");
	wxString scanned = SpanExcluding(fn, illegals);
	if (scanned == fn)
	{
		// BW added 22July08; check for a name clash
		bool bNamesClash = gpApp->GetDocument()->FilenameClash(fn);
		if (bNamesClash)
		{
			// IDS_TYPED_DOCNAME_CLASHES
			wxMessageBox(_("The name you typed clashes with an existing document name. Please type a different name.")
				,_T(""), wxICON_WARNING);
			return; // leave user in the dialog, to fix the name
		}
	}
	else
	{
		// there is at least one illegal character, replace each such
		// by a space, beep, and show the modified string to the user
		//int nFound = -1;
		// whm Note: Since different platforms have slightly differing illegal characters for file
		// names, etc., we will not hard code the search for characters as does the MFC version.
		// Instead, we'll scan the file name char by char and change any that are illegal to spaces.
		int ct;
		int foundPos;
		for (ct = 0; ct < (int)fn.Length(); ct++)
		{
			foundPos = FindOneOf(fn, illegals);
			if (foundPos != -1)
				fn[foundPos] = _T(' ');
		}
		m_strFilename = fn;
		//TransferDataFromWindow(); // whm removed 21Nov11
		// whm note 21Nov11 the TransferDataFromWindow() call above would defeat the preceding code
		// that replaces any illegal char with a space, so I've removed it entirely.
		::wxBell();
		// if we decide to verbally tell the user what the beep means:
		//wxString message;
		//message = message.Format(_("Names cannot include these characters: %s (Note: An .xml extension will be automatically added.) Please try the New... command again."),illegals.c_str());
		//wxMessageBox(message, _("Bad characters found in name"), wxICON_INFORMATION);	}
	}
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

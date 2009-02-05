/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ProgressDlg.cpp
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CProgressDlg class. 
/// The CProgressDlg class puts up a progress bar/gauge which tracks the
/// working processes that typically take a while to complete.
/// The interface resources for the CProgressDlg are defined in ProgressDlgFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		The CProgressDlg class is derived from wxDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ProgressDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ProgressDlg.h"
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
#include "ProgressDlg.h" 

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog


CProgressDlg::CProgressDlg(wxWindow* parent) // dialog constructor
	: wxDialog(parent, -1, _("Progress"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	ProgressDlgFunc(this, TRUE, TRUE);
	// The declaration is: ProgressDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

	m_docFile = _T("");
	m_phraseCount = _T("");
	m_nTotalPhrases = 0;
	m_xofy = _T("");

	// Use wxGenericValidator to transfer strings to static text controls
	pStatic1 = (wxStaticText*)FindWindowById(IDC_STATIC_FILENAME);
	pStatic1->SetValidator(wxGenericValidator(&m_docFile));
	pStatic2 = (wxStaticText*)FindWindowById(IDC_STATIC_XOFY);
	pStatic2->SetValidator(wxGenericValidator(&m_xofy));
	pStatic3 = (wxStaticText*)FindWindowById(IDC_STATIC_TOTAL);
	pStatic3->SetValidator(wxGenericValidator(&m_phraseCount));
}

// event handler table
BEGIN_EVENT_TABLE(CProgressDlg, wxDialog)
	EVT_INIT_DIALOG(CProgressDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
END_EVENT_TABLE()

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg message handlers

void CProgressDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) 
{
	//CDialog::OnInitDialog();
	
	
}

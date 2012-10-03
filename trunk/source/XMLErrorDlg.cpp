/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			XMLErrorDlg.cpp
/// \author			Bruce Waters, revised for wxWidgets by Bill Martin
/// \date_created	6 January 2005
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CXMLErrorDlg class. 
/// The CXMLErrorDlg class provides a dialog to notify the user that an XML read 
/// error has occurred, giving a segment of the offending text and a character
/// count of approximately where the error occurred.
/// \derivation		The CXMLErrorDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in XMLErrorDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "XMLErrorDlg.h"
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
#include "XMLErrorDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CXMLErrorDlg, AIModalDialog)
	EVT_INIT_DIALOG(CXMLErrorDlg::InitDialog)
END_EVENT_TABLE()


CXMLErrorDlg::CXMLErrorDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("XML Document Parsing Error"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	XMLErrorDlgFunc(this, TRUE, TRUE);
	// The declaration is: XMLErrorDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

	m_errorStr = ""; // a CBString
	m_messageStr = _T("");
	m_offsetStr = _T("");
	
}

CXMLErrorDlg::~CXMLErrorDlg() // destructor
{
	
}

void CXMLErrorDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp);

	wxTextCtrl* pEditErr = (wxTextCtrl*)FindWindowById(IDC_EDIT_ERR_XML);
	wxTextCtrl* pEditOffset = (wxTextCtrl*)FindWindowById(IDC_EDIT_OFFSET);

	// make the fonts show user-defined font point size in the error box
	// but use 12 point size elsewhere; we don't use SetFontAndDirectionalityForDialogControl()
	// here because of the special requirements and the CStatic instances needing to be set too
	CopyFontBaseProperties(pApp->m_pNavTextFont,pApp->m_pDlgSrcFont);
	pApp->m_pDlgSrcFont->SetPointSize(pApp->m_dialogFontSize);
	pEditErr->SetFont(*pApp->m_pDlgSrcFont);

	// whm note: I don't think the dialog's static text 
	// nor the character number where occurred needs to 
	// have special font treatment, so I've not included
	// the MFC code below that assigns nav text basic
	// properties to the pEditOffset, pStatMsg and pStatLbl.

	// whm put the char number in the pEditOffset box
	pEditOffset->SetValue(m_offsetStr);

	// put the error text in the pEditErr box
#ifdef _UNICODE
	wxString errorStr;
	gpApp->Convert8to16(m_errorStr,errorStr);
	pEditErr->SetValue(errorStr);
#else
	// WX NOTE: ANSI builds automatically use the SetWindowTextA function
	// so the wx version should simply use the usual SetValue() equivalent
	pEditErr->SetValue((char*)m_errorStr);
#endif
	// Note: We don't need RTL support, since UTF-8 data will be LTR and
	// if converted from UTF-16 it will be unreadable anyway

}

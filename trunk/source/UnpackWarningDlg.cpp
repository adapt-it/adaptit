/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			UnpackWarningDlg.cpp
/// \author			Bill Martin
/// \date_created	20 July 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CUnpackWarningDlg class. 
/// The CUnpackWarningDlg class provides a dialog that issues a warning to the user
/// that the document was packed by the Unicode version of the program and now the
/// non-Unicode version is attempting to unpack it, or vs versa.
/// \derivation		The CUnpackWarningDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in UnpackWarningDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "UnpackWarningDlg.h"
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
#include "UnpackWarningDlg.h"

// event handler table
BEGIN_EVENT_TABLE(CUnpackWarningDlg, AIModalDialog)
	EVT_INIT_DIALOG(CUnpackWarningDlg::InitDialog)
END_EVENT_TABLE()


CUnpackWarningDlg::CUnpackWarningDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Adapt It and Adapt It Unicode Mismatched For Unpack"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pUnpackDlgSizer = UnpackWarningDlgFunc(this, TRUE, TRUE);
	// The declaration is: UnpackWarningDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
}

CUnpackWarningDlg::~CUnpackWarningDlg() // destructor
{
	
}

void CUnpackWarningDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	wxTextCtrl* pEdit = (wxTextCtrl*)FindWindowById(IDC_EDIT_UNPACK_WARN);

	// get all the resource text strings and add them
	wxString msg;
	wxString bit;
	bit = _("The document was packed by Adapt It, but you are trying to unpack using Adapt It Unicode;");
	msg = bit;
	bit = _("or it was packed by Adapt It Unicode and you are trying to use Adapt It to unpack it. ");
	msg += bit;
	bit = _("(The configuration files of these two applications are not compatible and packing incorporates the project one.) "); 
	msg += bit;
	bit = _("Unpacking will not proceed further. To unpack this particular file you must switch applications, then try again."); 
	msg += bit;

	// put them in the dialog's edit box
	pEdit->SetValue(msg);
	pUnpackDlgSizer->Layout();
	::wxBell();
}

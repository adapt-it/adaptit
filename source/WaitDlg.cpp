/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			WaitDlg.cpp
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the CWaitDlg class. 
/// The CWaitDlg class provides a custom "Please wait" dialog to notify the
/// user that the current process will take some time to complete.
/// The CWaitDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// \derivation		The CWaitDlg class is derived from wxDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in WaitDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "WaitDlg.h"
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
#include "WaitDlg.h"
 
/////////////////////////////////////////////////////////////////////////////
// CWaitDlg dialog

CWaitDlg::CWaitDlg(wxWindow* parent) // dialog constructor
	: wxDialog(parent, -1, _("Please Wait..."),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
				, m_nWaitMsgNum(0)
{
	pWaitDlgSizer = WaitDlgFunc(this, TRUE, TRUE);
	// This dialog function is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	// The declaration is: WaitDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

	pStatic = (wxStaticText*)FindWindowById(IDC_PLEASE_WAIT);
	// Use wxGenericValidator to transfer WaitMsg string to static text control
	pStatic->SetValidator(wxGenericValidator(&WaitMsg));
}

// event handler table
BEGIN_EVENT_TABLE(CWaitDlg, wxDialog)
	EVT_INIT_DIALOG(CWaitDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
END_EVENT_TABLE()

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg message handlers
void CWaitDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	switch (m_nWaitMsgNum)
	{
		case 0: // no longer used
			// IDS_WAIT_FOR_DOC_LAYOUT
			WaitMsg = _("Please wait while Adapt It lays out the document again...");
			break;
		case 1: // no longer used
			// IDS_WAIT_FOR_RTF_OUTPUT
			WaitMsg = _("Please wait for Adapt It to output the RTF file. This may take a while...");
			break;
		case 2: // no longer used
			// IDS_WAIT_FOR_BT_COLLECTION
			WaitMsg = _("Please wait while Adapt It collects the back translations and stores them.\nThis may take a while...");
			break;
		case 3: 
			WaitMsg = _("Please wait while Adapt It Reads the File...");
			pStatic->SetLabel(WaitMsg); // not needed with validator
			break;
		case 4: 
			WaitMsg = _("Please wait while Adapt It Saves the File...");
			pStatic->SetLabel(WaitMsg); // not needed with validator
			break;
		case 5: 
			WaitMsg = _T("");
			pStatic->Hide(); // this selection just hides the static text message leaving the Title "Please Wait..."
			break;
		default:
			WaitMsg = _("Please wait. This may take a while...");
	}
	TransferDataToWindow();
	Refresh();
	pWaitDlgSizer->Layout();
}


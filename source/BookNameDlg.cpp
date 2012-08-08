/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			BookNameDlg.h
/// \author			Bruce Waters
/// \date_created	7 August 2012
/// \date_revised	
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CBookName class. 
/// The CBookName class provides a handler for the "Change Book Name..." menu item. It is
/// also called at other times, not just due to an explicit menu item click - such as when
/// creating a new document, or opening a document which has no book name defined but does
/// have a valid bookID code within it. This handler class lets the user get an appropriate
/// book name defined, given a valid bookID code -- it supports the Paratext list of 123
/// book ids and full book name strings (the latter are localizable). A book name is
/// needed for exports of xhtml or to Pathway, so this dialog provides the functionality
/// for associating a book name with every document which contains a valid bookID.
/// \derivation		The CBookName class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "BookNameDlg.h"
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
#include "Adapt_It.h" // need this for AIModalDialog definition
#include "Adapt_It_wdr.h"
#include "BookNameDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CBookName, AIModalDialog)
	EVT_INIT_DIALOG(CBookName::InitDialog)
	EVT_BUTTON(wxID_OK, CBookName::OnOK)
	EVT_BUTTON(wxID_CANCEL, CBookName::OnCancel)
END_EVENT_TABLE()

CBookName::CBookName(
		wxWindow* parent,
		wxString* title,
		wxString* pstrBookCode, 
		bool      bShowCentered) : AIModalDialog(parent, -1, *title, wxDefaultPosition,
					wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	m_pBookNameDlgSizer = BookNameDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	m_bShowItCentered = bShowCentered;
	m_bookCode = *pstrBookCode;
}

CBookName::~CBookName() // destructor
{
}

void CBookName::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	if (m_bShowItCentered)
	{
		this->Centre(wxHORIZONTAL);
	}
}

void CBookName::OnOK(wxCommandEvent& event)
{
	event.Skip();
}

void CBookName::OnCancel(wxCommandEvent& event)
{
	event.Skip();
}


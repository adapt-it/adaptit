/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CCTableEditDlg.cpp
/// \author			Bill Martin
/// \date_created	19 June 2007
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CCCTableEditDlg class. 
/// The CCCTableEditDlg class provides a simple dialog with a large text control 
/// for user editing of CC tables.
/// \derivation		The CCCTableEditDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "TableEditDlg.h"
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
#include "CCTableEditDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CCCTableEditDlg, AIModalDialog)
	EVT_INIT_DIALOG(CCCTableEditDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CCCTableEditDlg::OnOK)
END_EVENT_TABLE()

CCCTableEditDlg::CCCTableEditDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Edit Consistent Changes Table"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	CCTableEditDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	// use wxValidator for simple dialog data transfer
	pEditCCTable = (wxTextCtrl*)FindWindowById(IDC_EDIT_CCT);
	wxASSERT(pEditCCTable != NULL);
	//pEditCCTable->SetValidator(wxGenericValidator(&m_ccTable));

}

CCCTableEditDlg::~CCCTableEditDlg() // destructor
{
}

void CCCTableEditDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	//m_ccTable = _T(""); // this should not be set to null string, otherwise an existing table can't be edited
	
	// make the fonts show user-defined font point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, pEditCCTable, NULL,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, pEditCCTable, NULL, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif
	//TransferDataToWindow(); // whm removed 21Nov11
	pEditCCTable->ChangeValue(m_ccTable); // whm added 21Nov11
	// whm 21Nov11 note: the caller CCCTabbedDialog::DoEditor() accesses the m_ccTable value before this 
	// dialog is destroyed.
}

// whm 11Jan12 added OK handler which is needed to set value of m_tableName after having
// removed the SetValidator() call in the constructor.
void CCCTableEditDlg::OnOK(wxCommandEvent& event) 
{
	m_ccTable = pEditCCTable->GetValue();
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

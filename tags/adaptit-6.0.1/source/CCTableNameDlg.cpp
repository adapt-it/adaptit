/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CCTableNameDlg.cpp
/// \author			Bill Martin
/// \date_created	19 June 2007
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CCCTableNameDlg class. 
/// The CCCTableNameDlg class provides a simple dialog for the input of a consistent
/// changes table name from the user.
/// \derivation		The CCCTableNameDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in CCTableNameDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "CCTableNameDlg.h"
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
#include "CCTableNameDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CCCTableNameDlg, AIModalDialog)
	EVT_INIT_DIALOG(CCCTableNameDlg::InitDialog)
END_EVENT_TABLE()

CCCTableNameDlg::CCCTableNameDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Create CC Table"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	CCTableNameDlgFunc(this, TRUE, TRUE);
	// The declaration is: CCTableNameDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);

	// use wxValidator for simple dialog data transfer
	m_pEditTableName = (wxTextCtrl*)FindWindow(IDC_EDIT_TBLNAME);
	m_pEditTableName->SetValidator(wxGenericValidator(&m_tableName));

	m_pEditCtrlAsStatic = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC);
	wxColour sysColorBtnFace = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	m_pEditCtrlAsStatic->SetBackgroundColour(sysColorBtnFace);
}

CCCTableNameDlg::~CCCTableNameDlg() // destructor
{
}

void CCCTableNameDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	m_tableName = _T("");

	wxString helpString;
	helpString = _("Type the filename (omit the extention) for the consistent changes table. Adapt it will automatically append a .cct extension; and the file will be created in the following folder:\n\n%s");
	// whm 14Jul11 Note: The App's m_ccTableInputsAndOutputsFolderPath may be an empty string if no project is
	// active (can be the case since the Tools > Load Consistent Changes... menu item is enabled even when
	// no project is active. When no project is active we simply use the m_lastCcTablePath location for
	// saving newly created cc table files.
	if (gpApp->m_bProtectCCTableInputsAndOutputsFolder)
	{
		helpString = helpString.Format(helpString,gpApp->m_ccTableInputsAndOutputsFolderPath.c_str());
	}
	else
	{
		helpString = helpString.Format(helpString,gpApp->m_lastCcTablePath.c_str());
	}
	wxLogDebug(helpString);
	m_pEditCtrlAsStatic->ChangeValue(helpString);

	// make the font show user's desired point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, m_pEditTableName, NULL,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, m_pEditTableName, NULL, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif

	m_pEditTableName->SetFocus();
}


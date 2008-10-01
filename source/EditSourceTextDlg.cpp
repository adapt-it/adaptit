/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			EditSourceTextDlg.cpp
/// \author			Bill Martin
/// \date_created	13 July 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the CEditSourceTextDlg class. 
/// The CEditSourceTextDlg class provides a dialog in which the user can edit the
/// source text. Restrictions are imposed to prevent such editing while glossing, or 
/// if the source text has disparate text types, is a retranslation or has a free
/// translation or filtered information contained within it.
/// \derivation		The CEditSourceTextDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in EditSourceTextDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "EditSourceTextDlg.h"
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
#include "EditSourceTextDlg.h"
#include "helpers.h"

// next two are for version 2.0 which includes the option of a 3rd line for glossing

// This global is defined in Adapt_ItView.cpp.
//extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

// the following globals make it easy to access the sublists and their counts; for use in 
// CEditSourceTextDlg and in the subsequent CTransferMarkersDlg especialy
//extern SPList* gpOldSrcPhraseList;
//extern SPList* gpNewSrcPhraseList;
extern int gnCount;    // count of old srcphrases (user selected these) after unmerges, etc
extern int gnNewCount; // count of new srcphrases (after user finished editing the source text)

// BEW additions 07May08, for the refactored code
extern EditRecord gEditRecord; // store info pertinent to generalized editing in this global structure
// end BEW additions 07 May08


/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CEditSourceTextDlg, AIModalDialog)
	EVT_INIT_DIALOG(CEditSourceTextDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
END_EVENT_TABLE()


CEditSourceTextDlg::CEditSourceTextDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Edit Source Text"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	m_strNewSourceText = _T("");
	m_strOldSourceText = _T("");
	//m_bEditMarkersWanted = FALSE;
	
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	EditSourceTextDlgFunc(this, TRUE, TRUE);
	// The declaration is: EditSourceTextDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	// TODO: Get pointerf for the following:
	// ID_TEXTCTRL_EDIT_SOURCE_AS_STATIC1 
	// ID_TEXTCTRL_EDIT_SOURCE_AS_STATIC2
	// IDC_EDIT_OLD_SOURCE_TEXT
	// IDC_EDIT_NEW_SOURCE // used in TransferMarkersDlg (which can be removed from project)
	// IDC_EDIT_FOLLCONTEXT // used in RetranslationDlg but both are modal dialogs so shouldn't
	// interfere with each other.
	// 
	pSrcTextEdit = (wxTextCtrl*)FindWindowById(IDC_EDIT_NEW_SOURCE);
	pSrcTextEdit->SetValidator(wxGenericValidator(&m_strNewSourceText)); // needed; OnEditSourceText() initializes this

	pPreContextEdit = (wxTextCtrl*)FindWindowById(IDC_EDIT_PRECONTEXT); // read only edit control
	pPreContextEdit->SetValidator(wxGenericValidator(&m_preContext)); // needed; OnEditSourceText() initializes this
	pPreContextEdit->SetBackgroundColour(gpApp->sysColorBtnFace); //(wxSYS_COLOUR_WINDOW);

	pFollContextEdit = (wxTextCtrl*)FindWindowById(IDC_EDIT_FOLLCONTEXT); // read only edit control
	pFollContextEdit->SetValidator(wxGenericValidator(&m_follContext)); // needed; OnEditSourceText() initializes this
	pFollContextEdit->SetBackgroundColour(gpApp->sysColorBtnFace); //(wxSYS_COLOUR_WINDOW);

	pOldSrcTextEdit = (wxTextCtrl*)FindWindowById(IDC_EDIT_OLD_SOURCE_TEXT); // read only edit control
	pOldSrcTextEdit->SetValidator(wxGenericValidator(&m_strOldSourceText)); // needed; OnEditSourceText() initializes this
	pOldSrcTextEdit->SetBackgroundColour(gpApp->sysColorBtnFace); //(wxSYS_COLOUR_WINDOW);

	// The following two are for static text within read-only multi-line wxEditCtrls on the dialog
	
	pTextCtrlEditAsStatic1 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_EDIT_SOURCE_AS_STATIC1); // read only edit control
	pTextCtrlEditAsStatic1->SetBackgroundColour(gpApp->sysColorBtnFace); //(wxSYS_COLOUR_WINDOW);
	
	pTextCtrlEditAsStatic2 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_EDIT_SOURCE_AS_STATIC2); // read only edit control
	pTextCtrlEditAsStatic2->SetBackgroundColour(gpApp->sysColorBtnFace); //(wxSYS_COLOUR_WINDOW);
}

CEditSourceTextDlg::~CEditSourceTextDlg() // destructor
{
	
}

void CEditSourceTextDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	// next stuff copied from CRetranslationDlg's OnInitDialog() and then modified

	// make the fonts show user's desired point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, pSrcTextEdit, pPreContextEdit,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, pSrcTextEdit, pPreContextEdit, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif

	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, pFollContextEdit, pOldSrcTextEdit,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, pFollContextEdit, pOldSrcTextEdit, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif

	//#ifdef _RTL_FLAGS
	//gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, pTgtEdit, NULL,
	//							NULL, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bTgtRTL);
	//#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	//gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, pTgtEdit, NULL, 
	//							NULL, NULL, gpApp->m_pDlgTgtFont);
	//#endif
	TransferDataToWindow();

	// make sure the end of the text is scolled into view
	pPreContextEdit->SetFocus();
	pPreContextEdit->ShowPosition(pPreContextEdit->GetLastPosition());

	// set the focus to the source text edit box
	pSrcTextEdit->SetFocus();
	pSrcTextEdit->SetSelection(-1,-1); // -1,-1 selects all

	//pEditSourceTextDlgSizer->Layout();
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CEditSourceTextDlg::OnOK(wxCommandEvent& event) 
{
	
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}

void CEditSourceTextDlg::OnCancel(wxCommandEvent& WXUNUSED(event))
{

	//
	EndModal(wxID_CANCEL); //wxDialog::OnCancel(event);
}

// other class methods


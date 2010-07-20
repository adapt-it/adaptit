/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			PeekAtFile.h
/// \author			Bruce Waters
/// \date_created	14 July 2010
/// \date_revised	
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CPeekAtFileDlg class. 
/// The CPeekAtFileDlg class provides a simple dialog with a large multiline text control 
/// for the user to be able to peek at as many as the first 200 lines of a selected file
/// (if the selection is multiple, only the first file in the list is used) from the right
/// hand pane of the Move Or Copy Folders Or Files dialog, accessible from the
/// Administrator menu.
/// \derivation		The CPeekAtFileDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "PeekAtFile.h"
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
#include "AdminMoveOrCopy.h"
#include "PeekAtFile.h"
#include "helpers.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CPeekAtFileDlg, AIModalDialog)
	EVT_INIT_DIALOG(CPeekAtFileDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_OK, CPeekAtFileDlg::OnClose)
END_EVENT_TABLE()

CPeekAtFileDlg::CPeekAtFileDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Show what is at the start of the selected file"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	PeekAtFileFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_pEditCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_LINES100);
	wxASSERT(m_pEditCtrl != NULL);
	// set ptr to the parent dialog (we are its friend)
	m_pAdminMoveOrCopy = (AdminMoveOrCopy*)parent;

	// Set the message text (two lines), then make the message text control read only
	m_pMsgCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_PEEKMSG);
	wxASSERT(m_pMsgCtrl != NULL);
}

CPeekAtFileDlg::~CPeekAtFileDlg() // destructor
{
}

void CPeekAtFileDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	wxASSERT(m_pAdminMoveOrCopy);

	m_pMsgCtrl->SetEditable(TRUE);
	wxString eol = _T("\n");
	Line1Str = _(
"Peek shows a few hundred lines of the file (actually, up to 16kB of the file's contents).");
	Line2Str = _(
"The text is read-only, any changes you type will not be accepted. Font, encoding and direction use the source text settings.");
	m_pMsgCtrl->AppendText(Line1Str);
	m_pMsgCtrl->AppendText(eol);
	m_pMsgCtrl->AppendText(Line2Str);
	m_pMsgCtrl->SetInsertionPointEnd();
	m_pMsgCtrl->SetEditable(FALSE);

	// Set source font colour to black temporarily. Don't use SetFontAndDirectionalityForDialogControl() because it
	// keeps the point size to whatever m_dialogFontSize is, and I prefer to use source
	// text's current size, as some arabic  or other complex scripts at 12 point size are
	// very difficult to read. Directionality and point size don't need changing.
	m_storeColor = gpApp->m_sourceColor;
	wxColour color = wxColour(0,0,0); // black
	gpApp->m_pSrcFontData->SetColour(color);
	// set the font to be used in the edit control to our changed m_pSourceFont
	m_pEditCtrl->SetFont(*gpApp->m_pSourceFont);

	//#ifdef _RTL_FLAGS
	//gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pEditCtrl, NULL,
	//							NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	//#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	//gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pEditCtrl, NULL, 
	//							NULL, NULL, gpApp->m_pDlgSourceFont);
	//#endif

	// get the first 200 lines or so (16 kb actually), or all of them if the file is shorter,
	// into the text ctrl
	//bool bPopulatedOK = PopulateTextCtrlByLines(m_pEditCtrl, &m_filePath, 200);
	bool bPopulatedOK = PopulateTextCtrlWithChunk(m_pEditCtrl, &m_filePath, 16);  // 16 kB
	if (!bPopulatedOK)
	{
		// should not fail, as binary files should be excluded, so use English text
		wxString msg;
		msg = msg.Format(_T(
"PopulateTextCtrlByLines() failed, so nothing is visible. wxTextFile failed to open the file with path: %s"),
		m_filePath);
		wxMessageBox(msg.c_str(),_T("Error"),wxICON_WARNING);
	}
	m_pEditCtrl->SetInsertionPointEnd();
	m_pEditCtrl->SetEditable(FALSE);

}

void CPeekAtFileDlg::OnClose(wxCommandEvent& event)
{
	// two lines for debugging, is the apppended text actually there? Yes, both lines are there
	//wxString firstLine = m_pMsgCtrl->GetLineText(0);
	//wxString secondLine = m_pMsgCtrl->GetLineText(1);

	// restore original colour of the source font
	gpApp->m_pSrcFontData->SetColour(m_storeColor);

	// clear the text from the edit box
	m_pEditCtrl->Clear();
	event.Skip();
}


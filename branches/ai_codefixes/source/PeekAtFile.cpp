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
#include <wx/tooltip.h>

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
	EVT_BUTTON(ID_BUTTON_TOGGLE_TEXT_DIRECTION, CPeekAtFileDlg::OnBnClickedToggleDirectionality)

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

	// now the stuff which is involved in dynamic changes
	// support for dynamic button and destruction/recreation of wxTextCtrl
	// (the text control has no tooltip, but the button does)
	m_pContainingHBoxSizer = (wxBoxSizer*)m_pEditCtrl->GetContainingSizer();
	wxASSERT(m_pContainingHBoxSizer->IsKindOf(CLASSINFO(wxBoxSizer)));

	m_pToggleDirBtn = (wxButton*)FindWindowById(ID_BUTTON_TOGGLE_TEXT_DIRECTION);
	wxASSERT(m_pToggleDirBtn->IsKindOf(CLASSINFO(wxButton)));

	m_pToggleBtnTooltip = m_pToggleDirBtn->GetToolTip();
	wxASSERT(m_pToggleBtnTooltip->IsKindOf(CLASSINFO(wxToolTip)));
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
"The text is read-only, any changes you type will not be accepted. Font and encoding use the source text settings.");
	m_pMsgCtrl->AppendText(Line1Str);
	m_pMsgCtrl->AppendText(eol);
	m_pMsgCtrl->AppendText(Line2Str);
	m_pMsgCtrl->SetInsertionPoint(0);
	m_pMsgCtrl->SetEditable(FALSE);

    // Set source font colour to black temporarily. Don't use
    // SetFontAndDirectionalityForDialogControl() because it keeps the point size to
    // whatever m_dialogFontSize is, and I prefer to use source text's current size, as
    // some arabic or other complex scripts at 12 point size are very difficult to read.
	// Point size doesn't need changing, but directionality may need reversing - we
	// provide a button for that.
	m_storeColor = gpApp->m_sourceColor;
	wxColour color = wxColour(0,0,0); // black
	gpApp->m_pSrcFontData->SetColour(color);
	// set the font to be used in the edit control to our changed m_pSourceFont
	m_pEditCtrl->SetFont(*gpApp->m_pSourceFont);

	// dynamic button labels (localizable)
	m_btnToRTL_Label = _("Display the text Right-To-Left");
	m_btnToLTR_Label = _("Display the text Left-To-Right");

	// The default directionality, when the text is first displayed, will be the
	// directionality of the source text in the current project; but we provide a button
	// to reverse it if that turns out to be a bad default for the data being viewed.
	// The dialog resource assumes LTR, and the button label starts out as for
	// m_btnToRTL_Label above. If the source text for the current project is RTL, we must
	// change the button label here before the dialog displays
#ifdef _RTL_FLAGS
	// _RTL_FLAGS is defined only within the Unicode build
	if (gpApp->m_bSrcRTL)
	{
		ChangeBtnLabelToLTR();
		m_pEditCtrl->SetLayoutDirection(wxLayout_RightToLeft);
		m_curDir = itsRTL;
	}
	else
	{
		// the button label is already "Display the text Right-To-Left", so just set the
		// directionality
		m_pEditCtrl->SetLayoutDirection(wxLayout_LeftToRight);
		m_curDir = itsLTR;
	}
#else
	// the ANSI application should only show LTR text, and the button to reverse the
	// directionality needs to be hidden
	m_pEditCtrl->SetLayoutDirection(wxLayout_LeftToRight);
	m_curDir = itsLTR;
	m_pToggleDirBtn->Show(FALSE);
#endif
	

	// get the first 200 lines or so (16 kb actually), or all of them if the file is shorter,
	// into the text ctrl
	bool bPopulatedOK = PopulateTextCtrlWithChunk(m_pEditCtrl, &m_filePath, 16);  // 16 kB
	if (!bPopulatedOK)
	{
		// should not fail, as binary files should be excluded, so use English text
		wxString msg;
		msg = msg.Format(_T(
"PopulateTextCtrlByLines() failed, so nothing is visible. wxTextFile failed to open the file with path: %s"),
		m_filePath.c_str());
		wxMessageBox(msg,_T("Error"),wxICON_WARNING);
	}
	m_pEditCtrl->SetInsertionPoint(0);
	m_pEditCtrl->SetEditable(FALSE);
}

void CPeekAtFileDlg::ChangeBtnLabelToLTR()
{
	m_pToggleDirBtn->SetLabel(m_btnToLTR_Label);
}

void CPeekAtFileDlg::ChangeBtnLabelToRTL()
{
	m_pToggleDirBtn->SetLabel(m_btnToRTL_Label);
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

void CPeekAtFileDlg::OnBnClickedToggleDirectionality(wxCommandEvent& WXUNUSED(event))
{
	// alter the direction of the text displayed in the wxTextCtrl
	m_curDir = ReverseTextDirectionality(m_curDir);

	// make the button label comply with the new directionality value (it has to have the
	// opposite directionality in the label's text)
	if (m_curDir == itsLTR)
	{
		// we've just changed the text directionality to LTR, so the button must say RTL
		ChangeBtnLabelToRTL();
	}
	else
	{
		// we've just changed the text directionality to RTL, so the button must say LTR
		ChangeBtnLabelToLTR();
	}
}


enum TxtDir CPeekAtFileDlg::ReverseTextDirectionality(enum TxtDir currentDir)
{
	enum TxtDir newDir = currentDir == itsLTR ? itsRTL : itsLTR; // toggle the directionality

	// destroy the wxTextCtrl and create a new one with the opposite directionality
	int nID = m_pEditCtrl->GetId(); // the ID was dynamically assigned, so we need to get it
	m_viewedText = m_pEditCtrl->GetValue(); // store the control's text
	m_pEditCtrl->Clear();
	delete m_pEditCtrl;
	m_pEditCtrl = new wxTextCtrl(this, nID, wxT(""), wxDefaultPosition, wxSize(640,440), wxTE_MULTILINE|wxVSCROLL|wxHSCROLL );

	// set the new directionality
	if (newDir == itsRTL)
	{
		m_pEditCtrl->SetLayoutDirection(wxLayout_RightToLeft);
	}
	else
	{
		m_pEditCtrl->SetLayoutDirection(wxLayout_LeftToRight);
	}
	// now re-do the initialization stuff from InitDialog()
	m_pEditCtrl->SetFont(*gpApp->m_pSourceFont);
	m_pEditCtrl->ChangeValue(m_viewedText);
	m_pEditCtrl->SetInsertionPoint(0);
	m_pEditCtrl->SetEditable(FALSE);

	// now add the control to the sizer & get the dialog's layout refreshed...
	m_pContainingHBoxSizer->Add(m_pEditCtrl, 1, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

	// adjust the layout and refresh the client area
	this->Layout();
	Refresh();
	return newDir;
}


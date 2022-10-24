/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			RetranslationDlg.cpp
/// \author			Bill Martin
/// \date_created	21 June 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CRetranslationDlg class. 
/// The CRetranslationDlg class implements the dialog class which provides the
/// dialog in which the user can enter a new translation (retranslation). The
/// source text to be translated/retranslated plus the preceding and following
/// contexts are also displayed.
/// \derivation		The CRetranslationDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "RetranslationDlg.h"
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
#include <wx/clipbrd.h>
#include "RetranslationDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CRetranslationDlg, AIModalDialog)
EVT_INIT_DIALOG(CRetranslationDlg::InitDialog)
EVT_BUTTON(IDC_COPY_RETRANSLATION_TO_CLIPBOARD, CRetranslationDlg::OnCopyRetranslationToClipboard)
EVT_BUTTON(ID_GET_RETRANSLATION_FROM_CLIPBOARD, CRetranslationDlg::OnPasteRetranslationFromClipboard)
EVT_SPINCTRL(ID_SPINCTRL_RETRANS, CRetranslationDlg::OnSpinValueChanged)
EVT_BUTTON(wxID_OK, CRetranslationDlg::OnOK)
END_EVENT_TABLE()


CRetranslationDlg::CRetranslationDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Type Retranslation Text"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	m_retranslation = _T("");
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pRetransSizer = RetranslationDlgFunc(this, TRUE, TRUE);
	// The declaration is: RetranslationDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
    // whm 5Mar2019 Note: The OK and Cancel buttons in the RetranslationDlgFunc() function
    // are now using the wxStdDialogButtonSizer that locates the OK and Cancel buttons at bottom
    // right, and we need not call the ReverseOkCancelButtonsForMac() function.

	pSpinCtrl = (wxSpinCtrl*)FindWindowById(ID_SPINCTRL_RETRANS);

	// whm 31Aug2021 modified line below to use the AutoCorrectTextCtrl class which is now
	// used as a custom control in wxDesigner's RetranslationDlgFunc() dialog.
	pRetransBox = (AutoCorrectTextCtrl*)FindWindowById(IDC_EDIT_RETRANSLATION);
}

CRetranslationDlg::~CRetranslationDlg() // destructor
{
	
}

void CRetranslationDlg::OnSpinValueChanged(wxSpinEvent& event)
{
	wxUnusedVar(event);
	int newFontSize = pSpinCtrl->GetValue();
	int maxSize = pSpinCtrl->GetMax();
	int minSize = pSpinCtrl->GetMin();
	bool bInRange = FALSE; // initialise
	bInRange = (newFontSize <= maxSize && newFontSize >= minSize) ? TRUE : FALSE;
	if (bInRange)
	{
		gpApp->SetFontAndDirectionalityForRetranslationControl(gpApp->m_pTargetFont, pRetransBox,
			gpApp->m_pRetransFont, newFontSize); // default is bIsRTL FALSE
		// Reset the value in AI.h  gApp->m_nRetransDlg_FontSize
		// 
		// whm 24Oct2022 Note: It isn't really necessary to set the App's m_nRetransDlg_FontSize value
		// here. It is set in the OnOK() handler below. Setting it here has the side effect that the
		// changed spin control's value will be stored in the project configuration file even if the user
		// clicks on Cancel to abort the Retranslation dialog and any changes made there. This side effect
		// won't do any real harm because the spin control's value is limited to a small range and presumably
		// its value was set for better readability of any Retranslation text, even if no textual changes 
		// were made, and the new value may as well be saved for better visibility of text in the dialog.
		gpApp->m_nRetransDlg_FontSize = newFontSize;
		// file storage of the (possibly) updated font size is done in OnOK() because
		// the user may change the size more than once while the Retranslation dlg is open
		pRetransSizer->Layout(); // redo layout after changing font size
	}
	else
	{
		// Keep it simple, just let the LogUserAction() list know what happened
		// and the text size won't have been altered. The SpinCtrl keeps the range limits
		// correct, so it's unlikely control will ever enter this block
		wxBell();
		wxString msg = _T("Out of range error, in RetranslationDlg, OnSpinValueChanged(event)");
		gpApp->LogUserAction(msg);
	}
}

void CRetranslationDlg::OnCopyRetranslationToClipboard(wxCommandEvent& WXUNUSED(event)) 
{
	if (pRetransBox == NULL)
	{
		::wxBell();
		wxMessageBox(_T("Failure to obtain pointer to the edit box control. No copy was done.\n"),
			_T(""), wxICON_EXCLAMATION | wxOK);
		return;
	}
	pRetransBox->SetFocus();
 	pRetransBox->SetSelection(-1,-1); // -1,-1 selects all
	pRetransBox->Copy(); // copy to the clipboard using wxTextEntry's built in function (CF_TEXT format)

	m_retranslation = pRetransBox->GetValue(); // whm added 21Nov11
	
	int len = m_retranslation.Length();
    // BEW 5Aug22 copying to the clipboard sort of makes the existing retranslation have a validity which
	// should not lightly be made tenuous (as would be the case if -1,-1 was the selection setting). So
	// change here to the cursor being set to the end of the text
    pRetransBox->SetSelection(len,len);
}
void CRetranslationDlg::OnPasteRetranslationFromClipboard(wxCommandEvent& WXUNUSED(event))
{
	if (pRetransBox == NULL)
	{
		::wxBell();
		wxMessageBox(_T("Failure to obtain pointer to the edit box control. No Paste was done.\n"),
			_T(""), wxICON_EXCLAMATION | wxOK);
		return;
	}
	pRetransBox->SetFocus();
	pRetransBox->SetSelection(-1, -1); // -1,-1 selects all -- we want to replace all with clipboard contents
	m_retranslation = pRetransBox->GetValue(); // get the current value, before the Paste is done

	// Get the text from the clipboard, if open
	bool bClipboardTextAbsent = TRUE;
	wxString strClipboardContents = wxEmptyString; // initialise
	if (wxTheClipboard->Open())
	{
		if (wxTheClipboard->IsSupported(wxDF_TEXT))
		{
			wxTextDataObject data;
			wxTheClipboard->GetData(data);
			strClipboardContents = data.GetText();
			if (!strClipboardContents.IsEmpty())
			{
				bClipboardTextAbsent = FALSE;
			}
		}
		wxTheClipboard->Close();
	}
	// Check the clipboard is not empty, if it is, just warn
	if (bClipboardTextAbsent)
	{
		// nothing to do, so tell user
		wxMessageBox(_("The clipboard was empty; there is nothing to Paste."),
			_("No Clipboard Text"), wxICON_INFORMATION | wxOK);
		return;
	}
	if (strClipboardContents != m_retranslation)
	{
		// The clipboard holds text which differs in some way from what is already
		// in the dialog, so do the Paste to get the clipboard contents to replace it
		pRetransBox->Paste(); // paste from the clipboard using wxTextEntry's built in function (CF_TEXT format)
	}
	pRetransBox->SetValue(strClipboardContents); // make sure the box has the new value
	m_retranslation = pRetransBox->GetValue(); // and reset tje member variable to be sure it's correct value

	int len = m_retranslation.Length();
	// BEW 5Aug22 pasting from the clipboard should replace the existing contents. So
	// change here to the cursor being set to the end of the text
	pRetransBox->SetSelection(len, len);
}


void CRetranslationDlg::SyncFontSize(int newSize)
{
	pSpinCtrl->SetValue(newSize);
}

void CRetranslationDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is a method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	// BEW 2Sep22 connect up to the wxSpinCtrl
 
	int newSize = gpApp->m_nRetransDlg_FontSize;
	gpApp->SetFontAndDirectionalityForRetranslationControl(gpApp->m_pTargetFont, pRetransBox, 
					gpApp->m_pRetransFont, newSize); // default is bIsRTL FALSE
	// That has restored the saved fontSize, but the SpinCtrl will still have a default value,
	// so sync the control to the newSize passed in.
	SyncFontSize(newSize);

	pRetransBox->ChangeValue(m_retranslation); // whm added 21Nov11
	pRetransBox->SetFocus();
    //BEW 5Sep22 the text should come up all selected when the dialog is opened
    pRetransBox->SetSelection(-1,-1);
	// when both params of SetSelection above are -1 all text is selected

	pRetransSizer->Layout(); // redo layout after changing anything, including fontSize
}

// whm added this OnOK() handler 13Jan12 to compensate for commenting out the SetValidator()
// calls in the class constructor
void CRetranslationDlg::OnOK(wxCommandEvent& event) 
{
	// Update the file store for the current value font size (int) value - the user may have
	// changed it while in the dialog
	// whm 24Oct2022 change. The current value for font size now only needs to be assigned to
	// the App's m_nRetransDlg_FontSize value, so that it gets saved in the project configuration
	// file AI-ProjectConfiguration.aic.
	gpApp->m_nRetransDlg_FontSize = pSpinCtrl->GetValue();

	//int fontSize = gpApp->m_nRetransDlg_FontSize;
	//wxString strStoreName = gpApp->m_strRetransFontSizeFilename; // it's named _T("retrans_font_size.txt")
	//bool updatedOK = gpApp->UpdateFontSizeStore(strStoreName, fontSize);
	//wxUnusedVar(updatedOK);

	m_retranslation = pRetransBox->GetValue(); // whm added 13Jan12
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

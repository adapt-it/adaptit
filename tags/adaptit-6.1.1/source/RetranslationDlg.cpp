/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			RetranslationDlg.cpp
/// \author			Bill Martin
/// \date_created	21 June 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CRetranslationDlg class. 
/// The CRetranslationDlg class implements the dialog class which provides the
/// dialog in which the user can enter a new translation (retranslation). The
/// source text to be translated/retranslated plus the preceding and following
/// contexts are also displayed.
/// \derivation		The CRetranslationDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in RetranslationDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
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
#include "RetranslationDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CRetranslationDlg, AIModalDialog)
	EVT_INIT_DIALOG(CRetranslationDlg::InitDialog)
	EVT_BUTTON(IDC_COPY_RETRANSLATION_TO_CLIPBOARD, CRetranslationDlg::OnCopyRetranslationToClipboard)
	EVT_BUTTON(IDC_BUTTON_TOGGLE, CRetranslationDlg::OnButtonToggleContext)
END_EVENT_TABLE()


CRetranslationDlg::CRetranslationDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Type Retranslation Text"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	m_retranslation = _T("");
	m_sourceText = _T("");
	m_preContext = _T("");
	m_follContext = _T("");
	m_preContextSrc = _T("");
	m_follContextSrc = _T("");
	m_preContextTgt = _T("");
	m_follContextTgt = _T("");
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pRetransSizer = RetranslationDlgFunc(this, TRUE, TRUE);
	// The declaration is: RetranslationDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	// use pointers to dialog's controls and wxValidator for simple dialog data transfer
	pSrcPrecContextBox = (wxTextCtrl*)FindWindowById(IDC_EDIT_PRECONTEXT);
	//pSrcPrecContextBox->SetValidator(wxGenericValidator(&m_preContext)); // whm removed 21Nov11

	pSrcTextToTransBox = (wxTextCtrl*)FindWindowById(IDC_EDIT_SOURCE_TEXT);
	//pSrcTextToTransBox->SetValidator(wxGenericValidator(&m_sourceText)); // whm removed 21Nov11

	pRetransBox = (wxTextCtrl*)FindWindowById(IDC_EDIT_RETRANSLATION);
	//pRetransBox->SetValidator(wxGenericValidator(&m_retranslation)); // whm removed 21Nov11

	pSrcFollContextBox = (wxTextCtrl*)FindWindowById(IDC_EDIT_FOLLCONTEXT);
	//pSrcFollContextBox->SetValidator(wxGenericValidator(&m_follContext)); // whm removed 21Nov11

}

CRetranslationDlg::~CRetranslationDlg() // destructor
{
	
}

void CRetranslationDlg::OnCopyRetranslationToClipboard(wxCommandEvent& WXUNUSED(event)) 
{
	if (pRetransBox == NULL)
	{
		::wxBell();
		wxMessageBox(_T("Failure to obtain pointer to the edit box control. No copy was done.\n"),
			_T(""), wxICON_EXCLAMATION);
		return;
	}
	pRetransBox->SetFocus();
	pRetransBox->SetSelection(-1,-1); // -1,-1 selects all
	pRetransBox->Copy(); // copy to the clipboard using wxTextCtrl's built in function (CF_TEXT format)

	//TransferDataFromWindow(); // whm removed 21NOv11
	m_preContext = pSrcPrecContextBox->GetValue(); // whm added 21Nov11
	m_follContext = pSrcFollContextBox->GetValue(); // whm added 21Nov11
	m_retranslation = pRetransBox->GetValue(); // whm added 21Nov11
	m_sourceText = pSrcTextToTransBox->GetValue(); // whm added 21Nov11
	
	int len = m_retranslation.Length();
	pRetransBox->SetSelection(len,len);
}

void CRetranslationDlg::OnButtonToggleContext(wxCommandEvent& WXUNUSED(event))
{
	// update m_retranslation, otherwise the UpdateData() call below will clear any retranslation
	// done to date by the user
	m_retranslation = pRetransBox->GetValue();

	wxStaticText* pPreContext = (wxStaticText*)FindWindowById(IDC_STATIC_SRC);
	wxStaticText* pFollContext = (wxStaticText*)FindWindowById(IDC_STATIC_TGT);
	wxString s1;
	// IDS_SOURCE_STR
	s1 = s1.Format(_("Source"));
	wxString s2;
	// IDS_TARGET_STR
	s2 = s2.Format(_("Target"));
	wxString pre;
	wxString foll;
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

	// set the toggle source/target context button's title
	wxString btn;
	wxButton* pBtn = (wxButton*)FindWindowById(IDC_BUTTON_TOGGLE);

	m_bShowingSource = m_bShowingSource ? FALSE : TRUE; // toggle the value

	if (m_bShowingSource)
	{
		// now need to show the source again
		m_preContext = m_preContextSrc;
		m_follContext = m_follContextSrc;

		// set up the two static texts for the preceding and following contexts
		// IDS_STATIC1
		pre = pre.Format(_("%s language's text for the preceding context (including punctuation, if any):"),s1.c_str());
		pPreContext->SetLabel(pre);
		// IDS_STATIC2
		foll = foll.Format(_("%s language's text for the following context (including punctuation, if any):"),s1.c_str());
		pFollContext->SetLabel(foll);
		// IDS_TOGGLE_BTN
		btn = btn.Format(_("Show %s Context"),s2.c_str());
		if (pBtn != 0)
		{
			pBtn->SetLabel(btn);
		}

		// set the font for the preceding context text edit box (assume the fonts were
		// correctly set up by OnInitDialog())
		if (pSrcPrecContextBox != 0)
		{
			pSrcPrecContextBox->SetFont(*pApp->m_pDlgSrcFont);
		}

		// set the font for the following context text edit box
		if (pSrcFollContextBox != 0)
		{
			pSrcFollContextBox->SetFont(*pApp->m_pDlgSrcFont);
		}

#ifdef _RTL_FLAGS
		if (gpApp->m_bSrcRTL)
		{
			pSrcPrecContextBox->SetLayoutDirection(wxLayout_RightToLeft);
		}
		else
		{
			pSrcPrecContextBox->SetLayoutDirection(wxLayout_LeftToRight);
		}

		if (gpApp->m_bSrcRTL)
		{
			pSrcFollContextBox->SetLayoutDirection(wxLayout_RightToLeft);
		}
		else
		{
			pSrcFollContextBox->SetLayoutDirection(wxLayout_LeftToRight);
		}
#endif
	}
	else
	{
		// now need to show the target again
		m_preContext = m_preContextTgt;
		m_follContext = m_follContextTgt;

		// set up the two static texts for the preceding and following contexts
		// IDS_STATIC1
		pre = pre.Format(_("%s language's text for the preceding context (including punctuation, if any):"),s2.c_str());
		pPreContext->SetLabel(pre);
		// IDS_STATIC2
		foll = foll.Format(_("%s language's text for the following context (including punctuation, if any):"),s2.c_str());
		pFollContext->SetLabel(foll);
		// IDS_TOGGLE_BTN
		btn = btn.Format(_("Show %s Context"),s1.c_str());
		if (pBtn != 0)
		{
			pBtn->SetLabel(btn);
		}

		// set the font for the preceding context text edit box
		if (pSrcPrecContextBox != 0)
		{
			pSrcPrecContextBox->SetFont(*pApp->m_pDlgTgtFont);
		}

		// set the font for the following context text edit box
		if (pSrcFollContextBox != 0)
		{
			pSrcFollContextBox->SetFont(*pApp->m_pDlgTgtFont);
		}

#ifdef _RTL_FLAGS
		if (gpApp->m_bTgtRTL)
		{
			pSrcPrecContextBox->SetLayoutDirection(wxLayout_RightToLeft);
		}
		else
		{
			pSrcPrecContextBox->SetLayoutDirection(wxLayout_LeftToRight);
		}

		if (gpApp->m_bTgtRTL)
		{
			pSrcFollContextBox->SetLayoutDirection(wxLayout_RightToLeft);
		}
		else
		{
			pSrcFollContextBox->SetLayoutDirection(wxLayout_LeftToRight);
		}
#endif
	}
	//TransferDataToWindow(); // whm removed 21Nov11
	pSrcPrecContextBox->ChangeValue(m_preContext); // whm added 21Nov11
	pSrcFollContextBox->ChangeValue(m_follContext); // whm added 21Nov11
	pRetransBox->ChangeValue(m_retranslation); // whm added 21Nov11
	pSrcTextToTransBox->ChangeValue(m_sourceText); // whm added 21Nov11

	if (pSrcPrecContextBox != 0)
	{
		// make sure the end of the text is scolled into view
		pSrcPrecContextBox->SetFocus();
		// wxTextCtrl::ShowPosition(long pos) makes the line containing the given position visible
		// and GetLastPosition() returns the last char position existing in the text box.
		pSrcPrecContextBox->ShowPosition(pSrcPrecContextBox->GetLastPosition());
	}
	pRetransSizer->Layout(); // redo layout after toggling different button label
	pRetransBox->SetFocus();
}

void CRetranslationDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is a method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
		
	m_bShowingSource = TRUE;

	// get the display strings set up for the initial state
	m_preContext = m_preContextSrc;
	m_follContext = m_follContextSrc;

	// set up the two static texts for the preceding and following contexts
	wxStaticText* pPreContext = (wxStaticText*)FindWindowById(IDC_STATIC_SRC);
	wxStaticText* pFollContext = (wxStaticText*)FindWindowById(IDC_STATIC_TGT);
	wxString s1;
	// IDS_SOURCE_STR
	s1 = s1.Format(_("Source"));
	wxString pre;
	// IDS_STATIC1
	pre = pre.Format(_("%s language's text for the preceding context (including punctuation, if any):"),s1.c_str());
	pPreContext->SetLabel(pre);
	wxString foll;
	// IDS_STATIC2
	foll = foll.Format(_("%s language's text for the following context (including punctuation, if any):"),s1.c_str());
	pFollContext->SetLabel(foll);

	// set the toggle source/target context button's title
	wxString s2;
	// IDS_TARGET_STR
	s2 = s2.Format(_("Target"));
	wxString btn;
	// IDS_TOGGLE_BTN
	btn = btn.Format(_("Show %s Context"),s2.c_str());
	wxButton* pBtn = (wxButton*)FindWindowById(IDC_BUTTON_TOGGLE);
	if (pBtn != 0)
	{
		pBtn->SetLabel(btn);
	}

	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, pSrcTextToTransBox, pSrcPrecContextBox,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, pSrcTextToTransBox, pSrcPrecContextBox, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif

	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, pSrcFollContextBox, NULL,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, pSrcFollContextBox, NULL, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif

	// make sure the end of the text is scolled into view
	pSrcPrecContextBox->SetFocus();
	pSrcPrecContextBox->ShowPosition(pSrcPrecContextBox->GetLastPosition());

	// set the font for the retranslation edit box & give it the focus
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, pRetransBox, NULL,
								NULL, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bTgtRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, pRetransBox, NULL, 
								NULL, NULL, gpApp->m_pDlgTgtFont);
	#endif

	//TransferDataToWindow(); // make the changes visible - whm removed 21Nov11
	pSrcPrecContextBox->ChangeValue(m_preContext); // whm added 21Nov11
	pSrcFollContextBox->ChangeValue(m_follContext); // whm added 21Nov11
	pRetransBox->ChangeValue(m_retranslation); // whm added 21Nov11
	pSrcTextToTransBox->ChangeValue(m_sourceText); // whm added 21Nov11

	pRetransBox->SetFocus();
	pRetransBox->SetSelection(-1,-1);
	// when both params of SetSelection above are -1 all text is selected
	// MFC SetSel() had third param TRUE indicating not to scroll, but not available in wx.
	//TransferDataToWindow(); // whm removed 21Nov11 redundant call
	pRetransSizer->Layout(); // redo layout after toggling different button label
}

/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			PlaceRetranslationInternalMarkers.cpp
/// \author			Bill Martin
/// \date_created	29 May 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the CPlaceRetranslationInternalMarkers class. 
/// The CPlaceRetranslationInternalMarkers class provides a dialog which is presented to the user
/// during export of the target text in the event that RebuildTargetText() needs user
/// input as to the final placement of markers that were merged together during Retranslation.
/// \derivation		The CPlaceRetranslationInternalMarkers class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in PlaceRetranslationInternalMarkers.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "PlaceRetranslationInternalMarkers.h"
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
#include "PlaceRetranslationInternalMarkers.h"
#include "SourcePhrase.h"
#include "helpers.h"

// globals

extern CSourcePhrase* gpSrcPhrase; // defined in Adapt_ItView.cpp
extern wxString tgtStr; // ditto
extern SPList gSrcPhrases; // ditto

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CPlaceRetranslationInternalMarkers, AIModalDialog)
	EVT_INIT_DIALOG(CPlaceRetranslationInternalMarkers::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_OK, CPlaceRetranslationInternalMarkers::OnOK)
	EVT_BUTTON(IDC_BUTTON_PLACE, CPlaceRetranslationInternalMarkers::OnButtonPlace)
END_EVENT_TABLE()


CPlaceRetranslationInternalMarkers::CPlaceRetranslationInternalMarkers(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Place Standard Format Markers Within Retranslation"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	PlaceInternalMarkersDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_srcPhrase = _T("");
	m_tgtPhrase = _T("");
	
	pTextCtrlAsStaticPlaceIntMkrs = (wxTextCtrl*)FindWindowById(IDC_TEXTCTRL_AS_STATIC_PLACE_INT_MKRS);
	wxASSERT(pTextCtrlAsStaticPlaceIntMkrs != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	pTextCtrlAsStaticPlaceIntMkrs->SetBackgroundColour(backgrndColor);

	// other attribute initializations
}

CPlaceRetranslationInternalMarkers::~CPlaceRetranslationInternalMarkers() // destructor
{
	
}

void CPlaceRetranslationInternalMarkers::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	
	// make the edit boxes & list box use the correct fonts, use default size
	// use the current target language font for the list box, etc
	pEditDisabled = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC);

	// make the fonts show user's desired point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, pEditDisabled, NULL,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, pEditDisabled, NULL, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif

	pListBox = (wxListBox*)FindWindowById(IDC_LIST_MARKERS);
	pEditTarget = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT);

	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, pEditTarget, NULL,
								pListBox, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bTgtRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, pEditTarget, NULL, 
								pListBox, NULL, gpApp->m_pDlgTgtFont);
	#endif

	// set up edit boxes and list box
	wxString markers;
	wxString src = _T("");
	SPList* pList = &gSrcPhrases;
	SPList::Node* pos = pList->GetFirst();
	wxASSERT(pos != NULL);
	bool bFirst = TRUE;
	while (pos != NULL)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();

		// compose the original source text
		if (pSrcPhrase->m_markers.IsEmpty())
		{
			if (src.IsEmpty())
			{
				if (!pSrcPhrase->m_bNullSourcePhrase)
					src += pSrcPhrase->m_srcPhrase;
			}
			else
			{
				if (!pSrcPhrase->m_bNullSourcePhrase)
					src += _T(" ") + pSrcPhrase->m_srcPhrase;
			}
		}
		else
		{
			src += pSrcPhrase->m_markers;
			if (!pSrcPhrase->m_bNullSourcePhrase)
				src += pSrcPhrase->m_srcPhrase;
		}

		// accumulate the list of internal markers
		if (bFirst)
		{
			bFirst = FALSE; // markers of first have been placed already, if any
		}
		else
		{
			markers = pSrcPhrase->m_markers;
			if (!markers.IsEmpty())
				pListBox->Append(markers);
		}
	}

	// hilight first in the listbox
	if (pListBox->GetCount() > 0)
		pListBox->SetSelection(0);

	// set the dialog's member strings for source and target
	m_srcPhrase = src;
	m_tgtPhrase = tgtStr;
	pEditDisabled->SetValue(m_srcPhrase);
	pEditTarget->SetValue(m_tgtPhrase);

}

void CPlaceRetranslationInternalMarkers::OnButtonPlace(wxCommandEvent& WXUNUSED(event))
{
	int nSel;
	nSel = pListBox->GetSelection();
	if (nSel == -1) // LB_ERR
	{
		wxMessageBox(_("List box error when getting the current selection, place manually instead"), _T(""), wxICON_EXCLAMATION);
	}
	m_markers = pListBox->GetString(nSel);


	// get the selection for the CEdit - we are interested in the starting
	// position of the selection
	wxString str;
	long nStartChar;
	long nEndChar;
	int len;
	str = pEditTarget->GetValue();
	// Note: Under wxTextCtrl set for multiline contents, the lines are separated by Unix-style \n
	// characters, even under Windows (where they would be separated by \r\n sequences under MFC).
	len = str.Length();
	pEditTarget->GetSelection(&nStartChar,&nEndChar);
	const wxChar* pBuff = str.GetData(); // assume it won't fail 
	wxChar* pBufStart = (wxChar*)pBuff;
	wxChar* pEnd;
	pEnd = pBufStart + len; // whm added
	wxASSERT(*pEnd == _T('\0'));
	wxChar* ptr = pBufStart;

	// find a suitable place to put the marker(s) - if user clicked on a
	// word, put it at end of preceding word, if contiguous to space, then
	// preceding spaces (and reduce spaces to one)
	
	ptr += nStartChar;
	// whm Note: If m_tgtPhrase is not preceded by a space the following loop
	// could become infinite, so I've recoded it below to be safe.
//a:	if (*ptr != _T(' '))
//	{
//		ptr--;
//		goto a;
//	}
	while (ptr > pBufStart && *ptr != _T(' '))
	{
		ptr--;
	}
	if (*ptr == _T(' '))
	{
		nStartChar = (int)(ptr - pBufStart);
		nEndChar = nStartChar + 1;
	}
	else
	{
		nEndChar = nStartChar; // if something is wrong, just insert at
							   // start of the selection
	}
	pEditTarget->SetSelection(nStartChar,nEndChar);
	// insure m_markers is padded with a space on each end
	m_markers.Trim(FALSE); // trim left end
	m_markers.Trim(TRUE); // trim right end
	m_markers = _T(' ') + m_markers + _T(' ');
	pEditTarget->Replace(nStartChar, nEndChar, m_markers);

	// remove the top item from the list box (ie. the selected item)
	pListBox->Delete(nSel); //int nLeft = m_listBox.DeleteString(nSel);
	if (pListBox->GetCount() > 0)
		pListBox->SetSelection(0);
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CPlaceRetranslationInternalMarkers::OnOK(wxCommandEvent& event) 
{
	m_tgtPhrase = pEditTarget->GetValue();
	tgtStr = m_tgtPhrase;

	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods


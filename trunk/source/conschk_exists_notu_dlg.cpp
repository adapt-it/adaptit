/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			conschk_exists_notu_dlg.cpp
/// \author			Bruce Waters
/// \date_created	1 September 2011
/// \date_revised	
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the ConsChk_Empty_noTU_Dlg class. 
/// The ConsChk_Empty_noTU_Dlg class provides an "inconsistency found" dialog which the
/// user employs for for fixing a KB-Document inconsistency. Deals with the document pile
/// having pSrcPhrase with m_bHasKBEntry (or m_bHasGlossingKBEntry if the current mode is
/// glossing mode) TRUE, but KB lookup failed to find a CTargetUnit for the source text at
/// this location in the document
/// \derivation		The ConsChk_Empty_noTU_Dlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "conschk_exists_notu_dlg.h"
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
//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
//#include <wx/valgen.h> // for wxGenericValidator
#include "Adapt_It.h" // need this for AIModalDialog definition
#include "Adapt_It_wdr.h"
#include "helpers.h"
#include "Adapt_ItDoc.h"
#include "Adapt_ItCanvas.h"
#include "MainFrm.h"
#include "conschk_exists_notu_dlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast
extern bool gbIsGlossing;

// event handler table
BEGIN_EVENT_TABLE(conschk_exists_notu_dlg, AIModalDialog)
	EVT_INIT_DIALOG(conschk_exists_notu_dlg::InitDialog)
	EVT_BUTTON(wxID_OK, conschk_exists_notu_dlg::OnOK)
	//EVT_BUTTON(wxID_CANCEL, conschk_exists_notu_dlg::OnCancel)
END_EVENT_TABLE()

conschk_exists_notu_dlg::conschk_exists_notu_dlg(
		wxWindow* parent,
		wxString* title,
		wxString* srcPhrase,
		wxString* adaptation,
		wxString* notInKBStr,
		bool      bShowCentered) : AIModalDialog(parent, -1, *title, wxDefaultPosition,
					wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pConsChk_exists_notu_dlgSizer = ConsistencyCheck_ExistsNoTU_DlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);

	m_bShowItCentered = bShowCentered;
	m_sourcePhrase = *srcPhrase;
	m_targetPhrase = *adaptation;
	m_notInKBStr = *notInKBStr;
	m_bShowItCentered = bShowCentered;
}

conschk_exists_notu_dlg::~conschk_exists_notu_dlg() // destructor
{
}

void conschk_exists_notu_dlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	m_bDoAutoFix = FALSE; // default
	m_pAutoFixChkBox = (wxCheckBox*)FindWindowById(ID_CHECK_DO_SAME2);
	wxASSERT(m_pAutoFixChkBox != NULL);
	m_pAutoFixChkBox->SetValue(FALSE); // start with it turned off
	actionTaken = no_fix_needed; // temporary default, OnOK() will set it

	m_pTextCtrlSrcText = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SOURCE_PHRASE_2);
	wxASSERT(m_pTextCtrlSrcText != NULL);
	// put the passed in source phrase value into the wxTextCtrl, then make it read only
	m_pTextCtrlSrcText->ChangeValue(m_sourcePhrase);
	m_pTextCtrlSrcText->SetEditable(FALSE); // now it's read-only

	m_pTextCtrlTgtText = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_TARGET_PHRASE_2);
	wxASSERT(m_pTextCtrlTgtText != NULL);
	// put the passed in source phrase value into the wxTextCtrl, then make it read only
	m_pTextCtrlTgtText->ChangeValue(m_targetPhrase);
	m_pTextCtrlTgtText->SetEditable(FALSE); // now it's read-only

	m_pStaticCtrl = (wxStaticText*)FindWindowById(ID_TEXT_EXISTS_STR);
	wxASSERT(m_pStaticCtrl != NULL);

	m_pStoreNormallyRadioBtn = (wxRadioButton*)FindWindowById(ID_RADIO_STORE_NORMALLY);
	wxASSERT(m_pStoreNormallyRadioBtn != NULL);

	m_pNotInKBRadioBtn = (wxRadioButton*)FindWindowById(ID_RADIO_NOT_IN_KB_LEAVEINDOC);
	wxASSERT(m_pNotInKBRadioBtn != NULL);
	m_radioNotInKBLabelStr.Empty();
	// put in the correct string, for this radio button label;  "<Not In KB>" 
	wxString msg2 = m_pNotInKBRadioBtn->GetLabel();
	m_radioNotInKBLabelStr = m_radioNotInKBLabelStr.Format(msg2, m_notInKBStr.c_str());
	m_pNotInKBRadioBtn->SetLabel(m_radioNotInKBLabelStr);
	// get the pixel difference in the label's changed text
	int difference = CalcLabelWidthDifference(msg2, m_radioNotInKBLabelStr, m_pNotInKBRadioBtn);

	// make the fonts show user-defined font point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pTextCtrlSrcText, NULL,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pTextCtrlTgtText, NULL,
								NULL, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bTgtRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pTextCtrlSrcText, NULL, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pTextCtrlTgtText, NULL, 
								NULL, NULL, gpApp->m_pDlgTgtFont);
	#endif

	// get the dialog to resize to the new label string lengths
	int width = 0;
	int myheight = 0;
	this->GetSize(&width, &myheight);
	// use the difference value calculated above to widen the dialog window and then call
	// Layout() to get the attached sizer hierarchy recalculated and laid out
	int sizeFlags = 0;
	sizeFlags |= wxSIZE_USE_EXISTING;
	int clientWidth = 0;
	int clientHeight = 0;
	CMainFrame *pFrame = gpApp->GetMainFrame();
	pFrame->GetClientSize(&clientWidth,&clientHeight);
    // ensure the adjusted width of the dialog won't exceed the client area's width for the
    // frame window
    if (difference < clientWidth - width)
	{
		this->SetSize(wxDefaultCoord, wxDefaultCoord, width + difference, wxDefaultCoord);
	}else
	{
		this->SetSize(wxDefaultCoord, wxDefaultCoord, clientWidth - 2, wxDefaultCoord);
	}
	this->Layout(); // automatically calls Layout() on top level sizer

	if (m_bShowItCentered)
	{
		this->Centre(wxHORIZONTAL);
	}

	{ // reused local var names here, so need a new scope in order to hide them
	// get the dialog to resize to the new label string lengths
	int width = 0;
	int myheight = 0;
	this->GetSize(&width, &myheight);
	// use the difference value calculated above to widen the dialog window and then call
	// Layout() to get the attached sizer hierarchy recalculated and laid out
	int sizeFlags = 0;
	sizeFlags |= wxSIZE_USE_EXISTING;
	int clientWidth = 0;
	int clientHeight = 0;
	CMainFrame *pFrame = gpApp->GetMainFrame();
	pFrame->GetClientSize(&clientWidth,&clientHeight);
    // ensure the adjusted width of the dialog won't exceed the client area's width for the
    // frame window
    if (difference < clientWidth - width)
	{
		this->SetSize(wxDefaultCoord, wxDefaultCoord, width + difference, wxDefaultCoord);
	}else
	{
		this->SetSize(wxDefaultCoord, wxDefaultCoord, clientWidth - 2, wxDefaultCoord);
	}
	this->Layout(); // automatically calls Layout() on top level sizer

	if (m_bShowItCentered)
	{
		this->Centre(wxHORIZONTAL);
	}
	// work out where to place the dialog window
	wxRect rectScreen;
	rectScreen = wxGetClientDisplayRect();

	wxClientDC dc(gpApp->GetMainFrame()->canvas);
	gpApp->GetMainFrame()->canvas->DoPrepareDC(dc);// adjust origin
	gpApp->GetMainFrame()->PrepareDC(dc); // wxWidgets' drawing.cpp sample also 
										  // calls PrepareDC on the owning frame
	int newXPos,newYPos;
	// CalcScrolledPosition translates logical coordinates to device ones. 
	gpApp->GetMainFrame()->canvas->CalcScrolledPosition(m_ptBoxTopLeft.x,
											m_ptBoxTopLeft.y,&newXPos,&newYPos);
	m_ptBoxTopLeft.x = newXPos;
	m_ptBoxTopLeft.y = newYPos;
	// we leave the width and height the same
	gpApp->GetMainFrame()->canvas->ClientToScreen(&m_ptBoxTopLeft.x,
									&m_ptBoxTopLeft.y); // now it's screen coords
	int height = m_nTwoLineDepth;
	wxRect rectDlg;
	GetClientSize(&rectDlg.width, &rectDlg.height); // dialog's window
	rectDlg = NormalizeRect(rectDlg); // in case we ever change from MM_TEXT mode // use our own
	int dlgHeight = rectDlg.GetHeight();
	int dlgWidth = rectDlg.GetWidth();
	wxASSERT(dlgHeight > 0);
	int left = (rectScreen.GetWidth() - dlgWidth)/2;
	if (m_ptBoxTopLeft.y + height < rectScreen.GetBottom() - dlgHeight)
	{
        // put dlg near the bottom of screen (BEW modified 28Feb06 to have -80 rather than
        // -30) because the latter value resulted in the bottom buttons of the dialog being
        // hidden by the status bar at the screen bottom
		//SetSize(left,rectScreen.GetBottom()-dlgHeight-80,540,132,wxSIZE_USE_EXISTING);
		SetSize(left,rectScreen.GetBottom()-dlgHeight-80,wxDefaultCoord,wxDefaultCoord,wxSIZE_USE_EXISTING);
	}
	else
	{
		// put dlg at the top of the screen
		//SetSize(left,rectScreen.GetTop()+40,540,132,wxSIZE_USE_EXISTING);
		SetSize(left,rectScreen.GetTop()+40, wxDefaultCoord,wxDefaultCoord,wxSIZE_USE_EXISTING);
	}
	}
	// It's a simple dialog, I'm not bothering with validators and TransferDataTo/FromWindow calls
	//TransferDataToWindow();
}

void conschk_exists_notu_dlg::OnOK(wxCommandEvent& event)
{
	// get the auto-fix flag
	m_bDoAutoFix = m_pAutoFixChkBox->GetValue();

	// set the fixit action
	if (m_pStoreNormallyRadioBtn->GetValue())
	{
		actionTaken = store_nonempty_meaning;
	}
	else
	{
		actionTaken = make_it_Not_In_KB;
	}
	event.Skip();
}

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FreeTransAdjustDlg.cpp
/// \author			Bruce Waters
/// \date_created	16 November 2013
/// \rcs_id $Id: FreeTransAdjustDlg.cpp 2883 2013-10-14 03:58:57Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the FreeTransAdjustDlg class.
/// The FreeTransAdjustDlg class provides a handler for the "Adjust..." button in the 
/// compose bar. It provides options for the user when his typing of a free translation
/// exceeds the space available for displaying it. Options include joining to the
/// following or previous section, splitting the text (a child dialog allows the user to
/// specify where to make the split), or removing the last word typed and otherwise do
/// nothing except close the dialog - which allows the user to make further edits (for
/// example, to use fewer words that convey the correct meaning) without the dialog
/// forcing itself open (unless the edits again exceed allowed space).
/// The wxDesigner resource is FTAdjustFunc
/// \derivation		The FreeTransAdjustDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "FreeTransAdjustDlg.h"
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
#include "Adapt_It.h"
#include "Adapt_It_wdr.h"
#include "helpers.h"
#include "FreeTrans.h"
#include  "MainFrm.h"
#include "Adapt_ItCanvas.h"
#include "FreeTransAdjustDlg.h"

extern bool gbIsGlossing;

// event handler table
BEGIN_EVENT_TABLE(FreeTransAdjustDlg, AIModalDialog)
	EVT_INIT_DIALOG(FreeTransAdjustDlg::InitDialog)
	EVT_BUTTON(wxID_OK, FreeTransAdjustDlg::OnOK)
	//EVT_BUTTON(wxID_CANCEL, FreeTransAdjustDlg::OnCancel)
	EVT_RADIOBOX(ID_RADIOBOX_ADJUST, FreeTransAdjustDlg::OnRadioBoxAdjust)
END_EVENT_TABLE()

FreeTransAdjustDlg::FreeTransAdjustDlg(
		wxWindow* parent) : AIModalDialog(parent, -1, _("Adjust Section or Typing"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	m_pFreeTransAdjustSizer = FTAdjustFunc(this, TRUE, TRUE);

	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_pApp = &wxGetApp();
	m_pMainFrame = (CMainFrame*)parent;
	m_pFreeTrans = m_pApp->GetFreeTrans(); // for access to the one and only CFreeTrans class's instance
	//bool bOK;
	//bOK = m_pApp->ReverseOkCancelButtonsForMac(this); // <- unneeded so long as we have
														// no Cancel button
	//bOK = bOK; // avoid warning

	CentreOnParent();
}

FreeTransAdjustDlg::~FreeTransAdjustDlg() // destructor
{
}

void FreeTransAdjustDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_pRadioBoxAdjust = (wxRadioBox*)FindWindowById(ID_RADIOBOX_ADJUST);

	// work out where to place the dialog window
	int myTopCoord, myLeftCoord, newXPos, newYPos;
	wxRect rectDlg;
	GetSize(&rectDlg.width, &rectDlg.height); // dialog's window frame
	wxClientDC dc(m_pMainFrame->canvas);
	m_pMainFrame->canvas->DoPrepareDC(dc);// adjust origin
	// wxWidgets' drawing.cpp sample calls PrepareDC on the owning frame
	m_pMainFrame->PrepareDC(dc); 
	// CalcScrolledPosition translates logical coordinates to device ones, m_ptBoxTopLeft
	// has been initialized to the topleft of the cell (from m_pActivePile) where the
	// phrase box currently is
	m_pMainFrame->canvas->CalcScrolledPosition(m_ptBoxTopLeft.x, m_ptBoxTopLeft.y,&newXPos,&newYPos);
	m_pMainFrame->canvas->ClientToScreen(&newXPos, &newYPos); // now it's screen coords
	RepositionDialogToUncoverPhraseBox(m_pApp, 0, 0, rectDlg.width, rectDlg.height,
										newXPos, newYPos, myTopCoord, myLeftCoord);
	SetSize(myLeftCoord, myTopCoord, wxDefaultCoord, wxDefaultCoord, wxSIZE_USE_EXISTING);
}

void FreeTransAdjustDlg::OnOK(wxCommandEvent& event)
{
	// Set radio button selection so the caller of FreeTransAdjustDlg can get the user's
	// desired option
	selection = m_pRadioBoxAdjust->GetSelection();
	event.Skip();
}

//void FreeTransAdjustDlg::OnCancel(wxCommandEvent& event)
//{
//	event.Skip();
//}


void FreeTransAdjustDlg::OnRadioBoxAdjust(wxCommandEvent& WXUNUSED(event))
{
	// Probably don't need this handler
}




/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AIPrintPreviewFrame.cpp
/// \author			Kevin Bradford
/// \date_created	23 September 2011
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General
///                 Public License (see license directory)
/// \description	This is the implementation file for the CAIPrintPreviewFrame class.
/// The CAIPrintPreviewFrame class is the derived from wxPreviewFrame.
/// It allows control of the underlying frame/window
/// during the print preview process.
/// \derivation		The CAIPrintPreviewFrame class is derived from wxPreviewFrame.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "AIPrintPreviewFrame.h"
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

#include <wx/print.h> // for wxPrintPreview

#include "AIPrintPreviewFrame.h"
#include "Adapt_It.h"
#include "Adapt_ItView.h"
#include "FreeTrans.h"
#include "MainFrm.h"

//#define Print_failure

extern bool gbCheckInclFreeTransText;
extern bool gbCheckInclGlossesText;

CAIPrintPreviewFrame::CAIPrintPreviewFrame(
	CAdapt_ItApp* App,
	wxPrintPreviewBase *  preview,
	wxWindow *  parent,
	const wxString &  title,
	const wxPoint &  pos,
	const wxSize &  size,
	long  style,
	const wxString &  name )
	: wxPreviewFrame( preview, parent, title, pos, size, style, name)
{
	pApp = App;
	wxASSERT(pApp != NULL);
	bHideGlossesOnClose = FALSE;
	bHideFreeTranslationsOnClose = FALSE;
#if defined(Print_failure)
#if defined(_DEBUG) && defined(__WXGTK__)
    wxLogDebug(_T("AIPrintPreviewFrame  createor AIPrintPreviewFrame() line 59 at creation: gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d,\n           bHideFreeTranslationsOnClose = %d , m_bFreeTranslationMode = %d"),
               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)pApp->m_bFreeTranslationMode, (int)bHideFreeTranslationsOnClose, (int)pApp->m_bFreeTranslationMode);
#endif
#endif
}

CAIPrintPreviewFrame::~CAIPrintPreviewFrame(void)
{
#if defined(Print_failure)
#if defined(_DEBUG) && defined(__WXGTK__)
    wxLogDebug(_T("AIPrintPreviewFrame  ~AIPrintPreviewFrame() line 62 before SwitchScreenFreeTranslationMode() is called: \n                  gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d, m_bFreeTranslationMode = %d"),
               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)pApp->m_bFreeTranslationMode, (int)pApp->m_bFreeTranslationMode);
#endif
#endif
	if (bHideGlossesOnClose	 == TRUE)
		pApp->GetView()->ShowGlosses();
	if (bHideFreeTranslationsOnClose == TRUE)
		pApp->GetFreeTrans()->SwitchScreenFreeTranslationMode(ftModeOFF);
	if (pApp->m_bFrozenForPrinting)
	{
		pApp->GetMainFrame()->Thaw();
		pApp->m_bFrozenForPrinting = FALSE;
	}
    // BEW added 19Nov11
    gbCheckInclFreeTransText = FALSE; // restore default OFF
    gbCheckInclGlossesText = FALSE; // restore default OFF
#if defined(Print_failure)
#if defined(_DEBUG) && defined(__WXGTK__)
    wxLogDebug(_T("AIPrintPreviewFrame  ~AIPrintPreviewFrame() line 80 after SwitchScreenFreeTranslationMode() is called: \n                  gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d, m_bFreeTranslationMode = %d"),
               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)pApp->m_bFreeTranslationMode, (int)pApp->m_bFreeTranslationMode);
#endif
#endif
}

// Setting this to true will redraw the underlying application view
//    before closing the window, allowing us to print glosses and
//    then hide them and repaint before exiting
void CAIPrintPreviewFrame::HideGlossesOnClose( bool bClose )
{
	bHideGlossesOnClose = bClose;
}
// Setting this to true will redraw the underlying application view
//    before closing the window, allowing us to print Free Translations and
//    then hide them and repaint before exiting
void CAIPrintPreviewFrame::HideFreeTranslationsOnClose( bool bClose )
{
	bHideFreeTranslationsOnClose = bClose;
}

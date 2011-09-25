/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AIPrintPreviewFrame.cpp
/// \author			Kevin Bradford
/// \date_created	23 September 2011
/// \date_revised	
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

CAIPrintPreviewFrame::CAIPrintPreviewFrame(
	CAdapt_ItView * view,
	wxPrintPreviewBase *  preview,  
	wxWindow *  parent,  
	const wxString &  title,  
	const wxPoint &  pos,  
	const wxSize &  size,  
	long  style,  
	const wxString &  name )
	: wxPreviewFrame( preview, parent, title, pos, size, style, name) 
{
	pView = view;
	wxASSERT(pView != NULL);
	bHideGlossesOnClose = FALSE;
}

CAIPrintPreviewFrame::~CAIPrintPreviewFrame(void)
{
	if (bHideGlossesOnClose	 == TRUE)
		pView->ShowGlosses();
	//wxPreviewFrame::~wxPreviewFrame();
}

// Setting this to true will redraw the underlying application view
//    before closing the window, allowing us to show glosses and repaint
//    before exiting
void CAIPrintPreviewFrame::HideGlossesOnClose( bool bClose )
{ 
	bHideGlossesOnClose = bClose;
}

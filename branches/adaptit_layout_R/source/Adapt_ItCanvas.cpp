/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Adapt_ItCanvas.cpp
/// \author			Bill Martin
/// \date_created	05 January 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public 
///                 License (see license directory)
/// \description	This is the implementation file for the CAdapt_ItCanvas class. 
/// The CAdapt_ItCanvas class implements the main Adapt It window based on
/// wxScrolledWindow. This is required because wxWidgets' doc/view framework
/// does not have an equivalent for the CScrolledView in MFC.
/// \derivation		The CAdapt_ItCanvas class is derived from wxScrolledWindow.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in MainFrm (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Adapt_ItCanvas.h"
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
#include <wx/docview.h>	// lost a lot of time trying to track down obscure compillation
						// errors after forgetting to include this! It is needed even
						// for the class to be able to recognize wxView and wxDocument.
#ifdef _USE_SPLITTER_WINDOW
#include <wx/splitter.h> // for wxSplitterWindow
#endif

#include "Adapt_It.h"
#include "Adapt_ItDoc.h"
#include "EarlierTranslationDlg.h"
#include "MainFrm.h"
#include "Adapt_ItView.h"
#include "Adapt_ItCanvas.h"
//#include "SourceBundle.h"
#include "Cell.h"
#include "Pile.h"
#include "Strip.h"
#include "NoteDlg.h"
#include "ViewFilteredMaterialDlg.h"

/// This global is defined in Adapt_ItView.cpp (for vertical edit functionality)
extern bool gbVerticalEditInProgress;

/// This global is defined in Adapt_ItView.cpp
extern EditStep gEditStep;

/// This global is defined in Adapt_ItView.cpp
extern EditRecord gEditRecord;

/// This global is defined in FindReplace.cpp.
extern bool gbReplaceAllIsCurrent;

/// This global is defined in PhraseBox.cpp.
extern long	gnStart;

/// This global is defined in PhraseBox.cpp.
extern long gnEnd;

extern bool gbHaltedAtBoundary;

/// This global is defined in Adapt_ItView.cpp.
extern int gnBeginInsertionsSequNum;

extern int gnEndInsertionsSequNum;

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; 

/// This global is defined in Adapt_ItView.cpp.
extern int gnOldSequNum;

/// This global is defined in Adapt_It.cpp.
extern CPile* gpNotePile;

/// This global is defined in Adapt_It.cpp.
extern wxPoint gptLastClick;

/// This global is defined in Adapt_It.cpp.
extern bool gbBundleStartIteratingBack;

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbShowTargetOnly;

extern const wxChar* filterMkr;
extern const wxChar* filterMkrEnd;

/// This global is defined in Adapt_It.cpp.
extern CPile* gpGreenWedgePile;

/// This global is defined in Adapt_It.cpp.
extern bool gbSuppressSetup;

//extern wxString gFreeTranslationStr;

/// This global is defined in PhraseBox.cpp.
extern wxString translation;

/// This global is defined in Adapt_It.cpp.
extern wxArrayPtrVoid* gpCurFreeTransSectionPileArray; // new creates on heap in InitInstance, and disposes in ExitInstance

extern bool	gbFindIsCurrent;
extern bool gbFindOrReplaceCurrent;
extern bool gbMatchedRetranslation;
extern int gnRetransEndSequNum; // sequ num of last srcPhrase in a matched retranslation
extern bool gbJustReplaced;
extern bool gbSaveSuppressFirst; // save the toggled state of the lines in the strips (across Find or
extern bool gbSaveSuppressLast;  // Find and Replace operations)
extern wxRect grectViewClient;

/// This global is defined in Adapt_It.cpp.
extern bool	gbRTL_Layout;	// ANSI version is always left to right reading; this flag can only
							// be changed in the Unicode version, using the extra Layout menu

extern bool gbIsPrinting;

/// This global is defined in MainFrm.cpp.
extern bool gbIgnoreScriptureReference_Receive;

/// This global is defined in MainFrm.cpp.
extern bool gbIgnoreScriptureReference_Send;

/// This global is defined in Adapt_It.cpp.
//extern CAdapt_ItApp* gpApp; // for rapid access to the app class

// IMPLEMENT_CLASS(CAdapt_ItCanvas, wxScrolledWindow)
IMPLEMENT_DYNAMIC_CLASS(CAdapt_ItCanvas, wxScrolledWindow)

// For drawing lines in a canvas
float xpos = -1;
float ypos = -1;


// event handler table
BEGIN_EVENT_TABLE(CAdapt_ItCanvas, wxScrolledWindow)
	//EVT_PAINT(CAdapt_ItCanvas::OnPaint) // Not needed for Adapt It - see note in OnPaint() handler
    EVT_PAINT(CAdapt_ItCanvas::OnPaint) // whm added 28May07
	//EVT_SIZE(CAdapt_ItCanvas::OnSize) // Not needed - see note in OnSize() handler
	//EVT_MENU(ID_SOME_MENU_ITEM, OnDoSomething)
	//EVT_UPDATE_UI(ID_SOME_MENU_ITEM, OnUpdateDoSomething)
	// ... other menu, button or control events
    //EVT_MOUSE_EVENTS(CAdapt_ItCanvas::OnMouseEvent)

	// wx Note: wxScrollEvent only appears to intercept scroll events for scroll bars manually
	// placed in wxWindow based windows. In order to handle scroll events for windows like
	// wxScrolledWindow, we must use wxScrollWinEvent in the functions and EVT_SCROLLWIN macro
	// here. 
    EVT_SCROLLWIN(CAdapt_ItCanvas::OnScroll)
	EVT_LEFT_DOWN(CAdapt_ItCanvas::OnLButtonDown)
	EVT_LEFT_UP(CAdapt_ItCanvas::OnLButtonUp)
	EVT_MOTION(CAdapt_ItCanvas::OnMouseMove) // whm added to activate OnMouseMove; MFC just had "ON_WM_MOUSEMOVE()"
END_EVENT_TABLE()

CAdapt_ItCanvas::CAdapt_ItCanvas()
{
}

#ifdef _USE_SPLITTER_WINDOW
CAdapt_ItCanvas::CAdapt_ItCanvas(wxSplitterWindow *splitter, 
	const wxPoint& pos, const wxSize& size, const long style)
	: wxScrolledWindow(splitter, wxID_ANY, pos, size, style)
#else
CAdapt_ItCanvas::CAdapt_ItCanvas(CMainFrame *frame, 
	const wxPoint& pos, const wxSize& size, const long style)
	: wxScrolledWindow(frame, wxID_ANY, pos, size, style)
#endif
{
	pView = NULL; // pView is set in the View's OnCreate() method Make CAdapt_ItCanvas' view pointer point to incoming view pointer
	pFrame = NULL; // pFrame is set in CMainFrame's 
}

CAdapt_ItCanvas::~CAdapt_ItCanvas(void)
{
}

// event handling functions

// whm Note: Whether using OnDraw() or OnPaint() the result is the same
//void CAdapt_ItCanvas::OnDraw(wxDC& dc) // note the & rather than *
//{
//	// wx behavior: This OnDraw() handler is called whenever our canvas scrolled
//	// window needs updating, i.e., when the scrollbar is changed, when the window 
//	// frame is resized, or when another window uncovers part or all of the canvas
//	// window.
//	// wx Note: The wx docs say that when implementing doc/view, the window on which 
//	// drawing is done needs to call the View's OnDraw() within the window's OnDraw
//	// method.
//	// wx Note: The wxScrollHelper::HandleOnPaint calls this canvas' OnDraw()
//	// method when the window needs painting. The HandleOnPaint routine calls
//	// DoPrepareDC beforehand. The wx docs say that DoPrepareDC "sets the
//	// device origin according to the current scroll position." However, the
//	// wx docs also say about wxScrolledWindow::OnDraw(wxDC& dc), that it is "Called
//	// by the default paint event handler to allow the application to define painting
//	// behaviour without having to worry about calling wxScrolledWindow::DoPrepareDC. 
//	// Instead of overriding this function, you may also just process the paint event
//	// in the derived class as usual, but then you will have to call DoPrepareDC()
//	// yourself."
//	if (pView)
//	{
//#ifdef _LOG_DEBUG_DRAWING
//		wxLogDebug(_T("Canvas calling View OnDraw()"));
//#endif
//		pView->OnDraw(& dc);
//	}
//}

void CAdapt_ItCanvas::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC paintDC(this);//wxAutoBufferedPaintDC paintDC(this);
	// whm 9Jun07 Note: use of wxAutoBufferedPaintDC() is supposed to recognize when
	// double buffering is being done by the system's graphics primitives, and avoids
	// adding another buffered layer. Using it here did not affect wxMac's problem
	// of failure to paint properly after scrolling.

#if wxUSE_GRAPHICS_CONTEXT
     wxGCDC gdc( paintDC ) ;
    wxDC &dc = m_useContext ? (wxDC&) gdc : (wxDC&) paintDC ;
#else
    wxDC &dc = paintDC ;
#endif
	if (!dc.IsOk())// using dc to avoid compiler warning
	{
		wxLogDebug(_T("canvas OnPaint() reports dc is not Ok!"));
	}
    DoPrepareDC(paintDC); // PrepareDC() now calls DoPrepareDC()
	
	if (pView)
	{
#ifdef _LOG_DEBUG_DRAWING
		wxLogDebug(_T("Canvas calling View OnDraw()"));
#endif
		pView->OnDraw(& paintDC);
	}
}


// whm Notes on DoPrepareDC():
// DoPrepareDC() is declared within the class WXDLLEXPORT wxScrollHelper in the scrolwin.h 
// header file in the ...\include\wx folder. The signature is: 
// virtual void DoPrepareDC(wxDC& dc);
// Since DoPrepareDC() is a virtual function, it would seem necessary to call its base class
// wxScrolledWindow::DoPrepareDC(dc); at the beginning of the CAdapt_ItCanvas::DoPrepareDC(wxDC& dc)
// override.
// The implementation of DoPrepareDC() in the scrlwing.cpp file in the ...\src\generic folder. 
// See the implementation code below.
// According to the internal comment, the #ifdef directive should probably be 
// removed and the wxLayout_RightToLeft flag taken into consideration for all ports, 
// not just wxGTK.
// the wxWidgets docs say, "DoPrepareDC is called automatically within the default
// wxScrolledWindow::OnPaint event handler, so your wxScrolledWindow::OnDraw override 
// will be passed a 'pre-scrolled' device context. However, if you wish to draw from 
// outside of OnDraw (via OnPaint), or you wish to implement OnPaint yourself, you must 
// call this function yourself."
//
// The MFC docs say of CView::OnPrepareDC(), "Called by the framework before the OnDraw 
// member function is called for screen display and before the OnPrint member function is 
// called for each page during printing or print preview....The default implementation 
// of this function does nothing if the function is called for screen display. However, 
// this function is overridden in derived classes, such as CScrollView, to adjust 
// attributes of the device context; consequently, you should always call the base class 
// implementation at the beginning of your override."

//
// The code for the wxWidgets' DoPrepareDC() (from scrlwing.cpp) is simply this:
/*
void wxScrollHelper::DoPrepareDC(wxDC& dc)
{
    wxPoint pt = dc.GetDeviceOrigin();
#ifdef __WXGTK__
    // It may actually be correct to always query
    // the m_sign from the DC here, but I leve the
    // #ifdef GTK for now.
    if (m_win->GetLayoutDirection() == wxLayout_RightToLeft)
        dc.SetDeviceOrigin( pt.x + m_xScrollPosition * m_xScrollPixelsPerLine,
                            pt.y - m_yScrollPosition * m_yScrollPixelsPerLine );
    else
#endif
        dc.SetDeviceOrigin( pt.x - m_xScrollPosition * m_xScrollPixelsPerLine,
                            pt.y - m_yScrollPosition * m_yScrollPixelsPerLine );
    dc.SetUserScale( m_scaleX, m_scaleY );
}

MFC's ScrollView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo) method looks like this (from viewscrl.cpp):

void CScrollView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
	ASSERT_VALID(pDC);

#ifdef _DEBUG
	if (m_nMapMode == MM_NONE)
	{
		TRACE(traceAppMsg, 0, "Error: must call SetScrollSizes() or SetScaleToFitSize()");
		TRACE(traceAppMsg, 0, "\tbefore painting scroll view.\n");
		ASSERT(FALSE);
		return;
	}
#endif //_DEBUG
	ASSERT(m_totalDev.cx >= 0 && m_totalDev.cy >= 0);
	switch (m_nMapMode)
	{
	case MM_SCALETOFIT:
		pDC->SetMapMode(MM_ANISOTROPIC);
		pDC->SetWindowExt(m_totalLog);  // window is in logical coordinates
		pDC->SetViewportExt(m_totalDev);
		if (m_totalDev.cx == 0 || m_totalDev.cy == 0)
			TRACE(traceAppMsg, 0, "Warning: CScrollView scaled to nothing.\n");
		break;

	default:
		ASSERT(m_nMapMode > 0);
		pDC->SetMapMode(m_nMapMode);
		break;
	}

	CPoint ptVpOrg(0, 0);       // assume no shift for printing
	if (!pDC->IsPrinting())
	{
		ASSERT(pDC->GetWindowOrg() == CPoint(0,0));

		// by default shift viewport origin in negative direction of scroll
		ptVpOrg = -GetDeviceScrollPosition();

		if (m_bCenter)
		{
			CRect rect;
			GetClientRect(&rect);

			// if client area is larger than total device size,
			// override scroll positions to place origin such that
			// output is centered in the window
			if (m_totalDev.cx < rect.Width())
				ptVpOrg.x = (rect.Width() - m_totalDev.cx) / 2;
			if (m_totalDev.cy < rect.Height())
				ptVpOrg.y = (rect.Height() - m_totalDev.cy) / 2;
		}
	}
	pDC->SetViewportOrg(ptVpOrg);

	CView::OnPrepareDC(pDC, pInfo);     // For default Printing behavior
}

In addition, the base class' CView::OnPrepareDC(pDC, pInfo) code (from viewcore.cpp) doesn't really do 
anything much but looks like this:

void CView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
	ASSERT_VALID(pDC);
	UNUSED(pDC); // unused in release builds

	// Default to one page printing if doc length not known
	if (pInfo != NULL)
		pInfo->m_bContinuePrinting =
			(pInfo->GetMaxPage() != 0xffff || (pInfo->m_nCurPage == 1));
}

*/

void CAdapt_ItCanvas::DoPrepareDC(wxDC& dc) // this is called OnPrepareDC() in MFC
{
    // See notes above comparing MFC's OnPrepareDC and wxWidgets' DoPrepareDC. Here are more notes on
    // MFC's usage of OnPrepareDC. The MFC's OnPrepareDC() is called in the following situations:
	
    //    OnPrepareDC() is called TWICE at the beginning of each page during printing and print
    //    previewing in the following calling sequence (from MFC's viewprnt.cpp library source file):
    //      OnPrepareDC(&dcPrint, &printInfo);
    //      dcPrint.StartPage()
    //      OnPrepareDC(&dcPrint, &printInfo); [called again because StartPage resets the device attributes]
    //    In contrast, in the wx printing framework, its version of OnPrepareDC called DoPrepareDC() is
    //    not called explicitly at all anywhere that I can find in its print routines.
	
	//    The View's OnDraw(CDC* pDC) in the following calling sequence:
	//		CClientDC viewDC((CWnd*)m_pBundle->m_pView);
	//		m_pBundle->m_pView->OnPrepareDC(&viewDC);
	//		m_pBundle->m_pView->GetClientRect(&grectViewClient); // set the global rect used for speeding drawing
	//		viewDC.DPtoLP(&grectViewClient); // get the point converted to logical coords
	//		// draw the layout
	//		m_pBundle->Draw(pDC);
	
	//    The View's CreateBox(...) in the following calling sequence:
	//		// convert to device coords
	//		CClientDC aDC(this);
	//		OnPrepareDC(&aDC); // adjust origin
	//		aDC.LPtoDP(&rectBox);

	//    The View's ResizeBox(...) in the following calling sequence:
	//		// convert to device coords
	//		CClientDC aDC(this);
	//		OnPrepareDC(&aDC); // adjust origin
	//		CPoint ptrLoc = *pLoc;
	//		aDC.LPtoDP(&ptrLoc);

	//    The View's RecalcLayout(...) in the following calling sequence:
	//		// get a device context, and get the origin adjusted (gRectViewClient is ignored when printing)
	//		CClientDC viewDC(this);
	//		OnPrepareDC(&viewDC);
	//		GetClientRect(&grectViewClient); // set the global rect used for speeding drawing
	//		viewDC.DPtoLP(&grectViewClient); // get the point converted to logical coords

	//    The View's OnLButtonDown(...) in the following calling sequence:
	//		// get the point into logical coordinates
	//		CClientDC aDC(this); // make a device context
	//		OnPrepareDC(&aDC); // get origin adjusted
	//		aDC.DPtoLP(&point); // get the point converted to logical coords

	//    The View's OnLButtonUp(...) in the following calling sequence:
	//		CClientDC aDC(this); // make a device context
	//		OnPrepareDC(&aDC); // get origin adjusted
	//		aDC.DPtoLP(&point); // get the point converted to logical coords

	//    The View's OnMouseMove(...) in the following calling sequence:
	//		CClientDC aDC(this); // make a device context
	//		OnPrepareDC(&aDC); // get origin adjusted
	//		aDC.DPtoLP(&point); // get the point converted to logical coords

	//    The View's SelectDragRange(...) in the following calling sequence:
	//		SelectAnchorOnly(); // reduce to just the anchor, before we rebuild the selection
	//		CClientDC aDC(this); // make a device context
	//		OnPrepareDC(&aDC); // get origin adjusted
	//		COLORREF oldBkColor = aDC.SetBkColor(RGB(255,255,0)); // yellow

	//    The View's SelectAnchorOnly(...) in the following calling sequence:
	//		CClientDC aDC(this); // make a device context
	//		OnPrepareDC(&aDC); // get origin adjusted

	//    The View's LayoutStrip(...) in the following calling sequence:
	//		// get a device context, and get the origin adjusted
	//		CClientDC viewDC(this);
	//		OnPrepareDC(&viewDC);

	//    The View's OnEnChangeEditBox(...) in the following calling sequence:
	//		CDC* pDC = this->GetDC();
	//		OnPrepareDC(pDC);
	//		DrawFreeTranslations(pDC, m_pBundle, call_from_edit);
	//		this->ReleaseDC(pDC);

	//    The View's ExtendSelectionRight(...) in the following calling sequence:
	//		OnPrepareDC(&aDC); // adjust the origin
	//		... before various pCell->DrawCell(&aDC); calls

	//    The View's ExtendSelectionLeft(...) in the following calling sequence:
	//		OnPrepareDC(&aDC); // adjust the origin
	//		... before various pCell->DrawCell(&aDC); calls

	//    The View's AdjustDialogPosition(...) in the following calling sequence:
	//		CClientDC dc(pView);
	//		pView->OnPrepareDC(&dc); // adjust origin
	//		// use location where phrase box would be put
	//		dc.LPtoDP(&ptBoxTopLeft); // now it's device coords
	//		pView->ClientToScreen(&ptBoxTopLeft); // now it's screen coords

	//    The View's GetVisibleStrips(...) in the following calling sequence:
	//		CClientDC dc(this);
	//		OnPrepareDC(&dc); // adjust origin
	//		// find the index of the first strip which is visible
	//		...
	//		CRect rectClient;
	//		GetClientRect(&rectClient); // view's client area (device coords)
	//		dc.DPtoLP(&rectClient);

	//	  The ConsistencyCheckDlg.cpp's OnInitDialog() in the following calling sequenct:
	//		CClientDC dc(m_pView);
	//		m_pView->OnPrepareDC(&dc); // adjust origin
	//		dc.LPtoDP(&m_ptBoxTopLeft); // now it's device coords
	//		m_pView->ClientToScreen(&m_ptBoxTopLeft); // now it's screen coords

	//	  The FindReplace.cpp's OnFindNext() in the following calling sequenct:
	//		CClientDC dc(m_pParent);
	//		m_pParent->OnPrepareDC(&dc); // adjust origin

	//	  The PhraseBox.cpp's OnChar() in the following calling sequenct:
	//		pView->RemoveSelection();
	//		CClientDC dC(pView);
	//		pView->OnPrepareDC(&dC); // adjust origin
	//		pView->Invalidate();

	// whm note: The wx version doesn't need to do anything in this override of DoPrepareDC. In fact
	// DoPrepareDC doesn't have to be overridden in the wx version. We'll leave this here for the sake
	// of the above notes and just call the base class method.
	
	wxScrolledWindow::DoPrepareDC(dc);

}


// wx Note: The wxScrollEvent only appears to intercept scroll events for scroll bars 
// manually placed in wxWindow based windows. In order to handle scroll events for windows like
// wxScrolledWindow (that have built-in scrollbars like our canvas), we must use wxScrollWinEvent 
// in the functions and the EVT_SCROLLWIN macro in the event table.
// We don't actually use this since our base wxScrolledWindow handles scroll events in response
// to clicking on the thumb, arrows, or the paging parts of the canvas' scrollbar.
void CAdapt_ItCanvas::OnScroll(wxScrollWinEvent& event)
{
	//pApp->GetMainFrame()->SendSizeEvent();

	event.Skip();	// this is necessary for the built-in scrolling behavior of wxScrolledWindow
					// to be processed
}

bool CAdapt_ItCanvas::IsModified() const
{

	return FALSE;
}

//void CAdapt_ItCanvas::Modify(bool mod)
//{
//
//}

void CAdapt_ItCanvas::DiscardEdits()
{
}

void CAdapt_ItCanvas::OnLButtonDown(wxMouseEvent& event)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	CLayout* pLayout = pApp->m_pLayout;
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	wxASSERT(pView != NULL);

	wxPoint point;	// MFC passes CPoint point as second parameter to OnLButtonDown; wx gets it from the
					// wxMouseEvent via GetPosition. Both specify x an y coordinates relative to the upper
					// left corner of the window
					// Note: wxMouseEvent also has GetLogicalPosition() that automaticall translates to logical
					// position for the DC.
	point = event.GetPosition();	// GetPosition() gets the point on the view port relative to upper-left 
									// corner of the window.
	gptLastClick = point;	// used by AdjustDialogByClick; needs to be device coords (not logical coords) there
							// so we'll calculate logical coords for point below

	// BEW added 6Mar06: check if the point clicked was outside of an open note dialog - we'll
	// assume this is indended as a click to open a different note dialog without losing the note
	// from the current note dialog which is about to become disposed of - we have to restore the
	// text in that one as filtered note text at that source phrase location before the new
	// one is opened. If the click was not intended that way, then too bad - the note dialog
	// will close as if the OK button was pressed, which is safe because no information is lost
	wxRect dlgRect;
	if (pApp->m_pNoteDlg != NULL)
	{
		// a note dialog is open, so check for a click outside it's rectangle
		// whm note: since the MFC app's OnLButtonDown is in the View, It would seem the
		// following call to GetWindowRect() would wrongly assign the dimensions of the View's 
		// window rather than the m_pNoteDlg's window to dlgRect. Wouldn't the correct MFC call be
		// m_pNoteDlg->GetWindowRect(&dlgRect)? The MFC version seems to react correctly, however,
		// so there must be something I'm not seeing correctly about the following call to GetWindowRect.
		// For the wx version, I here call GetRect on the dialog's window pointer m_pNoteDlg.
		// Not sure why MFC versions works, but this does too, so I'll do it this way.
		dlgRect = pApp->m_pNoteDlg->GetRect(); //GetWindowRect(&dlgRect); // gets it as screen coords

		// if the point is not in this rect, then close it as if OK was pressed
		if (!dlgRect.Contains(point))
		{
			wxCommandEvent cevent(wxID_OK);
			pApp->m_pNoteDlg->OnOK(cevent);
			pApp->m_pNoteDlg = NULL;
			gpNotePile = NULL;
		}
	}

	// BEW moved the next 3 lines of code to the top from its earlier position
	// further down, so that clicking in a wedge will use logical coordinates when
	// determining which strip's wedge was clicked in
	gbBundleStartIteratingBack = FALSE; // BEW added 27Jun05 for free trans support, make sure it is
										// always FALSE when OnLButtonDown() is entered
	// get the point into logical coordinates
	wxClientDC aDC(this); // make a device context
	DoPrepareDC(aDC); // get origin adjusted (calls wxScrolledWindow::DoPrepareDC)
	//aDC.DPtoLP(&point); // get the point converted to logical coords - use CalcUnscrolledPosition below
	//int x = aDC.DeviceToLogicalX(point.x);// get the device X coord converted to logical coord
	//int y = aDC.DeviceToLogicalY(point.y);// get the device Y coord converted to logical coord
	//point.x = x;
	//point.y = y;

//#ifdef _DEBUG
//	// next bit is for verification of logical calculations
//	wxPoint logicalPoint(event.GetLogicalPosition(aDC)); // alternate for verification
//	// The CalcUnscrolledPosition() function also translates device coordinates
//	// to logical ones.
//	int newXPos,newYPos;
//	// In the next statement point should be device/screen coords because it was determined before DoPrepareDC(aDC)
//	// The logical position point determine by GetLogicalPosition above should be same as newXPos,newYPos
//	CalcUnscrolledPosition(point.x,point.y,&newXPos,&newYPos);
//	wxASSERT(newXPos == logicalPoint.x); //point.x = newXPos;
//	wxASSERT(newYPos == logicalPoint.y); //point.y = newYPos;
//#endif
	
	// we don't need to call CalcUnscrolledPosition here because GetLogicalPosition already
	// provides logical coordinates for the clicked point; wxPoint in device coords was needed
	// above to set the gptLastClick (used in AdjustDialogByClick), so we'll get the logical
	// coords of the point here.
	wxPoint logicalPoint(event.GetLogicalPosition(aDC));
	point = logicalPoint;

	//int xScrollUnits, yScrollUnits, xOrigin, yOrigin;
	//GetViewStart(&xOrigin, &yOrigin); // gets xOrigin and yOrigin in scroll units
	//GetScrollPixelsPerUnit(&xScrollUnits, &yScrollUnits); // gets pixels per scroll unit
	//// the point is in reference to the upper left position (origin)
	//point.x += xOrigin * xScrollUnits; // number pixels is ScrollUnits * pixelsPerScrollUnit
	//point.y += yOrigin * yScrollUnits;

//#ifdef _DEBUG
//	// the values set by CalcUnscrolledPosition should now equal those determined by calcs directly above
//	wxASSERT(newXPos == point.x);
//	wxASSERT(newYPos == point.y);
//#endif

	#ifdef _Trace_Click_FT
	/*
	TRACE2("OnLButtonDown after DPtoLP: point x = %d, y = %d\n", point.x,point.y);
	CSize siz = GetTotalSize();
	TRACE2("Scroll size: cx = %d, cy = %d\n",siz.cx,siz.cy);
	CPoint posn = GetScrollPosition();
	TRACE2("Scroll posn: x = %d, y = %d\n",posn.x,posn.y);
	*/
	#endif

    // a left button click must always halt auto-matching and inserting, so set the flag
    // false to cause this to happen; ditto for Replace All if it is in progress
	pApp->m_bAutoInsert = FALSE;
	gbReplaceAllIsCurrent = FALSE; // turn off Replace All
	
	CMainFrame *pFrame = pApp->GetMainFrame();
	pFrame = pFrame; // suppresses "local variable is initialized but not referenced" warning 
	wxASSERT(pFrame != NULL);
	wxASSERT(pFrame->m_pComposeBar != NULL);
	wxTextCtrl* pEditCompose = (wxTextCtrl*)
								pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
	wxString message;
	wxString chVerse;

    // BEW added 19 Apr 05 for support of clicking on the green wedge to open the dialog
    // for viewing filtered material. We allow a click anywhere in the rectangle defined by
    // the location and dimensions of the wedge to be treated as a valid click within the
    // wedge icon itself; updated 02Aug05 to handle RTL rendering in the Unicode version as
    // well (green wedge is at the pile's right for RTL layout)
	// Updated 07Sept05 to handle clicking in the peach coloured Note icon's rectangle to 
	// open a note window.
	CStrip* pClickedStrip = pView->GetNearestStrip(&point);

	wxPoint ptWedgeTopLeft;
	wxPoint ptWedgeBotRight;
	wxPoint ptNoteTopLeft;
	wxPoint ptNoteBotRight;
	if (pClickedStrip != NULL)
	{
		//ptWedgeTopLeft.x = pClickedStrip->m_rectStrip.GetLeft();
		//ptWedgeTopLeft.y = pClickedStrip->m_rectStrip.GetTop();
		ptWedgeTopLeft.x = pClickedStrip->GetStripRect_CellsOnly().GetLeft();
		ptWedgeTopLeft.y = pClickedStrip->GetStripRect_CellsOnly().GetTop();
		ptNoteTopLeft = ptWedgeTopLeft;
		ptWedgeBotRight = ptWedgeTopLeft;
		ptNoteBotRight = ptNoteTopLeft;
		int numPiles;

		// first check for and handle green wedge clicks, then check for and handle 
		// note clicks
		ptWedgeBotRight.y -= 2; // this will set y coord for bottom of the wedge's rect 
                                // - one pixel lower than the tip of the wedge, but still
                                // not in the cell below it
		ptWedgeTopLeft.y -= 7; // this gets the y coordinate set correctly for the top of 
                               // the wedge's rect, the x-coord of topleft of cells will
                               // give us the x coords we need, and we can assume the right
                               // boundaries are 9 pixels to the right of those for
                               // notes...
		ptNoteTopLeft.y -= 9; // sets the y coord for top of the note icon's rectangle, 
                              // ptNoteBotRight.y is already correct

		// get the number of piles in the clicked strip
#ifdef _ALT_LAYOUT_
		numPiles = pClickedStrip->GetPileIndicesCount();
#else
		numPiles = pClickedStrip->GetPileCount();
#endif
        // do the check only if we have not suppressed showing source text, and only if
        // there actually are piles in the strip (yeah, I know, there has to be; but safety
        // first isn't a bad idea)
		if (numPiles > 0 && !gbShowTargetOnly)
		{
			CPile* pPile = NULL;
			int indexPile;
			for (indexPile = 0; indexPile < numPiles; indexPile++)
			{
#ifdef _ALT_LAYOUT_
				pPile = pClickedStrip->GetPileByIndexInStrip(indexPile);
#else
				pPile = pClickedStrip->GetPileByIndex(indexPile);
#endif
				wxASSERT(pPile != NULL);

                // is there anything filtered here - if not, check for a note, if that
                // fails too, then just continue the loop
				if (pPile->GetSrcPhrase()->m_markers.Find(filterMkr) >= 0)
				{
					// there is some filtered material stored on this 
					// CSourcePhrase instance
					int xLeft;
					int xRight;
					wxRect pileRect = pPile->GetPileRect();
					xLeft = pileRect.GetLeft();

                    // if the Unicode application and we are laying out RTL, the wedge will
                    // be at the other end of the pile - so if this is the case, then set
                    // xLeft to be the x coord of the right boundary
					#ifdef _RTL_FLAGS
					if (gbRTL_Layout)
					{
						xLeft = pileRect.GetRight();
					}
					#endif
                    // BEW changed 16Jul05 to make the clickable rectagle a pixel wider all
                    // round so that less motor control is needed for the wedge click
					// wx note: The following is wxRect ok, since adjustments are made to the
					// upper left and lower right points to form a new wedgeRect
					xLeft -= 4;
					ptWedgeTopLeft.y -= 2; // BEW changed 18Nov05 to accomodate solid down 
										   // arrow design, was -= 1
					xRight = xLeft + 11;
					ptWedgeBotRight.y += 1;
					ptWedgeTopLeft.x = xLeft ; 
					ptWedgeBotRight.x = xRight;									
					wxRect wedgeRect(ptWedgeTopLeft, ptWedgeBotRight);

					// we have the required rectangle where the click would need to occur,
					// now test to see if the click actually occurred within it
					if (wedgeRect.Contains(point))
					{
						// user clicked in this wedge - so open the dialog

                        // keep track of the sequ num of the src phrase whose m_markers is
                        // being viewed and potentially edited in the
                        // ViewFilteredMarkersDlg. Since the dialog is non-modal, we need a
                        // way to identify the source phrase whose m_markers member is to
                        // be updated after edit (edit update is done in
                        // CViewFilteredMaterialDlg's OnBnClickedOK).
						pApp->m_nSequNumBeingViewed = pPile->GetSrcPhrase()->m_nSequNumber;

                        // BEW added 11Oct05, to allow clicked wedge's topmost cell in the
                        // layout to be given background highlighting, so user has visual
                        // feedback as to which pile the View Filtered Material dialog
                        // pertains to
						gpGreenWedgePile = pPile;

                        // We always completely destroy each instance of
                        // CViewFilteredMaterialDlg before creating another one
						if (pApp->m_pViewFilteredMaterialDlg != NULL)
						{
                            // user has clicked on another green wedge with the current
                            // modeless dialog still open. Assume the user did not intend
                            // to save any changes (since he failed to click on OK to save
                            // them before clicking on another green wedge. In such cases
                            // we'll just destroy the window, so a new one can be created
                            // with data for the new location.
							wxCommandEvent cevent(wxID_CANCEL);
							pApp->m_pViewFilteredMaterialDlg->OnCancel(cevent); // calls Destroy()
						}
						if (pApp->m_pNoteDlg != NULL)
						{
							return; // the Note dialog is open, so prevent a green wedge
									// click from opening the View Filtered Material dialog
						}
						if (pApp->m_pViewFilteredMaterialDlg == NULL)
						{
							pApp->m_pViewFilteredMaterialDlg = new 
												CViewFilteredMaterialDlg(pApp->GetMainFrame());
							// wx version: we don't need to call Create() for modeless dialog below:
							pView->AdjustDialogPositionByClick(pApp->m_pViewFilteredMaterialDlg,
														gptLastClick); // avoid click location
							pApp->m_pViewFilteredMaterialDlg->Show(TRUE);
						}
						// after user has finished with the dialog, we've nothing more to
						// do in OnLButtonDown() and so we return to the caller
						Refresh(); // get a refresh done
						return;
					}
					else
					{
						// no click in this wedge, so continue check for a note icon click
						goto u; 
					}
				} // end block for a pile with filtered material
				else
				{
                    // control will enter this block always via the above goto u; statement
                    // because the note text is always filtered; but this syntax below is
                    // useful because we can test for note clicks even when there is no
                    // note info stored yet
u:					if (pPile->GetSrcPhrase()->m_bHasNote)
					{
						// this pile has a note, so check if the click was in the note 
						// icon's rectangle
						int xLeft;
						int xRight;
						wxRect pileRect = pPile->GetPileRect();
						xLeft = pileRect.GetLeft();
						#ifdef _RTL_FLAGS
						if (gbRTL_Layout)
						{
							xLeft = pileRect.GetRight() + 6;
						}
						else
						{
							xLeft -= 13;
						}
						#else
							xLeft -= 13;
						#endif
						xRight = xLeft + 10; // at least 9,  but an extra one to make it 
											 // easier to hit
						ptNoteTopLeft.x = xLeft ; 
						ptNoteBotRight.x = xRight;									
                        // wx note: The following is wxRect ok, since adjustments are made
                        // to the upper left and lower right points to form a new wedgeRect
						wxRect noteRect(ptNoteTopLeft, ptNoteBotRight);

						// check if the click was in noteRect
						if (noteRect.Contains(point)) //if (noteRect.PtInRect(point))
						{
							// user clicked in the note icon - so open the note window
							if (pApp->m_pViewFilteredMaterialDlg != NULL)
								return; // if the green wedge dialog is open, prevent a 
										// Note icon click from opening the Note dialog

                            // BEW added 6Mar06 to cause return without any change if the
                            // user clicked on the note icon for a note already opened.
                            // Without this change the click has the effect of reopening
                            // the dialog empty and the note text has irretreivably been
                            // lost -- this block of code is never entered because the
                            // block at the very top of OnLButtonDown() detects the click
                            // outside the note dialog boundary and closes the note and
                            // saves its note text, so by the time control gets here,
                            // m_pNoteDlg will be NULL and gpNotePile will be NULL. I'll
                            // leave this code here because it is defensive, and if the
                            // earlier block was ever removed then we'll still have the
                            // required insurance for inadventent loss of a note
							if (pApp->m_pNoteDlg != NULL && pPile == gpNotePile)
							{
                                // if the note dialog is already open and the clicked
                                // pile's pointer is the same pointer as for the currently
                                // opened note dialog's pile, then return without doing
                                // anything other than a beep and restoring the focus to
                                // the note dialog
								::wxBell();
								wxTextCtrl* pEdit = (wxTextCtrl*)
									pApp->m_pNoteDlg->FindWindowById(IDC_EDIT_NOTE);
								pEdit->SetFocus();
								return;
							}

                            // keep track of the sequ num of the src phrase whose
                            // m_markers' filtered note information is being viewed and
                            // potentially edited in the note window. Since the dialog is
                            // non-modal, we need a way to identify the source phrase whose
                            // m_markers member is to be updated after edit (edit update is
                            // done in ...).
							pApp->m_nSequNumBeingViewed = pPile->GetSrcPhrase()->m_nSequNumber;

                            // BEW added 11Oct05, to allow clicked wedge's topmost cell in
                            // the layout to be given background highlighting, so user has
                            // visual feedback as to which pile the View Filtered Material
                            // dialog pertains to
							gpNotePile = pPile;

							// We always completely destroy each instance of CNoteDlg
							// before creating another one
							if (pApp->m_pNoteDlg != NULL)
							{
                                // user has clicked on a note icon with the current
                                // modeless note dialog still open. BEW changed 5Mar06: a
                                // click on a different note icon, with a note dialog open,
                                // should save the currently open dialog's note text as if
                                // he has clicked the OK button. Why? Because that does not
                                // destroy any info, and it is far more likely he'd want to
                                // keep the note, and is just wanting to move quickly to
                                // the next one. So, to get rid of the note, force him to
                                // do the action to explicitly remove it; but a click
                                // elsewhere should never be such an action. -- Tried
                                // implementing it here but failed. So I'll try do it by
                                // intercepting a click outside the note dialog itself and
                                // invoking the OnBnCLickedOk() function if that has
                                // happened.
                                // (Legacy behaviour: Assume the user did not intend to
                                // save any changes (since he failed to click on OK to save
                                // them before clicking).In such cases we'll just destroy
                                // the window, so a new one can be created with data for
                                // the new location.)
								pApp->m_pNoteDlg->Destroy();
								pApp->m_pNoteDlg = NULL;
							}
							if (pApp->m_pNoteDlg == NULL)
							{
								pApp->m_pNoteDlg = new CNoteDlg(this);
								// As with the ViewFilteredMaterialDlg, the modeless NoteDlg 
								// doesn't need a Create() call.
								pView->AdjustDialogPositionByClick(pApp->m_pNoteDlg,
													gptLastClick); // avoid click location
								pApp->m_pNoteDlg->Show(TRUE);
							}
                            // after user has finished with the dialog, we've nothing more
                            // to do in OnLButtonDown() and so we return to the caller
							Refresh(); // get a refresh done
							return;
						}
					}
					else
					{
						continue;
					}
				}
			} // end of loop for checking each pile of the strip
		}
	}
	
    // a left click in the view when the "View Earlier Translation" modeless dialog is the
    // active window, must make the view window the active one
	bool bMadeViewActive = FALSE;
	if (pApp->m_pEarlierTransDlg != NULL)
	{
		// Need to conditional compile HWND for different platforms here and below:
		if (!(pApp->m_pEarlierTransDlg->GetHandle() == NULL))
		{
            // WX Note: ::IsWindow() is not available in wxWidgets but it should be
            // sufficient here to just check if the Dlg window is being shown
			if (pApp->m_pEarlierTransDlg->IsShown())
			{
				pApp->m_pEarlierTransDlg->Show(FALSE); // hide the dialog window
				bMadeViewActive = TRUE;

				if (pApp->m_pTargetBox == NULL)
					goto y; // check, just in case, and do the longer cleanup if the box 
							// is not there
				// restore focus to the targetBox
				if (pApp->m_pTargetBox != NULL)
				{
					if (pApp->m_pTargetBox->IsShown())
					{
						pApp->m_pTargetBox->SetSelection(gnStart,gnEnd);
						pApp->m_pTargetBox->SetFocus();
					}
				}
y:				; // I may put some code here later
			}
		}
	}

    // a left click in the view when a find or replace modeless dialog is the active
    // window, must hide the dialog window and cause the phrase box to be set up at
    // whatever location was the last active one - to do this, copy & adjust the code in
    // OnCancel for CFindReplace whm adjusted the following to accommodate different
    // modeless dialogs for "Find" (pApp->m_pFindDlg) and "Find and Replace"
    // (pApp->m_pReplaceDlg).
	if (pApp->m_pFindDlg != NULL || pApp->m_pReplaceDlg != NULL || bMadeViewActive)
	{
		if (pApp->m_pFindDlg == NULL && pApp->m_pReplaceDlg == NULL && bMadeViewActive)
			return;
		if (pApp->m_pFindDlg != NULL || pApp->m_pReplaceDlg != NULL)
		{
			if ((pApp->m_pFindDlg != NULL && pApp->m_pFindDlg->IsShown()) || 
				(pApp->m_pReplaceDlg != NULL && pApp->m_pReplaceDlg->IsShown()))
			{
				if (pApp->m_pFindDlg != NULL && pApp->m_pFindDlg->IsShown())
					pApp->m_pFindDlg->Show(FALSE); // hide the dialog window
				if (pApp->m_pReplaceDlg != NULL && pApp->m_pReplaceDlg->IsShown())
					pApp->m_pReplaceDlg->Show(FALSE); // hide the dialog window
				gbFindIsCurrent = FALSE;
				gbFindOrReplaceCurrent = FALSE;

				// clear the globals
				gbMatchedRetranslation = FALSE;
				gnRetransEndSequNum = -1;

				if (gbJustReplaced)
				{
                    // we have cancelled just after a replacement, so we expect the phrase
                    // box to exist and be visible, so we only have to do a little tidying
                    // up before we return
					if (pApp->m_pTargetBox == NULL)
						goto x; // check, just in case, and do the longer cleanup if the 
                                // box is not there
					// restore focus to the targetBox
					if (pApp->m_pTargetBox != NULL)
					{
						if (pApp->m_pTargetBox->IsShown())
						{
							pApp->m_pTargetBox->SetSelection(gnStart,gnEnd);
							pApp->m_pTargetBox->SetFocus();
						}
					}
					gbJustReplaced = FALSE; // clear to default value
				}
				else
				{
                    // we have tried a FindNext since the previous replacement, so we
                    // expect the phrase box to have been destroyed by the time we enter
                    // this code block; so place the phrase box, if it has been destroyed
                    // whm note 12Aug08. Since the MFC version expects the phrase box to be
                    // NULL here, but in the wx version it never is null, we will remove
                    // the == NULL test here.
					//if (pApp->m_pTargetBox == NULL)
					//{
x:						CCell* pCell = 0;
						CPile* pPile = 0;
						if (!pApp->m_selection.IsEmpty())
						{
							CCellList::Node* cpos = pApp->m_selection.GetFirst();
							pCell = (CCell*)cpos->GetData(); // could be on any line
							wxASSERT(pCell);
							pPile = pCell->GetPile();
						}
						else
						{
                            // no selection, so find another way to define active location
                            // & place the phrase box
							int nCurSequNum = pApp->m_nActiveSequNum;
							if (nCurSequNum == -1)
							{
								nCurSequNum = pApp->GetMaxIndex(); // make active loc the last 
																// src phrase in the doc
								pApp->m_nActiveSequNum = nCurSequNum;
							}
							else if (nCurSequNum >= 0 && nCurSequNum <= pApp->GetMaxIndex())
							{
								pApp->m_nActiveSequNum = nCurSequNum;
							}
							else
							{
								// if all else fails, go to the start
								pApp->m_nActiveSequNum = 0;
							}
							pPile = pView->GetPile(pApp->m_nActiveSequNum);
						}
						CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();

                        // pPile is what we will use for the active pile, so set everything
                        // up there, provided it is not in a retranslation - if it is,
                        // place the box preceding it, if possible, else after it; but if
                        // we are glossing, then ignore the fact of the retranslation,
                        // since we can have a phrasebox within a retranslation when
                        // glossing
						CPile* pSavePile = pPile;
						while (!gbIsGlossing && pSrcPhrase->m_bRetranslation)
						{
							pPile = pView->GetPrevPile(pPile);
							if (pPile == NULL)
							{
								// if we get to the start, try again, going the other way
								pPile = pSavePile;
								while (pSrcPhrase->m_bRetranslation)
								{
									pPile = pView->GetNextPile(pPile);
									wxASSERT(pPile); // we'll assume this will never fail
									pSrcPhrase = pPile->GetSrcPhrase();
								}
								break;
							}
							pSrcPhrase = pPile->GetSrcPhrase();
						}
						pSrcPhrase = pPile->GetSrcPhrase();
						pApp->m_nActiveSequNum = pSrcPhrase->m_nSequNumber;
						pApp->m_pActivePile = pPile;
						pCell = pPile->GetCell(1); // we want the 2nd line, for phrase box
						//pApp->m_ptCurBoxLocation = pCell->m_ptTopLeft;

						// save old sequ number in case required for toolbar's Back button
						gnOldSequNum = pApp->m_nActiveSequNum;

						// place the phrase box
						pView->PlacePhraseBox(pCell,2);

                        // get a new active pile pointer, the PlacePhraseBox call did a
                        // recal of the layout
						pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
						wxASSERT(pApp->m_pActivePile);

						// scroll into view, just in case (but shouldn't be needed)
						ScrollIntoView(pApp->m_nActiveSequNum);
						pView->Invalidate(); // get window redrawn

						// restore focus to the targetBox
						if (pApp->m_pTargetBox != NULL)
						{
							if (pApp->m_pTargetBox->IsShown())
							{
								pApp->m_pTargetBox->SetSelection(gnStart,gnEnd);
								pApp->m_pTargetBox->SetFocus();
							}
						}
					//} // end block for null phrase box handle or phrase box not a window
					//else
					//{
					//	// phrase box exists so restore focus to the targetBox
					//	if (pApp->m_pTargetBox != NULL)
					//	{
					//		if (pApp->m_pTargetBox->IsShown())
					//		{
					//			pApp->m_pTargetBox->SetSelection(gnStart,gnEnd);
					//			pApp->m_pTargetBox->SetFocus();
					//		}
					//	}
					//}
					gbHaltedAtBoundary = FALSE;

					/* we don't have 5 lines now, only 3
					// toggle back to earlier number of lines per strip
					if (gbSaveSuppressFirst)
						pView->ToggleSourceLines();
					if (gbSaveSuppressLast)
						pView->ToggleTargetLines();
					*/
					return; // otherwise, we would go on to process the click, which we don't 
							// want to do
				} // end block for a click after a FindNext (which means phrase box will have 
				  // been destroyed)
			} // end block for FindReplace visible
		} // end block for FindReplace dialog not null when user clicked
	} // end block for find or replace active, or view has just been reactivated

	// I want to make the background of a clicked cell be yellow

	if (pApp->m_pLayout == NULL) return;
	if (pApp->m_pLayout->GetStripCount() == 0) return;

	CCell* pAnchor = pApp->m_pAnchor; // anchor cell, for use when extending 
                                      // selection (null if no selection current)
	int    nAnchorSequNum = -1; // if no anchor defined, set value -1; we can test 
                                //for this value later
	if (pAnchor != NULL)
	{
		nAnchorSequNum = pAnchor->GetPile()->GetSrcPhrase()->m_nSequNumber; // there 
			// is a pre-existing selection or at least the anchor click to make one
		wxASSERT(nAnchorSequNum >= 0);
	}

	CSourcePhrase* pCurSrcPhrase; // used in loop when extending selection
	int sequNum;

	// find which cell the click was in
	CCell* pCell = pView->GetClickedCell(&point); // returns NULL if click was 
												  // not in a cell
    // BEW added 03Aug08: disallow a click in the gray text area (preceding or following
    // context) during vertical editing mode; I'll code this block as if I was supporting
    // adaptations or glosses or free translations as entry points too, but for MFC it will
    // only be source text editing as the entry point, but the code will work even with the
    // extra stuff in it
	if (gbVerticalEditInProgress && pCell != NULL)
	{
		int nClickedSequNum = pCell->GetPile()->GetSrcPhrase()->m_nSequNumber;
		if (gEditStep == adaptationsStep && gEditRecord.bAdaptationStepEntered)
		{
            // use the bounds for the editable span to test if the click was in the gray
            // text before the left bound, or in the gray text following the right bound;
            // if either is true, warn the user and disallow the click
			if (nClickedSequNum < gEditRecord.nAdaptationStep_StartingSequNum ||
				nClickedSequNum > gEditRecord.nAdaptationStep_EndingSequNum)
			{
				// IDS_CLICK_IN_GRAY_ILLEGAL
				wxMessageBox(_(
"Attempting to put the active location within the gray text area while updating information in Vertical Edit mode is illegal. The attempt has been ignored."),
				_T(""), wxICON_WARNING);
				pApp->m_pTargetBox->SetFocus();
				return;
			}
		}
		else if (gEditStep == glossesStep && gEditRecord.bGlossStepEntered)
		{
			if (nClickedSequNum < gEditRecord.nGlossStep_StartingSequNum ||
				nClickedSequNum > gEditRecord.nGlossStep_EndingSequNum)
			{
				// IDS_CLICK_IN_GRAY_ILLEGAL
				wxMessageBox(_(
"Attempting to put the active location within the gray text area while updating information in Vertical Edit mode is illegal. The attempt has been ignored."),
				_T(""), wxICON_WARNING);
				pApp->m_pTargetBox->SetFocus();
				return;
			}
		}
		else if (gEditStep == freeTranslationsStep && 
					gEditRecord.bFreeTranslationStepEntered)
		{
			if (nClickedSequNum < gEditRecord.nFreeTranslationStep_StartingSequNum ||
				nClickedSequNum > gEditRecord.nFreeTranslationStep_EndingSequNum)
			{
				// IDS_CLICK_IN_GRAY_ILLEGAL
				wxMessageBox(_(
"Attempting to put the active location within the gray text area while updating information in Vertical Edit mode is illegal. The attempt has been ignored."),
				_T(""), wxICON_WARNING);
				pApp->m_pTargetBox->SetFocus();
				return;
			}
		}
	}

	// we may be going to drag-select, so prepare for drag
	gbHaltedAtBoundary = FALSE;
	//	TRACE1("OnLButtonDown: gbHalted %d\n",gbHaltedAtBoundary);
	//if (pCell != NULL && (pCell->GetCellIndex() == 0 || pCell->GetCellIndex() == 1))
	if (pCell != NULL && pCell->GetCellIndex() == 0)
	{
        // if we have a selection and shift key is being held down, we assume user is not
        // dragging, but rather is extending an existing selection, so check for this and
        // exit block if so
		if (event.ShiftDown() && pApp->m_selection.GetCount() > 0)
			goto t;
		// remove any old selection & update window immediately
		// TRACE1("OnLButtonDown()  about to call RemoveSelection(); m_selection's count is %d\n",
		//																	m_selection.GetCount());
		pView->RemoveSelection();
		Refresh();
		// TRACE0("View 26\n");

		// prepare for drag
		pApp->m_mouse = point;
		CaptureMouse(); //on Win32, SetCapture() is called via CaptureMouse() 
						// and DoCaptureMouse() in wxWidget sources wincmn.cpp 
						// and window.cpp
	}

t:	if (pCell == NULL)
	{
		// click was not in a cell, so just remove any existing selection
		pView->RemoveSelection();

		// click was not in a cell, so allow removal of the automatically 
		// inserted highlighting on target/glosses. This is effected
		// by clearing the following two globals to -1 values.
		gnBeginInsertionsSequNum = -1;
		gnEndInsertionsSequNum = -1;

		pApp->m_bSelectByArrowKey = FALSE;
		Refresh(); //Invalidate(); // must force a redraw, or else the selection 
                   // stays on the screen (UpdateWindow() doesn't work here)

        // can't initiate a drag selection unless we click on a cell, so clear drag support
        // variables too
		pApp->m_mouse.x = pApp->m_mouse.y = -1;
	}
	else // pCell is not NULL, that is, a cell was clicked on
	{
		if (event.ShiftDown())
		{
            // shift key is down, so extend the selection if there is an existing one on
            // the matching line
			pApp->m_bSelectByArrowKey = FALSE;
			if (pApp->m_selectionLine == -1)
			{
				// no current selection, so treat the SHIFT+click as an ordinary 
				// unshifted click
				goto a;
			}
			else
			{
                // there is a current selection, so extend it but only provided the click
                // was on the same line as the current selection is on
				wxASSERT(pApp->m_selection.GetCount() != 0);

				// local variables to use in the loops below
				CPile*	pEndPile;
				CPile*	pCurPile; // the one we use in the loop, starting from pOldSel's pile
				CStrip* pCurStrip; // the strip the starting pile is in
				CCell*	pCurCell; // the current cell in the current pile (used in loop)
				int nCurPileCount; // how many piles in current strip
				int nCurPile; // index of current pile (in loop)
				int nCurStrip; // index of current strip in which is the current pile

                // set all the above local variables from pCell and pAnchor (anchor is the
                // cell first clicked, pCell is the one at the end of the extend or drag)
				pEndPile = pCell->GetPile();
				pCurPile = pAnchor->GetPile();
				pCurStrip = pCurPile->GetStrip();
#ifdef _ALT_LAYOUT_
				nCurPileCount = pCurStrip->GetPileIndicesCount();
#else
				nCurPileCount = pCurStrip->GetPileCount();
#endif
				nCurPile = pCurPile->GetPileIndex(); // value of m_nPile
				nCurStrip = pCurStrip->GetStripIndex(); // value of m_nStrip

				if (pApp->m_selectionLine != pCell->GetCellIndex())
					goto b; // delete old selection then do new one, because lines not the same
				else
				{
					// its on the same line, so we can extend it
					sequNum = pCell->GetPile()->GetSrcPhrase()->m_nSequNumber;
					if (sequNum >= nAnchorSequNum)
					{
                        // we are extending forwards (to the "right" in logical order, but
                        // in the view it is rightwards for LTR layout, but leftwards for
                        // RTL layout)
						if (pApp->m_selectionLine == 0)
						{
                            // source text line, but respect boundaries, unless
                            // m_bRespectBoundaries is FALSE; first determine if the
                            // anchor cell is at a boundary, if it is then we cannot
                            // extend the selection in which case just do nothing
                            // except warn the user
							if (pApp->m_bRespectBoundaries)
							{
								if (pView->IsBoundaryCell(pAnchor) && pCell != pAnchor)
								{
									// warn user
									// IDS_CANNOT_EXTEND_FWD
									wxMessageBox(_(
"Sorry, but the application will not allow you to extend a selection forwards across any punctuation unless you use a technique for ignoring a boundary as well."),
									_T(""), wxICON_INFORMATION);
									event.Skip();
									return;
								}
							}

                            // first remove any selection preceding the anchor, since
                            // click was forward of the anchor
							pView->RemovePrecedingAnchor(&aDC, pAnchor);

							// extend the selection
							int sequ = nAnchorSequNum;
							while (sequ < sequNum)
							{
								sequ++; // next one

								// get the next pile
								if (nCurPile < nCurPileCount-1)
								{
									// there is at least one more pile in this strip, 
									// so access it
									nCurPile++; // index of next pile
#ifdef _ALT_LAYOUT_
									pCurPile = pCurStrip->GetPileByIndexInStrip(nCurPile);
#else
									pCurPile = pCurStrip->GetPileByIndex(nCurPile);
#endif
									pCurSrcPhrase = pCurPile->GetSrcPhrase();
									wxASSERT(pCurSrcPhrase->m_nSequNumber == sequ); // must match
									pCurCell = pCurPile->GetCell(pApp->m_selectionLine); // get the cell

									// if it is already selected, continue to next one, 
									// else select it
									if (!pCurCell->IsSelected())
									{
										aDC.SetBackgroundMode(pApp->m_backgroundMode);
										aDC.SetTextBackground(wxColour(255,255,0)); // yellow
										pCurCell->DrawCell(&aDC, pLayout->GetSrcColor());
										pCurCell->SetSelected(TRUE);

										// keep a record of it
										pApp->m_selection.Append(pCurCell);
									}

									// if we have reached a boundary, then break out, 
									// otherwise continue
									if (pView->IsBoundaryCell(pCurCell))
									{
										break;
									}
								} // end of block for test "cell isn't the strip's last"
								else
								{
                                    // we have reached the end of the strip, so go to
                                    // start of next
									nCurPile = 0; // first in next strip
									nCurStrip++; // the next strip's index
									// get the pointer to next strip
									pCurStrip = pLayout->GetStripByIndex(nCurStrip);
									// get the pointer to its first pile
#ifdef _ALT_LAYOUT_
									pCurPile = pCurStrip->GetPileByIndexInStrip(nCurPile);
									nCurPileCount = pCurStrip->GetPileIndicesCount(); // update 
											// so test above remains correct for the strip
#else
									pCurPile = pCurStrip->GetPileByIndex(nCurPile);
									nCurPileCount = pCurStrip->GetPileCount(); // update 
											// so test above remains correct for the strip
#endif
									pCurSrcPhrase = pCurPile->GetSrcPhrase();
									wxASSERT(pCurSrcPhrase->m_nSequNumber == sequ);
									// get the required cell if it is already selected, 
									// & continue to next one, else select it
									pCurCell = pCurPile->GetCell(pApp->m_selectionLine);
									if (!pCurCell->IsSelected())
									{
										aDC.SetBackgroundMode(pApp->m_backgroundMode);
										aDC.SetTextBackground(wxColour(255,255,0)); // yellow
										pCurCell->DrawCell(&aDC, pLayout->GetSrcColor());
										pCurCell->SetSelected(TRUE);

										// keep a record of it
										pApp->m_selection.Append(pCurCell);
									}

									// if we have reached a boundary, then break out, 
									// otherwise continue
									if (pView->IsBoundaryCell(pCurCell))
									{
										break;
									}
								}
							}
                            // user may have shortened an existing selection, so check
                            // for any selected cells beyond the last one clicked, and
                            // if they exist then remove them from the list and
                            // deselect them.
							CCell* pEndCell = pEndPile->GetCell(pApp->m_selectionLine);
							pView->RemoveLaterSelForShortening(&aDC, pEndCell);
						} // end of block for test that selectionLine is 0
						else
						{
							// second or third line, a shift click here does nothing as yet.
							;
						}
					} // end of block for extending to higher sequence numbers 
					  // (ie. visibly right for LTR layout, but visibly left for RTL layout)
					  
					// else extend to lower sequence numbers...
					else
					{
                        // we are extending backwards (ie. to the "left" for logical order,
                        // but in the view it is left for LTR layout, but right for RTL
                        // layout); ie. moving to lower sequ numbers
						if (pApp->m_selectionLine == 0)
						{
                            // block for source text selection extending the selection
                            // backwards; take boundaries into account, provided
                            // m_RespectBoundaries is TRUE; first determine if the anchor
                            // cell follows a boundary, if it is then we cannot extend the
                            // selection backwards, in which case just do nothing except
                            // warn the user
							CCell* pPrevCell;
							pPrevCell = pView->GetPrevCell(pAnchor, pApp->m_selectionLine);
							if (pApp->m_bRespectBoundaries)
							{
								if (pView->IsBoundaryCell(pPrevCell))
								{
									// warn user
									// IDS_CANNOT_EXTEND_BACK
									wxMessageBox(_(
"Sorry, it is not possible to extend the selection backwards at this location unless you use one of the methods for ignoring a boundary."),
									_T(""), wxICON_INFORMATION);
									event.Skip();
									return;
								}
							}
							  
							// first if there are any cells selected beyond 
							// the anchor cell, then get rid of them
							pView->RemoveFollowingAnchor(&aDC, pAnchor);

							int sequ = nAnchorSequNum; // starting point
							while (sequ > sequNum)
							{
								sequ--; // next one to the left

								// get the previous pile
								if (nCurPile > 0)
								{
									// there is at least one previous pile in this strip, 
									// so access it
									nCurPile--; // index of previous pile
#ifdef _ALT_LAYOUT_
									pCurPile = pCurStrip->GetPileByIndexInStrip(nCurPile);
#else
									pCurPile = pCurStrip->GetPileByIndex(nCurPile);
#endif
									pCurSrcPhrase = pCurPile->GetSrcPhrase();
									wxASSERT(pCurSrcPhrase->m_nSequNumber == sequ); // must match
									pCurCell = pCurPile->GetCell(pApp->m_selectionLine); // get the cell

									// if it is a boundary then we must break out 
									// of the loop
									if (pApp->m_bRespectBoundaries)
									{
										if (pView->IsBoundaryCell(pCurCell))
											break;
									}

									// if it is already selected, continue to next prev one, 
									// else select it
									if (!pCurCell->IsSelected())
									{
										aDC.SetBackgroundMode(pApp->m_backgroundMode);
										aDC.SetTextBackground(wxColour(255,255,0)); // yellow
										pCurCell->DrawCell(&aDC, pLayout->GetSrcColor());
										pCurCell->SetSelected(TRUE);

										// keep a record of it, retaining order of words/phrases
										pApp->m_selection.Insert(pCurCell);
									}
								}
								else
								{
                                    // we have reached the start of the strip, so go to end
                                    // of previous strip
									nCurStrip--; // the previous strip's index
									pCurStrip = pLayout->GetStripByIndex(nCurStrip); // prev strip 
#ifdef _ALT_LAYOUT_
									nCurPileCount = pCurStrip->GetPileIndicesCount(); // update 
											// this so test above remains correct for the strip
									nCurPile = nCurPileCount-1; // last in this strip
									pCurPile = pCurStrip->GetPileByIndexInStrip(nCurPile);  // pointer 
																			 // to its last pile
#else
									nCurPileCount = pCurStrip->GetPileCount(); // update this so 
													//test above remains correct for the strip
									nCurPile = nCurPileCount-1; // last in this strip
									pCurPile = pCurStrip->GetPileByIndex(nCurPile);  // pointer 
																			 // to its last pile
#endif
									pCurSrcPhrase = pCurPile->GetSrcPhrase();
									wxASSERT(pCurSrcPhrase->m_nSequNumber == sequ);
									// get the required cell
									pCurCell = pCurPile->GetCell(pApp->m_selectionLine);

									// if it is a boundary then we must break out of the loop
									if (pApp->m_bRespectBoundaries)
									{
										if (pView->IsBoundaryCell(pCurCell))
											break;
									}

									// if it is already selected, continue to next prev one, 
									// else select it
									if (!pCurCell->IsSelected())
									{
										aDC.SetBackgroundMode(pApp->m_backgroundMode);
										aDC.SetTextBackground(wxColour(255,255,0)); // yellow
										pCurCell->DrawCell(&aDC, pLayout->GetSrcColor());
										pCurCell->SetSelected(TRUE);

										// keep a record of it, preserving order of words/phrases
										pApp->m_selection.Insert(pCurCell);
									}
								}
							} // end sequ > sequNum  test block

                            // user may have shortened an existing selection, so check for
                            // any selected cells previous to the last one clicked, and if
                            // they exist then remove them from the list and deselect them.
							CCell* pEndCell = pEndPile->GetCell(pApp->m_selectionLine);
							pView->RemoveEarlierSelForShortening(&aDC,pEndCell);
						}  // end of block for test selectionLine == 0
						else
						{
                            // one of the target language lines, -- behaviour yet to be
                            // determined probably just ignore the click
							;
						}
					} // end of block for extending to lower sequence numbers 
					  // (ie. visibly left for LTR layout but visibly right for RTL layout)
				} // end block for a "same line" click which means extension of selection can 
				  // be done
			} // end block for extending a selection
		} // end of block for a click with SHIFT key down - for extending selection
		else
		{
			// SHIFT key is not down

            // found the cell, and the shift key is not down, so remove the old selection
            // (or shift key was down, but clicked cell was not on same line of a strip)
			if (pCell->GetCellIndex() == 1)
			{
                // second line - the phrase box's line (always): a click here places the
                // phraseBox in that cell clicked, unless the cell is part of a
                // retranslation
				CPile* pRetrPile = pCell->GetPile();
				wxASSERT(pRetrPile);
				if (!gbIsGlossing && pRetrPile->GetSrcPhrase()->m_bRetranslation)
				{
                    // make any single pile within a retranslation (other than a click in
                    // line 0 which causes a selection) inaccessible - user should
                    // treat a retranslation as a whole, & access it via toolbar buttons
					if (!pApp->m_bFreeTranslationMode) // BEW added 8Jul05 to allow making 
											// a retranslation pile the anchor location for 
											// a free translation by a click
					{
						// IDS_NO_ACCESS_TO_RETRANS
						::wxBell(); // a ding here might help too
						wxMessageBox(_(
"Sorry, to edit or remove a retranslation you must use the toolbar buttons for those operations."),_T(""),
						wxICON_INFORMATION);
						// put the focus back in the former place
						if (pApp->m_pTargetBox != NULL)
							if (pApp->m_pTargetBox->IsShown())
								pApp->m_pTargetBox->SetFocus();
						return;
					}
				}

                // We should clear target text highlighting if user clicks in a cell within
                // a stretch of text that is not already highlighted. We can clear it by
                // resetting the globals to -1. The highlighting should be retained if user
                // clicks in a cell within a stretch of highlighted text since the user is
                // probably correcting one or more cells that were not good translations
				if (pApp->m_nActiveSequNum < gnBeginInsertionsSequNum 
					|| pApp->m_nActiveSequNum > gnEndInsertionsSequNum)
				{
					gnBeginInsertionsSequNum = -1;
					gnEndInsertionsSequNum = -1;
				}

				// save old sequ number in case required for toolbar's Back button
				gnOldSequNum = pApp->m_nActiveSequNum;

				// set up a temporary pile pointer
				CPile* pile = NULL;

				// BEW added block 26Jun05 for free translation support
				if (pApp->m_bFreeTranslationMode)
				{
					// get the phrase box to the start of the free translation section if
					// in one, otherwise where clicked becomes the start of a free
					// translation section
					pile = pCell->GetPile();
					CSourcePhrase* pSP = pile->GetSrcPhrase();
					wxASSERT(pSP != NULL);
					while (pSP->m_bHasFreeTrans && !pSP->m_bStartFreeTrans)
					{
                        // there must be an earlier one which starts the free translation
                        // section, so iterate back until it is found; in the refactored
                        // design (April 2009) iterating back will not find a bundle start
                        // (there are no bundles anymore), the only possibility is to find
                        // the free translation section's start - there has to be one
                        // before the start of the doc is found, though it could be the
						// first pile of the doc; but we'll test for a malformed doc too
						// and make a fix as best we can
						CPile* pOldPile = pile; // keep the one we are leaving in case the
										// next line gives NULL and so the fix is needed
						pile = pView->GetPrevPile(pile);
						if (pile == NULL)
						{
                            // went past start of doc! we should find the section start
                            // before coming to the doc start, (check Split Document
                            // command - can it split within a free translation section
                            // without alerting user or helping, and so generate a
                            // doc-initial partial free trans section with no beginning?
                            // Assume it can for now...) So, fix the srcPhrase & leave
                            // phrase box here & return
							gbSuppressSetup = TRUE; // don't permit reentry at 
													// RecalcLayout() call
							pApp->m_nActiveSequNum = 0;
							CSourcePhrase* pOldSrcPhrase = pOldPile->GetSrcPhrase();
							pOldSrcPhrase->m_bStartFreeTrans = TRUE; // it didn't have it 
																	 // set, so do it
							pLayout->RecalcLayout(pApp->m_pSourcePhrases);
							pApp->m_pActivePile = pView->GetPile(0);
							wxASSERT(pApp->m_pActivePile != NULL);							

							// now we've located the phrase box, permit setup again
							gbSuppressSetup = FALSE; // allow reentry again
							return;
						}
						pSP = pile->GetSrcPhrase(); // get the sourcephrase on the previous pile
					} // end of while loop for test: pSP->m_bHasFreeTrans && !pSP->m_bStartFreeTrans

					pCell = pile->GetCell(1); // the correct pCell which is the free
					// trans anchor for this F.Tr section; or if no free translation
					// is at the place clicked, this one will become the anchor

                    // if about to place the phrase box elsewhere due to a click, and free
                    // translation mode is turned on, we don't want to retain the Compose
                    // Bar's edit box contents, since it will be different at the new
                    // location - it is sufficient to clear that edit box's contents and
                    // then the SetupCurFreeTransSection() call will, if appropriate, put
                    // the free translation text in the box, or none, or compose a default
                    // text, depending on what options are currently on and whether or not
                    // the place where the box was clicked has free translation text
                    // already - if the latter is true, then the phrase box will be
                    // automatically moved if necessary so that it is placed at the start
					// of the clicked free translation section
					wxString tempStr;
					tempStr.Empty();
					pEditCompose->SetValue(tempStr); // clear the box

					pApp->m_nActiveSequNum = pile->GetSrcPhrase()->m_nSequNumber;
					pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);

					// make m_bIsCurrentFreeTransSection FALSE on every pile
					pView->MakeAllPilesNonCurrent(pLayout);

					// the PlacePhraseBox() call calls CLayout::RecalcLayout()
					pView->PlacePhraseBox(pCell,1); // suppress both KB-related code blocks
					pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
					ScrollIntoView(pApp->m_nActiveSequNum);
					translation.Empty();

				} // end of block for test: pApp->m_bFreeTranslationMode == TRUE
				else
				{
					// not in free translation mode
					translation.Empty();

					#ifdef _Trace_Click_FT
					TRACE1("PlacePhraseBox() next, normal mode; key: %s\n", pApp->m_targetPhrase);
					#endif

					// if the user has turned on the sending of synchronized scrolling
					// messages, send the relevant message
					if (!gbIgnoreScriptureReference_Send)
					{
						pView->SendScriptureReferenceFocusMessage(pApp->m_pSourcePhrases,pCell->GetPile()->GetSrcPhrase());
					}

					pView->PlacePhraseBox(pCell); // calls RecalcLayout()
					ScrollIntoView(pApp->m_nActiveSequNum);
				}

				// restore default button image, and m_bCopySourcePunctuation to TRUE
				wxCommandEvent event;
				pApp->GetView()->OnButtonEnablePunctCopy(event);
				
				CPile* pPile;
				pPile = pView->GetPile(pApp->m_nActiveSequNum);
				wxASSERT(pApp->m_nActiveSequNum == pPile->GetSrcPhrase()->m_nSequNumber);

				// refresh status info at the bottom of the main window
				pApp->RefreshStatusBarInfo();

				// if we are in free translation mode, there is a bit more to do...
				if (pApp->m_bFreeTranslationMode)
				{
                    // put the focus in the compose bar's edit box, select any text there,
                    // but only when it has been composed from the target text or gloss
                    // text; if it is preexisting then put the cursor at the end of it
					pEditCompose->SetFocus();
					if (pApp->m_bTargetIsDefaultFreeTrans || pApp->m_bGlossIsDefaultFreeTrans)
					{
						if (pApp->m_pActivePile->GetSrcPhrase()->m_bHasFreeTrans)
						{
							// whm modified 24Aug06
							wxString tempStr;
							tempStr = pEditCompose->GetValue();
							int len = tempStr.Length(); 
							pEditCompose->SetSelection(len,len);
						}
						else
						{
							pEditCompose->SetSelection(-1,-1);// -1,-1 selects all
						}
					}

					// mark the current section
					pView->MarkFreeTranslationPilesForColoring(gpCurFreeTransSectionPileArray);
					if (pApp->m_nActiveSequNum >= 0 && 
											pApp->m_nActiveSequNum <= pApp->GetMaxIndex())
					{
						ScrollIntoView(pApp->m_nActiveSequNum);
					}
				} // end of block for test: pApp->m_bFreeTranslationMode
				return;

			} // end block for test: pCell->GetCellIndex() == 1
			if (pCell->GetCellIndex() == 2)
			{
				return; // prevent clicks in bottom line of piles selecting or doing anything
			}

			// if it's none of the above things, then just a plain old click for making a 
			// selection... so clear the old selection, then make a new one
			if (pCell->GetCellIndex() == 0)
			{
b:				if (pApp->m_selection.GetCount() != 0)
				{
					CCellList::Node* pos = pApp->m_selection.GetFirst();
					CCell* pOldSel;
					while (pos != NULL)
					{
						pOldSel = (CCell*)pos->GetData();
						pos = pos->GetNext();
						aDC.SetBackgroundMode(pApp->m_backgroundMode);
						aDC.SetTextBackground(wxColour(255,255,255)); // white
						pOldSel->DrawCell(&aDC, pLayout->GetSrcColor());
						pOldSel->SetSelected(FALSE);
					}
					pApp->m_selection.Clear();
					pApp->m_selectionLine = -1; // no selection
					pApp->m_pAnchor = NULL;
				}

				// then do the new selection
a:				pApp->m_bSelectByArrowKey = FALSE;
				aDC.SetBackgroundMode(pApp->m_backgroundMode);
				aDC.SetTextBackground(wxColour(255,255,0)); // yellow
				pCell->DrawCell(&aDC, pLayout->GetSrcColor());
				pCell->SetSelected(TRUE);

				// preserve record of the selection
				pApp->m_selection.Append(pCell);
				pApp->m_selectionLine = pCell->GetCellIndex();
				pApp->m_pAnchor = pCell;
			} //end of block for test: pCell->GetCellIndex() == 0

		} // end of else block for test: event.ShiftDown() == TRUE
	} // end of else block for test: pCell == NULL, i.e. pCell not null 

	event.Skip();
}

void CAdapt_ItCanvas::OnLButtonUp(wxMouseEvent& event)
// do the selection of the current pile, if not already selected; release mouse, and set
// direction
{
	gbReplaceAllIsCurrent = FALSE; // need this, otherwise after a Find and Replace (and even
    // though no replaces are done), if user cancels the find and replace dlg, then clicks
    // to remove the selection (this somehow sets gbReplaceAllIsCurrent to TRUE) then on
    // the next Find dialog, instead of doing the FindNext only as asked, the OnIdle()
    // handler for Replace All button is invoked and we get a spurious set of unwanted
    // replace alls. Probably the click to cancel the selection overwrites something but
    // there is nothing wrong with the code that I can spot, so I put an explicit gbRepl...
    // = FALSE here in order to force the flag back FALSE after the prior OnLButtonDown()
    // call removes the selection.

	CAdapt_ItApp* pApp = &wxGetApp();
	CLayout* pLayout = pApp->m_pLayout;
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	wxASSERT(pView != NULL);
	wxClientDC aDC(this); // make a device context
	DoPrepareDC(aDC); // get origin adjusted - this only has significance if gbIsPrinting - needed?
	
	//wxPoint point = event.GetPosition();
    // we don't need to call CalcUnscrolledPosition() here because GetLogicalPosition
    // already provides the logical position of the clicked point
	wxPoint point(event.GetLogicalPosition(aDC));

	//aDC.DPtoLP(&point); // get the point converted to logical coords - use CalcUnscrolledPosition below
	//int x = aDC.DeviceToLogicalX(point.x);// get the device X coord converted to logical coord
	//int y = aDC.DeviceToLogicalY(point.y);// get the device Y coord converted to logical coord
	//point.x = x;
	//point.y = y;

//#ifdef _DEBUG
//	wxPoint logicalPoint(event.GetLogicalPosition(aDC));
//#endif

//#ifdef _DEBUG
//	// The CalcUnscrolledPosition() function also translates device coordinates
//	// to logical ones.
//	int newXPos,newYPos;
//	CalcUnscrolledPosition(point.x,point.y,&newXPos,&newYPos);
//	wxASSERT(newXPos == logicalPoint.x); //point.x = newXPos;
//	wxASSERT(newYPos == logicalPoint.y); //point.y = newYPos;
//#endif
	
	//int xScrollUnits, yScrollUnits, xOrigin, yOrigin;
	//GetViewStart(&xOrigin, &yOrigin); // gets xOrigin and yOrigin in scroll units
	//GetScrollPixelsPerUnit(&xScrollUnits, &yScrollUnits); // gets pixels per scroll unit
	// the point is in reference to the upper left position (origin)
	//point.x += xOrigin * xScrollUnits; // number pixels is ScrollUnits * pixelsPerScrollUnit
	//point.y += yOrigin * yScrollUnits;

//#ifdef _DEBUG
//	// the logical positions calculated should all be the same
//	wxASSERT(newXPos == point.x);
//	wxASSERT(newYPos == point.y);
//#endif

	// can do a selection only if we have a non zero anchor pointer
	if (pApp->m_pAnchor != NULL)
	{
		// find which cell the cursor was over when the mouse was released (not a well-named 
		// function but it does what we want)
		CPile* pCurPile = NULL;
		CCell* pCell = pView->GetClickedCell(&point); // returns NULL if point was not in a cell

		// BEW added 03Oct08 for support of vertical editing, to prevent dragging
		// a selection into the gray text area either side of the editable span
		if (pCell != NULL)
		{
			pCurPile = pCell->GetPile();
			bool bIsOK = pView->CheckForVerticalEditBoundsError(pCurPile);
			if (!bIsOK)
			{
				// if FALSE was returned, RemoveSelection() has already been called
				return;
			}
		}

		if (pCell == NULL || pApp->m_selectionLine != pCell->GetCellIndex())
		{
            // oops, we missed a cell, or are in wrong line, so we have to clobber any
            // existing selection
			pView->RemoveSelection();
			Refresh(); 
			goto a;
		}

		if (pView->IsTypeDifferent(pApp->m_pAnchor,pCell))
		{
			// IDS_DIFF_TEXT_TYPE
			wxMessageBox(_(
"Sorry, you are trying to select text of different types, such as a heading and verse text, or some other illegal combination. Combining verse text with poetry is acceptable, other combinations are not."),
			_T(""), wxICON_EXCLAMATION);
			pView->RemoveSelection();
			Refresh(); //Invalidate();
			goto a;
		}

        // set the direction (this is always "logical direction", ie, to higher sequence
        // numbers is "rightwards" and to lower sequence number is "leftwards"; for an LTR
        // layout, logical order and visible mouse movement coincide, but for RTL layout,
        // the mouse moves in the opposite direction to logical direction)
		if (pApp->m_pAnchor->GetPile()->GetSrcPhrase()->m_nSequNumber <= 
											pCell->GetPile()->GetSrcPhrase()->m_nSequNumber)
		{
				pApp->m_curDirection = right;
		}
		else
		{
				pApp->m_curDirection = left;
		}

        // finish drag select, but only if not halted at a boundary (note: if selecting
        // forwards, the boundary sourcePhrase actually can be selected (and should be),
        // but unless the user is some kind of speed king with the mouse, the bounding
        // sourcePhrase will have been selected already by the OnMouseMove's internal
        // SelectDragRange() call; so we can use the gbHaltedAtBoundary value in
        // OnLButtonUp() to suppress the final selection without losing the selection of
        // the final element; for backwards selections, the bounding sourcePhrase must not
        // be selected, and so we need to suppress the code below in that case too.
		if (!gbHaltedAtBoundary)
		{
            // first make sure we don't get here when trying to extend a selection by
            // holding down the SHIFT key, because this is not a drag situation, and
            // control can still get to this point. A sufficient test is to check for the
            // SHIFT key down, if it is, we are done and must exit this block immediately
			if (event.ShiftDown() && pApp->m_selection.GetCount() > 0)
				goto a;

			CCellList::Node* pos = pApp->m_selection.Find(pCell);
			if (pos == NULL)
			{
				// cell is not yet in the selection, so add it
				aDC.SetBackgroundMode(pApp->m_backgroundMode);
				aDC.SetTextBackground(wxColour(255,255,0)); // yellow
				pApp->m_bSelectByArrowKey = FALSE;
				pCell->DrawCell(&aDC, pLayout->GetSrcColor());
				pCell->SetSelected(TRUE);

				// preserve record of the selection
				if (pApp->m_curDirection == right)
				{
					pApp->m_selection.Append(pCell);
				}
				else
				{
					// whm Note: wxList::Insert(pCell) inserts the pCell at the 
					// front of the list by default
					pApp->m_selection.Insert(pCell);
				}
			}
		}
	}

	// clear the drag variables and release the mouse
a:	pApp->m_mouse.x = pApp->m_mouse.y = -1;
    // In wx, it is an error to call ReleaseMouse() if the canvas did not previously call
    // CaptureMouse() so we'll check first to make sure canvas has captured the mouse
	if (HasCapture()) // whm added if (HasCapture()) because wx asserts if ReleaseMouse 
	{				  // is called without capture
		ReleaseMouse(); // assume no failure
	}
	gbHaltedAtBoundary = FALSE; // ensure it is cleared
	
	event.Skip(); //CScrollView::OnLButtonUp(nFlags, point);
}

// The Adapt It canvas is a child window of CMainFrame, along with other windows belonging
// to CMainFrame - notably the menu bar, tool bar, status bar, mode bar and compose bar.
// The menu bar, tool bar and status bar windows are associated with the wxFrame by the
// SetMenuBar(), SetToolBar(), and SetStatusBar() methods.
// Adapt It's mode bar and compose bar are unique to Adapt It and are managed separately
// from the other standard wxFrame's "bar" windows. These Adapt It specific bars are
// managed in CMainFrame, and its OnSize() handler, rather than here in the canvas class.
/*
void CAdapt_ItCanvas::OnSize(wxSizeEvent& event)
{
}
*/

void CAdapt_ItCanvas::OnMouseMove(wxMouseEvent& event)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = pApp->GetView();
	if (pApp->m_pAnchor == NULL)
		return; // no anchor, so we can't possibly have a selection

	wxClientDC aDC(this); // make a device context
	DoPrepareDC(aDC); // get origin adjusted
	
	//wxPoint point = event.GetPosition();
    // whm note: The wx docs seem to indicate that, once DoPrepareDC is called,
    // event.GetPosition() should return a point that is already converted to logical
    // coords, but testing shows that's not the case. GetPosition() always returns
    // device/screen coordinates; only GetLogicalPosition() returns the point converted to
    // logical coords
	wxPoint point(event.GetLogicalPosition(aDC));

//#ifdef _DEBUG
//	wxPoint logicalPoint(event.GetLogicalPosition(aDC));
//#endif

//#ifdef _DEBUG
//	//aDC.DPtoLP(&point); // get the point converted to logical coords - the MFC function
//	// the following wx method should be equivalent to the MFC way of doing it
//	int x = aDC.DeviceToLogicalX(point.x);// get the device X coord converted to logical coord
//	int y = aDC.DeviceToLogicalY(point.y);// get the device Y coord converted to logical coord
//	wxASSERT(x == logicalPoint.x); //point.x = x;
//	wxASSERT(y == logicalPoint.y); //point.y = y;
//#endif
	
//#ifdef _DEBUG
//	// CalcUnscrolledPosition is an alternate method of above
//	int newXPos,newYPos;
//	CalcUnscrolledPosition(point.x,point.y,&newXPos,&newYPos);
//	wxASSERT(newXPos == logicalPoint.x); //point.x = newXPos;
//	wxASSERT(newYPos == logicalPoint.y); //point.y = newYPos;
//#endif

	//int xScrollUnits, yScrollUnits, xOrigin, yOrigin;
	//GetViewStart(&xOrigin, &yOrigin); // gets xOrigin and yOrigin in scroll units
	//GetScrollPixelsPerUnit(&xScrollUnits, &yScrollUnits); // gets pixels per scroll unit
	//// the point is in reference to the upper left position (origin)
	//point.x += xOrigin * xScrollUnits; // number pixels is ScrollUnits * pixelsPerScrollUnit
	//point.y += yOrigin * yScrollUnits;

//#ifdef _DEBUG
//	wxASSERT(newXPos == point.x);
//	wxASSERT(newYPos == point.y);
//#endif

	if (event.Dragging()) // whm note: Dragging() works here; LeftDown() doesn't
	{
		// do the following only provided the button is down
		if (pApp->m_mouse.x != point.x || pApp->m_mouse.y != point.y)
		{
			// there has been movement, so check if more selection is 
			// required & act accordingly
			CCell* pCell = pView->GetClickedCell(&point); // returns NULL 
											// if the point was not in a cell
			if (pCell == NULL)
			{
				pView->SelectAnchorOnly(); // is independent of direction
			}
			else
			{
				pView->SelectDragRange(pApp->m_pAnchor,pCell); // internally sets 
											// m_curDirection to left or right
			}
		}
	}

	//event.Skip(); //CScrollView::OnMouseMove(nFlags, point); 
	// event.Skip apparently not needed here
}

/* 
// A code snippit from the wxWidgets doodling program. 
// It illustrates how to capture and process stuff while 
// dragging the mouse with the left button.
void CAdapt_ItCanvas::OnMouseEvent(wxMouseEvent& event)
{
  if (!view)
    return;
    
  static DoodleSegment *currentSegment = (DoodleSegment *) NULL;

  wxClientDC dc(this);
  PrepareDC(dc);

  dc.SetPen(*wxBLACK_PEN);

  wxPoint pt(event.GetLogicalPosition(dc));

  if (currentSegment && event.LeftUp())
  {
    if (currentSegment->lines.Number() == 0)
    {
      delete currentSegment;
      currentSegment = (DoodleSegment *) NULL;
    }
    else
    {
      // We've got a valid segment on mouse left up, so store it.
	  CAdapt_ItDoc* pDoc = (CAdapt_ItDoc *)view->GetDocument();
	  wxASSERT(pDoc != NULL);

      pDoc->GetCommandProcessor()->Submit(new DrawingCommand(_T("Add Segment"), 
											DOODLE_ADD, pDoc, currentSegment));

      view->GetDocument()->Modify(TRUE);
      currentSegment = (DoodleSegment *) NULL;
    }
  }
  
  if (xpos > -1 && ypos > -1 && event.Dragging())
  {
    if (!currentSegment)
      currentSegment = new DoodleSegment;

    DoodleLine *newLine = new DoodleLine;
    newLine->x1 = (long)xpos; 
    newLine->y1 = (long)ypos;
    newLine->x2 = pt.x; 
    newLine->y2 = pt.y;
    currentSegment->lines.Append(newLine);
    
    dc.DrawLine( (long)xpos, (long)ypos, pt.x, pt.y);
  }
  xpos = pt.x;
  ypos = pt.y;
}
*/

// BEW new version 12May09 - logic identical to Bill's except at the very end, where the
// last test is made
void CAdapt_ItCanvas::ScrollIntoView(int nSequNum)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	if (pApp->m_nActiveSequNum == -1)
	{
		return; // do nothing if the phrase box is hidden because we are at doc end
	}
	CLayout* pLayout = pApp->m_pLayout;
	bool debugDisableScrollIntoView = FALSE; // set TRUE to disable ScrollIntoView
	if (!debugDisableScrollIntoView)
	{
		CAdapt_ItView* pView = pApp->GetView();
		CPile* pPile = pView->GetPile(nSequNum);
		CStrip* pStrip = pPile->GetStrip();
		wxRect rectStrip = pStrip->GetStripRect();

		// get the visible rectangle's coordinates
		wxRect visRect; // wxRect rectClient;
        // wx note: calling GetClientSize on the canvas produced different results in wxGTK
        // and wxMSW, so I'll use my own GetCanvasClientSize() which calculates it from the
        // main frame's client size after subtracting the controlBar's height and
        // composeBar's height (if visible).
		wxSize canvasSize;
		canvasSize = pApp->GetMainFrame()->GetCanvasClientSize();
		visRect.width = canvasSize.GetWidth();
		visRect.height = canvasSize.GetHeight();

        // calculate the window depth, and then how many strips are fully visible in it; we
        // will use the latter in order to change the behaviour so that instead of
        // scrolling so that the active strip is at the top (which hides preceding context
        // and so is a nuisance), we will scroll to somewhere a little past the window
        // center (so as to show more, rather than less, of any automatic inserted material
        // which may have background highlighting turned on)...
        
		// BEW 26Apr09: legacy app included 3 pixels plus height of free trans line (when
		// in free translation mode) in the m_curPileHeight value; the refactored design
		// doesn't so I'll have to add them here
		int nWindowDepth = visRect.GetHeight();
		int nStripHeight = pLayout->GetPileHeight() + pLayout->GetCurLeading();
		if (pApp->m_bFreeTranslationMode)
		{
			nStripHeight += 3 + pLayout->GetTgtTextHeight();
		}
		int nVisStrips = nWindowDepth / nStripHeight;
		
		// get the current horizontal and vertical pixels currently scrolled
		int xPixelsPerUnit,yPixelsPerUnit; // needed farther below
		GetScrollPixelsPerUnit(&xPixelsPerUnit,&yPixelsPerUnit);

        // determine how much preceding context (ie. how many strips) we want to try make
        // visible above the phrase box when auto inserting (so as to show as much
        // highlighted material as possible); we make this amount equal to nVisStrips less
        // two (one for the phrase box strip itself, and another for one strip of following
        // context below it) when auto inserting, but a much bigger value (see below) when
        // auto inserting stops (so that more of any auto inserted & hilighted adapatations
        // will be visible to the user without scrolling)
		int nPrecedingContextDepth = 2 * nStripHeight; // nStripHeight includes the leading

        // Get the required y-coord of the top of the phrase box's strip where the "strip"
        // includes its preceding leading -- that is, the distance from the start of the
        // document to the beginning of the leading for the active strip (the new value was
        // determined by a prior call to RecalcLayout)
		int yDistFromDocStartToStripTopLeading = 
										pPile->GetPileRect().GetTop() - pApp->m_curLeading;
		
        // wx version design considerations 14Sep06: Most of the MFC ScrollIntoView code is
        // designed to figure out how much to scroll from the current scroll position to
        // get to the new position using ScrollWindow. But, the wx version equivalent of
        // ScrollWindow is Scroll(x,y) which scrolls to a given position in the doc,
        // eliminating the need to determine an "amount" to scroll from a current position.

		// if auto inserting, use a small nPrecedingContextDepth value; but if not (eg. as
        // when the auto insertions have just stopped) then put the box near the bottom of
        // the window to show more of the preceding context
		if (!pApp->m_bAutoInsert)
		{
			if (pApp->m_bDrafting || !pApp->m_bSingleStep) // whm added
				nPrecedingContextDepth = (nVisStrips - 3) * nStripHeight; // whm changed 2 to 3
			else
				nPrecedingContextDepth = nVisStrips / 2 * nStripHeight;
		}

		// get the desired logical top (ie. y Distance) to the desired scroll position
		// (this calculation will yield a negative number if the target strip is closer
		// to the top of the virtual document than the value of nPrecedingContextDepth)
		int desiredViewTop = yDistFromDocStartToStripTopLeading - nPrecedingContextDepth;

		// make a sanity check on the above value
		if (desiredViewTop < 0)
			desiredViewTop = 0; 

		//-------------------- now the desiredViewBottom calculations ---------------
		
        // Determine the desired bottom position in the document of the content we wish to
        // view. We make the bottom position to include two strips including their leading,
        // unless there is no room for that much following context (as when we're at the
        // bottom of the document).
		wxSize virtDocSize;
		GetVirtualSize(&virtDocSize.x,&virtDocSize.y); // GetVirtualSize gets size in pixels
		int desiredViewBottom = pPile->GetPileRect().GetBottom();
		if (pApp->m_bFreeTranslationMode)
		{
			desiredViewBottom += 3 + pLayout->GetTgtTextHeight();
		}
		// increment desiredViewBottom by the amount of two strips; note, this value may
		// be greater than virtDocSize.y, so if we get to use it, we'll need to do a
		// sanity check first and make it equal to virtDocSize.y
		desiredViewBottom += 2*nStripHeight;

		// sanity check on the above value
		if (desiredViewBottom > virtDocSize.y)
			desiredViewBottom = virtDocSize.y;		
		
		//-------------------- now the current position data ------------------------

		// get the current y distance to the top of the view, that is, to the top of the
		// client area
		int newXPos,current_yDistFromDocStartToViewTop;
		CalcUnscrolledPosition(0,0,&newXPos,&current_yDistFromDocStartToViewTop);

		//------- now check if no scroll needed, or if needed, do it -----------------

		// first check if no scroll is needed - return if it isn't needed
		if ((desiredViewTop >= current_yDistFromDocStartToViewTop) &&
			(desiredViewBottom <= current_yDistFromDocStartToViewTop + nWindowDepth))
		{
			// no scroll needed, the box is visible and within the preceding and following
			// context depths, so just return
			return;
		}

		// handle the situation where the desiredViewTop is < (i.e., above) 
		// the current logical view top
		if (desiredViewTop < current_yDistFromDocStartToViewTop)
		{
			Scroll(0,desiredViewTop / yPixelsPerUnit); // Scroll takes scroll units not pixels
			return;
		}

		// handle the situation where the desiredViewBottom is > (i.e., above) 
		// the current logical view bottom
		if (desiredViewBottom > current_yDistFromDocStartToViewTop + nWindowDepth)
		{
			Scroll(0, (desiredViewBottom - nWindowDepth) / yPixelsPerUnit); //takes scroll units
			return;
		}
	}
}

/*
void CAdapt_ItCanvas::ScrollIntoView(int nSequNum)
// MFC Notes: A kludge is built in, with the syntax which is commented out, the bottom of
// the offset visible rectangle ends up about 97 pixels (give or take one pixel) lower (ie.
// larger) than it should be when compared with the strip + leading sizes, so I've reduced
// the .bottom value by 97 and then the function works fine. The older code would not
// scroll down when a wrap forced a pile down to the start of the next strip, and the
// phrase box would end up below the visible area with no scroll done. With the kludge this
// no longer happens. No idea why the old logic was wrong.
// BEW changed 03Jan06. The versions from 2.4.0 to 2.4.1i had a "smart scroll" block which,
// when highlighting was turned on, calculated the number of highlighted strips and tried
// to show them in the window; but this turned out to be a source of crashes when the
// number of such strips became large (as when doing large numbers of automatic insertions
// without any halts) because the code for calculating the starting and ending hilighted
// strips relied on pile pointers but when many insertions are done inevitably a bundle
// advance will happen and this frees the old pile pointers, leading to a crash when the
// code attempted to use the pointer for the first pile in the highlighted section. So I've
// removed the smart code and we just now go with the old legacy code which attempts to
// keep the phrase box about the middle of the window. (This means that when many auto
// insertions are done, the start of the highlighted section will be above the visible
// rectangle more often than when smart scrolling was done, necessitating a manual scroll
// to see it all when that is the case.)
//
// whm Note: We remove the kluge in the wxWidgets version. The MFC version had an error
// calling pFrame->GetClientRect; the client rect should be determined by calling
// pView->GetClientRect which would eliminate the need for nWindowHeightReduction kluge.
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	if (pApp->m_nActiveSequNum == -1)
	{
		return; // do nothing if the phrase box is hidden because we are at doc end
	}

	CLayout* pLayout = pApp->m_pLayout;
	bool debugDisableScrollIntoView = FALSE; // set TRUE to disable ScrollIntoView
	if (!debugDisableScrollIntoView)
	{
		CAdapt_ItView* pView = pApp->GetView();

#ifdef _LOG_DEBUG_SCROLLING
		if (pApp->m_nActiveSequNum >1193)
		{
			wxLogDebug(_T("********** ScrollIntoView  BEGIN  ****************\n"));
		}
#endif

		CPile* pPile = pView->GetPile(nSequNum);


#ifdef _LOG_DEBUG_SCROLLING
		wxString trace;
		trace = trace.Format(
		_T("\n Scrolling #0 sequNum: %d   m_targetPhrase: %s\n"), 
		nSequNum,pApp->m_targetPhrase.c_str());
		wxLogDebug(trace);
#endif

		//wxRect rectPile = pPile->GetPileRect(); // in logical coords (pixels) 
		//                                        // from doc/bundle start
        // BEW 26Apr09, for refactored view code: get the pile's rect, in logical coords
        // (pixels) from doc/bundle start; but note: in the legacy app, the pile rectangle
        // had the height of the free trans line plus 3 extra pixels vertically; and in the
        // new code, the pile rectangle only encompasses the height of the CCell instances;
        // so I will use that and add the extra pixels to the height to get the equivalent
        // to what was in the legacy app - so that Bill's arithmentic is not messed below
        // up by me using the wrong rectPile height; similarly, a compatibility function,
        // GetStripRect() is needed to get the same strip rectangle as in the legacy app 
        // (the calcs are in logical coords (pixels) from doc/bundle start)
		//wxRect rectPile = pPile->GetPileRect();
		//rectPile.SetHeight(rectPile.GetHeight() + 3 + pLayout->GetTgtTextHeight());
		CStrip* pStrip = pPile->GetStrip();
		wxRect rectStrip = pStrip->GetStripRect();
		//rectStrip.SetTop(rectStrip.GetTop() - pLayout->GetCurLeading()); // unneeded
		// ******* CHECK THE ABOVE ASSUMPTIONS, OR CHECK BY TESTING SCROLLING ******
		// (searching for "m_rectStrip = in CAdapt_ItView.cpp confirms the assumptions)

#ifdef _LOG_DEBUG_SCROLLING
		int nStripIndex = pStrip->m_nStripIndex; // index of strip where phrasebox is
		trace = trace.Format(_T(" Scrolling #1 (log) rectStrip top: %d  bottom: %d  nStripIndex: %d\n"),
						rectStrip.GetTop(),rectStrip.GetBottom(),nStripIndex);
		wxLogDebug(trace);
#endif

		// get the visible rectangle's coordinates
		wxRect visRect; // wxRect rectClient;
        // whm: At this point the MFC version had an error calling pFrame->GetClientRect;
        // should be pView->GetClientRect which would eliminate the need for
        // nWindowHeightReduction kluge.
		
        // wx note: calling GetClientSize on the canvas produced different results in wxGTK
        // and wxMSW, so I'll use my own GetCanvasClientSize() which calculates it from the
        // main frame's client size after subtracting the controlBar's height and
        // composeBar's height (if visible).
		wxSize canvasSize;
		canvasSize = pApp->GetMainFrame()->GetCanvasClientSize();
		visRect.width = canvasSize.GetWidth();
		visRect.height = canvasSize.GetHeight();

        // MFC Note: A kludge, else visRect.bottom is too large a value (perhaps status bar
        // and horiz scroll bar are counted as within client area, so have to allow room by
        // adjusting the lower value to be used in tests below; 97 is a bit bigger than
        // horiz scroll bar and status bar however.) It has the effect of getting a scroll
        // down to occur a little sooner than perhaps otherwise expected. 97 is not an
        // arbitrary value, it seems to be about what is needed; moreover this value has
        // stayed constant constant over Windows 95, 98 and XP. I've no idea why it's
        // necessary; but without it, the bottom of the visible rectangle is numerically a
        // certain value and the bottom of a strip which extends below the bottom of the
        // visible rectangle has a value about 90 or so less - which ought never to be the
        // case. So making his height reduction by reducing the vis rect's bottom value &
        // using that gets the scrolls working as expected
		//int nWindowHeightReduction = 97; // The kludge - no longer needed

        // calculate the window depth, and then how many strips are fully visible in it; we
        // will use the latter in order to change the behaviour so that instead of
        // scrolling so that the active strip is at the top (which hides preceding context
        // and so is a nuisance), we will scroll to somewhere a little past the window
        // center (so as to show more, rather than less, of any automatic inserted material
        // which may have background highlighting turned on)...
        
		// BEW 26Apr09: legacy app included 3 pixels plus height of free trans line (when
		// in free translation mode) in the m_curPileHeight value; the refactored design
		// doesn't so I'll have to add them here
		int nWindowDepth = visRect.GetHeight();
		//int nStripHeight = pApp->m_curPileHeight + pApp->m_curLeading;
		int nStripHeight = pLayout->GetPileHeight() + pLayout->GetCurLeading();
		if (pApp->m_bFreeTranslationMode)
		{
			nStripHeight += 3 + pLayout->GetTgtTextHeight();
		}
		int nVisStrips = nWindowDepth / nStripHeight;
		

		#ifdef _LOG_DEBUG_SCROLLING
		if (pApp->m_nActiveSequNum >1193)
		{
			wxLogDebug(_T(
			"ScrollIntoView nWindowDepth %d nStripHeight %d nVisStrips %d\n"),
			nWindowDepth,nStripHeight,nVisStrips);
		}
		#endif

		// get the current horizontal and vertical pixels scrolled
		int xPixelsPerUnit,yPixelsPerUnit; // needed farther below
		GetScrollPixelsPerUnit(&xPixelsPerUnit,&yPixelsPerUnit);

        // determine how much preceding context (ie. how many strips) we want to try make
        // visible above the phrase box when auto inserting (so as to show as much
        // highlighted material as possible); we make this amount equal to nVisStrips less
        // two (one for the phrase box strip itself, and another for one strip of following
        // context below it) when auto inserting, but a much bigger value (see below) when
        // auto inserting stops (so that more of any auto inserted & hilighted adapatations
        // will be visible to the user without scrolling)
		int nPrecedingContextDepth = 2 * nStripHeight; // nStripHeight includes the leading

        // Get the required y-coord of the top of the phrase box's strip plus preceding
        // leading -- that is, the distance from the start of the document to the beginning
        // of the leading for the active strip (the new value was determined by a prior
        // call to RecalcLayout). This distance increases as the visRectFromBundleStart
        // scrolls down, but suddenly decreases to a small value when the caller has just
        // done a bundle advance. called yDistActiveStripTop in MFC.
		int yDistFromBundleStartToStripTopLeading = pPile->GetPileRect().GetTop() 
													- pApp->m_curLeading;
		
		// //////////////////////////// Simplified Routine Below /////////////////////////
        // From here to end of commented out section is an alternate/simplified wx code
        // solution. It replaces the legacy ScrollIntoView behavior which is commented out
        // toward the end of the function.
		//
        // wx version design considerations 14Sep06: Most of the MFC ScrollIntoView code is
        // designed to figure out how much to scroll from the current scroll position to
        // get to the new position using ScrollWindow. But, the wx version equivalent of
        // ScrollWindow is Scroll(x,y) which scrolls to a given position in the doc,
        // eliminating the need to determine an "amount" to scroll from a current position.
		//
        // Note: wxWindow class does have a ScrollWindow(int dx, int dy) where dx and dy
        // are amount of pixels to scroll.
		//
        // Also, one cannot really depend on the "current position" being valid,
        // particularly when ScrollIntoView is called immediately after a bundle retreat,
        // which leaves the scroll bars (at this point in ScrollIntoView) in the position
        // they previously had in the previous bundle. Moreover, RecalcLayout has by now
        // calculated a new strip position for the active location that we want to scroll
        // to. So why not just scroll to this new active location's position, adjusting the
        // scroll position by the desired preceding and/or following context to remain
        // visible where possible.

        // if auto inserting, use a small nPrecedingContextDepth value; but if not (eg. as
        // when the auto insertions have just stopped) then put the box near the bottom of
        // the window to show more of the preceding context
		if (!pApp->m_bAutoInsert)
		{
			if (pApp->m_bDrafting || !pApp->m_bSingleStep) // whm added
				nPrecedingContextDepth = (nVisStrips - 3) * nStripHeight; // whm changed 2 to 3
			else
				nPrecedingContextDepth = nVisStrips / 2 * nStripHeight;
		}
        // Determine the desired top position in the document of the content we wish to
        // view. We make the top position to include the nPrecedingContextDepth, unless
        // there is no room for it (as when we're at the beginning of the document).
		int desiredViewTop = yDistFromBundleStartToStripTopLeading;
		
		if (yDistFromBundleStartToStripTopLeading - nPrecedingContextDepth >= 0)
		{
			// decrement desiredViewTop by the amount of the preceding context needed
			desiredViewTop -= nPrecedingContextDepth;
		}
		else
		{
            // there is not enough document left to scroll up leaving the
            // nPrecedingContextDepth so leave the desiredViewTop set at
            // yDistFromBundleStartToStripTopLeading (see above)
			;
		}
        // Determine the desired bottom position in the document of the content we wish to
        // view. We make the bottom position to include the nFollowingContextDepth, unless
        // there is no room for it (as when we're at the bottom of the document).
		wxSize virtDocSize;
		GetVirtualSize(&virtDocSize.x,&virtDocSize.y); // GetVirtualSize gets size in pixels
		int desiredViewBottom = pPile->GetPileRect().GetBottom();
		if (desiredViewBottom + 2*nStripHeight <= virtDocSize.y)
		{
			// increment desiredViewBottom by the amount of two strips
			desiredViewBottom += 2*nStripHeight;
		}

		int nMidScreenOffsetFromTop = nWindowDepth / 2;
		
		
		// get the current horizontal and vertical pixels scrolled
		int newXPos,newYPos;
		int logicalViewTop, logicalViewBottom, logicalViewMiddle;
		CalcUnscrolledPosition(0,0,&newXPos,&newYPos);
		
		logicalViewTop = newYPos;
		logicalViewBottom = logicalViewTop + nWindowDepth;
		logicalViewMiddle = logicalViewTop + nMidScreenOffsetFromTop;
		
		// sanity check:
		if (desiredViewTop < 0)
			desiredViewTop = 0;
		if (desiredViewBottom > virtDocSize.y)
			desiredViewBottom = virtDocSize.y;

        // If desiredViewTop is greater than or equal to the logical view top, and the
        // desiredViewBottom is less than or equal to the logical view bottom, we do not
        // need to do anything (no scroll needed).
		if (desiredViewTop >= logicalViewTop && desiredViewBottom <= logicalViewBottom)
		{
			return;
		}
		// handle the situation where the desiredViewTop is < (i.e., above) 
		// the logicalViewTop
		if (desiredViewTop < logicalViewTop)
		{
			Scroll(0,desiredViewTop / yPixelsPerUnit); // Scroll takes scroll units not pixels
			return; // don't do legacy routine below
		}
		if (desiredViewBottom > logicalViewBottom)
		{
			Scroll(0,(desiredViewBottom + (desiredViewBottom - logicalViewBottom) 
						- nWindowDepth)/yPixelsPerUnit);
			return; // don't do legacy routine below
		}
	}
}
*/
		// ****** Bill's Simplified Routine Is Above ******
/*
		// //////////// Legacy Scrolling Routine Below ////////////////////////////////
        // This legacy scrolling routine suffers from the problem that it doesn't make
        // transitions between bundles very well. Also, I think it is overly complicated,
        // so I've replaced it in the wxWidgets version with the simplified routine above.
		//
        // The routine below is a closer implementation of the MFC code for ScrollIntoView.
        // It basically emulates MFC's "amount" to scroll. I changed some of the variable
        // names to make it more self documenting. scrollPos is now called
        // distDocStartToViewRect. In the end I could not get it to properly handle
        // clicking on a previous bundle's target phrase (after the bundle advance, it
        // insisted on jumping to the beginning of the new bundle.

		wxPoint distDocStartToViewRect; //was called: wxPoint scrollPos;
		//int xPixelsPerUnit,yPixelsPerUnit; // needed farther below
		//GetScrollPixelsPerUnit(&xPixelsPerUnit,&yPixelsPerUnit);
        // MFC's GetScrollPosition() "gets the location in the document to which the upper
        // left corner of the view has been scrolled. It returns values in logical units."
        // wx note: The wx docs only say of GetScrollPos(), that it "Returns the built-in
        // scrollbar position." Testing shows that this means it gets the logical position
        // of the upper left corner, but it is in "scroll units" which need to be converted
        // to device (pixel) units by multiplying the coordinate by pixels per unit.
        // Another wxWidgets call is GetViewStart() which "gets the current horizontal and
        // vertical pixels scrolled."
		// OBSERVATION and CAUTION: 
        // RecalcLayout() is normally called before this ScrollIntoView is called. In
        // RecalcLayout, the SetScrollbars() function is called. When it is called I notice
        // during debug tracing that the main window's scroll bars immediately adjust in
        // length to reflect the new bundle's newly calculated doc length, especially
        // whenever a bundle advance or retreat was just done. CAUTION: In the current
        // function we must be aware that RecalcLayout has changed the logical layout of
        // the doc and that all logical coordinates related to the visRect and the scroll
        // position need to be recalculated based on the newly Recalc'd document. At the
        // moment at this point in ScrollIntoView, the old doc's layout is still diaplayed
        // on screen.
		
		// get the current horizontal and vertical pixels scrolled
		//GetViewStart(&distDocStartToViewRect.x,&distDocStartToViewRect.y);
		//                       // MFC: CPoint scrollPos = GetScrollPosition();
		//distDocStartToViewRect.x *= xPixelsPerUnit; wxASSERT(distDocStartToViewRect.x == 0);
		//distDocStartToViewRect.y *= yPixelsPerUnit;

//#ifdef _DEBUG
//		// The following lines are a test to make sure that GetScrollPos returns the same final values
//		// that calling GetViewStart returns.
//		wxPoint testdistDocStartToViewRect;
//		testdistDocStartToViewRect.x = GetScrollPos(wxHORIZONTAL);
//		testdistDocStartToViewRect.y = GetScrollPos(wxVERTICAL); 
//		testdistDocStartToViewRect.x *= xPixelsPerUnit; wxASSERT(testdistDocStartToViewRect.x == 0);
//		testdistDocStartToViewRect.y *= yPixelsPerUnit;
//		wxASSERT(testdistDocStartToViewRect.x == distDocStartToViewRect.x);
//		wxASSERT(testdistDocStartToViewRect.y == distDocStartToViewRect.y);
//#endif

		// get the current horizontal and vertical pixels scrolled
		CalcUnscrolledPosition(0,0,&distDocStartToViewRect.x,&distDocStartToViewRect.y);

		#ifdef _Trace_Box_Loc_Wrong
		wxString trace;
		if (pApp->m_nActiveSequNum >1193)
		{
		trace = trace.Format("ScrollIntoView visRect top: %d  bottom: %d  distDocStartToViewRect  y: %d\n",
					visRect.top, visRect.bottom, distDocStartToViewRect.y);
		TRACE0(trace);
		}
		#endif
		
        // whm: I'm introducing a new wxRect called visRectFromBundleStart here to help
        // clarify what the rect actually represents after the Offset operation. Before the
        // Offset call below visRect was set at 0,0 for its upper left coordinate position,
        // because it was assigned its rect values by GetCanvasClientSize() which always
        // returns an upper left coordinate position of 0,0.
		wxRect visRectFromBundleStart = visRect;
        // offset the visRect in logical coord space (if the scroll car is at the bottom of
        // the bar then this offsets the rectangle downwards so that the top of it will be
        // at the nLimit value of the scrolling vertical limit; for Adapt It, there is no
        // horizontal scrolling)
		visRectFromBundleStart.Offset(distDocStartToViewRect); // MFC: visRect.OffsetRect(scrollPos);
		// visRectFromBundleStart now has its upper left coord position 

		#ifdef _Trace_Box_Loc_Wrong
		if (pApp->m_nActiveSequNum >1193)
		{
		trace = trace.Format(
			"ScrollIntoView after offset: visRectFromBundleStart top: %d  bottom: %d\n",
			visRectFromBundleStart.top,visRectFromBundleStart.bottom);
		TRACE0(trace);
		}
		#endif
		
		// determine the vertical scroll needed
        // MFC docs say about GetScrollLimit(SB_VERT), "Call this CWnd member function to
        // retrieve the maximum scrolling position of the scroll bar. The int return value
        // specifies the maximum position of a scroll bar if successful; otherwise 0. wx
        // docs say about GetScrollRange(wxVERTICAL), "Returns the built-in scrollbar
        // range" Testing shows that GetScrollRange gets the number of scroll units
        // INCLUDING the size of the last page. To get the actual scrolling limit in the
        // same terms as MFC's GetScrollLimit where it indicates the upper left y
        // coordinate of the maximally scrolled window, we must subtract some pixels from
        // the total range that WX's GetScrollRange gives us. See note below.
		int nLimit = GetScrollRange(wxVERTICAL); //int nLimit = GetScrollLimit(SB_VERT);
        // whm note: GetscrollRange returns scroll units, and MFC's GetScrollLimit
        // apparently returns device units (pixels), so must multiply the value obtained
        // from GetScrollRange by yPixelsPerUnit GetScrollRange got the range of scroll
        // units for the whole virtual document, not just the value of the upper left y
        // coord for the window when maximally scrolled. To find the y coord for the client
        // window at its maximal scrolled extent we need to do the following:
		// Take the modulus of DocLengthInScrollUnits % ScrollUnitsPerPage
        // The modulus operation will give the number of scroll units that exist beyond the
        // last fully scrolled page. Multiply this value by pixelsPerScrollUnit to get the
        // amount the y coord of the client view should be moved up toward the beginning of
        // the doc to be the same value that MFC's GetScrollLimit obtains.
		int scrollUnitsPerPage = GetScrollThumb(wxVERTICAL);
		int unitsBeyondLastFullScrolledPage = nLimit % scrollUnitsPerPage;
		// reduce the nLimit by the unitsBeyoneLastFullScrolledPage.
		nLimit -= unitsBeyondLastFullScrolledPage; // to make wxWindow::GetScrollRange == 
												   // CWnd::GetScrollLimit
		nLimit *= yPixelsPerUnit; // convert scroll units to pixels

        // if auto inserting, use a small nPrecedingContextDepth value; but if not (eg. as
        // when the auto insertions have just stopped) then put the box near the bottom of
        // the window to show more of the preceding context
		if (!pApp->m_bAutoInsert)
		{
			nPrecedingContextDepth = (nVisStrips - 2) * nStripHeight; 
		}

		#ifdef _Trace_Box_Loc_Wrong
		if (pApp->m_nActiveSequNum >1193) 
		{
		TRACE1("FLAG m_bAutoInsert:  %d\n",pApp->m_bAutoInsert);
		TRACE3("ScrollIntoView nLimit %d (scroll limit) yDistFromBundleStartToStripTopLeading %d nPrecedingContextDepth %d\n",
				nLimit,yDistFromBundleStartToStripTopLeading,nPrecedingContextDepth);
		}
		#endif

        // Don't scroll if the active strip is further down than one strip height from the
        // top of the visible rectangle AND its bottom is further up than two strip heights
        // from the bottom of the visible rectangle (calculations done in logical coords);
        // but if both conditions are not satisfied then a scroll either up or down will be
        // required.
		if (yDistFromBundleStartToStripTopLeading > visRectFromBundleStart.GetTop() + nStripHeight && 
			rectStrip.GetBottom() < visRectFromBundleStart.GetBottom() - 2*nStripHeight)
		{
			#ifdef _Trace_Box_Loc_Wrong
			if (pApp->m_nActiveSequNum >1193) 
			{
			TRACE0("*** Exit early, box within one strip of top or bottom ***\n");
			}
			#endif

			return;
		}

        // BEW changed 28Sep05 to fix a scroll positioning bug which occurred when the
        // caller has just done an advance of the bundle which results in the box y-coord
        // suddenly being much smaller but the scroll car is at the bottom of the bar. The
        // legacy code did not check for this, and instead just continued with the
        // visRectFromBundleStart at the bottom of the new bundle -- and consequently the
        // phrase box was above the top of the visRectFromBundleStart -- making the app
        // appear to have hung, and especially so when the ChooseTranslation dialog was
        // opened (the coord calculations were sometimes so far out that the dialog would
        // open off screen and the user would get a bell and no response to any keyboard or
        // mouse action - only an ESC keypress would invoke the dlg's Cancel button and
        // restore responsiveness, but the phrase box remained off window until the user
        // manually scrolled to show it again. The legacy code was nice in that it
        // minimized scroll repositioning, so that the user had a visual sense of working
        // down through the document - the box going lower and lower as he worked. The
        // replacement code will try to maintain this type of behaviour, but it will need
        // to check for bundle advancements and scroll up when these happen.

		int scrollDistanceNeeded;	//int yDist; // +ve or -ve; this is the indicator 
                        // for whether the visible rectangle is lower than we want it to be
                        // (scrollDistanceNeeded will be +ve), or higher than we want it to
                        // be (scrollDistanceNeeded -ve); so dependending on this value we
                        // scroll the visRectFromBundleStart up or down
                        // (scrollDistanceNeeded is calculated as the top of the visible
                        // rectangle minus the distance to the start of the preceding
                        // context pixel span which is to remain visible above the strip
                        // which has the phrasebox)
		//yDist = visRect.top - (yDistActiveStripTop - nPrecedingContextDepth); // could be -ve
		scrollDistanceNeeded = visRectFromBundleStart.GetTop() - 
								yDistFromBundleStartToStripTopLeading; // could be -ve

		if (scrollDistanceNeeded > 0) // if (yDist > 0)
		{
            // the visRectFromBundleStart's top is situated too low, so that some desired
            // context preceding the box is not visible, maybe even the box is not visible,
            // so a scroll up is mandatory. We now have to figure out by what amount, since
            // we can't presume the desired amount of preceding context is always going to
            // be available (a bundle advance may have located the box in the first strip)
			int delta= 0;

            // If we scroll up we must make sure we don't scroll past the start of the
            // bundle. The calculation for a safe scroll up reduces to the condition that
            // yDist must be greater or equal to the current scrollPos.y value; if not,
            // then we can only scroll to the top (a shorter distance than otherwise
            // wanted).
			if (distDocStartToViewRect.y < scrollDistanceNeeded) //if (scrollPos.y < yDist)
			{
				#ifdef _Trace_Box_Loc_Wrong
				if (pApp->m_nActiveSequNum >1193) 
				{
				TRACE2("**Unsafe scroll up, so scrollPos.y reset to zero: yDist  %d, old distDocStartToViewRect.y  %d\n",
						yDist,distDocStartToViewRect.y);
				TRACE3("**Unsafe calc: visRectFromBundleStart.top  %d - yDistFromBundleStartToStripTopLeading %d + nPrecedingContextDepth %d\n",
						visRectFromBundleStart.top,yDistFromBundleStartToStripTopLeading,nPrecedingContextDepth);
				TRACE1("**Unsafe's nLimit value = %d\n", nLimit);
				}
				#endif

                // scrollDistanceNeeded would be a scroll up too far; so scroll just to the
                // beginning of the bundle, i.e., scroll position zero.
				delta = distDocStartToViewRect.y; //delta = 0;
                // The 2nd param of wxWindow::SetScrollPos is the position in scroll units,
                // whereas the second parameter of MFC's SetScrollPos is apparently device
                // units (pixels), therefore we need to divide the wx version position
                // value by yPixelsPerUnit.
				// 
				int posn = delta;
				posn = posn / yPixelsPerUnit;
				//SetScrollPos(wxVERTICAL,0,TRUE); //SetScrollPos(SB_VERT,0,TRUE);
				//                          // WX's SetScrollPos takes scroll units
                // Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount to
                // scroll in device units (pixels). The equivalent in wx is Scroll(x,y) in
                // which x and y are in SCROLL UNITS (pixels divided by pixels per unit).
                // Also MFC's ScrollWindow takes parameters whose value represents an
                // "amount" to scroll from the current position, whereas the
                // wxScrolledWindow::Scroll takes parameters which represent an absolute
                // "position" in scroll units. To convert the amount we need to add the
                // amount to (or subtract from if negative) the logical pixel unit of the
                // upper left point of the client viewing area; then convert to scroll
                // units in Scroll().
                // whm note: wxScrolledWindow::Scroll() scrolls the window so the view
                // start is at the given point (expressed in scroll units)
				 Scroll(0,posn); //Scroll(0,yOrigin); //ScrollWindow(0,delta); 
													  // Scroll() takes scroll units
			}
			else
			{
				// scrollDistanceNeeded is a safe distance to scroll up, so do it
				delta = distDocStartToViewRect.y - scrollDistanceNeeded; // is +ve

                // The 2nd param of wxWindow::SetScrollPos is the position in scroll units,
                // whereas the second parameter of MFC's SetScrollPos is device units
                // (pixels), therefore we need to divide the wx version position value by
                // yPixelsPerUnit.
				int posn = delta;
				posn = posn / yPixelsPerUnit;
				//SetScrollPos(wxVERTICAL,posn,TRUE); //SetScrollPos(SB_VERT,delta,TRUE); 
				//                                 // WX's SetScrollPos takes scroll units
                // Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount to
                // scroll in device units (pixels). The equivalent in wx is Scroll(x,y) in
                // which x and y are in SCROLL UNITS (pixels divided by pixels per unit).
                // Also MFC's ScrollWindow takes parameters whose value represents an
                // "amount" to scroll from the current position, whereas the
                // wxScrolledWindow::Scroll takes parameters which represent an absolute
                // "position" in scroll units. To convert the amount we need to add the
                // amount to (or subtract from if negative) the logical pixel unit of the
                // upper left point of the client viewing area; then convert to scroll
                // units in Scroll().
				// whm: we want to scroll to the posn as calculated above
                // whm note: wxScrolledWindow::Scroll() scrolls the window so the view
                // start is at the given point (expressed in scroll units)
				Scroll(0,posn); //Scroll(0,yOrigin); //ScrollWindow(0,delta); 
					// makes logical coord Origin less negative, that is, a scroll up

				#ifdef _Trace_Box_Loc_Wrong
				if (pApp->m_nActiveSequNum >1193) 
				{
				TRACE1("Safe scroll up by yDist =  %d\n",yDist);
				}
				#endif

			}
		}
		else
		{
            // the visRectFromBundleStart's top is at or above the desired amount of
            // preceding context, i.e., yDistFromBundleStartToStripTopLeading is equal or
            // greater than the top of visRectFromBundleStart's rect space; so a scroll
            // down is required to make more context above active location visible.
            // We also must test to prevent the phrase box from getting within a strip's
            // distance from the bottom of the visible rectangle, because we want the user
            // to have some following context so he can do his adapting meaningfully
			if (rectStrip.GetBottom() > visRectFromBundleStart.GetBottom() - 
				nStripHeight) //if (rectStrip.bottom > nAdjustedVisRectBottom - nStripHeight)
			{
                // a scroll down is needed, if it is possible; we will try to scroll down
                // by at least one strip's depth; if we can't do so because we are at or
                // near the scroll limit already then put the visible rectangle at the
                // scroll limit & do whatever scroll is needed for that to happen
	godown:		int increment = 0;	// whm: In g++ the lower position of this label generated 
                        // an error because the label jump skips the initialization of
                        // increment; so I've moved the label up so that it initializes
                        // increment.

	//godown:		if (rectStrip.GetBottom() < nLimit - nStripHeight)
				if (rectStrip.GetBottom() < nLimit - nStripHeight)
				{
                    // there is room for safe scrolling down by at least one strip plus its
                    // leading, so work out the actual scroll distance required (may be
                    // much greater than one strip) and do it
					increment = yDistFromBundleStartToStripTopLeading 
						- nPrecedingContextDepth - visRectFromBundleStart.GetTop(); 
					wxASSERT(increment >= 0); // whm changed from > to >=
					
					int posn = distDocStartToViewRect.y + increment;
					posn = posn / yPixelsPerUnit;
					//SetScrollPos(wxVERTICAL,posn,TRUE); //SetScrollPos(SB_VERT,scrollPos.y + increment,TRUE);
                    // Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount
                    // to scroll in device units (pixels). The equivalent in wx is
                    // Scroll(x,y) in which x and y are in SCROLL UNITS (pixels divided by
                    // pixels per unit). Also MFC's ScrollWindow takes parameters whose
                    // value represents an "amount" to scroll from the current position,
                    // whereas the wxScrolledWindow::Scroll takes parameters which
                    // represent an absolute "position" in scroll units. To convert the
                    // amount we need to add the amount to (or subtract from if negative)
                    // the logical pixel unit of the upper left point of the client viewing
                    // area; then convert to scroll units in Scroll().
                    // whm note: wxScrolledWindow::Scroll() scrolls the window so the view
                    // start is at the given point (expressed in scroll units)
					Scroll(0,posn);  //Scroll(0,yOrigin); //ScrollWindow(0,-increment); 
						// makes logical coord Origin more negative, that is, a scroll down
					
					#ifdef _Trace_Box_Loc_Wrong
					if (pApp->m_nActiveSequNum >1193) 
					{
					TRACE1("Safe scroll down by increment =  %d\n",increment);
					}
					#endif
				}
				else
				{
                    // we are at or near the scroll limit, so just scroll down whatever
                    // distance we remain short of the limit, i.e, just scroll to the
                    // position of nLimit
					increment = nLimit - visRectFromBundleStart.GetBottom(); // +ve
					
					int posn = increment; //int posn = nLimit;
					posn = posn / yPixelsPerUnit;
					//SetScrollPos(wxVERTICAL,posn,TRUE); //SetScrollPos(SB_VERT,nLimit,TRUE);
                    // Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount
                    // to scroll in device units (pixels). The equivalent in wx is
                    // Scroll(x,y) in which x and y are in SCROLL UNITS (pixels divided by
                    // pixels per unit). Also MFC's ScrollWindow takes parameters whose
                    // value represents an "amount" to scroll from the current position,
                    // whereas the wxScrolledWindow::Scroll takes parameters which
                    // represent an absolute "position" in scroll units. To convert the
                    // amount we need to add the amount to (or subtract from if negative)
                    // the logical pixel unit of the upper left point of the client viewing
                    // area; then convert to scroll units in Scroll(). whm note:
                    // wxScrolledWindow::Scroll() scrolls the window so the view start is
                    // at the given point (expressed in scroll units)
					Scroll(0,posn); //ScrollWindow(0,-increment); 
						//posn is the limit in scroll units // makes logical coord Origin more 
						//negative, that is, a scroll down

					#ifdef _Trace_Box_Loc_Wrong
					if (pApp->m_nActiveSequNum >1193) 
					{
					TRACE1("**Shortened scroll down by increment =  %d\n",increment);
					}
					#endif
				}
			}
			else
			{
                // we don't need to scroll. However, if we do nothing here we can get the
                // phrasebox moving forward doing auto insertions in the strip at the very
                // bottom of the visible rectangle which is a nuisance because the user
                // sees not enough meaningful context ahead of the box location; so we'll
                // detect this and when it happens we'll force a scroll down to put the
                // active strip much further up in the window
				if (rectStrip.GetBottom() > visRectFromBundleStart.GetBottom() - nStripHeight)
				{
					#ifdef _Trace_Box_Loc_Wrong
					if (pApp->m_nActiveSequNum >1193) 
					{
					TRACE0("***Force a scroll down as active strip is at bottom of visible rectangle***\n");
					}
					#endif

					goto godown;
				}
				// otherwise do nothing
				;
				#ifdef _Trace_Box_Loc_Wrong
				if (pApp->m_nActiveSequNum >1193) 
				{
				TRACE0("***Don't scroll in either direction***\n");
				}
				#endif
			}
		}
		*/

int CAdapt_ItCanvas::ScrollDown(int nStrips)
// returns positive y-distance for the scroll down (whm: return value is never used)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CLayout* pLayout = pApp->m_pLayout;
	wxPoint scrollPos;
	int xPixelsPerUnit,yPixelsPerUnit;
	GetScrollPixelsPerUnit(&xPixelsPerUnit,&yPixelsPerUnit);
	
    // MFC's GetScrollPosition() "gets the location in the document to which the upper left
    // corner of the view has been scrolled. It returns values in logical units." wx note:
    // The wx docs only say of GetScrollPos(), that it "Returns the built-in scrollbar
    // position." I assume this means it gets the logical position of the upper left
    // corner, but it is in scroll units which need to be converted to device (pixel) units
	//scrollPos.x = GetScrollPos(wxHORIZONTAL); //wxPoint scrollPos = GetScrollPosition();
	//scrollPos.y = GetScrollPos(wxVERTICAL); //wxPoint scrollPos = GetScrollPosition();
	//scrollPos.x *= xPixelsPerUnit; wxASSERT(scrollPos.x == 0);
	//scrollPos.y *= yPixelsPerUnit;

//#ifdef _DEBUG
//	int xOrigin, yOrigin;
//	// the view start is effectively the scroll position, but GetViewStart returns scroll units
//	GetViewStart(&xOrigin, &yOrigin); // gets xOrigin and yOrigin in scroll units
//	xOrigin = xOrigin * xPixelsPerUnit; // number pixels is ScrollUnits * pixelsPerScrollUnit
//	yOrigin = yOrigin * yPixelsPerUnit;
//	wxASSERT(xOrigin == scrollPos.x);
//	wxASSERT(yOrigin == scrollPos.y);
//#endif

//#ifdef _DEBUG
//	int newXPos, newYPos;
//	// We can simplify all of the above by using this calc instead
//	CalcUnscrolledPosition(0,0,&newXPos,&newYPos);
//	wxASSERT(newXPos == scrollPos.x); // scrollPos.x = newXPos;
//	wxASSERT(newYPos == scrollPos.y); // scrollPos.y = newYPos;
//#endif
	
	CalcUnscrolledPosition(0,0,&scrollPos.x,&scrollPos.y);

	wxRect visRect; // wxRect rectClient;
	//GetClientSize(&visRect.width,&visRect.height); //pFrame->GetClientRect(&rectClient);
    // wx note: calling GetClientSize on the canvas produced different results in wxGTK and
    // wxMSW, so I'll use my own GetCanvasClientSize() which calculates it from the main
    // frame's client size.
	wxSize canvasSize;
	canvasSize = pApp->GetMainFrame()->GetCanvasClientSize();
	visRect.width = canvasSize.x;
	visRect.height = canvasSize.y;

	// make adjustment due to fact that some parameters values are calculated differently
	// in the refactored application
	int nCurrentPileHeight = pLayout->GetPileHeight();
	if (pApp->m_bFreeTranslationMode)
	{
		// the legacy app included the 3 pixels and tgt text height in the 
		// m_curPileHeight calculation
		nCurrentPileHeight += 3 + pLayout->GetTgtTextHeight();
	}

	// calculate the window depth
	int yDist = 0;
	int nLimit = GetScrollRange(wxVERTICAL);
    // whm note: GetscrollRange returns scroll units, and MFC's GetScrollLimit apparently
    // returns device units (pixels), so must multiply the value obtained from
    // GetScrollRange by yPixelsPerUnit GetScrollRange got the range of scroll units for
    // the whole virtual document, not just the value of the upper left y coord for the
    // window when maximally scrolled.
    // To find the y coord for the client window at its maximal scrolled extent we need to
    // do the following:
	// Take the modulus of DocLengthInScrollUnits % ScrollUnitsPerPage
    // The modulus operation will give the number of scroll units that exist beyond the
    // last fully scrolled page. Multiply this value by pixelsPerScrollUnit to get the
    // amount the y coord of the client view should be moved up toward the beginning of the
    // doc to be the same value that MFC's GetScrollLimit obtains.
	int scrollUnitsPerPage = GetScrollThumb(wxVERTICAL);
	int unitsBeyondLastFullScrolledPage = nLimit % scrollUnitsPerPage;
	// reduce the nLimit by the unitsBeyondLastFullScrolledPage.
	nLimit -= unitsBeyondLastFullScrolledPage; // to make wxWindow::GetScrollRange == 
											   // CWnd::GetScrollLimit
	nLimit *= yPixelsPerUnit; // convert scroll units to pixels
	int nMaxDist = nLimit - scrollPos.y;

	// do the vertical scroll asked for
	//yDist = pApp->m_curPileHeight + pApp->m_curLeading;
	yDist = nCurrentPileHeight + pLayout->GetCurLeading();
	yDist *= nStrips;

	if (yDist > nMaxDist)
	{
		scrollPos.y += nMaxDist;

		int posn = scrollPos.y;
		posn = posn / yPixelsPerUnit;
		//SetScrollPos(wxVERTICAL,posn,TRUE); //SetScrollPos(SB_VERT,scrollPos.y,TRUE); 
		//                                     // WX's SetScrollPos takes scroll units
        // Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount to scroll in
        // device units (pixels). The equivalent in wx is Scroll(x,y) in which x and y are
        // in SCROLL UNITS (pixels divided by pixels per unit). Also MFC's ScrollWindow
        // takes parameters whose value represents an "amount" to scroll from the current
        // position, whereas the wxScrolledWindow::Scroll takes parameters which represent
        // an absolute "position" in scroll units. To convert the amount we need to add the
        // amount to (or subtract from if negative) the logical pixel unit of the upper
        // left point of the client viewing area; then convert to scroll units in Scroll().
        // whm note: wxScrolledWindow::Scroll() scrolls the window so the view start is at
        // the given point (expressed in scroll units)
		Scroll(0,posn);
		Refresh();
		return nMaxDist;
	}
	else
	{
		scrollPos.y += yDist;

		int posn = scrollPos.y;
		posn = posn / yPixelsPerUnit;
		//SetScrollPos(wxVERTICAL,posn,TRUE); //SetScrollPos(SB_VERT,scrollPos.y,TRUE); 
		//                                     // WX's SetScrollPos takes scroll units
        // Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount to scroll in
        // device units (pixels). The equivalent in wx is Scroll(x,y) in which x and y are
        // in SCROLL UNITS (pixels divided by pixels per unit). Also MFC's ScrollWindow
        // takes parameters whose value represents an "amount" to scroll from the current
        // position, whereas the wxScrolledWindow::Scroll takes parameters which represent
        // an absolute "position" in scroll units. To convert the amount we need to add the
        // amount to (or subtract from if negative) the logical pixel unit of the upper
        // left point of the client viewing area; then convert to scroll units in Scroll().
        // whm note: wxScrolledWindow::Scroll() scrolls the window so the view start is at
        // the given point (expressed in scroll units)
		Scroll(0,posn);
		Refresh();
		return yDist;
	}
}

int CAdapt_ItCanvas::ScrollUp(int nStrips)
// returns positive y-distance for the scroll up (whm: return value is never used)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CLayout* pLayout = pApp->m_pLayout;
	wxPoint scrollPos; 
	int xPixelsPerUnit,yPixelsPerUnit;
	GetScrollPixelsPerUnit(&xPixelsPerUnit,&yPixelsPerUnit);
	
    // MFC's GetScrollPosition() "gets the location in the document to which the upper left
    // corner of the view has been scrolled. It returns values in logical units."
    // wx note: The wx docs only say of GetScrollPos(), that it "Returns the built-in
    // scrollbar position." I assume this means it gets the logical position of the upper
    // left corner, but it is in scroll units which need to be converted to device (pixel)
    // units
	//scrollPos.x = GetScrollPos(wxHORIZONTAL); //wxPoint scrollPos = GetScrollPosition();
	//scrollPos.y = GetScrollPos(wxVERTICAL); //wxPoint scrollPos = GetScrollPosition();
	//scrollPos.x *= xPixelsPerUnit; wxASSERT(scrollPos.x == 0);
	//scrollPos.y *= yPixelsPerUnit;

//#ifdef _DEBUG
//	int xOrigin, yOrigin;
//	// the view start is effectively the scroll position, but GetViewStart returns scroll units
//	GetViewStart(&xOrigin, &yOrigin); // gets xOrigin and yOrigin in scroll units
//	xOrigin = xOrigin * xPixelsPerUnit; // number pixels is ScrollUnits * pixelsPerScrollUnit
//	yOrigin = yOrigin * yPixelsPerUnit;
//	wxASSERT(xOrigin == scrollPos.x); // scrollPos.x = xOrigin;
//	wxASSERT(yOrigin == scrollPos.y); // scrollPos.y = yOrigin;
//#endif

//#ifdef _DEBUG
//	int newXPos, newYPos;
//	// We can simplify all of the above by using this calc instead
//	CalcUnscrolledPosition(0,0,&newXPos,&newYPos);
//	wxASSERT(newXPos == scrollPos.x); // scrollPos.x = newXPos;
//	wxASSERT(newYPos == scrollPos.y); // scrollPos.y = newYPos;
//#endif
	
	CalcUnscrolledPosition(0,0,&scrollPos.x,&scrollPos.y);

	int yDist;
	int nMaxDist = scrollPos.y;

	// make adjustment due to fact that some parameters values are calculated differently
	// in the refactored application
	int nCurrentPileHeight = pLayout->GetPileHeight();
	if (pApp->m_bFreeTranslationMode)
	{
		// the legacy app included the 3 pixels and tgt text height in the 
		// m_curPileHeight calculation
		nCurrentPileHeight += 3 + pLayout->GetTgtTextHeight();
	}

	// do the vertical scroll asked for
	//yDist = pApp->m_curPileHeight + pApp->m_curLeading;
	yDist = nCurrentPileHeight + pLayout->GetCurLeading();
	yDist *= nStrips;

	if (yDist > nMaxDist)
	{
        // the amount of scroll wanted is greater than the amount the window is already
        // scrolled, so only scroll up the exact amount the window is scrolled, bringing it
        // up to an unscrolled state
		scrollPos.y -= nMaxDist;
		int posn = scrollPos.y;
		wxASSERT(posn == 0); // should be zero
		posn = posn / yPixelsPerUnit;
		//SetScrollPos(wxVERTICAL,posn,TRUE); //SetScrollPos(SB_VERT,scrollPos.y,TRUE);
        // Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount to scroll in
        // device units (pixels). The equivalent in wx is Scroll(x,y) in which x and y are
        // in SCROLL UNITS (pixels divided by pixels per unit). Also MFC's ScrollWindow
        // takes parameters whose value represents an "amount" to scroll from the current
        // position, whereas the wxScrolledWindow::Scroll takes parameters which represent
        // an absolute "position" in scroll units. To convert the amount we need to add the
        // amount to (or subtract from if negative) the logical pixel unit of the upper
        // left point of the client viewing area; then convert to scroll units in Scroll().
        // whm note: wxScrolledWindow::Scroll() scrolls the window so the view start is at
        // the given point (expressed in scroll units)
		Scroll(0,posn);
		Refresh();
		return nMaxDist;
	}
	else
	{
		// scroll up the amount requested
		scrollPos.y -= yDist;

		int posn = scrollPos.y;
		posn = posn / yPixelsPerUnit;
		//SetScrollPos(wxVERTICAL,posn,TRUE); //SetScrollPos(SB_VERT,scrollPos.y,TRUE);
        // Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount to scroll in
        // device units (pixels). The equivalent in wx is Scroll(x,y) in which x and y are
        // in SCROLL UNITS (pixels divided by pixels per unit). Also MFC's ScrollWindow
        // takes parameters whose value represents an "amount" to scroll from the current
        // position, whereas the wxScrolledWindow::Scroll takes parameters which represent
        // an absolute "position" in scroll units. To convert the amount we need to add the
        // amount to (or subtract from if negative) the logical pixel unit of the upper
        // left point of the client viewing area; then convert to scroll units in Scroll().
        // whm note: wxScrolledWindow::Scroll() scrolls the window so the view start is at
        // the given point (expressed in scroll units)
		Scroll(0,posn);
		Refresh();
		return yDist;
	}
}

/*
// whm: ScrollToNearTop is NOT USED ANY LONGER!!!
// whm: wx version changes the names of some variables for ease of understanding and
// maintenance. The wx version also must convert scroll units to pixels scrolled because
// most of the scrolling functions in wx use or return scroll units, rather than pixels.
void CAdapt_ItCanvas::ScrollToNearTop(int nSequNum)
{
	bool debugDisableScrollIntoView = FALSE; // set TRUE to disable ScrollIntoView
	if (!debugDisableScrollIntoView)
	{
		CAdapt_ItApp* pApp = &wxGetApp();
		wxASSERT(pApp != NULL);
		CAdapt_ItView* pView = pApp->GetView();
		// get the phrase box location and strip index
		CPile* pPile = pView->GetPile(nSequNum);
		int nStripIndex = pPile->m_pStrip->m_nStripIndex;
		int scrollDistanceNeeded; // always calculated so as to be positive

		#ifdef _Trace_Box_Loc_Wrong2
		TRACE1("BUG nStripIndex  %d\n",nStripIndex);
		#endif

        // get the current horizontal and vertical pixels scrolled (this value is the value
        // pertaining to the phrase box's previous position, which we are in the process of
        // working out what scroll is required; the layout for the new position of the
        // phrase box will have been done before ScrollToNearTop() was called, so the new
        // location's offset from the start of the document is known and ResizeBox() will
        // have been called -- the latter uses the active pile's rect values and has offset
        // them by an LPtoDP() calculation which uses the scrollPos.y value (pre-scroll))
		wxPoint distDocStartToViewRect; //wxPoint scrollPos;
		int xPixelsPerUnit,yPixelsPerUnit;
		GetScrollPixelsPerUnit(&xPixelsPerUnit,&yPixelsPerUnit);
        // MFC's GetScrollPosition() "gets the location in the document to which the upper
        // left corner of the view has been scrolled. It returns values in logical units."
        // wx note: The wx docs only say of GetScrollPos(), that it "Returns the built-in
        // scrollbar position." I assume this means it gets the logical position of the
        // upper left corner, but it is in scroll units which need to be converted to
        // device (pixel) units by multiplying by PixelsPerUnit
		distDocStartToViewRect.x = GetScrollPos(wxHORIZONTAL); //wxPoint scrollPos = GetScrollPosition();
		distDocStartToViewRect.y = GetScrollPos(wxVERTICAL); //wxPoint scrollPos = GetScrollPosition();
		distDocStartToViewRect.x *= xPixelsPerUnit; wxASSERT(distDocStartToViewRect.x == 0);
		distDocStartToViewRect.y *= yPixelsPerUnit;
        // Rather than using the wxWindow::GetScrollPos, we could use the
        // wxScrolledWindow::GetViewStart() which returns the same value(s) in scroll units
        // as wxWindow::GetScrollPos(), but GetViewStart returns both the x and y positions
        // whereas GetScrollPos(int orientation) returns only the x or the y depending on
        // the input parameter.
		
		// testing below
		int xDistScrollUnits, yDistScrollUnits, xtemp,ytemp;
		GetViewStart(&xDistScrollUnits, &yDistScrollUnits); // gets xOrigin and yOrigin in scroll units
		xtemp = xDistScrollUnits * xPixelsPerUnit; // number pixels is ScrollUnits * pixelsPerScrollUnit
		ytemp = yDistScrollUnits * yPixelsPerUnit;
		wxASSERT(distDocStartToViewRect.x == xtemp);
		wxASSERT(distDocStartToViewRect.y == ytemp);
		// testing above

		#ifdef _Trace_Box_Loc_Wrong2
		TRACE1("STNT ScrollPos.y  %d  BEFORE OffsetRect()\n",scrollPos.y);
		#endif

        // Get the required y-coord of the top of the phrase box's strip plus preceding
        // leading -- that is, the distance from the start of the document to the beginning
        // of the leading for the active strip (the new value was determined by a prior
        // call to RecalcLayout). This distance increases as the visRect scrolls down, but
        // suddenly decreases to a small value when the caller has just done a bundle
        // advance.
		int yDistFromBundleStartToStripTopLeading = pPile->GetPileRect().GetTop() - pApp->m_curLeading;
		wxASSERT(yDistFromBundleStartToStripTopLeading >= 0);

		#ifdef _Trace_Box_Loc_Wrong2
		TRACE1("STNT yDistFromBundleStartToStripTopLeading  %d\n",yDistFromBundleStartToStripTopLeading);
		#endif

        // get the strip height, we want the scrolled position to be such that a strip of
        // context remains visible at the top of the window above the current strip's
        // location
		int nStripHeight = pApp->m_curPileHeight + pApp->m_curLeading;

		#ifdef _Trace_Box_Loc_Wrong2
		TRACE1("STNT nStripHeight  %d  \n",nStripHeight);
		#endif

		// get the visible rectangle's coordinates
		wxRect visRect; // wxRect rectClient;
		//GetClientSize(&visRect.width,&visRect.height);//GetClientRect(&rectClient);
        // wx note: calling GetClientSize on the canvas produced different results in wxGTK
        // and wxMSW, so I'll use my own GetCanvasClientSize() which calculates it from the
        // main frame's client size.
		wxSize canvasSize;
		canvasSize = pApp->GetMainFrame()->GetCanvasClientSize();
		visRect.width = canvasSize.x;
		visRect.height = canvasSize.y;

		//wxRect visRect = rectClient;
		int nWindowDepth = visRect.GetHeight(); // to get accurate nLimit below

		#ifdef _Trace_Box_Loc_Wrong2
		TRACE2("STNT visRect {top  %d , left  %d } BEFORE OffsetRect(scrollPos)\n",
			visRect.GetTop(),visRect.GetLeft());
		#endif
		
		wxRect visRectFromBundleStart = visRect;

		// offset the visRect in logical coord space
		visRectFromBundleStart.Offset(distDocStartToViewRect); //visRect.OffsetRect(scrollPos);

		#ifdef _Trace_Box_Loc_Wrong2
		TRACE2("STNT visRect {top  %d , left  %d } AFTER OffsetRect(scrollPos)\n",visRectFromBundleStart.top,visRectFromBundleStart.left);
		#endif

        // bleed out the case where the phrase box's strip is the one with index 0 --
        // because for this situation we cannot possibly show a strip above the phrase
        // box's strip, instead we must reset the scroll to 0 and bring the Origin back to
        // (0,0)
		if (nStripIndex == 0)
		{
			//scrollDistanceNeeded = scrollPos.y; // distance to scroll the visRect up
			//scrollPos.y =0;

			//int posn = scrollPos.y;
			int posn = 0; //posn = posn / yPixelsPerUnit;
			//??//SetScrollPos(wxVERTICAL,posn,TRUE); //SetScrollPos(SB_VERT,scrollPos.y,TRUE); // posn will be zero
            // Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount to
            // scroll in device units (pixels). The equivalent in wx is Scroll(x,y) in
            // which x and y are in SCROLL UNITS (pixels divided by pixels per unit). Also
            // MFC's ScrollWindow takes parameters whose value represents an "amount" to
            // scroll from the current position, whereas the wxScrolledWindow::Scroll takes
            // parameters which represent an absolute "position" in scroll units. To
            // convert the amount we need to add the amount to (or subtract from if
            // negative) the logical pixel unit of the upper left point of the client
            // viewing area; then convert to scroll units in Scroll().
			//int xOrigin,yOrigin;
			//GetViewStart(&xOrigin,&yOrigin);
			//// the x scroll position remains 0 scroll units, but the y scroll position is 
			// scrollDistanceNeeded / yPixelsPerUnit
			//yOrigin = yOrigin * yPixelsPerUnit; // convert from scroll units to pixels
			//// scroll up is negative increment in scroll units
			//yOrigin = yOrigin - abs(scrollDistanceNeeded); // scrollDistanceNeeded is the 
			//                 // scroll amount in pixels; it will be negative to scroll up
			//yOrigin /= yPixelsPerUnit;
			//wxASSERT(yOrigin >= 0);
			Scroll(0,posn); //ScrollWindow(0,scrollDistanceNeeded); // with scroll units, 
							// we simply Scroll to 0,0
			Refresh(); //Invalidate();
			return;
		}

        // do we need to scroll up or down? What this amounts to is a test for
        // yDistFromBundleStartToStripTopLeading > or < than the sum of the (offset)
        // visRect.top plus nStripHeight. If the greater than test yields TRUE then the
        // active strip lies more than one strip (including its leading) below the top of
        // the visRect, and so a scroll down is required to keep the active strip having
        // just one visible strip above it; if the less than test yields TRUE then the
        // active strip lies above one strip down from the top of the visRect, and so a
        // scroll up is required to keep the active strip having just one visible strip
        // above it; if both tests are FALSE then nothing need be done because that means
        // the phrase box's strip is already exactly where we want it to be - showing
        // exactly one strip plus its leading above the box's strip in the visRect.
        // BEW modified 07Oct05, because if the phrase box has not come close enough to the
        // end of the bundle to force a bundle recalculation, but the scroll car has
        // advanced to its maximum position and so cannot support a further scroll down for
        // the visible rectange, the legacy code did not take the latter fact into account
        // and the phrase box was shown above where it should be - so I've added code to
        // test for this and we don't scroll once the scroll car has come to the vertical
        // limit.
		int nLimit = GetScrollRange(wxVERTICAL); //GetScrollLimit(SB_VERT);
		// whm note: GetscrollRange returns scroll units, and MFC's GetScrollLimit apparently returns
		// device units (pixels), so must multiply the value obtained from GetScrollRange by yPixelsPerUnit
		nLimit *= yPixelsPerUnit;
		nLimit -= nWindowDepth; // to make wxWindow::GetScrollRange == CWnd::GetScrollLimit we subtract window depth

        // first test to see if a scroll down is required (active location needs to move up
        // in the visible window), or a scroll up (active location needs to move down in
        // the visible window)
		if (yDistFromBundleStartToStripTopLeading > visRectFromBundleStart.GetTop() + nStripHeight)
		{
            // scroll down is required (active location needs to move up in the visible
            // window), i.e., visRectFromBundleStart.y needs to become greater
			#ifdef _Trace_Box_Loc_Wrong2
			TRACE1("STNT  Scroll DOWN, box is too low;  nLimit = %d\n",nLimit);
			#endif

			// calculate the scroll distance (scrollDistanceNeeded)
			scrollDistanceNeeded = yDistFromBundleStartToStripTopLeading - 
									(visRectFromBundleStart.GetTop() + nStripHeight); 
			wxASSERT(scrollDistanceNeeded >= 0);

			#ifdef _Trace_Box_Loc_Wrong2
			TRACE3("STNT scrollDistanceNeeded calc: %d - ( %d + %d) ",yDistFromBundleStartToStripTopLeading,visRectFromBundleStart.GetTop(),nStripHeight);
			TRACE1(" =  %d\n",scrollDistanceNeeded);
			#endif

            // BEW added 07Oct05, test that the calculated scroll amount does not take the
            // scroll car past the limit; it it does, make the scroll be just to the limit,
            // or no scroll if already at the limit, so that the phrase box will show lower
            // in the visible rectangle -- that is we must abandon the requirement that we
            // keep the box near the top, in this circumstance
			if (distDocStartToViewRect.y + scrollDistanceNeeded > nLimit)
			{
				// we can't go that far, so just do what we can
				scrollDistanceNeeded = nLimit - distDocStartToViewRect.y;
				distDocStartToViewRect.y += scrollDistanceNeeded;

				//int posn = scrollPos.y;
				//posn = posn / yPixelsPerUnit;
				// we set the scroll pos at the limit
				int posn = distDocStartToViewRect.y / yPixelsPerUnit;
				//??//SetScrollPos(wxVERTICAL,posn,TRUE); //SetScrollPos(SB_VERT,scrollPos.y,TRUE);
                // Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount to
                // scroll in device units (pixels). The equivalent in wx is Scroll(x,y) in
                // which x and y are in SCROLL UNITS (pixels divided by pixels per unit).
                // Also MFC's ScrollWindow takes parameters whose value represents an
                // "amount" to scroll from the current position, whereas the
                // wxScrolledWindow::Scroll takes parameters which represent an absolute
                // "position" in scroll units. To convert the amount we need to add the
                // amount to (or subtract from if negative) the logical pixel unit of the
                // upper left point of the client viewing area; then convert to scroll
                // units in Scroll().
				//int xOrigin,yOrigin;
				//GetViewStart(&xOrigin,&yOrigin);
				//yOrigin = yOrigin * yPixelsPerUnit; // convert from scroll units to pixels
				//yOrigin = yOrigin - scrollDistanceNeeded; // scrollDistanceNeeded is the scroll amount in pixels
				//// the x scroll position remains 0 scroll units, but the y scroll position is -(delta / yPixelsPerUnit)
				//yOrigin /= yPixelsPerUnit;
				Scroll(0,posn); //ScrollWindow(0,-scrollDistanceNeeded); 
						// posn is the limit in scroll units // makes the Origin be more -ve 
						// in the y-axis, ie, a scroll down
				//ScrollWindow(0,-scrollDistanceNeeded);
				Refresh(); //Invalidate();

				#ifdef _Trace_Box_Loc_Wrong2
				TRACE1("Adjusted Scroll DOWN:  %d  pixels\n",delta);
				#endif

				return;
			}
			else
			{
				// distDocStartToViewRect.y + scrollDistanceNeeded <= nLimit
				// do the vertical scroll required to bring the phrase box up near the 
				// top of the client rectangle
				distDocStartToViewRect.y += scrollDistanceNeeded;

				int posn = distDocStartToViewRect.y;
				posn = posn / yPixelsPerUnit;
				//??//SetScrollPos(wxVERTICAL,posn,TRUE); //SetScrollPos(SB_VERT,scrollPos.y,TRUE);
                // Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount to
                // scroll in device units (pixels). The equivalent in wx is Scroll(x,y) in
                // which x and y are in SCROLL UNITS (pixels divided by pixels per unit).
                // Also MFC's ScrollWindow takes parameters whose value represents an
                // "amount" to scroll from the current position, whereas the
                // wxScrolledWindow::Scroll takes parameters which represent an absolute
                // "position" in scroll units. To convert the amount we need to add the
                // amount to (or subtract from if negative) the logical pixel unit of the
                // upper left point of the client viewing area; then convert to scroll
                // units in Scroll().
				//int xOrigin,yOrigin;
				//GetViewStart(&xOrigin,&yOrigin);
				//yOrigin = yOrigin * yPixelsPerUnit; // convert from scroll units to pixels
				//// scroll up in scroll units is negative increment
				//yOrigin = yOrigin - abs(delta); // delta is the scroll amount in pixels; 
				//                                    // it will be negative to scroll up
				// the x scroll position remains 0 scroll units, but the 
				// y scroll position is -(delta / yPixelsPerUnit)
				//yOrigin /= yPixelsPerUnit;
				// whm: we want to scroll to the posn as calculated above
				Scroll(0,posn); //Scroll(0,yOrigin); //ScrollWindow(0,-delta); 
						// makes the Origin be more -ve in the y-axis, ie, a scroll down
				//ScrollWindow(0,-delta);
				Refresh(); //Invalidate();

					#ifdef _Trace_Box_Loc_Wrong2
					TRACE1("Unadjusted Scroll DOWN:  %d  pixels\n",delta);
					#endif

				return;
			}
		}
		else
		{
			// a scroll up is needed (active location need to move down in the visible 
			// window)
			#ifdef _Trace_Box_Loc_Wrong2
			TRACE0("STNT Scroll UP, box is too high\n");
			#endif
			
			// calculate the scroll distance
			scrollDistanceNeeded = distDocStartToViewRect.y + nStripHeight - 
											yDistFromBundleStartToStripTopLeading;
			wxASSERT(scrollDistanceNeeded >= 0);

			#ifdef _Trace_Box_Loc_Wrong2
			TRACE3("STNT delta calc: %d  +  %d  -  %d) ",distDocStartToViewRect.y,
					nStripHeight,yDistFromBundleStartToStripTopLeading);
			TRACE1(" =  %d\n",scrollDistanceNeeded);
			#endif

			distDocStartToViewRect.y -= scrollDistanceNeeded;

			int posn = distDocStartToViewRect.y;
			posn = posn / yPixelsPerUnit;
			//??//SetScrollPos(wxVERTICAL,posn,TRUE); //SetScrollPos(SB_VERT,scrollPos.y,TRUE);
            // Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount to
            // scroll in device units (pixels). The equivalent in wx is Scroll(x,y) in
            // which x and y are in SCROLL UNITS (pixels divided by pixels per unit). Also
            // MFC's ScrollWindow takes parameters whose value represents an "amount" to
            // scroll from the current position, whereas the wxScrolledWindow::Scroll takes
            // parameters which represent an absolute "position" in scroll units. To
            // convert the amount we need to add the amount to (or subtract from if
            // negative) the logical pixel unit of the upper left point of the client
            // viewing area; then convert to scroll units in Scroll().
			//int xOrigin,yOrigin;
			//GetViewStart(&xOrigin,&yOrigin);
			// the x scroll position remains 0 scroll units, but the y 
			// scroll position is delta / yPixelsPerUnit
			//yOrigin = yOrigin * yPixelsPerUnit; // convert from scroll units to pixels
			//// scroll up in scroll units is negative increment
			//yOrigin = yOrigin - abs(delta); // delta is the scroll amount in pixels; 
			//                                     // it will be negative to scroll up
			//yOrigin /= yPixelsPerUnit;
			Scroll(0,posn); //Scroll(0,yOrigin); //ScrollWindow(0,distDocStartToViewRect); 
						// makes the Origin be less -ve in the y-axis, ie, a scroll up
			//ScrollWindow(0,distDocStartToViewRect);
			Refresh(); //Invalidate();
			return;
		}
	}
}
*/

// whm added 13Sep06 to simplify scrolling of phrasebox or strip anchor location into view
// GetAnchorSegmentVisibility returns either anchorIsAboveVisRect, anchorIsBelowVisRect, or
// anchorIsWithinVisRect. The enum value anchorIsAboveVisRect is returned if any part of
// the segment rect plus nPrecStrips are above the grectViewClient. The enum value
// anchorIsBelowVisRect is returned if any part of the segment rect (including strips that
// are part of the segment) plus nFollStrips is located below the grectViewClient. The enum
// value anchorIsWithinVisRect is returned if the segment, its leading and any required
// visible strips are wholly within grectViewClient.
// Where GetAnchorSegmentVisibility() is used, the appropriate scrolling routine
// (ScrollIntoView or ScrollToNearTop) only needs to be called it returns
// anchorIsAboveVisRect or anchorIsBelowVisRect.
//enum AnchorStripLocation CAdapt_ItCanvas::GetAnchorSegmentVisibility(
//                                                 int nPrecStrips, int nFollStrips)
//{
// When not in free translation mode, the "anchor segment" is simply the strip plus the
// preceding leading where RecalcLayout says the phrasebox should be located. When in free
// translation mode, the "anchor segment" may be formed by the outer dimensions of one or
// more strips, as many as are required for the whole free translation segment, plus the
// preceding leading of the first strip - the strip containing the anchor/phrasebox
// location.
//
//	wxRect stripRectLogical;
//	//stripRectLogical = wxRect(pApp->m_ptCurBoxLocation, 
//	CPile* pPile = pApp->m_pActivePile; // current box location
//	wxASSERT(pPile != NULL);
//	int precedingContextHeight = nPrecStrips * (pApp->m_curPileHeight + pApp->m_curLeading);
//	int followingContextHeight = nFollStrips * (pApp->m_curPileHeight + pApp->m_curLeading);
//
//	if (pApp->m_bFreeTranslationMode)
//	{
//		// the "anchor segment" is defined by the upper left point of the first strip and the
//		// lower right point of the last strip
//		CPile* pLastPile;
//		pLastPile = (CPile*)gpCurFreeTransSectionPileArray->Last();
//		wxASSERT(pLastPile != NULL);
//		stripRectLogical = wxRect(pPile->m_pStrip->m_rectStrip.GetLeftTop(),
//							pLastPile->m_pStrip->m_rectStrip.GetBottomRight());
//	}
//	else
//	{
//		// the "anchor segment" is simply the strip rect containing the phrasebox
//		stripRectLogical = pPile->m_pStrip->m_rectStrip;
//	}
//	// expand stripRectLogical in the vertical direction to include the height of the leading,
//	// and any precedingContextHeight requirements, i.e., the top of the rect should be set to 
//	// a smaller value (decremented by the combined values of the leading height and 
//	// precedingContextHeight)
//	int adjustedStripTop = stripRectLogical.GetTop() + precedingContextHeight;
//	adjustedStripTop -= pApp->m_curLeading;
//	stripRectLogical.SetTop(adjustedStripTop);
//	// further expand stripRectLogical in the vertical direction to include the height of any
//	// followingContextHeight, i.e., the bottom of the rect should be set to a larger value
//	// (incremented by the value of the followingContextHeight).
//	int adjustedStripBottom = stripRectLogical.GetBottom();
//	adjustedStripBottom += followingContextHeight;
//	stripRectLogical.SetBottom(adjustedStripBottom);
//
//	// Now determine the relative location of stripRectLogical in relation to the current view's
//	// rect; stripRectLogical is either within (intersects) the current view's rect, or lies
//	// above or below it.
//	// The intersection is the largest rectangle contained in both existing
//	// rectangles. Hence, when IntersectRect(&r,&rectTest) returns TRUE, it
//	// indicates that there is some client area to draw on.
//	if (stripRectLogical.GetTop() < grectViewClient.GetTop())
//		return anchorIsAboveVisRect;
//	if (stripRectLogical.GetBottom() > grectViewClient.GetBottom())
//		return anchorIsBelowVisRect;
//	return anchorIsWithinVisRect;
//}

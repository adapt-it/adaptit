/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AIPrintout.cpp
/// \author			Bill Martin
/// \date_created	28 February 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the AIPrintout class. 
/// The AIPrintout class manages the functions for printing and print previewing from
/// within Adapt It using the File | Print and File | Print Preview menu selections.
/// \derivation		The AIPrintout class is derived from wxPrintout.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in AIPrintout.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "AIPrintout.h"
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
#include <wx/print.h>

#include "Adapt_It.h"
#include "Adapt_ItView.h"
#include "Pile.h"
#include "Cell.h"
#include "MainFrm.h"
//#include "SourceBundle.h"
#include "Adapt_ItCanvas.h"
#include "Layout.h"
#include "AIPrintout.h"


/// This global is defined in Adapt_It.cpp.
//extern CAdapt_ItApp* gpApp; // for rapid access to the app class

extern int gnCurPage;

/// This global is defined in Adapt_ItView.cpp.
extern bool gbPrintFooter;

extern bool gbPrintingSelection;
extern bool gbPrintingRange;
extern int gnFromChapter;
extern int gnFromVerse;
extern int gnToChapter;
extern int gnToVerse;
extern bool gbIsBeingPreviewed;
extern bool gbSuppressPrecedingHeadingInRange;
extern bool gbIncludeFollowingHeadingInRange;
extern bool gbIsPrinting;
extern int gnPrintingLength;
extern int gnTopGap;
extern int gnBottomGap;
extern int gnFooterTextHeight;

// This global is defined in Adapt_It.cpp.
//extern bool	gbRTL_Layout;	// ANSI version is always left to right reading; this flag can only
							// be changed in the Unicode version, using the extra Layout menu

// The AIPrPreviewFrame class below ///////////////////////////////////////////////////////////// 

// These class methods can be uncommented if it is necessary to implement the AIPreviewFrame class in
// order to access the preview frame's methods such as OnCloseWindow().
// 
//BEGIN_EVENT_TABLE(AIPreviewFrame, wxPreviewFrame)
//	EVT_CLOSE(AIPreviewFrame::OnCloseWindow)
//END_EVENT_TABLE()
//	
//AIPreviewFrame::AIPreviewFrame(wxPrintPreview* preview, wxWindow* parent, const wxString& title, 
//		const wxPoint& pos, const wxSize& size)
//	:wxPreviewFrame(preview,parent,title,pos,size)
//{
//}
//
//void AIPreviewFrame::OnCloseWindow(wxCloseEvent& event)
//{
//	wxPreviewFrame::OnCloseWindow(event); // must call the base class method for the window to be destroyed
//}

// End of AIPrPreviewFrame class  ///////////////////////////////////////////////////////////// 

// The AIPrintout class below ///////////////////////////////////////////////////////////// 
IMPLEMENT_DYNAMIC_CLASS(AIPrintout, wxPrintout)

// WX Documentation Note: The following comments utilize a nifty Visual Studio Addin called
// HyperAddin which is freely available from http://www.codeplex.com/hyperAddin/. HyperAddin also has
// built-in automatic comment wrapping support and other features that enhance the commenting process
// within Visual Studio 2005 or 2008 (it does not work with Visual Studio 2003). To jump to the
// underlined hyperlink reference just use CTRL + left mouse click on the link. The functions with these
// links also have hyperlinks back to the print_flow reference below.

//////////////////// Order of called methods in the wxWidgets printing framework ///////////////////
// whm Note: The order of function calls when printing or print-previewing in the wxWidgets printing 
// framework is:
// #print_flow
// 0. code:CAdapt_ItView::OnPrint() 
//    or code:CAdapt_ItView::OnPrintPreview() event handler [invoked by File | Print or File | Print Preview command] 
// 1. code:AIPrintout::AIPrintout() constructor [executed once for File | Print; twice for File | Print Preview]
// 2. The wxPrinter printer object calls printer.Print() see code:#printer_Print() [puts up platform specific print dialog]
// 3. code:AIPrintout::OnPreparePrinting()
// 4. code:AIPrintout::GetPageInfo()
// 5. code:AIPrintout::OnBeginPrinting()
//    6. code:AIPrintout::OnBeginDocument()
//       7. code:AIPrintout::HasPage()
//       8. code:AIPrintout::OnPrintPage()
//    9. AIPrintout::OnEndDocument() [not currently overridden in either MFC or WX]
// 10. code:AIPrintout::OnEndPrinting()
// In Step 2 if printer.Print() returns FALSE, printing aborts and no other steps are completed.
// Steps 6-9 are done for each copy of a document when multiple copies are requested
// Steps 7-8 are done for each page to be printed in a given document
//////////////////// Order of called methods in the wxWidgets printing framework ///////////////////

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// Called to construct an AIPrintout object. This constructor freezes the canvas which is kept frozen
/// until the corresponding destructor is called. 
////////////////////////////////////////////////////////////////////////////////////////////
AIPrintout::AIPrintout(const wxChar *title):wxPrintout(title)
{
	// refactored 6Apr09
	CAdapt_ItApp* pApp = &wxGetApp();
	// See code:#print_flow for the order of calling of this AIPrintout constructor.
	
    // whm: to avoid problems with calls to the View's Draw() method we should freeze the
    // canvas here at the beginning of the print preview routine, and unfreeze it in the
    // AIPrintout destructor after printing ends. For non-preview printing it is not
    // necessary to freeze the canvas.
	pApp->GetMainFrame()->canvas->Freeze();

	// in the refactored design, not all strips may be fully filled, so since printing is
	// likely to print one or more incomplete strips, the best way to prevent that is to
	// do a fill recalc of the layout (but leave piles untouched) before printing, so that
	// all strips are properly filled
	pApp->m_nSaveActiveSequNum = pApp->m_nActiveSequNum;
/* 
    // this call of RecalcLayout is pointless, we need to first set up the printing page's
    // width and height, and the scaling, and then call RecalcLayout() - all of that is
    // done in OnPreparePinting()
	CLayout* pLayout = pApp->m_pLayout;
#ifdef _NEW_LAYOUT
	pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
	//pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
	//pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_and_piles);
#else
	pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
*/
	pApp->m_docSize = pApp->m_pLayout->GetLogicalDocSize(); // copy m_logicalDocSize value 
															// back to app's member
	pApp->m_saveDocSize = pApp->m_docSize; // store original size (can dispense with this
			// here if we wish, because OnPreparePrinting() will make same call)
	// the following, defined in the app class, is a kluge to prevent problems which would
	// happen due to the ~AIPrintout() destructor being called twice after a Print
	// Preview, so I'm counting times reentered, and only letting the function do any work
	// on the first time entered
	pApp->m_nAIPrintout_Destructor_ReentrancyCount = 1;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// Called when the AIPrintout object is about to be destroyed. The main clean up code for
/// restoring the document to its previous state it had before printing is executed here in
/// the destructor. The cleanup involves: Calling RestoreOriginalList() after printing a
/// selection or range, and RemoveSelection() when printing a selection or range, but not
/// previewing; resetting some global printing flags (including gbIsPrinting to FALSE);
/// calling the View's RestoreIndices(), ClearPagesList(), and RecalcLayout(); the canvas'
/// ScrollIntoView(), etc., and thaws out the canvas which was frozen in the corresponding
/// constructor.
/// BEW 25May09: For some reason known only to wxWidgets, the function is called twice
/// when closing a Print Preview; and setting the active sequence number to the saved
/// value on the app, and then setting the save app value to -1 is bad news unless we do
/// something about it, because the RecalcLayout() the second time round will muck up the
/// active location, since the active sequence number gets wrongly reset to -1 on the
/// second call! So, I'll try a global to count the number of times reentered, and for the
/// second or later reenters, just exist immediately without doing anything. Crude, but it
/// will have to do. A pity the cleanup is done in the destructor, but I've not got the
/// time to try figure out why Bill put it so late. 
////////////////////////////////////////////////////////////////////////////////////////////
AIPrintout::~AIPrintout()
{
	// refactored 6Apr09
	CAdapt_ItApp* pApp = &wxGetApp();

    // Note: The code below cleans up the indices and flags after a print and/or print
    // preview operation. It could not go in OnEndPrinting because that gets called earlier
    // and more often in the wx version (especially when doing print preview).
	CAdapt_ItView* pView = pApp->GetView();

	if (pApp->m_nAIPrintout_Destructor_ReentrancyCount == 1) 
	{
		// so the the stuff in this block only when we enter this function the first time

		// restore original doc size
		pApp->m_docSize = pApp->m_saveDocSize;
		// and put that value back into CLayout::m_logicalDocSize
		pApp->m_pLayout->RestoreLogicalDocSizeFromSavedSize();
		pApp->m_pLayout->m_pOffsets = NULL; // restore default NULL value for PageOffsets instance

		// if we were printing a selection, restore the original state first (but don't restore the
		// selection), then do tidy up of everything else & get a new layout calculated; likewise if
		// we were printing a chapter & verse range
		if (gbPrintingSelection  || gbPrintingRange)
		{
			pView->RestoreOriginalList(pApp->m_pSaveList, pApp->m_pSourcePhrases); // ignore return value,
															// either we aborted, or all was well
			// we want any selection retained if we have been doing a print preview, but we want the
			// selection removed if we have been printing
			if (!gbIsBeingPreviewed && gbPrintingSelection)
			{
				pView->RemoveSelection();
			}

			gbPrintingSelection = FALSE;
			gbPrintingRange = FALSE;

			// restore defaults for the checkboxes
			gbSuppressPrecedingHeadingInRange = FALSE;
			gbIncludeFollowingHeadingInRange = FALSE;
		}

		// clean up
		//pView->RestoreIndices();
		pView->ClearPagesList();
		gbIsPrinting = FALSE;
		// wx version: I think the All Pages button gets enabled

		// layout again for the screen, get an updated pointer to the active location, restore
		// the phrase box, scroll, invalidate window to restore pre-printing appearance (if we
		// had been printing a selection, it will get restored now because the globals will
		// have been preserved)
		pApp->m_nActiveSequNum = pApp->m_nSaveActiveSequNum;
		// get a temporary m_pActivePile pointer
		pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
#ifdef _NEW_LAYOUT
		//pApp->m_pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles); 
														// this one gives a crash
		pApp->m_pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles); // no
														// crash
		//pApp->m_pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_and_piles); //
														//too slow!! and doesn't fix problems
#else
		pApp->m_pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
	#endif

		pApp->m_nSaveActiveSequNum = -1; // **** NOTE ***** this can only be safely done once,
            // after the above RecalcLayout() call, and if the function is reentered and we
            // don't cause it to do nothing by the test put at the top, then RecalcLayout()
            // gets called with an active sequence number of -1, which messes up the phrase
            // box placement etc, hence the kluge of using
            // m_nAIPrintout_Destructor_ReentrancyCount to prevent the repeat of the
            // calculations in this block
	} // block ends here so that a call to ScrollIntoView() is done for each time entered,
	// otherwise, the scroll position gets lost for the second entrance, and a manual
	// scroll is then required to make the active strip visible; but having the call done
	// each time solves this problem

	// recalculate the active pile & update location for phraseBox creation
	pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
	if (pApp->m_pActivePile != NULL) // whm added 27Feb05 to avoid crash when m_nActiveSequNum == -1
								// as can be the case when user finishes adapting a doc, dismisses
								// the dialog that informs of such, then immediately does print
								// preview. When closing print preview this routine would otherwise
								// crash below because GetPile makes m_pActivePile NULL.
	{
		pApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);
		pApp->m_nStartChar = 0;
		pApp->m_nEndChar = -1; // ensure initially all is selected
		pApp->m_pTargetBox->SetSelection(-1,-1); // select all
		pApp->m_pTargetBox->SetFocus();
	}
   	
	pView->Invalidate();
	pApp->m_pLayout->PlaceBox();
	wxWindow* pWnd;
	pWnd = wxWindow::FindFocus(); // the box is not visible when the focus is set by the above code,
							 // so unfortunately the cursor will have to be manually put back in the
							 // box
	
	// Code to thaw the canvas needs to go here in OnCloseWindow, because OnEndPrinting gets called
	// prematurely by the framework as each preview page is about to be shown.
	// (Also, now that BEW has added the m_nAIPrintout_Destructor_ReentrancyCount kluge,
	// we must permit a balanced number of calls to Thaw(), although we permit the stuff
	// in the block above to be done only once)
	if (pApp->GetMainFrame()->canvas->IsFrozen())
		pApp->GetMainFrame()->canvas->Thaw();

	pApp->m_nAIPrintout_Destructor_ReentrancyCount++; // count the number of times this function is
						// entered, we do nothing in this block if it is is entered more than once
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if the wxDC associated with printing is valid, otherwise FALSE
/// \param      page      -> the current page being rendered by this OnPrintPage()
/// \remarks
/// Called from: Called by the framework to draw the current page while printing - it is called 
/// once for each page that needs to be printed. Returning false cancels the print job.
/// OnPrintPage() is called after OnBeginDocument() and once for each time HasPage() 
/// returns true for a given document. The application can use wxPrintout::GetDC to obtain 
/// a device context to validate and draw on.
////////////////////////////////////////////////////////////////////////////////////////////
bool AIPrintout::OnPrintPage(int page)
{
	// refactored 6Apr09
	// See code:#print_flow for the order of calling of OnPrintPage().
	CAdapt_ItApp* pApp = &wxGetApp();
	CAdapt_ItView* pView = pApp->GetView();
	CLayout* pLayout = pApp->m_pLayout;
    wxDC *pDC = GetDC();
    if (pDC)
    {
		gnCurPage = page; // set the global for use by CStrip's Draw() function

		// The code block below properly scales the text to appear the correct size within both
		// the print preview and on the printer. 

		// Scale the display context to the correct size for either the preview or for printer.
		// OnPrintPage() is called for both print preview and for actual printing to printer.
		
		// Get the logical pixels per inch of screen and printer
		int ppiScreenX, ppiScreenY;
		GetPPIScreen(&ppiScreenX, &ppiScreenY);
		int ppiPrinterX, ppiPrinterY;
		GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
        // This scales the DC so that the printout roughly represents the the screen scaling. The
        // wxPrintout docs say, "The text point size _should_ be the right size but in fact is too small
        // for some reason. This is a detail that will need to be addressed at some point but can be
        // fudged for the moment."
		float scale = (float)((float)ppiPrinterX/(float)ppiScreenX);

		// Now we have to check in case our real page size is reduced (e.g. because
		// we're drawing to a print preview memory DC)
		int pageWidthInPixels, pageHeightInPixels;
		int w, h;
		pDC->GetSize(&w, &h);
		this->GetPageSizePixels(&pageWidthInPixels, &pageHeightInPixels);
        // If printer pageWidth == current DC width, then this doesn't change. But w might be the
        // preview bitmap width, so scale down.
		float overallScaleX = scale * (float)(w/(float)pageWidthInPixels);
		float overallScaleY = scale * (float)(h/(float)pageHeightInPixels);
		pDC->SetUserScale(overallScaleX, overallScaleY); 

		// Calculate the scaling factor for passing to PrintFooter. This factor must be multiplied by
		// any absolute displacement distance desired which is expressed in mm. For example, we want
		// the footer to be placed 12.7mm (half inch) below the bottom printing margin. In logical
		// units a 12.7mm displacement below the bottom printing margin is 12.7*logicalUnitsFactor
		// added to the y axis coordinate.
		float logicalUnitsFactor = (float)(ppiPrinterX/(scale*25.4));

		POList* pList = &pApp->m_pagesList;
		POList::Node* pos = pList->Item(gnCurPage-1);
		PageOffsets* pOffsets = (PageOffsets*)pos->GetData();

        // BEW added 10Jul09; inform CLayout of the PageOffsets instance which is current
        // for the page being printed - Draw() needs this information in order to work out
        // the strips to be drawn
		pLayout->m_pOffsets = pOffsets;

        // Note: Printing of headers and/or footers needs to be done before we call
        // pDC->SetLogicalOrigin(0,pOffsets->nTop) below because headers and footers are always placed
        // on the page in constant coordinates in relation to the top and bottom page margins (scaled
        // for the appropriate pDC display context).
		
		// For drawing of footer, whose position is in relation to the whole page, margins, etc., it is easiest
		// to set the AIPrintout logical origin at 0,0 and draw the footer with x and y coords in
		// relation to the upper left corner of the paper.
		wxRect fitRect = this->GetLogicalPageMarginsRect(*pApp->pPgSetupDlgData);
		this->SetLogicalOrigin(0,0);
		
		// Now draw the footer for the page (logical origin for the printout page is at 0,0)
		if (gbPrintFooter)
		{
			pView->PrintFooter(pDC,fitRect,logicalUnitsFactor,page);
		}
		
        // Set the upper left starting point of the drawn page to the point where the upper margin and
        // left margin intersect. We call SetLogicalOrigin() called on AIPrintout below to set the
		// initial drawing point for what's drawn on this page to the upper left corner - beginning at
		// the point where the printout's logical top and left page margins intersect.
		this->SetLogicalOrigin(fitRect.x, fitRect.y);
		//this->SetLogicalOrigin(0, fitRect.y); // test with Gilaki doc
		
		// SetLogicalOrigin is only documented as a method of wxPrintout, but it is also available for
		// wxDC. Since the "Strips" that will be drawn in OnDraw() below store their logical coordinates
		// based on their position in the whole virtual document, we need to set the logical origin so
		// that strips will start drawing from the top of our printout/preview page.
		pDC->SetLogicalOrigin(0,pOffsets->nTop); // MFC used pDC->SetWindowOrg(0,pOffsets->nTop);
		//pDC->SetLogicalOrigin(400,pOffsets->nTop); // test with Gilaki doc

		pView->OnDraw(pDC);
        
		return TRUE;
    }
    else
        return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// Called from: Called by the framework after OnPreparePrinting() and GetPageInfo() but before 
/// OnBeginDocument(). OnBeginPrinting() is only called once at the beginning of printing. 
/// It is not called when the user selects a different page from print preview.
////////////////////////////////////////////////////////////////////////////////////////////
void AIPrintout::OnBeginPrinting()
{
	// refactored 6Apr09 -- do nothing except ensure the gbIsPrinting flag is TRUE
	gbIsPrinting = TRUE;

	// set the mapping mode
	// The highest resolution choices are wxMM_TWIPS and wxMM_LOMETRIC.
	// With wxMM_TWIPS each logical unit is 1/20th point or 1/1440th of an inch.
	// With wxMM_LOMETRIC each logical unit is 1/10th of a mm or roughly 1/250th of an inch.
	// With both wxMM_TWIPS and wxMM_LOMETRIC positive x is to the right, positive y is up.
	// See code:#print_flow for the order of calling of OnBeginPrinting().
	// whm Note: It turns out that we maintain the wxMM_TEXT map mode even when printing in the wx
	// version, therefore the SetMapMode call below is commented out.
	//GetDC()->SetMapMode(wxMM_LOMETRIC); // MFC uses MM_LOENGLISH in which each logical unit is 1/100th of an inch
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// Called from: CAUTION: OnEndPrinting() works differently from the function of the same name 
/// in MFC!! Like MFC, OnEndPrinting() is called by the framework after a given document has 
/// printed. However, surprisingly, OnEndPrinting() is also called after each page is displayed 
/// in print preview mode. Also, note that when more than one copy of a document is printed, 
/// OnEndPrinting() is called at the end of each document copy, and before the OnEndPrinting() 
/// method of wxPrintout is executed. Therefore, the behavior of OnEndPrinting in the wx version 
/// makes it unsuitable as a location for the cleanup code that is done in MFC's OnEndPrinting(). 
/// The cleanup code has moved to AIPrintout's destructor in the wx version.
////////////////////////////////////////////////////////////////////////////////////////////
void AIPrintout::OnEndPrinting()
{
    // See code:#print_flow for the order of calling of OnEndPrinting().
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if the page will be printed; FALSE if the printing of pages for the current
///             document copy should stop (OnPrintPage will no longer be called for the current 
///             document), and print control will proceed to OnEndDocument().
/// \param      pageNum     -> 
/// \remarks
/// Called from: Called the first time after OnBeginDocument(). It is called just before each 
/// page is printed. By default, HasPage() behaves as if the document has only one page. 
////////////////////////////////////////////////////////////////////////////////////////////
bool AIPrintout::HasPage(int pageNum)
{
 	// refactored 6Apr09
   // See code:#print_flow for the order of calling of HasPage().
	CAdapt_ItApp* pApp = &wxGetApp();
	wxPrintDialogData printDialogData(*pApp->pPrintData); 
	
	if (pageNum >= printDialogData.GetMinPage() && pageNum <= printDialogData.GetMaxPage())
		return TRUE;
	else
		return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     FALSE if the user has cancelled printing
/// \param      startPage     -> the minimum page value that the user can select for printing or previewing
/// \param      endPage       -> the maximum page value that the user can select for printing or previewing
/// \remarks
/// Called from: the printing framework after OnBeginPrinting() and once just before each copy of a 
/// multi-copy printout (it is only called once if only 1 copy of a document is printed.
////////////////////////////////////////////////////////////////////////////////////////////
bool AIPrintout::OnBeginDocument(int startPage, int endPage)
{
	// refactored 6Apr09
	// See code:#print_flow for the order of calling of OnBeginDocument().
	
    if (!wxPrintout::OnBeginDocument(startPage, endPage))
        return false;

	// this initialization of gbIsPrinting need to be done because OnEndPrinting is called after each
	// page is rendered in print preview and gbIsPrinting is set FALSE there.
	gbIsPrinting = TRUE;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      minPage       <- the minimum page value that the user can select for printing or previewing
/// \param      maxPage       <- the maximum page value that the user can select for printing or previewing
/// \param      selPageFrom   <- the starting page of any selection of pages made by the user
/// \param      selPageTo     <- the ending page of any selection of pages made by the user
/// \remarks
/// Called from: The printing framework; OnPreparePrinting() is called immediately before this 
/// GetPageInfo(). In the preceding call to OnPreparePrinting(), PaginateDoc() is called which 
/// populates the App's m_pagesList which this GetPageInfo accesses to determine its four parameter
/// values which are returned to the caller. GetPageInfo() is only called once at the beginning of 
/// the print or preview; it is not called when the user selects a different page from within print 
/// preview. Note: if minPage is zero, the page number controls in the print dialog will be disabled.
////////////////////////////////////////////////////////////////////////////////////////////
void AIPrintout::GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo)
{
 	// refactored 6Apr09
	// See code:#print_flow for the order of calling of GetPageInfo().
 	CAdapt_ItApp* pApp = &wxGetApp();
 
	wxPrintDialogData printDialogData(*pApp->pPrintData); 
	*minPage = 1;
	*maxPage = pApp->m_pagesList.GetCount();
	if (printDialogData.GetAllPages())
	{
		*selPageFrom = 1;
		*selPageTo = pApp->m_pagesList.GetCount();
	}
	else
	{
		*selPageFrom = printDialogData.GetMinPage();
		*selPageTo = printDialogData.GetMaxPage();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return         nothing
/// \remarks
/// Called from: OnPreparePrinting() is called automatically by the framework when an
/// OnPrint() event is handled (via wxID_PRINT standard Identifier). According to the docs,
/// "It is called once by the framework before any other demands are made of the wxPrintout
/// object. This gives the object an opportunity to calculate the number of pages in the
/// document, and any other print preparations that are necessary before actual printing or
/// print preview is done." OnPreparePrinting() is always called once just before a print
/// preview dialog is displayed, and it is not called again when a different page is
/// selected in print preview mode. When File | Print is selected OnPreparePrinting() is
/// not called until after the the OK button is pressed at the print dialog, so, if user
/// cancels the print dialog OnPreparePrinting() is never called in that case. Unlike the
/// MFC version, which actually calls up the print dialog from within its version of
/// OnPreparePrinting (by calling the MFC DoPreparePrinting(pInfo) function), the wx
/// version works differently. The print dialog is called up in its high level OnPrint()
/// handler at the point where the wxPrinter object's Print() method is called. There is
/// therefore no need for cleanup code to fix indices, call RecalcLayout, etc, here in
/// OnPreparePrinting. All that is done in the AIPrintout's destructor, rather than here in
/// OnPreparePrinting(). See notes within OnPreparePrinting() for more detail.
////////////////////////////////////////////////////////////////////////////////////////////
void AIPrintout::OnPreparePrinting() 
{
	// refactored 6Apr09
 	CAdapt_ItApp* pApp = &wxGetApp();
	pApp->m_docSize = pApp->m_pLayout->GetLogicalDocSize(); // copy m_logicalDocSize value 
															// back to app's member
 	pApp->m_saveDocSize = pApp->m_docSize; // also done in creator of AIPrintout class

  // whm notes:
    // 
    // 1. OnPreparePrinting() is called automatically by the framework when an OnPrint() event is
    // handled (via wxID_PRINT standard Identifier). According to the docs, "It is called once by the
    // framework before any other demands are made of the wxPrintout object. This gives the object an
    // opportunity to calculate the number of pages in the document, and any other print preparations
    // that are necessary before actual printing or print preview is done." OnPreparePrinting() is
    // always called once just before a print preview dialog is displayed, and it is not called again
    // when a different page is selected in print preview mode. When File | Print is selected
    // OnPreparePrinting() is not called until after the the OK button is pressed at the print dialog,
    // so, if user cancels the print dialog OnPreparePrinting() is never called in that case.
    // 
    // 2. MFC uses a CPrintInfo* pInfo parameter, but the wx version creates a wxPrintData object
    // (pPrintData) in OnInit() of the App. pPrintData is also used as a data member of
    // wxPrintDialogData and wxPageSetupDialogData, as part of the mechanism for transferring data
    // between the print dialogs and the application.
    // 
    // 3. For the wx version, as the wxWidgets' printing sample illustrates, it is not necessary to
    // explicitly change the display context's mapping mode for printing and print previewing, but that
    // printing can be done readily just using the default wxMM_TEXT mapping mode. Also it is evident
    // that most of the manipulation that MFC does of the printing routines that are conditional on the
    // gbIsPrinting global (especially in the handling of negative y axis offsets) are not necessary.
    // 
    // 4. Note: The page setup routines in wx mostly return values in millimeters, so I've revised the
    // App to maintain page size and margin data in metric (millimeters) as well as MFC's thousandths of
    // an inch. The internal routines here in OnPreparePrinting() handle the data in mm.
    // 
    // 5. The MFC version actually calls up the print dialog from within its version of
    // OnPreparePrinting (by calling the MFC DoPreparePrinting(pInfo) function). The wx version works
    // differently. The print dialog is called up in its high level OnPrint() handler at the point where
    // the wxPrinter object's Print() method is called. There is therefore no need for cleanup code to
    // fix indices, call RecalcLayout, etc. All that is done in the AIPrintout's destructor.
    // 
    // See code:#print_flow for the order of calling of OnPreparePrinting().
    
	CAdapt_ItView* pView = pApp->GetView();

    // whm Note: Most of the approximately 50 blocks of code where gbIsPrinting is used in the MFC
    // version (in Adapt_ItCanvas.cpp, Adapt_ItView.cpp, AIPrintout.cpp, Cell.cpp, Pile.cpp and
    // Strip.cpp), are removed in the wx version. The wx version does not use any negative offsets in
    // printing. We can't remove gbIsPrinting altogether because of the way that it is used in the
    // Strip's Draw() function.
    // 
    // whm update: It is not sufficient to set gbIsPrinting to TRUE only here, because OnEndPrinting()
    // sets it to FALSE after each page is drawn in print preview. It is also set to TRUE in
    // OnBeginDocument().
	gbIsPrinting = TRUE;

	// The MFC version deletes the CPrintDialog object created in the CPrintInfo constructor,
	// and substitutes a customized print dialog. The wx version is not using a customized print dialog
	// at present, but we may provide one eventually if it becomes clear how to tie in to the standard
	// print dialog for each individual platform.

	// whm: get a pointer to the wxPrintDialogData object (from printing sample)
	wxPrintDialogData printDialogData(*pApp->pPrintData); 
	
	// set the page range to defaults
	printDialogData.SetMinPage(1); // must start at 1
	printDialogData.SetMaxPage(0xffff); // don't know how many yet

	// clear any old view settings for an earlier print, in case they were not cleared
	pView->ClearPagesList();
	/* removed 6Apr09
	// in case the user chooses a chapter/verse range, save the current indices
	//pView->SaveIndicesForRange();	// this doesn't reset m_endIndex, etc, just sets m_saveRange... 
									// indices values; do this first because next line resets m_maxIndex
	//pView->SaveAndSetIndices(pApp->m_maxIndex);	// this sets m_maxIndex as m_endIndex, since we layout 
													// whole doc for printing
	*/
	if (gbPrintingRange)
	{
		// set up the range, layout, etc. (It is SetupRangePrintOp()'s responsibility to set up
		// the new index values, since the old settings have already been saved)
		bool bSetupOK = pView->SetupRangePrintOp(gnFromChapter,gnFromVerse,gnToChapter,gnToVerse,pApp->pPrintData,
            			gbSuppressPrecedingHeadingInRange, gbIncludeFollowingHeadingInRange);

		if(!bSetupOK)
		{
			// IDS_RANGE_SETUP_FAILED
			wxMessageBox(_("Sorry, setup of the chapter and verse range failed. This printing operation has been aborted. Perhaps the document does not contain chapter or verse numbers within the range you specified."), _T(""), wxICON_STOP);
			return; // abort the print operation
		}
	}

	bool bDefaultPgSetupInfoAvailable;
	bDefaultPgSetupInfoAvailable = pApp->pPgSetupDlgData->GetDefaultInfo();
	if (!bDefaultPgSetupInfoAvailable)
	{
        // The default page setup information is not available, so set the default page setup dimensions
        // for A4 paper. The default page setup info generally won't be available unless the user has
        // explicitly called up the page setup dialog.
		pApp->pPgSetupDlgData->SetPaperSize(wxSize(210,297)); // sets A4 paper size in mm (210mm x 297mm)
		pApp->pPgSetupDlgData->SetMarginTopLeft(wxPoint(25,25)); // sets top left margin in mm (approx 1 inch)
		pApp->pPgSetupDlgData->SetMarginBottomRight(wxPoint(25,25)); // sets bottom right margin in mm (approx 1 inch)
	}
	
	// whm: Caution: The following commented out call to SetDefaultInfo(TRUE) would cause the page
	// setup handler to be inable to call up the page setup dialog after a print preview.
	//pApp->pPgSetupDlgData->SetDefaultInfo(TRUE);	// TRUE means we return default printer information without 
	//												// showing a dialog (Windows only)

    // whm Notes:
    // 
    // 1. If the user has not accessed the page setup dialog pPgSetupDlgData's GetPaperSize() and
    // actually selected a paper size explicitly, this GetPaperSize() method will return a (0,0) size,
    // Check for a (0,0) sized paper.
    // 
    // 2. It should not be necessary to store the m_paperSizeCode value here from within the print or
    // print preview routines. If the user changes the paper size selection (which can only be done from
    // the Page Setup Dialog), the App's storage variables should only be changed from there not here.
    // 
    // 3. For compatibility with MFC version we could store the paper size in config file as an int
    // representative of MFC's DEVMODE enum values, however the paper size codes in MFC and wx don't
    // match because they use different enum structures. To get better compatibility between the config
    // values stored in MFC and wx, we map the wx paper size code to MFC's code using our
    // MapWXtoMFCPaperSizeCode function in OnFilePageSetup() and store in pApp->m_paperSizeCode.
    //
    // 4. The App's m_pageWidth and m_pageLength are stored in thousandths of an inch, i.e., 8269 x
    // 11692 for A4, 8500 x 11000 for Letter. Neither the MFC nor the wx version use m_pageWidth and
    // m_pageLength to draw text on the screen apart from print operations. Moreover, the wx version
    // doesn't need to use these values in its print and print preview routines. Therefore, I don't see
    // any reason to change these values held on the App during printing. They really only store the
    // page width and length for eventual writing to the config files 

    // MFC Note: Because PrintSetup, if user changes from Portrait to Landscape, does not result in the
    // width and height being swapped, we have to test for the change in orientation and fix it up -
    // this mismatch will obtain if m_bIsPortraitOrientation is still TRUE when GetPageOrientation()
    // returns 2 (== Landscape) - the next two if's are needed only if user changes orientation in the
    // Print Setup dialog; if he does it in the Page Setup dialog, my other code gets everything fixed
    // right, but Print Setup doesn't inform the view of the interchange of x and y axes in MFC, so I
    // must do it here.
	int nOrientation = pApp->GetPageOrientation();
	if (nOrientation == 1)
	{
		pApp->m_bIsPortraitOrientation = TRUE;
	}
	else
	{
		pApp->m_bIsPortraitOrientation = FALSE;
	}

	// the above width & length are portrait settings, so check if we have landscape currently set, 
	// if so, switch values to agree
	if (!pApp->m_bIsPortraitOrientation)
	{
        // Swap the width and length values for landscape in the pPgSetupDlgData object.
        // 
        // Note: the pPrintData->SetOrientation(wxLANDSCAPE) method will already have been called (via
        // SetPageOrientation() function) for landscape mode. That call switches the printer into that
        // mode when printing, so we only need to manually reverse the dimensions of the paper size
        // (here), and set the logical unit page printing width nPagePrintingWidthLU. The
        // nPagePrintingWidthLU is assigned to m_docSize (used in RecalcLayout) and passed to our
        // PaginateDoc() function.
		int width = pApp->m_pageWidthMM;
		int length = pApp->m_pageLengthMM;
		pApp->pPgSetupDlgData->SetPaperSize(wxSize(length,width)); // reverse parameters for landscape
	}

    // At this point we should have a valid (non-zero) paper size stored in our pPgSetupDlgData object.
    // It should have x and y values which reflect the page orientation m_bIsPortraitOrientation. The
    // paper size is in mm (default is 210 mm by 297 mm for A4). If we have landscape mode, the
    // paperSize will be 297 mm by 210 mm.
	wxSize paperSize_mm;
	paperSize_mm = pApp->pPgSetupDlgData->GetPaperSize();
	wxASSERT(paperSize_mm.x != 0);
	wxASSERT(paperSize_mm.y != 0);
	
    // We should also have valid (non-zero) margins stored in our pPgSetupDlgData object.
	wxPoint topLeft_mm, bottomRight_mm; // MFC uses CRect for margins, wx uses wxPoint
	topLeft_mm = pApp->pPgSetupDlgData->GetMarginTopLeft(); // returns top (y) and left (x) margins as wxPoint in milimeters
	bottomRight_mm = pApp->pPgSetupDlgData->GetMarginBottomRight(); // returns bottom (y) and right (x) margins as wxPoint in milimeters
	wxASSERT(topLeft_mm.x != 0);
	wxASSERT(topLeft_mm.y != 0);
	wxASSERT(bottomRight_mm.x != 0);
	wxASSERT(bottomRight_mm.y != 0);
	
	// whm Note: Up to this point the code in OnPreparePrinting() has focused on insuring that we have
	// good (or default) page and margin sizes established before proceeding with printing and/or print
	// previewing. Now we focus on getting the actual dimensions of the printing area and determining
	// the scaling factors we'll use to properly draw into our differing display contexts (screen
	// resolutions are generally different from printer resolutions).
	
    // whm Note: The MFC application assumed the screen resolution would always be 96dpi, or each dot
    // being approximately 1/100th of an inch; hence, it divided the calculated printing area (whose
    // value was in 1000ths of an inch) by 10 to get the assumed printing width (nPrintingWidth) to pass
    // to RecalcLayout (via the m_docSize global) and to PaginateDoc (directly as parameter). I've
    // written the code below for the wx version so that no screen resolution is assumed.
	
	// The code below was patterned after the wxWidgets printing sample.
	// 
	// whm Note: For a good explanation of GDI Mapping Modes, SetWindowOrg and SetViewportOrg see:
	// http://wvware.sourceforge.net/caolan/mapmode.html
	// See also: http://functionx.com/visualc/gdi/gdicoord.htm for a decent graphical illustration
	// of SetViewportOrg() and SetMappingMode effects for the various mapping modes.
	//
    // whm: Our target display context for pagination purposes is the printed page. We want to draw
    // strips within the printed page's margins - from the top margin to the bottom margin - paginating
    // the document into full pages plus any final partially filled page.
    // 
	// See CAdapt_ItView::SetupRangePrintOp() and CPrintOptionsDlg::InitDialog() for coding similar to 
	// that below which calculates the page dimensions in logical units. 
	// TODO: It might be good to write a separate function to do the job for all three places. The 
	// main difference here is that we can use the wxPrintout::GetPPIScreen() and 
	// wxPrintout::GetPPIPrinter() convenience methods, but the method used in SetupRangePrintOp() 
	// and CPrintOptionsDlg's InitDialog() would also work here.
    // 
    // When printing, Adapt It's RecalcLayout generates strips whose width is based on the margin
    // dimensions of a printed page (printer device context), rather than RecalcLayout's usual (when not
    // printing) docSize.x based on the dimensions of the main window's client area in device units or
    // pixels. Since this OnPreparePrinting function is also called for a print preview display context,
    // if we were to call the GetLogicalPageMarginsRect function during a print preview, it won't give
    // us the length data of printed page display context we need for calling PaginateDoc. Instead we'll
    // first calculate the length between top and bottom margins of a printed page in mm, then convert
    // the result to appropriate device units for use by PaginateDoc.
	int pageWidthBetweenMarginsMM, pageHeightBetweenMarginsMM;
    // The size data returned by GetPageSizeMM is not the actual paper size edge to edge, nor the size
    // within the margins, but it is the usual printable area of a paper, i.e., the limit of where most
    // printers are physically able to print; it is the area in between the actual paper size and the
	// usual margins. We therefore start with the raw paperSize and determine the intended print area
	// between the margins.
	pageWidthBetweenMarginsMM = paperSize_mm.x - topLeft_mm.x - bottomRight_mm.x;
	pageHeightBetweenMarginsMM = paperSize_mm.y - topLeft_mm.y - bottomRight_mm.y;
	// Now, convert the pageHeightBetweenMargins to logical units for use in calling PaginateDoc.
	// Get the logical pixels per inch of screen and printer.
	int ppiScreenX, ppiScreenY;
	GetPPIScreen(&ppiScreenX, &ppiScreenY); // usually 96 dpi but may differ on newer/future computers
	int ppiPrinterX, ppiPrinterY;
	GetPPIPrinter(&ppiPrinterX, &ppiPrinterY); // dependent on the printer
    // Calculate the scale for the DC so that the printout represents the screen scaling. The
    // printing sample has the following comment: "The text point size _should_ be the right size but in
    // fact is too small for some reason. This is a detail that will need to be addressed at some point
    // but can be fudged for the moment."
	float scale = (float)((float)ppiPrinterX/(float)ppiScreenX);
    // Calculate the conversion factor for converting millimetres into logical units. There are approx.
    // 25.4 mm to the inch. There are ppi device units to the inch. Therefore 1 mm corresponds to
    // ppi/25.4 device units. We also divide by the screen-to-printer scaling factor, because we need to
    // unscale to pass logical units to PaginateDoc.
	float logicalUnitsFactor = (float)(ppiPrinterX/(scale*25.4)); // use the more precise conversion factor
	int nPagePrintingWidthLU, nPagePrintingLengthLU;
	nPagePrintingWidthLU = (int)(pageWidthBetweenMarginsMM * logicalUnitsFactor);
	nPagePrintingLengthLU = (int)(pageHeightBetweenMarginsMM * logicalUnitsFactor);

	// Set the document size (the width explicitly).
	// whm: m_docSize.x is normally the width of the main window client area in pixels. For printing we
	// want this to be the size of the printable area within the page's margins. 
	pApp->m_docSize.x = nPagePrintingWidthLU; 
    // Note: the document's length m_docSize.y is determined after call to PaginateDoc() farther below,
    // so just set the .y value to zero here - then call CopyLogicalDocSizeFromApp() to inform
	// the CLayout::m_logicalDocSize member (used in RecalcLayout() of the different strip width
	// to be used for populating the strips with piles for printing
	pApp->m_docSize.y = 0;
    pApp->m_pLayout->CopyLogicalDocSizeFromApp();
   
	// check for a selection, if it exists, assume user wants to print it & setup accordingly
	if (pApp->m_selectionLine == -1)
	{
		// no selection, so assume whole document
		gbPrintingSelection = FALSE;

		// force the selection button to be disabled, set the page range to defaults
		printDialogData.SetSelection(FALSE); // must start at 1
		
        // Recalc the layout with the new width.
        // 
        // whm note: RecalcLayout uses m_docSize.x as its width in calculating the number of strips for
        // the current document. When layout out for the screen is intended ReclacLayout sets
        // m_docSize.x equal to the client size of the main window - RHSlop. But, in printing, the
        // client window size is ignored and the value for m_docSize.x is assigned (above) based on the
        // resolution width of the display context, i.e., the number of dots printed per paper width
        // when printing, or the number of pixels per width of simulated paper in the print preview
        // (the number of pixels per width will vary depending on the scale set in preview).
		//pView->RecalcLayout(gpApp->m_pSourcePhrases,0,gpApp->m_pBundle);
		//gpApp->m_pActivePile = pView->GetPile(gpApp->m_nActiveSequNum);
#ifdef _NEW_LAYOUT
		//pApp->m_pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
		pApp->m_pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#else
		pApp->m_pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
		pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
	}
	else
	{
		// a selection is current, so set up for it
		printDialogData.SetSelection(TRUE);

		// MFC version: we want the pages radio button to be enabled, so fix Flags for that
		// whm: The wx version appears to have no way to change the "Pages" radio button; it is not
		// generally disabled. I think that just doing SetSelection(TRUE) above is sufficient.

		// Be able to recover the fact that we are printing a selection.
		gbPrintingSelection = TRUE;

		// Store the selection parameters, so we can later restore it (in case we are here because
		// of a Print Preview command - we must allow user to go from there to a print directly).
		//pView->StoreSelection(pApp->m_selectionLine); // BEW removed 25Jun09, because
		//the previous gbPrintingSelection = TRUE; line meant that no storage of the
		//selection was done in StoreSelection() here anyway, and moreover, there is no
		//matching RestoreSelection() except the one which was in RecalcLayout() which
		//likewise would not do anything because gbPrintingSelection is TRUE; and since
		//for other reasons the refactored layout needs not to store selections across a
		//RecalcLayout() call (it gives a crash as old pointers are left hanging) we
		//should remove it here too - and the functions themselves, they are used nowhere
		//else

		CCellList::Node* pos = pApp->m_selection.GetFirst();
		CCell* pCell = pos->GetData();
		//int nBeginSN = pCell->m_pPile->m_pSrcPhrase->m_nSequNumber;
		int nBeginSN = pCell->GetPile()->GetSrcPhrase()->m_nSequNumber;
		pos = pApp->m_selection.GetLast();
		pCell = pos->GetData();
		//int nEndSN = pCell->m_pPile->m_pSrcPhrase->m_nSequNumber;
		int nEndSN = pCell->GetPile()->GetSrcPhrase()->m_nSequNumber;

		bool bOK;
		bOK = pView->GetSublist(pApp->m_pSaveList, pApp->m_pSourcePhrases, nBeginSN, nEndSN);

        // At this point, the selection has been temporarily made the whole document, so we must clobber
        // the selection (note: RemoveSelection() does not affect gbPrintingSelection value) but leaves
        // the global parameters unchanged, so it can later be restored.
		pApp->m_selectionLine = -1;
		pApp->m_selection.Clear();
		pApp->m_pAnchor = NULL;

        // Recalc the layout with the new width (note, the gbPrintingSelection value being TRUE means
        // that the SaveSelection() and RestoreSelection() calls in RecalcLayout() do nothing).
		//pView->RecalcLayout(gpApp->m_pSourcePhrases,0,gpApp->m_pBundle);
#ifdef _NEW_LAYOUT
		//pApp->m_pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
		pApp->m_pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#else
		pApp->m_pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif

		// Set safe values for a non-active location (but leave m_targetPhrase unchanged).
		pApp->m_pActivePile = NULL;
		pApp->m_nActiveSequNum = -1;
        // whm: The MFC version destroyed the phrasebox at this point, but, in the wx version we don't
        // destroy the phrasebox - we could hide it if necessary but I'll leave it showing for now.
	}

	// do pagination
	// 
	// whm: In the following call to PaginateDoc, we use the current document m_nStripCount,
	// because the PaginateDoc() call here is done within OnPreparePrinting() which is called after the
	// print dialog has been dismissed with OK, and thus we are paginating the actual doc to print and
	// not merely simulating it for purposes of getting the pages edit box values for the print options
	// dialog.
	bool bOK;
	//bOK = pView->PaginateDoc(gpApp->m_pBundle->m_nStripCount,nPagePrintingLengthLU,NoSimulation);
																	// (doesn't call RecalcLayout())
	bOK = pView->PaginateDoc(pApp->m_pLayout->GetStripArray()->GetCount(), nPagePrintingLengthLU,
							NoSimulation); // (doesn't call RecalcLayout())
	if (!bOK)
	{
		// PaginateDoc will have notified the user of any problem, so just return here - we can't print
		// without paginating the doc. Cleanup of the doc's indices, etc, is done in the AIPrintout
		// destructor.
		return;
	}

	// pagination succeeded, so set the initial values for pInfo
	int nTotalPages;
	nTotalPages = pApp->m_pagesList.GetCount();
	printDialogData.SetMaxPage(nTotalPages);
	printDialogData.SetMinPage(1);
	printDialogData.SetFromPage(1);
	printDialogData.SetToPage(nTotalPages);

    // whm Note: The MFC version calls a DoPreparePrinting(pInfo) function at this point which puts up
    // the print dialog. The ordering of printing events in the two frameworks is different. In the wx
    // version, the print dialog is put up when the printer.Print() method is called in the higher level
    // OnPrint() handler in the view - this happens BEFORE OnPreparePrinting() is ever called. Hence, in
    // the wx version the OnPrint() handler (and the print dialog) exit before OnPreparePrinting()
    // starts executing. In the MFC version, however, DoPreparePrinting() is called first by MFC's
    // default OnFilePrint() handler (not overridden in MFC code); then, within DoPreparePrinting() the
    // print dialog gets called by DoPreparePrinting() towards the end of OnPreparePrinting. These order
    // differences mandate a radical change in the contents of the various printing functions to
    // implement similar functionality in the wxWidgets version.
	
    // The MFC version also has a code here (near the end of OnPreparePrinting and long after print
    // dialog is dismissed) to set up things for any range print called for from the print dialog. The
    // setup above sets things up for printing a selection. The setup that the MFC version does at this
    // point sets things up for a chapter:verse range to print. In the wx version that code is placed at
    // the beginning of the View's OnPrint() higher level handler.
    // 
    // Also, in the MFC version, if printing had to abort, the remainder of OnPreparePrinting there was
    // devoted to cleaning up indices, flags, doing RecalcLayout, etc - things which the wx version does
    // in the AIPrintout's class destructor.
} // end of OnPreparePrinting()


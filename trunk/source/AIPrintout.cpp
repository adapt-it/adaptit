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
	pApp->pAIPrintout = this;
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
/// will have to do. 18Jul09 BEW moved the code, including the reentracy protection
/// global, into a function called DoPrintCleanup() and put the latter in the app class. 
////////////////////////////////////////////////////////////////////////////////////////////
AIPrintout::~AIPrintout()
{
	CAdapt_ItApp* pApp = &wxGetApp();
	pApp->DoPrintCleanup();
	pApp->pAIPrintout = NULL;
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

#ifdef Print_failure
		int internalDC_Y = pDC->DeviceToLogicalY(0);
		wxLogDebug(_T("\n page = %d , DC offset: %d , pOffsets: nTop %d  nBottom %d , nFirstStrip %d  nLastStrip %d"),
			gnCurPage, internalDC_Y, pOffsets->nTop, pOffsets->nBottom, pOffsets->nFirstStrip, pOffsets->nLastStrip);
#endif

        // Note: Printing of headers and/or footers needs to be done before we call
        // pDC->SetLogicalOrigin(0,pOffsets->nTop) below because headers and footers are always placed
        // on the page in constant coordinates in relation to the top and bottom page margins (scaled
        // for the appropriate pDC display context).
		
		// For drawing of footer, whose position is in relation to the whole page, margins, etc., it is easiest
		// to set the AIPrintout logical origin at 0,0 and draw the footer with x and y coords in
		// relation to the upper left corner of the paper. 
		
		// BEW changed 15Jul09: wxPrintout is not documented, and the Printing Guidlines
		// are very sparse. It turns out that the printed page, from the top left point
		// where the margins intersect, functions like the client rectangle when printing
		// to the view window - that is, it is in "device" coordinates. That means that we
		// have to reset the AIPrintout DC's origin to (0,0) before we call
		// GetLogicalPageMarginsRect() because the latter needs to get the top left x and
		// y coordinates relative to a (0,0) "page" margin (the printing area, not the
		// physical page top left) -- for our default margins in Adapt It, that means
		// we'll get (x=72, y=78) for the x & y values in fitRect -- we have to do this
		// resetting of the origin to (0,0) adjustment for every page printed, (has no
		// effect when print previewing, the latter stays correct). Then, after printing
		// the footer using that (0,0) origin for the AIPrintout DC, we then have to make
		// the printing of strips start at the (fitRect.x,fitRect.y) as origin, so we do a
		// call to SetLogicalOrigin() with those coordinates. Then the printing comes out
		// right for each page. Finally!
		this->SetLogicalOrigin(0,0);
		wxRect fitRect = this->GetLogicalPageMarginsRect(*pApp->pPgSetupDlgData);

#ifdef Print_failure
		wxLogDebug(_T("fitRect = this->GetLogicalPageMarginsRect() x %d  y %d , width %d  height %d"),
			fitRect.x, fitRect.y, fitRect.width, fitRect.height);
#endif
/* some rects I investigated to see what they x, y, width and height values are, but we don't need them
		wxRect paperRectPixels = this->GetPaperRectPixels();
#ifdef Print_failure
		wxLogDebug(_T("paperRectPixels = this->GetPaperRectPixels() x %d  y %d , width %d  height %d"),
			paperRectPixels.x, paperRectPixels.y, paperRectPixels.width, paperRectPixels.height);
#endif

		wxRect paperRectLogical = this->GetLogicalPaperRect();
#ifdef Print_failure
		wxLogDebug(_T("paperRectLogical = this->GetLogicalPaperRect() x %d  y %d , width %d  height %d"),
			paperRectLogical.x, paperRectLogical.y, paperRectLogical.width, paperRectLogical.height);
#endif

		wxRect pageRectLogical = this->GetLogicalPageRect();
#ifdef Print_failure
		wxLogDebug(_T("pageRectLogical = this->GetLogicalPageRect() x %d  y %d , width %d  height %d"),
			pageRectLogical.x, pageRectLogical.y, pageRectLogical.width, pageRectLogical.height);
#endif
*/
		//this->SetLogicalOrigin(0,0); // BEW 15Jul09 moved this to precede the setting of fitRect
		
		// Now draw the footer for the page (logical origin for the printout page is at 0,0)
		if (gbPrintFooter)
		{
			pView->PrintFooter(pDC,fitRect,logicalUnitsFactor,page);
		}
		
        // Set the upper left starting point of the drawn page to the point where the upper
        // margin and left margin intersect. We call SetLogicalOrigin() called on
        // AIPrintout below to set the initial drawing point for what's drawn on this page
        // to the upper left corner - beginning at the point where the printout's logical
        // top and left page margins intersect.
        // This gets the paper "device" set up right, then below we use the top left of the
        // first strip to be printed for the setting of the origin for the pDC we pass to
        // CLayout for strip printing -- this makes the strip print at the right location
        // in the paper "device".
		this->SetLogicalOrigin(fitRect.x, fitRect.y);

#ifdef Print_failure
		wxLogDebug(_T("this->SetLogicalOrigin(), this = AIPrintout:wxPrintout x %d , y %d"),
			fitRect.x, fitRect.y );
#endif
		
        // SetLogicalOrigin is only documented as a method of wxPrintout, but it is also
        // available for wxDC. Since the "Strips" that will be drawn in OnDraw() below
        // store their logical coordinates based on their position in the whole virtual
        // document, we need to set the logical origin so that strips will start drawing
        // from the top of our printout/preview page.
		pDC->SetLogicalOrigin(0,pOffsets->nTop); // MFC used pDC->SetWindowOrg(0,pOffsets->nTop);

#ifdef Print_failure
		wxLogDebug(_T("pDC->SetLogicalOrigin(),                                x %d  y %d "),
			0, pOffsets->nTop);
#endif

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
 	CAdapt_ItApp* pApp = &wxGetApp();
	//CAdapt_ItView* pView = pApp->GetView();

	int nPagePrintingWidthLU = 0;
	int nPagePrintingLengthLU = 0;
	bool bAllsWell = pApp->CalcPrintableArea_LogicalUnits(nPagePrintingWidthLU, nPagePrintingLengthLU);
	if (!bAllsWell)
	{
		// document state is not changed yet
		return;
	}

	// ensure gbIsPrinting is turned on - it gets turned off when the Print Options Dlg is
	// shown so that RecalcLayout() calls will draw the view correctly (the user may move
	// the dialog relative to the visible part of the document in the view window), so we
	// must ensure it is back on when OnPreparePrinting()is next called, because the
	// LayoutAndPaginate() call below will need to redo the layout to the strip width as
	// required for the physical page, rather than the view
	gbIsPrinting = TRUE;

	// use the page printing dimensions to paginate, calling RecalcLayout(), and if there
	// is a selection, to make the selection temporarily become the whole document, and
	// then paginate using PaginateDoc(), a view function, so whatever is then in
	// m_pSourcePhrases is apportioned into a list of PageOffsets structs which specify
	// the beginning and end of what is to be printed on each page. This function is used
	// for both pagination of a print preview, and pagination of an actual real (paper)
	// printout, so that the number of printed pages is always the same number as shown in
	// the print preview. These structs are then used by OnPrintPage() for printing or
	// previewing. 
	bAllsWell = pApp->LayoutAndPaginate(nPagePrintingWidthLU,nPagePrintingLengthLU);
	if (!bAllsWell)
	{
		// just return, ~AIPrintout() will restore the document's original state
		return;
	}

	// get a pointer to the wxPrintDialogData object (from printing sample)
	wxPrintDialogData printDialogData(*pApp->pPrintData);

	// pagination succeeded, so set the initial values for pInfo
	int nTotalPages;
	nTotalPages = pApp->m_pagesList.GetCount();
	printDialogData.SetMaxPage(nTotalPages);
	printDialogData.SetMinPage(1);
	printDialogData.SetFromPage(1);
	printDialogData.SetToPage(nTotalPages);

    // whm Note: The MFC version calls a DoPreparePrinting(pInfo) function at this point
    // which puts up the print dialog. The ordering of printing events in the two
    // frameworks is different. In the wx version, the print dialog is put up when the
    // printer.Print() method is called in the higher level OnPrint() handler in the view -
    // this happens BEFORE OnPreparePrinting() is ever called. Hence, in the wx version the
    // OnPrint() handler (and the print dialog) exit before OnPreparePrinting() starts
    // executing. In the MFC version, however, DoPreparePrinting() is called first by MFC's
    // default OnFilePrint() handler (not overridden in MFC code); then, within
    // DoPreparePrinting() the print dialog gets called by DoPreparePrinting() towards the
    // end of OnPreparePrinting. These order differences mandate a radical change in the
    // contents of the various printing functions to implement similar functionality in the
    // wxWidgets version.
	
    // The MFC version also has a code here (near the end of OnPreparePrinting and long
    // after print dialog is dismissed) to set up things for any range print called for
    // from the print dialog. The setup above sets things up for printing a selection. The
    // setup that the MFC version does at this point sets things up for a chapter:verse
    // range to print. In the wx version that code is placed at the beginning of the View's
    // OnPrint() higher level handler.
    // 
    // Also, in the MFC version, if printing had to abort, the remainder of
    // OnPreparePrinting there was devoted to cleaning up indices, flags, doing
    // RecalcLayout, etc - things which the wx version does in the AIPrintout's class
    // destructor.
    //wxLogDebug(_T("OnPreparePrint() END"));
} // end of OnPreparePrinting()


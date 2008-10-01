/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			PrintOptionsDlg.cpp
/// \author			Bill Martin
/// \date_created	10 November 2006
/// \date_revised	1 March 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the CPrintOptionsDlg class. 
/// The CPrintOptionsDlg class provides a dialog for the user to enter a range
/// of pages, range of chapter/verse and specify how to handle margin elements 
/// such as section headings and footers at print time. This dialog pops up
/// before the standard print dialog when the user selects File | Print.
/// \derivation		The CPrintOptionsDlg class is derived from wxDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in PrintOptionsDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "PrintOptionsDlg.h"
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
//#include <wx/valtext.h> // for wxTextValidator
#ifdef __WXGTK__
#include <wx/dcps.h> // for wxPostScriptDC
#else
#include <wx/dcprint.h> // for wxPrinterDC
#endif

#include "Adapt_It.h"
#include "PrintOptionsDlg.h"
#include "Adapt_ItView.h" 
#include "Cell.h"
#include "Pile.h"
#include "MainFrm.h"
#include "Adapt_ItCanvas.h"

// next two are for version 2.0 which includes the option of a 3rd line for glossing

// This global is defined in Adapt_ItView.cpp.
//extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

// This global is defined in Adapt_ItView.cpp.
//extern bool	gbEnableGlossing; // TRUE makes Adapt It revert to Shoebox functionality only

extern bool gbPrintingSelection;
extern bool	gbPrintingRange;
extern int	gnFromChapter;
extern int	gnFromVerse;
extern int	gnToChapter;
extern int	gnToVerse;

/// This global is defined in Adapt_ItView.cpp.
extern bool gbPrintFooter;

extern bool gbSuppressPrecedingHeadingInRange;
extern bool gbIncludeFollowingHeadingInRange;
extern int gnPrintingWidth;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

// event handler table
BEGIN_EVENT_TABLE(CPrintOptionsDlg, AIModalDialog)
	EVT_INIT_DIALOG(CPrintOptionsDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_RADIOBUTTON(ID_RADIO_ALL, CPrintOptionsDlg::OnAllPagesBtn)
	EVT_RADIOBUTTON(ID_RADIO_SELECTION, CPrintOptionsDlg::OnSelectBtn)
	EVT_RADIOBUTTON(IDC_RADIO_PAGES, CPrintOptionsDlg::OnPagesBtn)
	EVT_RADIOBUTTON(IDC_RADIO_CHAPTER_VERSE_RANGE, CPrintOptionsDlg::OnRadioChapterVerseRange)
	EVT_SET_FOCUS(CPrintOptionsDlg::OnSetfocus)
	EVT_TEXT(IDC_EDIT_PAGES_FROM, CPrintOptionsDlg::OnEditPagesFrom)
	EVT_TEXT(IDC_EDIT_PAGES_TO, CPrintOptionsDlg::OnEditPagesTo)
	EVT_TEXT(IDC_EDIT1, CPrintOptionsDlg::OnEditChapterFrom)
	EVT_TEXT(IDC_EDIT2, CPrintOptionsDlg::OnEditVerseFrom)
	EVT_TEXT(IDC_EDIT3, CPrintOptionsDlg::OnEditChapterTo)
	EVT_TEXT(IDC_EDIT4, CPrintOptionsDlg::OnEditVerseTo)
	EVT_BUTTON(wxID_OK, CPrintOptionsDlg::OnOK)
	EVT_CHECKBOX(IDC_CHECK_SUPPRESS_FOOTER, CPrintOptionsDlg::OnCheckSuppressFooter)
	EVT_CHECKBOX(IDC_CHECK_SUPPRESS_PREC_HEADING, CPrintOptionsDlg::OnCheckSuppressPrecedingHeading)
	EVT_CHECKBOX(IDC_CHECK_INCLUDE_FOLL_HEADING, CPrintOptionsDlg::OnCheckIncludeFollowingHeading)
END_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      parent   -> 
/// \remarks
/// The CPrintOptionsDlg constructor. Called from the View's OnPrint() high level handler (activated on the
/// File | Print menu command). It uses the C++ resource code created by wxDesigner to create the dialog, 
/// sets some variables, and gets pointers to the dialog controls. 
////////////////////////////////////////////////////////////////////////////////////////////
CPrintOptionsDlg::CPrintOptionsDlg(wxWindow* parent)// ,wxPrintout* pPrintout) // dialog constructor
	: AIModalDialog(parent, -1, _("Choose your special print options"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	PrintOptionsDlgFunc(this, TRUE, TRUE);
	// The declaration is: PrintOptionsDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_pView = gpApp->GetView();

	//m_pPrintout = pPrintout; // initialize pointer to the AIPrintout of the caller (OnPrint in the View)

	m_bPrintingRange = FALSE;
	gbPrintingRange = FALSE;
	m_bSuppressFooter = FALSE;
	gbPrintFooter = TRUE;
	
	pEditChFrom = (wxTextCtrl*)FindWindowById(IDC_EDIT1);
	wxASSERT(pEditChFrom != NULL);
	
	pEditVsFrom = (wxTextCtrl*)FindWindowById(IDC_EDIT2);
	wxASSERT(pEditVsFrom != NULL);
	
	pEditChTo = (wxTextCtrl*)FindWindowById(IDC_EDIT3);
	wxASSERT(pEditChTo != NULL);
	
	pEditVsTo = (wxTextCtrl*)FindWindowById(IDC_EDIT4);
	wxASSERT(pEditVsTo != NULL);
	
	pEditPagesFrom = (wxTextCtrl*)FindWindowById(IDC_EDIT_PAGES_FROM);
	wxASSERT(pEditPagesFrom != NULL);
	
	pEditPagesTo = (wxTextCtrl*)FindWindowById(IDC_EDIT_PAGES_TO);
	wxASSERT(pEditPagesTo != NULL);
	
	pEditAsStatic = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_EDIT_AS_STATIC); // the read only information
	wxASSERT(pEditAsStatic != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	pEditAsStatic->SetBackgroundColour(backgrndColor);
	
	pRadioAll = (wxRadioButton*)FindWindowById(ID_RADIO_ALL);
	wxASSERT(pRadioAll != NULL);
	
	pRadioSelection = (wxRadioButton*)FindWindowById(ID_RADIO_SELECTION);
	wxASSERT(pRadioSelection != NULL);
	
	pRadioPages = (wxRadioButton*)FindWindowById(IDC_RADIO_PAGES);
	wxASSERT(pRadioPages != NULL);
	
	pRadioChVs = (wxRadioButton*)FindWindowById(IDC_RADIO_CHAPTER_VERSE_RANGE);
	wxASSERT(pRadioChVs != NULL);
	
	pCheckSuppressPrecSectHeading = (wxCheckBox*)FindWindowById(IDC_CHECK_SUPPRESS_PREC_HEADING);
	wxASSERT(pCheckSuppressPrecSectHeading != NULL);
	
	pCheckIncludeFollSectHeading = (wxCheckBox*)FindWindowById(IDC_CHECK_INCLUDE_FOLL_HEADING);
	wxASSERT(pCheckIncludeFollSectHeading != NULL);
	
	pCheckSuppressPrintingFooter = (wxCheckBox*)FindWindowById(IDC_CHECK_SUPPRESS_FOOTER);
	wxASSERT(pCheckSuppressPrintingFooter != NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// The CPrintOptionsDlg destructor.  
////////////////////////////////////////////////////////////////////////////////////////////
CPrintOptionsDlg::~CPrintOptionsDlg() // destructor
{
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called to initialize the CPrintOptionsDlg object. It sets the initial state of the dialog 
/// controls.
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

    // Get the logical page dimensions for paginating the document and printing.
    // 
    // whm Note: For initializing the CPrintOptionsDlg, we only simulate the calculation of the
    // document's layout without changing it, as would a direct call to RecalcLayout() would do. We
    // simulate it because we only want to paginate the document at this point to know what the possible
    // page range is for entering into the dialog's Pages from: and Pages to: edit boxes. To effect the
    // simulation we call RecalcLayout_SimulateOnly() instead of the usual RecalcLayout().
    // RecalcLayout_SimulateOnly() does not change the state of the document or any of its
    // indices/variables; it does the same calculations as RecalcLayout(), but only as far as necessary
    // to determine the layout parameters (number of Strips) necessary for our version of PaginateDoc()
    // to determine how many pages it would take to print the document.
    // 
	// ////////////////////////////////////////////////////////////////////////////////////////////
	// The stuff below could go into a separate function - see also CAdapt_ItView::SetupRangePrintOp
    // Determine the length of the printed page in logical units.
	int pageWidthBetweenMarginsMM, pageHeightBetweenMarginsMM;
	wxSize paperSize_mm;
	paperSize_mm = gpApp->pPgSetupDlgData->GetPaperSize();
	wxASSERT(paperSize_mm.x != 0);
	wxASSERT(paperSize_mm.y != 0);
     // We should also have valid (non-zero) margins stored in our pPgSetupDlgData object.
	wxPoint topLeft_mm, bottomRight_mm; // MFC uses CRect for margins, wx uses wxPoint
	topLeft_mm = gpApp->pPgSetupDlgData->GetMarginTopLeft(); // returns top (y) and left (x) margins as wxPoint in milimeters
	bottomRight_mm = gpApp->pPgSetupDlgData->GetMarginBottomRight(); // returns bottom (y) and right (x) margins as wxPoint in milimeters
	wxASSERT(topLeft_mm.x != 0);
	wxASSERT(topLeft_mm.y != 0);
	wxASSERT(bottomRight_mm.x != 0);
	wxASSERT(bottomRight_mm.y != 0);
	// The size data returned by GetPageSizeMM is not the actual paper size edge to edge, nor the size
    // within the margins, but it is the usual printable area of a paper, i.e., the limit of where most
    // printers are physically able to print; it is the area in between the actual paper size and the
	// usual margins. We therefore start with the raw paperSize and determine the intended print area
	// between the margins.
	pageWidthBetweenMarginsMM = paperSize_mm.x - topLeft_mm.x - bottomRight_mm.x;
	pageHeightBetweenMarginsMM = paperSize_mm.y - topLeft_mm.y - bottomRight_mm.y;
	
	// Now, convert the pageHeightBetweenMarginsMM to logical units for use in calling PaginateDoc.
	// 
	// Get the logical pixels per inch of screen and printer.
	// whm NOTE: We can't yet use the wxPrintout::GetPPIScreen() and GetPPIPrinter() methods because
	// the wxPrintout object is not created yet at the time this print options dialog is displayed.
	// But, we can do the same calculations by using the wxDC::GetPPI() method call on both a
	// wxPrinterDC and a wxClientDC of our canvas.
	//
    // Set up printer and screen DCs and determine the scaling factors between printer and screen.
    wxASSERT(gpApp->pPrintData->IsOk());

#ifdef __WXGTK__
	// Linux requires we use wxPostScriptDC rather than wxPrinterDC
	// Note: If the Print Preview display is drawn with text displaced up and off the display on wxGTK,
	// the wxWidgets libraries probably were not configured properly. They should have included a
	// --with-gnomeprint parameter in the configure call.
	wxPostScriptDC printerDC(*gpApp->pPrintData); // MUST use * here otherwise printerDC will not be initialized properly
#else
	wxPrinterDC printerDC(*gpApp->pPrintData); // MUST use * here otherwise printerDC will not be initialized properly
#endif
	
	wxASSERT(printerDC.IsOk());
	wxSize printerSizePPI = printerDC.GetPPI(); // gets the resolution of the printer in pixels per inch (dpi)
	wxClientDC canvasDC(gpApp->GetMainFrame()->canvas);
	wxSize canvasSizePPI = canvasDC.GetPPI(); // gets the resolution of the screen/canvas in pixels per inch (dpi)
	float scale = (float)printerSizePPI.GetHeight() / (float)canvasSizePPI.GetHeight();

    // Calculate the conversion factor for converting millimetres into logical units. There are approx.
    // 25.4 mm to the inch. There are ppi device units to the inch. Therefore 1 mm corresponds to
    // ppi/25.4 device units. We also divide by the screen-to-printer scaling factor, because we need to
    // unscale to pass logical units to PaginateDoc.
	float logicalUnitsFactor = (float)(printerSizePPI.GetHeight()/(scale*25.4)); // use the more precise conversion factor
	int nPagePrintingWidthLU, nPagePrintingLengthLU;
	nPagePrintingWidthLU = (int)(pageWidthBetweenMarginsMM * logicalUnitsFactor);
	nPagePrintingLengthLU = (int)(pageHeightBetweenMarginsMM * logicalUnitsFactor);
	// The stuff above could go into a separate function - see also CAdapt_ItView::SetupRangePrintOp
	// ////////////////////////////////////////////////////////////////////////////////////////////
	
    // whm Caution: It is not really appropriate to assume as MFC version does that, by converting from
    // thousandths to hundredths, the calculation will result in a measure equivalent to screen pixels.
    // While most displays today are 96 dpi where each pixel approximates to one hundredth of an inch,
    // the OLPC XO computer has a screen that is 200 dpi in its high reflective B&W mode.
	
    // Determine if we have a selection, and if so, only calculate for it rather than the whole
    // document. The following pSPList will point either to the whole list (gpApp->m_pSourcePhrases) if
    // there is no selection, or pSPList will point to a temporary list of selected source phrases
    // (pSelectedSPList) if there is a selection.
	SPList* pSPList; 
	SPList* pSelectedSPList = new SPList; // list of selected SPs not likely to be large but we'll created it on the heap anyway
	// The following will be sequence numbers for either the whole list, or the list of selected
	// source phrases if there is a selection.
	int nBeginSequNum;
	int nEndSequNum;
	if (gpApp->m_selectionLine == -1)
	{
		// there is no selection, so make pSPList pointer point to the whole list
		pSPList = gpApp->m_pSourcePhrases;
		nBeginSequNum = 0; // for no selection we always start at pSPList's sequence number 0 
		nEndSequNum = pSPList->GetCount() - 1; // for no selection we do the whole pSPList, nEndSequNum is last item //gpApp->m_endIndex;
	}
	else
	{
		// there is a selection, so we'll simulate the recalc based on just the selected list of source
		// phrases, but since we're simulating, we won't change the App's list of m_pSourcePhrases
		// 
		// We also won't call StoreSelection() because we won't need to restore it after the
		// simulation; but just get the sequence numbers to build a temporary SPList for the simulation.
		CCellList::Node* cpos = gpApp->m_selection.GetFirst();
		CCell* pCell = cpos->GetData();
		int nBeginSN,nEndSN;
		nBeginSN = pCell->m_pPile->m_pSrcPhrase->m_nSequNumber;
		cpos = gpApp->m_selection.GetLast();
		pCell = cpos->GetData();
		nEndSN = pCell->m_pPile->m_pSrcPhrase->m_nSequNumber;
		
        // We want to simulate the recalc (creation of strips) based on only the selected source
        // phrases, so rather than calling GetSublist() [which modifies the App's m_pSourcePhrases list]
        // we will create a temporary list here to hand over to RecalcLayout_SimulateOnly() rather than
        // giving it the App's m_pSourcePhrases list.
		// Copy across to pSelectedSPList the pointers to the source phrases which are wanted for the 
		// selection sublist.
		SPList::Node* spos = gpApp->m_pSourcePhrases->Item(nBeginSN);
		int sn = nBeginSN;
		while (sn <= nEndSN)
		{
			CSourcePhrase* pSrcPhrase = spos->GetData();
			spos = spos->GetNext();
			sn++;
			wxASSERT(pSrcPhrase != NULL);
			pSelectedSPList->Append(pSrcPhrase);
		}
		nBeginSequNum = 0;
		nEndSequNum = nEndSN - nBeginSN;
		wxASSERT(nEndSequNum > nBeginSequNum);
		//  make the pSPList pointer point to the list of selected source phrases
		pSPList = pSelectedSPList;
	}

	// Simulate the recalc of the doc to determine how many Strips we need to print.
	CAdapt_ItView* pView = gpApp->GetView();
	int nTotalStrips_Simulated;
	wxSize sizeTotal_Simulated(nPagePrintingWidthLU,nPagePrintingLengthLU);
	// RecalcLayout_SimulateOnly returns simulated docSize in its sizeTotal_Simulated reference parameter,
	// and the number of strips in its return value.
	nTotalStrips_Simulated = pView->RecalcLayout_SimulateOnly(pSPList, sizeTotal_Simulated,nBeginSequNum,nEndSequNum);
	
	// don't leak memory
	pSelectedSPList->Clear();
	delete pSelectedSPList;
	pSPList = (SPList*)NULL;

    // whm: Populate the Pages from: and to: edit boxes with the possible range (accounting for any
    // selection). This requires that we call PaginateDoc() to determine the number of pages for the
    // current page setup, selection, etc.
    // 
    // In the following call to PaginateDoc, we use the nTotalStrips_Simulated value returned from the
    // above RecalcLayout_SimulateOnly call, because the PaginateDoc() call here is done only for
    // purposes of getting the pages edit box values for the print options dialog and we haven't changed
    // any indices for the layout of the main document.
	bool bOK;
	bOK = pView->PaginateDoc(nTotalStrips_Simulated,nPagePrintingLengthLU,DoSimulation); // (doesn't call RecalcLayout())
	if (!bOK)
	{
		// PaginateDoc will have notified the user of any problem, so just return here - we can't print
		// without paginating the doc. Cleanup of the doc's indices, etc, is done in the AIPrintout
		// destructor.
		//return;
		// TODO: notify user that page range is not accurate
	}
	// Populate the Pages from: and Pages to: edit boxes
	int nNumPages = (int)gpApp->m_pagesList.GetCount();
	wxString nNumPagesStr;
	nNumPagesStr << nNumPages;
	pEditPagesFrom->SetValue(_T("1"));
	pEditPagesTo->SetValue(nNumPagesStr);
	
	// Populate the Chapter/Verse Range edit boxes with the maximum possible range - if any ch:vs
	// markers exist. We'll scan the doc's list of source phrases, inspecting their m_markers and m_chapterVerse
	// members; then we use AnalyseReference() to parse out what we find for populating the
	// Chapter/Verse Range edit boxes.
	// 
	wxString ChVsFirst = _T(""); // will contain the first instance of any pSrcPhrase->m_chapterVerse in the doc
	wxString ChVsLast = _T(""); // will contain the last instance of any pSrcPhrase->m_chapterVerse in the doc

	SPList* pList = gpApp->m_pSourcePhrases;
	wxASSERT(pList != NULL);
	CSourcePhrase* pSrcPhrase = NULL;

	// Scan the SrcPhrase structure to get certain information that we need upfront.
	// This should work whether we have unstructured data or not
	SPList::Node* pos = pList->GetFirst();
	wxASSERT(pos != NULL);
	bool bFirst = TRUE;
	bool bHasSectHdg = FALSE;
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase);
		if (!pSrcPhrase->m_markers.IsEmpty())
		{
			// Check for existence of section headings which may be \s, \s1, \s2, \s3, or \s4
			if (pSrcPhrase->m_markers.Find(_T("\\s")) != -1)
			{
				wxString strMkrs = pSrcPhrase->m_markers;
				int spos;
				spos = strMkrs.Find(_T("\\s"));
				wxASSERT(spos >= 0); // must be the case
				// other non-section heading markers also start with s, so we only consider those which
				// are section headings
				if (spos + 1 < (int)strMkrs.Length())
				{
					// there is a second char in the \s marker; check if it is space, '1', '2', '3', or '4'
					wxChar secondCh = strMkrs.GetChar(spos +1);
					if (secondCh == _T(' ') || secondCh == _T('1') || secondCh == _T('2') || secondCh == _T('3') || secondCh == _T('4'))
						bHasSectHdg = TRUE;
				}
			}
		}
		if (pSrcPhrase->m_chapterVerse != _T("") && (pSrcPhrase->m_bChapter || pSrcPhrase->m_bVerse))
		{
			if (bFirst)
			{
				ChVsFirst = pSrcPhrase->m_chapterVerse;		// get the first m_ChapterVerse
				bFirst = FALSE;
			}
			ChVsLast = pSrcPhrase->m_chapterVerse;			// get the last m_ChapterVerse
		}
	}

	if (!bHasSectHdg)
	{
		// the doc has no section headings, so disable the two checkboxes related to them
		pCheckSuppressPrecSectHeading->Enable(FALSE);
		pCheckIncludeFollSectHeading->Enable(FALSE);
	}

	if (ChVsFirst.IsEmpty())
	{
		// we have no chapter:verse markers in the doc
		wxASSERT(ChVsLast.IsEmpty());
	}
	else
	{
		// We have chapter:verse markers in the doc, so parse them to get our expected range; use the
		// services of the View's AnalyseReference().
		int nChFirst = 0;	// start chapter of range
		int nChLast = 0;	// ending chapter of range
		int nVsFirst = 0;	// start verse of range
		int nVsLast = 0;	// ending verse of range
		int vfirst, vlast;
		CAdapt_ItView* pView = gpApp->GetView();
		// BW added extra parameter Oct 2004, set it  > 0 so it has no effect on Bill's code here
		if (pView->AnalyseReference(ChVsFirst,nChFirst,vfirst,vlast,1))
		{
			// set the FromChapter and FromVerse of dialog to default to what's in the text
			wxString strChFrom,strVsFrom;
			strChFrom << nChFirst;
			strVsFrom << vfirst;
			pEditChFrom->SetValue(strChFrom);
			pEditVsFrom->SetValue(strVsFrom);
			nVsFirst = vfirst;
		}
		// BW added extra parameter Oct 2004, set it  > 0 so it has no effect on Bill's code here
		if (pView->AnalyseReference(ChVsLast,nChLast,vfirst,vlast,1))
		{
			// set the ToChapter and ToVerse of dialog to default to what's in the text
			wxString strChTo,strVsTo;
			strChTo << nChLast;
			strVsTo << vlast;
			pEditChTo->SetValue(strChTo);
			pEditVsTo->SetValue(strVsTo);
			nVsLast = vlast;
		}
	}

    // The "Selection" radio button should be disabled if there was no selection existing before
    // invoking the print options dialog.
	if (gpApp->m_selectionLine != -1)
	{
		// there is a selection so enable and set the "Selection" radio button by default
		pRadioSelection->Enable(TRUE);
		pRadioSelection->SetValue(TRUE);
		// unset the other radio buttons
		pRadioAll->SetValue(FALSE);
		pRadioPages->SetValue(FALSE);
		pRadioChVs->SetValue(FALSE);
	}
	else
	{
		// there is no selection current, so disable the "Selection" radio button and make the "All"
		// button the default button set, unsetting the others
		pRadioAll->SetValue(TRUE);
		// disable the "Selection" radio button
		pRadioSelection->Enable(FALSE);
		// unset the other radio buttons
		pRadioSelection->SetValue(FALSE);
		pRadioPages->SetValue(FALSE);
		pRadioChVs->SetValue(FALSE);
	}
	// disable all the edit boxes, since neither the "All" nor "Selection" above uses the edit boxes
	if (pEditPagesFrom->IsEnabled())
		pEditPagesFrom->Enable(FALSE);
	if (pEditPagesTo->IsEnabled())
		pEditPagesTo->Enable(FALSE);
	if (pEditChFrom->IsEnabled())
		pEditChFrom->Enable(FALSE);
	if (pEditVsFrom->IsEnabled())
		pEditVsFrom->Enable(FALSE);
	if (pEditChTo->IsEnabled())
		pEditChTo->Enable(FALSE);
	if (pEditVsTo->IsEnabled())
		pEditVsTo->Enable(FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the "Print >>" button on the dialog is pressed (same as an OK button on a normal
/// dialog). It sets the printing globals to reflect the users choices made in the Print Options dialog. 
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnOK(wxCommandEvent& event) 
{
	// whm note: Since we are not fiddling with our print options within a customized print dialog, we
	// could do all this from within the wxID_OK block of code in OnPrint(). 
	// 
	// set the globals for the ch & verse range
	// MFC version doesn't do any validation checking of the numbers entered into the ch:verse edit boxes
	// TODO: do we need to do some validation of the number data? We could veto the OnOK() process to allow the
	// user to correct any malformed numbers.
	wxString s;
	// get the edit box strings and convert them to numbers
	s = pEditChFrom->GetValue();
	if (!s.IsEmpty())
		gnFromChapter = wxAtoi(s);
	else
		gnFromChapter = 0; // whm added default

	s = pEditVsFrom->GetValue();
	if (!s.IsEmpty())
		gnFromVerse = wxAtoi(s);
	else
		gnFromVerse = 0; // whm added default
	
	s = pEditChTo->GetValue();
	if (!s.IsEmpty())
		gnToChapter = wxAtoi(s);
	else
		gnToChapter = 0; // whm added default
	
	s = pEditVsTo->GetValue();
	if (!s.IsEmpty())
		gnToVerse = wxAtoi(s);
	else
		gnToVerse = 0; // whm added default
	
	gbPrintingRange = pRadioChVs->GetValue();
	gbPrintFooter = !m_bSuppressFooter; // reverse the boolean value here
	gbSuppressPrecedingHeadingInRange = pCheckSuppressPrecSectHeading->GetValue();
	gbIncludeFollowingHeadingInRange = pCheckIncludeFollSectHeading->GetValue();

	// check about what to do with any section headings, if they precede or follow the range
	if(!gbPrintingRange)
	{
		// we only get here if no range print is wanted, in which case we set booleans back to default values
		gbSuppressPrecedingHeadingInRange = FALSE;
		gbIncludeFollowingHeadingInRange = FALSE;
	}
	
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when focus shifts to a different element or control in the dialog. This handler only detects
/// when focus comes to the edit box controls (i.e., the user clicks in an edit box). It functions to
/// insure that the radio button associated with the edit box is selected (assumes that if the user is
/// entering data into an edit box, the user wants to also select the associated radio button).
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnSetfocus(wxFocusEvent& event) 
{
	// MFC uses separate focus handlers, but wx is designed to handle focus events in a single handler with
	// queries to event.GetEventObject() and event.GetEventType() and setting focus on the desired control
	// based on those event methods.
	if (event.GetEventObject() == pEditPagesFrom || event.GetEventObject() == pEditPagesTo)
	{
		if (event.GetEventType() == wxEVT_SET_FOCUS)
		{
			pRadioAll->SetValue(FALSE);
			pRadioSelection->SetValue(FALSE);
			pRadioPages->SetValue(TRUE);
			pRadioChVs->SetValue(FALSE);
		}
	}
	if (event.GetEventObject() == pEditChFrom || event.GetEventObject() == pEditVsFrom
		|| event.GetEventObject() == pEditChTo || event.GetEventObject() == pEditVsTo)
	{
		if (event.GetEventType() == wxEVT_SET_FOCUS)
		{
			pRadioAll->SetValue(FALSE);
			pRadioSelection->SetValue(FALSE);
			pRadioPages->SetValue(FALSE);
			pRadioChVs->SetValue(TRUE);
		}
	}

	// In the MFC version the focus handlers also had code to detect if the "Selection" button had
	// previously been selected, and if so, it proceeded to "restore the full document." But that is
	// not needed in the wx version because no changes have been made to the document at this point and
	// the print dialog itself has not been invoked.
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the user clicks on the "All" radio button. It insures that the "All" radio button is
/// selected and all other radio buttons in the group are not selected. It also disables the edit
/// boxes for page and chapter/verse ranges which are not appropriate for data entry as long as the "All"
/// radio button is selected.
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnAllPagesBtn(wxCommandEvent& WXUNUSED(event))
{
	// When user clicks on the "All" button, we need to unselect the other radio buttons.
	// Normally, when we have 4 radio buttons forming a "group" and one of them is selected, the other 3
	// should automatically get unselected, but to be safe we'll they are unselected.
	pRadioSelection->SetValue(FALSE);
	pRadioPages->SetValue(FALSE);
	pRadioChVs->SetValue(FALSE);

	// disable all the edit boxes
	pEditChFrom->Enable(FALSE);
	pEditVsFrom->Enable(FALSE);
	pEditChTo->Enable(FALSE);
	pEditVsTo->Enable(FALSE);
	pEditPagesFrom->Enable(FALSE);
	pEditPagesTo->Enable(FALSE);

	// whm Note: In the MFC version this OnPageBtn() handler had code to detect if the "Selection"
	// button had previously been set, and if so, it undertook to get the document back into a certain
	// state. This code is not needed in the wx version, because at the point in the Print Options
	// Dialog, nothing has changed in the document, OnPreparePrinting has not been called etc.
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the user clicks on the "Select" radio button. It insures that the "Selection" radio button 
/// is selected and all other radio buttons in the group are not selected. It also disables the edit
/// boxes for page and chapter/verse ranges which are not appropriate for data entry as long as the 
/// "Selection" radio button is selected.
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnSelectBtn(wxCommandEvent& WXUNUSED(event))
{
	pRadioAll->SetValue(FALSE);
	pRadioPages->SetValue(FALSE);
	pRadioChVs->SetValue(FALSE);

	// disable all the edit boxes
	pEditChFrom->Enable(FALSE);
	pEditVsFrom->Enable(FALSE);
	pEditChTo->Enable(FALSE);
	pEditVsTo->Enable(FALSE);
	pEditPagesFrom->Enable(FALSE);
	pEditPagesTo->Enable(FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the user clicks on the "Pages" radio button. It insures that the "Pages" radio button 
/// is selected and all other radio buttons in the group are not selected. It also enables the "from"
/// and "to" edit boxes, but disables the edit boxes for chapter/verse ranges. When the "Pages" radio
/// button is clicked, focus moves to the first empty edit box associated with the "pages" button.
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnPagesBtn(wxCommandEvent& WXUNUSED(event)) 
{
	pRadioAll->SetValue(FALSE);
	pRadioSelection->SetValue(FALSE);
	pRadioChVs->SetValue(FALSE);

	// enable the from and to edit boxes
	pEditPagesFrom->Enable(TRUE);
	pEditPagesTo->Enable(TRUE);
	// disable the Chapter/Verse Range edit boxes
	pEditChFrom->Enable(FALSE);
	pEditVsFrom->Enable(FALSE);
	pEditChTo->Enable(FALSE);
	pEditVsTo->Enable(FALSE);

	// When the Pages radio button is clicked, move focus to the first
	// empty edit box, i.e., the from: edit box or the to: edit box.
	if (pEditPagesFrom->GetValue() == _T(""))
		pEditPagesFrom->SetFocus();
	else if (pEditPagesTo->GetValue() == _T(""))
		pEditPagesTo->SetFocus();
	else
		pEditPagesFrom->SetFocus();

	// whm Note: In the MFC version this OnPageBtn() handler had code to detect if the "Selection"
	// button had previously been set, and if so, it undertook to get the document back into a certain
	// state. This code is not needed in the wx version, because at the point in the Print Options
	// Dialog, nothing has changed in the document, OnPreparePrinting has not been called etc.
}
	
////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the user clicks on the "Chapter/Verse Range" radio button. It insures that the
/// "Chapter/Verse Range" radio button is selected and all other radio buttons in the group are 
/// not selected. It also enables the four edit boxes associated with the Chapter/Verse Range radio
/// button, and disables the two edit boxes associated with the Pages radio button. When the
/// "Chapter/Verse Range" radio button is clicked, focus moves to the first empty edit box associated 
/// with it.
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnRadioChapterVerseRange(wxCommandEvent& WXUNUSED(event))
{
	pRadioAll->SetValue(FALSE);
	pRadioSelection->SetValue(FALSE);
	pRadioPages->SetValue(FALSE);
	
	// enable the Chapter/Verse Range edit boxes
	pEditChFrom->Enable(TRUE);
	pEditVsFrom->Enable(TRUE);
	pEditChTo->Enable(TRUE);
	pEditVsTo->Enable(TRUE);
	// disable the from and to edit boxes
	pEditPagesFrom->Enable(FALSE);
	pEditPagesTo->Enable(FALSE);

	// when the Chapter/Verse Range radio button is clicked, move focus to the first
	// empty edit box, which would often be from:chapter, unless user is coming back to edit
	// some more values (via click of hot key on the Chapter/Verse Range radio button) after having
	// moved the focus elsewhere.
	if (pEditChFrom->GetValue() == _T(""))
		pEditChFrom->SetFocus();
	else if (pEditVsFrom->GetValue() == _T(""))
		pEditVsFrom->SetFocus();
	else if (pEditChTo->GetValue() == _T(""))
		pEditChTo->SetFocus();
	else if (pEditVsTo->GetValue() == _T(""))
		pEditVsTo->SetFocus();
	else
		pEditChFrom->SetFocus();

	// whm Note: In the MFC version this OnPageBtn() handler had code to detect if the "Selection"
	// button had previously been set, and if so, it undertook to get the document back into a certain
	// state. This code is not needed in the wx version, because at the point in the Print Options
	// Dialog, nothing has changed in the document, OnPreparePrinting has not been called etc.

}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the user clicks on the Suppress Footer checkbox. It insures that the
/// m_bSuppressFooter variable is assigned accordingly. 
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnCheckSuppressFooter(wxCommandEvent& WXUNUSED(event)) 
{
	m_bSuppressFooter = pCheckSuppressPrintingFooter->GetValue();
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the user clicks on the Suppress Preceding Section Heading checkbox. It insures 
/// that the gbSuppressPrecedingHeadingInRange global is assigned accordingly. 
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnCheckSuppressPrecedingHeading(wxCommandEvent& WXUNUSED(event))
{
	gbSuppressPrecedingHeadingInRange = pCheckSuppressPrecSectHeading->GetValue();
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the user clicks on the Include Following Section Heading checkbox. It insures 
/// that the gbIncludeFollowingHeadingInRange global is assigned accordingly. 
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnCheckIncludeFollowingHeading(wxCommandEvent& WXUNUSED(event))
{
	gbIncludeFollowingHeadingInRange = pCheckIncludeFollSectHeading->GetValue();
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the user makes any change in the Pages "from" edit box. It first insures that the Pages
/// radio button is selected, enables the Pages related edit boxes, and disables the four edit boxes
/// associated with the "Chapter/Verse Range" radio button. 
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnEditPagesFrom(wxCommandEvent& WXUNUSED(event))
{
	// user has chamged a Pages range, so insure that the Pages radio button is selected if not already
	if (pRadioPages->GetValue() == FALSE)
	{
		pRadioAll->SetValue(FALSE);
		pRadioSelection->SetValue(FALSE);
		pRadioPages->SetValue(TRUE);
		pRadioChVs->SetValue(FALSE);
	}
	// enable the Pages edit boxes
	if (!pEditPagesFrom->IsEnabled())
		pEditPagesFrom->Enable(TRUE);
	if (!pEditPagesTo->IsEnabled())
		pEditPagesTo->Enable(TRUE);
	// disable the Chapter/Verse edit boxes
	if (pEditChFrom->IsEnabled())
		pEditChFrom->Enable(FALSE);
	if (pEditVsFrom->IsEnabled())
		pEditVsFrom->Enable(FALSE);
	if (pEditChTo->IsEnabled())
		pEditChTo->Enable(FALSE);
	if (pEditVsTo->IsEnabled())
		pEditVsTo->Enable(FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the user makes any change in the Pages "to" edit box. It first insures that the Pages
/// radio button is selected, enables the Pages related edit boxes, and disables the four edit boxes
/// associated with the "Chapter/Verse Range" radio button. 
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnEditPagesTo(wxCommandEvent& WXUNUSED(event))
{
	// user has chamged a Pages range, so insure that the Pages radio button is selected if not already
	if (pRadioPages->GetValue() == FALSE)
	{
		pRadioAll->SetValue(FALSE);
		pRadioSelection->SetValue(FALSE);
		pRadioPages->SetValue(TRUE);
		pRadioChVs->SetValue(FALSE);
	}
	// enable the Pages edit boxes
	if (!pEditPagesFrom->IsEnabled())
		pEditPagesFrom->Enable(TRUE);
	if (!pEditPagesTo->IsEnabled())
		pEditPagesTo->Enable(TRUE);
	// disable the Chapter/Verse edit boxes
	if (pEditChFrom->IsEnabled())
		pEditChFrom->Enable(FALSE);
	if (pEditVsFrom->IsEnabled())
		pEditVsFrom->Enable(FALSE);
	if (pEditChTo->IsEnabled())
		pEditChTo->Enable(FALSE);
	if (pEditVsTo->IsEnabled())
		pEditVsTo->Enable(FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the user makes any change in the "chapter from" edit box. It first insures that the
/// Chapter/Verses Range radio button is selected, enables all the related edit boxes, and disables 
/// the two edit boxes associated with the "Pages" radio button. 
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnEditChapterFrom(wxCommandEvent& WXUNUSED(event))
{
	// user has chamged a Chapter/Verse range, so insure that the Chapter/Verse Range radio button is selected if not already
	if (pRadioChVs->GetValue() == FALSE)
	{
		pRadioAll->SetValue(FALSE);
		pRadioSelection->SetValue(FALSE);
		pRadioPages->SetValue(FALSE);
		pRadioChVs->SetValue(TRUE);
	}
	// enable the Chapter/Verse edit boxes
	if (!pEditChFrom->IsEnabled())
		pEditChFrom->Enable(TRUE);
	if (!pEditVsFrom->IsEnabled())
		pEditVsFrom->Enable(TRUE);
	if (!pEditChTo->IsEnabled())
		pEditChTo->Enable(TRUE);
	if (!pEditVsTo->IsEnabled())
		pEditVsTo->Enable(TRUE);
	// disable the Pages edit boxes
	if (pEditPagesFrom->IsEnabled())
		pEditPagesFrom->Enable(FALSE);
	if (pEditPagesTo->IsEnabled())
		pEditPagesTo->Enable(FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the user makes any change in the "verse" (from) edit box. It first insures that the
/// Chapter/Verses Range radio button is selected, enables all the related edit boxes, and disables 
/// the two edit boxes associated with the "Pages" radio button. 
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnEditVerseFrom(wxCommandEvent& WXUNUSED(event))
{
	// user has chamged a Chapter/Verse range, so insure that the Chapter/Verse Range radio button is selected if not already
	if (pRadioChVs->GetValue() == FALSE)
	{
		pRadioAll->SetValue(FALSE);
		pRadioSelection->SetValue(FALSE);
		pRadioPages->SetValue(FALSE);
		pRadioChVs->SetValue(TRUE);
	}
	// enable the Chapter/Verse edit boxes
	if (!pEditChFrom->IsEnabled())
		pEditChFrom->Enable(TRUE);
	if (!pEditVsFrom->IsEnabled())
		pEditVsFrom->Enable(TRUE);
	if (!pEditChTo->IsEnabled())
		pEditChTo->Enable(TRUE);
	if (!pEditVsTo->IsEnabled())
		pEditVsTo->Enable(TRUE);
	// disable the Pages edit boxes
	if (pEditPagesFrom->IsEnabled())
		pEditPagesFrom->Enable(FALSE);
	if (pEditPagesTo->IsEnabled())
		pEditPagesTo->Enable(FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the user makes any change in the "chapter to" edit box. It first insures that the
/// Chapter/Verses Range radio button is selected, enables all the related edit boxes, and disables 
/// the two edit boxes associated with the "Pages" radio button. 
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnEditChapterTo(wxCommandEvent& WXUNUSED(event))
{
	// user has chamged a Chapter/Verse range, so insure that the Chapter/Verse Range radio button is selected if not already
	if (pRadioChVs->GetValue() == FALSE)
	{
		pRadioAll->SetValue(FALSE);
		pRadioSelection->SetValue(FALSE);
		pRadioPages->SetValue(FALSE);
		pRadioChVs->SetValue(TRUE);
	}
	// enable the Chapter/Verse edit boxes
	if (!pEditChFrom->IsEnabled())
		pEditChFrom->Enable(TRUE);
	if (!pEditVsFrom->IsEnabled())
		pEditVsFrom->Enable(TRUE);
	if (!pEditChTo->IsEnabled())
		pEditChTo->Enable(TRUE);
	if (!pEditVsTo->IsEnabled())
		pEditVsTo->Enable(TRUE);
	// disable the Pages edit boxes
	if (pEditPagesFrom->IsEnabled())
		pEditPagesFrom->Enable(FALSE);
	if (pEditPagesTo->IsEnabled())
		pEditPagesTo->Enable(FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the user makes any change in the "verse" (to) edit box. It first insures that the
/// Chapter/Verses Range radio button is selected, enables all the related edit boxes, and disables 
/// the two edit boxes associated with the "Pages" radio button. 
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnEditVerseTo(wxCommandEvent& WXUNUSED(event))
{
	// user has chamged a Chapter/Verse range, so insure that the Chapter/Verse Range radio button is selected if not already
	if (pRadioChVs->GetValue() == FALSE)
	{
		pRadioAll->SetValue(FALSE);
		pRadioSelection->SetValue(FALSE);
		pRadioPages->SetValue(FALSE);
		pRadioChVs->SetValue(TRUE);
	}
	// enable the Chapter/Verse edit boxes
	if (!pEditChFrom->IsEnabled())
		pEditChFrom->Enable(TRUE);
	if (!pEditVsFrom->IsEnabled())
		pEditVsFrom->Enable(TRUE);
	if (!pEditChTo->IsEnabled())
		pEditChTo->Enable(TRUE);
	if (!pEditVsTo->IsEnabled())
		pEditVsTo->Enable(TRUE);
	// disable the Pages edit boxes
	if (pEditPagesFrom->IsEnabled())
		pEditPagesFrom->Enable(FALSE);
	if (pEditPagesTo->IsEnabled())
		pEditPagesTo->Enable(FALSE);
}

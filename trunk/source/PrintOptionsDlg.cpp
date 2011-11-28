/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			PrintOptionsDlg.cpp
/// \author			Bill Martin
/// \date_created	10 November 2006
/// \date_revised	1 March 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CPrintOptionsDlg class.
/// The CPrintOptionsDlg class provides a dialog for the user to enter a range
/// of pages, range of chapter/verse and specify how to handle margin elements
/// such as section headings and footers at print time. This dialog pops up
/// before the standard print dialog when the user selects File | Print.
/// \derivation		The CPrintOptionsDlg class is derived from AIModalDialog.
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
#include "Cell.h"
#include "Pile.h"
#include "Layout.h"
#include "FreeTrans.h"
#include "PrintOptionsDlg.h"
#include "Adapt_ItView.h"
#include "MainFrm.h"
#include "Adapt_ItCanvas.h"
#include "WaitDlg.h"

#define Print_failure

// next two are for version 2.0 which includes the option of a 3rd line for glossing

// This global is defined in Adapt_ItView.cpp.
//extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

// This global is defined in Adapt_ItView.cpp.
//extern bool	gbGlossingVisible; // TRUE makes Adapt It revert to Shoebox functionality only

extern bool	gbPrintingRange;
extern int	gnFromChapter;
extern int	gnFromVerse;
extern int	gnToChapter;
extern int	gnToVerse;

/// This global is defined in Adapt_ItView.cpp.
extern bool gbPrintFooter;

extern bool gbSuppressPrecedingHeadingInRange;
extern bool gbIncludeFollowingHeadingInRange;
extern bool	gbCheckInclFreeTransText; // klb 9/9/2011
extern bool	gbCheckInclGlossesText;   // klb 9/9/2011
extern bool gbGlossingVisible;


/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

// event handler table
BEGIN_EVENT_TABLE(CPrintOptionsDlg, AIModalDialog)
	EVT_INIT_DIALOG(CPrintOptionsDlg::InitDialog)
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
	EVT_BUTTON(wxID_CANCEL, CPrintOptionsDlg::OnCancel)
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

	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);

	m_pView = gpApp->GetView();

	//m_pPrintout = pPrintout; // initialize pointer to the AIPrintout of the caller (OnPrint in the View)

	m_bPrintingRange = FALSE;
	gpApp->m_bPrintingRange = FALSE;
	m_bSuppressFooter = FALSE;
	gbPrintFooter = TRUE;

	// whm 30Aug11 added the next two for the "Additional text to include
	// in Printouts" section of the "Special Print Options" dialog.
	pCheckInclFreeTransText = (wxCheckBox*)FindWindowById(ID_CHECKBOX_INCLUDE_FREE_TRANS_TEXT);
	wxASSERT(pCheckInclFreeTransText != NULL);

	pCheckInclGlossesText = (wxCheckBox*)FindWindowById(ID_CHECKBOX_INCLUDE_GLOSSES_TEXT);
	wxASSERT(pCheckInclGlossesText != NULL);

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
	CAdapt_ItApp* pApp = &wxGetApp();

#if defined(Print_failure)
#if defined(__WXDEBUG__) && defined(__WXGTK__)
    wxLogDebug(_T("PrintOptionsDlg  ~CPrintOptionsDlg() line 205 at start: gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d ,\n        bHideFreeTranslationsOnClose = %d, m_bFreeTranslationMode = %d"),
               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)bHideFreeTranslationsOnClose, (int)pApp->m_bFreeTranslationMode);
#endif
#endif
	if (bHideFreeTranslationsOnClose)
	{
		pApp->GetFreeTrans()->SwitchScreenFreeTranslationMode(ftModeOFF);
	}
	if (bHideGlossesOnClose)
	{
		pApp->GetView()->ShowGlosses();
	}
	if (pApp->m_bFrozenForPrinting)
	{
		pApp->GetMainFrame()->Thaw();
		pApp->m_bFrozenForPrinting = FALSE;
	}
	// BEW added 19Nov11
    gbCheckInclFreeTransText = FALSE; // restore default OFF
    gbCheckInclGlossesText = FALSE; // restore default OFF

#if defined(Print_failure)
#if defined(__WXDEBUG__) && defined(__WXGTK__)
    wxLogDebug(_T("PrintOptionsDlg  ~CPrintOptionsDlg() line 228 at end: gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d ,\n        bHideFreeTranslationsOnClose = %d, m_bFreeTranslationMode = %d"),
               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)bHideFreeTranslationsOnClose, (int)pApp->m_bFreeTranslationMode);
#endif
#endif

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
	//wxLogDebug(_T("InitDialog() START"));
	CAdapt_ItApp* pApp = &wxGetApp();
#if defined(__WXDEBUG__) && defined(Print_failure)
    wxLogDebug(_T("InitDialog() on entry, m_selectionLine = %d , m_bPrintingSelection %d    (at line 250)"),
               pApp->m_selectionLine, pApp->m_bPrintingSelection);
#endif
    bool bSelectionOK = TRUE;
    bool bEncounteredSelection = FALSE;
    if (pApp->m_selectionLine != -1 && pApp->m_pAnchor != NULL && !pApp->m_selection.IsEmpty())
    {
        bEncounteredSelection = TRUE;
        bSelectionOK = pApp->SaveSelection();
        if (!bSelectionOK)
        {
            // don't expect a failure, so just alert developer and halt
            wxMessageBox(_T("CPrintOptionsDlg::InitDialog() failed trying to save the selection. Will halt. Fix me please."));
            wxASSERT(FALSE);
            return;
        }
    }

	// whm 30Aug11 added for the "Additional text to include in Printouts" section of
	// the "Special Print Options" dialog.

    // BEW 1Oct11, the m_pagesList of PageOffsets structs is not yet set up, and
    // LayoutAndPaginate() below will call RecalcLayout() in order to get the strips
    // populated in conformity with the page printable width. Draw() won't be called, but
    // it would be good to have insurance that the new DrawFreeTranslationsForPrinting(),
    // if control enters it when there are no PageOffset structs in the list in m_pagesList
    // of the app class, would do nothing if called prematurely. A handy way to do that is
    // to have CLayout::m_pOffsets pointer member, which is initialized to NULL when the
    // CLayout object was created, be set to NULL before any actual printing happens. We do
    // so here, and also in the destructor for AIPrintout() - in the latter so that a
    // second print attempt won't find a leftover nonNULL value in m_pOffsets from a
    // previous print session. What we do in DrawFreeTranslationsForPrinting() is test for
    // NULL - and if it is, we exit immediately so that DrawFreeTranslationsForPrinting()
    // does not try to do anything when no page is defined and ready for printing.
	pApp->m_pLayout->m_pOffsets = NULL;

	// whm 25Sep11 modified. Enable the box and put a check in it if the document actually
	// contains glosses, otherwise uncheck and disable the check box.
	//
	// BEW 5Oct11, prior to LayoutAndPaginate() being called, mode switches to get glosses
	// see and/or free translations visible (if in the document - we can't anticipate,
	// however, whether or not the user will uncheck their checkboxes in the dialog) -
	// otherwise, the calculations will be done with the current mode (probably with no
	// glosses or free translations visible) in which case the strip height will be too
	// small and the pagination won't agree with the Print Preview. So the mode needs to
	// be switched here & now, and a layout done. I'll do as Kevin did in view's
	// OnPrintPreview(), except I'll add a Freeze() here, and a Thaw() in the dialog's
	// destructor, so as to prevent Draw() changes when the layout is recalculated until
	// after the dialog is dismissed
	bool bDocHasGlosses = pApp->DocHasGlosses(pApp->m_pSourcePhrases);
	bool bDocHasFreeTrans = pApp->DocHasFreeTranslations(pApp->m_pSourcePhrases);

	// we'll unilaterally Freeze(), and unlaterally later Thaw(), whether or not we
	// subsequently turn on glossing or free translation modality
	pApp->GetMainFrame()->Freeze();
	pApp->m_bFrozenForPrinting = TRUE;

	// whm 25Sep11 modified. Enable the box and put a check in it if the document actually
	// contains free translations, otherwise uncheck and disable the check box.
	// BEW 5Oct11, work out if we need to turn on the gloss or free tr modalities, and set
	// booleans so that the dialog destructor can turn them back off when we are done
	bHideFreeTranslationsOnClose = FALSE;
	if (bDocHasFreeTrans)
	{
		if (!pApp->m_bFreeTranslationMode)
		{
			// if free trans mode is not on in the document, then we'll need to turn the
			// mode back off when done
			bHideFreeTranslationsOnClose = TRUE;
			pApp->GetFreeTrans()->SwitchScreenFreeTranslationMode(ftModeON);
		}
		pCheckInclFreeTransText->Enable(TRUE);
		pCheckInclFreeTransText->SetValue(TRUE);
		// Deprecated 19Nov11 --> Note the global flag gbCheckInclFreeTransText is set in OnOK()
		// BEW 19Nov11, CLayout::SetPileAndStripHeight() needs to use gbCheckInclFreeTransText
		// to get the correct strip height to use in the PaginateDoc() call within the
		// LayoutAndPaginate() call below, and the strip height will therefore not be correct
		// unless gbCheckInclFreeTransText is set TRUE here if, indeed, there actually are
		// free translations (failure to do this results in the dialog showing the max page
        // number as lower than what Print Preview shows - and the latter shows the correct
        // value. Therefore, we will set the flag here, because setting it in OnOK() is too
        // late.
        gbCheckInclFreeTransText = TRUE;
	}
	else
	{
		pCheckInclFreeTransText->Enable(FALSE);
		pCheckInclFreeTransText->SetValue(FALSE);
		// BEW deprecated comment on 19Nov11 (Note the global flag gbCheckInclFreeTransText
        // is set in OnOK()  -- see the block above for the reason)
        gbCheckInclFreeTransText = FALSE;
	}

	bHideGlossesOnClose = FALSE;
	if (bDocHasGlosses)
	{
		if (!gbGlossingVisible)
		{
			// if glosses are not visible, then we'll need to turn the
			// mode back off when done
			bHideGlossesOnClose = TRUE;
			pApp->GetView()->ShowGlosses();
		}
		pCheckInclGlossesText->Enable(TRUE);
		pCheckInclGlossesText->SetValue(TRUE);
		// Deprecated 19Nov11 --> Note the global flag gbCheckInclGlossesText is set in OnOK()
		// BEW 19Nov11, CLayout::SetPileAndStripHeight() needs to use gbCheckInclGlossesText
		// to get the correct strip height to use in the PaginateDoc() call within the
		// LayoutAndPaginate() call below, and the strip height will therefore not be correct
		// unless gbCheckInclGlossesText is set TRUE here if, indeed, there actually are
		// gloses in the doc (failure to do this results in the dialog showing the max page
        // number as lower than what Print Preview shows - and the latter shows the correct
        // value. Therefore, we will set the flag here, because setting it in OnOK() is too
        // late.
        gbCheckInclGlossesText = TRUE;
	}
	else
	{
		pCheckInclGlossesText->Enable(FALSE);
		pCheckInclGlossesText->SetValue(FALSE);
		// Deprecated comment 19Nov11 --> Note the global flag gbCheckInclGlossesText is set
		//  in OnOK()  -- for the reason, see the comment in the block above
        gbCheckInclGlossesText = FALSE;
	}
#if defined(Print_failure)
#if defined(__WXDEBUG__) && defined(__WXGTK__)
    wxLogDebug(_T("PrintOptionsDlg  InitDialog() line 356: gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d, m_bFreeTranslationMode = %d"),
               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)pApp->m_bFreeTranslationMode);
#endif
#endif
	// Get the logical page dimensions for paginating the document and printing.
    int nPagePrintingWidthLU;
	int nPagePrintingLengthLU;
	bool bOK = pApp->CalcPrintableArea_LogicalUnits(nPagePrintingWidthLU, nPagePrintingLengthLU);
	if (!bOK)
	{
		// what should we do here, I guess it shouldn't happen, so I will just return and
		// hope -- maybe a beep too
		::wxBell();
		pApp->m_bIsPrinting = FALSE;
		return;
	}
    if (bEncounteredSelection)
    {
        bSelectionOK = pApp->RestoreSelection();
        if (!bSelectionOK)
        {
            // don't expect a failure, so just alert developer and halt
            wxMessageBox(_T("CPrintOptionsDlg::InitDialog() failed trying to restore the selection before LayoutAndPaginate(). Will halt. Fix me please."));
            wxASSERT(FALSE);
            return;
        }
        pApp->ClearSavedSelection();
    }

#if defined(__WXDEBUG__) && defined(Print_failure)
    wxLogDebug(_T("InitDialog() before LayoutAndPaginate(), m_selectionLine = %d , m_bPrintingSelection %d    (at line 376)"),
               pApp->m_selectionLine, pApp->m_bPrintingSelection);
#endif

	// setup the layout for the new pile width, and get the PageOffsets structs list populated
	// (LayoutAndPaginate() calls view class's PaginateDoc() & the latter does the PageOffsets
	// structs)
	bOK = pApp->LayoutAndPaginate(nPagePrintingWidthLU,nPagePrintingLengthLU);
	if (!bOK)
	{
		// what should we do here, I guess it shouldn't happen, but just in case....
		// Note, the document will have been modified here, so I think the right thing to
		// do is to call DoPrintCleanup()  to ensure the original state of the document
		// will be restored. Maybe ~AIPrintout() will be called too? I'm not sure, but we
		// can prevent reentrancy into DoPrintCleanup() by using the
		// m_nAIPrintout_Destructor_ReentrancyCount variable, DoPrintCleanup() will make
		// it = 2 on return, and it ~AIPrintout() destructor is later called, we won't get
		// a crash do to a second attempt to restore the document which has already been
		// restored here
		gpApp->m_nAIPrintout_Destructor_ReentrancyCount = 1;
		gpApp->DoPrintCleanup();
		::wxBell(); // we can still sound the bell, to let the user (or developer!) know something went bad
		return;
	}
#if defined(__WXDEBUG__) && defined(Print_failure)
    wxLogDebug(_T("InitDialog() after LayoutAndPaginate(), m_selectionLine = %d , m_bPrintingSelection %d    (at line 401)"),
               pApp->m_selectionLine, pApp->m_bPrintingSelection);
#endif

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
    /*
    if (bEncounteredSelection)
    {
        bSelectionOK = pApp->RestoreSelection();
        if (!bSelectionOK)
        {
            // don't expect a failure, so just alert developer and halt
            wxMessageBox(_T("put a message here."));
            wxASSERT(FALSE);
            return;
        }
    }
    */
#if defined(__WXDEBUG__) && defined(Print_failure)
    wxLogDebug(_T("InitDialog() before setting buttons, m_selectionLine = %d , m_bPrintingSelection %d    (at line 513)"),
               pApp->m_selectionLine, pApp->m_bPrintingSelection);
#endif
    // The "Selection" radio button should be disabled if there was no selection existing before
    // invoking the print options dialog.
	//if (gpApp->m_selectionLine != -1)
	if (pApp->m_bPrintingSelection) // LayoutAndPaginate() will have either set it or cleared it already
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

	// turn off gbIsPrinting before the Print Options Dlg shows, otherwise, when OnPaint()
	// has CLayout::Draw() called, tests for gbIsPringing TRUE will give crashes; we want
	// the flag to be FALSE while the dialog is up
	pApp->m_bIsPrinting = FALSE;

	/*
    if (bEncounteredSelection)
    {
        pApp->ClearSavedSelection();
    }
    */

//	wxLogDebug(_T("InitDialog() END"));
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
	CAdapt_ItApp* pApp = &wxGetApp();

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

	gpApp->m_bPrintingRange = pRadioChVs->GetValue();
	gbPrintFooter = !m_bSuppressFooter; // reverse the boolean value here
	gbSuppressPrecedingHeadingInRange = pCheckSuppressPrecSectHeading->GetValue();
	gbIncludeFollowingHeadingInRange = pCheckIncludeFollSectHeading->GetValue();

	// Deprecated comment 19Nov11 --> Load new selections for including freetext and
	// gloss info - klb 9/9/2011
    // BEW 19Nov11, CLayout::SetPileAndStripHeight() needed to use gbCheckInclFreeTransText
    // and/or gbCheckInclGlossesText, to get the correct strip height to use in the
    // PaginateDoc() call within the LayoutAndPaginate() call in InitDialog(), and the
    // strip height would therefore not be correct unless the correct values of these
    // flags were set in InitDialog() -- if, indeed, there actually are
    // free translations and/or glosses in the document. (Failure to set the flags
    // TRUE when need, in InitDialog(), results in the dialog showing the max page
    // number as lower than what Print Preview shows - and the latter shows the correct
    // value. Therefore, we will set the flag there, because setting it here in OnOK() is
    // too late, and for that reason the next two lines are now commented out.
	//gbCheckInclFreeTransText = pCheckInclFreeTransText->GetValue();
	//gbCheckInclGlossesText = pCheckInclGlossesText->GetValue();

	// check about what to do with any section headings, if they precede or follow the range
	if(!gpApp->m_bPrintingRange)
	{
		// we only get here if no range print is wanted, in which case we set booleans back to default values
		gbSuppressPrecedingHeadingInRange = FALSE;
		gbIncludeFollowingHeadingInRange = FALSE;
	}

    // we handle the possibility of the user having set a page range, in CAdapt_ItView::OnPrint()
    // after the latter has returned

	pApp->m_nAIPrintout_Destructor_ReentrancyCount = 1; // BEW added 18Jul09
#if defined(Print_failure)
#if defined(__WXDEBUG__) && defined(__WXGTK__)
    wxLogDebug(_T("PrintOptionsDlg  OnOK() line 626 at end: gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d , m_bFreeTranslationMode = %d"),
               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)pApp->m_bFreeTranslationMode);
#endif
#endif
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when the "Cancel" button on the dialog is pressed.
/// It deletes the set of PageOffsets structs. Failure to do this when the user cancels
/// would leak a lot of 16 byte memory blocks! (That's how I found we need this handler!)
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnCancel(wxCommandEvent& event)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	CAdapt_ItView* pView = gpApp->GetView();
	pView->ClearPagesList();
	pApp->m_nAIPrintout_Destructor_ReentrancyCount = 1; // BEW added 18Jul09
	// BEW 19Nov11, added the next two lines, because as of 19Nov these flags are set, not
	// in OnOK() any longer, but earlier, within InitDialog() -- because the calculation of
	// a correct strip height needs to know if free translation and/or gloss lines are to
	// be included in the printing, and that knowledge has to be available to the
	// LayoutAndPaginate() call done within InitDialog() -- and hence, if the user cancels
	// from the dialog, the flags have to here be restored to their default values
    gbCheckInclFreeTransText = FALSE;
	gbCheckInclGlossesText = FALSE;

	event.Skip();
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> (unused)
/// \remarks
/// Called when focus shifts to a different element or control in the dialog. This handler only detects
/// when focus comes to the edit box controls (i.e., the user clicks in an edit box). It functions to
/// ensure that the radio button associated with the edit box is selected (assumes that if the user is
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
	// for an explanation why the following test is here, see the 21Jul09 comment in the
	// header comment for the OnRadioChapterVerseRange() function
	if (gpApp->m_bPrintingSelection)
	{
		// not allowed to change the "printing selection" choice at this stage
		pRadioAll->SetValue(FALSE);
		pRadioSelection->SetValue(TRUE);
		::wxBell();
		return;
	}
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
	// for an explanation why the following test is here, see the 21Jul09 comment in the
	// header comment for the OnRadioChapterVerseRange() function
	// (Actually, this test is not needed because if there was no selection, this button
	// will be disabled prior to the dialog being shown)
	if (!gpApp->m_bPrintingSelection)
	{
		// it's now too late to change to a "printing selection" choice at this stage,
		// because there is no way for the user to set up a selection from within the
		// print options dialog, so just give an audible warning and do nothing
		::wxBell();
		return;
	}
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
	// for an explanation why the following test is here, see the 21Jul09 comment in the
	// header comment for the OnRadioChapterVerseRange() function
	if (gpApp->m_bPrintingSelection)
	{
		// not allowed to change the "printing selection" choice at this stage
		pRadioPages->SetValue(FALSE);
		pRadioSelection->SetValue(TRUE);
		::wxBell();
		return;
	}
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
/// BEW changed 21Jul09, a selection should have priority over a chapter:verse range,
/// because the selection is something the user does explicitly in the view before
/// invoking the print command - and if he's done a selection when he asks for printing,
/// we assume he means it. Therefore, the chapter:verse range option should only be
/// available when he's not already set up a selection - and we can test for a selection
/// having been set up because if so, gbPrintingSelection will be already TRUE when
/// control first gets the chance to enter this function, so use the boolean to suppress
/// switching to a range print in that circumstance. A ring of the bell should suffice as
/// a warning. (Our code does not permit making a selection, choosing print, then in the
/// print options dialog unchecking the Selection radiobutton in order to take some other
/// option such as full doc print, or range print, because the selection will already have
/// been temporarily made to be the whole document, and we can't change that fact at this
/// late stage - the selection must be printed, the only other option is to Cancel right
/// out of the print operation)
////////////////////////////////////////////////////////////////////////////////////////////
void CPrintOptionsDlg::OnRadioChapterVerseRange(wxCommandEvent& WXUNUSED(event))
{
	if (gpApp->m_bPrintingSelection)
	{
		// not allowed to change the "printing selection" choice at this stage
		pRadioChVs->SetValue(FALSE);
		pRadioSelection->SetValue(TRUE);
		::wxBell();
		return;
	}
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
	// enable the section header checkboxes
	pCheckSuppressPrecSectHeading->Enable(TRUE);
	pCheckIncludeFollSectHeading->Enable(TRUE);

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
	// user has changed a Pages range, so ensure that the Pages radio button is selected if not already
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
	// user has chamged a Pages range, so ensure that the Pages radio button is selected if not already
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
	// user has chamged a Chapter/Verse range, so ensure that the Chapter/Verse Range radio button is selected if not already
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
	// user has chamged a Chapter/Verse range, so ensure that the Chapter/Verse Range radio button is selected if not already
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
	// user has chamged a Chapter/Verse range, so ensure that the Chapter/Verse Range radio button is selected if not already
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
	// user has chamged a Chapter/Verse range, so ensure that the Chapter/Verse Range radio button is selected if not already
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

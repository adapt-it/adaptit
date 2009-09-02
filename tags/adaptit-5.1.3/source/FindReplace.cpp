/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FindReplace.cpp
/// \author			Bill Martin
/// \date_created	24 July 2006
/// \date_revised	8 June 2007
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for two classes: CFindDlg and CReplaceDlg.
/// These classes provide a find and/or find and replace dialog in which the user can find 
/// the location(s) of specified source and/or target text. The dialog has an "Ignore case" 
/// checkbox, a "Special Search" button, and other options. The replace fdialog allows the 
/// user to specify a replacement string.
/// Both CFindDlg and CReplaceDlg are created as a Modeless dialogs. They are created on 
/// the heap and are displayed with Show(), not ShowModal().
/// \derivation		The CFindDlg and CReplaceDlg classes are derived from wxDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in FindReplace.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "FindReplace.h"
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
#include "FindReplace.h"
#include "Adapt_ItView.h"
#include "Cell.h"
#include "Pile.h"
#include "Strip.h"
#include "SourcePhrase.h"
#include "Layout.h"
#include "Adapt_ItDoc.h"
#include "MainFrm.h" // whm added 24Jul06
#include "Adapt_ItCanvas.h" // whm added 24Jul06
#include "helpers.h"

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbEnableGlossing; // TRUE makes Adapt It revert to Shoebox functionality only

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

/// This global is defined in Adapt_It.cpp.
extern wxChar	gSFescapechar;

extern bool		gbFind;
extern bool		gbFindIsCurrent;
extern bool		gbJustReplaced;
extern int		gnRetransEndSequNum;
extern wxString	gOldConcatStr;
extern wxString	gOldConcatStrNoPunct;
extern bool		gbFindOrReplaceCurrent;
extern bool		gbSaveSuppressFirst; // save the toggled state of the lines in the strips (across Find or
extern bool		gbSaveSuppressLast;  // Find and Replace operations)

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// for ReplaceAll support
bool	gbFound; // TRUE if Find Next succeeded in making a match, else FALSE

/// TRUE if a Find and Replace operation is current, otherwise FALSE
bool	gbReplaceAllIsCurrent = FALSE;
bool	gbJustCancelled = FALSE;


// the CFindDlg class //////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS( CFindDlg, wxDialog )

// event handler table
BEGIN_EVENT_TABLE(CFindDlg, wxDialog)
	EVT_INIT_DIALOG(CFindDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_CANCEL, CFindDlg::OnCancel)	
	
	EVT_COMBOBOX(IDC_COMBO_SFM, CFindDlg::OnSelchangeComboSfm)
	EVT_BUTTON(IDC_BUTTON_SPECIAL, CFindDlg::OnButtonSpecial)
	
	EVT_BUTTON(wxID_OK, CFindDlg::OnFindNext)
	EVT_RADIOBUTTON(IDC_RADIO_SRC_ONLY_FIND, CFindDlg::OnRadioSrcOnly)
	EVT_RADIOBUTTON(IDC_RADIO_TGT_ONLY_FIND, CFindDlg::OnRadioTgtOnly)
	EVT_RADIOBUTTON(IDC_RADIO_SRC_AND_TGT_FIND, CFindDlg::OnRadioSrcAndTgt)
	EVT_RADIOBUTTON(IDC_RADIO_RETRANSLATION, CFindDlg::OnRadioRetranslation)
	EVT_RADIOBUTTON(IDC_RADIO_NULL_SRC_PHRASE, CFindDlg::OnRadioNullSrcPhrase)
	EVT_RADIOBUTTON(IDC_RADIO_SFM, CFindDlg::OnRadioSfm)
END_EVENT_TABLE()
	
// these are unique to CFindDlg

CFindDlg::CFindDlg()
{
}

CFindDlg::~CFindDlg()
{
}

CFindDlg::CFindDlg(wxWindow* parent) // dialog constructor
	: wxDialog(parent, -1, _("Find"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pFindDlgSizer = FindDlgFunc(this, TRUE, TRUE);
	// The declaration is: FindDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);

	m_marker = -1;
	m_srcStr = _T("");
	m_replaceStr = _T("");
	m_tgtStr = _T("");
	m_bIncludePunct = FALSE;
	m_bSpanSrcPhrases = FALSE;
	m_bIgnoreCase = FALSE;

	// Get pointers to dialog controls created in FindDlgFunc() [by wxDesigner]
	
	m_pRadioSrcTextOnly = (wxRadioButton*)FindWindowById(IDC_RADIO_SRC_ONLY_FIND);
	wxASSERT(m_pRadioSrcTextOnly != NULL);

	m_pRadioTransTextOnly = (wxRadioButton*)FindWindowById(IDC_RADIO_TGT_ONLY_FIND);
	wxASSERT(m_pRadioTransTextOnly != NULL);

	m_pRadioBothSrcAndTransText = (wxRadioButton*)FindWindowById(IDC_RADIO_SRC_AND_TGT_FIND);
	wxASSERT(m_pRadioBothSrcAndTransText != NULL);

	m_pCheckIgnoreCase = (wxCheckBox*)FindWindowById(IDC_CHECK_IGNORE_CASE_FIND);
	wxASSERT(m_pCheckIgnoreCase != NULL);
	m_pCheckIgnoreCase->SetValidator(wxGenericValidator(&m_bIgnoreCase)); // use validator

	m_pFindNext = (wxButton*)FindWindow(wxID_OK); // just use FindWindow here to limit search to CFindDlg
	wxASSERT(m_pFindNext != NULL);

	m_pStaticSrcBoxLabel = (wxStaticText*)FindWindowById(IDC_STATIC_SRC_FIND);
	wxASSERT(m_pStaticSrcBoxLabel != NULL);

	m_pEditSrc = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC_FIND);
	wxASSERT(m_pEditSrc != NULL);
	m_pEditSrc->SetValidator(wxGenericValidator(&m_srcStr)); // use validator

	m_pStaticTgtBoxLabel = (wxStaticText*)FindWindowById(IDC_STATIC_TGT_FIND);
	wxASSERT(m_pStaticTgtBoxLabel != NULL);

	m_pEditTgt = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT_FIND);
	wxASSERT(m_pEditTgt != NULL);
	m_pEditTgt->SetValidator(wxGenericValidator(&m_tgtStr)); // use validator

	m_pButtonSpecialNormal = (wxButton*)FindWindowById(IDC_BUTTON_SPECIAL);
	wxASSERT(m_pButtonSpecialNormal != NULL);

	m_pCheckIncludePunct = (wxCheckBox*)FindWindowById(IDC_CHECK_INCLUDE_PUNCT_FIND);
	wxASSERT(m_pCheckIncludePunct != NULL);
	m_pCheckIncludePunct->SetValidator(wxGenericValidator(&m_bIncludePunct)); // use validator

	m_pCheckSpanSrcPhrases = (wxCheckBox*)FindWindowById(IDC_CHECK_SPAN_SRC_PHRASES_FIND);
	wxASSERT(m_pCheckSpanSrcPhrases != NULL);
	m_pCheckSpanSrcPhrases->SetValidator(wxGenericValidator(&m_bSpanSrcPhrases)); // use validator

	m_pSpecialSearches = (wxStaticText*)FindWindowById(IDC_STATIC_SPECIAL);
	wxASSERT(m_pSpecialSearches != NULL);

	m_pSelectAnSfm = (wxStaticText*)FindWindowById(IDC_STATIC_SELECT_MKR);
	wxASSERT(m_pSelectAnSfm != NULL);

	m_pFindRetranslation = (wxRadioButton*)FindWindowById(IDC_RADIO_RETRANSLATION);
	wxASSERT(m_pFindRetranslation != NULL);
	m_pFindRetranslation->SetValidator(wxGenericValidator(&m_bFindRetranslation)); // use validator

	m_pFindPlaceholder = (wxRadioButton*)FindWindowById(IDC_RADIO_NULL_SRC_PHRASE);
	wxASSERT(m_pFindPlaceholder != NULL);
	m_pFindPlaceholder->SetValidator(wxGenericValidator(&m_bFindNullSrcPhrase)); // use validator

	m_pFindSFM = (wxRadioButton*)FindWindowById(IDC_RADIO_SFM);
	wxASSERT(m_pFindSFM != NULL);
	m_pFindSFM->SetValidator(wxGenericValidator(&m_bFindSFM)); // use validator

	m_pComboSFM = (wxComboBox*)FindWindowById(IDC_COMBO_SFM); 
	wxASSERT(m_pComboSFM != NULL);
	m_pComboSFM->SetValidator(wxGenericValidator(&m_marker)); // use validator
}

void CFindDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	gbFound = FALSE;
	gbReplaceAllIsCurrent = FALSE;
	gbJustCancelled = FALSE;

	// make the font show user-chosen point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pEditSrc, NULL,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pEditSrc, NULL, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif


	// for glossing or adapting
	if (gbIsGlossing && gbGlossingUsesNavFont)
	{
		#ifdef _RTL_FLAGS
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, m_pEditTgt, NULL,
									NULL, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bNavTextRTL);
		#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, m_pEditTgt, NULL, 
									NULL, NULL, gpApp->m_pDlgTgtFont);
		#endif
	}
	else
	{
		#ifdef _RTL_FLAGS
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pEditTgt, NULL,
									NULL, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bTgtRTL);
		#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pEditTgt, NULL, 
									NULL, NULL, gpApp->m_pDlgTgtFont);
		#endif
	}

	// set default checkbox values
	m_bIncludePunct = FALSE;
	m_bSpanSrcPhrases = FALSE;

	// hide the special stuff
	m_bSpecialSearch = FALSE;

	wxASSERT(m_pButtonSpecialNormal != NULL);
	wxString str;
	str = _("Special Search"); //str.Format(IDS_SPECIAL_SEARCH);
	m_pButtonSpecialNormal->SetLabel(str);
	
	wxASSERT(m_pFindRetranslation != NULL);
	m_pFindRetranslation->Hide();
	// pButton->Enable(FALSE);
	
	wxASSERT(m_pFindPlaceholder != NULL);
	m_pFindPlaceholder->Hide();
	// pButton->Enable(FALSE);
	
	wxASSERT(m_pFindSFM != NULL);
	m_pFindSFM->Hide();
	// pButton->Enable(FALSE);
	
	wxASSERT(m_pComboSFM != NULL);
	m_pComboSFM->Hide();
	
	wxASSERT(m_pSelectAnSfm != NULL);
	m_pSelectAnSfm->Hide();
	
	wxASSERT(m_pSpecialSearches != NULL);
	m_pSpecialSearches->Hide();

	// if glossing is ON, the "Search, retaining target text's punctuation" and
	// "Allow the search to occur in the text spanning multiple piles" have to be
	// hidden; since merging is not permitted and only the gloss line can be accessed
	// as the 'target'
	if (gbIsGlossing)
	{
		wxASSERT(m_pCheckIncludePunct != NULL);
		m_pCheckIncludePunct->Hide();
		
		wxASSERT(m_pCheckSpanSrcPhrases != NULL);
		m_pCheckSpanSrcPhrases->Hide();
	}

	// get the normal defaults set up
	wxASSERT(m_pRadioSrcTextOnly != NULL);
	m_pRadioSrcTextOnly->SetValue(TRUE);
	
	wxASSERT(m_pStaticTgtBoxLabel != NULL);
	m_pStaticTgtBoxLabel->Hide();
	
	wxASSERT(m_pEditTgt != NULL);
	m_pEditTgt->Hide();

	// whm added 17Feb05 in support of USFM and SFM Filtering
	// Hard coded sfm/descriptions strings were deleted from the Data
	// Properties attribute of the IDC_COMBO_SFM control in the
	// IDD_FIND_REPLACE dialog. The combox Sort attribute was also set.
	// Here we populate m_comboSFM combo box with the appropriate sfms 
	// depending on gCurrentSfmSet.
	// wx revision of behavior: We should list only those sfms which
	// are actually used in the currently open document (including any
	// unknown markers).
	MapSfmToUSFMAnalysisStruct::iterator iter;
	USFMAnalysis* pSfm;
	wxString key;
	wxString cbStr;
	m_pComboSFM->Clear(); // whm added 26Oct08
    // wx version: I've modified the m_pComboSFM->Append() routines to only put sensible
    // sfms in the combo box, i.e., none of the _basestyle markers or others that are
    // merely used internally by Adapt It are now included.
	switch(gpApp->gCurrentSfmSet)
	{
	case UsfmOnly:
		{
			for (iter = gpApp->m_pUsfmStylesMap->begin(); 
				iter != gpApp->m_pUsfmStylesMap->end(); ++iter)
			{
				// Retrieve each USFMAnalysis struct from the map
				key = iter->first;
				pSfm = iter->second;
				if (pSfm->marker.Find(_T('_')) == 0) // skip it if it is a marker beginning with _
					continue;
				cbStr = gSFescapechar;
				cbStr += pSfm->marker;
				cbStr += _T("  ");
				cbStr += pSfm->description;
				m_pComboSFM->Append(cbStr);
			}
			break;
		}
	case PngOnly:
		{
			for (iter = gpApp->m_pPngStylesMap->begin(); 
				iter != gpApp->m_pPngStylesMap->end(); ++iter)
			{
				// Retrieve each USFMAnalysis struct from the map
				key = iter->first;
				pSfm = iter->second;
				if (pSfm->marker.Find(_T('_')) == 0) // skip it if it is a marker beginning with _
					continue;
				cbStr = gSFescapechar;
				cbStr += pSfm->marker;
				cbStr += _T("  ");
				cbStr += pSfm->description;
				m_pComboSFM->Append(cbStr);
			}
			break;
		}
	case UsfmAndPng:
		{
			for (iter = gpApp->m_pUsfmAndPngStylesMap->begin(); 
				iter != gpApp->m_pUsfmAndPngStylesMap->end(); ++iter)
			{
				// Retrieve each USFMAnalysis struct from the map
				key = iter->first;
				pSfm = iter->second;
				if (pSfm->marker.Find(_T('_')) == 0) // skip it if it is a marker beginning with _
					continue;
				cbStr = gSFescapechar;
				cbStr += pSfm->marker;
				cbStr += _T("  ");
				cbStr += pSfm->description;
				m_pComboSFM->Append(cbStr);
			}
			break;
		}
	default:
		{
			// this should never happen
			for (iter = gpApp->m_pUsfmStylesMap->begin(); 
				iter != gpApp->m_pUsfmStylesMap->end(); ++iter)
			{
				// Retrieve each USFMAnalysis struct from the map
				key = iter->first;
				pSfm = iter->second;
				if (pSfm->marker.Find(_T('_')) == 0) // skip it if it is a marker beginning with _
					continue;
				cbStr = gSFescapechar;
				cbStr += pSfm->marker;
				cbStr += _T("  ");
				cbStr += pSfm->description;
				m_pComboSFM->Append(cbStr);
			}
		}
	}

	// set the default button to Find Next button explicitly (otherwise, an MFC bug makes it
	// the Replace All button)
	wxASSERT(m_pFindNext != NULL);
	m_pFindNext->SetDefault(); // ID for Find Next button

	m_nCount = 0; // nothing matched yet

	pFindDlgSizer->Layout(); // force the sizers to resize the dialog
}
// initdialog here above

void CFindDlg::DoFindNext() 
{
	// this handles the wxID_OK special identifier assigned to the "Find Next" button
	CAdapt_ItView* pView = gpApp->GetView();
	pView->RemoveSelection();
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();

	// do nothing if past the end of the last srcPhrase, ie. at the EOF
	if (gpApp->m_nActiveSequNum == -1)
	{	
		wxASSERT(gpApp->m_pActivePile == 0);
		::wxBell();
		return;
	}

	int nKickOffSequNum = gpApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber;
	if (gbJustReplaced)
	{
		// do all the housekeeping tasks associated with a move, after a replace was done
		// if the targetBox is visible, store the contents in KB since we will advance the
		// active location potentially elsewhere
		// whm Note 12Aug08. The following should be OK even though in the wx version the
		// m_pTargetBox is never NULL (because of the IsShown() test which constrains the
		// logic to be the same as in the MFC version.
		if (gpApp->m_pTargetBox != NULL)
		{
			if (gpApp->m_pTargetBox->IsShown())
			{
				if (!gbIsGlossing)
				{
					pView->MakeLineFourString(gpApp->m_pActivePile->GetSrcPhrase(),
											gpApp->m_targetPhrase);
					pView->RemovePunctuation(pDoc,&gpApp->m_targetPhrase,from_target_text);
				}
                // the store will fail if the user edited the entry out of the KB, as the
                // latter cannot know which srcPhrases will be affected, so these will
                // still have their m_bHasKBEntry set true. We have to test for this, ie. a
                // null pRefString but the above flag TRUE is a sufficient test, and if so,
                // set the flag to FALSE
				CRefString* pRefStr;
				bool bOK;
				if (gbIsGlossing)
				{
					pRefStr = pView->GetRefString(gpApp->m_pGlossingKB, 1,
						gpApp->m_pActivePile->GetSrcPhrase()->m_key,gpApp->m_targetPhrase);
					if (pRefStr == NULL && 
						gpApp->m_pActivePile->GetSrcPhrase()->m_bHasGlossingKBEntry)
						gpApp->m_pActivePile->GetSrcPhrase()->m_bHasGlossingKBEntry = FALSE;
					bOK = pView->StoreText(gpApp->m_pGlossingKB,
								gpApp->m_pActivePile->GetSrcPhrase(),gpApp->m_targetPhrase);
				}
				else
				{
					pRefStr = pView->GetRefString(gpApp->m_pKB,
										gpApp->m_pActivePile->GetSrcPhrase()->m_nSrcWords,
										gpApp->m_pActivePile->GetSrcPhrase()->m_key,
										gpApp->m_targetPhrase);
					if (pRefStr == NULL && gpApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry)
						gpApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry = FALSE;
					bOK = pView->StoreText(gpApp->m_pKB,
								gpApp->m_pActivePile->GetSrcPhrase(),gpApp->m_targetPhrase);
				}
			}
		}

		// now we can get rid of the phrase box till wanted again
		// wx version just hides the phrase box
		gpApp->m_pTargetBox->Hide(); // MFC version calls DestroyWindow();
		gpApp->m_pTargetBox->ChangeValue(_T("")); // need to set it to null str since it 
											   // won't get recreated
		gpApp->m_targetPhrase = _T("");

        // did we just replace in a retranslation, if so, reduce the kick off sequ num by
        // one otherwise anything matchable in the first srcPhrase after the retranslation
        // will not get matched (ie. check if the end of a retranslation precedes the
        // current location and if so, we'll want to make that the place we move forward
        // from so that we actually try to match in the first pile after the retranslation)
        // We don't need the adjustment if glossing is ON however.
		int nEarlierSN = nKickOffSequNum -1;
		CPile* pPile = pView->GetPile(nEarlierSN);
		bool bRetrans = pPile->GetSrcPhrase()->m_bRetranslation;
		if ( !gbIsGlossing && bRetrans)
			nKickOffSequNum = nEarlierSN;

		gbJustReplaced = FALSE;
		m_nCount = 0; // nothing currently matched
	}

    // in some situations (eg. after a merge in a replacement) a LayoutStrip call is
    // needed, otherwise the destruction of the targetBox window will leave an empty white
    // space at the active loc.
#ifdef _NEW_LAYOUT
		gpApp->m_pLayout->RecalcLayout(gpApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
		gpApp->m_pLayout->RecalcLayout(gpApp->m_pSourcePhrases, create_strips_keep_piles);
#endif

	// restore the pointers which were clobbered
	CPile* pPile = pView->GetPile(gpApp->m_nActiveSequNum);
	gpApp->m_pActivePile = pPile;

	TransferDataFromWindow();

	int nAtSequNum = gpApp->m_nActiveSequNum;
	int nCount = 1;

	gbFindIsCurrent = TRUE; // turn it back off after the active location cell is drawn

	// do the Find Next operation
	bool bFound;
	bFound = pView->DoFindNext(nKickOffSequNum,
									m_bIncludePunct,
									m_bSpanSrcPhrases,
									m_bSpecialSearch,
									m_bSrcOnly,
									m_bTgtOnly,
									m_bSrcAndTgt,
									m_bFindRetranslation,
									m_bFindNullSrcPhrase,
									m_bFindSFM,
									m_srcStr,
									m_tgtStr,
									m_sfm,
									m_bIgnoreCase,
									nAtSequNum,
									nCount);

	gbJustReplaced = FALSE;

	if (bFound)
	{
		gbFound = TRUE; // set the global

		// inform the dialog about how many were matched
		m_nCount = nCount;

		// place the dialog window so as not to obscure things
		// work out where to place the dialog window
		m_nTwoLineDepth = 2 * gpApp->m_nTgtHeight;
		if (gbEnableGlossing)
		{
			if (gbGlossingUsesNavFont)
				m_nTwoLineDepth += gpApp->m_nNavTextHeight;
			else
				m_nTwoLineDepth += gpApp->m_nTgtHeight;
		}
		m_ptBoxTopLeft = gpApp->m_pActivePile->GetCell(1)->GetTopLeft();
		wxRect rectScreen;
		rectScreen = wxGetClientDisplayRect();

		wxClientDC dc(gpApp->GetMainFrame()->canvas);
		pView->canvas->DoPrepareDC(dc); // adjust origin
		gpApp->GetMainFrame()->PrepareDC(dc); // wxWidgets' drawing.cpp sample 
									// also calls PrepareDC on the owning frame

		if (!gbIsGlossing && gpApp->m_bMatchedRetranslation)
		{
			// use end of retranslation
			CCellList::Node* cpos = gpApp->m_selection.GetLast();
			CCell* pCell = (CCell*)cpos->GetData();
			wxASSERT(pCell != NULL);
			CPile* pPile = pCell->GetPile();
			pCell = pPile->GetCell(1); // last line
			m_ptBoxTopLeft = pCell->GetTopLeft();

			int newXPos,newYPos;
			// CalcScrolledPosition translates logical coordinates to device ones. 
			gpApp->GetMainFrame()->canvas->CalcScrolledPosition(m_ptBoxTopLeft.x,
												m_ptBoxTopLeft.y,&newXPos,&newYPos);
			m_ptBoxTopLeft.x = newXPos;
			m_ptBoxTopLeft.y = newYPos;
		}
		else
		{
			// use location where phrase box would be put
			int newXPos,newYPos;
			// CalcScrolledPosition translates logical coordinates to device ones. 
			gpApp->GetMainFrame()->canvas->CalcScrolledPosition(m_ptBoxTopLeft.x,
												m_ptBoxTopLeft.y,&newXPos,&newYPos);
			m_ptBoxTopLeft.x = newXPos;
			m_ptBoxTopLeft.y = newYPos;
		}
		gpApp->GetMainFrame()->canvas->ClientToScreen(&m_ptBoxTopLeft.x,
									&m_ptBoxTopLeft.y); // now it's screen coords
		int height = m_nTwoLineDepth;
		wxRect rectDlg;
		GetClientSize(&rectDlg.width, &rectDlg.height); // dialog's window
		rectDlg = NormalizeRect(rectDlg); // in case we ever change from MM_TEXT mode
		int dlgHeight = rectDlg.GetHeight();
		int dlgWidth = rectDlg.GetWidth();
		wxASSERT(dlgHeight > 0);
		int left = (rectScreen.GetWidth() - dlgWidth)/2;
		if (m_ptBoxTopLeft.y + height < rectScreen.GetBottom() - 50 - dlgHeight)
		{
			// put dlg near the bottom of screen
			SetSize(left,rectScreen.GetBottom()-dlgHeight-50,500,150,wxSIZE_USE_EXISTING);
		}
		else
		{
			// put dlg near the top of the screen
			SetSize(left,rectScreen.GetTop()+40,500,150,wxSIZE_USE_EXISTING);
		}
		Update();
		// In wx version we seem to need to scroll to the found location
		gpApp->GetMainFrame()->canvas->ScrollIntoView(nAtSequNum); // whm added 7Jun07
	}
	else
	{
		m_nCount = 0; // none matched
		gbFound = FALSE;
		pView->FindNextHasLanded(gpApp->m_nActiveSequNum,FALSE); // show old active location

		Update();
		::wxBell();
	}
}

void CFindDlg::DoRadioSrcOnly() 
{
	wxASSERT(m_pStaticSrcBoxLabel != NULL);
	m_pStaticSrcBoxLabel->Show(TRUE);
	
	wxASSERT(m_pEditSrc != NULL);
	m_pEditSrc->SetFocus();
	m_pEditSrc->SetSelection(-1,-1); // -1,-1 selects all
	m_pEditSrc->Show(TRUE);
	
	wxASSERT(m_pStaticTgtBoxLabel != NULL);
	m_pStaticTgtBoxLabel->Hide();
	
	wxASSERT(m_pEditTgt != NULL);
	m_pEditTgt->Hide();
	m_bSrcOnly = TRUE;
	m_bTgtOnly = FALSE;
	m_bSrcAndTgt = FALSE;
}

void CFindDlg::DoRadioTgtOnly() 
{
	wxASSERT(m_pStaticSrcBoxLabel != NULL);
	m_pStaticSrcBoxLabel->Hide();
	
	wxASSERT(m_pEditSrc != NULL);
	m_pEditSrc->Hide();
	
	wxASSERT(m_pStaticTgtBoxLabel != NULL);
	wxString str;
	str = _("        Translation:"); // str.Format(IDS_TRANS);
	m_pStaticTgtBoxLabel->SetLabel(str);
	m_pStaticTgtBoxLabel->Show(TRUE);
	
	wxASSERT(m_pEditTgt != NULL);
	m_pEditTgt->SetFocus();
	m_pEditTgt->SetSelection(-1,-1); // -1,-1 selects all
	m_pEditTgt->Show(TRUE);
	m_bSrcOnly = FALSE;
	m_bTgtOnly = TRUE;
	m_bSrcAndTgt = FALSE;
}

void CFindDlg::DoRadioSrcAndTgt() 
{
	wxASSERT(m_pStaticSrcBoxLabel != NULL);
	m_pStaticSrcBoxLabel->Show(TRUE);
	
	wxASSERT(m_pEditSrc != NULL);
	m_pEditSrc->Show(TRUE);
	
	wxASSERT(m_pStaticTgtBoxLabel != NULL);
	wxString str;
	str = _("With Translation:"); //IDS_WITH_TRANS
	m_pStaticTgtBoxLabel->SetLabel(str);
	m_pStaticTgtBoxLabel->Show(TRUE);
	
	wxASSERT(m_pEditTgt != NULL);
	m_pEditTgt->SetFocus();
	m_pEditTgt->SetSelection(-1,-1); // -1,-1 selects all
	m_pEditTgt->Show(TRUE);
	m_bSrcOnly = FALSE;
	m_bTgtOnly = FALSE;
	m_bSrcAndTgt = TRUE;	
}


void CFindDlg::OnSelchangeComboSfm(wxCommandEvent& WXUNUSED(event))
{
    // wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a
    // call to Clear() so we must check to see if the listbox contains no items and if so
    // return immediately
	if (m_pComboSFM->GetCount() == 0)
		return;

	TransferDataFromWindow();

	// get the new value of the combobox & convert to an sfm
	m_marker = m_pComboSFM->GetSelection();
	wxASSERT(m_marker != -1);
	m_markerStr = m_pComboSFM->GetStringSelection();
	wxASSERT(!m_markerStr.IsEmpty());
	m_sfm = SpanExcluding(m_markerStr, _T(" ")); // up to the first space character
	wxASSERT(m_sfm.Left(1) == gSFescapechar); // it must start with a backslash 
						// to be a valid sfm, or with sfm escape char set by user
}

void CFindDlg::OnButtonSpecial(wxCommandEvent& WXUNUSED(event)) 
{
	if (m_bSpecialSearch)
	{
		m_bSpecialSearch = FALSE; // toggle flag value, we are 
								  // returning to 'normal' search mode
		
		wxASSERT(m_pButtonSpecialNormal != NULL);
		wxString str;
		str = _("Special Search"); //IDS_SPECIAL_SEARCH
		m_pButtonSpecialNormal->SetLabel(str);
		
		wxASSERT(m_pFindRetranslation != NULL);
		m_pFindRetranslation->SetValue(TRUE);
		m_bFindRetranslation = FALSE;
		m_pFindRetranslation->Hide();
		
		wxASSERT(m_pFindPlaceholder != NULL);
		m_pFindPlaceholder->SetValue(FALSE);
		m_bFindNullSrcPhrase = FALSE;
		m_pFindPlaceholder->Hide();
		
		wxASSERT(m_pFindSFM != NULL);
		m_pFindSFM->SetValue(FALSE);
		m_bFindSFM = FALSE;
		m_pFindSFM->Hide();
		
		wxASSERT(m_pComboSFM != NULL);
		m_pComboSFM->Hide();
		
		wxASSERT(m_pSelectAnSfm != NULL);
		m_pSelectAnSfm->Hide();

		wxASSERT(m_pSpecialSearches != NULL);
		m_pSpecialSearches->Hide();

		// enable everything which is part of the normal search stuff
		wxASSERT(m_pCheckIgnoreCase != NULL);
		m_pCheckIgnoreCase->Enable(TRUE);
		
		wxASSERT(m_pRadioSrcTextOnly != NULL);
		m_pRadioSrcTextOnly->SetValue(TRUE);
		m_pRadioSrcTextOnly->Enable(TRUE);
		
		wxASSERT(m_pRadioTransTextOnly != NULL);
		m_pRadioTransTextOnly->SetValue(FALSE);
		m_pRadioTransTextOnly->Enable(TRUE);
		
		wxASSERT(m_pRadioBothSrcAndTransText != NULL);
		m_pRadioBothSrcAndTransText->SetValue(FALSE);
		m_pRadioBothSrcAndTransText->Enable(TRUE);
		
		wxASSERT(m_pStaticTgtBoxLabel != NULL);
		m_pStaticTgtBoxLabel->Hide();
		
		wxASSERT(m_pStaticSrcBoxLabel != NULL);
		m_pStaticSrcBoxLabel->Show(TRUE);

		wxASSERT(m_pEditTgt != NULL);
		m_pEditTgt->Hide();
		
		wxASSERT(m_pEditSrc != NULL);
		m_pEditSrc->Show(TRUE);
		
		wxASSERT(m_pCheckIncludePunct != NULL);
		m_pCheckIncludePunct->Show(TRUE);
		
		wxASSERT(m_pCheckSpanSrcPhrases != NULL);
		m_pCheckSpanSrcPhrases->Show(TRUE);
	}
	else
	{
		m_bSpecialSearch = TRUE; // toggle flag value, we are 
								 // entering 'special' mode
		
		wxASSERT(m_pButtonSpecialNormal != NULL);
		wxString str;
		str = _("Normal Search"); //IDS_NORMAL_SEARCH
		m_pButtonSpecialNormal->SetLabel(str);
		
		wxASSERT(m_pFindRetranslation != NULL);
		m_pFindRetranslation->SetValue(TRUE);
		m_bFindRetranslation = TRUE;
		m_pFindRetranslation->Enable(TRUE);
		m_pFindRetranslation->Show(TRUE);
		
		wxASSERT(m_pFindPlaceholder != NULL);
		m_pFindPlaceholder->SetValue(FALSE);
		m_bFindNullSrcPhrase = 0;
		m_pFindPlaceholder->Enable(TRUE);
		m_pFindPlaceholder->Show(TRUE);
		
		wxASSERT(m_pFindSFM != NULL);
		m_pFindSFM->SetValue(FALSE);
		m_bFindSFM = FALSE;
		m_pFindSFM->Enable(TRUE);
		m_pFindSFM->Show(TRUE);
		
		wxASSERT(m_pSpecialSearches != NULL);
		m_pSpecialSearches->Show(TRUE);
		
		wxASSERT(m_pComboSFM != NULL);
		m_pComboSFM->Show(TRUE);
		
		wxASSERT(m_pSelectAnSfm != NULL);
		m_pSelectAnSfm->Show(TRUE);

		// get the initial value of the combobox, as the default, then disable it
		wxASSERT(m_pComboSFM->GetCount() > 0);
		m_pComboSFM->SetSelection(0);
		m_markerStr = m_pComboSFM->GetStringSelection();
		wxASSERT(!m_markerStr.IsEmpty());
		m_sfm = SpanExcluding(m_markerStr,_T(" ")); // up to the first space character
		wxASSERT(m_sfm.Left(1) == gSFescapechar); // it must start with a backslash 
						// to be a valid sfm, or with the user set escape character
		// now disable the box until it's explicitly wanted
		m_pComboSFM->Enable(FALSE); // it should start off disabled

		// disable everything which is not part of the special search stuff
		wxASSERT(m_pCheckIgnoreCase != NULL);
		m_pCheckIgnoreCase->Enable(FALSE);
		
		wxASSERT(m_pRadioSrcTextOnly != NULL);
		m_pRadioSrcTextOnly->SetValue(TRUE);
		m_pRadioSrcTextOnly->Enable(FALSE);
		
		wxASSERT(m_pRadioTransTextOnly != NULL);
		m_pRadioTransTextOnly->SetValue(FALSE);
		m_pRadioTransTextOnly->Enable(FALSE);
		
		wxASSERT(m_pRadioBothSrcAndTransText != NULL);
		m_pRadioBothSrcAndTransText->SetValue(FALSE);
		m_pRadioBothSrcAndTransText->Enable(FALSE);
		
		wxASSERT(m_pStaticTgtBoxLabel != NULL);
		m_pStaticTgtBoxLabel->Hide();
		
		wxASSERT(m_pStaticSrcBoxLabel != NULL);
		m_pStaticSrcBoxLabel->Hide();

		wxASSERT(m_pEditTgt != NULL);
		m_pEditTgt->Hide();
		
		wxASSERT(m_pEditSrc != NULL);
		m_pEditSrc->Hide();
		
		wxASSERT(m_pCheckIncludePunct != NULL);
		m_pCheckIncludePunct->Hide();
		
		wxASSERT(m_pCheckSpanSrcPhrases != NULL);
		m_pCheckSpanSrcPhrases->Hide();
	}
	pFindDlgSizer->Layout(); // force the top dialog sizer to re-layout the dialog
}

void CFindDlg::OnRadioSfm(wxCommandEvent& event) 
{
	wxASSERT(m_pComboSFM != NULL);
	m_pComboSFM->Enable(TRUE);
	
	wxASSERT(m_pFindRetranslation != NULL);
	m_pFindRetranslation->SetValue(FALSE);
	m_bFindRetranslation = FALSE;
	
	wxASSERT(m_pFindPlaceholder != NULL);
	m_pFindPlaceholder->SetValue(FALSE);
	m_bFindNullSrcPhrase = FALSE;
	
	wxASSERT(m_pFindSFM != NULL);
	m_pFindSFM->SetValue(TRUE);
	m_bFindSFM = TRUE;

	// get initial value
	OnSelchangeComboSfm(event);
	TransferDataToWindow();
}
	
void CFindDlg::OnRadioRetranslation(wxCommandEvent& WXUNUSED(event)) 
{
	wxASSERT(m_pComboSFM != NULL);
	m_pComboSFM->Enable(FALSE);
	
	wxASSERT(m_pFindRetranslation != NULL);
	m_pFindRetranslation->SetValue(TRUE);
	m_bFindRetranslation = TRUE;
	
	wxASSERT(m_pFindPlaceholder != NULL);
	m_pFindPlaceholder->SetValue(FALSE);
	m_bFindNullSrcPhrase = FALSE;
	
	wxASSERT(m_pFindSFM != NULL);
	m_pFindSFM->SetValue(FALSE);
	m_bFindSFM = FALSE;
	TransferDataToWindow();
}

void CFindDlg::OnRadioNullSrcPhrase(wxCommandEvent& WXUNUSED(event)) 
{
	wxASSERT(m_pComboSFM != NULL);
	m_pComboSFM->Enable(FALSE);
	
	wxASSERT(m_pFindRetranslation != NULL);
	m_pFindRetranslation->SetValue(FALSE);
	m_bFindRetranslation = FALSE;
	
	wxASSERT(m_pFindPlaceholder != NULL);
	m_pFindPlaceholder->SetValue(TRUE);
	m_bFindNullSrcPhrase = TRUE;
	
	wxASSERT(m_pFindSFM != NULL);
	m_pFindSFM->SetValue(FALSE);
	m_bFindSFM = FALSE;
	TransferDataToWindow();
}

// BEW 3Aug09 removed unneeded FindNextHasLanded() call in OnCancel()
void CFindDlg::OnCancel(wxCommandEvent& WXUNUSED(event)) 
{
	wxASSERT(gpApp->m_pFindDlg != NULL);

	// destroying the window, but first clear the variables to defaults
	m_srcStr = _T("");
	m_tgtStr = _T("");
	m_replaceStr = _T("");
	m_marker = 0;
	m_markerStr = _T("");
	m_sfm = _T("");
	m_bFindRetranslation = FALSE;
	m_bFindNullSrcPhrase = FALSE;
	m_bFindSFM = FALSE;
	m_bSrcOnly = TRUE;
	m_bTgtOnly = FALSE;
	m_bSrcAndTgt = FALSE;
	m_bSpecialSearch = FALSE;
	m_bFindDlg = TRUE;

	TransferDataToWindow();
	Destroy();

	gbFindIsCurrent = FALSE;
	gbJustReplaced = FALSE; // clear to default value 

	// no selection, so find another way to define active location 
	// & place the phrase box
	int nCurSequNum = gpApp->m_nActiveSequNum;
	if (nCurSequNum == -1)
	{
		nCurSequNum = gpApp->GetMaxIndex(); // make active loc the last 
											// src phrase in the doc
		gpApp->m_nActiveSequNum = nCurSequNum;
	}
	else if (nCurSequNum >= 0 && nCurSequNum <= gpApp->GetMaxIndex())
	{
		gpApp->m_nActiveSequNum = nCurSequNum;
	}
	else
	{
		// if all else fails, go to the start
		gpApp->m_nActiveSequNum = 0;
	}
	gbJustCancelled = TRUE;
	gbFindOrReplaceCurrent = FALSE; // turn it back off

	gpApp->m_pFindDlg = (CFindDlg*)NULL;

	// clear the globals
	gpApp->m_bMatchedRetranslation = FALSE;
	gnRetransEndSequNum = -1;

	//	wxDialog::OnCancel(); // don't call base class because we are modeless
							  // - use this only if its modal
}

void CFindDlg::OnFindNext(wxCommandEvent& WXUNUSED(event))
{
	DoFindNext();
}

void CFindDlg::OnRadioSrcOnly(wxCommandEvent& WXUNUSED(event))
{
	DoRadioSrcOnly();
	pFindDlgSizer->Layout(); // force the top dialog sizer to re-layout the dialog
}

void CFindDlg::OnRadioTgtOnly(wxCommandEvent& WXUNUSED(event))
{
	DoRadioTgtOnly();
	pFindDlgSizer->Layout(); // force the top dialog sizer to re-layout the dialog
}

void CFindDlg::OnRadioSrcAndTgt(wxCommandEvent& WXUNUSED(event))
{
	DoRadioSrcAndTgt();
	pFindDlgSizer->Layout(); // force the top dialog sizer to re-layout the dialog
}



// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CReplaceDlg !!!!!!!!!!!!!!!!!!!!!!!!!!!

IMPLEMENT_DYNAMIC_CLASS( CReplaceDlg, wxDialog )

// event handler table
BEGIN_EVENT_TABLE(CReplaceDlg, wxDialog)
	EVT_INIT_DIALOG(CReplaceDlg::InitDialog) // not strictly necessary for 
											 // dialogs based on wxDialog
	EVT_BUTTON(wxID_CANCEL, CReplaceDlg::OnCancel)	
	
	EVT_BUTTON(IDC_REPLACE, CReplaceDlg::OnReplaceButton)
	EVT_BUTTON(IDC_REPLACE_ALL_BUTTON, CReplaceDlg::OnReplaceAllButton)
	EVT_BUTTON(wxID_OK, CReplaceDlg::OnFindNext)
	EVT_RADIOBUTTON(IDC_RADIO_SRC_ONLY_REPLACE, CReplaceDlg::OnRadioSrcOnly)
	EVT_RADIOBUTTON(IDC_RADIO_TGT_ONLY_REPLACE, CReplaceDlg::OnRadioTgtOnly)
	EVT_RADIOBUTTON(IDC_RADIO_SRC_AND_TGT_REPLACE, CReplaceDlg::OnRadioSrcAndTgt)
	EVT_CHECKBOX(IDC_CHECK_SPAN_SRC_PHRASES_REPLACE, CReplaceDlg::OnSpanCheckBoxChanged)
	EVT_UPDATE_UI(IDC_REPLACE_ALL_BUTTON, CReplaceDlg::UpdateReplaceAllButton)
END_EVENT_TABLE()

CReplaceDlg::CReplaceDlg()
{
}

CReplaceDlg::~CReplaceDlg()
{
}

CReplaceDlg::CReplaceDlg(wxWindow* parent) // dialog constructor
	: wxDialog(parent, -1, _("Find and Replace"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // This dialog function below is generated in wxDesigner, and defines the controls and
    // sizers for the dialog. The first parameter is the parent which should normally be
    // "this". The second and third parameters should both be TRUE to utilize the sizers
    // and create the right size dialog.
	pReplaceDlgSizer = ReplaceDlgFunc(this, TRUE, TRUE);
	// The declaration is: ReplaceDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);

	//m_marker = -1;
	m_srcStr = _T("");
	m_replaceStr = _T("");
	m_tgtStr = _T("");
	m_bIncludePunct = FALSE;
	m_bSpanSrcPhrases = FALSE;
	m_bIgnoreCase = FALSE;

	m_pRadioSrcTextOnly = (wxRadioButton*)FindWindowById(IDC_RADIO_SRC_ONLY_REPLACE);
	wxASSERT(m_pRadioSrcTextOnly != NULL);

	m_pRadioTransTextOnly = (wxRadioButton*)FindWindowById(IDC_RADIO_TGT_ONLY_REPLACE);
	wxASSERT(m_pRadioTransTextOnly != NULL);

	m_pRadioBothSrcAndTransText = (wxRadioButton*)FindWindowById(IDC_RADIO_SRC_AND_TGT_REPLACE);
	wxASSERT(m_pRadioBothSrcAndTransText != NULL);

	m_pCheckIgnoreCase = (wxCheckBox*)FindWindowById(IDC_CHECK_IGNORE_CASE_REPLACE);
	wxASSERT(m_pCheckIgnoreCase != NULL);
	m_pCheckIgnoreCase->SetValidator(wxGenericValidator(&m_bIgnoreCase)); // use validator

	m_pFindNext = (wxButton*)FindWindow(wxID_OK); // just use FindWindow here to 
												  // limit search to CReplaceDlg
	wxASSERT(m_pFindNext != NULL);

	m_pStaticSrcBoxLabel = (wxStaticText*)FindWindowById(IDC_STATIC_SRC_REPLACE);
	wxASSERT(m_pStaticSrcBoxLabel != NULL);

	m_pEditSrc = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC_REPLACE);
	wxASSERT(m_pEditSrc != NULL);
	m_pEditSrc->SetValidator(wxGenericValidator(&m_srcStr)); // use validator

	m_pStaticTgtBoxLabel = (wxStaticText*)FindWindowById(IDC_STATIC_TGT_REPLACE);
	wxASSERT(m_pStaticTgtBoxLabel != NULL);

	m_pEditTgt = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT_REPLACE);
	wxASSERT(m_pEditTgt != NULL);
	m_pEditTgt->SetValidator(wxGenericValidator(&m_tgtStr)); // use validator

	m_pButtonReplace = (wxButton*)FindWindowById(IDC_REPLACE);
	wxASSERT(m_pButtonReplace != NULL);
	
	m_pButtonReplaceAll = (wxButton*)FindWindowById(IDC_REPLACE_ALL_BUTTON);
	wxASSERT(m_pButtonReplaceAll != NULL);
	
	m_pCheckIncludePunct = (wxCheckBox*)FindWindowById(IDC_CHECK_INCLUDE_PUNCT_REPLACE);
	wxASSERT(m_pCheckIncludePunct != NULL);
	m_pCheckIncludePunct->SetValidator(wxGenericValidator(&m_bIncludePunct)); // use validator

	m_pCheckSpanSrcPhrases = (wxCheckBox*)FindWindowById(IDC_CHECK_SPAN_SRC_PHRASES_REPLACE);
	wxASSERT(m_pCheckSpanSrcPhrases != NULL);
	m_pCheckSpanSrcPhrases->SetValidator(wxGenericValidator(&m_bSpanSrcPhrases)); // use validator

	m_pEditReplace = (wxTextCtrl*)FindWindowById(IDC_EDIT_REPLACE);
	wxASSERT(m_pEditReplace != NULL);
	m_pEditReplace->SetValidator(wxGenericValidator(&m_replaceStr)); // use validator
	
}

void CReplaceDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	gbFound = FALSE;
	gbReplaceAllIsCurrent = FALSE;
	gbJustCancelled = FALSE;

	// make the font show user-chosen point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pEditSrc, NULL,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pEditSrc, NULL, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif


	// for glossing or adapting
	if (gbIsGlossing && gbGlossingUsesNavFont)
	{
		#ifdef _RTL_FLAGS
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, 
				m_pEditTgt, m_pEditReplace, NULL, NULL, gpApp->m_pDlgTgtFont, 
				gpApp->m_bNavTextRTL);
		#else // Regular version, only LTR scripts supported, 
			  // so use default FALSE for last parameter
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, 
				m_pEditTgt, m_pEditReplace, NULL, NULL, gpApp->m_pDlgTgtFont);
		#endif
	}
	else
	{
		#ifdef _RTL_FLAGS
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, 
				m_pEditTgt, m_pEditReplace, NULL, NULL, gpApp->m_pDlgTgtFont, 
				gpApp->m_bTgtRTL);
		#else // Regular version, only LTR scripts supported, 
			  // so use default FALSE for last parameter
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, 
				m_pEditTgt, m_pEditReplace, NULL, NULL, gpApp->m_pDlgTgtFont);
		#endif
	}

	// set default checkbox values
	m_bIncludePunct = FALSE;
	m_bSpanSrcPhrases = FALSE;

	// disable the search in src only radio button, it makes to sense to
	// replace if we only search source language text
	wxASSERT(m_pRadioSrcTextOnly != NULL);
	m_pRadioSrcTextOnly->SetValue(FALSE);
	m_pRadioSrcTextOnly->Enable(FALSE);


	// if glossing is ON, the "Search, retaining target text punctuation" and
	// "Allow the search to occur in the text spanning multiple piles" have to be
	// hidden; since merging is not permitted and only the gloss line can be accessed
	// as the 'target'
	if (gbIsGlossing)
	{
		wxASSERT(m_pCheckIncludePunct != NULL);
		m_pCheckIncludePunct->Hide();
		
		wxASSERT(m_pCheckSpanSrcPhrases != NULL);
		m_pCheckSpanSrcPhrases->Hide();
	}

	// get the normal defaults set up
	wxASSERT(m_pRadioTransTextOnly != NULL);
	m_pRadioTransTextOnly->SetValue(TRUE);
	
	wxASSERT(m_pEditSrc != NULL);
	m_pEditSrc->Hide();
	
	wxASSERT(m_pStaticSrcBoxLabel != NULL);
	m_pStaticSrcBoxLabel->Hide();
	
	wxASSERT(m_pStaticTgtBoxLabel != NULL);
	wxString str;
	str = _("        Translation:"); //IDS_TRANS
	m_pStaticTgtBoxLabel->SetLabel(str);
	m_pStaticTgtBoxLabel->Show(TRUE);
	
	wxASSERT(m_pEditTgt != NULL);
	m_pEditTgt->Show(TRUE);

    // set the default button to Find Next button explicitly (otherwise, an MFC bug makes
    // it the Replace All button)
	// whm note: The "Find Next" button is assigned the wxID_OK identifier
	wxASSERT(m_pFindNext != NULL);
	m_pFindNext->SetDefault(); // ID for Find Next button

	m_nCount = 0; // nothing matched yet

	pReplaceDlgSizer->Layout(); // force the sizers to resize the dialog
}

void CReplaceDlg::DoFindNext() 
{
	// this handles the wxID_OK special identifier assigned to the "Find Next" button
	CAdapt_ItView* pView = gpApp->GetView();
	pView->RemoveSelection();
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();

	// do nothing if past the end of the last srcPhrase, ie. at the EOF
	if (gpApp->m_nActiveSequNum == -1)
	{	
		wxASSERT(gpApp->m_pActivePile == 0);
		::wxBell();
		return;
	}

	int nKickOffSequNum = gpApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber;
	if (gbJustReplaced)
	{
		// do all the housekeeping tasks associated with a move, after a replace was done
		// if the targetBox is visible, store the contents in KB since we will advance the
		// active location potentially elsewhere
		if (gpApp->m_pTargetBox != NULL)
		{
			if (gpApp->m_pTargetBox->IsShown())
			{
				if (!gbIsGlossing)
				{
					pView->MakeLineFourString(gpApp->m_pActivePile->GetSrcPhrase(),
												gpApp->m_targetPhrase);
					pView->RemovePunctuation(pDoc,&gpApp->m_targetPhrase,from_target_text);
				}
                // the store will fail if the user edited the entry out of the KB, as the
                // latter cannot know which srcPhrases will be affected, so these will
                // still have their m_bHasKBEntry set true. We have to test for this, ie. a
                // null pRefString but the above flag TRUE is a sufficient test, and if so,
                // set the flag to FALSE
				CRefString* pRefStr;
				bool bOK;
				if (gbIsGlossing)
				{
					pRefStr = pView->GetRefString(gpApp->m_pGlossingKB, 1,
						gpApp->m_pActivePile->GetSrcPhrase()->m_key,gpApp->m_targetPhrase);
					if (pRefStr == NULL && 
						gpApp->m_pActivePile->GetSrcPhrase()->m_bHasGlossingKBEntry)
						gpApp->m_pActivePile->GetSrcPhrase()->m_bHasGlossingKBEntry = FALSE;
					bOK = pView->StoreText(gpApp->m_pGlossingKB,
								gpApp->m_pActivePile->GetSrcPhrase(),gpApp->m_targetPhrase);
				}
				else
				{
					pRefStr = pView->GetRefString(gpApp->m_pKB,
										gpApp->m_pActivePile->GetSrcPhrase()->m_nSrcWords,
										gpApp->m_pActivePile->GetSrcPhrase()->m_key,
										gpApp->m_targetPhrase);
					if (pRefStr == NULL && gpApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry)
						gpApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry = FALSE;
					bOK = pView->StoreText(gpApp->m_pKB,
								gpApp->m_pActivePile->GetSrcPhrase(),gpApp->m_targetPhrase);
				}
			}
		}

		// now we can get rid of the phrase box till wanted again
		// wx version just hides the phrase box
		gpApp->m_pTargetBox->Hide(); // MFC version calls DestroyWindow()
		gpApp->m_pTargetBox->ChangeValue(_T("")); // need to set it to null str since it 
											   // won't get recreated
		gpApp->m_targetPhrase = _T("");

        // did we just replace in a retranslation, if so, reduce the kick off sequ num by
        // one otherwise anything matchable in the first srcPhrase after the retranslation
        // will not get matched (ie. check if the end of a retranslation precedes the
        // current location and if so, we'll want to make that the place we move forward
        // from so that we actually try to match in the first pile after the retranslation)
        // We don't need the adjustment if glossing is ON however.
		int nEarlierSN = nKickOffSequNum -1;
		CPile* pPile = pView->GetPile(nEarlierSN);
		bool bRetrans = pPile->GetSrcPhrase()->m_bRetranslation;
		if ( !gbIsGlossing && bRetrans)
			nKickOffSequNum = nEarlierSN;

		gbJustReplaced = FALSE;
		m_nCount = 0; // nothing currently matched
	}

    // in some situations (eg. after a merge in a replacement) a LayoutStrip call is
    // needed, otherwise the destruction of the targetBox window will leave an empty white
    // space at the active loc.
#ifdef _NEW_LAYOUT
		gpApp->m_pLayout->RecalcLayout(gpApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
		gpApp->m_pLayout->RecalcLayout(gpApp->m_pSourcePhrases, create_strips_keep_piles);
#endif

	// restore the pointers which were clobbered
	CPile* pPile = pView->GetPile(gpApp->m_nActiveSequNum);
	gpApp->m_pActivePile = pPile;

	TransferDataFromWindow();

	int nAtSequNum = gpApp->m_nActiveSequNum;
	int nCount = 1;

	gbFindIsCurrent = TRUE; // turn it back off after the active location cell is drawn

	// do the Find Next operation
	bool bFound;
	bFound = pView->DoFindNext(nKickOffSequNum,
									m_bIncludePunct,
									m_bSpanSrcPhrases,
									FALSE, //m_bSpecialSearch, // not used in this case
									m_bSrcOnly,
									m_bTgtOnly,
									m_bSrcAndTgt,
									FALSE, //m_bFindRetranslation, // not used in this case
									FALSE, //m_bFindNullSrcPhrase, // not used in this case
									FALSE, //m_bFindSFM,// not used in this case
									m_srcStr,
									m_tgtStr,
									m_sfm,
									m_bIgnoreCase,
									nAtSequNum,
									nCount);

	gbJustReplaced = FALSE;

	if (bFound)
	{
		gbFound = TRUE; // set the global

		// enable the Replace and Replace All buttons
		wxASSERT(m_pButtonReplace != NULL);
		m_pButtonReplace->Enable(TRUE);
		
		wxASSERT(m_pButtonReplaceAll != NULL);
		m_pButtonReplaceAll->Enable(TRUE);

		// inform the dialog about how many were matched
		m_nCount = nCount;

		// place the dialog window so as not to obscure things
		// work out where to place the dialog window
		m_nTwoLineDepth = 2 * gpApp->m_nTgtHeight;
		if (gbEnableGlossing)
		{
			if (gbGlossingUsesNavFont)
				m_nTwoLineDepth += gpApp->m_nNavTextHeight;
			else
				m_nTwoLineDepth += gpApp->m_nTgtHeight;
		}
		m_ptBoxTopLeft = gpApp->m_pActivePile->GetCell(1)->GetTopLeft();
		wxRect rectScreen;
		rectScreen = wxGetClientDisplayRect();

		wxClientDC dc(gpApp->GetMainFrame()->canvas);
		pView->canvas->DoPrepareDC(dc); // adjust origin
		gpApp->GetMainFrame()->PrepareDC(dc); // wxWidgets' drawing.cpp sample also calls 
											  // PrepareDC on the owning frame
		if (!gbIsGlossing && gpApp->m_bMatchedRetranslation)
		{
			// use end of retranslation
			CCellList::Node* cpos = gpApp->m_selection.GetLast();
			CCell* pCell = (CCell*)cpos->GetData();
			wxASSERT(pCell != NULL); 
			CPile* pPile = pCell->GetPile();
			pCell = pPile->GetCell(1); // last line
			m_ptBoxTopLeft = pCell->GetTopLeft();

			int newXPos,newYPos;
			// CalcScrolledPosition translates logical coordinates to device ones. 
			gpApp->GetMainFrame()->canvas->CalcScrolledPosition(m_ptBoxTopLeft.x,
											m_ptBoxTopLeft.y,&newXPos,&newYPos);
			m_ptBoxTopLeft.x = newXPos;
			m_ptBoxTopLeft.y = newYPos;
		}
		else
		{
			// use location where phrase box would be put
			int newXPos,newYPos;
			// CalcScrolledPosition translates logical coordinates to device ones. 
			gpApp->GetMainFrame()->canvas->CalcScrolledPosition(m_ptBoxTopLeft.x,
											m_ptBoxTopLeft.y,&newXPos,&newYPos);
			m_ptBoxTopLeft.x = newXPos;
			m_ptBoxTopLeft.y = newYPos;
		}
		gpApp->GetMainFrame()->canvas->ClientToScreen(&m_ptBoxTopLeft.x,&m_ptBoxTopLeft.y);
		int height = m_nTwoLineDepth;
		wxRect rectDlg;
		GetClientSize(&rectDlg.width, &rectDlg.height);
		rectDlg = NormalizeRect(rectDlg); // in case we ever change from MM_TEXT mode
		int dlgHeight = rectDlg.GetHeight();
		int dlgWidth = rectDlg.GetWidth();
		wxASSERT(dlgHeight > 0);
		int left = (rectScreen.GetWidth() - dlgWidth)/2;
		if (m_ptBoxTopLeft.y + height < rectScreen.GetBottom() - 50 - dlgHeight)
		{
			// put dlg near the bottom of screen
			SetSize(left,rectScreen.GetBottom()-dlgHeight-50,500,150,wxSIZE_USE_EXISTING);
		}
		else
		{
			// put dlg near the top of the screen
			SetSize(left,rectScreen.GetTop()+40,500,150,wxSIZE_USE_EXISTING);
		}
		Update();
		// In wx version we seem to need to scroll to the found location
		gpApp->GetMainFrame()->canvas->ScrollIntoView(nAtSequNum); // whm added 7Jun07
	}
	else
	{
		m_nCount = 0; // none matched
		gbFound = FALSE;

		// disable the Replace and Replace All buttons
		wxASSERT(m_pButtonReplace != NULL);
		m_pButtonReplace->Enable(FALSE);
		
		wxASSERT(m_pButtonReplaceAll != NULL);
		m_pButtonReplaceAll->Enable(FALSE);
		pView->FindNextHasLanded(gpApp->m_nActiveSequNum,FALSE); // show old active location
		
		Update();
		::wxBell();
	}
}

void CReplaceDlg::OnSpanCheckBoxChanged(wxCommandEvent& WXUNUSED(event))
{
	bool bCheckboxValue = m_pCheckSpanSrcPhrases->GetValue();
	if (bCheckboxValue)
	{
		// the user has requested matching be attempted across multiple piles, so in order
		// to ensure the user can see and adjust what is done before the next Find Next is
		// done, we suppress the Replace All option by disabling the button for it,
		// because Replace All, in order to work, must do an OnFindNext() after each
		// OnReplace() has been done. Note: we don't place with the gbReplaceAllIsCurrent
		// flag, it is unnecessary to do so because it is set only in the Replace All
		// button's handler, so disabling the button suffices.
		m_bSpanSrcPhrases = TRUE;
		m_pCheckSpanSrcPhrases->SetValue(TRUE);
		m_pButtonReplaceAll->Enable(FALSE);
	}
	else
	{
		// the user has just unchecked the box for matching across piles, so re-enable the
		// Replace All button
		m_bSpanSrcPhrases = FALSE;
		m_pCheckSpanSrcPhrases->SetValue(FALSE);
		m_pButtonReplaceAll->Enable();
	}
}

void CReplaceDlg::UpdateReplaceAllButton(wxUpdateUIEvent& event)
{
	if (m_bSpanSrcPhrases)
	{
		// keep the Replace All button disabled
		event.Enable(FALSE);
	}
	else
	{
		// keep the Replace All button enabled
		event.Enable(TRUE);
	}
}

void CReplaceDlg::DoRadioSrcOnly() 
{
	wxASSERT(m_pStaticSrcBoxLabel != NULL);
	m_pStaticSrcBoxLabel->Show(TRUE);
	
	wxASSERT(m_pEditSrc != NULL);
	m_pEditSrc->SetFocus();
	m_pEditSrc->SetSelection(-1,-1); // -1,-1 selects all
	m_pEditSrc->Show(TRUE);
	
	wxASSERT(m_pStaticTgtBoxLabel != NULL);
	m_pStaticTgtBoxLabel->Hide();
	
	wxASSERT(m_pEditTgt != NULL);
	m_pEditTgt->Hide();
	m_bSrcOnly = TRUE;
	m_bTgtOnly = FALSE;
	m_bSrcAndTgt = FALSE;
}

void CReplaceDlg::DoRadioTgtOnly() 
{
	wxASSERT(m_pStaticSrcBoxLabel != NULL);
	m_pStaticSrcBoxLabel->Hide();
	
	wxASSERT(m_pEditSrc != NULL);
	m_pEditSrc->Hide();
	
	wxASSERT(m_pStaticTgtBoxLabel != NULL);
	wxString str;
	str = _("        Translation:"); //IDS_TRANS
	m_pStaticTgtBoxLabel->SetLabel(str);
	m_pStaticTgtBoxLabel->Show(TRUE);
	
	wxASSERT(m_pEditTgt != NULL);
	m_pEditTgt->SetFocus();
	m_pEditTgt->SetSelection(-1,-1); // -1,-1 selects all
	m_pEditTgt->Show(TRUE);
	m_bSrcOnly = FALSE;
	m_bTgtOnly = TRUE;
	m_bSrcAndTgt = FALSE;
}

void CReplaceDlg::DoRadioSrcAndTgt() 
{
	wxASSERT(m_pStaticSrcBoxLabel != NULL);
	m_pStaticSrcBoxLabel->Show(TRUE);
	
	wxASSERT(m_pEditSrc != NULL);
	m_pEditSrc->Show(TRUE);
	
	wxASSERT(m_pStaticTgtBoxLabel != NULL);
	wxString str;
	str = _("With Translation:"); //IDS_WITH_TRANS
	m_pStaticTgtBoxLabel->SetLabel(str);
	m_pStaticTgtBoxLabel->Show(TRUE);
	
	wxASSERT(m_pEditTgt != NULL);
	m_pEditTgt->SetFocus();
	m_pEditTgt->SetSelection(-1,-1); // -1,-1 selects all
	m_pEditTgt->Show(TRUE);
	m_bSrcOnly = FALSE;
	m_bTgtOnly = FALSE;
	m_bSrcAndTgt = TRUE;	
}

void CReplaceDlg::OnReplaceButton(wxCommandEvent& event) 
{
	TransferDataFromWindow();

	// do nothing if past the end of the last srcPhrase, ie. at the EOF
	if (gpApp->m_nActiveSequNum == -1)
	{	
		wxASSERT(gpApp->m_pActivePile == 0);
		::wxBell();
		return;
	}

	// check there is a selection
	m_bSelectionExists = TRUE;
	if (gpApp->m_selection.GetCount() == 0)
	{
		m_bSelectionExists = FALSE;
		// IDS_NO_SELECTION_YET
		wxMessageBox(_(
"Sorry, you cannot do a Replace operation because there is no selection yet."),
		_T(""), wxICON_INFORMATION);
		gbJustReplaced = FALSE;
		return;
	}

	// check that m_nCount value is same as number of cells selected in the view
	wxASSERT(m_nCount == (int)gpApp->m_selection.GetCount());
	
	bool bOK = gpApp->GetView()->DoReplace(gpApp->m_nActiveSequNum,m_bIncludePunct,
											m_tgtStr,m_replaceStr,m_nCount);

    // clear the global strings, in case they were set by OnButtonMerge() (we can safely
    // not clear them, since DoReplace clears them when first entered, but its best to play
    // safe
	gOldConcatStr = _T("");
	gOldConcatStrNoPunct = _T("");

	if (!bOK)
	{
		wxMessageBox(_(
		"Failed to do the replacement without error. The next Find Next will be skipped."),
		_T(""), wxICON_INFORMATION);
		return;
	}
	
    // let everyone know that a replace was just done (this will have put phrase box at the
    // active location in order to make the replacement, so the view state will not be the
    // same if OnCancel is called next, cf. if the latter was called after a Find which
    // matched something & user did not replace - in the latter case the phrase box will
    // have been destroyed and the view will only show a selection at the active loc.
	gbJustReplaced = TRUE;

    // BEW changed 17Jun09, because having an automatic OnFindNext() call after a
    // replacment means that the user gets no chance to see and verify that what has
    // happened is actually what he wanted to happen. We aren't dealing with connected
    // text, so the protocols for that scenario don't apply here; the discrete
    // CSourcePhrase break up of the original text, and the need to ensure that
    // associations between source & target going into the KB are valid associations,
    // requires that the user be able to make a visual check and press Find Next button
    // again only after he's satisfied and/or fixed it up to be as he wants after the
    // replacement has been made; however, a Replace All button press (we allow it only if
    // not matching across multiple piles) must be allowed to work - so we allow it in that
    // circumstance
    // 
	// a replace must be followed by an attempt to find next match, so do it (see comment
	// above)
	if (gbReplaceAllIsCurrent)
	{
		OnFindNext(event);
	}
}

void CReplaceDlg::OnReplaceAllButton(wxCommandEvent& event) 
{
	TransferDataFromWindow(); // TODO: check this!!!

	// do nothing if past the end of the last srcPhrase, ie. at the EOF
	if (gpApp->m_nActiveSequNum == -1)
	{	
		wxASSERT(gpApp->m_pActivePile == 0);
		::wxBell();
		return;
	}

	if (gpApp->m_selection.GetCount() == 0)
	{
		// no selection yet, so do a Find Next
		OnFindNext(event);
		if (!gbFound)
		{
			// return, nothing was matched
			::wxBell();
			return;
		}
	}

    // if we get here then we have a selection, and the OnIdle() handler can now start
    // doing the replacements following by finds, till no match happens or eof reached
	Hide();
	gbReplaceAllIsCurrent = TRUE;
	
	pReplaceDlgSizer->Layout(); // force the top dialog sizer to re-layout the dialog
}

bool CReplaceDlg::OnePassReplace()
{
	wxCommandEvent event;
	OnReplaceButton(event); // includes an OnFindNext() call after the DoReplace()

	if (gbFound)
		return TRUE;
	else
		return FALSE;
}

void CReplaceDlg::OnCancel(wxCommandEvent& WXUNUSED(event)) 
{
	CAdapt_ItView* pView = gpApp->GetView();
	wxASSERT(gpApp->m_pReplaceDlg != NULL);

	// clear the globals
	gpApp->m_bMatchedRetranslation = FALSE;
	gnRetransEndSequNum = -1;

	// destroying the window, but first clear the variables to defaults
	m_srcStr = _T("");
	m_tgtStr = _T("");
	m_replaceStr = _T("");

	m_markerStr = _T("");
	m_sfm = _T("");

	m_bSrcOnly = TRUE;
	m_bTgtOnly = FALSE;
	m_bSrcAndTgt = FALSE;

	m_bFindDlg = FALSE; // it is TRUE in the CFindDlg

	TransferDataToWindow();
	Destroy();

	gbFindIsCurrent = FALSE;

	if (gbJustReplaced)
	{
        // we have cancelled just after a replacement, so we expect the phrase box to exist
        // and be visible, so we only have to do a little tidying up before we return whm
        // 12Aug08 Note: In the wx version m_pTargetBox is never NULL. Therefore, I think
        // we should remove the == NULL check below and always jump to the "longer" cleanup
        // in the else block below.
		//if (gpApp->m_pTargetBox == NULL)
		// whm Note: moved the gbJustReplaced = FALSE statement here 12Aug08 because in
		// its placement below, it would never be reset to FALSE. The logic is not real
		// clear, however, because if in the MFC version the target box were NULL, the
		// goto a jump would be made without resetting the boolean (which seems to me
		// to be faulty logic somewhere).
		gbJustReplaced = FALSE; // clear to default value 
		goto a; // check, just in case, and do the longer cleanup if the box is not there
	}
	else
	{
        // we have tried a FindNext since the previous replacement, so we expect the phrase
        // box to have been destroyed by the time we enter this code block; so place the
        // phrase box, if it has been destroyed
		// wx version the phrase box always exists
        // whm 11Aug08 - here we need to remove the m_pTargetBox == NULL test, because in
        // the MFC version we "expect the phrase box to have been destroyed by the time we
        // enter this code block". But we don't destroy the phrase box in the wx version so
        // we should always execute the following block.
		//if (gpApp->m_pTargetBox == NULL)
		//{
			// in wx version this block should never execute
a:			CCell* pCell = 0;
			CPile* pPile = 0;
			if (!gpApp->m_selection.IsEmpty())
			{
				CCellList::Node* cpos = gpApp->m_selection.GetFirst();
				pCell = cpos->GetData(); // could be on any line
				wxASSERT(pCell != NULL);
				pPile = pCell->GetPile();
			}
			else
			{
				// no selection, so find another way to define active location 
				// & place the phrase box
				int nCurSequNum = gpApp->m_nActiveSequNum;
				if (nCurSequNum == -1)
				{
					nCurSequNum = gpApp->GetMaxIndex(); // make active loc the 
														// last src phrase in the doc
					gpApp->m_nActiveSequNum = nCurSequNum;
				}
				else if (nCurSequNum >= 0 && nCurSequNum <= gpApp->GetMaxIndex())
				{
					gpApp->m_nActiveSequNum = nCurSequNum;
				}
				else
				{
					// if all else fails, go to the start
					gpApp->m_nActiveSequNum = 0;
				}
				pPile = pView->GetPile(gpApp->m_nActiveSequNum);
			}

			// preserve enough information to be able to recreate the appropriate selection
			// since placing the phrase box will clobber the current selection
			CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
			int nCount = 0;
			if (!gpApp->m_selection.IsEmpty())
			{
				nCount = gpApp->m_selection.GetCount();
				wxASSERT(nCount >0);
			}
			int nSaveSelSequNum = pSrcPhrase->m_nSequNumber; // if in a retrans, 
                                // selection will not be where phrase box ends up

            // pPile is what we will use for the active pile, so set everything up there,
            // provided it is not in a retranslation - if it is, place the box preceding
            // it, if possible; but if glossing is ON, we can have the box within a
            // retranslation in which case ignore the block of code
			CPile* pSavePile = pPile;
			if (!gbIsGlossing)
			{
				while (pSrcPhrase->m_bRetranslation)
				{
					pPile = pView->GetPrevPile(pPile);
					if (pPile == NULL)
					{
						// if we get to the start, try again, going the other way
						pPile = pSavePile;
						while (pSrcPhrase->m_bRetranslation)
						{
							pPile = pView->GetNextPile(pPile);
							wxASSERT(pPile != NULL); // we'll assume this will never fail
							pSrcPhrase = pPile->GetSrcPhrase();
						}
						break;
					}
					pSrcPhrase = pPile->GetSrcPhrase();
				}
			}
			pSrcPhrase = pPile->GetSrcPhrase();
			gpApp->m_nActiveSequNum = pSrcPhrase->m_nSequNumber;
			gpApp->m_pActivePile = pPile;
			pCell = pPile->GetCell(1); // we want the 2nd line, for phrase box

			// scroll into view, just in case (but shouldn't be needed)
			gpApp->GetMainFrame()->canvas->ScrollIntoView(gpApp->m_nActiveSequNum);

			// place the phrase box
			gbJustCancelled = TRUE;
			pView->PlacePhraseBox(pCell,2);

			// get a new active pile pointer, the PlacePhraseBox call did a recal of the layout
			gpApp->m_pActivePile = pView->GetPile(gpApp->m_nActiveSequNum);
			wxASSERT(gpApp->m_pActivePile != NULL);

			// get a new pointer to the pile at the start of the selection, since the recalc
			// also clobbered the old one
			CPile* pSelPile = pView->GetPile(nSaveSelSequNum);
			wxASSERT(pSelPile != NULL);

			pView->Invalidate(); // get window redrawn
			gpApp->m_pLayout->PlaceBox();

			// restore focus to the targetBox
			if (gpApp->m_pTargetBox != NULL)
			{
				if (gpApp->m_pTargetBox->IsShown())
				{
					gpApp->m_pTargetBox->SetSelection(-1,-1); // -1,-1 selects all
					gpApp->m_pTargetBox->SetFocus();
				}
			}

			// recreate the selection to be in line 1, hence ignoring boundary flags
			CCell* pAnchorCell = pSelPile->GetCell(0); // first line, index 0 cell
			if (nCount > 0)
			{
				gpApp->m_pAnchor = pAnchorCell;
				CCellList* pSelection = &gpApp->m_selection;
				wxASSERT(pSelection->IsEmpty());
				gpApp->m_selectionLine = 0;
				wxClientDC aDC(gpApp->GetMainFrame()->canvas); // make a device context

				// then do the new selection, start with the anchor cell

				aDC.SetBackgroundMode(gpApp->m_backgroundMode);
				aDC.SetTextBackground(wxColour(255,255,0)); // yellow
				pAnchorCell->DrawCell(&aDC, gpApp->m_pLayout->GetSrcColor());
				gpApp->m_bSelectByArrowKey = FALSE;
				pAnchorCell->SetSelected(TRUE);

				// preserve record of the selection
				pSelection->Append(pAnchorCell);

				// extend the selection to the right, if more than one pile is involved
				if (nCount > 1)
				{
					// extend the selection (shouldn't be called when glossing is ON
					// because we inhibit matching across piles in that circumstance)
					pView->ExtendSelectionForFind(pAnchorCell,nCount);
				}
			}
	}

	gbFindOrReplaceCurrent = FALSE; // turn it back off

	gpApp->m_pReplaceDlg = (CReplaceDlg*)NULL;

	//	wxDialog::OnCancel(); //don't call base class because we 
	//	                      // are modeless - use this only if its modal
}

void CReplaceDlg::OnFindNext(wxCommandEvent& WXUNUSED(event))
{
	DoFindNext();
}

void CReplaceDlg::OnRadioSrcOnly(wxCommandEvent& WXUNUSED(event))
{
	DoRadioSrcOnly();
	pReplaceDlgSizer->Layout(); // force the top dialog sizer to re-layout the dialog
}

void CReplaceDlg::OnRadioTgtOnly(wxCommandEvent& WXUNUSED(event))
{
	DoRadioTgtOnly();
	pReplaceDlgSizer->Layout(); // force the top dialog sizer to re-layout the dialog
}

void CReplaceDlg::OnRadioSrcAndTgt(wxCommandEvent& WXUNUSED(event))
{
	DoRadioSrcAndTgt();
	pReplaceDlgSizer->Layout(); // force the top dialog sizer to re-layout the dialog
}


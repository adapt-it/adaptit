/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			EarlierTranslationDlg.cpp
/// \author			Bill Martin
/// \date_created	23 June 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CEarlierTranslationDlg class. 
/// The CEarlierTranslationDlg class allows the user to view an earlier translation made
/// within the same document (choosing its location by reference), and optionally jump 
/// there if desired.
/// The CEarlierTranslationDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// \derivation		The CEarlierTranslationDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in EarlierTranslationDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "EarlierTranslationDlg.h"
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
#include "EarlierTranslationDlg.h"
#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"
#include "SourcePhrase.h"
#include "Pile.h"
#include "MainFrm.h"			// These 2 #includes are needed on the Mac
#include "Adapt_ItCanvas.h"
#if defined(__WXGTK__)
#include "MainFrm.h"
#include "Adapt_ItCanvas.h"
#endif

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

// This global is defined in Adapt_ItView.cpp.
//extern bool	gbGlossingVisible; // TRUE makes Adapt It revert to Shoebox functionality only

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

int			gnLastEarlierChapter = 1; // preserves last user choice, so it can be the default on
int			gnLastEarlierVerse = 1;	  // the next entry to the dialog

/// This global is defined in Adapt_ItView.cpp.
extern int	gnOldSequNum;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// This global is defined in Adapt_ItView.cpp.
extern bool gbVerticalEditInProgress;

// This global is defined in Adapt_It.cpp.
//extern bool	gbRTL_Layout;	// ANSI version is always left to right reading; this flag can only
							// be changed in the NRoman version, using the extra Layout menu

// event handler table
BEGIN_EVENT_TABLE(CEarlierTranslationDlg, AIModalDialog)
	EVT_INIT_DIALOG(CEarlierTranslationDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CEarlierTranslationDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, CEarlierTranslationDlg::OnCancel)
	EVT_CLOSE(CEarlierTranslationDlg::OnClose)
	//EVT_KEY_DOWN(CEarlierTranslationDlg::OnKeyDown) // this doesn't catch an ESC key press
	EVT_BUTTON(IDC_GET_CHVERSE_TEXT, CEarlierTranslationDlg::OnGetChapterVerseText)
	EVT_BUTTON(IDC_CLOSE_AND_JUMP, CEarlierTranslationDlg::OnCloseAndJump)
	EVT_BUTTON(IDC_SHOW_MORE, CEarlierTranslationDlg::OnShowMoreContext)
	EVT_BUTTON(IDC_SHOW_LESS, CEarlierTranslationDlg::OnShowLessContext)
END_EVENT_TABLE()


CEarlierTranslationDlg::CEarlierTranslationDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("View Translation or Glosses Elsewhere In The Document"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) // &~ wxCLOSE_BOX)
{
	m_srcText = _T("");
	m_tgtText = _T("");
	m_nChapter = 0;
	m_nVerse = 0;
	m_strBeginChVerse = _T("");
	m_strEndChVerse = _T("");
	m_nExpansionIndex = -1;

	m_pView = gpApp->GetView();

	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pEarlierTransSizer = EarlierTransDlgFunc(this, TRUE, TRUE);
	// The declaration is: EarlierTransDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	// Get pointers to controls and set validators
	m_pSrcTextBox = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC_TEXT);
	wxASSERT(m_pSrcTextBox != NULL);
	//m_pSrcTextBox->SetValidator(wxGenericValidator(&m_srcText));

	m_pTgtTextBox = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT_TEXT);
	wxASSERT(m_pTgtTextBox != NULL);
	//m_pTgtTextBox->SetValidator(wxGenericValidator(&m_tgtText));

	m_pChapterSpinCtrl = (wxSpinCtrl*)FindWindowById(IDC_SPIN_CHAPTER);
	wxASSERT(m_pChapterSpinCtrl != NULL);
	//m_pChapterSpinCtrl->SetValidator(wxGenericValidator(&m_nChapter));

	m_pVerseSpinCtrl = (wxSpinCtrl*)FindWindowById(IDC_SPIN_VERSE);
	wxASSERT(m_pVerseSpinCtrl != NULL);
	//m_pVerseSpinCtrl->SetValidator(wxGenericValidator(&m_nVerse));

	m_pBeginChVerseStaticText = (wxStaticText*)FindWindowById(IDC_STATIC_BEGIN);
	wxASSERT(m_pBeginChVerseStaticText != NULL);
	//m_pBeginChVerseStaticText->SetValidator(wxGenericValidator(&m_strBeginChVerse));

	m_pEndChVerseStaticText = (wxStaticText*)FindWindowById(IDC_STATIC_END);
	wxASSERT(m_pEndChVerseStaticText != NULL);
	//m_pEndChVerseStaticText->SetValidator(wxGenericValidator(&m_strEndChVerse));
}

CEarlierTranslationDlg::~CEarlierTranslationDlg() // destructor
{
	;
}

void CEarlierTranslationDlg::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	// Note: CEarlierTranslationDlg is modeless. When the user clicks on the close X box in upper
	// right hand corner of the dialog, the wx design does NOT call this handler, but instead calls
	// the OnOK() handler (see below)

	// We must call destroy because the dialog is modeless.
	Destroy();
	//delete gpApp->m_pEarlierTransDlg; // BEW added 19Nov05, to prevent memory leak // No, this is harmful in wx!!!
	gpApp->m_pEarlierTransDlg = NULL;
	//wxDialog::OnCancel(event); // we are running modeless so don't call the base class method
}

void CEarlierTranslationDlg::OnClose(wxCloseEvent& WXUNUSED(event))
{	
	// This OnClose handler does not get called when user presses the esc key
	// TODO: Implement a different way to capture esc key closing of the dialog window so we can call Destroy. 
	Destroy();
	//delete gpApp->m_pEarlierTransDlg; // BEW added 19Nov05, to prevent memory leak // No, this is harmful in wx!!!
	gpApp->m_pEarlierTransDlg = NULL;
	//wxDialog::OnCancel(event); // we are running modeless so don't call the base class method
}

void CEarlierTranslationDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	
	//// make the spin buttons work properly
	m_pChapterSpinCtrl->SetRange(0,150);
	m_pVerseSpinCtrl->SetRange(1,2000); // allow up to 2,000; for greater values, type directly in the box

	// initialize to safe values ie. 1:1
	m_nChapter = gnLastEarlierChapter;
	m_nVerse = gnLastEarlierVerse;

	// first, use the current source and target language fonts for the 
	// edit boxes (in case there are special characters); or if glossing is on and
	// navText font is used for rendering, then set up that instead for the 'target'

	// make the fonts show user's desired point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pSrcTextBox, NULL,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pSrcTextBox, NULL, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif

	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pTgtTextBox, NULL,
								NULL, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bTgtRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pTgtTextBox, NULL, 
								NULL, NULL, gpApp->m_pDlgTgtFont);
	#endif

	if (gbIsGlossing && gbGlossingUsesNavFont)
	{
		// redo it, the navText direction might be different from tgt direction
		#ifdef _RTL_FLAGS
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, m_pTgtTextBox, NULL,
									NULL, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bNavTextRTL);
		#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pTgtTextBox, NULL, 
									NULL, NULL, gpApp->m_pDlgTgtFont);
		#endif
	}


	EnableLessButton(FALSE);
	EnableMoreButton(FALSE);
	EnableJumpButton(FALSE);

	// clear the expansion arrays
	for (int i=0; i<10; ++i)
	{
		m_preContext[i] = -1;
		m_follContext[i] = -1;
	}
	m_nExpansionIndex = -1;

	//TransferDataToWindow(); // whm removed 21Nov11
	m_pSrcTextBox->ChangeValue(m_srcText); // whm added 21Nov11
	m_pTgtTextBox->ChangeValue(m_tgtText); // whm added 21Nov11
	m_pChapterSpinCtrl->SetValue(m_nChapter); // whm added 21Nov11
	m_pVerseSpinCtrl->SetValue(m_nVerse); // whm added 21Nov11
	m_pBeginChVerseStaticText->SetLabel(m_strBeginChVerse); // whm added 21Nov11
	m_pEndChVerseStaticText->SetLabel(m_strEndChVerse); // whm added 21Nov11


	// when in vertical edit mode, don't permit jumping because it may jump the user
	// out of the editable span into the gray text area
	if (gbVerticalEditInProgress)
	{
		wxButton* pBtn = (wxButton*)FindWindowById(IDC_CLOSE_AND_JUMP);
		pBtn->Show(FALSE);
	}
}

bool CEarlierTranslationDlg::IsMarkedForVerse(CSourcePhrase* pSrcPhrase)
{
	wxArrayString* pList = pSrcPhrase->m_pMedialMarkers;

	if (pSrcPhrase->m_markers.IsEmpty())
	{
		// there are no word or phrase initial markers, so check for medial ones
		int count = pList->GetCount();
		if (count == 0)
		{
			// no medial markers either
			return FALSE;
		}
		else
		{
			// there are medial markers - so check if any of them are a verse marker
			wxString str;
			for ( int n = 0; n < count; n++ )
			{
				str = pList->Item(n);
				if (str.Find(_T("\\v")) >= 0)
				{
					// we have found a verse marker
					return TRUE;
				}
			}
		}
		return FALSE;
	}
	else
	{
		// contains word or phrase initial markers
		if (pSrcPhrase->m_markers.Find(_T("\\v")) < 0)
		{
			// no verse marker in the string
			return FALSE;
		}
		else
		{
			// we have come to a source phrase marked for a verse
			return TRUE;
		}
	}
}

void CEarlierTranslationDlg::ScanVerse(SPList::Node*& pos, CSourcePhrase* pSrcPhrase, SPList* WXUNUSED(pList))
{
	// we've found the start of the verse, so extract its source and target text strings
	m_strBeginChVerse = pSrcPhrase->m_chapterVerse;
	m_srcText = m_strBeginChVerse + _T(" ") + pSrcPhrase->m_srcPhrase;
	if (gbIsGlossing)
	{
		m_tgtText = m_strBeginChVerse + _T(" ") + pSrcPhrase->m_gloss;
	}
	else // adapting
	{
		m_tgtText = m_strBeginChVerse + _T(" ") + pSrcPhrase->m_targetStr;
	}
	m_nFirstSequNumBasic = pSrcPhrase->m_nSequNumber;
	m_nCurLastSequNum = pSrcPhrase->m_nSequNumber; // a safe default, until set again below

	// accumulate the rest until the end of the verse
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		if (pSrcPhrase->m_markers.IsEmpty() && pSrcPhrase->m_pMedialMarkers->GetCount() == 0)
		{
			// no possibility of a new verse starting, so accumulate this one's strings
			m_srcText += _T(" ") + pSrcPhrase->m_srcPhrase; // source is never null
			if (gbIsGlossing)
			{
				if (!pSrcPhrase->m_gloss.IsEmpty())
					m_tgtText += _T(" ") + pSrcPhrase->m_gloss; // get target text if not null
			}
			else // adapting
			{
				if (!pSrcPhrase->m_targetStr.IsEmpty())
					m_tgtText += _T(" ") + pSrcPhrase->m_targetStr; // get target text if not null
			}
			m_nLastSequNumBasic = pSrcPhrase->m_nSequNumber;
			m_nCurLastSequNum = pSrcPhrase->m_nSequNumber;
		}
		else
		{
			// there are markers, so check - it could be the start of the next verse
			if (IsMarkedForVerse(pSrcPhrase))
			{
				// it is the start of the next verse, so end accumulation
				break;
			}
			else
			{
				// accumulate this one's strings too
				m_srcText += _T(" ") + pSrcPhrase->m_srcPhrase; // source is never null
				if (gbIsGlossing)
				{
					if (!pSrcPhrase->m_gloss.IsEmpty())
						m_tgtText += _T(" ") + pSrcPhrase->m_gloss; // get target text if not null
				}
				else // adapting
				{
					if (!pSrcPhrase->m_targetStr.IsEmpty())
						m_tgtText += _T(" ") + pSrcPhrase->m_targetStr; // get target text if not null
				}
				m_nLastSequNumBasic = pSrcPhrase->m_nSequNumber;
				m_nCurLastSequNum = pSrcPhrase->m_nSequNumber;
			}
		}
	}
	m_strEndChVerse = m_strBeginChVerse;
}

void CEarlierTranslationDlg::EnableMoreButton(bool bEnableFlag)
{
	wxButton* pMoreBtn = (wxButton*)FindWindowById(IDC_SHOW_MORE);
	wxASSERT(pMoreBtn);
	if (bEnableFlag)
		pMoreBtn->Enable(TRUE);
	else
		pMoreBtn->Enable(FALSE);
}

void CEarlierTranslationDlg::EnableLessButton(bool bEnableFlag)
{
	wxButton* pLessBtn = (wxButton*)FindWindowById(IDC_SHOW_LESS);
	wxASSERT(pLessBtn);
	if (bEnableFlag)
		pLessBtn->Enable(TRUE);
	else
		pLessBtn->Enable(FALSE);
}

void CEarlierTranslationDlg::EnableJumpButton(bool bEnableFlag)
{
	wxButton* pJumpBtn = (wxButton*)FindWindowById(IDC_CLOSE_AND_JUMP);
	wxASSERT(pJumpBtn);
	if (bEnableFlag)
		pJumpBtn->Enable(TRUE);
	else
		pJumpBtn->Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CEarlierTranslationDlg message handlers

void CEarlierTranslationDlg::OnGetChapterVerseText(wxCommandEvent& WXUNUSED(event)) 
{
	//TransferDataFromWindow(); // whm removed 21Nov11
	m_srcText = m_pSrcTextBox->GetValue(); // whm added 21Nov11
	m_tgtText = m_pTgtTextBox->GetValue(); // whm added 21Nov11
	m_nChapter = m_pChapterSpinCtrl->GetValue(); // whm added 21Nov11
	m_nVerse = m_pVerseSpinCtrl->GetValue(); // whm added 21Nov11
	m_strBeginChVerse = m_pBeginChVerseStaticText->GetLabel(); // whm added 21Nov11
	m_strEndChVerse = m_pEndChVerseStaticText->GetLabel(); // whm added 21Nov11

	gnLastEarlierChapter = m_nChapter;
	gnLastEarlierVerse = m_nVerse;

	// clear the expansion arrays
	for (int i=0; i<10; ++i)
	{
		m_preContext[i] = -1;
		m_follContext[i] = -1;
	}
	m_nExpansionIndex = -1;

	wxString str1;
	str1 << m_pChapterSpinCtrl->GetValue(); // GetValue returns int and << converts it to string

	wxString str2;
	str2 << m_pVerseSpinCtrl->GetValue(); // GetValue returns int and << converts it to string

	m_chapterVerse = str1 + _T(":") + str2;
	m_verse = str2;

	int nWantedChapter = m_nChapter;
	int nWantedVerse = m_nVerse;
	m_nCurPrecChapter = m_nChapter;
	m_nCurPrecVerse = m_nVerse;
	m_nCurFollChapter = m_nChapter;
	m_nCurFollVerse = m_nVerse;
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	SPList* pList = pApp->m_pSourcePhrases;
	wxASSERT(pList);
	if (pList->IsEmpty())
	{
		::wxBell();
		return;
	}
	SPList::Node* pos;

	// find the nominated chapter and verse, if possible, using the wxString for chapt:verse;
	// if it fails, assume a range & try again with integers

	wxString str;
	if (m_nChapter == 0)
	{
		// special case, either its non-scripture, or a chapterless book like 2John
		pos = pList->GetFirst();
		wxASSERT(pos != NULL);

		// first, assume it's a chapterless book like 2 John, try find the verse
		while (pos != NULL)
		{
			CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();
			wxASSERT(pSrcPhrase);
			if (pSrcPhrase->m_chapterVerse == m_verse)
			{
				ScanVerse(pos,pSrcPhrase,pList);
				EnableLessButton(FALSE);
				EnableMoreButton(TRUE);
				EnableJumpButton(TRUE);
				// show the text strings in the dialog's edit boxes
				//TransferDataToWindow(); // whm removed 21Nov11
				m_pSrcTextBox->ChangeValue(m_srcText); // whm added 21Nov11
				m_pTgtTextBox->ChangeValue(m_tgtText); // whm added 21Nov11
				m_pChapterSpinCtrl->SetValue(m_nChapter); // whm added 21Nov11
				m_pVerseSpinCtrl->SetValue(m_nVerse); // whm added 21Nov11
				m_pBeginChVerseStaticText->SetLabel(m_strBeginChVerse); // whm added 21Nov11
				m_pEndChVerseStaticText->SetLabel(m_strEndChVerse); // whm added 21Nov11
				return;
			}
		}

		// didn't find the verse, so tell the user
		// IDS_NO_SUCH_CHVERSE
		wxMessageBox(_("Sorry, the application was not able to find the verse you specified."),_T(""), wxICON_INFORMATION | wxOK);
		m_srcText.Empty();
		m_tgtText.Empty();
		EnableLessButton(FALSE);
		EnableMoreButton(FALSE);
		EnableJumpButton(FALSE);
		//TransferDataToWindow(); // whm removed 21Nov11
		m_pSrcTextBox->ChangeValue(m_srcText); // whm added 21Nov11
		m_pTgtTextBox->ChangeValue(m_tgtText); // whm added 21Nov11
		m_pChapterSpinCtrl->SetValue(m_nChapter); // whm added 21Nov11
		m_pVerseSpinCtrl->SetValue(m_nVerse); // whm added 21Nov11
		m_pBeginChVerseStaticText->SetLabel(m_strBeginChVerse); // whm added 21Nov11
		m_pEndChVerseStaticText->SetLabel(m_strEndChVerse); // whm added 21Nov11
		return;
	}
	else
	{
		// there are chapter markers, so look for chapter & verse
		pos = pList->GetFirst();
		wxASSERT(pos != NULL);
		while (pos != NULL)
		{
			CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();
			wxASSERT(pSrcPhrase);
			if (pSrcPhrase->m_chapterVerse == m_chapterVerse)
			{
				ScanVerse(pos,pSrcPhrase,pList);
				EnableLessButton(FALSE);
				EnableMoreButton(TRUE);
				EnableJumpButton(TRUE);
				// show the text strings in the dialog's edit boxes
				//TransferDataToWindow(); // whm removed 21Nov11
				m_pSrcTextBox->ChangeValue(m_srcText); // whm added 21Nov11
				m_pTgtTextBox->ChangeValue(m_tgtText); // whm added 21Nov11
				m_pChapterSpinCtrl->SetValue(m_nChapter); // whm added 21Nov11
				m_pVerseSpinCtrl->SetValue(m_nVerse); // whm added 21Nov11
				m_pBeginChVerseStaticText->SetLabel(m_strBeginChVerse); // whm added 21Nov11
				m_pEndChVerseStaticText->SetLabel(m_strEndChVerse); // whm added 21Nov11
				return;
			}
		}

		// verse not found, so try again, this time assuming we may have the wanted ch & verse within
		// a range in the text, such as 3-7, or 3,4 etc.
		pos = pList->GetFirst();
		wxASSERT(pos != NULL);
		while (pos != NULL)
		{
			CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();
			wxASSERT(pSrcPhrase);

			int chapter;
			int firstVerse;
			int lastVerse;
			// BW added extra parameter Oct 2004, set it  > 0 so it has no effect on my code here 
			bool bOK = m_pView->AnalyseReference(
				pSrcPhrase->m_chapterVerse,chapter,firstVerse,lastVerse,200);

			// if we've already passed the chapter, return
			if (bOK && chapter > nWantedChapter)
				goto a;

			if (bOK && chapter == nWantedChapter 
				&& (nWantedVerse >= firstVerse && nWantedVerse <= lastVerse))
			{

				ScanVerse(pos,pSrcPhrase,pList);
				EnableLessButton(FALSE);
				EnableMoreButton(TRUE);
				EnableJumpButton(TRUE);
				//TransferDataToWindow(); // whm removed 21Nov11
				m_pSrcTextBox->ChangeValue(m_srcText); // whm added 21Nov11
				m_pTgtTextBox->ChangeValue(m_tgtText); // whm added 21Nov11
				m_pChapterSpinCtrl->SetValue(m_nChapter); // whm added 21Nov11
				m_pVerseSpinCtrl->SetValue(m_nVerse); // whm added 21Nov11
				m_pBeginChVerseStaticText->SetLabel(m_strBeginChVerse); // whm added 21Nov11
				m_pEndChVerseStaticText->SetLabel(m_strEndChVerse); // whm added 21Nov11
				m_nCurPrecChapter = chapter;
				m_nCurPrecVerse = firstVerse;
				m_nCurFollChapter = chapter;
				m_nCurFollVerse = lastVerse;
				return;
			}

			if (bOK && chapter == nWantedChapter && lastVerse > nWantedVerse) // if passed wanted verse
				goto a;
		}

		// not found, so tell the user
		// IDS_NO_SUCH_CHAPTER
a:		str = str.Format(_("Sorry, but the chapter and verse combination %s does not exist in this document. The command will be ignored."),m_chapterVerse.c_str());
		wxMessageBox(str,_T(""), wxICON_INFORMATION | wxOK);
		EnableLessButton(FALSE);
		EnableMoreButton(FALSE);
		EnableJumpButton(FALSE);
	}	
}

void CEarlierTranslationDlg::OnCloseAndJump(wxCommandEvent& event) 
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	gnOldSequNum = pApp->m_nActiveSequNum;

	// jump to the requested location
	int nSequNum = m_nFirstSequNumBasic;
	CPile* pPile = m_pView->GetPile(nSequNum);

	wxCommandEvent Okevent = wxID_OK;
	OnOK(Okevent);	// get rid of the dialog - calls destroy and deletes gpApp->m_pEarlierTransDlg pointer
	event.Skip();
#if defined(_DEBUG)
		wxLogDebug(_T("CEarlierTranslationDlg:  in OnCloseAndJump(), BEFORE GoThereSafely(),  vert ScrollPos = %d"), pApp->GetMainFrame()->canvas->GetScrollPos(wxVERTICAL));
#endif
	#if defined(__WXGTK__)
	pPile = pPile; // avoid compiler warning
	m_pView->GoThereSafely(nSequNum);
#if defined(_DEBUG)
		wxLogDebug(_T("CEarlierTranslationDlg:  in OnCloseAndJump(), AFTER GoThereSafely(),  vert ScrollPos = %d"), pApp->GetMainFrame()->canvas->GetScrollPos(wxVERTICAL));
#endif
	#else
	m_pView->Jump(pApp,pPile->GetSrcPhrase());
	#endif
}

void CEarlierTranslationDlg::OnShowMoreContext(wxCommandEvent& WXUNUSED(event)) 
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	SPList* pList = pApp->m_pSourcePhrases;
	wxASSERT(pList);
	SPList::Node* pos;
	CSourcePhrase* pSrcPhrase;

	int nIndex = m_nExpansionIndex + 1;

	// try the preceding context expansion first
	if (m_nCurPrecChapter == 1 && m_nCurPrecVerse == 1)
	{
		// we are at chapter 1 verse 1, so there is no more preceding context available
		if (nIndex == 0)
			m_preContext[nIndex] = m_nFirstSequNumBasic;
		else
			m_preContext[nIndex] = m_preContext[nIndex-1];
		goto a;
	}
	else
	{
		// there is potentially some preceding context, so accumulate the preceding verse;
		// but the verse numbering might not have started at 1, so check for nFinalSequNum = -1
		// which indicates we have gone as far as we can go
		int nFinalSequNum; // the final sequ number of the source phrases in the preceding verse
		if (nIndex == 0)
			nFinalSequNum = m_nFirstSequNumBasic - 1;
		else
			nFinalSequNum = m_preContext[nIndex-1] - 1;

		// check for having already reached the start of the document, and make safe if so
		if (nFinalSequNum == -1)
		{
			m_preContext[nIndex] = m_preContext[nIndex-1];
			goto a;
		}
		
		// space the verses apart a bit
		m_srcText = _T("     ") + m_srcText;
		m_tgtText = _T("     ") + m_tgtText;

			// get the position of this element, and its source phrase; then iterate over the
		// rest until the start of the verse is found
		pos = pList->Item(nFinalSequNum);
		wxASSERT(pos != 0);

		while (pos != 0)
		{
			pSrcPhrase = (CSourcePhrase*)pos->GetData();
			pos = pos->GetPrevious();
			wxASSERT(pSrcPhrase);

			// accumulate the source and target strings
			m_srcText = pSrcPhrase->m_srcPhrase + _T(" ") + m_srcText;
			if (gbIsGlossing)
			{
				if (!pSrcPhrase->m_gloss.IsEmpty())
					m_tgtText = pSrcPhrase->m_gloss + _T(" ") + m_tgtText;
			}
			else // adapting
			{
				if (!pSrcPhrase->m_targetStr.IsEmpty())
					m_tgtText = pSrcPhrase->m_targetStr + _T(" ") + m_tgtText;
			}

			m_preContext[nIndex] = pSrcPhrase->m_nSequNumber;

			if (!pSrcPhrase->m_chapterVerse.IsEmpty() || IsMarkedForVerse(pSrcPhrase))
			{
				// we are at the start of the preceding verse, so we must exit the loop
				// after doing some housekeeping
				if (!pSrcPhrase->m_chapterVerse.IsEmpty())
				{
					m_srcText = pSrcPhrase->m_chapterVerse + _T(" ") + m_srcText;
					m_tgtText = pSrcPhrase->m_chapterVerse + _T(" ") + m_tgtText;
					m_strBeginChVerse = pSrcPhrase->m_chapterVerse;

					// analyse the reference
					int chapter;
					int firstVerse;
					int lastVerse;
					bool bOK;
					// BW added extra parameter Oct 2004, set it  > 0 so it has no effect on my code here 
					bOK = m_pView->AnalyseReference(
						pSrcPhrase->m_chapterVerse,chapter,firstVerse,lastVerse,200);
					bOK = bOK; // avoid warning TODO: test for failures? (BEW 2Jan12, No, we want
							   // processing to continue even when FALSE returned)
					m_nCurPrecChapter = chapter;
					m_nCurPrecVerse = firstVerse; // need this in case we get to chapter 1 verse 1
				}
				break;
			}
		}
	}

	// now attempt the following context expansion
a:	if (m_nCurLastSequNum >= (int)pList->GetCount() - 1)
	{
		// we are at the end of the list, so there is no more following context
		if (nIndex == 0)
			m_follContext[nIndex] = m_nLastSequNumBasic;
		else
			m_follContext[nIndex] = m_follContext[nIndex-1];
		goto b;
	}
	else
	{
		// there is some following context, so accumulate the next verse
		int nNextSequNum; // the first sequ number of the source phrases in the following verse
		if (nIndex == 0)
			nNextSequNum = m_nLastSequNumBasic + 1;
		else
			nNextSequNum = m_follContext[nIndex-1] + 1;
		m_nCurLastSequNum = nNextSequNum;

		// get the position of this element, and its source phrase; then iterate over the
		// rest until the end of the verse is found, or the end of the document
		pos = pList->Item(nNextSequNum);
		wxASSERT(pos != 0);
		pSrcPhrase = (CSourcePhrase*)pos->GetData(); // should have non-empty m_chapterVerse
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase);
		if (!pSrcPhrase->m_chapterVerse.IsEmpty())
		{
			m_srcText += _T("     ") + pSrcPhrase->m_chapterVerse; // space apart a bit
			m_tgtText += _T("     ") + pSrcPhrase->m_chapterVerse;
			m_strEndChVerse = pSrcPhrase->m_chapterVerse;

			// analyse the reference
			int chapter;
			int firstVerse;
			int lastVerse;
			bool bOK;
			// BW added extra parameter Oct 2004, set it  > 0 so it has no effect on Bill's code here 
			bOK = m_pView->AnalyseReference(
				pSrcPhrase->m_chapterVerse,chapter,firstVerse,lastVerse,200);
			bOK = bOK; // avoid warning TODO: test for failures? (BEW 2Jan12, No, we want
					   // processing to continue even when FALSE returned)
			m_nCurFollChapter = chapter;
			m_nCurFollVerse = lastVerse; // we don't need this stuff, but no harm to compute it
		}

		// accumulate the source and target strings for this initial source phrase of the "next" verse
		m_srcText += _T(" ") + pSrcPhrase->m_srcPhrase;
		if (gbIsGlossing)
		{
			if (!pSrcPhrase->m_gloss.IsEmpty())
				m_tgtText += _T(" ") + pSrcPhrase->m_gloss;
		}
		else // adapting
		{
			if (!pSrcPhrase->m_targetStr.IsEmpty())
				m_tgtText += _T(" ") + pSrcPhrase->m_targetStr;
		}

		m_nCurLastSequNum = pSrcPhrase->m_nSequNumber;
		m_follContext[nIndex] = pSrcPhrase->m_nSequNumber;

		while (pos != 0)
		{
			pSrcPhrase = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();
			wxASSERT(pSrcPhrase);

			if (!pSrcPhrase->m_chapterVerse.IsEmpty() || IsMarkedForVerse(pSrcPhrase)
				|| pSrcPhrase->m_nSequNumber >= (int)pList->GetCount() - 1)
			{
				// we are at the start of the verse after the one we are scanning, or the end
				// of the document, so we must exit the loop after doing some housekeeping
				if (pSrcPhrase->m_nSequNumber >= (int)pList->GetCount() - 1)
				{
					// we must accumulate these strings too, then exit the loop
					m_srcText += _T(" ") + pSrcPhrase->m_srcPhrase;
					if (gbIsGlossing)
					{
						if (!pSrcPhrase->m_gloss.IsEmpty())
							m_tgtText += _T(" ") + pSrcPhrase->m_gloss;
					}
					else // adapting
					{
						if (!pSrcPhrase->m_targetStr.IsEmpty())
							m_tgtText += _T(" ") + pSrcPhrase->m_targetStr;
					}
					m_nCurLastSequNum = pSrcPhrase->m_nSequNumber;
					m_follContext[nIndex] = pSrcPhrase->m_nSequNumber;
				}
				break;
			}
			else
			{
				// we are somewhere within the verse, so accumulate the strings
				m_srcText += _T(" ") + pSrcPhrase->m_srcPhrase;
				if (gbIsGlossing)
				{
					if (!pSrcPhrase->m_gloss.IsEmpty())
						m_tgtText += _T(" ") + pSrcPhrase->m_gloss;
				}
				else // adapting
				{
					if (!pSrcPhrase->m_targetStr.IsEmpty())
						m_tgtText += _T(" ") + pSrcPhrase->m_targetStr;
				}
				m_nCurLastSequNum = pSrcPhrase->m_nSequNumber;
				m_follContext[nIndex] = pSrcPhrase->m_nSequNumber;
			}
		}
	}

b:	m_nExpansionIndex = nIndex; // update its value
	if (m_nExpansionIndex >= 0)
		EnableLessButton(TRUE);
	if (m_nExpansionIndex >= 9)
		EnableMoreButton(FALSE);
	//TransferDataToWindow(); // whm removed 21Nov11
	m_pSrcTextBox->ChangeValue(m_srcText); // whm added 21Nov11
	m_pTgtTextBox->ChangeValue(m_tgtText); // whm added 21Nov11
	m_pChapterSpinCtrl->SetValue(m_nChapter); // whm added 21Nov11
	m_pVerseSpinCtrl->SetValue(m_nVerse); // whm added 21Nov11
	m_pBeginChVerseStaticText->SetLabel(m_strBeginChVerse); // whm added 21Nov11
	m_pEndChVerseStaticText->SetLabel(m_strEndChVerse); // whm added 21Nov11
}

void CEarlierTranslationDlg::OnShowLessContext(wxCommandEvent& event) 
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	SPList* pList = pApp->m_pSourcePhrases;
	wxASSERT(pList);
	SPList::Node* pos;
	CSourcePhrase* pSrcPhrase;

	int nIndex = m_nExpansionIndex - 1;
	if (nIndex < 0)
	{
		m_nExpansionIndex = -1;
		EnableLessButton(FALSE);
		EnableMoreButton(TRUE);

		// clear the expansion arrays
		for (int i=0; i<10; ++i)
		{
			m_preContext[i] = -1;
			m_follContext[i] = -1;
		}
		OnGetChapterVerseText(event);
		return;
	}

	EnableLessButton(TRUE);
	EnableMoreButton(TRUE);

		// get the preceding expansion state
	int nFirstSequNum = m_preContext[nIndex];
	int nLastSequNum = m_follContext[nIndex];
	m_nCurLastSequNum = nLastSequNum;

	// remove memory of that state
	m_preContext[nIndex] = -1;
	m_follContext[nIndex] = -1;
	m_nExpansionIndex = nIndex;

	// accumulate the verses
	int sn = nFirstSequNum;
	pos = pList->Item(sn);
	wxASSERT(pos != 0);
	m_srcText.Empty();
	m_tgtText.Empty();

	bool bIsFirst = TRUE;
	while (pos != 0 && sn <= nLastSequNum)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData(); // may have non-empty m_chapterVerse
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase);

		if (!pSrcPhrase->m_chapterVerse.IsEmpty())
		{
			int chapter;
			int firstVerse;
			int lastVerse;

			if (bIsFirst)
			{
				m_srcText += pSrcPhrase->m_chapterVerse; // space apart a bit
				m_tgtText += pSrcPhrase->m_chapterVerse;
				m_strBeginChVerse = pSrcPhrase->m_chapterVerse;
				bIsFirst = FALSE;

				// analyse the reference
				bool bOK;
				// BW added extra parameter Oct 2004, set it  > 0 so it has no effect on my code here 
				bOK = m_pView->AnalyseReference(
					pSrcPhrase->m_chapterVerse,chapter,firstVerse,lastVerse,200);
				bOK = bOK; // avoid warning TODO: test for failures? (BEW 2Jan12, No, we want
						   // processing to continue even when FALSE returned)
				m_nCurPrecChapter = chapter;
				m_nCurPrecVerse = firstVerse;
			}
			else
			{
				m_srcText += _T("     ") + pSrcPhrase->m_chapterVerse; // space apart a bit
				m_tgtText += _T("     ") + pSrcPhrase->m_chapterVerse;
				m_strEndChVerse = pSrcPhrase->m_chapterVerse;

				// analyse the reference
				bool bOK;
				// BW added extra parameter Oct 2004, set it  > 0 so it has no effect on my code here 
				bOK = m_pView->AnalyseReference(
					pSrcPhrase->m_chapterVerse,chapter,firstVerse,lastVerse,200);
				bOK = bOK; // avoid warning TODO: test for failures? (BEW 2Jan12, No, we want
						   // processing to continue even when FALSE returned)
				m_nCurFollChapter = chapter;
				m_nCurFollVerse = lastVerse;
			}

			// analyse the reference
			bool bOK;
			// BW added extra parameter Oct 2004, set it  > 0 so it has no effect on my code here 
			bOK = m_pView->AnalyseReference(
				pSrcPhrase->m_chapterVerse,chapter,firstVerse,lastVerse,200);
			bOK = bOK; // avoid warning TODO: test for failures? (BEW 2Jan12, No, we want
					   // processing to continue even when FALSE returned)
			m_nCurFollChapter = chapter;
			m_nCurFollVerse = lastVerse; // we don't need this stuff, but no harm to compute it
		}

		// accumulate the source and target strings for this source phrase
		m_srcText += _T(" ") + pSrcPhrase->m_srcPhrase;
		if (gbIsGlossing)
		{
			if (!pSrcPhrase->m_gloss.IsEmpty())
				m_tgtText += _T(" ") + pSrcPhrase->m_gloss;
		}
		else // adapting
		{
			if (!pSrcPhrase->m_targetStr.IsEmpty())
				m_tgtText += _T(" ") + pSrcPhrase->m_targetStr;
		}

		// update the sequence number
		sn++;
	}

	m_nCurLastSequNum = sn;
	//TransferDataToWindow(); // whm removed 21Nov11
	m_pSrcTextBox->ChangeValue(m_srcText); // whm added 21Nov11
	m_pTgtTextBox->ChangeValue(m_tgtText); // whm added 21Nov11
	m_pChapterSpinCtrl->SetValue(m_nChapter); // whm added 21Nov11
	m_pVerseSpinCtrl->SetValue(m_nVerse); // whm added 21Nov11
	m_pBeginChVerseStaticText->SetLabel(m_strBeginChVerse); // whm added 21Nov11
	m_pEndChVerseStaticText->SetLabel(m_strEndChVerse); // whm added 21Nov11

}


// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
// WX Note: If user types ESC key, or clicks on the "Close" button, this OnOK() 
// handler is called followed by a call to the class Destructor.
void CEarlierTranslationDlg::OnOK(wxCommandEvent& WXUNUSED(event)) 
{
	// Note: The dialog's Close button has the wxID_OK identifier to this OnOK() handler is called
	// when user clicks "Close". This OnOK() handler is also called when the user clicks on the 
	// "Close and Jump Here" button.
	Destroy();
	//delete gpApp->m_pEarlierTransDlg; // BEW added 19Nov05, to prevent memory leak // No, this is harmful in wx!!!
	gpApp->m_pEarlierTransDlg = NULL;
	//AIModalDialog::OnOK(event); // we are running modeless so don't call the base class method
}


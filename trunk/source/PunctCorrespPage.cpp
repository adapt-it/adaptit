/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			PunctCorrespPage.cpp
/// \author			Bill Martin
/// \date_created	8 August 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CPunctCorrespPageWiz and CPunctCorrespPagePrefs classes.
/// It also defines a CPunctCorrespPageCommon class that
/// contains the common elements and methods of the two above classes.
/// These classes create a wizard page in the StartWorkingWizard and/or a panel in the
/// EditPreferencesDlg that allow the user to edit the punctuation correspondences for a 
/// project. 
/// The interface resources for CPunctCorrespPageWiz and CPunctCorrespPagePrefs are 
/// defined in PunctCorrespPageFunc() which was developed and is maintained by wxDesigner.
/// \derivation		CPunctCorrespPageWiz is derived from wxWizardPage, CPunctCorrespPagePrefs from wxPanel and CPunctCorrespPageCommon from wxPanel.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in PunctCorrespPage.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "PunctCorrespPage.h"
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
#include <wx/wizard.h>

// whm 14Jun12 modified to #include <wx/fontdate.h> for wxWidgets 2.9.x and later
#if wxCHECK_VERSION(2,9,0)
#include <wx/fontdata.h>
#endif

#include "Adapt_It.h"
#include "Pile.h"
#include "Layout.h"
#include "Adapt_ItDoc.h"
#include "PunctCorrespPage.h"
#include "FontPage.h"
#include "CaseEquivPage.h"
#include "AdaptitConstants.h"
#include "Adapt_ItView.h" 
#include "helpers.h"

// This global is defined in Adapt_It.cpp.
//extern wxWizard* pStartWorkingWizard; 

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// This global is defined in Adapt_It.cpp.
extern CFontPageWiz* pFontPageWiz;

/// This global is defined in Adapt_It.cpp.
extern CCaseEquivPageWiz* pCaseEquivPageWiz;

extern bool			gbIsGlossing;
extern wxString		translation;

// the following are common functions - used by the CPunctCorrespPageWiz class 
// and the CPunctCorrespPagePrefs class

void CPunctCorrespPageCommon::DoSetDataAndPointers()
{
	for (int i=0; i < MAXPUNCTPAIRS; i++)
	{
		m_srcPunctStr[i] = _T("");
		m_tgtPunctStr[i] = _T("");
		if (i < MAXTWOPUNCTPAIRS) // if (i < 10)
		{
			m_srcTwoPunctStr[i] = _T("");
			m_tgtTwoPunctStr[i] = _T("");
		}
	}

	// wxGenericValidator doesn't appear to work for dialogs with complex
	// nested panels and controls. So I've added a GetDataFromEdits() 
	// method added code to PopulateWithGlyphs() to effect data transfer. 

	m_editSrcPunct[0] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC0);
	m_editSrcPunct[1] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC1);
	m_editSrcPunct[2] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC2);
	m_editSrcPunct[3] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC3);
	m_editSrcPunct[4] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC4);
	m_editSrcPunct[5] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC5);
	m_editSrcPunct[6] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC6);
	m_editSrcPunct[7] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC7);
	m_editSrcPunct[8] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC8);
	m_editSrcPunct[9] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC9);
	m_editSrcPunct[10] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC10);
	m_editSrcPunct[11] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC11);
	m_editSrcPunct[12] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC12);
	m_editSrcPunct[13] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC13);
	m_editSrcPunct[14] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC14);
	m_editSrcPunct[15] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC15);
	m_editSrcPunct[16] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC16);
	m_editSrcPunct[17] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC17);
	m_editSrcPunct[18] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC18);
	m_editSrcPunct[19] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC19);
	m_editSrcPunct[20] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC20);
	m_editSrcPunct[21] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC21);
	m_editSrcPunct[22] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC22);
	m_editSrcPunct[23] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC23);
	m_editSrcPunct[24] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC24);
	m_editSrcPunct[25] = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC25);

	m_editTgtPunct[0] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT0);
	m_editTgtPunct[1] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT1);
	m_editTgtPunct[2] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT2);
	m_editTgtPunct[3] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT3);
	m_editTgtPunct[4] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT4);
	m_editTgtPunct[5] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT5);
	m_editTgtPunct[6] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT6);
	m_editTgtPunct[7] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT7);
	m_editTgtPunct[8] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT8);
	m_editTgtPunct[9] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT9);
	m_editTgtPunct[10] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT10);
	m_editTgtPunct[11] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT11);
	m_editTgtPunct[12] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT12);
	m_editTgtPunct[13] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT13);
	m_editTgtPunct[14] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT14);
	m_editTgtPunct[15] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT15);
	m_editTgtPunct[16] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT16);
	m_editTgtPunct[17] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT17);
	m_editTgtPunct[18] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT18);
	m_editTgtPunct[19] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT19);
	m_editTgtPunct[20] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT20);
	m_editTgtPunct[21] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT21);
	m_editTgtPunct[22] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT22);
	m_editTgtPunct[23] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT23);
	m_editTgtPunct[24] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT24);
	m_editTgtPunct[25] = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT25);

	m_editSrcTwoPunct[0] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2SRC0);
	m_editSrcTwoPunct[1] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2SRC1);
	m_editSrcTwoPunct[2] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2SRC2);
	m_editSrcTwoPunct[3] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2SRC3);
	m_editSrcTwoPunct[4] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2SRC4);
	m_editSrcTwoPunct[5] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2SRC5);
	m_editSrcTwoPunct[6] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2SRC6);
	m_editSrcTwoPunct[7] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2SRC7);
	m_editSrcTwoPunct[8] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2SRC8);
	m_editSrcTwoPunct[9] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2SRC9);

	m_editTgtTwoPunct[0] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2TGT0);
	m_editTgtTwoPunct[1] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2TGT1);
	m_editTgtTwoPunct[2] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2TGT2);
	m_editTgtTwoPunct[3] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2TGT3);
	m_editTgtTwoPunct[4] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2TGT4);
	m_editTgtTwoPunct[5] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2TGT5);
	m_editTgtTwoPunct[6] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2TGT6);
	m_editTgtTwoPunct[7] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2TGT7);
	m_editTgtTwoPunct[8] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2TGT8);
	m_editTgtTwoPunct[9] = (wxTextCtrl*)FindWindowById(IDC_EDIT_2TGT9);

	pToggleUnnnnBtn = (wxButton*)FindWindowById(IDC_TOGGLE_UNNNN_BTN);
	pTextCtrlAsStaticText = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_PUNCT_CORRESP_PAGE);
	// Make the wxTextCtrl that is displaying static text have window background color
	wxColor backgrndColor = this->GetBackgroundColour();
	pTextCtrlAsStaticText->SetBackgroundColour(backgrndColor);
}


void CPunctCorrespPageCommon::DoInit()
{
	gpApp->m_pLayout->m_bPunctuationChanged = FALSE;

#ifndef _UNICODE // ANSI version
	// Hide the "Show U+nnnn" button for ANSI version
	pToggleUnnnnBtn->Hide();

#endif

	// Note: Punctuation Edit Box fonts are set and font size adjustments
	// are made in SetupForGlyphs().
	SetupForGlyphs();

#ifdef _UNICODE
	m_bShowingChars = TRUE;
#endif

	PopulateWithGlyphs();

	// The rest of InitDialog below was in MFC's OnEditPunctCorresp() of the View:
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	wxASSERT(pView != NULL);

	// save the source punctuation list, so we can figure out if the user changed the
	// source punctuation list - and if he does, we will have to retokenize and possibly
	// rebuild the document
	// 
	// whm corrected 22May09 to use m_punctuationBeforeEdit[0] and remove the remnants of pApp->m_savePunctuation[].
	// Before this correction, DoPunctuationChanges() was always being called in the OnOk() handler even when no changes were
	// made to punctuation in Preferences.
	m_punctuationBeforeEdit[0] = pApp->m_punctuation[0];
	m_punctuationBeforeEdit[1] = pApp->m_punctuation[1]; //BEW 4Mar11, added, but may not
								// actually be needed, but it should not be left unset

	//int activeSequNum; // set but not used
	if (pApp->m_nActiveSequNum < 0)
	{
		// must not have data yet, or we are at EOF and so no pile is currently active
		; //activeSequNum = -1;
	}
	else
	{
		// we are somewhere in the midst of the data, so a pile will be active
		//activeSequNum = pApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber;
		//pApp->m_curIndex = activeSequNum;

		// remove any current selection, as we can't be sure of any pointers
		// depending on what user may choose to alter
		pView->RemoveSelection();
	}
	strSavePhraseBox = pApp->m_targetPhrase;
	CopyInitialPunctSets(); // fill the m_srcPunctStrBeforeEdit[], m_tgtPunctStrBeforeEdit[], 
							// etc arrays, so we can compare the items later to see if the
							// user has changed anything
}

// BEW added 4Mar11, so as to prevent a DoPunctuationChanges() call happening merely
// because, for example, the user entered Preferences, did nothing, and exited
// Preferences!
// Call this in PunctCorrespPage's OnOK() function, and only if the return value is TRUE
// should DoPunctuationChanges() be called.
bool CPunctCorrespPageCommon::AreBeforeAndAfterPunctSetsDifferent()
{
	int index;
	bool bSomethingIsDifferent = FALSE;
	for (index = 0; index < MAXPUNCTPAIRS; index++)
	{
		if ( !(m_srcPunctStrBeforeEdit[index].IsEmpty() && m_srcPunctStr[index].IsEmpty())
			&& (m_srcPunctStrBeforeEdit[index] != m_srcPunctStr[index]))
		{
			bSomethingIsDifferent = TRUE;
		}
		if ( !(m_tgtPunctStrBeforeEdit[index].IsEmpty() && m_tgtPunctStr[index].IsEmpty())
			&& (m_tgtPunctStrBeforeEdit[index] != m_tgtPunctStr[index]))
		{
			bSomethingIsDifferent = TRUE;
		}
	}
	if (bSomethingIsDifferent)
		return TRUE;
	for (index = 0; index < MAXTWOPUNCTPAIRS; index++)
	{
		if ( !(m_srcTwoPunctStrBeforeEdit[index].IsEmpty() && m_srcTwoPunctStr[index].IsEmpty())
			&& (m_srcTwoPunctStrBeforeEdit[index] != m_srcTwoPunctStr[index]))
		{
			bSomethingIsDifferent = TRUE;
		}
		if ( !(m_tgtTwoPunctStrBeforeEdit[index].IsEmpty() && m_tgtTwoPunctStr[index].IsEmpty())
			&& (m_tgtTwoPunctStrBeforeEdit[index] != m_tgtTwoPunctStr[index]))
		{
			bSomethingIsDifferent = TRUE;
		}
	}
	return bSomethingIsDifferent;
}


// call this after the punctuation correspondences page has had its data set up, and
// before the user has a chance to change anything in the page
void CPunctCorrespPageCommon::CopyInitialPunctSets()
{
	int index;
	for (index = 0; index < MAXPUNCTPAIRS; index++)
	{
		m_srcPunctStrBeforeEdit[index] = m_srcPunctStr[index];
		m_tgtPunctStrBeforeEdit[index] = m_tgtPunctStr[index];
	}		
	for (index = 0; index < MAXTWOPUNCTPAIRS; index++)
	{
		m_srcTwoPunctStrBeforeEdit[index].Empty();
		m_tgtTwoPunctStrBeforeEdit[index].Empty();
	}
	for (index = 0; index < MAXTWOPUNCTPAIRS; index++)
	{
		if (!m_srcTwoPunctStr[index].IsEmpty())
		{
			m_srcTwoPunctStrBeforeEdit[index] = m_srcTwoPunctStr[index];
		}
		if (m_tgtTwoPunctStr[index].IsEmpty())
		{
			m_tgtTwoPunctStrBeforeEdit[index] = m_tgtTwoPunctStr[index];
		}
	}
}


// The next functions are helpers which make it easy to set or reset the boxes to show
// either unicode characters (LTR or RTL), or U+nnnn representations (always LTR because
// they will be shown in natural order, rather than in human script's reading order) These
// functions added in version 2.2.1 to support the "Show U+nnnn" / "Show Characters" toggle
// button added in that version.

void CPunctCorrespPageCommon::PopulateWithGlyphs()
// Copies the App's m_punctPairs and m_twopunctPairs char data to the
// respective m_srcPunctStr[], m_tgtPunctStr[], m_srcTwoPunctStr[] and 
// m_tgtTwoPunctStr[] local string arrays. Since the wxGenericValidator
// doesn't appear to be transferring data between the text ctrls and
// the strings, I've set the text in the edit ctrls manually.
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp);
	int index;
	for (index = 0; index < MAXPUNCTPAIRS; index++)
	{
		if (pApp->m_punctPairs[index].charSrc == _T('\0'))
		{
			m_srcPunctStr[index] = _T("");
			m_editSrcPunct[index]->SetValue(m_srcPunctStr[index]);
		}
		else
		{
			m_srcPunctStr[index] = pApp->m_punctPairs[index].charSrc;
			m_editSrcPunct[index]->SetValue(m_srcPunctStr[index]);
		}
		if (pApp->m_punctPairs[index].charTgt == _T('\0'))
		{
			m_tgtPunctStr[index] = _T("");
			m_editTgtPunct[index]->SetValue(m_tgtPunctStr[index]);
		}
		else
		{
			m_tgtPunctStr[index] = pApp->m_punctPairs[index].charTgt;
			m_editTgtPunct[index]->SetValue(m_tgtPunctStr[index]);
		}
	} // end of for loop

	for (index = 0; index < MAXTWOPUNCTPAIRS; index++)
	{
		// src ones
		if (pApp->m_twopunctPairs[index].twocharSrc[0] == _T('\0'))
		{
			m_srcTwoPunctStr[index] = _T(""); // if first was null, second is too, so string is null
			m_editSrcTwoPunct[index]->SetValue(m_srcTwoPunctStr[index]);
		}
		else
		{
			m_srcTwoPunctStr[index] = pApp->m_twopunctPairs[index].twocharSrc[0];
			if (pApp->m_twopunctPairs[index].twocharSrc[1] != _T('\0'))
			{
				m_srcTwoPunctStr[index] += pApp->m_twopunctPairs[index].twocharSrc[1];
				m_editSrcTwoPunct[index]->SetValue(m_srcTwoPunctStr[index]);
			}
		}
		// tgt ones
		if (pApp->m_twopunctPairs[index].twocharTgt[0] == _T('\0'))
		{
			m_tgtTwoPunctStr[index] = _T(""); // if first was null, second is too, so string is null
			m_editTgtTwoPunct[index]->SetValue(m_tgtTwoPunctStr[index]);
		}
		else
		{
			m_tgtTwoPunctStr[index] = pApp->m_twopunctPairs[index].twocharTgt[0];
			if (pApp->m_twopunctPairs[index].twocharTgt[1] != _T('\0'))
			{
				m_tgtTwoPunctStr[index] += pApp->m_twopunctPairs[index].twocharTgt[1];
				m_editTgtTwoPunct[index]->SetValue(m_tgtTwoPunctStr[index]);
			}
		}
	} // end of for loop
}

void CPunctCorrespPageCommon::SetupForGlyphs()
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp); 

	CopyFontBaseProperties(pApp->m_pSourceFont,pApp->m_pDlgSrcFont); // CopyFontBaseProperties sets encoding
	pApp->m_pDlgSrcFont->SetPointSize(pApp->m_dialogFontSize);

	CopyFontBaseProperties(pApp->m_pTargetFont,pApp->m_pDlgTgtFont); // CopyFontBaseProperties sets encoding
	pApp->m_pDlgTgtFont->SetPointSize(pApp->m_dialogFontSize);

	for (int i=0; i<MAXPUNCTPAIRS; i++)
	{
		m_editSrcPunct[i]->SetFont(*pApp->m_pDlgSrcFont); 
#ifdef _RTL_FLAGS
		if (pApp->m_bSrcRTL)
		{
			m_editSrcPunct[i]->SetLayoutDirection(wxLayout_RightToLeft);
		}
		else
		{
			m_editSrcPunct[i]->SetLayoutDirection(wxLayout_LeftToRight);
		}
#endif
		if (i < MAXTWOPUNCTPAIRS) //if (i < 10)
		{
			m_editSrcTwoPunct[i]->SetFont(*pApp->m_pDlgSrcFont);
#ifdef _RTL_FLAGS
			if (pApp->m_bSrcRTL)
			{
				m_editSrcTwoPunct[i]->SetLayoutDirection(wxLayout_RightToLeft);
			}
			else
			{
				m_editSrcTwoPunct[i]->SetLayoutDirection(wxLayout_LeftToRight);
			}
#endif
		}
	}
	for (int j=0; j<MAXPUNCTPAIRS; j++)
	{
		m_editTgtPunct[j]->SetFont(*pApp->m_pDlgTgtFont);
#ifdef _RTL_FLAGS
		if (pApp->m_bTgtRTL)
		{
			m_editTgtPunct[j]->SetLayoutDirection(wxLayout_RightToLeft);
		}
		else
		{
			m_editTgtPunct[j]->SetLayoutDirection(wxLayout_LeftToRight);
		}
#endif
		if (j < MAXTWOPUNCTPAIRS) //if (j < 10)
		{
			m_editTgtTwoPunct[j]->SetFont(*pApp->m_pDlgTgtFont);
#ifdef _RTL_FLAGS
			if (pApp->m_bTgtRTL)
			{
				m_editTgtTwoPunct[j]->SetLayoutDirection(wxLayout_RightToLeft);
			}
			else
			{
				m_editTgtTwoPunct[j]->SetLayoutDirection(wxLayout_LeftToRight);
			}
#endif
		}
	}
	pPunctCorrespPageSizer->Layout();

#ifdef _UNICODE
	wxString s;
	// IDS_SHOWING_UNNNN
	s = s.Format(_("U+nnnn"));
	wxString btn;
	// IDS_SHOWING_BTN
	btn = btn.Format(_("Show %s"),s.c_str());
	if (pToggleUnnnnBtn != 0)
	{
		pToggleUnnnnBtn->SetLabel(btn);
	}
#endif
}

void CPunctCorrespPageCommon::UpdateAppValues(bool bFromGlyphs)
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp);

	if (bFromGlyphs)
	{
		// since DDX filled class variables with characters, update app values with no conversion
		int index;
		for (index = 0; index < MAXPUNCTPAIRS; index++)
		{
			if (m_srcPunctStr[index].IsEmpty())
			{
				pApp->m_punctPairs[index].charSrc = _T('\0');
			}
			else
			{
				// whm 11Jun12 Note: the if block above ensures that m_srcPunctStr[index] is not empty here
				pApp->m_punctPairs[index].charSrc = m_srcPunctStr[index].GetChar(0);
			}
			if (m_tgtPunctStr[index].IsEmpty())
			{
				pApp->m_punctPairs[index].charTgt = _T('\0');
			}
			else
			{
				// whm 11Jun12 Note: the if block above ensures that m_tgtPunctStr[index] is not empty here
				pApp->m_punctPairs[index].charTgt = m_tgtPunctStr[index].GetChar(0);
			}
		}

		for (index = 0; index < MAXTWOPUNCTPAIRS; index++)
		{
			if (m_srcTwoPunctStr[index].IsEmpty())
			{
				pApp->m_twopunctPairs[index].twocharSrc[0] = _T('\0');
				pApp->m_twopunctPairs[index].twocharSrc[1] = _T('\0');
				pApp->m_twopunctPairs[index].twocharTgt[0] = _T('\0');
				pApp->m_twopunctPairs[index].twocharTgt[1] = _T('\0');
			}
			else
			{
				// there is something in the pair to be dealt with, do src side
				// whm 11Jun12 Note: the if block above ensures that m_srcTwoPunctStr[index] is not empty here
				pApp->m_twopunctPairs[index].twocharSrc[0] = m_srcTwoPunctStr[index].GetChar(0);
				if (m_srcTwoPunctStr[index].GetChar(1) == _T('\0'))
				{
					pApp->m_twopunctPairs[index].twocharSrc[1] = _T('\0');
				}
				else
				{
					pApp->m_twopunctPairs[index].twocharSrc[1] = m_srcTwoPunctStr[index].GetChar(1);
				}

				// now do target side
				if (m_tgtTwoPunctStr[index].IsEmpty())
				{
					// target side is empty
					pApp->m_twopunctPairs[index].twocharTgt[0] = _T('\0');
					pApp->m_twopunctPairs[index].twocharTgt[1] = _T('\0');
				}
				else
				{
					// target side has something
					// whm 11Jun12 Note: the if block above ensures that m_tgtTwoPunctStr[index] is not empty here
					pApp->m_twopunctPairs[index].twocharTgt[0] = m_tgtTwoPunctStr[index].GetChar(0);
					if (m_tgtTwoPunctStr[index].GetChar(1) == _T('\0'))
					{
						pApp->m_twopunctPairs[index].twocharTgt[1] = _T('\0');
					}
					else
					{
						pApp->m_twopunctPairs[index].twocharTgt[1] = m_tgtTwoPunctStr[index].GetChar(1);
					}
				}
			}
		}
	}
	else
	{
#ifdef _UNICODE // don't need this block for an ANSI build, since U+nnnn toggle button is hidden
		// convert from unicode numbers to unicode characters first, then update app values
		int index;
		for (index = 0; index < MAXPUNCTPAIRS; index++)
		{
			wxString s;
			if (m_srcPunctStr[index].IsEmpty())
			{
				pApp->m_punctPairs[index].charSrc = _T('\0');
				if (m_tgtPunctStr[index].IsEmpty())
				{
					pApp->m_punctPairs[index].charTgt = _T('\0');
				}
				else
				{
					s = UnMakeUNNNN(m_tgtPunctStr[index]);
					if (s.IsEmpty())
						// if an error, make this cell empty
						pApp->m_punctPairs[index].charTgt = _T('\0');
					else
						pApp->m_punctPairs[index].charTgt = s.GetChar(0); // whm 11Jun12 ok. s cannot be empty here
				}
			}
			else
			{
				s = UnMakeUNNNN(m_srcPunctStr[index]);
				if (s.IsEmpty())
				{
					// if an error, make this cell empty
					pApp->m_punctPairs[index].charSrc = _T('\0');
				}
				else
				{
					pApp->m_punctPairs[index].charSrc = s.GetChar(0); // whm 11Jun12 ok. s cannot be empty here
				}

				// do the target one
				if (m_tgtPunctStr[index].IsEmpty())
				{
					pApp->m_punctPairs[index].charTgt = _T('\0');
				}
				else
				{
					s = UnMakeUNNNN(m_tgtPunctStr[index]);
					if (s.IsEmpty())
						// if an error, make this cell empty
						pApp->m_punctPairs[index].charTgt = _T('\0');
					else
						pApp->m_punctPairs[index].charTgt = s.GetChar(0); // whm 11Jun12 ok. s cannot be empty here
				}
			}
		}

		for (index = 0; index < MAXTWOPUNCTPAIRS; index++)
		{
			wxString s1; // for "nnnn" strings
			wxString s2; // for "nnnn" strings
			wxString firstStr; // for "nnnn" converted to wxChar
			wxString secondStr; // for "nnnn" converted to wxChar
			if (m_srcTwoPunctStr[index].IsEmpty())
			{
				pApp->m_twopunctPairs[index].twocharSrc[0] = _T('\0');
				pApp->m_twopunctPairs[index].twocharSrc[1] = _T('\0');
				pApp->m_twopunctPairs[index].twocharTgt[0] = _T('\0');
				pApp->m_twopunctPairs[index].twocharTgt[1] = _T('\0');
			}
			else
			{
				// there is something in the pair to be dealt with, do src side
				if (!ExtractSubstrings(m_srcTwoPunctStr[index],s1,s2))
				{
					// if an error, just make this row empty
					pApp->m_twopunctPairs[index].twocharSrc[0] = _T('\0');
					pApp->m_twopunctPairs[index].twocharSrc[1] = _T('\0');
					pApp->m_twopunctPairs[index].twocharTgt[0] = _T('\0');
					pApp->m_twopunctPairs[index].twocharTgt[1] = _T('\0');
					continue;
				}
				firstStr.Empty();
				firstStr = UnMakeUNNNN(s1); // should check here, but I'll assume that anyone editing
				secondStr.Empty();			// the U+nnnn value knows he should retain leading zeros
				secondStr = UnMakeUNNNN(s2);
				// whm 11Jun12 modified. The UnMakeUNNNN() function above will return an empty string
				// if s1 is an empty string, so we need to protect the firstStr.GetChar(0) call below
				if (!firstStr.IsEmpty())
					pApp->m_twopunctPairs[index].twocharSrc[0] = firstStr.GetChar(0);
				// when firstStr is empty just keep the _T('\0') char assigned to twocharSrc[0] above 
				if (secondStr.IsEmpty())
				{
					pApp->m_twopunctPairs[index].twocharSrc[1] = _T('\0');
				}
				else
				{
					pApp->m_twopunctPairs[index].twocharSrc[1] = secondStr.GetChar(0);
				}

				// now do target side
				if (m_tgtTwoPunctStr[index].IsEmpty())
				{
					pApp->m_twopunctPairs[index].twocharTgt[0] = _T('\0');
					pApp->m_twopunctPairs[index].twocharTgt[1] = _T('\0');
				}
				else
				{
					// there is something to be dealt with on the target side
					if (!ExtractSubstrings(m_tgtTwoPunctStr[index],s1,s2))
					{
						// if an error, just make this row empty
						pApp->m_twopunctPairs[index].twocharTgt[0] = _T('\0');
						pApp->m_twopunctPairs[index].twocharTgt[1] = _T('\0');
						continue;
					}
					firstStr.Empty();
					firstStr = UnMakeUNNNN(s1);  // won't bother to check
					pApp->m_twopunctPairs[index].twocharTgt[0] = firstStr.GetChar(0);
					firstStr.Empty();

					// now the second one
					secondStr.Empty();
					secondStr = UnMakeUNNNN(s2); 
					if (secondStr.IsEmpty())
					{
						pApp->m_twopunctPairs[index].twocharTgt[1] = _T('\0');
					}
					else
					{
						pApp->m_twopunctPairs[index].twocharTgt[1] = secondStr.GetChar(0);
					}
				}
			}
		}
#endif // for _UNICODE defined
	}

	// next compute m_srcPunctuation and m_tgtPunctuation class variable contents, which will 
	// then be copied to m_punctuation[0] and m_punctuation[1] on the app
	GetPunctuationSets(); // compute the derived src and tgt punctuation lists

	// set, or reset, the app variables which store the punctuation lists
	pApp->m_punctuation[0] = m_srcPunctuation;
	pApp->m_punctuation[1] = m_tgtPunctuation;
	
	// BEW added 5May05, the tests beleow, to allow for user to change the status of ordinary
	// single or double quote characters. I can't just use .Find() here because MS have made it
	// a 'smart Find' in that searching for " returns a non-zero result if there is a curly
	// double quote in the string, and similarly for using Find() to look for a ' character.
	// So I'll have to code my own test.
	// Update the status of ordinary apostrophe and double-quote
	if (pApp->ContainsOrdinaryQuote(m_srcPunctuation, _T('\"')))
	{
		// double-quote is still source punctuation
		pApp->m_bDoubleQuoteAsPunct = TRUE;
	}
	else
	{
		pApp->m_bDoubleQuoteAsPunct = FALSE; // it's a word-building character
	}
	if (pApp->ContainsOrdinaryQuote(m_srcPunctuation, _T('\'')))
	{
		// single-quote is still source punctuation
		pApp->m_bSingleQuoteAsPunct = TRUE;
	}
	else
	{
		pApp->m_bSingleQuoteAsPunct = FALSE; // it's a word-building character
	}
}

void CPunctCorrespPageCommon::GetPunctuationSets()
{
	gpApp->GetPunctuationSets(m_srcPunctuation, m_tgtPunctuation);
}

#ifdef _UNICODE
void CPunctCorrespPageCommon::SetupForUnicodeNumbers()
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp);

	CopyFontBaseProperties(pApp->m_pNavTextFont,pApp->m_pDlgSrcFont); // using m_pNavTextFont properties and encoding
	pApp->m_pDlgSrcFont->SetPointSize(9); // 9 point
	pApp->m_pDlgSrcFont->SetWeight(wxFONTWEIGHT_NORMAL); // not bold or light font

	for (int i=0; i<MAXPUNCTPAIRS; i++)
	{

		m_editSrcPunct[i]->SetFont(*pApp->m_pDlgSrcFont); // uses m_pNavTextFont's properties and encoding
#ifdef _RTL_FLAGS
		// force LTR directionality for numbers
		m_editSrcPunct[i]->SetLayoutDirection(wxLayout_LeftToRight);
#endif
		if (i < MAXTWOPUNCTPAIRS)
		{
			m_editSrcTwoPunct[i]->SetFont(*pApp->m_pDlgSrcFont); // uses m_pNavTextFont's properties and encoding
#ifdef _RTL_FLAGS
			// force LTR directionality for numbers
			m_editSrcTwoPunct[i]->SetLayoutDirection(wxLayout_LeftToRight);
#endif
		}
	}
	for (int j=0; j<MAXPUNCTPAIRS; j++)
	{

		m_editTgtPunct[j]->SetFont(*pApp->m_pDlgSrcFont); // uses m_pNavTextFont's properties and encoding
#ifdef _RTL_FLAGS
		// force LTR directionality for numbers
		m_editTgtPunct[j]->SetLayoutDirection(wxLayout_LeftToRight);
#endif
		if (j < MAXTWOPUNCTPAIRS)
		{
			m_editTgtTwoPunct[j]->SetFont(*pApp->m_pDlgSrcFont); // uses m_pNavTextFont's properties and encoding
#ifdef _RTL_FLAGS
			// force LTR directionality for numbers
			m_editTgtTwoPunct[j]->SetLayoutDirection(wxLayout_LeftToRight);
#endif
		}
	}

	wxString s;
	// IDS_SHOWING_CHARS
	s = s.Format(_("Characters"));
	wxString btn;
	// IDS_SHOWING_BTN
	btn = btn.Format(_("Show %s"),s.c_str());
	if (pToggleUnnnnBtn != 0)
	{
		pToggleUnnnnBtn->SetLabel(btn);
	}
}

wxString CPunctCorrespPageCommon::MakeUNNNN(wxString& chStr)
{
	wxString prefix = _T(""); // some people said U+ makes the strings too wide, so leave
							 // it off_T("U+");
	// whm 11Jun12 Note: I think chStr will always have at least a value of T('\0'), so
	// GetChar(0) won't ever be called on an empty string, but to be safe test for empty
	// string.
	wxChar theChar;
	if (!chStr.IsEmpty())
		theChar = chStr.GetChar(0);
	else
		theChar = _T('\0');
	wxChar str[6] = {_T('\0'),_T('\0'),_T('\0'),_T('\0'),_T('\0'),_T('\0')};
	wxChar* pStr = str;
	wxSnprintf(pStr,6,_T("%x"),(int)theChar);
	wxString s = pStr;
	if (s == _T("0"))
	{
		s.Empty();
		return s;
	}
	int len = s.Length();
	if (len == 2)
		s = _T("00") + s;
	else if (len == 3)
		s = _T("0") + s;
	return prefix + s;
}

int CPunctCorrespPageCommon::HexToInt(const wxChar hexDigit)
{
	switch (hexDigit)
	{
	case _T('f'):
	case _T('F'):
		return 15;
	case _T('e'):
	case _T('E'):
		return 14;
	case _T('d'):
	case _T('D'):
		return 13;
	case _T('c'):
	case _T('C'):
		return 12;
	case _T('b'):
	case _T('B'):
		return 11;
	case _T('a'):
	case _T('A'):
		return 10;
	case _T('9'):
		return 9;
	case _T('8'):
		return 8;
	case _T('7'):
		return 7;
	case _T('6'):
		return 6;
	case _T('5'):
		return 5;
	case _T('4'):
		return 4;
	case _T('3'):
		return 3;
	case _T('2'):
		return 2;
	case _T('1'):
		return 1;
	case _T('0'):
	default:
		return 0;
	}
}

wxString CPunctCorrespPageCommon::UnMakeUNNNN(wxString& nnnnStr)
{
	int len = nnnnStr.Length();
	if (len == 0)
		return _T(""); // return an empty string if we passed in an empty one
	if (len > 4)
	{
		wxString msg;
		msg = msg.Format(_("Error, hex number longer than 4 digits: %s\n"),nnnnStr.c_str());
		wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
		return _T("");
	}
	wxString s;
	s.Empty();
	wxStringBuffer pBuff(nnnnStr,5);
	wxChar* pNNNN = pBuff;
	wxChar hexDigit;
	int value = 0;
	int part = 0;
	hexDigit = *pNNNN++;
	part = HexToInt(hexDigit);
	value += 4096 * part;
	hexDigit = *pNNNN++;
	part = HexToInt(hexDigit);
	value += 256 * part;
	hexDigit = *pNNNN++;
	part = HexToInt(hexDigit);
	value += 16 * part;
	hexDigit = *pNNNN++;
	part = HexToInt(hexDigit);
	value += part;
	s = (wxChar)value;
	return s;
}

bool CPunctCorrespPageCommon::ExtractSubstrings(wxString& dataStr,wxString& s1,wxString& s2)
// return TRUE if no error, FALSE if the string has extra spaces 
// (length should be 9 if two characters in the box, or 4 if there is only one)
{
	wxString set = _T("0123456789abcdefABCDEF");
	wxString hold;
	int len = dataStr.Length();
	if (len > 9)
	{
		wxString msg;
		msg = msg.Format(_("Error, pair of hex numbers plus space longer than 9 digits: %s\n"),dataStr.c_str());
		wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
		msg += _T("In PunctCorrespPage ") + msg;
		gpApp->LogUserAction(msg);
		return FALSE;
	}
	wxString s;
	s.Empty();
	wxStringBuffer pBuff(dataStr,10);
	wxChar* pNNNN = pBuff;
	s1.Empty();
	s2.Empty();

	// get the first nnnn string, there must be one since the caller would not otherwise
	// have invoked this function
	int i;
	for (i = 0; i < 4; i++)
	{
		hold = *(pNNNN + i);
		if (FindOneOf(hold,set) == -1)
		{
			wxString msg;
			msg = msg.Format(_("Error, invalid hex digit: %s\n"),hold.c_str());
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			msg += _T("In PunctCorrespPage ") + msg;
			gpApp->LogUserAction(msg);
			return FALSE;
		}
		else
		{
			s1 += hold;
		}
	}

	// the second might not exist, so first check
	if (len > 5)
	{
		// there is probably a second nnnn value, so try get it
		for (i = 5; i < 9; i++)
		{
			hold = *(pNNNN + i);
			if (FindOneOf(hold,set) == -1)
			{
				wxString msg;
				msg = msg.Format(_("Error, invalid hex digit: %s\n"),hold.c_str());
				wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
				msg += _T("In PunctCorrespPage ") + msg;
				gpApp->LogUserAction(msg);
				return FALSE;
			}
			else
			{
				s2 += hold;
			}
		}
	}
	else
	{
		s2.Empty();
	}
	return TRUE;
}

void CPunctCorrespPageCommon::PopulateWithUnicodeNumbers()
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp);

	int index;
	for (index = 0; index < MAXPUNCTPAIRS; index++)
	{
		if (pApp->m_punctPairs[index].charSrc == _T('\0'))
		{
			m_srcPunctStr[index] = _T("");
		}
		else
		{
		m_srcPunctStr[index] = pApp->m_punctPairs[index].charSrc;
		}
		m_srcPunctStr[index] = MakeUNNNN(m_srcPunctStr[index]); // convert to U+nnnn
																// or an empty string
		if (pApp->m_punctPairs[index].charTgt == _T('\0'))
		{
			m_tgtPunctStr[index] = _T("");
		}
		else
		{
			m_tgtPunctStr[index] = pApp->m_punctPairs[index].charTgt;
			m_tgtPunctStr[index] = MakeUNNNN(m_tgtPunctStr[index]); // convert to U+nnnn
		}
	}

	int len;
	for (index = 0; index < MAXTWOPUNCTPAIRS; index++)
	{
		if (pApp->m_twopunctPairs[index].twocharSrc[0] == _T('\0'))
		{
			len = 0;
		}
		else
		{
			len = 1;
		}
		m_srcTwoPunctStr[index] = pApp->m_twopunctPairs[index].twocharSrc[0];
		if (pApp->m_twopunctPairs[index].twocharSrc[1] != _T('\0'))
		{
			m_srcTwoPunctStr[index] += pApp->m_twopunctPairs[index].twocharSrc[1];
			len++; // make it 1 or 2
		}
		if (len == 1)
		{
			m_srcTwoPunctStr[index] = MakeUNNNN(m_srcTwoPunctStr[index]);
		}
		else if (len == 2)
		{
			wxString first = m_srcTwoPunctStr[index].GetChar(0);
			wxString second = m_srcTwoPunctStr[index].GetChar(1);
			m_srcTwoPunctStr[index] = MakeUNNNN(first);
			m_srcTwoPunctStr[index] = m_srcTwoPunctStr[index] + _T(" ") + MakeUNNNN(second);
		}

		if (pApp->m_twopunctPairs[index].twocharTgt[0] == _T('\0'))
		{
			m_tgtTwoPunctStr[index] = _T(""); // if first was null, second may be or might not be
			len = 0;
			if (pApp->m_twopunctPairs[index].twocharTgt[1] != _T('\0'))
			{
				m_tgtTwoPunctStr[index] += pApp->m_twopunctPairs[index].twocharTgt[1];
				len = 1;
			}
		}
		else
		{
			m_tgtTwoPunctStr[index] = pApp->m_twopunctPairs[index].twocharTgt[0];
			len = 1;
			if (pApp->m_twopunctPairs[index].twocharTgt[1] != _T('\0'))
			{
				m_tgtTwoPunctStr[index] += pApp->m_twopunctPairs[index].twocharTgt[1];
				len = 2;
			}
			if (len == 1)
			{
				m_tgtTwoPunctStr[index] = MakeUNNNN(m_tgtTwoPunctStr[index]);
			}
			else if (len == 2)
			{
				wxString first = m_tgtTwoPunctStr[index].GetChar(0);
				wxString second = m_tgtTwoPunctStr[index].GetChar(1);
				m_tgtTwoPunctStr[index] = MakeUNNNN(first);
				m_tgtTwoPunctStr[index] = m_tgtTwoPunctStr[index] + _T(" ") + MakeUNNNN(second);
			}
		}
	}
}

void CPunctCorrespPageCommon::OnBnClickedToggleUnnnn(wxCommandEvent& WXUNUSED(event))
{
	if (m_bShowingChars)
	{
		//Set up for unicode U+nnnn values in this block
		GetDataFromEdits();
		UpdateAppValues(TRUE); // TRUE means the class variables store glyphs

		// set up the dialog to make it show U+NNNN values
		SetupForUnicodeNumbers();

		// setup the display of the numbers
		PopulateWithUnicodeNumbers();

		StoreDataIntoEdits();
	}
	else
	{
		// set up the dialog to make it return to showing the glyphs, as updated by any user
		// editing of the NNNN numbers
		GetDataFromEdits();
		UpdateAppValues(FALSE); // FALSE means "not from glyphs", ie. class variables contain nnnn

		SetupForGlyphs();

		// setup the display of the glyphs
		PopulateWithGlyphs();

		StoreDataIntoEdits();
	}

	m_bShowingChars = m_bShowingChars ? FALSE : TRUE; // toggle the value
	pPunctCorrespPageSizer->Layout();
}
#endif // for _UNICODE

void CPunctCorrespPageCommon::GetDataFromEdits()
{
	wxString Temp;
	int index;
	for (index = 0; index < MAXPUNCTPAIRS; index++)
	{
		m_srcPunctStr[index] = m_editSrcPunct[index]->GetValue();
		m_tgtPunctStr[index] = m_editTgtPunct[index]->GetValue();
	}

	for (index = 0; index < MAXTWOPUNCTPAIRS; index++)
	{
		m_srcTwoPunctStr[index] = m_editSrcTwoPunct[index]->GetValue();
		m_tgtTwoPunctStr[index] = m_editTgtTwoPunct[index]->GetValue();
	}
}

void CPunctCorrespPageCommon::StoreDataIntoEdits()
{
	wxString Temp;
	int index;
	for (index = 0; index < MAXPUNCTPAIRS; index++)
	{
		 m_editSrcPunct[index]->SetValue(m_srcPunctStr[index]);
		 m_editTgtPunct[index]->SetValue(m_tgtPunctStr[index]);
	}

	for (index = 0; index < MAXTWOPUNCTPAIRS; index++)
	{
		 m_editSrcTwoPunct[index]->SetValue(m_srcTwoPunctStr[index]);
		 m_editTgtTwoPunct[index]->SetValue(m_tgtTwoPunctStr[index]);
	}
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! end of common functions !!!!!!!!!!!!!!!!!!!!!!!!!!1

// the CPunctCorrespPageWiz class //////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS( CPunctCorrespPageWiz, wxWizardPage )

// event handler table
BEGIN_EVENT_TABLE(CPunctCorrespPageWiz, wxWizardPage)
	EVT_INIT_DIALOG(CPunctCorrespPageWiz::InitDialog)
    EVT_WIZARD_PAGE_CHANGING(-1, CPunctCorrespPageWiz::OnWizardPageChanging) // handles MFC's OnWizardNext() and OnWizardBack
#ifdef _UNICODE
	EVT_BUTTON(IDC_TOGGLE_UNNNN_BTN, CPunctCorrespPageWiz::OnBnClickedToggleUnnnn)
#endif
    EVT_WIZARD_CANCEL(-1, CPunctCorrespPageWiz::OnWizardCancel)
END_EVENT_TABLE()

CPunctCorrespPageWiz::CPunctCorrespPageWiz()
{
}

CPunctCorrespPageWiz::CPunctCorrespPageWiz(wxWizard* parent) // dialog constructor
{
	Create( parent );

	// Note: This constructor is only called when the new CPunctCorrespPageWiz statement is
	// encountered in the App before the wizard displays. It is not called
	// when the page changes to/from the docPage.

	punctPgCommon.DoSetDataAndPointers();
}

CPunctCorrespPageWiz::~CPunctCorrespPageWiz() // destructor
{
}

bool CPunctCorrespPageWiz::Create( wxWizard* parent)
{
	wxWizardPage::Create( parent );
	CreateControls();
	return TRUE;
}

void CPunctCorrespPageWiz::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	punctPgCommon.pPunctCorrespPageSizer = PunctCorrespPageFunc(this, TRUE, TRUE);
	//m_scrolledWindow = new wxScrolledWindow( this, -1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxHSCROLL|wxVSCROLL );
	//m_scrolledWindow->SetSizer(punctPgCommon.pPunctCorrespPageSizer);
}

// implement wxWizardPage functions
wxWizardPage* CPunctCorrespPageWiz::GetPrev() const 
{ 
	// add code here to determine the previous page to show in the wizard
	return pFontPageWiz; 
}
wxWizardPage* CPunctCorrespPageWiz::GetNext() const
{
	// add code here to determine the next page to show in the wizard
    return pCaseEquivPageWiz;
}

void CPunctCorrespPageWiz::OnWizardCancel(wxWizardEvent& WXUNUSED(event))
{
	// This OnWizardCancel is only called when cancel is hit
	// while on the docPage.
    //if ( wxMessageBox(_T("Do you really want to cancel?"), _T("Question"),
    //                    wxICON_QUESTION | wxYES_NO, this) != wxYES )
    //{
    //    // not confirmed
    //    event.Veto();
    //}
	gpApp->LogUserAction(_T("In PunctCorrespPage: user Cancel from wizard"));
}

// This InitDialog is called from the DoStartWorkingWizard() function
// in the App
void CPunctCorrespPageWiz::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	punctPgCommon.DoInit(); // we dont use Init() because it is a method of wxPanel itself
}

// Prevent leaving the page if either the source or target language
// names are left blank

// the following function is only needed in the wizard's punct corresp page
void CPunctCorrespPageWiz::OnWizardPageChanging(wxWizardEvent& event)
// This code taken from MFC's CPunctMap::OnOK() function
{
	// Can put any code that needs to execute regardless of whether
	// Next or Prev button was pushed here.

	// Determine which direction we're going and implement
	// the MFC equivalent of OnWizardNext() and OnWizardBack() here
	bool bMovingForward = event.GetDirection();
	// The remainder of OnWizardPageChanging below was taken from 
	// MFC's OnEditPunctCorresp() method, the part after the 
	// DoEditPunctCorresp() call.
	//CAdapt_ItApp* pApp = &wxGetApp();
	//wxASSERT(pApp != NULL);

	if (bMovingForward)
	{

#ifdef _UNICODE
		if (!punctPgCommon.m_bShowingChars)
		{
			//UpdateData(TRUE); // get the nnnn numbers into the class variables
			punctPgCommon.GetDataFromEdits();
			punctPgCommon.UpdateAppValues(FALSE); // FALSE means "not from glyphs", ie. class variables contain nnnn
		}
		else
		{
			punctPgCommon.GetDataFromEdits();
			punctPgCommon.UpdateAppValues(TRUE); // TRUE means "from glyphs", ie. class variables contain punct chars
		}
#else // ANSI compiled version
		punctPgCommon.GetDataFromEdits();
		punctPgCommon.UpdateAppValues(TRUE); // TRUE means the data is punctuation characters, (not encoding values)
#endif
		// whm 21Dec07 removed call to DoPunctuationChanges() below since the NoReparse
		// parameter effectively causes the function to simply return without doing anything.
		// The addition of the calls to UpdateData and UpdateAppValues above is what was
		// needed to effect punctuation changes.
		// Calling of DoPunctuationChanges taken from MFC's CPunctCorrespPageWiz::OnWizardNext()
		//pApp->DoPunctuationChanges(&punctPgCommon, NoReparse); // NoReparse within wizard since no doc is open there
		
		// Movement through wizard pages is sequential - the next page is the caseEquivPageWiz.
		// The pCaseEquivPageWiz's InitDialog need to be called here just before going to it
		wxInitDialogEvent idevent;
		pCaseEquivPageWiz->InitDialog(idevent);
	}
}

#ifdef _UNICODE
void CPunctCorrespPageWiz::OnBnClickedToggleUnnnn(wxCommandEvent& event)
{
	punctPgCommon.OnBnClickedToggleUnnnn(event);
}
#endif

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CPunctCorrespPagePrefs !!!!!!!!!!!!!!!!!!!!!!!!!!!
IMPLEMENT_DYNAMIC_CLASS( CPunctCorrespPagePrefs, wxPanel )

// event handler table
BEGIN_EVENT_TABLE(CPunctCorrespPagePrefs, wxPanel)
	EVT_INIT_DIALOG(CPunctCorrespPagePrefs::InitDialog)
END_EVENT_TABLE()

CPunctCorrespPagePrefs::CPunctCorrespPagePrefs()
{
}

CPunctCorrespPagePrefs::CPunctCorrespPagePrefs(wxWindow* parent) // dialog constructor
{
	Create( parent );

	// Note: This constructor is only called when the new CPunctCorrespPagePrefs statement is
	// encountered in the App before the wizard displays. It is not called
	// when the page changes to/from the docPage.

	punctPgCommon.DoSetDataAndPointers();
}

CPunctCorrespPagePrefs::~CPunctCorrespPagePrefs() // destructor
{
}

bool CPunctCorrespPagePrefs::Create( wxWindow* parent)
{
	wxPanel::Create( parent );
	CreateControls();
	GetSizer()->Fit(this);
	return TRUE;
}

void CPunctCorrespPagePrefs::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	punctPgCommon.pPunctCorrespPageSizer = PunctCorrespPageFunc(this, TRUE, TRUE);
}

// This InitDialog is called from the DoStartWorkingWizard() function
// in the App
void CPunctCorrespPagePrefs::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	punctPgCommon.DoInit(); // we dont use Init() because it is a method of wxPanel itself
}

// OnOK() is used only in the EditPreferencesDlg
void CPunctCorrespPagePrefs::OnOK(wxCommandEvent& WXUNUSED(event))
// This code taken from MFC's CPunctMap::OnOK() function
{
#ifdef _UNICODE
	if (!punctPgCommon.m_bShowingChars)
	{
		//UpdateData(TRUE); // get the nnnn numbers into the class variables
		punctPgCommon.GetDataFromEdits();
		punctPgCommon.UpdateAppValues(FALSE); // FALSE means "not from glyphs", ie. class variables contain nnnn
	}
	else
	{
		//UpdateData(); // get the data up to date in the class's variables
		punctPgCommon.GetDataFromEdits();
		punctPgCommon.UpdateAppValues(TRUE); // TRUE means "from glyphs", ie. class variables contain punct chars
	}
#else // ANSI compiled version
	//UpdateData(); // get the data up to date in the class's variables
	punctPgCommon.GetDataFromEdits();
	punctPgCommon.UpdateAppValues(TRUE); // TRUE means the data is punctuation characters, (not encoding values)
#endif
	// the phrase box might be past the end of the doc, if so, the DoPunctuationChanges
	// call will crash because m_pActivePile is undefined. So put in a safety-first block
	// here (cloned from same code in doc's OnFileSave_Protected() )
	if (gpApp->m_pActivePile == NULL || gpApp->m_nActiveSequNum == -1)
	{
		int sequNumAtEnd = gpApp->GetMaxIndex();
		if (sequNumAtEnd == -1)
		{
			// no document is open, so don't need to so any reconstitutions
			return;
		}
		gpApp->m_pActivePile = gpApp->GetDocument()->GetPile(sequNumAtEnd);
		gpApp->m_nActiveSequNum = sequNumAtEnd;
		wxString boxValue;
		if (gbIsGlossing)
		{
			boxValue = gpApp->m_pActivePile->GetSrcPhrase()->m_gloss;
		}
		else
		{
			boxValue = gpApp->m_pActivePile->GetSrcPhrase()->m_adaption;
			translation = boxValue;
		}
		gpApp->m_targetPhrase = boxValue;
		gpApp->m_pTargetBox->ChangeValue(boxValue);
		gpApp->GetView()->PlacePhraseBox(gpApp->m_pActivePile->GetCell(1),2);
		gpApp->GetView()->Invalidate();
	}

	if (punctPgCommon.AreBeforeAndAfterPunctSetsDifferent())
	{
		gpApp->m_pLayout->m_bPunctuationChanged = TRUE;
		gpApp->DoPunctuationChanges(&punctPgCommon,DoReparse);
	}
}

#ifdef _UNICODE
void CPunctCorrespPagePrefs::OnBnClickedToggleUnnnn(wxCommandEvent& event)
{
	punctPgCommon.OnBnClickedToggleUnnnn(event);
}
#endif

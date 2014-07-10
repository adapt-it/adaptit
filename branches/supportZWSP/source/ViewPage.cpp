/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ViewPage.cpp
/// \author			Bill Martin
/// \date_created	17 August 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CViewPage class. 
/// The CViewPage class creates a wxPanel that allows the 
/// user to define the various view parameters. 
/// The panel becomes a "View" tab of the EditPreferencesDlg.
/// The interface resources are loaded by means of the ViewPageFunc()
/// function which was developed and is maintained by wxDesigner.
/// \derivation		The CViewPage class is derived from wxPanel.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ViewPage.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ViewPage.h"
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
#include <wx/colordlg.h>
#include "ViewPage.h"
#include "Adapt_It.h"
#include "MainFrm.h"
#include "Pile.h"
#include "Layout.h"

/// This global is defined in Adapt_ItView.cpp.
extern short gnExpandBox;

IMPLEMENT_DYNAMIC_CLASS( CViewPage, wxPanel )

// event handler table
BEGIN_EVENT_TABLE(CViewPage, wxPanel)
	EVT_INIT_DIALOG(CViewPage::InitDialog)
	EVT_BUTTON(IDC_BUTTON_CHOOSE_HIGHLIGHT_COLOR, CViewPage::OnButtonHighlightColor)
	EVT_CHECKBOX(IDC_CHECK_SHOW_ADMIN_MENU, CViewPage::OnCheckShowAdminMenu)
	EVT_CHECKBOX(ID_CHECKBOX_ENABLE_INSERT_ZWSP, CViewPage::OnCheckboxEnableInsertZWSP)
	EVT_RADIOBOX(ID_RADIOBOX_SCROLL_INTO_VIEW, CViewPage::OnRadioPhraseBoxMidscreen)
END_EVENT_TABLE()

CViewPage::CViewPage()
{
}

CViewPage::CViewPage(wxWindow* parent) // dialog constructor
{
	Create( parent );

	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	tempMaxToDisplay = 0;
	tempPrecCntxt = 0;
	tempFollCntxt = 0;
	tempLeading = 0;
	tempGapWidth = 0;
	tempLMargin = 0;
	tempMultiplier = gnExpandBox;
	tempDlgFontSize = 12;
	tempSuppressFirst = FALSE;
	tempSuppressLast = FALSE;
	tempMakeWelcomeVisible = TRUE;
	tempHighlightAutoInsertions = TRUE;
	tempShowAdminMenu = pApp->m_bShowAdministratorMenu;

	m_pEditLeading = (wxTextCtrl*)FindWindowById(IDC_EDIT_LEADING);
	m_pEditGapWidth = (wxTextCtrl*)FindWindowById(IDC_EDIT_GAP_WIDTH);
	m_pEditLeftMargin = (wxTextCtrl*)FindWindowById(IDC_EDIT_LEFTMARGIN);
	m_pEditMultiplier = (wxTextCtrl*)FindWindowById(IDC_EDIT_MULTIPLIER);
	m_pEditDlgFontSize = (wxTextCtrl*)FindWindowById(IDC_EDIT_DIALOGFONTSIZE);

	m_pCheckWelcomeVisible = (wxCheckBox*)FindWindowById(IDC_CHECK_WELCOME_VISIBLE);
	m_pCheckHighlightAutoInsertedTrans = (wxCheckBox*)FindWindowById(IDC_CHECK_HIGHLIGHT_AUTO_INSERTED_TRANSLATIONS);
	m_pPanelAutoInsertColor = (wxPanel*)FindWindowById(ID_PANEL_AUTO_INSERT_COLOR);
	m_pCheckShowAdminMenu = (wxCheckBox*)FindWindowById(IDC_CHECK_SHOW_ADMIN_MENU);
	m_pRadioBox = (wxRadioBox*)FindWindowById(ID_RADIOBOX_SCROLL_INTO_VIEW);
	m_pCheckboxEnableInsertZWSP = (wxCheckBox*)FindWindowById(ID_CHECKBOX_ENABLE_INSERT_ZWSP);
}

CViewPage::~CViewPage() // destructor
{
}

bool CViewPage::Create( wxWindow* parent)
{
	wxPanel::Create( parent );
	CreateControls();
	GetSizer()->Fit(this);
	return TRUE;
}

void CViewPage::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pViewPageSizer = ViewPageFunc(this, TRUE, TRUE);
}

void CViewPage::OnButtonHighlightColor(wxCommandEvent& WXUNUSED(event)) 
{
	// change auto-insertions highlight colour

	wxColourData colorData;
	colorData.SetColour(tempAutoInsertionsHighlightColor);
	colorData.SetChooseFull(TRUE);
	wxColourDialog colorDlg(this,&colorData);
	colorDlg.Centre();
	if(colorDlg.ShowModal() == wxID_OK)
	{
		// BEW 22Aug09 fixed failure to change colour
		colorData = colorDlg.GetColourData();
		tempAutoInsertionsHighlightColor  = colorData.GetColour();
		m_pPanelAutoInsertColor->SetBackgroundColour(tempAutoInsertionsHighlightColor);
		m_pPanelAutoInsertColor->Refresh();
	}	
}

void CViewPage::OnCheckboxEnableInsertZWSP(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	if (pApp->m_bEnableZWSPInsertion)
	{
		// It's on, so turn it back off
		pApp->m_bEnableZWSPInsertion = FALSE;
	}
	else
	{
		// It's off, so turn it on
		pApp->m_bEnableZWSPInsertion = TRUE;
	}
}

void CViewPage::OnCheckShowAdminMenu(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	bool enableMenuCode;
	enableMenuCode = TRUE; 
	//enableMenuCode = FALSE; // **** comment out this line to activate Admin menu handling ****
	if (enableMenuCode == FALSE)
	{
		 //used for 5.1.1 to 5.1.3 "fixed" versions for Bob's Brazil workshop, built on 21Aug09
		 //also used for 5.1.4 bug fix release
		wxMessageBox(_T(
	"This functionality is a work in progress and it is not finished yet. Please wait for next release."),
		_T(""), wxICON_INFORMATION | wxOK);
		m_pCheckShowAdminMenu->SetValue(FALSE);
		 return;
	}
	else
	{
		// This block executed when enableMenuCode is TRUE
		bool bFlag = pApp->m_bShowAdministratorMenu;
		if (bFlag)
		{
			// menu is currently shown and administrator wants it hidden, no password required
			// for hiding it
			pApp->m_bShowAdministratorMenu = FALSE;

			// code for removing the menu and updating the menu bar is in
			// the OnEditPreferences() handler in the view class
		}
		else
		{
			// someone wants to have the administrator menu made visible, this requires a
			// password and we always accept the secret default "admin" password; note,
			// although we won't document the fact, anyone can type an arbitrary password
			// string in the relevant line of the basic configuration file, and the code below
			// will accept it when next that config file is read in - ie. at next launch
			wxString message = _("Access to Administrator privileges requires that you type a password");
			wxString caption = _("Type Administrator Password");
			wxString default_value = _T("");
			wxString password = ::wxGetPasswordFromUser(message,caption,default_value,this); 
			if (password == _T("admin") || 
				(password == pApp->m_adminPassword && !pApp->m_adminPassword.IsEmpty()))
			{
				// a valid password was typed
				pApp->m_bShowAdministratorMenu = TRUE;
				
				// code for installing the menu and updating the menu bar is in
				// the OnEditPreferences() handler in the view class
			}
			else
			{
				// invalid password - turn the checkbox back off, beep also
				::wxBell();
				m_pCheckShowAdminMenu->SetValue(FALSE);
			}
		}
		// whm 20Jul11 added: Even though the "Show Administrator Menu... (Password 
		// protected)" menu item is a toggle menu item, and would normally toggle its 
		// own state each time it is invoked, we need to ensure its toggle state is in 
		// sync with the internal toggling of the App's m_bShowAdministratorMenu flag 
		// - which can be set either here or in the Edit | Preferences... | View tab. 
		// Therefore we explicitly set the menu's toggle state both here and in the 
		// other View tab location.
		wxMenuBar* pMenuBar = pApp->GetMainFrame()->GetMenuBar();
		pMenuBar->Check(ID_VIEW_SHOW_ADMIN_MENU,pApp->m_bShowAdministratorMenu);

	}
}

// MFC's OnSetActive() has no direct equivalent in wxWidgets. 
// It would not be needed in any case since in our
// design InitDialog is moved to public and called once 
// in the App where the wizard pages are constructed before 
// wizard itself starts.

// ViewPage not used in any wizards, only as tab in notebook dialogs
//void CViewPage::OnWizardPageChanging(wxWizardEvent& event)
//{
//}

void CViewPage::OnOK(wxCommandEvent& WXUNUSED(event))
{
	// Notes: Any changes made to OnOK should also be made to 
	// OnWizardPageChanging above.
	// In DoStartWorkingWizard, CViewPage::OnWizardPageChanging() 
	// is called.
	// Validation of the language page data should be done in the caller's
	// OnOK() method before calling CViewPage::OnOK().

	// User pressed OK so assume user wants to store the dialog's values.
	// put the source & target language names in storage on the App
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

	// Transfer values from controls to vars and save new values back on the App
	int nVal;
	wxString strTemp;

	/* refactored 22Mar09, this value no longer needed now we have no bundles
	// so set it to a value which we can output in the config file safely but not use
	strTemp = m_pEditMaxSrcWordsDisplayed->GetValue();
	nVal = wxAtoi(strTemp);
	pApp->m_nMaxToDisplay = nVal;
	*/
	pApp->m_nMaxToDisplay = pApp->GetMaxIndex() + 1; // count of CSourcePhrase instances

	/* refactored 22Mar09, this value no longer needed now we have no bundles
	// so set it to a value which we can output in the config file safely but not use
	strTemp = m_pEditMinPrecContext->GetValue();
	nVal = wxAtoi(strTemp);
	pApp->m_nPrecedingContext = nVal;
	*/
	pApp->m_nPrecedingContext = 30; // arbitrary const value, we no longer use it

	/* refactored 22Mar09, this value no longer needed now we have no bundles
	// so set it to a value which we can output in the config file safely but not use
	strTemp = m_pEditMinFollContext->GetValue();
	nVal = wxAtoi(strTemp);
	pApp->m_nFollowingContext = nVal;
	*/
	pApp->m_nFollowingContext = 40; // arbitrary const value, we no longer use it

	// BEW added 4Jun09; various lines and tests for refactored view layout support
	CLayout* pLayout = pApp->m_pLayout;
	pLayout->m_bViewParamsChanged = FALSE; // start by assuming the user made no changes

	strTemp = m_pEditLeading->GetValue();
	nVal = wxAtoi(strTemp);
	if (nVal != pApp->m_curLeading)
		pLayout->m_bViewParamsChanged = TRUE;
	pApp->m_curLeading = nVal;

	strTemp = m_pEditGapWidth->GetValue();
	nVal = wxAtoi(strTemp);
	if (nVal != pApp->m_curGapWidth)
		pLayout->m_bViewParamsChanged = TRUE;
	pApp->m_curGapWidth = nVal;

	strTemp = m_pEditLeftMargin->GetValue();
	nVal = wxAtoi(strTemp);
	if (nVal != pApp->m_curLMargin)
		pLayout->m_bViewParamsChanged = TRUE;
	pApp->m_curLMargin = nVal;

	strTemp = m_pEditMultiplier->GetValue();
	nVal = wxAtoi(strTemp);
	if (nVal != gnExpandBox)
		pLayout->m_bViewParamsChanged = TRUE;
	gnExpandBox = nVal;

	strTemp = m_pEditDlgFontSize->GetValue();
	nVal = wxAtoi(strTemp);
	pApp->m_dialogFontSize = nVal;

	pApp->m_bSuppressFirst = TRUE; // retain these because the config file expects
	pApp->m_bSuppressLast = TRUE; // them, but we won't use these values any more
	pApp->m_bSuppressWelcome = !m_pCheckWelcomeVisible->GetValue();
	pApp->m_bSuppressTargetHighlighting = !m_pCheckHighlightAutoInsertedTrans->GetValue();
	pApp->m_AutoInsertionsHighlightColor = tempAutoInsertionsHighlightColor;
	pApp->m_bShowAdministratorMenu = m_pCheckShowAdminMenu->GetValue();
}

void CViewPage::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

	// initialize our local temp variables from those on the App
	tempLeading = pApp->m_curLeading;
	tempGapWidth = pApp->m_curGapWidth;
	tempLMargin = pApp->m_curLMargin;
	tempMultiplier = gnExpandBox;
	tempDlgFontSize = pApp->m_dialogFontSize; // added missed initialization
	tempMakeWelcomeVisible = !pApp->m_bSuppressWelcome;
	tempUseStartupWizardOnLaunch = pApp->m_bUseStartupWizardOnLaunch; // always remains true since version 3
	tempHighlightAutoInsertions = !pApp->m_bSuppressTargetHighlighting;
	tempAutoInsertionsHighlightColor = pApp->m_AutoInsertionsHighlightColor;
	int nRadioBoxSelection = pApp->m_bKeepBoxMidscreen ? 0 : 1;
	m_pRadioBox->SetSelection(nRadioBoxSelection);
	// BEW added 9Jul14
	m_pCheckboxEnableInsertZWSP->SetValue(pApp->m_bEnableZWSPInsertion);

	// transfer initial values to controls
	wxString strTemp;
	strTemp.Empty();

	strTemp.Empty();
	strTemp << tempLeading;
	m_pEditLeading->SetValue(strTemp);

	strTemp.Empty();
	strTemp << tempGapWidth;
	m_pEditGapWidth->SetValue(strTemp);

	strTemp.Empty();
	strTemp << tempLMargin;
	m_pEditLeftMargin->SetValue(strTemp);

	strTemp.Empty();
	strTemp << tempMultiplier;
	m_pEditMultiplier->SetValue(strTemp);

	strTemp.Empty();
	strTemp << tempDlgFontSize;
	m_pEditDlgFontSize->SetValue(strTemp);

	// next two are no longer used, BEW 4Jun09
	m_pCheckWelcomeVisible->SetValue(tempMakeWelcomeVisible);
	m_pCheckHighlightAutoInsertedTrans->SetValue(tempHighlightAutoInsertions);
	m_pCheckShowAdminMenu->SetValue(tempShowAdminMenu);

	m_pPanelAutoInsertColor->SetBackgroundColour(tempAutoInsertionsHighlightColor);

	// Since most users won't likely want any particular setting in this
	// panel, we won't set focus to any particular control.
}

void  CViewPage::OnRadioPhraseBoxMidscreen(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

	// Get the new state of the radiobox
	int nRadioBoxSelection = m_pRadioBox->GetSelection();
	// make the scroll-into-view regime match the new setting
	if (nRadioBoxSelection == 0)
	{
		// This is the first button, the one for the phrase box staying midscreen
		pApp->m_bKeepBoxMidscreen = TRUE;
	}
	else
	{
		// This is the second button, the one for the phrase box doing the moves down the
		// strips as the user works, the strips not moving vertically until the box nears
		// the bottom of the client area
		pApp->m_bKeepBoxMidscreen = FALSE;
	}
}



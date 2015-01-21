/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AutoSavingPage.cpp
/// \author			Bill Martin
/// \date_created	18 August 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CAutoSavingPage class. 
/// The CAutoSavingPage class creates a wxPanel that allows the 
/// user to define the various document and knowledge base saving parameters. 
/// The panel becomes a "Auto-Saving" tab of the EditPreferencesDlg.
/// The interface resources are loaded by means of the AutoSavingPageFunc()
/// function which was developed and is maintained by wxDesigner.
/// \derivation		The CAutoSavingPage class is derived from wxPanel.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in AutoSavingPage.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "AutoSavingPage.h"
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
#include "AutoSavingPage.h"

#include "Adapt_It.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp*	gpApp; // if we want to access it fast

IMPLEMENT_DYNAMIC_CLASS( CAutoSavingPage, wxPanel )

// event handler table
BEGIN_EVENT_TABLE(CAutoSavingPage, wxPanel)
	EVT_INIT_DIALOG(CAutoSavingPage::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_RADIOBUTTON(IDC_RADIO_BY_MINUTES, CAutoSavingPage::OnRadioByMinutes)
	EVT_RADIOBUTTON(IDC_RADIO_BY_MOVES, CAutoSavingPage::OnRadioByMoves)
	EVT_CHECKBOX(IDC_CHECK_NO_AUTOSAVE, CAutoSavingPage::OnCheckNoAutoSave)

	//BEW added 21Aug09, as spin buttons were not connected to their text controls
	EVT_SPIN_UP(IDC_SPIN_MINUTES, CAutoSavingPage::OnSpinUpMinutes)
	EVT_SPIN_DOWN(IDC_SPIN_MINUTES, CAutoSavingPage::OnSpinDownMinutes)
	EVT_SPIN_UP(IDC_SPIN_MOVES, CAutoSavingPage::OnSpinUpMoves)
	EVT_SPIN_DOWN(IDC_SPIN_MOVES, CAutoSavingPage::OnSpinDownMoves)
	EVT_SPIN_UP(IDC_SPIN_KB_MINUTES, CAutoSavingPage::OnSpinUpKBMinutes)
	EVT_SPIN_DOWN(IDC_SPIN_KB_MINUTES, CAutoSavingPage::OnSpinDownKBMinutes)
END_EVENT_TABLE()


CAutoSavingPage::CAutoSavingPage()
{
}

CAutoSavingPage::CAutoSavingPage(wxWindow* parent) // dialog constructor
{
	Create( parent );

	tempNoAutoSave = FALSE;
	tempKBMinutes = 0;
	tempMinutes = 0;
	tempMoves = 0;
	tempSeconds = 0;
	tempIsDocTimeButton = TRUE;

	m_pCheckNoAutoSave = (wxCheckBox*)FindWindowById(IDC_CHECK_NO_AUTOSAVE);
	wxASSERT(m_pCheckNoAutoSave != NULL);
	m_pRadioByMinutes = (wxRadioButton*)FindWindowById(IDC_RADIO_BY_MINUTES);
	wxASSERT(m_pRadioByMinutes != NULL);

	m_pRadioByMoves = (wxRadioButton*)FindWindowById(IDC_RADIO_BY_MOVES);
	wxASSERT(m_pRadioByMoves != NULL);

	m_pEditMinutes = (wxTextCtrl*)FindWindowById(IDC_EDIT_MINUTES);
	wxASSERT(m_pEditMinutes != NULL);

	m_pEditMoves = (wxTextCtrl*)FindWindowById(IDC_EDIT_MOVES);
	wxASSERT(m_pEditMoves != NULL);

	m_pEditKBMinutes = (wxTextCtrl*)FindWindowById(IDC_EDIT_KB_MINUTES);
	wxASSERT(m_pEditKBMinutes != NULL);

	m_pTextCtrlAsStatic = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_AUTOSAVE);
	wxASSERT(m_pTextCtrlAsStatic != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	//m_pTextCtrlAsStatic->SetBackgroundColour(backgrndColor);
	m_pTextCtrlAsStatic->SetBackgroundColour(gpApp->sysColorBtnFace);

	m_pSpinMinutes = (wxSpinButton*)FindWindowById(IDC_SPIN_MINUTES);
	wxASSERT(m_pSpinMinutes != NULL);

	m_pSpinMoves = (wxSpinButton*)FindWindowById(IDC_SPIN_MOVES);
	wxASSERT(m_pSpinMoves != NULL);

	m_pSpinKBMinutes = (wxSpinButton*)FindWindowById(IDC_SPIN_KB_MINUTES);
	wxASSERT(m_pSpinKBMinutes != NULL);
}

CAutoSavingPage::~CAutoSavingPage() // destructor
{
}

bool CAutoSavingPage::Create( wxWindow* parent)
{
	wxPanel::Create( parent );
	CreateControls();
	GetSizer()->Fit(this);
	return TRUE;
}

void CAutoSavingPage::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pAutoSavingPageSizer = AutoSavingPageFunc(this, TRUE, TRUE);
}

void CAutoSavingPage::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	wxCommandEvent dummyevent;
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

	// Set up the spin buttons - In WX version range is set in the resources via wxDesigner

	// initialize our local temp variables from those on the App
	tempNoAutoSave = pApp->m_bNoAutoSave;
#ifdef Test_m_bNoAutoSave
	// Test_m_bNoAutoSave symbol #defined at start of Adapt_It.h
	#ifdef _DEBUG
		if (tempNoAutoSave)
		{
		wxLogDebug(_T("m_bNoAutoSave  in InitDialog() of AutoSavingPage.cpp; tempNoAutoSave is TRUE and value transferred to checkbox control"));
		}
		else
		{
		wxLogDebug(_T("m_bNoAutoSave  in InitDialog() of AutoSavingPage.cpp; tempNoAutoSave is FALSE and value transferred to checkbox control"));
		}
	#endif
#endif
	tempKBMinutes = pApp->m_timeSettings.m_tsKB.GetMinutes();
	tempMinutes = pApp->m_timeSettings.m_tsDoc.GetMinutes();
	tempMoves = pApp->m_nMoves;
	tempIsDocTimeButton = pApp->m_bIsDocTimeButton;

	// initialize spin buttons
	m_pSpinMinutes->SetValue(tempMinutes);
	m_pSpinMoves->SetValue(tempMoves);
	m_pSpinKBMinutes->SetValue(tempKBMinutes);

	// initialize radio buttons
	if (tempIsDocTimeButton)
	{
		m_pRadioByMinutes->SetValue(TRUE);
		m_pRadioByMoves->SetValue(FALSE);
		OnRadioByMinutes(dummyevent);
	}
	else
	{
		m_pRadioByMinutes->SetValue(FALSE);
		m_pRadioByMoves->SetValue(TRUE);
		OnRadioByMoves(dummyevent);
	}

	// transfer initial values to controls
	m_pCheckNoAutoSave->SetValue(tempNoAutoSave);
    // whm 11Jun09 moved OnCheckNoAutoSave() from above to ensure control has the updated
    // value before OnCheckNoAutoSave() is called.
	OnCheckNoAutoSave(dummyevent);
	m_pRadioByMinutes->SetValue(tempIsDocTimeButton);
	m_pRadioByMoves->SetValue(!tempIsDocTimeButton);
	wxString strTemp;
	strTemp.Empty();
	strTemp << tempMinutes;
	m_pEditMinutes->SetValue(strTemp);

	strTemp.Empty();
	strTemp << tempMoves;
	m_pEditMoves->SetValue(strTemp);

	strTemp.Empty();
	strTemp << tempKBMinutes;
	m_pEditKBMinutes->SetValue(strTemp);

	// Since most users won't likely want any particular setting in this
	// panel, we won't set focus to any particular control.
}

void CAutoSavingPage::OnCheckNoAutoSave(wxCommandEvent& WXUNUSED(event))
{
	tempNoAutoSave = m_pCheckNoAutoSave->IsChecked();
	if (tempNoAutoSave)
	{
		// turn off the edit boxes
		EnableAll(FALSE);
	}
	else
	{
		// turn on the required edit boxes
		EnableAll(TRUE);
	}
}

//BEW added 21Aug09, as spin buttons were not connected to their text controls
void CAutoSavingPage::OnSpinUpMinutes(wxSpinEvent& WXUNUSED(event))
{
	int max = m_pSpinMinutes->GetMax();
	tempMinutes = m_pSpinMinutes->GetValue();
	wxString num;
	if (tempMinutes < max)
		tempMinutes += 1;
	else
		::wxBell();
	num << tempMinutes;
	m_pEditMinutes->ChangeValue(num);
}

void CAutoSavingPage::OnSpinDownMinutes(wxSpinEvent& WXUNUSED(event))
{
	int min = m_pSpinMinutes->GetMin();
	tempMinutes = m_pSpinMinutes->GetValue();
	wxString num;
	if (tempMinutes > min)
		tempMinutes -= 1;
	else
		::wxBell();
	num << tempMinutes;
	m_pEditMinutes->ChangeValue(num);
}

void CAutoSavingPage::OnSpinUpMoves(wxSpinEvent& WXUNUSED(event))
{
	int max = m_pSpinMoves->GetMax();
	tempMinutes = m_pSpinMoves->GetValue();
	wxString num;
	if (tempMoves < max)
		tempMoves += 1;
	else
		::wxBell();
	num << tempMoves;
	m_pEditMoves->ChangeValue(num);
}

void CAutoSavingPage::OnSpinDownMoves(wxSpinEvent& WXUNUSED(event))
{
	int min = m_pSpinMoves->GetMin();
	tempMinutes = m_pSpinMoves->GetValue();
	wxString num;
	if (tempMoves > min)
		tempMoves -= 1;
	else
		::wxBell();
	num << tempMoves;
	m_pEditMoves->ChangeValue(num);
}

void CAutoSavingPage::OnSpinUpKBMinutes(wxSpinEvent& WXUNUSED(event))
{
	int max = m_pSpinKBMinutes->GetMax();
	tempKBMinutes = m_pSpinKBMinutes->GetValue();
	wxString num;
	if (tempKBMinutes < max)
		tempKBMinutes += 1;
	else
		::wxBell();
	num << tempKBMinutes;
	m_pEditKBMinutes->ChangeValue(num);
}

void CAutoSavingPage::OnSpinDownKBMinutes(wxSpinEvent& WXUNUSED(event))
{
	int min = m_pSpinKBMinutes->GetMin();
	tempKBMinutes = m_pSpinKBMinutes->GetValue();
	wxString num;
	if (tempKBMinutes > min)
		tempKBMinutes -= 1;
	else
		::wxBell();
	num << tempKBMinutes;
	m_pEditKBMinutes->ChangeValue(num);
}

/*
	wxTextCtrl* m_pEditMinutes;
	wxTextCtrl*	m_pEditMoves;
	wxTextCtrl* m_pEditKBMinutes;
*/
void CAutoSavingPage::OnRadioByMinutes(wxCommandEvent& WXUNUSED(event)) 
{
	m_pEditMinutes->Enable();
	m_pEditMoves->Enable(FALSE);
	tempIsDocTimeButton = TRUE;
}

void CAutoSavingPage::OnRadioByMoves(wxCommandEvent& WXUNUSED(event)) 
{
	m_pEditMinutes->Enable(FALSE);
	m_pEditMoves->Enable();
	tempIsDocTimeButton = FALSE;
}

void CAutoSavingPage::EnableAll(bool bEnable)
{
	if (bEnable)
	{
		m_pRadioByMinutes->Enable();
		m_pRadioByMoves->Enable();
		m_pEditKBMinutes->Enable();
		if (tempIsDocTimeButton)
		{
			m_pEditMinutes->Enable();
			m_pEditMoves->Enable(FALSE);
		}
		else
		{
			m_pEditMinutes->Enable(FALSE);
			m_pEditMoves->Enable(TRUE);
		}
	}
	else
	{
		// disable all the edit boxes
		m_pRadioByMinutes->Enable(FALSE);
		m_pRadioByMoves->Enable(FALSE);
		m_pEditMinutes->Enable(FALSE);
		m_pEditMoves->Enable(FALSE);
		m_pEditKBMinutes->Enable(FALSE);
	}
}

// MFC's OnSetActive() has no direct equivalent in wxWidgets. 
// It would not be needed in any case since in our
// design InitDialog is moved to public and called once 
// in the App where the wizard pages are constructed before 
// wizard itself starts.

// AutoSavingPage not used in any wizards, only as tab in notebook dialogs

void CAutoSavingPage::OnOK(wxCommandEvent& WXUNUSED(event))
{
	// Validation of the language page data should be done in the caller's
	// OnOK() method before calling CAutoSavingPage::OnOK().

	// User pressed OK so assume user wants to store the dialog's values.
	// put the source & target language names in storage on the App
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

	// Transfer values from controls to vars and save new values back on the App
#ifdef Test_m_bNoAutoSave
	// Test_m_bNoAutoSave symbol #defined at start of Adapt_It.h
	#ifdef _DEBUG
		wxLogDebug(_T("m_bNoAutoSave  in OnOK() of AutoSavingPage.cpp; checkbox has value %d"), (int)m_pCheckNoAutoSave->IsChecked());
	#endif
#endif
	pApp->m_bNoAutoSave = m_pCheckNoAutoSave->IsChecked();
#ifdef Test_m_bNoAutoSave
	// Test_m_bNoAutoSave symbol #defined at start of Adapt_It.h
	#ifdef _DEBUG
	if (pApp->m_bNoAutoSave)
		wxLogDebug(_T("m_bNoAutoSave  in OnOK() of AutoSavingPage.cpp; value saved to m_pNoAutoSave in Adapt_It.cpp was TRUE"));
	else
		wxLogDebug(_T("m_bNoAutoSave  in OnOK() of AutoSavingPage.cpp; value saved to m_pNoAutoSave in Adapt_It.cpp was FALSE"));
	#endif
#endif
	pApp->m_bIsDocTimeButton = m_pRadioByMinutes->GetValue();

	int nVal;
	wxString strTemp;
	strTemp = m_pEditMinutes->GetValue();
	nVal = wxAtoi(strTemp);
	wxTimeSpan tsDoc(0,nVal,0,0); // 0 hours, 10 minutes, 0 seconds, 0 milliseconds - wxWidgets
	pApp->m_timeSettings.m_tsDoc = tsDoc;

	strTemp = m_pEditKBMinutes->GetValue();
	nVal = wxAtoi(strTemp);
	wxTimeSpan tsKB(0,nVal,0,0); // 0 hours, 15 minutes, 0 seconds, 0 milliseconds - wxWidgets
	pApp->m_timeSettings.m_tsKB = tsKB;

	strTemp = m_pEditMoves->GetValue();
	nVal = wxAtoi(strTemp);
	pApp->m_nMoves = nVal;
}




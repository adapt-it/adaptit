/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ConsistencyCheckDlg.cpp
/// \author			Bill Martin
/// \date_created	11 July 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CConsistencyCheckDlg class. 
/// The CConsistencyCheckDlg class allows the user to respond to inconsistencies between
/// the adaptations stored in the document and those stored in the knowledge base. In the
/// dialog the user may accept what is in the document as a KB entry, type a new entry to
/// be stored in the document and KB, accept <no adaptation>, or ignore the inconsistency
/// (to be fixed later). For any fixes the user can also check a box to "Auto-fix later
/// instances the same way." 
/// \derivation		The CConsistencyCheckDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ConsistencyCheckDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ConsistencyCheckDlg.h"
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
#include "ConsistencyCheckDlg.h"
#include "TargetUnit.h"
#include "Adapt_ItView.h"
#include "Adapt_ItCanvas.h"
#include "MainFrm.h"
#include "KB.h"
#include "SourcePhrase.h"
#include "RefString.h"
#include "helpers.h"

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

// This global is defined in Adapt_ItView.cpp.
//extern bool	gbEnableGlossing; // TRUE makes Adapt It revert to Shoebox functionality only

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

//extern bool gbRemovePunctuationFromGlosses;

extern bool gbIgnoreIt; // used in CAdapt_ItView::DoConsistencyCheck
						// when the "Ignore it, I will fix it later" button was hit in the dialog

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CConsistencyCheckDlg, AIModalDialog)
	EVT_INIT_DIALOG(CConsistencyCheckDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_OK, CConsistencyCheckDlg::OnOK)
	EVT_RADIOBUTTON(IDC_RADIO_LIST_SELECT, CConsistencyCheckDlg::OnRadioListSelect)
	EVT_RADIOBUTTON(IDC_RADIO_ACCEPT_CURRENT, CConsistencyCheckDlg::OnRadioAcceptCurrent)
	EVT_RADIOBUTTON(IDC_RADIO_TYPE_NEW, CConsistencyCheckDlg::OnRadioTypeNew)
	EVT_LISTBOX(IDC_LIST_TRANSLATIONS, CConsistencyCheckDlg::OnSelchangeListTranslations)
	EVT_SET_FOCUS(CConsistencyCheckDlg::OnSetfocusEditTypeNew)
	EVT_BUTTON(IDC_NOTHING, CConsistencyCheckDlg::OnButtonNoAdaptation)
	EVT_BUTTON(IDC_BUTTON_IGNORE_IT, CConsistencyCheckDlg::OnButtonIgnoreIt)
END_EVENT_TABLE()


CConsistencyCheckDlg::CConsistencyCheckDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Inconsistency Found"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	ConsistencyCheckDlgFunc(this, TRUE, TRUE);
	// The declaration is: ConsistencyCheckDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	m_keyStr = _T("");
	m_adaptationStr = _T("");
	m_newStr = _T("");
	m_bDoAutoFix = FALSE;
	m_chVerse = _T("");
	
	// Get pointers to the dialog's controls
	m_pEditCtrlChVerse = (wxTextCtrl*)FindWindowById(IDC_EDIT_CH_VERSE); 
	wxASSERT(m_pEditCtrlChVerse != NULL);
	m_pEditCtrlChVerse->SetBackgroundColour(gpApp->sysColorBtnFace); // read only background color
	
	m_pListBox = (wxListBox*)FindWindowById(IDC_LIST_TRANSLATIONS);
	wxASSERT(m_pListBox != NULL);
	
	m_pEditCtrlNew = (wxTextCtrl*)FindWindowById(IDC_EDIT_TYPE_NEW);
	wxASSERT(m_pEditCtrlNew != NULL);
	
	m_pEditCtrlAdaptation = (wxTextCtrl*)FindWindowById(IDC_EDIT_ADAPTATION); 
	wxASSERT(m_pEditCtrlAdaptation != NULL);
	m_pEditCtrlAdaptation->SetBackgroundColour(gpApp->sysColorBtnFace); // read only background color
	
	m_pEditCtrlKey = (wxTextCtrl*)FindWindowById(IDC_EDIT_KEY);
	wxASSERT(m_pEditCtrlKey != NULL);
	m_pEditCtrlKey->SetBackgroundColour(gpApp->sysColorBtnFace); // read only background color

	// use wxValidator for simple dialog data transfer
	m_pEditCtrlChVerse->SetValidator(wxGenericValidator(&m_chVerse));
	m_pEditCtrlNew->SetValidator(wxGenericValidator(&m_newStr));
	m_pEditCtrlAdaptation->SetValidator(wxGenericValidator(&m_adaptationStr));
	m_pEditCtrlKey->SetValidator(wxGenericValidator(&m_keyStr));

}

CConsistencyCheckDlg::~CConsistencyCheckDlg() // destructor
{
	
}

void CConsistencyCheckDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	wxString s;
	s = _("<no adaptation>"); //IDS_NO_ADAPTATION that is, "<no adaptation>" 
	gbIgnoreIt = FALSE; // default

	// first, use the current source and target language fonts for the list box
	// and edit boxes (in case there are special characters)
	// make the fonts show user's desired point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pEditCtrlKey, NULL,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pEditCtrlKey, NULL, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif

	if (gbIsGlossing && gbGlossingUsesNavFont)
	{
		#ifdef _RTL_FLAGS
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, m_pEditCtrlAdaptation, m_pEditCtrlNew,
									m_pListBox, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bNavTextRTL);
		#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, m_pEditCtrlAdaptation, m_pEditCtrlNew, 
									m_pListBox, NULL, gpApp->m_pDlgTgtFont);
		#endif
	}
	else
	{
		#ifdef _RTL_FLAGS
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pEditCtrlAdaptation, m_pEditCtrlNew,
									m_pListBox, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bTgtRTL);
		#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pEditCtrlAdaptation, m_pEditCtrlNew, 
									m_pListBox, NULL, gpApp->m_pDlgTgtFont);
		#endif
	}

	// put the key and its translation, or gloss, in their boxes
	// the dialog class's m_adaptationStr can hold either an adaptation, or a gloss - the
	// latter will be the case when glossing is ON
	m_keyStr = m_pSrcPhrase->m_key;
	if (gbIsGlossing)
		m_adaptationStr = m_pSrcPhrase->m_gloss;
	else
		m_adaptationStr = m_pSrcPhrase->m_adaption;

	// fill the listBox, if there is a targetUnit available
	if (m_bFoundTgtUnit)
	{
		wxASSERT(m_pTgtUnit != NULL);
		CRefString* pRefString;
		TranslationsList::Node* pos = m_pTgtUnit->m_pTranslations->GetFirst();
		wxASSERT(pos != NULL);
		while (pos != NULL)
		{
			pRefString = (CRefString*)pos->GetData();
			pos = pos->GetNext();
			wxString str = pRefString->m_translation;
			if (str.IsEmpty())
			{
				str = s; // user needs to be able to see a "<no adaptation>" entry
			}
			m_pListBox->Append(str);
			// m_pListBox is not sorted but it is safest to call FindListBoxItem before calling SetClientData
			int nNewSel = gpApp->FindListBoxItem(m_pListBox, str, caseSensitive, exactString);
			wxASSERT(nNewSel != -1); // we just added it so it must be there!
			m_pListBox->SetClientData(nNewSel,pRefString);
		}

		// select the first string in the listbox by default
		m_pListBox->SetSelection(0,TRUE);

	}
	else
	{
		// the listBox will be empty, so disable the top of the 3 radio buttons
		wxRadioButton* pButton = (wxRadioButton*)FindWindowById(IDC_RADIO_LIST_SELECT);
		wxASSERT(pButton != NULL);
		pButton->Enable(FALSE);
	}

	// work out where to place the dialog window
	wxRect rectScreen;
	rectScreen = wxGetClientDisplayRect();

	wxClientDC dc(gpApp->GetMainFrame()->canvas);
	gpApp->GetMainFrame()->canvas->DoPrepareDC(dc);// adjust origin
	gpApp->GetMainFrame()->PrepareDC(dc); // wxWidgets' drawing.cpp sample also calls PrepareDC on the owning frame
	//dc.LPtoDP(&m_ptBoxTopLeft); // now it's device coords
	//int xScrollUnits, yScrollUnits, xOrigin, yOrigin;
	//pApp->GetMainFrame()->canvas->GetViewStart(&xOrigin, &yOrigin); // gets xOrigin and yOrigin in scroll units
	//pApp->GetMainFrame()->canvas->GetScrollPixelsPerUnit(&xScrollUnits, &yScrollUnits); // gets pixels per scroll unit
	//m_ptBoxTopLeft.x = xOrigin * xScrollUnits; // number pixels is ScrollUnits * pixelsPerScrollUnit
	//m_ptBoxTopLeft.y = yOrigin * yScrollUnits;

	int newXPos,newYPos;
	// CalcScrolledPosition translates logical coordinates to device ones. 
	gpApp->GetMainFrame()->canvas->CalcScrolledPosition(m_ptBoxTopLeft.x,m_ptBoxTopLeft.y,&newXPos,&newYPos);
	m_ptBoxTopLeft.x = newXPos;
	m_ptBoxTopLeft.y = newYPos;
	// we leave the width and height the same
	gpApp->GetMainFrame()->canvas->ClientToScreen(&m_ptBoxTopLeft.x,&m_ptBoxTopLeft.y); // now it's screen coords
	int height = m_nTwoLineDepth;
	wxRect rectDlg;
	GetClientSize(&rectDlg.width, &rectDlg.height); // dialog's window
	rectDlg = NormalizeRect(rectDlg); // in case we ever change from MM_TEXT mode // use our own
	int dlgHeight = rectDlg.GetHeight();
	int dlgWidth = rectDlg.GetWidth();
	wxASSERT(dlgHeight > 0);
	int left = (rectScreen.GetWidth() - dlgWidth)/2;
	if (m_ptBoxTopLeft.y + height < rectScreen.GetBottom() - dlgHeight)
	{
		// put dlg near the bottom of screen (BEW modified 28Feb06 to have -80 rather than -30)
		// because the latter value resulted in the bottom buttons of the dialog being hidden
		// by the status bar at the screen bottom
		SetSize(left,rectScreen.GetBottom()-dlgHeight-80,540,132,wxSIZE_USE_EXISTING);
	}
	else
	{
		// put dlg at the top of the screen
		SetSize(left,rectScreen.GetTop()+40,540,132,wxSIZE_USE_EXISTING);
	}
	
	TransferDataToWindow();

	// call this to set up the initial default condition, provided there will be something
	// in the list to select in the first place (which amounts to the targetUnit existing)
	if (m_bFoundTgtUnit)
	{
		wxCommandEvent levent;
		OnSelchangeListTranslations(levent);
	}
	else
	{
		// no list box content, so first radio button will be disabled, so make the
		// default button in this case be the second one - ie. to accept the existing transl'n
		wxRadioButton* pRadio = (wxRadioButton*)FindWindowById(IDC_RADIO_ACCEPT_CURRENT);
		wxASSERT(pRadio != NULL);
		m_finalAdaptation = m_adaptationStr;
		pRadio->SetValue(TRUE);
	}

}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CConsistencyCheckDlg::OnOK(wxCommandEvent& event) 
{
	//UpdateData(TRUE); // The equivalent of UpdateData() [TransferDataFromWindow()] is not 
						// needed when using wx's validators because they transfer data from 
						// the variable to the control when the dialog is initialized, and 
						// transfer data from the control to the associated variable when 
						// the dialog is dismissed with OnOK() 
	gbIgnoreIt = FALSE;

	// see if the type new radio button is checked, & if so use the
	// string in the edit box to its right
	wxRadioButton* pRadio = (wxRadioButton*)FindWindowById(IDC_RADIO_TYPE_NEW);
	wxASSERT(pRadio != NULL);
	int nChecked = pRadio->GetValue();
	if (nChecked)
	{
		m_finalAdaptation = m_newStr;
	}
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods
void CConsistencyCheckDlg::OnRadioListSelect(wxCommandEvent& WXUNUSED(event)) 
{
	wxString s;
	s = _("<no adaptation>"); //IDS_NO_ADAPTATION that is, "<no adaptation>" 
	
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBox))
	{
		wxMessageBox(_("List box error: probably nothing is selected yet. If so, then select the translation you want to use."), 
			_T(""), wxICON_EXCLAMATION);
	}
	
	int nSel;
	nSel = m_pListBox->GetSelection();
	//if (nSel == -1) //LB_ERR
	//{
	//	wxMessageBox(_("List box error: probably nothing is selected yet. If so, then select the translation you want to use."), 
	//		_T(""), wxICON_EXCLAMATION);
	//}
	wxString str = _T("");
	if (nSel != -1) //LB_ERR
		str = m_pListBox->GetStringSelection(); //m_listBox.GetText(nSel,str);
	if (str == s)
		str = _T(""); // restore null string
	m_finalAdaptation = str;
}

void CConsistencyCheckDlg::OnRadioAcceptCurrent(wxCommandEvent& WXUNUSED(event)) 
{
	m_finalAdaptation = m_adaptationStr;
}

void CConsistencyCheckDlg::OnRadioTypeNew(wxCommandEvent& WXUNUSED(event)) 
{
	m_newStr = _T("");
	TransferDataToWindow();
}

void CConsistencyCheckDlg::OnSelchangeListTranslations(wxCommandEvent& WXUNUSED(event)) 
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	//if (m_pListBox->GetCount() == 0)
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBox))
		return;

	wxString s;
	s = _("<no adaptation>"); //IDS_NO_ADAPTATION that is, "<no adaptation>" 
	int nSel;
	nSel = m_pListBox->GetSelection();
	//if (nSel == -1) //LB_ERR
	//{
	//	wxMessageBox(_("List box error: probably nothing is selected yet. If so, then select the translation you want to use."), 
	//		_T(""), wxICON_EXCLAMATION);
	//}
	wxString str = _T("");
	if (nSel != -1) // LB_ERR
		str = m_pListBox->GetStringSelection();
	if (str == s)
		str = _T(""); // restore null string
	m_finalAdaptation = str;

	// also make the relevant radio button be turned on
	wxRadioButton* pRadio = (wxRadioButton*)FindWindowById(IDC_RADIO_LIST_SELECT);
	wxASSERT(pRadio != NULL);
	pRadio->SetValue(TRUE);
	wxRadioButton* pRadio2 = (wxRadioButton*)FindWindowById(IDC_RADIO_ACCEPT_CURRENT);
	wxASSERT(pRadio2 != NULL);
	pRadio2->SetValue(FALSE);
	pRadio = (wxRadioButton*)FindWindowById(IDC_RADIO_TYPE_NEW);
	wxASSERT(pRadio != NULL);
	pRadio->SetValue(FALSE);

	TransferDataToWindow();
}


void CConsistencyCheckDlg::OnSetfocusEditTypeNew(wxFocusEvent& event) 
{
	// wx note: we could also do this using wxWindow::FindFocus()
	if (event.GetEventObject() == m_pEditCtrlNew)
	{
		if (event.GetEventType() == wxEVT_SET_FOCUS)
		{
			// make the relevant radio button be turned on
			wxRadioButton* pRadio1 = (wxRadioButton*)FindWindowById(IDC_RADIO_LIST_SELECT);
			wxASSERT(pRadio1 != NULL);
			pRadio1->SetValue(FALSE);
			wxRadioButton* pRadio2 = (wxRadioButton*)FindWindowById(IDC_RADIO_ACCEPT_CURRENT);
			wxASSERT(pRadio2 != NULL);
			pRadio2->SetValue(FALSE);
			wxRadioButton* pRadio = (wxRadioButton*)FindWindowById(IDC_RADIO_TYPE_NEW);
			wxASSERT(pRadio != NULL);
			pRadio->SetValue(TRUE);
			TransferDataToWindow();
		}
	}
}

void CConsistencyCheckDlg::OnButtonNoAdaptation(wxCommandEvent& WXUNUSED(event)) 
{
	TransferDataFromWindow(); // make sure m_bDoAutoFix is updated correctly
	EndModal(0);
}

void CConsistencyCheckDlg::OnButtonIgnoreIt(wxCommandEvent& WXUNUSED(event)) 
{
	TransferDataFromWindow(); // make sure m_bDoAutoFix is updated correctly
	gbIgnoreIt = TRUE;
	EndModal(0);
}


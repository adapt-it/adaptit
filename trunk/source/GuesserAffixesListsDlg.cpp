/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GuesserAffixesListsDlg.cpp
/// \author			Kevin Bradford & Bruce Waters
/// \date_created	7 March 2014
/// \rcs_id $Id: GuesserAffixesListDlg.cpp 3296 2013-06-12 04:56:31Z adaptit.bruce@gmail.com $
/// \copyright		2014 Bruce Waters, Bill Martin, Kevin Bradford, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the GuesserAffixesListsDlg class. 
/// The GuesserAffixesListsDlg class provides a dialog in which the user can create a list of
/// prefixes in two columns, each line being a src language prefix in column one, and a
/// target language prefix in column two. Similarly, a separate list of suffix pairs can
/// be created. No hyphens should be typed. Explicitly having these morpheme sets makes
/// the guesser work far more accurately.
/// \derivation		The GuesserAffixesListsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "GuesserAffixesListsDlg.h"
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
#include "Adapt_It.h"
#include <wx/html/htmlwin.h>
#include "HtmlFileViewer.h"
#include "GuesserAffixesListsDlg.h"

// event handler table
BEGIN_EVENT_TABLE(GuesserAffixesListsDlg, AIModalDialog)
	EVT_INIT_DIALOG(GuesserAffixesListsDlg::InitDialog)
	EVT_BUTTON(wxID_OK, GuesserAffixesListsDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, GuesserAffixesListsDlg::OnCancel)
	EVT_RADIOBUTTON(ID_RADIO_PREFIXES_LIST, GuesserAffixesListsDlg::OnRadioButtonPrefixesList)
	EVT_RADIOBUTTON(ID_RADIO_SUFFIXES_LIST, GuesserAffixesListsDlg::OnRadioButtonSuffixesList)
	EVT_BUTTON(ID_BUTTON_GUESSER_DLG_EXPLAIN, GuesserAffixesListsDlg::OnExplanationDlgWanted)
END_EVENT_TABLE()

GuesserAffixesListsDlg::GuesserAffixesListsDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Construct Suffixes and Prefixes Lists"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	GuesserAffixListDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_pApp = &wxGetApp();

	m_pRadioSuffixesList = (wxRadioButton*)FindWindowById(ID_RADIO_SUFFIXES_LIST);
	wxASSERT(m_pRadioSuffixesList != NULL);
	m_pRadioPrefixesList = (wxRadioButton*)FindWindowById(ID_RADIO_PREFIXES_LIST);
	wxASSERT(m_pRadioPrefixesList != NULL);
	
	m_pHyphenSrcSuffix = (wxStaticText*)FindWindowById(ID_STATICTEXT_SRC_SUFFIX); // to left of src affix text box
	m_pHyphenSrcPrefix = (wxStaticText*)FindWindowById(ID_STATICTEXT_SRC_PREFIX); // to right of src affix text box
	m_pHyphenTgtSuffix = (wxStaticText*)FindWindowById(ID_STATICTEXT_TGT_SUFFIX); // to left of tgt affix text box
	m_pHyphenTgtPrefix = (wxStaticText*)FindWindowById(ID_STATICTEXT_TGT_PREFIX); // to right of tgt affix text box




	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	// other attribute initializations
}

void GuesserAffixesListsDlg::OnCancel(wxCommandEvent& event)
{
	event.Skip();
}


void GuesserAffixesListsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	m_bSuffixesListIsCurrent = TRUE;



	// Start with suffix pairs showing in the list
	LoadDataForListType(suffixesListType);
	//LoadDataForListType(prefixesListType);
}

GuesserAffixesListsDlg::~GuesserAffixesListsDlg(void){}


void GuesserAffixesListsDlg::OnRadioButtonPrefixesList(wxCommandEvent& WXUNUSED(event))
{
	// The user's click has already changed the value held by the radio button
	m_pRadioPrefixesList->SetValue(TRUE);
	m_pRadioSuffixesList->SetValue(FALSE);

	// Toggle the flag
	m_bSuffixesListIsCurrent = FALSE;


// TODO  add rest

	bool result = LoadDataForListType(prefixesListType);
	if (!result)
	{
		// Something went wrong.... fix it or whatever here

	}
}

void GuesserAffixesListsDlg::OnRadioButtonSuffixesList(wxCommandEvent& WXUNUSED(event))
{
	// The user's click has already changed the value held by the radio button
	m_pRadioPrefixesList->SetValue(FALSE);
	m_pRadioSuffixesList->SetValue(TRUE);

	// Toggle the flag
	m_bSuffixesListIsCurrent = TRUE;


// TODO  add rest

	bool result = LoadDataForListType(suffixesListType);
	if (!result)
	{
		// Something went wrong.... fix it or whatever here

	}
}


bool GuesserAffixesListsDlg::LoadDataForListType(PairsListType myType)
{
	wxWindow* pContainingWindow = sizerHyphensArea->GetContainingWindow();
	wxASSERT(pContainingWindow != NULL);
	if (myType == prefixesListType)
	{
		// We are displaying pairs for the Prefixes List
		
		// Hide the "hyphens" which suggest suffixes are being currently entered
		m_pHyphenSrcSuffix->Hide();
		m_pHyphenTgtSuffix->Hide();
		m_pHyphenSrcPrefix->Show(); // show the one to right of src affix text box
		m_pHyphenTgtPrefix->Show(); // show the one to right of tgt affix text box
		sizerHyphensArea->Layout();
		pContainingWindow->Refresh(); // needed, otherwise a phantom hyphen shows
									  // at the start of the relocated textbox

// TODO  add rest

		return TRUE;
	}
	else
	{
		// We are displaying pairs for the Suffixes List

		// Hide the "hyphens" which suggest prefixes are being currently entered
		m_pHyphenSrcPrefix->Hide();
		m_pHyphenTgtPrefix->Hide();
		m_pHyphenSrcSuffix->Show(); // show the one to left of src affix text box
		m_pHyphenTgtSuffix->Show(); // show the one to left of tgt affix text box
		sizerHyphensArea->Layout();
		pContainingWindow->Refresh(); // needed, otherwise a phantom hyphen shows
									  // at the start of the relocated textbox


		// TODO  add rest

		return TRUE;
	}
}


/*
	// The user's click has already changed the value held by the radio button
	m_pRadioKBType2->SetValue(TRUE);
	m_pRadioKBType1->SetValue(FALSE);

	// Tell the app what value we have chosen - Thread_DoEntireKbDeletion may
	// need this value
	m_pApp->m_bAdaptingKbIsCurrent = FALSE;

	// Set the appropriate label for above the listbox
	m_pAboveListBoxLabel->SetLabel(m_glsListLabel);

	// Set the appropriate label text for the second text control
	m_pNonSrcLabel->SetLabel(m_glossesLanguageCodeLabel);

	// Record which type of KB we are defining
	m_bKBisType1 = FALSE;

	// Get it's data displayed (each such call "wastes" one of the sublists, we only
	// display the sublist wanted, and a new ListKbs() call is done each time -- not
	// optimal for efficiency, but it greatly simplifies our code)
#if defined(_DEBUG) && defined(_WANT_DEBUGLOG)
	wxLogDebug(_T("OnRadioButton2KbsPageType2(): This page # (m_nCurPage + 1) = %d"), m_nCurPage + 1);
#endif
	LoadDataForPage(m_nCurPage);
*/


void GuesserAffixesListsDlg::OnExplanationDlgWanted(wxCommandEvent& WXUNUSED(event))
{
	// Display the GuesserExplanations.htm file in the platform's web browser
	// The "GuesserExplanations.htm" file should go into the m_helpInstallPath
	// for each platform, which is determined by the GetDefaultPathForHelpFiles() call.
	wxString helpFilePath = m_pApp->GetDefaultPathForHelpFiles() + m_pApp->PathSeparator
								+ m_pApp->m_GuesserExplanationMessageFileName;
	bool bSuccess = TRUE;

	wxLogNull nogNo;
	bSuccess = wxLaunchDefaultBrowser(helpFilePath,wxBROWSER_NEW_WINDOW); // result of 
				// using wxBROWSER_NEW_WINDOW depends on browser's settings for tabs, etc.

	if (!bSuccess)
	{
		wxString msg = _(
		"Could not launch the default browser to open the HTML file's URL at:\n\n%s\n\nYou may need to set your system's settings to open the .htm file type in your default browser.\n\nDo you want Adapt It to show the Help file in its own HTML viewer window instead?");
		msg = msg.Format(msg, helpFilePath.c_str());
		int response = wxMessageBox(msg,_("Browser launch error"),wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);
		m_pApp->LogUserAction(msg);
		if (response == wxYES)
		{
			wxString title = _("How to use the Suffixes and Prefixes Lists dialog");
			m_pApp->m_pHtmlFileViewer = new CHtmlFileViewer(this,&title,&helpFilePath);
			m_pApp->m_pHtmlFileViewer->Show(TRUE);
			m_pApp->LogUserAction(_T("Launched GuesserExplanation.htm in HTML Viewer"));
		}
	}
	else
	{
		m_pApp->LogUserAction(_T("Launched GuesserExplanation.htm in browser"));
	}
}

void GuesserAffixesListsDlg::OnOK(wxCommandEvent& event) 
{

	event.Skip();
}



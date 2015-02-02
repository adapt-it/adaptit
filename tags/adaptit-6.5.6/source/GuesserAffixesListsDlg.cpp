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
/// the guesser work far more accurately. klb
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
	EVT_BUTTON(ID_BUTTON_ADD, GuesserAffixesListsDlg::OnAdd)
	EVT_BUTTON(ID_BUTTON_UPDATE, GuesserAffixesListsDlg::OnUpdate)
	EVT_BUTTON(ID_BUTTON_INSERT, GuesserAffixesListsDlg::OnInsert)
	EVT_BUTTON(ID_BUTTON_DELETE, GuesserAffixesListsDlg::OnDelete)
	EVT_BUTTON(wxID_CANCEL, GuesserAffixesListsDlg::OnCancel)
	EVT_RADIOBUTTON(ID_RADIO_PREFIXES_LIST, GuesserAffixesListsDlg::OnRadioButtonPrefixesList)
	EVT_RADIOBUTTON(ID_RADIO_SUFFIXES_LIST, GuesserAffixesListsDlg::OnRadioButtonSuffixesList)
	EVT_BUTTON(ID_BUTTON_GUESSER_DLG_EXPLAIN, GuesserAffixesListsDlg::OnExplanationDlgWanted)
	EVT_LIST_ITEM_SELECTED(ID_LISTCTRL_SRC_TGT_AFFIX_PAIR, GuesserAffixesListsDlg::OnListItemSelected)
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

	m_pAffixPairsList = (wxListCtrl*)FindWindowById(ID_LISTCTRL_SRC_TGT_AFFIX_PAIR);

	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	// other attribute initializations
	m_bPrefixesLoaded = false; // Need to load affixes from app / file
	m_bSuffixesLoaded = false;

	m_pPrefixesPairsArray = new AffixPairsArray();
	m_pSuffixesPairsArray = new AffixPairsArray();

	// BEW 5Dec14 added the following for support of limiting how many prefix and suffix guesses, per word, are allows
	// and for tuning algorithms to the average length of affixes - a hint only, Less (long), Default, More (long) -
	// the latter feature Alan may as yet disallow
	// BEW 5Dec14 added support for affix limiting, and potentially for tuning algoriths to affix lengths
	m_pMaxPrefixesChoice = (wxChoice*)FindWindowById(ID_CHOICE_PREFIXES_LIMIT);
	m_pMaxSuffixesChoice = (wxChoice*)FindWindowById(ID_CHOICE_SUFFIXES_LIMIT);
}

void GuesserAffixesListsDlg::OnCancel(wxCommandEvent& event)
{
	if (m_bPrefixesUpdated == true || m_bSuffixesUpdated == true)
	{
		wxString msg = _(
			"You have made changes to affixes: Do you want to save them?");
		//msg = msg.Format(msg, helpFilePath.c_str());
		int response = wxMessageBox(msg,_("Changes not saved!"),wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT);
		if (response == wxYES)
		{
			bool bPrefixesOK, bSuffixesOK;
			if (m_bPrefixesUpdated == true)
			{
				bPrefixesOK = m_pApp->DoGuesserPrefixWriteToFile();
				wxUnusedVar(bPrefixesOK); // whm 21Jan2015 avoid warning
			}
			if (m_bSuffixesUpdated == true)
			{
				bSuffixesOK = m_pApp->DoGuesserSuffixWriteToFile();
				wxUnusedVar(bSuffixesOK); // whm 21Jan2015 avoid warning
			}
		}
	}
	event.Skip();
	EndModal(wxID_CANCEL); //wxDialog::OnCancel(event);
}

void GuesserAffixesListsDlg::AdjustLabels(bool bSuffixListChosen)
{
	if (bSuffixListChosen)
	{
		m_pSrcLabel->SetLabel(m_suffixesSrcLabel);
		m_pTgtLabel->SetLabel(m_suffixesTgtLabel);
	}
	else
	{
		m_pSrcLabel->SetLabel(m_prefixesSrcLabel);
		m_pTgtLabel->SetLabel(m_prefixesTgtLabel);
	}
}


void GuesserAffixesListsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	m_pltCurrentAffixPairListType = suffixesListType;

	m_prefixesSrcLabel = _("Source Language Prefix:");
	m_prefixesTgtLabel = _("Target Language Prefix:");
	m_suffixesSrcLabel = _("Source Language Suffix:");
	m_suffixesTgtLabel = _("Target Language Suffix:");

	// Start with suffix pairs showing in the list?
	LoadDataForListType(m_pltCurrentAffixPairListType);

	m_pSrcLabel = (wxStaticText*)FindWindowById(ID_LABEL_SRC_AFFIX);
	m_pTgtLabel = (wxStaticText*)FindWindowById(ID_LABEL_TGT_AFFIX);

	// Set list for suffixes to be entered first as the default when opening
	m_bSuffixListChosen = TRUE; // default when first opened
	AdjustLabels(m_bSuffixListChosen);

	m_pSrcAffix = (wxTextCtrl*)FindWindowById(ID_TEXT_SRC_AFFIX);
	m_pTgtAffix = (wxTextCtrl*)FindWindowById(ID_TEXT_TGT_AFFIX);
	m_pSrcAffix->SetFocus();

	m_pBtnInsert = (wxButton*)FindWindowById(ID_BUTTON_INSERT);
	m_pBtnInsert->Enable(false); // Nothing Selected, so cannot insert
	m_pBtnUpdate = (wxButton*)FindWindowById(ID_BUTTON_UPDATE);	
	m_pBtnUpdate->Enable(false);
	m_pBtnDelete = (wxButton*)FindWindowById(ID_BUTTON_DELETE);
	m_pBtnDelete->Enable(false);

	m_bPrefixesUpdated = false;
	m_bSuffixesUpdated = false;

	// BEW added 5Dec14
	if (m_pApp->m_iMaxPrefixes == -1)
	{
		// It's never been set as yet
		m_iMaxPrefxs = 1;
	}
	else
	{
		// It has been set earlier (value comes from project config file)
		m_iMaxPrefxs = m_pApp->m_iMaxPrefixes;
	}
	m_pMaxPrefixesChoice->SetSelection(m_iMaxPrefxs);
	if (m_pApp->m_iMaxSuffixes == -1)
	{
		// It's never been set as yet
		//m_iMaxSuffxs = 1;
		m_iMaxSuffxs = 3; // Alan Buseman's default is 3, so we'll go with that
	}
	else
	{
		// It has been set earlier (value comes from project config file)
		m_iMaxSuffxs = m_pApp->m_iMaxSuffixes;
	}
	m_pMaxSuffixesChoice->SetSelection(m_iMaxSuffxs);

// TODO -- handle the radio buttons, if Alan allows the length hinting

}

GuesserAffixesListsDlg::~GuesserAffixesListsDlg(void)
{
	// Clear up Guesser Prefix/Suffix Arrays - klb
	ClearAffixList( prefixesListType );
	ClearAffixList( suffixesListType );
	/*	if (!m_pPrefixesPairsArray->IsEmpty())
	{
		m_pPrefixesPairsArray->Clear();
	}
	if (!m_pSuffixesPairsArray->IsEmpty())
	{
		m_pSuffixesPairsArray->Clear();
	}
	*/
	delete m_pPrefixesPairsArray;
	delete m_pSuffixesPairsArray;
}

void GuesserAffixesListsDlg::ClearAffixList( PairsListType type )
{
	// Clear desired list and delete items
	wxASSERT(type == prefixesListType || type == suffixesListType);
	int ct;
	
	if (type == prefixesListType && !m_pPrefixesPairsArray->IsEmpty())
	{
		for (ct = 0; ct < (int)m_pPrefixesPairsArray->GetCount(); ct++)
		{ 
			AffixPair* pAffix = m_pPrefixesPairsArray->Item(ct);
			delete pAffix;
		}
		m_pPrefixesPairsArray->Clear();
	}
	else if (type == suffixesListType && !m_pSuffixesPairsArray->IsEmpty())
	{
		for (ct = 0; ct < (int)m_pSuffixesPairsArray->GetCount(); ct++)
		{ 
			AffixPair* pAffix = m_pSuffixesPairsArray->Item(ct);
			delete pAffix;
		}
		m_pSuffixesPairsArray->Clear();
	}
	
}

void GuesserAffixesListsDlg::OnRadioButtonPrefixesList(wxCommandEvent& WXUNUSED(event))
{
	// The user's click has already changed the value held by the radio button
	m_pRadioPrefixesList->SetValue(TRUE);
	m_pRadioSuffixesList->SetValue(FALSE);

	m_bSuffixListChosen = FALSE;
	AdjustLabels(m_bSuffixListChosen);
	
	// No longer anything selected so disable buttons
	m_pBtnUpdate->Disable();
	m_pBtnInsert->Disable();
	m_pBtnDelete->Disable();

	bool result = LoadDataForListType(m_pltCurrentAffixPairListType = prefixesListType);
	wxUnusedVar(result); // Avoid warning...figured I would leave in bool return for future error signaling
	wxWindow* pContainingWindow = GetParent();
	wxASSERT(pContainingWindow != NULL);
	if (pContainingWindow->GetSizer() != NULL)
	{
		pContainingWindow->GetSizer()->Layout();
	}
	pContainingWindow->Refresh();
}

void GuesserAffixesListsDlg::OnRadioButtonSuffixesList(wxCommandEvent& WXUNUSED(event))
{
	// The user's click has already changed the value held by the radio button
	m_pRadioPrefixesList->SetValue(FALSE);
	m_pRadioSuffixesList->SetValue(TRUE);

	m_bSuffixListChosen = TRUE;
	AdjustLabels(m_bSuffixListChosen);

	// No longer anything selected so disable buttons
	m_pBtnUpdate->Disable();
	m_pBtnInsert->Disable();
	m_pBtnDelete->Disable();

	bool result = LoadDataForListType(m_pltCurrentAffixPairListType = suffixesListType);
	wxUnusedVar(result); // Avoid warning...figured I would leave in bool return for future error signaling
	wxWindow* pContainingWindow = GetParent();
	wxASSERT(pContainingWindow != NULL);
	if (pContainingWindow->GetSizer() != NULL)
	{
		pContainingWindow->GetSizer()->Layout();
	}
	pContainingWindow->Refresh();
}


bool GuesserAffixesListsDlg::LoadDataForListType(PairsListType myType)
{
	//wxWindow* pContainingWindow = GetContainingWindow();
	wxWindow* pContainingWindow = GetParent();
	wxASSERT(pContainingWindow != NULL);

	if (!m_bPrefixesLoaded)
	{
		// Load affixes from app
		LoadPrefixes();
	}
	if (!m_bSuffixesLoaded)
	{
		// Load affixes from app
		LoadSuffixes();
	}

	// start with an empty list
	//m_pAffixPairsList->ClearAll();

	wxListItem col[2];
	// Based on what Mike does in DVCSLogDlg...
	col[0].SetId(0);
	col[0].SetText(_("Source"));
	col[0].SetWidth(22);
	m_pAffixPairsList->InsertColumn(0, col[0]);

	col[1].SetId(1);
	col[1].SetText(_("Target"));
	col[1].SetWidth(220);
	m_pAffixPairsList->InsertColumn(1, col[1]);
	m_pAffixPairsList->SetColumnWidth(0,wxLIST_AUTOSIZE_USEHEADER);

	m_pAffixPairsList->DeleteAllItems();
	
	if (myType == prefixesListType)
	{
		// We are displaying pairs for the Prefixes List
		
		// Hide the "hyphens" which suggest suffixes are being currently entered
		m_pHyphenSrcSuffix->Hide();
		m_pHyphenTgtSuffix->Hide();
		m_pHyphenSrcPrefix->Show(); // show the one to right of src affix text box
		m_pHyphenTgtPrefix->Show(); // show the one to right of tgt affix text box
		//sizerHyphensArea->Layout();
		if (pContainingWindow->GetSizer() != NULL)
		{
			pContainingWindow->GetSizer()->Layout();
			//DoLayoutAdaptation();
		}
		pContainingWindow->Refresh(); // needed, otherwise a phantom hyphen shows
									  // at the start of the relocated textbox

		// Load Prefixes, if they exist -klb
		if (GetPrefixes() && GetPrefixes()->Count() > 0)
		{
			for (size_t n = 0; n < GetPrefixes()->Count(); n++)
			{
				wxListItem item;
				item.SetId(n);
				m_pAffixPairsList->InsertItem(item);

				m_pAffixPairsList->SetItem(n, 0, GetPrefixes()->Item(n)->srcLangAffix);
				m_pAffixPairsList->SetItem(n, 1, GetPrefixes()->Item(n)->tgtLangAffix);
			}
		}

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
		if (pContainingWindow->GetSizer() != NULL)
		{
			pContainingWindow->GetSizer()->Layout();
			//DoLayoutAdaptation();
		}
		pContainingWindow->Refresh(); // needed, otherwise a phantom hyphen shows
									  // at the start of the relocated textbox


		// Load Suffixes, if they exist -klb
		if (GetSuffixes() && GetSuffixes()->Count() > 0)
		{
			for (size_t n = 0; n < GetSuffixes()->Count(); n++)
			{
				wxListItem item;
				item.SetId(n);
				m_pAffixPairsList->InsertItem(item);

				m_pAffixPairsList->SetItem(n, 0, GetSuffixes()->Item(n)->srcLangAffix);
				m_pAffixPairsList->SetItem(n, 1, GetSuffixes()->Item(n)->tgtLangAffix);
			}
		}


		return TRUE;
	}
}

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
			m_pApp->LogUserAction(_("Launched GuesserExplanation.htm in HTML Viewer"));
		}
	}
	else
	{
		m_pApp->LogUserAction(_("Launched GuesserExplanation.htm in browser"));
	}
}

void GuesserAffixesListsDlg::OnOK(wxCommandEvent& event) 
{
	
	bool bPrefixesOK, bSuffixesOK;
	if (m_bPrefixesUpdated == true)
	{
		bPrefixesOK = m_pApp->DoGuesserPrefixWriteToFile();
		wxUnusedVar(bPrefixesOK); // whm 21Jan2015 avoid warning
	}
	if (m_bSuffixesUpdated == true)
	{
		bSuffixesOK = m_pApp->DoGuesserSuffixWriteToFile();
		wxUnusedVar(bSuffixesOK); // whm 21Jan2015 avoid warning
	}

	// BEW 5Dec14 get the m_iMaxPrefxs and m_iMaxSuffxs values and put them into the
	// app's m_iMaxPrefixes and m_iMaxSuffixes members, so they can be written out to the
	// project configuration file
	int nSel = m_pMaxPrefixesChoice->GetSelection(); // selection index is equiv to value, except
													 // nSel == 3 is 'no limit' which we assign to 99
	m_iMaxPrefxs = nSel;
	if (nSel >= 3)
		m_iMaxPrefxs = 99;

	nSel = m_pMaxSuffixesChoice->GetSelection(); // selection index is equiv to value, except
												 // nSel == 4 is 'no limit' which we assign to 99
	m_iMaxSuffxs = nSel;
	if (nSel >= 4)
		m_iMaxSuffxs = 99;

	m_pApp->m_iMaxPrefixes = m_iMaxPrefxs;
	m_pApp->m_iMaxSuffixes = m_iMaxSuffxs;

	event.Skip();
}
void GuesserAffixesListsDlg::OnAdd(wxCommandEvent& event) 
{
	wxLogNull logNo;
	wxString sSrc = m_pSrcAffix->GetValue();
	wxString sTgt = m_pTgtAffix->GetValue();
	// BEW changed 24Nov14, it is acceptable (though not useful) for the target affix to be absent
	//if (sSrc.IsNull() || sSrc.Len() == 0 || sTgt.IsNull() || sTgt.Len() == 0)
	if (sSrc.IsNull() || sSrc.Len() == 0)
		{
		if (sSrc.IsNull() || sSrc.Len() == 0)
			m_pSrcAffix->SetFocus();
		//else
		//	m_pTgtAffix->SetFocus();
		wxMessageBox(_("Please type in an affix pair to Add"),_T(""),wxICON_INFORMATION | wxOK, this);
		//event.Skip();
	}
	else if ((m_pltCurrentAffixPairListType == prefixesListType && PrefixExistsAlready(sSrc)) ||
			 (m_pltCurrentAffixPairListType == suffixesListType && SuffixExistsAlready(sSrc))) // If already exists
	{
		wxString msg;
		msg = _("Affix \"%s\" already exists. Multiple identical affixes are not allowed.");
		msg = msg.Format(msg, sSrc.c_str());
		wxMessageBox(msg,_T(""),wxICON_INFORMATION | wxOK, this);
		//event.Skip(); 
	}
	else
	{
		CGuesserAffix* pNewAffix = new CGuesserAffix(sSrc, sTgt);

		if (m_pltCurrentAffixPairListType == prefixesListType)
		{
			m_bPrefixesLoaded = FALSE;
			m_pApp->GetGuesserPrefixes()->Add(pNewAffix);
			m_bPrefixesUpdated = true;
		}
		else
		{
			m_bSuffixesLoaded = FALSE;
			m_pApp->GetGuesserSuffixes()->Add(pNewAffix);
			m_bSuffixesUpdated = true;
		}
		LoadDataForListType(m_pltCurrentAffixPairListType);
	}
	m_pSrcAffix->ChangeValue(_T(""));
	m_pTgtAffix->ChangeValue(_T(""));

	wxWindow* pContainingWindow = GetParent();
	wxASSERT(pContainingWindow != NULL);

	if (pContainingWindow->GetSizer() != NULL)
	{
		pContainingWindow->GetSizer()->Layout();
	}
	pContainingWindow->Refresh(); // needed, otherwise a phantom hyphen shows
								  // at the start of the relocated textbox
	DeselectButtonsAfterClick();
	event.Skip();
}
void GuesserAffixesListsDlg::OnUpdate(wxCommandEvent& event) 
{
	wxLogNull logNo;
	wxString sSrc = m_pSrcAffix->GetValue();
	wxString sTgt = m_pTgtAffix->GetValue();
	long itemIndex = (long)wxNOT_FOUND; // initialize
	wxString warningMsg = _("No selection!\nSelect the affix pair to be updated,\nthen click the button again.");
	// BEW changed 24Nov14, it is acceptable (though not useful) for the target affix to be absent
	//if (sSrc.IsNull() || sSrc.Len() == 0 || sTgt.IsNull() || sTgt.Len() == 0)
	if (sSrc.IsNull() || sSrc.Len() == 0)
	{
		if (sSrc.IsNull() || sSrc.Len() == 0)
			m_pSrcAffix->SetFocus();
		//else
		//	m_pTgtAffix->SetFocus();
		DeselectButtonsAfterClick();
		wxMessageBox(_("First select the pair to be Updated, so their affixes will appear in the two text boxes.\nEdit them there, then click Update."),_T(""),wxICON_INFORMATION | wxOK, this);
		event.Skip();
		return;
	}
	else
	{		
		itemIndex = GetSelectedItemIndex(); // will return (long)-1 if there is no selection current
		if (itemIndex == (long)wxNOT_FOUND)
		{
			DeselectButtonsAfterClick();
			wxMessageBox(warningMsg, _T(""), wxICON_INFORMATION | wxOK, this);
			event.Skip();
			return;
		}
		if (m_pltCurrentAffixPairListType == prefixesListType)
		{
			m_bPrefixesLoaded = false;
			CGuesserAffix m_SelectedAffix = CGuesserAffix(GetCellContentsString(itemIndex, 0), GetCellContentsString(itemIndex, 1));
			int iIndex = m_pApp->FindGuesserPrefixIndex(m_SelectedAffix);
			m_pApp->GetGuesserPrefixes()->Item(iIndex).setSourceAffix(sSrc);
			m_pApp->GetGuesserPrefixes()->Item(iIndex).setTargetAffix(sTgt);
			m_bPrefixesUpdated = true;
		}
		else
		{
			m_bSuffixesLoaded = false;
			CGuesserAffix m_SelectedAffix = CGuesserAffix(GetCellContentsString(itemIndex, 0), GetCellContentsString(itemIndex, 1));
			int iIndex = m_pApp->FindGuesserSuffixIndex(m_SelectedAffix);
			m_pApp->GetGuesserSuffixes()->Item(iIndex).setSourceAffix(sSrc);
			m_pApp->GetGuesserSuffixes()->Item(iIndex).setTargetAffix(sTgt);
			m_bSuffixesUpdated = true;
		}
		LoadDataForListType(m_pltCurrentAffixPairListType);
		m_pSrcAffix->ChangeValue(_T(""));
		m_pTgtAffix->ChangeValue(_T(""));
		DeselectButtonsAfterClick();
		event.Skip();
	}
}
void GuesserAffixesListsDlg::OnInsert(wxCommandEvent& event) 
{
	wxLogNull logNo;
	wxString sSrc = m_pSrcAffix->GetValue();
	wxString sTgt = m_pTgtAffix->GetValue();
	long itemIndex = (long)wxNOT_FOUND; // initialize
	wxString warningMsg = _("No selection!\nSelect the affix pair before which the new pair will be inserted,\nthen click the button again.");
	// BEW changed 24Nov14, it is acceptable (though not useful) for the target affix to be absent
	//if (sSrc.IsNull() || sSrc.Len() == 0 || sTgt.IsNull() || sTgt.Len() == 0)
	if (sSrc.IsNull() || sSrc.Len() == 0)
	{
		if (sSrc.IsNull() || sSrc.Len() == 0)
			m_pSrcAffix->SetFocus();
		//else
		//	m_pTgtAffix->SetFocus();
		wxMessageBox(_("Please type in an affix pair to Insert"),_T(""),wxICON_INFORMATION | wxOK, this);
		DeselectButtonsAfterClick();
		event.Skip();
		return;
	}
	else
	{
		itemIndex = GetSelectedItemIndex(); // will return (long)-1 if there is no selection current
		if (itemIndex == (long)wxNOT_FOUND)
		{
			wxMessageBox(warningMsg, _T(""), wxICON_INFORMATION | wxOK, this);
			event.Skip();
			return;
		}
		CGuesserAffix* pNewAffix = new CGuesserAffix(sSrc, sTgt);
		if (m_pltCurrentAffixPairListType == prefixesListType)
		{
			m_bPrefixesLoaded = FALSE;
			CGuesserAffix m_SelectedAffix = CGuesserAffix(GetCellContentsString(itemIndex, 0), GetCellContentsString(itemIndex, 1));
			int iIndex = m_pApp->FindGuesserPrefixIndex(m_SelectedAffix);
			m_pApp->GetGuesserPrefixes()->Insert(pNewAffix, iIndex);
			m_bPrefixesUpdated = true;
		}
		else
		{
			m_bSuffixesLoaded = FALSE;
			CGuesserAffix m_SelectedAffix = CGuesserAffix(GetCellContentsString(itemIndex,0), GetCellContentsString(itemIndex,1));
			int iIndex = m_pApp->FindGuesserSuffixIndex(m_SelectedAffix);
			m_pApp->GetGuesserSuffixes()->Insert(pNewAffix, iIndex);
			m_bSuffixesUpdated = true;
		}
		LoadDataForListType(m_pltCurrentAffixPairListType);
		m_pSrcAffix->ChangeValue(_T(""));
		m_pTgtAffix->ChangeValue(_T(""));
		DeselectButtonsAfterClick();
		event.Skip();
	}
}
void GuesserAffixesListsDlg::OnDelete(wxCommandEvent& event) 
{
	wxLogNull logNo;
	wxString sSrc = m_pSrcAffix->GetValue();
	wxString sTgt = m_pTgtAffix->GetValue();
	long itemIndex = (long)wxNOT_FOUND; // initialize
	wxString warningMsg = _("No selection!\nSelect the affix pair to be deleted,\nthen click the button again.");
	// BEW changed 24Nov14, it is acceptable for the target affix to be absent
	//if (sSrc.IsNull() || sSrc.Len() == 0 || sTgt.IsNull() || sTgt.Len() == 0)
	if (sSrc.IsNull() || sSrc.Len() == 0)
	{
		if (sSrc.IsNull() || sSrc.Len() == 0)
			m_pSrcAffix->SetFocus();
		//else
		//	m_pTgtAffix->SetFocus();
		DeselectButtonsAfterClick();
		wxMessageBox(_("Please select an affix pair to delete"), _T(""), wxICON_INFORMATION | wxOK, this);
		event.Skip();
		return;
	}
	else
	{
		itemIndex = GetSelectedItemIndex(); // will return (long)-1 if there is no selection current
		if (itemIndex == (long)wxNOT_FOUND)
		{
			DeselectButtonsAfterClick();
			wxMessageBox(warningMsg, _T(""), wxICON_INFORMATION | wxOK, this);
			event.Skip();
			return;
		}
		if (m_pltCurrentAffixPairListType == prefixesListType)
		{
			m_bPrefixesLoaded = FALSE;
			CGuesserAffix m_CurrentAffix = CGuesserAffix(GetCellContentsString(itemIndex,0), GetCellContentsString(itemIndex,1));
			int iIndex = m_pApp->FindGuesserPrefixIndex(m_CurrentAffix);
			m_pApp->GetGuesserPrefixes()->RemoveAt(iIndex);
			m_bPrefixesUpdated = true;
		}
		else
		{
			m_bSuffixesLoaded = FALSE;
			CGuesserAffix m_CurrentAffix = CGuesserAffix(GetCellContentsString(itemIndex,0), GetCellContentsString(itemIndex,1));
			int iIndex = m_pApp->FindGuesserSuffixIndex(m_CurrentAffix);
			m_pApp->GetGuesserSuffixes()->RemoveAt(iIndex);
			m_bSuffixesUpdated = true;
		}
		m_pSrcAffix->ChangeValue(_T(""));
		m_pTgtAffix->ChangeValue(_T(""));
		DeselectButtonsAfterClick();
		LoadDataForListType(m_pltCurrentAffixPairListType);
	}
}

void GuesserAffixesListsDlg::OnListItemSelected(wxListEvent& event)
{
	m_pSrcAffix->ChangeValue(GetCellContentsString(GetSelectedItemIndex(), 0));
	m_pTgtAffix->ChangeValue(GetCellContentsString(GetSelectedItemIndex(), 1));
	m_pBtnInsert->Enable();
	m_pBtnUpdate->Enable();
	m_pBtnDelete->Enable();
	event.Skip();
}

void GuesserAffixesListsDlg::DeselectButtonsAfterClick()
{
	m_pSrcAffix->ChangeValue(GetCellContentsString(GetSelectedItemIndex(), 0));
	m_pTgtAffix->ChangeValue(GetCellContentsString(GetSelectedItemIndex(), 1));
	m_pBtnInsert->Enable(FALSE);
	m_pBtnUpdate->Enable(FALSE);
	m_pBtnDelete->Enable(FALSE);
}


AffixPairsArray* GuesserAffixesListsDlg::GetPrefixes()
{
	return m_pPrefixesPairsArray;
}

AffixPairsArray* GuesserAffixesListsDlg::GetSuffixes()
{
	return m_pSuffixesPairsArray;
}

// Load/refresh prefixes from app
bool GuesserAffixesListsDlg::LoadPrefixes() 
{
	m_bPrefixesLoaded = true;

	CGuesserAffixArray* pArray = NULL; // Pointer to current list
	ClearAffixList( prefixesListType );

	// Get and load prefixes (if any)
	if (m_pApp->GetGuesserPrefixes() && !m_pApp->GetGuesserPrefixes()->IsEmpty())
		{	
			pArray = m_pApp->GetGuesserPrefixes();
			for (int i = 0; i < (int)pArray->GetCount(); i++)
			{				
				CGuesserAffix m_currentGuesserAffix = pArray->Item(i);
				AffixPair* m_ap = new AffixPair();
				m_ap->pairType = prefixesListType;
				m_ap->srcLangAffix = m_currentGuesserAffix.getSourceAffix();
				m_ap->tgtLangAffix = m_currentGuesserAffix.getTargetAffix();

				// Add pair to control
				m_pPrefixesPairsArray->Add(m_ap);
				
			}
		}

	return TRUE;
}

// Load/refresh suffixes from app
bool GuesserAffixesListsDlg::LoadSuffixes() 
{
	m_bSuffixesLoaded = true;
	
	CGuesserAffixArray* pArray = NULL; // Pointer to current list
	ClearAffixList( suffixesListType );

	// Get and load suffixes (if any)
	if (m_pApp->GetGuesserSuffixes() && !m_pApp->GetGuesserSuffixes()->IsEmpty())
		{	
			pArray = m_pApp->GetGuesserSuffixes();
			for (int i = 0; i < (int)pArray->GetCount(); i++)
			{				
				CGuesserAffix m_currentGuesserAffix = pArray->Item(i);
				AffixPair* m_ap = new AffixPair();
				m_ap->pairType = suffixesListType;
				m_ap->srcLangAffix = m_currentGuesserAffix.getSourceAffix();
				m_ap->tgtLangAffix = m_currentGuesserAffix.getTargetAffix();

				// Add pair to control
				m_pSuffixesPairsArray->Add(m_ap);
			}
		}

	return TRUE;
}

// Helper function to Get index of selected item from control
long GuesserAffixesListsDlg::GetSelectedItemIndex() 
{
	//
	long itemIndex = -1;
	itemIndex = m_pAffixPairsList->GetNextItem(itemIndex,
			                                 wxLIST_NEXT_ALL,
				                             wxLIST_STATE_SELECTED);
	// Got the selected item index
#if defined(_DEBUG)
	wxLogDebug(m_pAffixPairsList->GetItemText(itemIndex));
#endif
	//if (itemIndex == -1)
	//	wxMessageBox(_("Selected affix pair not found!"),_T(""),wxICON_INFORMATION | wxOK, this);
	// BEW 21Jan15 A message here is too early. Instead, return the -1 for a 'not found' when there
	// is no line currently selected in the list control, and then handle the -1 in the caller with
	// an appropriate action and warning. The action should be to leave the text boxes unchanged but
	// otherwise return from the caller without doing anything. The message should telll the user to
	// first make a selection and then try the button click.
	return itemIndex;
 
}
bool GuesserAffixesListsDlg::PrefixExistsAlready(wxString sSrc)
{
	wxASSERT(m_pPrefixesPairsArray != NULL);
	
	// CHeck to see if Prefix Exists, return true if so
	if(GetPrefixes() != NULL && GetPrefixes()->GetCount() > 0)
		for (int i = 0; i < (int)GetPrefixes()->GetCount(); i++)
		{				
			if (m_pPrefixesPairsArray->Item(i)->srcLangAffix.CmpNoCase( sSrc ) == 0 )
			{
				return true;
			}
			
		}
	return false;
}
bool GuesserAffixesListsDlg::SuffixExistsAlready(wxString sSrc)
{
	wxASSERT(m_pSuffixesPairsArray != NULL);
	
	// Check to see if Suffix Exists, return true if so
	if(GetSuffixes() != NULL && GetSuffixes()->GetCount() > 0)
		for (int i = 0; i < (int)GetSuffixes()->GetCount(); i++)
		{				
			if (m_pSuffixesPairsArray->Item(i)->srcLangAffix.CmpNoCase( sSrc ) == 0 )
			{
				return true;
			}
			
		}
	return false;
}
// Column 0 = source, 1 = target
wxString GuesserAffixesListsDlg::GetCellContentsString( long row_number, int column ) 
{
   wxListItem     row_info;  
   wxString       cell_contents_string;
 
   // Set what row it is (m_itemId is a member of the regular wxListCtrl class)
   row_info.m_itemId = row_number;
   // Set what column of that row we want to query for information.
   row_info.m_col = column;
   // Set text mask
   row_info.m_mask = wxLIST_MASK_TEXT;
 
   // Get the info and store it in row_info variable.   
   m_pAffixPairsList->GetItem( row_info );
 
   // Extract the text out that cell
   cell_contents_string = row_info.m_text; 
 
   return cell_contents_string;
}

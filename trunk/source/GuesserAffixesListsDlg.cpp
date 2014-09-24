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
				bPrefixesOK = m_pApp->DoGuesserPrefixWriteToFile();
			if (m_bSuffixesUpdated == true)
				bSuffixesOK = m_pApp->DoGuesserSuffixWriteToFile();
		}
	}
	EndModal(wxID_CANCEL); //wxDialog::OnCancel(event);
}


void GuesserAffixesListsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	m_pltCurrentAffixPairListType = suffixesListType;

	// Start with suffix pairs showing in the list?
	LoadDataForListType(m_pltCurrentAffixPairListType);

	m_pSrcLangAffix = (wxStaticText*)FindWindowById(ID_TEXT_SRC_AFFIX);
	m_pTgtLangAffix = (wxStaticText*)FindWindowById(ID_TEXT_TGT_AFFIX);

	m_pBtnInsert = (wxButton*)FindWindowById(ID_BUTTON_INSERT);
	m_pBtnInsert->Enable(false); // Nothing Selected, so cannot insert

	m_bPrefixesUpdated = false;
	m_bSuffixesUpdated = false;
}

GuesserAffixesListsDlg::~GuesserAffixesListsDlg(void){}


void GuesserAffixesListsDlg::OnRadioButtonPrefixesList(wxCommandEvent& WXUNUSED(event))
{
	// The user's click has already changed the value held by the radio button
	m_pRadioPrefixesList->SetValue(TRUE);
	m_pRadioSuffixesList->SetValue(FALSE);

// TODO  add rest

	bool result = LoadDataForListType(m_pltCurrentAffixPairListType = prefixesListType);
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

// TODO  add rest

	bool result = LoadDataForListType(m_pltCurrentAffixPairListType = suffixesListType);
	if (!result)
	{
		// Something went wrong.... fix it or whatever here

	}
}


bool GuesserAffixesListsDlg::LoadDataForListType(PairsListType myType)
{
	
	wxWindow* pContainingWindow = sizerHyphensArea->GetContainingWindow();
	wxASSERT(pContainingWindow != NULL);

	if (!m_bPrefixesLoaded)
	{
		// Load affixes from app / file
		LoadPrefixes();
	}
	if (!m_bSuffixesLoaded)
	{
		// Load affixes from app / file
		LoadSuffixes();
	}

	// start with an empty list
	//m_pAffixPairsList->ClearAll();

	wxListItem col[2];
	// Based on what Mike does in DVCSLogDlg...
	col[0].SetId(0);
	col[0].SetText(_T("Source"));
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
		sizerHyphensArea->Layout();
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
		sizerHyphensArea->Layout();
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
	
	bool bPrefixesOK, bSuffixesOK;
	if (m_bPrefixesUpdated == true)
		bPrefixesOK = m_pApp->DoGuesserPrefixWriteToFile();
	if (m_bSuffixesUpdated == true)
		bSuffixesOK = m_pApp->DoGuesserSuffixWriteToFile();
	event.Skip();
}
void GuesserAffixesListsDlg::OnAdd(wxCommandEvent& event) 
{
	wxString sSrc = m_pSrcLangAffix->GetLabel();
	wxString sTgt = m_pTgtLangAffix->GetLabel();
	if (sSrc.IsNull() || sSrc.Len() == 0 || sTgt.IsNull() || sTgt.Len() == 0)
	{
		if (sSrc.IsNull() || sSrc.Len() == 0)
			m_pSrcLangAffix->SetFocus();
		else
			m_pTgtLangAffix->SetFocus();
		wxMessageBox(_("Please input an affix pair to add"),_T(""),wxICON_INFORMATION | wxOK, this);
		event.Skip();
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
	m_pSrcLangAffix->SetLabel(_T(""));
	m_pSrcLangAffix->Layout();
	m_pTgtLangAffix->SetLabel(_T(""));
	m_pTgtLangAffix->Layout();
}
void GuesserAffixesListsDlg::OnUpdate(wxCommandEvent& event) 
{
	wxString sSrc = m_pSrcLangAffix->GetLabel();
	wxString sTgt = m_pTgtLangAffix->GetLabel();
	if (sSrc.IsNull() || sSrc.Len() == 0 || sTgt.IsNull() || sTgt.Len() == 0)
	{
		if (sSrc.IsNull() || sSrc.Len() == 0)
			m_pSrcLangAffix->SetFocus();
		else
			m_pTgtLangAffix->SetFocus();
		wxMessageBox(_("Please input an updated affix pair for the selected pair"),_T(""),wxICON_INFORMATION | wxOK, this);
		event.Skip();
	}
	else
	{		
		if (m_pltCurrentAffixPairListType == prefixesListType)
		{
			m_bPrefixesLoaded = false;
			CGuesserAffix m_SelectedAffix = CGuesserAffix(GetCellContentsString(GetSelectedItemIndex(),0), GetCellContentsString(GetSelectedItemIndex(),1));
			int iIndex = m_pApp->FindGuesserPrefixIndex(m_SelectedAffix);
			m_pApp->GetGuesserPrefixes()->Item(iIndex).setSourceAffix(sSrc);
			m_pApp->GetGuesserPrefixes()->Item(iIndex).setTargetAffix(sTgt);
			m_bPrefixesUpdated = true;
		}
		else
		{
			m_bSuffixesLoaded = false;
			CGuesserAffix m_SelectedAffix = CGuesserAffix(GetCellContentsString(GetSelectedItemIndex(),0), GetCellContentsString(GetSelectedItemIndex(),1));
			int iIndex = m_pApp->FindGuesserSuffixIndex(m_SelectedAffix);
			m_pApp->GetGuesserSuffixes()->Item(iIndex).setSourceAffix(sSrc);
			m_pApp->GetGuesserSuffixes()->Item(iIndex).setTargetAffix(sTgt);
			m_bSuffixesUpdated = true;
		}
		LoadDataForListType(m_pltCurrentAffixPairListType);
	}
	m_pSrcLangAffix->SetLabel(_T(""));
	m_pSrcLangAffix->Layout();
	m_pTgtLangAffix->SetLabel(_T(""));
	m_pTgtLangAffix->Layout();
}
void GuesserAffixesListsDlg::OnInsert(wxCommandEvent& event) 
{
	wxString sSrc = m_pSrcLangAffix->GetLabel();
	wxString sTgt = m_pTgtLangAffix->GetLabel();
	if (sSrc.IsNull() || sSrc.Len() == 0 || sTgt.IsNull() || sTgt.Len() == 0)
	{
		if (sSrc.IsNull() || sSrc.Len() == 0)
			m_pSrcLangAffix->SetFocus();
		else
			m_pTgtLangAffix->SetFocus();
		wxMessageBox(_("Please input an affix pair to insert"),_T(""),wxICON_INFORMATION | wxOK, this);
		event.Skip();
	}
	else
	{
		CGuesserAffix* pNewAffix = new CGuesserAffix(sSrc, sTgt);
		
		if (m_pltCurrentAffixPairListType == prefixesListType)
		{
			m_bPrefixesLoaded = FALSE;
			CGuesserAffix m_SelectedAffix = CGuesserAffix(GetCellContentsString(GetSelectedItemIndex(),0), GetCellContentsString(GetSelectedItemIndex(),1));
			int iIndex = m_pApp->FindGuesserPrefixIndex(m_SelectedAffix);
			m_pApp->GetGuesserPrefixes()->Insert(pNewAffix, iIndex);
			m_bPrefixesUpdated = true;
		}
		else
		{
			m_bSuffixesLoaded = FALSE;
			CGuesserAffix m_SelectedAffix = CGuesserAffix(GetCellContentsString(GetSelectedItemIndex(),0), GetCellContentsString(GetSelectedItemIndex(),1));
			int iIndex = m_pApp->FindGuesserSuffixIndex(m_SelectedAffix);
			m_pApp->GetGuesserSuffixes()->Insert(pNewAffix, iIndex);
			m_bSuffixesUpdated = true;
		}
		LoadDataForListType(m_pltCurrentAffixPairListType);
	}
	m_pSrcLangAffix->SetLabel(_T(""));
	m_pSrcLangAffix->Layout();
	m_pTgtLangAffix->SetLabel(_T(""));
	m_pTgtLangAffix->Layout();
}
void GuesserAffixesListsDlg::OnDelete(wxCommandEvent& event) 
{
	wxString sSrc = m_pSrcLangAffix->GetLabel();
	wxString sTgt = m_pTgtLangAffix->GetLabel();
	if (sSrc.IsNull() || sSrc.Len() == 0 || sTgt.IsNull() || sTgt.Len() == 0)
	{
		if (sSrc.IsNull() || sSrc.Len() == 0)
			m_pSrcLangAffix->SetFocus();
		else
			m_pTgtLangAffix->SetFocus();
		wxMessageBox(_("Please selet an affix pair to delete"),_T(""),wxICON_INFORMATION | wxOK, this);
		event.Skip();
	}
	else
	{
		if (m_pltCurrentAffixPairListType == prefixesListType)
		{
			m_bPrefixesLoaded = FALSE;
			CGuesserAffix m_CurrentAffix = CGuesserAffix(GetCellContentsString(GetSelectedItemIndex(),0), GetCellContentsString(GetSelectedItemIndex(),1));
			int iIndex = m_pApp->FindGuesserPrefixIndex(m_CurrentAffix);
			m_pApp->GetGuesserPrefixes()->RemoveAt(iIndex);
			m_bPrefixesUpdated = true;
		}
		else
		{
			m_bSuffixesLoaded = FALSE;
			CGuesserAffix m_CurrentAffix = CGuesserAffix(GetCellContentsString(GetSelectedItemIndex(),0), GetCellContentsString(GetSelectedItemIndex(),1));
			int iIndex = m_pApp->FindGuesserSuffixIndex(m_CurrentAffix);
			m_pApp->GetGuesserSuffixes()->RemoveAt(iIndex);
			m_bSuffixesUpdated = true;
		}
		LoadDataForListType(m_pltCurrentAffixPairListType);
	}
}



void GuesserAffixesListsDlg::OnListItemSelected(wxListEvent& event)
{
	m_pSrcLangAffix->SetLabel(GetCellContentsString(GetSelectedItemIndex(), 0));
	m_pTgtLangAffix->SetLabel(GetCellContentsString(GetSelectedItemIndex(), 1));
	m_pBtnInsert->Enable();
}

AffixPairsArray* GuesserAffixesListsDlg::GetPrefixes()
{
	return m_pPrefixesPairsArray;
}

AffixPairsArray* GuesserAffixesListsDlg::GetSuffixes()
{
	return m_pSuffixesPairsArray;
}

// Load/refresh prefixes from file / app
bool GuesserAffixesListsDlg::LoadPrefixes() 
{
	m_bPrefixesLoaded = true;
		
	CGuesserAffixArray* pArray = NULL; // Pointer to current list
	m_pPrefixesPairsArray->Empty();

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

// Load/refresh suffixes from file / app
bool GuesserAffixesListsDlg::LoadSuffixes() 
{
	m_bSuffixesLoaded = true;
	
	CGuesserAffixArray* pArray = NULL; // Pointer to current list
	m_pSuffixesPairsArray->Empty();

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
	wxLogDebug(m_pAffixPairsList->GetItemText(itemIndex));
	if (itemIndex == -1)
		wxMessageBox(_("Selected affix pair not found!"),_T(""),wxICON_INFORMATION | wxOK, this);
	return itemIndex;
 
/*	for (;;) {
		itemIndex = m_pAffixPairsList->GetNextItem(itemIndex,
			                                 wxLIST_NEXT_ALL,
				                             wxLIST_STATE_SELECTED);
 
		if (itemIndex == -1) break;
 
		// Got the selected item index
		wxLogDebug(m_pAffixPairsList->GetItemText(itemIndex));
	}
*/
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




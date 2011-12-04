/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ChooseLanguageCode.cpp
/// \author			Bruce Waters
/// \date_created	2 December 2011
/// \date_revised	
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the ChooseLanguageCode class, 
///                 a dialog handler for the LiftLangFilterFunc() dialog 
/// In this dialog is shown two or more 2- or 3-letter language codes, and needs to choose
/// just one - so that code for importing will grab data from just the chosen language, rather
/// than from other languages in the same context - such as multilanguage glosses in LIFT files
/// \derivation		The ChooseLanguageCode class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ChooseLanguageCode.h"
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
#include <wx/listctrl.h>

#include "Adapt_It.h"
#include "helpers.h"
#include "ChooseLanguageCode.h"

extern bool gbIsGlossing;

// event handler table
BEGIN_EVENT_TABLE(ChooseLanguageCode, AIModalDialog)
	EVT_INIT_DIALOG(ChooseLanguageCode::InitDialog)
	EVT_BUTTON(wxID_OK, ChooseLanguageCode::OnOK)
	EVT_BUTTON(wxID_CANCEL, ChooseLanguageCode::OnCancel)
END_EVENT_TABLE()

ChooseLanguageCode::ChooseLanguageCode(wxWindow* parent) // constructor
: AIModalDialog(parent, -1, _("Select One Language Code"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	LiftLangFilterFunc(this, TRUE, TRUE);
			
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(m_pApp != NULL);
	/*
	wxListBox* m_pListTgtCodes;
	wxListBox* m_pListGlossCodes;
	wxTextCtrl* m_pTextCtrlSrcCode;
	wxButton* m_pBtnCancel;
	wxButton* m_pBtnOK;
	*/
	
	m_pListTgtCodes = (wxListBox*)FindWindowById(ID_LISTBOX_TGT);
	wxASSERT(m_pListTgtCodes != NULL);

	m_pListGlossCodes = (wxListBox*)FindWindowById(ID_LISTBOX_GLOSS);
	wxASSERT(m_pListGlossCodes != NULL);

	m_pTextCtrlSrcCode = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SRC_LANG);
	wxASSERT(m_pTextCtrlSrcCode != NULL);

	m_pBtnCancel = (wxButton*)FindWindowById(wxID_CANCEL);
	wxASSERT(m_pBtnCancel != NULL);

	m_pBtnOK = (wxButton*)FindWindowById(wxID_OK);
	wxASSERT(m_pBtnOK != NULL);
}

ChooseLanguageCode::~ChooseLanguageCode() // destructor
{
}

void ChooseLanguageCode::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	//InitDialog() is not virtual, no call needed to a base class
	int count = m_pApp->m_LIFT_multilang_codes.GetCount();
	int index;
	for (index = 0; index < count; index++)
	{
		// populate both lists, but only one will be shown enabled
		wxString aCode = m_pApp->m_LIFT_multilang_codes.Item(index);
		m_pListTgtCodes->Append(aCode);
		m_pListGlossCodes->Append(aCode);
	}
	m_pTextCtrlSrcCode->ChangeValue(m_pApp->m_LIFT_src_lang_code);

	if (gbIsGlossing)
	{
		// disable the left list, for the target language choice
		m_pListTgtCodes->Enable(FALSE);
		m_pListGlossCodes->SetFocus();
		m_pListGlossCodes->SetSelection(0);
	}
	else
	{
		// adapting mode, disable the right list, for glossing mode's 
		// language choice
		m_pListGlossCodes->Enable(FALSE);
		m_pListTgtCodes->SetFocus();
		m_pListTgtCodes->SetSelection(0);
	}
	m_pTextCtrlSrcCode->SetSelection(0L,0L);
	m_pTextCtrlSrcCode->Enable(FALSE);

	// TODO....
	// BE SURE TO CHECK that the tgt code chosen is diff from any gloss code chosen & warn
	// & force fix before accepting	
}

void ChooseLanguageCode::OnOK(wxCommandEvent& event) 
{
	// get the chosen language code, and store it on the app member for
	// that purpose
	int nSel;
	wxString s;
	if (gbIsGlossing)
	{
		nSel = m_pListGlossCodes->GetSelection();
		s = m_pListGlossCodes->GetString(nSel);
		// check that we are not mixing languages
		if (m_pApp->m_glossesLanguageCode.IsEmpty())
		{
			// the user has not set the gloss language's ethnologue code
			// as yet, so all we can do is assume the data is appropriate,
			// but we'll use the code we've obtained to set it now (actually
			// I don't think we've as yet provided any way for the user to
			// set the glossing language's ethnologue code, other than this
			// way - via a LIFT import -- this is a bit dangerous, as it 
			// could mix languages)
			m_pApp->m_glossesLanguageCode = s;
			m_pApp->m_LIFT_chosen_lang_code = s;
		}
		else
		{
			// there is a gloss language code already set, so only accept
			// the user's choice provided s/he has chosen the same code
			if (m_pApp->m_glossesLanguageCode == s)
			{
				// the codes match, so accept the LIFT data for import
				m_pApp->m_LIFT_chosen_lang_code = s;
			}
			else
			{
				// mismatched ethnologue codes, tell the user 'no deal, try again'
				wxString msg;
				msg = msg.Format(_T("The glossing-mode language already has the code %s which does not match the code %s that you chose from the LIFT file.\nImporting your chosen glossing language would mix two different languages in the one glossing knowledge base, which is not allowed.\n Either select the same code, or if there is no matching code, click Cancel to abort the import."),
					m_pApp->m_glossesLanguageCode.c_str(), s.c_str());
				wxMessageBox(msg, _T("Not the same language"));
				m_pApp->m_LIFT_chosen_lang_code.Empty();

				// keep the dialog open for another try
				return;
			}
		}
	}
	else
	{
		nSel = m_pListTgtCodes->GetSelection();
		s = m_pListTgtCodes->GetString(nSel);
		// check that we are not mixing languages
		if (m_pApp->m_targetLanguageCode.IsEmpty())
		{
			// the user has not set the target language's ethnologue code
			// as yet, so all we can do is assume the data is appropriate,
			// but we'll use the code we've obtained to set it now
			m_pApp->m_targetLanguageCode = s;
			m_pApp->m_LIFT_chosen_lang_code = s;
		}
		else
		{
			// there is a target code already set, so only accept the user's
			// choice provided s/he has chosen the same code
			if (m_pApp->m_targetLanguageCode == s)
			{
				// the codes match, so accept the LIFT data for import
				m_pApp->m_LIFT_chosen_lang_code = s;
			}
			else
			{
				// mismatched ethnologue codes, tell the user 'no deal, try again'
				wxString msg;
				msg = msg.Format(_T("The target language already has the code %s which does not match the code %s that you chose from the LIFT file.\nImporting your chosen language would mix two different languages in the one knowledge base, which is not allowed.\n Either select the same code, or if there is no matching code, click Cancel to abort the import."),
					m_pApp->m_targetLanguageCode.c_str(), s.c_str());
				wxMessageBox(msg, _T("Not the same language"));
				m_pApp->m_LIFT_chosen_lang_code.Empty();

				// keep the dialog open for another try
				return;
			}
		}
	}
	 
	event.Skip();
}

void ChooseLanguageCode::OnCancel(wxCommandEvent& event)
{
	// A cancel click is a choice to abort all that follows -- such as the
	// particular import being attempted

	event.Skip();

	// TODO
}

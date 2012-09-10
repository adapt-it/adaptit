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
#include <wx/msgdlg.h>

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
		m_arrTargetCodes.Add(aCode); // preserve the actual code
		m_arrGlossesCodes.Add(aCode); // preserve the actual code
		// set up the display lines which the user will see
		int length = aCode.Len();
		bool bIsEven = (length/2)*2 == length;
		wxString printName;
		wxString displayLineStr = aCode;
		bool bOK = GetLanguageCodePrintName(aCode, printName);
		//m_arrTargetLangNames.Add(printName); <<-- NO! must not use
		m_arrGlossesLangNames.Add(printName); // preserve for OK button handler
		if (bOK || ( !bOK && !printName.IsEmpty() ))
		{
			// add the name in parentheses
			if (bIsEven)
			{
				displayLineStr += _T("   ( "); // 3 spaces
			}
			else
			{
				displayLineStr += _T("  ( "); // 3-letter code, so 2 spaces
			}
			displayLineStr += printName;
			displayLineStr += _T(" )");
		}
		
		m_pListTgtCodes->Append(displayLineStr);
		m_pListGlossCodes->Append(displayLineStr);
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
}

void ChooseLanguageCode::OnOK(wxCommandEvent& event) 
{
	// get the chosen language code, and store it on the app member for
	// that purpose
	m_bUserCanceled = FALSE;
	int nSel;
	wxString s;
	if (gbIsGlossing)
	{
		nSel = m_pListGlossCodes->GetSelection();
		s = m_arrGlossesCodes.Item(nSel);
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
			
			// protect from choosing the target language's code
			if (m_pApp->m_targetLanguageCode == s)
			{
				// warn the user
				wxString msg;
				int style = wxYES_NO | wxCANCEL | wxNO_DEFAULT;
				msg = msg.Format(_T("The target language already has the code %s. But your chosen code for the glossing language, %s, is the same language. \nThis is allowable, but choose No if you don't want the target and glossing languages to be the same, and then select a different language or Cancel.\nDo you want the languages to be the same?"),
					m_pApp->m_targetLanguageCode.c_str(), s.c_str());
				int answer = wxMessageBox(msg, _T("Beware: same languages"), style);
				if (answer == wxYES)
				{
					// go ahead, allow the choice
					;
				}
				else if (answer == wxCANCEL)
				{
					// user wants to cancel the import
					m_bUserCanceled = TRUE;
					event.Skip();
					return;
				}
				else
				{
					// user gave a No answer, so return to the dialog
					m_bUserCanceled = FALSE;
					m_pApp->m_LIFT_chosen_lang_code.Empty();
					return; // keep the dialog open 
				}
			}
			m_bUserCanceled = FALSE;
			m_pApp->m_glossesLanguageCode = s;
			m_pApp->m_glossesName = m_arrGlossesLangNames.Item(nSel); // KBPage of prefs will display it
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
				// this next line will change the glossing language name 
				// (which may have been manually set by the user in the KBPage
				// of the Preferences (ie. Backups and Misc page) to a different
				// name) to whatever comes from the ISO639-1 or -3 standard;
				// but nothing hangs on this as far as the app's operation is
				// concerned so we'll allow it
				m_pApp->m_glossesName = m_arrGlossesLangNames.Item(nSel);
			}
			else
			{
				// mismatched ethnologue codes, tell the user 'no deal, try again'
				wxString msg;
				msg = msg.Format(_T("The glossing-mode language already has the code %s which does not match the code %s that you chose from the LIFT file.\nImporting your chosen glossing language would mix two different languages in the one glossing knowledge base, which is not allowed.\n Either select the same code, or if there is no matching code, click Cancel to abort the import."),
					m_pApp->m_glossesLanguageCode.c_str(), s.c_str());
				wxMessageBox(msg, _T("Not the same language"));
				m_pApp->m_LIFT_chosen_lang_code.Empty();
				m_bUserCanceled = FALSE;
				// keep the dialog open for another try
				return;
			}
		}
	}
	else // we are dealing with adaptation mode, populating the target text KB
	{
		m_bUserCanceled = FALSE;
		nSel = m_pListTgtCodes->GetSelection();
		s = m_arrTargetCodes.Item(nSel);
		// check that we are not mixing languages
		if (m_pApp->m_targetLanguageCode.IsEmpty() || m_pApp->m_targetLanguageCode == NOCODE)	// mrh June 2012 - account for NOCODE as well as empty
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
                // We MUST NOT enable the commented out line below. It's there deliberately
                // and deliberately commented out, to document the fact that we don't
                // override the target language name, and that it is not an oversight that
                // we don't do so!!! Whatever tgt name happens to be in a LIFT import file
                // from a who-knows-where source could be different from what we currently
                // have. The application's paths are set from the user's manually entered
                // source and target language names, so we can't let anything we don't
                // control go and change the target name willy-nilly. Functionalities would
                // break all over the place!
				// **** NO! **** -->> m_pApp->m_targetName = m_arrTargetLangNames.Item(nSel); <<--- **** NO! ****
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
	m_bUserCanceled = TRUE;
	event.Skip();
}

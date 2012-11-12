/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ChooseLanguageCode.h
/// \author			Bruce Waters
/// \date_created	2 December 2011
/// \rcs_id $Id$
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the ChooseLanguageCode class, 
///                 a dialog handler for the LiftLangFilterFunc() dialog 
/// In this dialog is shown two or more 2- or 3-letter language codes, and needs to choose
/// just one - so that code for importing will grab data from just the chosen language, rather
/// than from other languages in the same context - such as multilanguage glosses in LIFT files
/// \derivation		The ChooseLanguageCode class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ChooseLanguageCode_h
#define ChooseLanguageCode_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ChooseLanguageCode.h"
#endif

class ChooseLanguageCode : public AIModalDialog
{

public:
	ChooseLanguageCode(wxWindow* parent); // constructor
	virtual ~ChooseLanguageCode(void); // destructor
	wxString m_strTgtCode; // for output, set in OK button handler, adapting mode
	wxString m_strTgtDisplayLine; // display as, for example, "en   (English)"
	wxString m_strGlossCode; // for output, set in OK button handler, glossing mode
	wxString m_strGlossDisplayCode; // as above, e.g "knx  (Kangri)"
	// for input, get values from variables stored in CAdapt_ItApp; they are three:
	//wxString	m_LIFT_chosen_lang_code; // user's chosen language (that is, its code)
	//wxString	m_LIFT_subfield_delimiters; // for comma and any other subfield delimiters allowed
	//wxArrayString m_LIFT_multilang_codes; // the codes for each language in entries
	// These are populated from the LIFT file, and if there is more than one gloss
	// or definition language, then the dialog is displayed for the user to choose
	// which he wants; but if there is only one, no dialog is displayed and the single
	// existing one is used automatically
	bool m_bUserCanceled;
	//wxArrayString m_arrTargetLangNames; <<--- NO! Must not use this
	wxArrayString m_arrGlossesLangNames;

protected:
	wxListBox* m_pListTgtCodes;
	wxListBox* m_pListGlossCodes;
	wxTextCtrl* m_pTextCtrlSrcCode;
	wxButton* m_pBtnCancel;
	wxButton* m_pBtnOK;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);

private:
	CAdapt_ItApp* m_pApp;
	wxArrayString m_arrTargetCodes;
	wxArrayString m_arrGlossesCodes;

	DECLARE_EVENT_TABLE()
};

#endif /* ChooseLanguageCode_h */

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GuesserSettingsDlg.h
/// \author			Bill Martin
/// \date_created	27 January 2010
/// \date_revised	27 January 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CGuesserSettingsDlg class. 
/// The CGuesserSettingsDlg class provides a dialog in which the user can change the basic
/// settings for the Guesser.
/// \derivation		The CGuesserSettingsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef GuesserSettingsDlg_h
#define GuesserSettingsDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "GuesserSettingsDlg.h"
#endif

class CGuesserSettingsDlg : public AIModalDialog
{
public:
	CGuesserSettingsDlg(wxWindow* parent); // constructor
	virtual ~CGuesserSettingsDlg(void); // destructor
	// other methods
	wxCheckBox* pCheckUseGuesser;
	wxSlider* pSlider;
	wxCheckBox* pAllowCCtoOperateOnUnchangedOutput;
	wxStaticText* pStaticTextNumCorInAdaptationsGuesser;
	wxStaticText* pStaticTextNumCorInGlossingGuesser;
	wxPanel* pPanelGuessColorDisplay;
	bool bUseAdaptationsGuesser;
	int nGuessingLevel;
	int nCorrespondencesLoadedInAdaptationsGuesser;
	int nCorrespondencesLoadedInGlossingGuesser;
	bool bAllowCConUnchangedGuesserOutput;
	wxColour tempGuessHighlightColor;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnChooseGuessHighlightColor(wxCommandEvent& WXUNUSED(event));

private:
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* GuesserSettingsDlg_h */

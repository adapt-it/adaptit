/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ConsChk_Empty_noTU_Dlg.h
/// \author			Bruce Waters
/// \date_created	31 August2011
/// \rcs_id $Id$
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the ConsChk_Empty_noTU_Dlg class. 
/// The ConsChk_Empty_noTU_Dlg class provides an "inconsistency found" dialog which the
/// user employs for for fixing a KB-Document inconsistency. Deals with the document pile
/// having pSrcPhrase with m_bHasKBEntry (or m_bHasGlossingKBEntry if the current mode is
/// glossing mode) TRUE, but KB lookup failed to find a CTargetUnit for the source text at
/// this location in the document
/// \derivation		The ConsChk_Empty_noTU_Dlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ConsChk_Empty_noTU_Dlg_h
#define ConsChk_Empty_noTU_Dlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ConsChk_Empty_noTU_Dlg.h"
#endif

/// The ConsChk_Empty_noTU_Dlg class provides an "inconsistency found" dialog which the
/// user employs for for fixing a KB-Document inconsistency. Deals with the document pile
/// having pSrcPhrase with m_bHasKBEntry (or m_bHasGlossingKBEntry if the current mode is
/// glossing mode) TRUE, but KB lookup failed to find a CTargetUnit for the source text at
/// this location in the document
class ConsChk_Empty_noTU_Dlg : public AIModalDialog
{
public:
	ConsChk_Empty_noTU_Dlg(
		wxWindow* parent,
		wxString* title,
		wxString* srcPhrase, // for pSrcPhrase->m_key value
		wxString* tgtPhrase, // for the empty target phrase
		wxString* modeWord, // for modeWordAdapt or modeWordGloss
		wxString* modeWordPlusArticle, // for modeWordAdaptPlusArticle or modeWordGlossPlusArticle
		wxString* notInKBStr, // for <Not In KB>, that is, strNotInKB
		wxString* noneOfThisStr,
		bool      bShowCentered); // constructor
	virtual ~ConsChk_Empty_noTU_Dlg(); // destructor

	// member variables
	wxTextCtrl*    m_pTextCtrlSrcText;
	wxTextCtrl*	   m_pTextCtrlAorG;
	wxStaticText*  m_pMessageLabel;
	wxString	   m_messageLabelStr;
	wxRadioButton* m_pNoAdaptRadioBtn;
	wxRadioButton* m_pLeaveHoleRadioBtn;
	wxRadioButton* m_pNotInKBRadioBtn;
	wxRadioButton* m_pAorGRadioBtn;
	wxCheckBox*	   m_pAutoFixChkBox;
	wxString	   m_aorgStr;
	wxString	   m_aorgTextCtrlStr;
	wxString	   m_emptyStr;
	// the following store the creation passed-in param values
	wxString		m_sourcePhrase; // store *srcPhrase
	wxString		m_modeWord; // for modeWordAdapt or modeWordGloss
	wxString		m_modeWordPlusArticle; // for modeWordAdaptPlusArticle or modeWordGlossPlusArticle
	wxString		m_notInKBStr; // for <Not In KB>, that is, strNotInKB
	wxString		m_noneOfThisStr; // for strNoAdapt or strNoGloss
	bool			m_bDoAutoFix; // for the Auto-fix checkbox value
	wxSizer*		pConsChk_Empty_noTU_DlgSizer; // whm 31Aug11 added, but BEW says it's not needed
	enum FixItAction actionTaken;
	bool			m_bShowItCentered;
	wxPoint			m_ptBoxTopLeft;
	int				m_nTwoLineDepth;
	bool			m_bGlossing;
	wxString		m_saveAorG;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnRadioEnterEmpty(wxCommandEvent& WXUNUSED(event));
	void OnRadioLeaveHole(wxCommandEvent& WXUNUSED(event));
	void OnRadioNotInKB(wxCommandEvent& WXUNUSED(event));
	void OnRadioTypeAorG(wxCommandEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	//void OnCancel(wxCommandEvent& event);
	
private:
	void			EnableAdaptOrGlossBox(bool bEnable);

	// class attributes
	
	// other class attributes

	DECLARE_EVENT_TABLE()
};
#endif /* ConsChk_Empty_noTU_Dlg_h */

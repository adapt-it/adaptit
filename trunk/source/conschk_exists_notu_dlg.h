/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			conschk_exists_notu_dlg.h
/// \author			Bruce Waters
/// \date_created	1 Sepember 2011
/// \date_revised	
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the conschk_exists_notu_dlg class. 
/// The conschk_exists_notu_dlg class provides an "inconsistency found" dialog which the
/// user employs for for fixing a KB-Document inconsistency. Deals with the document pile
/// having pSrcPhrase with m_bHasKBEntry (or m_bHasGlossingKBEntry if the current mode is
/// glossing mode) TRUE, but KB lookup failed to find a CTargetUnit for the source text at
/// this location in the document, and there is an existing adaptation. 
/// (We won't need this for glossing mode, because the <Not In KB> is not available there,
/// and so the fix is fully determinate and can be done without showing any dialog)
/// \derivation		The conschk_exists_notu_dlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef conschk_exists_notu_dlg_h
#define conschk_exists_notu_dlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "conschk_exists_notu_dlg.h"
#endif

/// The conschk_exists_notu_dlg class provides an "inconsistency found" dialog which the
/// user employs for for fixing a KB-Document inconsistency. Deals with the document pile
/// having pSrcPhrase with m_bHasKBEntry (or m_bHasGlossingKBEntry if the current mode is
/// glossing mode) TRUE, but KB lookup failed to find a CTargetUnit for the source text at
/// this location in the document, and there is an existing adaptation. 
/// (We won't need this for glossing mode, because the <Not In KB> is not available there,
/// and so the fix is fully determinate and can be done without showing any dialog)
class conschk_exists_notu_dlg : public AIModalDialog
{
public:
	conschk_exists_notu_dlg(
		wxWindow* parent,
		wxString* title,
		wxString* srcPhrase, // for pSrcPhrase->m_key value
		wxString* adaptation, // for pSrcPhrase->m_adaption value
		wxString* notInKBStr, // for <Not In KB>, that is, strNotInKB
		bool      bShowCentered); // constructor
	virtual ~conschk_exists_notu_dlg(); // destructor

	// member variables
	wxTextCtrl*    m_pTextCtrlSrcText;
	wxTextCtrl*	   m_pTextCtrlTgtText;
	wxStaticText*  m_pStaticCtrl;
	wxString	   m_radioNotInKBLabelStr;
	wxRadioButton* m_pStoreNormallyRadioBtn;
	wxRadioButton* m_pNotInKBRadioBtn;
	wxCheckBox*	   m_pAutoFixChkBox;
	// the following store the creation passed-in param values
	wxString		m_sourcePhrase; // store *srcPhrase
	wxString		m_targetPhrase; // store *adaptation
	wxString		m_modeWordPlusArticle; // for modeWordAdaptPlusArticle
	wxString		m_notInKBStr; // for <Not In KB>, that is, strNotInKB
	bool			m_bDoAutoFix; // for the Auto-fix checkbox value
	wxSizer*		pConsChk_exists_notu_dlgSizer; // not really needed
	enum FixItAction action;
	bool			m_bShowItCentered;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

private:
	// class attributes
	
	// other class attributes

	DECLARE_EVENT_TABLE()
};
#endif /* conschk_exists_notu_dlg_h */

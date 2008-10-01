/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ConsistencyCheckDlg.h
/// \author			Bill Martin
/// \date_created	11 July 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CConsistencyCheckDlg class. 
/// The CConsistencyCheckDlg class allows the user to respond to inconsistencies between
/// the adaptations stored in the document and those stored in the knowledge base. In the
/// dialog the user may accept what is in the document as a KB entry, type a new entry to
/// be stored in the document and KB, accept <no adaptation>, or ignore the inconsistency
/// (to be fixed later). For any fixes the user can also check a box to "Auto-fix later
/// instances the same way." 
/// \derivation		The CConsistencyCheckDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ConsistencyCheckDlg_h
#define ConsistencyCheckDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ConsistencyCheckDlg.h"
#endif

class CTargetUnit;
class CAdapt_ItView;
class CAdapt_ItCanvas;
class CAdapt_ItApp;
class CKB;
class CSourcePhrase;

/// The CConsistencyCheckDlg class allows the user to respond to inconsistencies between
/// the adaptations stored in the document and those stored in the knowledge base. In the
/// dialog the user may accept what is in the document as a KB entry, type a new entry to
/// be stored in the document and KB, accept <no adaptation>, or ignore the inconsistency
/// (to be fixed later). For any fixes the user can also check a box to "Auto-fix later
/// instances the same way." 
/// \derivation		The CConsistencyCheckDlg class is derived from AIModalDialog.
class CConsistencyCheckDlg : public AIModalDialog
{
public:
	CConsistencyCheckDlg(wxWindow* parent); // constructor
	virtual ~CConsistencyCheckDlg(void); // destructor
	// other methods
	//enum { IDD = IDD_CONS_CHECK_DLG };
	// wx Note: The MFC version uses actual controls here, but since we create our controls for the
	// dialog in ConsistencyCheckDlgFunc (via wxDesigner produced Adapt_It_wdr.h and Adapt_It_wdr.cpp 
	// resource files), we only need to maintain pointers to the controls here in the ConsistencyCheckDlg 
	// class; the class constructor gets the pointers from the existing controls using FindWindowById 
	// statements and the wxGenericValidator assignments manage the transfer of data between the controls
	// and the data variables held in this class. If we try do it with the actual controls below, as
	// in MFC, we won't be interacting with the dialog's actual controls, but only with controls maintained
	// on the local stack.
	wxTextCtrl* m_pEditCtrlChVerse;
	wxListBox*	m_pListBox;
	wxTextCtrl*	m_pEditCtrlNew;
	wxTextCtrl*	m_pEditCtrlAdaptation;
	wxTextCtrl*	m_pEditCtrlKey;
	wxString	m_keyStr;
	wxString	m_adaptationStr; // adaptation, or the gloss when glossing is ON
	wxString	m_newStr;
	bool		m_bDoAutoFix;
	wxString	m_chVerse;
	bool			m_bFoundTgtUnit;
	CTargetUnit*	m_pTgtUnit;
	CAdapt_ItApp*	m_pApp;
	CKB*			m_pKBCopy;
	wxString		m_finalAdaptation;
	CSourcePhrase*  m_pSrcPhrase;
	wxPoint			m_ptBoxTopLeft;
	int				m_nTwoLineDepth;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnRadioListSelect(wxCommandEvent& WXUNUSED(event));
	void OnRadioAcceptCurrent(wxCommandEvent& WXUNUSED(event));
	void OnRadioTypeNew(wxCommandEvent& WXUNUSED(event));
	void OnSelchangeListTranslations(wxCommandEvent& WXUNUSED(event));
	void OnSetfocusEditTypeNew(wxFocusEvent& event);
	void OnButtonNoAdaptation(wxCommandEvent& WXUNUSED(event));
	void OnButtonIgnoreIt(wxCommandEvent& WXUNUSED(event));

private:
	// class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ConsistencyCheckDlg_h */

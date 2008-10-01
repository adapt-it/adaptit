/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			RetranslationDlg.h
/// \author			Bill Martin
/// \date_created	21 June 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CRetranslationDlg class. 
/// The CRetranslationDlg class provides a dialog in which the user can enter 
/// a new translation (retranslation).
/// The source text to be translated/retranslated plus the preceding and following
/// contexts are also displayed.
/// \derivation		The CRetranslationDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef RetranslationDlg_h
#define RetranslationDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "RetranslationDlg.h"
#endif

/// The CRetranslationDlg class provides a dialog in which the user can enter 
/// a new translation (retranslation).
/// The source text to be translated/retranslated plus the preceding and following
/// contexts are also displayed.
/// \derivation		The CRetranslationDlg class is derived from AIModalDialog.
class CRetranslationDlg : public AIModalDialog
{
public:
	CRetranslationDlg(wxWindow* parent); // constructor
	virtual ~CRetranslationDlg(void); // destructor // whm make all destructors virtual

	//enum { IDD = IDD_RETRANSLATION_DLG };
	wxSizer*	pRetransSizer;
	wxTextCtrl*	pSrcPrecContextBox;
	wxTextCtrl*	pSrcTextToTransBox;
	wxTextCtrl*	pRetransBox;
	wxTextCtrl*	pSrcFollContextBox;
	wxString	m_retranslation;
	wxString	m_sourceText;
	wxString	m_preContext;
	wxString	m_follContext;
	wxString	m_follContextSrc;
	wxString	m_preContextSrc;
	wxString	m_follContextTgt;
	wxString	m_preContextTgt;
	bool		m_bShowingSource;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnCopyRetranslationToClipboard(wxCommandEvent& WXUNUSED(event));
	void OnButtonToggleContext(wxCommandEvent& WXUNUSED(event));

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* RetranslationDlg_h */

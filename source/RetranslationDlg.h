/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			RetranslationDlg.h
/// \author			Bill Martin
/// \date_created	21 June 2004
/// \rcs_id $Id$
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
	//wxTextCtrl*	pSrcPrecContextBox;
	//wxTextCtrl*	pSrcTextToTransBox; // BEW 31AUg22 removed
	wxTextCtrl*	pRetransBox;
	//wxTextCtrl*	pSrcFollContextBox;
	wxString	m_retranslation;
	wxSpinCtrl* pSpinCtrl; // BEW added 2Sep22
	void SyncFontSize(int newSize);

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnCopyRetranslationToClipboard(wxCommandEvent& WXUNUSED(event));
	void OnPasteRetranslationFromClipboard(wxCommandEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnSpinValueChanged(wxSpinEvent& event);

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* RetranslationDlg_h */

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBExportImportOptionsDlg.h
/// \author			Bill Martin
/// \date_created	11 December 2011
/// \date_revised	11 December 2011
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CKBExportImportOptionsDlg class. 
/// The CKBExportImportOptionsDlg class provides a dialog in which the user can select options for KB exports or imports
/// \derivation		The CKBExportImportOptionsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef KBExportImportOptionsDlg_h
#define KBExportImportOptionsDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KBExportImportOptionsDlg.h"
#endif

class CKBExportImportOptionsDlg : public AIModalDialog
{
public:
	CKBExportImportOptionsDlg(wxWindow* parent); // constructor
	virtual ~CKBExportImportOptionsDlg(void); // destructor
	// other methods
	wxRadioBox* pRadioBoxSfmOrLIFT;
	wxCheckBox* pCheckUseSuffixExportDateTimeStamp;
	wxSizer* pKBExportImportOptionsDlgSizer;
	wxSize m_computedDlgSize; // stores the computed size of the dialog's sizer - accounting for its current layout state

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

private:
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* KBExportImportOptionsDlg_h */

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ExportSaveAsDlg.h
/// \author			Bill Martin
/// \date_created	14 June 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CExportSaveAsDlg class. 
/// The CExportSaveAsDlg class provides a dialog in which the user can indicate wither
/// the export of the source text (or target text) is to be in sfm format or RTF format.
/// The dialog also has an "Export/Filter Options" button to access that sub-dialog.
/// \derivation		The CExportSaveAsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ExportSaveAsDlg_h
#define ExportSaveAsDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ExportSaveAsDlg.h"
#endif

/// The CExportSaveAsDlg class provides a dialog in which the user can indicate wither
/// the export of the source text (or target text) is to be in sfm format or RTF format.
/// The dialog also has an "Export/Filter Options" button to access that sub-dialog.
/// \derivation		The CExportSaveAsDlg class is derived from AIModalDialog.
class CExportSaveAsDlg : public AIModalDialog
{
public:
	CExportSaveAsDlg(wxWindow* parent); // constructor
	virtual ~CExportSaveAsDlg(void); // destructor
	// other methods
	//enum {IDD = IDD_FILE_EXPORT};
	bool m_ExportToRTF;
	wxString m_StaticTitle;
	wxStaticText* pStaticTitle;
	wxCheckBox* pCheckUsePrefixExportTypeOnFilename;
	wxCheckBox* pCheckUseSuffixExportDateTimeStamp;
	wxRadioButton* pExportAsSfm;
	wxRadioButton* pExportAsRTF;
	wxTextCtrl* pTextCtrlAsStaticExpSaveAs1;
	wxTextCtrl* pTextCtrlAsStaticExpSaveAs2;
	wxTextCtrl* pTextCtrlAsStaticExpSaveAs3;

	wxSizer* pExportSaveAsSizer;
	ExportType exportType;

	void OnBnClickedRadioExportAsSfm(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioExportAsRtf(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedButtonExportFilterOptions(wxCommandEvent& WXUNUSED(event));
protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ExportSaveAsDlg_h */

/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			ExportOptionsDlg.h
/// \author			Bill Martin
/// \date_created	14 June 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CExportOptionsDlg class. 
/// The CExportOptionsDlg class provides a sub-dialog accessible either from the 
/// ExportSaveAsDlg dialog, or the ExportInterlinearDlg. It provides options whereby
/// the user can filter certain markers from the exported output; and also indicate
/// whether back translations, free translations, and notes are to be formatted as
/// boxed paragraphs, rows of interlinear tables, balloon comments, or footnotes.
/// \derivation		The CExportOptionsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ExportOptionsDlg_h
#define ExportOptionsDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ExportOptionsDlg.h"
#endif

enum TransferDirection
{
	ToControls,
	FromControls
};


/// The CExportOptionsDlg class provides a sub-dialog accessible either from the 
/// ExportSaveAsDlg dialog, or the ExportInterlinearDlg. It provides options whereby
/// the user can filter certain markers from the exported output; and also indicate
/// whether back translations, free translations, and notes are to be formatted as
/// boxed paragraphs, rows of interlinear tables, balloon comments, or footnotes.
/// \derivation		The CExportOptionsDlg class is derived from AIModalDialog.
class CExportOptionsDlg : public AIModalDialog
{
public:
	CExportOptionsDlg(wxWindow* parent); // constructor
	virtual ~CExportOptionsDlg(void); // destructor
	// other methods
	//enum { IDD = IDD_EXPORT_OPTIONS_DLG };

	// wx version note: all dialog controls are created in Adapt_It_wdr, therefore
	// we must only declare pointers here in the class and associate them with the
	// already created controls.
	wxCheckListBox* m_pListBoxSFMs;
	// whm Note: The IDD_EXPORT_OPTIONS_DLG dialog's m_listBoxSFMs control
	// is a CCheckListBox, and as such, requires that the "Owner Draw" property
	// of the dialog's attribute be set from NO to "Fixed" or "Variable" (since 
	// we just have text and it should all be of the same height we use "Fixed"), 
	// and the "Has Strings" property attribute set from FALSE to TRUE.
	wxSizer* pExportOptionsSizer;

	wxRadioButton* pRadioExportAll;
	wxRadioButton* pRadioSelectedMarkersOnly;
	wxButton* pButtonIncludeInExport;
	wxButton* pButtonFilterOutFromExport;
	wxButton* pButtonUndo;
	wxTextCtrl* pTextCtrlAsStaticExportOptions;
	wxStaticText* pStaticSpecialNote;

	// controls below added for version 3 in SFM export mode:
	wxCheckBox* pCheckPlaceFreeTrans;
	wxCheckBox* pCheckPlaceBackTrans;
	wxCheckBox* pCheckPlaceAINotes;
	// controls below added for version 3 in RTF export mode:
	wxCheckBox* pCheckPlaceFreeTransInterlinear;
	wxCheckBox* pCheckPlaceBackTransInterlinear;
	wxCheckBox* pCheckPlaceAINotesInterlinear;

	wxString excludedWholeMkrs;

	bool bFilterListChangesMade;


protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);

	void OnLbnSelchangeListSfms(wxCommandEvent& WXUNUSED(event));
	void OnCheckListBoxToggle(wxCommandEvent& event);
	void OnBnClickedRadioExportAll(wxCommandEvent& event);
	void OnBnClickedRadioExportSelectedMarkers(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedButtonIncludeInExport(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedButtonFilterOutFromExport(wxCommandEvent& WXUNUSED(event));
	void LoadActiveSFMListBox();
	void OnBnClickedButtonUndo(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedCheckPlaceFreeTrans(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedCheckPlaceAiNotes(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedCheckPlaceBackTrans(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedCheckInterlinearPlaceAiNotes(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedCheckInterlinearPlaceBackTrans(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedCheckInterlinearPlaceFreeTrans(wxCommandEvent& WXUNUSED(event));

	// wx version addition: a function to transfer data between the wxCheckListBox and
	// the m_exportFilterFlags array (similar to TransferDataTo/FromWindow)
	void TransferFilterFlagValuesBetweenCheckListBoxAndArray(enum TransferDirection transferDirection); 

private:

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ExportOptionsDlg_h */

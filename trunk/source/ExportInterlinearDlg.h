/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ExportInterlinearDlg.h
/// \author			Bill Martin
/// \date_created	14 June 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CExportInterlinearDlg class. 
/// The CExportInterlinearDlg class provides a dialog in which the user can specify
/// the inclusion or exclusion of various parts of the text or a span of references
/// for the interlinear RTF export. The dialog also has an "Export filter/options" 
/// button to access that sub-dialog.
/// \derivation		The CExportInterlinearDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ExportInterlinearDlg_h
#define ExportInterlinearDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ExportInterlinearDlg.h"
#endif

/// The CExportInterlinearDlg class provides a dialog in which the user can specify
/// the inclusion or exclusion of various parts of the text or a span of references
/// for the interlinear RTF export. The dialog also has an "Export filter/options" 
/// button to access that sub-dialog.
/// \derivation		The CExportInterlinearDlg class is derived from AIModalDialog.
class CExportInterlinearDlg : public AIModalDialog
{
public:
	CExportInterlinearDlg(wxWindow* parent); // constructor
	virtual ~CExportInterlinearDlg(void); // destructor
	// other methods
	//enum { IDD = IDD_DLG_FILE_EXPORT_INTERLINEAR };
	
	wxCheckBox* pCheckIncludeNavText;
	wxCheckBox* pCheckIncludeSrcText;
	wxCheckBox* pCheckIncludeTgtText;
	wxCheckBox* pCheckIncludeGlsText;
	wxCheckBox* pCheckNewTablesForNewLineMarkers;
	wxCheckBox* pCheckCenterTablesForCenteredMarkers;

	wxRadioButton* pRadioUsePortrait;
	wxRadioButton* pRadioUseLandscape;
	wxTextCtrl* pEditFromChapter;
	wxTextCtrl* pEditFromVerse;
	wxTextCtrl* pEditToChapter;
	wxTextCtrl* pEditToVerse;
	wxTextCtrl* pTextCtrlAsStaticExpInterlinear;
	wxCheckBox* pCheckUsePrefixExportProjNameOnFilename;
	wxCheckBox* pCheckUsePrefixExportTypeOnFilename;
	wxCheckBox* pCheckUseSuffixExportDateTimeStamp;

	bool m_bIncludeNavText;
	bool m_bIncludeSourceText;
	bool m_bIncludeTargetText;
	bool m_bIncludeGlossText;
	bool m_bOutputAll;
	bool m_bOutputPrelim;
	bool m_bOutputFinal;
	bool m_bOutputCVRange;
	bool m_bUsePgSetupDefaults;
	bool m_bPortraitOrientation;
	bool m_bDisableRangeButtons;
	bool m_bNewTableForNewLineMarker;
	bool m_bCenterTableForCenteredMarker;
	int m_nFromChapter;
	int m_nFromVerse;
	int m_nToChapter;
	int m_nToVerse;
	void OnRadioOutputAll(wxCommandEvent& WXUNUSED(event));
	void OnRadioOutputChapterVerseRange(wxCommandEvent& WXUNUSED(event));
	void OnRadioOutputPrelim(wxCommandEvent& WXUNUSED(event));
	void OnRadioOutputFinal(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedButtonRtfExportFilterOptions(wxCommandEvent& WXUNUSED(event));
	void GetMarkerInventoryFromCurrentDoc();
	void OnBnClickedCheckIncludeNavText(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedCheckNewTablesForNewlineMarkers(wxCommandEvent& WXUNUSED(event));

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

private:
	// class attributes
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ExportInterlinearDlg_h */

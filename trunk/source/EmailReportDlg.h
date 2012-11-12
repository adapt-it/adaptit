/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			EmailReportDlg.h
/// \author			Bill Martin
/// \date_created	7 November 2010
/// \rcs_id $Id$
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CEmailReportDlg class. 
/// The CEmailReportDlg class provides a dialog in which the user can report a problem
/// or provide feedback to the Adapt It developers.
/// \derivation		The CEmailReportDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef EmailReportDlg_h
#define EmailReportDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "EmailReportDlg.h"
#endif

// a helper class for CEmailReportDlg
class CLogViewer : public AIModalDialog
{
public:
	CLogViewer(wxWindow* parent); // constructor
	virtual ~CLogViewer(void); // destructor
};

class CEmailReportDlg : public AIModalDialog
{
public:
	CEmailReportDlg(wxWindow* parent); // constructor
	virtual ~CEmailReportDlg(void); // destructor
	// other methods
	enum ReportType
	{
		Report_a_problem,
		Give_feedback
	};
	enum ReportType reportType;

	bool bEmailSendSuccessful;
	bool bCurrentEmailReportWasLoadedFromFile;
	wxString LoadedFilePathAndName;

	// The following are used to determine if changes have been made in the
	// three editable fields of the report.
	bool bSubjectHasUnsavedChanges;
	bool bYouEmailAddrHasUnsavedChanges;
	bool bDescriptionBodyHasUnsavedChanges;
	bool bSendersNameHasUnsavedChanges;
	bool bPackedDocToBeAttached;
	wxString templateTextForDescription;
	wxString saveDescriptionBodyText;
	wxString saveSubjectSummary;
	wxString saveMyEmailAddress;
	wxString saveSendersName;
	wxString saveAttachDocLabel;
	wxToolTip* pBtnAttachTooltip;
	wxString saveAttachDocTooltip;
	wxString packedDocInBase64;
	wxString userLogInBase64;
	CBString userLogInASCII;

	wxString packedDocumentFileName;
	
	wxTextCtrl* pTextYourEmailAddr;
	wxTextCtrl* pTextDeveloperEmails;
	wxTextCtrl* pTextEmailSubject;
	wxTextCtrl* pTextSendersName;
	wxStaticBox* pStaticBoxTextDescription;
	wxTextCtrl* pTextDescriptionBody;
	wxCheckBox* pLetAIDevsKnowHowIUseAI;
	wxButton* pButtonViewUsageLog;
	wxStaticText* pStaticAIVersion;
	wxStaticText* pStaticReleaseDate;
	wxStaticText* pStaticDataType;
	wxStaticText* pStaticFreeMemory;
	wxStaticText* pStaticSysLocaleName;
	wxStaticText* pStaticInterfaceLanguage;
	wxStaticText* pStaticSysEncoding;
	wxStaticText* pStaticSysLayoutDir;
	wxStaticText* pStaticwxWidgetsVersion;
	wxStaticText* pStaticOSVersion;
	wxButton* pButtonSaveReportAsTextFile;
	wxButton* pButtonLoadASavedReport;
	wxButton* pButtonClose;
	wxRadioButton* pRadioSendItDirectlyFromAI;
	wxRadioButton* pRadioSendItToMyEmailPgm;
	wxButton* pButtonAttachAPackedDoc;
	wxButton* pButtonSendNow;
	wxSizer* pEmailReportDlgSizer;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnBtnSendNow(wxCommandEvent& WXUNUSED(event));
	void OnBtnSaveReportAsXmlFile(wxCommandEvent& WXUNUSED(event));
	void OnBtnLoadASavedReport(wxCommandEvent& WXUNUSED(event));
	void OnBtnClose(wxCommandEvent& WXUNUSED(event));
	void OnBtnAttachPackedDoc(wxCommandEvent& WXUNUSED(event));
	void OnBtnViewUsageLog(wxCommandEvent& WXUNUSED(event));
	void OnYourEmailAddressEditBoxChanged(wxCommandEvent& WXUNUSED(event));
	void OnSubjectSummaryEditBoxChanged(wxCommandEvent& WXUNUSED(event));
	void OnDescriptionBodyEditBoxChanged(wxCommandEvent& WXUNUSED(event));
	void OnSendersNameEditBoxChanged(wxCommandEvent& WXUNUSED(event));
	bool DoSaveReportAsXmlFile(bool PromptForSaveChanges, wxString nameSuffix, wxString& nameUsed);
	bool BuildEmailReportXMLFile(wxString filePathAndName,bool bReplaceExistingReport);
	bool bMinimumFieldsHaveData();
	wxString FormatSysInfoIntoString();

private:
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* EmailReportDlg_h */

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			EmailReportDlg.cpp
/// \author			Bill Martin
/// \date_created	7 November 2010
/// \date_revised	7 November 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CEmailReportDlg class. 
/// The CEmailReportDlg class provides a dialog in which the user can report a problem
/// or provide feedback to the Adapt It developers.
/// \derivation		The CEmailReportDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in EmailReportDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "EmailReportDlg.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
//#include <wx/valtext.h> // for wxTextValidator
#include <wx/textfile.h> // for wxTextFile
#include <wx/tokenzr.h>
#include <wx/dir.h> // for wxDir
#include "Adapt_It.h"
#include "helpers.h"
#include "XML.h"
#include "EmailReportDlg.h"

// event handler table
BEGIN_EVENT_TABLE(CEmailReportDlg, AIModalDialog)
	EVT_INIT_DIALOG(CEmailReportDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	// Samples:
	EVT_BUTTON(ID_BUTTON_SEND_NOW, CEmailReportDlg::OnBtnSendNow)
	EVT_BUTTON(ID_BUTTON_SAVE_REPORT_AS_TEXT_FILE, CEmailReportDlg::OnBtnSaveReportAsXmlFile)
	EVT_BUTTON(ID_BUTTON_LOAD_SAVED_REPORT, CEmailReportDlg::OnBtnLoadASavedReport)
	EVT_BUTTON(ID_BUTTON_VIEW_USAGE_LOG, CEmailReportDlg::OnBtnViewUsageLog)
	EVT_BUTTON(ID_BUTTON_ATTACH_PACKED_DOC, CEmailReportDlg::OnBtnAttachPackedDoc)
	EVT_BUTTON(wxID_OK, CEmailReportDlg::OnBtnClose) // The "Close" button uses wxID_OK symbol
	EVT_TEXT(ID_TEXTCTRL_MY_EMAIL_ADDR, CEmailReportDlg::OnYourEmailAddressEditBoxChanged)
	EVT_TEXT(ID_TEXTCTRL_SUMMARY_SUBJECT, CEmailReportDlg::OnSubjectSummaryEditBoxChanged)
	EVT_TEXT(ID_TEXTCTRL_DESCRIPTION_BODY, CEmailReportDlg::OnDescriptionBodyEditBoxChanged)
	//EVT_MENU(ID_SOME_MENU_ITEM, CEmailReportDlg::OnDoSomething)
	//EVT_UPDATE_UI(ID_SOME_MENU_ITEM, CEmailReportDlg::OnUpdateDoSomething)
	//EVT_BUTTON(ID_SOME_BUTTON, CEmailReportDlg::OnDoSomething)
	//EVT_CHECKBOX(ID_SOME_CHECKBOX, CEmailReportDlg::OnDoSomething)
	//EVT_RADIOBUTTON(ID_SOME_RADIOBUTTON, CEmailReportDlg::DoSomething)
	//EVT_LISTBOX(ID_SOME_LISTBOX, CEmailReportDlg::DoSomething)
	//EVT_COMBOBOX(ID_SOME_COMBOBOX, CEmailReportDlg::DoSomething)
	// ... other menu, button or control events
END_EVENT_TABLE()

CEmailReportDlg::CEmailReportDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _(""),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pEmailReportDlgSizer = EmailReportDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bEmailSendSuccessful = FALSE;
	
	pTextDeveloperEmails = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_DEVS_EMAIL_ADDR);
	wxASSERT(pTextDeveloperEmails != NULL);
	
	pTextYourEmailAddr = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_MY_EMAIL_ADDR);
	wxASSERT(pTextYourEmailAddr != NULL);
	
	pTextEmailSubject = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SUMMARY_SUBJECT);
	wxASSERT(pTextEmailSubject != NULL);
	
	// Note: STATIC_TEXT_DESCRIPTION is a pointer to a wxStaticBoxSizer which wxDesigner casts back
	// to the more generic wxSizer*, so we'll use a wxDynamicCast() to cast it back to its original
	// object class. Then we can call wxStaticBoxSizer::GetStaticBox() in it.
	wxStaticBoxSizer* psbSizer = wxDynamicCast(STATIC_TEXT_DESCRIPTION,wxStaticBoxSizer); // use dynamic case because wxDesigner cast it to wxSizer*
	pStaticBoxTextDescription = (wxStaticBox*)psbSizer->GetStaticBox();
	wxASSERT(pStaticBoxTextDescription != NULL);

	pTextDescriptionBody = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_DESCRIPTION_BODY);
	wxASSERT(pTextDescriptionBody != NULL);
	
	pLetAIDevsKnowHowIUseAI = (wxCheckBox*)FindWindowById(ID_CHECKBOX_LET_DEVS_KNOW_AI_USAGE);
	wxASSERT(pLetAIDevsKnowHowIUseAI != NULL);
	
	pButtonViewUsageLog = (wxButton*)FindWindowById(ID_BUTTON_VIEW_USAGE_LOG);
	wxASSERT(pButtonViewUsageLog != NULL);
	
	pStaticAIVersion = (wxStaticText*)FindWindowById(ID_TEXT_AI_VERSION);
	wxASSERT(pStaticAIVersion != NULL);
	
	pStaticReleaseDate = (wxStaticText*)FindWindowById(ID_TEXT_RELEASE_DATE);
	wxASSERT(pStaticReleaseDate != NULL);
	
	pStaticDataType = (wxStaticText*)FindWindowById(ID_TEXT_DATA_TYPE);
	wxASSERT(pStaticDataType != NULL);
	
	pStaticFreeMemory = (wxStaticText*)FindWindowById(ID_TEXT_FREE_MEMORY);
	wxASSERT(pStaticFreeMemory != NULL);
	
	pStaticSysLocaleName = (wxStaticText*)FindWindowById(ID_TEXT_SYS_LOCALE);
	wxASSERT(pStaticSysLocaleName != NULL);
	
	pStaticInterfaceLanguage = (wxStaticText*)FindWindowById(ID_TEXT_INTERFACE_LANGUAGE);
	wxASSERT(pStaticInterfaceLanguage != NULL);
	
	pStaticSysEncoding = (wxStaticText*)FindWindowById(ID_TEXT_SYS_ENCODING);
	wxASSERT(pStaticSysEncoding != NULL);
	
	pStaticSysLayoutDir = (wxStaticText*)FindWindowById(ID_TEXT_SYS_LAYOUT_DIR);
	wxASSERT(pStaticSysLayoutDir != NULL);
	
	pStaticwxWidgetsVersion = (wxStaticText*)FindWindowById(ID_TEXT_WXWIDGETS_VERSION);
	wxASSERT(pStaticwxWidgetsVersion != NULL);
	
	pStaticOSVersion = (wxStaticText*)FindWindowById(ID_TEXT_OS_VERSION);
	wxASSERT(pStaticOSVersion != NULL);
	
	pButtonSaveReportAsTextFile = (wxButton*)FindWindowById(ID_BUTTON_SAVE_REPORT_AS_TEXT_FILE);
	wxASSERT(pButtonSaveReportAsTextFile != NULL);
	
	pButtonLoadASavedReport = (wxButton*)FindWindowById(ID_BUTTON_LOAD_SAVED_REPORT);
	wxASSERT(pButtonLoadASavedReport != NULL);
	
	pButtonClose = (wxButton*)FindWindowById(wxID_OK);
	wxASSERT(pButtonClose != NULL);
	
	pRadioSendItDirectlyFromAI = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_SEND_DIRECTLY_FROM_AI);
	wxASSERT(pRadioSendItDirectlyFromAI != NULL);
	
	pRadioSendItToMyEmailPgm = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_SEND_TO_MY_EMAIL);
	wxASSERT(pRadioSendItToMyEmailPgm != NULL);
	
	pButtonAttachAPackedDoc = (wxButton*)FindWindowById(ID_BUTTON_ATTACH_PACKED_DOC);
	wxASSERT(pButtonAttachAPackedDoc != NULL);
	
	pButtonSendNow = (wxButton*)FindWindowById(ID_BUTTON_SEND_NOW);
	wxASSERT(pButtonSendNow != NULL);
	
	// use wxValidator for simple dialog data transfer
	// sample text control initialization below:
	//wxTextCtrl* pEdit;
	//pEdit = (wxTextCtrl*)FindWindowById(IDC_TEXTCONTROL);
	//pEdit->SetValidator(wxGenericValidator(&m_stringVariable));
	//pEdit->SetBackgroundColour(sysColorBtnFace);

	// sample radio button control initialization below:
	//wxRadioButton* pRadioB;
	//pRadioB = (wxRadioButton*)FindWindowById(IDC_RADIO_BUTTON);
	//pRadioB->SetValue(TRUE);
	//pRadioB->SetValidator(wxGenericValidator(&m_bVariable));

	// other attribute initializations
}

CEmailReportDlg::~CEmailReportDlg() // destructor
{
	
}

void CEmailReportDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	CAdapt_ItApp* pApp = &wxGetApp();
	
	packedDocumentFileName = _T("");
	bSubjectHasUnsavedChanges = FALSE;
	bYouEmailAddrHasUnsavedChanges = FALSE;
	bDescriptionBodyHasUnsavedChanges = FALSE;
	LoadedFilePathAndName = _T("");

	pTextDeveloperEmails->ChangeValue(pApp->m_aiDeveloperEmailAddresses);

	wxString templateText1,templateText2;
	if (reportType == Report_a_problem)
	{
		SetTitle(_("Report a problem"));
		// set the Title of the wxStaticBoxSizer surrounding the edit box
		pStaticBoxTextDescription->SetLabel(_("Please provide a descriptioin of the problem (for body of Email)"));
		// Add template text guide to the multiline wxTextCtrl to guide the user
		templateText1 = _("What steps will reproduce the problem?");
		templateText1 += _T("\n1. \n2. \n3. \n");
		templateText2 = _("Provide any other information you think would be helpful:");
		templateText2 += _T("\n");
		templateText1 += templateText2;
		templateTextForDescription = templateText1; // can use this to compare a loaded report and know if it differs from the template
	}
	else if (reportType == Give_feedback)
	{
		SetTitle(_T("Give feedback"));
		// set the Title of the wxStaticBoxSizer surrounding the edit box
		pStaticBoxTextDescription->SetLabel(_("Please provide feedback details (for body of Email)"));
		// Add template text guide to the multiline wxTextCtrl to guide the user
		templateText1 = _("In what ways has Adapt It been a helpful tool for your work?");
		templateText1 += _T("\n1. \n2. \n3. \n");
		templateText2 = _("What suggestions do you have that would make Adapt It more helpful?");
		templateText2 += _T("\n1. \n2. \n");
		templateText1 += templateText2;
		templateTextForDescription = templateText1; // can use this to compare a loaded report and know if it differs from the template
	}
	// load template text into the Description edit box
	pTextDescriptionBody->ChangeValue(templateText1);
	
	// The following three strings are used to determine if the user has made changes to
	// the template description, the subject/summary or the user's email address.
	saveDescriptionBodyText = pTextDescriptionBody->GetValue(); // it will be same as templateText1
	saveSubjectSummary = pTextEmailSubject->GetValue(); // it will be empty
	saveMyEmailAddress = pTextYourEmailAddr->GetValue(); // it will be empty
	
	// Fill in the System Information fields
	pStaticAIVersion->SetLabel(pApp->GetAppVersionOfRunningAppAsString()); //ID_TEXT_AI_VERSION
	// Get date from string constants at beginning of Adapt_It.h.
	wxString versionDateStr;
	versionDateStr.Empty();
	versionDateStr << VERSION_DATE_YEAR;
	versionDateStr += _T("-");
	versionDateStr << VERSION_DATE_MONTH;
	versionDateStr += _T("-");
	versionDateStr << VERSION_DATE_DAY;
	pStaticReleaseDate->SetLabel(versionDateStr); //ID_TEXT_RELEASE_DATE
	
	wxString UnicodeOrAnsiBuild = _T("Regular (not Unicode)");
#ifdef _UNICODE
	UnicodeOrAnsiBuild = _T("UNICODE");
#endif
	pStaticDataType->SetLabel(UnicodeOrAnsiBuild); //ID_TEXT_DATA_TYPE
	
	wxString memSizeStr;
	wxMemorySize memSize;
	wxLongLong MemSizeMB;
	memSize = ::wxGetFreeMemory();
	MemSizeMB = memSize / 1048576;
	memSizeStr << MemSizeMB;
	pStaticFreeMemory->SetLabel(memSizeStr); //ID_TEXT_FREE_MEMORY
	
	wxString tempStr;
	tempStr.Empty();
	wxString locale = pApp->m_pLocale->GetLocale();
	if (!locale.IsEmpty())
	{
		tempStr = locale;
	}
	if (!pApp->m_pLocale->GetCanonicalName().IsEmpty())
	{
		tempStr += _T(' ');
		tempStr += pApp->m_pLocale->GetCanonicalName();
	}
	pStaticSysLocaleName->SetLabel(tempStr); //ID_TEXT_SYS_LOCALE
	
	wxString strUILanguage;
	// Fetch the UI language info from the global currLocalizationInfo struct
	strUILanguage = pApp->currLocalizationInfo.curr_fullName;
	strUILanguage.Trim(FALSE);
	strUILanguage.Trim(TRUE);
	pStaticInterfaceLanguage->SetLabel(strUILanguage); //ID_TEXT_INTERFACE_LANGUAGE
	
	pStaticSysEncoding->SetLabel(pApp->m_systemEncodingName); //ID_TEXT_SYS_ENCODING
	
	wxLayoutDirection layoutDir = pApp->GetLayoutDirection();
	wxString layoutDirStr;
	layoutDirStr.Empty();
	switch (layoutDir)
	{
	case wxLayout_LeftToRight: // wxLayout_LeftToRight has enum value of 1
		layoutDirStr = _("Left-to-Right");
		break;
	case wxLayout_RightToLeft: // wxLayout_LeftToRight has enum value of 2
		layoutDirStr = _("Right-to-Left");
		break;
	default:
		layoutDirStr = _("System Default");// = wxLayout_Default which has enum value of 0
	}
	pStaticSysLayoutDir->SetLabel(layoutDirStr); //ID_TEXT_SYS_LAYOUT_DIR
	
	wxString versionStr;
	versionStr.Empty();
	versionStr << wxMAJOR_VERSION;
	versionStr << _T(".");
	versionStr << wxMINOR_VERSION;
	versionStr << _T(".");
	versionStr << wxRELEASE_NUMBER;
	pStaticwxWidgetsVersion->SetLabel(versionStr); //ID_TEXT_WXWIDGETS_VERSION
	
	wxString osVersionStr;
	wxString archName,OSSystemID,OSSystemIDName,hostName;
	int OSMajorVersion, OSMinorVersion;
	wxPlatformInfo platInfo;
	archName = platInfo.GetArchName(); // returns "32 bit" on Windows
	OSSystemID = platInfo.GetOperatingSystemIdName(); // returns "Microsoft Windows NT" on Windows
	OSSystemIDName = platInfo.GetOperatingSystemIdName();
	//OSMajorVersion = platInfo.GetOSMajorVersion();
	//OSMinorVersion = platInfo.GetOSMinorVersion();
	wxOperatingSystemId sysID;
	sysID = ::wxGetOsVersion(&OSMajorVersion,&OSMinorVersion);
	osVersionStr = archName;
	osVersionStr += _T(' ') + OSSystemID;
	osVersionStr += _T(' ');
	osVersionStr << OSMajorVersion;
	osVersionStr += _T('.');
	osVersionStr << OSMinorVersion;
	pStaticOSVersion->SetLabel(osVersionStr); //ID_TEXT_OS_VERSION
	
	pEmailReportDlgSizer->Layout();

	pTextYourEmailAddr->SetFocus(); // start with focus in sender's email address field
}

// OnBtnSendNow(), unless returned prematurely because of incomplete information,
// or failure of the email to be sent, invokes EndModal(wxID_OK) to close the dialog.
void CEmailReportDlg::OnBtnSendNow(wxCommandEvent& WXUNUSED(event)) 
{
	if (!bMinimumFieldsHaveData())
		return;

	// TODO: implement the acutal sending of the report directly or to the user's email program
	// here. If successful set bEmailSendSuccessful to TRUE;

	// If the email cannot be sent, automatically invoke the OnBtnSaveReportAsXmlFile() 
	// handler so the user can save any edits made before closing the dialog.
	if (!bEmailSendSuccessful)
	{
		wxMessageBox(_("Your email could not be sent at this time. Adapt It will save a copy of your report in your work folder (for sending at a later time)."),_("Email protocols not yet implemented!"),wxICON_INFORMATION);
		bool bPromptForMissingData = FALSE;
		bool bSavedOK;
		bSavedOK = DoSaveReportAsXmlFile(bPromptForMissingData);
	}

	EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}

void CEmailReportDlg::OnBtnSaveReportAsXmlFile(wxCommandEvent& WXUNUSED(event))
{
	bool bPromptForMissingData = TRUE;
	bool bSavedOK;
	bSavedOK = DoSaveReportAsXmlFile(bPromptForMissingData);
	
}

bool CEmailReportDlg::DoSaveReportAsXmlFile(bool PromptForSaveChanges)
{
	if (PromptForSaveChanges && !bMinimumFieldsHaveData())
		return TRUE;

	CAdapt_ItApp* pApp = &wxGetApp();
	wxString reportPathAndName;
	wxString uniqueReportName;
	wxString rptType;
	if (reportType == Report_a_problem)
	{
		rptType = _T("Problem");
	}
	else
	{
		rptType = _T("Feedback");
	}
	
	// If the data currently in view was loaded from an existing email report file
	// save it back to the same file name, otherwise get a unique file name.
	if (bCurrentEmailReportWasLoadedFromFile && !LoadedFilePathAndName.IsEmpty())
	{
		reportPathAndName = LoadedFilePathAndName;
	}
	else
	{
		reportPathAndName = pApp->m_emailReportFolderPathOnly; // no PathSeparator or filename at end of path
		reportPathAndName += pApp->PathSeparator + _T("AI_Report") + rptType + _T(".xml");
	}
	
	// Compose xml report and write it to reportPathAndName or uniqueReportName
	// if reportPathAndName doesn't already exist the uniqueReportName returned by 
	// GetUniqueIncrementedFileName() below will be of the form AI_ReportProblem.xml or
	// AI_ReportFeedback.xml. If reportPathAndName already exists the function below
	// will return the file name at the end of the path in the form of AI_ReportProblem01.xml
	// or AI_ReportFeedback01.xml.
	bool bReportBuiltOK = TRUE;
	bool bReplaceExistingReport = bCurrentEmailReportWasLoadedFromFile;
	if (!wxFileExists(reportPathAndName))
	{
		// save the file as reportPathAndName
		bReportBuiltOK = BuildEmailReportXMLFile(reportPathAndName,bReplaceExistingReport);
		
	}
	else
	{
		if (!bReplaceExistingReport)
		{
			reportPathAndName = GetUniqueIncrementedFileName(reportPathAndName,2,_T("")); 
			// save the file using the reportPathAndName which is now guaranteed to be a unique name
		}
		bReportBuiltOK = BuildEmailReportXMLFile(reportPathAndName,bReplaceExistingReport);
	}
	wxString msg;
	if (!bReportBuiltOK)
	{
		msg = msg.Format(_("Could not create the email report to disk at the following work folder path:\n   %s"),reportPathAndName.c_str());
		wxMessageBox(msg,_("Problem creating or writing the xml file"),wxICON_WARNING);
	}
	else
	{
		msg = msg.Format(_("The email report was saved at the following work folder path:\n   %s"),reportPathAndName.c_str());
		wxMessageBox(msg,_("Your report was saved for later use or reference"),wxICON_INFORMATION);
		bSubjectHasUnsavedChanges = FALSE;
		bYouEmailAddrHasUnsavedChanges = FALSE;
		bDescriptionBodyHasUnsavedChanges = FALSE;
	}
	return bReportBuiltOK;
}

void CEmailReportDlg::OnBtnLoadASavedReport(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = &wxGetApp();
	// Adapt It saves email reports in the current work folder (which may be a custom work folder) at
	// the path m_emailReportFolderPathOnly. Get an array of the report file names that match the
	// reportType.
	// 
	// Prompt to save any changes before loading another report
	int response = wxYES;
	if (bSubjectHasUnsavedChanges || bYouEmailAddrHasUnsavedChanges || bDescriptionBodyHasUnsavedChanges)
	{
		response = wxMessageBox(_("You made changes to this report - Do you want to save those changes?"),_T(""),wxYES_NO | wxICON_INFORMATION);
		if (response == wxYES)
		{
			bool bPromptForMissingData = TRUE;
			bool bSavedOK;
			bSavedOK = DoSaveReportAsXmlFile(bPromptForMissingData);
		}
	}
	wxArrayString reportsArray;
	reportsArray.Clear();
	wxString fileNameStr;
	wxString rptType;
	wxDir dir(pApp->m_emailReportFolderPathOnly);
	if (dir.IsOpened())
	{
		if (reportType == Report_a_problem)
		{
			if (dir.GetFirst(&fileNameStr,_T("AI_ReportProblem*.xml")))
			{
				reportsArray.Add(fileNameStr);
			}
			while (dir.GetNext(&fileNameStr))
			{
				reportsArray.Add(fileNameStr);
			}
			rptType = _T("Problem");
		}
		else if (reportType == Give_feedback)
		{
			if (dir.GetFirst(&fileNameStr,_T("AI_ReportFeedback*.xml")))
			{
				reportsArray.Add(fileNameStr);
			}
			while (dir.GetNext(&fileNameStr))
			{
				reportsArray.Add(fileNameStr);
			}
			rptType = _T("Feedback");
		}
	}
	wxString msg;
	msg = msg.Format(_("Select one of the following previously saved %s report files:"),rptType.c_str());
	wxSingleChoiceDialog ChooseEmailReportToLoad(this,msg,_T("Saved email reports located in your work folder"),reportsArray);
	if (ChooseEmailReportToLoad.ShowModal() == wxID_OK)
	{
		int userSelectionInt;
		wxString userSelectionStr;
		userSelectionStr = ChooseEmailReportToLoad.GetStringSelection();
		userSelectionInt = ChooseEmailReportToLoad.GetSelection();
		bool bReadOK;
		wxString pathAndName = pApp->m_emailReportFolderPathOnly + pApp->PathSeparator + userSelectionStr;
		bReadOK = ReadEMAIL_REPORT_XML(pathAndName);
		if (bReadOK)
		{
			wxASSERT(pApp->m_pEmailReportData != NULL);
			pTextYourEmailAddr->ChangeValue(pApp->m_pEmailReportData->fromAddress);
			pTextEmailSubject->ChangeValue(pApp->m_pEmailReportData->subjectSummary);
			pTextDescriptionBody->ChangeValue(pApp->m_pEmailReportData->emailBody);
			wxString packedDocPathAndName;
			packedDocPathAndName = pApp->m_pEmailReportData->packedDocumentFilePathName;
			if (!packedDocPathAndName.IsEmpty())
			{
				// The packedDocPathAndName previously pointed to a packed document.
				// Verify that the packed document still exists at the packedDocPathAndName path
				// If it no longer exists, notify user that it won't be sent via email unless
				// it is created again by clicking on the "Attach a packed document" button before
				// sending.
				if (!wxFileExists(packedDocPathAndName))
				{
					wxString pdMsg1,pdMsg2,pdMsg3;
					pdMsg1 = _("The email you loaded previously had a packed document attached at the following path:");
					pdMsg2 = pdMsg2.Format(_T("\n   %s\n"),packedDocPathAndName.c_str());
					pdMsg1 += pdMsg2;
					pdMsg3 = _("If you want to attach a packed document, you must attach the packed doument\nagain before sending this email. Do you want to attach one now?");
					pdMsg1 += pdMsg3;
					int response;
					response = wxMessageBox(pdMsg1,_("Could not find the packed document in your work folder"),wxYES_NO | wxICON_WARNING);
				}
			
			}
			wxString userLogPathAndName;
			userLogPathAndName = pApp->m_pEmailReportData->usageLogFilePathName;
			if (!userLogPathAndName.IsEmpty())
			{
				// The userLogPathAndName previously pointed to the user log.
				// Verify that the user log exists at the userLogPathAndName path
				// If the log doesn't exist, notify the user
			}
			bCurrentEmailReportWasLoadedFromFile = TRUE;
			LoadedFilePathAndName = userSelectionStr;
		}
	}
}

void CEmailReportDlg::OnBtnClose(wxCommandEvent& WXUNUSED(event))
{
	// Check for changes before closing, and allow user to save changes before closing
	int response = wxYES;
	if (bSubjectHasUnsavedChanges || bYouEmailAddrHasUnsavedChanges || bDescriptionBodyHasUnsavedChanges)
	{
		response = wxMessageBox(_("You made changes to this report - Do you want to save those changes?"),_T("This dialog is about to close..."),wxYES_NO | wxICON_INFORMATION);
		if (response == wxYES)
		{
			bool bPromptForMissingData = FALSE;
			bool bSavedOK;
			bSavedOK = DoSaveReportAsXmlFile(bPromptForMissingData);
		}
	}
	EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}

void CEmailReportDlg::OnBtnAttachPackedDoc(wxCommandEvent& WXUNUSED(event))
{
	wxMessageBox(_("This button not yet implemented!"),_T(""),wxICON_INFORMATION);
}

void CEmailReportDlg::OnBtnViewUsageLog(wxCommandEvent& WXUNUSED(event))
{
	wxMessageBox(_("This button not yet implemented!"),_T(""),wxICON_INFORMATION);
}

// Builds the xml report as Problem or Feedback report
// Returns TRUE if the wxTextfile creation and opening process was successful
// FALSE otherwise.
bool CEmailReportDlg::BuildEmailReportXMLFile(wxString filePathAndName, bool bReplaceExistingReport)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxTextFile textFile;
	bool bOK = TRUE;
	if (bReplaceExistingReport)
	{
		wxASSERT(::wxFileExists(filePathAndName));
		bOK = ::wxRemoveFile(filePathAndName);
	}
	if (bOK)
	{
		bOK = textFile.Create(filePathAndName); // Create() must only be called when the file doesn't exist - verified above
		if (bOK)
		{
			// Note textFile is empty at this point
			bOK = textFile.Open(filePathAndName);
			wxString composeXmlStr;
			if (textFile.IsOpened())
			{
				CBString xmlPrologue;
				wxString tab1,tab2,tab3;
				tab1 = _T("\t");
				tab2 = _T("\t\t");
				tab3 = _T("\t\t\t");
				pApp->GetEncodingStringForXmlFiles(xmlPrologue); // builds xmlPrologue and adds "\r\n" to it
				composeXmlStr = wxString::FromAscii(xmlPrologue); // first string in xml file
				composeXmlStr.Replace(_T("\r\n"),_T("")); // remove the ending \r\n added by GetEncodingStringForXmlFiles wxTextFile::AddLine adds its own eol
				textFile.AddLine(composeXmlStr);
				// now we start building the actual xml part of the file (remainder of file)
				// We use the data stored in m_pUserProfiles
				composeXmlStr.Empty();
				composeXmlStr += _T('<');
				if (reportType == Report_a_problem)
				{
					composeXmlStr += wxString::FromAscii(adaptitproblemreport);
				}
				else
				{
					composeXmlStr += wxString::FromAscii(adaptitfeedbackreport);
				}
				composeXmlStr += _T('>');
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
				
				composeXmlStr = tab1 + _T('<') + wxString::FromAscii(reportemailheader);
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
				
				wxString tempStr;
				composeXmlStr = tab2 + wxString::FromAscii(emailfrom);
				tempStr = pTextYourEmailAddr->GetValue();
				composeXmlStr += _T("=\"") + tempStr + _T("\"");
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
				
				// The email subject and email body are the only two user editable fields that
				// might need to have entities replaced with their xml representations.
				composeXmlStr = tab2 + wxString::FromAscii(emailsubject);
				tempStr = pTextEmailSubject->GetValue();
				// Note: wxTextFile takes care of the conversion of UTF16 to UTF8 when it writes the file.
				// Call the App's InsertEntities() which takes wxString and returns wxString
				tempStr = pApp->InsertEntities(tempStr);
				composeXmlStr += _T("=\"") + tempStr + _T("\"");
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
			
				composeXmlStr = tab1 + _T('>');
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();

				composeXmlStr = tab1 + _T("</") + wxString::FromAscii(reportemailheader) + _T('>');
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();

				composeXmlStr = tab1 + _T('<') + wxString::FromAscii(reportemailbody) + _T('>');
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
				
				wxString str = pTextDescriptionBody->GetValue();
				// Note: wxTextFile takes care of the conversion of UTF16 to UTF8 when it writes the file.
				// Call the App's InsertEntities() which takes wxString and returns wxString
				composeXmlStr = pApp->InsertEntities(str);
				// Note: The embedded eols from wxTextCtrl are \n chars on all platforms. 
				// We need to break up each of these into separate lines for our wxTextFile 
				// storage
				wxStringTokenizer tkz(composeXmlStr,_T("\n"));
				wxString tempToken;
				while (tkz.HasMoreTokens())
				{
					// we don't indent these - they are PCDATA within the <ReportEmailBody> ... </ReportEmailBody> tags
					tempToken = tkz.GetNextToken();
					textFile.AddLine(tempToken);
					composeXmlStr.Empty();
				}
				
				composeXmlStr = tab1 + _T("</") + wxString::FromAscii(reportemailbody) + _T('>');
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
				
				composeXmlStr = tab1 + _T('<') + wxString::FromAscii(reportattachmentusagelog);
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
				
				composeXmlStr = tab2 + wxString::FromAscii(usagelogfilepathname);
				tempStr = pApp->m_usageLogFilePathAndName;
				composeXmlStr += _T("=\"") + tempStr + _T("\"");
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
			
				composeXmlStr.Empty();
				composeXmlStr = tab1 + _T('>');
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
			
				composeXmlStr = tab1 + _T("</") + wxString::FromAscii(reportattachmentusagelog) + _T('>');
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
				
				composeXmlStr = tab1 + _T('<') + wxString::FromAscii(reportattachmentpackeddocument);
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
				
				composeXmlStr = tab2 + wxString::FromAscii(packeddocumentfilepathname);
				tempStr.Empty();
				// Add the packeddocumentfilepathname attribute's value only when user has actually 
				// clicked the "Attach a packed document" button and created one.
				if (!packedDocumentFileName.IsEmpty())
				{
					tempStr = pApp->m_packedDocumentFilePathOnly + pApp->PathSeparator + packedDocumentFileName;
				}
				composeXmlStr += _T("=\"") + tempStr + _T("\"");
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
			
				composeXmlStr.Empty();
				composeXmlStr = tab1 + _T('>');
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
			
				composeXmlStr = tab1 + _T("</") + wxString::FromAscii(reportattachmentpackeddocument) + _T('>');
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
				
				composeXmlStr = _T("</");
				if (reportType == Report_a_problem)
				{
					composeXmlStr += wxString::FromAscii(adaptitproblemreport);
				}
				else
				{
					composeXmlStr += wxString::FromAscii(adaptitfeedbackreport);
				}
				composeXmlStr += _T('>');
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();

				textFile.Write();
				textFile.Close();

				if (bReplaceExistingReport)
				{
					bCurrentEmailReportWasLoadedFromFile = FALSE;
					LoadedFilePathAndName = _T("");
				}
			}
		}
	}
	return bOK;
}

bool CEmailReportDlg::bMinimumFieldsHaveData()
{
	if (pTextYourEmailAddr->GetValue().IsEmpty())
	{
		wxMessageBox(_("Please enter your email address"),_T("Information missing or incomplete"),wxICON_WARNING);
		pTextYourEmailAddr->SetFocus();
		return FALSE; // keep dialog open
	}
	if (pTextEmailSubject->GetValue().IsEmpty())
	{
		wxMessageBox(_("Please enter a brief summary/subject for your email"),_T("Information missing or incomplete"),wxICON_WARNING);
		pTextEmailSubject->SetFocus();
		return FALSE; // keep dialog open
	}
	// For the Description/Body text a minimum would be that it must differ from the original template text.
	// It may be a loaded report in which the Description was already filled it, so we don't want to compare
	// the edit box contents with the saveDescriptionBodyText in this case.
	if (pTextDescriptionBody->GetValue() == templateTextForDescription)
	{
		wxMessageBox(_("Please enter provide some description for the body of your email"),_T("Information missing or incomplete"),wxICON_WARNING);
		pTextDescriptionBody->SetFocus();
		return FALSE; // keep dialog open
	}
	// If we get here all necessary fields have data.
	return TRUE;
}

void CEmailReportDlg::OnYourEmailAddressEditBoxChanged(wxCommandEvent& WXUNUSED(event))
{
	// Set bThereAreUnsavedChanges to TRUE if the edit box change results in
	// an actual change from save... values established in InitDialgo()
	if (pTextYourEmailAddr->GetValue() != saveMyEmailAddress)
	{
		bYouEmailAddrHasUnsavedChanges = TRUE;
	}
	else
	{
		bYouEmailAddrHasUnsavedChanges = FALSE;
	}
}
void CEmailReportDlg::OnSubjectSummaryEditBoxChanged(wxCommandEvent& WXUNUSED(event))
{
	// Set bThereAreUnsavedChanges to TRUE if the edit box change results in
	// an actual change from save... values established in InitDialgo()
	if (pTextEmailSubject->GetValue() != saveSubjectSummary)
	{
		bSubjectHasUnsavedChanges = TRUE;
	}
	else
	{
		bSubjectHasUnsavedChanges = FALSE;
	}
}
void CEmailReportDlg::OnDescriptionBodyEditBoxChanged(wxCommandEvent& WXUNUSED(event))
{
	// Set bThereAreUnsavedChanges to TRUE if the edit box change results in
	// an actual change from save... values established in InitDialgo()
	if (pTextDescriptionBody->GetValue() != saveDescriptionBodyText)
	{
		bDescriptionBodyHasUnsavedChanges = TRUE;
	}
	else
	{
		bDescriptionBodyHasUnsavedChanges = FALSE;
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			EmailReportDlg.cpp
/// \author			Bill Martin
/// \date_created	7 November 2010
/// \rcs_id $Id$
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
#include <wx/filename.h> // for wxFileName
#include <wx/wfstream.h> // for wxFileInputStream
#include <wx/txtstrm.h> // for wxTextInputStream
#include <wx/zipstrm.h> // for wxZipInputStream & wxZipOutputStream
#include <wx/mstream.h> // for wxMemoryInputStream
#include <wx/tooltip.h> // for wxToolTip
#include <wx/hyperlink.h> // whm 6Aug2019 added

// whm 9Jun12 added the following for wxWidgets 2.9.3
#if wxCHECK_VERSION(2,9,1)
#include <wx/base64.h> // for wxBase64 manipulations
#else
#include "base64.h"
#endif

// libcurl includes:
#include <curl/curl.h>
//#include <curl/types.h>
#include <curl/easy.h>

#include "Adapt_It.h"
#include "Adapt_ItDoc.h"
#include "helpers.h"
#include "XML.h"
#include "EmailReportDlg.h"

// whm 9Jun12 remove the use of base64.h and base64.cpp since wxWidgets 2.9.3 now has built-in
// functions to encode and decode base64.
//#include "base64.h" // for wxBase64 encode of zipped/packed attachments
	
static int totalBytesSent = 0;

// a helper class for CEmailReportDlg
CLogViewer::CLogViewer(wxWindow* parent)
: AIModalDialog(parent, -1, _("View User Log File"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	LogViewerFunc(this, TRUE, TRUE); // wxDesigner dialog function

    // whm 5Mar2019 Note: The LogViewerFunc() dialog simply has a "Close" (wxID_OK) button that is
    // aligned right at the bottom of the dialog. We need not use wxStdDialogButtonSizer, nor
    // the ReverseOkCancelButtonsForMac() function in this case.

	wxStaticText* pPathAndName;
	pPathAndName = (wxStaticText*)FindWindowById(ID_TEXT_LOG_FILE_PATH_AND_NAME);
	wxASSERT(pPathAndName != NULL);
	wxTextCtrl* pLoggedText;
	pLoggedText = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_LOGGED_TEXT);
	wxASSERT(pLoggedText != NULL);
	CAdapt_ItApp* pApp = &wxGetApp();
	pPathAndName->SetLabel(pApp->m_usageLogFilePathAndName);
	bool bLoadedOK;
	bLoadedOK = pLoggedText->LoadFile(pApp->m_usageLogFilePathAndName);
	if (!bLoadedOK)
	{
		wxString msg;
		msg = msg.Format(_("Cannot display user log:\n%s"),pApp->m_usageLogFilePathAndName.c_str());
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(msg,_T(""),wxICON_INFORMATION | wxOK);
	}
}

CLogViewer::~CLogViewer() // destructor
{
}

// event handler table
BEGIN_EVENT_TABLE(CEmailReportDlg, AIModalDialog)
	EVT_INIT_DIALOG(CEmailReportDlg::InitDialog)
	EVT_BUTTON(ID_BUTTON_SEND_NOW, CEmailReportDlg::OnBtnSendNow)
	EVT_BUTTON(ID_BUTTON_SAVE_REPORT_AS_TEXT_FILE, CEmailReportDlg::OnBtnSaveReportAsXmlFile)
	EVT_BUTTON(ID_BUTTON_LOAD_SAVED_REPORT, CEmailReportDlg::OnBtnLoadASavedReport)
	EVT_BUTTON(ID_BUTTON_VIEW_USAGE_LOG, CEmailReportDlg::OnBtnViewUsageLog)
	EVT_BUTTON(ID_BUTTON_ATTACH_PACKED_DOC, CEmailReportDlg::OnBtnAttachPackedDoc)
	EVT_BUTTON(wxID_OK, CEmailReportDlg::OnBtnClose) // The "Close" button uses wxID_OK symbol
    EVT_RADIOBUTTON(ID_RADIOBUTTON_SEND_DIRECTLY_FROM_AI, CEmailReportDlg::OnRadioBtnSendDirectlyFromAI)
    EVT_RADIOBUTTON(ID_RADIOBUTTON_SEND_TO_MY_EMAIL, CEmailReportDlg::OnRadioBtnSendToEmail)
    EVT_HYPERLINK(ID_HYPERLINK_MAILTO, CEmailReportDlg::OnHyperLinkMailToClicked)
    EVT_TEXT(ID_TEXTCTRL_MY_EMAIL_ADDR, CEmailReportDlg::OnYourEmailAddressEditBoxChanged)
	EVT_TEXT(ID_TEXTCTRL_SUMMARY_SUBJECT, CEmailReportDlg::OnSubjectSummaryEditBoxChanged)
	EVT_TEXT(ID_TEXTCTRL_DESCRIPTION_BODY, CEmailReportDlg::OnDescriptionBodyEditBoxChanged)
	EVT_TEXT(ID_TEXTCTRL_SENDERS_NAME, CEmailReportDlg::OnSendersNameEditBoxChanged)
END_EVENT_TABLE()

CEmailReportDlg::CEmailReportDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Send Report to Adapt It Developers"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pEmailReportDlgSizer = EmailReportDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
    
    // whm 5Mar2019 Note: The EmailReportDlgFunc() dialog has a "Close" (wxID_OK), but it is grouped with
    // other non-standard buttons in the left part of the dialog. We cannot use either the 
    // wxStdDialogButtonSizer, nor the ReverseOkCancelButtonsForMac() function in this case.

	bEmailSendSuccessful = FALSE;
	
	pTextDeveloperEmails = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_DEVS_EMAIL_ADDR);
	wxASSERT(pTextDeveloperEmails != NULL);
	
	pTextYourEmailAddr = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_MY_EMAIL_ADDR);
	wxASSERT(pTextYourEmailAddr != NULL);
    // Set background color of required fields to light yellow
    pTextYourEmailAddr->SetBackgroundColour(wxColour(255, 255, 150)); // light yellow
	
	pTextEmailSubject = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SUMMARY_SUBJECT);
	wxASSERT(pTextEmailSubject != NULL);
    // Set background color of required fields to light yellow
    pTextEmailSubject->SetBackgroundColour(wxColour(255, 255, 150)); // light yellow


	pTextSendersName = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SENDERS_NAME);
	wxASSERT(pTextSendersName != NULL);
    // Set background color of required fields to light yellow
    pTextSendersName->SetBackgroundColour(wxColour(255, 255, 150)); // light yellow

	// Note: STATIC_TEXT_DESCRIPTION is a pointer to a wxStaticBoxSizer which wxDesigner casts back
	// to the more generic wxSizer*, so we'll use a wxDynamicCast() to cast it back to its original
	// object class. Then we can call wxStaticBoxSizer::GetStaticBox() in it.
	wxStaticBoxSizer* psbSizer = wxDynamicCast(STATIC_TEXT_DESCRIPTION,wxStaticBoxSizer); // use dynamic case because wxDesigner cast it to wxSizer*
	pStaticBoxTextDescription = (wxStaticBox*)psbSizer->GetStaticBox();
	wxASSERT(pStaticBoxTextDescription != NULL);

	pTextDescriptionBody = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_DESCRIPTION_BODY);
	wxASSERT(pTextDescriptionBody != NULL);
    // Set background color of required fields to light yellow
    pTextDescriptionBody->SetBackgroundColour(wxColour(255, 255, 150)); // light yellow

	pLetAIDevsKnowHowIUseAI = (wxCheckBox*)FindWindowById(ID_CHECKBOX_LET_DEVS_KNOW_AI_USAGE);
	wxASSERT(pLetAIDevsKnowHowIUseAI != NULL);
	
	pButtonViewUsageLog = (wxButton*)FindWindowById(ID_BUTTON_VIEW_USAGE_LOG);
	wxASSERT(pButtonViewUsageLog != NULL);
	
    pTextFillOutYellowAreas = (wxStaticText*)FindWindowById(ID_TEXT_FILL_OUT_YELLOW_AREAS);
    wxASSERT(pTextFillOutYellowAreas != NULL);
    // Set background color of required fields to light yellow
    pTextFillOutYellowAreas->SetBackgroundColour(wxColour(255, 255, 150)); // light yellow

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

    pHTMLHyperLinkSendToEmailPgm = (wxHyperlinkCtrl*)FindWindowById(ID_HYPERLINK_MAILTO);
    wxASSERT(pHTMLHyperLinkSendToEmailPgm != NULL);
	
	pButtonAttachAPackedDoc = (wxButton*)FindWindowById(ID_BUTTON_ATTACH_PACKED_DOC);
	wxASSERT(pButtonAttachAPackedDoc != NULL);
	
	pButtonSendNow = (wxButton*)FindWindowById(ID_BUTTON_SEND_NOW);
	wxASSERT(pButtonSendNow != NULL);

	// This dialog does not have the equivalent of an OK and Cancel button, hence
	// we don't call ReverseOkCancelButtonsForMac(this) here.

}

CEmailReportDlg::~CEmailReportDlg() // destructor
{
}

void CEmailReportDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	CAdapt_ItApp* pApp = &wxGetApp();
	totalBytesSent = 0;
	packedDocumentFileName = _T("");
	bSubjectHasUnsavedChanges = FALSE;
	bPackedDocToBeAttached = FALSE;
	bYouEmailAddrHasUnsavedChanges = FALSE;
	bDescriptionBodyHasUnsavedChanges = FALSE;
	bSendersNameHasUnsavedChanges = FALSE;
	bCurrentEmailReportWasLoadedFromFile = FALSE;
	LoadedFilePathAndName = _T("");
	saveAttachDocLabel= pButtonAttachAPackedDoc->GetLabel();
	pBtnAttachTooltip = pButtonAttachAPackedDoc->GetToolTip();
	saveAttachDocTooltip = pBtnAttachTooltip->GetTip();

	if (!::wxFileExists(pApp->m_usageLogFilePathAndName))
	{
		pLetAIDevsKnowHowIUseAI->SetValue(FALSE);
		pLetAIDevsKnowHowIUseAI->Enable(FALSE);
	}
	else
	{
		pLetAIDevsKnowHowIUseAI->SetValue(TRUE);
		pLetAIDevsKnowHowIUseAI->Enable(TRUE);
	}

	pTextDeveloperEmails->ChangeValue(pApp->m_aiDeveloperEmailAddresses);

	wxString templateText1,templateText2;
	if (reportType == Report_a_problem)
	{
		SetTitle(_("Report a problem"));
		// set the Title of the wxStaticBoxSizer surrounding the edit box
		pStaticBoxTextDescription->SetLabel(_("Please provide a description of the problem (for body of Email)"));
		// Add template text guide to the multiline wxTextCtrl to guide the user
		templateText1 = _("What steps will reproduce the problem? (Edit the steps below:)");
		templateText1 += _T("\n1. \n2. \n3. \n");
		templateText2 = _("Provide any other information you think would be helpful (such as attaching an Adapt It document, user log, etc.):");
		templateText2 += _T("\n");
        templateText2 += _T("-- Note: Adapt It documents are .xml files located in the Adaptations folder of your project folder.");
        templateText2 += _T("\n");
        templateText2 += _T("-- Note: Your Adapt It user log is a UsageLog_user.txt file located in the _LOGS_EMAIL_REPORTS folder of your Adapt It Unicode Work folder.");
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
	saveSendersName = pTextSendersName->GetValue(); // it will be empty
	
    // whm 6Aug2019 implemented send to email program, so commented out line below
	//pRadioSendItToMyEmailPgm->Disable(); // for 6.0.0 we didn't implement this send to email program option

    // TODO: Do work-around for Linux platform which doesn't set the controls background to the light
    // yellow - only the multi-line wxTextCtrl on Linux gets the light yellow backgroun (see constructor above).
    // Try setting the actual text background on Linux to a light yellow using wxTextAttribute class and/or the
    // SetStyle... class.

    // TODO: I've Disabled the "Send it directly from Adapt It..." radio button, the "Send Now" button
    // and the "Attach this document (packed)" button below until the internal email functionality is working.
    // Also, must eventually decide which radio button is to be the default once both email functions are working.
    // TODO: Enable the "Send directly from Adapt It radio button, the "Send Now" button and the "Attach this
    // document..." button in the dialog after Michael helps me get the php code working on the adapt-it.org server. 
    pRadioSendItDirectlyFromAI->Disable();
    pButtonSendNow->Enable(); // whm 7Feb2020 changed initially to Enable since it is the only option
    pButtonAttachAPackedDoc->Disable();
    pRadioSendItDirectlyFromAI->SetValue(FALSE); // this is not the default now
    pRadioSendItToMyEmailPgm->SetValue(TRUE); // this is the default for now

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
	// GDLC 6May11 Used the app's GetFreeMemory()
	memSize = pApp->GetFreeMemory();
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
	sysID = sysID; // avoid warning
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

size_t curl_send_callback(void *ptr, size_t size, size_t nmemb, void *userdata) 
{
	ptr = ptr; // avoid "unreferenced formal parameter" warning
	userdata = userdata; // avoid "unreferenced formal parameter" warning
    wxString sizeStr;
	totalBytesSent += size * nmemb;
	sizeStr << size * nmemb;
	wxString msg;
	msg = msg.Format(_T("In curl send callback: sending %s bytes."),sizeStr.c_str());
	wxLogDebug(msg);
    return size * nmemb; // Return amount processed
}

// OnBtnSendNow(), unless returned prematurely because of incomplete information,
// or failure of the email to be sent, invokes EndModal(wxID_OK) to close the dialog.
void CEmailReportDlg::OnBtnSendNow(wxCommandEvent& WXUNUSED(event)) 
{
	CAdapt_ItApp* pApp = &wxGetApp();
	if (!bMinimumFieldsHaveData())
		return;

    bool bSendToUsersEmail = FALSE;
	wxString output;
	wxString errorStr;
	wxString senderName;
	senderName = pTextSendersName->GetValue();
	senderName.Trim(FALSE);
	senderName.Trim(TRUE);
	wxString emailBody;
	emailBody = pTextDescriptionBody->GetValue();
	emailBody.Trim(FALSE);
	emailBody.Trim(TRUE);
	wxString senderEmailAddr;
	senderEmailAddr = pTextYourEmailAddr->GetValue();
	senderEmailAddr.Trim(FALSE);
	senderEmailAddr.Trim(TRUE);
	wxString emailSubject;
	emailSubject = pTextEmailSubject->GetValue();
	emailSubject.Trim(FALSE);
	emailSubject.Trim(TRUE);
	wxString rptType;
	wxString systemInfo;
	systemInfo = FormatSysInfoIntoString();
	if (reportType == Report_a_problem)
	{
		rptType = _T("Problem");
	}
	else if (reportType == Give_feedback)
	{
		rptType = _T("Feedback");
	}

	bool bIncludeUserLogFile = FALSE;
	wxString userLogPathAndFile;
	userLogPathAndFile = pApp->m_usageLogFilePathAndName;
	if (pLetAIDevsKnowHowIUseAI->IsEnabled() && pLetAIDevsKnowHowIUseAI->GetValue() == TRUE && ::wxFileExists(userLogPathAndFile))
	{
		bIncludeUserLogFile = TRUE;
	}


	int curl_result = -1;
	
	// Implement the acutal sending of the report directly or to the user's email program
	// here. If successful set bEmailSendSuccessful to TRUE;
	if (pRadioSendItDirectlyFromAI->GetValue() == TRUE)
	{
		CURL *curl;
		CURLcode res;

		curl = curl_easy_init(); // curl is the handle
		
		struct curl_httppost *formpost=NULL;
		struct curl_httppost *lastptr=NULL;
		struct curl_slist *headerlist=NULL;
		static const char buf[] = "Expect:";
		char error[CURL_ERROR_SIZE];
  
		wxString userLogContents;
		userLogContents.Empty();
		if (pLetAIDevsKnowHowIUseAI->GetValue() == TRUE)
		{
			wxFileInputStream input(pApp->m_usageLogFilePathAndName);
			wxTextInputStream text( input );
			wxString line;
			while (!input.Eof())
			{
				line = text.ReadLine();
				line += _T("\r\n");
				userLogContents += line;
			}
			
			userLogInASCII = userLogContents.To8BitData();
			// Convert userLogContents to zip archive format and base64 encoding
			wxString tempZipFile;
			wxString nameInZip;
			wxString exportPath = pApp->m_usageLogFilePathAndName;
			wxFileName fn(exportPath);
			tempZipFile = fn.GetName();
			tempZipFile += _T('.');
			tempZipFile += _T("txt");
			nameInZip = tempZipFile;
			exportPath = fn.GetPath() + pApp->PathSeparator + fn.GetName();
			exportPath += _T('.');
			exportPath += _T("zip");
			
			CBString bstr;
			bstr = userLogContents.To8BitData();
			wxFFileOutputStream zippedfile(exportPath);
			// then, declare a zip stream placed on top of it (as zip generating filter)
			wxZipOutputStream zipStream(zippedfile);
			// wx version: Since our pack data is already in an internal buffer in memory, we can
			// use wxMemoryInputStream to access packByteStr; run it through a wxZipOutputStream
			// filter and output the resulting zipped file via wxFFOutputStream.
			wxMemoryInputStream memStr(bstr,bstr.GetLength());
			// create a new entry in the zip file using the .aiz file name
			zipStream.PutNextEntry(nameInZip);
			// finally write the zipped file, using the data associated with the zipEntry
			zipStream.Write(memStr);
			if (!zipStream.Close() || !zippedfile.Close() || 
				zipStream.GetLastError() == wxSTREAM_WRITE_ERROR) // Close() finishes writing the 
															// zip returning TRUE if successfully
			{
				wxString msg;
				msg = msg.Format(_("Could not write to the packed/zipped file: %s"),exportPath.c_str());
                // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                pApp->m_bUserDlgOrMessageRequested = TRUE;
                wxMessageBox(msg,_T(""),wxICON_ERROR | wxOK);
			} 
			if (wxFileExists(exportPath))
			{
				wxFile f(exportPath,wxFile::read);
				wxFileOffset fileLen;
				fileLen = f.Length();
				// read the raw byte data of the packed aip file into pByteBuf (char buffer on the heap)
				wxUint8* pByteBuf = (wxUint8*)malloc(fileLen + 1);
				memset(pByteBuf,0,fileLen + 1); // fill with nulls
				f.Read(pByteBuf,fileLen);
				wxASSERT(pByteBuf[fileLen] == '\0'); // should end in NULL
				// Since pByteBuf has embedded null characters, it needs to be processed 
				// to a base64 form before using it with formpost.
				
				// whm 9Jun12 modified: As of wxWidgets 2.9.1 it now has built-in wxBase64 
				// encode and decode functions, so we can conditionally compile the app to 
				// use the built-in functions when building against wxWidgets 2.9.1 or newer.
#if wxCHECK_VERSION(2,9,1)
				wxMemoryBuffer memBuff;
				memBuff.AppendData(pByteBuf,fileLen);
				userLogInBase64 = wxBase64Encode(memBuff);
#else
				std::string encoded;
				encoded = base64_encode(reinterpret_cast<const unsigned char*>(pByteBuf),fileLen);
				userLogInBase64 = wxString(encoded.c_str(), wxConvUTF8);
#endif
				free((void*)pByteBuf);

				// whm modified 21Nov2013 to save the actual zip file at our
				// local exportPath out to the adapt-it.org server and allow
				// the feedback.php file attach it directly from its temporary
				// location on the server to the email. This eliminates the
				// need to do the base64 encoding here as well as doing a
				// _POST of that data.
				
				// TODO: Use curl to send zip file to adapt-it.org server
				//if (!SendFileToServer(curl, res, exportPath))
				//{
				//}
			}

		}

		wxLogDebug(wxT("OnSend"));

		// BEW 8Oct12, moved this call to OnInit() since KbServer will need curl to have
		// been initialized; and in OnInit() I used a conditional define to just leave out
		// CURL_GLOBAL_WIN32 which is included inthe "_ALL" parameter, and just used
		// CURL_GLOBAL_SSL for OS X and Linux 
		//curl_global_init(CURL_GLOBAL_ALL);

		CBString tempStr;

		// Fill in the sender's name field
		// Note: sendername is used in feedback.php on the adapt-it.org server to receive the senderName POST
		tempStr = senderName.ToUTF8();
		curl_formadd(&formpost,
					&lastptr,
					CURLFORM_COPYNAME, "sendername",
					CURLFORM_COPYCONTENTS, tempStr.GetBuffer(),
					CURLFORM_END);

		// Fill in the comments field
		// Note: comments is used in feedback.php on the adapt-it.org server to receive the emailBody POST
		tempStr = emailBody.ToUTF8();
		curl_formadd(&formpost,
					&lastptr,
					CURLFORM_COPYNAME, "emailbody",
					CURLFORM_COPYCONTENTS, tempStr.GetBuffer(),
					CURLFORM_END);

		// Fill in the emailaddr field (email address of sender)
		// Note: senderemailaddr is used in feedback.php on the adapt-it.org server to receive the senderEmailAddr POST
		tempStr = senderEmailAddr.ToUTF8();
		curl_formadd(&formpost,
					&lastptr,
					CURLFORM_COPYNAME, "senderemailaddr",
					CURLFORM_COPYCONTENTS, tempStr.GetBuffer(),
					CURLFORM_END);

		// Fill in the emailsubject field (email subject field)
		// Note: emailsubject is used in feedback.php on the adapt-it.org server to receive the emailSubject POST
		tempStr = emailSubject.ToUTF8();
		curl_formadd(&formpost,
					&lastptr,
					CURLFORM_COPYNAME, "emailsubject",
					CURLFORM_COPYCONTENTS, tempStr.GetBuffer(),
					CURLFORM_END);

		// Fill in the reporttype field (the type of email report, i.e., "Problem" or "Feedback")
		// Note: reporttype is used in feedback.php on the adapt-it.org server to receive the rptType POST
		tempStr = rptType.ToUTF8();
		curl_formadd(&formpost,
					&lastptr,
					CURLFORM_COPYNAME, "reporttype",
					CURLFORM_COPYCONTENTS, tempStr.GetBuffer(),
					CURLFORM_END);

		// Fill in the sysinfo field (containing system information, version numbers etc.)
		// Note: sysinfo is used in feedback.php on the adapt-it.org server to receive the systemInfo POST
		tempStr = systemInfo.ToUTF8();
		curl_formadd(&formpost,
					&lastptr,
					CURLFORM_COPYNAME, "sysinfo",
					CURLFORM_COPYCONTENTS, tempStr.GetBuffer(),
					CURLFORM_END);

		wxString attachLogNotification;
		if (bIncludeUserLogFile)
		{
			attachLogNotification = _T("Log is Attached:");
			// Fill in the attachnote field (a string that has either "Note: User Log is Attached", or
			// "Note: No User Log Attached)
			// Note: attachnote is used in feedback.php on the adapt-it.org server to receive the 
			// attachNotification POST
			tempStr = attachLogNotification.ToUTF8();
			curl_formadd(&formpost,
					&lastptr,
					CURLFORM_COPYNAME, "attachlog",
					CURLFORM_COPYCONTENTS, tempStr.GetBuffer(),
					CURLFORM_END);
			// Fill in the userlog field (containing the contents of the user log file)
			// Note: userlog is used in feedback.php on the adapt-it.org server to receive the systemInfo POST
			tempStr = userLogInBase64.ToUTF8();
			curl_formadd(&formpost,
					&lastptr,
					CURLFORM_COPYNAME, "userlog",
					// TODO: after debugging use the base64 form below [removing the base64_encode() php
					// function processing in feedback.php file]
					CURLFORM_COPYCONTENTS, tempStr.GetBuffer(),
					CURLFORM_END);
			
		}
		else
		{
			attachLogNotification = _T("User Log NOT Attached");
			// Fill in the attachnote field (a string that has either "Note: User Log is Attached", or
			// "Note: No User Log Attached)
			// Note: attachnote is used in feedback.php on the adapt-it.org server to receive the 
			// attachNotification POST
			tempStr = attachLogNotification.ToUTF8();
			curl_formadd(&formpost,
					&lastptr,
					CURLFORM_COPYNAME, "attachlog",
					CURLFORM_COPYCONTENTS, tempStr.GetBuffer(),
					CURLFORM_END);
		}

		wxString attachPackedDocNotification;
		if (bPackedDocToBeAttached)
		{
			attachPackedDocNotification = _T("Packed Document Is Attached:");
			tempStr = attachPackedDocNotification.ToUTF8();
			curl_formadd(&formpost,
					&lastptr,
					CURLFORM_COPYNAME, "attachdoc",
					CURLFORM_COPYCONTENTS, tempStr.GetBuffer(),
					CURLFORM_END);
			// Fill in the userlog field (containing the contents of the user log file)
			// Note: userlog is used in feedback.php on the adapt-it.org server to receive the systemInfo POST
			tempStr = packedDocInBase64.ToUTF8();
			curl_formadd(&formpost,
					&lastptr,
					CURLFORM_COPYNAME, "packdoc",
					CURLFORM_COPYCONTENTS, tempStr.GetBuffer(),
					CURLFORM_END);
		}
		else
		{
			attachPackedDocNotification = _T("Packed Document NOT Attached");
			tempStr = attachPackedDocNotification.ToUTF8();
			curl_formadd(&formpost,
					&lastptr,
					CURLFORM_COPYNAME, "attachdoc",
					CURLFORM_COPYCONTENTS, tempStr.GetBuffer(),
					CURLFORM_END);
		}

		// curl = curl_easy_init(); // curl is the handle // whm moved to
		// beginning of OnBtnSendNow()
		// initalize custom header list (stating that Expect: 100-continue is not wanted
		headerlist = curl_slist_append(headerlist, buf);
		
		//if (bIncludeUserLogFile)
		//{
			// Fill in the sysinfo field (containing system information, version numbers etc.)
			// Note: emailsubject is used in feedback.php on the adapt-it.org server to receive the systemInfo POST
			//wxFileName fn(pApp->m_usageLogFilePathAndName);
			//wxString baseName = fn.GetFullName();
			//headerlist = curl_slist_append(headerlist, "Content-Type: application/x-zip-compressed;  name=\"UsageLog_Bill Martin.zip\"\n");
			//headerlist = curl_slist_append(headerlist, "Content-Transfer-Encoding: base64\n");
			//headerlist = curl_slist_append(headerlist, "Content-Disposition: attachment; filename=\"UsageLog_Bill Martin.zip\"\n");
			//headerlist = curl_slist_append(headerlist, "UEsDBBQAAAAIAHyxOj5YbcAMeAAAAMoBAAAYAAAAVXNhZ2VMb2dfQmlsbCBN"
			//											"YXJ0aW4udHh0hdBBDsIwEAPAOxJ/yA+66+4mxH9A6hd64oRAofyfiILEgWjv"
			//											"I8u2TsAEUU2VKATScubSbpe2XtNjW9v2vB8Pv2oWzhorUGWkbFfWCS0P1TfL"
			//											"lfBAqVKdMmS+M6eBVgKViUodFsufYu8siVWh1VidYtXv6jv9/8gXUEsBAhQA"
			//											"FAAAAAgAfLE6PlhtwAx4AAAAygEAABgAAAAAAAAAAAAgAAAAAAAAAFVzYWdl"
			//											"TG9nX0JpbGwgTWFydGluLnR4dFBLBQYAAAAAAQABAEYAAACuAAAAAAA=");
			//headerlist = curl_slist_append(headerlist, );
			//headerlist = curl_slist_append(headerlist, );
			//curl_formadd(&formpost,
			//			&lastptr,
			//			CURLFORM_COPYNAME, "user-log",
			//			CURLFORM_FILECONTENT, baseName.ToUTF8(),
			//			CURLFORM_CONTENTHEADER, headerlist,
			//			CURLFORM_END);
		//}

		if(curl) 
		{
			// what URL receives this POST
            // whm 22Apr2019 added - use the URL from AI-BasicConfiguration.aic file if it exists
            if (!pApp->m_serverURL.IsEmpty() && !pApp->m_phpFileName.IsEmpty())
            {
                wxString urlStr = pApp->m_serverURL + pApp->m_phpFileName;
                CBString urlCStr;
                urlCStr = urlStr.ToUTF8();
                curl_easy_setopt(curl, CURLOPT_URL, urlCStr.GetBuffer()); // Use this URL for SSL connection
            }
            else
            {
                // whm 17Jul2019 modified the CURLOPT_URL to use the feedback_phpMailer.php file instead of the feedback.php used in the past (before Michael's change to smtp.ionos.com).
                curl_easy_setopt(curl, CURLOPT_URL, "https://adapt-it.org/feedback_phpMailer.php"); // Use this default URL for SSL connection
            }
			
			// Note: the path in the following CRULOPT_CAINFO option is a path to the ca-buncle.crt file
			// in the Windows distribution of Adapt It. During development the ca-bundle.crt file
			// was created by the developer by invoking a perl script called mk-ca-bundle.pl located 
			// at c:\curl-7.21.2\lib\ca-bundle.crt on the developer's machine. Linux and Mac systems
			// know how to find their own ca-bundle.crt files.
			wxString ca_bundle_path; 
#ifdef __WXMSW__
			ca_bundle_path = pApp->m_setupFolder + pApp->PathSeparator + _T("curl-ca-bundle.crt"); // path to ca bundle in setup folder on user's machine
#else
			ca_bundle_path = pApp->GetDefaultPathForXMLControlFiles();
			ca_bundle_path += pApp->PathSeparator;
			ca_bundle_path += _T("curl-ca-bundle.crt");
#endif
			wxLogDebug(_T("The CA bundle certificate path is: %s"),ca_bundle_path.c_str());
			tempStr = ca_bundle_path.ToUTF8();
			curl_easy_setopt(curl, CURLOPT_CAINFO, tempStr.GetBuffer()); // tell curl where the curl-ca-bundle.crt file is
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L); // 1 enables peer verification (SSL) - looks for curl-ca-bundle.crt at path above
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L); // 1 enables host verification (SSL) - verifies the server at adapt-it.org
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_send_callback);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
			curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
			curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

			res = curl_easy_perform(curl);
			curl_result = res;

			if (res != 0)
			{
				bEmailSendSuccessful = FALSE;
				errorStr = wxString::FromUTF8(error);
				output = output.Format(_T("Curl result: %d error = %s\n"), res, errorStr.c_str());
			}
			else
			{
				bEmailSendSuccessful = TRUE;
				errorStr = _T("Success!");
				output = output.Format(_T("Curl result: %d operation = %s\n"), res, errorStr.c_str());
			}
			pApp->LogUserAction(output);
			wxLogDebug(output);

			// always cleanup
			curl_easy_cleanup(curl);

			// then cleanup the formpost chain
			curl_formfree(formpost);
			// free slist
			curl_slist_free_all (headerlist);
			
		}
		else
		{
			// This shouldn't happen. English error message "curl could not be initialized"
			wxMessageBox(_T("The curl utility could not be initialized"),_T(""),wxICON_INFORMATION | wxOK);
			pApp->LogUserAction(_T("The curl utility could not be initialized"));
		}
		// whm 13Oct12 moved the following curl_global_cleanup()
		// function to the App's OnExit() function, since the
		// curl_global_init() function is now in the App's OnInit().
		//curl_global_cleanup();
	}
	else
	{
		// This block should perform the same work that the OnHyperLinkMailToClicked handler performs.
        // That handler collects all pertinent information from the fields filled out by user, as well
        // as system info, etc, and send that via the hyperlink
        wxHyperlinkEvent hevent;
        OnHyperLinkMailToClicked(hevent);
        bSendToUsersEmail = TRUE;
        bEmailSendSuccessful = TRUE; // to avoid error message below
    }

	// If the email cannot be sent, automatically invoke the OnBtnSaveReportAsXmlFile() 
	// handler so the user can save any edits made before closing the dialog.
	if (!bEmailSendSuccessful)
	{
		wxString msg1,msg2;
		msg1 = _("Your email could not be sent at this time.\nAdapt It will save a copy of your report in the %s folder (for sending at a later time).");
        msg1 = msg1.Format(msg1, pApp->m_logsEmailReportsFolderName.c_str());
		msg2 = msg2.Format(_("Error %d: %s"),curl_result,errorStr.c_str());
		msg2 = _T("\n\n") + msg2;
		msg1 = msg1 + msg2;
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(msg1,_T(""),wxICON_INFORMATION | wxOK);
		bool bPromptForMissingData = FALSE;
		bool bSavedOK;
		wxString nameSuffix = _T("");
		wxString nameUsed = _T("");
		bSavedOK = DoSaveReportAsXmlFile(bPromptForMissingData,nameSuffix,nameUsed);
		if (!bSavedOK)
		{
			wxString path,msg;
			path = pApp->m_logsEmailReportsFolderPath + pApp->PathSeparator + pApp->m_logsEmailReportsFolderName;
			msg = msg.Format(_("Unable to save the report to the following path:\n\n%s"),path.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,_T(""),wxICON_EXCLAMATION | wxOK);
			return;
		}
	}
	else
	{
		bool bPromptForMissingData = FALSE;
		bool bSavedOK;
		wxString nameSuffix = _T("Sent");
		wxString nameUsed = _T("");
		bSavedOK = DoSaveReportAsXmlFile(bPromptForMissingData,nameSuffix,nameUsed);
		if (!bSavedOK)
		{
			wxString path,msg;
			path = pApp->m_logsEmailReportsFolderPath + pApp->PathSeparator + pApp->m_logsEmailReportsFolderName;
			msg = msg.Format(_("Unable to save the report to the following path:\n\n%s"),path.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,_T(""),wxICON_EXCLAMATION | wxOK);
			return;
		}
		else
		{
			wxString msg1;
            if (bSendToUsersEmail)
                msg1 = msg1.Format(_("Your email was sent to your default email program, which should have opened automatically. You may need to activate the email program's window in the task bar to bring it into view. You can finish editing your report there and send it from your email program to the Adapt It developers at developers@adapt-it.org from you email program. \nThank you for your report.\nAdapt It put a copy of the report in your work folder at:\n   %s"), nameUsed.c_str());
            else
			    msg1 = msg1.Format(_("Your email was sent to the Adapt It developers at developers@adapt-it.org.\nThank you for your report.\nThe email contained %d bytes of data.\nAdapt It will put a copy of the sent report in your work folder at:\n   %s"),totalBytesSent,nameUsed.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg1,_T(""),wxICON_INFORMATION | wxOK);
		}
	}

	EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void CEmailReportDlg::OnBtnSaveReportAsXmlFile(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = &wxGetApp();
	pApp->LogUserAction(_T("Initiated OnBtnSaveReportAsXmlFile()"));
	bool bPromptForMissingData = TRUE;
	bool bSavedOK;
	wxString nameSuffix = _T("");
	wxString nameUsed = _T("");
	bSavedOK = DoSaveReportAsXmlFile(bPromptForMissingData,nameSuffix,nameUsed);
	wxCHECK_RET(bSavedOK, _T("OnBtnSaveReportAsXmlFile(): DoSaveReportAsXmlFile() returned FALSE, line 810 in EmailReportDlg.cpp, the report may not have been saved"));
}

bool CEmailReportDlg::DoSaveReportAsXmlFile(bool PromptForSaveChanges, wxString nameSuffix, wxString& nameUsed)
{
	if (PromptForSaveChanges && !bMinimumFieldsHaveData())
		return TRUE;

	// whm 12Jul11 Note. For email Problem and Feedback reports we always save 
	// those in the special _LOGS_EMAIL_REPORTS folder regardless of whether navigation
	// protection is in effect. The nav protection for _REPORTS_LOGS only really
	// applies to Retranslation Reports. Other types of reports/logs including
	// this dialog's email reports go to the special folder regardless of the 
	// protection setting for _LOGS_EMAIL_REPORTS in AssignLocationsForInputsAndOutputs.

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
	if (bCurrentEmailReportWasLoadedFromFile && !LoadedFilePathAndName.IsEmpty() && nameSuffix.IsEmpty())
	{
		reportPathAndName = LoadedFilePathAndName;
	}
	else
	{
		// whm 12Jul11 note: Email problem and feedback reports and usage logs are 
		// stored in the App's m_workFolderPath (or m_customWorkFolderPath if 
		// appropriate). They cannot be stored in a project path because email 
		// reports and logs can be generated from AI without there being a project
		// open (i.e., whenever the user clicks Cancel from the wizard or Collab 
		// dialog, then accesses the Report a Problem... or Give Feedback... on the 
		// Help menu). 
		// Ensure that the _LOGS_EMAIL_REPORTS folder exists
		bool bOK = TRUE;
		if (!::wxDirExists(pApp->m_logsEmailReportsFolderPath))
			bOK = ::wxMkdir(pApp->m_logsEmailReportsFolderPath);
		if (!bOK)
		{
			return FALSE; // caller can notify user
		}
		// 
		reportPathAndName = pApp->m_logsEmailReportsFolderPath; // no PathSeparator or filename at end of path
		reportPathAndName += pApp->PathSeparator + _T("AI_Report") + rptType + nameSuffix + _T(".xml");
	}
	
	// Compose xml report and write it to reportPathAndName
	bool bReportBuiltOK = TRUE;
	bool bReplaceExistingReport = bCurrentEmailReportWasLoadedFromFile;
	// if it is a sent report don't replace any existing one
	if (!nameSuffix.IsEmpty())
	{
		bReplaceExistingReport = FALSE;
	}
	if (!bReplaceExistingReport)
	{
		reportPathAndName = GetUniqueIncrementedFileName(reportPathAndName,incrementViaDate_TimeStamp,TRUE,2,_T("_")); 
		// save the file using the reportPathAndName which is now guaranteed to be a unique name
	}
	bReportBuiltOK = BuildEmailReportXMLFile(reportPathAndName,bReplaceExistingReport);

	nameUsed = reportPathAndName; // return the actual name used to caller via nameUsed reference parameter
	wxString msg;
	if (!bReportBuiltOK)
	{
		msg = msg.Format(_("Could not create the email report to disk at the following work folder path:\n   %s"),reportPathAndName.c_str());
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(msg,_("Problem creating or writing the xml file"),wxICON_EXCLAMATION | wxOK);
		pApp->LogUserAction(msg);
	}
	else
	{
		// if it is a sent report don't give this message because the caller already gives a more detailed message
		if (nameSuffix.IsEmpty())
		{
			msg = msg.Format(_("The email report was saved at the following work folder path:\n   %s"),reportPathAndName.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,_("Your report was saved for later use or reference"),wxICON_INFORMATION | wxOK);
			pApp->LogUserAction(msg);
		}
		bSubjectHasUnsavedChanges = FALSE;
		bYouEmailAddrHasUnsavedChanges = FALSE;
		bDescriptionBodyHasUnsavedChanges = FALSE;
		bSendersNameHasUnsavedChanges = FALSE;
	}
	return bReportBuiltOK;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) 
{
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

/* // whm commented out 6Dec2013 - may be useful in the future
bool CEmailReportDlg::SendFileToServer(CURL *curl, CURLcode& res, const wxString localPathAndName)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	bool bSuccess = FALSE;
	FILE *fp;
	char *url = "https://adapt-it.org/tmp/attachment.tmp";
	char outfilename[FILENAME_MAX];
	strncpy(outfilename, (const char*)localPathAndName.mb_str(wxConvUTF8), FILENAME_MAX);	
	curl = curl_easy_init();
	if (curl) {
		wxString ca_bundle_path;
#ifdef __WXMSW__
		ca_bundle_path = pApp->m_setupFolder + pApp->PathSeparator + _T("curl-ca-bundle.crt"); // path to ca bundle in setup folder on user's machine
#else
		ca_bundle_path = pApp->GetDefaultPathForXMLControlFiles();
		ca_bundle_path += pApp->PathSeparator;
		ca_bundle_path += _T("curl-ca-bundle.crt");
#endif
		CBString tempStr = ca_bundle_path.ToUTF8();
		fp = fopen(outfilename,"wb");
		curl_easy_setopt(curl, CURLOPT_CAINFO, tempStr.GetBuffer()); // tell curl where the curl-ca-bundle.crt file is
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L); // 1 enables peer verification (SSL) - looks for curl-ca-bundle.crt at path above
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L); // 1 enables host verification (SSL) - verifies the server at adapt-it.org
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		res = curl_easy_perform(curl);
		if (res != 0)
			bSuccess = FALSE;
		else
			bSuccess = TRUE;
		curl_easy_cleanup(curl);
		fclose(fp);
	}
	else
	{
		bSuccess = FALSE;
	}

	return bSuccess;
}
*/

void CEmailReportDlg::OnBtnLoadASavedReport(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = &wxGetApp();
	pApp->LogUserAction(_T("Initiated OnBtnLoadASavedReport()"));
	// Adapt It saves email reports in a special _LOGS_EMAIL_REPORTS folder in the current 
	// work folder (which may be a custom work folder) at the path m_logsEmailReportsFolderPath. 
	// Get an array of the report file names that match the reportType.
	// 
	// Prompt to save any changes before loading another report
	int response = wxYES;
	if (bSubjectHasUnsavedChanges || bYouEmailAddrHasUnsavedChanges 
		|| bDescriptionBodyHasUnsavedChanges || bSendersNameHasUnsavedChanges)
	{
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        response = wxMessageBox(_("You made changes to this report - Do you want to save those changes?"),_T(""),wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);
		if (response == wxYES)
		{
			bool bPromptForMissingData = TRUE;
			bool bSavedOK;
			wxString nameSuffix = _T("");
			wxString nameUsed = _T("");
			bSavedOK = DoSaveReportAsXmlFile(bPromptForMissingData,nameSuffix,nameUsed);
			if (!bSavedOK)
			{
				wxString path,msg;
				path = pApp->m_logsEmailReportsFolderPath + pApp->PathSeparator + pApp->m_logsEmailReportsFolderName;
				msg = msg.Format(_("Unable to save the report to the following path:\n\n%s"),path.c_str());
                // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                pApp->m_bUserDlgOrMessageRequested = TRUE;
                wxMessageBox(msg,_T(""),wxICON_EXCLAMATION | wxOK);
				pApp->LogUserAction(msg);
				// return; 
				// allow the Load operation to continue 
			}
		}
	}
	wxArrayString reportsArray;
	reportsArray.Clear();
	wxString fileNameStr;
	wxString rptType;
	wxDir dir(pApp->m_logsEmailReportsFolderPath);
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
		//int userSelectionInt; // TODO: whm: write code to implement at some future date
		wxString userSelectionStr;
		userSelectionStr = ChooseEmailReportToLoad.GetStringSelection();
		//userSelectionInt = ChooseEmailReportToLoad.GetSelection();
		bool bReadOK;
		wxString pathAndName = pApp->m_logsEmailReportsFolderPath + pApp->PathSeparator + userSelectionStr;
		bReadOK = ReadEMAIL_REPORT_XML(pathAndName,_T(""),0);
		if (bReadOK)
		{
			wxASSERT(pApp->m_pEmailReportData != NULL);
			pTextYourEmailAddr->ChangeValue(pApp->m_pEmailReportData->fromAddress);
			pTextEmailSubject->ChangeValue(pApp->m_pEmailReportData->subjectSummary);
			pTextSendersName->ChangeValue(pApp->m_pEmailReportData->sendersName);
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
                    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                    pApp->m_bUserDlgOrMessageRequested = TRUE;
                    response = wxMessageBox(pdMsg1,_("Could not find the packed document in your work folder"),wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);
					pApp->LogUserAction(pdMsg1);
					if (response == wxYES)
					{
						// attach any open doc as packed doc
						wxCommandEvent evt;
						OnBtnAttachPackedDoc(evt);
					}
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
			LoadedFilePathAndName = pApp->m_logsEmailReportsFolderPath + pApp->PathSeparator + userSelectionStr;
		}
	}
}

void CEmailReportDlg::OnBtnClose(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = &wxGetApp();
	// Check for changes before closing, and allow user to save changes before closing
	int response = wxYES;
	if (bSubjectHasUnsavedChanges || bYouEmailAddrHasUnsavedChanges 
		|| bDescriptionBodyHasUnsavedChanges || bSendersNameHasUnsavedChanges)
	{
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        response = wxMessageBox(_("You made changes to this report - Do you want to save those changes?"),_T("This dialog is about to close..."),wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);
		if (response == wxYES)
		{
			bool bPromptForMissingData = FALSE;
			bool bSavedOK;
			wxString nameSuffix = _T("");
			wxString nameUsed = _T("");
			bSavedOK = DoSaveReportAsXmlFile(bPromptForMissingData,nameSuffix,nameUsed);
			if (!bSavedOK)
			{
				wxString path,msg;
				path = pApp->m_logsEmailReportsFolderPath + pApp->PathSeparator + pApp->m_logsEmailReportsFolderName;
				msg = msg.Format(_("Unable to save the report to the following path:\n\n%s"),path.c_str());
                // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                pApp->m_bUserDlgOrMessageRequested = TRUE;
                wxMessageBox(msg,_T(""),wxICON_EXCLAMATION | wxOK);
				pApp->LogUserAction(msg);
				// return;
				// allow the dialog to close via the EndModal() call below
			}
		}
	}
	EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void CEmailReportDlg::OnBtnAttachPackedDoc(wxCommandEvent& WXUNUSED(event))
{
	// If no document is open, tell user that he must open a document first that he
	// wants to have packed and attached to the email report.
	CAdapt_ItApp* pApp = &wxGetApp();
	pApp->LogUserAction(_T("Initiated OnBtnAttachPackedDoc()"));
	CAdapt_ItDoc* pDoc = pApp->GetDocument();
	if (pApp->m_pSourcePhrases->GetCount() == 0)
	{
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_("A document must be open before Adapt It can pack it.\nClose the report dialog and open a document, then try again."),_("No document open"),wxICON_INFORMATION | wxOK);
		pApp->LogUserAction(_T("A document must be open before Adapt It can pack it.\nClose the report dialog and open a document, then try again."));
		return;
	}
	
	if (bPackedDocToBeAttached)
	{
		// user wants to remove the attached document
		bPackedDocToBeAttached = FALSE;
		pButtonAttachAPackedDoc->SetLabel(saveAttachDocLabel);
		pBtnAttachTooltip->SetTip(saveAttachDocTooltip);
		pButtonAttachAPackedDoc->SetToolTip(pBtnAttachTooltip);
		pButtonAttachAPackedDoc->Enable();
		pButtonAttachAPackedDoc->Refresh();
		pApp->LogUserAction(_T("Detached an Attached Packed Doc()"));
		return;
	}

	// Get the raw data used for packing a document in preparation for attaching it to the email report
    // DoPackDocument() below is what is used in the Doc's OnFilePackDocument()
	//CBString* packByteStr;
	//packByteStr = new CBString;
	wxString exportPathUsed; // to get the export path of the .aip file created by DoPackDocument() below
	exportPathUsed.Empty();
	if (!pDoc->DoPackDocument(exportPathUsed,FALSE)) // assembles the raw data into the packByteStr byte buffer (CBString)
	{
		// Any errors will be displayed by DoPackDocument, so just return here
		pApp->LogUserAction(_T("Error in DoPackDocument() called from CEmailReportDlg"));
		return;
	}
	// the packed document is located at exportPathUsed
	if (wxFileExists(exportPathUsed))
	{
		wxFile f(exportPathUsed,wxFile::read);
		wxFileOffset fileLen;
		fileLen = f.Length();
		// read the raw byte data of the packed aip file into pByteBuf (char buffer on the heap)
		wxUint8* pByteBuf = (wxUint8*)malloc(fileLen + 1);
		memset(pByteBuf,0,fileLen + 1); // fill with nulls
		f.Read(pByteBuf,fileLen);
		wxASSERT(pByteBuf[fileLen] == '\0'); // should end in NULL
		// Since pByteBuf has embedded null characters, it needs to be processed 
		// to a base64 form before using it with formpost.
		// whm 9Jun12 modified to use the built-in wxBase64Encode() function.
		// TODO: Verify that the built-in wxBase64Encode() function works in identical
		// fashion to the old base64 class method. Then, remove base64.h and base64.cpp 
		// from the adaptit repository.
#if wxCHECK_VERSION(2,9,1)
		wxMemoryBuffer memBuff;
		memBuff.AppendData(pByteBuf,fileLen);
		packedDocInBase64 = wxBase64Encode(memBuff);
#else
		std::string encoded;
		encoded = base64_encode(reinterpret_cast<const unsigned char*>(pByteBuf),fileLen);
		packedDocInBase64 = wxString(encoded.c_str(), wxConvUTF8);
#endif
		free((void*)pByteBuf);
	}
	bPackedDocToBeAttached = TRUE;
	pButtonAttachAPackedDoc->SetLabel(_("Remove attached document")); // whm added 16Jul11
	pBtnAttachTooltip->SetTip(_("Click to detach the packed adaptation document from this email"));
	pButtonAttachAPackedDoc->SetToolTip(pBtnAttachTooltip);
}

void CEmailReportDlg::OnBtnViewUsageLog(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = &wxGetApp();
	pApp->LogUserAction(_T("Initiated OnBtnViewUsageLog()"));
	CLogViewer logDlg((wxWindow*)pApp->GetMainFrame());
	logDlg.ShowModal();
}

void CEmailReportDlg::OnRadioBtnSendDirectlyFromAI(wxCommandEvent& WXUNUSED(event))
{
    CAdapt_ItApp* pApp = &wxGetApp();
    pApp->LogUserAction(_T("Initiated OnBtnSendDirectlyFromAI()"));
    this->pButtonSendNow->Enable(); // always enable "Send Now" button here
    pRadioSendItDirectlyFromAI->SetValue(TRUE);
    pRadioSendItToMyEmailPgm->SetValue(FALSE);
}

void CEmailReportDlg::OnRadioBtnSendToEmail(wxCommandEvent& WXUNUSED(event))
{
    CAdapt_ItApp* pApp = &wxGetApp();
    pApp->LogUserAction(_T("Initiated OnBtnSendToEmail()"));
    pRadioSendItDirectlyFromAI->SetValue(FALSE);
    pRadioSendItToMyEmailPgm->SetValue(TRUE);
}

void CEmailReportDlg::OnHyperLinkMailToClicked(wxHyperlinkEvent& WXUNUSED(event))
{
    CAdapt_ItApp* pApp = &wxGetApp();
    pApp->LogUserAction(_T("Initiated OnHyperLinkMailToClicked()"));
    // Note: We want, I think, a click on this hyperlink itself to immediately send 
    // the email text info to the user's email program - as most users would expect.
    // So, I think it best to disable the "Send Now" button to prevent a subsequent
    // click on that button while the user's email program is loading/displaying the email there.
    this->pButtonSendNow->Disable();
    // Also Note: Disabling the Send Now button here might be considered problematic, should the 
    // user change their mind and decide to send the email directly from AI using the top radio button. 
    // In that case the "Send Now" button would still be disabled. But, that is OK I think, 
    // a change of mind would involve the user subsequently clicking the top radio button, and 
    // the handler for that bittpm will always enable the "Send Now" button.
    // Probably we should just keep the email report dialog open and let the user click
    // the dialog's "Close" button themselves after interacting with the hyperlink/email pgm.
    //
    wxString emailSubject;
    emailSubject = pTextEmailSubject->GetValue();
    wxString emailBody;
    emailBody = pTextDescriptionBody->GetValue();
    wxString senderName;
    senderName = pTextSendersName->GetValue();
    wxString senderEmail;
    senderEmail = pTextYourEmailAddr->GetValue();
    wxString sysInfo;
    sysInfo = FormatSysInfoIntoString();

    wxString launchURLStr;
    // Note: The launchURLStr should contain all the text for the TO:, Subject:, and Body: fields of the
    // user's email. It could be formatted as plain text or as HTML for a nicer appearance.
    //
    // Flesh out the URL call string for launchURLStr, then explicitly call the wx function 
    // bool wxLaunchDefaultBrowser (const wxString & url, int flags = 0)
    // Build the launchURLStr with the following text components concatenated together:
    // 1. "mailto:developers@adapt-it.org"
    // 2. "?subject=[Adapt%20It%20Problem%20report]%20" for Problem report, or "subject=[Adapt%20It%20Feedback]" for Feedback report
    // 3. content of emailSubject field URL encoded where needed in user edits
    // 4. "&body="
    // 5. content of emailBody field URL encoded where needed in user edits
    // 6. content of the sysInfo returned from the FormatSysInfoIntoString() function (preceeded and followed by 'do not remove' lines)
    //  
    launchURLStr = _T("mailto:developers@adapt-it.org");
    if (reportType == Report_a_problem)
    {
        launchURLStr += _T("?subject=[Adapt%20It%20Problem%20report]%20");
    }
    else
    {
        launchURLStr += _T("?subject=[Adapt%20It%20Feedback]%20");
    }
    launchURLStr += FormatEditBoxStringInfoIntoURLSafeString(emailSubject);
    launchURLStr += _T("&body=");
    launchURLStr += FormatEditBoxStringInfoIntoURLSafeString(emailBody);
    if (reportType == Report_a_problem)
    {
        launchURLStr += _T("%0A%2A%2A%2A%20AFTER%20EDITING%20AND%20ATTACHING%20ANY%20FILES,%20CLICK%20ON%20YOUR%20EMAIL%20PROGRAM'S%20SEND%20BUTTON%20%2A%2A%2A%0A%0A");
    }
    else
    {
        launchURLStr += _T("%0A%2A%2A%2A%20Thank%20you%20for%20your%20comments!%20CLICK%20ON%20YOUR%20EMAIL%20PROGRAM'S%20SEND%20BUTTON%20%2A%2A%2A%0A%0A");
    }
    launchURLStr += _T("---%20Do%20not%20remove%20the%20following%20information%20---%0A");
    launchURLStr += _T("Name%3A%20");
    launchURLStr += FormatEditBoxStringInfoIntoURLSafeString(senderName);
    launchURLStr += _T("%0AEmail%3A%20");
    launchURLStr += FormatEditBoxStringInfoIntoURLSafeString(senderEmail);
    launchURLStr += _T("%0A");
    launchURLStr += FormatEditBoxStringInfoIntoURLSafeString(sysInfo);
    launchURLStr += _T("---%20Do%20not%20remove%20the%20information%20above%20---");
    // The following value for launchURLStr is a working example:
    //launchURLStr = _T("mailto:support@adapt-it.org?subject=[Adapt%20It%20Problem%20report]&body=What%20steps%20will%20reproduce%20the%20problem%3F%20(Edit%20the%20steps%20below%3A)%0A1.%20%0A2.%20%0A3.%20%0AProvide%20any%20other%20information%20you%20think%20would%20be%20helpful%20(such%20as%20attaching%20an%20Adapt%20It%20document%2C%20user%20log%2C%20etc.)%3A%0A--%20Note%3A%20%20Adapt%20It%20documents%20are%20.xml%20files%20located%20in%20the%20Adaptations%20folder%20of%20your%20project%20folder.%0A--%20Note%3A%20%20Your%20Adapt%20It%20user%20log%20is%20a%20UsageLog_user.txt%20file%20located%20in%20the%20_LOGS_EMAIL_REPORTS%20folder%20of%20your%20Adapt%20It%20Unicode%20Work%20folder.%0A%0A--%20Do%20not%20remove%20the%20following%20information%20--%0AName%3A%20%20%20Bill%0AEmail:%20%20%20bill_martin%40sil.org%0AAI%20Version%3A%20%20%206.9.4%0ARelease%20Date%3A%20%20%202019-5-9%0AData%20type%3A%20%20%20UNICODE%0AFree%20Memory%20(MB)%3A%20%20%20301%0ASys%20Locale%3A%20%20%20English%20(U.S.)%20en_US%0AInterface%20Language%3A%20%20%20English%20(U.S.)%0ASys%20Encoding%3A%20%20%20UTF-8%0ASys%20Layout%20Dir%3A%20%20%20System%20Default%0AwxWidgets%20version%3A%20%20%203.0.2%0AOS%20version%3A%20%20%2064%20bit%20Linux%204.4%0A--%20Do%20not%20remove%20the%20information%20above%20--%0A"); // should also be able to use support@adapt-it.org - TEST!!
   
    // The following curl function might be used, but would need to be tested first.
    // Note: curl has a function called curl_easy_escape() that can be used to encode chars for this:
    // CURL *curl = curl_easy_init();
    //if (curl) {
    //    char *output = curl_easy_escape(curl, "data to convert", 15);
    //    if (output) {
    //        printf("Encoded: %s\n", output);
    //        curl_free(output);
    //    }
    //}
    
    // Call wxLaunchDefaultBrowser() function to manually send the text info for the user's default email program.
    bool bLaunchedOK = FALSE;
    bLaunchedOK = wxLaunchDefaultBrowser(launchURLStr);
    bLaunchedOK = bLaunchedOK; // avoid warning

    // Note: the dialog stays open at end of this handler so the user can close it or
    // other action such as "Save report as text file (xml)", etc.
    //event.Skip(); // don't call Skip since we've handled the call of wxLaunchDefaultBrowser manually above
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
		bOK = wxRemoveFile(filePathAndName);
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
				composeXmlStr = wxString::FromAscii ( (const char*)xmlPrologue ); // first string in xml file
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
				
				// The email subject, email body and sender's name are the only user editable 
				// fields that might need to have entities replaced with their xml representations.
				composeXmlStr = tab2 + wxString::FromAscii(emailsendersname);
				tempStr = pTextSendersName->GetValue();
				// Note: wxTextFile takes care of the conversion of UTF16 to UTF8 when it writes the file.
				// Call the App's InsertEntities() which takes wxString and returns wxString
				tempStr = pApp->InsertEntities(tempStr);
				composeXmlStr += _T("=\"") + tempStr + _T("\"");
				textFile.AddLine(composeXmlStr);
				composeXmlStr.Empty();
				
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
				
                // We should include system info here at the beginning of the email body so that it
                // will be available to us from the xml report in the event that the user sends us the 
                // xml report only (that has happened when the email reporting was broke or not 
                // possible from user's location).
                wxString sysInfoStr = FormatSysInfoIntoString();
				wxString str = sysInfoStr + _T("\n") + _T("- - - - - - - - - - - - - - - - - - - -") + _T("\n") + pTextDescriptionBody->GetValue();
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
					tempStr = pApp->m_packedInputsAndOutputsFolderPath + pApp->PathSeparator + packedDocumentFileName;
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
    CAdapt_ItApp* pApp = &wxGetApp();
    if (pTextYourEmailAddr->GetValue().IsEmpty())
	{
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_("Please enter your email address"),_("Information missing or incomplete"),wxICON_EXCLAMATION | wxOK);
		pTextYourEmailAddr->SetFocus();
		return FALSE; // keep dialog open
	}
	else
	{
		// User supplied something in the email address field
		// Check to ensure it is a well-formed email address.
		bool bValidAddress = TRUE;
		int nPosAtSymbol = -1;
		wxString tempStr = pTextYourEmailAddr->GetValue();
		tempStr.Trim(FALSE);
		tempStr.Trim(TRUE);
		// Valid email addresses must have an '@' character
		if (tempStr.Find(_T('@')) != wxNOT_FOUND)
		{
			nPosAtSymbol = tempStr.Find(_T('@'));
			wxString localPart = tempStr.Mid(0,nPosAtSymbol);
			wxString domainPart = tempStr.Mid(nPosAtSymbol+1);
			if (localPart.IsEmpty() || domainPart.IsEmpty())
			{
				bValidAddress = FALSE;
			}
		}
		else
		{
			bValidAddress = FALSE;
		}
		
		if (!bValidAddress)
		{
			wxString msg = _("The email address you entered [%s] is not valid - please enter a valid email address");
			msg = msg.Format(msg,tempStr.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,_("Information missing or incomplete"),wxICON_EXCLAMATION | wxOK);
			pTextYourEmailAddr->SetFocus();
			return FALSE; // keep dialog open
		}
	}
	if (pTextEmailSubject->GetValue().IsEmpty())
	{
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_("Please enter a brief summary/subject for your email"),_("Information missing or incomplete"),wxICON_EXCLAMATION | wxOK);
		pTextEmailSubject->SetFocus();
		return FALSE; // keep dialog open
	}
	// For the Description/Body text a minimum would be that it must differ from the original template text.
	// It may be a loaded report in which the Description was already filled it, so we don't want to compare
	// the edit box contents with the saveDescriptionBodyText in this case.
	if (pTextDescriptionBody->GetValue() == templateTextForDescription)
	{
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_("Please enter some description for the body of your email"),_("Information missing or incomplete"),wxICON_EXCLAMATION | wxOK);
		pTextDescriptionBody->SetFocus();
		return FALSE; // keep dialog open
	}
	if (pTextSendersName->GetValue().IsEmpty())
	{
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_("Please enter your name so we can respond to you by name"),_("Information missing or incomplete"),wxICON_EXCLAMATION | wxOK);
		pTextSendersName->SetFocus();
		return FALSE; // keep dialog open
	}

	// If we get here all necessary fields have data.
	return TRUE;
}

void CEmailReportDlg::OnYourEmailAddressEditBoxChanged(wxCommandEvent& WXUNUSED(event))
{
	// Set bYouEmailAddrHasUnsavedChanges to TRUE if the edit box change results in
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
	// Set bSubjectHasUnsavedChanges to TRUE if the edit box change results in
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
	// Set bDescriptionBodyHasUnsavedChanges to TRUE if the edit box change results in
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

void CEmailReportDlg::OnSendersNameEditBoxChanged(wxCommandEvent& WXUNUSED(event))
{
	// Set bSendersNameHasUnsavedChanges to TRUE if the edit box change results in
	// an actual change from save... values established in InitDialgo()
	if (pTextSendersName->GetValue() != saveSendersName)
	{
		bSendersNameHasUnsavedChanges = TRUE;
	}
	else
	{
		bSendersNameHasUnsavedChanges = FALSE;
	}
}

wxString CEmailReportDlg::FormatSysInfoIntoString()
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxString sysInfoStr;
	sysInfoStr.Empty();
	// Fill in the System Information fields
	// Get the AI Version number from the App
	sysInfoStr = _T("AI Version: ");
	sysInfoStr += pApp->GetAppVersionOfRunningAppAsString(); //ID_TEXT_AI_VERSION
	sysInfoStr += _T('\n');
	// Get date from string constants at beginning of Adapt_It.h.
	wxString versionDateStr;
	versionDateStr.Empty();
	versionDateStr << VERSION_DATE_YEAR;
	versionDateStr += _T("-");
	versionDateStr << VERSION_DATE_MONTH;
	versionDateStr += _T("-");
	versionDateStr << VERSION_DATE_DAY;
	sysInfoStr += _T("Release Date: ");
	sysInfoStr += versionDateStr;
	sysInfoStr += _T('\n');
	
	wxString UnicodeOrAnsiBuild = _T("Regular (not Unicode)");
#ifdef _UNICODE
	UnicodeOrAnsiBuild = _T("UNICODE");
#endif
	sysInfoStr += _T("Data type: ");
	sysInfoStr += UnicodeOrAnsiBuild; //ID_TEXT_DATA_TYPE
	sysInfoStr += _T('\n');
	
	wxString memSizeStr;
	wxMemorySize memSize;
	wxLongLong MemSizeMB;
	memSize = pApp->GetFreeMemory();
	MemSizeMB = memSize / 1048576;
	memSizeStr << MemSizeMB;
	sysInfoStr += _T("Free Memory (MB): ");
	sysInfoStr += memSizeStr; //ID_TEXT_FREE_MEMORY
	sysInfoStr += _T('\n');
	
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
	sysInfoStr += _T("Sys Locale: ");
	sysInfoStr += tempStr; //ID_TEXT_SYS_LOCALE
	sysInfoStr += _T('\n');
	
	wxString strUILanguage;
	// Fetch the UI language info from the global currLocalizationInfo struct
	strUILanguage = pApp->currLocalizationInfo.curr_fullName;
	strUILanguage.Trim(FALSE);
	strUILanguage.Trim(TRUE);
	sysInfoStr += _T("Interface Language: ");
	sysInfoStr += strUILanguage; //ID_TEXT_INTERFACE_LANGUAGE
	sysInfoStr += _T('\n');
	
	sysInfoStr += _T("Sys Encoding: ");
	sysInfoStr += pApp->m_systemEncodingName; //ID_TEXT_SYS_ENCODING
	sysInfoStr += _T('\n');
	
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
	sysInfoStr += _T("Sys Layout Dir: ");
	sysInfoStr += layoutDirStr; //ID_TEXT_SYS_LAYOUT_DIR
	sysInfoStr += _T('\n');
	
	wxString versionStr;
	versionStr.Empty();
	versionStr << wxMAJOR_VERSION;
	versionStr << _T(".");
	versionStr << wxMINOR_VERSION;
	versionStr << _T(".");
	versionStr << wxRELEASE_NUMBER;
	sysInfoStr += _T("wxWidgets version: ");
	sysInfoStr += versionStr; //ID_TEXT_WXWIDGETS_VERSION
	sysInfoStr += _T('\n');
	
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
	sysID = sysID; // avoid warning
	osVersionStr = archName;
	osVersionStr += _T(' ') + OSSystemID;
	osVersionStr += _T(' ');
	osVersionStr << OSMajorVersion;
	osVersionStr += _T('.');
	osVersionStr << OSMinorVersion;
	sysInfoStr += _T("OS version: ");
	sysInfoStr += osVersionStr; //ID_TEXT_OS_VERSION
	sysInfoStr += _T('\n');
	return sysInfoStr;
}

// whm 1Dec2019 added: This function makes the input string safe to use with the
// mailto: URL character sequence when the string is sent to the user's default email
// client using the launch commnad. 
// Input strings to this function would primarily be from the subject and body fields
// that compose the mailto: string.
// This function can be used to process text data coming from within a single
// or multiline wxTextCtrl such as the user-editable fields of the EmailReportDlg 
// dialog (the "Description/body field is multi-line so it may have embedded \n characters).
// The function replaces certain URL metacharacters (see below) with %nn hex equivalents,
// for example, a space character becomes %20, a colon character : becomes %3F, an 
// ampersand & character becomes % , a newline characer \n becomes %0A, etc.
wxString CEmailReportDlg::FormatEditBoxStringInfoIntoURLSafeString(wxString str)
{
    wxString tempStr;
    tempStr = str;
    // The following illegal characters should be replaced by their %nn hex equivalents to make them safe
    // for the subject and body elements of a mailto: sequence of chars:
    // The forbidden printable ASCII characters are :
    // Linux / Unix :
    //    / (forward slash)
    // Windows :
    //    < (less than)
    //    > (greater than)
    //    : (colon - sometimes works, but is actually NTFS Alternate Data Streams)
    //    " (double quote)
    //    / (forward slash)
    //    \ (backslash)
    //    | (vertical bar or pipe)
    //    ? (question mark)
    //    * (asterisk)
    // The Non - printable characters
    // Linux / Unix:
    //    0 (NULL byte)
    // Windows :
    //    0 - 31 (ASCII control characters)
    // In addition to the "illegal" chars, the following characters should be replaced by 
    // their %nn hex equivalents to make safe for the subject and body of a mailto: sequence
    // of chars:
    //    \n newline replaced by %0A
    //    space replaced by %20
    //    & replaced by 
    //    @ replaced by 
    //    colon : replaced by 
    //    TODO:
    //wxString charsToConvert = _T("\n\r &@:/\\?|*\"<>");
    // Do a series of wxString::Replace() operations
    bool replaceAll = TRUE; // the 3rd parameter of Replace() is TRUE by default, but we'll make it expolicit here
    tempStr.Replace(_T("\n"), _T("%0A"), replaceAll);
    tempStr.Replace(_T("\r"), _T(""), replaceAll); // normally a str coming from a wxTextCtrl should not have any \r, only \n chars, so remove any \r chars
    tempStr.Replace(_T("\t"), _T(""), replaceAll); // remove any embedded tab chars
    tempStr.Replace(_T(" "), _T("%20"), replaceAll);
    tempStr.Replace(_T("&"), _T("%26"), replaceAll);
    tempStr.Replace(_T("@"), _T("%40"), replaceAll);
    tempStr.Replace(_T(":"), _T("%3A"), replaceAll);
    tempStr.Replace(_T("/"), _T("%2F"), replaceAll);
    tempStr.Replace(_T("\\"), _T("%5C"), replaceAll);
    tempStr.Replace(_T("?"), _T("%3F"), replaceAll);
    tempStr.Replace(_T("|"), _T("%7C"), replaceAll);
    tempStr.Replace(_T("*"), _T("%2A"), replaceAll);
    tempStr.Replace(_T("\""), _T("%22"), replaceAll);
    tempStr.Replace(_T("<"), _T("%3C"), replaceAll);
    tempStr.Replace(_T(">"), _T("%3E"), replaceAll);
    return tempStr;
}

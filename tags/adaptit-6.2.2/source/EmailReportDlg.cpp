/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			EmailReportDlg.cpp
/// \author			Bill Martin
/// \date_created	7 November 2010
/// \date_revised	11 November 2010
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

// libcurl includes:
#include <curl/curl.h>
//#include <curl/types.h>
#include <curl/easy.h>

#include "Adapt_It.h"
#include "Adapt_ItDoc.h"
#include "helpers.h"
#include "XML.h"
#include "EmailReportDlg.h"
#include "base64.h" // for wxBase64 encode of zipped/packed attachments
	
static int totalBytesSent = 0;

// a helper class for CEmailReportDlg
CLogViewer::CLogViewer(wxWindow* parent)
: AIModalDialog(parent, -1, _("View User Log File"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	LogViewerFunc(this, TRUE, TRUE); // wxDesigner dialog function
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
		wxMessageBox(msg,_T(""),wxICON_INFORMATION);
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
	
	bEmailSendSuccessful = FALSE;
	
	pTextDeveloperEmails = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_DEVS_EMAIL_ADDR);
	wxASSERT(pTextDeveloperEmails != NULL);
	
	pTextYourEmailAddr = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_MY_EMAIL_ADDR);
	wxASSERT(pTextYourEmailAddr != NULL);
	
	pTextEmailSubject = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SUMMARY_SUBJECT);
	wxASSERT(pTextEmailSubject != NULL);

	pTextSendersName = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SENDERS_NAME);
	wxASSERT(pTextSendersName != NULL);
	
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
	saveSendersName = pTextSendersName->GetValue(); // it will be empty
	
	pRadioSendItToMyEmailPgm->Disable(); // for 6.0.0 we probably won't get to implementing this option

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
			// TODO: modify below to convert userLogContents to zip archive format and base64 encoding
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
				wxMessageBox(msg,_T(""),wxICON_ERROR);
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
				userLogInBase64 = wxBase64::Encode(pByteBuf,fileLen);
				free((void*)pByteBuf);
			}

		}

		wxLogDebug(wxT("OnSend"));

		curl_global_init(CURL_GLOBAL_ALL);

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

		curl = curl_easy_init(); // curl is the handle
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
			curl_easy_setopt(curl, CURLOPT_URL, "https://adapt-it.org/feedback.php"); // Use this URL for SSL connection
			//curl_easy_setopt(curl, CURLOPT_URL, "http://adapt-it.org/feedback.php"); // Use this URL for non-secured connection
			
			// Note: the path in the following CRULOPT_CAINFO option is a path to the ca-buncle.crt file
			// in the Windows distribution of Adapt It. During development the ca-bundle.crt file
			// was created by the developer by invoking a perl script called mk-ca-bundle.pl located 
			// at c:\curl-7.21.2\lib\ca-bundle.crt on the developer's machine. Linux and Mac systems
			// know how to find their own ca-bundle.crt files.
			// TODO: determine if we need to conditional compile the next line for the Windows only port
			// of if it can also be used this way for Linux and the Mac
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
			wxMessageBox(_T("The curl utility could not be initialized"),_T(""),wxICON_INFORMATION);
			pApp->LogUserAction(_T("The curl utility could not be initialized"));
		}
		curl_global_cleanup(); // whm added 8May12 to see if it avoids memory leaks. No, but it should be called here.
	}
	else
	{
		// TODO: Add code to send the email to the user's default email client
	}

	// If the email cannot be sent, automatically invoke the OnBtnSaveReportAsXmlFile() 
	// handler so the user can save any edits made before closing the dialog.
	if (!bEmailSendSuccessful)
	{
		wxString msg1,msg2;
		msg1 = _("Your email could not be sent at this time.\nAdapt It will save a copy of your report in your work folder (for sending at a later time).");
		msg2 = msg2.Format(_("Error %d: %s"),curl_result,errorStr.c_str());
		msg2 = _T("\n\n") + msg2;
		msg1 = msg1 + msg2;
		wxMessageBox(msg1,_T(""),wxICON_INFORMATION);
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
			wxMessageBox(msg,_T(""),wxICON_WARNING);
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
			wxMessageBox(msg,_T(""),wxICON_WARNING);
			return;
		}
		else
		{
			wxString msg1;
			msg1 = msg1.Format(_("Your email was sent to the Adapt It developers.\nThank you for your report.\nThe email contained %d bytes of data.\nAdapt It will put a copy of the sent report in your work folder at:\n   %s"),totalBytesSent,nameUsed.c_str());
			wxMessageBox(msg1,_T(""),wxICON_INFORMATION);
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
	// those in the special _REPORTS_LOGS folder regardless of whether navigation
	// protection is in effect. The nav protection for _REPORTS_LOGS only really
	// applies to Retranslation Reports. Other types of reports/logs including
	// this dialog's email reports go to the special folder regardless of the 
	// protection setting for _REPORTS_LOGS in AssignLocationsForInputsAndOutputs.

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
		wxMessageBox(msg,_("Problem creating or writing the xml file"),wxICON_WARNING);
		pApp->LogUserAction(msg);
	}
	else
	{
		// if it is a sent report don't give this message because the caller already gives a more detailed message
		if (nameSuffix.IsEmpty())
		{
			msg = msg.Format(_("The email report was saved at the following work folder path:\n   %s"),reportPathAndName.c_str());
			wxMessageBox(msg,_("Your report was saved for later use or reference"),wxICON_INFORMATION);
			pApp->LogUserAction(msg);
		}
		bSubjectHasUnsavedChanges = FALSE;
		bYouEmailAddrHasUnsavedChanges = FALSE;
		bDescriptionBodyHasUnsavedChanges = FALSE;
		bSendersNameHasUnsavedChanges = FALSE;
	}
	return bReportBuiltOK;
}

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
		response = wxMessageBox(_("You made changes to this report - Do you want to save those changes?"),_T(""),wxYES_NO | wxICON_INFORMATION);
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
				wxMessageBox(msg,_T(""),wxICON_WARNING);
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
		bReadOK = ReadEMAIL_REPORT_XML(pathAndName,NULL,0);
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
					response = wxMessageBox(pdMsg1,_("Could not find the packed document in your work folder"),wxYES_NO | wxICON_WARNING);
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
		response = wxMessageBox(_("You made changes to this report - Do you want to save those changes?"),_T("This dialog is about to close..."),wxYES_NO | wxICON_INFORMATION);
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
				wxMessageBox(msg,_T(""),wxICON_WARNING);
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
		wxMessageBox(_("A document must be open before Adapt It can pack it.\nClose the report dialog and open a document, then try again."),_("No document open"),wxICON_INFORMATION);
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
		packedDocInBase64 = wxBase64::Encode(pByteBuf,fileLen);
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
		wxMessageBox(_("Please enter some description for the body of your email"),_T("Information missing or incomplete"),wxICON_WARNING);
		pTextDescriptionBody->SetFocus();
		return FALSE; // keep dialog open
	}
	if (pTextSendersName->GetValue().IsEmpty())
	{
		wxMessageBox(_("Please enter your name so we can respond to you by name"),_T("Information missing or incomplete"),wxICON_WARNING);
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
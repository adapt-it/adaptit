/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBServer.cpp
/// \author			Kevin Bradford, Bruce Waters
/// \date_created	26 September 20012
/// \date_revised	
/// \copyright		2012 Kevin Bradford, Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for functions involved in kbserver support. 
/// This .cpp file contains member functions of the CAdapt_ItApp class, and so is an
/// extension to the Adapt_It.cpp class.
/////////////////////////////////////////////////////////////////////////////




// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KbServer.h"
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

#include <wx/utils.h> // for ::wxDirExists, ::wxSetWorkingDirectory, etc
#include <wx/textfile.h> // for wxTextFile

#include "Adapt_It.h" // has to be here, since _KBSERVER symbol is defined there
#if defined(_KBSERVER)


#include "TargetUnit.h"
#include "KB.h"
//#include "AdaptitConstants.h"
#include "RefString.h"
#include "RefStringMetadata.h"
#include "helpers.h"
//#include "BString.h"
#include "Xhtml.h"
#include "KbServer.h"

extern bool		gbIsGlossing;
static int		totalBytesSent = 0;

// A Note on the placement of SetupForKBServer(), and the protective wrapping boolean
// m_bIsKBServerProject, defaulting to FALSE initially and then possibly set to TRUE by
// reading the project configuration file.
// Since the KBs must be loaded before SetupForKBServer() is called, and the function for
// setting them up is CreateAndLoadKBs(), it may appear that at the end of the latter
// would be an appropriate place, within the TRUE block of an if (m_bIsKBServerProject)
// test. However, looking at which functionalities, at a higher level, call
// CreateAndLoadKBs(), many of these are too early for any association of a project with
// a kbserver to have been set up already. Therefore, possibly after a read of the
// project configuration file may be appropriate -
// since it's that configuration file which sets or clears the m_bIsKBServerProject app
// member boolean.This is so: there are 5 contexts where the project config file is read:
// in OnOpenDocument() for a MRU open which bypasses the ProjectPage of the wizard -- NO
// LONGER, Bill has removed MRU support by approx 1 October 2012; in
// DoUnpackDocument(), in HookUpToExistingProject() for setting up a collaboration, in
// the OpenExistingProjectDlg.cpp file, associated with the "Access an existing
// adaptation project" feature (for transforming adaptations to glosses); and most
// importantly, in the frequently called OnWizardPageChanging() function of
// ProjectPage.cpp. These are all appropriate places for calling SetupForKBServer() late
// in such functions, in an if TRUE test block using m_bIsKBServerProject. (There is no
// need, however, to call it in OpenExistingProjectDlg() because the project being opened
// is not changed in the process, and since it's adaptations are being transformed to
// glosses, it would not be appropriate to assume that the being-constructed new project
/// should be, from it's inception, a kb sharing one. The user can later make it one if he
// so desires.)

//=============================== KbServer class ===================================

std::string str_CURLbuffer;

IMPLEMENT_DYNAMIC_CLASS(KbServer, wxObject)

// the default constructor, probably will never be used
KbServer::KbServer()
{
	m_pApp = &wxGetApp();

	/* removed, to make KbServer class more independent of CKB class
	// set up pointer copies for the current project's m_pKB and m_pGlossingKB CKB* pointers
	m_pMyKB = SetKB(adaptingKB);
	m_pMyGlossingKB = SetKB(glossingKB);

	if (!SetKBTypeForServer())
	{
		delete this;
		m_pApp->SetKbServer(NULL);
		return;
	}
	*/
	// Get the url, username, and (for the development code only) password credentials;
	// the function call empties the credentials strings, and resets them from the
	// credentials.txt file; if there is any error, then all three are url, username, and
	// password are returned empty ( and a message to the developer will have been seen)
	if(!GetCredentials(m_kbServerURLBase, m_kbServerUsername, m_kbServerPassword))
	{
		delete this;
		m_pApp->SetKbServer(NULL);
		return;
	}

	// Get the lastsync datetime string (as a CBString) from persistent storage (which
	// temporarily is the single-line file lastsync.txt in the project folder)
	m_kbServerLastSync = ImportLastSyncDateTime();
	if (m_kbServerLastSync.IsEmpty())
	{
		delete this;
		m_pApp->SetKbServer(NULL);
		return;
	}

}

KbServer::KbServer(CAdapt_ItApp* pApp)
{	
	m_pApp = pApp; // avoid compiler warning

	/* removed, to make KbServer class more independent of CKB class
	// set up pointer copies for the current project's m_pKB and m_pGlossingKB CKB* pointers
	m_pMyKB = SetKB(adaptingKB);
	m_pMyGlossingKB = SetKB(glossingKB);

	if (!SetKBTypeForServer())
	{
		delete this;
		return;
	}
	*/
	// Initial Testing 
	/*
	m_kbServerURLBase = _T("https://kbserver.jmarsden.org/entry");
	m_kbServerUsername = _T("kevin_bradford@sil.org");
	m_kbServerPassword = _T("password");
	m_kbTypeForServer = 1;
	*/

	// Get the url, username, and (for the development code only) password credentials;
	// the function call empties the credentials strings, and resets them from the
	// credentials.txt file; if there is any error, then all three are url, username, and
	// password are returned empty ( and a message to the developer will have been seen)
	if(!GetCredentials(m_kbServerURLBase, m_kbServerUsername, m_kbServerPassword))
	{
		delete this;
		m_pApp->SetKbServer(NULL);
		return;
	}

	// Get the lastsync datetime string (as a CBString) from persistent storage (which
	// temporarily is the single-line file lastsync.txt in the project folder)
	m_kbServerLastSync = ImportLastSyncDateTime();
	if (m_kbServerLastSync.IsEmpty())
	{
		delete this;
		m_pApp->SetKbServer(NULL);
		return;
	}
}

KbServer::~KbServer()
{
	; // nothing to do as yet
}

/*
CKB* KbServer::SetKB(enum KBType currentKBType)
{
	if (currentKBType == adaptingKB)
	{
		return m_pApp->m_pKB;
	}
	else
	{
		return m_pApp->m_pGlossingKB;
	}
}
*/

void KbServer::ErasePassword()
{
	m_kbServerPassword.Empty();
}

int	KbServer::GetKBTypeForServer()
{
	return m_kbTypeForServer;
}

// these return CBString
CBString KbServer::GetServerURL()
{
	return KbServer::m_kbServerURLBase;
}
CBString KbServer::GetServerUsername()
{
	return m_kbServerUsername;
}
CBString KbServer::GetServerPassword()
{
	return m_kbServerPassword;
}
CBString KbServer::GetServerLastSync()
{
	return m_kbServerLastSync;
}

CBString KbServer::GetSourceLanguageCode()
{
	return m_pApp->Convert16to8(m_pApp->m_sourceLanguageCode);
}
CBString KbServer::GetTargetLanguageCode()
{
	return m_pApp->Convert16to8(m_pApp->m_targetLanguageCode);
}


/// If KBs have not been loaded, return false. If the loads have not been done, m_pKB and m_pGlossingKB will be still
/// NULL, and m_bKBReady and m_bGlossingKBReady will both be FALSE - the latter two
/// conditions are how we test for bad placement.
bool KbServer::SetKBTypeForServer()
{
	//note: similar code above can be removed when this is tested
	m_kbTypeForServer = 1; // default is to assume adapting KB is wanted
	if (gbIsGlossing)
	{
		m_kbTypeForServer = 2;
	}
/*
	// the two KBs must have been successfully loaded
	if (m_pApp->m_bKBReady && m_pApp->m_bGlossingKBReady)
	{
		if (gbIsGlossing)
		{
			m_kbTypeForServer = 2;
		}
	}
	else
	{
		// warn developer that the logic is wrong
		wxString msg = _T("GetKBTypeForServer()called in SetupForKBServer(): Logic error, m_bKBReady and m_bGlossingKBReady are not both TRUE yet. Fix this!");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return false;
	}
*/	
	return true;	
}

// Return FALSE if pf not successfully opened, in which case the parent does not need to
// close pf; otherwise, return TRUE for a successful open & parent must close it when done
bool KbServer::GetTextFileOpened(wxTextFile* pf, wxString& path)
{
	bool bSuccess = FALSE;
	// for 2.9.4 builds, the conditional compile isn't needed I think, and for Linux and
	// Mac builds which are Unicode only, it isn't needed either but I'll keep it for now
#ifndef _UNICODE
		// ANSI
		bSuccess = pf->Open(path); // read ANSI file into memory
#else
		// UNICODE
		bSuccess = pf->Open(path, wxConvUTF8); // read UNICODE file into memory
#endif
	return bSuccess;
}


/// Return TRUE, and the url and username credentials (and while doing testing, also the
/// kbserver password) to be stored in the app class. Temporarily this data is storedin a
/// file called credentials.txt located in the project folder and contains url, username,
/// password, one string per line. But later something more permanent will be used, and the
/// kbserver password will never be stored in the app (so a permanent version of this code
/// will only have the first two params in its signature) once a releasable version of kbserver
/// support has been built (use a hidden file for url and username in the project folder?)
/// BEW 3Oct12 changed to return CBString rather than wxString
//bool CAdapt_ItApp::GetCredentials(wxString& url, wxString& username, wxString& password)
bool KbServer::GetCredentials(CBString& url, CBString& username, CBString& password)
{
	bool bSuccess = FALSE;
	url.Empty(); username.Empty(); password.Empty();

	wxString filename = _T("credentials.txt");
	wxString path = m_pApp->m_curProjectPath + m_pApp->PathSeparator + filename;
	bool bCredentialsFileExists = ::wxFileExists(path);
	if (!bCredentialsFileExists)
	{
		// couldn't find credentials.txt file in project folder
		wxString msg = _T("wxFileExists() called in KbServer::GetCredentials(): The credentials.txt file does not exist");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return FALSE; // signature params are empty still
	}
	wxTextFile f;
	// for 2.9.4 builds, the conditional compile isn't needed I think, and for Linux and
	// Mac builds which are Unicode only, it isn't needed either but I'll keep it for now
	bSuccess = GetTextFileOpened(&f, path);
	if (!bSuccess)
	{
		// warn developer that the wxTextFile could not be opened
		wxString msg = _T("GetTextFileOpened()called in GetCredentials(): The wxTextFile could not be opened");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return FALSE; // signature params are empty still
	}
	size_t numLines = f.GetLineCount();
	if (numLines < 3)
	{
		// warn developer that the wxTextFile lackes expected 3 lines: url, username,
		// password -- NOTE: password is only a TEMPORARY PARAMETER
		wxString msg;
		msg = msg.Format(_T("GetTextFileOpened()called in GetCredentials(): The credentials.txt file lacks one or more lines, it has %d of expected 3 (url,username,password)"),
			numLines);
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		f.Close();
		return FALSE; // signature params are empty still
	}
	url = m_pApp->Convert16to8(f.GetLine(0));
	username = m_pApp->Convert16to8(f.GetLine(1));
	password = m_pApp->Convert16to8(f.GetLine(2));

	wxLogDebug(_T("GetCredentials(): url = %s  ,  username = %s , password = %s"), 
		m_pApp->Convert8to16(url).c_str(), m_pApp->Convert8to16(username).c_str(), m_pApp->Convert8to16(password).c_str());

	f.Close();
	

// *** TODO *** the permanent code -- alter the above

	return TRUE;
}

// Get and return the dateTime value last stored in persistent (i.e. on disk) storage.
// Temporarily this storage is a file called lastsync.txt located in the project folder,
// but later something more permanent will be used (a hidden file in the project folder?)
// KbServer class has a private CBString member, m_kbServerLastSync to store the 
// returned value. (May be called more than once in the life of a KbServer instance)
// BEW 3Oct12 changed to return CBString rather than wxString
CBString KbServer::ImportLastSyncDateTime()
{
	CBString dateTimeStr; dateTimeStr.Empty();

	wxString filename = _T("lastsync.txt");
	wxString path = m_pApp->m_curProjectPath + m_pApp->PathSeparator + filename;
	bool bLastSyncFileExists = ::wxFileExists(path);
	if (!bLastSyncFileExists)
	{
		// couldn't find lastsync.txt file in project folder
		wxString msg = _T("wxFileExists()called in ImportLastSyncDateTime(): The wxTextFile, taking path to lastsync.txt, does not exist");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return dateTimeStr; // it's empty still
	}
	wxTextFile f;
	bool bSuccess = FALSE;
	// for 2.9.4 builds, the conditional compile isn't needed I think, and for Linux and
	// Mac builds which are Unicode only, it isn't needed either but I'll keep it for now
	bSuccess = GetTextFileOpened(&f, path);
	if (!bSuccess)
	{
		// warn developer that the wxTextFile could not be opened
		wxString msg = _T("GetTextFileOpened()called in ImportLastSyncDateTime(): The wxTextFile could not be opened");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return dateTimeStr; // it's empty still
	}
	size_t numLines = f.GetLineCount();
	if (numLines == 0)
	{
		// warn developer that the wxTextFile is empty
		wxString msg = _T("GetTextFileOpened()called in ImportLastSyncDateTime(): The lastsync.txt file is empty");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		f.Close();
		return dateTimeStr; // it's empty still
	}
	// whew, finally, we have the lastsync datetime string
	//dateTimeStr = f.GetLine(0); 
	dateTimeStr = m_pApp->Convert16to8(f.GetLine(0)); // return a CBString
	f.Close();
	

// *** TODO *** the permanent code -- alter the above

	return dateTimeStr;
}

// Takes the kbserver's datetime supplied with downloaded data, as stored in the
// m_kbServerLastSync member, and stores it on disk (temporarily in the file lastsync.txt
// located in the AI project folder) Return TRUE if no error, FALSE otherwise.
// BEW 3Oct12, changed to use CBString rather than wxString
bool KbServer::ExportLastSyncDateTime()
{
	wxString filename = _T("lastsync.txt");
	wxString path = m_pApp->m_curProjectPath + m_pApp->PathSeparator + filename;
	bool bLastSyncFileExists = ::wxFileExists(path);
	if (!bLastSyncFileExists)
	{
		// couldn't find lastsync.txt file in project folder - tell developer
		wxString msg = _T("wxFileExists()called in ExportLastSyncDateTime(): The wxTextFile, taking path to lastsync.txt, does not exist");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return FALSE;
	}
	wxTextFile f;
	bool bSuccess = FALSE;
    // for 2.9.4 builds, the conditional compile within GetTextFileOpened() isn't needed I
    // think, and for Linux and Mac builds which are Unicode only, it isn't needed either
    // but I'll keep it for now
	bSuccess = GetTextFileOpened(&f, path);
	if (!bSuccess)
	{
		// warn developer that the wxTextFile could not be opened
		wxString msg = _T("GetTextFileOpened()called in ExportLastSyncDateTime(): The wxTextFile, taking path to lastsync.txt, could not be opened");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return FALSE;
	}
	f.Clear(); // chuck earlier value
	f.AddLine(m_pApp->Convert8to16(m_kbServerLastSync)); // BEW 3Oct12, this param, in Adapt_It.h is a CBString already
	f.Close();
	

// *** TODO *** the permanent code -- alter the above

	return TRUE;
}

// Prompt the user to type in the appropriate kbserver password, and return it. It will be
// stored in the app's m_kbServerPassword CBString member for a short time until no longer needed, and
// then that member is cleared by calling ErasePassword()
// BEW 3Oct12, changed to use CBString rather than wxString
 CBString KbServer::GetKBServerPassword()
{
	// temporarily, do nothing. Later on, replace this code with a dialog for getting the
	// password, etc - if that fails,or if user cancels, then return a null string so that
	// the caller can clobber the KbServer instance
	CBString myPassword;
	myPassword.Empty();
	return myPassword;
}


// callback function for curl  
size_t curl_read_data_callback(void *ptr, size_t size, size_t nmemb, void *userdata) 
{
	//
	//ptr = ptr; // avoid "unreferenced formal parameter" warning
	userdata = userdata; // avoid "unreferenced formal parameter" warning
    
	//wxString msg;
	//msg = msg.Format(_T("In curl_read_data_callback: sending %s bytes."),(size*nmemb));
	//wxLogDebug(msg);
    	
	str_CURLbuffer.append((char*)ptr, size*nmemb);
	return size*nmemb;
}
/*
wxString KbServer::LookupEntryForSourcePhrase( wxString wxStr_SourceEntry )
{
	CURL *curl;
	CURLcode result;
	char charUrl[1024];
	char charUserpwd[511];

	wxString wxStr_URL = GetServerURL() + '/' + GetSourceLanguageCode() + '/' + GetTargetLanguageCode() + '/' + 
		wxString::Format(_T("%d"), GetKBTypeForServer()) + '/' + wxStr_SourceEntry; 
	strncpy( charUrl , wxStr_URL.c_str() , 1024 );

	wxString wxStr_Authentication = GetServerUsername() + ':' + GetServerPassword();
	strncpy( charUserpwd , wxStr_Authentication.c_str() , 511 );	
		
	curl_global_init(CURL_GLOBAL_ALL); 
	curl = curl_easy_init(); 
	
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, charUserpwd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback); 
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, str_CURLbuffer);

		result = curl_easy_perform(curl);

		if (result) {
			printf("Result code: %d\n", result);
		} 
		curl_global_cleanup();
	}

	return str_CURLbuffer;

}
*/
wxString KbServer::LookupEntryForSourcePhrase( wxString wxStr_SourceEntry )
{
	CURL *curl;
	CURLcode result;
	CBString charUrl;
	CBString charUserpwd;

	CBString slash('/'); // the same CBString constructor also supports the syntax
	CBString colon(':'); // CBString slash = '/'; by means of C++ implicit conversion
	CBString kbType;
	wxItoa(GetKBTypeForServer(),kbType);

	charUrl = GetServerURL() + slash + GetSourceLanguageCode() + slash + GetTargetLanguageCode() +
					slash + kbType + slash + m_pApp->Convert16to8(wxStr_SourceEntry); 
	charUserpwd = GetServerUsername() + colon + GetServerPassword();
	
	curl_global_init(CURL_GLOBAL_ALL); 
	curl = curl_easy_init(); 
	
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback); 
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, str_CURLbuffer);

		result = curl_easy_perform(curl);

		if (result) {
			printf("Result code: %d\n", result);
		} 
		curl_global_cleanup();
	}

	return str_CURLbuffer;
}


//=============================== end of KbServer class ============================

#endif // for _KBSERVER


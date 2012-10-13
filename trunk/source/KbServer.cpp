/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KbServer.cpp
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

#if defined(_KBSERVER)

using namespace std;
#include <string>

#include "Adapt_It.h"
#include "TargetUnit.h"
#include "KB.h"
//#include "AdaptitConstants.h"
#include "RefString.h"
#include "RefStringMetadata.h"
#include "helpers.h"
//#include "BString.h"
#include "Xhtml.h"
#include "KbServer.h"

// for wxJson support
#include "json_defs.h" // BEW tweaked to disable 64bit integers, else we get compile errors
#include "jsonreader.h"
#include "jsonwriter.h"
#include "jsonval.h"

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
// Scrap the above comment if we choose instead to instantiate KbServer on demand and
// destroy it immediately afterwards.

//=============================== KbServer class ===================================

std::string str_CURLbuffer;

IMPLEMENT_DYNAMIC_CLASS(KbServer, wxObject)

KbServer::KbServer()
{
	// Initial Testing
	/*
	m_kbServerURLBase = _T("https://kbserver.jmarsden.org/entry");
	m_kbServerUsername = _T("kevin_bradford@sil.org");
	m_kbServerPassword = _T("password");
	m_kbTypeForServer = 1;
	*/
}

KbServer::~KbServer()
{
	; // nothing to do as yet
}

void KbServer::ErasePassword()
{
	m_kbServerPassword.Empty();
}

CBString KbServer::ToUtf8(const wxString& str)
{
	// converts UTF-16 strings to UTF-8  No need for #ifdef _UNICODE and #else
	// define blocks.
	wxCharBuffer tempBuf = str.mb_str(wxConvUTF8);
	return CBString(tempBuf);
}

wxString KbServer::ToUtf16(CBString& bstr)
{
	// whm 21Aug12 modified. No need for #ifdef _UNICODE and #else
	// define blocks.
	wxWCharBuffer buf(wxConvUTF8.cMB2WC(bstr.GetBuffer()));
	return wxString(buf);
}

// the private getters

int KbServer::GetKBServerType()
{
	return m_kbServerType;
}

wxString KbServer::GetKBServerURL()
{
	return m_kbServerURLBase;
}
wxString KbServer::GetKBServerUsername()
{
	return m_kbServerUsername;
}
wxString KbServer::GetKBServerPassword()
{
	return m_kbServerPassword;
}
wxString KbServer::GetKBServerLastSync()
{
	return m_kbServerLastSync;
}

wxString KbServer::GetSourceLanguageCode()
{
	return m_kbSourceLanguageCode;
}

wxString KbServer::GetTargetLanguageCode()
{
	return m_kbTargetLanguageCode;
}

wxString KbServer::GetGlossLanguageCode()
{
	return m_kbGlossLanguageCode;
}

wxString KbServer::GetPathToPersistentDataStore()
{
	return m_pathToPersistentDataStore;
}

wxString KbServer::GetPathSeparator()
{
	return m_pathSeparator;
}

wxString KbServer::GetCredentialsFilename()
{
	return m_credentialsFilename;
}

wxString KbServer::GetLastSyncFilename()
{
	return m_lastSyncFilename;
}


// the public setters
//
void KbServer::SetKBServerType(int type)
{
	m_kbServerType = type;
}

void KbServer::SetKBServerURL(wxString url)
{
	m_kbServerURLBase = url;
}

void KbServer::SetKBServerUsername(wxString username)
{
	m_kbServerUsername = username;
}

void KbServer::SetKBServerPassword(wxString pw)
{
	m_kbServerPassword = pw;
}

void KbServer::SetKBServerLastSync(wxString lastSyncDateTime)
{
	m_kbServerLastSync = lastSyncDateTime;
}

void KbServer::SetSourceLanguageCode(wxString sourceCode)
{
	m_kbSourceLanguageCode = sourceCode;
}

void KbServer::SetTargetLanguageCode(wxString targetCode)
{
	m_kbTargetLanguageCode = targetCode;
}

void KbServer::SetGlossLanguageCode(wxString glossCode)
{
	m_kbGlossLanguageCode = glossCode;
}

void KbServer::SetPathToPersistentDataStore(wxString metadataPath)
{
	m_pathToPersistentDataStore = metadataPath;
}

void KbServer::SetPathSeparator(wxString separatorStr)
{
	m_pathSeparator = separatorStr;
}

void KbServer::SetCredentialsFilename(wxString credentialsFName)
{
	m_credentialsFilename = credentialsFName;
}
void KbServer::SetLastSyncFilename(wxString lastSyncFName)
{
	m_lastSyncFilename = lastSyncFName;
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



// Get and return the dateTime value last stored in persistent (i.e. on disk) storage.
// Temporarily this storage is a file called lastsync.txt located in the project folder,
// but later something more permanent will be used (a hidden file in the project folder?)
// KbServer class has a private CBString member, m_kbServerLastSync to store the
// returned value. (May be called more than once in the life of a KbServer instance)

wxString KbServer::ImportLastSyncDateTime()
{
	wxString dateTimeStr; dateTimeStr.Empty();

	wxString path = GetPathToPersistentDataStore() + GetPathSeparator() + GetLastSyncFilename();
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
	dateTimeStr = f.GetLine(0);
	f.Close();


// *** TODO *** the permanent code -- alter the above if necessary

	return dateTimeStr;
}

// Takes the kbserver's datetime supplied with downloaded data, as stored in the
// m_kbServerLastSync member, and stores it on disk (temporarily in the file lastsync.txt
// located in the AI project folder) Return TRUE if no error, FALSE otherwise.
bool KbServer::ExportLastSyncDateTime()
{
	wxString path = GetPathToPersistentDataStore() + GetPathSeparator() + GetLastSyncFilename();
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
	f.AddLine(GetKBServerLastSync());
	f.Close();

// *** TODO *** the permanent code -- alter the above

	return TRUE;
}

// callback function for curl
size_t curl_read_data_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	//ptr = ptr; // avoid "unreferenced formal parameter" warning
	userdata = userdata; // avoid "unreferenced formal parameter" warning

	//wxString msg;
	//msg = msg.Format(_T("In curl_read_data_callback: sending %s bytes."),(size*nmemb));
	//wxLogDebug(msg);

	str_CURLbuffer.append((char*)ptr, size*nmemb);
	return size*nmemb;
}

size_t curl_update_callback(void* ptr, size_t size, size_t nitems, void* userp)
{
	// userp will be the pointer to Kevin's std:string object, strCURLbuffer
	size = 1;
	nitems = (*((std::string*)userp)).length();
	ptr = (void*)(*((std::string*)userp)).c_str();
	return nitems; // strictly speaking, nitems*size, but size is 1
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

	//curl_global_init(CURL_GLOBAL_ALL); whm 13Oct12 removed
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
		// whm 13Oct12 modified. Changed the following cleanup call to
		// use the _easy_ version rather than the _global_ version.
		// See the similar code in EmailReportDlg.cpp about line 758.
		curl_easy_cleanup(curl); //curl_global_cleanup();
	}

	return str_CURLbuffer;

}
*/
// return the CURLcode value
int KbServer::LookupEntryForSourcePhrase( wxString wxStr_SourceEntry )
{
	CURL *curl;
	CURLcode result;
	wxString aUrl; // convert to utf8 when constructed
	wxString aPwd; // ditto

	CBString charUrl;
	CBString charUserpwd;

	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString kbType;
	wxItoa(GetKBServerType(),kbType);
	wxString container = _T("entry");

	aUrl = GetKBServerURL() + slash + container + slash+ GetSourceLanguageCode() +
			slash + GetTargetLanguageCode() + slash + kbType + slash + wxStr_SourceEntry;
	charUrl = ToUtf8(aUrl);
	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	// curl_global_init(CURL_GLOBAL_ALL); BEW only needs to be called once
	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);
		//curl_easy_setopt(curl, CURLOPT_WRITEDATA, str_CURLbuffer);

		result = curl_easy_perform(curl);

#if defined (_DEBUG) && defined (__WXGTK__)
        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif

		if (result) {
			printf("LookupEntryForSourcePhrase() result code: %d\n", result);
			return (int)result;
		}
		// whm 13Oct12 modified. Changed the following cleanup call to
		// use the _easy_ version rather than the _global_ version.
		// See the similar code in EmailReportDlg.cpp about line 758.
		curl_easy_cleanup(curl); //curl_global_cleanup();
	}

	return 0;
}

int KbServer::SendEntry(wxString srcPhrase, wxString tgtPhrase)
{
	CURL *curl;
	CURLcode result; // result code
	struct curl_slist* headers = NULL;
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString kbType;
	wxItoa(GetKBServerType(),kbType);
	wxJSONValue jsonval; // construct JSON object
	CBString strVal; // to store wxString form of the jsonval object, for curl
	wxString container = _T("entry");
	wxString aUrl, aPwd;

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	// populate the JSON object
	jsonval[_T("sourcelanguage")] = GetSourceLanguageCode();
	jsonval[_T("targetlanguage")] = GetTargetLanguageCode();
	jsonval[_T("source")] = srcPhrase;
	jsonval[_T("target")] = tgtPhrase;
	jsonval[_T("user")] = GetKBServerUsername();
	jsonval[_T("type")] = kbType;

	// convert it to string form
	wxJSONWriter writer; wxString str;
	writer.Write(jsonval, str);
	// convert it to utf-8 stored in CBString
	strVal = ToUtf8(str);

	aUrl = GetKBServerURL() + slash + container + slash;
	charUrl = ToUtf8(aUrl);

	// prepare curl
	curl = curl_easy_init();

	if (curl)
	{
		// add headers
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "Accept: application/json");
		// set data
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)strVal);

		result = curl_easy_perform(curl);

		if (result) {
			printf("SendEntry() result code: %d\n", result);
			return result;
		}
		curl_easy_cleanup(curl);
	}

	return 0;
}

// Returns the entryID, or -1 if there was an error. filter defaults to onlyUndeletedPairs;
// this is a filtering flag, when it equals onlyUndeletedPairs the lookup only considers
// (pseudo) undeleted src/tgt pairs as candidates for a successful lookup; if
// onlyDeletePairs is the value, only (pseudo) deleted pairs are considered; finally,
// allPairs considers all possibilities and will return the index of any pair which
// matches, but this is unlikely to be helpful because no information regarding deletion
// state is returned.
int KbServer::LookupEntryID(wxString srcPhrase, wxString tgtPhrase, bool& bDeleted)
{
	int entryID = -1;
	bDeleted = 0;
	int curlcode = LookupEntryForSourcePhrase(srcPhrase);
	if (curlcode == CURLE_OK)
	{
		if (!str_CURLbuffer.empty())
		{
			wxString myList = wxString::FromUTF8(str_CURLbuffer.c_str());
			wxJSONValue jsonval;
			wxJSONReader reader;
			int numErrors = reader.Parse(myList, &jsonval);
			if (numErrors > 0)
			{
				wxMessageBox(_T("LookupEntryID(): reader.Parse() returned errors, so will return wxNOT_FOUND"),
					_T("kbserver error"), wxICON_ERROR | wxOK);
				return -1;
			}
			int listSize = jsonval.Size();
			int index;
			for (index = 0; index < listSize; index++)
			{
				// we need to get the deleted flag's value
				entryID = jsonval[index][_T("id")].AsInt();
				int deletedFlag = jsonval[index][_T("deleted")].AsInt();
				bDeleted = deletedFlag == 1 ? TRUE : FALSE;
				wxString aTgtPhrase = jsonval[index][_T("target")].AsString();
				if (aTgtPhrase == tgtPhrase)
				{
					return entryID;
				}
			}
		}
	}
	// not found
	return -1;
}

int KbServer::PseudoDeleteEntry(wxString srcPhrase, wxString tgtPhrase)
{
	int entryID = wxNOT_FOUND; // -1
	bool bDeleted =FALSE; // initialize variable
	entryID = LookupEntryID(srcPhrase, tgtPhrase, bDeleted);
	// if it is already deleted, just return
	if (bDeleted)
	{
		return CURLE_OK;
	}
	wxString entryIDStr;
	wxItoa(entryID, entryIDStr);
	CURL *curl;
	CURLcode result; // result code
	struct curl_slist* headers = NULL;
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString kbType;
	wxItoa(GetKBServerType(),kbType);
	wxJSONValue jsonval; // construct JSON object
	CBString strVal; // to store wxString form of the jsonval object, for curl
	wxString container = _T("entry");
	wxString aUrl, aPwd;

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	// populate the JSON object
	jsonval[_T("deleted")] = 1;

	// convert it to string form
	wxJSONWriter writer; wxString str;
	writer.Write(jsonval, str);
	// convert it to utf-8 stored in CBString
	strVal = ToUtf8(str);

	aUrl = GetKBServerURL() + slash + container + slash + entryIDStr;
	charUrl = ToUtf8(aUrl);

	// prepare curl
	curl = curl_easy_init();

	if (curl)
	{
		// add headers
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "Accept: application/json");
		// set data
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT"); // this way avoids turning on file processing
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)strVal);

		result = curl_easy_perform(curl);

		curl_slist_free_all(headers);
		if (result) {
			printf("PseudoDeleteEntry() result code: %d\n", result);
			return result;
		}
		curl_easy_cleanup(curl);
	}
	return 0;
}

int KbServer::LookupEntryField(wxString source, wxString target, wxString& field)
{
	int result = 0;


	return result;
}

//=============================== end of KbServer class ============================

#endif // for _KBSERVER


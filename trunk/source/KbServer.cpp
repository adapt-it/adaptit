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
const size_t npos = (size_t)-1; // largest possible value, for use with std:find()

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

KbServer::KbServer(int whichType)
{
	wxASSERT(whichType == 1 || whichType == 2);
	m_kbServerType = whichType;
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

// the public setters & interrogatives

void KbServer::EnableKBSharing(bool bEnable)
{
	m_bEnableKBSharing = bEnable;
}

bool KbServer::IsKBSharingEnabled()
{
	return m_bEnableKBSharing;
}

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

void KbServer::UpdateLastSyncTimestamp()
{
	m_kbServerLastSync.Empty();
	m_kbServerLastSync = m_kbServerLastTimestampReceived;
	m_kbServerLastTimestampReceived.Empty();
	ExportLastSyncTimestamp(); // ignore the returned boolean (if FALSE, a message will have been seen)
}

// This function MUST be called, for any GET operation, before the material in
// str_CURLbuffer is parsed as a JSON string; failure to do so will leave the headers data
// in the string, and cause the json parse to fail.
// Returns the input string, s, after timestamp extraction, as just the json data material
// at the end of the received data stream from the GET request
std::string KbServer::ExtractTimestampThenRemoveHeaders(std::string s, wxString& timestamp)
{
	// make the standard string into a wxString
	timestamp.Empty(); // initialize
	CBString cbstr(str_CURLbuffer.c_str());
	wxString buffer(ToUtf16(cbstr));
	wxString srchStr = _T("X-MySQL-Date: ");
	int length = srchStr.Len();
	int offset = buffer.Find(srchStr);
	if (offset == wxNOT_FOUND)
	{
		s.clear(); // this should force a failure downstream
		return s;
	}
	else
	{
		// throw away everything preceding the matched string
		buffer = buffer.Mid(offset + length);
		// buffer now commences with the timestamp we want to extract - extract everything
		// up to the next '\r' or '\n' - whichever comes first
		int offset_to_CR = buffer.Find(_T('\r'));
		int offset_to_LF = buffer.Find(_T('\n'));
		offset = wxMin(offset_to_CR, offset_to_LF);
		if (offset > 0)
		{
			timestamp = buffer.Left(offset);

			// now we have extracted the timestamp, find where the json starts; we search for
			// "Content-Type: application/json", and then jump any following carriage
			// returns and or newlines, and then we are at the start of the json payload
			wxString cont_type = _T("Content-Type: application/json");
			offset = buffer.Find(cont_type);
			if (offset == wxNOT_FOUND)
			{
				s.clear(); // this should force a failure downstream
				timestamp.Empty();
				return s;
			}
			else
			{
				// remove what proceeds the matched string, up to its end
				length = cont_type.Len();
				buffer = buffer.Mid(offset + length);
				// now we've some line ends to skip over
				wxChar CR = _T('\r');
				wxChar LF = _T('\n');
				while (buffer[0] == CR || buffer[0] == LF)
				{
					buffer = buffer.Mid(1);
				}
				// okay, now the json material commences at the first wxChar of buffer
			}
		}
		else
		{
			s.clear(); // this should force a failure downstream
			return s;
		}
	}
	s = (char*)ToUtf8(buffer);
	return s;
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



// Get and return the timestamp value last stored in persistent (i.e. on disk) storage.
// Temporarily this storage is a file called lastsync.txt located in the project folder,
// but later something more permanent will be used (a hidden file in the project folder?)
// KbServer class has a private CBString member, m_kbServerLastSync to store the
// returned value. (May be called more than once in the life of a KbServer instance)

wxString KbServer::ImportLastSyncTimestamp()
{
	wxString dateTimeStr; dateTimeStr.Empty();

	wxString path = GetPathToPersistentDataStore() + GetPathSeparator() + GetLastSyncFilename();
	bool bLastSyncFileExists = ::wxFileExists(path);
	if (!bLastSyncFileExists)
	{
		// couldn't find lastsync.txt file in project folder
		wxString msg = _T("wxFileExists()called in ImportLastSyncTimestamp(): The wxTextFile, taking path to lastsync.txt, does not exist");
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
		wxString msg = _T("GetTextFileOpened()called in ImportLastSyncTimestamp(): The wxTextFile could not be opened");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return dateTimeStr; // it's empty still
	}
	size_t numLines = f.GetLineCount();
	if (numLines == 0)
	{
		// warn developer that the wxTextFile is empty
		wxString msg = _T("GetTextFileOpened()called in ImportLastSyncTimestamp(): The lastsync.txt file is empty");
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
bool KbServer::ExportLastSyncTimestamp()
{
	wxString path = GetPathToPersistentDataStore() + GetPathSeparator() + GetLastSyncFilename();
	bool bLastSyncFileExists = ::wxFileExists(path);
	if (!bLastSyncFileExists)
	{
		// couldn't find lastsync.txt file in project folder - tell developer
		wxString msg = _T("wxFileExists()called in ExportLastSyncTimestamp(): The wxTextFile, taking path to lastsync.txt, does not exist");
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
		wxString msg = _T("GetTextFileOpened()called in ExportLastSyncTimestamp(): The wxTextFile, taking path to lastsync.txt, could not be opened");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return FALSE;
	}
	f.Clear(); // chuck earlier value
	f.AddLine(GetKBServerLastSync());
	f.Close();

// *** TODO *** the permanent code -- alter the above

	return TRUE;
}

// accessors for the private arrays
wxArrayInt* KbServer::GetIDsArray()
{
	return &m_arrID;
}

wxArrayInt*	KbServer::GetDeletedArray()
{
	return &m_arrDeleted;
}

wxArrayString* KbServer::GetTimestampArray()
{
	return &m_arrTimestamp;
}

wxArrayString* KbServer::GetSourceArray()
{
	return &m_arrSource;
}

wxArrayString* KbServer::GetTargetArray()
{
	return &m_arrTarget;
}

wxArrayString* KbServer::GetUsernameArray()
{
	return &m_arrUsername;
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
// return the CURLcode value, downloaded JSON data is extracted and stored in the private
// arrays for that purpose, and other classes can obtain it using public getters which
// each return a pointer to a single one of the arrays
int KbServer::LookupEntriesForSourcePhrase( wxString wxStr_SourceEntry )
{
	str_CURLbuffer.clear(); // always make sure it is cleared for accepting new data

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

	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);
		//curl_easy_setopt(curl, CURLOPT_WRITEDATA, str_CURLbuffer); // <-- not needed
		//curl_easy_setopt(curl, CURLOPT_HEADER, 1L); // comment out when header info is
													  //not needed in the download (and below)

		result = curl_easy_perform(curl);

#if defined (_DEBUG) // && defined (__WXGTK__)
        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif

		if (result) {
			str_CURLbuffer.clear(); // always clear it before returning
			printf("LookupEntryForSourcePhrase() result code: %d Error: %s\n",
				result, curl_easy_strerror(result));
			curl_easy_cleanup(curl);
			return (int)result;
		}
	}
	curl_easy_cleanup(curl);

	// uncomment out only for "changed since" downloads, and also uncomment out the
	// CURLOPT_HEADER, 1L line above, so that header info is inserted in the data stream
	// Extract the timestamp, and remove the headers, leaving only the json data
	//str_CURLbuffer = ExtractTimestampThenRemoveHeaders(str_CURLbuffer, m_kbServerLastTimestampReceived);

#if defined (_DEBUG) //&& defined (__WXGTK__)
        //CBString ss(str_CURLbuffer.c_str());
        //wxString sshowit = ToUtf16(ss);
        //wxLogDebug(_T("Adjusted str_CURLbuffer: %s"), sshowit.c_str());
#endif

	//  make the json data accessible (result is CURLE_OK if control gets to here)
	if (!str_CURLbuffer.empty())
	{
        // Before extracting the substrings from the JSON data, the storage arrays must be
        // cleared with a call of ClearAllPrivateStorageArrays()
		ClearAllPrivateStorageArrays(); // always must start off empty

		wxString myList = wxString::FromUTF8(str_CURLbuffer.c_str());
		wxJSONValue jsonval;
		wxJSONReader reader;
		int numErrors = reader.Parse(myList, &jsonval);
		if (numErrors > 0)
		{
			// a non-localizable message will do, it's unlikely to ever be seen
			wxMessageBox(_T("LookupEntriesForSourcePhrase(): reader.Parse() returned errors, so will return wxNOT_FOUND"),
				_T("kbserver error"), wxICON_ERROR | wxOK);
			str_CURLbuffer.clear(); // always clear it before returning
			return -1;
		}
		int listSize = jsonval.Size();
		int index;
		for (index = 0; index < listSize; index++)
		{
            // We can extract id, source phrase, target phrase, deleted flag value,
            // username, and timestamp string; but for a lookup supporting a single
            // CSourcePhrase, only we need to extract source phrase, target phrase, and the
            // value of the deleted flag. So the others can be commented out.
			m_arrSource.Add(jsonval[index][_T("source")].AsString());
			m_arrTarget.Add(jsonval[index][_T("target")].AsString());
			m_arrDeleted.Add(jsonval[index][_T("deleted")].AsInt());
			//m_arrID.Add(jsonval[index][_T("id")].AsInt());
			//m_arrUsername.Add(jsonval[index][_T("user")].AsString());
			//m_arrTimestamp.Add(jsonval[index][_T("timestamp")].AsString());
		}
		str_CURLbuffer.clear(); // always clear it before returning
	}

	return 0;
}

// return the CURLcode value, downloaded JSON data is extracted and stored in the private
// arrays for that purpose, and other classes can obtain it using public getters which
// each return a pointer to a single one of the arrays
int KbServer::ChangedSince(wxString timeStamp)
{
	str_CURLbuffer.clear(); // always make sure it is cleared for accepting new data

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
	wxString changedSince = _T("/?changedsince=");

	aUrl = GetKBServerURL() + slash + container + slash+ GetSourceLanguageCode() + slash +
			GetTargetLanguageCode() + slash + kbType + changedSince + timeStamp;
	charUrl = ToUtf8(aUrl);
	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);
		// We want the download's timestamp, so we must ask for the headers to be added
		curl_easy_setopt(curl, CURLOPT_HEADER, 1L);

		result = curl_easy_perform(curl);

#if defined (_DEBUG) // && defined (__WXGTK__)
        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif

		if (result) {
			str_CURLbuffer.clear(); // always clear it before returning
			printf("LookupEntryForSourcePhrase() result code: %d Error: %s\n",
				result, curl_easy_strerror(result));
			curl_easy_cleanup(curl);
			return (int)result;
		}
	}
	curl_easy_cleanup(curl);


	//  make the json data accessible (result is CURLE_OK if control gets to here)
	if (!str_CURLbuffer.empty())
	{
        // Extract the timestamp, and remove the headers, leaving only the json data in
        // str_CURLbuffer, ready for subfield extraction to take place in the loop below
		str_CURLbuffer = ExtractTimestampThenRemoveHeaders(str_CURLbuffer, m_kbServerLastTimestampReceived);

		// If eyeball verification of the removal of preceding headers information is
		// wanted, then uncomment out the three lines in the conditional compile here
		#if defined (_DEBUG)
			//CBString ss(str_CURLbuffer.c_str());
			//wxString sshowit = ToUtf16(ss);
			//wxLogDebug(_T("Adjusted str_CURLbuffer: %s"), sshowit.c_str());
		#endif
        // Before extracting the substrings from the JSON data, the storage arrays must be
        // cleared with a call of ClearAllPrivateStorageArrays()
		ClearAllPrivateStorageArrays(); // always must start off empty

		wxString myList = wxString::FromUTF8(str_CURLbuffer.c_str());
		wxJSONValue jsonval;
		wxJSONReader reader;
		int numErrors = reader.Parse(myList, &jsonval);
		if (numErrors > 0)
		{
			// a non-localizable message will do, it's unlikely to ever be seen
			wxMessageBox(_T("ChangedSince(): reader.Parse() returned errors, so will return wxNOT_FOUND"),
				_T("kbserver error"), wxICON_ERROR | wxOK);
			str_CURLbuffer.clear(); // always clear it before returning
			return -1;
		}
		int listSize = jsonval.Size();
#if defined (_DEBUG)
		// get feedback about now many entries we got
		if (listSize > 0)
		{
			wxLogDebug(_T("ChangedSince() returned %d entries, for data added to kbserver since %s"),
				listSize, timeStamp);
		}
		else
		{
			wxLogDebug(_T("ChangedSince() did not return any entries, for data added to kbserver since %s"),
				timeStamp);
		}
#endif
		int index;
		for (index = 0; index < listSize; index++)
		{
            // We can extract id, source phrase, target phrase, deleted flag value,
            // username, and timestamp string; but for supporting the sync of a local KB we
            // need only to extract source phrase, target phrase, and the value of the
            // deleted flag. So the others can be commented out.
			m_arrSource.Add(jsonval[index][_T("source")].AsString());
			m_arrTarget.Add(jsonval[index][_T("target")].AsString());
			m_arrDeleted.Add(jsonval[index][_T("deleted")].AsInt());
			//m_arrID.Add(jsonval[index][_T("id")].AsInt());
			//m_arrUsername.Add(jsonval[index][_T("user")].AsString());
			//m_arrTimestamp.Add(jsonval[index][_T("timestamp")].AsString());
#if defined (_DEBUG)
			// list what entries were returned
			wxLogDebug(_T("Downloaded:  %s  ,  %s  ,  deleted = %d"),
				(m_arrSource[index]).c_str(), (m_arrTarget[index]).c_str(), (m_arrDeleted[index]));
#endif
		}
		str_CURLbuffer.clear(); // always clear it before returning
	}

	return 0;
}

void KbServer::ClearStrCURLbuffer()
{
	str_CURLbuffer.clear();
}

void KbServer::ClearAllPrivateStorageArrays()
{
	m_arrID.clear();
	m_arrDeleted.clear();
	m_arrSource.clear();
	m_arrTarget.clear();
	m_arrUsername.clear();
	m_arrTimestamp.clear();
}

void KbServer::ClearOneIntArray(wxArrayInt* pArray)
{
	pArray->clear();
}

void KbServer::ClearOneStringArray(wxArrayString* pArray)
{
	pArray->clear();
}

// Note: before running LookupEntryFields(), ClearStrCURLbuffer() should be called,
// also the storage arrays should be cleared with a call of ClearAllPrivateStorageArrays()
// and always remember to clear str_CURLbuffer before returning.
// Note 2: don't rely on CURLE_OK not being returned for a lookup failure, CURLE_OK will
// be returned even when there is no entry in the database. It's the returned ascii
// string, "No matching entry found" put into str_CURLbuffer which tells us that the entry
// is not in the kbserver database, so use that fact
int KbServer::LookupEntryFields(wxString sourcePhrase, wxString targetPhrase)
{

	CURL *curl;
	CURLcode result;
	wxString aUrl; // convert to utf8 when constructed
	wxString aPwd; // ditto
	const char notPresentStr[] = "No matching entry found";
	ClearAllPrivateStorageArrays(); // always must start off empty

	CBString charUrl;
	CBString charUserpwd;

	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString kbType;
	wxItoa(GetKBServerType(),kbType);
	wxString container = _T("entry");

	aUrl = GetKBServerURL() + slash + container + slash+ GetSourceLanguageCode() +
			slash + GetTargetLanguageCode() + slash + kbType + slash + sourcePhrase +
			slash + targetPhrase;
	charUrl = ToUtf8(aUrl);
	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);
		//curl_easy_setopt(curl, CURLOPT_HEADER, 1L); // comment out when header info is
													  //not needed in the download
		result = curl_easy_perform(curl);

#if defined (_DEBUG) //&& defined (__WXGTK__)
        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("Returned to str_CURLbuffer: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif

		if (result) {
			str_CURLbuffer.clear(); // always clear it before returning
			printf("LookupEntryFields() result code: %d, cURL Error: %s\n",
				result, curl_easy_strerror(result));
			curl_easy_cleanup(curl);
			return (int)result;
		}
	}
	curl_easy_cleanup(curl);

	// Find out if there is no entry, if so, clear the buffer and return 1 to indicate
	// failure to find the entry (the find() will work whether or not headers precede the
	// search string)
	if (str_CURLbuffer.find(notPresentStr) != npos)
	{
		// found the notPresentstr, so the looked up pair is not in the database
		str_CURLbuffer.clear();
		return 1;
	}

	// uncomment out only for "changed since" downloads, and also uncomment out the
	// CURLOPT_HEADER, 1L line above, so that header info is inserted in the data stream
	// Extract the timestamp, and remove the headers, leaving only the json data
	//str_CURLbuffer = ExtractTimestampThenRemoveHeaders(str_CURLbuffer, m_kbServerLastTimestampReceived);

#if defined (_DEBUG) //&& defined (__WXGTK__)
        //CBString ss(str_CURLbuffer.c_str());
        //wxString sshowit = ToUtf16(ss);
        //wxLogDebug(_T("Adjusted str_CURLbuffer: %s"), sshowit.c_str());
#endif
	// If there was no matching entry in the database, "No matching entry found" is
	// returned, so test for this

	//  make the json data accessible (result is CURLE_OK if control gets to here)
	if (!str_CURLbuffer.empty())
	{
		// Note: normally before calling LookupEntryForSrcTgtPair() the storage arrays
		// should be cleared with a call of ClearAllPrivateStorageArrays()
		wxString myObject = wxString::FromUTF8(str_CURLbuffer.c_str());
		wxJSONValue jsonval;
		wxJSONReader reader;
		int numErrors = reader.Parse(myObject, &jsonval);
		if (numErrors > 0)
		{
			// a non-localizable message will do, it's unlikely to ever be seen
			wxMessageBox(_T("LookupEntryForSrcTgtPair(): reader.Parse() returned errors, so will return wxNOT_FOUND"),
				_T("kbserver error"), wxICON_ERROR | wxOK);
			str_CURLbuffer.clear(); // always clear it before returning
			return -1;
		}
		// we extract id, source phrase, target phrase, deleted flag value, username,
		// and timestamp string; for index value 0 only (there should only be one json
		// object to deal with)
		m_arrID.Add(jsonval[_T("id")].AsInt());
		m_arrDeleted.Add(jsonval[_T("deleted")].AsInt());
		m_arrSource.Add(jsonval[_T("source")].AsString());
		m_arrTarget.Add(jsonval[_T("target")].AsString());
		m_arrUsername.Add(jsonval[_T("user")].AsString());
		m_arrTimestamp.Add(jsonval[_T("timestamp")].AsString());
		str_CURLbuffer.clear(); // always clear it before returning
	}

	return 0;
}

int KbServer::CreateEntry(wxString srcPhrase, wxString tgtPhrase, bool bDeletedFlag) // was SendEntry()
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
	jsonval[_T("deleted")] = bDeletedFlag ? 1 : 0;

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

		curl_slist_free_all(headers);
		if (result) {
			printf("CreateEntry() result code: %d Error: %s\n",
				result, curl_easy_strerror(result));
			curl_easy_cleanup(curl);
			return result;
		}
	}
	curl_easy_cleanup(curl);

	return 0;
}

// Return 0 (CURLE_OK) if no error, a CURLcode error code if there was an error
int KbServer::PseudoDeleteOrUndeleteEntry(int entryID, enum DeleteOrUndeleteEnum op)
{
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
	switch ( op )
	{
	case doDelete:
	default:
		jsonval[_T("deleted")] = 1;
		break;
	case doUndelete:
		jsonval[_T("deleted")] = 0;
		break;
	}

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
		headers = curl_slist_append(headers, "Accept: application/json");
		headers = curl_slist_append(headers, "Content-Type: application/json");
		// set data
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT"); // this way avoids turning on file processing
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)strVal);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		result = curl_easy_perform(curl);

		curl_slist_free_all(headers);
		if (result) {
			printf("PseudoDeleteOrUndeleteEntry() result code: %d\n", result);
			curl_easy_cleanup(curl);
			return result;
		}
	}
	curl_easy_cleanup(curl);
	return 0;
}



//=============================== end of KbServer class ============================

#endif // for _KBSERVER

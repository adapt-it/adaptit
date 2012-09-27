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
    #pragma implementation "TargetUnit.h"
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

#include "Adapt_It.h"
#include "TargetUnit.h"
#include "AdaptitConstants.h"
#include "RefString.h"
#include "RefStringMetadata.h"
#include "helpers.h"

extern bool		gbIsGlossing;

#if defined(_KBSERVER)

// Call SetupForKBServer() when opening a project which has been designated as associating
// with a kbserver (ie. m_bKBServerProject is TRUE), or when the user, in the GUI,
// designates the current project as being a kb sharing one. Return TRUE for a successful
// setup, FALSE if something was not right and in that case don't perform a setup.
bool CAdapt_ItApp::SetupForKBServer()
{
	int aType = GetKBTypeForServer();
	if (aType == -1)
	{
		return FALSE;
	}
	m_kbTypeForServer = aType; // the value is good, either 1 or 2

	// Get the last sync datetime from where it is stored
	if (m_kbServerLastSync.IsEmpty())
	{
        // no kbserver exists before this date (my 65th birthday), so it will always result
        // in an all-records download once the connection is established
		m_kbServerLastSync = _T("2012-05-22 00:00:00"); 
	}
	else
	{
		// once there is a value in m_kbServerLastSync wxString, we'll get the latest and
		// put it where the kbserver client API functions can grab it
		m_kbServerLastSync = GetLastSyncDateTime();
		if (m_kbServerLastSync.IsEmpty())
		{
			// there was an error tying to get the lastsync datetime, a message has been
			// seen already
			return FALSE;
		}
		// if control gets to here, what's in m_kbServerLastSync should be a valid datetime
	}

	// Get the url, username, and (for the development code only) password credentials;
	// the function call empties the credentials strings, and resets them from the
	// credentials.txt file; if there is any error, then all three are url, username, and
	// password are returned empty ( and a message to the developer will have been seen)
	bool bGotCredentials = GetCredentials(m_kbServerURL, m_kbServerUsername, m_kbServerPassword);
	if (!bGotCredentials)
	{
		return FALSE;
	}


	// Test that m_sourceLanguageCodeand m_targetLanguageCode contain values - we will
	// assume that they are correct if non-empty, for the present (later, we need to
	// validate them against the iso639 codes). If one or both are empty, instruct user to
	// use Preferences, Backups and Misc tab, to set a code for each.
	// User could see the error message, so make it localizable.
	if (m_sourceLanguageCode.IsEmpty() || m_targetLanguageCode.IsEmpty())
	{
		// warn, and return FALSE
		
		wxString msg = _("Either the language code for the source language, or for target language, or both, is empty.\nYou must set a valid code for both in order for sharing of your KB to take place.\n The Backups and Misc page of Preferences allows you to set up the needed codes. Do so now, and then try again.");
		LogUserAction(msg);
		wxMessageBox(msg, _("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return FALSE;
	}

    // A code for m_glossesLanguageCode is unnecessary if KB sharing is not required for
    // glossing mode, or if glossing mode is not ever going to be used. It would be
    // annoying to warn user that kbserver support won't work in glossing mode if no code
    // is set, at every entry to the project. So we'll not do any check here - but we will
    // check here if glossing mode is currently on when SetupForKBServer() is called.
    // Otherwise, such a test can wait until the user actually enters glossing mode in a kb
    // sharing project and the code finds that m_glossesLanguageCode is empty.
	if (gbIsGlossing)
	{
		// glossing mode is ON, so assume sharing of glossing data is wanted - test a code
		// is set, warn if not and exit FALSE
		if (m_glossesLanguageCode.IsEmpty())
		{
			// warn, and return FALSE
			wxString msg = 
_("Glossing mode is currently turned on, so the glossing KB is currently the active KB.\nKB server support will not work for the glossing KB if a valid language code is not defined, which is the case at present.\n The Backups and Misc page of Preferences allows you to set up the needed code. Do so now, and then try again.");
			LogUserAction(msg);
			wxMessageBox(msg, _("Error in support for kbserver"), wxICON_ERROR | wxOK);
			return FALSE;
		}
	}
	return TRUE;
}

bool CAdapt_ItApp::ReleaseKBServer()
{
	// the only task at present is to make sure that the datetime value in
	// m_kbServerLastSync is written to to persistent storage. That may be to a hidden
	// file later on, but for new we overwrite the single line in lastsync.txt stored in
	// the project folder
	wxString dateTimeStr; dateTimeStr.Empty();

	// temporary code starts
	wxString filename = _T("lastsync.txt");
	wxString path = m_curProjectPath + PathSeparator + filename;
	bool bLastSyncFileExists = ::wxFileExists(path);
	if (!bLastSyncFileExists)
	{
		// couldn't find lastsync.txt file in project folder - tell developer
		wxString msg = _T("wxFileExists()called in ReleaseKBServer(): The wxTextFile, taking path to lastsync.txt, does not exist");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return FALSE;
	}
	wxTextFile f;
	bool bSuccess = FALSE;
	// for 2.9.4 builds, the conditional compile isn't needed I think, and for Linux and
	// Mac builds which are Unicode only, it isn't needed either but I'll keep it for now
	bSuccess = GetTextFileOpened(&f, path);
	if (!bSuccess)
	{
		// warn developer that the wxTextFile could not be opened
		wxString msg = _T("GetTextFileOpened()called in ReleaseKBServer(): The wxTextFile, taking path to lastsync.txt, could not be opened");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return FALSE;
	}
	f.Clear(); // chuck earlier value
	f.AddLine(m_kbServerLastSync);
	f.Close();
	// temporary code ends
	

// *** TODO *** the permanent GUI code, and then comment out the above

	return TRUE;
}


// Ff the parent function is misplaced, preceding LoadKB() and LoadGlossingKB() calls,
// then return -1 plus give the developer a message to fix this; otherwise return 1 for an
// adapting KB, 2 for a glossing KB.
int CAdapt_ItApp::GetKBTypeForServer()
{
	int type = 1; // default is to assume adapting KB is wanted

	// the two KBs must have been successfully loaded
	if (m_bKBReady && m_bGlossingKBReady)
	{
		if (gbIsGlossing)
		{
			type = 2;
		}
	}
	else
	{
		// warn developer that the logic is wrong
		wxString msg = _T("GetKBTypeForServer()called in SetupForKBServer(): Logic error, m_bKBReady and m_bGlossingKBReady are not both TRUE yet. Fix this!");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return -1;
	}
	return type;
}

// Return the dateTime value last returned from the kbserver and stored at the client;
// temporarily this storage is a file called lastsync.txt located in the project folder,
// but later something more permanent will be used (a hidden file for such metadata?)
wxString CAdapt_ItApp::GetLastSyncDateTime()
{
	wxString dateTimeStr; dateTimeStr.Empty();
	// temporary code starts
	wxString filename = _T("lastsync.txt");
	wxString path = m_curProjectPath + PathSeparator + filename;
	bool bLastSyncFileExists = ::wxFileExists(path);
	if (!bLastSyncFileExists)
	{
		// couldn't find lastsync.txt file in project folder
		wxString msg = _T("wxFileExists()called in SetupForKBServer(): The wxTextFile, taking path to lastsync.txt, does not exist");
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
		wxString msg = _T("GetTextFileOpened()called in SetupForKBServer(): The wxTextFile, taking path to lastsync.txt, could not be opened");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return dateTimeStr; // it's empty still
	}
	size_t numLines = f.GetLineCount();
	if (numLines == 0)
	{
		// warn developer that the wxTextFile is empty
		wxString msg = _T("GetTextFileOpened()called in SetupForKBServer(): The lastsync.txt file is empty");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		f.Close();
		return dateTimeStr; // it's empty still
	}
	dateTimeStr = f.GetLine(0); // whew, finally, we have the lastsync datetime string
	f.Close();
	// end temporary code
	
// **** TODO ****  - the permanent GUI implementation for this goes here - comment out the above

	return dateTimeStr;
}

// Return TRUE, and the url and username credentials (and while doing testing, also the
// kbserver password) to be stored in the app class. Temporarily this data is storedin a
// file called credentials.txt located in the project folder and contains url, username,
// password, one string per line. But later something more permanent will be used, and the
// kbserver password will never be stored in the app once a releasable version of kbserver
// support has been built (use a hidden file for url and username?)
bool CAdapt_ItApp::GetCredentials(wxString& url, wxString& username, wxString& password)
{
	bool bSuccess = FALSE;
	url.Empty(); username.Empty(); password.Empty();

	// temporary code starts
	wxString filename = _T("credentials.txt");
	wxString path = m_curProjectPath + PathSeparator + filename;
	bool bCredentialsFileExists = ::wxFileExists(path);
	if (!bCredentialsFileExists)
	{
		// couldn't find credentials.txt file in project folder
		wxString msg = _T("wxFileExists() called in SetupForKBServer(): The credentials.txt file does not exist");
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
		wxString msg = _T("GetTextFileOpened()called in SetupForKBServer(): The wxTextFile, taking path to credentials.txt, could not be opened");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return FALSE; // signature params are empty still
	}
	size_t numLines = f.GetLineCount();
	if (numLines < 3)
	{
		// warn developer that the wxTextFile lackes expected 3 lines: url, username, password
		wxString msg;
		msg = msg.Format(_T("GetTextFileOpened()called in SetupForKBServer(): The credentials.txt file lacks one or more lines, it has %d of expected 3 (url,username,password)"),
			numLines);
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		f.Close();
		return FALSE; // signature params are empty still
	}
	url = f.GetLine(0);
	username = f.GetLine(1);
	password = f.GetLine(2);
	wxLogDebug(_T("GetCredentials(): url = %s  ,  username = %s , password = %s"), url, username, password);
	f.Close();
	// end temporary code
	
// **** TODO ****  - the permanent GUI implementation for this goes here - comment out the above

	return TRUE;
}

// Return FALSE if pf not successfully opened, in which case the parent does not need to
// close pf; otherwise, return TRUE for a successful open & parent must close it when done
bool CAdapt_ItApp::GetTextFileOpened(wxTextFile* pf, wxString& path)
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




// more functions...


#endif // for _KBSERVER


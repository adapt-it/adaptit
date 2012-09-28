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
#include "KB.h"
#include "AdaptitConstants.h"
#include "RefString.h"
#include "RefStringMetadata.h"
#include "helpers.h"

extern bool		gbIsGlossing;

/// A Note on the placement of SetupForKBServer(), and the protective wrapping boolean
/// m_bIsKBServerProject, defaulting to FALSE initially and then possibly set to TRUE by
/// reading the project configuration file.
/// Since the KBs must be loaded before SetupForKBServer() is called, and the function for
/// setting them up is CreateAndLoadKBs(), it may appear that at the end of the latter
/// would be an appropriate place, within the TRUE block of an if (m_bIsKBServerProject)
/// test. However, looking at which functionalities, at a higher level, call
/// CreateAndLoadKBs(), many of these are too early for any association of a project with
/// a kbserver to have been set up already. Therefore, possibly after a read of the
/// project configuration file may be appropriate -
/// since it's that configuration file which sets or clears the m_bIsKBServerProject app
/// member boolean.This is so: there are 5 contexts where the project config file is read:
/// in OnOpenDocument() for a MRU open which bypasses the ProjectPage of the wizard; in
/// DoUnpackDocument(), in HookUpToExistingProject() for setting up a collaboration, in
/// the OpenExistingProjectDlg.cpp file, associated with the "Access an existing
/// adaptation project" feature (for transforming adaptations to glosses); and most
/// importantly, in the frequently called OnWizardPageChanging() function of
/// ProjectPage.cpp. These are all appropriate places for calling SetupForKBServer() late
/// in such functions, in an if TRUE test block using m_bIsKBServerProject. (There is no
/// need, however, to call it in OpenExistingProjectDlg() because the project being opened
/// is not changed in the process, and since it's adaptations are being transformed to
/// glosses, it would not be appropriate to assume that the being-constructed new project
/// should be, from it's inception, a kb sharing one. The user can later make it one if he
/// so desires.)

#if defined(_KBSERVER)

/// Call SetupForKBServer() when re-opening a project which has been designated earlier as
/// associating with a kbserver (ie. m_bKBServerProject is TRUE after the project's
/// configuration file has been read), or when the user, in the GUI, designates the current
/// project as being a kb sharing one. Return TRUE for a successful setup, FALSE if
/// something was not right and in that case don't perform a setup.
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
	// all's well
	return TRUE;
}

/// Return TRUE of the required codes are defined - there will be at least two required
/// (for source and target languages), and if bRequireGlossesLanguageCode is TRUE (it is
/// default FALSE) then a third must have been defined - the code for the glossing
/// language. Return FALSE if these conditions are not met.
bool CAdapt_ItApp::CheckForLanguageCodes(bool bRequireGlossesLanguageCode)
{
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
    // is set, at every entry to the project. So we will check hereonly when the param is TRUE.
	// The caller will need to test for whether or not kbserver support is wanted for the
	// glossing KB - a good test would be to check if the glossing KB has data, and if it
	// does, then pass in TRUE. (Use IsGlossingKBPopulated() to make the aforementioned test.)
	if (bRequireGlossesLanguageCode)
	{
		// assume sharing of glossing data is wanted - test if a language code for glosses
		// language is set, warn if not and exit FALSE
		if (m_glossesLanguageCode.IsEmpty())
		{
			// warn, and return FALSE
			wxString msg = 
_("The glosses knowledge base contains data, so a language code for that data should be defined.\nKB server support will not work for the glossing KB if a valid language code is not defined, which is the case at present.\n The Backups and Misc page of Preferences allows you to set up the needed code. Do so now, and then try again.");
			LogUserAction(msg);
			wxMessageBox(msg, _("Error in support for kbserver"), wxICON_ERROR | wxOK);
			return FALSE;
		}
	}
	return TRUE;
}

/// Return TRUE if the first map in the glossing KB has at least one entry, or if glossing
/// mode is currently turned on, FALSE otherwise. (The other nine maps are less likely to
/// have data, so testing the first is sufficient for the glossing KB.) Pass this
/// function's return value, when when TRUE is returned, as the argument to the call
/// CheckForLanguageCodes(bool bRequireGlossesLanguageCode), because the latter function
/// must check that a language code exists for the glossing KB before any kbserver API
/// functions try access the server when doing glossing, so as to block the access attempt
/// when the required parameters are not all defined.
bool CAdapt_ItApp::IsGlossingKBPopulatedOrGlossingModeON()
{
	if (gbIsGlossing)
	{
		return TRUE;
	}
	if (m_pGlossingKB == NULL)
	{
		// this is unlikely, so an Engish message will suffice to warn the developer to
		// fix the problem pronto - when in an AI project, m_pKB and m_pGlossingKB should
		// never be NULL
		wxString msg = _T("In IsGlossingKBPopulalted(), m_pGlossingKB is NULL for the current Adapt It project. Fix immediately, or doing glossing will crash the app.");
		wxMessageBox(msg, _T("Error in kbserver support"), wxICON_ERROR);
		return FALSE;
	}
	return m_pGlossingKB->m_pMap[0]->size() > 0; // map[0] always has most data, if it's empty, assume the rest are too
}

/// Return TRUE if there was no error, FALSE otherwise. The function is used for doing
/// cleanup, and any needed making of data persistent between adapting sessions within a
/// project which is a KB sharing project, when the user exits the project or Adapt It is
/// shut down.
bool CAdapt_ItApp::ReleaseKBServer()
{
	// the only task at present is to make sure that the datetime value in
	// m_kbServerLastSync is written to to persistent storage. That may be to a hidden
	// file later on, but for new we overwrite the single line in lastsync.txt stored in
	// the project folder
	bool bOK = StoreLastSyncDateTime();

	// now make sure that nonsense values are in the holding variables, so that any switch
	// to a different project doesn't carry with it valid kbserver parameters
	m_kbTypeForServer = -1; // only 1 or 2 are valid values
	m_bIsKBServerProject = FALSE;
	m_kbServerURL.Empty();
	m_kbServerUsername.Empty();
	m_kbServerPassword.Empty(); 
	m_kbServerLastSync.Empty();
	
// *** TODO *** more of the permanent GUI code if needed in this function

	// ************ NOTE NOTE NOTE *******************
	// If we find we need to do any last minute kbserver accesses to get any pending
	// uploads and/or downloads done before release (since EraseKB() is typically called
	// after ReleaseKBServer() returns, and EraseKB() doesn't update the KB on disk before
	// it does it's erasure of the in-memory copy of the KB), so we should do those final
	// things here and put code to SAVE THE KB which is active RIGHT HERE!
	return bOK;
}

// Prompt the user to type in the appropriate kbserver password, and return it. It will be
// stored in the app's m_kbServerPassword wxString member during the adapting session, and
// then that member is cleared by ReleaseKBServer() on exit from the project session or
// from the app.
wxString CAdapt_ItApp::GetKBServerPassword()
{
    // temporarily, just get it from m_kbServerPassword and return it so the caller can put
    // it back there. Later on, replace this code with a dialog for getting the password.
	wxString myPassword = m_kbServerPassword;
	return myPassword;
}

/// Takes the kbserver's datetime supplied with downloaded data, and stores it in the app
/// wxString member variable m_kbServerLastSync. Return TRUE if no error, FALSE otherwise.
bool CAdapt_ItApp::SetLastSyncDateTime(wxString datetime)
{
	// *** TODO *** add code when we get downloads from kbserver working

	datetime.Empty(); // do nothing useful, avoid compiler warning

	return TRUE;
}

/// Return TRUE if all went well, FALSE if unsuccessful (and show a message for developer);
/// use this to send the current datetime in m_kbServerLastSync member to permanent storage
/// on disk -- for our testing code, this will be the file lastsync.txt in the project
/// folder, but for a release version it will probably be to a hidden file in the same place
bool CAdapt_ItApp::StoreLastSyncDateTime()
{
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


/// If the parent function, SetupForKBServer(), is misplaced so that it is located
/// somewhere preceding LoadKB() and LoadGlossingKB() calls, then return -1 plus give the
/// developer a message to fix this; otherwise return 1 for an adapting KB, 2 for a
/// glossing KB. If the loads have not been done, m_pKB and m_pGlossingKB will be still
/// NULL, and m_bKBReady and m_bGlossingKBReady will both be FALSE - the latter two
/// conditions are how we test for bad placement.
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

/// Return the dateTime value last returned from the kbserver and stored in the client.
/// Temporarily this storage is a file called lastsync.txt located in the project folder,
/// but later something more permanent will be used (a hidden file in the project folder?)
/// The app class has a wxString member, m_kbServerLastSync to store the returned value.
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

/// Return TRUE, and the url and username credentials (and while doing testing, also the
/// kbserver password) to be stored in the app class. Temporarily this data is storedin a
/// file called credentials.txt located in the project folder and contains url, username,
/// password, one string per line. But later something more permanent will be used, and the
/// kbserver password will never be stored in the app once a releasable version of kbserver
/// support has been built (use a hidden file for url and username in the project folder?)
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


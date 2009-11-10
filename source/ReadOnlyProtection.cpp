/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ReadOnlyProtection.cpp
/// \author			Bruce Waters
/// \date_created	10 November 2009
/// \date_revised	
/// \copyright		2009 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public 
///                 License (see license directory)
/// \description	Implementation file for read-only protection of a user
///					working in a project folder, and someone accesses that
///					folder remotely (such as an Administrator using the
///					Custom Work Folder Location command), and tries to do somee
///					work there which would, if there were no protection, potentially
///					lead to the user unknowingly losing data from either a document
///					or the KB, or both.
/// \derivation		The ReadOnlyProtection class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ReadOnlyProtection.h"
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
#include <wx/dir.h>

#include "Adapt_It.h"
#include "ReadOnlyProtection.h"

// Define type safe pointer lists
//#include "wx/listimpl.cpp"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(ReadOnlyProtection, wxObject)

ReadOnlyProtection::ReadOnlyProtection()
{
	m_pApp = &wxGetApp();
}

ReadOnlyProtection::ReadOnlyProtection(CAdapt_ItApp* pApp)  // use this one, it sets m_pLayout
{
	m_pApp = pApp;
}


ReadOnlyProtection::~ReadOnlyProtection()
{

}

// implementation

void ReadOnlyProtection::Initialize()
{
	m_strAIROP_Prefix = _T("~AIROP"); // first part of the filename
	m_strLock_Suffix = _T(".lock"); // the suffix to add to the filename's end

	// obtain the host machine's name, and the current user's id (ie. the name used
	// in path specifications);  set defaults if either or both can't be determined
	m_strLocalUsername = GetLocalUsername();
	if (m_strLocalUsername.IsEmpty())
	{
		m_strLocalUsername = _T("UnknownUser");
	}
	m_strLocalMachinename = GetLocalMachinename();
	if (m_strLocalMachinename.IsEmpty())
	{
		m_strLocalMachinename = _T("UnknownMachine");
	}
	// set up the filename for read only protection, which is for this particular user
	// and host machine; it remains unchanged for the session
	m_strReadOnlyProtectionFilename = MakeReadOnlyProtectionFilename(m_strAIROP_Prefix,
					m_strLock_Suffix, m_strLocalMachinename, m_strLocalUsername);

	// at launch, this one should be empty
	m_strTheOtherReadOnlyProtectionFilename.Empty();
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the current user id, or an empty string if it cannot be determined
/// \remarks	Obtain from wxWidgets "Network, user and OS functions" calls; if empty string
///				returned, the caller should set up a default string such as "UnknownUser"
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::GetLocalUsername()
{
	wxString theName = ::wxGetUserId(); // returns empty string if not found
	return theName;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the host machine's name, or an empty string if it cannot be determined
/// \remarks	Obtain from wxWidgets "Network, user and OS functions" calls; if empty string
///				returned, the caller should set up a default string such as "UnknownMachine"
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::GetLocalMachinename()
{
	// get's the host machine's name (ignores domain name)
	wxString theMachine = ::wxGetHostName(); // returns empty string if not found
	return theMachine;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the username part of "~AIROP-machinename-username.lock"
///	\param		strFilename		->	the read-only protect filename that was found
/// \remarks	extract the username string from the filename; pass filename by value so
///				we can play with the string internally with impunity
///				Internally accesses the CAdapt_ItApp class's wxString member m_strLock_Suffix
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::ExtractUsername(wxString strFilename)
{
	wxString theName = _T(""); // not localizable
	int offset = strFilename.Find(_T("-")); // first hyphen, machinename follows it
	wxASSERT(offset > 5);
	theName = strFilename.Mid(offset + 1);
	offset = theName.Find(_T("-")); // second hyphen, username follows it
	wxASSERT(offset > 0);
	theName = theName.Mid(offset + 1); // theName now starts with username
	offset = theName.Find(m_strLock_Suffix); // finds the ".lock" string at end of theName
	wxASSERT(offset > 0);
	theName = theName.Left(offset);
	return theName;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the machinename part of "~AIROP-machinename-username.lock"
///	\param		strFilename		->	the read-only protect filename that was found
/// \remarks	extract the machinename string from the filename; pass filename by value so
///				we can play with the string internally with impunity
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::ExtractMachinename(wxString strFilename)
{
	wxString theMachine = _T(""); // not localizable
	int offset = strFilename.Find(_T("-")); // first hyphen, machinename follows it
	wxASSERT(offset > 5);
	theMachine = strFilename.Mid(offset + 1);
	offset = theMachine.Find(_T("-")); // second hyphen, username follows it
	wxASSERT(offset > 0);
	theMachine = theMachine.Left(offset);
	return theMachine;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the composed filename, of form "~AIROP-machinename-username.lock"
///	\param		prefix		->	"~AIROP" always
/// \param		suffix		->	".lock" always
///	\param		machinename	->	local computer's name
/// \param		username	->	local user's name
/// \remarks	compose the read-only protection filename, for the local machine & user
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::MakeReadOnlyProtectionFilename(
					const wxString prefix, // pass in m_strAIROP_Prefix
					const wxString suffix, // pass in m_strLock_Suffix
					const wxString machinename,
					const wxString username)
{
	wxString str = prefix;
	str += _T("-") + machinename;
	str += _T("-") + username;
	return str += suffix;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		return TRUE if users or machines or both don't match; return FALSE when both
///				users and machines are the same (that is, FALSE means the running instance
///				making the test has ownership of the project folder already, and so writing of
///				KB and documents should not be prevented, also return FALSE if noone currently
///				owns the folder being checked
///	\param		localMachine	->	local computer's name
/// \param		localUser		->	local user's name
///	\param		theOtherMachine	->	target folder's computer's name (can be local machine)
/// \param		theOtherUser	->	target folder's user's name (can be the local user)
/// \remarks	Test to determine whether or not the target folder is owned currently, and if
///				it is, then who owns it - whether myself, or someone else. Returning a FALSe
///				value tells the caller that I can safely take possession of the target folder
///				for writing
///////////////////////////////////////////////////////////////////////////////////////////
bool ReadOnlyProtection::IsDifferentUserOrMachine(wxString& localMachine, wxString& localUser,
											wxString& theOtherMachine, wxString& theOtherUser)
{
	// first check for nobody currently owning the folder - if that is the case
	// then return FALSE so that I can become its owner
	if (theOtherMachine.IsEmpty() || theOtherUser.IsEmpty())
		return FALSE;
	// if we get here, someone is the owner of the target folder, so find out who
	if (localMachine != theOtherMachine)
		return TRUE;
	if (localUser != theOtherUser)
		return TRUE;
	// if we get here, each is a match for the other, so I'm already the owner of the
	// target folder, so return FALSE
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the absolute path to the read-only protection file in the passed in folder,
///				if it exists in the folder; otherwise, an empty string
///	\param		projectFolderPath ->	absolute path to the project folder being checked
/// \remarks	Search for a matching file in the project folder. wxRegEx class is not safe
///				to use because regular expression support for Unicode may not work - one system
///				does not support matching across character block boundaries, another (eg. VIM)
///				allows this but limits matches to 128 (enough for our purposes but the wx 
///				documentation doesn't indicate if this is supported). So we don't use regular
///				expressions to obtain a match; instead, we search for the prefix "~AIROP" and
///				the suffix ".lock" and the presence of two hyphens. That much extremely unlikely
///				to occur by accident in an arbitrary filename from another unrelated source.
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::GetReadOnlyProtectionFileInProjectFolder(wxString& projectFolderPath)
{
	wxString theFilename = _T("");
	// test to make sure we are not trying to look at a file rather than a folder

	bool bIsReallyAFile = ::wxFileExists(projectFolderPath);
	if (bIsReallyAFile)
	{
		// this message should be localizable; we allow processing to continue, returning an 
		// empty string (the user should then retry to access a directory)
		wxString mssg;
		mssg.Format(_("The project folder being tested, %s, is really a file. Adapt It will continue running, but you should next try to properly locate a project folder."),
				projectFolderPath);
		wxMessageBox(mssg,_("Warning: Not a folder!"), wxICON_WARNING);
		return theFilename;
	}
	bool bDirectoryExists = wxDir::Exists(projectFolderPath); // a static function
	if (bDirectoryExists)
	{
		wxDir dir(projectFolderPath);
		bool bIsOpened = dir.IsOpened();
		if (!bIsOpened)
		{
			// it didn't open, this is unexpected and probably is something for the developer
			// to fix, so warn and abort
			wxString mssg;
			mssg.Format(_T("GetReadOnlyProtectionFileInProjectFolder(): the directory %s failed to open for enumeration. Now aborting."),
				projectFolderPath);
			wxMessageBox(mssg,_T("wxDir Error: Could not open directory"), wxICON_ERROR);
			wxExit();
			return theFilename;
		}
		else
		{
			// it was opened okay; so return an empty string if there are no files within,
			// otherwise find (using wildcard *) the one and only read-only projection
			// file - if it exists; if found, return it to caller, it none is found, return
			// the empty string
			bool bHasFiles = FALSE;
			// make the wildcarded file spec
			wxString theFileSpec = m_strAIROP_Prefix;
			theFileSpec += _T("-*");
			theFileSpec += m_strLock_Suffix;
			bHasFiles = dir.HasFiles(theFileSpec); // try match "~AIROP-*.lock"
			if (bHasFiles)
			{
				// a file matching the specification is present, so get it & return its
				// filename to the caller
				bool bGotIt = dir.GetFirst(&theFilename, theFileSpec, wxDIR_FILES);
				if (!bGotIt)
				{
					// unexpectedly failed to get it, so tell developer and then abort
					wxString mssg;
					mssg.Format(_T("GetReadOnlyProtectionFileInProjectFolder(): the directory %s has the file ~AIROP-*.lock, but GetFirst() failed to get it. Now aborting."),
						projectFolderPath);
					wxMessageBox(mssg,_T("wxDir Error: GetFirst() failed"), wxICON_ERROR);
					wxExit();
					return theFilename;
				}
			}
			else
			{
				// no match, so tell the caller so (return an empty string)
				return theFilename; // it's still the empty string
			}
		}
	}
	else
	{
		// the directory should exist - warn user and call OnExit() to abort; only the 
		// developers should ever get to see this error so don't make it localizable
		wxString mssg;
		mssg.Format(_T("GetReadOnlyProtectionFileInProjectFolder(): the path, %s, to the passed in project folder was tested and found to not exist! Now aborting."),
			projectFolderPath);
		wxMessageBox(mssg,_T("wxDir Error: directory does not exist"), wxICON_ERROR);
		wxExit();
	}
	return theFilename;
}




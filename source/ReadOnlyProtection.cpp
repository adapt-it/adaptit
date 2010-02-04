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
#include <wx/filename.h>

#include "Adapt_It.h"
#include "helpers.h"
#include "ReadOnlyProtection.h"

// Define type safe pointer lists
//#include "wx/listimpl.cpp"

//#define _DEBUG_ROP // comment out when wxLogDebug calls no longer needed

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

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \remarks	Initialize the ReadOnlyProtection class. Specifically, for the local
/// running Adapt It instance, and local user, on the local machine, create a suitable
/// read-only protection file with .lock extension to be kept ready for use for as long as
/// this session lasts. Called from CAdapt_ItApp::OnInit() only. 
///////////////////////////////////////////////////////////////////////////////////////////
void ReadOnlyProtection::Initialize()
{
	m_strAIROP_Prefix = _T("~AIROP"); // first part of the filename
	m_strLock_Suffix = _T(".lock"); // the suffix to add to the filename's end

	// obtain the host machine's name, and the current user's id (ie. the name used
	// in path specifications);  set defaults if either or both can't be determined
	m_strLocalUsername = GetLocalUsername(); // hyphens have been filtered out
	if (m_strLocalUsername.IsEmpty())
	{
		m_strLocalUsername = _T("UnknownUser"); // not localizable, just need 'something'
	}
	m_strLocalMachinename = GetLocalMachinename(); // hyphens have been filtered out
	if (m_strLocalMachinename.IsEmpty())
	{
		m_strLocalMachinename = _T("UnknownMachine"); // ditto
	}
	m_strLocalProcessID = GetLocalProcessID();
	// set up the filename for read only protection, which is for this particular user
	// and host machine and process ID; it remains unchanged for the session
	m_strReadOnlyProtectionFilename = MakeReadOnlyProtectionFilename(m_strAIROP_Prefix,
		m_strLock_Suffix, m_strLocalMachinename, m_strLocalUsername, m_strLocalProcessID);

	// at launch, this one should be empty
	m_strOwningReadOnlyProtectionFilename.Empty();
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the current user id, or an empty string if it cannot be determined
/// \remarks	Obtain from wxWidgets "Network, user and OS functions" calls; if empty string
///				returned, the caller should set up a default string such as "UnknownUser"
/// BEW added 4Feb10: the filter function ChangeHyphensToUnderscores which ensures all machine
/// names and user names do not have any hyphens. (Then we can safely use hyphen for name
/// delimitation and safe parsing of the lock file's name.)
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::GetLocalUsername()
{
	wxString theName = ::wxGetUserId(); // returns empty string if not found
	if (!theName.IsEmpty())
	{
		// filter out of the name any hyphens
		theName = ChangeHyphensToUnderscores(theName);	
	}
	return theName;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the host machine's name, or an empty string if it cannot be determined
/// \remarks	Obtain from wxWidgets "Network, user and OS functions" calls; if empty string
///				returned, the caller should set up a default string such as "UnknownMachine"
/// BEW added 4Feb10: the filter function ChangeHyphensToUnderscores which ensures all machine
/// names and user names do not have any hyphens. (Then we can safely use hyphen for name
/// delimitation and safe parsing of the lock file's name.)
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::GetLocalMachinename()
{
	// get's the host machine's name (ignores domain name)
	wxString theMachine = ::wxGetHostName(); // returns empty string if not found
	if (!theMachine.IsEmpty())
	{
		// filter out of the name any hyphens
		theMachine = ChangeHyphensToUnderscores(theMachine);	
	}
	return theMachine;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the host machine's process ID for the running instance of Adapt It, or 
///             the string generated by converting 0xFFFF if there was an error
/// \remarks	Obtain from wxWidgets "Process control functions" calls
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::GetLocalProcessID()
{
	unsigned long pid = ::wxGetProcessId();
	if (pid == 0)
	{
		pid = 0xFFFF;
	}
	wxString pidValueStr = wxString::Format(_T("%i"),pid);
	return pidValueStr;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the username part of "~AIROP-machinename-username.lock"
///	\param		strFilename		->	the read-only protect filename that was found
/// \remarks	extract the username string from the filename; pass filename by value so
///				we can play with the string internally with impunity
///				Internally accesses the CAdapt_ItApp class's wxString member m_strLock_Suffix
///				BEW note, 4Feb10, this use of hyphen as a delimiter for parsing is safe
///				because hyphens are filtered out of machinename and username strings
///				before thsy are used to construct the lock file's name
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
	offset = theName.Find(_T("-")); // finds the next hyphen
	wxASSERT(offset > 0);
	theName = theName.Left(offset);
	return theName;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the processID part of "~AIROP-machinename-username-processID.lock"
///	\param		strFilename		->	the read-only protect filename that was found
/// \remarks	extract the process ID string from the filename; pass filename by value so
///				we can play with the string internally with impunity
///				Internally accesses the CAdapt_ItApp class's wxString member m_strLock_Suffix
///				BEW note, 4Feb10, this use of hyphen as a delimiter for parsing is safe
///				because hyphens are filtered out of machinename and username strings
///				before thsy are used to construct the lock file's name
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::ExtractProcessID(wxString strFilename)
{
	wxString theID = _T(""); // not localizable
	int offset = strFilename.Find(_T("-")); // first hyphen, machinename follows it
	wxASSERT(offset > 5);
	theID = strFilename.Mid(offset + 1);
	offset = theID.Find(_T("-")); // second hyphen, username follows it
	wxASSERT(offset > 0);
	theID = theID.Mid(offset + 1); // theID now starts with username
	offset = theID.Find(_T("-")); // third hyphen, processID follows it
	wxASSERT(offset > 0);
	theID = theID.Mid(offset + 1); // theID now starts with processID string
	offset = theID.Find(m_strLock_Suffix); // finds the ".lock" string at end of theName
	wxASSERT(offset > 0);
	theID = theID.Left(offset);
	return theID;
}
///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the machinename part of "~AIROP-machinename-username.lock"
///	\param		strFilename		->	the read-only protect filename that was found
/// \remarks	extract the machinename string from the filename; pass filename by value so
///				we can play with the string internally with impunity
///				BEW note, 4Feb10, this use of hyphen as a delimiter for parsing is safe
///				because hyphens are filtered out of machinename and username strings
///				before thsy are used to construct the lock file's name
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
					const wxString username,
					const wxString processID)
{
	wxString str = prefix;
	str += _T("-") + machinename;
	str += _T("-") + username;
	str += _T("-") + processID;
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
bool ReadOnlyProtection::IsDifferentUserOrMachineOrProcess(wxString& localMachine,
					wxString& localUser, wxString& localProcessID, wxString& theOwningMachine,
					wxString& theOwningUser, wxString& theOwningProcessID)
{
	// first check for nobody currently owning the folder - if that is the case
	// then return FALSE so that I can become its owner
	if (theOwningMachine.IsEmpty() || theOwningUser.IsEmpty() || theOwningProcessID.IsEmpty())
		return FALSE;
	// if we get here, someone is the owner of the target folder, so find out who
	if (localMachine != theOwningMachine)
		return TRUE;
	if (localUser != theOwningUser)
		return TRUE;
	if (localProcessID != theOwningProcessID)
		return TRUE;
    // if we get here, each is a match for the other, so I & my process already am the
    // owner of the target folder, so return FALSE
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the absolute path to the read-only protection file in the passed in folder,
///				if it exists in the folder; otherwise, an empty string
///	\param		projectFolderPath ->	absolute path to the project folder being checked
/// \remarks	Search for a matching file in the project folder. wxRegEx class is not safe
///             to use because regular expression support for Unicode may not work - one
///             system does not support matching across character block boundaries,
///             another (eg. VIM) allows this but limits matches to 128 (enough for our
///             purposes but the wx documentation doesn't indicate if this is
///             supported). So we don't use regular expressions to obtain a match;
///             instead, we enumerate all files matching a file specification which
///             includes the prefix "~AIROP" and the suffix ".lock" and the presence of
///             a hyphen following the prefix. That much extremely unlikely to occur by
///             accident in an arbitrary filename from another unrelated source. There
///             should only be one returned. An empty string is returned if there was no
///             match made.
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
		mssg = mssg.Format(
_("The project folder being tested, %s, is really a file. Adapt It will continue running, but you should next try to properly locate a project folder."),
projectFolderPath.c_str());
		wxMessageBox(mssg, _("Warning: Not a folder!"), wxICON_WARNING);
		return theFilename;
	}
	// sadly the following wxDir class function does not support a path which is a URI
	//bool bDirectoryExists = wxDir::Exists(projectFolderPath); // a static function
	bool bDirectoryExists = ::wxDirExists(projectFolderPath);
#ifdef _DEBUG_ROP
	wxLogDebug(_T("GetReadOnlyProtectionFileInProjectFolder: projectFolderPath %s ,  exists? = %s"), 
		projectFolderPath, bDirectoryExists ? _T("TRUE") : _T("FALSE"));
#endif
	if (bDirectoryExists)
	{
		// sadly in the next call, wxDir does not support a URI, and .HasFiles() call
		// returns FALSE, but the dir.IsOpened() call works, so don't use .HasFiles()
		wxDir dir(projectFolderPath);
		bool bIsOpened = dir.IsOpened();
#ifdef _DEBUG_ROP
		wxLogDebug(_T("GetReadOnlyProtectionFileInProjectFolder:  dir.IsOpened()? = %s"), 
			bIsOpened ? _T("TRUE") : _T("FALSE"));
#endif
		if (!bIsOpened)
		{
			// it didn't open, this is unexpected and probably is something for the developer
			// to fix, so warn and abort
			wxString mssg;
			mssg = mssg.Format(
_T("GetReadOnlyProtectionFileInProjectFolder(): the directory %s failed to open for enumeration. Now aborting."), 
projectFolderPath.c_str());
			wxMessageBox(mssg, _T("wxDir() Err: directory not opened"), wxICON_ERROR);
			wxExit();
			return theFilename;
		}
		else
		{
			// it was opened okay; so return an empty string if there are no files within,
			// otherwise find (using wildcard *) the one and only read-only projection
			// file - if it exists; if found, return it to caller, it none is found, return
			// the empty string
			wxString aFile = _T("");  // store any read-only protection file we find, here

			// make the wildcarded file spec
			wxString theFileSpec = m_strAIROP_Prefix;
			theFileSpec += _T("-*");
			theFileSpec += m_strLock_Suffix;
            // use wxDir to enumerate all file in directory and look for the first which
            // begins with "~AIROP-" and ends with ".lock"; or better still, use
            // GetAllFiles() which is easier as it uses a string array & supports wildcards
			wxArrayString files;
			int numFiles = dir.GetAllFiles(projectFolderPath, &files, theFileSpec, wxDIR_FILES);
			if (numFiles > 0)
			{
				// a file matching ~AIROP-*.lock has been found, take the first such
				// (there should only be one anyway)
				wxString path = files.Item(0);
				wxFileName fn(path);
				aFile = fn.GetFullName();
			}
#ifdef _DEBUG_ROP
			wxLogDebug(_T("GetReadOnlyProtectionFileInProjectFolder: theFileSpec:  %s "),theFileSpec);
			wxLogDebug(_T("GetReadOnlyProtectionFileInProjectFolder: ::wxFindFirstFile returns:  %s "),
				aFile.IsEmpty() ? _T("Empty String") : aFile);
#endif
			if (!aFile.IsEmpty())
			{
				theFilename = aFile;
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
		// the directory should exist - warn user. However, if setting up a custom folder location
		// and not going through with defining a project folder, then detect theFilename
		// being empty and silently return an empty string
		if (theFilename.IsEmpty())
			return theFilename;
		/*
		wxString mssg;
		mssg = mssg.Format(
_T("GetReadOnlyProtectionFileInProjectFolder(): the path, %s, to the passed in project folder does not exist! Now aborting."),
projectFolderPath.c_str());
		wxMessageBox(mssg, _T("wxDir() Err: the directory does not exist"), wxICON_ERROR);
		wxExit();
		*/
	}
	// populate the "owning" private members before returning
	m_strOwningMachinename = ExtractMachinename(theFilename);
	m_strOwningUsername = ExtractUsername(theFilename);
	m_strOwningProcessID = ExtractProcessID(theFilename);
	return theFilename;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the protection file (with form ~AIROP-*.lock, where * is
///             machinename-username-processID) is not open for writing. This would only
///             be the case if it is a zombie left over when the app died or power was lost.
///             Return FALSE if it is open for writing.
///	\param		projectFolderPath  ->  absolute path to the project folder being checked
///	\param      ropFile            ->  the read-only protection filename (see above)
/// \remarks	Builds the path to the protection filename and attempts to remove the file.
///             This will succeed only if the file is not open for writing. A side-effect
///             therefore is the successful removal of the file if in fact it was a zombie.
///////////////////////////////////////////////////////////////////////////////////////////
bool ReadOnlyProtection::IsZombie(wxString& folderPath, wxString& ropFile)
{
	wxString pathToFile = folderPath + m_pApp->PathSeparator + ropFile;
	wxASSERT(::wxFileExists(pathToFile));
    // The file can be removed only if it is a (closed) one, that is, a zombie left over
    // from a crash or power loss (regardless of whoever was the former owner for writing)
	bool bRemoved = ::wxRemoveFile(pathToFile);
	if (bRemoved)
	{
		// we were able to remove it, so it's a zombie
		return TRUE;
	}
	else
	{
        // we could not remove it, so it is already opened by someone else's process, (even
        // the original one I initiated myself)
		return FALSE;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if a created protection file (with form ~AIROP-*.lock, where * is
///             machinename-username-processID) was removed, FALSE if it could not be
///             removed.
/// \param      projectFolderPath   ->  absolute path to the project folder which we are
///                                     attempting to find out whether it is currently 
///                                     owned for writing or not
/// \param      ropFile             ->  filename for the read-only protection file; see
///                                     other header comments for format - it's title
///                                     includes the name of the machine, user, and the
///                                     running process's integer ID.
/// \remarks Does the work of removing a given read-only protection file (ROP file), but
/// first it checks to make sure it is entitled to do the removal. The ROP file has to be
/// owned my running Adapt It instance, and I must be the account holder, and the machine
/// mine, to qualify for removal. It the user, machine, or process is not mine, then the
/// file is left in place.
////////////////////////////////////////////////////////////////////////////////////////////
bool ReadOnlyProtection::RemoveROPFile(wxString& projectFolderPath, wxString& ropFile)
{
	wxString pathToFile = projectFolderPath + m_pApp->PathSeparator + ropFile;
	wxASSERT(::wxFileExists(pathToFile));

	// removal will only be possible if I on the localMachine in the process having
	// the localProcessID, became the owner in the first place; so we must test
	// that all the ducks line up for this running instance to be able to remove it,
	// and report back to the caller what the outcome was
	bool bRemoved = TRUE;
	bool bItsNotMe = IsItNotMe(projectFolderPath);
	if (bItsNotMe)
	{
		// not me, or not same process, so the file cannot be removed
		return FALSE;
	}
	else
	{
		// it's an opened protection file, and my process qualifies for closing it and
		// then deleting it; do so
		m_pApp->m_pROPwxFile->Close(); // ignore returned boolean, this shouldn't fail
		bRemoved = ::wxRemoveFile(pathToFile); // may fail
	}
	return bRemoved;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the owning username, or owning machine, or owning process which owns
///             write permission, is different from my (local) username, machinename, and/or
///             running process; if FALSE is returned, then my process on my machine is the
///             owner of write permission
/// \remarks	Encapsulates the test for whether or not I and my process & machine is the
///             one that originally gained ownership of the project folder for writing
///////////////////////////////////////////////////////////////////////////////////////////
bool ReadOnlyProtection::IsItNotMe(wxString& projectFolderPath)
{
	m_strOwningReadOnlyProtectionFilename = 
							GetReadOnlyProtectionFileInProjectFolder(projectFolderPath);
	m_strOwningMachinename = ExtractMachinename(m_strOwningReadOnlyProtectionFilename);
	m_strOwningUsername = ExtractUsername(m_strOwningReadOnlyProtectionFilename);
	m_strOwningProcessID = ExtractProcessID(m_strOwningReadOnlyProtectionFilename);
	bool bItsNotMe = IsDifferentUserOrMachineOrProcess(m_strLocalMachinename,
						m_strLocalUsername, m_strLocalProcessID, m_strOwningMachinename, 
						m_strOwningUsername, m_strOwningProcessID);
	return bItsNotMe;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if a created protection file (with form ~AIROP-*.lock, where * is
///             machinename-username-processID) is already present in the passed in
///             folder and is open for writing; if there is no such file in the folder
///             yet, or if there is such a file but it is not opened for writing (ie. a
///             zombie left over from a power down, or app crash), then return FALSE, and
///             if there was a zombie, automatically remove it as well.
/// \param      projectFolderPath   ->  absolute path to the project folder which we are
///                                     attempting to find out whether it is currently 
///                                     owned for writing or not
/// \remarks A private function which finds out whether or not the folder is owned for
/// writing by someone (whether me on my Adapt It instance, or someone else on some other
/// machine). A side-effect of detecting a zombie projection file is to automatically
/// remove it; only projection files currently open for writing are relevant for the
/// return value, and if one such is present.
//////////////////////////////////////////////////////////////////////////////////////////// 
bool ReadOnlyProtection::IsTheProjectFolderOwnedForWriting(wxString& projectFolderPath)
{
	m_strOwningReadOnlyProtectionFilename.Empty();
	m_strOwningUsername.Empty();
	m_strOwningMachinename.Empty();
	// get the file, if one exists in the folder, else get an empty string
	// (this call also populates the m_strOwningMachinename, m_strOwningUsername and
	// m_strOwningProcessID private strings based on the info in the protection filename)
	m_strOwningReadOnlyProtectionFilename = 
							GetReadOnlyProtectionFileInProjectFolder(projectFolderPath);
	if (m_strOwningReadOnlyProtectionFilename.IsEmpty())
	{
		// there is no read-only protection file in this project folder
		return FALSE; // tell caller I now have qualified to own it for writing
	}
	else
	{
		// there is a read-only protection file in this project, and so there
		// are two possibilities:
		// (1) it's a zombie left over after a crash or power loss - in which case we
		// can delete it because the project folder is not then owned by anyone, or
		// (2) someone else's process (or another one which I initiated) has the 
		//     ownership of write permission - and so we must return TRUE
		bool bIsZombie = IsZombie(projectFolderPath, m_strOwningReadOnlyProtectionFilename);
		if(bIsZombie)
		{
            // it was a zombie, and the test had the side effect of removing it...
            // so I can now become the owner for writing
			return FALSE;
		}
	}
	// write permission is currently owned by someone (it could be me though)
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if a created protection file (with form ~AIROP-*.lock, where * is
///             machinename-username-processID) is already present in the passed in
///             folder and is open for writing; if there is no such file in the folder
///             yet, or if the folder already is owned by me from my running process, then
///             return FALSE
///	\param		projectFolderPath  ->  absolute path to the project folder containing the
///	                                   read-only protection file which we are attempting
///	                                   to protect from having data lost by access by a
///	                                   remote running Adapt It instance
/// \remarks The return value always must be assigned to the flag
/// CAdapt_ItApp::m_bReadOnlyAccess. TRUE is returned if the folder is already owned by
/// someone else, FALSE is returned if the local machine succeeds in getting ownership of
/// the project folder for write purposes. If two instances of Adapt It access the same
/// projrect folder, the winner as far as getting ownership of writing permission is the
/// instance that enters the project first. Ownership is relinquished briefly while the
/// owning instance closes a document and then opens a different document in the same
/// project. This gives a remote instance a brief window of opportunity to get full
/// access, but the remote instance would have to either enter the project during that
/// brief window of opportunity, or open a document in the project during that window -
/// either possibility is unlikely unless people explicitly agree and time their actions
/// accordingly .
/// Call this function when the running instance enters a project project or opens or
/// creates a document
//////////////////////////////////////////////////////////////////////////////////////////// 
bool ReadOnlyProtection::SetReadOnlyProtection(wxString& projectFolderPath) 
{
	bool bIsOwned = IsTheProjectFolderOwnedForWriting(projectFolderPath);
#ifdef _DEBUG_ROP
	wxLogDebug(_T("\n **BEGIN** SetReadOnlyProtection:  Is the folder owned? bIsOwned = %s"), 
			bIsOwned ? _T("TRUE") : _T("FALSE"));
#endif
	if (bIsOwned)
	{
		// we need to distinguish me on my machine in my running process, from
		// another user/machine/or process (even if the process is a second one
		// which I initiated on this machine and am trying to access the same
		// project folder that my earlier process succeeded in getting access to)
		bool bItsNotMe = IsItNotMe(projectFolderPath);
#ifdef _DEBUG_ROP
		wxLogDebug(_T("SetReadOnlyProtection:  Is is NOT me?  bItsNotMe = %s"), 
				bItsNotMe ? _T("TRUE") : _T("FALSE"));
#endif
		if (bItsNotMe)
		{
			// we can call SetReadOnlyProtection() serially, eg. when entering a
			// project for the first time, and then each time a document is opened
			// or created in that project folder; so we don't want to have this
			// message shown more than once - so wrap it in a test so that once
			// in the read-only folder, the user doesn't see it again
			if (!m_pApp->m_bReadOnlyAccess)
			{
				// I don't qualify to own this project folder, I can only have read-only
				// access (this message is localizable)
				wxMessageBox(
_("You have READ-ONLY access to this project folder."),_("Another process owns write permission"),
				wxICON_INFORMATION);
			}
			return TRUE; // return TRUE to app member m_bReadOnlyAccess
		}
		else
		{
            // it's me, in the same process which originally got ownership so just return
            // FALSE as I have full write permission already, and FALSE maintains that
#ifdef _DEBUG_ROP
			wxLogDebug(_T("SetReadOnlyProtection:  Its ME,  so returning FALSE"));
#endif
			return FALSE;  // return FALSE to app member m_bReadOnlyAccess
		}
	}
	else
	{
		// the project folder is not owned for writing - this covers the the zombie
		// case, and also when no running Adapt It instance has as yet entered this
		// project folder... so set my process up as its owner
		wxASSERT(!m_strReadOnlyProtectionFilename.IsEmpty()); // m_pROP->Initialize() 
            // should have made it already; likewise the members m_strLocalUsername &
            // m_strLocalMachinename and m_strLocalProcessID
		wxASSERT(!m_strLocalMachinename.IsEmpty());
		wxASSERT(!m_strLocalUsername.IsEmpty());
		wxASSERT(!m_strLocalProcessID.IsEmpty());

		m_strOwningMachinename = m_strLocalMachinename;
		m_strOwningUsername = m_strLocalUsername;
		m_strOwningProcessID = m_strLocalProcessID;
		m_strOwningReadOnlyProtectionFilename = m_strReadOnlyProtectionFilename;
		wxString readOnlyProtectionFilePath = projectFolderPath + m_pApp->PathSeparator + 
												m_strReadOnlyProtectionFilename;
		m_pApp->m_pROPwxFile->Open(readOnlyProtectionFilePath,wxFile::write);
		wxASSERT(m_pApp->m_pROPwxFile->IsOpened()); // check it got opened
	}
#ifdef _DEBUG_ROP
	wxLogDebug(_T("SetReadOnlyProtection:  Nobody owns it yet,  so returning FALSE"));
#endif
	return FALSE; // return FALSE to app member m_bReadOnlyAccess
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the protection file (with form ~AIROP-*.lock, where * is
///             machinename-username-processID) was successfully removed; else FALSE.
///	\param		projectFolderPath  ->  absolute path to the project folder containing the
///	                                   read-only protection file which we are attempting
///	                                   to remove
/// \remarks Removal can only be done by the process which currently has ownership of write
/// permission for the passed in project folder; a second process coming along later cannot
/// force the owner to relinquish ownership. The return value always be used for setting
/// or clearing pApp->m_bReadOnlyAccess; TRUE is returned if Removal was successful, FALSE
/// is returned if ownership is retained be someone else because this process does not qualify
/// for removing of protection.
/// Call this function when the running instance relinguishes the project
//////////////////////////////////////////////////////////////////////////////////////////// 
bool ReadOnlyProtection::RemoveReadOnlyProtection(wxString& projectFolderPath) 
{
	// get the current protection file, if there is one (this call tests for a zombie
	// as well, and if it finds one it deletes it & returns FALSE)
	bool bRemoved = FALSE;
	bool bOwned = IsTheProjectFolderOwnedForWriting(projectFolderPath);
	if (bOwned)
	{
		//  the following line effects the removal only if the protection file, when
		//  tested, turns out to be for me on my machine in the process which originally
		//  was successful in getting ownership of write privileges for this project folder
		bRemoved = RemoveROPFile(projectFolderPath, m_strOwningReadOnlyProtectionFilename);
		if (bRemoved)
		{
			m_strOwningMachinename.Empty();
			m_strOwningUsername.Empty();
			m_strOwningProcessID.Empty();
			m_strOwningReadOnlyProtectionFilename.Empty();
		}
	}
	else
	{
		// it's not owned, and this covers the zombie case too; return TRUE as these
		// circumstances is equivalent to having succeeded in removing ownership
		m_strOwningMachinename.Empty();
		m_strOwningUsername.Empty();
		m_strOwningProcessID.Empty();
		m_strOwningReadOnlyProtectionFilename.Empty();
		bRemoved = TRUE;
	}
	return bRemoved; 
}

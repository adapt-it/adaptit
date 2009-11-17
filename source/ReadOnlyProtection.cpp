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
		m_strLocalUsername = _T("UnknownUser"); // not localizable, just need 'something'
	}
	m_strLocalMachinename = GetLocalMachinename();
	if (m_strLocalMachinename.IsEmpty())
	{
		m_strLocalMachinename = _T("UnknownMachine"); // ditto
	}
	// set up the filename for read only protection, which is for this particular user
	// and host machine; it remains unchanged for the session
	m_strReadOnlyProtectionFilename = MakeReadOnlyProtectionFilename(m_strAIROP_Prefix,
					m_strLock_Suffix, m_strLocalMachinename, m_strLocalUsername);

	// at launch, this one should be empty
	m_strOwningReadOnlyProtectionFilename.Empty();

	// make the file descriptor integer default to stderr for a safe useless value
	//ropFD = wxFile::fd_stderr;
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
		mssg.Format(
_("The project folder being tested, %s, is really a file. Adapt It will continue running, but you should next try to properly locate a project folder."),
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
			mssg.Format(
_T("GetReadOnlyProtectionFileInProjectFolder(): the directory %s failed to open for enumeration. Now aborting."),
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
					mssg.Format(
_T("GetReadOnlyProtectionFileInProjectFolder(): the directory %s has the file ~AIROP-*.lock, but GetFirst() failed to get it. Now aborting."),
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
		mssg.Format(
_T("GetReadOnlyProtectionFileInProjectFolder(): the path, %s, to the passed in project folder was tested and found to not exist! Now aborting."),
			projectFolderPath);
		wxMessageBox(mssg,_T("wxDir Error: directory does not exist"), wxICON_ERROR);
		wxExit();
	}
	return theFilename;
}

/* f.IsOpened() returns true if already opened, so is a bad test - try remove instead
bool ReadOnlyProtection::IsTheReadOnlyProtectionFileAZombie(wxString& folderPath, wxString& ropFile)
{
	wxString pathToFile = folderPath + m_pApp->PathSeparator + ropFile;
	wxASSERT(::wxFileExists(pathToFile));
	wxFile f(pathToFile,wxFile::write); // will fail if it is already opened
	bool bIsOpened = f.IsOpened();
	if (bIsOpened)
	{
		// we were able to open it, so it must be a zombie,  close it again before returning
		f.Close();
		return TRUE;
	}
	else
	{
		// we could not open it, so it is already opened by someone else, so not a zombie
		return FALSE;
	}
}
*/
bool ReadOnlyProtection::IsTheReadOnlyProtectionFileAZombie(wxString& folderPath, wxString& ropFile)
{
	wxString pathToFile = folderPath + m_pApp->PathSeparator + ropFile;
	bool bAlreadyOpen = FALSE;
	wxASSERT(::wxFileExists(pathToFile));
	bool bRemoved = RemoveROPFile(folderPath,ropFile,bAlreadyOpen);
	if (bRemoved)
	{
		// we were able to remove it, so it must have been a zombie
		wxASSERT(!bAlreadyOpen);
		return TRUE;
	}
	else
	{
		// we could not remove it, so it is already opened by someone else,
		// so not a zombie, and so it remains undeleted
		wxASSERT(bAlreadyOpen);
		return FALSE;
	}
}

bool ReadOnlyProtection::RemoveROPFile(wxString& folderPath, wxString& ropFile, 
										bool& bAlreadyOpen)
{
	bAlreadyOpen = FALSE;
	wxString pathToFile = folderPath + m_pApp->PathSeparator + ropFile;
	wxASSERT(::wxFileExists(pathToFile));
	// in case IsTheReadOnlyProtectionFileAZombie() was not called before calling
	// RemoveROPFile(), we'll try a removal and if it fails assume it was because
	// it was already open
	bool bRemoved = ::wxRemoveFile(pathToFile);
	if (bRemoved)
	{
		bAlreadyOpen = FALSE;
	}
	else
	{
		bAlreadyOpen = TRUE;
	}
	return bRemoved;
}
/* old version, assumed f.IsOpened() would yield false if it was already open, but not so
bool ReadOnlyProtection::RemoveROPFile(wxString& folderPath, wxString& ropFile, 
										bool& bAlreadyOpen)
{
	bAlreadyOpen = FALSE;
	wxString pathToFile = folderPath + m_pApp->PathSeparator + ropFile;
	wxASSERT(::wxFileExists(pathToFile));
	// in case IsTheReadOnlyProtectionFileAZombie() was not called before calling
	// RemoveROPFile(), we'd do the test for the file being opened, and return
	// FALSE if that is the case, because we can't remove a file currently open
	// for writing
	wxFile f(pathToFile,wxFile::write); // will fail if it is already opened
	bool bIsOpened = f.IsOpened();
	if (!bIsOpened)
	{
		// we didn't succeed in opening it, so it must have been already opened
		// so tell the caller it hasn't (and can't) be removed
		bAlreadyOpen = TRUE;
		return FALSE;
	}
	// if we get here, we succeeded in opening it, so close it again and then delete it
	f.Close();
	bool bRemoved = ::wxRemoveFile(pathToFile); //
	return bRemoved;
}
*/
/* old version, I now delete the file in the zombie test so need different syntax
bool ReadOnlyProtection::IsTheProjectFolderOwnedByAnother(wxString& projectFolderPath)
{
	bool bItsNotMe = FALSE; // assume I'm the owner of write permission until 
							// proven otherwise
	m_strOwningReadOnlyProtectionFilename.Empty();
	m_strOwningUsername.Empty();
	m_strOwningMachinename.Empty();
	// get the file, if one exists in the folder, else get an empty string
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
		// are three possibilities:
		// (1) it's a zombie left over after a crash or power loss
		// (2) it's opened, but I'm the owner already (shouldn't happen often,
		//		because we close and remove the file whenever the owner leaves
		//		the owned project folder; so if it is present and not a zombie,
		//		then (3) should be the case usually, but I'll allow for the
		//		possibility of a novel set of actions that get the user back to
		//		his own current project)
		// (3) it's opened, and someone else has the ownership of write permission
		bool bIsZombie = IsTheReadOnlyProtectionFileAZombie(projectFolderPath, 
										m_strOwningReadOnlyProtectionFilename);
		if(bIsZombie)
		{
			// remove it, then return FALSE
			bool bAlreadyOpen = FALSE;
			bool bWasRemoved = RemoveROPFile(projectFolderPath, 
								m_strOwningReadOnlyProtectionFilename, bAlreadyOpen);
			if (!bWasRemoved)
			{
				wxASSERT(bAlreadyOpen == FALSE); // the zombie test established it was
						// not open, so the removal failure was because ::wxRemoveFile()
						// itself failed, and the developer needs to figure out why
				// This is a fatal error, because if we leave it in the project folder
				// we could end up with this zombie and a genuine one being in the same
				// folder - and that would break the constraint that we allow only one
				// such file per project folder - so the developer needs to know & fix it
				wxString mssg;
				mssg.Format(
_T("IsTheProjectFolderOwnedByAnother(): the zombie read-only protection file: %s, in the project folder: %s, was not successfully removed. Now aborting."),
					m_strOwningReadOnlyProtectionFilename, projectFolderPath);
				wxMessageBox(mssg,_T("ReadOnlyProtection Error: zombie not removed"), wxICON_ERROR);
				wxExit();
				return FALSE;
			}
			// if we get to here, it was successfully removed, so the local user can
			// now become the owner for writing
			return FALSE;
		}
		// next, check out if I (the local user) can become the owner for writing; as
		// the read-only protection file exists and is opened for writing - but is it
		// mine, or someone else's?
		m_strOwningMachinename = ExtractMachinename(m_strOwningReadOnlyProtectionFilename);
		m_strOwningUsername = ExtractUsername(m_strOwningReadOnlyProtectionFilename);
		bItsNotMe = IsDifferentUserOrMachine(m_strLocalMachinename, m_strLocalUsername,
												m_strOwningMachinename, m_strOwningUsername);
	}
	return bItsNotMe;
}
*/
bool ReadOnlyProtection::IsTheProjectFolderOwnedByAnother(wxString& projectFolderPath)
{
	bool bItsNotMe = FALSE; // assume I'm the owner of write permission until 
							// proven otherwise
	m_strOwningReadOnlyProtectionFilename.Empty();
	m_strOwningUsername.Empty();
	m_strOwningMachinename.Empty();
	// get the file, if one exists in the folder, else get an empty string
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
		// are three possibilities:
		// (1) it's a zombie left over after a crash or power loss
		// (2) it's opened, but I'm the owner already (shouldn't happen often,
		//		because we close and remove the file whenever the owner leaves
		//		the owned project folder; so if it is present and not a zombie,
		//		then (3) should be the case usually, but I'll allow for the
		//		possibility of a novel set of actions that get the user back to
		//		his own current project)
		// (3) it's opened, and someone else has the ownership of write permission
		bool bIsZombie = IsTheReadOnlyProtectionFileAZombie(projectFolderPath, 
										m_strOwningReadOnlyProtectionFilename);
		if(bIsZombie)
		{
			// it was a zombie, because it was removed in the above call...
			// so the local user can now become the owner for writing
			return FALSE;
		}
		// next, check out if I (the local user) can become the owner for writing; as
		// the read-only protection file exists and is opened for writing - but is it
		// mine, or someone else's?
		m_strOwningMachinename = ExtractMachinename(m_strOwningReadOnlyProtectionFilename);
		m_strOwningUsername = ExtractUsername(m_strOwningReadOnlyProtectionFilename);
		bItsNotMe = IsDifferentUserOrMachine(m_strLocalMachinename, m_strLocalUsername,
												m_strOwningMachinename, m_strOwningUsername);
	}
	return bItsNotMe;
}

// the return value sets or clears pApp->m_bReadOnlyAccess
bool ReadOnlyProtection::SetReadOnlyProtection(wxString& projectFolderPath) // call when entering project
{
	bool bIsOwnedByAnother = IsTheProjectFolderOwnedByAnother(projectFolderPath);
	if (bIsOwnedByAnother)
	{
		// inform the user he has only read-only access; the message is localizable
		wxMessageBox(
_("You have READ_ONLY access to this project, until the other person leaves the project folder."),_("Another person has write permission"),
		wxICON_INFORMATION);
		return TRUE; // the caller which receives this value must be app member m_bReadOnlyAccess
	}
	// either noone as yet has write ownership of the folder, or I already do
	wxString ropFilename = _T("");
	ropFilename = GetReadOnlyProtectionFileInProjectFolder(projectFolderPath);
	if (ropFilename.IsEmpty())
	{
		// none currently has ownership for writing for this particular project 
		// folder as yet; so set myself up as its owner
		wxASSERT(!m_strReadOnlyProtectionFilename.IsEmpty()); // m_pROP->Initialize() should have 
				// made it already; likewise the members m_strLocalUsername & m_strLocalMachinename
		wxASSERT(!m_strLocalMachinename.IsEmpty());
		wxASSERT(!m_strLocalUsername.IsEmpty());
		// declare me to be the owner of this project folder
		m_strOwningMachinename = m_strLocalMachinename;
		m_strOwningUsername = m_strLocalUsername;
		m_strOwningReadOnlyProtectionFilename = m_strReadOnlyProtectionFilename;
		wxString readOnlyProtectionFilePath = projectFolderPath + m_pApp->PathSeparator + 
												m_strReadOnlyProtectionFilename;
		m_pApp->m_pROPwxFile->Open(readOnlyProtectionFilePath,wxFile::write);
		wxASSERT(m_pApp->m_pROPwxFile->IsOpened()); // check it got opened
	}
	else
	{
		// I must have ownership, so nothing to do except check it's so
		bool bItsMe = !IsDifferentUserOrMachine(m_strLocalMachinename, m_strLocalUsername,
												m_strOwningMachinename, m_strOwningUsername);
		if (!bItsMe)
		{
			// if it is not me, then who is it? It should not be possible to get into
			// this block, so do something useful here & it may involve some correction
			// manually (such as restarting the machine)
			wxString readOnlyProtectionFilePath = projectFolderPath + m_pApp->PathSeparator + 
												m_strReadOnlyProtectionFilename;
			// try to open it for writing
			m_pApp->m_pROPwxFile->Open(readOnlyProtectionFilePath,wxFile::write);
			// check if it failed to be opened (because it's already open)
			bool bFailedToOpen = !m_pApp->m_pROPwxFile->IsOpened();
			if (bFailedToOpen)
			{
				// We've a problem Houston! Someone has hold of the project folder, but we
				// have already determined above that that was not the case. Tell the user
				// what to do.
				wxMessageBox(
_("Someone other than you has ownership for writing to this project. This will prevent you saving your work. Either wait until the other person leaves the project, or shut down your machine to break the connection and then start it again."),_("Project is owned by someone else"),
				wxICON_INFORMATION);
				return TRUE;
			}
			else
			{
				// it was opened, so must be a zombie -- try get rid of it and then
				//  become the owner
				m_pApp->m_pROPwxFile->Close(); // close wxFILE object
				wxString ropFile = GetReadOnlyProtectionFileInProjectFolder(projectFolderPath);
				bool bAlreadyOpen = FALSE;
				bool bRemoved = RemoveROPFile(projectFolderPath, ropFile, bAlreadyOpen);
				if (bRemoved && !bAlreadyOpen)
				{
					// it should now be possible to become the owner of write privileges
					m_strOwningMachinename = m_strLocalMachinename;
					m_strOwningUsername = m_strLocalUsername;
					m_strOwningReadOnlyProtectionFilename = m_strReadOnlyProtectionFilename;
					wxString readOnlyProtectionFilePath = projectFolderPath + m_pApp->PathSeparator + 
															m_strReadOnlyProtectionFilename;
					m_pApp->m_pROPwxFile->Open(readOnlyProtectionFilePath,wxFile::write);
					wxASSERT(m_pApp->m_pROPwxFile->IsOpened()); // check it got opened
				}
				else
				{
					wxMessageBox(
_("Something unknown is preventing you from getting write access to the project. This will prevent you saving your work. Shut down your machine and then start it again."),_("Unknown write priveleges problem"),
					wxICON_INFORMATION);
					return TRUE;
				}
			}
		}
	}
	return FALSE; // the caller which receives this value must be app member m_bReadOnlyAccess
}

// Removal can only be done by the user who currently has ownership of write permission
// for the passed in project folder; a second person coming along later cannot force the
// owner to relinquish ownership.
// The return value always should clear pApp->m_bReadOnlyAccess to FALSE
bool ReadOnlyProtection::RemoveReadOnlyProtection(wxString& projectFolderPath) // call when leaving project
{
	wxString readOnlyProtectionFilePath = 
		projectFolderPath + m_pApp->PathSeparator + m_strOwningReadOnlyProtectionFilename;
	wxASSERT(::wxFileExists(readOnlyProtectionFilePath));

	bool bOwnedByAnother = IsTheProjectFolderOwnedByAnother(projectFolderPath);
	if (bOwnedByAnother)
	{
		// then I can't own it, so my leaving the project folder has to leave the current
		// owner's write permission unchanged (his ~AIROP*.lock file remains as is), and
		// all I do on my running AI instance is clear my local member variables, and
		// set my local m_bReadOnlyAccess to default FALSE (for wherever I'm going next)
		m_strOwningMachinename.Empty();
		m_strOwningUsername.Empty();
		m_strOwningReadOnlyProtectionFilename.Empty();
		return FALSE; // the caller which receives this value must be app member m_bReadOnlyAccess
	}
	else
	{
		// it's either owned by me, or its a zombie (possibly left over from a crash or power
		// loss) - find out which is the case and do the required removal actions
		bool bRemoved = FALSE;
		wxString ropFile = GetReadOnlyProtectionFileInProjectFolder(projectFolderPath);
		bool bIsZombie = IsTheReadOnlyProtectionFileAZombie(projectFolderPath,ropFile);
		if (bIsZombie)
		{
			// it's removed, so it was a zombie
			bRemoved = TRUE;
		}
		else
		{
			// it's not yet removed and I must own it, so close it and remove it
			m_pApp->m_pROPwxFile->Close();
			bRemoved = ::wxRemoveFile(readOnlyProtectionFilePath);
		}
		// check it got removed (release build will just assume it did, this is unlikely to fail)
		wxASSERT(bRemoved);
	}
	// clear my local member variables and have the caller set m_bReadOnlyAccess to FALSE
	// (yes, I know this code is above and the function could be written simpler, but it
	// is easier to follow what's going on this way)
	m_strOwningMachinename.Empty();
	m_strOwningUsername.Empty();
	m_strOwningReadOnlyProtectionFilename.Empty();
	return FALSE; // the caller which receives this value must be app member m_bReadOnlyAccess
}
/* old version
bool ReadOnlyProtection::RemoveReadOnlyProtection(wxString& projectFolderPath) // call when leaving project
{
	wxString readOnlyProtectionFilePath = 
		projectFolderPath + m_pApp->PathSeparator + m_strOwningReadOnlyProtectionFilename;
	wxASSERT(::wxFileExists(readOnlyProtectionFilePath));

	bool bOwnedByAnother = IsTheProjectFolderOwnedByAnother(projectFolderPath);
	if (bOwnedByAnother)
	{
		// then I can't own it, so my leaving the project folder has to leave the current
		// owner's write permission unchanged (his ~AIROP*.lock file remains as is), and
		// all I do on my running AI instance is clear my local member variables, and
		// set my local m_bReadOnlyAccess to default FALSE (for wherever I'm going next)
		m_strOwningMachinename.Empty();
		m_strOwningUsername.Empty();
		m_strOwningReadOnlyProtectionFilename.Empty();
		return FALSE; // the caller which receives this value must be app member m_bReadOnlyAccess
	}
	else
	{
		// it's either owned by me, or its a zombie (possibly left over from a crash or power
		// loss) - find out which is the case and do the required removal actions
		bool bRemoved = FALSE;
		wxString ropFile = GetReadOnlyProtectionFileInProjectFolder(projectFolderPath);
		bool bIsZombie = IsTheReadOnlyProtectionFileAZombie(projectFolderPath,ropFile);
		if (bIsZombie)
		{
			// it's closed already, so just remove it
			bRemoved = ::wxRemoveFile(readOnlyProtectionFilePath);
		}
		else
		{
			// it's not yet closed and I own it, so close it and remove it
			m_pApp->m_pROPwxFile->Close();
			bRemoved = ::wxRemoveFile(readOnlyProtectionFilePath);
		}
		// check it got removed (release build will just assume it did, this is unlikely to fail)
		wxASSERT(bRemoved);
	}
	// clear my local member variables and have the caller set m_bReadOnlyAccess to FALSE
	// (yes, I know this code is above and the function could be written simpler, but it
	// is easier to follow what's going on this way)
	m_strOwningMachinename.Empty();
	m_strOwningUsername.Empty();
	m_strOwningReadOnlyProtectionFilename.Empty();
	return FALSE; // the caller which receives this value must be app member m_bReadOnlyAccess
}
*/



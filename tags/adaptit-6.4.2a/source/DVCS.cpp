/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			DVCS.cpp
/// \author			Mike Hore
/// \date_created	22 December 2011
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public
///                 License (see license directory)
/// \description This is the implementation file for the DVCS interface.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "DVCS.h"
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
#include <wx/wizard.h>
#include <wx/filesys.h> // for wxFileName

// whm 14Jun12 modified to #include <wx/fontdate.h> for wxWidgets 2.9.x and later
#if wxCHECK_VERSION(2,9,0)
#include <wx/fontdata.h>
#endif

#include "Adapt_It.h"
#include "MainFrm.h"
#include "Adapt_ItDoc.h"
#include "Adapt_ItView.h"
#include "Adapt_ItCanvas.h"
#include "ProjectPage.h"
#include "LanguagesPage.h"
#include "FontPage.h"
#include "PunctCorrespPage.h"
#include "CaseEquivPage.h"
#include "UsfmFilterPage.h"
#include "DocPage.h"
#if wxCHECK_VERSION(2,9,0)
	// Use the built-in scrolling wizard features available in wxWidgets  2.9.x
#else
	// The wxWidgets library being used is pre-2.9.x, so use our own modified
	// version named wxScrollingWizard located in scrollingwizard.h
#include "scrollingwizard.h" // whm added 13Nov11 - needs to be included before "StartWorkingWizard.h" below
#endif
#include "StartWorkingWizard.h"
//#include "SourceBundle.h"
#include "Pile.h"
#include "Layout.h"
#include "WhichBook.h"
#include "helpers.h"
#include "errno.h"
#include "ReadOnlyProtection.h"
#include "Uuid_AI.h"

#include "DVCS.h"



/*	Implementation notes:

	Our DVCS engine is Git.  It uses a command-line interface.  So our job here is to create the command line, then
	send it to git using wxExecute(), then handle the return results.

	This is conceptually very simple, so originally I just wrote procedural code.  But now that we want everything to be as OOP as
	possible, we have a DVCS class which just has one object, gpApp->m_pDVCS, instantiated in the application's OnInit() function.

	All DVCS calls from the application are made via the function gpApp->m_pDVCS->DoDVCS().  This function takes two int parms,
	action and parm.  action is a code telling us what to do, and parm is used for various things depending on the action.

	Now the git command line has the form

		git <command> <options> <arguments>

	So we use 3 wxStrings, which are members (instance variables) of the DVCS class, git_command, git_options and git_arguments.
	We set them up with separate functions depending on the actual command we want to perform, before calling wxExecute in the
	main function which does the work, call_git().

	When git is called, the current directory is our current repository.  Therefore, whenever our main function is called, the 
    first thing we do is cd to the Adaptations directory which is our repository.  We use wxSetWorkingDirectory() for this.  
 */


// constructor:

DVCS::DVCS(void)
{
	m_pApp = &wxGetApp();
	m_user = m_pApp->m_AIuser;
}

// destructor:

DVCS::~DVCS(void)
{
	m_pApp->m_pDVCS = NULL;
}



/*	call_git is the function that actually calls git.
 
	If git isn't found, as well as a nonzero result from wxExecute, we get a message in our errors ArrayString
	on the Mac and Windows, saying the command gave an error (2 in both cases).  But on Linux we don't get
	a message back at all.

	In all cases if git returns an error we get a message in errors.  So we'll need to distinguish between git
	not being found, or being found and its execution returning an error.

	The int we return is the result code from calling wxExecute, so zero means success.
*/

int  DVCS::call_git ( bool bDisplayOutput )
{
	wxString		str, str1, local_arguments;
	wxArrayString	errors;
    long			result;
	int				count, i;
	int				returnCode = 0;		// 0 = no error.  Let's be optimistic here

//#ifdef	__WXMAC__
//	str = _T("/usr/local/bin/git ");
//#else
	str = _T("git ");
//#endif

	// If there's a file or directory path in the arguments string, we need to escape any spaces with backslash space.
	// We do this in a local copy so the caller can reuse the arguments string if needed.  But on Windows this doesn't
	// work, because backslash is the path separator!  It seems we need to put the whole string within quote (") marks.

	local_arguments = git_arguments;

#ifdef __WXMSW__
	if (!local_arguments.IsEmpty())
		local_arguments = (_T("\"")) + local_arguments + (_T("\""));
#else
	local_arguments.Replace (_T(" "), _T("\\ "), TRUE);
#endif

	// git complains if there are trailing blanks on the command line, so we only add the options and arguments if
	//  they're actually there:

	str = str + git_command;
	if (!git_options.IsEmpty())
	str = str + _T(" ") + git_options;
	if (!local_arguments.IsEmpty())
	str = str + _T(" ") + local_arguments;

// wxMessageBox (str);		// debugging

	result = wxExecute (str, git_output, errors, 0);

	// The only indication we get that something went wrong, is a nonzero result.  It seems to always be 255, whatever the error.
	// It may mean that git wasn't found, or it could be an illegal git command.  The details will appear in a separate wx message or
	// will land in our errors wxArrayString.  See below.

	if (result)		// An error occurred
	{
		returnCode = result;
	}
	else
	{		// git's stdout will land in our git_output wxArrayString.  There can be a number of strings.
			// Just concatenating them with a newline between looks OK so far.  We only display a message
			// with the output if we've been asked to.  Otherwise the caller will handle.
		if (bDisplayOutput)
		{
			count = git_output.GetCount();
			if (count)
			{	str1.Clear();
				for (i=0; i<count; i++)
					str1 = str1 + git_output.Item(i) + /*_T(" ")*/ _T("\n");
				wxMessageBox (str1);
			}
		}
	}

// If anything landed in our errors wxArrayString, we'll display it.  We'll probably need to enhance this a bit eventually.

	if (returnCode)						// an error occurred
	{
		count = errors.GetCount();
		if (count)
		{	str1.Clear();
			for (i=0; i<count; i++)
				str1 = str1 + errors.Item(i) + _T(" ");

			wxMessageBox (_T("Error message:\n") + str1);
		}
		else			// there was an error, but no error message.  Normally wx will display a message, but it's asynchronous.
						// It should come up shortly.  But sometimes it doesn't!
		{
			wxMessageBox (_T("An error occurred -- further information should follow."));
		}
	}

	return returnCode;
}


// Setup functions:

// We now init the repository automatically the first time we commit a file.  We want to do this silently, so if everything succeeds
//  we no longer show a message.

int  DVCS::init_repository ()
{
    int     returnCode;

    if ( wxDirExists(_T(".git")) )
        return 0;           // init already done -- just return

	git_command = _T("init");
    git_arguments.Clear();
    git_options.Clear();
	returnCode = call_git (FALSE);

    if (returnCode)         // git init failed -- most likely git isn't installed.
        wxMessageBox(_T("We couldn't set up version control.  Possibly Git has not been installed yet.  You could contact your administrator for help."));

//    else
//        wxMessageBox(_T("Version control is now set up for this project.")); -- we now don't need a message here

    return returnCode;
}


// add_file() adds the file to the staging area, ready for a commit.  This works even if the doc isn't being tracked yet,
//  in which case it starts tracking, then does the add.

int  DVCS::add_file (wxString fileName)
{
	int				returnCode;

// First we init the repository if necessary.  This does nothing if it's already initialized.

    returnCode = init_repository();
    if (returnCode)
        return returnCode;

	git_command = _T("add");
	git_arguments = fileName;
    git_options.Clear();
	returnCode = call_git (FALSE);

    return returnCode;
}


// (Feb 13) - following what Paratext does, if a file isn't being tracked yet, we just silently
//  start tracking it.  This is very easy with git, as we have to add the file to the staging area
//  with git add <file>, and this starts tracking the file even if it wasn't tracked before.

int  DVCS::commit_file (wxString fileName)
{
	wxString		local_owner = m_pApp->m_owner;
	int				commitCount = m_pApp->m_commitCount,
                    returnCode;

#ifndef __WXMSW__
	local_owner.Replace (_T(" "), _T("\\ "), TRUE);
#endif

// first we add the file to the staging area, and bail out on error.
    
    returnCode = add_file(fileName);
    if (returnCode)
        return returnCode;

// now we commit it -- soon we'll have to accept a passed-in string for the commit message.
    
	git_command = _T("commit");
	git_arguments.Clear();
	git_options << _T("-m \"");
	git_options << commitCount;

	if (commitCount == 1)
		git_options << _T(" commit\"");
	else
		git_options << _T(" commits\"");

	return call_git (FALSE);
}


// Changing to git, we've completely revamped this bit.  To get earlier versions, the first thing is to call
// setup_versions().  This reads the log and initializes things.  Then we call get_prev_version() to keep going
// back.  Both these functions return something negative if there's no more.  If we want to return to the latest
// version, we call get_latest_version().

int  DVCS::setup_versions ( wxString fileName )
{
	wxString	nextLine, str;
    //	int			test;
    
    git_output.Clear();
    git_command = _T("log");
    git_options = _T("--pretty=oneline");
    git_arguments = fileName;

    if (call_git (FALSE))
        return -2;				// maybe git's not installed!
    
    git_lineNumber = 0;
    git_count = git_output.GetCount();

    return 0;
}

int  DVCS::get_version ( bool latest, wxString fileName )
{
	wxString	nextLine, str;
    
    // The log has multiple entries, each one line long, with the format
    // <40 hex digits> <comment for that commit>
    // We get the next line as given by git_lineNumber.  If there aren't any
    // more lines, we return -1.  Otherwise we return the result of the call_git() call.

    if (latest)
        git_lineNumber = 0;
    else
        git_lineNumber += 1;
    
    if ( git_lineNumber >= git_count )
        return -1;

    nextLine = git_output.Item(git_lineNumber);
    str = nextLine.BeforeFirst(_T(' '));

    if ( wxIsEmpty(str) )       // shouldn't really happen
        return -1;
    
    git_command = _T("checkout ") + str;
    git_options.Clear();
    git_arguments = fileName;
    
    return call_git(FALSE);
}

int  DVCS::log_file (wxString fileName)
{
	git_output.Clear();
	git_command = _T("log");
	git_arguments = fileName;
	return call_git (TRUE);
}

int  DVCS::log_project()
{
	git_output.Clear();
	git_command = _T("log");
	return call_git (TRUE);
}


// Main function.  This is the only one called from outside this file.
//  It just clears the global wxStrings, cd's to the current repository, then dispatches to the
//  appropriate function to do the work.  We return as a result whatever that function returns.
//  If the cd fails, this means that AdaptIt doesn't have a current project yet.  We complain and bail out.

int  DVCS::DoDVCS ( int action, int parm )
{
	wxString		str;
	int				result = 0;
	bool			bResult;
    wxString		saveWorkDir = wxGetCwd();			// save the current working directory

	git_command.Clear();  git_options.Clear();  git_arguments.Clear();				// Clear the globals

// Next we cd into our repository.  We use wxSetWorkingDirectory() and spaces in pathnames are OK.

	str = m_pApp->m_curAdaptionsPath;
	bResult = ::wxSetWorkingDirectory (str);

	if (!bResult)
	{
		wxMessageBox ( _T("There doesn't appear to be a current project!") );
		return 99;
	}

// Now, what have we been asked to do?

	switch (action)
	{
		case DVCS_VERSION:
			git_output.Clear();
			git_command = _T("version");
			result = call_git (TRUE);
			break;

		case DVCS_COMMIT_FILE:		result = commit_file (m_pApp->m_curOutputFilename);             break;

        case DVCS_SETUP_VERSIONS:   result = setup_versions (m_pApp->m_curOutputFilename);          break;
        case DVCS_PREV_VERSION:     result = get_version (FALSE, m_pApp->m_curOutputFilename);		break;
        case DVCS_LATEST_VERSION:   result = get_version (TRUE, m_pApp->m_curOutputFilename);		break;
 
		case DVCS_LOG_FILE:			result = log_file (m_pApp->m_curOutputFilename);	break;
		case DVCS_LOG_PROJECT:		result = log_project();								break;

		default:
			wxMessageBox (_T("Internal error - illegal DVCS command"));
			result = -1;
	}

	wxSetWorkingDirectory (saveWorkDir);		// restore working directory back to what it was
	return result;
}

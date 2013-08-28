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

	This is conceptually very simple, so originally I just wrote procedural code.  But we want everything to be as OOP as
	possible, so we have a DVCS class which just has one object, gpApp->m_pDVCS, instantiated in the application's 
    OnInit() function.

	All DVCS calls from the application are made via the function gpApp->m_pDVCS->DoDVCS().  This function takes an int parms,
	action, which is a code telling us what to do.

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
	m_user = m_pApp->m_strUserID;
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
    wxLogNull       logNo;              // avoid unwanted system messages

	str = _T("git ");

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

	// hg used to complain if there were trailing blanks on the command line, so we only add the options and arguments if
	//  they're actually there:

	str = str + git_command;
	if (!git_options.IsEmpty())
	str = str + _T(" ") + git_options;
	if (!local_arguments.IsEmpty())
	str = str + _T(" ") + local_arguments;

//wxMessageBox (str);		// uncomment for debugging

	result = wxExecute (str, git_output, errors, 0);

    if (result == -1)       // sometimes I get this on the Mac, but it seems to be spurious, and a retry works.
    {
//        wxMessageBox(_T("-1 result"));
        result = wxExecute (str, git_output, errors, 0);        // just retry once!
    }

	// The only indication we get that something went wrong, is a nonzero result.  It seems to always be 255, whatever the error.
	// It may mean that git wasn't found, or it could be an illegal git command.  The details will appear in a separate wx message or
	// will land in our errors wxArrayString.  See below.

	if (result)		// An error occurred
    {
//wxMessageBox(_T("an error came up!"));      // uncomment for debugging
        if (!bDisplayOutput)                // if we're not to display output, we just get straight out, returning the error code.
            return result;

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

// We init the repository automatically the first time we commit a file.  We want to do this silently, so if everything succeeds
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

    return returnCode;
}


// update_user_details() sets the user.email and user.name items in a git config call.  These have to be correctly set before a commit.
// Since our m_strUserID and m_strUsername strings may have changed, or our current directory may have changed, we just query
// git for the current values, and change them if they differ from those strings.

int  DVCS::update_user_details ()
{
    int     returnCode;
    

    git_command = _T("config");
    git_arguments.Clear();

// check user.email agrees with m_strUserID
    git_output.Clear();
    git_options = _T("--get user.email");
    returnCode = call_git(FALSE);

    if ( returnCode || ( git_output.Item(0) != m_pApp->m_strUserID ) )      // an error might just mean there's no user.email yet
    {   git_options = _T("user.email");
        git_arguments = m_pApp->m_strUserID;    // this can contain spaces but it works like a filename and we handle that properly
        returnCode = call_git (FALSE);
        if (returnCode)  return returnCode;
    }

// check user.name agrees with m_strUsername
    git_output.Clear();
    git_options = _T("--get user.name");
    returnCode = call_git(FALSE);
    if ( returnCode || ( git_output.Item(0) != m_pApp->m_strUsername ) )      // an error might just mean there's no user.name yet
    {   git_options = _T("user.name");
        git_arguments = m_pApp->m_strUsername;  // this can contain spaces but it works like a filename and we handle that properly
        returnCode = call_git (FALSE);
        if (returnCode)  return returnCode;
    }

    return 0;
}

// add_file() adds the file to the staging area, ready for a commit.  This works even if the doc isn't being tracked yet,
//  in which case it starts tracking, then does the add.

int  DVCS::add_file (wxString fileName)
{
	int				returnCode;

// First we init the repository if necessary.  This does nothing if it's already initialized.

    returnCode = init_repository();
    if (returnCode)  return returnCode;

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
	int     commitCount = m_pApp->m_commitCount,
            returnCode;

// first we add the file to the staging area, and bail out on error.

    returnCode = add_file(fileName);
    if (returnCode)  return returnCode;

// next, before the commit, we update the user's details if necessary:
    
    returnCode = update_user_details();
    if (returnCode)  return returnCode;

// now we commit the file:

	git_command = _T("commit");
	git_arguments.Clear();
	git_options = _T("-m \"");

    if ( wxIsEmpty(m_version_comment) )
    {                   // user didn't enter a comment.  We just put "n commits"
        git_options << commitCount;
        if (commitCount == 1)
            git_options << _T(" commit");
        else
            git_options << _T(" commits");
    }
    else                // we use the user's comment
        git_options << m_version_comment;
    git_options << _T("\"");

	return call_git (FALSE);
}


// Changing to git, we've completely revamped this bit.  To get earlier versions, the first thing is to call
// setup_versions().  This reads the log and initializes things, and returns the number of versions, or
// a negative number on error.
// Then we call get_version() to get a specific version.

int  DVCS::setup_versions ( wxString fileName )
{
	wxString	nextLine, str;

    git_output.Clear();
    git_command = _T("log");
    git_options = _T("--pretty=format:%H#%cn#%cd#%s");      // hash, committer name, commit date, comment - using # as a separator
                                                            // since it can't appear in any of the fields except maybe the last.
    git_arguments = fileName;

    if (call_git (FALSE))
        return -2;				// maybe git's not installed!

    m_pApp->m_DVCS_log = &git_output;       // save pointer to log in app global for our dialog.  This is OK since this
                                            //  DVCS object lasts for the whole application run.
    git_count = git_output.GetCount();
    return git_count;
}

/*  get_version() calls git to checkout the given version number, defined by the line number in the log which we should
    have already read.  The log has multiple entries, each one line long.  The first line is line zero, giving the most 
    recent version.
    The log line format is what we asked for in setup_versions():
 
        <40 hex digits hash>#<committer name>#commit date#<commit comment>
 
    The caller should ensure we're not being asked for a nonexistent line, but we check anyway, and return -1 on out of bounds
    of if for some reason the line is empty.  This will be a bug...
    Otherwise we return zero normally, or if git returns an error, we return that (which must be positive).
*/
 
int  DVCS::get_version ( int version_num, wxString fileName )
{
	wxString	nextLine, str;
    int         returnCode;


    if ( version_num >= git_count || version_num < 0)
        return -1;                  // return -1 on out of bounds, which shouldn't happen anyway

    nextLine = git_output.Item (version_num);
    str = nextLine.BeforeFirst(_T('#'));        // get the version hash for checkout call

    if ( wxIsEmpty(str) )                       // shouldn't really happen
        return -1;

    git_command = _T("checkout ") + str;
    git_options.Clear();
    git_arguments = fileName;

    returnCode = call_git(FALSE);
    if (returnCode)  return returnCode;                 // bail out on error, returning the error code

    str = nextLine.AfterFirst(_T('#'));                 // skip version hash
    m_version_committer = str.BeforeFirst(_T('#'));     // get committer name
    str = str.AfterFirst(_T('#'));                      // now skip the name
    m_version_date = str.BeforeFirst(_T('#'));          // get commit date
    m_version_comment = str.AfterFirst(_T('#'));        // and the rest of the string, after the separator, is the comment.
                                                        // By making this the last field, it can contain anything, even our # separator
    return 0;                                           // return no error
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

// Now if we're just checking for git being there, we don't have to do much:

    if (action == DVCS_CHECK)
    {   git_output.Clear();
        git_command = _T("version");
        result = call_git (parm != 0);      // Only display the version if the parm is nonzero
        return result;
    }

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
		case DVCS_COMMIT_FILE:		result = commit_file (m_pApp->m_curOutputFilename);             break;

        case DVCS_SETUP_VERSIONS:   result = setup_versions (m_pApp->m_curOutputFilename);          break;
        case DVCS_GET_VERSION:      result = get_version (parm, m_pApp->m_curOutputFilename);		break;

		default:
			wxMessageBox (_T("Internal error - illegal DVCS command"));
			result = -1;
	}

	wxSetWorkingDirectory (saveWorkDir);		// restore working directory back to what it was
	return result;
}


//  =================================================================================================
//
//                              Save and commit dialog
//
//  =================================================================================================

//  This dialog could be moved to separate .h and .cpp files, but it's short and simple, so I
//  haven't bothered yet.


// Declaration of the DVCSDlg class

class DVCSDlg : public AIModalDialog
{
public:
    DVCSDlg (wxWindow *parent);         // constructor

    wxSizer*        m_dlgSizer;
    wxTextCtrl*     m_comment;
    wxStaticText*   m_blurb;

};

// Implementation of the DVCSDlg class

DVCSDlg::DVCSDlg(wxWindow *parent)
                : AIModalDialog (   parent, -1, wxString(_T("Save in History")),
                                    wxDefaultPosition,
                                    wxDefaultSize,
                                    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	m_dlgSizer = DVCSDlgFunc ( this, TRUE, TRUE );

    m_comment = (wxTextCtrl*) FindWindowById(IDC_COMMIT_COMMENT);
    m_blurb = (wxStaticText*) FindWindowById(IDC_COMMIT_BLURB);
}


bool DVCS::AskSaveAndCommit (wxString blurb)
{
    wxString        comment;
    CAdapt_ItApp*   pApp = &wxGetApp();
    DVCSDlg         dlg ( pApp->GetMainFrame() );

    pApp->ReverseOkCancelButtonsForMac(&dlg);           // doing the right thing here
	dlg.Centre();

// Now if blurb is non-empty, we set that as the informative text in the dialog.  Otherwise we leave the
//  default text which is already there.

    if ( !wxIsEmpty(blurb) )
        dlg.m_blurb->SetLabel (blurb);

    if (dlg.ShowModal() != wxID_OK)
        return FALSE;                   // Bail out if user cancelled, and return FALSE to caller

// Now we get the comment, and save it in our instance variable:
    m_version_comment = dlg.m_comment->GetValue();
    return TRUE;
}


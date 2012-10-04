/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			DVCS.cpp
/// \author			Mike Hore
/// \date_created	22 December 2011
/// \date_revised
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

	Our DVCS engine is Mercurial (hg).  It uses a command-line interface.  So our job here is to create the command line, then
	send it to hg using wxExecute(), then handle the return results.  We do this in the main function CallDVCS().
 
	The command line has the form
		
		hg <command> <options> <arguments>
 
	So we use 3 wxStrings, hg_command, hg_options and hg_arguments.  These are global to this file, so that we can easily
	set them up with separate functions depending on the actual command we want to perform, before calling wxExecute in the
	main function.
 
	Another approach would have been to set up a class with these wxStrings as members, and the different functions as methods.  
	This would have been a bit of overkill since we don't currently need to hang on to any state from one call to the next, and 
	we'd only need a single object of this class.  So instead we'll keep it simple and only bring in the heavy machinery if
	we need to later.
 
	The hg man says we can use wildcards in paths.  In practice this only works in the current directory or the immediate
	parent.  Therefore, whenever our main function is called, the first thing we do is cd to the Adaptations directory which
	is our repository.  We use wxSetWorkingDirectory() for this.  Then after that we can just use filenames without having 
	to fully path them out, and wildcards work.
 */
 
 
 
wxString		hg_command, hg_options, hg_arguments;
wxArrayString	hg_output;
int				hg_count, hg_lineNumber;


/*	call_hg is the function that actually calls hg.
	For some utterly unaccountable reason, under Windows or Linux, we can just find "hg" since if it's
	properly installed, it's in one of the paths listed in the PATHS environment variable.  However
	on the Mac, although its location is in PATHS, and we can just type "hg <whatever>" in a terminal
	window and it gets found, it doesn't get found by the exec() group of functions (which wxExecute
	calls).  So we have to put the full path.  Hopefully this won't matter, since the default installation
	of hg on the Mac always puts it in /usr/local/bin, so we can just go ahead and assume that.
 
	If hg isn't found, as well as a nonzero result from wxExecute, we get a message in our errors ArrayString 
	on the Mac and Windows, saying the command gave an error (2 in both cases).  But on Linux we don't get 
	a message back at all.
 
	In all cases if hg returns an error we get a message in errors.  So we'll need to distinguish between hg
	not being found, or being found and its execution returning an error.
*/

int  call_hg ( bool bDisplayOutput )
{
	wxString		str, str1, local_arguments;
	wxArrayString	errors;
    long			result;
	int				count, i;
	int				returnCode = 0;		// 0 = no error.  Let's be optimistic here

#ifdef	__WXMAC__
	str = _T("/usr/local/bin/hg ");
#else
	str = _T("hg ");
#endif

	// If there's a file or directory path in the arguments string, we need to escape any spaces with backslash space.  
	// We do this in a local copy so the caller can reuse the arguments string if needed.  But on Windows this doesn't
	// work, because backslash is the path separator!  It seems we need to put the whole string within quote (") marks.

	local_arguments = hg_arguments;

#ifdef __WXMSW__
	if (!local_arguments.IsEmpty())
		local_arguments = (_T("\"")) + local_arguments + (_T("\""));
#else
	local_arguments.Replace (_T(" "), _T("\\ "), TRUE);
#endif

	// hg complains if there are trailing blanks on the command line, so we only add the options and arguments if
	//  they're actually there:

	str = str + hg_command; 
	if (!hg_options.IsEmpty())
	str = str + _T(" ") + hg_options;
	if (!local_arguments.IsEmpty())
	str = str + _T(" ") + local_arguments;

//wxMessageBox (str);		// debugging

	result = wxExecute (str, hg_output, errors, 0);

	// The only indication we get that something went wrong, is a nonzero result.  It seems to always be 255, whatever the error.
	// It may mean that hg wasn't found, or it could be an illegal hg command.  The details will appear in a separate wx message or
	// will land in our errors wxArrayString.  See below.

	if (result)		// An error occurred
	{	
//wxMessageBox( _T("error!!!") );		// debugging
		returnCode = result;
	}
	else
	{		// hg's stdout will land in our hg_output wxArrayString.  There can be a number of strings.
			// Just concatenating them with a newline between looks OK so far.  We only display a message
			// with the output if we've been asked to.  Otherwise the caller will handle.
		if (bDisplayOutput) 
		{
			count = hg_output.GetCount();  
			if (count) 
			{	str1.Clear();
				for (i=0; i<count; i++)
					str1 = str1 + hg_output.Item(i) + /*_T(" ")*/ _T("\n");
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
						// It should come up shortly.
		{
			wxMessageBox (_T("An error occurred -- further information should follow."));
		}
	}

	return returnCode;
}


// Setup functions:

int  init_repository ()
{	
	hg_command = _T("init");
	return call_hg (FALSE);
}

// add_file is called when the current (new) file is to be added to version control.  No commits have been done
//  yet, so we initialize the commit count to zero.  If the file is already under version control we just
//  return without doing anything -- we must leave the commit count alone.

int  add_file (wxString fileName)
{
	CAdapt_ItApp*	pApp = &wxGetApp();
	
	if (pApp->m_commitCount >= 0)       // already under version control - do nothing
        return 0;
	
	pApp->m_commitCount = 0;
	
	hg_command = _T("add");
	hg_arguments = fileName;
	return call_hg (FALSE);
}


// add_all_files() adds all documents in the Adaptations folder to version control.  They should all end in
//  "*.xml" so we pass that to hg.  Note this gives them all "A" status.  They're not really truly under
//  version control until the first commit.

int  add_all_files()
{	
	hg_command = _T("add");
	hg_arguments = _T("glob:*.xml");		// Note: unlike in a terminal, we do need to explicitly put glob: !
	
	return call_hg (FALSE);
}

// remove_file() is called to remove the given file from version control.  The file's not deleted.

int  remove_file (wxString fileName)
{
	hg_command = _T("forget");
	hg_arguments = fileName;
	return call_hg (FALSE);
}

// remove_project() removes the whole current project from version control.  Nothing's actually deleted.

int  remove_project()
{
	hg_command = _T("forget");
	hg_arguments = _T("glob:*.xml");		// Note: unlike in a terminal, we do need to explicitly put glob: !
	return call_hg (FALSE);
}	


// get_prev_revision() uses hg's log to get the previous changeset number.  The first time in we pass TRUE and
//  we know to read the log and initialize the counter.  Then on each subsequent call we read earlier log entries
//  and return the previous changeset number.  If we hit the end, we return -1.  If call_hg() returned an error,
//  we return -2.

int  get_prev_revision ( bool bFirstTime, wxString fileName )
{
	wxString	nextLine, str;
//	int			test;

	if (bFirstTime)
	{
		hg_output.Clear();
		hg_command = _T("log");
		hg_arguments = fileName;
		if (call_hg (FALSE))
			return -2;				// maybe hg's not installed!
		hg_lineNumber = 0;
		hg_count = hg_output.GetCount();
	}

// The log has multiple entries, each with a first line that looks like
// changeset:  9:<big hex number>
// We loop till we get such a line, then parse out the number.

//	test = hg_count;
//	test = hg_lineNumber;
	
	while (TRUE) 
	{
		if (hg_lineNumber >= hg_count)  return -1;		// we've hit the end
	
		nextLine = hg_output.Item(hg_lineNumber);  hg_lineNumber += 1;
		if (nextLine.Find(_T("changeset")) != wxNOT_FOUND)  break;
	}
	str = nextLine.AfterFirst (_T(':'));
	str = str.BeforeLast (_T(':'));
	hg_lineNumber += 1;
//	test = wxAtoi(str);
	return  wxAtoi (str);		
		// if for some reason the line didn't contain a number, this returns zero, which we can live with
}


bool  commit_valid()
{
	CAdapt_ItApp*	pApp = &wxGetApp();

	if (pApp->m_AIuser == NOOWNER || pApp->m_owner == NOOWNER)  return TRUE;
	
	if (pApp->m_AIuser != pApp->m_owner)
	{
		wxMessageBox ( _T("Sorry, it appears the owner of this document is ") + pApp->m_owner 
					  + _T(" but the currently logged in user is ") + pApp->m_AIuser 
					  + _T(".  Only the document's owner can commit changes to it.") );
		return FALSE;
	}
	else  return TRUE;
}
			

int  commit_file (wxString fileName)
{
	CAdapt_ItApp*	pApp = &wxGetApp();
	wxString		local_owner = pApp->m_owner;
	int				commitCount = pApp->m_commitCount;
	
	if (!commit_valid()) return -1;

#ifndef __WXMSW__
	local_owner.Replace (_T(" "), _T("\\ "), TRUE);
#endif
	
	hg_command = _T("commit");
//	hg_options << _T("-m \"revision ");
	hg_options << _T("-m \"");
	hg_options << commitCount;
	
	if (commitCount == 1)
		hg_options << _T(" commit\" -u ");
	else
		hg_options << _T(" commits\" -u ");

	hg_options << local_owner;
	hg_arguments = fileName;
	return call_hg (FALSE);
}

int  commit_project()
{
	if (!commit_valid()) return -1;

	hg_command = _T("commit");
	hg_options = _T("-m \"whole project commit\"");
	return call_hg (FALSE);
}

int  log_file (wxString fileName)
{
	hg_output.Clear();
	hg_command = _T("log");
	hg_arguments = fileName;
	return call_hg (TRUE);
}

int  log_project()
{
	hg_output.Clear();
	hg_command = _T("log");
	return call_hg (TRUE);
}

// For reverting, instead of passing a filename, we assume the current file.  Probably we'll do this
// for the other functions as well.

int  revert_to_revision ( int revision )
{
	CAdapt_ItApp*	pApp = &wxGetApp();
	wxString		fileName = pApp->m_curOutputFilename;
	wxString		strRevision = wxString::Format	(_T("%i"), revision);		

	hg_command = _T("revert");
	hg_arguments = fileName;	// ????MUST CONVERT revision TO wxString!!!!
	hg_options = _T("-C -r ") + strRevision;
	
	return call_hg (FALSE);
}



// Main function.  This is the only one called from outside this file.
//  It just clears the global wxStrings, cd's to the current repository, then dispatches to the 
//  appropriate function to do the work.  We return as a result whatever that function returns.
//  If the cd fails, this means that AdaptIt doesn't have a current project yet.  We complain and bail out.

int  CallDVCS ( int action, int parm )
{
	wxString		str;
	CAdapt_ItApp*	pApp = &wxGetApp();
	int				result;
	bool			bResult;
    wxString		saveWorkDir = wxGetCwd();			// save the current working directory
	
	hg_command.Clear();  hg_options.Clear();  hg_arguments.Clear();				// Clear the globals

// Next we cd into our repository.  We use wxSetWorkingDirectory() and spaces in pathnames are OK.

	str = pApp->m_curAdaptionsPath;
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
			hg_output.Clear();
			hg_command = _T("version");
			result = call_hg (TRUE);
			break;
		
		case DVCS_INIT_REPOSITORY:	result = init_repository();							break;

		case DVCS_ADD_FILE:			result = add_file (pApp->m_curOutputFilename);		break;
		case DVCS_ADD_ALL_FILES:	result = add_all_files();							break;
			
		case DVCS_REMOVE_FILE:		result = remove_file(pApp->m_curOutputFilename);	break;
		case DVCS_REMOVE_PROJECT:	result = remove_project();							break;

		case DVCS_COMMIT_FILE:		result = commit_file (pApp->m_curOutputFilename);	break;
		case DVCS_REVERT_FILE:		result = revert_to_revision (parm);					break;

		case DVCS_LOG_FILE:			result = log_file (pApp->m_curOutputFilename);		break;
		case DVCS_LOG_PROJECT:		result = log_project();								break;
		case DVCS_LATEST_REVISION:	result = get_prev_revision (TRUE, pApp->m_curOutputFilename);		break;
		case DVCS_PREV_REVISION:	result = get_prev_revision (FALSE, pApp->m_curOutputFilename);		break;

		default:
			wxMessageBox (_T("Internal error - illegal DVCS command"));
			result = -1;
	}

	wxSetWorkingDirectory (saveWorkDir);		// restore working directory back to what it was
	return result;
}

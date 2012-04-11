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
#include "scrollingwizard.h" // whm added 13Nov11 for wxScrollingWizard - need to include this here before "StartWorkingWizard.h" below
#include "StartWorkingWizard.h"
//#include "SourceBundle.h"
#include "Pile.h"
#include "Layout.h"
#include "WhichBook.h"
#include "helpers.h"
#include "errno.h"

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
	is our repository.  Then after that we can just use filenames without having to fully path them out, and wildcards work.
 */
 
 
 
wxString		hg_command, hg_options, hg_arguments;


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

int  call_hg()
{
	wxString		str, str1, local_arguments;
	wxArrayString	output, errors;
    long			result;
	int				count, i;
	int				returnCode = 0;		// 0 = no error.  Let's be optimistic here

#ifdef	__WXMAC__
	str = _T("/usr/local/bin/hg ");
#else
	str = _T("hg ");
#endif

	// If there's a file or directory path in the arguments string, we need to replace any spaces with backslash.  
	// We do this in a local copy so the caller can reuse the arguments string if needed.  We don't need to do
	// this on Windows, and indeed we mustn't, because backslash is the path separator!

	local_arguments = hg_arguments;
#ifndef __WXWIN__
	local_arguments.Replace (_T(" "), _T("\\ "), TRUE);
#endif

	// hg complains if there are trailing blanks on the command line, so we only add the options and arguments if
	//  they're actually there:

	str = str + hg_command; 
	if (!hg_options.IsEmpty())
	str = str + _T(" ") + hg_options;
	if (!local_arguments.IsEmpty())
	str = str + _T(" ") + local_arguments;

//	wxMessageBox(str);

	result = wxExecute (str, output, errors, 0);

	// The only indication we get that something went wrong, is a nonzero result.  It seems to always be 255, whatever the error.
	// It may mean that hg wasn't found, or it could be an illegal hg command.  Eventually we should suss this out a bit
	// more, using the errors wxArrayString.

	if (result)		// An error occurred
	{	
wxMessageBox( _T("error!!!") );
		returnCode = result;
	}
	else
	{		// hg's stdout will land in our output wxArrayString.  There can be a number of strings.
			// Just concatenating them with a space between looks OK so far.
		count = output.GetCount();  
		if (count) 
		{	str1.Clear();
			for (i=0; i<count; i++)
				str1 = str1 + output.Item(i) + /*_T(" ")*/ _T("\n");
			wxMessageBox (str1);
		}
	}
	
// If anything landed in our errors wxArrayString, we'll display it.  We'll probably need to enhance this a bit eventually.

	count = errors.GetCount();
	if (count)
	{	str1.Clear();
		for (i=0; i<count; i++)
			str1 = str1 + errors.Item(i) + _T(" ");
			
		wxMessageBox (str1);
	}

	return returnCode;
}


// Setup functions:

int  init_repository ()
{	
	hg_command = _T("init");
	return call_hg();
}

// add_file is called when the current (new) file is to be added to version control.

int  add_file (wxString fileName)
{
	hg_command = _T("add");
	hg_arguments = fileName;
	return call_hg();
}

// add_all_files() adds all documents in the Adaptations folder to version control.  They should all end in
//  "*.xml" so we pass that to hg.  Note this gives them all "A" status.  They're not really truly under
//  version control until the first commit.

int  add_all_files()
{	
	hg_command = _T("add");
	hg_arguments = _T("glob:*.xml");		// Note: unlike in a terminal, we do need to explicitly put glob: !
	
	return call_hg();
}

// remove_file() is called to remove the given file from version control.  The file's not deleted.

int  remove_file (wxString fileName)
{
	hg_command = _T("forget");
	hg_arguments = fileName;
	return call_hg();
}

// remove_project() removes the whole current project from version control.  Nothing's actually deleted.

int  remove_project()
{
	hg_command = _T("forget");
	hg_arguments = _T("glob:*.xml");		// Note: unlike in a terminal, we do need to explicitly put glob: !
	return call_hg();
}	

int  commit_file (wxString fileName)
{
	hg_command = _T("commit");
	hg_options = _T("-m \"single file commit\"");
	hg_arguments = fileName;
	return call_hg();
}

int  commit_project()
{
	hg_command = _T("commit");
	hg_options = _T("-m \"whole project commit\"");
	return call_hg();
}

int  log_file (wxString fileName)
{
	hg_command = _T("log");
	hg_arguments = fileName;
	return call_hg();
}

int  log_project()
{
	hg_command = _T("log");
	return call_hg();
}

// For reverting, instead of passing a filename, we assume the current file.  Probably we'll do this
// for the other functions as well.

int  revert_current_file()
{
	CAdapt_ItApp*	pApp = &wxGetApp();
	wxString		fileName = pApp->m_curOutputFilename;
	int				revision = pApp->m_LatestRevisionNumber - 1;	// We keep things simple by only ever reverting
																//  to the previous revision.
	hg_command = _T("revert");
	hg_arguments = fileName;	// ????MUST CONVERT revision TO wxString!!!!

	// NOT FINISHED YET
	revision = revision;
	return 0;
}



// Main function.  This is the only one called from outside this file.
//  It just clears the global wxStrings, cd's to the current repository, then dispatches to the 
//  appropriate function to do the work.  We return as a result whatever that function returns.
//  If the cd fails, this means that AdaptIt doesn't have a current project yet.  We complain and bail out.

int  CallDVCS ( int action )
{
	wxString		str;
	CAdapt_ItApp*	pApp = &wxGetApp();
	int				result;
	bool			bResult;
    wxString		saveWorkDir = wxGetCwd();			// save the current working directory
	
	hg_command.Clear();  hg_options.Clear();  hg_arguments.Clear();		// Clear the global wxStrings

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
			hg_command = _T("version");
			result = call_hg();
			break;
		
		case DVCS_INIT_REPOSITORY:	result = init_repository();							break;

		case DVCS_ADD_FILE:			result = add_file (pApp->m_curOutputFilename);		break;
		case DVCS_ADD_ALL_FILES:	result = add_all_files();							break;
			
		case DVCS_REMOVE_FILE:		result = remove_file(pApp->m_curOutputFilename);	break;
		case DVCS_REMOVE_PROJECT:	result = remove_project();							break;

		case DVCS_COMMIT_FILE:		result = commit_file (pApp->m_curOutputFilename);	break;
		case DVCS_COMMIT_PROJECT:	result = commit_project();							break;

		case DVCS_LOG_FILE:			result = log_file (pApp->m_curOutputFilename);		break;
		case DVCS_LOG_PROJECT:		result = log_project();								break;
		
		case DVCS_REVERT_FILE:		result = revert_current_file();						break;

		default:
			wxMessageBox (_T("Internal error - illegal DVCS command"));
			result = -1;
	}

	wxSetWorkingDirectory (saveWorkDir);		// restore working directory back to what it was
	return result;
}


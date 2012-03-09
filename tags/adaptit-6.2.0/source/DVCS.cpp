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


int  CallDVCS ( int action )
{
	wxString		str, str1, hg_command;
	wxArrayString	output, errors;
	char			command[1024];
    long			result;
	int				count, i;
	int				returnCode = 0;		// 0 = no error.  Let's be optimistic here


	switch (action)
	{
		case DVCS_CHECK:
			hg_command = _T("version");
			break;
		default:
			hg_command = _T("illegal");
	}

	// For some utterly unaccountable reason, under Windows or Linux, we can just find "hg" since if it's
	// properly installed, it's in one of the paths listed in the PATHS environment variable.  However
	// on the Mac, although its location is in PATHS, and we can just type "hg <whatever>" in a terminal
	// window and it gets found, it doesn't get found by the exec() group of functions (which wxExecute
	// calls).  So we have to put the full path.  Hopefully this won't matter, since the default installation
	// of hg on the Mac always puts it in /usr/local/bin.
	
	// If hg isn't found, as well as a nonzero result from wxExecute, we get a message in our errors ArrayString 
	// on the Mac and Windows, saying the command gave an error (2 in both cases).  But on Linux we don't get 
	// a message back at all.
	// In all cases if hg returns an error we get a message in errors.  So we'll need to distinguish between hg
	// not being found, or being found and it's execution returning an error.


#ifdef	__WXMAC__
	str = _T("/usr/local/bin/hg ");
#else
	str = _T("hg ");
#endif

	str = str + hg_command;
	strcpy ( command, (const char*) str.mb_str(wxConvUTF8) );		// convert s to ASCII char string in buf

	result = wxExecute ( str, output, errors, 0 );

	// The only indication we get that something went wrong, is a nonzero result.  This may mean that
	// hg wasn't found, or it could be an illegal hg command.  Eventually we should suss this out a bit
	// more.

    if (result)		// An error occurred
    {	
		wxMessageBox (_T("We couldn't find Mercurial.  Please check that it's installed properly."));
		returnCode = 1;
	}
    else
	{				// hg's stdout will land in our output wxArrayString.  There can be a number of strings.
					// Just concatenating them with a space between looks OK so far.
		count = output.GetCount();  str1.Clear();
		for (i=0; i<count; i++)
			str1 = str1 + output.Item(i) + _T(" ");

        wxMessageBox (str1);
	}

	// If anything landed in our errors wxArrayString, we'll display it:
	count = errors.GetCount();
	if (count)
	{	str1.Clear();
		for (i=0; i<count; i++)
			str1 = str1 + errors.Item(i) + _T(" ");

		wxMessageBox (str1);
	}
	return returnCode;
}



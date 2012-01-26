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

#if defined(TEST_DVCS)

int     count = 0;

int spawn (char* program, char** arg_list)
{
    pid_t   child_pid;

    child_pid = fork();

    if (child_pid != 0)
    {           // This is the parent process continuing.  We just return the child pid.
        return (child_pid);
    }
    else
    {           // This is the child.  We execute the given program, passing the arg list.
        execvp (program, arg_list);
    // we get here only on an error:
        return (-1);        // -1 can't be a pid

    }
}


void  CallDVCS ( int action )
{
#ifdef blogggs  -- old stuff but we might need something like this on Windows...

    char        hg_loc[]    = "hg";
    char*       hg_args[]   =
    {
        "hg",
        "version",
        NULL
    };

    pid_t       child_pid;

    int         returnvalue;
    wxString    msg;
    int         child_status;


    child_pid = spawn (hg_loc, hg_args);
    if (child_pid == -1)
    {
        wxMessageBox (_T("Wheels off!!"));
        return;
    }

// To clean up properly, we now wait till hg has finished:
    child_pid = waitpid (child_pid, &child_status, 0);
    msg = msg.Format (_T("main prog continues - child pid - %d"), child_pid);

#endif



    FILE*     stream;

    char      buf[BUFSIZ+1];
    wxString  s, tmpStr;
    int       cnt;
    int       result;

    fflush (NULL);

// Here we fire up hg as a subprocess, open stream as a pipe to its stdout, and wait for it to finish:
    stream = popen ("/usr/local/bin/hg version", "r");

// Read hg's output:
    while (!feof(stream))
    {
        cnt = fread (buf, sizeof(char), BUFSIZ, stream);
        if (cnt > 0)
        {
            tmpStr = wxString (buf, wxConvUTF8, cnt);
            s += tmpStr;
        }
    }

    fflush (NULL);
    result = pclose (stream);

// The only indication we get that something went wrong, is a nonzero result:
    if (result)
    {               // An error occurred -- probably hg isn't where it should be
        wxMessageBox (_T("We couldn't find Mercurial.  Please check that it's installed properly."));
    }
    else
        wxMessageBox (s);


}

#endif

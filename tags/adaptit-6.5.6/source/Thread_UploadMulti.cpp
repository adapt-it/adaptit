/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_UploadMulti.cpp
/// \author			Bruce Waters
/// \date_created	1 March 2013
/// \rcs_id $Id: Thread_UploadMulti.cpp 3126 2013-03-01 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the Thread_UploadMulti class.
/// The Thread_UploadMulti is a thread class for uploading a subset of normal (i.e. not
/// pseudo-deleted) local KB entries (i.e. several src/tgt pairs) to the remote kbserver
/// database. Uploading the normal entries of the local KB will be done by creating and
/// firing off many of these threads - up to 50 - so as not to overload the capacitiy of
/// slow processors to clear the threads quickly enough so that memory does not become full
/// of them. Only normal entries from the local KB which are not yet in the remote DB are
/// included in these upload threads.
/// This thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes. It is created on the heap. The need for a mutex is
/// avoided by ensuring that everything the thread needs for its job is contained
/// internally before it is Run().
/// There is a possibility, small though, of one of the entries being uploaded already
/// being in the KB (because another user caused that to happen between testing for those
/// to be sent, and the actual sending of them) - that would cause the createentry attempt
/// to fail, and the payload of entries would fail to all be entered into the remote DB. So
/// we need to alert the user with a wxMessage() in the event of such failure. The remedy
/// is to tell the user, in that message, to try again when noone else is doing any
/// uploading. We can also advise the user, when clicking the Send All Entries button, to
/// do so when noone else is trying to upload - either manually or by doing adapting.
/// Having a caution in the dialog over the button is probably a good way to do that.
/// \derivation		The Thread_UploadMulti class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Thread_UploadMulti.h"
#endif

#if defined(_KBSERVER)

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

#include "wx/thread.h"

// other includes
#include "BString.h"
#include "KbServer.h"
#include "Thread_UploadMulti.h"
#include "Adapt_It.h"

Thread_UploadMulti::Thread_UploadMulti():wxThread()
{
    // On creation, the creating function needs to then set the public variables m_pKbSvr,
    // and m_entry, if the thread is to actually do something useful
	m_pKbSvr = NULL;
	//m_pApp = &wxGetApp();
}

Thread_UploadMulti::~Thread_UploadMulti()
{
}

void Thread_UploadMulti::OnExit()
{
}

void* Thread_UploadMulti::Entry()
{
	rv = CURLE_OK;
	rv = m_pKbSvr->BulkUpload(m_threadIndex, m_url, m_username, m_password, m_jsonUtf8Str);
	if (rv != CURLE_OK)
	{
		if (rv == CURLE_HTTP_RETURNED_ERROR)
		{
			// the recursion support when there was an error -- nothing to do here
			;
		}
		else if (rv == CURLE_COULDNT_RESOLVE_HOST)
		{
// ******* TODO  ****** -- a localizable error message -- tell the user the connection to the kbserver
// has been lost, maybe temporarily, or not achieved yet, test web connectivity and make 
// sure right url was typed in
		}
		; // any other errors, just ignore them? or give an 'undefined error' message
	}
	return (void*)NULL;
}

#endif // for _KBSERVER

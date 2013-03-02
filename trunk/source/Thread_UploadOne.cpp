/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_UploadOne.cpp
/// \author			Bruce Waters
/// \date_created	1 March 2013
/// \rcs_id $Id: Thread_UploadOne.cpp 3126 2013-03-01 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the Thread_UploadOne class.
/// The Thread_UploadOne is a thread class for uploading a single normal (i.e. not
/// pseudo-deleted) local KB entry (i.e. a src/tgt pair) to the remote kbserver database.
/// Uploading the normal entries of the local KB will be done by creating and firing off
/// many of these threads - perhaps slowed down in their creation by a timer, so as not to
/// overload the capacitiy of slow processors to clear the threads quickly enough so that
/// memory does not become full of them. Network latency determines how many will be
/// hanging around, and, of course, the timer interval.
/// The thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes.
/// It is created on the heap, and uploads just a single new entry to the KB server.
/// Errors are ignored - and of course anytime the src/tgt pair is sent to a DB which
/// already has that pair, the creation attempt for the entry in the remote DB will fail.
/// Well, we just don't care - we just want to get the ones not already in there, into
/// there - and those are the ones that will succeed.
/// \derivation		The Thread_UploadOne class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Thread_UploadOne.h"
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
#include "KbServer.h"
#include "Thread_UploadOne.h"
#include "Adapt_It.h"

extern int countCurrentUploadThreadDeaths;

Thread_UploadOne::Thread_UploadOne():wxThread()
{
	// On creation, the creating function needs to then set the public variables m_pKbSvr,
	// and m_entry, if the thread is to actually do something useful
	m_pKbSvr = NULL;
	//m_pApp = &wxGetApp();
}

Thread_UploadOne::~Thread_UploadOne()
{
}

void Thread_UploadOne::OnExit()
{
}

void* Thread_UploadOne::Entry()
{
	/* params for CreateEntry_Minimal()
	KbServer*			m_pKbSvr;
	KbServerEntry		m_entry;
	wxString			m_kbType;
	wxString			m_password;
	wxString			m_username;
	wxString			m_srcLangCode;
	wxString			m_tgtLangCode;
	wxString			m_url;
	*/
	wxASSERT(m_pKbSvr != NULL && !m_entry.source.IsEmpty());
	int rv = CURLE_OK;
	rv = m_pKbSvr->CreateEntry_Minimal(m_entry, m_kbType, m_password, m_username,
										m_srcLangCode, m_tgtLangCode, m_url);
	if (rv != CURLE_OK)
	{
		// do nothing on error
		;
	}
	return (void*)NULL;
}

#endif // for _KBSERVER

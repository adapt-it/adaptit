/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_ChangedSince.cpp
/// \author			Bruce Waters
/// \date_created	12 February 2013
/// \rcs_id $Id: Thread_ChangedSince.cpp 3092 2013-02-12 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the Thread_ChangedSince class.
/// The Thread_ChangedSince is a "detached" thread class for downloading a bunch of
/// database entries from kbserver, to be merged to the local KB. The download is
/// incremental, that is, those entries added remotely since the time of the last
/// ChangedSince() type of download - whether done by a thread, or manually
/// (synchronously). This thread downloads to a queue, the latter is protected by a mutex,
/// and the main thread at each idle even removes the frontmost struct and merges it's
/// contents to the local KB. This thread does not "see" the latter, it only sees the end
/// of the queue and adds each struct (actually, ptr to struct) to its end.
/// The thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes.
/// \derivation		The Thread_ChangedSince class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Thread_ChangedSince.h"
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
//#include "Adapt_It.h"
//#include "KB.h"
#include "KbServer.h"
#include "Thread_ChangedSince.h"

Thread_ChangedSince::Thread_ChangedSince():wxThread()
{
	m_pApp = &wxGetApp();
	m_pKbSvr = m_pApp->GetKbServer(m_pApp->GetKBTypeForServer());
}

Thread_ChangedSince::~Thread_ChangedSince()
{

}

void Thread_ChangedSince::OnExit()
{

}

void* Thread_ChangedSince::Entry()
{
	// Note: the static s_QueueMutex is used within ChangedSince_Queued() at the point
	// where an entry (in the form of a pointer to struct) is being added to the end of
	// the queue m_queue in the m_pKbSvr instance
	wxString timeStamp = m_pKbSvr->GetKBServerLastSync();
	int rv = m_pKbSvr->ChangedSince_Queued(timeStamp); // 2nd param, bDoTimestampUpdate is default TRUE
	// If rv = 0 (ie. no error) is returned, the server's downloaded timestamp will have
	// been used at the end of ChangedSince_Queued() to update the stored value at the
	// client end , so it doesn't need to be done here - but that's provided
	// bDoTimestampUpdate param takes the default TRUE value; if an explicit
	// FALSE were passed in, no timestamp update would be done
	if (rv != 0)
	{
		; // do nothing, error handling is at a lower level
	}
	return (void*)NULL;
}

#endif // for _KBSERVER

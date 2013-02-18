/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_PseudoDelete.cpp
/// \author			Bruce Waters
/// \date_created	18 February 2013
/// \rcs_id $Id: Thread_PseudoDelete.cpp 3101 2013-02-18 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the Thread_PseudoDelete class.
/// The Thread_PseudoDelete is a thread class for PUTing a 1 value in the deleted flag
/// within a kbserver entry which is for eventual sharing; it decouples the transmission
/// from the user's normal adapting work, which is needed because of high network latency
/// causing unacceptable delays in the responsiveness of the GUI for the interlinear
/// layout.
/// The thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes.
/// It is created on the heap, and changes the value of deleted in just a single entry 
/// of the kbserver database
/// \derivation		The Thread_PseudoDelete class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Thread_PseudoDelete.h"
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
#include "Adapt_It.h"
#include "KB.h"
#include "KbServer.h"
#include "Thread_PseudoDelete.h"

Thread_PseudoDelete::Thread_PseudoDelete():wxThread()
{
	//m_pApp = &wxGetApp();
	// The location which creates and fires off the thread should set
	// m_source and m_translation after creating the thread object and 
	// before calling Run()
	m_translation.Empty(); // default, caller should set it after creation
}

Thread_PseudoDelete::~Thread_PseudoDelete()
{

}

void Thread_PseudoDelete::OnExit()
{

}

void* Thread_PseudoDelete::Entry()
{
	long entryID = 0; // initialize (it might not be used)
	wxASSERT(!m_source.IsEmpty()); // the key must never be an empty string
	int rv;
	rv = m_pKbSvr->LookupEntryFields(m_source, m_translation);
	if (rv == CURLE_HTTP_RETURNED_ERROR)
	{
        // If the lookup failed, we must assume it was because there was no matching entry
        // (ie. HTTP 404 was returned) - in which case we will do no more and let the
        // thread die. The alternative would be the following, which is much to much to be
        // worth the bother....
		// 1. create a normal entry using Thread_CreateEntry()
		// 2. look it up using LookupEntryFields() to get the entrie's ID
		// 3. pseudo-delete the new entry using PseudoDeleteOrUndeleteEntry(), passing in 
		// doDelete enum value -- a total of 4 latency-laden calls. No way! (The chance of
		// the initial lookup failing is rather unlikely, because pseudo-deleting is only
		// done via the KB Editor dialog, and that should have put the remote database
		// into the correct state previously, making an error here unlikely.)
		return (void*)NULL;
	}
	else
	{
		// No error from the lookup, so get the entry ID, and the value of the deleted flag
		KbServerEntry e = m_pKbSvr->GetEntryStruct(); // accesses m_entryStruct
		entryID = e.id; // an delete of a normal entry will need this value
#if defined(_DEBUG)
		wxLogDebug(_T("LookupEntryFields in Thread_PseudoDelete: id = %d , source = %s , translation = %s , deleted = %d , username = %s"),
			e.id, e.source, e.translation, e.deleted, e.username);
#endif
		// If the remote entry has 0 for the deleted flag's value, then go ahead and
		// delete it; but if it has 1 already, there is nothing to do except let the
		// thread die
		if (e.deleted == 0)
		{
			// do a pseudo-delete here, use the entryID value above (reuse rv)
			rv = m_pKbSvr->PseudoDeleteOrUndeleteEntry(entryID, doDelete);
		}
	}
	return (void*)NULL;
}

bool Thread_PseudoDelete::TestDestroy()
{
  return true;
}

#endif // for _KBSERVER


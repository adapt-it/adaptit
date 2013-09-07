/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_DoEntireKbDeletion.cpp
/// \author			Bruce Waters
/// \date_created	24 August 2013
/// \rcs_id $Id: Thread_DoEntireKbDeletion.cpp 3092 2013-02-12 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the Thread_DoEntireKbDeletion class.
/// The Thread_DoEntireKbDeletion is a "detached" thread class for deleting all
/// the entries for a specified kb database defined by a srclanguage/nonsourcelanguage
/// code pair; and after all those are deleted so that the code pair owns no entries, the
/// code pair definition is removed as the last step. This thread for a large (eg. 40,000
/// entry) database, in a high latency environment (e.g. approx 5 secs latency per entry
/// deletion), may take days to complete. It can be safely killed at any point, and
/// restarted in a later session - each entry deleted is gone forever, even if the thread
/// does not complete in a single session. Internally, it runs a loop in which a child
/// thread is fired to delete a single entry each time. It tracks how many have been
/// deleted, and the loop completes when the deletion count equals the queue size, or when
/// there is a http error (such as 404 Not Found). At that point the KB definition in the
/// kb table can be deleted without causing a foreign key constraint error.
/// \derivation		The Thread_DoEntireKbDeletion class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////


// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Thread_DoEntireKbDeletion.h"
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
#include "KbServer.h"
#include "Thread_DoEntireKbDeletion.h"

Thread_DoEntireKbDeletion::Thread_DoEntireKbDeletion(KbServer* pStatelessKbServer, 
			long kbDefinitionID, size_t nQueueSize):wxThread()
{
	m_pApp = &wxGetApp();
	m_pKbSvr = pStatelessKbServer;
	m_TotalEntriesToDelete = nQueueSize;
	m_idForKbDefinitionToDelete = kbDefinitionID;
}

Thread_DoEntireKbDeletion::~Thread_DoEntireKbDeletion()
{

}

void Thread_DoEntireKbDeletion::OnExit()
{

}

void* Thread_DoEntireKbDeletion::Entry()
{
	// Do the work here, in a loop. Note: the last step, deleting the kb definition,
	// should be done from this thread too - there's no need to post an event to get it
	// done in the main thread; but the status bar should track N of M deletions, so that
	// if the Manager is open, the administrator will know there's no point in trying a
	// new deletion attempt until all M have been deleted - and that might not happen in
	// the one session
	
	int rv = 0; // initialize return value, when loop completes, rv will return the
				// value CURLE_HTTP_RETURNED_ERROR - this is our cue to break from the loop


// TODO -- the code



	return (void*)NULL;
}

#endif // for _KBSERVER

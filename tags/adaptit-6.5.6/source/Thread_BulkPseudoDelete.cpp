/// \project		adaptit
/// \file			Thread_BulkPseudoDelete.cpp
/// \author			Bruce Waters
/// \date_created	22 October 2013
/// \rcs_id $Id: Thread_BulkPseudoDelete.cpp 3101 2013-02-18 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the Thread_BulkPseudoDelete class.
/// Thread_BulkPseudoDelete is a thread class for PUTing a 1 value in the deleted flag
/// within a kbserver entry which is for eventual sharing - doing so for potentially
/// thousands of requested deletions from the KB Editor dialog; using a thread decouples
/// the transmissions from the user's normal adapting work, which is needed because of high
/// network latency causing unacceptable delays in the responsiveness of the GUI for the
/// interlinear layout.
/// The thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes, and clear the arrays from which it gets the source
/// phrases and target phrases for the pairs to be pseudo-deleted. (Until those arrays are
/// cleared, the user is prevented from initiating a further bulk delete.)
/// It is created on the heap, and changes the value of the deleted flag in as many entries 
/// of the kbserver database as their are entries in one or other of the input arrays
/// \derivation		The Thread_BulkPseudoDelete class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Thread_BulkPseudoDelete.h"
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
#include "KbServer.h"
#include "Thread_BulkPseudoDelete.h"

extern wxMutex s_BulkDeleteMutex;


Thread_BulkPseudoDelete::Thread_BulkPseudoDelete():wxThread()
{
	m_pApp = &wxGetApp();
	m_pKbSvr = m_pApp->GetKbServer(m_pApp->GetKBTypeForServer());

	// The location which creates and fires off the thread should set
	// the arrays, etc, after creating the thread object and 
	// before calling Run()
}

Thread_BulkPseudoDelete::~Thread_BulkPseudoDelete()
{

}

void Thread_BulkPseudoDelete::OnExit()
{

}

void* Thread_BulkPseudoDelete::Entry()
{
	int rv;
	long entryID = 0; // initialize (it might not be used)
	wxString srcPhrase;
	wxString nonsrcPhrase;
	size_t count = m_pSourcePhrasesArray->size();
	size_t index; 
	for (index = 0; index < count; index++)
	{
		srcPhrase = m_pSourcePhrasesArray->Item(index);
		nonsrcPhrase = m_pNonSourcePhrasesArray->Item(index);
		if (srcPhrase.IsEmpty())
		{
			// the key must never be an empty string
			continue;
		}

		s_BulkDeleteMutex.Lock();

		rv = m_pKbSvr->LookupEntryFields(srcPhrase, nonsrcPhrase);
		if (rv == CURLE_HTTP_RETURNED_ERROR)
		{
			// If the lookup failed, we must assume it was because there was no matching entry
			// (ie. HTTP 404 was returned) - in which case we will do no more and let the
			// thread die
			return (void*)NULL;
		}
		else
		{
			// No error from the lookup, so get the entry ID, and the value of the deleted flag
			KbServerEntry e = m_pKbSvr->GetEntryStruct(); // accesses m_entryStruct
			entryID = e.id; // an delete of a normal entry will need this value
#if defined(_DEBUG)
			wxLogDebug(_T("LookupEntryFields in Thread_BulkPseudoDelete: id = %d , source = %s , translation = %s , deleted = %d , username = %s"),
				e.id, e.source.c_str(), e.translation.c_str(), e.deleted, e.username.c_str());
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

		s_BulkDeleteMutex.Unlock();

	}
	return (void*)NULL;
}

#endif // for _KBSERVER


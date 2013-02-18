/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_PseudoUndelete.cpp
/// \author			Bruce Waters
/// \date_created	18 February 2013
/// \rcs_id $Id: Thread_PseudoUndelete.cpp 3101 2013-02-18 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the Thread_PseudoUndelete class.
/// The Thread_PseudoUndelete is a thread class for PUTing a 0 value in the deleted flag
/// within a kbserver entry which is for eventual sharing; it decouples the transmission
/// from the user's normal adapting work, which is needed because of high network latency
/// causing unacceptable delays in the responsiveness of the GUI for the interlinear
/// layout.
/// The thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes.
/// It is created on the heap, and changes the value of deleted in just a single entry 
/// of the kbserver database
/// \derivation		The Thread_PseudoUndelete class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Thread_PseudoUndelete.h"
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
#include "Thread_PseudoUndelete.h"

Thread_PseudoUndelete::Thread_PseudoUndelete():wxThread()
{
	//m_pApp = &wxGetApp();
	// The location which creates and fires off the thread should set
	// m_source and m_translation after creating the thread object and 
	// before calling Run()
	m_translation.Empty(); // default, caller should set it after creation
}

Thread_PseudoUndelete::~Thread_PseudoUndelete()
{

}

void Thread_PseudoUndelete::OnExit()
{

}

void* Thread_PseudoUndelete::Entry()
{
	long entryID = 0; // initialize (it might not be used)
	wxASSERT(!m_source.IsEmpty()); // the key must never be an empty string
	int rv;
	rv = m_pKbSvr->LookupEntryFields(m_source, m_translation);
	if (rv == CURLE_HTTP_RETURNED_ERROR)
	{
		// we've more work to do - if the lookup failed, we must assume it was because
		// there was no matching entry (ie. HTTP 404 was returned) - in which case we
		// should attempt to create it (so as to be in sync with the change just done in
		// the local KB due to the undeletion); if the creation fails, just give up and
		// let the thread die
		rv = m_pKbSvr->CreateEntry(m_source, m_translation); // kbType is supplied internally from m_pKbSvr
	}
	else
	{
		// no error from the lookup, so get the entry ID, and the value of the deleted flag
		KbServerEntry e = m_pKbSvr->GetEntryStruct(); // accesses m_entryStruct
		entryID = e.id; // an undelete of a pseudo-delete will need this value
#if defined(_DEBUG)
		wxLogDebug(_T("LookupEntryFields in Thread_PseudoUndelete: id = %d , source = %s , translation = %s , deleted = %d , username = %s"),
			e.id, e.source, e.translation, e.deleted, e.username);
#endif
		// If the remote entry has 1 for the deleted flag's value, then go ahead and
		// undelete it; but if it has 0 already, there is nothing to do except let the
		// thread die
		if (e.deleted == 1)
		{
			// do an un-pseudodelete here, use the entryID value above (reuse rv)
			rv = m_pKbSvr->PseudoDeleteOrUndeleteEntry(entryID, doUndelete);
		}
	}
	return (void*)NULL;
}

bool Thread_PseudoUndelete::TestDestroy()
{
  return true;
}

#endif // for _KBSERVER




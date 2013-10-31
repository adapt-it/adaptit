/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_CreateEntry.cpp
/// \author			Bruce Waters
/// \date_created	29 January 2013
/// \rcs_id $Id: Thread_CreateEntry.cpp 3065 2013-01-29 02:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the Thread_CreateEntry class.
/// The Thread_CreateEntry is a thread class for uploading a single new kbserver entry for
/// eventual sharing; it decouples the upload from the user's normal adapting work --
/// needed because of the potential for high network latency causing unacceptable delays in
/// the responsiveness of the GUI for the interlinear layout.
/// The thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes.
/// It is created on the heap, and uploads just a single new entry to the KB server
/// \derivation		The Thread_CreateEntry class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Thread_CreateEntry.h"
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
#include "Thread_CreateEntry.h"

extern wxMutex s_BulkDeleteMutex;

Thread_CreateEntry::Thread_CreateEntry():wxThread()
{
	//m_pApp = &wxGetApp();
	// The location which creates and fires off the thread should set
	// m_source and m_translation after creating the thread object and 
	// before calling Run()
	m_translation.Empty(); // default, caller should set it after creation
}

Thread_CreateEntry::~Thread_CreateEntry()
{

}

void Thread_CreateEntry::OnExit()
{

}

void* Thread_CreateEntry::Entry()
{
	long entryID = 0; // initialize (it might not be used)
	wxASSERT(!m_source.IsEmpty()); // the key must never be an empty string
	int rv;

	s_BulkDeleteMutex.Lock();

	rv = m_pKbSvr->CreateEntry(m_source, m_translation); // kbType is supplied internally from m_pKbSvr
	if (rv == CURLE_HTTP_RETURNED_ERROR)
	{
		// we've more work to do - may need to un-pseudodelete a pseudodeleted entry
		int rv2 = m_pKbSvr->LookupEntryFields(m_source, m_translation);
		KbServerEntry e = m_pKbSvr->GetEntryStruct();
		entryID = e.id; // an undelete of a pseudo-delete will need this value
#if defined(_DEBUG)
		wxLogDebug(_T("LookupEntryFields in Thread_CreateEntry: id = %d , source = %s , translation = %s , deleted = %d , username = %s"),
					e.id, e.source.c_str(), e.translation.c_str(), e.deleted, e.username.c_str());
#endif
		if (rv2 == CURLE_HTTP_RETURNED_ERROR)
		{
#if defined(_DEBUG)
			wxBell(); // we don't expect any error
#endif
			;
		}
		else
		{
			if (e.deleted == 1)
			{
				// do an un-pseudodelete here, use the entryID value above
				// (reuse rv2, because if it fails we'll attempt nothing additional
				//  here, not even to tell the user anything)
				rv2 = m_pKbSvr->PseudoDeleteOrUndeleteEntry(entryID, doUndelete);
			}
		}
	}

	s_BulkDeleteMutex.Unlock();

	return (void*)NULL;
}

bool Thread_CreateEntry::TestDestroy()
{
  return true;
}

#endif // for _KBSERVER

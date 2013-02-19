/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_KbEditorUpdateButton.cpp
/// \author			Bruce Waters
/// \date_created	19 February 2013
/// \rcs_id $Id: Thread_KbEditorUpdateButton.cpp 3106 2013-02-19 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the Thread_KbEditorUpdateButton class.
/// The Thread_KbEditorUpdateButton is a thread class for the support of the Update
/// button in the KB Editor dialog, in the context of KB sharing. This button allows the
/// selected target text entry to be edited in a wxTextCtrl, typically to fix a typo, and
/// the edited result replaces what was originally selected. Under the hood the KB
/// Editor's code does the following things... the original scr-tgt pair is pseudo-deleted,
/// and the new src-tgt pair is dealt with as follows:
/// (a) the user edited the target text and it has become unique (so a new normal entry is
/// put in the local KB); or
/// (b) the edited target text matches an existing normal entry in the local KB (in which
/// case no change is made to the local KB); or
/// (c) the edited target text matches a pseudo-deleted entry in the local KB (in which
/// case the pseudo-deleted local KB entry is undeleted to become a normal entry).
/// 
/// This Thread_KbEditorUpdateButton class supports scenarios (a) and (c). (Case (b) needs
/// no KB server supporting code.) What Thread_KbEditorUpdateButton does is attempt to
/// pseudo-delete the src-oldTgt entry in the remote DB, and after that completes, for (c)
/// it attempts to undelete the pseudo-deleted src-newTgt entry in the remote DB, provided
/// that the remote DB actually has that entry and has it with deleted flag set to 1 - but
/// if it isn't there (case (a)), it will instead create a normal src-newTgt entry in the
/// remote DB.
/// This thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes.
/// It is created on the heap, and potentially calls several kbserver API functions.
/// \derivation		The Thread_KbEditorUpdateButton class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Thread_KbEditorUpdateButton.h"
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
#include "Thread_KbEditorUpdateButton.h"

Thread_KbEditorUpdateButton::Thread_KbEditorUpdateButton():wxThread()
{
	//m_pApp = &wxGetApp();
	// The location which creates and fires off the thread should set
	// m_source and m_translation after creating the thread object and 
	// before calling Run()
	m_oldTranslation.Empty(); // default, caller should set it after creation
	m_newTranslation.Empty(); // default, caller should set it after creation
}

Thread_KbEditorUpdateButton::~Thread_KbEditorUpdateButton()
{

}

void Thread_KbEditorUpdateButton::OnExit()
{

}

void* Thread_KbEditorUpdateButton::Entry()
{
	long entryID = 0; // initialize (it might not be used)
	wxASSERT(!m_source.IsEmpty()); // the key must never be an empty string
	int rv;
	rv = m_pKbSvr->LookupEntryFields(m_source, m_oldTranslation);
	if (rv == CURLE_HTTP_RETURNED_ERROR)
	{
        // If the lookup failed, we must assume it was because there was no matching entry
        // (ie. HTTP 404 was returned) - in which case none of the collaborating clients
        // has seen this entry yet, and so the simplest thing is just not to bother
        // creating it here and then immediately pseudo-deleting it. That won't mess with
        // any of the other client's KBs needlessly.
		;
	}
	else
	{
		// No error from the lookup, so get the entry ID, and the value of the deleted flag
		KbServerEntry e = m_pKbSvr->GetEntryStruct(); // accesses m_entryStruct
		entryID = e.id; // a pseudo-delete will need this value
#if defined(_DEBUG)
		wxLogDebug(_T("LookupEntryFields in Thread_KbEditorUpdateButton: id = %d , source = %s , translation = %s , deleted = %d , username = %s"),
			e.id, e.source.c_str(), e.translation.c_str(), e.deleted, e.username.c_str());
#endif
		// If the remote entry has 0 for the deleted flag's value (which is what we expect),
		// then go ahead and pseud0-delete it; but if it has 1 already, there is nothing to 
		// do with this src-oldTgt pair, the pseudo-deletion is already in the remote DB
		if (e.deleted == 0)
		{
			// do a pseudo-delete here, use the entryID value from above
			rv = m_pKbSvr->PseudoDeleteOrUndeleteEntry(entryID, doDelete);
		}
	}
	//                          ***** part 2 *****
	// That takes care of the m_oldTranslation that was updated; now deal with the 
	// scr-m_newTranslation pair -- another lookup is needed...
	rv = m_pKbSvr->LookupEntryFields(m_source, m_newTranslation);
	if (rv == CURLE_HTTP_RETURNED_ERROR)
	{
        // If the lookup failed, we must assume it was because there was no matching entry
        // (ie. HTTP 404 was returned) - in which case none of the collaborating clients
        // has seen this entry yet, and so we should now create it in the remote DB now
		rv = m_pKbSvr->CreateEntry(m_source, m_newTranslation);
		// Ignore errors, if it didn't succeed, no big deal - someone else will sooner or
		// later succeed in adding this one to the remote DB.
	}
	else
	{
		// No error from the lookup, so get the entry ID, and the value of the deleted flag
		KbServerEntry e = m_pKbSvr->GetEntryStruct(); // accesses m_entryStruct
		entryID = e.id; // a pseudo-delete will need this value
#if defined(_DEBUG)
		wxLogDebug(_T("LookupEntryFields in Thread_KbEditorUpdateButton: id = %d , source = %s , translation = %s , deleted = %d , username = %s"),
			e.id, e.source.c_str(), e.translation.c_str(), e.deleted, e.username.c_str());
#endif
		// If the remote entry has 0 for the deleted flag's value - then the remote DB
		// already has this as a normal entry, so we've nothing to do here. On the other
		// hand, if the flag's value is 1 (it's currently pseudo-deleted in the remote DB)
		// then go ahead and undo the pseudo-deletion, making a normal entry
		if (e.deleted == 1)
		{
			// do an undelete of the pseudo-deletion, use the entryID value from above
			rv = m_pKbSvr->PseudoDeleteOrUndeleteEntry(entryID, doUndelete);
		}
		// Ignore errors, for the same reason as above (i.e. it's no big deal)
	}
	return (void*)NULL;
}

bool Thread_KbEditorUpdateButton::TestDestroy()
{
  return true;
}

#endif // for _KBSERVER






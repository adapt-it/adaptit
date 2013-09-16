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
#include "MainFrm.h"
#include "Thread_DoEntireKbDeletion.h"
#include "KBSharingMgrTabbedDlg.h"

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
	// the one session. Also pStatelessKbServer passed in was created on the heap, and so
	// if the kb definition gets deleted successfully in this thread, we can also do the
	// deletion of the stateless KbServer instance after that, also in this thread. Its
	// pointer should be set NULL as well, as that pointer is permanent in the CAdapt_ItApp
	// instantiation, and testing it for NULL is important for its management.
	
	CURLcode rv = (CURLcode)0; // initialize return value, when loop completes, rv will return the
	// value CURLE_HTTP_RETURNED_ERROR - but unfortunately, there may be an
	// unexpected http error -- that means we can't rely on the error as a diagnotic for
	// when the loop is completed. So we'll do a for loop, since we know how many we need
	// to delete, and count
	DownloadsQueue* pQueue = m_pKbSvr->GetDownloadsQueue();
	wxASSERT((pQueue != NULL) && (!pQueue->IsEmpty()) && (pQueue->GetCount() == m_TotalEntriesToDelete));
	DownloadsQueue::iterator iter;
	KbServerEntry* pKbSvrEntry = NULL;
	int nonsuccessCount = 0;
	size_t successCount = 0;
    // Iterate over all entry structs, and for each, do a https DELETE request to have it
	// deleted from the database. Note we do not use a nested thread - if we did, the loop
	// would generate threads so quickly that the server would be swamped trying
	// unsuccessfully to keep up. Instead, each DeleteSingleKbEntry() call is run
	// synchronously, and the next iteration doesn't begin until the present call has
	// returned its error code. How quickly the job gets done depends primarily on two
	// things: a) how many entries need to be deleted, and b) the network latency
	// currently being experienced
	for (iter = pQueue->begin(); iter != pQueue->end(); iter++)
	{
		pKbSvrEntry = *iter;
		int id = (int)pKbSvrEntry->id;
		rv = (CURLcode)m_pKbSvr->DeleteSingleKbEntry(id);

        // We don't expect any failures, but just in case there are some, count how many;
        // also, ignore any failures and keep iterating to get as many done in the one
        // session as possible. If the loop completes in the one session with a zero
        // failure count, then continue on to try removing the kb definition itself,
        // because it owns no entries and therefore no constraints would prevent it being
        // removed. But if there were failures we can't attempt the kb definition deletion
        // because the presence of owned entries would result in a foreign key constraint
        // error, which we don't want the user to see. Instead, in that case he can just
        // try the removal again at a later time - we need to put up a message to that
        // effect if so.
		if (rv != CURLE_OK)
		{
			nonsuccessCount++;
		}
		else
		{
			successCount++;
#if defined(_DEBUG)
			// track what we delete and it's ID
			wxLogDebug(_T("Thread_DoEntireKbDeletion(): id = %d, src = %s , non-src = %s"),
				pKbSvrEntry->id, pKbSvrEntry->source.c_str(), pKbSvrEntry->translation.c_str());
#endif
			// Copy it to the app member ready for display in main window at bottom
			m_pApp->m_nIterationCounter = successCount;
			if ((successCount/2)*2 == successCount)  // for initial testing, ask for feedback every 2nd deletion
			//if ((successCount/10)*10 == successCount)
			{
                // Update the value every 10th iteration, otherwise more frequently may bog
                // the process down if latency is very low
				wxCommandEvent eventCustom(wxEVT_KbDelete_Update_Progress);
				wxPostEvent(m_pApp->GetMainFrame(), eventCustom); // custom event handlers are in CMainFrame
			}
		}
	}

	// If control gets to here, we've either deleted all entries of the selected database,
	// or most of them with some errors resulting in a those entries remaining owned by
	// the selected kb definition. Only if all entries were deleted can we now attempt the
	// removal of the KB definition itself. Otherwise, tell the user some were not
	// deleted, and he'll have to retry later - and leave the definition in the Mgr list.
	if (nonsuccessCount > 0)
	{
		// There were some failures... (and the Manager GUI may not be open -- see comment
		// in the else block for details)
#if defined(_DEBUG)
			wxLogDebug(_T("Thread_DoEntireKbDeletion(): deletion errors (number of entries failing) = %d"),
				nonsuccessCount);
#endif
		


	}
	else
	{
        // Yay!! No failures. So the kb pair comprising the kb database definition can now
        // be removed in the current Adapt It session. However, we can't be sure the user
        // has the Manager GUI still open, it may be many hours since he set the removal
        // running -- how do we handle this communication problem? If the GUI is still
        // open, and the kbs page is still the active page, then we want the list updated
        // to reflect the kb definition has gone. If the GUI isn't open, we just remove the
        // definition from the kb table of the mysql database, and no more needs to be done
        // other than housekeeping cleanup (eg. clearing of the queue of KbServerEntry
        // structs, and deletion of the stateless kbserver instance used for doing this
		// work. How do we proceed...? I think a boolean on the app to say that the
		// Manager GUI is open for business, and another that the kbs page is active, and
		// we also need to check that the radiobutton setting hasn't changed (to switch
		// between a adapting kbs versus glossing kbs) on that page.

		// Delete the definition
		CURLcode result = CURLE_OK;
		result = (CURLcode)m_pKbSvr->RemoveKb((int)m_idForKbDefinitionToDelete);
		if (result != CURLE_OK)
		{
            // The definition should have been deleted, but wasn't. Tell the user to try
            // again later. (This error is quite unexpected, an English error message will
            // do). Then do the cleanup and let the thread die
			wxString msg = _T("Thread_DoEntireKbDeletion): unexpected failure to remove a shared KB (definition) after completing the removal of all the entries it owned.\n Try again later. The next attempt may succeed.");
			wxString title = _T("Error: could not remove the emptied database's definition");
			m_pApp->LogUserAction(msg);
			wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);

			// Housekeeping cleanup -- this stuff is needed whether the Manager
			// GUI is open or not
			pQueue->clear();
			delete m_pApp->m_pKbServerForDeleting; // the KbServer instance supplying services 
												   // for our deletion attempt
			m_pApp->m_pKbServerForDeleting = NULL;
			m_pApp->m_srcLangCodeOfCurrentRemoval.Empty();
			m_pApp->m_nonsrcLangCodeOfCurrentRemoval.Empty();
			m_pApp->m_kbTypeOfCurrentRemoval = -1;
			m_pApp->m_bKbSvrMgr_DeleteAllIsInProgress = FALSE;
			return (void*)NULL;
		} // end of TRUE block for test: if (result != CURLE_OK)

		// If control gets to here, then the kb definition was removed successfully from
		// the kb table. It remains to get the KB Sharing Manager gui to update to show
		// the correct result, if it is still running. If not running we have nothing to
		// do and the thread can die.
		KBSharingMgrTabbedDlg* pGUI = m_pApp->GetKBSharingMgrTabbedDlg();
		if (pGUI != NULL)
		{
			// The KB Sharing Manager gui is running, so we've some work to do here...
			// 2 custom events to be posted -- one to cause Mgr gui to update kb page,
			// and the other to make the wxStatusBar go back to one unlimited
			// field, and reset it and update the bar.




		} // end of TRUE block for test:  if (pGUI != NULL)
	}
	// Housekeeping cleanup -- this stuff is needed whether the Manager GUI is open or not
	pQueue->clear();
	delete m_pApp->m_pKbServerForDeleting; // the KbServer instance supplying services 
										   // for our deletion attempt
	m_pApp->m_pKbServerForDeleting = NULL;
	m_pApp->m_srcLangCodeOfCurrentRemoval.Empty();
	m_pApp->m_nonsrcLangCodeOfCurrentRemoval.Empty();
	m_pApp->m_kbTypeOfCurrentRemoval = -1;
	m_pApp->m_bKbSvrMgr_DeleteAllIsInProgress = FALSE;


	return (void*)NULL;
}

#endif // for _KBSERVER

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Timer_KbServerChangedSince.cpp
/// \author			Bruce Waters
/// \date_created	27 February 2013
/// \rcs_id $Id: Timer_KbServerChangedSince.cpp 3124 2013-02-27 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the Timer_KbServerChangedSince class.
/// The Timer_KbServerChangedSince class provides a shot at predetermined intervals - we
/// set the interval in the spin control in the KbSharing dialog handler class - allowed
/// values range from 1 through 10 (minutes). The Start() function, which takes the
/// interval (in milliseconds) starts the timer running, or if running already, it
/// restarts the timer with the new passed in timer interval value. The only function which
/// may be overridden is the Notify() member, and whatever work is to be done is done
/// there - our implementation only sets a boolean to true - the OnIdle() handler then
/// picks that up when true and does a ChangedSince threaded call to the remote KbServer
/// to get the new entries added since the lastsync timestamp value. These are added to a
/// KbServer::m_queue member, and the idle handler's code removes one entry per idle event
/// from the start of the queue; downloaded entries are appended to the end of the queue.
/// We use the default constructor, and so we must override Notify() in order to process
/// the EVENT_TIMER event using the class's built in event handling.
/// \derivation		The Timer_KbServerChangedSince class is derived from wxTimer.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Timer_KbServerChangedSince.h"
#endif

#if defined(_KBSERVER)

#include <wx/timer.h>
#include "Adapt_It.h"
#include "Timer_KbServerChangedSince.h"

Timer_KbServerChangedSince::Timer_KbServerChangedSince():wxTimer()
{
	m_pApp = &wxGetApp();
}

Timer_KbServerChangedSince::~Timer_KbServerChangedSince()
{
}

void Timer_KbServerChangedSince::Notify()
{
	if (this->IsRunning())
	{
		//int interval = GetInterval(); // milliseconds (uncomment out for debugging)
		m_pApp->m_bKbServerIncrementalDownloadPending = TRUE;
		// after being used for a test in OnIdle(), it is there cleared to FALSE ready for
		// the next shot after the thread has been created and fired off
	}
}

#endif

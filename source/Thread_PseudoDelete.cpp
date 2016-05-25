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
/// within a KBserver entry which is for eventual sharing; it decouples the transmission
/// from the user's normal adapting work, which is needed because of high network latency
/// causing unacceptable delays in the responsiveness of the GUI for the interlinear
/// layout.
/// The thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes.
/// It is created on the heap, and changes the value of deleted in just a single entry 
/// of the KBserver database
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

extern wxMutex s_BulkDeleteMutex;

Thread_PseudoDelete::Thread_PseudoDelete():wxThread()
{
	m_pApp = &wxGetApp();
	// The location which creates and fires off the thread should set
	// m_source and m_translation after creating the thread object and 
	// before calling Run()
	m_translation.Empty(); // default, caller should set it after creation
	m_pKbSvr = m_pApp->GetKbServer(m_pApp->GetKBTypeForServer());
}

Thread_PseudoDelete::~Thread_PseudoDelete()
{

}

void Thread_PseudoDelete::OnExit()
{
}

void* Thread_PseudoDelete::Entry()
{


	return (void*)NULL;
}

bool Thread_PseudoDelete::TestDestroy() // we don't call this
{
	return TRUE;
}


#endif // for _KBSERVER


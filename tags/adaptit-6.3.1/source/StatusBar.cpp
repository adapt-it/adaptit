/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			StatusBar.cpp
/// \author			Erik Brommers, taken from the wxStatusBar example by
///                 Vadim Zeitlin.
/// \date_created	2 October 2012
/// \date_revised	2 October 2012
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CStatusBar class.
/// The CStatusBar class extends the wxStatusBar and provides support for a
/// progress bar in the status bar area along with the status texts.
/// The progress bar represents the total progress of ALL long-running tasks;
/// this allows for multiple items to be worked on at once without getting
/// in the way of dialogs, etc.
/// Use the following methods to control the CStatusBar:
/// 1. StartProgress(): adds a long-running task to the internal list; if the
///					progress bar isn't visible, this call will display it.
/// 2. UpdateProgress(): updates the current value and message for the specified
///					task. 
/// 3. FinishProgress(): completes work on the specified task. If the task is
///					the last one being worked on, the internal progress bar counters
///					are reset and the progress bar is hidden; if not, the
///					progress bar values are adjusted to reflect the task's 
///					completion and the task is removed from the internal list.
/// \derivation		wxStatusBar
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

// other includes
#include <wx/statusbr.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/dynarray.h>
#include <wx/arrimpl.cpp>
#include "StatusBar.h"

WX_DEFINE_OBJARRAY(PIArray);

BEGIN_EVENT_TABLE(CStatusBar, wxStatusBar)
    EVT_SIZE(CStatusBar::OnSize)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// CProgressItem
// ----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// Constructor (CProgressItem destructor is in the the header - StatusBar.h)
///////////////////////////////////////////////////////////////////////////////////////
CProgressItem::CProgressItem(const wxString& title, const wxString& message, int maximum)
{
	m_title = title;
	m_message = message;
	m_curValue = 0;
	m_maxValue = maximum;
}

// ----------------------------------------------------------------------------
// CStatusBar
// ----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// Constructor
///////////////////////////////////////////////////////////////////////////////////////
CStatusBar::CStatusBar(wxWindow *parent)
           : wxStatusBar(parent, wxID_ANY)
{
    static const int widths[Field_Max] = { -1, 100, 100 };

    SetFieldsCount(Field_Max);
    SetStatusWidths(Field_Max, widths);

	m_Gauge = new wxGauge(this, wxID_ANY, 200, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL|wxNO_BORDER);
	m_Gauge->Hide(); // hide the gauge initially
}

///////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// Destructor
///////////////////////////////////////////////////////////////////////////////////////
CStatusBar::~CStatusBar()
{
	// clean up any open CProgressItems (they should be taken care of in FinishProgress(),
	// but just in case...)
	int count = (int)m_items.GetCount();
	CProgressItem *pi = NULL;
	if (count > 0)
	{
		for (int i=(count - 1); i>=0; i--)
		{
			// delete all the items in the list
			pi = m_items.Item(i);
			if (pi != NULL)
				delete pi;
		}
		m_items.Clear();
	}
	if (m_Gauge != NULL)
		delete m_Gauge;
}

// ----------------------------------------------------------------------------
// Event Handlers
// ----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// Resize event handler. Redraws the progress bar (m_Gauge) at the far right field 
/// of the status bar.
///////////////////////////////////////////////////////////////////////////////////////
void CStatusBar::OnSize(wxSizeEvent& event)
{
	if (!m_Gauge)
	{
		return;
	}

    wxRect rect;
    GetFieldRect(Field_Gauge, rect);
	m_Gauge->SetSize(rect.x + 2, rect.y + 2, rect.width - 4, rect.height - 4);

    event.Skip();
}

// ----------------------------------------------------------------------------
// Progress Bar - related helper methods
// ----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// Adds a long-running task (a ProgressItem object) to the internal PIArray. This 
/// ProgressItem object contains a similar set of data as the wxProgressDialog -- title,
/// current status message, current value and maximum value (or range).
/// When an object is added via StartProgress(), the embedded progress bar is diaplayed,
/// and the range of the progress bar is expanded to hold the range + whatever other items
/// are being worked on.
///////////////////////////////////////////////////////////////////////////////////////
void CStatusBar::StartProgress(const wxString& title, const wxString& message, int maximum)
{
	// add this progress item to our array
	CProgressItem *pi = new CProgressItem(title, message, maximum);
	m_items.Add(pi);
	// update and show the progress bar
	if (m_items.GetCount() == 1)
	{
		// only working on this item -- set the range to our maximum
		m_Gauge->SetRange(maximum);
	}
	else 
	{
		// working on several items -- add the max to the range
		m_Gauge->SetRange(m_Gauge->GetRange() + maximum);
	}
	m_Gauge->SetToolTip(message);
	SetStatusText(message);
	m_Gauge->Show();
	// refresh the UI
	Update();
}

///////////////////////////////////////////////////////////////////////////////////////
/// \return     true if succeeded
/// \remarks
/// Updates the progress on the item specified by const wxString& title. If found, the
/// ProgressItem is updated, as is the embedded Progress Bar. Note that UpdateProgress
/// DOES NOT clear out the ProgressItem (or the Progress Bar) if the value has reached
/// its maximum. Callers need to make a separate call to FinishProgress() to clean up
/// once a long-running process has completed.
///////////////////////////////////////////////////////////////////////////////////////
bool CStatusBar::UpdateProgress(const wxString& title, int value, const wxString& newmsg)
{
	// is this a real progress item?
	if (title.IsEmpty())
	{
		// blank progress item -- just return
		return false;
	}
	// look for the progress item
	int index = FindProgressItem(title);
	if (index >= 0)
	{
		CProgressItem *pi = m_items.Item(index);
		// figure out how much we're changing the value by
		int delta = ((pi->GetRange() < value) ? pi->GetRange() : value) - pi->GetValue();
		// set the new value, unless it's bigger than the range (in which case just take the range value)
		// Note that we're not removing any progress items until FinishProgress() is called, even if they
		// meet / exceed their range
		pi->SetValue((pi->GetRange() < value) ? pi->GetRange() : value);
		// also set the new message
		pi->SetMessage(newmsg);
		// update the progress bar
		m_Gauge->SetValue(m_Gauge->GetValue() + delta);
		m_Gauge->SetToolTip(newmsg);
		SetStatusText(newmsg);
	}
	// refresh the UI
	Update();
	// return the result (i.e., whether we found the item to update)
	return (index >= 0);
}

///////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// Cleans up the ProgressItem associated with the specified title. This method removes
/// the ProgressItem from the PIArray, updates the embedded progress bar, and hides
/// the progress bar if all work items have been completed.
///////////////////////////////////////////////////////////////////////////////////////
void CStatusBar::FinishProgress(const wxString& title)
{
	int index = FindProgressItem(title);
	if (index >= 0)
	{
		CProgressItem *pi = m_items.Item(index);
		// if this is the last item, clear everything out and hide the control
		if ((int)m_items.GetCount() == 1)
		{
			m_Gauge->SetValue(0);
			m_Gauge->SetRange(100);
			m_Gauge->SetToolTip(_T(""));
			m_Gauge->Hide();
		}
		else
		{
			// NOT the last item -- 
			// Adjust the value and range of the embedded progress bar
			m_Gauge->SetValue(m_Gauge->GetValue() - pi->GetValue());
			m_Gauge->SetRange(m_Gauge->GetRange() - pi->GetRange());
			m_Gauge->SetToolTip(m_items.Last()->GetMessage());
		}
		// delete the item and clean up memory allocation
		delete pi;
		m_items.RemoveAt(index);
	}
	// refresh the UI
	SetStatusText(_T(""));
	Update();
}

///////////////////////////////////////////////////////////////////////////////////////
/// \return     index of item (-1 if not found).
/// \remarks
/// Internal helper method -- returns the index of the specified ProgressItem.
///////////////////////////////////////////////////////////////////////////////////////
int CStatusBar::FindProgressItem(const wxString& title)
{
	int nResult = -1;
	// find the progress item we're trying to update
	int index = 0;
	bool bFound = false;
	CProgressItem *pi = NULL;
	size_t count = m_items.GetCount();
	for (index=0; index < (int)count; index++)
	{
		pi = m_items.Item(index);
		if (pi->GetTitle() == title)
		{
			bFound = true;
			break;
		}
	}
	return (bFound) ? index : nResult;
}

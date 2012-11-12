/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			StatusBar.h
/// \author			Erik Brommers
/// \date_created	02 October 2012
/// \rcs_id $Id$
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This header file defines the custom status bar used by
///                 Adapt It. The CStatusBar supports 2 text status fields
///					as well as a progress bar indicator. 
///					Several helper methods are defined to add/update progress
///					for multiple long-running tasks	in the status bar; 
///					the progress bar displays the cumulative progress of ALL
///					the progress items added to the internal array via StartProgress().
/// \derivation		wxStatusBar.
/////////////////////////////////////////////////////////////////////////////

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <wx/statusbr.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/dynarray.h>

/////////////////////////////////////////////////////////////////////////////
// CProgressItem -- represents a single currently-running progress bar task.
// The CStatusBar holds an array of these that it combines to give the overall progress bar
// indicator.
/////////////////////////////////////////////////////////////////////////////
class CProgressItem
{
public:
	CProgressItem();
	CProgressItem(const wxString& title, const wxString& message, int maximum = 100);

	inline wxString GetTitle() { return m_title;}
	inline void SetTitle(const wxString& newTitle) {m_title = newTitle;}
	inline wxString GetMessage() { return m_message;}
	inline void SetMessage(const wxString& newMessage) {m_message = newMessage;}
	inline int GetValue() { return m_curValue;}
	inline void SetValue(int newValue) {m_curValue = newValue;}
	inline int GetRange() { return m_maxValue;}
	inline void SetRange(int newValue) {m_maxValue = newValue;}
private:
	wxString m_title;	// Title of the long-running task (e.g., "Saving KB")
	wxString m_message;	// Detailed message (e.g., "Opening project -- 3 out of 25 files")
	int m_curValue;		// Current value on the progress bar -- needs to be < m_maxValue
	int m_maxValue;		// Maximum value on the progress bar for this item
};

WX_DECLARE_OBJARRAY(CProgressItem*, PIArray);

/////////////////////////////////////////////////////////////////////////////
// CStatusBar -- wxStatusBar-derived class that 
/////////////////////////////////////////////////////////////////////////////
class CStatusBar : public wxStatusBar
{
public:
    CStatusBar(wxWindow *parent);
    virtual ~CStatusBar();

    void OnSize(wxSizeEvent& event);

	// progress bar - related helper methods
	void StartProgress(const wxString& title, const wxString& message, int maximum = 100);
	bool UpdateProgress(const wxString& title, int value, const wxString& newmsg = wxEmptyString);
	void FinishProgress(const wxString& title);

private:
	int FindProgressItem(const wxString& title); 

    enum
    {
        Field_Text,
		Field_Text2,
        Field_Gauge,
        Field_Max
    };

	wxGauge *m_Gauge;
	PIArray m_items;

    DECLARE_EVENT_TABLE()

};

#endif // STATUSBAR_H

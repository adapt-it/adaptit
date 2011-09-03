/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			PlaceInternalMarkers.h
/// \author			Bill Martin
/// \date_created	29 May 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CPlaceInternalMarkers class. 
/// The CPlaceInternalMarkers class provides a dialog which is presented to the user
/// during export of the target text in the event that RebuildTargetText() needs user
/// input as to the final placement of markers that were merged together during adaptation.
/// \derivation		The CPlaceInternalMarkers class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef PlaceInternalMarkers_h
#define PlaceInternalMarkers_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "PlaceInternalMarkers.h"
#endif

/// The CPlaceInternalMarkers class provides a dialog which is presented to the user
/// during export of the target text in the event that RebuildTargetText() needs user
/// input as to the final placement of markers that were merged together during adaptation.
/// \derivation		The CPlaceInternalMarkers class is derived from AIModalDialog.
class CPlaceInternalMarkers : public AIModalDialog
{
public:
	CPlaceInternalMarkers(wxWindow* parent); // constructor
	virtual ~CPlaceInternalMarkers(void); // destructor
	// other methods

	//enum { IDD = IDD_PLACE_INTERNAL_MARKERS };
	wxString	m_srcPhrase;
	wxString	m_tgtPhrase;
	wxString	m_markers;

	wxTextCtrl* pEditDisabled;
	wxListBox* pListBox;
	wxTextCtrl* pEditTarget;
protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnButtonPlace(wxCommandEvent& WXUNUSED(event));

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* PlaceInternalMarkers_h */

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			PlaceRetranslationInternalMarkers.h
/// \author			Bill Martin
/// \date_created	29 May 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CPlaceRetranslationInternalMarkers class. 
/// The CPlaceRetranslationInternalMarkers class provides a dialog which is presented to the user
/// during export of the target text in the event that RebuildTargetText() needs user
/// input as to the final placement of markers that were merged together during Retranslation.
/// \derivation		The CPlaceRetranslationInternalMarkers class is derived from AIModalDialog.
/// BEW 1Apr10, updated for support of doc version 5
/////////////////////////////////////////////////////////////////////////////

#ifndef PlaceRetranslationInternalMarkers_h
#define PlaceRetranslationInternalMarkers_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "PlaceRetranslationInternalMarkers.h"
#endif

/// The CPlaceRetranslationInternalMarkers class provides a dialog which is presented to
/// the user during export of the target text in the event that RebuildTargetText() needs
/// user input as to the final placement of markers that because retranslation-medial when
/// the retranslation was created. That is, any endmarkers on the first CSourcePhrase, any
/// non-endmarkers on the final CSourcePhrase (of the retranslation), and any other
/// markers whether endmarkers or not, on any CSourcePhrase instances between the first
/// and the last. (The way this class works will, in doc version 5, be the same as the way
/// the CPlaceInternalMarkers class works - and both use the same dialog resource.) 
/// \derivation The CPlaceRetranslationInternalMarkers class is derived from
/// AIModalDialog.
class CPlaceRetranslationInternalMarkers : public AIModalDialog
{
public:
	CPlaceRetranslationInternalMarkers(wxWindow* parent); // constructor
	virtual ~CPlaceRetranslationInternalMarkers(void); // destructor
	// other methods

	// getters and setters
	void	SetNonEditableString(wxString str); // sets m_srcPhrase
	void	SetUserEditableString(wxString str); // sets m_tgtPhrase
	void	SetPlaceableDataStrings(wxArrayString* pMarkerDataArray); // populates pListBox
	wxString	GetPostPlacementString(); // for returning m_tgtPhrase data, after
										  // placements are finished, to the caller
private:
	// the next 3 are for accepting data from outside using the setters
	wxArrayString m_markersToPlaceArray;
	wxString	m_srcPhrase;
	wxString	m_tgtPhrase;
	wxString	m_markers;
	// control pointers - these have to be initialized in the creator, 
	// and not in in InitDialog() which would be too late
	wxTextCtrl* pEditDisabled;
	wxTextCtrl* pEditTarget;
	wxListBox* pListBox;
	wxTextCtrl*	pTextCtrlAsStaticPlaceIntMkrs;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnButtonPlace(wxCommandEvent& WXUNUSED(event));

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* PlaceRetranslationInternalMarkers_h */

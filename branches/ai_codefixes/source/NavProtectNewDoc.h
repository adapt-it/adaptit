/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			NavProtectNewDoc.h
/// \author			Bruce Waters
/// \date_created	9 August 2010
/// \date_revised
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the NavProtectNewDoc class.
/// The NavProtectNewDoc class provides a dialog interface for the user to create a New
/// Document from a list of loadable source text (typically USFM marked up) plain text
/// files, obtained from a folder called 'Source Data' which is a child of the currently
/// open project folder. (If this Source Data folder exists, and has at least one loadable
/// file in it - where loadable is defined as "returning TRUE from being tested by the
/// helpers.cpp function IsLoadableFile()", then the application auto-configures to only
/// show the user files from this folder for which no document of the same name has been
/// created. This class is in support of the "Navigation Protection" feature, to hide from
/// the user the ability to do file and/or folder navigation using the standard file input
/// browser which is shown when <New Document> is clicked, or the File / New... command is
/// invoked, in legacy Adapt It versions. The latter is still what happens if the Source
/// Data folder does not exists, or it exists but the filter tests above return a FALSE
/// result.
/// \derivation		The NavProtectNewDoc class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef NavProtectNewDoc_h
#define NavProtectNewDoc_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "NavProtectNewDoc.h"
#endif

class NavProtectNewDoc : public AIModalDialog
{

public:
	NavProtectNewDoc(wxWindow* parent); // constructor
	virtual ~NavProtectNewDoc(void); // destructor


private:
	// wx version pointers for dialog controls
	wxButton* m_pCreateNewDocButton;
	wxButton* m_pCancelButton;
	wxTextCtrl* m_pFileTitleTextCtrl;

public:

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnCreateNewDocButton(wxCommandEvent& event);
	void OnCancelButton(wxCommandEvent& event);
	void OnBnClickedCancel(wxCommandEvent& event);

private:

	DECLARE_EVENT_TABLE()
};
#endif /* NavProtectNewDoc_h */

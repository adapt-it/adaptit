/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			NavProtectNewDoc.h
/// \author			Bruce Waters
/// \date_created	9 August 2010
/// \rcs_id $Id$
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the NavProtectNewDoc class.
/// The NavProtectNewDoc class provides a dialog interface for the user to create a New
/// Document from a list of loadable source text (typically USFM marked up) plain text
/// files, obtained from a folder called '__SOURCE_INPUTS' which is a child of the currently
/// open project folder. (This __SOURCE_INPUTS folder is now routinely created in the
/// project folder when a project is created. When the AssignLocationsForInputsAndOutputs
/// dialog is set to protect source text inputs from navigation, the application only
/// shows the user files from this folder for which no document of the same name has been
/// created. This class is in support of the "Navigation Protection" feature, to hide from
/// the user the ability to do file and/or folder navigation using the standard file input
/// browser which is shown when <New Document> is clicked, or the File / New... command is
/// invoked, in legacy Adapt It versions. The latter is still what happens if the App's
/// m_bProtectSourceOutputsFolder is FALSE.
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
	wxButton* m_pInputFileButton;
	wxButton* m_pCancelButton;
	wxTextCtrl* m_pInstructionsStaticCtrl;
	wxListBox* m_pMonoclineListOfFiles;

public:
	wxString GetUserFileName();

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnInputFileButton(wxCommandEvent& event);
	void OnCancelButton(wxCommandEvent& event);
	void OnBnClickedCancel(wxCommandEvent& event);
	void OnItemSelected(wxCommandEvent& WXUNUSED(event));
	void OnDoubleClick(wxCommandEvent& WXUNUSED(event));
private:
	// This class relies on the contents of the app member string, m_sourceInputsFolderPath,
	// in order to get the path to the monocline list of files in the "__SOURCE_INPUTS"
	// folder. This will be an empty string if that folder has not been defined. When user 
	// navigation protection is in effect, the __SOURC_INPUTS folder may appear to the user
	// to be empty if there is not at least one file within the __SOURCE_INPUTS folder for which 
	// IsLoadableFile() returns TRUE.
	CAdapt_ItApp* m_pApp;
	wxString m_userFilename; // is an empty string, if Cancel pressed, otherwise, it stores
							 // whatever filename the user selected or double-clicked


	DECLARE_EVENT_TABLE()
};
#endif /* NavProtectNewDoc_h */

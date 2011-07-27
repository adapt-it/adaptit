/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CollabUtilities.h
/// \author			Bruce Waters, from code taken from SetupEditorCollaboration.h by Bill Martin
/// \date_created	27 July 2011
/// \date_revised	
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is a header file containing some utility functions used by Adapt It's
///                 collaboration feature - collaborating with either Paratext or Bibledit. 
/////////////////////////////////////////////////////////////////////////////
//
#ifndef collabUtilities_h
#define collabUtilities_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "CollabUtilities.h"
#endif

#ifndef _string_h_loaded
#define _string_h_loaded
#include "string.h"
#endif
#include "Adapt_It.h"


class CBString;
class SPList;	// declared in SourcePhrase.h WX_DECLARE_LIST(CSourcePhrase, SPList); macro 
				// and defined in SourcePhrase.cpp WX_DEFINE_LIST(SPList); macro
class CSourcePhrase;

// returns the absolute path to the folder being used as the Adapt It work folder, whether
// in standard location or in a custom location - but for the custom location only
// provided it is a "locked" custom location (if not locked, then the path to the standard
// location is returned, i.e. m_workFolderPath, rather than m_customWorkFolderPath). The
// "lock" condition ensures that a snooper can't set up a PT or BE collaboration and
// the remote user not being aware of it.
wxString SetWorkFolderPath_For_Collaboration();
bool IsEthnologueCodeValid(wxString& code);
// the next function is created from OnWizardPageChanging() in Projectpage.cpp, and
// tweaked so as to remove support for the latter's context of a wizard dialog
bool HookUpToExistingAIProject(CAdapt_ItApp* pApp, wxString* pProjectName, wxString* pProjectFolderPath);
// a module for doing the layout and getting the view ready for the user to start
// adapting;; it is not limited to being used in a Collaboration scenario
void SetupLayoutAndView(CAdapt_ItApp* pApp, wxString& docTitle);
// move the newSrc string of just-obtained (from PT or BE) source text, currently in the
// .temp folder, to the __SOURCE_INPUTS folder, creating the latter folder if it doesn't
// already exist, and storing in a file with filename constructed from fileTitle plus an
// added .txt extension; if a file of that name already exists there, overwrite it.
bool MoveTextToFolderAndSave(CAdapt_ItApp* pApp, wxString& folderPath, 
				wxString& pathCreationErrors, wxString& theText, wxString& fileTitle,
				bool bAddBOM = FALSE);
wxString GetTextFromFileInFolder(CAdapt_ItApp* pApp, wxString folderPath, wxString& fileTitle);
wxString GetTextFromFileInFolder(wxString folderPathAndName); // an override of above function
wxString GetTextFromAbsolutePathAndRemoveBOM(wxString& absPath);
bool OpenDocWithMerger(CAdapt_ItApp* pApp, wxString& pathToDoc, wxString& newSrcText, 
					   bool bDoMerger, bool bDoLayout, bool bCopySourceWanted);
void UnloadKBs(CAdapt_ItApp* pApp);
bool CreateNewAIProject(CAdapt_ItApp* pApp, wxString& srcLangName, wxString& tgtLangName,
						wxString& srcEthnologueCode, wxString& tgtEthnologueCode,
						bool bDisableBookMode);
wxString ChangeFilenameExtension(wxString filenameOrPath, wxString extn);
bool KeepSpaceBeforeEOLforVerseMkr(wxChar* pChar); //BEW added 13Jun11

wxString GetPathToRdwrtp7(); // used in GetSourceTextFromEditor::OnInit()
wxString GetBibleditInstallPath();  // used in GetSourceTextFromEditor::OnInit()






#endif


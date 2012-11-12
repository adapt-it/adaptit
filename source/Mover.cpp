/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Mover.h
/// \author			Jonathan Field; modified by Bill Martin for the WX version
/// \date_created	11 November 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the Mover class. 
/// The Mover class encapsulates the logic involved in moving a source document from the main Adaptions 
/// folder to an individual book folder, or from an individual book folder back up to 
/// the main Adaptions folder.
/// \derivation		The Mover class is not a derived class.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in UnitsPage.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Mover.h"
#endif

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
#include <wx/valgen.h> // for wxGenericValidator
#include "Adapt_It.h"
//#include "DataTypes.h"
#include "Mover.h"
#include "helpers.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

// Inputs :
//   FileNameSansPath :
//     If MoveDeeper, then FileNameSansPath must be the name of a file in the Adaptations folder that is to be moved into the current book folder.
//     If !MoveDeeper, then FileNameSansPath must be the name of a file in the current book folder that is to be moved into the Adaptations folder.
//   MoveDeeper : See FileNameSansPath for details.
void Mover::BeginMove(wxString FileNameSansPath, bool MoveDeeper)
{
	CancelMove();
	this->FileNameSansPath = FileNameSansPath;
	this->MoveDeeper = MoveDeeper;
	if (MoveDeeper) {
		SourceFolderPath = gpApp->GetAdaptationsFolderPath();
		DestinationFolderPath = gpApp->GetCurrentBookFolderPath();;
	} else {
		SourceFolderPath = gpApp->GetCurrentBookFolderPath();
		DestinationFolderPath = gpApp->GetAdaptationsFolderPath();
	}
	this->InMove = true;
}

// Return value overview :
//   0 means "success".
//   > 0 means "user intervention required".
//   < 0 means "unrecoverable error - move aborted".
// Return values (detailed) :
//   0 = All is fine.
//   +1 = Requested document is open with unsaved changes and we don't know whether to save or discard the changes.  Ask the user, then set either SaveChanges or DiscardChanges to true and call FinishMove again, or if the user asks to cancel, call CancelMove.
//   +2 = Destination file exists.  If you like, you can ask the user whether they wish to overwrite the existing file.  If they say 'yes', then set OverwriteExistingFile to true and try again.  Otherwise, call CancelMove.
//   -1 = Any unexpected error.  (Expected errors get a negative return value less than negative one.)  As with all other negative return codes, you do _not_ need to call CancelMove after getting this return code.
//   -2 = Invalid settings - SaveChanges and DiscardChanges are _both_ set to true.  (Note that you do not need to call CancelMove in this case.)
//   -3 = FinishMove called without corresponding preceding call to BeginMove.  (This can happen if you call BeginMove, then call FinishMove and get a negative return value, and then call FinishMove again without re-invoking BeginMove.)
//   -4 = Book violation - i.e. you're trying to move text from one book into a folder for a different book.  This is not allowed.  (Note that you do not need to call CancelMove in this case.)
int Mover::FinishMove()
{
	if (!InMove) return DOCUMENTMOVER_ERROR_BEGINMOVENOTCALLED;

	if (SaveChanges && DiscardChanges) {
		CancelMove();
		return DOCUMENTMOVER_ERROR_INVALIDSETTINGS_SAVECHANGESANDDISCARDCHANGES;
	}

	wxString SourcePath = ConcatenatePathBits(SourceFolderPath, FileNameSansPath);
	wxString DestinationPath = ConcatenatePathBits(DestinationFolderPath, FileNameSansPath);

	bool RequestedFileIsOpen = gpApp->IsOpenDoc(SourceFolderPath, FileNameSansPath);
	bool OpenFileHasUnsavedChanges = gpApp->GetDocHasUnsavedChanges();
	bool RequestedFileIsOpenWithUnsavedChanges = RequestedFileIsOpen && OpenFileHasUnsavedChanges;
	if (RequestedFileIsOpenWithUnsavedChanges) {
		if (SaveChanges) {
			gpApp->SaveDocChanges();
		} else if (DiscardChanges) {
			gpApp->DiscardDocChanges();
		} else return DOCUMENTMOVER_USERINTERVENTIONREQUIRED_PROMPTSAVECHANGES;
	}

	// This block performs the book validity test, which prevents the user from moving a document that 
	// is from _one_ Bible book, out of the Adaptations folder into a folder intended for a _different_
	// Bible book.  However, there is no restriction on moving files out of book folders back into the 
	// Adaptations folder.
	// NOTE that we do this _after_ checking for unsaved changes.  This is just in case the unsaved 
	// changes would otherwise have affected the results of the book validity test.
	wxString BookIndicatorInSpecifiedFile;
	if (MoveDeeper && 
		FileContainsBookIndicator(SourcePath, BookIndicatorInSpecifiedFile) && 
		BookIndicatorInSpecifiedFile != gpApp->GetBookIndicatorStringForCurrentBookFolder()) 
	{
		CancelMove();
		return DOCUMENTMOVER_ERROR_BOOKVIOLATION;
	}

	bool DestinationFileExists = FileExists(DestinationPath);
	if (DestinationFileExists && !OverwriteExistingFile) 
		return DOCUMENTMOVER_USERINTERVENTIONREQUIRED_PROMPTOVERWRITEEXISTINGFILE;

	// The MoveFileEx() function is MFC only. Since is uses the MOVEFILE_REPLACE_EXISTING flag
	// the MFC docs say that MoveFileEx() cannot be used to move directories only files, which
	// is what we would expect here as we are moving only one file at a time. Under wx we can
	// use ::wxCopyFile(), and if it succeeds use ::wxRemoveFile() to remove it from its
	// previous location
	//if (MoveFileEx(SourcePath, DestinationPath, MOVEFILE_REPLACE_EXISTING) == 0) 
	if (!::wxCopyFile(SourcePath, DestinationPath))
	{
		CancelMove();
		return DOCUMENTMOVER_ERROR_UNEXPECTED;
	}
	else if (!::wxRemoveFile(SourcePath))
	{
		CancelMove();
		return DOCUMENTMOVER_ERROR_UNEXPECTED;
	}

	if (RequestedFileIsOpen) {
		gpApp->CloseDocDiscardingAnyUnsavedChanges();
	}

	return DOCUMENTMOVER_SUCCESS;
}

void Mover::CancelMove()
{
	if (InMove) {
		this->FileNameSansPath.Empty();
		this->SourceFolderPath.Empty();
		this->DestinationFolderPath.Empty();
		this->InMove = false;
	}
}

/*
Mover::Mover(void)
{
}
*/

Mover::~Mover(void)
{
}

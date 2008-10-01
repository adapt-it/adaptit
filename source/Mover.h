/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			Mover.h
/// \author			Jonathan Field; modified by Bill Martin for AdaptItWX
/// \date_created	11 November 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the Mover class. 
/// The Mover class encapsulates the logic involved in moving a source document from the main Adaptions 
/// folder to an individual book folder, or from an individual book folder back up to 
/// the main Adaptions folder.
/// \derivation		The Mover class is not a derived class.
/////////////////////////////////////////////////////////////////////////////

#ifndef Mover_h
#define Mover_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Mover.h"
#endif


#define DOCUMENTMOVER_SUCCESS 0
#define DOCUMENTMOVER_USERINTERVENTIONREQUIRED_PROMPTSAVECHANGES +1
#define DOCUMENTMOVER_USERINTERVENTIONREQUIRED_PROMPTOVERWRITEEXISTINGFILE +2
#define DOCUMENTMOVER_ERROR_UNEXPECTED -1
#define DOCUMENTMOVER_ERROR_INVALIDSETTINGS_SAVECHANGESANDDISCARDCHANGES -2
#define DOCUMENTMOVER_ERROR_BEGINMOVENOTCALLED -3
#define DOCUMENTMOVER_ERROR_BOOKVIOLATION -4

/*
  Author :
    Initially created by Jonathan Field obo Bruce Waters for the "Adapt It" project 
	developed through Wycliffe Bible Translators.
  Purpose :
    Encapsulate the logic involved in moving a source document from the main Adaptions 
	folder to an individual book folder, or from an individual book folder back up to 
	the main Adaptions folder.
  Usage :
    Have UI instantiate this class and invoke BeginMove, then FinishMove until 
	successful or we give up.  If we give up after a non-negative return value from 
	FinishMove then we must call CancelMove.  However, if we give up because of a 
	negative return value from FinishMove then in effect CancelMove has already been 
	called for us.
*/

/// The Mover class encapsulates the logic involved in moving a source document from the main Adaptions 
/// folder to an individual book folder, or from an individual book folder back up to 
/// the main Adaptions folder.
/// \derivation		The Mover class is not a derived class.
class Mover
{

protected:

	bool InMove;
	wxString FileNameSansPath;
	bool MoveDeeper;
	wxString SourceFolderPath;
	wxString DestinationFolderPath;

public:

	bool SaveChanges;
	bool DiscardChanges;
	bool OverwriteExistingFile;

	void BeginMove(wxString FileNameSansPath, bool MoveDeeper);

	int FinishMove();

	void CancelMove();

	Mover(void) : InMove(false), SaveChanges(false), DiscardChanges(false), OverwriteExistingFile(false) {}

	~Mover(void);

};
#endif /* Mover_h */

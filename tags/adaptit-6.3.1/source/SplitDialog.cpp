/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SplitDialog.cpp
/// \author			Jonathan Field; modified by Bill Martin for the WX version
/// \date_created	15 May 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the SplitDialog class.
/// The SplitDialog class handles the interface between the user and the
/// SplitDialog dialog which is designed to enable the user to split Adapt
/// It documents into smaller unit files.
/// The SplitDialog class is derived from AIModalDialog (DialogBase in MFC).
/// whm Note: SplitDialog was created for the MFC version by Jonathan Field. 
/// Jonathan based the SplitDialog class on a base class he created called 
/// DialogBase (which in turn is based on CDialog). Using such a base dialog 
/// with its special handlers is a good idea, and could have saved some 
/// repetition in other AI dialogs, but complicates things at this late stage 
/// for the wxWidgets version, since his DialogBase is very MFC centric. 
/// Therefore, rather than create a wxWidgets wxDialogBase class, I've 
/// implemented SplitDialog and its methods without dependency on a wxDialogBase 
/// class.
/// \derivation		SplitDialog is derived from AIModalDialog and the supporting Chapter class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "SplitDialog.h"
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

#include <wx/filename.h>
#include <wx/dir.h>

// other includes
#include "Adapt_It.h"
#include "Adapt_ItDoc.h"
#include "Adapt_ItView.h"
#include "Adapt_ItCanvas.h"
#include "SplitDialog.h"
#include "SourcePhrase.h"
#include "helpers.h"
#include "MainFrm.h"
#include "Pile.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

extern bool gbDoingSplitOrJoin;

/// BEW added 02Nov05 to enable PlacePhraseBox(), when called from the app setter function
/// SetCurrentSourcePhrase(), to suppress copying of the source text's word at the box location
/// when a jump to a new splitting location is effected from within the SplitDialog. This
/// prevents a spurious 'adaptation' being saved and put in the KB as a byproduct of doing the
/// split operation.
bool gbIsDocumentSplittingDialogActive = FALSE;

// Define type safe pointer lists
#include "wx/listimpl.cpp"

/// This macro together with the macro list declaration in the .h file
/// complete the definition of a new safe pointer list class called ChList.
WX_DEFINE_LIST(ChList);

// event handler table
BEGIN_EVENT_TABLE(CSplitDialog, AIModalDialog)
	EVT_INIT_DIALOG(CSplitDialog::InitDialog)
	EVT_BUTTON(wxID_OK, CSplitDialog::OnOK)
	EVT_BUTTON(IDC_BUTTON_NEXT_CHAPTER, CSplitDialog::OnBnClickedButtonNextChapter)
	EVT_BUTTON(IDC_BUTTON_SPLIT_NOW, CSplitDialog::OnBnClickedButtonSplitNow)
	EVT_RADIOBUTTON(IDC_RADIO_PHRASEBOX_LOCATION, CSplitDialog::OnBnClickedRadioPhraseboxLocation)
	EVT_RADIOBUTTON(IDC_RADIO_CHAPTER_SFMARKER, CSplitDialog::OnBnClickedRadioChapterSfmarker)
	EVT_RADIOBUTTON(IDC_RADIO_DIVIDE_INTO_CHAPTERS, CSplitDialog::OnBnClickedRadioDivideIntoChapters)
END_EVENT_TABLE()

CSplitDialog::CSplitDialog(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Split The Open Document Into Smaller Documents"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pSplitDialogSizer = SplitDialogFunc(this, TRUE, TRUE);
	// The declaration is: UnitsDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
}

CSplitDialog::~CSplitDialog() // destructor
{
	
}
bool CSplitDialog::SplitAtPhraseBox_IsChecked() 
{
	return IsRadioButtonSelected(IDC_RADIO_PHRASEBOX_LOCATION);
}

bool CSplitDialog::SplitAtNextChapter_IsChecked() 
{
	return IsRadioButtonSelected(IDC_RADIO_CHAPTER_SFMARKER);
}
bool CSplitDialog::SplitIntoChapters_IsChecked() 
{
	return IsRadioButtonSelected(IDC_RADIO_DIVIDE_INTO_CHAPTERS);
}

// helper function - MFC version has equivalent in the DialogBase class
bool CSplitDialog::IsRadioButtonSelected(const int ID)
{
	wxRadioButton* pRB = (wxRadioButton*)FindWindowById(ID);// whm added 10Apr04 not in MFC
	return pRB->GetValue(); // returns TRUE if selected, false otherwise
}

void CSplitDialog::OnOK(wxCommandEvent& event)
{
	event.Skip(); //EndModal(wxID_OK); // In MFC app OnBnClickedOK() simply calls OnOK()
}

int CSplitDialog::GetListItem(wxListBox* pFileList, wxString& s)
{
	int index = pFileList->FindString(s);
	if (index != -1) //LB_ERR
	{
		return index;
	}
	else
	{
		return -1;
	}
}

void CSplitDialog::OnBnClickedButtonNextChapter(wxCommandEvent& WXUNUSED(event))
{
	GoToNextChapter_Interactive();

	// keep the dialog window away from the phrase box so the latter is visible
	CAdapt_ItView* pView = gpApp->GetView();
	pView->AdjustDialogPosition(this);
}

bool CSplitDialog::GoToNextChapter_Interactive()
{
	CSourcePhrase *sp;

	sp = gpApp->FindNextChapter(gpApp->GetSourcePhraseList(), gpApp->GetCurrentSourcePhrase());
	if (sp) {
		gpApp->SetCurrentSourcePhrase(sp);
		return true;
	} 
	else 
	{
		// IDS_NO_MORE_CHAPTERS
		wxMessageBox(_("There are no more chapters after the current phrasebox location."),_T(""), wxICON_INFORMATION | wxOK); //TellUser();
		return false;
	}
}

void CSplitDialog::OnBnClickedButtonSplitNow(wxCommandEvent& WXUNUSED(event))
{
	// BEW added 23Jun07
	gbDoingSplitOrJoin = TRUE; // use to suppress document backups generation during the splitting

	gpApp->LogUserAction(_T("Executing Split Now in CSplitDialog"));

	// If the user requested that we split at the next chapter, then move the phrasebox location to the
	// beginning of the next chapter so that we can, for the rest of the function, use the same code for
	// both when they've selected "split at next chapter" and "split at current phrasebox location".
	if (this->SplitAtNextChapter_IsChecked()) {
		if (!GoToNextChapter_Interactive()) 
		{
			// hilight the current document where it appears in the list
			wxString curDocFilename = gpApp->GetCurrentDocFileName();
			int index = GetListItem(pFileList,curDocFilename);
			if (index != -1)
			{
				pFileList->SetSelection(index,TRUE);
			}
			gbDoingSplitOrJoin = FALSE; // restore default
			return;
		}
	}

	if (!this->SplitIntoChapters_IsChecked()) 
	{
		SplitAtPhraseBoxLocation_Interactive();

		// keep the dialog out of the way of the phrasebox
		CAdapt_ItView* pView = gpApp->GetView();
		pView->AdjustDialogPosition(this);
	} else 
	{
		SplitIntoChapters_Interactive();
	}
	// hilight the current document where it appears in the list
	wxString curDocFilename = gpApp->GetCurrentDocFileName();
	int index = GetListItem(pFileList,curDocFilename);
	if (index != -1)
	{
		pFileList->SetSelection(index,TRUE);
	}

	gpApp->RefreshStatusBarInfo();
	gbDoingSplitOrJoin = FALSE; // restore document backups capability
}

void CSplitDialog::SplitAtPhraseBoxLocation_Interactive()
{
	// Show the "Splitting... Please wait." message
	pSplittingWait->Show(TRUE);
	pSplittingWait->Update();
	CAdapt_ItView* pView = gpApp->GetView();

	wxString OriginalFileName;
	wxString OriginalFilePath;
	wxString FirstFileName;
	wxString FirstFilePath;
	wxString SecondFileName;
	wxString SecondFilePath;
	bool SecondFileNameIsOriginalFileName;
	bool FirstFileNameIsOriginalFileName;
	bool FirstFileNameIsSecondFileName = false;
	CAdapt_ItDoc *d = gpApp->GetDocument();

	OriginalFileName = gpApp->GetCurrentDocFileName();
	OriginalFilePath = gpApp->GetCurrentDocPath();

	// check for a dirty document, and automatically save it before splitting if it is dirty
	if (gpApp->GetDocHasUnsavedChanges())
	{
		// save
		if (!OriginalFilePath.IsEmpty())
		{
			// BEW changed 29Apr10
			//d->DoFileSave(TRUE); // TRUE - show wait/progress dialog
			// whm 24Aug11 Note. I don't think a wait dialog is really needed for
			// the split operation since it is likely to go quickly
			d->DoFileSave_Protected(FALSE,_T("")); // TRUE - show wait/progress dialog
		}
	}

	// Verify first filename.
	FirstFileName = pFileName1->GetValue();
	FirstFileName.Trim(FALSE); // trim left end
	FirstFileName.Trim(TRUE); // trim right end
	if (FirstFileName.IsEmpty()) 
	{
		// IDS_SUPPLY_NAME_FOR_SPLIT
		wxMessageBox(_("Please supply a suitable name for the split-off document part."),_T(""),wxICON_INFORMATION | wxOK); //TellUser();
		return;
	}
	FirstFileName = CAdapt_ItApp::ApplyDefaultDocFileExtension(FirstFileName);
	FirstFileNameIsOriginalFileName = false;
	if (FirstFileName.CmpNoCase(OriginalFileName) == 0) 
	{
		FirstFileNameIsOriginalFileName = true;
	}
	FirstFilePath = ConcatenatePathBits(gpApp->GetCurrentDocFolderPath(), FirstFileName);

	// Verify second filename if supplied.
	SecondFileName = pFileName2->GetValue(); // for compiling with VisStudio 7
	SecondFileName.Trim(FALSE); // trim left end
	SecondFileName.Trim(TRUE); // trim right end
	SecondFileNameIsOriginalFileName = SecondFileName.IsEmpty();
	if (!SecondFileNameIsOriginalFileName) 
	{
		SecondFileName = CAdapt_ItApp::ApplyDefaultDocFileExtension(SecondFileName);
		if (SecondFileName.CmpNoCase(OriginalFileName) == 0) 
		{
			SecondFileNameIsOriginalFileName = true;
		}
	}
	if (SecondFileNameIsOriginalFileName) 
	{
		SecondFileName = OriginalFileName;
	}
	SecondFilePath = ConcatenatePathBits(gpApp->GetCurrentDocFolderPath(), SecondFileName);

	// are the names the same? (if so, we must block the split unilaterally, else get data loss)
	if (FirstFileName == SecondFileName)
		FirstFileNameIsSecondFileName = true;
	if (FirstFileNameIsSecondFileName)
	{
		// IDS_MUST_BE_DIFFERENT_NAMES
		wxMessageBox(_("Your name choices would result in the two document parts having the same name, which would cause the data in the first part to be lost.\nPlease make the name in the second box different than the name of the currently open document."),_T(""),wxICON_EXCLAMATION | wxOK); //TellUser();
		return;
	}

	// We can silently overwrite the original file if either of the FirstFileName or SecondFileName
	// has the same name as it, because that would not lose data. However, if one of FirstFileName
	// or SecondFileName, or both, is different than OriginalFileName, then the user must be asked
	// whether or not to overwrite when there is name identity with either of FirstFileName or
	// SecondFileName, or both - and cancel the operation if an overwrite is not wanted.

	// NOTE: poorly named files can lead to data loss if the user is not careful. EG. If he has part
	// of a chapter's content in a file along with all of another chapter, and the other part of the chapter
	// is in a file which is named only with a book name and chapter number -- if he then splits the first
	// (not checking and wrongly thinking the first contains all the chapter not just part of it)
	// and when prompted overwrites the second, the second's part chapter content will be lost. To
	// avoid these problems, part chapters should be reflected by the file's name -- ie. include the
	// verse range in the file's name.

	if (!FirstFileNameIsOriginalFileName && FileExists(FirstFilePath)) 
	{
		wxString msg;
		wxString msg2;
		wxString initial;
		wxString common;
		initial = _("WARNING: a document called  %s  already exists and you have specified that the split off document-part has the same name.\n"); //.LoadString(IDS_FIRST_DOC_EXISTS); // assume no error
		msg = msg.Format(initial.c_str(),FirstFileName.c_str());
		common = _("If you click OK the content of the  %s  document will be overwritten. This could cause data loss. If you are unsure, you should click Cancel and first check what is in the  %s  and  %s  documents.\n\nDo you want to overwrite it?"); //.LoadString(IDS_COMMON_SPLIT_WARNING_PART); // ditto
		msg2 = msg2.Format(common.c_str(),FirstFileName.c_str(),FirstFileName.c_str(),OriginalFileName.c_str());
		msg += msg2;
		int nResult = wxMessageBox(msg,_T(""),wxOK | wxCANCEL);
		if (nResult == wxCANCEL) 
		{
			return;
		}
	}
	if (!SecondFileNameIsOriginalFileName && FileExists(SecondFilePath)) 
	{
		wxString msg;
		wxString msg2;
		wxString initial;
		wxString common;
		initial = _("WARNING: a document called  %s  already exists and you have specified that the second document-part, which is to remain open, has the same name.\n"); //.LoadString(IDS_REMAINDER_DOC_EXISTS); // assume no error
		msg = msg.Format(initial.c_str(),SecondFileName.c_str());
		common = _("If you click OK the content of the  %s  document will be overwritten. This could cause data loss. If you are unsure, you should click Cancel and first check what is in the  %s  and  %s  documents.\n\nDo you want to overwrite it?"); //.LoadString(IDS_COMMON_SPLIT_WARNING_PART); // ditto
		msg2 = msg2.Format(common.c_str(),SecondFileName.c_str(),SecondFileName.c_str(),OriginalFileName.c_str());
		msg += msg2;
		int nResult = wxMessageBox(msg,_T(""),wxOK | wxCANCEL);
		if (nResult == wxCANCEL) 
		{
			return;
		}
	}

	// get the Book ID code (such as MAT or REV or 1TH etc) - when not in book mode, we get it
	// from the document's first pSrcPhrase (we get an empty string if it is not there), and for
	// book mode, we get it from the BookNamePair struct for the currently active Bible book folder -
	// and getting it in the latter way can never fail
	wxString bookID = gpApp->GetBookID();

	// Do the actual split.
	pView->canvas->Freeze();
	SPList *SourcePhrases2 = gpApp->m_pSourcePhrases;
	SPList *SourcePhrases1 = SplitOffStartOfList(SourcePhrases2, gpApp->m_nActiveSequNum); // refactored 26Apr09

	// BEW added test 02Nov05, to check the user actually advanced the phrasebox from the
	// starting position in the document - if he didn't and he invoked the split, the SourcePhrases1
	// list would be empty, and then CascadeSourcePhraseListChange() would fail when the update of
	// the sequence numbers was attempted
	if (SourcePhrases1->GetCount() == 0)
	{
		pView->canvas->Thaw();
		pSplittingWait->Show(FALSE);
		// IDS_BOX_NOT_MOVED_FORWARD
		wxMessageBox(_("The split was aborted because the phrase box was still at the start of the document."),_T(""), wxICON_EXCLAMATION | wxOK);
		return;
	}

	gpApp->m_pSourcePhrases = SourcePhrases1;
	gpApp->CascadeSourcePhraseListChange(false);
	gpApp->ChangeDocUnderlyingFileDetailsInPlace(FirstFileName);
	gpApp->SaveDocChanges();

	// the second list of sourcephrases won't yet have an \id line with the book ID code,
	// and a TextType of identification, so we need to insert a new sourcephrase to store 
	// that info before saving, provided we indeed have found a valid book ID (if the string
	// is non-empty, it will be a valid one)
	gpApp->m_pSourcePhrases = SourcePhrases2;
	// BEW 29Mar10, add a check for an existing book ID string (3-letter code) at the
	// start of the second document - if there is one there, assume the user is splitting
	// a composite document made up of bible books, and so retain what is there instead of
	// copying the split off part's book id.
	wxString bookID2 = gpApp->GetBookID(); // the check uses IsValidBookID() 
										   // which converts to lower case first
	if (!bookID.IsEmpty() && bookID2.IsEmpty())
	{
		gpApp->AddBookIDToDoc(gpApp->m_pSourcePhrases, bookID);
	}

	// sequence number updating will occur in the next function call
	gpApp->CascadeSourcePhraseListChange(false);
	gpApp->ChangeDocUnderlyingFileDetailsInPlace(SecondFileName);
	gpApp->SaveDocChanges();

	// Now free memory by deleting SourcePhrases1 and it's contents.
	gpApp->DeleteSourcePhraseListContents(SourcePhrases1);
	if (SourcePhrases1 != NULL) // whm 11Jun12 added NULL test
		delete SourcePhrases1;

	// Finish the effective-rename of what was the current document if applicable.
	// (The new filename has already been allocated and saved to at this point, so all we need to do,
	// if applicable, is delete the original file with the original filename.)
	if (!FirstFileNameIsOriginalFileName && !SecondFileNameIsOriginalFileName) 
	{
		if (!::wxRemoveFile(OriginalFilePath))
		{
			::wxBell();
		}
	}

	// Update screen.
	gpApp->CascadeSourcePhraseListChange(true);
	gpApp->SetCurrentSourcePhraseByIndex(0);

	// update Title bar (BEW added 14Aug09)
	wxString strUserTyped = gpApp->m_curOutputFilename;
	d->SetDocumentWindowTitle(strUserTyped, strUserTyped);

	// update the file list to show all current files, including the file just split off and saved
	// -- but don't show any plain text files or backup files
	ListFiles();

	// clear the filename used for the split off file - we don't want the user to reuse it by mistake!
	// likewise the filename used for the open doc part on the screen
	pFileName1->SetValue(_T(""));
	pFileName2->SetValue(_T(""));

	pView->canvas->Thaw();
	pSplittingWait->Show(FALSE);
	//IDS_SPLIT_SUCCEEDED
	wxMessageBox(_("Splitting the document succeeded."),_T(""),wxICON_INFORMATION | wxOK);
	gpApp->LogUserAction(_T("Splitting the document succeeded - split at phrasebox."));
}

// Returns a list of Chapter objects.  Does not modify SourcePhrases.  This means that each SourcePhrase
// ends up in _two_ SPList instances - SourcePhrases, and Chapter.SourcePhrases for one of the output 
// Chapter instances.  Thus, be careful managing memory, that you neither double-delete nor fail to delete.
// *cChapterDigits is set to the maximum number of digits used in any of the chapter numbers.
ChList *CSplitDialog::DoSplitIntoChapters(wxString WorkingFolderPath, wxString FileNameBase, 
										  SPList *SourcePhrases, int *cChapterDigits)
{
	// Show the "Splitting... Please wait." message
	pSplittingWait->Show(TRUE);
	pSplittingWait->Update();
	CAdapt_ItDoc *d = gpApp->GetDocument();

	// check for a dirty document, and automatically save it before splitting if it is dirty
	wxString OriginalFilePath = gpApp->GetCurrentDocPath();
	if (gpApp->GetDocHasUnsavedChanges())
	{
		// save
		if (!OriginalFilePath.IsEmpty())
		{
			// BEW changed 29Apr10
			//d->DoFileSave(TRUE); // TRUE - show wait/progress dialog
			// whm 24Aug11 Note. I don't think a wait dialog is really needed for
			// the split operation since it is likely to go quickly
			d->DoFileSave_Protected(FALSE,_T("")); // TRUE - show wait/progress dialog
		}
	}

	// get the Book ID code (such as MAT or REV or 1TH etc) - when not in book mode, we get it
	// from the document's first pSrcPhrase (we get an empty string if it is not there), and for
	// book mode, we get it from the BookNamePair struct for the currently active Bible book folder -
	// and getting it in the latter way can never fail
	wxString bookID = gpApp->GetBookID();

	//	 Don't get tricked by the book \id tag preceding the first chapter marker!
	//   And don't get tricked by multi-chapter passages that begin other than at the start of a chapter!
	//   In both these cases, the source phrases preceding the first chapter marker should go into the new 
	//   file along with the source phrases after that first chapter marker.
	//   e.g. If a book contains John 3:18 to the end of John 5, then we should only have two output files,
	//   one containing John 3:18 through to the end of John 4, and the second containing John 5.
	//   This behaviour ensures that, if an entire book is split into chapters, 
	//   the \id tag (which always precedes the first chapter marker) will be included in the file with the 
	//   first chapter, instead of being in a file all by itself.

	ChList *rv = new ChList();
	Chapter *c;
	SPList::Node* p;
	CSourcePhrase *sp;
	wxString ChapterString;

	*cChapterDigits = 0;

	c = new Chapter();
	c->Number = 0;
	c->SourcePhrases = new SPList();
	rv->Append(c);

	// Split into chapters.
	// BEW modified algorithm 04Nov05, because when a section head precedes the start of a new chapter,
	// the \c which marks the chapter 'start' precedes the section head (and so is stored on the m_markers
	// member of the CSourcePhrase instance which stores the first word of the section head) but the
	// m_chapterVerse member at this point is still empty, and it gets content only later when the first
	// word of a new verse was parsed and on that word's CSourcePhrase instance is where we get a string
	// like "2:1" (for chapter 2 verse 1) stored in m_chapterVerse - so the revised algorithm needs to
	// create a new Chapter() instance when the \c is encountered, but test every CSourcePhrase instance
	// for a non-empty m_chapterVerse and when it finds the first such, extract the chapter number
	// and set the Number member of Chapter() at that time.
	bool bCountedChapter = TRUE;
	//SPList::Node* save_pos = NULL; // set but not used
	p = SourcePhrases->GetFirst();
	while (p) {
		sp = (CSourcePhrase*)p->GetData();
		//save_pos = p; // for use in the MoveFinalEndmarkersToEndOfLastChapter() call
		p = p->GetNext();
		if (sp->GetStartsNewChapter()) 
		{
            // a new chapter starts, either at the first verse or the start of a section
            // heading which precedes the start of the first verse; so get fresh Chapter
            // storage ready
			bCountedChapter = FALSE;
			if (c->Number != 0) 
			{
				c = new Chapter();
				c->Number = 0; // will get set later
				c->SourcePhrases = new SPList();
				rv->Append(c); // append the pointer to the chapter object 
							   // to the end of the list of chapters
			}
            // the second and subsequent lists of sourcephrases won't yet have an \id line
            // with the book ID code included, so we need to insert a new sourcephrase to
            // store that info in each such sublist, provided we indeed have found a valid
            // book ID (if the string is non-empty, it will be a valid one) Note (29Mar10)
            // we assume the document being split into chapters is all from one Bible book,
            // so that the book ID code is the same for each chapter, so we don't make any
            // test here (unlike the other 2 options) for the possibility of a "chapter"
            // starting with an \id field with a different book ID code in it - that would
            // be a crazy situation and we can discount it as a realistic possibility; just
            // use what is in bookID if it is non-empty
			if (rv->GetCount() >= 2)
			{
				if (!bookID.IsEmpty())
				{
					gpApp->AddBookIDToDoc(c->SourcePhrases, bookID);
				}
			}
		}

		// check each m_chapterVerse string for a non-empty "n:m" string, use the first such
		if (sp->ChapterColonVerseStringIsNotEmpty() && !bCountedChapter)
		{
			bCountedChapter = TRUE;
			ChapterString = sp->GetChapterNumberString();
			if (ChapterString.Length() > 0 && ChapterString.Length() < 7 && IsAnsiDigitsOnly(ChapterString)) 
			{
				c->Number = wxAtoi(ChapterString); // BEW changed 26Oct05; was = atoi(ChapterString);
				c->NumberString = ChapterString;
				if ((int)ChapterString.Length() > *cChapterDigits) 
				{
					*cChapterDigits = ChapterString.Length();
				}
			}
		}
		// put the chapter's CSourcePhrase instances into the sublist
		c->SourcePhrases->Append(sp);

	}

	// Assign filenames.
	ChList::Node* pc;
	pc = rv->GetFirst();
	while (pc) {
		c = (Chapter*)pc->GetData();
		pc = pc->GetNext();
		while ((int)c->NumberString.Length() < *cChapterDigits) 
		{
			c->NumberString = _T('0') + c->NumberString;
		}
		c->FileName = FileNameBase + c->NumberString;
		c->FileName = gpApp->ApplyDefaultDocFileExtension(c->FileName);
		c->FilePath = ConcatenatePathBits(WorkingFolderPath, c->FileName);
	}

	return rv;
}

void CSplitDialog::SplitIntoChapters_Interactive()
{
	CAdapt_ItDoc *d = gpApp->GetDocument();
	wxString FileNameBase;
	ChList *Chapters;
	int cChapterDigits;
	ChList::Node* p;
	Chapter *c;
	SPList *OriginalSourcePhraseList = gpApp->m_pSourcePhrases;
	wxString OriginalFilePath = gpApp->GetCurrentDocPath();

	// Validate FileNameBase.
	FileNameBase = pFileName1->GetValue();
	if (FileNameBase.IsEmpty()) 
	{
		// IDS_SUPPLY_FILENAME_BASE
		wxMessageBox(_("Please supply the file name base."),_T(""),wxICON_INFORMATION | wxOK); //TellUser();
		return;
	}

	// Perform an in-memory split.
	Chapters = DoSplitIntoChapters(gpApp->GetCurrentDocFolderPath(), 
									FileNameBase, OriginalSourcePhraseList, &cChapterDigits);
	if (Chapters->GetCount() == 1) 
	{
		pSplittingWait->Show(FALSE);
		// IDS_ONLY_ONE_CHAPTER_IN_DOC
		wxMessageBox(_("There is only one chapter in this document."),_T(""), wxICON_EXCLAMATION | wxOK); //TellUser();
		return;
	}

	// Ensure ALL destination filenames are available.
	p = Chapters->GetFirst();
	while (p) {
		c = (Chapter*)p->GetData();
		p = p->GetNext();
		if (FileExists(c->FilePath)) 
		{
			// IDS_FILENAME_CONFLICT
			wxMessageBox(_("There is a name conflict with an existing file.  Please specify a different file name base."),_T(""),wxICON_EXCLAMATION | wxOK);//TellUser();
			pSplittingWait->Show(FALSE);
			return;
		}
	}

	// Save the split bits.
	p = Chapters->GetFirst();
	while (p) 
	{
		c = (Chapter*)p->GetData();
		p = p->GetNext();
		gpApp->m_pSourcePhrases = c->SourcePhrases;
		gpApp->ChangeDocUnderlyingFileNameInPlace(c->FileName);
		gpApp->CascadeSourcePhraseListChange(false); // sequence numbers renumbered here too
		gpApp->SaveDocChanges();
	}
	pSplittingWait->Show(FALSE);

	// Delete the original file.
	if (!::wxRemoveFile(OriginalFilePath))
	{
		::wxBell();
	}

	// Close the current document, discarding changes.
	gpApp->CloseDocDiscardingAnyUnsavedChanges();

	// Free memory.
	if (OriginalSourcePhraseList != NULL) // whm 11Jun12 added NULL test
		delete OriginalSourcePhraseList;
	p = Chapters->GetFirst();
	while (p) 
	{
		c = (Chapter*)p->GetData();
		p = p->GetNext();
		if (c->SourcePhrases != gpApp->m_pSourcePhrases) 
		{
			gpApp->DeleteSourcePhraseListContents(c->SourcePhrases);
			if (c->SourcePhrases != NULL) // whm 11Jun12 added NULL test
				delete c->SourcePhrases;
		}
		if (c != NULL) // whm 11Jun12 added NULL test
			delete c;
	}
	if (Chapters != NULL) // whm 11Jun12 added NULL test
		delete Chapters;

	// Close the dialog box.
	wxCommandEvent event;
	OnOK(event);
	// IDS_SPLIT_SUCCEEDED
	wxMessageBox(_("Splitting the document succeeded."),_T(""),wxICON_INFORMATION | wxOK); //TellUser();
	gpApp->LogUserAction(_T("Splitting the document succeeded - split into chapters interactive."));

	// having the end result be an empty window could be confusing, so have the Start Working...
	// wizard come up at the Document page, so that the user can do something meaningful -- such
	// as to get a document open, or Cancel etc.
	d->OnFileOpen(event);
}

bool CSplitDialog::CurrentDocSpansMoreThanOneChapter()
{
	
	/*
	  Algorythm :
		Assume that the number of chapters equals the number of "\\c" markers.
	*/

	int cChapters = 0;

	SPList::Node* p;
	//CAdapt_ItDoc *d;
	SPList *ol;
	CSourcePhrase *sp;

	//d = gpApp->GetDocument();
	ol = gpApp->m_pSourcePhrases;
	p = ol->GetFirst();
	while (p) {
		sp = (CSourcePhrase*)p->GetData();
		p = p->GetNext();
		if (p) 
		{
			// TODO: Look at, e.g. Luke.adt and 1 Timothy.adt and see what structure fields have useful values.
			if (sp->m_markers.Find(_T("\\c ")) != -1) {
				cChapters += 1;
				if (cChapters > 1) {
					return true;
				}
			}
		}
	}
	
	return false;
}

void CSplitDialog::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	//InitDialog() is not virtual, no call needed to a base class
	// whm added pointers to dialog controls for wx version
	pSplitAtPhraseBox = (wxRadioButton*)FindWindowById(IDC_RADIO_PHRASEBOX_LOCATION);
	wxASSERT(pSplitAtPhraseBox != NULL);
	pSplitAtNextChapter = (wxRadioButton*)FindWindowById(IDC_RADIO_CHAPTER_SFMARKER);
	wxASSERT(pSplitAtNextChapter != NULL);
	pSplitIntoChapters = (wxRadioButton*)FindWindowById(IDC_RADIO_DIVIDE_INTO_CHAPTERS);
	wxASSERT(pSplitIntoChapters != NULL);
	pLocateNextChapter = (wxButton*)FindWindowById(IDC_BUTTON_NEXT_CHAPTER);
	wxASSERT(pLocateNextChapter != NULL);
	pSplittingWait = (wxStaticText*)FindWindowById(IDC_STATIC_SPLITTING_WAIT);
	wxASSERT(pSplittingWait != NULL);
	pFileListLabel = (wxStaticText*)FindWindowById(IDC_STATIC_FOLDER_DOCS);
	wxASSERT(pFileListLabel != NULL);
	pFileName1 = (wxTextCtrl*)FindWindowById(IDC_EDIT1);
	wxASSERT(pFileName1 != NULL);
	pFileName2 = (wxTextCtrl*)FindWindowById(IDC_EDIT2);
	wxASSERT(pFileName2 != NULL);
	pFileList = (wxListBox*)FindWindowById(IDC_LIST_FOLDER_DOCS) ;
	wxASSERT(pFileList != NULL);
	pFileName1Label = (wxStaticText*)FindWindowById(IDC_STATIC_SPLIT_NAME);
	wxASSERT(pFileName1Label != NULL);
	pFileName2Label = (wxStaticText*)FindWindowById(IDC_STATIC_REMAIN_NAME) ;
	wxASSERT(pFileName2Label != NULL);

	// whm 29Aug11 added
	// make the "Documents in the folder Adaptations" list look read-only
	pFileList->SetBackgroundColour(gpApp->sysColorBtnFace);
	
	pSplittingWait->Show(FALSE);

	// make the font show user's desired dialog point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, pFileName1, pFileName2,
									pFileList, NULL, gpApp->m_pDlgGlossFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, pFileName1, pFileName2, 
													pFileList, NULL, gpApp->m_pDlgGlossFont);
	#endif


	// Set default values and stuff.
	if (!CurrentDocSpansMoreThanOneChapter()) 
	{
		pSplitAtNextChapter->Enable(FALSE);
		pSplitIntoChapters->Enable(FALSE);
		pLocateNextChapter->Enable(FALSE);
	}
	pSplitAtPhraseBox->SetValue(TRUE);
	RadioButtonsChanged();
	ListFiles();

	// update status bar with project name and chapter:verse
	gpApp->RefreshStatusBarInfo();

	// keep the dialog window away from the phrase box so the latter is visible
	CAdapt_ItView* pView = gpApp->GetView();
	pView->AdjustDialogPosition(this);

	// hilight the current document where it appears in the list
	wxString curDocFilename = gpApp->GetCurrentDocFileName();
	int index = GetListItem(pFileList,curDocFilename);
	if (index != -1)
	{
		pFileList->SetSelection(index,TRUE);
	}

	gpApp->RefreshStatusBarInfo();
	
	// adjust for substituted items
	pSplitDialogSizer->Layout();
}

void CSplitDialog::ListFiles()
{
	wxString Message;
	wxString s;
	wxString FolderName;
	wxString FolderPath;

	// Set the label above the listbox.
	s = _("Documents in the folder %s"); //IDS_DOCS_IN_FOLDER
	FolderName = gpApp->GetCurrentDocFolderDisplayName();
	FolderPath = gpApp->GetCurrentDocFolderPath();
	Message = Message.Format(s.c_str(), FolderName.c_str());
	pFileListLabel->SetLabel(Message);

	wxString PathSeparator = wxFileName::GetPathSeparator();
	// Fill the listbox. 
	// BEW modified 02Nov05, to only show document files; the FALSE parameter
	// effects this; the default TRUE value would list all files in the folder
	//this->FillListBoxWithListOfFilesInPath(FileList, FolderPath, FALSE);
	
	// whm note: Code below from JF's DialogBase::FillListBoxWithListOfFilesInPath(CListBox& lb, CString Path, BOOL bShowAllFiles)
	// In wx version we're not implementing JF's DialogBase class, but using the code from its function here.
	//wxString FolderPathAndFilespecWildcards = FolderPath + PathSeparator + _T("*.*");
	pFileList->Clear(); // clear out all entries from the listbox
	wxArrayString files;
	// enumerate the document files in the Adaptations folder or the current book folder; and
	// note that internally GetPossibleAdaptionDocuments excludes any files with names of the
	// form *.BAK (these are backup XML document files, and for each there will be present
	// an *.xml file which has identical content -- it is the latter we enumerate) and also note
	// the result could be an empty m_acceptedFilesList, but have the caller of EnumerateDocFiles
	// check it for no entries in the list
	//wxArrayString docs; //CStringList docs;
	gpApp->GetPossibleAdaptionDocuments(&files, FolderPath);
	wxString nextDoc;
	int ct;
	for (ct = 0; ct < (int)files.GetCount(); ct++)
	{
		nextDoc = files.Item(ct);
		pFileList->Append(nextDoc);
	}
}

void CSplitDialog::OnBnClickedRadioPhraseboxLocation(wxCommandEvent& WXUNUSED(event))
{
	RadioButtonsChanged();
}

void CSplitDialog::OnBnClickedRadioChapterSfmarker(wxCommandEvent& WXUNUSED(event))
{
	RadioButtonsChanged();
}

void CSplitDialog::OnBnClickedRadioDivideIntoChapters(wxCommandEvent& WXUNUSED(event))
{
	RadioButtonsChanged();
}

void CSplitDialog::RadioButtonsChanged()
{
	wxString s;

	if (!pSplitIntoChapters->GetValue() == TRUE) 
	{
		pFileName2->Show(TRUE);
		pFileName2Label->Show(TRUE);
		s = _("(Obligatory) Type a suitable name for the split off document part:"); //IDS_SPLIT_DOC_NAME
	} else {
		pFileName2->Hide();
		pFileName2Label->Hide();
		s = _("(Obligatory) Type a generic name to which each chapter number can be added"); //IDS_GENERIC_DOC_NAME
	}
	pFileName1Label->SetLabel(s);
}

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ToolbarPage.cpp
/// \author			Erik Brommers
/// \date_created	8 March 2013
/// \rcs_id $Id: $
/// \copyright		2013 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CToolbarPagePrefs class.
/// This class creates a panel in the EditPreferencesDlg that allow the user 
/// to edit the toolbar display. The interface resources for CToolbarPagePrefs
/// are defined in ToolbarPageFunc() which was developed and is maintained by
/// wxDesigner.
/// \derivation		wxPanel.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ToolbarPage.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ToolbarPage.h"
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
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
#include <wx/wizard.h>
#include <wx/imaglist.h>
#include <wx/renderer.h>	// for the checkboxes on the list control

#include "Adapt_It.h"
#include "ToolbarPage.h"
#include "AdaptitConstants.h"
#include "helpers.h"

// toolbar size examples
#include "../res/vectorized/edit-paste_22.cpp"
#include "../res/vectorized/edit-paste_32.cpp"

#include "../res/vectorized/document_new_16.cpp"
#include "../res/vectorized/document_open_16.cpp"
#include "../res/vectorized/document_save_16.cpp"
#include "../res/vectorized/edit_cut_16.cpp"
#include "../res/vectorized/edit_copy_16.cpp"
#include "../res/vectorized/edit_paste_16.cpp"
#include "../res/vectorized/document_print_16.cpp"
#include "../res/vectorized/dialog_guesser_16.cpp"
#include "../res/vectorized/dialog_notes_16.cpp"
#include "../res/vectorized/note_next_16.cpp"
#include "../res/vectorized/note_prev_16.cpp"
#include "../res/vectorized/note_delete_all_16.cpp"
#include "../res/vectorized/bounds_go_16.cpp"
#include "../res/vectorized/bounds_stop_16.cpp"
#include "../res/vectorized/format_hide_punctuation_16.cpp"
#include "../res/vectorized/format_show_punctuation_16.cpp"
#include "../res/vectorized/go_first_16.cpp"
#include "../res/vectorized/go_last_16.cpp"
#include "../res/vectorized/go_previous_16.cpp"
#include "../res/vectorized/go_up_16.cpp"
#include "../res/vectorized/go_down_16.cpp"
#include "../res/vectorized/phrase_new_16.cpp"
#include "../res/vectorized/phrase_remove_16.cpp"
#include "../res/vectorized/retranslation_new_16.cpp"
#include "../res/vectorized/retranslation_edit_16.cpp"
#include "../res/vectorized/retranslation_delete_16.cpp"
#include "../res/vectorized/placeholder_new_16.cpp"
#include "../res/vectorized/placeholder_delete_16.cpp"
#include "../res/vectorized/dialog_choose_translation_16.cpp"
#include "../res/vectorized/show_target_16.cpp"
#include "../res/vectorized/show_source_target_16.cpp"
#include "../res/vectorized/dialog_view-translation-or-glosses_16.cpp"
#include "../res/vectorized/punctuation_copy_16.cpp"
#include "../res/vectorized/punctuation_do_not_copy_16.cpp"
#include "../res/vectorized/help_browser_16.cpp"


/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

IMPLEMENT_DYNAMIC_CLASS( CToolbarPagePrefs, wxPanel )

// event handler table
BEGIN_EVENT_TABLE(CToolbarPagePrefs, wxPanel)
	EVT_INIT_DIALOG(CToolbarPagePrefs::InitDialog)
//	EVT_LEFT_UP(CToolbarPagePrefs::OnClickLstToolbarButtons)
END_EVENT_TABLE()

CToolbarPagePrefs::CToolbarPagePrefs()
{
}

CToolbarPagePrefs::CToolbarPagePrefs(wxWindow* parent) // dialog constructor
{
	Create( parent );

}

CToolbarPagePrefs::~CToolbarPagePrefs() // destructor
{
}

bool CToolbarPagePrefs::Create( wxWindow* parent)
{
	wxPanel::Create( parent );
	CreateControls();
	GetSizer()->Fit(this);
	return TRUE;
}

void CToolbarPagePrefs::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pToolbarPageSizer = ToolbarPageFunc(this, TRUE, TRUE);

	// set size bitmaps
	wxStaticBitmap *pbmp = (wxStaticBitmap*)FindWindowById(ID_BMP_TOOLBAR_SMALL);
	pbmp->SetBitmap(gpApp->wxGetBitmapFromMemory(edit_paste_png_16));
	pbmp = (wxStaticBitmap*)FindWindowById(ID_BMP_TOOLBAR_MEDIUM);
	pbmp->SetBitmap(gpApp->wxGetBitmapFromMemory(edit_paste_png_22));
	pbmp = (wxStaticBitmap*)FindWindowById(ID_BMP_TOOLBAR_LARGE);
	pbmp->SetBitmap(gpApp->wxGetBitmapFromMemory(edit_paste_png_32));

	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	// Create and populate the toolbar button list
	memcpy(m_bToolbarButtons, pApp->m_bToolbarButtons, sizeof(pApp->m_bToolbarButtons));
	PopulateList();

	// initial values
	m_toolbarSize = pApp->m_toolbarSize;
	wxRadioButton *pRdo = NULL;
	switch (m_toolbarSize)
	{
	case btnLarge:
		pRdo = (wxRadioButton*)FindWindowById(ID_RDO_TOOLBAR_LARGE);
		pRdo->SetValue(true);
		break;
	case btnMedium:
		pRdo = (wxRadioButton*)FindWindowById(ID_RDO_TOOLBAR_MEDIUM);
		pRdo->SetValue(true);
		break;
	case btnSmall:
	default:
		pRdo = (wxRadioButton*)FindWindowById(ID_RDO_TOOLBAR_SMALL);
		pRdo->SetValue(true);
		break;
	}
	m_bShowToobarText = pApp->m_bShowToolbarIconAndText;
	wxChoice *pShowText = (wxChoice*)FindWindowById(ID_CBO_TOOLBAR_ICON);
	pShowText->Select((m_bShowToobarText == true) ? 0 : 1); 
}

void CToolbarPagePrefs::PopulateList()
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	wxBitmap bmp;
	int index = 0;
	ToolbarButtonInfo tbInfo[] =
	{
		{wxID_NEW, _("New"), _("New"), _("Create a new document"), gpApp->wxGetBitmapFromMemory(document_new_png_16), wxNullBitmap, wxNullBitmap},
		{wxID_OPEN, _("Open"), _("Open"), _("Open an existing document"), gpApp->wxGetBitmapFromMemory(document_open_png_16), wxNullBitmap, wxNullBitmap},
		{wxID_SAVE, _("Save"), _("Save"), _("Save the active document"), gpApp->wxGetBitmapFromMemory(document_save_png_16), wxNullBitmap, wxNullBitmap},
		{0, _T(""), _T(""), _T(""), wxNullBitmap, wxNullBitmap, wxNullBitmap},
		{ID_EDIT_CUT, _("Cut"), _("Cut"), _("Cut the selection and put it on the Clipboard"), gpApp->wxGetBitmapFromMemory(edit_cut_png_16), wxNullBitmap, wxNullBitmap},
		{ID_EDIT_COPY, _("Copy"), _("Copy"), _("Copy the selection and put it on the Clipboard"), gpApp->wxGetBitmapFromMemory(edit_copy_png_16), wxNullBitmap, wxNullBitmap},
		{ID_EDIT_PASTE, _("Paste"), _("Paste"), _("Insert Clipboard contents"), gpApp->wxGetBitmapFromMemory(edit_paste_png_16), wxNullBitmap, wxNullBitmap},
		{0, _T(""), _T(""), _T(""), wxNullBitmap, wxNullBitmap, wxNullBitmap},
		{wxID_PRINT, _("Print"), _("Print"), _("Print the active document"), gpApp->wxGetBitmapFromMemory(document_print_png_16), wxNullBitmap, wxNullBitmap},
		{0, _T(""), _T(""), _T(""), wxNullBitmap, wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_GUESSER, _("Guesser"), _("Change Guesser Settings"), _("Change settings for guessing the translation text"), gpApp->wxGetBitmapFromMemory(dialog_guesser_png_16), wxNullBitmap, wxNullBitmap},
		{0, _T(""), _T(""), _T(""), wxNullBitmap, wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_CREATE_NOTE, _("Notes"), _("Open a Note dialog"), _("Create a note dialog and open it for typing"), gpApp->wxGetBitmapFromMemory(dialog_notes_png_16), wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_PREV_NOTE, _("Previous Note"), _("Jump to the previous Note"), _("Go back and open the previous note"), gpApp->wxGetBitmapFromMemory(note_prev_png_16), wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_NEXT_NOTE, _("Next Note"), _("Jump to the next Note"), _("Go forward and open the next note"), gpApp->wxGetBitmapFromMemory(note_next_png_16), wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_DELETE_ALL_NOTES, _("Delete All Notes"), _("Delete All Notes"), _("Delete all the notes currently in the document"), gpApp->wxGetBitmapFromMemory(note_delete_all_png_16), wxNullBitmap, wxNullBitmap},
		{0, _T(""), _T(""), _T(""), wxNullBitmap, wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_RESPECTING_BDRY, _("Ignore Boundaries"), _("Ignore Boundaries"), _("Ignore boundaries when making selections"), gpApp->wxGetBitmapFromMemory(bounds_stop_png_16), wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_SHOWING_PUNCT, _("Hide Punctuation"), _("Hide Punctuation"), _("Don't show punctuation with the text"), gpApp->wxGetBitmapFromMemory(format_show_punctuation_png_16), wxNullBitmap, wxNullBitmap},
		{0, _T(""), _T(""), _T(""), wxNullBitmap, wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_TO_END, _("End"), _("Advance to End"), _("Advance to the end of the data"), gpApp->wxGetBitmapFromMemory(go_last_png_16), wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_TO_START, _("Start"), _("Back to Start"), _("Go back to the start of the data"), gpApp->wxGetBitmapFromMemory(go_first_png_16), wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_STEP_DOWN, _("Down"), _("Move down one step"), _("Move the bundle down one step towards the bottom of the file"), gpApp->wxGetBitmapFromMemory(go_down_png_16), wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_STEP_UP, _("Up"), _("Move up one step"), _("Move bundle back up one step towards the start of the file"), gpApp->wxGetBitmapFromMemory(go_up_png_16), wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_BACK, _("Back"), _("Jump back"), _("Jump back to the last active location"), gpApp->wxGetBitmapFromMemory(go_previous_png_16), wxNullBitmap, wxNullBitmap},
		{0, _T(""), _T(""), _T(""), wxNullBitmap, wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_MERGE, _("New Phrase"), _("Make a phrase"), _("Merge selected words into a phrase"), gpApp->wxGetBitmapFromMemory(phrase_new_png_16), wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_RESTORE, _("Delete Phrase"), _("Unmake A Phrase"), _("Restore selected phrase to a sequence of word objects"), gpApp->wxGetBitmapFromMemory(phrase_remove_png_16), wxNullBitmap, wxNullBitmap},
		{0, _T(""), _T(""), _T(""), wxNullBitmap, wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_RETRANSLATION, _("New Retranslation"), _("Do A Retranslation"), _("The selected section is a retranslation, not an adaptation"), gpApp->wxGetBitmapFromMemory(retranslation_new_png_16), wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_EDIT_RETRANSLATION, _("Edit Retranslation"), _("Edit A Retranslation"), _("Edit the retranslation at the selection or at the active location"), gpApp->wxGetBitmapFromMemory(retranslation_edit_png_16), wxNullBitmap, wxNullBitmap},
		{ID_REMOVE_RETRANSLATION, _("Delete Retranslation"), _("Remove A Retranslation"), _("Remove the whole of the retranslation"), gpApp->wxGetBitmapFromMemory(retranslation_delete_png_16), wxNullBitmap, wxNullBitmap},
		{0, _T(""), _T(""), _T(""), wxNullBitmap, wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_NULL_SRC, _("New Placeholder"), _("Insert A Placeholder"), _("Insert a placeholder into the source language text"), gpApp->wxGetBitmapFromMemory(placeholder_new_png_16), wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_REMOVE_NULL_SRCPHRASE, _("Delete Placeholder"), _("Remove A Placeholder"), _("Restore selected phrase to a sequence of word objects"), gpApp->wxGetBitmapFromMemory(placeholder_delete_png_16), wxNullBitmap, wxNullBitmap},
		{0, _T(""), _T(""), _T(""), wxNullBitmap, wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_CHOOSE_TRANSLATION, _("Choose Translation"), _("Show The Choose Translation Dialog"), _("Force the Choose Translation dialog to be shown"), gpApp->wxGetBitmapFromMemory(dialog_choose_translation_png_16), wxNullBitmap, wxNullBitmap},
		{ID_SHOWING_ALL, _("Target Only"), _("Show Target Text Only"), _("Show target text only"), gpApp->wxGetBitmapFromMemory(show_source_target_png_16), wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_EARLIER_TRANSLATION, _("View Translations"), _("View Translation or Glosses Elsewhere in the Document"),
			_("View translation or glosses elsewhere in the document; locate them by chapter and verse"), gpApp->wxGetBitmapFromMemory(dialog_view_translation_or_glosses_png_16), wxNullBitmap, wxNullBitmap},
		{ID_BUTTON_NO_PUNCT_COPY, _("No Punctuation Copy"), _("No Punctuation Copy"), _("Suppress the copying of source text punctuation temporarily"), gpApp->wxGetBitmapFromMemory(punctuation_copy_png_16), wxNullBitmap, wxNullBitmap},
		{wxID_HELP, _("Help"), _("Help"), _("Display Adapt It program help topics"), gpApp->wxGetBitmapFromMemory(help_browser_png_16), wxNullBitmap, wxNullBitmap},
		{-1, _T(""), _T(""), _T(""), wxNullBitmap, wxNullBitmap, wxNullBitmap},
	};

	wxListCtrl* pLst = (wxListCtrl*)FindWindowById(ID_LST_TOOLBAR_BUTTONS);
	// clear out items if necessary
	pLst->ClearAll();
	// Add columns       
    wxListItem col0, col1;
    col0.SetId(0); // checkbox
    col0.SetWidth(22);
    col0.SetImage(-1);
    pLst->InsertColumn(0, col0);
    col1.SetId(1); // toolbar button (icon and text)
    col1.SetWidth(250);
    col1.SetImage(-1);
    pLst->InsertColumn(1, col1);

	// add the toolbar buttons
	wxImageList *il = new wxImageList(16, 16);

	// wxListCtrl block with native checkboxes -- taken from wxWiki
	// first, add images for checkboxes (checked and unchecked)
	wxBitmap unchecked_bmp(16, 16),
             checked_bmp(16, 16);
 
    // Bitmaps must not be selected by a DC for addition to the image list but I don't see
    // a way of diselecting them in wxMemoryDC so let's just use a code block to end the scope
    {
        wxMemoryDC renderer_dc;
 
        // Unchecked
        renderer_dc.SelectObject(unchecked_bmp);
        renderer_dc.SetBackground(*wxTheBrushList->FindOrCreateBrush(GetBackgroundColour(), wxSOLID));
        renderer_dc.Clear();
        wxRendererNative::Get().DrawCheckBox(this, renderer_dc, wxRect(0, 0, 16, 16), 0);
 
        // Checked
        renderer_dc.SelectObject(checked_bmp);
        renderer_dc.SetBackground(*wxTheBrushList->FindOrCreateBrush(GetBackgroundColour(), wxSOLID));
        renderer_dc.Clear();
        wxRendererNative::Get().DrawCheckBox(this, renderer_dc, wxRect(0, 0, 16, 16), wxCONTROL_CHECKED);
    }
    // the add order must respect the wxCLC_XXX_IMGIDX defines in the headers !
    il->Add(unchecked_bmp);
    il->Add(checked_bmp);
	// END wxListCtrl block with native checkboxes

	// now add the toolbar images
	while (tbInfo[index].toolId != -1)
	{
		if (tbInfo[index].toolId != 0)
		{
			bmp = tbInfo[index].bmpSmall;
			il->Add(bmp, wxNullBitmap);
		}
		index++;
	}
	pLst->SetImageList(il, wxIMAGE_LIST_SMALL);

	index = 0;
	long lItem = 0;

	// loop through the tbInfo array and add the toolbar buttons available to the current profile
	while (tbInfo[index].toolId != -1)
	{
		if (tbInfo[index].toolId == 0)
		{
			; // do nothing
		}
		else if ((pApp->m_nWorkflowProfile == 0) || (pApp->ToolBarItemIsVisibleInThisProfile(pApp->m_nWorkflowProfile, tbInfo[index].longHelpString)))
		{
			// this toolbar item is visible in this profile -- add it to the list
			wxListItem li;
			li.SetId(lItem);
			li.SetImage((m_bToolbarButtons[index] == true) ? 1 : 0); // set the check by setting the first column image
			li.SetText(_(""));
			li.SetData((long)index);
			long something = pLst->InsertItem(li);
			pLst->SetItem(something, 1, tbInfo[index].shortHelpString, ((int)lItem + 2 /* slot 0 and 1 are checkbox images */));
			li.SetStateMask(wxLIST_MASK_IMAGE | wxLIST_MASK_TEXT | wxLIST_MASK_DATA);
			lItem++;
		}
		// fetch the next item
		index++;
	}
	// set the column widths to auto-resize
//    pLst->SetColumnWidth( 0, wxLIST_AUTOSIZE );
//    pLst->SetColumnWidth( 1, wxLIST_AUTOSIZE );

}

// This InitDialog is called from the DoStartWorkingWizard() function
// in the App
void CToolbarPagePrefs::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
}

// OnOK() is used only in the EditPreferencesDlg
void CToolbarPagePrefs::OnOK(wxCommandEvent& WXUNUSED(event))
// This code taken from MFC's CPunctMap::OnOK() function
{
	// update the app's settings
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	// toolbar size and icon / text setting
	pApp->m_toolbarSize = m_toolbarSize;
	pApp->m_bShowToolbarIconAndText = m_bShowToobarText;

	// toolbar buttons
	wxListCtrl* pLst = (wxListCtrl*)FindWindowById(ID_LST_TOOLBAR_BUTTONS);
	wxListItem li;
	for (int i = 0L; i < pLst->GetItemCount(); i++)
	{
		li.SetId((long)i);
		li.SetMask(wxLIST_MASK_IMAGE | wxLIST_MASK_DATA);
		pLst->GetItem(li);
		// update the value for this item
		m_bToolbarButtons[(long)li.GetData()] = (li.GetImage() == 1) ? true : false;
	}
	// copy over the values from the member variable to the app's value
	memcpy(pApp->m_bToolbarButtons, m_bToolbarButtons, sizeof(m_bToolbarButtons));

	// tell UI to redraw itself
	pApp->ConfigureToolBarForUserProfile();
}

// Event handlers
void CToolbarPagePrefs::OnRadioToolbarSmall(wxCommandEvent& WXUNUSED(event))
{
	// update our internal value based on the new selection
	m_toolbarSize = btnSmall;
}

void CToolbarPagePrefs::OnRadioToolbarMedium (wxCommandEvent& WXUNUSED(event))
{
	// update our internal value based on the new selection
	m_toolbarSize = btnMedium;
}

void CToolbarPagePrefs::OnRadioToolbarLarge (wxCommandEvent& WXUNUSED(event))
{
	// update our internal value based on the new selection
	m_toolbarSize = btnLarge;
}

void CToolbarPagePrefs::OnCboToolbarIcon (wxCommandEvent& WXUNUSED(event))
{
	// update our internal value based on the new selection
	wxChoice *pShowText = (wxChoice*)FindWindowById(ID_CBO_TOOLBAR_ICON);
	m_bShowToobarText = (pShowText->GetSelection() == 0);
}

// 
// Minimal button selection from Bill's email from 6 May 2012 (minimal icon set) - 11 buttons:
// 1. Guesser dialog
// 2. Note dialog
// 3. Ignore boundaries / stop at boundaries
// 4. Show / hide punctuation
// 5. Make / unmake a phrase (* 2 buttons at the moment)
// 6. Retranslation / Edit Retranslation (* 2 buttons at the moment)
// 7. Insert / Remove placeholder (* 2 buttons at the moment)
// 8. Choose Translation dialog
// 9. Show Target / Source and Target
// 10. View Translations or Glosses elsewhere in the document
// 11. No punctuation copy / enable punctuation
void CToolbarPagePrefs::OnBnToolbarMinimal (wxCommandEvent& WXUNUSED(event))
{
	// TODO: 5-7 are separate buttons; these should be replaced by toggle buttons.
	wxListCtrl* pLst = (wxListCtrl*)FindWindowById(ID_LST_TOOLBAR_BUTTONS);
	// first, turn everything off
	for (int i = 0L; i < pLst->GetItemCount(); i++)
	{
		pLst->SetItemColumnImage(i, 0, 0);
	}
	// now turn on the onew we want
	pLst->SetItemColumnImage(7, 0, 1); // Guesser
	pLst->SetItemColumnImage(8, 0, 1); // Notes
	pLst->SetItemColumnImage(12, 0, 1); // Ignore Boundaries
	pLst->SetItemColumnImage(13, 0, 1); // Show Punctuation
	pLst->SetItemColumnImage(19, 0, 1); // Make Phrase
	pLst->SetItemColumnImage(20, 0, 1); // Unmake Phrase
	pLst->SetItemColumnImage(21, 0, 1); // New Retranslation
	pLst->SetItemColumnImage(22, 0, 1); // Edit Retranslation
	pLst->SetItemColumnImage(24, 0, 1); // New Placeholder
	pLst->SetItemColumnImage(25, 0, 1); // Delete Placeholder
	pLst->SetItemColumnImage(26, 0, 1); // Choose Translation
	pLst->SetItemColumnImage(27, 0, 1); // Show Target / Source and Target
	pLst->SetItemColumnImage(28, 0, 1); // View Translations
	pLst->SetItemColumnImage(29, 0, 1); // Punctuation Copy
}

void CToolbarPagePrefs::OnBnToolbarReset (wxCommandEvent& event)
{
	// small icon
	OnRadioToolbarSmall(event);
	// icon only (no text)
	wxChoice *pShowText = (wxChoice*)FindWindowById(ID_CBO_TOOLBAR_ICON);
	pShowText->Select((m_bShowToobarText == true) ? 0 : 1); 
	m_bShowToobarText = false;
	// initial button selection -- everything on
	wxListCtrl* pLst = (wxListCtrl*)FindWindowById(ID_LST_TOOLBAR_BUTTONS);
	for (int i = 0L; i < pLst->GetItemCount(); i++)
	{
		pLst->SetItemColumnImage(i, 0, 1);
	}
}

// EDB note 9 March 2013:
// There's probably a cleaner way to do this. We're running off the wxListCtrl's ItemSelected event
// to perform the check / uncheck on the item.
// The wxWiki uses a mouseUp event, but that's not tracking inside the wxListCtrl (it catches it for the
// ToolbarPage outside the control, though... sigh).
// Here we toggle the checkbox on the selected item, then deselect the item so that it can be checked again;
// this causes a slight delay in how quickly the control will catch the next event, but it does allow the
// item to be checked / unchecked in succession.
void CToolbarPagePrefs::OnClickLstToolbarButtons (wxListEvent& event)
{
	wxListCtrl* pLst = (wxListCtrl*)FindWindowById(ID_LST_TOOLBAR_BUTTONS);

	if (event.m_itemIndex != -1)
	{
		// found the selection -- now pull out the current image
		wxListItem li;
		li.SetId(event.m_itemIndex);
		li.SetMask(wxLIST_MASK_IMAGE);
		pLst->GetItem(li);
		// toggle the image
		pLst->SetItemColumnImage(event.m_itemIndex, 0, ((li.GetImage() == 1) ? 0 : 1));
		// deselect the item so it can be clicked again
		pLst->SetItemState(event.m_itemIndex, 0, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
	}
}

void CToolbarPagePrefs::SetLstCheck(long lItem, bool bChecked) 
{
	wxListCtrl* pLst = (wxListCtrl*)FindWindowById(ID_LST_TOOLBAR_BUTTONS);
	pLst->SetItemColumnImage(lItem, 0, (bChecked) ? 1 : 0);
}


bool CToolbarPagePrefs::GetLstCheck(long lItem)
{
	wxListCtrl* pLst = (wxListCtrl*)FindWindowById(ID_LST_TOOLBAR_BUTTONS);
	// wxListCtrl doesn't have a GetItemImage() call -- you need to call GetItem() 
	// and then pull out the image from the wxListItem
	wxListItem li;
	li.SetId(lItem);
	li.SetMask(wxLIST_MASK_IMAGE);
	pLst->GetItem(li);
	// return the checkbox value
	return (li.GetImage() == 1) ? true : false;
}
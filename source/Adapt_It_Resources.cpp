/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Adapt_It_Resources.cpp
/// \author			Bill Martin
/// \date_created	21 January 2019
/// \rcs_id $Id$
/// \copyright		2019 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file containing Adapt It's resources and resource 
/// functions used by Adapt It. These resources were created initially by wxDesigner. The wxDesigner
/// generated C++ code in Adapt_It_wdr.cpp was copied to this implementation file, and is now maintained
/// here for building into the Adapt It application rather than from wxDesigner and the old 
/// Adapt_It_wdr.cpp file. The Adapt_It_wdr.h and Adapt_It_wdr.cpp files are no longer a part of 
/// the Adapt It project. Changes and additions made within wxDesigner can continue to be 
/// exported to C++ into the Adapt_It_wdr.h and Adapt_It_wdr.cpp files, but those changes and
/// additions must be hand copied from Adapt_It_wdr.cpp over to this Adapt_It_Resources.cpp file 
/// where they can be tweaked apart from wxDesigner, and thereby remove problem/obsolete code that 
/// wxDesigner was generating. 
/// With the advent of wxWidgets version 3.1.x and later, the old wxDesigner generated code was
/// producing code that would cause run-time asserts mostly due to improper combinations of wxSizer
/// wxALIGNMENT... style flags. Also wxDesigner's generated code was also increasingly resulting in 
/// deprecated code warnings, such as those related to its use of its outdated wxFont constants, 
/// where it was using wxSWISS, wxNORMAL, wxBOLD, instead of the newer constants wxFONTFAMILY_SWISS, 
/// wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD.
///
/// NOTE: You might find it helpful to use a file comparison utility like WinMerge (Windows)
/// or Meld (Linux) to see where the differences are, and any new additions between the
/// wxDesigner generated Adapt_It_wdr.* files and the newer Adapt_It_Resources.* files.
/// 
/////////////////////////////////////////////////////////////////////////////
/// IMPORTANT: For any new or revised resource function that you do in wxDesigner
/// you must export it to C++ code which is saved in the old Adapt_It_wdr.h and
/// Adapt_It_wdr.cpp files. Then, make sure you hand copy the extern const int symbol 
/// declarations from the Adapt_It_wdr.h header file to an appropriate place into 
/// the Adapt_It_Resources.h file, and the C++ implementation code for the given
/// new/revised function from the old Adapt_It_wdr.cpp file into this Adapt_It_Resources.cpp
/// file. If you fail to copy the code from the old wxDesigner files into the new
/// resource files (Adapt_It_Resources.*), your coding changes made within 
/// wxDesigner will not be recognized when compiled - as of 21 January 2019, only the 
/// Adapt_It_Resources.h and Adapt_It_Resources.cpp files are now included within
/// the Adapt It project.
/////////////////////////////////////////////////////////////////////////////
/// IMPORTANT: If/When wxDesigner is used in the future, ensure that its
/// "Write C++ source" dialog has the third radio button option:
/// (.) int ID_TEXT; ID_TEXT = wxNewId(); selected
/// so that it doesn't create IDs in succession from a certain starting point,
/// since to do so will make it difficult to copy new wxDesigner function code
/// from wxDesigner's Adapt_It_wrd.h and Adapt_It_wrd.cpp file to the new
/// Adapt_It_Resources.h and Adapt_It_Resources.cpp files.
/// Previously all ID numbers were generated automatically within wxDesigner,
/// whereas they should from this point forward be simply generated on the fly
/// by the use of wxNewId() statements in the resource code.
/////////////////////////////////////////////////////////////////////////////
/// These Functions in Adapt_It_Resources.cpp required modifications from their
/// wxDesigner C++ generated code:
/// AboutDlgFunc() - updated deprecated wxFont constants(2x)
/// LanguagesPageFunc() - updated deprecated wxFont constants(1x)
/// FontsPageFunc() - updated deprecated wxFont constants(1x)
/// CaseEquivDlgFunc() - updated deprecated wxFont constants(3x)
/// WelcomeDlgFunc() - updated deprecated wxFont constants(2x)
/// DocPageFunc() - updated deprecated wxFont constants(1x)
/// ProjectPageFunc() - updated deprecated wxFont constants(1x)
/// BackupsAndKBPageFunc() - updated deprecated wxFont constants(1x)
/// ViewPageFunc() - updated deprecated wxFont constants(1x)
/// UnitsPageFunc() - updated deprecated wxFont constants(1x)
/// AutoSavingPageFunc() - updated deprecated wxFont constants(1x)
/// XMLErrorDlgFunc() - updated deprecated wxFont constants(1x)
/// ConsistencyCheckDlgFunc() - updated deprecated wxFont constants(1x)
/// KBEditorPanelFunc() - updated deprecated wxFont constants(1x)
/// SetEncodingDlgFunc() - updated deprecated wxFont constants(3x)
/// PrintOptionsDlgFunc() - updated deprecated wxFont constants(1x)
/// PunctCorrespPageFunc() - updated deprecated wxFont constants(1x)
/// UsfmFilterPageFunc() - updated deprecated wxFont constants(1x)
/// ConsistencyCheck_EmptyNoTU_DlgFunc() - updated deprecated wxFont constants(2x)
/// ConsistencyCheck_ExistsNoTU_DlgFunc() - updated deprecated wxFont constants(1x)
/// BookNameDlgFunc() - updated deprecated wxFont constants(1x)
/// ToolbarPageFunc() - updated deprecated wxFont constants(1x)
/// DVCSNavDlgFunc() - updated deprecated wxFont constants(3x)
/// SharedKBManagerUsersPageFunc2() - updated deprecated wxFont constants(1x)
/// SharedKBManager_CreateKbsPageFunc2() - updated deprecated wxFont constants(1x)
/// Bulk_Delete_Pairs_Func() - updated deprecated wxFont constants(1x)
/// GuesserAffixListDlgFunc() - updated deprecated wxFont constants(6x)
/// SharedKBManager_CreateCodesPageFunc() - updated deprecated wxFont constants(1x)
/// AI_PT_ConflictingVersesFunc() - updated deprecated wxFont constants(3x)
/// ConflictResolutionActionFunc() - updated deprecated wxFont constants(3x)
/// ServDisc_KBserversDlg() - updated deprecated wxFont constants(2x)
/// CollabProjectMigrationDlgFunc() - updated deprecated wxFont constants(3x)
///
/// [Document other changes here]
///
/////////////////////////////////////////////////////////////////////////////

#include "Adapt_It_wdr.cpp"

// ************** You can generate new dialogs within wxDesigner but *****************
// ************** copy the code it generates in Adapt_It_wdr.cpp for *****************
// ************** a new resource function, placing that new function *****************
// ************** into this implementation file for actual incorporation *************
// ************** into the Adapt It code base for compilation. ***********************




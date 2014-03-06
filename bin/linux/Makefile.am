# these are the headers for your project
noinst_HEADERS = $(wildcard $(srcdir)/../../source/*.h)

#########################################################################
# APPLICATION SECTION
#########################################################################

# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = adaptit

# Install either the x86-64 or x86 (80386) binary build of bibledit-rdwrt
# Detect the architecture based on /bin/ls. If the architecture is not
# x86-64 or 80386 then error.
#be_arch_name = $(shell file /bin/ls | sed 's/.* \(\w*86-*\w*\).*/\1/')
#be_arch_dir = $(if $(filter x86-64,$(be_arch_name)),x86_64,$(if $(filter 80386,$(be_arch_name)),x86,$(error Unknown architecture for adaptit-bibledit-rdwrt)))
#bin_SCRIPTS = ../adaptit-bibledit-rdwrt/lucid-$(be_arch_dir)/adaptit-bibledit-rdwrt

# Localization data
SUBDIRS = po
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = config.rpath

#
# Main resource files and help data
# (part of main adaptit package)
#
install-adaptit-data: install-xmlDATA install-miscDATA \
	install-htmlmainDATA install-docAdminDATA install-docStartDATA

xmldir = $(pkgdatadir)
xml_DATA = ../../xml/AI_USFM.xml ../../xml/AI_UserProfiles.xml \
	../../xml/books.xml ../../xml/aiDefault.css
miscdir = $(pkgdatadir)
misc_DATA = ../../docs/iso639-3codes.txt \
        ../../docs/curl-ca-bundle.crt

# HTML help data
htmlmaindir = $(pkgdatadir)/help
htmlmain_DATA = ../../docs/Help_for_Administrators.htm \
	../../docs/Adapt_It_Quick_Start.htm \
	../../docs/RFC5646message.htm
docAdmindir = $(pkgdatadir)/help/Images/Admin_help
docAdmin_DATA = $(wildcard $(srcdir)/../../docs/Images/Admin_help/*)
docStartdir = $(pkgdatadir)/help/Images/Adapt_It_Quick_Start
docStart_DATA = $(wildcard $(srcdir)/../../docs/Images/Adapt_It_Quick_Start/*)

#
# Optional resource files and help data
# (part of adaptit documentation package)
#
htmlhelpdir = $(pkgdatadir)/help
htmlhelp_DATA = adaptit.htb
install-adaptit-docs-data: install-htmlhelpDATA install-docFileDATA

docFiledir = $(pkgdatadir)/help
docFile_DATA = ../../docs/Localization_Readme.txt

# Rule to build adaptit.htb
adaptit.htb: ../../hlp
	 (cd $<; zip -qr - . -x \*.svn\* ) > $@

#
# the application source, library search path, and link libraries
#
adaptit_SOURCES = \
	../../source/AIPrintout.cpp ../../source/AIPrintPreviewFrame.cpp ../../source/Adapt_It.cpp \
	../../source/Adapt_ItCanvas.cpp ../../source/Adapt_ItDoc.cpp ../../source/Adapt_ItView.cpp \
	../../source/Adapt_It_wdr.cpp ../../source/AdminMoveOrCopy.cpp \
	../../source/AssignLocationsForInputsAndOutputs.cpp \
	../../source/AdminEditMenuProfile.cpp ../../source/AutoSavingPage.cpp \
	../../source/base64.cpp ../../source/BString.cpp ../../source/BookNameDlg.cpp \
	../../source/CCModule.cpp ../../source/CCTabbedDialog.cpp \
	../../source/CCTableEditDlg.cpp ../../source/CCTableNameDlg.cpp \
	../../source/CaseEquivPage.cpp ../../source/Cell.cpp \
	../../source/ChooseConsistencyCheckTypeDlg.cpp ../../source/ChooseLanguageDlg.cpp ../../source/CollabUtilities.cpp \
	../../source/conschk_exists_notu_dlg.cpp ../../source/ConsChk_Empty_noTU_Dlg.cpp ../../source/ChooseLanguageCode.cpp \
	../../source/ChooseCollabOptionsDlg.cpp \
	../../source/ChooseTranslation.cpp ../../source/ClientServerConnection.cpp ../../source/CollectBacktranslations.cpp \
	../../source/ComposeBarEditBox.cpp ../../source/ConsistencyCheckDlg.cpp \
	../../source/ConsistentChanger.cpp ../../source/convauto.cpp \
	../../source/CorGuess.cpp ../../source/CreateNewAIProjForCollab.cpp \
	../../source/DocPage.cpp ../../source/EarlierTranslationDlg.cpp \
	../../source/EditPreferencesDlg.cpp ../../source/EditSourceTextDlg.cpp ../../source/EmailReportDlg.cpp \
	../../source/DVCS.cpp ../../source/DVCSNavDlg.cpp ../../source/DVCSLogDlg.cpp \
	../../source/ExportFunctions.cpp \
	../../source/ExportInterlinearDlg.cpp ../../source/ExportOptionsDlg.cpp ../../source/ExportSaveAsDlg.cpp \
	../../source/FilenameConflictDlg.cpp ../../source/FindReplace.cpp \
	../../source/FontPage.cpp ../../source/FreeTrans.cpp ../../source/FreeTransAdjustDlg.cpp ../../source/FreeTransSplitterDlg.cpp \
	../../source/GetSourceTextFromEditor.cpp \
	../../source/GoToDlg.cpp ../../source/GuesserAffix.cpp ../../source/GuesserSettingsDlg.cpp \
	../../source/HtmlFileViewer.cpp \
	../../source/json_defs.h ../../source/jsonreader.cpp ../../source/jsonreader.h \
	../../source/jsonval.cpp ../../source/jsonval.h \
	../../source/jsonwriter.cpp ../../source/jsonwriter.h \
	../../source/JoinDialog.cpp ../../source/KB.cpp ../../source/KBExportImportOptionsDlg.cpp \
	../../source/KBEditor.cpp ../../source/KBEditSearch.cpp ../../source/KBPage.cpp \
	../../source/KbServer.cpp ../../source/KBSharing.cpp ../../source/KBSharingMgrTabbedDlg.cpp \
	../../source/KBSharingStatelessSetupDlg.cpp ../../source/KbSharingSetup.cpp \
	../../source/LanguageCodesDlg.cpp ../../source/LanguagesPage.cpp ../../source/Layout.cpp \
	../../source/ListDocumentsInOtherFolderDialog.cpp ../../source/MainFrm.cpp ../../source/md5.cpp \
	../../source/md5_SB.cpp ../../source/MergeUpdatedSrc.cpp \
	../../source/MoveDialog.cpp \
	../../source/Mover.cpp ../../source/MyListBox.cpp ../../source/NavProtectNewDoc.cpp \
	../../source/NoteDlg.cpp ../../source/Notes.cpp \
	../../source/OpenExistingProjectDlg.cpp ../../source/OutputFilenameDlg.cpp \
	../../source/Pathway.cpp ../../source/PeekAtFile.cpp ../../source/PhraseBox.cpp \
	../../source/Pile.cpp ../../source/Placeholder.cpp ../../source/PlaceInternalMarkers.cpp \
	../../source/PlaceInternalPunct.cpp ../../source/PlaceRetranslationInternalMarkers.cpp \
	../../source/PrintOptionsDlg.cpp ../../source/ProjectPage.cpp ../../source/PunctCorrespPage.cpp \
	../../source/ReadOnlyProtection.cpp ../../source/RefString.cpp \
	../../source/RemoveSomeTgtEntries.cpp \
	../../source/RefStringMetadata.cpp ../../source/Retranslation.cpp ../../source/RetranslationDlg.cpp \
	../../source/scrollingdialog.cpp ../../source/scrollingwizard.cpp \
	../../source/SetDelay.cpp ../../source/SetEncodingDlg.cpp ../../source/SetupEditorCollaboration.cpp \
	../../source/SilConverterSelectDlg.cpp \
	../../source/SourcePhrase.cpp ../../source/SplitDialog.cpp ../../source/Stack.cpp \
	../../source/StartWorkingWizard.cpp ../../source/StatusBar.cpp \
	../../source/Strip.cpp ../../source/TargetUnit.cpp \
	../../source/tellenc.cpp ../../source/Thread_BulkPseudoDelete.cpp \
	../../source/Thread_ChangedSince.cpp ../../source/Thread_CreateEntry.cpp \
	../../source/Thread_DoEntireKbDeletion.cpp \
	../../source/Thread_KbEditorUpdateButton.cpp ../../source/Thread_PseudoDelete.cpp \
	../../source/Thread_PseudoUndelete.cpp ../../source/Thread_UploadMulti.cpp \
	../../source/Thread_UploadToKBServer.cpp ../../source/Timer_KbServerChangedSince.cpp \
	../../source/ToolbarPage.cpp ../../source/TransformToGlossesDlg.cpp \
	../../source/UsernameInput.cpp ../../source/UsfmFilterPage.cpp \
	../../source/UnitsDlg.cpp ../../source/UnitsPage.cpp ../../source/UnpackWarningDlg.cpp \
	../../source/Uuid_AI.cpp ../../source/ViewFilteredMaterialDlg.cpp \
	../../source/ViewPage.cpp ../../source/WaitDlg.cpp \
	../../source/Welcome.cpp ../../source/WhichBook.cpp ../../source/WhichFilesDlg.cpp \
	../../source/Xhtml.cpp \
	../../source/XML.cpp ../../source/XMLErrorDlg.cpp ../../source/helpers.cpp

adaptit_LDFLAGS = $(WX_LIBS) $(CURL_LIBS) -luuid



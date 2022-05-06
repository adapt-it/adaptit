# these are the headers for your project
noinst_HEADERS = $(wildcard $(srcdir)/../../source/*.h)

#########################################################################
# APPLICATION SECTION
#########################################################################

# whm added the following due to "warning: source file '../../source/*.cpp' is in a subdirectory,
# ...but option 'subdir-objects' is disabled"
#AUTOMAKE_OPTIONS = subdir-objects

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
	../../xml/books.xml ../../xml/aiDefault.css \
	../../docs/CC/FwdSlashInsertAtPuncts.cct \
	../../docs/CC/FwdSlashRemoveAtPuncts.cct
miscdir = $(pkgdatadir)
misc_DATA = ../../docs/iso639-3codes.txt \
        ../../docs/curl-ca-bundle.crt

# whm testing rule below
#distdir = $(pkgdatadir)/dist
#distdir:
#	mkdir -p distdir

# AFM font data
afmdir = $(pkgdatadir)/gs_afm
afm_DATA = $(wildcard $(srcdir)/../../gs_afm/*)

# HTML help data
htmlmaindir = $(pkgdatadir)/help
htmlmain_DATA = ../../docs/Help_for_Administrators.htm \
	../../docs/Adapt_It_Quick_Start.htm \
	../../docs/RFC5646message.htm \
	../../docs/GuesserExplanation.htm
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
docFile_DATA = ../../docs/Localization_Readme.txt \
	../../docs/Tok_Pisin_fragment_1John.txt \
	../../docs/KJV_1Jn_2.12-17.txt

# Rule to build adaptit.htb
adaptit.htb: ../../hlp
	 (cd $<; zip -qr - . -x \*.svn\* ) > $@

#
# the application source, library search path, and link libraries
#
adaptit_SOURCES = \
	../../source/Adapt_It.cpp \
	../../source/Adapt_ItCanvas.cpp \
	../../source/Adapt_ItDoc.cpp \
	../../source/Adapt_It_Resources.cpp \
	../../source/Adapt_ItView.cpp \
	../../source/AdminEditMenuProfile.cpp \
	../../source/AdminMoveOrCopy.cpp \
	../../source/AIPrintout.cpp \
	../../source/AIPrintPreviewFrame.cpp \
	../../source/AssignLocationsForInputsAndOutputs.cpp \
	../../source/Authenticate2Dlg.cpp \
	../../source/AutoSavingPage.cpp \
	../../source/base64.cpp \
	../../source/BookNameDlg.cpp \
	../../source/BString.cpp \
	../../source/CaseEquivPage.cpp \
	../../source/CCModule.cpp \
	../../source/CCTabbedDialog.cpp \
	../../source/CCTableEditDlg.cpp \
	../../source/CCTableNameDlg.cpp \
	../../source/Cell.cpp \
	../../source/ChooseCollabOptionsDlg.cpp \
	../../source/ChooseConsistencyCheckTypeDlg.cpp \
	../../source/ChooseLanguageCode.cpp \
	../../source/ChooseLanguageDlg.cpp \
	../../source/ChooseTranslation.cpp \
	../../source/ClientServerConnection.cpp \
	../../source/CollabProjectMigrationDlg.cpp \
	../../source/CollabProtectEditorBulkSettingsDlg.cpp \
	../../source/CollabUtilities.cpp \
	../../source/CollabVerseConflictDlg.cpp \
	../../source/CollectBacktranslations.cpp \
	../../source/ComposeBarEditBox.cpp \
	../../source/ConflictResActionDlg.cpp \
	../../source/ConsChk_Empty_noTU_Dlg.cpp \
	../../source/conschk_exists_notu_dlg.cpp \
	../../source/ConsistencyCheckDlg.cpp \
	../../source/ConsistentChanger.cpp \
	../../source/convauto.cpp \
	../../source/CorGuess.cpp \
	../../source/CreateNewAIProjForCollab.cpp \
	../../source/DocPage.cpp \
	../../source/DVCS.cpp \
	../../source/DVCSLogDlg.cpp \
	../../source/DVCSNavDlg.cpp \
	../../source/EarlierTranslationDlg.cpp \
	../../source/EditPreferencesDlg.cpp \
	../../source/EditSourceTextDlg.cpp \
	../../source/EmailReportDlg.cpp \
	../../source/ExportFunctions.cpp \
	../../source/ExportInterlinearDlg.cpp \
	../../source/ExportOptionsDlg.cpp \
	../../source/ExportSaveAsDlg.cpp \
	../../source/FilenameConflictDlg.cpp \
	../../source/FindReplace.cpp \
	../../source/FontPage.cpp \
	../../source/FreeTrans.cpp \
	../../source/FreeTransAdjustDlg.cpp \
	../../source/FreeTransSplitterDlg.cpp \
	../../source/GetSourceTextFromEditor.cpp \
	../../source/GoToDlg.cpp \
	../../source/GuesserAffix.cpp \
	../../source/GuesserAffixesListsDlg.cpp \
	../../source/GuesserSettingsDlg.cpp \
	../../source/helpers.cpp \
	../../source/HtmlFileViewer.cpp \
	../../source/InstallGitOptionsDlg.cpp \
	../../source/JoinDialog.cpp \
	../../source/jsonreader.cpp \
	../../source/jsonval.cpp \
	../../source/jsonwriter.cpp \
	../../source/KB.cpp \
	../../source/KBEditor.cpp \
	../../source/KBEditSearch.cpp \
	../../source/KBExportImportOptionsDlg.cpp \
	../../source/KBPage.cpp \
	../../source/KbServer.cpp \
	../../source/KBSharing.cpp \
	../../source/KBSharingAuthenticationDlg.cpp \
	../../source/KBSharingMgrTabbedDlg.cpp \
	../../source/KbSharingSetup.cpp \
	../../source/KbSvrHowGetUrl.cpp \
	../../source/LanguageCodesDlg.cpp \
	../../source/LanguageCodesDlg_Single.cpp \
	../../source/LanguagesPage.cpp \
	../../source/Layout.cpp \
	../../source/ListDocumentsInOtherFolderDialog.cpp \
	../../source/MainFrm.cpp \
	../../source/md5.cpp \
	../../source/md5_SB.cpp \
	../../source/MergeUpdatedSrc.cpp \
	../../source/MoveDialog.cpp \
	../../source/Mover.cpp \
	../../source/MyListBox.cpp \
	../../source/NavProtectNewDoc.cpp \
	../../source/NewUserCredentialsDlg.cpp \
	../../source/NoteDlg.cpp \
	../../source/Notes.cpp \
	../../source/OpenExistingProjectDlg.cpp \
	../../source/OutputFilenameDlg.cpp \
	../../source/Pathway.cpp \
	../../source/PeekAtFile.cpp \
	../../source/PhraseBox.cpp \
	../../source/Pile.cpp \
	../../source/Placeholder.cpp \
	../../source/PlaceInternalMarkers.cpp \
	../../source/PlaceInternalPunct.cpp \
	../../source/PlaceRetranslationInternalMarkers.cpp \
	../../source/PrintOptionsDlg.cpp \
	../../source/ProjectPage.cpp \
	../../source/PunctCorrespPage.cpp \
	../../source/ReadOnlyProtection.cpp \
	../../source/RefString.cpp \
	../../source/RefStringMetadata.cpp \
	../../source/RemoveSomeTgtEntries.cpp \
	../../source/Retranslation.cpp \
	../../source/RetranslationDlg.cpp \
	../../source/scrollingdialog.cpp \
	../../source/scrollingwizard.cpp \
	../../source/ServDisc_KBserversDlg.cpp \
	../../source/SetDelay.cpp \
	../../source/SetEncodingDlg.cpp \
	../../source/SetupEditorCollaboration.cpp \
	../../source/SilConverterSelectDlg.cpp \
	../../source/SourcePhrase.cpp \
	../../source/SplitDialog.cpp \
	../../source/Stack.cpp \
	../../source/StartWorkingWizard.cpp \
	../../source/StatusBar.cpp \
	../../source/Strip.cpp \
	../../source/TargetUnit.cpp \
	../../source/tellenc.cpp \
	../../source/Timer_KbServerChangedSince.cpp \
	../../source/ToolbarPage.cpp \
	../../source/TransformToGlossesDlg.cpp \
	../../source/UnitsDlg.cpp \
	../../source/UnitsPage.cpp \
	../../source/UnpackWarningDlg.cpp \
	../../source/UsernameInput.cpp \
	../../source/UsfmFilterPage.cpp \
	../../source/Uuid_AI.cpp \
	../../source/ViewFilteredMaterialDlg.cpp \
	../../source/ViewPage.cpp \
	../../source/WaitDlg.cpp \
	../../source/Welcome.cpp \
	../../source/WhichBook.cpp \
	../../source/WhichFilesDlg.cpp  \
	../../source/Xhtml.cpp \
	../../source/XML.cpp \
	../../source/XMLErrorDlg.cpp

adaptit_LDFLAGS = $(WX_LIBS) $(CURL_LIBS) -luuid



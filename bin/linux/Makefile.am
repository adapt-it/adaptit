# these are the headers for your project
noinst_HEADERS = $(wildcard $(srcdir)/../../source/*.h)

#########################################################################
# APPLICATION SECTION
#########################################################################

# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = adaptit

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
	../../xml/books.xml
miscdir = $(pkgdatadir)
misc_DATA = ../../docs/iso639-3codes.txt \
        ../../docs/curl-ca-bundle.crt \
	$(wildcard $(srcdir)/../../license/*.txt) \
	bibledit-rdwrt/lucid-$(host_cpu)/bibledit-rdwrt

# HTML help data
htmlmaindir = $(pkgdatadir)/help
htmlmain_DATA = ../../docs/Help_for_Administrators.htm \
	../../docs/Adapt_It_Quick_Start.htm
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
	../../source/base64.cpp ../../source/BString.cpp ../../source/CCModule.cpp ../../source/CCTabbedDialog.cpp \
	../../source/CCTableEditDlg.cpp ../../source/CCTableNameDlg.cpp \
	../../source/CaseEquivPage.cpp ../../source/Cell.cpp \
	../../source/ChooseConsistencyCheckTypeDlg.cpp ../../source/ChooseLanguageDlg.cpp ../../source/CollabUtilities.cpp \
	../../source/conschk_exists_notu_dlg.cpp ../../source/ConsChk_Empty_noTU_Dlg.cpp \
	../../source/ChooseTranslation.cpp ../../source/CollectBacktranslations.cpp \
	../../source/ComposeBarEditBox.cpp ../../source/ConsistencyCheckDlg.cpp \
	../../source/ConsistentChanger.cpp ../../source/convauto.cpp \
	../../source/CorGuess.cpp ../../source/DocPage.cpp ../../source/EarlierTranslationDlg.cpp \
	../../source/EditPreferencesDlg.cpp ../../source/EditSourceTextDlg.cpp ../../source/EmailReportDlg.cpp \
	../../source/ExportFunctions.cpp \
	../../source/ExportInterlinearDlg.cpp ../../source/ExportOptionsDlg.cpp ../../source/ExportSaveAsDlg.cpp \
	../../source/FilenameConflictDlg.cpp ../../source/FindReplace.cpp \
	../../source/FontPage.cpp ../../source/FreeTrans.cpp ../../source/GetSourceTextFromEditor.cpp \
	../../source/GoToDlg.cpp ../../source/GuesserSettingsDlg.cpp ../../source/HtmlFileViewer.cpp \
	../../source/JoinDialog.cpp ../../source/KB.cpp \
	../../source/KBEditor.cpp ../../source/KBEditSearch.cpp ../../source/KBPage.cpp \
	../../source/LanguageCodesDlg.cpp ../../source/LanguagesPage.cpp ../../source/Layout.cpp \
	../../source/ListDocumentsInOtherFolderDialog.cpp ../../source/MainFrm.cpp ../../source/md5.cpp \
	../../source/MergeUpdatedSrc.cpp \
	../../source/MoveDialog.cpp \
	../../source/Mover.cpp ../../source/MyListBox.cpp ../../source/NavProtectNewDoc.cpp \
	../../source/NoteDlg.cpp ../../source/Notes.cpp \
	../../source/OpenExistingProjectDlg.cpp ../../source/OutputFilenameDlg.cpp \
	../../source/PeekAtFile.cpp ../../source/PhraseBox.cpp \
	../../source/Pile.cpp ../../source/Placeholder.cpp ../../source/PlaceInternalMarkers.cpp \
	../../source/PlaceInternalPunct.cpp ../../source/PlaceRetranslationInternalMarkers.cpp \
	../../source/PrintOptionsDlg.cpp ../../source/ProjectPage.cpp ../../source/PunctCorrespPage.cpp \
	../../source/ReadOnlyProtection.cpp ../../source/RefString.cpp \
	../../source/RefStringMetadata.cpp ../../source/Retranslation.cpp ../../source/RetranslationDlg.cpp \
	../../source/SetDelay.cpp ../../source/SetEncodingDlg.cpp ../../source/SetupEditorCollaboration.cpp \
	../../source/SilConverterSelectDlg.cpp \
	../../source/SourcePhrase.cpp ../../source/SplitDialog.cpp ../../source/Stack.cpp \
	../../source/StartWorkingWizard.cpp ../../source/Strip.cpp ../../source/TargetUnit.cpp \
	../../source/tellenc.cpp ../../source/TransformToGlossesDlg.cpp \
	 ../../source/UsfmFilterPage.cpp \
	../../source/UnitsDlg.cpp ../../source/UnitsPage.cpp ../../source/UnpackWarningDlg.cpp \
	../../source/Uuid_AI.cpp ../../source/ViewFilteredMaterialDlg.cpp \
	../../source/ViewPage.cpp ../../source/WaitDlg.cpp \
	../../source/Welcome.cpp ../../source/WhichBook.cpp ../../source/WhichFilesDlg.cpp \
	../../source/XML.cpp ../../source/XMLErrorDlg.cpp ../../source/helpers.cpp

adaptit_LDFLAGS = $(WX_LIBS) $(CURL_LIBS) -luuid



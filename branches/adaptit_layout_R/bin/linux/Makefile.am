# these are the headers for your project
noinst_HEADERS = ../../source/AIPrintout.h ../../source/Adapt_It.h \
	../../source/Adapt_ItCanvas.h ../../source/Adapt_ItDoc.h ../../source/Adapt_ItView.h \
	../../source/Adapt_It_Resources.h ../../source/Adapt_It_wdr.h ../../source/AdaptitConstants.h \
	../../source/AutoSavingPage.h ../../source/BString.h ../../source/CCModule.h \
	../../source/CCTabbedDialog.h ../../source/CCTableEditDlg.h ../../source/CCTableNameDlg.h \
	../../source/CaseEquivPage.h ../../source/Cell.h ../../source/ChooseConsistencyCheckTypeDlg.h \
	../../source/ChooseLanguageDlg.h ../../source/ChooseTranslation.h ../../source/CollectBacktranslations.h \
	../../source/ComposeBarEditBox.h ../../source/ConsistencyCheckDlg.h ../../source/ConsistentChanger.h \
	../../source/DocPage.h ../../source/EarlierTranslationDlg.h ../../source/EditPreferencesDlg.h \
	../../source/EditSourceTextDlg.h ../../source/ExportFunctions.h ../../source/ExportInterlinearDlg.h \
	../../source/ExportOptionsDlg.h ../../source/ExportSaveAsDlg.h ../../source/FilterPage.h \
	../../source/FindReplace.h ../../source/FontPage.h ../../source/GoToDlg.h \
	../../source/JoinDialog.h ../../source/KB.h ../../source/KBEditor.h ../../source/KBPage.h \
	../../source/LanguagesPage.h ../../source/Layout.h ../../source/ListDocumentsInOtherFolderDialog.h ../../source/MainFrm.h \
	../../source/MoveDialog.h ../../source/Mover.h ../../source/MyListBox.h ../../source/NoteDlg.h \
	../../source/OpenExistingProjectDlg.h ../../source/OutputFilenameDlg.h ../../source/PhraseBox.h \
	../../source/Pile.h ../../source/PlaceInternalMarkers.h ../../source/PlaceInternalPunct.h \
	../../source/PlaceRetranslationInternalMarkers.h ../../source/PrintOptionsDlg.h ../../source/ProjectPage.h \
	../../source/PunctCorrespPage.h ../../source/RefString.h ../../source/RetranslationDlg.h \
	../../source/SetDelay.h ../../source/SetEncodingDlg.h ../../source/SilConverterSelectDlg.h \
	../../source/SourcePhrase.h ../../source/SplitDialog.h ../../source/Stack.h \
	../../source/StartWorkingWizard.h ../../source/Strip.h ../../source/TargetUnit.h \
	../../source/TransformToGlossesDlg.h ../../source/USFMPage.h ../../source/UnitsDlg.h \
	../../source/UnitsPage.h ../../source/UnpackWarningDlg.h ../../source/ViewFilteredMaterialDlg.h \
	../../source/ViewPage.h ../../source/WaitDlg.h ../../source/Welcome.h ../../source/WhichBook.h \
	../../source/WhichFilesDlg.h ../../source/XML.h ../../source/XMLErrorDlg.h ../../source/helpers.h

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

# Resource files and help data
xmldir = $(pkgdatadir)
xml_DATA = ../../xml/AI_USFM.xml ../../xml/books.xml

# HTML help data
htmlhelpdir = $(pkgdatadir)/help
htmlhelp_DATA = adaptit.htb

#doc_DATA = $(shell ls ../../docs/*.htm ../../docs/*.txt ../../docs/*.doc)
#docImagesdir = $(docdir)/Images/Adapt_It_Quick_Start/
#docImages_DATA = $(shell ls ../../docs/Images/Adapt_It_Quick_Start/*.gif)

# Rule to build adaptit.htb
adaptit.htb: ../../hlp
	 (cd $<; zip -qr - . -x \*.svn\* ) > $@

# the application source, library search path, and link libraries
adaptit_SOURCES = ../../source/AIPrintout.cpp ../../source/Adapt_It.cpp \
	../../source/Adapt_ItCanvas.cpp ../../source/Adapt_ItDoc.cpp ../../source/Adapt_ItView.cpp \
	../../source/Adapt_It_wdr.cpp ../../source/AutoSavingPage.cpp ../../source/BString.cpp \
	../../source/CCModule.cpp ../../source/CCTabbedDialog.cpp ../../source/CCTableEditDlg.cpp \
	../../source/CCTableNameDlg.cpp ../../source/CaseEquivPage.cpp ../../source/Cell.cpp \
	../../source/ChooseConsistencyCheckTypeDlg.cpp ../../source/ChooseLanguageDlg.cpp ../../source/ChooseTranslation.cpp \
	../../source/CollectBacktranslations.cpp ../../source/ComposeBarEditBox.cpp ../../source/ConsistencyCheckDlg.cpp \
	../../source/ConsistentChanger.cpp ../../source/DocPage.cpp ../../source/EarlierTranslationDlg.cpp \
	../../source/EditPreferencesDlg.cpp ../../source/EditSourceTextDlg.cpp ../../source/ExportFunctions.cpp \
	../../source/ExportInterlinearDlg.cpp ../../source/ExportOptionsDlg.cpp ../../source/ExportSaveAsDlg.cpp \
	../../source/FilterPage.cpp ../../source/FindReplace.cpp ../../source/FontPage.cpp \
	../../source/GoToDlg.cpp ../../source/JoinDialog.cpp ../../source/KB.cpp \
	../../source/KBEditor.cpp ../../source/KBPage.cpp ../../source/LanguagesPage.cpp ../../source/Layout.cpp \
	../../source/ListDocumentsInOtherFolderDialog.cpp ../../source/MainFrm.cpp ../../source/MoveDialog.cpp \
	../../source/Mover.cpp ../../source/MyListBox.cpp ../../source/NoteDlg.cpp \
	../../source/OpenExistingProjectDlg.cpp ../../source/OutputFilenameDlg.cpp ../../source/PhraseBox.cpp \
	../../source/Pile.cpp ../../source/PlaceInternalMarkers.cpp \
	../../source/PlaceInternalPunct.cpp ../../source/PlaceRetranslationInternalMarkers.cpp \
	../../source/PrintOptionsDlg.cpp ../../source/ProjectPage.cpp ../../source/PunctCorrespPage.cpp \
	../../source/RefString.cpp ../../source/RetranslationDlg.cpp ../../source/SetDelay.cpp \
	../../source/SetEncodingDlg.cpp ../../source/SilConverterSelectDlg.cpp ../../source/SourcePhrase.cpp \
	../../source/SplitDialog.cpp ../../source/Stack.cpp ../../source/StartWorkingWizard.cpp \
	../../source/Strip.cpp ../../source/TargetUnit.cpp ../../source/tellenc.cpp \
	../../source/TransformToGlossesDlg.cpp ../../source/USFMPage.cpp ../../source/UnitsDlg.cpp \
	../../source/UnitsPage.cpp ../../source/UnpackWarningDlg.cpp \
	../../source/ViewFilteredMaterialDlg.cpp ../../source/ViewPage.cpp ../../source/WaitDlg.cpp \
	../../source/Welcome.cpp ../../source/WhichBook.cpp ../../source/WhichFilesDlg.cpp \
	../../source/XML.cpp ../../source/XMLErrorDlg.cpp ../../source/helpers.cpp
adaptit_LDFLAGS = $(WX_LIBS) 



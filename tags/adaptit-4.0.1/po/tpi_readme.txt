This is the localization folder for the Tok Pisin language (of Papua New Guinea) for use with the wxWidgets version of Adapt It.

When properly installed and configured Adapt It will read the Tok Pisin strings contained in the compiled .mo files in this folder and use them instead of English in Adapt It's user interface. 

The Tok Pisin language is spoken by over 4 million people within the country of Papua New Guinea (PNG). It is one of the three official languages of the country (the other two being English and Hiri Motu). Tok Pisin is an English based Creole language and most people who speak it also speak one of the over 800 vernacular languages of PNG. 

Tok Pisin borrows heavily from English for words used to express previously unknown (often technical) concepts. Hence, the translations in the Tok Pisin localization have many "transliterated" words which are simply words borrowed from English, but spelled with a more phonetic spelling representative of how Papua New Guinea speakers would say them. The concepts of many of these transliterated terms would not be familiar to those Papua New Guineans who have had little or no exposure to computers, computer software or the specific verbal imagery that is used within the Adapt It software program. 

Below is a list of many of these "technical" terms with a brief comment about the nature of the term as it is used in this localization. Those who are involved in training Tok Pisin speakers to use Adapt It would be well advised to help those they train to understand these technical terms sufficiently as part of their training. The terms at the top of the list will likely be encountered more often than those toward the bottom of the list.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
fail - English: file. Specifically, a computer file that is typically created and stored on a disk.

dokumen - English: document. Within Adapt It a "document" is much more than just a source text and a correspopnding target text. It includes all of the filtered (hidden) information, standard format markers, and many other characteristics of the adaptation process. In fact Adapt It documents include all the information necessary to reconstruct the source text, target text, and even both knowledge bases. They include information about virtually everything that was done in the process of adaptation. Therefore, Adapt It "documents" cannot be edited with normal word processors as though they were simple text files. They are very complex files written in a special XML format.

folda - English: folder, directory or subdirectory. A named storage location used in modern computer file systems. Folders or directories have their own names and become a place where one or more files can be stored. Computer folders can be, and often are, created inside of other folders. Therefore, folders are the main way to organize files and folders into groups on a computer.

wok folda - English: work folder. Within Adapt It "work folder" usually refers specifically to the "Adapt It Work" (or "Adapt It Unicode Work") folder which Adapt It automatically creates on the user's computer when Adapt It is first installed.

projek - English: project. Adapt It creates a separate project for each pair of source and target languages used in adaptation/translation work. 

projek folda - English: project folder. Adapt It keeps all the files related to a particular project together inside its own project folder. Adapt It creates the name of a project folder using the names of the source language and target language. A project name and its folder name might look something like this: "LanguageA to LanguageB adaptations", where "LangaugeA" is the "source" language and "LanguageB" is the "target" language used for adaptation (see 'sos' and 'taget' below).

Besik Seting Fail - English: Basic Configuration File. This is a text file that keeps track of basic Adapt It settings. It is named AI-BasicConfiguration.aic and is always located in the user's work folder (see "wok folda" above).

Projek Seting Fail - English: Project Configuration File. This is a text file that keeps track of the project settings for each Adapt It project on the computer. It is named AI-ProjectConfiguration.aic and Adapt It creates one such file in each project folder (see "projek folda" above).

menu - English: menu. Used in Adapt It, as in most computer software, to describe a list of choices that a user can make or selected from.

daialog - English: dialog. Refers to a rectangular box or window that appears on the screen in which a user can provide information that the program needs for some operation or process.

fres, fres boks - English: phrase, phrase box. The phrase box is the focal point for the adaptation process and is where translation (or target) text is entered to represent the meaning of the source word or phrase (located or previously selected) above the translation point.

teks - English: text. Refers to written text in a particular language.

deta - English: data. Refers to texts or other information that is organized or structured in some way.

bafa - English: buffer. A place in computer memory which can temporarily hold data.

sos, sos teks - English: source, source text. The "source" designates the language text from which the user is translating or adapting. Often the source language is a second language that the user knows or has learned. The source text has its own unique font which can be set initially during initial project setup or at any later time using the Fonts page in program Preferences (from the Edit menu).

taget, taget teks - English: target, target text. The "target" designates the language into which the user is translating or adapting. Often the target language is the user's own mother tongue. The target text has its own unique font which can be set initially during initial project setup or at any later time in program Preferences (from the Edit menu).

toksave teks, toksave tokples - English: navigation text, navigation language. The "navigation" language refers to the text that is used within the Adapt It program to indicate the style, location and any other information about a portion of the source and its corresponding target text being shown in the display. The navigation text is often a language of wider communication that is different from either the source or target texts (perhaps the language known by a translation consultant). The navigation text is displayed above the piles of text at the point where the navigation info text applies. It is also the font that is used for informational text within dialogs and other parts of the program interface, including lists of file, folder and document names. The navigation text has its own unique font which can be set initially during initial project setup or at any later time in program Preferences (from the Edit menu).

glos, glos teks - English: gloss, gloss text. The "gloss" text is only used and/or made visible in a special interlinear-like "glossing" mode within Adapt It. Glosses are kept in a separate knowledge base, and can be displayed in a separate interlinear line within Adapt It's display (in addition to the source text, target text and navigation text). Glosses do not use a separate font, but can be assigned the same font used by either the "target" or the "navigation" language.

Lukim Glos - English: Show Glosses. This selection on the Advanced Menu, makes any glosses that have been done previously, appear in a separate line across the main window. The glosses can only be viewed, but not changed unless the "Wokim Glos" check box (see below) is also checked to activate glossing mode. With glosses visible, they can be available as a (read only) clue to the translator while adapting from the source text to the target text. For example, an adaptation project might use the Nyindrou language as a source text and the Bipi language as a target text. Glosses in this case might be done in Tok Pisin, because speakers of both languages know Tok Pisin in addition to their own languages. Glosses made in Tok Pisin in this case could add valuable clues to the translator using Adapt It to translate from Nyindrou to Bipi.

Wokim Glos - English: Glossing (Activate glossing mode). This mode is activated by putting checking a check box which is only visible to the user while Lukim Glos is activated (see above). When the "Wokim Glos" check box is checked Adapt It is in glossing mode, and normal source-to-target adapting is suspended, and the user focuses on making glosses in the glossing language from the source text. To accommodate the glossing mode, the phrase box moves to the glossing line instead of the target text line, so that everything that is entered into the phrase box (while in glossing mode) is considered a gloss and is saved only in the separate glossing knowledge base. The font used for actual glossing defaults to use the Taget Teks Font, but the user can change the glossing font to use the Toksave Font instead (selectable from the Advanced Menu).

nolis bes, kb - English: knowledge base, kb. A special file that stores the target text adaptations or translations that correspond with words and phrases in the source text. A separate knowledge base is stored for gloss entries. Both knowledge base files are stored within the project's work folder. Sometimes the term knowledge base is abbreviated as kb or KB.

leta - English: character, letter, digit or punctuation mark.

kes, kepital leta, smol leta - Engliish: Case, capital letter (upper case), small letter (lower case).

pangsuesen, mak bilong pangsuesen - English: punctuation, punctuation mark. The default punctuation characters are: ? . , ; : " ! ( ) < > { } [ ]

plesholda - English: placeholder. Refers to the location within a source text which has been opened and left empty - in order to have a location established where a target text word or phrase can be inserted. Placeholders in Adapt It's source text are indicated by an ellipsis ... at the location of the placeholder. Placeholders cannot be inserted into source text which is part of a "retranslation". Also a span of source text which contains a previously inserted placeholder ..., cannot be merged without first removing the placeholder.

seleksen, selektim - English: selection, select. Refers to making a choice by clicking on an item in a list, or clicking on a menu item, or clicking on a button (usually beside an option) in the program. A selection can be made with a mouse click, or can sometimes also be made by moving a selection with arrow keys. An important skill to have when using Adapt It is to be able to select two or more source text words in order to merge them into a phrase to be adapted. Selecting two or more source text words can be accomplished by dragging the mouse over the source text words (with the left mouse button held down while moving the mouse cursor over the source words to be selected). Selecting source text words may also be done more efficiently by holding the ALT key down and using the RIGHT or LEFT arrow key, which makes a selection starting at the location of the phrase box

batan - English: button. Refers to a rectangular button (or sometimes a "radio" button) in a dialog or on the interface.

klikim, klikim batan - English: click on, click on. Refers to clicking on a button.

dabel klikim - English: double click on. Refers to double clicking on a list item.

kursa - English: Cursor. Refers to the insertion point (usually placed by a mouse click).

tik, tikim, tikim boks - English: checkmark, check, check the box. Refers to making a choice in a checkbox. In PNG, a "checkmark" is called a "tik" and "checking a box" is called "tikim boks"

standet fomat maka, SIL standet fomat - English: Standard format marker, SIL standard format, referring to codes that begin with a backslash marker character \ followed by an abbreviated code such as \c, \v, \p, etc., and a space. Markers that specify a chapter (\c) or a verse (\v) are followed by a space and a number. Adapt It generally hides all standard format markers so the user does not need to work directly with them while working within Adapt It. Adapt It can utilize the new Uniform Standard Format Marker (USFM) set or the older PNG marker set, or both.

dilitim, or rausim - English: delete or erase. Within the Adapt It program this usually refers removing a computer file from a folder or a disk, which (if Adapt It cannot do it automatically by itself) generally requires the use of a separate computer file managing program to accomplish.

Ekspot, Ekspotim - English: Export. Adapt It can export certain kinds of information out to separate disk files in order to make that information available to other programs or to other Adapt It projects. Other programs cannot use Adapt It's files directly, so the information they need must be exported from Adapt It using the various export options found on the Adapt It File menu. 

Impot, Impotim - English: Import. Adapt It can import dictionary records to quickly build up its knowledge base, if such records have been already prepared in another program. Words and their meaning(s) are added to Adapt It's knowledge base if they do not already exist. Such dictionary records must be formatted with certain backslash codes in order for Adapt It to distinguish the dictionary main entry (marked with \lx) from the translation(s) of that entry (marked with \ge). Such dictionary entries can be created by hand or can come from a Shoebox or Toolbox database. You can use the "Import To Knowledge Base..." command on the File menu to Import such dictionary records.

kod - English: code, usually referring to a standard format marker, but sometimes also to a locale code.

stail - English: style, especially in reference to format style as governed by the use of standard format markers.

kensel, kenselim - English: Cancel. Generally a user can click on a Cancel button in a dialog to abort a process or action and close the dialog without having done anything (or made any changed to the document).

Ris Teks Fomat (RTF) - English: Rich Text Format (RTF). A type of document export that Adapt It can produce, which can be opened in a word processor such as Microsoft Word or Open Office. Rich text documents are formatted nicely to look like the text in published books.

intalinea - English: Interlinear. Refers to a document format in which words and phrases of one language text are aligned vertically with corresponding words and phrases of a second, third or fourth language. Adapt It's Interlinear documents are formatted as RTF documents and use table rows and cells to contain the vertically aligned words and phrases.

bek trenslesen - English: Back translation. A lexically literal, but grammatically free translation of a target text made "back" into a language of wider communication (such as English) that can be understood by an outside consultant for the purposes of checking the accuracy, and clarity of a translation. Back translations are stored as "filtered" (hidden) information. The presence of any existing back translation is marked by a small green wedge icon at the point in the text where the back translation (along with any other filtered information) is located. The actual type and content of the filtered information stored there can be viewed by clicking on the small green wedge.

fri trenslesen - English: Free translation. Similar to a back translation in purpose, but made more like a paraphrase translation of the target text back into a consultant's language for use in translation checking. Free translations are stored as "filtered" (hidden) information. The presence of any existing free translation is marked by a small green wedge icon at the point in the text where the free translation (along with any other filtered information) is located. The actual type and content of the filtered information stored there can be viewed by clicking on the small green wedge.

not - English: Note. Refers specifically to Adapt It Notes in particular, which are free-form notes or comments that are embedded within the document using the "Open a Note dialog" tool bar button. Like Back translations and Free translations, Adapt It Notes are stored as "filtered" (hidden) information. The presence of a hidden Adapt It Note is marked by a small yellow icon shaped like a ring notepad at the point in the text where the Note is located. The content of such Notes can be viewed by clicking on the small yellow icon.

sistem - English: system, usually a reference to a computer operating system.

mod - English: mode. Used in Adapt It to describe a state of the program in which certain actions are enabled, disabled or constrained in some way - such as "Book Folder Mode," "Glossing Mode", or "Free Translation Mode".

lokal - English: locale, a reference to a formal language region as recognized by computer systems.

bekap - English: backup. Refers to the making of backup files for documents and knowledge bases.

pekap - English: pack, pack up. Note: Some speakers of Tok Pisin do not distinguish between b and p and so they may not easily distinguish between bekap and pekap, even though they should understand the difference between bekap and pekap as used in the program.

kontrol ba - English: Control bar.

kompos ba - English: Compose bar.

pail - English: Pile. In the main window of Adapt It, texts are aligned vertically into "piles" which represent a source text word or (merged) phrase along with its corresponding target or adaptation text aligned directly underneath it. Optionally any gloss text may also be aligned below the source text, if "Lukim Glos" is selected. (if "Wokim Glos" is also selected, the target text line and gloss line switch places so that glosses are entered into the phrase box instead of target language adaptations). Any Navigation Text appearing, is shown above the pail to which it applies.

strip - English: Strip. In the main window of Adapt It, the different language texts are spread out horizontally across the screen in "strips". Strips are composed of "piles" (see above) of related text parts placed side-by-side across the screen.

Konsistensi Sek - English: Consistency Check. A process in which Adapt It compares the contents of one or more documents with the contents of the knowledge base. If a document contains words/phrases and/or adaptations which are not found in the knowledge base, Adapt It allows the user to interactively update the knowledge base to be consistent with the current document(s).

Konsisten Senis - English: Consistent Changes. A process in which 1 to 4 consistent change tables can be loaded into Adapt It, and used to pre-process source text words before they are copied to the phrase box. This process is only done when there are no previous translations for the given source word. Consistent change tables can be loaded or unloaded (disabled) at any time. The use of consistent change tables can be especially helpful if a target language differs from the source text in regular (consistent) ways, such as predictable differences in sound/spelling or vocabulary.

teb - English: tab. A manila folder-like tab in a dialog which contains another page of information or settings.

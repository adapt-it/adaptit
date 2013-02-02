IMPORTANT NOTES: (1) YOU MUST HAVE ADMINISTRATOR OR POWERUSER 
PRIVILEGES TO INSTALL THIS SOFTWARE ON WINDOWS 7, WINDOWS 
VISTA AND WINDOWS XP; (2) YOU SHOULD UNINSTALL ANY VERSION
5.X.X OF ADAPT IT BEFORE INSTALLING VERSION 6.4.0. YOUR DATA 
IS NOT AFFECTED BY UNINSTALLING.
---------------------------------------------------------------------
Adapt It WX
---------------------------------------------------------------------
Version 6.4.1
Versions of Adapt It beginning with Version 4.0.0 are cross-platform 
capable, being built with the wxWidgets cross-platform GUI framework 
(hence the WX in the name). Packages are also available for Linux, 
and the Macintosh. See http://adapt-it.org for more information.

See the file "Adapt It changes.txt" for a description of the latest
changes that have been incorporated into this version.

Adapt It WX is a copyrighted program. See license/LICENSE.txt for
more information.
 
This software is supplied to you without any warranty whatsoever. 

---------------------------------------------------------------------
What Data Format(s) does Adapt It WX accept as input?
---------------------------------------------------------------------
Both Adapt It WX and Adapt It WX Unicode require that texts input as 
source language texts must be Plain Text. Adapt It cannot accept word 
processing documents that have been saved as (binary) documents using 
the word processor's native format - usually with a .doc or .odt 
extension. Adapt It also cannot accepts documents that have been saved 
in rich text (.rtf) format as input source texts. A future version of 
Adapt It may be able to accept some type of XML formatted document for 
input as source text, but this current version 6.x does not accept 
.xml format documents as input texts. Hence, this version of Adapt It 
will not accept documents saved as Word 2003 XML documents as input
texts.

It is recommended (but not required) that your input source language 
texts be marked with Unified Standard Format Markers (USFM). The USFM 
system is described fully at:

   http://confluence.ubs-icap.org/display/USFM/Home

If you have source text files that use the older PNG 1998 standard 
format marker system, Adapt It can accept such files also. Both the 
USFM and PNG 1998 marking systems are composed of plain text marker 
codes that beginning with a backslash character \ and are immediately 
followed by an abbreviated code (with no intervening space). Such 
backslash codes indicate the type of text following the backslash 
code, or the format of the text associated with the backslash code. 
Here is a short sample of plain text utilizing a few of the more 
common backslash codes that Adapt It can input and understand:

\id JHN English Standard Version (ESV)
\mt John
\c 1
\s The Word Became Flesh
\p
\v 1 In the beginning was the Word, and the Word was with God, and 
the Word was God. \v 2 He was in the beginning with God. \v 3 All 
things were made through him, and without him was not any thing 
made that was made. \v 4 In him was life,\f \fv 1:4 \ft Or \fq was 
not any thing made. That which has been made was life in him\f* 
and the life was the light of men. \v 5 The light shines in the 
darkness, and the darkness has not overcome it.
\p
\v 6 There was a man sent from God, whose name was John. \v 7 He 
came as a witness, to bear witness about the light, that all might 
believe through him. \v 8 He was not the light, but came to bear 
witness about the light.

Note in the above sample that backslash markers are always combined 
with an abbreviated code such as id, mt, c, p, v, etc to form a 
backslash code to indicate something about the text. The id marker 
is helpful for identifying the book the text belongs to, especially 
Scripture texts. Some markers like \c and \v expect to be followed 
by a number to indicate a the beginning of a specific location in 
the text (\c 1 indicates the beginning of chapter 1; \v 5 indicates 
the beginning of verse 5). Some markers expect to have some text 
associated with the marker, such as \s The Word Became Flesh, in 
which the \s marker indicates that the text following it is to be 
considered a section heading. Other markers, such as \p, simply 
indicate the start of something in the text. In this case the \p 
indicates that the text following the marker starts a new paragraph. 
Some markers like those which indicate footnotes can be more complex.
In the sample text above a footnote starts with the \f backslash 
code and ends with the "ending" backslash code for footnotes which 
is \f*. Other markers such as \fv, \ft and \fq used above are 
embedded content backslash markers that identify certain parts 
within an overall footnote.

The USFM system has many other backslash codes available for use, 
but you may wish to only use a dozen or fewer to mark the main 
parts or formatting of your input source texts. If you are working 
with Scripture, it is especially handy to have chapter and verse 
markers in your source texts because Adapt It can use them as 
navigation points and references in the text. The use of standard 
format markers also enables Adapt It to make nicely formatted rich 
text exported documents (see below).

If you have an existing dictionary which can produce a plain text 
file containing lexical items (marked with \lx) followed by one or 
more glosses or meanings (each marked with \ge), Adapt It can import 
such files to quickly build up its knowledge base.

---------------------------------------------------------------------
What Data Format(s) does Adapt It WX produce and/or export?
---------------------------------------------------------------------
Adapt It WX stores its adaptation documents in a special XML format 
that only Adapt It knows how to interpret. These XML format 
adaptation documents are stored in the project's "Adaptations" 
folder. The data contained in these XML documents can be read with 
an editor, but you should not try to edit Adapt It's XML documents
directly in an external editor or other word processor. 

Adapt It WX stores its knowledge bases also in a special XML format. 
Like its XML adaptation documents, these knowledge base files 
contain information that only Adapt It knows how to interpret, and 
you should not edit these files directly. Adapt It stores a regular 
knowledge base and a glossing knowledge base for each project within 
the project folder.

Adapt It can produce a number of different kinds of exports from its 
store of data, documents and knowledge bases. Knowledge bases can be 
exported to a plain text standard format that can be used to build a 
dictionary within a separate dictionary management program such as 
Toolbox or Field Works. It can also export its knowledge base in
LIFT format as a means of starting a dictionary project in other
programs that recognize the LIFT format - such as WeSay or FLEX.

Adapt It can export its source language texts, its adapted or 
translated language texts, glosses, and free translations in standard 
format (.txt), preserving any backslash codes as described above. 
Source texts, translated texts, glosses and free translations can 
also be exported in rich text format (.rtf) for later use in a 
word processor for printing nicely formatted documents. Adapt It can 
also export documents in Interlinear form (as .rtf formatted files) 
which can be used or nicely printed from a word processor.

---------------------------------------------------------------------
What is Adapt It used for?
---------------------------------------------------------------------
Adapt It WX provides tools for translating either text or scripture 
from one language which you know, to another related language known 
to you or a coworker. No linguistic analysis is performed. Thus it 
can be an appropriate tool for native speakers who have no linguistic 
training. The success and quality of a translation/adaptation project 
depend heavily on the quality of the source texts used and the 
bilingual (or multilingual) capabilities shared by the members making 
up the project team. Adapt It can often be used to quickly produce a 
draft or pre-draft of a translation which can then go through a 
rigorous process of revision, testing and checking to produce an 
accurate and polished translation of high quality.

---------------------------------------------------------------------
How does Adapt It work?
---------------------------------------------------------------------
Adapt It has many capabilities. See the various documentation files 
and the HTML Help from inside Adapt It for the details of how to set 
up a project and proceed with translation work using Adapt It. 

Adapt It is a sophisticated "Translation Memory" program. This means 
that Adapt It remembers how you translate words and phrases as you 
work. Adapt It records what it observes as you translate in special 
files called knowledge bases. Once you start translating, Adapt It is 
always looking 10 words ahead of the translation point in the source 
text to see if it "remembers" how you translated words or 
combinations of words previously. If it finds something you've 
translated previously it can enter the translation automatically into 
the "Phrase box" that it displays at the translation point and move 
on to the next word or phrase. If you've translated a given word or 
phrase more than one way, Adapt It will stop momentarily and display 
each way you've translated that given word or phrase previously, and 
wait for you to choose which translation fits best, or wait for you 
to enter a new translation for that given word or phrase. After 
typing the translation you simply press the "ENTER" key and Adapt It 
moves the phrase box along to the next part of the text. Adapt It 
continues in this manner and the process picks up more speed the more 
you translate, since Adapt It will "remember" more translations and 
become "smarter" as you continue with your translation. 

Adapt It has various "modes" including "drafting mode" and 
"reviewing mode". In "drafting mode" Adapt It enters all translations 
that it knows about without stopping until it encounters a word or 
phrase that it doesn't know how to translate, or until it encounters 
a word or phrase that has more than one translation - in either case 
it must stop and wait for you to either enter a fresh translation, or 
choose from a list of known translations.

As you translate it may be necessary to group source words together 
to make a more natural translation in the target language. The 
grouping of source words into phrases can be done by dragging the 
mouse over the source words and immediately typing the translation 
(Adapt It takes care of lining up everything), or by holding down 
the ALT key while pressing the LEFT or RIGHT ARROW keys to select 
source words you want to group together into phrases. Previously 
merged source words can be "un-merged" by using the appropriate tool 
bar buttons.

Adapt it is fun to use! It also has many advanced features that make 
it a powerful tool for translation programs. You will want to study 
Adapt It's documentation and online helps to help you use Adapt It's 
full potential.

---------------------------------------------------------------------
Online Help System:
---------------------------------------------------------------------
Online Help is available both from within the Adapt It WX program 
(access Help Topics through the Help Menu), and from the Adapt It 
WX Start Menu group (in Windows installations). 

---------------------------------------------------------------------
Documentation:
---------------------------------------------------------------------
The following documentation files are installed (with links to them 
in the Adapt It Start Menu group on Windows installations):
   1. Adapt_It_Quick_Start.htm has basic startup information in an 
      html file; this file also includes "how to" instructions for 
      about a score of common Adapt It procedures. Also accessible
      from Adapt It's Help Menu.
   2. Help_for_Administrators.htm has information for administrators
      detailing how to set up Adapt It to collaborate with Paratext
      or Bibledit, as well as other administrative functions. Also
      accessible from Adapt It's Administrator Menu.
   3. Adapt It Tutorial.doc has a tutorial. The tutorial takes you 
      through the initial launch operation, and then through the 
      common operating procedures for doing an adaptation. The data 
      used in the tutorial is in a file called "Tok Pisin fragment 
      1John.txt" included with this package. It is ANSI data, but 
      uses only characters which are also correct UTF-8, so it can 
      be used also as a source text with Adapt It Unicode.
   4. Adapt It Reference. It is a MS Word/Open Office *.doc file 
      If your word processor cannot open it, contact Bill Martin 
      (bill_martin@sil.org) or Bruce Waters (bruce_waters@sil.org) 
      who can supply the file in .PDF or .RTF format instead. 
      The PDF format is the preferred alternate format because it 
      is approximately the same size as the Word document from 
      which it is made (about 2.3 MB). The RTF file is significantly 
      larger (4MB).
   5. Localization_Readme.txt describes how the localization system 
      works in Adapt It WX and Adapt It WX Unicode. It also tells 
      how anyone can modify an existing interface translation, or 
      prepare a new language interface for Adapt It using Adapt It's
      Pootle localization web service at: 
         http://pootle.sil.org/admin/users.html
      Another way to contribute to Adapt It's localization needs
      is to use the Poedit program which is freely available from 
      the Internet (see the Localization_Readme.txt file). 
   6. Adapt It changes.txt describes the changes from previous 
      versions of Adapt It WX (starting with version 4.0.0).
   7. Known Issues and Limitations.txt lists issues and/or 
      limitations that were recognized but unresolved at the time 
      of the current release.

---------------------------------------------------------------------
Localization:
---------------------------------------------------------------------
Version 4 of Adapt It (Adapt It WX) introduced a flexible interface 
and localization mechanism making it possible for the user to select 
any interface language of choice from a list. Choosing an interface 
language other than English can be done when Adapt It is first run, 
or at any later time by selecting "Choose Interface Language..." from 
Adapt It's View menu. It is possible to produce a program interface 
for Adapt It in any language desired, without the need for creating 
a separate program file for each interface language supported. See 
the Localization_Readme.txt file for more information.

---------------------------------------------------------------------
Cross-platform and Free Open Source Software:
---------------------------------------------------------------------
Adapt It WX and Adapt It WX Unicode utilize the wxWidgets Cross-
Platform GUI Framework, which is an open source library of 
programming tools for producing computer programs that run on all the
major computing platforms - including Microsoft Windows, Linux/Unix, 
and Macintosh. Adapt It WX and Adapt It WX Unicode do not depend in 
any way on the Microsoft .NET Framework or on any other proprietary 
technology. Adapt It is both "Free" in the sense of Freedom, and 
"Free" in the sense of available without cost. The developers of 
Adapt It welcome the input and participation of the open source 
community. We especially welcome reports of successes or difficulties
in the use of Adapt It by those who have an interest or experience 
in translation/adaptation work.

---------------------------------------------------------------------
Responsive and Timely Help is Available:
---------------------------------------------------------------------
Bill Martin and Bruce Waters have had over 50 years of combined 
experience in translation and training of national mother-tongue 
translators. They endeavour to respond quickly and effectively to 
email questions and/or bug reports. Usually if a bug can be 
identified, a fix or work-around for a problem can be provided within
24 hours. As the Adapt It user base grows and more users gain 
experience using Adapt It, the Adapt It User Forum will provide an 
increasing share of help for Adapt It users. Be sure to examine the 
help documents and help system provided with Adapt It, as they can 
be invaluable sources of information.

The current user forum is located at:
http://groups.google.com/group/AdaptIt-Talk

Bug reports and and other Issues for discussion can be posted at:
http://code.google.com/p/adaptit/issues/list


We hope you enjoy using Adapt It!

---------------------------------------------------------------------
The Adapt It Development Team:
---------------------------------------------------------------------
Bill Martin    - bill_martin@sil.org
Bruce Waters   - bruce_waters@sil.org
Erik Brommers  - erik_brommers@sil.org
Kevin Bradford - kevin_bradford@sil.org
Graeme Costin  - adaptit@costincomputingservices.com.au (focus on
                   MacOSX)
Michael Hore   - mike_hore@aapt.net.au
Bob Buss       - bob_buss@wycliffe.org (focus on HTML Help system)
---------------------------------------------------------------------

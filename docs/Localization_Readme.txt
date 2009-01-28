Adapt It WX - Localization Information

Beginning with version 4.0.0 of Adapt It WX and Adapt It WX Unicode, you can
easily switch the language of Adapt It's interface to any language into which 
Adapt It's interface has been translated.

The full installations of Adapt It WX and Adapt It WX Unicode come with 
interface translations for English (the default), Spanish, French, Russian,
Portuguese, Indonesian, PNG Tok Pisin, Chinese and Azeri. Each language 
interface is called a "Localization." Other localizations are in preparation. 
Some of the above localizations initially are just partial translations, that 
is, some interface text and interface messages are not yet translated - in 
which case those parts of the interface will simply display in English 
instead of the other language localization. The Adapt It team is seeking 
translators to complete the above mentioned localizations, as well as produce 
new localizations for Adapt It.

Where localization folders and files are located after installation:

The Adapt It WX program localizations for a given language are in a file 
called <AppName>.mo, and the general wxWidgets library's localizations 
are in a file called wxstd.mo (<AppName> is the name for the application
on the given platform - Adapt_It on Windows, adaptit on Linux and AdaptIt
on the Macintosh. These two .mo files keep the same file names
regardless of what language/localization they contain in them. Hence, 
to keep things organized, they are kept in separate subfolders. 
Different platforms have different conventins where the operating system
expects to find localization files. In a Windows installations there is
a "Languages" parent folder within the installation folder where Adapt It
is installed. Windows installers set up the localization with the 
following folder/file structure under the installation folder (the 
<installation_folder> is normally c:\Program Files\Adapt It WX  for the 
regular version; c:\Program Files\Adapt It WX Unicode for the Unicode 
version - a different location for Linux and the Macintosh versions -
see below):

Windows Installations:

<installation_folder>
    |
    |
    Languages
       |- az -               <- subfolder for Azeri
       |     |- Adapt_It.mo
       |
       |- es -               <- subfolder for Spanish
       |     |- Adapt_It.mo
       |     |- wxstd.mo
       |
       |- fr -               <- subfolder for French
       |     |- Adapt_It.mo
       |     |- wxstd.mo
       |
       |- id -               <- subfolder for Indonesian
       |     |- Adapt_It.mo
       |
       |- pt -               <- subfolder for Portuguese
       |     |- Adapt_It.mo
       |     |- wxstd.mo
       |
       |- ru -               <- subfolder for Russian
       |     |- Adapt_It.mo
       |     |- wxstd.mo
       |
       |- tpi -               <- subfolder for PNG Tok Pisin
       |     |- Adapt_It.mo
       |     |- wxstd.mo
       |
       |- zh -               <- subfolder for Mandarin Chinese
       |     |- Adapt_It.mo
       |     |- wxstd.mo
       |
       | ... [other localizations]

Linux/Macintosh Installations:

Linux and Macintosh installers set up the localization with the 
following folder/file structure under the localization folder (the 
<localization_folder> is normally /usr/share/locale for Linux and
AdaptIt.app/Contents/Resources/locale on the Macintosh. Note that
non-Windows installations also have a LC_MESSAGES folder in the
folder structure: 

<localization_folder>
    |
    |
    |- az -               <- subfolder for Azeri
    |     |- LC_MESSAGES
    |           |- Adapt_It.mo
    |
    |- es -               <- subfolder for Spanish
    |     |- LC_MESSAGES
    |           |- Adapt_It.mo
    |           |- wxstd.mo
    |
    |- fr -               <- subfolder for French
    |     |- LC_MESSAGES
    |           |- Adapt_It.mo
    |           |- wxstd.mo
    |
    |- id -               <- subfolder for Indonesian
    |     |- LC_MESSAGES
    |           |- Adapt_It.mo
    |
    |- pt -               <- subfolder for Portuguese
    |     |- LC_MESSAGES
    |           |- Adapt_It.mo
    |           |- wxstd.mo
    |
    |- ru -               <- subfolder for Russian
    |     |- LC_MESSAGES
    |           |- Adapt_It.mo
    |           |- wxstd.mo
    |
    |- tpi -               <- subfolder for PNG Tok Pisin
    |     |- LC_MESSAGES
    |           |- Adapt_It.mo
    |           |- wxstd.mo
    |
    |- zh -               <- subfolder for Mandarin Chinese
    |     |- LC_MESSAGES
    |           |- Adapt_It.mo
    |           |- wxstd.mo
    |
    | ... [other localizations]

The localization mechanism in Adapt It is smart enough to find and 
load the desired localization. It uses the same .mo files (see diagram 
above) regardless of whether the application is the Regular (non-Unicode) 
or the Unicode version of the application. If localization folders/files 
are present when Adapt It is run for the first time, it will show the 
"Choose Language Dialog" and remember the choice each time the application 
is run subsequently. After the first run of the application, anytime the 
user wishes to change the localization, he/she can select the "Change 
Interface Language..." menu item from the View menu. The application then 
displays the localizations that are actually installed on the user's 
computer, displaying them in a list for the user to choose from. He/She 
can choose any that are installed. If a different language is chosen 
from the current interface language, a dialog pops up to remind him/her 
that the new language interface won't show up until after Adapt It is 
restarted.

From time to time new localizations become available and sometimes the
interface elements of Adapt It may change and new localizations should
be installed - usually these are included with the installer when a newly
updated version of Adapt It becomes available. Once in a while a 
localization is updated or corrected. In this case you only need to 
download and install the localization installer (about 1MB in size) to 
get any new and/or updated localizations. Once a new or updated Adapt_It.mo 
and/or wxstd.mo files is present in a named language subfolder, they 
become immediately available to the program and can be chosen using the 
"Change Interface Language..." menu item from the View menu. 

How to EDIT an EXISTING localization for Adapt It's interface:

As mentioned the localizations initially provided with Adapt It WX and
Adapt It WX Unicode are only partial. These partial localizations 
include Spanish, French, Russian, Portuguese, Indonesian, Chinese
and Azeri. If you would like to help complete or correct any of the 
existing translations for these languages, the Adapt It team welcomes 
your help! If you want to help complete or correct the localizations 
for Spanish, French, Russian, Portuguese, Indonesian, Chinese, or Azeri, 
here is how you can do so:
   * The easiest way to edit or correct an existing localization is
to first install a free program called Poedit. PoEdit is an easy to 
use program with a graphical interface that helps you translate 
Adapt It's interface elements. Poedit can be downloaded from the 
internet at the following address:

http://www.poedit.net/download.php

After downloading and installing Poedit on your computer, you need
to get a .po file for the Adapt It localization you wish to modify 
or create. The existing localization .po files (or a default.po 
template) can be downloaded from Adapt It's open source site at:

http://code.google.com/p/adaptit/source

The best way currently to get the localization files is to use a 
subversion program (such as TortoiseSVN on Windows) and do an
anonymous "check out" of the source code for the entire adaptit 
project from Google Code. The po directory contains all the po
files for the existing Adapt It localizations. If you cannot 
figure out how to use subversion, you can email Bill Martin at 
bill_martin@sil.org and request him to send you the appropriate 
Adapt It po file via email attachment.

The .po files are currently named in the repository as follows:

az.po	   - Azeri
es.po      - Spanish
fr.po      - French
id.po      - Indonesian
pt.po      - Portuguese
ru.po      - Russian
tip.po     - Tok Pisin (Papua New Guinea)
zh.po      - Chinese (Mandarin)
default.po - an "empty" localization ready to begin a new language
             langauge localization using Poedit.

Once you have the appropriate po file for a given language, and 
you have installed the Poedit program, you can then use Poedit 
to update or create a localization for Adapt It's interface in 
that given language. 

Here is how to edit/update a localization po file with Poedit:
   * Acquire the appropriate po file and copy it to your computer (see 
above).
   * Double click on the .po file you want to work on (for example 
double click on the es.po file for Spanish, fr.po for French, etc) 
which should cause Poedit to run and automatically load the 
localization file (if Poedit is installed).
   * With the appropriate .po file loaded into Poedit, translate any 
text that has not been translated. All text items that have not yet 
been translated are located at the top of the list. Some bits of text 
may additionally be labeled as "fuzzy" which simply means that they may 
need to be examined for accuracy. Once any "fuzzy" text string are 
corrected or verified for accuracy, you can use Poedit's Edit menu to 
remove their "fuzzy" designation by typing ALT-U or by selecting the
"Translation is fuzzy" menu item which works like a toggle switch to 
remove the "fuzzy" check mark on that item in the list. 
   * Save your translation work in Poedit. When saving, Poedit 
automatically creates an updated .mo file which is a compiled version 
of the interface translations created from the .po file. If you were
working on the Spanish es.po file, for example, Poedit will save both
the es.po file and it will also save it as an es.mo compiled file. It 
is actually the mo file that Adapt It reads to display the interface 
language text within Adapt It. But, before Adapt It can read it, its
name must be changed to an appropriate name for the operating system
you have Adapt It installed on (see the next point below).
   * Rename any current .mo file (within the appropriate localization 
subfolder) to something like Adapt_It.old (Windows) or adaptit.old 
(Linux); and then copy the new .mo file to that same directory, and 
rename the new .mo file using the original name the .mo file had 
before you renamed it, i.e., Adapt_It.mo (for Windows), or adaptit.mo 
(for Linux). For example, if you loaded es.po into Poedit and edited 
or changed any translations and saved them, Poedit automatically 
creates a new es.mo file. You would locate the appropriate language 
directory (es, fr, pt, etc) and copy the newly produced .mo file from 
Poedit there; rename any existing .mo file there to Adapt_It.old (or 
adaptit.old on Linux); then rename the newly created es.mo file to 
Adapt_It.mo (or adaptit.mo on Linux). 
(Note: on Linux you should insure that the new mo file has the 
proper read permissions).
Once you have done this Adapt It should automatically show any 
editing changes or new translations the next time you run Adapt It 
with that chosen localization as the interface language of your 
choice. 
   * Send your updated or newly created po file to bill_martin@sil.org
who will then see that it gets incorporated into Adapt It updates so
that other Adapt It users may benefit.

If you have not already changed Adapt It's interface language to the
language of your choice, here is how to do it: On Adapt It's View 
menu select the "Change Interface Language..." item. Your interface 
language should appear in the list of choices. Select the new interface 
language and click OK. Adapt It reminds you that you will need to quit 
and restart Adapt It for the change to become effective.

How to prepare a NEW localization for Adapt It's interface:

Any translator can prepare a new localization for Adapt It. See the
discussion above on how to get a .po file from Adapt It's open source
Google Code site or via email from bill_martin@sil.org. To create a
translation of Adapt It's interface into a new language, you will 
need the default.po template file. This template file is contains
all the message text from Adapt It's interface ready to translate.
The easiest way to do the actual translation is to first install a 
free program called Poedit. Poedit can be downloaded from the internet 
at the following address:

http://www.poedit.net/download.php

After downloading and installing Poedit on your computer, you can 
locate the default.po file on your computer and simply double click 
on that file. If you have installed Poedit, Poedit will start up and 
load the default.po template file. PoEdit is an easy to use program 
with a graphical interface that helps you translate Adapt It's
interface elements. When your translation work is saved within Poedit, 
Poedit automatically creates an updated default.mo file (this is a
compiled version of the interface translations that you create in the 
default.po file. 

There are about 1,700 text elements to translate in Adapt It's 
interface, so translating the interface into a new language is a 
fairly large task. If you do translate Adapt It's interface there 
are a few remaining steps you must do in order to be able to use 
your newly translated Adapt It interface/localization:

1. The easiest way is to send the default.po file you have 
translated to Bill Martin. He would like to incorporate your 
translation/localization into Adapt It's installer so others can
make use of it. Send him a copy of your default.po file as an
email attachment, and include the name of the language you have
translated Adapt It into. Bill will process the file and send
you a small installer application that will enable you to easily
add the localization to your Adapt It installation. He will also
add any existing wxstd.mo localization file if it already exists
for that language.
   
2. If you are anxious to use your translation/localization you
prepare using Poedit for Adapt It interface right away, you can 
install the localization yourself on your own computer. Here are 
the steps:

For Windows:
   2.a. Locate the "Languages" folder on your computer. On Windows
this folder will be located by default at: 
 c:\Program Files\Adapt It WX\Languages
or, if you've installed the Unicode version it will be located at:
 c:\Program Files\Adapt It WX Unicode\Languages

   2.b. Within your "Languages" folder create a subfolder and give
this subfolder a name that corresponds to the two-letter or 
three-letter abbreviation for that language, as given at the IANA 
language registry web site at:

http://www.iana.org/assignments/language-subtag-registry

Note: Your subfolder should be named with the abbreviation used as 
the "Subtag" for that language as listed on the IANA website.

   2.c. Copy your default.mo file (which Poedit produced when you
last saved your localization work using Poedit) into the newly 
created subfolder you created in step 2.b above.

   2.d. Rename the default.mo file to Adapt_It.mo

For Linux:

   2.a. Locate the "/usr/share/locale" folder on your computer. 

   2.b. Look through the names of any existing subfolders within the
locale directory to see if the language already has a localization
subfolder. The subfolders in the locale directory represent the
two-letter or three-letter abbreviations for languages as given at
the IANA language registry web site at:

http://www.iana.org/assignments/language-subtag-registry

If no folder exists with your language's abbreviation, you can create
one yourself using the abbreviation from the IANA registry, or from the
three-letter language code listed in the Ethnologue if your langauge 
isn't listed in the IANA registry. For example, on Linux you could do
this:

   cd /usr/share/locale
   sudo mkdir xxx

where xxx is the abbreviation for the language localization you want to
be located at:

 /usr/share/locale/xxx

Note: If you use a name from the IANA registry, your subfolder should 
be named with the abbreviation used as the "Subtag" for that language 
as listed on the IANA website.

   2.c. Copy your default.mo file (which Poedit produced when you
last saved your localization work using Poedit) into the newly 
created subfolder you created in step 2.b above.

   2.d. Rename the default.mo file to adapit.mo
 
   2.e Change the permissions for reading, i.e.,   chmod +r adaptit.mo

3. Your new localization should now be available for use within 
Adapt It WX or Adapt It WX Unicode. Run Adapt It and on the View menu 
select the "Change Interface Language..." item. Your interface language 
should appear in the list of choices. Select the new interface language 
and click OK. Adapt It reminds you that you will need to quit and restart 
Adapt It for the change to become effective.

As mentioned in 1 above, the easiest way to add a new interface
localization is to send your translation prepared by Poedit to Bill 
Martin who will gladly do the steps in point 2 above and send you
a special installer to add your new localization, and also make
the localization you prepare available to other Adapt It users.

Thank you in advance for any localization work that you do to make
Adapt It more widely available to translators around the world!

Bill Martin
bill_martin@sil.org

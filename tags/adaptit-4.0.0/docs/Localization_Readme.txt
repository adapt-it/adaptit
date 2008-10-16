Adapt It WX - Localization Information

Beginning with version 4.0.0 of Adapt It WX and Adapt It WX Unicode, you can
easily switch the language of Adapt It's interface to any language into which 
Adapt It's interface has been translated.

The full installations of Adapt It WX and Adapt It WX Unicode come with 
interface translations for English (the default), Spanish, French, Russian,
Portuguese, Indonesian, PNG Tok Pisin, and Chinese. Each language interface 
is called a "Localization." Other localizations are in preparation. Some of 
the above localizations initially are just partial translations, that is, 
some interface text and interface messages are not yet translated - in which 
case those parts of the interface will simply display in English instead of
the other language localization.

Where localization folders and files are located after installation:

The AdaptItWX program localizations for a given language are in a file 
called Adapt_It.mo, and the general wxWidgets library's localizations 
are in a file called wxstd.mo. These two files keep the same file names
regardless of what language/localization they contain in them. Hence, 
to keep things organized, they are kept in separate subfolders of a 
"Languages" parent folder within the installation folder where Adapt It
is installed. The installer sets up the localization with the following 
folder/file structure under the installation folder (the 
<installation_folder> is normally c:\Program Files\Adapt It WX  for the 
regular version; c:\Program Files\Adapt It WX Unicode for the Unicode 
version - a different location for Linux and the Macintosh versions):

<installation_folder>
    |
    |
    Languages
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
       |     |- wxstd.mo
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
 
The localization mechanism in Adapt It is smart enough to find and 
load the desired localization. It uses the same .mo files (see diagram 
above) regardless of whether the application is the Regular (non-Unicode) 
or the Unicode version of the application. If localization folders/files 
are present when Adapt It is run for the first time, it will show the 
"Choose Language Dialog" and remember that choice each time the application 
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
localization is updated of corrected. In this case you only need to 
download and install the localization installer (about 1MB in size) to 
get any new and/or updated localizations. Once a new or updated Adapt_It.mo 
and/or wxstd.mo files is present in a named language subfolder, they 
become immediately available to the program and can be chosen using the 
"Change Interface Language..." menu item from the View menu. 

How to EDIT an EXISTING localization for Adapt It's interface:

As mentioned the localizations initially provided with Adapt It WX and
Adapt It WX Unicode are only partial. These partial localizations 
include Spanish, French, Russian, Portuguese, Indonesian, and Chinese.
If you would like to help complete or correct any of the existing
translations for these languages, the Adapt It team welcomes your help!
If you want to help complete or correct the localizations for Spanish, 
French, Russian, Portuguese, Indonesian, or Chinese, here is how you
can do so:
   * The easiest way to edit or correct an existing localization is
to first install a free program called Poedit. PoEdit is an easy to 
use program with a graphical interface that helps you translate 
Adapt It's interface elements. Poedit can be downloaded from the 
internet at the following address:

http://www.poedit.net/download.php

After downloading and installing Poedit on your computer, you can 
locate the .po file for the existing localization. Using the diagram
shown above as a guide, you should be able to locate the .po file for
the localization you wish to edit or complete. Although not listed as
a file in the above diagram, the localization .po files for each of the
existing localizations are in the localization folders named es, fr,
id, pt, ru, tpi and zh. For example, along with Adapt_It.mo and wxstd.mo
you should find a file named es.po in the es subfolder; a file named 
fr.po in the fr subfolder, etc. These files are the files that Poedit 
can use to update or complete a localization for that given language. 
Here is how to update them with Poedit:
   * Double click on the appropriate .po file (for example double click
on the es.po file for Spanish, fr.po for French, etc) which should
cause Poedit to run and automatically load the localization file (if 
Poedit is installed).
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
name must be changed to Adapt_It.mo (see the next point below).
   * The last step is to rename the old Adapt_It.mo (within the 
appropriate localization subfolder) to something like Adapt_It.old;
and then rename the new .mo file to Adapt_It.mo within that same
localization folder. For example, if you loaded es.po into Poedit and
edited or changed any translations and saved them, Poedit automatically
creates a new es.mo file. You would rename any existing Adapt_It.mo
file to Adapt_It.old; then rename the newly created es.mo file to
Adapt_It.mo. Once you have done this Adapt It should automatically 
show any editing changes or new translations the next time you run
Adapt It with that chosen localization as the interface 
language of your choice. 

If you have not already changed Adapt It's interface language to the
language of your choice, here is how to do it: On Adapt It's View 
menu select the "Change Interface Language..." item. Your interface 
language should appear in the list of choices. Select the new interface 
language and click OK. Adapt It reminds you that you will need to quit 
and restart Adapt It for the change to become effective.

How to prepare a NEW localization for Adapt It's interface:

Any translator can prepare a new localization for Adapt It. After 
installing Adapt It WX or Adapt It WX Unicode, you will find two 
files located within the "Languages" folder of the installation 
(see above diagram). One file is called default.po and the other is 
called default.mo. These files are templates that you can use to 
prepare a new localization for Adapt It's interface. The easiest 
way to do the actual translation is to first install a free program 
called Poedit. Poedit can be downloaded from the internet at the 
following address:

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

There are about 1,700 elements to translate in Adapt It's interface,
so translating the interface into a new language is a fairly large
task. If you do translate Adapt It's interface there are a few 
remaining steps you must do in order to be able to use your newly
translated Adapt It interface/localization:

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

   2.a. Locate the "Languages" folder on your computer. On Windows
this folder will be located by default at: 
 c:\Program Files\Adapt It WX\Languages
or, if you've installed the Unicode version it will be located at:
 c:\Program Files\Adapt It WX Unicode\Languages

   2.b. Within your "Languages" folder create a subfolder and give
this subfolder a name that corresponds to the two-letter or 
three-letter abbreviation for that language as given at the IANA 
language registry web site at:

http://www.iana.org/assignments/language-subtag-registry

Note: Your subfolder should be named with the abbreviation used as 
the "Subtag" for that language as listed on the IANA website.

   2.c. Copy your default.mo file (which Poedit produced when you
last saved your localization work using Poedit) into the newly 
created subfolder you created in step 2.b above.

   2.d. Rename the default.mo file to Adapt_It.mo

3. Your new localization is now available for use within 
Adapt It WX or Adapt It WX Unicode. Run Adapt It WX or Adapt It WX
Unicode. On the View menu select the "Change Interface Language..."
item. Your interface language should appear in the list of choices.
Select the new interface language and click OK. Adapt It reminds you
that you will need to quit and restart Adapt It for the change to
become effective.

As mentioned in 1 above, the easiest way to add a new interface
localization is to send your translation prepared by Poedit to Bill 
Martin who will gladly do the steps in point 2 above and send you
a special installer to add your new localization, and also make
the localization you prepare available to other Adapt It users.

Thank you in advance for any localization work that you do to make
Adapt It more widely available to translators around the world!

Bill Martin
bill_martin@sil.org

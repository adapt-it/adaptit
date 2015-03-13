# Introduction #

RTF free translation export halts prematurely


# Details #

I investigated your data to find the cause of the RTF free translation export halting early, at verse 21, rather than exporting all the free translations in the file.

As I suspected, the problem is within code which I wrote for doing the free translation export. I'll try fix that before the release of the next version.

I'll explain why it failed in some detail here, so that you can avoid the problem in future, still using version 5.2.1, without having to wait for a new release.

The cause? Our code for building the RTF output will fail if there is an end-marker (such as \w**) without a preceding matching start-marker (such as \w) preceding it. It turned out that something rare happened at verse 21, involving a merge of two words, and the marker \w was between the two words. That merger made \w become a "medial marker" that is, a marker which is inside a sequence of merged source text words (rather than at the start or end of the sequence). Our free translation export code did not properly handle that medial \w marker, so it was absent from the free translation text which our code constructed for verse 21. The free translation text for verses 21 and 22 that our code created was this (the font may not show some special characters correctly - please ignore that as it doesn't matter):**

\v 21 Yesu na bheghesebuwa ndibhaaki nibhakukidha ka tau\_ni\_Kapelenaumu. Toolo sa sbhaato ndya Bhayu\_daaya ndyaduwa tooli_yabhakakolilyagaaku_ muliimo, \w_du\_mbi**nakugoodha kidya Bhayu\_daaya yabhalamiilyaga, \w \w** nakwegeesa.
\v 22 Bhaakpa ndibhamwoka nibhakisweka si\_i\_ni_ nanga keegeesa bhya bhegheesa sakilagilo sa di\_i\_ni_sa Ki\_yu\_daaya yaegeesaaga na maani_.

As you can see, after the word  muliimo there is an end-marker \w**, and there is no \w start-marker preceding it. When our RTF production code tried to make a valid RTF file with that data, it halted at the unmatched \w**, and that is why you only got that much of the document exported, only as far as the middle of verse 21. (When we fix our code for producing the free translation,we will have to make sure that any 'medial marker' is included in the free translation text, so that the marker and endmarker always come in matching pairs - then the RTF production code will not fail.)

Here is a little more explanation which may help you avoid this problem with version 5.2.1.

\w and its endmarker \w**in the USFM (Unified Standard Format Marking) standard are used for marking a "wordlist text item" in the text. The source text at the relevant point in verse 21 then had a phrase marked up for later inclusion in a word-list which would be included in the published scripture book. The phrase in the source text was:  Kilo kya Sabhato eki**

In Adapt It, before the merger of the words "Obu" and "Kilo" to make the phrase "Obu Kilo", the USFM markup of the original source text looked like this, and below I've shown where you made a merger of two words, Obu and Kilo, to form the phrase Obu Kilo:

> ... ya    Kapelenaumu.      Obu     \w     Kilo          kya     Sabhato    eki   \w_Bayu\_daa...
      _|    merger made here   |_           

Because the \w marker was between Obu and Kilo, it cannot be moved forwards or backwards, so for the merger "Obu Kilo" it is stored in a special location. (Our code then failed to get hold of it for the free translation, resulting in the error.)_

Here is the important point:

1.  Now, if the \w had been in front of the word Obu, then the RTF output would have exported all 45 verses. That is, the following would not have halted the export short at verse 21, even if you still made the phrase "Obu Kilo"  (I tested this, and it works correctly):

> ... ya    Kapelenaumu.      \w   Obu    Kilo               kya   Sabhato   eki   \w**Bayu\_daaya
      |    merger made here    |**



2. Alternatively, if the \w and its matching end-marker \w**were not present at all in the source text (before you used the data to form the document for adaptation purposes), then you would have the following -- and it too would result in all 45 verses being exported correctly in the free translation export:**

> ... ya   Kapelenaumu.     Obu        Kilo                 kya   Sabhato  eki    Bayu\_daaya
     _|    merger made here   |_          

So, I hope you can see how the problem arose, and perhaps a way to avoid it happening again with version 5.2.1.

If it happens again, there are several ways you can proceed.....

(1)  If you make a merger, and then in the Adapt It window you see the navigation text above the source text line say:   "Medial markers: \w"
Then you can be certain that an RTF export of the free translation text will fail at that point, and your exported file will lack the material which follows it. (The marker may be some other marker than \w, it just depends on what the hidden marker was that was within the selection when you formed the merger. Any marker will lead to this problem, not just \w, if it ends up in the middle of a merger.) To get around this problem,
(1)
(a) put the phrase box under the merger, that is, under "Obu Kilo" and hit the Unmerge button, so that Obu and Kilo are no longer a phrase (temporarily), and then
(b) do the RTF free translation export -- now the marker is no longer medial, and the free translation export will not halt early.
(c) select the words Obu and Kilo, and re-make the merger.

(2) If you don't need to leave the \w and \w**markers in the source text file, then you can search for \w and simply delete these markers from the Plain Text file, before you use the file to make a document in Adapt It for translation into Kwamba.**

(3) If you see "Medial markers: \somemarker" in the Adapt It window, and it doesn't matter too much about how you do the merger, then do one of these:
(a) Unmerge the merger, and make your selection large enough so that the matching end-marker is also included within the selection, and re-make this larger merger. Then the RTF free translation export will work fine. Or
(b) Unmerge the merger, and make you selection select only those words that are between the marker and its end-marker, then re-do this smaller merger. Then the RTF export will work fine. Or
(c) Unmerge the merger, and don't make a merger there at all. Instead, make a selection which includes both the marker and the end-marker, and click the 'new' button to "Do a Retranslation". The retranslation will not be entered in the knowledge base, but that is not likely to matter much. Your free translation RTF export will then work correctly.
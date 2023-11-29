# Adapt It

---

- User site: http://adapt-it.org/
- Windows and OS X Installers: http://adapt-it.org/download/all-versions/
- Linux packages: http://packages.sil.org/

---

[![Build Status](https://github.com/adapt-it/adaptit/actions/workflows/tests.yml/badge.svg)](https://github.com/adapt-it/adaptit/actions/workflows/tests.yml)

**Adapt It** is an easy-to-use application that helps users translate or adapt source text documents into target or translated text. This process depends on the knowledge of the human translator in understanding both languages. In linguistics jargon, Adapt It is a _Machine Assisted Human Translation (MAHT)_ editor.

<img src="https://github.com/adapt-it/adaptit/blob/master/res/screenshots/ai_main.png" width=512 height=400 alt="Main screen" title="Main screen" />

Adapt It maintains a translation memory called a _Knowledge Base_ which stores previously-entered word and phrase adaptations that can be automatically inserted into the target text, thus speeding the translation process. Adapt It uses plain text (txt) files for input and expects the use of Unified Standard Format Markers (USFM) in the input file. The translated text can be exported to several formats -- USFM, Rich Text (rtf), HTML, or -- on Windows and Linux -- mobile formats such as .epub via the [SIL Pathway](https://software.sil.org/pathway/) program.

<img src="https://github.com/adapt-it/adaptit/blob/master/res/screenshots/ai_export.png" width=444 height=400 alt="Export Options" title="Export Options" />

Adapt It is written in C++ and utilizes the wxWidgets cross-platform library. Supported platforms include Windows, OS X and Linux variants.

## Contributing to the Project

For those wanting to contribute to this project, check out the [wiki pages](https://github.com/adapt-it/adaptit/wiki#want-to-contribute). If you're a coder, tester, interface designer, technical writer, or would like to localize Adapt It to your language, we'd love to talk!

## Philosophy

Adapt It is a free open source tool for quickly translating between related languages. We seek to create, as a community, an adaptation tool that will run on all major platforms and facilitate translation of [USFM](https://ubsicap.github.io/usfm/) text, and support collaboration between other translation tools such as [Paratext](https://paratext.org/) and [Bibledit](https://bibledit.org/).

## License

See [LICENSE](https://github.com/adapt-it/adaptit/blob/master/license/LICENSING.txt).

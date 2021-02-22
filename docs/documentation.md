---
author: Jérôme Dockès -- <jerome@dockes.org>
title: labelbuddy documentation
monobackgroundcolor: "#E2E2E2"
monofont: monospace
fontsize: 17px
margin-left: 0
margin-right: 0
header-includes: |
  <style>
  body {
  max-width: 50em;
  }
  pre {
  white-space: pre;
  }
  pre code {
  white-space: pre;
  overflow: scroll;
  }
  </style>
---

# Introduction

labelbuddy is an
[open-source](https://github.com/jeromedockes/labelbuddy/blob/main/LICENSE.txt)
desktop application for annotating documents. It can be used for example for
Part Of Speech tagging, Named Entity Recognition, sentiment analysis and
document classification ...

It aims to be easy to install and use, and can efficiently handle many
documents, labels and annotations.

## labelbuddy vs other annotation tools

There are several tools for annotating documents. Most of them, such as
[doccano](https://doccano.github.io/doccano/) and
[labelstudio](https://labelstud.io/) are meant to run on a web server and be
used online. If you are crowdsourcing annotations and want many users to
contribute annotations to a central database without installing anything on
their machine you should turn to one of these tools.

However if you do not plan to host such a tool on a server, it may not be
convenient for each annotator to install one of these rather complex tools and
run a local server and database management system on their own machine in order
to annotate documents. In this case, it may be easier to rely on a desktop
application such as labelbuddy, which is a more lightweight solution.

labelbuddy supports the input and output formats of doccano so it is
[possible](#copying-annotations-to-and-from-doccano) to switch from one to the
other or to combine the work of annotators that use either.

## Quick start

The easiest way to install labelbuddy is from a [binary
distribution](#from-binary-copies). Then to try it out, you can start labelbuddy
and open a (temporary) demo database by selecting "Demo" from the "File" menu.
You can play around with labelbuddy's features in this temporary project. If you
decide to start creating annotations that you want to keep, open a new database
and import your [documents](#importing-documents) and
[labels](#importing-labels).

Note: a [labelbuddy database](managing-projects) is actually just a regular file
on your disk (an [SQLite](https://www.sqlite.org/index.html) database).

# Installation

## From binary copies

Packages are available for Debian and Windows
[here](https://github.com/jeromedockes/labelbuddy/releases).

### On Debian or Ubuntu

-   [Download](https://github.com/jeromedockes/labelbuddy/releases) the package
    that corresponds to your architecture (if you don't know it is most likely
    `amd64` aka `x86-64` but you can check with `dpkg --print-architecture` or
    `uname -m`).
-   Install it with `apt`, for example:

```
sudo apt install labelbuddy_0.0.1-1_amd64.deb
```

### On Windows

-   [Download](https://github.com/jeromedockes/labelbuddy/releases)
    and run `labelbuddy_windows_installer.exe`

## From source

-   Download a source distribution from
    [here](https://github.com/jeromedockes/labelbuddy/releases), or get
    the latest code by cloning or downloading [the labelbuddy git
    repository](https://github.com/jeromedockes/labelbuddy/).

-   labelbuddy uses [Qt 5](https://www.qt.io/). Qt can probably be installed
    with your package manager, for example: `apt-get install qt5-default`.
    Otherwise you can get it from the [Qt downloads
    page](https://www.qt.io/download-qt-installer).

-   labelbuddy can be built with [CMake](https://cmake.org/). In an
    empty directory run:

    ```
    cmake /path/to/labelbuddy
    cmake --build .
    ```

-   It can also be built with
    [qmake](https://doc.qt.io/qt-5/qmake-manual.html):

    ```
    qmake /path/to/labelbuddy
    make
    ```

# Using labelbuddy

Documents and labels can be imported into labelbuddy from various formats. Once
you have imported documents and labels into the labelbuddy database, you can
annotate the documents and finally export your annotations. It is also possible
to import annotations exported from labelbuddy or doccano.

Documents and labels that are already in the database are skipped if you try to
import them again.

## Importing documents

In the "Import / Export" tab, click "Import docs & annotations" and select a
file. The format is deduced from the filename extension.

For each document, you will import its text, that you will annotate.
Optionally, you can also associate with it some metadata, for example a
title, DOI, author ... This data is not used by labelbuddy. It is stored
and bundled with the document when you export it.

These texts and metadata can be imported into the labelbuddy database from
several plain text formats.

### From `.txt`

The simplest format you can use is a `.txt`. In this case, the file must contain
one document per line. The newlines that separate documents are not considered
part of the document and are discarded.

While convenient, this format has some limitations: you cannot associate
metadata with the documents, and the documents cannot contain newlines.
Moreover, the file's encoding will be interpreted based on your locale settings.
The other import formats share none of these limitations.

### From `.json`

The file must be a JSON file containing one JSON array. Each element of
the array represents one document and its (optional) metadata. These
elements can be either a JSON array of the form
`[text, metadata]` or a JSON object with the keys
`"text"` and `"meta"`.

`text` is a string literal containing the text of the
document. `meta` is optional (whether you are using the array
or object format). If specified, it is a JSON object containing user
data about the document.

Therefore imported JSON files might look like:

-   using the array format:

    ```
    [
    ["text of first doc", {"title": "doc 1", "DOI": "123"}],
    ["text of second doc"]
    ]
    ```

    (note the second document doesn't have any metadata)

-   using the object format (compatible with doccano):

    ```
    [
    {"text": "text of first doc", "meta": {"title": "doc 1", "DOI": "123"}},
    {"text of second doc"}
    ]
    ```
  Moreover, using the object format it is also possible to 
  [import annotations](#importing-annotations) together with a new document,
  or for a document already in the database.

### From `.jsonl`

When importing a `.json` file the whole file is read into memory before
inserting the documents in the database. If you prefer documents to be read one
by one, you can use [JSON Lines](https://jsonlines.org/). It is almost the same
as the JSON format, but instead of having one JSON array, the file must contain
one JSON document per line. For example:

```
["text of first doc", {"title": "doc 1", "DOI": "123"}]
["text of second doc"]
```

(Note the outer brackets are removed and the documents are not separated
by commas.)

As for `.json`, `.jsonl` also allows [importing
annotations](#importing-annotations).

### From `.xml`

You can also use a simple XML format. In this case as well, the documents
are read one by one. The root element must be `document_set`
and contain any number of `document` elements. Each
`document` contains the text of a document, and metadata can
be stored in its attributes. For example:

```
<?xml version="1.0" encoding="UTF-8"?>
<document_set>
  <document DOI="123" title="doc 1">text of first doc</document>
  <document>text of second doc</document>
</document_set>
```

## Importing labels

As for documents, the format is deduced from the filename extension when
importing labels. You specify the label name and an optional label color (which
can be changed from within the GUI application).

### From `.txt`

The text file contains one label per line. For example:

```
Noun
Verb
Adjective
```

You can specify a color for each label (or labels that contain newlines)
by using the `.json` format.

### From `.json`

The file must contain one JSON array with one element per label. As for
documents, each label can be represented by a JSON array or a JSON
object. If it is an array, the first element is the label name and the
(optional) second one is a color string. If it is an object, it must
have the key `text` and optionally the key
`background_color`. For example:

```
[
["Noun", "#ff0000"],
["Verb", "yellow"],
["Adjective"]
]
```

Or using the object format (compatible with doccano):

```
[
{"text": "Noun", "background_color": "#ff0000"},
{"text": "Verb", "background_color": "yellow"},
{"text": "Adjective"}
]
```

## Annotating documents

Once you have imported labels and documents you can see them in the
"Dataset" tab. You can delete labels or documents and change the color
associated with each label. You then go to the "Annotate" tab. (If you
double-click a document it will be opened in the "Annotate" tab).

To annotate a document, select the region you want to label with the
mouse and click on the appropriate label.

Once you have created annotations, you can select any of them by
clicking it. (It becomes bold and underlined and) you can change its
label by clicking on a different one or remove the annotation by
clicking "remove".

If you create a new annotation that overlaps with a previously existing
one, the previously existing one is automatically removed.

### Summary of key bindings in the "Annotate" tab

- **\<Ctrl\>** and scroll the mouse:   zoom or dezoom the text
- **\<Ctrl\>-F**: search
- **\<Enter\>**: next search match
- **\<Shift\>-\<Enter\>**: previous search match
- **\<Ctrl\>-J**, **\<Ctrl\>-N**, **\<Down\>**: scroll down one line
- **\<Ctrl\>-K**, **\<Ctrl\>-P**, **\<Up\>**: scroll up one line
- **\<Ctrl\>-D**: scroll down one page
- **\<Ctrl\>-U**: scroll up one page

## Exporting annotations

Once you are satisfied with your annotations you can export them to an
`.json`, `.jsonl` or `.xml` file to
share them or use them in other applications.

Back in the "Import / Export" tab, click "Export". You can choose to
export all documents or only those that have annotations. You can choose
to export the text of the documents or not. If you don't export the
text, the documents can be identified from metadata you may have
associated with them, or by the MD5 checksum of the text that is always
exported. You can also provide an "Annotation approver" (user name),
that will be exported as the `annotation_approver` (this term
is used for compatibility with doccano).

When clicking "Export" you are asked to select a file and the resulting
format will depend on the filename extension.

### Exporting to `.json`

The file will contain one JSON array, with one object per document. Each element
is always on one separate line. Each object has the keys "text" (optional),
`annotation_approver` (optional), `document_md5_checksum` (always), `labels`
(always), and `meta` (always, containing the metadata provided when importing
the document, if any).

The value for `labels` is a JSON array, with one element per annotation. Each
annotation is an array containing 3 elements: the position of the first
character (starting from 0 at the begining of the text), the position of one
past the last character, and the label name. For example if the text starts with
"hello" and you highlighted exactly that word, and labelled it with `label_1`,
the associated annotation will be `[0, 5, "label_1"]`.

In summary, exported annotations for the documents in the examples above
might look like:

```
[
{"annotation_approver":"jerome","document_md5_checksum":"f5a42de39848dbdadf79aade46135b7a","labels":[[0,4,"Noun"]],"meta":{"DOI":"123","title":"doc 1"},"text":"text of first doc"},
{"annotation_approver":"jerome","document_md5_checksum":"d5c080bd4c6033f977182e757a0059b1","labels":[[0,4,"Verb"],[8,14,"Adjective"]],"meta":{},"text":"text of second doc"}
]
```

### Exporting to `.jsonl`

If you choose to export to a [JSON lines](https://jsonlines.org/) file, the
content will be almost the same as the JSON one, but with just one JSON object
per line and not one JSON array containing all the documents:

```
{"annotation_approver":"jerome","document_md5_checksum":"f5a42de39848dbdadf79aade46135b7a","labels":[[0,4,"Noun"]],"meta":{"DOI":"123","title":"doc 1"},"text":"text of first doc"}
{"annotation_approver":"jerome","document_md5_checksum":"d5c080bd4c6033f977182e757a0059b1","labels":[[0,4,"Verb"],[8,14,"Adjective"]],"meta":{},"text":"text of second doc"}
```

### Exporting to `.xml`

If you choose a `.xml` file the result is a UTF-8 encoded XML
document. The root element is `annotated_document_set`, it
contains zero or more `annotated_document`, each containing:

-   a `document_md5_checksum` element with the checksum as
    its text
-   if you chose to export the documents' content: a
    `document` element with the content as its text, and the
    metadata as its attributes.
-   otherwise: an empty `meta` element with the metadata as its
    attributes.
-   an `annotation_set` element, containing
    `annotation` elements, each containing:
    -   a `start_char`
    -   an `end_char`
    -   a `label`

So for our example it looks like:

```
<?xml version="1.0" encoding="UTF-8"?>
<annotated_document_set>
    <annotated_document>
        <document_md5_checksum>f5a42de39848dbdadf79aade46135b7a</document_md5_checksum>
        <document DOI="123" title="doc 1">text of first doc</document>
        <annotation_approver>jerome</annotation_approver>
        <annotation_set>
            <annotation>
                <start_char>0</start_char>
                <end_char>4</end_char>
                <label>Noun</label>
            </annotation>
        </annotation_set>
    </annotated_document>
    <annotated_document>
        <document_md5_checksum>d5c080bd4c6033f977182e757a0059b1</document_md5_checksum>
        <document>text of second doc</document>
        <annotation_approver>jerome</annotation_approver>
        <annotation_set>
            <annotation>
                <start_char>0</start_char>
                <end_char>4</end_char>
                <label>Verb</label>
            </annotation>
            <annotation>
                <start_char>8</start_char>
                <end_char>14</end_char>
                <label>Adjective</label>
            </annotation>
        </annotation_set>
    </annotated_document>
</annotated_document_set>
```

## Importing annotations

Annotations that were exported to `.json` or `.jsonl` can be imported back into
the same or another labelbuddy database. Simply use the "Import docs &
annotations" button and select the exported file. Labels used in the annotations
that are not in the database will be added (with an arbitrary color that can be
changed in the application).

For documents already in the database, annotations will be imported whether the
document's text was exported together with the annotations or not. If the text
is not present in the exported file, the MD5 checksum will be used to associate
the annotations with the correct document.

To avoid mixing annotations from different sources, if the document already
contains annotations in the database, the new annotations will not be added.

For documents that are not in the database, their text must have been exported
together with the annotations and in this case both the document and the
annotations will be added to the database.

### Copying annotations to and from doccano

Documents and annotations exported from doccano can also be imported into a
labelbuddy database. To do so, when exporting from doccano select the format
"jsonl (text label)". Make sure to save them in a file with the `.jsonl`
extension (not `.json`) otherwise labelbuddy will try to parse it as JSON and
JSON Lines is not valid JSON.

**Note:** doccano strips leading and trailing whitespace from documents when
importing them. Therefore if you import the result into a labelbuddy database
that already contains the original documents, it may not be recognized as being
the same (labelbuddy doesn't modify the imported documents) and you might end up
with (near) duplicate documents in the database.

Similarly, annotations exported from labelbuddy in the `.jsonl` format together
with the document's text can be imported into doccano (selecting the "JSONL"
import format). 

**Note:** if the original document contained leading whitespace, labelbuddy
annotations will appear shifted when doccano removes the whitespace. Moreover,
doccano allows duplicate documents so if the documents were already in the
doccano database, they will appear as new (duplicate) documents rather than new
annotations for existing documents.

## Managing projects

Each labelbuddy project (a set of documents, labels and annotations) is
an [SQLite](https://www.sqlite.org/index.html) database. That is a
single binary file on your disk that you can copy, backup, or share, like
any other file.

(Advanced users can also open a connection directly to the database to
query it or even modify it -- at your own risk! back it up before and
set "`PRAGMA foreign_keys = ON`")

When you first start labelbuddy it creates a new database in
`~/labelbuddy_data.sqlite3`. You can switch to a different one from the "File"
menu by selecting "Open" or "New". The path to the current database is displayed
in the "Import / Export" tab.

The next time you start labelbuddy, it will open the last database that
you opened.

The database to open can also be specified when invoking labelbuddy from the command line:
```
labelbuddy /path/to/my_project.sqlite3
```

If you just want to give labelbuddy a try and don't have documents or labels
yet, you can also select "Demo" from the "File" menu to open a temporary
database pre-loaded with a few examples.

# Conclusion

labelbuddy was created using C++, [Qt](https://www.qt.io/),
[SQLite](https://www.sqlite.org/index.html), tools from the [GNU
project](https://www.gnu.org/), and more.

If you find a bug, kindly open an issue on the labelbuddy
[repository](https://github.com/jeromedockes/labelbuddy/issues).

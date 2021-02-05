# `labelbuddy`: a small tool for annotating documents

## Installation

### From compiled binaries

Packages are available for Debian and Windows [here](https://osf.io/m2bcg/).

### From source

`labelbuddy` uses `Qt 5`. `Qt` can probably be installed with your package
manager, for example: `apt-get install qt5-default`. Otherwise you can get it
from the [downloads page](https://www.qt.io/download-qt-installer).

Then `labelbuddy` can be build either with `CMake` 
```
cmake /path/to/labelbuddy
cmake --build .
```

or `qmake`: `qmake && make`.


## Usage

### Importing documents

When using `labelbuddy` for the first time, go to the "Import / Export" tab to
import documents and labels.
Documents can be provided either in a `.txt`, `.jsonl`, `.json` or `.xml` file.

If `.txt` it must contain one document per line.

If `.json` it must contain a json array, where each element is an array
containing two elements: the text of the document, and a json object which is
not used by `labelbuddy` but is stored and exported together with annotations.
For example:

```
[
["text of first document", {"title": "my book"}],
["text of second document", {}],
...
]
```

(here for example `{"title": "my book"}` is optional and can be used to
associate user metadata with the document and its annotations).

If `.jsonl` (json lines), it is the same thing without the outer brackets and
each json document must be on one line.
If `.xml` it must have the format:

```
<?xml version='1.0' ?>
<document_set>
  <document doi="123" author="me">text of first document</document>
  ...
</document_set>
```

Here also, the attributes of each `<document>` element are optional and used to
store extra metadata.


### Importing labels

Labels must be imported from a `.json` file of the form

```
[
    {"text": "label name", "background_color": "#00ffaa"},
    {"text": "label 2", "background_color": "#ffaa00"},
    ...
]
```

`"background_color"` is optional and can be changed from within the application.

### Annotating documents

Once documents and labels are imported we can see them in the "Dataset" tab and
select a document to annotate from there, or go directly to the "Annotate" tab.
There, select a snippet of text with the mouse and click on a label to annotate
it. Clicking on an already labelled snipped will select it; then its label can
be changed or it can be removed by clicking "remove".

### Exporting annotations

Back to the "Import / Export" tab, click "Export" and select a file in which
annoations will be written. If the file extension is `.json` or `.jsonl` the
file will one `json` document, for each document in the database, containing the
text and annotations, each on a separate line. If it is `xml` annotations are
exported in xml.

### Managing projects

We can switch to a different project by using the "File" menu and selecting a
location where a new database will be created. The path to the database to use
can also be passed as an argument when starting `labelbuddy` from the command
line.

## Reporting bugs

Please report bugs by opening an issue in
[https://github.com/jeromedockes/labelbuddy](https://github.com/jeromedockes/labelbuddy)


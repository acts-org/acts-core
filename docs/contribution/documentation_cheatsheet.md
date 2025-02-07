<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Documentation Markdown Cheatsheet

Below there are some snippets for creating documentation with the Myst Markdown parser.  Some examples might not work correctly locally if you did not build the full API documentation with `make docs-with-api`. Some links for further reading:

* The full documentation of the Myst Parser can be found [here](https://myst-parser.readthedocs.io/en/latest/index.html).
* A list of Sphinx directives can be found [here](https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html#directives).
* A list of Doxygen directives can be found [here](https://breathe.readthedocs.io/en/latest/directives.html).

## reStructuredText directives in Myst Markdown

* reStructuredText:

```text
.. directivename:: arguments
   :key1: val1
   :key2: val2

   This is
   directive content
```

* Myst Markdown (instead of ::: also ``` is possible as delimiter):
```text
:::{directivename} arguments
---
key1: val1
key2: val2
---
This is
directive content
:::
```


## Link to API documentation

* Code:

```text
A link to {class}`Acts::Volume`.
```

* Result :

A link to {class}`Acts::Volume`.

## Pull in API documentation

* Code:

```text
:::{doxygenclass} Acts::Volume
---
members: center
---
:::
```

* Result:

:::{doxygenclass} Acts::Volume
---
members: center
---
:::

(cheatsheetlabels)=
## Cross-referencing and labels

* Setting a label (should be in front of a heading or something like that):

```text
## Cross-referencing and labels
(cheatsheetlabels)=
```

* Referencing a label (with empty `[]` the text of the heading is used):

```text
Click [here](cheatsheetlabels) to come to the label.
Automatic label text: [](cheatsheetlabels).
```

Click [here](cheatsheetlabels) to come to the label.
Automatic label text: [](cheatsheetlabels).

## Info boxes

* Creating boxes (other types are `attention`, `caution`, `danger`, `error`, `hint`, `important`, `tip`, `warning`):

````text
:::{note}
This is something good to know
:::
````

* This looks like:

:::{note}
This is something good to know
:::

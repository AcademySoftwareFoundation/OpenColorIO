# OCIO v2 Documentation Effort

Since the API autodoc effort is happening simultaneously it will be easier for
a lot of contributors to do a light-weight build of the Sphinx docs, which
doesn't require building OCIO in its entirety.

## How to Build RST-Only Docs

In addition to the general OCIO v2 dependencies (Python and Sphinx), install the
Python packages listed in the `docs/requirements.txt` file.

```
pip install -r docs/requirements.txt
```

Build the sphinx docs locally...

```
cd docs
mkdir _build
sphinx-build -b html . _build
firefox _build/index.html
```


## Quirks

The vuepress theme that we've migrated to has some quirks to its design. For
example it only allows two nested table of contents (TOC). So things have to be
organized in a slightly different way than other sphinx projects.

The root-level `toc_redirect.rst` points to where to find the different section
TOCs. The name and contents of each sections TOC is defined in that
sub-directory's `_index.rst` file.

In this TOC the `:caption:` directive determines what the name of the section
will be in the side-bar, and in the header of the website. The *H1* header
determines the name of the page in the right/left arrows navigation bar. In a
lot of cases this ends up doubling up the name on the page, but this seems
unavoidable at the present time. If additional explanatory text is put in the
`_index.rst` files then it shouldn't be as problematic.

The site will show all *H1* headers in the side panel by default, these then
expand when selected to show all *H2* headers.

Due to the limited TOC and side-bar depth, we shouldn't be afraid of looong
pages with many *H2* headings to break down the page into logical quadrants.
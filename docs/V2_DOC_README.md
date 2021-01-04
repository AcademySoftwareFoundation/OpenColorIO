# OCIO v2 Documentation Effort

Since the API autodoc effort is happening simultaneously it will be easier for
a lot of contributors to do a light-weight build of the Sphinx docs, which
doesn't require building OCIO in its entirety.

## How to Build HTML Docs

In addition to the general OCIO v2 dependencies, install Doxygen, Sphinx, and 
the Python packages listed in the `docs/requirements.txt` file.

Doxygen can be installed with a package manager, or by downloading a binary 
distribution from [Doxygen's website](https://www.doxygen.nl/download.html). 
Use one of the following commands to install Doxygen with a platform-specific 
package manager:

Linux (Debian, etc.):

```
apt-get install doxygen
```

Linux (Fedora, etc.):

```
yum install doxygen
```

macOS ([Homebrew](https://brew.sh/)):

```
brew install doxygen
```

Windows ([Chocolatey](https://chocolatey.org/)):

```
choco install doxygen.install
```

Sphinx and the other Python dependencies can be installed with a single `pip` 
command on all platforms:

```
pip install -r docs/requirements.txt
```

To build only the RST docs (excluding API) locally:

```
cd docs
mkdir _build
sphinx-build -b html . _build
<your web browser name> _build/index.html
```

To build all HTML docs (including API) locally:

```
mkdir _build
cd _build
cmake ../. -DOCIO_BUILD_DOCS=ON -DCMAKE_INSTALL_PREFIX=../_install
make -j<your thread count> docs
<your web browser name> docs/build-html/index.html
```

Python 3 is required to build the documentation. If you have multiple Python
installs you'll need to make sure pip and CMake find the correct version. You 
can manually inform cmake of which to use by adding this option to the above 
`cmake` command, which configures the documentation build:

```
-DPython_ROOT=<Path to Python 3 root directory>
```

To ease the documentation build and test on macOS one could also add these 
three lines in the ~/.zshrc file:

```
alias python=/usr/local/bin/python3
alias pip=/usr/local/bin/pip3
alias <your web browser name>=open -a "<your web browser application name>"
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

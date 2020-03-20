/*
  Copyright 2008 Larry Gritz and the other authors and contributors.
  All Rights Reserved.
  Based on BSD-licensed software Copyright 2004 NVIDIA Corp.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of the software's owners nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  (This is the Modified BSD License)
*/


/// \file
/// \brief Simple parsing of program command-line arguments.


#ifndef INCLUDED_OCIO_ARGPARSE_H
#define INCLUDED_OCIO_ARGPARSE_H

#include <string>
#include <vector>

#ifndef OPENCOLORIO_PRINTF_ARGS  /* See comments in strutil.h */
#   ifndef __GNUC__
#       define __attribute__(x)
#   endif
#   define OPENCOLORIO_PRINTF_ARGS(fmtarg_pos, vararg_pos) \
        __attribute__ ((format (printf, fmtarg_pos, vararg_pos) ))
#endif

class ArgOption;   // Forward declaration



/////////////////////////////////////////////////////////////////////////////
///
/// \class ArgParse
///
/// Argument Parsing
///
/// The parse function takes a list of options and variables or functions
/// for storing option values and return <0 on failure:
///
/// \code
///    static int
///    parse_files (int argc, const char *argv[])
///    {
///        for (int i = 0;  i < argc;  i++)
///            filenames.push_back (argv[i]);
///        return 0;
///    }
/// 
///    ...
///
///    ArgParse ap;
///
///    ap.options ("Usage: myapp [options] filename...",
///            "%*", parse_objects, "",
///            "-camera %f %f %f", &camera[0], &camera[1], &camera[2],
///                  "set the camera position",
///            "-lookat %f %f %f", &lx, &ly, &lz,
///                  "set the position of interest",
///            "-oversampling %d", &oversampling,  "oversamping rate",
///            "-passes %d", &passes, "number of passes",
///            "-lens %f %f %f", &aperture, &focalDistance, &focalLength,
///                   "set aperture, focal distance, focal length",
///            "-format %d %d %f", &width, &height, &aspect,
///                   "set width, height, aspect ratio",
///            "-v", &flag, "verbose output",
///            NULL);
///
///    if (ap.parse (argc, argv) < 0) {
///        std::cerr << ap.geterror() << std::endl;
///        ap.usage ();
///        return EXIT_FAILURE;
///    }
/// \endcode
///
/// The available argument types are:
///    - no \% argument - bool flag
///    - \%d - 32bit integer
///    - \%f - 32bit float
///    - \%F - 64bit float (double)
///    - \%s - std::string
///    - \%L - std::vector<std::string>  (takes 1 arg, appends to list)
///    - \%* - catch all non-options and pass individually as an (argc,argv) 
///            sublist to a callback, each immediately after it's found
///
/// There are several special format tokens:
///    - "<SEPARATOR>" - not an option at all, just a description to print
///                     in the usage output.
///
/// Notes:
///   - If an option doesn't have any arguments, a flag argument is assumed.
///   - Flags are initialized to false.  No other variables are initialized.
///   - The empty string, "", is used as a global sublist (ie. "%*").
///   - Sublist functions are all of the form "int func(int argc, char **argv)".
///   - If a sublist function returns -1, parse() will terminate early.
///
/////////////////////////////////////////////////////////////////////////////


class ArgParse {
public:
    ArgParse ();
    ArgParse (const ArgParse & ) = delete;
    ArgParse & operator= (const ArgParse & ) = delete;
    ArgParse (int argc, const char **argv);
    ~ArgParse ();

    /// Declare the command line options.  After the introductory
    /// message, parameters are a set of format strings and variable
    /// pointers.  Each string contains an option name and a scanf-like
    /// format string to enumerate the arguments of that option
    /// (eg. "-option %d %f %s").  The format string is followed by a
    /// list of pointers to the argument variables, just like scanf.  A
    /// NULL terminates the list.
    int options (const char *intro, ...);

    /// With the options already set up, parse the command line.
    /// Return 0 if ok, -1 if it's a malformed command line.
    int parse (int argc, const char **argv);

    /// Return any error messages generated during the course of parse()
    /// (and clear any error flags).  If no error has occurred since the
    /// last time geterror() was called, it will return an empty string.
    std::string geterror () const;

    /// Deprecated
    ///
    std::string error_message () const { return geterror (); }

    /// Print the usage message to stdout.  The usage message is
    /// generated and formatted automatically based on the command and
    /// description arguments passed to parse().
    void usage () const;

    /// Return the entire command-line as one string.
    ///
    std::string command_line () const;

private:
    int m_argc;                           // a copy of the command line argc
    const char **m_argv;                  // a copy of the command line argv
    mutable std::string m_errmessage;     // error message
    ArgOption *m_global;                  // option for extra cmd line arguments
    std::string m_intro;
    std::vector<ArgOption *> m_option;

    ArgOption *find_option(const char *name);
    void error (const char *format, ...) OPENCOLORIO_PRINTF_ARGS(2,3);
    int found (const char *option);      // number of times option was parsed
};

#endif // INCLUDED_OCIO_ARGPARSE_H

#!/usr/bin/python

"""
Small Script to extract reStructuredText from OCIO headers
    - http://sphinx.pocoo.org/rest.html
    - http://docutils.sourceforge.net/docs/ref/rst/restructuredtext.html
    
This looks for the tokens:

/*+doc
and puts everything
inside of them
*/
//+doc in the RST file.

"""

RUNTEST = False
import sys, os, string


# Remove the specified number of whitespace from the start of the string
# If non-whitespace text is encountered in the 'indent' area, it is preserved
def _RemoveIndent(s, indent):
    firstcharindex = s.find(s.strip())
    startindex = min(firstcharindex, indent)
    return s[startindex:]

def ExtractRst(inputstr, ofile):
    inMultiline = False
    onelineDocTag = '//+doc'
    multilineDocTag = '/*+doc'
    multilineEndTag = '*/'
    multilineStartIndent = 0
    newOutputFile = False
    
    for line in inputstr.splitlines():
        if inMultiline:
            if multilineEndTag in line:
                data = line[:line.index(multilineEndTag)]
                ofile.write(_RemoveIndent(data, multilineStartIndent) + '\n\n')
                inMultiline = False
                multilineStartIndent = 0
            else:
                ofile.write(_RemoveIndent(line, multilineStartIndent) + '\n')
        elif onelineDocTag in line:
            docstring = line.split('//+doc', 1)[1]
            # TODO: Add this part to the CPP extracting script
            if "*New File*" in docstring:
                filename = line.split('*New File*', 1)[1].strip()
                # Make sure that the file name is valid here
                if newOutputFile:
                    ofile.close()
                ofile = open(filename, 'w')
                newOutputFile = True
            ofile.write(docstring.strip() + '\n')
        else:
            if multilineDocTag in line:
                multilineStartIndent = line.index(multilineDocTag)
                inMultiline = True
                # TODO: Handle code after the multiline start tag?

if __name__ == "__main__":
    
    if not RUNTEST:
        
        if len(sys.argv) <= 2:
            sys.stderr.write("\nYou need to specify an input and output file\n\n")
            sys.exit(1)
        
        src = open(sys.argv[1]).read()
        output = open(sys.argv[2], 'w')
        ExtractRst(src, output)
        output.close()
        
    elif RUNTEST:
        #print _RemoveIndent('',4)
        #print _RemoveIndent('sfdfsd',4)
        #print _RemoveIndent('  sfdfsd',4)
        #print _RemoveIndent('      sfdfsd',4)

        testdata = """

#include <...>

void Random C++ Code

//+doc Some one-line doc. Everything on the line after the onelineDocTag gets added.

/*+doc What should this do?
!rst::This is a multi-line rst text

there is a blank line above this
      this is before the final endtag */

class TestClass
    /*+doc What should this do?
    !rst::additional rst indented code
    more indented code
This is left indented
    This is original indent
        This is indented 4 spaces
    */




"""
        ExtractRst(testdata, sys.stdout)


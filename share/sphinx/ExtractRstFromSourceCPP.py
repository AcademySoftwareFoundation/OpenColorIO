#!/usr/bin/python

"""
Small Script to extract reStructuredText from OCIO headers
    - http://sphinx.pocoo.org/rest.html
    - http://docutils.sourceforge.net/docs/ref/rst/restructuredtext.html
"""

# TODO: extract void foo() { blah = 0 }; signatures correctly
# TODO: handle typedef and enums better
# TODO: handle OCIOEXPORT macro better
# TODO: handle thow() funcs better

RUNTEST = False

import re, sys

single_rst_comment = r"//(?P<single_comment>(!cpp:|!rst::).*\n)"
block_rst_comment = r"/\*(?P<block_comment>(!cpp:|!rst::)([^*]*\*+)+?/)"
rst_comment_regex = re.compile(r"(%s)|(%s)" % (single_rst_comment, block_rst_comment), re.MULTILINE)
func_signature_regex = re.compile(r"(?P<sig_name>[^ ]*\(.*\))")

rst_types = ["!rst::", "!cpp:class::", "!cpp:function::", "!cpp:member::",
             "!cpp:type::"]

def getRstType(string):
    for rtype in rst_types:
        if string[0 : len(rtype)] == rtype:
            return rtype[1:]
    return None

def getNextCodeLine(string, rst_type, from_pos):
    
    end = from_pos
    signature = ""
    
    if rst_type == "rst::":
        return signature, end
    
    if rst_type == "cpp:class::":
        
        class_open = False
        
        # first non-blank line that starts with 'class'
        found_signature = False
        
        # loop till the end of the class '};'
        ## skip other open/close '{' '}'
        skip_close = False
        x = end
        while x < len(string):
            if string[x] != '\n' and not found_signature:
                signature += string[x]
            if string[x] == '\n' and not found_signature:
                signature = signature.strip()
                if signature != '':
                    found_signature = True
                    signature = signature.replace("class", "")
                    # TODO: this seem a bit dirty
                    signature = signature.replace("OCIOEXPORT ", "")
                    signature = signature.strip()
                    signature = signature.split(' ', 1)[0]
            if string[x] == '{' and not class_open:
                class_open = True
            elif string[x] == '{' and class_open:
                skip_close = True
            elif string[x] == '}' and skip_close:
                skip_close = False
            elif string[x] == '}':
                end = x
                break
            x += 1
        return signature, end
    
    # else
    skip = False
    while string[end] != ";":
        if string[end] != ' ' and skip:
            skip = False
            signature += ' '
        if string[end] == '\n':
            skip = True
        if not skip:
            signature += string[end]
        end += 1
    signature += string[end]
    # TODO: this seem a bit dirty
    signature = signature.replace("OCIOEXPORT ", "")
    signature = signature.replace(" throw()", "")
    signature = signature.strip()
    if signature[len(signature)-1] == ';':
        signature = signature[:len(signature)-1]
    
    # hack hack hack
    if rst_type == "cpp:type::":
        if signature[:7] == "typedef":
            bits = signature.split()
            signature = bits[len(bits)-1]
        if signature[:4] == "enum":
            bits = signature.split()
            signature = bits[1]
    
    return signature, end

def getNextCommentLine(string, from_pos, buffer = ""):
    end = from_pos
    tmp = ""
    while string[end] != "\n":
        tmp += string[end]
        end += 1
    tmp += string[end]
    if tmp.lstrip()[:2] == "//":
        if tmp.lstrip()[2:][0] == " ":
            buffer += tmp.lstrip()[3:]
        else:
            buffer += tmp.lstrip()[2:]
        buffer, end = getNextCommentLine(string, end+1, buffer)
    else:
        end = from_pos
    return buffer, end

class Comment:
    
    def __init__(self, comment, start, end):
        self.comment = comment
        self.start = start
        self.end = end
    
    def getRstType(self):
        return getRstType(self.comment)
    
    def __str__(self):
        
        buffer = self.comment
        for rtype in rst_types:
            if buffer[0 : len(rtype)] == rtype:
                buffer =  buffer[len(rtype):]
        
        buffer_lines = buffer.splitlines()
        buffer_lines[0] = buffer_lines[0].strip()
        
        if self.getRstType() == "rst::":
            buffer_lines.append('')
            buffer = '\n'.join(buffer_lines)
            return buffer
        
        if buffer_lines[0] != '':
            buffer_lines.insert(0, '')
        for x in range(0, len(buffer_lines)):
            buffer_lines[x] = "   %s" % buffer_lines[x]
        buffer_lines.append('')
        buffer = '\n'.join(buffer_lines)
        
        return buffer

def ExtractRst(string, fileh):
    
    items = []
    
    for item in rst_comment_regex.finditer(string):
        start, end = item.span()
        itemdict = item.groupdict()
        if itemdict["single_comment"] != None:
            ##
            buf = itemdict["single_comment"]
            comment, end = getNextCommentLine(string, end)
            buf += comment
            ##
            items.append(Comment(buf, start, end))
        
        elif itemdict["block_comment"] != None:
            ##
            itemdict["block_comment"] = \
                itemdict["block_comment"][:len(itemdict["block_comment"])-2]
            buf_lines = itemdict["block_comment"].splitlines()
            indent = 0
            if len(buf_lines) > 1:
                for char in buf_lines[1]:
                    if char != ' ':
                        break
                    indent += 1
            # remove indent
            bufa = [buf_lines[0]]
            for x in range(1, len(buf_lines)):
                bufa.append(buf_lines[x][indent:])
            buf = '\n'.join(bufa) + '\n'
            ##
            items.append(Comment(buf, start, end))
            
    ##
    fileh.write('\n')
    namespaces = []
    for thing in items:
        rst_type = thing.getRstType()
        
        # .. cpp:function:: SomeClass::func2(const char * filename, std::istream& foo)
        #    this is some of the documentation
        #    for this function
        
        signature, end = getNextCodeLine(string, rst_type, thing.end)
        
        # if we are a class work out the begining and end so we can
        # give function signatures the correct namespace
        if rst_type == "cpp:class::":
            tmp = { 'name': signature, 'start': thing.end, 'end': end }
            namespaces.append(tmp)
            fileh.write(".. %s %s\n" % (rst_type, signature) )
        elif rst_type != "rst::":
            for namespace in namespaces:
                if end > namespace['start'] and end < namespace['end']:
                    func = func_signature_regex.search(signature)
                    funcpart = str(func.groupdict()["sig_name"])
                    signature = signature.replace(funcpart, "%s::%s" % (namespace['name'], funcpart))
                    break
            fileh.write(".. %s %s\n" % (rst_type, signature) )
        
        fileh.write(str(thing))
        
        fileh.write('\n')
        
    fileh.flush()

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
        testdata = """

//!rst:: -------------

// this comment should be ignored

//!rst:: foobar
// this is apart of the same
// comment

// this is also ignored

/* this is
a block comment which is
ignored */

//!cpp:class::
// this is a comment about the class
class FooBar : public std::exception
{
    ...
};

/*!cpp:class::
this is also a comment about this class
*/
   
	
class FooBar2 : public std::exception
{
    ...
};

/*!cpp:class::
this is also a comment about this class with no new line */
class FooBar3 : public std::exception
{
    ...
};

//!cpp:class::
class SomeClass
{
public:
    
    //!cpp:function::
    // this is some cool function for
    // some purpose
    //    this line is indented
    static fooPtr func1();
    
    /*!cpp:function::
    this is a much better func for some other
    purpose
          this is also indented */
    static barPtr func2();
    
    /*!cpp:function:: this func wraps over two
    lines which needs
    to be caught
    */
    static weePtr func2(const char * filename,
                        std::istream& foo);
};

//!cpp:function:: the class namespace should still get set correctly
void foobar1();

//!cpp:class:: this is some super informative
// docs
class SomeClass
{
public:
    //!cpp:function:: the class namespace should still get set correctly
    void foobar2();
};

//!cpp:function:: the class namespace should still get set correctly
void foobar3();

/*!rst:: this is a rst block
**comment which needs**
to be supported
*/

"""
        ExtractRst(testdata)


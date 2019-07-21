# Security and OpenColorIO

The OpenColorIO Technical Steering Committee (TSC) takes security very
seriously. We strive to design secure software and regularly utilise analysis
tools to help identify potential vulnerabilities.

Users should exercise caution when working with untrusted data (config.ocio
files, LUTs, etc.). Code injection bugs have been found in much simpler data
structures, so it would be foolish to assume OpenColorIO is immune.

## Reporting Vulnerabilities

Quickly resolving security related issues is a priority. If you think
you've found a potential vulnerability in OpenColorIO, please report it by
emailing security@opencolorio.org. Only TSC members and ASWF project
management have access to these messages.

Include detailed steps to reproduce the issue, and any other information that
could aid an investigation. Someone will assess the report and make every
effort to respond within 14 days.

## Outstanding Security Issues

None

## Addressed Security Issues

None

## File Format Expectations

Attempting to read a .ocio file will:
* Return success and produce a valid Config data structure in memory
* Fail with an error

The bulk of data read comes from transform files referenced by a Config. 
These files may be arbitrarily large, and may live on any accessible volume. 
Referenced file paths support environment variable expansion, making OCIO's 
Processor invokation susceptible to environmental redirection. Files are only 
read when needed for finalizing a Processor, so no transform files should be 
accessed as a result of loading a Config.

It is a bug if some file causes the library to crash.  It is a serious
security issue if some file causes arbitrary code execution.

OpenColorIO will attempt to associate a file's data and layout with a 
registered file format, regardless of a file's extension. Invalid input data
results in an error.

## Runtime Library Expectations

We consider the library to run with the same privilege as the linked
code.  As such, we do not guarantee any safety against malformed
arguments.   Provided functions are called with well-formed
parameters, we expect the same set of behaviors as with file
loading.

It is a bug if calling a function with well-formed arguments causes
the library to crash.  It is a security issue if calling a function
with well-formed arguments causes arbitrary code execution.

We do not consider this as severe as file format issues because
in most deployments the parameter space is not exposed to potential
attackers.

## Proper Data Redaction

A common concern when working with sensitive data is to ensure
that distributed files are clean and do not possess any hidden
data.   There are a few surprising ways in which OpenVDB can
maintain data that appears erased.

The best practice for building a clean VDB is populate an
empty grid voxel-by-voxel with the desired data and only
copy known and trusted metadata fields.

## Metadata

VDBs will try to preserve metadata through most operations.  This can
provide an unexpected sidechannel for communication.

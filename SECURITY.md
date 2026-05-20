<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright Contributors to the OpenColorIO Project. -->

# Security and OpenColorIO

The OpenColorIO Technical Steering Committee (TSC) takes security very
seriously. We strive to design secure software, and utilize continuous 
integration and code analysis tools to help identify potential 
vulnerabilities.

Users should exercise caution when working with untrusted data (config files, 
LUTs, etc.). OCIO takes every precaution to read only valid data, but it 
would be naive to say our code is immune to every exploit.

## Reporting Vulnerabilities

Quickly resolving security related issues is a priority. The best way to report a 
vulnerability is to file a GitHub security advisory. If that is not possible, it 
is also fine to email your report to security@opencolorio.org. Only the project 
administrators have access to these reports.

Include detailed steps to reproduce the issue, and any other information that
could aid an investigation. Someone will assess the report and make every
effort to respond within 14 days.

## File Format Expectations

Attempting to read an OCIO config (YAML) file will:
* Return success and produce a valid Config data structure in memory
* Fail with an error

The vast majority of data used by OCIO comes from user-supplied transform 
files referenced in a config. These files are arbitrarily large, may live 
on any accessible volume, and are loaded lazily when requested by a Processor. 
Referenced file paths may contain environment variables for expansion at load 
time. Config authors should consider the implications of this when defining 
file and search paths, to avoid the possibility of a maliciously modified 
environment redirecting file reads to an insecure location.

It is a bug if some file causes the library to crash. It is a serious
security issue if some file causes arbitrary code execution.

OpenColorIO will attempt to associate a file's data and layout with a 
registered file format, regardless of its extension. Only expected data 
structures will be read for each format. It is a bug if reading seriously 
invalid or malformed data from a file does not result in an error from the 
file format reader ultimately used with a file.

## Runtime Library Expectations

We consider the library to run with the same privilege as the linked
code. As such, we do not guarantee any safety against malformed arguments. 
Provided functions are called with well-formed parameters, we expect the same 
set of behaviors as with file loading.

It is a bug if calling a function with well-formed arguments causes the 
library to crash. It is a security issue if calling a function with 
well-formed arguments causes arbitrary code execution.

## History of CVE Fixes

CVE-2026-42450 -- Stack buffer overflow in sscanf. (Fixed in OCIO 2.5.2)

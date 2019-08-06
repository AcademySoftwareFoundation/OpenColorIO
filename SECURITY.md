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

We do not consider this as severe as file format issues because in most 
deployments the parameter space is not exposed to potential attackers.

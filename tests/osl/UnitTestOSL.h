// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef OPENCOLORIO_OSL_UNITTEST_OSL_H
#define OPENCOLORIO_OSL_UNITTEST_OSL_H


#include "UnitTestTypes.h"

#include <OpenImageIO/errorhandler.h>

#include <string>


// Trap the OSL messages in case of error.
class ErrorRecorder final : public OIIO::ErrorHandler
{
public:
    ErrorRecorder() : ErrorHandler() {}
 
    virtual void operator()(int errcode, const std::string & msg)
    {
        if (errcode >= EH_ERROR)
        {
            if (m_errormessage.size()
                && m_errormessage[m_errormessage.length() - 1] != '\n')
            {
                m_errormessage += '\n';
            }
            m_errormessage += msg;
        }
    }

    inline bool haserror() const { return m_errormessage.size(); }
 
    std::string geterror(bool erase = true)
    {
        std::string s;
        if (erase)
        {
            std::swap(s, m_errormessage);
        }
        else
        {
            s = m_errormessage;
        }
        return s;
    }

private:
    std::string m_errormessage;
};

// Excecute in-memory the OSL shader.
void ExecuteOSLShader(const std::string & shaderName,
                      const Image & inValues,
                      const Image & outValues,
                      float threshold,
                      float minValue,
                      bool relativecomparison);


#endif /* OPENCOLORIO_OSL_UNITTEST_OSL_H */

// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_MEASURE_H
#define INCLUDED_OCIO_MEASURE_H


#include <chrono>
#include <string>
#include <stdexcept>


// Utility to measure time in ms.
class Measure
{
public:
    Measure() = delete;
    Measure(const Measure &) = delete;

    explicit Measure(const char * explanation)
        :   m_explanations(explanation)
    {
    }

    explicit Measure(const char * explanation, unsigned iterations)
        :   m_explanations(explanation)
        ,   m_iterations(iterations)
    {
    }

    ~Measure()
    {
        if(m_started)
        {
            pause();
        }
        print();
    }

    void resume()
    {
        if(m_started)
        {
            throw std::runtime_error("Measure already started.");
        }

        m_started = true;
        m_start = std::chrono::high_resolution_clock::now();
    }

    void pause()
    {
        std::chrono::high_resolution_clock::time_point end
           = std::chrono::high_resolution_clock::now();

        if(m_started)
        {
            const std::chrono::duration<float, std::milli> duration = end - m_start;

            m_duration += duration;
        }
        else
        {
            throw std::runtime_error("Measure already stopped.");
        }

        m_started = false;
    }
    
    void print() const noexcept
    {
        std::cout << "\n"
                  << m_explanations << "\n"
                  << "  Processing took: "
                  << (m_duration.count() / float(m_iterations))
                  <<  " ms" << std::endl;
    }

private:
    const std::string m_explanations;
    const unsigned m_iterations = 1;

    bool m_started = false;
    std::chrono::high_resolution_clock::time_point m_start;

    std::chrono::duration<float, std::milli> m_duration { 0 };
};


#endif // INCLUDED_OCIO_MEASURE_H

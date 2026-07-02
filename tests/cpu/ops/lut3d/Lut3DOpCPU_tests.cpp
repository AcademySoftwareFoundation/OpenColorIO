// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <limits>

#include "ops/lut3d/Lut3DOpCPU.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


void Lut3DRendererNaNTest(OCIO::Interpolation interpol)
{
    OCIO::Lut3DOpDataRcPtr lut = std::make_shared<OCIO::Lut3DOpData>(interpol, 4);

    float * values = &lut->getArray().getValues()[0];
    // Change LUT so that it is not identity.
    values[65] += 0.001f;

    OCIO::ConstLut3DOpDataRcPtr lutConst = lut;
    OCIO::ConstOpCPURcPtr renderer = OCIO::GetLut3DRenderer(lutConst);

    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float pixels[16] = { qnan, qnan, qnan, 0.5f,
                         0.5f, 0.3f, 0.2f, qnan,
                          inf,  inf,  inf,  inf,
                         -inf, -inf, -inf, -inf };

    renderer->apply(pixels, pixels, 4);

    OCIO_CHECK_CLOSE(pixels[0], values[0], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[1], values[1], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[2], values[2], 1e-7f);
    OCIO_CHECK_ASSERT(OCIO::IsNan(pixels[7]));
    OCIO_CHECK_CLOSE(pixels[8], 1.0f, 1e-7f);
    OCIO_CHECK_CLOSE(pixels[9], 1.0f, 1e-7f);
    OCIO_CHECK_CLOSE(pixels[10], 1.0f, 1e-7f);
    OCIO_CHECK_EQUAL(pixels[11], inf);
    OCIO_CHECK_CLOSE(pixels[12], 0.0f, 1e-7f);
    OCIO_CHECK_CLOSE(pixels[13], 0.0f, 1e-7f);
    OCIO_CHECK_CLOSE(pixels[14], 0.0f, 1e-7f);
    OCIO_CHECK_EQUAL(pixels[15], -inf);
}

OCIO_ADD_TEST(Lut3DRenderer, nan_linear_test)
{
    Lut3DRendererNaNTest(OCIO::INTERP_LINEAR);
}

OCIO_ADD_TEST(Lut3DRenderer, nan_tetra_test)
{
    Lut3DRendererNaNTest(OCIO::INTERP_TETRAHEDRAL);
}

#if OCIO_USE_AVX512 && defined(_WIN32)

#include "ops/lut3d/Lut3DOpCPU_AVX512.h"
#include <cstring>
#include <windows.h>

namespace
{
// Places a buffer so its last byte is immediately followed by a PAGE_NOACCESS page, so
// any read or write past the requested size raises an access violation right away
// instead of only sometimes, depending on heap layout.
class GuardedBuffer
{
public:
    explicit GuardedBuffer(size_t numFloats)
    {
        const size_t sizeBytes = numFloats * sizeof(float);
        const size_t pageSize = 4096;
        const size_t dataPages = (sizeBytes + pageSize - 1) / pageSize;

        m_base = static_cast<uint8_t *>(VirtualAlloc(nullptr, (dataPages + 1) * pageSize,
                                                       MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        OCIO_REQUIRE_ASSERT(m_base);

        DWORD oldProtect = 0;
        const BOOL ok = VirtualProtect(m_base + dataPages * pageSize, pageSize,
                                        PAGE_NOACCESS, &oldProtect);
        OCIO_REQUIRE_ASSERT(ok);

        m_ptr = reinterpret_cast<float *>(m_base + dataPages * pageSize - sizeBytes);
        memset(m_ptr, 0, sizeBytes);
    }

    ~GuardedBuffer()
    {
        VirtualFree(m_base, 0, MEM_RELEASE);
    }

    GuardedBuffer(const GuardedBuffer &) = delete;
    GuardedBuffer & operator=(const GuardedBuffer &) = delete;

    float * get() { return m_ptr; }

private:
    uint8_t * m_base = nullptr;
    float * m_ptr = nullptr;
};
} // anonymous namespace

// Diagnostic test for the "illegal instruction" crash seen in
// OpOptimizers/invlut_pair_identities on the windows-2025-vs2026 CI runner (not
// reproduced on other Windows configurations so far). Exercises the AVX512 tetrahedral
// LUT3D interpolation directly, sweeping pixel counts across the 16-lane boundary (to
// separate the masked "remainder" path from the main loop) and both a tiny grid (2,
// matching tests/data/files/lut_inv_pairs.ctf) and a normal-sized one (32), with every
// buffer (LUT, source, destination) placed against a guard page. Any out-of-bounds
// access, regardless of mechanism, will raise a clean, immediate access violation
// naming the exact (dim, numPixels) combination instead of an unexplained crash.
OCIO_ADD_TEST(Lut3DRenderer, avx512_tetrahedral_bounds)
{
    if (!OCIO::CPUInfo::instance().hasAVX512()) throw SkipException();

    for (int dim : { 2, 32 })
    {
        const int lutFloats = dim * dim * dim * 4;
        GuardedBuffer lut(lutFloats);
        float * lutPtr = lut.get();
        for (int i = 0; i < lutFloats; ++i)
        {
            lutPtr[i] = static_cast<float>(i % 7) / 7.0f;
        }

        for (int numPixels = 1; numPixels <= 33; ++numPixels)
        {
            std::cerr << "avx512_tetrahedral_bounds: dim=" << dim
                       << " numPixels=" << numPixels << std::endl;

            GuardedBuffer src(numPixels * 4);
            GuardedBuffer dst(numPixels * 4);
            float * srcPtr = src.get();
            for (int i = 0; i < numPixels * 4; ++i)
            {
                srcPtr[i] = static_cast<float>(i % 5) / 5.0f;
            }

            OCIO::applyTetrahedralAVX512(lut.get(), dim, src.get(), dst.get(), numPixels);
        }
    }
}

#endif // OCIO_USE_AVX512 && defined(_WIN32)


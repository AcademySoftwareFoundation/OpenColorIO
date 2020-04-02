// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYIMAGEDESC_H
#define INCLUDED_OCIO_PYIMAGEDESC_H

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

using PackedImageDescRcPtr = OCIO_SHARED_PTR<PackedImageDesc>;
using PlanarImageDescRcPtr = OCIO_SHARED_PTR<PlanarImageDesc>;

// ImageDesc does NOT claim ownership of its pixels or copy image data. This is problematic in 
// Python when image data passed to the constructor only exists in the function call's scope. 
// The py::buffer is garbage collected immediately following initialization, leaving a dangling 
// pointer in the ImageDesc. Consider this scenario:
//
//   >>> img1 = OCIO.PackedImageDesc(np.array(0.18, ...), ...)
//   >>> img1.getData()
//   np.array(0.18, ...)
//   >>> img2 = OCIO.PackedImageDesc(np.array(0.05, ...), ...)
//   >>> img2.getData()
//   np.array(0.05, ...)
//   >>> img1.getData()
//   np.array(0.05, ...)  # img1 and img2 are pointing to the same memory
//
// To get around this, ImageDesc and its subclasses are wrapped in a container struct which owns 
// persistent copies of the initializing py::buffer object(s).

struct OCIOHIDDEN PyImageDesc
{
    PyImageDesc() = default;
    virtual ~PyImageDesc() = default;

    virtual BitDepth getBitDepth() const noexcept { return m_img->getBitDepth(); }
    long getWidth() const noexcept { return m_img->getWidth(); }
    long getHeight() const noexcept { return m_img->getHeight(); }
    ptrdiff_t getXStrideBytes() const noexcept { return m_img->getXStrideBytes(); }
    ptrdiff_t getYStrideBytes() const noexcept { return m_img->getYStrideBytes(); }
    bool isRGBAPacked() const noexcept { return m_img->isRGBAPacked(); }
    bool isFloat() const noexcept { return m_img->isFloat(); }

    ImageDescRcPtr m_img;

private:
    PyImageDesc(const PyImageDesc &) = delete;
    PyImageDesc & operator= (const PyImageDesc &) = delete;
};

template<typename T, int N>
struct OCIOHIDDEN PyImageDescImpl : public PyImageDesc
{
    PyImageDescImpl() = default;
    virtual ~PyImageDescImpl() = default;

    OCIO_SHARED_PTR<T> getImg() const { return OCIO_DYNAMIC_POINTER_CAST<T>(m_img); }

    py::buffer m_data[N];

private:
    PyImageDescImpl(const PyImageDescImpl &) = delete;
    PyImageDescImpl & operator= (const PyImageDescImpl &) = delete;
};

using PyPackedImageDesc = PyImageDescImpl<PackedImageDesc, 1>;
using PyPlanarImageDesc = PyImageDescImpl<PlanarImageDesc, 4>;

void bindPyPackedImageDesc(py::module & m);
void bindPyPlanarImageDesc(py::module & m);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PYIMAGEDESC_H

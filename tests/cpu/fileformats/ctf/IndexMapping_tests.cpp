// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/ctf/IndexMapping.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(IndexMapping, test_accessors)
{
    OCIO::IndexMapping ref(4);
    ref.setPair(0u, 0.f, 0.f);
    ref.setPair(1u, 100.f, 1.f);
    ref.setPair(2u, 200.f, 2.f);
    ref.setPair(3u, 300.f, 3.f);

    OCIO_CHECK_THROW_WHAT(ref.setPair(5u, 300.f, 3.f),
                          OCIO::Exception,
                          "invalid. Should be less than");

    ref.validate();

    OCIO_CHECK_EQUAL(ref.getDimension(), 4u);
    float first, second;
    ref.getPair(0u, first, second);
    OCIO_CHECK_EQUAL(first, 0.f);  OCIO_CHECK_EQUAL(second, 0.f);
    ref.getPair(1u, first, second);
    OCIO_CHECK_EQUAL(first, 100.f);  OCIO_CHECK_EQUAL(second, 1.f);
    ref.getPair(2u, first, second);
    OCIO_CHECK_EQUAL(first, 200.f);  OCIO_CHECK_EQUAL(second, 2.f);
    ref.getPair(3u, first, second);
    OCIO_CHECK_EQUAL(first, 300.f);  OCIO_CHECK_EQUAL(second, 3.f);

    ref.resize(8);
    OCIO_CHECK_EQUAL(ref.getDimension(), 8u);

}

OCIO_ADD_TEST(IndexMapping, range_validation)
{
    OCIO::IndexMapping ref(4);
    ref.setPair(0u, 0.f, 0.f);
    ref.setPair(1u, 100.f, 1.f);
    ref.setPair(2u, 200.f, 2.f);
    ref.setPair(3u, 200.f, 3.f);  // illegal

    OCIO_CHECK_THROW_WHAT(ref.validate(), 
                          OCIO::Exception, 
                          "Index values must be increasing");

    ref.setPair(3u, 300.f, 2.f);  // illegal
    OCIO_CHECK_THROW_WHAT(ref.validate(), 
                          OCIO::Exception, 
                          "Index values must be increasing");
}

OCIO_ADD_TEST(IndexMapping, equality)
{
    OCIO::IndexMapping ref1(4);
    ref1.setPair(0u, 0.f, 0.f);
    ref1.setPair(1u, 100.f, 1.f);
    ref1.setPair(2u, 200.f, 2.f);
    ref1.setPair(3u, 300.f, 3.f);

    OCIO::IndexMapping ref2(4);
    ref2.setPair(0u, 0.f, 0.f);
    ref2.setPair(1u, 100.f, 1.f);
    ref2.setPair(2u, 200.f, 2.f);
    ref2.setPair(3u, 300.f, 3.f);

    OCIO_CHECK_ASSERT(ref1 == ref2);

    ref2.setPair(2u, 200.f, 2.1f);

    OCIO_CHECK_ASSERT(!(ref1 == ref2));
}


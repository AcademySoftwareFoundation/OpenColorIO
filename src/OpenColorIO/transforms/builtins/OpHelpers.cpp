// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "OpenEXR/half.h"
#include "ops/lut1d/Lut1DOp.h"
#include "transforms/builtins/OpHelpers.h"


namespace OCIO_NAMESPACE
{

double Interpolate1D(unsigned lutSize, const double * lutValues, double in)
{
    // The LUT values are organized that way:
    // { in[0], out[0],
    //   in[1], out[1],
    //   ...
    //   in[N], out[N] }
    //
    // where N is lutSize, and
    // in[n] is lutValues[n * 2] & out[n] is lutValues[n * 2 + 1].

    // Clamp if values are outside the domain of the LUT.
    if (in < lutValues[0])
    {
        return lutValues[1];
    }
    else if (in >= lutValues[2 * (lutSize-1)])
    {
        return lutValues[2 * (lutSize-1) + 1];
    }

    for (unsigned idx = 1; idx < lutSize; ++idx)
    {
        if (in < lutValues[2 * idx])
        {
            const unsigned minIdx = 2 * (idx - 1);
            const unsigned maxIdx = 2 * idx;

            const double inCoeff
                = (in - lutValues[minIdx]) / (lutValues[maxIdx] - lutValues[minIdx]);

            return lutValues[minIdx + 1] * (1.0 - inCoeff) 
                    + lutValues[maxIdx + 1] * inCoeff;
        }
    }

    throw Exception("Invalid interpolation value.");
}

void CreateLut(OpRcPtrVec & ops, 
               unsigned long lutDimension,
               std::function<float(double)> lutValueGenerator)
{
    Lut1DOpDataRcPtr lut = std::make_shared<Lut1DOpData>(Lut1DOpData::LUT_STANDARD,
                                                         lutDimension, false);
    lut->setInterpolation(INTERP_LINEAR);
    lut->setDirection(TRANSFORM_DIR_FORWARD);

    Array & array = lut->getArray();
    Array::Values & values = array.getValues();

    for (unsigned long idx = 0; idx < lutDimension; ++idx)
    {
        values[idx * 3 + 0] = lutValueGenerator(double(idx) / (lutDimension - 1.));
        values[idx * 3 + 1] = lutValueGenerator(double(idx) / (lutDimension - 1.));
        values[idx * 3 + 2] = lutValueGenerator(double(idx) / (lutDimension - 1.));
    }

    CreateLut1DOp(ops, lut, TRANSFORM_DIR_FORWARD);
}

void CreateLut(OpRcPtrVec & ops,
               unsigned long lutDimension,
               std::function<void(const double *, double *)> lutValueGenerator)
{
    Lut1DOpDataRcPtr lut = std::make_shared<Lut1DOpData>(lutDimension);
    lut->setInterpolation(INTERP_LINEAR);
    lut->setDirection(TRANSFORM_DIR_FORWARD);

    Array::Values & values = lut->getArray().getValues();

    for (unsigned long idx = 0; idx < lutDimension; ++idx)
    {
        const double in[3]
        {
            double(idx) / (lutDimension - 1.),
            double(idx) / (lutDimension - 1.),
            double(idx) / (lutDimension - 1.)
        };
    
        double out[3] {0., 0., 0.};

        lutValueGenerator(in, out);

        values[3 * idx + 0] = float(out[0]);
        values[3 * idx + 1] = float(out[1]);
        values[3 * idx + 2] = float(out[2]);
    }

    CreateLut1DOp(ops, lut, TRANSFORM_DIR_FORWARD);
}

void CreateHalfLut(OpRcPtrVec & ops, std::function<float(double)> lutValueGenerator)
{
    Lut1DOpDataRcPtr lut = std::make_shared<Lut1DOpData>(Lut1DOpData::LUT_INPUT_HALF_CODE,
                                                         65536, true);
    lut->setInterpolation(INTERP_LINEAR);
    lut->setDirection(TRANSFORM_DIR_FORWARD);

    Array & array = lut->getArray();
    Array::Values & values = array.getValues();

    const unsigned long lutDimension = array.getLength();
    for (unsigned long idx = 0; idx < lutDimension; ++idx)
    {
        half halfValue;
        halfValue.setBits((unsigned short)idx);

        double value = static_cast<float>(halfValue);

        if (halfValue.isNan())
        {
            value = 0.;
        }
        else if (halfValue.isInfinity())
        {
            value = halfValue.isNegative() ? -HALF_MAX : HALF_MAX;
        }

        values[idx * 3 + 0] = lutValueGenerator(value);
        values[idx * 3 + 1] = lutValueGenerator(value);
        values[idx * 3 + 2] = lutValueGenerator(value);
    }

    CreateLut1DOp(ops, lut, TRANSFORM_DIR_FORWARD);
}

} // namespace OCIO_NAMESPACE

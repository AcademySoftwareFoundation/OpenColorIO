// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_COLORSPACE_HELPERS_H
#define INCLUDED_OCIO_COLORSPACE_HELPERS_H


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

// List of strings used by family and description values from a color space.

class Strings;
using StringsRcPtr = OCIO_SHARED_PTR<Strings>;
using ConstStringsRcPtr = OCIO_SHARED_PTR<const Strings>;

class Strings
{
public:
    static StringsRcPtr Create();

    virtual size_t getNumString() const noexcept = 0;
    virtual bool empty() const noexcept = 0;

    virtual const char * getString(size_t idx) const = 0;
    virtual void appendString(const char * val) = 0;

protected:
    Strings() = default;
    virtual ~Strings() = default;

private:
    Strings(const Strings &) = delete;
    Strings & operator=(const Strings &) = delete;
};


// Some information from a color space.

class ColorSpaceInfo;
using ColorSpaceInfoRcPtr = OCIO_SHARED_PTR<ColorSpaceInfo>;
using ConstColorSpaceInfoRcPtr = OCIO_SHARED_PTR<const ColorSpaceInfo>;

class ColorSpaceInfo
{
public:
    static ConstColorSpaceInfoRcPtr Create(ConstConfigRcPtr config,
                                           const char * name,
                                           const char * family,
                                           const char * description);

    static ConstColorSpaceInfoRcPtr Create(ConstConfigRcPtr config,
                                           const char * name,
                                           const char * uiName,
                                           const char * family,
                                           const char * description);

    static ConstColorSpaceInfoRcPtr Create(ConstConfigRcPtr config,
                                           ConstColorSpaceRcPtr cs);

    virtual const char * getName() const noexcept = 0;
    virtual const char * getUIName() const noexcept = 0;
    virtual const char * getFamily() const noexcept = 0;
    virtual const char * getDescription() const noexcept = 0;

    // The family split into levels using the 'family separator'.
    virtual ConstStringsRcPtr getHierarchyLevels() const noexcept = 0;
    // The description split into lines.
    virtual ConstStringsRcPtr getDescriptions() const noexcept = 0;

protected:
    ColorSpaceInfo() = default;
    virtual ~ColorSpaceInfo() = default;

private:
    ColorSpaceInfo(const ColorSpaceInfo &) = delete;
    ColorSpaceInfo & operator=(const ColorSpaceInfo &) = delete;
};


//
// The following class provides a convenient access to active color
// spaces selected by a role or categories attached to color spaces.
//

class ColorSpaceMenuHelper;
using ColorSpaceMenuHelperRcPtr = OCIO_SHARED_PTR<ColorSpaceMenuHelper>;


class ColorSpaceMenuHelper
{
public:
    static ColorSpaceMenuHelperRcPtr Create(const ConstConfigRcPtr & config,
                                            const char * role,
                                            const char * categories);

    // Access to the color space names.
    virtual size_t getNumColorSpaces() const noexcept = 0;
    virtual const char * getColorSpaceName(size_t idx) const = 0;
    virtual const char * getColorSpaceUIName(size_t idx) const = 0;
    virtual ConstColorSpaceInfoRcPtr getColorSpace(size_t idx) const = 0;

    // Get the color space name from the UI name.
    virtual const char * getNameFromUIName(const char * uiName) const = 0;
    // Get the color space UI name from the name.
    virtual const char * getUINameFromName(const char * name) const = 0;

    // Add an additional color space to the menu.
    //
    // Note that an additional color space could be:
    // * an inactive color space,
    // * an active color space not having at least one of the selected categories,
    // * a newly created color space.
    virtual void addColorSpaceToMenu(ConstColorSpaceInfoRcPtr & cs) = 0;

    // Refresh the instance (i.e. needed following a configuration change for example).
    virtual void refresh(ConstConfigRcPtr config) = 0;

protected:
    ColorSpaceMenuHelper() = default;
    virtual ~ColorSpaceMenuHelper() = default;

private:
    ColorSpaceMenuHelper(const ColorSpaceMenuHelper &) = delete;
    ColorSpaceMenuHelper & operator=(const ColorSpaceMenuHelper &) = delete;
};


namespace ColorSpaceHelpers
{

// Get the processor using role names or color space names or UI color space names.
ConstProcessorRcPtr GetProcessor(const ConstConfigRcPtr & config,
                                 const char * inputColorSpaceName,
                                 const char * outputColorSpaceName);

// Add a new color space to the shared config instance. The output of the userTransform must 
// be in the specified connectionColorSpace.
//
// Note: If the config does not already use categories, we do not add them since that would 
// make a big change to how existing color spaces show up in menus.
void AddColorSpace(ConfigRcPtr & config,
                   const ColorSpaceInfo & colorSpaceInfo,
                   FileTransformRcPtr & userTransform,
                   const char * categories, // Could be null or empty.
                   const char * connectionColorSpaceName);

} // ColorSpaceHelpers


}  // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_COLORSPACE_HELPERS_H

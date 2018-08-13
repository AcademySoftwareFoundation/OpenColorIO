/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef INCLUDED_OCIO_OPDATACDL_H
#define INCLUDED_OCIO_OPDATACDL_H

#include <OpenColorIO/OpenColorIO.h>

#include "OpDataVec.h"


OCIO_NAMESPACE_ENTER
{
    // Private namespace to the OpData sub-directory
    namespace OpData
    {
      
        class CDL;
        typedef OCIO_SHARED_PTR<CDL> OpDataCDLRcPtr;


        class CDL : public OpData
        {
        public:

            // Enumeration of the CDL styles
            enum CDLStyle
            {
                CDL_V1_2_FWD = 0,  // Forward (version 1.2) style
                CDL_V1_2_REV,      // Reverse (version 1.2) style
                CDL_NO_CLAMP_FWD,  // Forward no clamping style
                CDL_NO_CLAMP_REV   // Reverse no clamping style
            };

            // Get the style to be used when reading ASC XML files 
            // that don't have a style attribute
            // Returns the style enumeration
            static inline CDLStyle GetDefaultStyle() { return CDL_V1_2_FWD; }

            // TODO: Rename the style getters/setters to getStyle/setStyle.
            // Should also rename on ExposureConstrast, Gamma, and Log operators.

            // TODO: Operations should deal only with the style enumerations. 
            // String representations should be handled at the XML component (for tags) 
            // or at the synColor component (for human-readable values).

            // Get CDLStyle from its name
            // name is the string representation of the style name
            // Returns the enumeration value.
            static CDLStyle GetCDLStyle(const char * name);

            // Get the string name for the enumeration
            // cdlStyle is the enum
            // Returns the string representation
            static const char * GetCDLStyleName(CDLStyle cdlStyle);

            // Type definition to hold the values of a SOP paramater
            // (scale, offset and power) for all channels
            struct ChannelParams
            {
                // Complete constructor for the red, green, blue and alpha channels
                ChannelParams(const double r, const double g, const double b, const double a)
                {
                    setRGBA(r, g, b, a);
                }

                // Constructor for the red, green and blue channels.
                // Set alpha channel to 1.0.
                ChannelParams(const double r, const double g, const double b)
                {
                    setRGBA(r, g, b, 1.0);
                }

                // Constructor that uses the same value for the red, green and blue channels.
                ChannelParams(const double x)
                {
                    setRGBA(x, x, x, 1.0);
                }

                // Set red, green and blue to 0.0, and alpha channel to 1.0.
                ChannelParams()
                {
                    setRGBA(0.0, 0.0, 0.0, 1.0);
                }

                // Returns the pointer to the array of doubles representing the RGBA channels
                const double * data() const { return m_data; }
                double * data() { return m_data; }

                // Set the red, green, blue and alpha channels
                // - r red component
                // - g green component
                // - b blue component
                // - a alpha component
                void setRGBA(const double r, const double g, const double b, const double a)
                {
                    setRGB(r, g, b);
                    setAlpha(a);
                }

                // Set the red, green, and blue channels. Alpha channel remains unchanged.
                // - r red component
                // - g green component
                // - b blue component
                void setRGB(const double r, const double g, const double b)
                {
                    m_data[0] = r;
                    m_data[1] = g;
                    m_data[2] = b;
                }

                // Set the alpha channel. Red, green, and blue channels remain unchanged.
                // a alpha component
                void setAlpha(const double a)
                {
                    m_data[3] = a;
                }

                // Copy the content of the red, green, and blue channels
                //   into the array of floats.
                void getRGB(float * rgb) const
                {
                    rgb[0] = (float)m_data[0];
                    rgb[1] = (float)m_data[1];
                    rgb[2] = (float)m_data[2];
                }

                // Copy the content of the red, green, blue, and alpha channels 
                //   into the array of floats.
                void getRGBA(float * rgba) const
                {
                    getRGB(rgba);
                    rgba[3] = (float)m_data[3];
                }

                // Access operator
                // - index representing the channel to access
                // Returns Channel value
                double operator[](const unsigned index) const
                {
                    if(index>=4)
                    {
                        throw Exception("Index is out of range");
                    }
                    return m_data[index];
                }

                // Access operator
                // - index representing the channel to access
                // Returns Channel value
                double operator[](const unsigned index)
                {
                    if(index>=4)
                    {
                        throw Exception("Index is out of range");
                    }
                    return m_data[index];
                }

                // Equality operator
                // - other right-hand side ChannelParam instance being compared for equality
                // Returns true if all channels have the exact same value, false otherwise.
                bool operator==(const ChannelParams& other) const
                {
                    return 
                        m_data[0] == other.m_data[0] &&
                        m_data[1] == other.m_data[1] &&
                        m_data[2] == other.m_data[2] &&
                        m_data[3] == other.m_data[3];
                }

                // Inequality operator
                // - other right-hand side ChannelParam instance being compared for inequality
                // Returns true if any of the channels do not have the exact same value, 
                // false otherwise.
                bool operator!=(const ChannelParams& other) const
                {
                    return !(*this == other);
                }

            private:
                double m_data[4];
            };


            CDL();

            CDL(BitDepth inBitDepth,                // Requested bit depth for the input
                BitDepth outBitDepth,               // Requested bit depth for the output
                const std::string& id,              // Identifier
                const std::string& name,            // Human readable identifier
                const Descriptions& descriptions,   // Human readable descriptions
                const CDLStyle& style,              // Style
                const ChannelParams& slopeParams,   // Parameters for red channel
                const ChannelParams& offsetParams,  // Parameters for green channel
                const ChannelParams& powerParams,   // Parameters for blue channel
                double saturation                   // Saturation
            );

            CDL(BitDepth inBitDepth,                // Requested bit depth for the input
                BitDepth outBitDepth,               // Requested bit depth for the output
                const CDLStyle& style,              // Style
                const ChannelParams& slopeParams,   // Parameters for red channel
                const ChannelParams& offsetParams,  // Parameters for green channel
                const ChannelParams& powerParams,   // Parameters for blue channel
                double saturation                   // Saturation
            );

            virtual ~CDL();

            // Check if a CDL is equal to this instance.
            // - other is the other CDL to compare with.
            // Return true if they are equal, false otherwise
            bool operator==(const OpData& other) const;

            // Get the CDL style being used.
            inline CDLStyle getCDLStyle() const { return m_cdlStyle; }

            // Get the operation type
            OpType getOpType() const { return OpData::CDLType; }

            // Get the operation type name
            const std::string& getOpTypeName() const;

            // Get the slope parameters
            const ChannelParams& getSlopeParams() const { return m_slopeParams; }

            // Get the offset parameters
            const ChannelParams& getOffsetParams() const { return m_offsetParams; }

            // Get the power parameters
            const ChannelParams& getPowerParams() const { return m_powerParams; }

            // Get the saturation parameter
            double getSaturation() const { return m_saturation; }

            // Checks if the style is in reverse direction
            bool isReverse() const;

            // Set the CDL style to be used
            // Note: For use in XML Reader, do not use on an existing object.
            void setCDLStyle(const CDLStyle& cdlStyle);

            // Set the slope parameters
            // Note: for use in XML Reader, must call validate() afterwards.
            // parameter vector for red
            void setSlopeParams(const ChannelParams& slopeParams);

            // Set the offset parameters
            // Note: for use in XML Reader, must call validate() afterwards.
            // parameter vector for green
            void setOffsetParams(const ChannelParams& offsetParams);

            // Set the power parameters
            // Note: for use in XML Reader, must call validate() afterwards.
            // parameter vector for blue
            void setPowerParams(const ChannelParams& powerParams);

            // Set the saturation parameter
            // Note: for use in XML Reader, must call validate() afterwards.
            void setSaturation(const double saturation);

            // True if the op is an identity
            virtual bool isIdentity() const;

            // Checks if the style is clamping
            virtual bool isClamping() const;

            // Get an identity replacement
            virtual std::auto_ptr<OpData> getIdentityReplacement() const;

            // Is channel independent ?
            virtual bool hasChannelCrosstalk() const;

            // Validate the state of the instance
            virtual void validate() const;

            // Get a string with the slope parameters
            std::string getSlopeString() const;

            // Get a string with the offset parameters
            std::string getOffsetString() const;

            // Get a string with the power parameters
            std::string getPowerString() const;

            // Get a string with the saturation parameter
            std::string getSaturationString() const;

            // Clone the OpData
            virtual OpData * clone(CloneType type) const;

            // Get the inverse of the OpData
            virtual void inverse(OpDataVec & v) const;

            // Is the inverse op ?
            bool isInverse(const OpDataCDLRcPtr & r) const;

            // Compose the two ops
            OpDataCDLRcPtr compose(const OpDataCDLRcPtr & r) const;

        protected:
            static std::string GetChannelParametersString(ChannelParams params);

        private:
            CDLStyle      m_cdlStyle;      // CDL style
            ChannelParams m_slopeParams;   // Slope parameters for RGB channels
            ChannelParams m_offsetParams;  // Offset parameters for RGB channels
            ChannelParams m_powerParams;   // Power parameters for RGB channels
            double        m_saturation;    // Saturation parameter
        };
    } // exit OpData namespace
}
OCIO_NAMESPACE_EXIT

#endif

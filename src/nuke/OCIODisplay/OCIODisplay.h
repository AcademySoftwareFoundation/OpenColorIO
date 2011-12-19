#ifndef INCLUDED_OCIO_NUKE_DISPLAY_H_
#define INCLUDED_OCIO_NUKE_DISPLAY_H_

// Include these early, for Nuke's headers under gcc 4.4.2.
#include <memory>
#include <cstdarg>

#include <DDImage/PixelIop.h>
#include <DDImage/Row.h>
#include <DDImage/Knob.h>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;


/*!
 * Use OpenColorIO to convert for display output.
 */
class OCIODisplay : public DD::Image::PixelIop {
    protected:

        bool m_hasLists; //!< Were colorspaces, OCIODisplay devices, and transform names found? If not, always error.
        DD::Image::ChannelSet m_layersToProcess; //!< layers (rgb channel groups) to process
        int m_colorSpaceIndex; //!< index of colorspace selection from the pulldown list knob
        int m_displayIndex, m_viewIndex;
        std::vector<std::string> m_colorSpaceNames; //!< list of colorspace names (memory for const char* s below)
        std::vector<std::string> m_displayNames, m_viewNames;
        std::vector<const char*> m_colorSpaceCstrNames; //!< list for the pulldown list knob (used raw)
        std::vector<const char*> m_displayCstrNames, m_viewCstrNames;
        float m_gain;
        float m_gamma;
        int m_channel;
        
        std::string m_contextKey1;
        std::string m_contextValue1;
        std::string m_contextKey2;
        std::string m_contextValue2;
        std::string m_contextKey3;
        std::string m_contextValue3;
        std::string m_contextKey4;
        std::string m_contextValue4;
        OCIO::ConstContextRcPtr getLocalContext();
        
        OCIO::DisplayTransformRcPtr m_transform;
        OCIO::ConstProcessorRcPtr m_processor;

        DD::Image::Knob *m_displayKnob, *m_viewKnob;
        void refreshDisplayTransforms();
        
    public:

        OCIODisplay(Node *node);

        virtual ~OCIODisplay();

        static const DD::Image::Op::Description description;

        /*! Return the command name that will be stored in Nuke scripts. */
        virtual const char *Class() const;

        /*!
         * Return a name for this class that will be shown to the user. The
         * default implementation returns Class(). You can return a different
         * (ie more user-friendly) name instead here, and there is no need for
         * this to be unique.
         * 
         * Nuke currently will remove any trailing digits and underscores from
         * this and add a new number to make a unique name for the new node.
         * 
         * \return "OCIODisplay"
         */
        virtual const char *displayName() const;

        /*!
         * Return help information for this node. This information is in the
         * pop-up window that the user gets when they hit the [?] button in
         * the lower-left corner of the control panel.
         */
        virtual const char *node_help() const;

        /*!
         * Define the knobs that will be presented in the control panel.
         */
        virtual void knobs(DD::Image::Knob_Callback f);

        /*!
         * Specify the channels required from input n to produce the channels
         * in mask by modifying mask in-place. (At least one channel in the
         * input is assumed.)
         *
         * Since colorspace conversions can have channel cross-talk, any rgb
         * output channel requires all its rgb bretheren. (Non-rgb
         * are passed through.)
         */
        virtual void in_channels(int n, DD::Image::ChannelSet& mask) const;

        /*!
         * Calculate the output pixel data.
         * \param rowY vertical line number
         * \param rowX inclusive left bound
         * \param rowXBound exclusive right bound
         * \param outputChannels a subset of out_channels(), the required channels to be produced
         */
        virtual void pixel_engine(
            const DD::Image::Row& in,
            int rowY, int rowX, int rowXBound,
            DD::Image::ChannelMask outputChannels,
            DD::Image::Row& out);

        /*!
         * When the display device changes,
         * regenerate the display transform list.
         */
        virtual int knob_changed(DD::Image::Knob *k);

        virtual void append(DD::Image::Hash& hash);


    protected:

        /*!
         * Check that colorspaces are available, and that the transform
         * is not a noop. (As OCIO whether a given transform is a noop, since it
         * can do more analysis than just name matching.)
         */
        virtual void _validate(bool for_real);

};


static DD::Image::Op* build(Node *node);

#endif // INCLUDED_OCIO_NUKE_DISPLAY_H_

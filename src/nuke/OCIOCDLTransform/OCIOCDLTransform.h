#ifndef INCLUDED_OCIO_NUKE_CDLTRANSFORM_H_
#define INCLUDED_OCIO_NUKE_CDLTRANSFORM_H_

// Include these early, for Nuke's headers under gcc 4.4.2.
#include <memory>
#include <cstdarg>

#include <DDImage/PixelIop.h>
#include <DDImage/Row.h>
#include <DDImage/Knob.h>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;


/*!
 * Iop that uses OpenColorIO to apply an ASC CDL transform
 */
class OCIOCDLTransform : public DD::Image::PixelIop {

    protected:

        // ASC CDL grade numbers
        float m_slope[3];
        float m_offset[3];
        float m_power[3];
        float m_saturation;

        static const char* dirs[];
        int m_dirindex;

        bool m_readFromFile;
        
        // ID used for exporting grades into .cc/.ccc files
        const char* m_file;
        std::string m_cccid;
        
        DD::Image::Knob* m_slopeKnob;
        DD::Image::Knob* m_offsetKnob;
        DD::Image::Knob* m_powerKnob;
        DD::Image::Knob* m_saturationKnob;
        DD::Image::Knob* m_fileKnob;
        DD::Image::Knob* m_cccidKnob;
        
        OCIO::ConstProcessorRcPtr m_processor;
        bool m_firstLoad;

        /*! Controlled by hidden "version" knob, incremented to redraw image */
        int m_reload_version;

    public:
        OCIOCDLTransform(Node *node);

        virtual ~OCIOCDLTransform();

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
         * \return "OCIOCDLTransform"
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

        //! The will handle the knob changes.
        virtual int knob_changed(DD::Image::Knob*);
        
        void refreshKnobEnabledState();
        void loadCDLFromFile();
        
        
        /*!
         * Specify the channels required from input n to produce the channels
         * in mask by modifying mask in-place. (At least one channel in the
         * input is assumed.)
         *
         * Since OCIOCDLTransform conversions can have channel cross-talk, any rgb
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


    protected:

        /*!
         * Check that colorspaces are available, and that the transform
         * is not a noop. (As OCIO whether a given transform is a noop, since it
         * can do more analysis than just name matching.)
         */
        virtual void _validate(bool for_real);

        /*!
         * Ensure Node hash is reflects all parameters
         */
        void append(DD::Image::Hash& nodehash);

};


static DD::Image::Op* build(Node *node);

#endif // INCLUDED_OCIO_NUKE_CDLTRANSFORM_H_

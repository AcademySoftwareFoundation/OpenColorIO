/*
Copyright (c) 2003-2012 Sony Pictures Imageworks Inc., et al.
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


#include "OpenColorIO_AE.h"

#include "OpenColorIO_AE_Context.h"
#include "OpenColorIO_AE_Dialogs.h"

#include "AEGP_SuiteHandler.h"

// this lives in OpenColorIO_AE_UI.cpp
std::string GetProjectDir(PF_InData *in_data);


static PF_Err About(  
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output )
{
    PF_SPRINTF( out_data->return_msg, 
                "OpenColorIO\r\r"
                "opencolorio.org\r"
                "version %s",
                OCIO::GetVersion() );
                
    return PF_Err_NONE;
}


static PF_Err GlobalSetup(    
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output )
{
    out_data->my_version    =   PF_VERSION( MAJOR_VERSION, 
                                            MINOR_VERSION,
                                            BUG_VERSION, 
                                            STAGE_VERSION, 
                                            BUILD_VERSION);

    out_data->out_flags     =   PF_OutFlag_DEEP_COLOR_AWARE     |
                                PF_OutFlag_PIX_INDEPENDENT      |
                                PF_OutFlag_CUSTOM_UI            |
                                PF_OutFlag_USE_OUTPUT_EXTENT    |
                                PF_OutFlag_I_HAVE_EXTERNAL_DEPENDENCIES;

    out_data->out_flags2    =   PF_OutFlag2_PARAM_GROUP_START_COLLAPSED_FLAG |
                                PF_OutFlag2_SUPPORTS_SMART_RENDER   |
                                PF_OutFlag2_FLOAT_COLOR_AWARE       |
                                PF_OutFlag2_PPRO_DO_NOT_CLONE_SEQUENCE_DATA_FOR_RENDER;
    
    
    GlobalSetup_GL();
    
    
    if(in_data->appl_id == 'PrMr')
    {
        PF_PixelFormatSuite1 *pfS = NULL;
        
        in_data->pica_basicP->AcquireSuite(kPFPixelFormatSuite,
                                            kPFPixelFormatSuiteVersion1,
                                            (const void **)&pfS);
                                            
        if(pfS)
        {
            pfS->ClearSupportedPixelFormats(in_data->effect_ref);
            
            pfS->AddSupportedPixelFormat(in_data->effect_ref,
                                            PrPixelFormat_BGRA_4444_32f);
            
            in_data->pica_basicP->ReleaseSuite(kPFPixelFormatSuite,
                                                kPFPixelFormatSuiteVersion1);
        }
    }
    
    return PF_Err_NONE;
}


static PF_Err GlobalSetdown(  
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output )
{
    GlobalSetdown_GL();
    
    return PF_Err_NONE;
}


static PF_Err ParamsSetup(
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output)
{
    PF_Err          err = PF_Err_NONE;
    PF_ParamDef     def;


    // readout
    AEFX_CLR_STRUCT(def);
    // we can time_vary once we're willing to print and scan ArbData text
    def.flags = PF_ParamFlag_CANNOT_TIME_VARY;
    
    ArbNewDefault(in_data, out_data, NULL, &def.u.arb_d.dephault);
    
    PF_ADD_ARBITRARY("OCIO",
                        UI_CONTROL_WIDTH,
                        UI_CONTROL_HEIGHT,
                        PF_PUI_CONTROL,
                        def.u.arb_d.dephault,
                        OCIO_DATA,
                        NULL);
    
    
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOX("",
                    "Use GPU",
                    FALSE,
                    0,
                    OCIO_GPU_ID);
                    

    out_data->num_params = OCIO_NUM_PARAMS;

    // register custom UI
    if (!err) 
    {
        PF_CustomUIInfo         ci;

        AEFX_CLR_STRUCT(ci);
        
        ci.events               = PF_CustomEFlag_EFFECT;
        
        ci.comp_ui_width        = ci.comp_ui_height = 0;
        ci.comp_ui_alignment    = PF_UIAlignment_NONE;
        
        ci.layer_ui_width       = 0;
        ci.layer_ui_height      = 0;
        ci.layer_ui_alignment   = PF_UIAlignment_NONE;
        
        ci.preview_ui_width     = 0;
        ci.preview_ui_height    = 0;
        ci.layer_ui_alignment   = PF_UIAlignment_NONE;

        err = (*(in_data->inter.register_ui))(in_data->effect_ref, &ci);
    }


    return err;
}

static PF_Err SequenceSetup(
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output )
{
    PF_Err err = PF_Err_NONE;
    
    SequenceData *seq_data = NULL;
    
    // set up sequence data
    if( (in_data->sequence_data == NULL) )
    {
        out_data->sequence_data = PF_NEW_HANDLE( sizeof(SequenceData) );
        
        seq_data = (SequenceData *)PF_LOCK_HANDLE(out_data->sequence_data);
        
        seq_data->path[0] = '\0';
        seq_data->relative_path[0] = '\0';
    }
    else // reset pre-existing sequence data
    {
        if( PF_GET_HANDLE_SIZE(in_data->sequence_data) != sizeof(SequenceData) )
        {
            PF_RESIZE_HANDLE(sizeof(SequenceData), &in_data->sequence_data);
        }
            
        seq_data = (SequenceData *)PF_LOCK_HANDLE(in_data->sequence_data);
    }
    
    
    seq_data->status = STATUS_UNKNOWN;
    seq_data->gpu_err = GPU_ERR_NONE;
    seq_data->prem_status = PREMIERE_UNKNOWN;
    seq_data->context = NULL;
    
    
    PF_UNLOCK_HANDLE(in_data->sequence_data);
    
    return err;
}


static PF_Err SequenceSetdown(
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output )
{
    PF_Err err = PF_Err_NONE;
    
    if(in_data->sequence_data)
    {
        SequenceData *seq_data = (SequenceData *)PF_LOCK_HANDLE(out_data->sequence_data);
        
        if(seq_data->context)
        {
            delete seq_data->context;
            
            seq_data->status = STATUS_UNKNOWN;
            seq_data->gpu_err = GPU_ERR_NONE;
            seq_data->prem_status = PREMIERE_UNKNOWN;
            seq_data->context = NULL;
        }
        
        PF_DISPOSE_HANDLE(in_data->sequence_data);
    }

    return err;
}


static PF_Err SequenceFlatten(
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output )
{
    PF_Err err = PF_Err_NONE;

    if(in_data->sequence_data)
    {
        SequenceData *seq_data = (SequenceData *)PF_LOCK_HANDLE(in_data->sequence_data);
        
        if(seq_data->context)
        {
            delete seq_data->context;
            
            seq_data->status = STATUS_UNKNOWN;
            seq_data->gpu_err = GPU_ERR_NONE;
            seq_data->prem_status = PREMIERE_UNKNOWN;
            seq_data->context = NULL;
        }

        PF_UNLOCK_HANDLE(in_data->sequence_data);
    }

    return err;
}



static PF_Boolean IsEmptyRect(const PF_LRect *r){
    return (r->left >= r->right) || (r->top >= r->bottom);
}

#ifndef mmin
    #define mmin(a,b) ((a) < (b) ? (a) : (b))
    #define mmax(a,b) ((a) > (b) ? (a) : (b))
#endif


static void UnionLRect(const PF_LRect *src, PF_LRect *dst)
{
    if (IsEmptyRect(dst)) {
        *dst = *src;
    } else if (!IsEmptyRect(src)) {
        dst->left   = mmin(dst->left, src->left);
        dst->top    = mmin(dst->top, src->top);
        dst->right  = mmax(dst->right, src->right);
        dst->bottom = mmax(dst->bottom, src->bottom);
    }
}


static PF_Err PreRender(
    PF_InData               *in_data,
    PF_OutData              *out_data,
    PF_PreRenderExtra       *extra)
{
    PF_Err err = PF_Err_NONE;
    PF_RenderRequest req = extra->input->output_request;
    PF_CheckoutResult in_result;
    
    req.preserve_rgb_of_zero_alpha = TRUE;

    ERR(extra->cb->checkout_layer(  in_data->effect_ref,
                                    OCIO_INPUT,
                                    OCIO_INPUT,
                                    &req,
                                    in_data->current_time,
                                    in_data->time_step,
                                    in_data->time_scale,
                                    &in_result));


    UnionLRect(&in_result.result_rect,      &extra->output->result_rect);
    UnionLRect(&in_result.max_result_rect,  &extra->output->max_result_rect);   
    
    return err;
}

#pragma mark-


template <typename InFormat, typename OutFormat>
static inline OutFormat Convert(InFormat in);

template <>
static inline float Convert<A_u_char, float>(A_u_char in)
{
    return (float)in / (float)PF_MAX_CHAN8;
}

template <>
static inline float Convert<A_u_short, float>(A_u_short in)
{
    return (float)in / (float)PF_MAX_CHAN16;
}

template <>
static inline float Convert<float, float>(float in)
{
    return in;
}

static inline float Clamp(float in)
{
    return (in > 1.f ? 1.f : in < 0.f ? 0.f : in);
}

template <>
static inline A_u_char Convert<float, A_u_char>(float in)
{
    return ( Clamp(in) * (float)PF_MAX_CHAN8 ) + 0.5f;
}

template <>
static inline A_u_short Convert<float, A_u_short>(float in)
{
    return ( Clamp(in) * (float)PF_MAX_CHAN16 ) + 0.5f;
}



typedef struct {
    PF_InData *in_data;
    void *in_buffer;
    A_long in_rowbytes;
    void *out_buffer;
    A_long out_rowbytes;
    int width;
} IterateData;

template <typename InFormat, typename OutFormat>
static PF_Err CopyWorld_Iterate(
    void *refconPV,
    A_long thread_indexL,
    A_long i,
    A_long iterationsL)
{
    PF_Err err = PF_Err_NONE;
    
    IterateData *i_data = (IterateData *)refconPV;
    PF_InData *in_data = i_data->in_data;
    
    InFormat *in_pix = (InFormat *)((char *)i_data->in_buffer + (i * i_data->in_rowbytes)); 
    OutFormat *out_pix = (OutFormat *)((char *)i_data->out_buffer + (i * i_data->out_rowbytes));
    
#ifdef NDEBUG
    if(thread_indexL == 0)
        err = PF_ABORT(in_data);
#endif

    for(int x=0; x < i_data->width; x++)
    {
        *out_pix++ = Convert<InFormat, OutFormat>( *in_pix++ );
    }
    
    return err;
}


typedef struct {
    PF_InData *in_data;
    void *in_buffer;
    A_long in_rowbytes;
    int width;
} SwapData;

static PF_Err Swap_Iterate(
    void *refconPV,
    A_long thread_indexL,
    A_long i,
    A_long iterationsL)
{
    PF_Err err = PF_Err_NONE;
    
    SwapData *i_data = (SwapData *)refconPV;
    PF_InData *in_data = i_data->in_data;
    
    PF_PixelFloat *pix = (PF_PixelFloat *)((char *)i_data->in_buffer + (i * i_data->in_rowbytes));
    
#ifdef NDEBUG
    if(thread_indexL == 0)
        err = PF_ABORT(in_data);
#endif

    for(int x=0; x < i_data->width; x++)
    {
        float temp;
        
        // BGRA -> ARGB
        temp       = pix->alpha; // BGRA temp B
        pix->alpha = pix->blue;  // AGRA temp B
        pix->blue  = temp;       // AGRB temp B
        temp       = pix->red;   // AGRB temp G
        pix->red   = pix->green; // ARRB temp G
        pix->green = temp;       // ARGB temp G
        
        pix++;
    }
    
    return err;
}


typedef struct {
    PF_InData               *in_data;
    void                    *buffer;
    A_long                  rowbytes;
    int                     width;
    OpenColorIO_AE_Context  *context;
} ProcessData;

static PF_Err Process_Iterate(
    void *refconPV,
    A_long thread_indexL,
    A_long i,
    A_long iterationsL)
{
    PF_Err err = PF_Err_NONE;
    
    ProcessData *i_data = (ProcessData *)refconPV;
    PF_InData *in_data = i_data->in_data;
    
    PF_PixelFloat *pix = (PF_PixelFloat *)((char *)i_data->buffer + (i * i_data->rowbytes)); 
    
#ifdef NDEBUG
    if(thread_indexL == 0)
        err = PF_ABORT(in_data);
#endif

    try
    {
        float *rOut = &pix->red;

        OCIO::PackedImageDesc img(rOut, i_data->width, 1, 4);
                                                
        i_data->context->processor()->apply(img);
    }
    catch(...)
    {
        err = PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
    
    
    return err;
}


// two functions below to get Premiere to run my functions multi-threaded
// because they couldn't bother to give me PF_Iterate8Suite1->iterate_generic

typedef PF_Err (*GenericIterator)(void *refconPV,
                                    A_long thread_indexL,
                                    A_long i,
                                    A_long iterationsL);

typedef struct {
    PF_InData       *in_data;
    GenericIterator fn_func;
    void            *refconPV;
    A_long          height;
} FakeData;

static PF_Err MyFakeIterator(
    void *refcon,
    A_long x,
    A_long y,
    PF_Pixel *in,
    PF_Pixel *out)
{
    PF_Err err = PF_Err_NONE;
    
    FakeData *i_data = (FakeData *)refcon;
    PF_InData *in_data = i_data->in_data;
    
    err = i_data->fn_func(i_data->refconPV, 1, y, i_data->height);
    
    return err;
}

typedef PF_Err (*GenericIterateFunc)(
        A_long          iterationsL,
        void            *refconPV,
        GenericIterator fn_func);

static PF_Err MyGenericIterateFunc(
        A_long          iterationsL,
        void            *refconPV,
        GenericIterator fn_func)
{
    PF_Err err = PF_Err_NONE;
    
    PF_InData **in_dataH = (PF_InData **)refconPV; // always put PF_InData first
    PF_InData *in_data = *in_dataH;
    
    PF_Iterate8Suite1 *i8sP = NULL;
    in_data->pica_basicP->AcquireSuite(kPFIterate8Suite, kPFIterate8SuiteVersion1, (const void **)&i8sP);

    if(i8sP && i8sP->iterate)
    {
        PF_EffectWorld fake_world;
        PF_NEW_WORLD(1, iterationsL, PF_NewWorldFlag_NONE, &fake_world);
        
        
        FakeData i_data = { in_data, fn_func, refconPV, iterationsL };
        
        err = i8sP->iterate(in_data, 0, iterationsL, &fake_world, NULL,
                            (void *)&i_data, MyFakeIterator, &fake_world);
        
        
        PF_DISPOSE_WORLD(&fake_world);
        
        in_data->pica_basicP->ReleaseSuite(kPFIterate8Suite, kPFIterate8SuiteVersion1);
    }
    else
    {
        for(int i=0; i < iterationsL && !err; i++)
        {
            err = fn_func(refconPV, 0, i, iterationsL);
        }
    }
    
    return err;
}


static PF_Err DoRender(
    PF_InData       *in_data,
    PF_EffectWorld  *input,
    PF_ParamDef     *OCIO_data,
    PF_ParamDef     *OCIO_gpu,
    PF_OutData      *out_data,
    PF_EffectWorld  *output)
{
    PF_Err              err     = PF_Err_NONE;

    AEGP_SuiteHandler suites(in_data->pica_basicP);

    PF_PixelFormatSuite1 *pfS = NULL;
    PF_WorldSuite2 *wsP = NULL;
    
    err = in_data->pica_basicP->AcquireSuite(kPFPixelFormatSuite, kPFPixelFormatSuiteVersion1, (const void **)&pfS);
    err = in_data->pica_basicP->AcquireSuite(kPFWorldSuite, kPFWorldSuiteVersion2, (const void **)&wsP);

    if(!err)
    {
        ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(OCIO_data->u.arb_d.value);
        SequenceData *seq_data = (SequenceData *)PF_LOCK_HANDLE(in_data->sequence_data);
        
        try
        {
            seq_data->status = STATUS_OK;
        
            std::string dir = GetProjectDir(in_data);

            // must always verify that our context lines up with the parameters
            // things like undo can change them without notice
            if(seq_data->context != NULL)
            {
                bool verified = seq_data->context->Verify(arb_data, dir);
                
                if(!verified)
                {
                    delete seq_data->context;
                    
                    seq_data->status = STATUS_UNKNOWN;
                    seq_data->context = NULL;
                }
            }
        
        
            if(arb_data->action == OCIO_ACTION_NONE)
            {
                seq_data->status = STATUS_NO_FILE;
            }
            else if(seq_data->context == NULL)
            {
                seq_data->source = arb_data->source;
                
                if(arb_data->source == OCIO_SOURCE_ENVIRONMENT)
                {
                    char *file = std::getenv("OCIO");
                    
                    if(file == NULL)
                        seq_data->status = STATUS_FILE_MISSING;
                }
                else if(arb_data->source == OCIO_SOURCE_STANDARD)
                {
                    std::string path = GetStdConfigPath(arb_data->path);
                    
                    if( path.empty() )
                    {
                        seq_data->status = STATUS_FILE_MISSING;
                    }
                    else
                    {
                        strncpy(seq_data->path, arb_data->path, ARB_PATH_LEN);
                        strncpy(seq_data->relative_path, arb_data->relative_path, ARB_PATH_LEN);
                    }
                }
                else if(arb_data->source == OCIO_SOURCE_CUSTOM)
                {
                    Path absolute_path(arb_data->path, dir);
                    Path relative_path(arb_data->relative_path, dir);
                    Path seq_absolute_path(seq_data->path, dir);
                    Path seq_relative_path(seq_data->relative_path, dir);
                    
                    if( absolute_path.exists() )
                    {
                        seq_data->status = STATUS_USING_ABSOLUTE;
                        
                        strncpy(seq_data->path, absolute_path.full_path().c_str(), ARB_PATH_LEN);
                        strncpy(seq_data->relative_path, absolute_path.relative_path(false).c_str(), ARB_PATH_LEN);
                    }
                    else if( relative_path.exists() )
                    {
                        seq_data->status = STATUS_USING_RELATIVE;
                        
                        strncpy(seq_data->path, relative_path.full_path().c_str(), ARB_PATH_LEN);
                        strncpy(seq_data->relative_path, relative_path.relative_path(false).c_str(), ARB_PATH_LEN);
                    }
                    else if( seq_absolute_path.exists() )
                    {
                        // In some cases, we may have a good path in sequence options but not in
                        // the arbitrary parameter.  An alert will not be provided because it is the
                        // sequence options that get checked.  Therefore, we have to use the sequence
                        // options as a last resort.  We copy the path back to arb data, but the change
                        // should not stick.
                        seq_data->status = STATUS_USING_ABSOLUTE;
                        
                        strncpy(arb_data->path, seq_absolute_path.full_path().c_str(), ARB_PATH_LEN);
                        strncpy(arb_data->relative_path, seq_absolute_path.relative_path(false).c_str(), ARB_PATH_LEN);
                    }
                    else if( seq_relative_path.exists() )
                    {
                        seq_data->status = STATUS_USING_RELATIVE;
                        
                        strncpy(arb_data->path, seq_relative_path.full_path().c_str(), ARB_PATH_LEN);
                        strncpy(arb_data->relative_path, seq_relative_path.relative_path(false).c_str(), ARB_PATH_LEN);
                    }
                    else
                        seq_data->status = STATUS_FILE_MISSING;
                }
            
            
                if(seq_data->status != STATUS_FILE_MISSING)
                {
                    seq_data->context = new OpenColorIO_AE_Context(arb_data, dir);
                }
            }
        }
        catch(...)
        {
            seq_data->status = STATUS_OCIO_ERROR;
        }
        
        
        if(seq_data->status == STATUS_FILE_MISSING || seq_data->status == STATUS_OCIO_ERROR)
        {
            err = PF_Err_INTERNAL_STRUCT_DAMAGED;
        }

        
        if(!err)
        {
            if(seq_data->context == NULL || seq_data->context->processor()->isNoOp())
            {
                err = PF_COPY(input, output, NULL, NULL);
            }
            else
            {
                GenericIterateFunc iterate_generic = suites.Iterate8Suite1()->iterate_generic;
                
                if(iterate_generic == NULL)
                    iterate_generic = MyGenericIterateFunc; // thanks a lot, Premiere
            
                // OpenColorIO only does float worlds
                // might have to create one
                PF_EffectWorld *float_world = NULL;
                
                PF_EffectWorld temp_world_data;
                PF_EffectWorld *temp_world = NULL;
                PF_Handle temp_worldH = NULL;
                
                
                PF_PixelFormat format;
                wsP->PF_GetPixelFormat(output, &format);
                
                if(in_data->appl_id == 'PrMr' && pfS)
                {
                    // the regular world suite function will give a bogus value for Premiere
                    pfS->GetPixelFormat(output, (PrPixelFormat *)&format);
                    
                    seq_data->prem_status = (format == PrPixelFormat_BGRA_4444_32f_Linear ?
                                                PREMIERE_LINEAR : PREMIERE_NON_LINEAR);
                }
                
                
                A_Boolean use_gpu = OCIO_gpu->u.bd.value;
                seq_data->gpu_err = GPU_ERR_NONE;
                A_long non_padded_rowbytes = sizeof(PF_PixelFloat) * output->width;
                

                if(format == PF_PixelFormat_ARGB128 &&
                    (!use_gpu || output->rowbytes == non_padded_rowbytes)) // GPU doesn't do padding
                {
                    err = PF_COPY(input, output, NULL, NULL);
                    
                    float_world = output;
                }
                else
                {
                    temp_worldH = PF_NEW_HANDLE(non_padded_rowbytes * (output->height + 1)); // little extra because we go over by a channel
                    
                    if(temp_worldH)
                    {
                        temp_world_data.data = (PF_PixelPtr)PF_LOCK_HANDLE(temp_worldH);
                        
                        temp_world_data.width = output->width;
                        temp_world_data.height = output->height;
                        temp_world_data.rowbytes = non_padded_rowbytes;
                        
                        float_world = temp_world = &temp_world_data;

                        // convert to new temp float world
                        IterateData i_data = { in_data, input->data, input->rowbytes,
                                                float_world->data, float_world->rowbytes,
                                                float_world->width * 4 };
                        
                        if(format == PF_PixelFormat_ARGB32 || format == PrPixelFormat_BGRA_4444_8u)
                        {
                            err = iterate_generic(float_world->height, &i_data,
                                                    CopyWorld_Iterate<A_u_char, float>);
                        }
                        else if(format == PF_PixelFormat_ARGB64)
                        {
                            err = iterate_generic(float_world->height, &i_data,
                                                    CopyWorld_Iterate<A_u_short, float>);
                        }
                        else if(format == PF_PixelFormat_ARGB128 ||
                                format == PrPixelFormat_BGRA_4444_32f ||
                                format == PrPixelFormat_BGRA_4444_32f_Linear)
                        {
                            err = iterate_generic(float_world->height, &i_data,
                                                    CopyWorld_Iterate<float, float>);
                        }
                        
                        // switch BGRA to ARGB for premiere
                        if(!err &&
                            (format == PrPixelFormat_BGRA_4444_8u ||
                            format == PrPixelFormat_BGRA_4444_32f_Linear ||
                            format == PrPixelFormat_BGRA_4444_32f))
                        {
                            SwapData s_data = { in_data, float_world->data,
                                            float_world->rowbytes, float_world->width };
                            
                            err = iterate_generic(float_world->height, &s_data,
                                                    Swap_Iterate);
                        }
                    }
                    else
                        err = PF_Err_OUT_OF_MEMORY;
                }
                
                
                if(!err)
                {
                    bool gpu_rendered = false;

                    // OpenColorIO processing
                    if(use_gpu)
                    {
                        if( HaveOpenGL() )
                        {
                            gpu_rendered = seq_data->context->ProcessWorldGL(float_world);
                            
                            if(!gpu_rendered)
                                seq_data->gpu_err = GPU_ERR_RENDER_ERR;
                        }
                        else
                            seq_data->gpu_err = GPU_ERR_INSUFFICIENT;
                    }
                    
                    if(!gpu_rendered)
                    {
                        ProcessData p_data = { in_data,
                                                float_world->data,
                                                float_world->rowbytes,
                                                float_world->width,
                                                seq_data->context };

                        err = iterate_generic(float_world->height, &p_data, Process_Iterate);
                    }
                }
                
                
                // copy back to non-float world and dispose
                if(temp_world)
                {
                    if(!err &&
                        (format == PrPixelFormat_BGRA_4444_8u ||
                        format == PrPixelFormat_BGRA_4444_32f_Linear ||
                        format == PrPixelFormat_BGRA_4444_32f))
                    {
                        SwapData s_data = { in_data, float_world->data,
                                            float_world->rowbytes, float_world->width };
                        
                        err = iterate_generic(float_world->height, &s_data, Swap_Iterate);
                    }
                    
                    if(!err)
                    {
                        IterateData i_data = { in_data, float_world->data,
                                                float_world->rowbytes, output->data,
                                                output->rowbytes, output->width * 4 };
                        
                        if(format == PF_PixelFormat_ARGB32 || format == PrPixelFormat_BGRA_4444_8u)
                        {
                            err = iterate_generic(output->height, &i_data,
                                                    CopyWorld_Iterate<float, A_u_char>);
                        }
                        else if(format == PF_PixelFormat_ARGB64)
                        {
                            err = iterate_generic(output->height, &i_data,
                                                    CopyWorld_Iterate<float, A_u_short>);
                        }
                        else if(format == PF_PixelFormat_ARGB128 ||
                                format == PrPixelFormat_BGRA_4444_32f ||
                                format == PrPixelFormat_BGRA_4444_32f_Linear)
                        {
                            err = iterate_generic(output->height, &i_data,
                                                    CopyWorld_Iterate<float, float>);
                        }
                            
                    }

                    PF_DISPOSE_HANDLE(temp_worldH);
                }
                    
                    
                PF_UNLOCK_HANDLE(OCIO_data->u.arb_d.value);
                PF_UNLOCK_HANDLE(in_data->sequence_data);


                if(seq_data->gpu_err == GPU_ERR_INSUFFICIENT)
                {
                    suites.AdvAppSuite2()->PF_AppendInfoText("OpenColorIO: GPU Insufficient");
                }
                else if(seq_data->gpu_err == GPU_ERR_RENDER_ERR)
                {
                    suites.AdvAppSuite2()->PF_AppendInfoText("OpenColorIO: GPU Render Error");
                }
            }
        }
    }

    if(pfS)
        in_data->pica_basicP->ReleaseSuite(kPFPixelFormatSuite, kPFPixelFormatSuiteVersion1);
    
    if(wsP)
        in_data->pica_basicP->ReleaseSuite(kPFWorldSuite, kPFWorldSuiteVersion2);
        
    
    
    return err;
}


static PF_Err SmartRender(
    PF_InData               *in_data,
    PF_OutData              *out_data,
    PF_SmartRenderExtra     *extra)

{
    PF_Err          err     = PF_Err_NONE,
                    err2    = PF_Err_NONE;
                    
    PF_EffectWorld *input, *output;
    
    PF_ParamDef OCIO_data, OCIO_gpu;

    // zero-out parameters
    AEFX_CLR_STRUCT(OCIO_data);
    AEFX_CLR_STRUCT(OCIO_gpu);
    
    
    // checkout input & output buffers.
    ERR(    extra->cb->checkout_layer_pixels( in_data->effect_ref, OCIO_INPUT, &input)  );
    ERR(    extra->cb->checkout_output( in_data->effect_ref, &output)   );


    // bail before param checkout
    if(err)
        return err;

#define PF_CHECKOUT_PARAM_NOW( PARAM, DEST ) \
        PF_CHECKOUT_PARAM(  in_data, (PARAM), in_data->current_time,\
                            in_data->time_step, in_data->time_scale, DEST )

    // checkout the required params
    ERR(    PF_CHECKOUT_PARAM_NOW( OCIO_DATA,   &OCIO_data )    );
    ERR(    PF_CHECKOUT_PARAM_NOW( OCIO_GPU,    &OCIO_gpu  )    );

    ERR(DoRender(   in_data, 
                    input, 
                    &OCIO_data,
                    &OCIO_gpu,
                    out_data, 
                    output));

    // Always check in, no matter what the error condition!
    ERR2(   PF_CHECKIN_PARAM(in_data, &OCIO_data )  );
    ERR2(   PF_CHECKIN_PARAM(in_data, &OCIO_gpu  )  );


    return err;
  
}


static PF_Err Render(
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output )
{
    return DoRender(in_data,
                    &params[OCIO_INPUT]->u.ld,
                    params[OCIO_DATA],
                    params[OCIO_GPU],
                    out_data,
                    output);
}


static PF_Err GetExternalDependencies(
    PF_InData                   *in_data,
    PF_OutData                  *out_data,
    PF_ExtDependenciesExtra     *extra)

{
    PF_Err err = PF_Err_NONE;
    
    SequenceData *seq_data = (SequenceData *)PF_LOCK_HANDLE(in_data->sequence_data);
    
    if(seq_data == NULL)
        return PF_Err_BAD_CALLBACK_PARAM;
    

    std::string dependency;
    
    if(seq_data->source == OCIO_SOURCE_ENVIRONMENT)
    {
        if(extra->check_type == PF_DepCheckType_ALL_DEPENDENCIES)
        {
            dependency = "$OCIO environment variable";
        }
        else if(extra->check_type == PF_DepCheckType_MISSING_DEPENDENCIES)
        {
            char *file = std::getenv("OCIO");
            
            if(!file)
                dependency = "$OCIO environment variable";
        }
    }
    else if(seq_data->source == OCIO_SOURCE_STANDARD)
    {
        if(extra->check_type == PF_DepCheckType_ALL_DEPENDENCIES)
        {
            dependency = "OCIO configuration " + std::string(seq_data->path);
        }
        else if(extra->check_type == PF_DepCheckType_MISSING_DEPENDENCIES)
        {
            std::string path = GetStdConfigPath(seq_data->path);
            
            if( path.empty() )
                dependency = "OCIO configuration " + std::string(seq_data->path);
        }
    }
    else if(seq_data->source == OCIO_SOURCE_CUSTOM && seq_data->path[0] != '\0')
    {
        std::string dir = GetProjectDir(in_data);
            
        Path absolute_path(seq_data->path, "");
        Path relative_path(seq_data->relative_path, dir);
    
        if(extra->check_type == PF_DepCheckType_ALL_DEPENDENCIES)
        {
            if( !absolute_path.exists() && relative_path.exists() )
            {
                dependency = relative_path.full_path();
            }
            else
                dependency = absolute_path.full_path();
        }
        else if(extra->check_type == PF_DepCheckType_MISSING_DEPENDENCIES &&
                    !absolute_path.exists() && !relative_path.exists() )
        {
            dependency = absolute_path.full_path();
        }
    }
    
    
    if( !dependency.empty() )
    {
        extra->dependencies_strH = PF_NEW_HANDLE(sizeof(char) * (dependency.size() + 1));
        
        char *p = (char *)PF_LOCK_HANDLE(extra->dependencies_strH);
        
        strcpy(p, dependency.c_str());
    }
    
    
    PF_UNLOCK_HANDLE(in_data->sequence_data);
    
    return err;
}


DllExport PF_Err PluginMain(
    PF_Cmd          cmd,
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output,
    void            *extra)
{
    PF_Err      err = PF_Err_NONE;
    
    try {
        switch(cmd) {
            case PF_Cmd_ABOUT:
                err = About(in_data,out_data,params,output);
                break;
            case PF_Cmd_GLOBAL_SETUP:
                err = GlobalSetup(in_data,out_data,params,output);
                break;
            case PF_Cmd_GLOBAL_SETDOWN:
                err = GlobalSetdown(in_data,out_data,params,output);
                break;
            case PF_Cmd_PARAMS_SETUP:
                err = ParamsSetup(in_data,out_data,params,output);
                break;
            case PF_Cmd_SEQUENCE_SETUP:
            case PF_Cmd_SEQUENCE_RESETUP:
                err = SequenceSetup(in_data, out_data, params, output);
                break;
            case PF_Cmd_SEQUENCE_FLATTEN:
                err = SequenceFlatten(in_data, out_data, params, output);
                break;
            case PF_Cmd_SEQUENCE_SETDOWN:
                err = SequenceSetdown(in_data, out_data, params, output);
                break;
            case PF_Cmd_SMART_PRE_RENDER:
                err = PreRender(in_data, out_data, (PF_PreRenderExtra*)extra);
                break;
            case PF_Cmd_SMART_RENDER:
                err = SmartRender(in_data, out_data, (PF_SmartRenderExtra*)extra);
                break;
            case PF_Cmd_RENDER:
                err = Render(in_data, out_data, params, output);
                break;
            case PF_Cmd_EVENT:
                err = HandleEvent(in_data, out_data, params, output, (PF_EventExtra *)extra);
                break;
            case PF_Cmd_ARBITRARY_CALLBACK:
                err = HandleArbitrary(in_data, out_data, params, output, (PF_ArbParamsExtra *)extra);
                break;
            case PF_Cmd_GET_EXTERNAL_DEPENDENCIES:
                err = GetExternalDependencies(in_data, out_data, (PF_ExtDependenciesExtra *)extra);
                break;
        }
    }
    catch(PF_Err &thrown_err) { err = thrown_err; }
    catch(...) { err = PF_Err_INTERNAL_STRUCT_DAMAGED; }
    
    return err;
}

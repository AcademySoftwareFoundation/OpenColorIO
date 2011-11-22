
//
//	mmm...arbitrary
//
//

#include "OpenColorIO_AE.h"

#ifndef __MACH__
#include <assert.h>
#endif


PF_Err
ArbNewDefault(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	PF_ArbitraryH		*arbPH)
{
	PF_Err err = PF_Err_NONE;
	
	if(arbPH)
	{
		*arbPH = PF_NEW_HANDLE( sizeof(ArbitraryData) );
		
		if(*arbPH)
		{
			ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(*arbPH);
			
			// set up defaults
			arb_data->version = CURRENT_ARB_VERSION;
			arb_data->path[0] = '\0';
			
			PF_UNLOCK_HANDLE(*arbPH);
		}
	}
	
	return err;
}


static PF_Err
ArbDispose(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	PF_ArbitraryH		arbH)
{
	if(arbH)
		PF_DISPOSE_HANDLE(arbH);
	
	return PF_Err_NONE;
}


static void
CopyArbData(ArbitraryData *out_arb_data, ArbitraryData *in_arb_data)
{
	// copy contents
	out_arb_data->version = in_arb_data->version;
	
	strcpy((char *)out_arb_data->path, (char *)in_arb_data->path);
}


static PF_Err
ArbCopy(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	PF_ArbitraryH		src_arbH,
	PF_ArbitraryH 		*dst_arbPH)
{
	PF_Err 	err 	= PF_Err_NONE;
	
	if(src_arbH && dst_arbPH)
	{
		// allocate using the creation function
		err = ArbNewDefault(in_data, out_data, refconPV, dst_arbPH);
		
		if(!err)
		{
			ArbitraryData *in_arb_data = (ArbitraryData *)PF_LOCK_HANDLE(src_arbH),
							*out_arb_data = (ArbitraryData *)PF_LOCK_HANDLE(*dst_arbPH);
			
			CopyArbData(out_arb_data, in_arb_data);
			
			PF_UNLOCK_HANDLE(src_arbH);
			PF_UNLOCK_HANDLE(*dst_arbPH);
		}
	}
	
	return err;
}


static PF_Err
ArbFlatSize(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	PF_ArbitraryH		arbH,
	A_u_long			*flat_data_sizePLu)
{
	// flat is the same size as inflated
	if(arbH)
		*flat_data_sizePLu = PF_GET_HANDLE_SIZE(arbH);
	
	return PF_Err_NONE;
}


static void 
SwapArbData(ArbitraryData *arb_data)
{

}


static PF_Err
ArbFlatten(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	PF_ArbitraryH		arbH,
	A_u_long			buf_sizeLu,
	void				*flat_dataPV)
{
	PF_Err 	err 	= PF_Err_NONE;
	
	if(arbH && flat_dataPV)
	{
		// they provide the buffer, we just move data
		ArbitraryData *in_arb_data = (ArbitraryData *)PF_LOCK_HANDLE(arbH),
						*out_arb_data = (ArbitraryData *)flat_dataPV;

		assert(buf_sizeLu >= PF_GET_HANDLE_SIZE(arbH));
	
		CopyArbData(out_arb_data, in_arb_data);
		
	#ifdef AE_BIG_ENDIAN
		// not that we're doing a PPC version of this...
		SwapArbData(out_arb_data);
	#endif
	
		PF_UNLOCK_HANDLE(arbH);
	}
	
	return err;
}


static PF_Err
ArbUnFlatten(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	A_u_long			buf_sizeLu,
	const void			*flat_dataPV,
	PF_ArbitraryH		*arbPH)
{
	PF_Err 	err 	= PF_Err_NONE;
	
	if(arbPH && flat_dataPV)
	{
		// they provide a flat buffer, we have to make the handle (using the default function)
		err = ArbNewDefault(in_data, out_data, refconPV, arbPH);
		
		if(!err && *arbPH)
		{
			ArbitraryData *in_arb_data = (ArbitraryData *)flat_dataPV,
							*out_arb_data = (ArbitraryData *)PF_LOCK_HANDLE(*arbPH);

			assert(buf_sizeLu <= PF_GET_HANDLE_SIZE(*arbPH));
		
			CopyArbData(out_arb_data, in_arb_data);
			
		#ifdef AE_BIG_ENDIAN
			SwapArbData(out_arb_data);
		#endif
		
			PF_UNLOCK_HANDLE(*arbPH);
		}
	}
	
	return err;
}

static PF_Err
ArbInterpolate(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	PF_ArbitraryH		left_arbH,
	PF_ArbitraryH		right_arbH,
	PF_FpLong			tF,
	PF_ArbitraryH		*interpPH)
{
	PF_Err 	err 	= PF_Err_NONE;
	
	assert(FALSE); // we shouldn't be doing this
	
	if(left_arbH && right_arbH && interpPH)
	{
		// allocate using our own func
		err = ArbNewDefault(in_data, out_data, refconPV, interpPH);
		
		if(!err && *interpPH)
		{
			// we're just going to copy the left_data
			ArbitraryData *in_arb_data = (ArbitraryData *)PF_LOCK_HANDLE(left_arbH),
							*out_arb_data = (ArbitraryData *)PF_LOCK_HANDLE(*interpPH);
			
			CopyArbData(out_arb_data, in_arb_data);
			
			PF_UNLOCK_HANDLE(left_arbH);
			PF_UNLOCK_HANDLE(*interpPH);
		}
	}
	
	return err;
}


static PF_Err
ArbCompare(PF_InData *in_data, PF_OutData *out_data,
	void				*refconPV,
	PF_ArbitraryH		a_arbH,
	PF_ArbitraryH		b_arbH,
	PF_ArbCompareResult	*compareP)
{
	PF_Err 	err 	= PF_Err_NONE;
	
	if(a_arbH && b_arbH)
	{
		ArbitraryData *a_data = (ArbitraryData *)PF_LOCK_HANDLE(a_arbH),
						*b_data = (ArbitraryData *)PF_LOCK_HANDLE(b_arbH);
		
		if( a_data->version == b_data->version &&
			!strcmp((char *)a_data->path, (char *)b_data->path) )
		{
			*compareP = PF_ArbCompare_EQUAL;
		}
		else
			*compareP = PF_ArbCompare_NOT_EQUAL;
		
		
		PF_UNLOCK_HANDLE(a_arbH);
		PF_UNLOCK_HANDLE(b_arbH);
	}
	
	return err;
}


PF_Err 
HandleArbitrary(
	PF_InData			*in_data,
	PF_OutData			*out_data,
	PF_ParamDef			*params[],
	PF_LayerDef			*output,
	PF_ArbParamsExtra	*extra)
{
	PF_Err 	err 	= PF_Err_NONE;
	
	if(extra->id == OCIO_DATA_ID)
	{
		switch(extra->which_function)
		{
			case PF_Arbitrary_NEW_FUNC:
				err = ArbNewDefault(in_data, out_data, extra->u.new_func_params.refconPV, extra->u.new_func_params.arbPH);
				break;
			case PF_Arbitrary_DISPOSE_FUNC:
				err = ArbDispose(in_data, out_data, extra->u.dispose_func_params.refconPV, extra->u.dispose_func_params.arbH);
				break;
			case PF_Arbitrary_COPY_FUNC:
				err = ArbCopy(in_data, out_data, extra->u.copy_func_params.refconPV, extra->u.copy_func_params.src_arbH, extra->u.copy_func_params.dst_arbPH);
				break;
			case PF_Arbitrary_FLAT_SIZE_FUNC:
				err = ArbFlatSize(in_data, out_data, extra->u.flat_size_func_params.refconPV, extra->u.flat_size_func_params.arbH, extra->u.flat_size_func_params.flat_data_sizePLu);
				break;
			case PF_Arbitrary_FLATTEN_FUNC:
				err = ArbFlatten(in_data, out_data, extra->u.flatten_func_params.refconPV, extra->u.flatten_func_params.arbH, extra->u.flatten_func_params.buf_sizeLu, extra->u.flatten_func_params.flat_dataPV);
				break;
			case PF_Arbitrary_UNFLATTEN_FUNC:
				err = ArbUnFlatten(in_data, out_data, extra->u.unflatten_func_params.refconPV, extra->u.unflatten_func_params.buf_sizeLu, extra->u.unflatten_func_params.flat_dataPV, extra->u.unflatten_func_params.arbPH);
				break;
			case PF_Arbitrary_INTERP_FUNC:
				err = ArbInterpolate(in_data, out_data, extra->u.interp_func_params.refconPV, extra->u.interp_func_params.left_arbH, extra->u.interp_func_params.right_arbH, extra->u.interp_func_params.tF, extra->u.interp_func_params.interpPH);
				break;
			case PF_Arbitrary_COMPARE_FUNC:
				err = ArbCompare(in_data, out_data, extra->u.compare_func_params.refconPV, extra->u.compare_func_params.a_arbH, extra->u.compare_func_params.b_arbH, extra->u.compare_func_params.compareP);
				break;
			// these are necessary for copying and pasting keyframes
			// for now, we better not be called to do this
			case PF_Arbitrary_PRINT_SIZE_FUNC:
				assert(FALSE); //err = ArbPrintSize(in_data, out_data, extra->u.print_size_func_params.refconPV, extra->u.print_size_func_params.arbH, extra->u.print_size_func_params.print_sizePLu);
				break;
			case PF_Arbitrary_PRINT_FUNC:
				assert(FALSE); //err = ArbPrint(in_data, out_data, extra->u.print_func_params.refconPV, extra->u.print_func_params.print_flags, extra->u.print_func_params.arbH, extra->u.print_func_params.print_sizeLu, extra->u.print_func_params.print_bufferPC);
				break;
			case PF_Arbitrary_SCAN_FUNC:
				assert(FALSE); //err = ArbScan(in_data, out_data, extra->u.scan_func_params.refconPV, extra->u.scan_func_params.bufPC, extra->u.scan_func_params.bytes_to_scanLu, extra->u.scan_func_params.arbPH);
				break;
		}
	}
	
	
	return err;
}

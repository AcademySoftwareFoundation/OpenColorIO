
//
//	mmm...our custom UI
//
//

#include "OpenColorIO_AE.h"

#include "OpenColorIO_AE_Dialogs.h"

#include "DrawbotBot.h"

static PF_Err
DrawEvent(	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*event_extra)
{
	PF_Err			err		=	PF_Err_NONE;
	
	//AEGP_SuiteHandler suites(in_data->pica_basicP);
	

	event_extra->evt_out_flags = 0;
	

	if (!(event_extra->evt_in_flags & PF_EI_DONT_DRAW) && params[OCIO_DATA]->u.arb_d.value != NULL)
	{
		if(event_extra->effect_win.area == PF_EA_CONTROL)
		{
			ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(params[OCIO_DATA]->u.arb_d.value);
			SequenceData *extract_status = (SequenceData *)PF_LOCK_HANDLE(in_data->sequence_data);
		
		
			DrawbotBot bot(in_data->pica_basicP, event_extra->contextH);
			
			
			bot.MoveTo(50, 50);
			bot.DrawString((char *)arb_data->path,
							kDRAWBOT_TextAlignment_Default,
							kDRAWBOT_TextTruncation_PathEllipsis,
							event_extra->effect_win.current_frame.right - 50);
			
			
			event_extra->evt_out_flags = PF_EO_HANDLED_EVENT;

			PF_UNLOCK_HANDLE(params[OCIO_DATA]->u.arb_d.value);
			PF_UNLOCK_HANDLE(in_data->sequence_data);
		}
	}

	return err;
}


static PF_Err
DoClick ( 
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*event_extra )
{
	PF_Err			err		=	PF_Err_NONE;
	
	ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(params[OCIO_DATA]->u.arb_d.value);
	SequenceData *seq_data = (SequenceData *)PF_LOCK_HANDLE(in_data->sequence_data);
	
	ExtensionMap extensions;
	
	for(int i=0; i < OCIO::FileTransform::getNumFormats(); i++)
	{
		const char *extension = OCIO::FileTransform::getFormatExtensionByIndex(i);
		const char *format = OCIO::FileTransform::getFormatNameByIndex(i);
	
		extensions[ extension ] = format;
	}
	
	
	bool result = OpenFile((char *)arb_data->path, ARB_PATH_LEN, extensions, NULL);
	
	
	if(result)
	{
		params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
		
		if(seq_data->context)
		{
			delete seq_data->context;
			
			seq_data->context = NULL;
		}
		
		try
		{
			seq_data->context = new OCIO_Context(arb_data);
		}
		catch(...)
		{
			// this is probably not necessary
			if(seq_data->context)
			{
				delete seq_data->context;
				
				seq_data->context = NULL;
			}
		}
	}
	
	
	PF_UNLOCK_HANDLE(params[OCIO_DATA]->u.arb_d.value);
	PF_UNLOCK_HANDLE(in_data->sequence_data);
	
	event_extra->evt_out_flags = PF_EO_HANDLED_EVENT;
	
	return err;
}


PF_Err
HandleEvent ( 
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*extra )
{
	PF_Err		err		= PF_Err_NONE;
	
	if (!err) 
	{
		switch (extra->e_type) 
		{
			case PF_Event_DRAW:
				err =	DrawEvent(in_data, out_data, params, output, extra);
				break;
			
			case PF_Event_DO_CLICK:
				err = DoClick(in_data, out_data, params, output, extra);
				break;
				
			case PF_Event_ADJUST_CURSOR:
			#ifdef MAC_ENV
				#if PF_AE_PLUG_IN_VERSION >= PF_AE100_PLUG_IN_VERSION
				SetMickeyCursor(); // the cute mickey mouse hand
				#else
				SetThemeCursor(kThemePointingHandCursor);
				#endif
				extra->u.adjust_cursor.set_cursor = PF_Cursor_CUSTOM;
			#else
				extra->u.adjust_cursor.set_cursor = PF_Cursor_FINGER_POINTER;
			#endif
				extra->evt_out_flags = PF_EO_HANDLED_EVENT;
				break;
		}
	}
	
	return err;
}

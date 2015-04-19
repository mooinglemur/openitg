#include "global.h"
#include "RageUtil.h"
#include "RageSoundReader_Resample_Good.h"

extern "C"
{
 #include <libavutil/opt.h>
 #include <libavresample/avresample.h>
}

#define channels source->GetNumChannels()

RageSoundReader_Resample_Good::RageSoundReader_Resample_Good()
{
	source = NULL;
	samplerate = -1;
	BufSamples = 0;
	eof = false;
	HighQuality = false;
}

/* Call this if the input position is changed or reset. */
void RageSoundReader_Resample_Good::Reset()
{
	BufSamples = 0;
	eof = false;

	/* Flush the resampler. */
	for( unsigned i = 0; i < resamplers.size(); ++i )
	{
		resample_channel &r = resamplers[i];
		if( r.resamp ) 
		{
			avresample_close( r.resamp );
			avresample_open( r.resamp );
		} 
		else 
		{	
			r.resamp = avresample_alloc_context();
			av_opt_set_int( r.resamp, "in_channel_layout", AV_CH_LAYOUT_MONO, 0 );
			av_opt_set_int( r.resamp, "out_channel_layout", AV_CH_LAYOUT_MONO, 0 );
			av_opt_set_int( r.resamp, "in_sample_rate", source->GetSampleRate(), 0 );
			av_opt_set_int( r.resamp, "out_sample_rate", samplerate, 0 );
			av_opt_set_int( r.resamp, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0 );
			av_opt_set_int( r.resamp, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0 );
			avresample_open( r.resamp );
		}
	}
}


/* Call this if the sample factor changes. */
void RageSoundReader_Resample_Good::ReopenResampler()
{
	for( unsigned i = 0; i < resamplers.size(); ++i )
	{
		resample_channel &r = resamplers[i];
		if( r.resamp )
		{
			avresample_close( r.resamp );
			av_opt_set_int( r.resamp, "in_sample_rate", source->GetSampleRate(), 0 );
			av_opt_set_int( r.resamp, "out_sample_rate", samplerate, 0 );
			avresample_open( r.resamp );
		} 
		else 
		{
			r.resamp = avresample_alloc_context();
			av_opt_set_int( r.resamp, "in_channel_layout", AV_CH_LAYOUT_MONO, 0 );
			av_opt_set_int( r.resamp, "out_channel_layout", AV_CH_LAYOUT_MONO, 0 );
			av_opt_set_int( r.resamp, "in_sample_rate", source->GetSampleRate(), 0 );
			av_opt_set_int( r.resamp, "out_sample_rate", samplerate, 0 );
			av_opt_set_int( r.resamp, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0 );
			av_opt_set_int( r.resamp, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0 );
			avresample_open( r.resamp );
		}

	}
}

void RageSoundReader_Resample_Good::Open(SoundReader *source_)
{
	source = source_;
	ASSERT(source);

	samplerate = source->GetSampleRate();

	for( unsigned i = 0; i < source->GetNumChannels(); ++i )
		resamplers.push_back( resample_channel() );
}


RageSoundReader_Resample_Good::~RageSoundReader_Resample_Good()
{
	for( unsigned i = 0; i < resamplers.size(); ++i )
	{
		if( resamplers[i].resamp )
			avresample_free( &resamplers[i].resamp );
	}

	delete source;
}

float RageSoundReader_Resample_Good::GetFactor() const
{
	return float(samplerate) / source->GetSampleRate();
}

void RageSoundReader_Resample_Good::SetSampleRate(int hz)
{
	samplerate = hz;
	ReopenResampler();
}

int RageSoundReader_Resample_Good::GetLength() const
{
	return source->GetLength();
}

int RageSoundReader_Resample_Good::GetLength_Fast() const
{
	return source->GetLength_Fast();
}

int RageSoundReader_Resample_Good::SetPosition_Accurate(int ms)
{
	Reset();
	return source->SetPosition_Accurate(ms);
}

int RageSoundReader_Resample_Good::SetPosition_Fast(int ms)
{
	Reset();
	return source->SetPosition_Fast(ms);
}

bool RageSoundReader_Resample_Good::FillBuf()
{
	int samples_free = BUFSIZE-BufSamples;
	if( eof )
		return true;
	if( !samples_free )
		return true;

	const int bytes_per_frame = sizeof(int16_t)*channels;
	int16_t *tmpbuf = (int16_t *) alloca( BUFSIZE*bytes_per_frame );
	int cnt = source->Read( (char *) tmpbuf, samples_free * bytes_per_frame );

	if( cnt == -1 )
	{
		SetError(source->GetError());
		return false;
	}

	if( cnt < samples_free * bytes_per_frame )
		eof = true;

	cnt /= bytes_per_frame;

	for( unsigned i = 0; i < channels; ++i )
	{
		resample_channel &r = resamplers[i];
		for( int s = 0; s < cnt; ++s )
			r.inbuf[s+BufSamples] = tmpbuf[s*resamplers.size()+i];
	}
	BufSamples += cnt;
	return true;
}

int RageSoundReader_Resample_Good::Read(char *bufp, unsigned len)
{
	int16_t *buf = (int16_t *) bufp;
	len /= sizeof(int16_t); /* bytes -> samples */
	const float factor = GetFactor();


	int bytes_read = 0;

	while( 1 )
	{
		int samples_used = 0, samples_output = 0;
		if ( BufSamples || eof )
		{
			for( unsigned i = 0; i < channels; ++i )
			{
				resample_channel &r = resamplers[i];
				ASSERT( r.resamp );
	//				float outbuf[BUFSIZE];
	//				samples_output = resample_process( r.resamp,
	//						factor,
	//						r.inbuf, BufSamples,
	//						eof,
	//						&samples_used,
	//						outbuf, len/channels);
	//

				uint8_t *outbuf;
				uint8_t *inbuf_cast = (uint8_t *)r.inbuf;
				int out_linesize;
				samples_output = len/channels;
				av_samples_alloc( &outbuf, &out_linesize, 1, samples_output, AV_SAMPLE_FMT_S16, 0 );

				if( !BufSamples && eof )
					samples_output = avresample_read( r.resamp, &outbuf, samples_output );
				else
					samples_output = avresample_convert( r.resamp, &outbuf, out_linesize, samples_output, &inbuf_cast, samples_output, BufSamples );
			
				if( samples_output < 0 )
					RageException::Throw( "Unexpected resample_process return value: %d", samples_output );

			//	memmove( r.inbuf, &r.inbuf[samples_used], sizeof(int16_t) * (BufSamples-samples_used) );

				for( int s = 0; s < samples_output; ++s )
				{
					buf[s*channels+i] = ((int16_t *)outbuf)[s];
				}
				av_freep(&outbuf);
			}
		}

//		BufSamples -= samples_used;
		BufSamples = 0; // Always slurps all

		if( !samples_output )
		{
			if( !len )
				return bytes_read; /* buffer full */
			if( eof )
				return bytes_read; /* EOF and we're completely flushed */
			if( !FillBuf() )
				return -1; /* source error */
		}
		len -= samples_output*channels;
		buf += samples_output*channels;
		bytes_read += samples_output*channels*sizeof(int16_t);
	}
}

SoundReader *RageSoundReader_Resample_Good::Copy() const
{
	printf("Debug: Copy!\n");
	SoundReader *new_source = source->Copy();
	RageSoundReader_Resample_Good *ret = new RageSoundReader_Resample_Good;

	for( unsigned i = 0; i < channels; ++i )
	{
		const resample_channel &r = resamplers[i];
		ASSERT( r.resamp );
		ret->resamplers.push_back( resample_channel() );
		ret->resamplers[i].resamp = avresample_alloc_context();
		av_opt_set_int( ret->resamplers[i].resamp, "in_channel_layout", AV_CH_LAYOUT_MONO, 0 );
		av_opt_set_int( ret->resamplers[i].resamp, "out_channel_layout", AV_CH_LAYOUT_MONO, 0 );
		av_opt_set_int( ret->resamplers[i].resamp, "in_sample_rate", source->GetSampleRate(), 0 );
		av_opt_set_int( ret->resamplers[i].resamp, "out_sample_rate", samplerate, 0 );
		av_opt_set_int( ret->resamplers[i].resamp, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0 );
		av_opt_set_int( ret->resamplers[i].resamp, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0 );
		avresample_open( ret->resamplers[i].resamp );

		memcpy( ret->resamplers[i].inbuf, r.inbuf, sizeof(r.inbuf));
	}
	ret->source = new_source;
	ret->HighQuality = HighQuality;
	ret->samplerate = samplerate;
	ret->BufSamples = BufSamples;
	ret->eof = eof;

//	ret->Open(new_source);
//	ret->SetSampleRate(samplerate);
	return ret;
}

/*
 * Copyright (c) 2003 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */


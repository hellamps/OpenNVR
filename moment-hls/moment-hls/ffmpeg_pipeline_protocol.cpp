

#include "ffmpeg_pipeline_protocol.h"

extern "C"
{
    #ifndef INT64_C
        #define INT64_C(c) (c ## LL)
        #define UINT64_C(c) (c ## ULL)
    #endif

    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavformat/url.h>
}


namespace MomentHls
{

static LogGroup libMary_logGroup_hls_pipe   ("mod_hls.pprotocol",   LogLevel::D);
std::string g_ProtocolName = "pipelineprotocol";

std::string CPipelineProtocol::GetProtocolName(void)
{
	return g_ProtocolName;
}

CPipelineProtocol::CPipelineProtocol()
{
	logD(hls_pipe, _func_);
}

CPipelineProtocol::~CPipelineProtocol()
{
	logD(hls_pipe, _func_, "id = ", m_Info.id);
	Deinit();
}

void CPipelineProtocol::Deinit()
{
	//logD(hls_pipe, _func_);
	m_Info = CTsMuxer::TsSegmentInfo();
}

bool CPipelineProtocol::Init(URLContext * pContext, const char * pFileName, int flags)
{
	logD(hls_pipe, _func_);
	if(!CTsMuxer::GetTsSegmentInfo(pFileName, m_Info))
	{
		logE(hls_pipe, _func_, "GetTsSegmentInfo failed");
		return false;
	}

	pContext->is_streamed = 1;

	return true;
}

bool CPipelineProtocol::Write( const unsigned char *buf, int size )
{
	return CTsMuxer::WriteData(m_Info, buf, size);
}

/************************************************************************/
/*                   ffmpeg protocol implementation                     */
/************************************************************************/

static int PipeOpen(URLContext *h, const char *filename, int flags)
{
	logD(hls_pipe, _func_, "filename = ", filename);
	CPipelineProtocol * pPipeProtocol = new (std::nothrow) CPipelineProtocol();

	if(!pPipeProtocol)
	{
		logE(hls_pipe, _func_, "out of memory");
		return AVERROR(ENOMEM);
	}

	if(pPipeProtocol->Init(h, filename, flags))
	{
		h->priv_data = reinterpret_cast<void *>(pPipeProtocol);
	}
	else
	{
		logE(hls_pipe, _func_, "Protocol->Init failed");
		delete pPipeProtocol;
		return AVERROR(errno);
	}

	return 0;
}

static int PipeWrite(URLContext *h, const unsigned char *buf, int size)
{
	//logD(hls_pipe, _func_);
	CPipelineProtocol * pPipeProtocol = reinterpret_cast<CPipelineProtocol *>(h->priv_data);

	if(!pPipeProtocol)
	{
		logE(hls_pipe, _func_, "!pPipeProtocol");
		return AVERROR(errno);
	}

	return pPipeProtocol->Write(buf, size) ? 0 : AVERROR(errno);
}

static int PipeClose(URLContext *h)
{
	logD(hls_pipe, _func_);
	CPipelineProtocol * pPipeProtocol = reinterpret_cast<CPipelineProtocol *>(h->priv_data);

	if(!pPipeProtocol)
	{
		logE(hls_pipe, _func_, "!pPipeProtocol");
		return AVERROR(errno);
	}

	delete pPipeProtocol;
	h->priv_data = NULL;

	return 0;
}

extern "C"
{

URLProtocol ff_buffer_protocol =
{
	g_ProtocolName.c_str(),		// const char *name;
	PipeOpen,					// int     (*url_open)( URLContext *h, const char *url, int flags);
	NULL,						// int     (*url_open2)(URLContext *h, const char *url, int flags, AVDictionary **options);
	NULL,						// int     (*url_read)( URLContext *h, unsigned char *buf, int size);
	PipeWrite,					// int     (*url_write)(URLContext *h, const unsigned char *buf, int size);
	NULL,						// int64_t (*url_seek)( URLContext *h, int64_t pos, int whence);
	PipeClose,					// int     (*url_close)(URLContext *h);
	NULL,						// struct URLProtocol *next;
	NULL,						// int (*url_read_pause)(URLContext *h, int pause);
	NULL,						// int64_t (*url_read_seek)(URLContext *h, int stream_index, int64_t timestamp, int flags);
	NULL,						// int (*url_get_file_handle)(URLContext *h);
	NULL,						// int (*url_get_multi_file_handle)(URLContext *h, int **handles, int *numhandles);
	NULL,						// int (*url_shutdown)(URLContext *h, int flags);
	sizeof(CPipelineProtocol *),// int priv_data_size;
	NULL,						// const AVClass *priv_data_class;
	0,							// int flags;
	NULL						// int (*url_check)(URLContext *h, int mask);
};

}

}   // namespace MomentHls


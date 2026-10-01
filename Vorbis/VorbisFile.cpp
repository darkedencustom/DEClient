#include "VorbisFile.h"
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

//������ �ҽ��κ��� ���� ���̺귯�� ������ ���� ������ ���۷�
//�����͸� �����ϰ� �˻��ϴ� ������ �۾��� �����ϴ� �ݹ��Լ�
size_t read_func_stream(void * ptr, size_t size,
						size_t nmemb, void * datasoure)
{
	if(!size || !nmemb)
	{
		return 0;
	}
	
	FileStream * pSrc = (FileStream *)datasoure;
	size_t nBytesToRead = size * nmemb;

	size_t nRead;
	if(FAILED(pSrc->Read(ptr, nBytesToRead, &nRead)))
	{
		return 0;
	}
	
	return (UINT32)nRead;
}

int seek_func_stream(void * datasource, ogg_int64_t offset, 
					 int whence)
{
	FileStream * pSrc = (FileStream *)datasource;
	LARGE_INTEGER	 liOffset;
	ULARGE_INTEGER	 lnNewPos;
	liOffset.QuadPart = offset;
	
	if(FAILED(pSrc->Seek(liOffset, whence, &lnNewPos)))
		return -1;
	
	return 0;
}

int close_func_stream(void * datasource)
{
	return 0;
}

long tell_func_stream(void * datasource)
{
	FileStream * pSrc = (FileStream *)datasource;	
	LARGE_INTEGER dlibMove;
	dlibMove.QuadPart = 0;
	ULARGE_INTEGER nNewPos;
	pSrc->Seek(dlibMove, SEEK_CUR, &nNewPos);	
	
	return static_cast<long>(nNewPos.QuadPart);
}

VorbisFile::VorbisFile()
{
	m_bOpen = false;
	m_bEOF = false;
	m_pVorbisInfo = NULL;
	m_nNumSamples = 0;
	m_szLastError[0] = 0;
}

void VorbisFile::SetError(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(m_szLastError, sizeof(m_szLastError) - 1, fmt, args);
	va_end(args);
	m_szLastError[sizeof(m_szLastError) - 1] = 0;
}

VorbisFile::~VorbisFile()
{
	Close();
}

bool VorbisFile::Open(std::string sFileName)
{
	if(m_bOpen)
	{
		SetError("Open: already open");
		return false;
	}

	//���Ͽ���
	if(FAILED(m_StreamSrc.Open(sFileName)))
	{
		char cwd[MAX_PATH] = "?";
		GetCurrentDirectoryA(MAX_PATH, cwd);
		SetError("cannot open file '%s' (errno %d: %s) cwd='%s'", sFileName.c_str(), errno, strerror(errno), cwd);
		return false;
	}

	//������ �ݹ��Լ��� ����
	ov_callbacks cb;
	cb.read_func = &read_func_stream;
	cb.seek_func = &seek_func_stream;
	cb.close_func = &close_func_stream;
	cb.tell_func = &tell_func_stream;
	
	//Ogg ��Ʈ ��Ʈ���� ����.
	int res = ov_open_callbacks((void*)&m_StreamSrc, &m_VorbisFile, NULL, 0, cb);		
	if ( res < 0 )
	{
		SetError("ov_open_callbacks failed (%d): '%s' is not a playable Ogg Vorbis stream", res, sFileName.c_str());
		return false;
	}

	m_bOpen = true;

	if(!GetSreamInfo())
	{
		if (!m_szLastError[0])
			SetError("GetSreamInfo failed for '%s'", sFileName.c_str());
		return false;
	}

	return true;
}

bool VorbisFile::GetSreamInfo()
{
	if(!m_bOpen)
	{
		return false;
	}

	//Vorbis ���� ������ �����´�.
	m_pVorbisInfo = ov_info(&m_VorbisFile, -1);
	if(!m_pVorbisInfo)
	{
		SetError("ov_info returned NULL");
		return false;
	}

	//���ϳ� PCM ������ ������ �����´�.
	m_nNumSamples = (UINT32)ov_pcm_total(&m_VorbisFile, -1);
	
	//WAVEFORMAT ����ü�� �����Ѵ�.
	m_WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;				//��������
	m_WaveFormatEx.nChannels =	m_pVorbisInfo->channels;		//���̺� ���ϳ��� �� ä�μ�
	m_WaveFormatEx.nSamplesPerSec = m_pVorbisInfo->rate;		//�ʴ� ���ü�(Hertz)	
	m_WaveFormatEx.wBitsPerSample = 16;							//�ϳ��� ������ ���� �� ��Ʈ�� ��
	m_WaveFormatEx.nBlockAlign = 
		m_WaveFormatEx.nChannels * m_WaveFormatEx.wBitsPerSample / 8;  
	//�ϳ��� ���ñ׷��� �����ϴ�ũ��(����Ʈ)
	m_WaveFormatEx.nAvgBytesPerSec = 
		m_WaveFormatEx.nSamplesPerSec * m_WaveFormatEx.nBlockAlign;
	//���̺� ������ �ʿ�� �ϴ� 1�ʴ� ����Ʈ���� �뿪��
	m_WaveFormatEx.cbSize = 0;									//���̺� ������ Ȯ���Ϸ��� ��쿡 ���
	
	return true;
}

UINT32 VorbisFile::GetSize()
{
	return m_nNumSamples * m_WaveFormatEx.nChannels *
		m_WaveFormatEx.wBitsPerSample / 8;   
}

bool VorbisFile::Read(BYTE * pBuffer, UINT32 dwSizeToRead, 
			  UINT32 * pdwSizeRead)
{
	if(!m_bOpen)
	{
		return false;
	}

	char *pCurBuffer = (char *)pBuffer;
	UINT32 nBytesRead = 0;
	int iSection = 0;

	while( ( nBytesRead < dwSizeToRead ) && !m_bEOF )
	{
		INT32 iRet = ov_read(&m_VorbisFile, pCurBuffer,
			dwSizeToRead - nBytesRead, &iSection);

		if( iRet == 0 || iSection != 0 )
		{
			if (iSection != 0)
				SetError("EOF forced by section=%d iRet=%d", iSection, (int)iRet);
			else
				SetError("EOF: ov_read returned 0");
			m_bEOF = true;				// iRet�� 0 �̸� ������ ���� ������ ����
		}
		else if( iRet < 0 )
		{
			return false;				// iRet�� 0 ���� ������ ������ �߻��� ����
		}

		nBytesRead += iRet;
		pCurBuffer += iRet;	
	}
	
	*pdwSizeRead = nBytesRead;
	
	return true;
}

bool VorbisFile::Reset()
{
	if(!m_bOpen)
	{
		return false;
	}

	//�ٽ� �б� �۾��� �����ϱ� ���� ������ ��ó������ �̵��Ѵ�.
	m_bEOF = false;
	ov_pcm_seek(&m_VorbisFile, 0);

	return true;
}

bool VorbisFile::Close()
{
	if(!m_bOpen)
	{
		return false;
	}
	
	//Vorbis ���� ��Ʈ���� �ݾƹ�����.
	ov_clear(&m_VorbisFile);

	return true;
}

WAVEFORMATEX * VorbisFile::GetFormat()
{
	return &m_WaveFormatEx;
}
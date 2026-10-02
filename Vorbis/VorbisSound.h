#ifndef __VORBISSOUND_H__
#define __VORBISSOUND_H__
//-----------------------------------------------------------------------------

#include <dsound.h>
#include "Define.h"
#include <string>

class VorbisFile;

class VorbisSound
{
public:
	VorbisSound();
	~VorbisSound();

	void		Init( LPDIRECTSOUND lpdSound );
	bool		Open(std::string sFileName);

	BYTE		GetSilenceData();	
	void		ServiceBuffer();
	bool		Play();
	bool		Stop();

	bool		SetVolume(int volume);
	int			GetVolume();

	// Why the last Open()/Play()/SetVolume() failed, for the caller to log.
	const char*	GetLastErrorText() const { return m_szLastError; }
	const WAVEFORMATEX& GetFormat() const { return m_WaveFormat; }

protected:
	bool		DoLoad();
	bool	    LoadSource(UINT32 & nBufferSize);
	bool		FillBuffer();
	void		SetError(const char* fmt, ...);

protected:
	std::string				m_sFileName;
	bool					m_bIsInit;
	LPDIRECTSOUND			m_lpdSound;
	LPDIRECTSOUNDBUFFER		m_pDSBuffer;
	WAVEFORMATEX			m_WaveFormat;
	DSBCAPS					m_Caps;

	VorbisFile *			m_pVobisFile;

	UINT32					m_nDataCursor;
	UINT32					m_nBytesPlayed;
	UINT32					m_nLastReadPos;

	char					m_szLastError[256];
};

//-----------------------------------------------------------------------------
#endif	__VORBISSOUND_H__
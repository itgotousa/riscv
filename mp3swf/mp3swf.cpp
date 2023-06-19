#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#define WIN32_LEAN_AND_MEAN
#include <tchar.h>
#include <windows.h>
#include <shellapi.h>
#include <io.h>
#include <wchar.h>
#include <fcntl.h>
#include <d2d1.h>
#include <dwrite.h>
#include <xaudio2.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#include "libmp3dec.h"

#define PRINT_TAG   1
#define SWF_TIMER   123
#define SWF_SECOND  124

#define S8      int8_t
#define U8      uint8_t
#define S16     int16_t
#define U16     uint16_t
#define S32     int32_t
#define U32     uint32_t

#define	sndCompressMP3  (0x20)
#define sndRate5K	5512
#define sndRate11K	11025
#define sndRate22K	22050
#define sndRate44K	44100
#define sndMono		(0x00)
#define sndStereo	(0x01)
#define snd8Bit     (0x01)
#define snd16Bit    (0x02)

#define ST_END                  0
#define ST_SHOWFRAME            1
#define ST_DOACTION             12
#define ST_SOUNDSTREAMBLOCK     19
#define ST_PLACEOBJECT2         26 /* The new style place w/ alpha color transform and name. */
#define ST_REMOVEOBJECT2        28 /* A more compact remove object that omits the character tag (just depth). */
#define ST_DEFINESHAPE3         32 /* A shape V3 includes alpha values. */
#define ST_DEFINEBITSLOSSLESS2  36 /* A lossless bitmap with alpha info. */
#define ST_FRAMELABEL           43 /* A string label for the current frame. */
#define ST_SOUNDSTREAMHEAD2     45 /* For lossless streaming sound, should not have needed this... */
#define ST_FILEATTRIBUTES		69 /* version 8 (required)- */
#define ST_METADATA				77 /* version 8 */
#define ST_CAMTASIA           	700 /* to identify generator software */
#if 0
U8   sign0800[9] = { 0x78, 0x00, 0x07, 0xd0, 0x00, 0x00, 0x17, 0x70, 0x00 };
U8  sign1024[9] = { 0x80, 0x00, 0x02, 0x80, 0x00, 0x00, 0x01, 0xE0, 0x00 };
#endif
static U16  tagTab[1024]; /* global variables : tag table */
static const char* tagName[1024];
static U8* streamSWF = NULL;

static void InitTagTab()
{
    int i;
    for (i = 0; i < 1024; i++) tagTab[i] = 0;
    tagTab[ST_END] = 1;
    tagTab[ST_SHOWFRAME] = 1;
    tagTab[ST_SOUNDSTREAMBLOCK] = 1;
    tagTab[ST_DOACTION] = 1;
    tagTab[ST_PLACEOBJECT2] = 1;
    tagTab[ST_DEFINESHAPE3] = 1;
    tagTab[ST_DEFINEBITSLOSSLESS2] = 1;
    tagTab[ST_FRAMELABEL] = 1;
    tagTab[ST_SOUNDSTREAMHEAD2] = 1;
    tagTab[ST_CAMTASIA] = 1;
    tagTab[ST_FILEATTRIBUTES] = 1;
    tagTab[ST_METADATA] = 1;
    tagTab[ST_REMOVEOBJECT2] = 1;

    for (i = 0; i < 1024; i++) tagName[i] = nullptr;
    tagName[ST_END] = "ST_END";
    tagName[ST_SHOWFRAME] = "ST_SHOWFRAME";
    tagName[ST_SOUNDSTREAMBLOCK] = "ST_SOUNDSTREAMBLOCK";
    tagName[ST_DOACTION] = "ST_DOACTION";
    tagName[ST_PLACEOBJECT2] = "ST_PLACEOBJECT2";
    tagName[ST_DEFINESHAPE3] = "ST_DEFINESHAPE3";
    tagName[ST_DEFINEBITSLOSSLESS2] = "ST_DEFINEBITSLOSSLESS2";
    tagName[ST_FRAMELABEL] = "ST_FRAMELABEL";
    tagName[ST_SOUNDSTREAMHEAD2] = "ST_SOUNDSTREAMHEAD2";
    tagName[ST_CAMTASIA] = "ST_CAMTASIA";
    tagName[ST_FILEATTRIBUTES] = "ST_FILEATTRIBUTES";
    tagName[ST_METADATA] = "ST_METADATA";
    tagName[ST_REMOVEOBJECT2] = "ST_REMOVEOBJECT2";
}

const int kBufSize = 1024 * 8;

class CMp3Decomp
{
private:
    U8*             src;               // mpeg src data
    U32				len;
    int             srcIndex;           // index into the source data to read
    U8              pcmBuf[kBufSize];   // buffered output data 
    HMP3DEC			hDec;
    U8				mpa_data[1792];
    int				mpa_Index;
    int             bufLength;          // number of bytes last filled with
    int             bufIndex;           // index into the out buffer
    //ScriptThread* thread;

    // get buffered data in bufOut. Return number of bytes written
    long GetBufferedData(S8* dst, S32 n)
    {
        U8* TmPPtr;
        int bytesToCopy = n < (bufLength - bufIndex) ? n : (bufLength - bufIndex);

        // dst can bee 0 during seeking
        TmPPtr = &pcmBuf[bufIndex];
        if (dst)
            memcpy(dst, TmPPtr, bytesToCopy);

        // adjust the callers index and the current buff pointer
        bufIndex += bytesToCopy;

        if (bufIndex >= bufLength)
            bufIndex = bufLength = 0;

        return bytesToCopy;

        return 0;
    }

public:
    CMp3Decomp()
    {
        src = nullptr;            // mpeg src data
        len = 0;
        bufLength = 0;      // number of bytes last filled with
        bufIndex = 0;       // index into the out buffer
        srcIndex = 0;
        mpa_Index = 0;
        hDec = MP3_decode_init();
    }

    ~CMp3Decomp()
    {
        if (nullptr != hDec)
            MP3_decode_close(hDec);
    }

    int Setup(U8* mp3, U32 bytes, bool reset = false)
    {
        src = mp3;
        len = bytes;
        srcIndex = 0;
        mpa_Index = 0;

        bufLength = 0;      // number of bytes last filled with
        bufIndex = 0;       // index into the out buffer
        //this->thread = thread;

        if (reset)
        {
            if (hDec)
                MP3_decode_close(hDec);
            hDec = MP3_decode_init();
        }

        return 0;
    }
    int Decompress(S16* dstWord, S32 nSamples, U8 bytesPerSample = 4) // return number of good samples
    {
        long            bytesCleared;
        S32             nBytes = nSamples * bytesPerSample; // thread->BytesPerBlock();
        S8* dstByte = (S8*)dstWord;

        int             nDataLen;
        int				CopyLen = 0;
        uint32_t		nFrameSize = 0;
        uint32_t		nFrameLen = 0;

        // check first if there is any buffered data
        if ((bytesCleared = GetBufferedData(dstByte, nBytes)) == nBytes)
            goto exit_gracefully;
        else if (nullptr == hDec)
        {
            // fill with silence
            if (dstByte)
                memset(dstByte, 0x00, nBytes);

            goto exit_gracefully;
        }
        else
        {
            nBytes -= bytesCleared;

            // dst byte can be zero during seeking
            if (dstByte)
                dstByte += bytesCleared;
        }

        // Loop until error occurs or bytes are saved for next pass
        while (nBytes > 0)
        {
            if (srcIndex >= len)
            {
                break;
            }

            CopyLen = 0;
            if (mpa_Index < 4)
            {
                CopyLen = 4 - mpa_Index;
                memcpy(mpa_data + mpa_Index, src, 4);
                mpa_Index = 4;
            }
            src += CopyLen;
            srcIndex += CopyLen;
            nFrameSize = MP3_GetFrameSize(*(uint32_t*)mpa_data);

            if ((int)(len - srcIndex) < (int)(nFrameSize - mpa_Index))
            {
                memcpy(mpa_data + mpa_Index, src, len - srcIndex);
                src += len - srcIndex;
                srcIndex = len;
                mpa_Index += len - srcIndex;
                break;
            }

            memcpy(mpa_data + mpa_Index, src, nFrameSize - mpa_Index);
            src += nFrameSize - mpa_Index;
            srcIndex += nFrameSize - mpa_Index;
            mpa_Index = 0;
            nFrameLen = MP3_decode_frame(hDec, pcmBuf, &nDataLen, mpa_data, nFrameSize);
            if (nDataLen > 0)
            {
                bufLength = nDataLen;
                bytesCleared = GetBufferedData(dstByte, nBytes);
                nBytes -= bytesCleared;

                // dst byte can be zero during seeking
                if (dstByte)
                    dstByte += bytesCleared;
            }
        }

    exit_gracefully:
        return 0;
    }
};

typedef S32 SCOORD;

typedef struct SRECT
{
    SCOORD xmin;
    SCOORD xmax;
    SCOORD ymin;
    SCOORD ymax;
} SRECT;

typedef S32 SFIXED;
typedef struct MATRIX
{
    SFIXED a;
    SFIXED b;
    SFIXED c;
    SFIXED d;
    SCOORD tx;
    SCOORD ty;
} MATRIX;

class SParser 
{
public:
    SParser()
    {
        m_script = nullptr;
        m_pos = 0;
        m_tagStart = 0;
        m_totalBytes = 0;
        m_currBytes = 0;
        m_bitBuf = 0;
        m_bitPos = 0;
        m_tagLen = 0;
        m_tagEnd = 0;
        m_tagPos = 0;
        m_tagId = 0xFFFF; // invalid tag ID
        m_tagType = 'X';
    }
    ~SParser() {}

    bool HasData() { return (nullptr != m_script); }
    const U8* GetBaseAddr() { return m_script; }
    U8* GetCurrentAddr() { return (U8*)(m_script + m_pos); }
    U32 GetPosition() { return m_pos; }
    U32 GetTagPosition() { return m_tagPos; }
    U8  GetTagType() { return m_tagType; }
    U32 GetTagEnd() { return m_tagEnd; }
    U32 GetTagLen() { return m_tagLen; }
    U32 GetTotalBytes() { return m_totalBytes; }
    U32 GetCurrBytes() { return m_currBytes; }

    U8 GetByte() { return m_script[m_pos++]; }
    U16 Get2Bytes()
    {
        const U8* s = m_script + m_pos;
        m_pos += 2;
        return (U16)s[0] | ((U16)s[1] << 8);
    }
    U32 Get4Bytes()
    {
        const U8* s = m_script + m_pos;
        m_pos += 4;
        return (U32)s[0] | ((U32)s[1] << 8) | ((U32)s[2] << 16) | ((U32)s[3] << 24);
    }

    // return -1 if the tag is not yet loaded given len data
    int GetTag()
    {
        if (0 == m_totalBytes || 0 == m_currBytes) return (-1);
        m_tagPos = m_pos;
        // we need more data before we can process this tag. each tag is 2 bytes
        if (m_currBytes - m_pos < 2) return (-1);

        U16 tagCL = Get2Bytes(); // tagCL is the code and length of this tag
        m_tagLen = tagCL & 0x3F;
        m_tagType = 'S'; // short tag type
        if (0x3F == m_tagLen) // it is a long tag 
        {
            if (m_currBytes - m_pos < 4)
            {
                // we need more data before we can process this tag
                m_pos = m_tagPos; // go back to the starting point of this tag
                return (-1);
            }
            m_tagType = 'L'; // long tag type
            m_tagLen = Get4Bytes();
        }
        m_tagEnd = m_pos + m_tagLen;
        if (m_tagEnd > m_currBytes)
        {
            // we need more data before we can process this tag
            m_pos = m_tagPos; // go back to the starting point of this tag
            return (-1);
        }
        m_tagId = (tagCL >> 6) & 0x03FF;
        return m_tagId;
    }

    void Attach(const U8* stream, U32 start, U32 end = 0x7FFFFFFF)
    {
        m_script = stream;
        m_pos = start;
        m_totalBytes = m_currBytes = end;
    }
    void GotoNextTag() { m_pos = m_tagEnd; }
    U32 GetNextTag() { return m_tagEnd; }

    void GetRect(SRECT* r)
    {
        m_bitPos = 0;
        m_bitBuf = 0;
        int nBits = (int)GetBits(5);
        r->xmin = GetSBits(nBits) / 20;
        r->xmax = GetSBits(nBits) / 20;
        r->ymin = GetSBits(nBits) / 20;
        r->ymax = GetSBits(nBits) / 20;
    }

    void GetMatrix(MATRIX* mat)
    {
        m_bitPos = 0;
        m_bitBuf = 0;
        // Scale terms
        if (GetBits(1))
        {
            int nBits = (int)GetBits(5);
            GetSBits(nBits);
            GetSBits(nBits);
            mat->a = mat->d = 1;
        }
        else
        {
            mat->a = mat->d = 1;
        }

        // Rotate/skew terms
        if (GetBits(1))
        {
            int nBits = (int)GetBits(5);
            GetSBits(nBits);
            GetSBits(nBits);
            mat->b = mat->c = 0;
        }
        else
        {
            mat->b = mat->c = 0;
        }

        {// Translate terms
            int nBits = (int)GetBits(5);
            mat->tx = GetSBits(nBits) / 20;
            mat->ty = GetSBits(nBits) / 20;
        }
    }

    U32 GetBits(int n)	// get n bits from the stream
    {
        U32 v = 0;
        for (;;)
        {
            int s = n - m_bitPos;
            if (s > 0) {
                // Consume the entire buffer
                v |= m_bitBuf << s;
                n -= m_bitPos;

                // Get the next buffer
                m_bitBuf = GetByte();
                m_bitPos = 8;
            }
            else {
                // Consume a portion of the buffer
                v |= m_bitBuf >> -s;
                m_bitPos -= n;
                m_bitBuf &= 0xFF >> (8 - m_bitPos);	// mask off the consumed bits
                return v;
            }
        }
    }

    S32 GetSBits(int n)	// extend the sign bit
    {
        S32 v = GetBits(n);
        if (v & (1L << (n - 1))) {
            v |= ((-1L) << n);
        }
        return v;
    }

protected:
    const U8* m_script;	/* contains the SWF stream */
    U32 m_pos;			/* the current position of script */
    U32 m_tagStart;		/* the position of the first Tag */
    U32 m_totalBytes;	/* the total bytes of this SWF stream */
    U32 m_currBytes;	/* the curernt bytes we have so far */
    // Bit Handling
    U32 m_bitBuf;
    int m_bitPos;
    // Tag Parser
    U8  m_tagType; // L(long) or S(short)
    U32 m_tagEnd;
    U32 m_tagLen;	// the legnth of the current tag
    U32 m_tagId;
    U32 m_tagPos;	// the position of the current tag.
};


class ScriptThread : public SParser
{
public:
    U16  wFormatTag;
    U16  nChannels;
    U32  nSamplesPerSec;
    U32  nAvgBytesPerSec;
    U16  nBlockAlign;
    U16  wBitsPerSample;

    U8  m_format;
    SRECT m_frame;
    U16 m_frameRate;
    U16 m_frameCount;
    S16 m_frameCurrent;
    bool m_AtEnd;

public:
    ScriptThread()
    {
        wFormatTag = WAVE_FORMAT_PCM;
        nChannels = 1;
        nSamplesPerSec = 0;
        nAvgBytesPerSec = 0;
        nBlockAlign = 0;
        wBitsPerSample = snd8Bit;

        m_format = 0;
        m_frameRate = 0;
        m_frameCount = 0;
        m_frameCurrent = -1;
        m_AtEnd = false;
        m_frame = { 0 };
    }

    ~ScriptThread()
    {}

    int InitFormat(U8 format, U16 sampleCount, U16 latencySeek)
    {
        // we only support MP3 format sound in SWF
        if (sndCompressMP3 != (0xF0 & format))
            return (-1);
        m_format = sndCompressMP3;
        U8 soundRate = (0x0C & format) >> 2;
        switch (soundRate)
        {
        case 0x00:
            nSamplesPerSec = sndRate5K;
            break;
        case 0x01:
            nSamplesPerSec = sndRate11K;
            break;
        case 0x02:
            nSamplesPerSec = sndRate22K;
            break;
        case 0x03:
            nSamplesPerSec = sndRate44K;
            break;
        }
        if (sndStereo == (0x01 & format)) 
            nChannels = 2;
        else 
            nChannels = 1;
        if (snd16Bit == (0x02 & format))
            wBitsPerSample = 16; // 16-bit per sample
        else
            wBitsPerSample = 8;  // 8-bit per sample

        nBlockAlign = (U16)((nChannels * wBitsPerSample) >> 3);
        nAvgBytesPerSec = nBlockAlign * nSamplesPerSec;

        return 0;
    }

    int PushData(U8* data, U32 chunkLen)
    {
        if (nullptr == data || chunkLen < 1024 * 1024)
            return -1;

        U8* p = (U8*)data;
        U32 bytes = chunkLen;

        if ('F' != *(p + 0) || 'W' != *(p + 1) || 'S' != *(p + 2) || 0x08 != *(p + 3))
            return -1;

        Attach(data, 8, chunkLen);
        GetRect(&m_frame);  //Get the frame 

        m_frameRate = Get2Bytes() >> 8;
        m_frameCount = Get2Bytes();

        return 0;
    }

    S16 GetFrameCurrent() { return m_frameCurrent; }
    U16 GetFrameRate() { return m_frameRate; }
    U16 GetFrameCount() { return m_frameCount; }
    bool IsAtEnd() { return m_AtEnd;  }

    int SoundStreamBlock()
    {
        return 0;
    }

    int SoundStreamHead2()
    {
        return 0;
    }

    int DoTag()
    {
        int res = 0;

        if (m_AtEnd)
            return 1;

        int code = GetTag();
        if (code < 0)
            return 1;

        switch (code)
        {
        case ST_SHOWFRAME:
            m_frameCurrent++;  // move the current frame one step
            break;
        case ST_SOUNDSTREAMBLOCK:
            res = SoundStreamBlock();
            if (0 != res)
                return 1;
            break;
        case ST_SOUNDSTREAMHEAD2:
            res = SoundStreamHead2();
            if (0 != res)
                return 1;
            break;
        case ST_END:
            m_AtEnd = true; // we reach the end
            return 1;
        default:
            break;
        }
        GotoNextTag();
        return res;
    }

    int DoFrame(U16 frameIdx)
    {
        int res = 0;
        // this while loop will stop when we meet ST_SHOWFRAME tag 
        while (m_frameCurrent < frameIdx && 0 == res)
        {
            res = DoTag();
        }
        return res;
    }
};

class VoiceCallback : public IXAudio2VoiceCallback
{
public:
    VoiceCallback() //: hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
    {
        m_sobj = nullptr;
    }

    ~VoiceCallback()
    {
        //CloseHandle(hBufferEndEvent); 
    }

    int AttachPlayer(ScriptThread* t)
    {
        m_sobj = t;
        return 0;
    }

    //Called when the voice has just finished playing a contiguous audio stream.
    void OnStreamEnd()
    {}

    void WaitPlayEnd()
    {
        //DWORD dwWaitResult = WaitForSingleObject(hBufferEndEvent, INFINITE);
    }
    //Unused methods are stubs
    void OnVoiceProcessingPassEnd()
    {

    }
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired)
    {

    }
    void OnBufferEnd(void* pBufferContext)
    {

    }

    void OnBufferStart(void* pBufferContext) { }
    void OnLoopEnd(void* pBufferContext) { }
    void OnVoiceError(void* pBufferContext, HRESULT Error) { }
private:
    ScriptThread* m_sobj;
    //HANDLE hBufferEndEvent;
};

class NativeSoundMix
{
public:
    NativeSoundMix()
    {
        m_playThread = nullptr;
        m_playEvent = nullptr;
        m_isOpen = false;
        NativeConstruct();
    }

    ~NativeSoundMix()
    {
        NativeDestruct();
    }

    int NativeConstruct()
    {
        m_isOpen = false;
        m_xaudio = nullptr;
        m_xaudioMasterVoice = nullptr;
        m_xaudioSourceVoice = nullptr;
        m_xaudioBuffer = { 0 };

        HRESULT hr = XAudio2Create(&m_xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (S_OK != hr)
        {
            return -1;
        }
        hr = m_xaudio->CreateMasteringVoice(&m_xaudioMasterVoice,
            XAUDIO2_DEFAULT_CHANNELS,
            XAUDIO2_DEFAULT_SAMPLERATE,
            0, NULL, NULL, AudioCategory_GameEffects);
        if (S_OK != hr)
        {
            return -1;
        }
        return 0;
    }

    int NativeDestruct()
    {
        return 0;
    }

    int EnterCritical()
    {
        //EnterCriticalSection(&m_playLock);
        return 0;
    }

    int LeaveCritical()
    {
        //LeaveCriticalSection(&m_playLock);
        return 0;
    }

    int OpenNativeDevice(U8 format, void* opaque)
    {
        // the device has been opened, so just do nothing
        if (m_isOpen) 
            return 0;

        // Open a waveform device for output
        WAVEFORMATEX pcmWaveFormat = { 0 };
        pcmWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
        pcmWaveFormat.cbSize = 0;

        // Please check Page 219 in swf_file_format_spec_v9.pdf 
        if (sndStereo == (0x01 & format))
            pcmWaveFormat.nChannels = 2; // sndStereo
        else
            pcmWaveFormat.nChannels = 1; // sndMono

        if (snd16Bit == (0x02 & format))
            pcmWaveFormat.wBitsPerSample = 16; // 16-bit per sample
        else
            pcmWaveFormat.wBitsPerSample = 8; // 8-bit per sample

        U8 soundRate = (0x0C & format) >> 2;
        switch (soundRate)
        {
        case 0x00:
            pcmWaveFormat.nSamplesPerSec = sndRate5K;
            break;
        case 0x01:
            pcmWaveFormat.nSamplesPerSec = sndRate11K;
            break;
        case 0x02:
            pcmWaveFormat.nSamplesPerSec = sndRate22K;
            break;
        case 0x03:
            pcmWaveFormat.nSamplesPerSec = sndRate44K;
            break;
        }
        pcmWaveFormat.nBlockAlign = (WORD)((pcmWaveFormat.nChannels * pcmWaveFormat.wBitsPerSample) >> 3);
        pcmWaveFormat.nAvgBytesPerSec = pcmWaveFormat.nSamplesPerSec * pcmWaveFormat.nBlockAlign;

        m_voiceCB.AttachPlayer((ScriptThread*)opaque);

        HRESULT hr = m_xaudio->CreateSourceVoice(&m_xaudioSourceVoice, &pcmWaveFormat, XAUDIO2_VOICE_NOPITCH, 1.0,
            reinterpret_cast<IXAudio2VoiceCallback*>(&m_voiceCB), NULL, NULL);
        if (FAILED(hr))
        {
            m_isOpen = false;
            return (-1);
        }
        hr = m_xaudioSourceVoice->Start(0, XAUDIO2_COMMIT_NOW);
        if (FAILED(hr))
        {
            m_isOpen = false;
            return (-1);
        }

        m_isOpen = true;

        return 0;
    }

    int CloseNativeDevice()
    {
        return 0;
    }
    int IsDeviceOpen()
    {
        return m_isOpen;
    }

    HANDLE GetPlayEvent()
    {
        return m_playEvent;
    }

private:
    IXAudio2* m_xaudio;
    IXAudio2MasteringVoice* m_xaudioMasterVoice;
    IXAudio2SourceVoice* m_xaudioSourceVoice;
    XAUDIO2_BUFFER			m_xaudioBuffer;
    VoiceCallback			m_voiceCB;

    //CRITICAL_SECTION	m_playLock;
    HANDLE				m_playThread;
    HANDLE				m_playEvent;
    bool				m_isOpen;
};


int AttachSWF(LPTSTR path);

ScriptThread player;
ID2D1Factory* gpD2DFactory = nullptr;
IDWriteFactory* gpWriteFactory = nullptr;
IDWriteTextFormat* gpTextFormat = nullptr;

ID2D1HwndRenderTarget* gpRenderTarget = nullptr;
ID2D1SolidColorBrush* gpBrush = nullptr;

TCHAR path[MAX_PATH + 1] = { 0 };
TCHAR text[MAX_PATH + 1] = { 0 };

#define  DEFAULT_DPI    96
LRESULT CALLBACK swfWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int r;
    HRESULT hr = S_OK;
    UINT uElapse;
    S16  frameIdx;
    RECT rcClient;

    switch (message) 
    {
    case WM_TIMER:
        if (SWF_SECOND == wParam)
        {
            if (player.HasData())
            {
                frameIdx = player.GetFrameCurrent();
                U16 frameCount = player.GetFrameCount();
                U16 frameRate = player.GetFrameRate();
                swprintf(text, MAX_PATH, TEXT("Curr:%5d | Total:%5d | Rate:%d"), frameIdx, frameCount, frameRate);
                rcClient.left = 10;
                rcClient.right = 300;
                rcClient.top = 50;
                rcClient.bottom = 100;
                InvalidateRect(hWnd, &rcClient, 1);
            }
        }
        else if (SWF_TIMER == wParam)
        {
            if (player.IsAtEnd())
            {
                KillTimer(hWnd, SWF_TIMER);
                MessageBox(hWnd, TEXT("Read the end of SWF file"), TEXT("Play successfully!"), MB_OK);
                break;
            }
            frameIdx = player.GetFrameCurrent();
            player.DoFrame(frameIdx + 1);
        }
        break;
    case WM_LBUTTONDOWN:
        ::PostMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
        break;
    case WM_KEYDOWN: 
        break;
    case WM_ERASEBKGND:
        return 0;
    case WM_PAINT:
#if 10
        if (nullptr == gpRenderTarget)
        {
            GetClientRect(hWnd, &rcClient);
            // Create a D2D hwnd render target
            D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties
                = D2D1::RenderTargetProperties();
            renderTargetProperties.dpiX = DEFAULT_DPI;
            renderTargetProperties.dpiY = DEFAULT_DPI;

            D2D1_HWND_RENDER_TARGET_PROPERTIES hwndRenderTragetproperties
                = D2D1::HwndRenderTargetProperties(hWnd,
                    D2D1::SizeU(rcClient.right, rcClient.bottom));
            hr = gpD2DFactory->CreateHwndRenderTarget(
                                renderTargetProperties,
                                hwndRenderTragetproperties,
                                &gpRenderTarget);
            if (S_OK != hr)
                return 0;

            if (nullptr != gpBrush)
            {
                gpBrush->Release();
                gpBrush = nullptr;
            }
            hr = gpRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 1.0f), &gpBrush);
            if (S_OK != hr)
                return 0;

        }

        gpRenderTarget->BeginDraw();
        gpRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White, 1.0f));
        gpRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        // Draw text
        D2D1_RECT_F layoutRect = D2D1::RectF(10.0f, 50.0f, 300.0f, 100.0f);
        gpRenderTarget->DrawText(text, wcslen(text), gpTextFormat, layoutRect, gpBrush);

        hr = gpRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            gpRenderTarget->Release();
            gpRenderTarget = nullptr;
            if (nullptr != gpBrush)
            {
                gpBrush->Release();
                gpBrush = nullptr;
            }
        }
#endif
        break;

    case WM_DROPFILES:
        if (0 == DragQueryFile((HDROP)wParam, 0, path, MAX_PATH))
        {
            MessageBox(hWnd, TEXT("Cannot Accept SWF file"), TEXT("Drag File"), MB_OK);
            break;
        }
        r = AttachSWF(path);
        if(0 != r)
        {
            MessageBox(hWnd, TEXT("Cannot Open SWF file"), TEXT("Drag File"), MB_OK);
            break;
        }
        if (0 == player.m_frameRate)
            break;

        uElapse = 1000 / player.m_frameRate;
        SetTimer(hWnd, SWF_TIMER, uElapse, NULL);

        break;
    case WM_CREATE:
        swprintf(text, MAX_PATH, TEXT("Drag SWF file here!"));
        DragAcceptFiles(hWnd, TRUE);
        break;
    case WM_CLOSE:
        KillTimer(hWnd, SWF_TIMER);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpstrCmdLine, int nCmdShow)
{
    ///////////////////////////////////////////////////////////
    // Initialize COM
    ///////////////////////////////////////////////////////////
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (S_OK != hr)
    {
        MessageBox(NULL, TEXT("Failed to create COM!"), TEXT("FAILURE"), MB_OK);
        return 1;
    }

    gpD2DFactory = nullptr;
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &(gpD2DFactory));
    if (S_OK != hr)
    {
        MessageBox(NULL, TEXT("Failed to create D2D!"), TEXT("FAILURE"), MB_OK);
        ::CoUninitialize();
        return 1;
    }
    gpWriteFactory = nullptr;
    // Create DWrite factory
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&gpWriteFactory));
    if (S_OK != hr)
    {
        MessageBox(NULL, TEXT("Failed to create DWrite!"), TEXT("FAILURE"), MB_OK);
        ::CoUninitialize();
        return 1;
    }

    gpTextFormat = nullptr;
    hr = gpWriteFactory->CreateTextFormat(
        L"Courier New",
        NULL,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        13.0f,
        L"en-us",
        &gpTextFormat
    );
    if (S_OK != hr)
    {
        MessageBox(NULL, TEXT("Failed to create TextFormat!"), TEXT("FAILURE"), MB_OK);
        ::CoUninitialize();
        return 1;
    }

    ///////////////////////////////////////////////////////////
    // Set up window
    ///////////////////////////////////////////////////////////
    const TCHAR WIN_CLASS_NAME[] = TEXT("XAUDIO2_SWF_WINDOW_CLASS");

    streamSWF = nullptr;

    WNDCLASSEXW  wcex = { 0 };

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = (WNDPROC)swfWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(hInstance, IDC_ARROW);
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = WIN_CLASS_NAME;
    wcex.hIconSm = NULL;

    ATOM r = RegisterClassEx(&wcex);
    if (0 == r)
    {
        MessageBox(NULL, TEXT("Failed to register window class!"), TEXT("FAILURE"), MB_OK);
        ::CoUninitialize();
        return 1;
    }

    HWND window = CreateWindow(
        WIN_CLASS_NAME,
        TEXT("SWF MP3 demo"),
        WS_OVERLAPPEDWINDOW,
        400,
        400,
        600,
        400,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!window) 
    {
        MessageBox(NULL, TEXT("Failed to create window!"), TEXT("FAILURE"), MB_OK);
        ::CoUninitialize();
        return 1;
    }

    ///////////////////////////////////////////////////////////
    // Show window and start message loop.
    ///////////////////////////////////////////////////////////
    SetTimer(window, SWF_SECOND, 1000, NULL);
    ShowWindow(window, SW_SHOW);

    MSG message;
    while (GetMessage(&message, NULL, 0, 0) > 0) 
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    KillTimer(window, SWF_SECOND);
    ::CoUninitialize();
    return (int)message.wParam;
}

int AttachSWF(LPTSTR path)
{
    //MessageBox(NULL, path, TEXT("Drag File"), MB_OK);

    int fd = 0;
    if (0 != _tsopen_s(&fd, path, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0))
    {
        MessageBox(NULL, TEXT("Cannot Open SWF file"), TEXT("Drag File"), MB_OK);
        return 0;
    }

    uint32_t size = (uint32_t)_lseek(fd, 0, SEEK_END); /* get the file size */
    if (size > 256*1024*1024)
    {
        MessageBox(NULL, TEXT("SWF file is tooooo big!"), TEXT("Drag File"), MB_OK);
        return 0;
    }

    _lseek(fd, 0, SEEK_SET); /* go to the begin of the file */

    U8* stream = (uint8_t*)malloc(size);
    if (NULL == stream)
    {
        MessageBox(NULL, TEXT("streamSWF malloc failed!"), TEXT("Drag File"), MB_OK);
        return 0;
    }

    uint32_t bytes = (uint32_t)_read(fd, stream, size);
    if (size != bytes)
    {
        MessageBox(NULL, TEXT("Read file failed!"), TEXT("Drag File"), MB_OK);
        free(stream);
        stream = nullptr;
        return 0;
    }

    _close(fd);

    if (0 != player.PushData(stream, bytes))
    {
        MessageBox(NULL, TEXT("Attach Data failed!"), TEXT("Drag File"), MB_OK);
        free(stream);
        stream = nullptr;
        return 0;
    }

    if (nullptr != streamSWF)
        free(streamSWF);

    streamSWF = stream;

    return 0;
}
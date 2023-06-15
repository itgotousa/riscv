/*
 How to build this source file by using MSVC compiler:
 
 Install Visual Studio 2022 community version software.
 Open "X64 Native Tools Command Prompt for VS 2022" command line window, and type:
 
 cl wav.cpp user32.lib gdi32.lib ole32.lib
 
 -- itgotousa@gmail.com
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xaudio2.h>

#define BUFF_STEP    (512*1024)

class VoiceCallback : public IXAudio2VoiceCallback
{
public:
    uint32_t m_idx;
    HANDLE hBufferEndEvent;
    VoiceCallback() : hBufferEndEvent( CreateEvent( NULL, FALSE, FALSE, NULL ))
    { m_idx = 0; }
    
    ~VoiceCallback() { CloseHandle( hBufferEndEvent ); }
    //Called when the voice has just finished playing a contiguous audio stream.
    void OnStreamEnd() { }

    void WaitPlayEnd()
    {
        DWORD dwWaitResult = WaitForSingleObject(hBufferEndEvent, INFINITE);
    }
    void OnBufferEnd(void * pBufferContext)    
    { 
        printf("-- It is called! %6d\n", m_idx++);
        SetEvent( hBufferEndEvent );
    }
    //Unused methods are stubs
    void OnVoiceProcessingPassEnd() { }
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) { }
    void OnBufferStart(void * pBufferContext) { }
    void OnLoopEnd(void * pBufferContext) { }
    void OnVoiceError(void * pBufferContext, HRESULT Error) { }
};

IXAudio2* xaudio;
IXAudio2MasteringVoice* xaudioMasterVoice;
IXAudio2SourceVoice* xaudioSourceVoice;
XAUDIO2_BUFFER xaudioBuffer = { 0 };
WAVEFORMATEXTENSIBLE wfx = { 0 };
//WAVEFORMATEX waveFmt = { 0 };

int main(int argc, char* argv[])
{
    VoiceCallback voiceCallback;

    if(argc < 2)
    {
        printf("Usage: %s wav_file\n", argv[0]);
        return 0;
    }

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if(FAILED(hr))
    {
        printf("Failed to initialize COM!\n");
        return -1;
    }
    
    hr = XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR );
    if(FAILED(hr))
    {
        printf("Failed to initialize XAudio!\n");
        return -1;
    }
    
    hr = xaudio->CreateMasteringVoice(&xaudioMasterVoice, XAUDIO2_DEFAULT_CHANNELS,
                XAUDIO2_DEFAULT_SAMPLERATE, 0, NULL, NULL, AudioCategory_GameEffects);

    if(FAILED(hr))
    {
        printf("Failed to initialize XAudio mastering voice!\n");
        return -1;
    }

    FILE* fi = fopen(argv[1], "rb");
    if(NULL == fi) 
    {
		printf("cannot open file [%s]!\n", argv[1]);
        return 0;
    }

    fseek(fi, 0L, SEEK_END);
    long int size = ftell(fi);
    printf("[%s]fie size is %d bytes!\n", argv[1], (int)size);
    fseek(fi, 0L, SEEK_SET);
    uint8_t* inbuf = (uint8_t*)malloc(size);
    if(NULL == inbuf) {
        printf("Cannot allocate %d bytes memory!\n", (int)size);
        return 0;
    }    
    size_t ret = fread(inbuf, size, 1, fi);   
	if(1 != ret) {
        printf("Cannot read %d bytes from file!\n", (int)size);
        goto _exit_app;
    }
    fclose(fi);

    uint8_t* p = inbuf;
    if( 'R' != p[0] || 'I' != p[1] || 'F' != p[2] || 'F' != p[3])
    {
        printf("No RIFF!\n");
        goto _exit_app;
    }

    if( 'W' != p[8] || 'A' != p[9] || 'V' != p[10] || 'E' != p[11] || 'f' != p[12] || 'm' != p[13] || 't' != p[14])
    {
        printf("No WAVEfmt!\n");
        goto _exit_app;
    }

    uint32_t fmtSize =  *((uint32_t*)(p+16));
    printf("fmtSize is %d\n", fmtSize);
    p += 20; 
    memcpy(&wfx, p, fmtSize);
    printf("wFormatTag %d : nChannels %d : nSamplesPerSec %d : nAvgBytesPerSec %d : nBlockAlign  %d : wBitsPerSample %d : cbSize %d\n"
           ,wfx.Format.wFormatTag
           ,wfx.Format.nChannels
           ,wfx.Format.nSamplesPerSec
           ,wfx.Format.nAvgBytesPerSec
           ,wfx.Format.nBlockAlign
           ,wfx.Format.wBitsPerSample
           ,wfx.Format.cbSize
           );
    p += fmtSize; 
    if( 'd' != p[0] || 'a' != p[1] || 't' != p[2] || 'a' != p[3])
    {
        printf("No data!\n");
        goto _exit_app;
    }
    uint32_t dataSize =  *((uint32_t*)(p+4));
    printf("dataSize is %d\n", dataSize);

    hr = xaudio->CreateSourceVoice(&xaudioSourceVoice, (WAVEFORMATEX*)&wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO,
                                reinterpret_cast<IXAudio2VoiceCallback*>(&voiceCallback), NULL, NULL);
    if(FAILED(hr))
    {
        printf("Failed to initialize XAudio voice [%08X]!\n", hr);
        return -1;
    }

    xaudioSourceVoice->Start(0, XAUDIO2_COMMIT_NOW);
    if(FAILED(hr))
    {
        printf("Failed to Start voice!\n");
        goto _exit_app;
    }
    p += 8;
    uint32_t pos = 0;
    printf("Start!\n");
    do
    {
        xaudioBuffer.pAudioData = (BYTE*)p + pos;
        xaudioBuffer.AudioBytes = BUFF_STEP;
        xaudioBuffer.Flags = 0;
        if(pos + BUFF_STEP > dataSize) // the last buffer
        {
            xaudioBuffer.AudioBytes = dataSize % BUFF_STEP;
            xaudioBuffer.Flags = XAUDIO2_END_OF_STREAM;
        }
        xaudioSourceVoice->SubmitSourceBuffer(&xaudioBuffer, NULL);
        voiceCallback.WaitPlayEnd();
       
        pos += BUFF_STEP;
        
    } while (pos < dataSize);
        
    printf("STOP!!!!\n");
    //xaudioSourceVoice->Stop(0, XAUDIO2_COMMIT_NOW);
    xaudioSourceVoice->FlushSourceBuffers();
    
_exit_app:
	if(NULL != inbuf) free(inbuf);
    return 0;
}


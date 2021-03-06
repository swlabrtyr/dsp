#include <stdio.h>
#include <math.h>
#include <portaudio.h>

#define FRAME_BLOCK_LEN 256
#define SAMPLING_RATE 44100
#define TWO_PI (3.14159265f * 2.0f)

PaStream *audioStream;
double si = 0; // Sampling increment of the modulator
float m_b0, m_b1, m_b2, m_a1, m_a2, m_v1, m_v2;

void normalize(float a0) {
  m_b0 /= a0;
  m_b1 /= a0;
  m_b2 /= a0;
  m_a1 /= a0;
  m_a2 /= a0;
}

float processFilter(float x) {
  float v =      x - m_a1*m_v1 - m_a2*m_v2;
  float y = m_b0*v + m_b1*m_v1 + m_b2*m_v2;

  m_v2 = m_v1;
  m_v1 = v;
  return y;
}

void initLPF(float f0, float q) {
  float w0 = TWO_PI * f0/SAMPLING_RATE; 
  float alpha = sin(w0)/(2*q); 
  float a0;

  m_b0 = (1 - cos(w0)) / 2;
  m_b1 = 1 - cos(w0);
  m_b2 = (1 - cos(w0)) / 2;
  a0 = 1 + alpha;
  m_a1 = -2 * cos(w0);
  m_a2 = 1 - alpha;
  normalize(a0);
}

int audio_callback ( const void *inputBuffer, void *outputBuffer,
                     unsigned long framesPerBuffer,
                     const PaStreamCallbackTimeInfo* timeInfo,
                     PaStreamCallbackFlags statusFlags,
                     void *userData )
{
  float *in = (float*) inputBuffer, *out = (float*) outputBuffer;
  float cutoffFreqHz = 400;
  static double phase = 0;
  unsigned long i;
  initLPF(cutoffFreqHz, M_SQRT1_2);

  for(i = 0; i < framesPerBuffer; i++) {
//    float sine = sin(phase);
    *out++ = processFilter(*in++);
    *out++ = processFilter(*in++);
    phase += si;
  }

  printf("Out signal: %f \n", *out);
  return paContinue;
}

void init_stuff()
{
  float frequency;
  int i, id;
  const PaDeviceInfo *info;
  const PaHostApiInfo *hostapi;
  PaStreamParameters outputParameters, inputParameters;

  printf("Type the modulator frequency in Hertz: ");
  scanf("%f", &frequency); /* get the modulator frequency */

  si = TWO_PI * frequency / SAMPLING_RATE; /* calculate sampling increment */

  printf("Initializing Portaudio. Please wait...\n");
  Pa_Initialize(); /* Initialize portaudio */

  for (i = 0; i < Pa_GetDeviceCount(); i++) {
    info = Pa_GetDeviceInfo(i); /* get information from current device */
    hostapi = Pa_GetHostApiInfo(info->hostApi); /* get info from current host API */

    if (info->maxOutputChannels > 0)
      printf("%d: [%s] %s (output)\n", i, hostapi->name, info->name); /* if current device supports output */
  }

  printf("\nType AUDIO output device number: ");
  scanf("%d", &id); /* get the output device from the user */

  info = Pa_GetDeviceInfo(id); /* get chose device information strucutre */
  hostapi = Pa_GetHostApiInfo(info->hostApi); /* get host API struct */

  printf("Opening AUDIO output device [%s] %s\n", hostapi->name, info->name);
  outputParameters.device = id; /* chosen device id*/
  outputParameters.channelCount = 2; /* stereo output */
  outputParameters.sampleFormat = paFloat32; /* 32 bit float output */
  outputParameters.suggestedLatency = info->defaultLowOutputLatency; /* set default */
  outputParameters.hostApiSpecificStreamInfo = NULL; /* no specific info */

  for (i = 0; i < Pa_GetDeviceCount(); i++) {
    info = Pa_GetDeviceInfo(i); /* get information from current device */
    hostapi = Pa_GetHostApiInfo(info->hostApi); /* info from host API */

    if (info->maxInputChannels > 0)
      printf("%d: [%s] %s (input)\n", i, hostapi->name, info->name);
  }

  printf("\nType AUDIO input device number: ");
  scanf("%d", &id); /* get the input device id from the user */

  info = Pa_GetDeviceInfo(id);
  hostapi = Pa_GetHostApiInfo(info->hostApi); /* get host API struct */

  printf("Opening AUDIO input device [%s] %s\n", hostapi->name, info->name);

  inputParameters.device = id; /* chosen device id */
  inputParameters.channelCount = 2; /* stereo input */
  inputParameters.sampleFormat = paFloat32; /* 32 bit float input */
  inputParameters.suggestedLatency = info->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = NULL;

  Pa_OpenStream( /* open the PaStream object */
                &audioStream,     /* portaudio stream object */
                &inputParameters, /* provide output parameters */
                &outputParameters, /* provide input parameters */
                SAMPLING_RATE,    /* set sampleing rate */
                FRAME_BLOCK_LEN,  /* set frames per buffer */
                paClipOff,        /* set no clip */
                audio_callback,   /* callback function address */
                NULL );
  
  Pa_StartStream(audioStream); /* start the callback */
  printf("running... press space bar and enter to exit\n");
}

void terminate_stuff()
{
  Pa_StopStream( audioStream ); /* stop callback */
  Pa_CloseStream( audioStream ); /* destroy the audio stream object */

  Pa_Terminate(); /* terminate portaudio*/
}

int main()
{
  init_stuff();
  while(getchar() != ' ') Pa_Sleep(100); /* wait for space-bar
					    and enter */
  terminate_stuff();
  return 0;
}

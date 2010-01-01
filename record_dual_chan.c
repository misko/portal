/*
Copyright (c) 2014, Michael (Misko) Dzamba.
All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <complex.h>
#include <fftw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <math.h>

//#define CAMERA_USB_MIC_DEVICE "hw:%d,0"
//#define CAMERA_USB_MIC_DEVICE "plughw:%d,0"
//#define CAMERA_USB_MIC_DEVICE "sysdefault:CARD=Camera%d"
#define MIC_DEVICE "plughw:%d,0"

#define MICS 1
#define SECONDS 0.5
#define RATE 16000
#define SAMPLES ((int)(RATE*SECONDS)) 
char ** fns;


/********************************************************/
/*---------------------------------------------------------------------
III. License

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

---------------------------------------------------------------------
IV. Author

David E. Narváez
dMaggot
david.narvaez@computer.org*/
void xcorr(fftw_complex * signala, fftw_complex * signalb, fftw_complex * result, int N)
{
    fftw_complex * signala_ext = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * (2 * N - 1));
    fftw_complex * signalb_ext = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * (2 * N - 1));
    fftw_complex * out_shifted = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * (2 * N - 1));
    fftw_complex * outa = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * (2 * N - 1));
    fftw_complex * outb = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * (2 * N - 1));
    fftw_complex * out = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * (2 * N - 1));

    fftw_plan pa = fftw_plan_dft_1d(2 * N - 1, signala_ext, outa, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_plan pb = fftw_plan_dft_1d(2 * N - 1, signalb_ext, outb, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_plan px = fftw_plan_dft_1d(2 * N - 1, out, result, FFTW_BACKWARD, FFTW_ESTIMATE);

    //zeropadding
    memset (signala_ext, 0, sizeof(fftw_complex) * (N - 1));
    memcpy (signala_ext + (N - 1), signala, sizeof(fftw_complex) * N);
    memcpy (signalb_ext, signalb, sizeof(fftw_complex) * N);
    memset (signalb_ext + N, 0, sizeof(fftw_complex) * (N - 1));

    fftw_execute(pa);
    fftw_execute(pb);

    fftw_complex scale = 1.0/(2 * N -1);
    for (int i = 0; i < 2 * N - 1; i++)
        out[i] = outa[i] * conj(outb[i]) * scale;

    fftw_execute(px);

    fftw_destroy_plan(pa);
    fftw_destroy_plan(pb);
    fftw_destroy_plan(px);

    fftw_free(signala_ext);
    fftw_free(signalb_ext);
    fftw_free(out_shifted);
    fftw_free(out);
    fftw_free(outa);
    fftw_free(outb);

    fftw_cleanup();

    return;
}
/********************************************************/


struct WaveHeader {
        char RIFF_marker[4];
        uint32_t file_size;
        char filetype_header[4];
        char format_marker[4];
        uint32_t data_header_length;
        uint16_t format_type;
        uint16_t number_of_channels;
        uint32_t sample_rate;
        uint32_t bytes_per_second;
        uint16_t bytes_per_frame;
        uint16_t bits_per_sample;
};

struct WaveHeader *genericWAVHeader(uint32_t sample_rate, uint16_t bit_depth, uint16_t channels){
    struct WaveHeader *hdr;
    hdr = (struct WaveHeader*) malloc(sizeof(*hdr));
    if (!hdr)
        return NULL;

    memcpy(&hdr->RIFF_marker, "RIFF", 4);
    memcpy(&hdr->filetype_header, "WAVE", 4);
    memcpy(&hdr->format_marker, "fmt ", 4);
    hdr->data_header_length = 16;
    hdr->format_type = 1;
    hdr->number_of_channels = channels;
    hdr->sample_rate = sample_rate;
    hdr->bytes_per_second = sample_rate * channels * bit_depth / 8;
    hdr->bytes_per_frame = channels * bit_depth / 8;
    hdr->bits_per_sample = bit_depth;

    return hdr;
}

int writeWAVHeader(int fd, struct WaveHeader *hdr){
    if (!hdr)
        return -1;

    write(fd, &hdr->RIFF_marker, 4);
    write(fd, &hdr->file_size, 4);
    write(fd, &hdr->filetype_header, 4);
    write(fd, &hdr->format_marker, 4);
    write(fd, &hdr->data_header_length, 4);
    write(fd, &hdr->format_type, 2);
    write(fd, &hdr->number_of_channels, 2);
    write(fd, &hdr->sample_rate, 4);
    write(fd, &hdr->bytes_per_second, 4);
    write(fd, &hdr->bytes_per_frame, 2);
    write(fd, &hdr->bits_per_sample, 2);
    write(fd, "data", 4);

    uint32_t data_size = hdr->file_size + 8 - 44;
    write(fd, &data_size, 4);

    return 0;
}	      



void ShortToReal(signed short* shrt,double* real,int siz) {
	int i;
	for(i = 0; i < siz; ++i) {
		real[i] = shrt[i]; // 32768.0;
	}
}



signed short *buffer[MICS];
fftw_complex * buffer_complex[MICS];
fftw_complex * buffer_complex_result;;
snd_pcm_t *capture_handles[MICS];
snd_pcm_hw_params_t *hw_paramss[MICS];
snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

void init_audio(snd_pcm_t ** capture_handle, snd_pcm_hw_params_t ** hw_params , char * s) {
  fprintf(stderr,"Trying to open %s\n",s);
  int err;
 
  
  if ((err = snd_pcm_open (capture_handle, s, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    fprintf (stderr, "cannot open audio device(%s)\n", 
             snd_strerror (err));
    exit (1);
  }
 
  //fprintf(stdout, "audio interface opened\n");
		   
  if ((err = snd_pcm_hw_params_malloc (hw_params)) < 0) {
    fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror (err));
    exit (1);
  }
 
  //fprintf(stdout, "hw_params allocated\n");
				 
  if ((err = snd_pcm_hw_params_any (*capture_handle, *hw_params)) < 0) {
    fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (err));
    exit (1);
  }
 
  //fprintf(stdout, "hw_params initialized\n");
	
  if ((err = snd_pcm_hw_params_set_access (*capture_handle, *hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf (stderr, "cannot set access type (%s)\n",
             snd_strerror (err));
    exit (1);
  }
 
  //fprintf(stdout, "hw_params access setted\n");
	
  if ((err = snd_pcm_hw_params_set_format (*capture_handle, *hw_params, format)) < 0) {
    fprintf (stderr, "cannot set sample format (%s)\n",
             snd_strerror (err));
    exit (1);
  }
 
  //fprintf(stdout, "hw_params format setted\n");
  unsigned int rate=RATE;	
  if ((err = snd_pcm_hw_params_set_rate_near (*capture_handle, *hw_params, &rate, 0)) < 0) {
    fprintf (stderr, "cannot set sample rate (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr,"RATE IS %u should be %d\n",rate,RATE);
  assert(rate==RATE);
	
  //fprintf(stdout, "hw_params rate setted\n");
 
  if ((err = snd_pcm_hw_params_set_channels (*capture_handle, *hw_params, 2)) < 0) {
    fprintf (stderr, "cannot set channel count (%s)\n",
             snd_strerror (err));
    exit (1);
  }
 
  //fprintf(stdout, "hw_params channels setted\n");
	
  if ((err = snd_pcm_hw_params (*capture_handle, *hw_params)) < 0) {
    fprintf (stderr, "cannot set parameters (%s)\n",
             snd_strerror (err));
    exit (1);
  }
 
  //fprintf(stdout, "hw_params setted\n");
	
  snd_pcm_hw_params_free (*hw_params);
 
  //fprintf(stdout, "hw_params freed\n");
	
  if ((err = snd_pcm_prepare (*capture_handle)) < 0) {
    fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  //fprintf(stdout, "audio interface prepared\n");

}

void *thread_capture(void *threadarg) {
  int *th_id = (int*)threadarg;
  //fprintf(stderr,"THREAD %d %d\n",*th_id,SAMPLES); 
  int err;
    if ((err = snd_pcm_readi (capture_handles[*th_id], buffer[*th_id], SAMPLES)) != SAMPLES) {
      fprintf (stderr, "read from audio interface failed (%s)\n",
               snd_strerror (err));
     exit (1);
  }
  //fprintf(stderr,"captured %d points\n",err);

 //now write the file
 /*
  struct WaveHeader *hdr;
  hdr = genericWAVHeader(RATE, 16, 2);
  uint32_t pcm_data_size = hdr->bytes_per_frame * err;
  hdr->file_size = pcm_data_size + 44 - 8;
  int filedesc = open(fns[*th_id], O_WRONLY | O_CREAT, 0644);
  fprintf(stderr,"WRITTING TO %s\n",fns[*th_id]);
  err = writeWAVHeader(filedesc, hdr);
    if (err) {
		fprintf(stderr,"Failed to write header\n");
		exit(1);
    }

  write(filedesc, buffer[*th_id], pcm_data_size);*/
  //now copy over to complex buffer
  //compute mean and stddev
  double mean[]={0,0};
  for (int i=0; i<SAMPLES; i++) {
    mean[0]+=buffer[*th_id][2*i];
    mean[1]+=buffer[*th_id][2*i+1];
  }
  mean[0]/=SAMPLES;
  mean[1]/=SAMPLES;
  //now stddev
  double var[]={0,0};
  for (int i=0; i<SAMPLES; i++) {
    var[0]+=pow(buffer[*th_id][2*i]-mean[0],2);
    var[1]+=pow(buffer[*th_id][2*i+1]-mean[1],2);
  }
  var[0]/=SAMPLES;
  var[1]/=SAMPLES;
  double stddev[]={sqrt(var[0]),sqrt(var[1])};
  //fprintf(stderr,"MEAN %e STDDEV %e\n",mean,stddev);
  for (int i=0; i<SAMPLES; i++) {
    buffer_complex[*th_id][i]=(buffer[*th_id][2*i]-mean[0])/stddev[0];
    buffer_complex[*th_id][i+SAMPLES]=(buffer[*th_id][2*i+1]-mean[1])/stddev[1];
  }
  /*for (int i=0; i<SAMPLES; i++) {
    buffer_complex[*th_id][i]=buffer[*th_id][i];
  }*/

  //for (int i=0; i<SAMPLES; i++ ){ 
  //  fprintf(stdout,"%lf %lf\n",creal(buffer_complex[*th_id][i]),creal(buffer_complex[*th_id][i+SAMPLES]));
  //} 
  return NULL;
}

int main (int argc, char *argv[]) {
	
  if (argc!=1+(MICS) ) {
	fprintf(stderr,"%s filename\n",argv[0]);
	exit(1);
  }
  assert(SAMPLES%2==0);


  fns = argv+1;

  for (int i=0; i<MICS; i++) {
    char b[1024];
    sprintf(b,MIC_DEVICE,i);
    fprintf(stderr,"Setting %d\n",i);
    init_audio(capture_handles+i,hw_paramss+i,b);
    buffer[i] = malloc(SAMPLES * snd_pcm_format_width(format) / 8 * 2);
    buffer_complex[i] = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * (SAMPLES));
    assert(buffer_complex[i]!=NULL);
  }
  buffer_complex_result = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * (SAMPLES)*2);
  assert(buffer_complex_result!=NULL);
  for (int j=0; j<100; j++) {
    pthread_t threads[2];
    int th_ids[] = {0,1};
    for (int i=0; i<MICS; i++) {
      pthread_create(threads+i,NULL,thread_capture,th_ids+i);
    }
  
  	/* Create worker thread */
    for (int i=0; i<MICS; i++) {
      pthread_join(threads[i], NULL );
    }
    //xcorr(buffer_complex[0], buffer_complex[1], buffer_complex_result, SAMPLES);
    xcorr(buffer_complex[0], buffer_complex[0]+SAMPLES, buffer_complex_result, SAMPLES);
    double c = 340;
    double maxd = 1.5;
    int peaks = (int)((maxd/c)*RATE*2);
    
    double w=0.0;
    double s=0.0;
    int maxes=5;
    for (int k=0; k<maxes; k++) {
      double max=0;
      int maxi=0;
      for (int i=SAMPLES-peaks; i<(SAMPLES+peaks); i++) {
        double x = creal(buffer_complex_result[i]*conj(buffer_complex_result[i]));
        if (x>max) {
          max=x;
          maxi=i;
        }
        //printf("%f + i%f\n", creal(buffer_complex[0][i]), cimag(buffer_complex[0][i]));
        //printf("%f + i%f\n", creal(buffer_complex[1][i]), cimag(buffer_complex[1][i]));
        //printf("%e + i%e\n", creal(buffer_complex_result[i]), cimag(buffer_complex_result[i]));
      }
      buffer_complex_result[maxi]=0;
      w+=maxi*max;
      s+=max;
    }
    double lag = (w/s-(double)SAMPLES)/RATE;
    fprintf(stderr,"%e %e %e\n",w/s, lag, lag*c);
  }
  for (int i=0; i<MICS; i++) {
    snd_pcm_close (capture_handles[i]); 
  }


  return 0; 
}

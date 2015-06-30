import wave, struct
import sys
import numpy as np
from scipy.io.wavfile import read, write
import scipy

if len(sys.argv)!=3:
  print "%s in1 in2" % sys.argv[0]
  sys.exit(1)

in_fns=sys.argv[1:]
print in_fns

def read_wav(fn):
  wav_in = read(fn)
  if len(wav_in[1].shape)>1 and wav_in[1].shape[1]>1:
    return wav_in[0],np.array(wav_in[1],dtype=np.int16)[:,1]
  return np.array(wav_in[1],dtype=np.int16)


data=map( read_wav, in_fns)

# compute the cross-correlation between y1 and y2
ycorr = scipy.correlate(data[0], data[1], mode='full')
exit(1)
xcorr = scipy.linspace(0, len(ycorr)-1, num=len(ycorr))
ycorr = ycorr/sqrt(np.mean(ycorr*ycorr))                # normalize ycorr so it too doesn't cause too big a final prob_grid values
                                                        # with our closely spaced mic's and our closely matched mic gains, not a big deal
                                                        
ycorr_envelope = abs(scipy.signal.hilbert(ycorr))       # this computes the Hilbert envelope, as recommended by the UCLA group to avoid
                                                        # spatial aliasing due to rapid oscillations in the cross-correlation function and
                                                        # the many zeros that imposes--alternative or additional:  square then, take low-pass
                                                        # filter, then sqrt to remove rapid beat frequencies.
y_corr_fft = scipy.fft(ycorr)
fft_max_period = len(y_corr_fft)/np.argmax(abs(y_corr_fft[0:len(y_corr_fft)/2]))
                                                        # find the peak of this fft--use to define how far around our maximum we will use ***
                                                        # for an FFT, the length = f_s / peak frequency = period in in terms of original ycorr index
print "period (argument) of the fft maximum = ",fft_max_period,np.argmax(abs(y_corr_fft[0:len(y_corr_fft)/2]))

xcorr_center = len(xcorr)/2                             # location of zero lag time <--I checked that this works for odd lengths also
corr_index_max = argmax(ycorr_envelope)                      # get the index of maximum of the cross-correlation
corr_index_low[k] = int(corr_index_max  - 1.5*fft_max_period)         # compute the range of lag time indices to use in including the maximum correlation peak
if corr_index_low[k] < 0:  corr_index_low[k] = 0
corr_index_high[k] = int(corr_index_max  + 1.5*fft_max_period)

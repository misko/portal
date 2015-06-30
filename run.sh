fs=44100
arecord -D 'sysdefault:CARD=Camera_1' -d 3 /dev/shm/out_${1}_1.wav -r $fs &
arecord -D 'sysdefault:CARD=Camera' -d 3 /dev/shm/out_${1}_0.wav -r $fs &

/*
 * calc-sound-delay: calculate delay between similar sounds in 2 sound files.
 * Copyright (C) 2011 Mikhail Yakshin AKA GreyCat
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>

#define BUFSIZE 44100 * 10

float* open_and_read_file(const char *path, SF_INFO *sfinfo)
{
	float* buf = (float*) malloc(sizeof(float) * BUFSIZE);

	SNDFILE *f = sf_open(path, SFM_READ, sfinfo);
	if (f == NULL) {
		fprintf(stderr, "Unable to open \"%s\"\n", path);
		return NULL;
	}
	int readcount = sf_readf_float(f, buf, BUFSIZE);
	sf_close(f);

	if (sfinfo->channels != 1) {
		fprintf(stderr, "File \"%s\" is not mono, unable to continue.\n", path);
		return NULL;
	}

	return buf;
}

float correlation(float *buf1, float *buf2, int shift)
{
	float res = 0;
	int i = 0;
	int j = shift;
	while (i < BUFSIZE - shift) {
		res += buf1[i] * buf2[j];
		i++;
		j++;
	}
	return res;
}

int find_delay_in_range(float *buf1, float *buf2, int p1, int p2, int step)
{
	int maxPos = 0;
	float maxVal = 0;
	int i;

	fprintf(stderr, "Sweeping (%d, %d) with step %d...\n", p1, p2, step);

	for (i = p1; i < p2; i += step) {
		float val = correlation(buf1, buf2, i);
		if (val > maxVal) {
			maxVal = val;
			maxPos = i;
		}
	}

	return maxPos;
}

int find_delay(float *buf1, float *buf2)
{
	int r1 = find_delay_in_range(buf1, buf2, 0, BUFSIZE, 1024);

	int p1 = r1 - 1024 * 2;
	if (p1 < 0)  p1 = 0;
	int p2 = r1 + 1024 * 2;
	if (p2 > BUFSIZE)  p2 = BUFSIZE;

	int r2 = find_delay_in_range(buf1, buf2, p1, p2, 16);

	p1 = r2 - 16 * 2;
	if (p1 < 0)  p1 = 0;
	p2 = r2 + 16 * 2;
	if (p2 > BUFSIZE)  p2 = BUFSIZE;

	int r3 = find_delay_in_range(buf1, buf2, p1, p2, 1);

	return r3;
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s <file1> <file2>\n", argv[0]);
		return 1;
	}

	SF_INFO sfinfo1;
	float *buf1 = open_and_read_file(argv[1], &sfinfo1);
	if (buf1 == NULL)  return 1;
	SF_INFO sfinfo2;
	float *buf2 = open_and_read_file(argv[2], &sfinfo2);
	if (buf2 == NULL)  return 1;

	if (sfinfo1.samplerate != sfinfo2.samplerate) {
		fprintf(stderr, "Files have different sample rates (%d vs %d), unable to continue.\n", sfinfo1.samplerate, sfinfo2.samplerate);
		return 2;
	}

	printf("%f\n", (float) find_delay(buf1, buf2) / sfinfo1.samplerate);

	return 0;
}

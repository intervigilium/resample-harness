/* resample.c - sampling rate conversion subroutines
 *
 * Original version available at the 
 * Digital Audio Resampling Home Page located at
 * http://ccrma.stanford.edu/~jos/resample/.
 *
 * Modified for use on Android by Ethan Chen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "resample.h"

#define IBUFFSIZE 4096		/* Input buffer size */

static inline short WordToHword(int v, int scl)
{
	short out;
	int llsb = (1 << (scl - 1));
	v += llsb;		/* round */
	v >>= scl;
	if (v > MAX_HWORD) {
		v = MIN_HWORD;
	}
	out = (short)v;
	return out;
}

/* Sampling rate conversion using linear interpolation for maximum speed. */
static int SrcLinear(short X[], short Y[], double factor, unsigned int *Time,
		     unsigned short Nx)
{
	short iconst;
	short *Xp, *Ystart;
	int v, x1, x2;

	double dt;		/* Step through input signal */
	unsigned int dtb;	/* Fixed-point version of Dt */
	unsigned int endTime;	/* When Time reaches EndTime, return to user */

	dt = 1.0 / factor;	/* Output sampling period */
	dtb = dt * (1 << Np) + 0.5;	/* Fixed-point representation */

	Ystart = Y;
	endTime = *Time + (1 << Np) * (int)Nx;
	while (*Time < endTime) {
		iconst = (*Time) & Pmask;
		Xp = &X[(*Time) >> Np];	/* Ptr to current input sample */
		x1 = *Xp++;
		x2 = *Xp;
		x1 *= ((1 << Np) - iconst);
		x2 *= iconst;
		v = x1 + x2;
		*Y++ = WordToHword(v, Np);	/* Deposit output */
		*Time += dtb;	/* Move to next sample by time increment */
	}
	return (Y - Ystart);	/* Return number of output samples */
}

struct rs_data *resample_init(int in_rate, int out_rate, int channels)
{
	struct rs_data *data;

	data = (struct rs_data *)calloc(sizeof(struct rs_data), 1);
	if (!data) {
		return NULL;
	}
	if (out_rate <= 0 || in_rate <= 0) {
		return NULL;
	}

	data->factor = out_rate / (double)in_rate;
	data->channels = channels;
	data->out_count = 0;
	data->x_off = 10;
	data->x_num = IBUFFSIZE - 2 * data->x_off;
	data->x_ptr = data->x_off;
	data->x_read = data->x_off;
	data->time = (data->x_off << Np);

	data->out_size = (int)(((double)(IBUFFSIZE)) * data->factor + 2.0);
	data->in_left = (short *)calloc(sizeof(short), IBUFFSIZE);
	data->out_left = (short *)calloc(sizeof(short), data->out_size);
	if (!data->in_left || !data->out_left) {
		resample_close(data);
		return NULL;
	}
	memset(data->in_left, 0, sizeof(short) * data->x_off);
	if (channels == 2) {
		data->in_right = (short *)calloc(sizeof(short), IBUFFSIZE);
		data->out_right =
		    (short *)calloc(sizeof(short), data->out_size);
		if (!data->in_right || !data->out_right) {
			resample_close(data);
			return NULL;
		}
		memset(data->in_right, 0, sizeof(short) * data->x_off);
	}
	return data;
}

int
resample(struct rs_data *data, short *in_left, short *in_right, int num_samples)
{
	int i, in_buffer_idx, last, buf_max;
	unsigned int time2;
	unsigned int out_samples;	/* number of output samples to compute */
	unsigned short num_out, creep;

	if (!data) {
		return -1;
	}

	last = 0;
	in_buffer_idx = 0;
	out_samples = (unsigned int)(factor * (double)num_samples + 0.5);
	do {
		/* read in_left/right to data->in_left/right */
		if (!last) {
			if (IBUFFSIZE > num_samples - in_buffer_idx) {
				buf_max = num_samples - in_buffer_idx;
				last =
				    (num_samples - (framecount - inCount)) - 1 +
				    data->x_read;
			} else {
				buf_max = IBUFFSIZE;
				last = 0;
			}
			for (i = 0; i < buf_max; i++) {
				data->in_left[i + data->x_off] =
				    in_left[in_buffer_idx];
				if (data->channels == 2) {
					data->in_right[i + data->x_off] =
					    in_right[in_buffer_idx];
				}
				in_buffer_idx++;
			}

			if (last && (last - data->x_off < data->x_num)) {
				data->x_num = last - data->x_off;
				if (data->x_num <= 0) {
					break;
				}
			}
		}

		num_out =
		    SrcLinear(data->in_left, data->out_left, data->factor,
			      data->time, data->x_num);
		if (data->channels == 2) {
			time2 = data->time;
			num_out =
			    SrcLinear(data->in_right, data->out_right,
				      data->factor, data->time2, data->x_num);
		}

		data->time -= (data->x_num << Np);
		data->x_ptr += data->x_num;
		creep = (data->time >> Np) - data->x_off;
		if (creep) {
			data->time -= (creep << Np);
			data->x_ptr += creep;
		}
		for (i = 0; i < IBUFFSIZE - data->x_ptr + data->x_off; i++) {
			data->in_left[i] =
			    data->in_left[i + data->x_ptr - data->x_off];
			if (data->channels == 2) {
				data->in_right[i] =
				    data->in_right[i + data->x_ptr -
						   data->x_off];
			}
		}
		data->x_read = i;
		data->x_ptr = data->x_off;
		data->out_count += num_out;
		if (data->out_count > out_samples) {
			num_out -= (data->out_count - out_samples);
			data->out_count = out_samples;
		}

		/* resampled data sits in data->out_left, data->out_right already */
	} while (data->out_count < out_samples);

	return data->out_count;
}

void resample_close(struct rs_data *data)
{
	if (data) {
		free(data->in_left);
		free(data->in_right);
		free(data->out_right);
		free(data->out_left);
		free(data);
		data = NULL;
	}
}

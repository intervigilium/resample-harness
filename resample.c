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

struct rs_data *resample_init(int in_rate, int out_rate)
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
	data->in_buf_offset = 10;
	data->in_buf_ptr = data->in_buf_offset;
	data->in_buf_read = data->in_buf_offset;
	data->time = (data->in_buf_offset << Np);

	data->in_buf_size = IBUFFSIZE;
	data->out_buf_size =
	    (int)(((double)(data->in_buf_size)) * data->factor + 2.0);

	data->in_buf =
	    (short *)calloc(sizeof(short),
			    data->in_buf_size + data->in_buf_offset);
	data->out_buf = (short *)calloc(sizeof(short), data->out_buf_size);
	if (!data->in_buf || !data->out_buf) {
		resample_close(data);
		return NULL;
	}
	memset(data->in_buf, 0, sizeof(short) * data->in_buf_offset);
	return data;
}

int
resample(struct rs_data *data, short *in_buf, int in_buf_size, short *out_buf,
	 int out_buf_size)
{
	int i, len;
	int num_in, num_out, num_creep, num_reuse;
	int out_total_samples;

	if (!data) {
		return -1;
	}

	data->in_buf_used = 0;
	out_total_samples = 0;

	if (data->out_buf_ptr && (out_buf_size - out_total_samples > 0)) {
		len = MIN(out_buf_size - out_total_samples, data->out_buf_ptr);
		/* copy leftover samples to the output */
		for (i = 0; i < len; i++) {
			out_buf[out_total_samples + i] = data->out_buf[i];
		}
		out_total_samples += len;
		/* shift remaining samples in output buffer to beginning */
		for (i = 0; i < data->out_buf_ptr - len; i++) {
			data->out_buf[i] = data->out_buf[len + i];
		}

		return out_total_samples;
	}

	for (;;) {
		/* grab input samples from buffer */
		len = data->in_buf_size - data->in_buf_read;
		if (len >= in_buf_size - data->in_buf_used) {
			len = in_buf_size - data->in_buf_used;
		}
		for (i = 0; i < len; i++) {
			data->in_buf[data->in_buf_read + i] =
			    in_buf[data->in_buf_used + i];
		}
		data->in_buf_used += len;
		data->in_buf_read += len;

		if (last && (data->in_buf_used == in_buf_size)) {
			/* pad buffer with zero if no more data */
			num_in = data->in_buf_read - data->in_buf_offset;
			for (i = 0; i < data->in_buf_offset; i++) {
				data->in_buf[data->in_buf_read + i] = 0;
			}
		} else {
			num_in = data->in_buf_read - 2 * data->in_buf_offset;
		}

		if (num_in <= 0) {
			break;
		}

		/* do linear interpolation */
		num_out =
		    SrcLinear(data->in_buf, data->out_buf, data->factor,
			      &data->time, (unsigned short)num_in);

		/* move time back num_in samples back */
		data->time -= num_in;
		data->in_buf_ptr += num_in;

		/* remove time accumulation */
		num_creep = (int)(data->time) - data->x_off;
		if (num_creep) {
			data->time -= num_creep;
			data->in_buf_ptr += num_creep;
		}

		/* copy input signal that needs to be reused */
		num_reuse =
		    data->in_buf_read - (data->in_buf_ptr -
					 data->in_buf_offset);
		for (i = 0; i < num_reuse; i++) {
			data->in_buf[i] =
			    data->
			    in_buf[(data->in_buf_ptr - data->in_buf_offset) +
				   i];
		}
		data->in_buf_read = num_reuse;
		data->in_buf_ptr = data->in_buf_offset;

		/* copy samples to output buffer */
		data->out_buf_ptr = num_out;
		if (data->out_buf_ptr && (out_buf_size - out_total_samples > 0)) {
			len =
			    MIN(out_buf_size - out_total_samples,
				data->out_buf_ptr);
			for (i = 0; i < len; i++) {
				out_buf[out_total_samples + i] =
				    data->out_buf[i];
			}
			out_total_samples += len;
			/* store uncopied output buffer */
			for (i = 0; i < data->out_buf_ptr - len; i++) {
				data->out_buf[i] = data->out_buf[len + i];
			}
			data->out_buf_ptr -= len;
		}
		if (data->out_buf_ptr) {
			break;
		}
	}

	return out_total_samples;
}

void resample_close(struct rs_data *data)
{
	if (data) {
		free(data->in_buf);
		free(data->out_buf);
		free(data);
		data = NULL;
	}
}

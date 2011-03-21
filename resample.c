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
	if (!data) {
		return -1;
	}

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

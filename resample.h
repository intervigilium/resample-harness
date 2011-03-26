/* resample.h - sampling rate conversion subroutines
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

#define PI (3.14159265358979232846)
#define PI2 (6.28318530717958465692)
#define D2R (0.01745329348)	/* (2*pi)/360 */
#define R2D (57.29577951)	/* 360/(2*pi) */

#define MAX(x,y) ((x)>(y) ?(x):(y))
#define MIN(x,y) ((x)<(y) ?(x):(y))
#define ABS(x)   ((x)<0   ?(-(x)):(x))
#define SGN(x)   ((x)<0   ?(-1):((x)==0?(0):(1)))

#define Nhc       8
#define Na        7
#define Np       (Nhc+Na)
#define Npc      (1<<Nhc)
#define Amask    ((1<<Na)-1)
#define Pmask    ((1<<Np)-1)
#define Nh       16
#define Nb       16
#define Nhxn     14
#define Nhg      (Nh-Nhxn)
#define NLpScl   13

#define MAX_HWORD (32767)
#define MIN_HWORD (-32768)

#ifndef MAX
#define MAX(x,y) ((x)>(y) ?(x):(y))
#endif
#ifndef MIN
#define MIN(x,y) ((x)<(y) ?(x):(y))
#endif

struct rs_data {
	double factor;
	unsigned int time;
	unsigned short in_buf_ptr;
	unsigned short out_buf_ptr;
	unsigned short in_buf_read;
	unsigned short in_buf_offset;
	int in_buf_used;
	int in_buf_size;
	int out_buf_size;
	short *in_buf;
	short *out_buf;
};

struct rs_data *resample_init(int in_rate, int out_rate);

int resample(struct rs_data *rs, short *in_buf, int in_buf_size, short *out_buf,
	     int out_buf_size, int last);

void resample_close(struct rs_data *rs);

/*
 * main interface to resample test harness
 */

#include <stdlib.h>
#include <stdio.h>
#include <sndfile.h>
#include "resample.h"

#define BUF_SIZE 2048

int main(int argc, char **argv)
{
	SNDFILE *ifp, *ofp;
	SF_INFO *if_info, *of_info;
	struct rs_data *rs;

	if (argc != 4) {
		printf
		    ("usage: resample <input file> <output file> <output sample rate>\n");
	} else {
		int out_srate = atoi(argv[3]);
		if_info = calloc(1, sizeof(SF_INFO));

		ifp = sf_open(argv[1], SFM_READ, if_info);
		if (!ifp) {
			printf("unable to open input file %s!\n", argv[1]);
			exit(EXIT_FAILURE);
		}

		of_info = calloc(1, sizeof(SF_INFO));
		of_info->samplerate = out_srate;
		of_info->channels = if_info->channels;
		of_info->format = if_info->format;

		ofp = sf_open(argv[2], SFM_WRITE, of_info);
		if (!ofp) {
			printf("unable to open output file %s!\n", argv[2]);
			exit(EXIT_FAILURE);
		}

		sf_count_t samples_written = 0;
		sf_count_t samples_read = 1;
		sf_count_t samples_resampled = 0;
		short *inbuf = calloc(BUF_SIZE, sizeof(short));
		short *outbuf = calloc(BUF_SIZE, sizeof(short));

		rs = resample_init(if_info->samplerate, out_srate);
		do {
			samples_read = sf_read_short(ifp, inbuf, BUF_SIZE);
			int last = samples_read != BUF_SIZE;
			if (samples_read > 0) {
				samples_resampled =
				    resample(rs, inbuf, samples_read,
					     outbuf, samples_read, last);
				samples_written =
				    sf_write_short(ofp, outbuf,
						   samples_resampled);
			}
		} while (samples_read > 0);
		resample_close(rs);

		free(inbuf);
		free(outbuf);
		sf_close(ifp);
		sf_close(ofp);
	}

	return 0;
}

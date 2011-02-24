/*
 * main interface to resample test harness
 */

#include <stdlib.h>
#include <stdio.h>
#include <sndfile.h>
#include "resample.h"

int
main(int argc, char **argv)
{
    SNDFILE *ifp, *ofp;
    SF_INFO *if_info, *of_info;

    if (argc != 3) {
        printf("usage: resample <input> <output>\n");
    } else {
        if_info = calloc(1, sizeof(SF_INFO));

        ifp = sf_open(argv[1], SFM_READ, if_info);
        if (!ifp) {
            printf("unable to open input file %s!\n", argv[1]);
            exit(EXIT_FAILURE);
        }

        of_info = calloc(1, sizeof(SF_INFO));
        of_info->samplerate = if_info->samplerate;
        of_info->channels = if_info->channels;
        of_info->format = if_info->format;

        ofp = sf_open(argv[2], SFM_WRITE, of_info);
        if (!ofp) {
            printf("unable to open output file %s!\n", argv[2]);
            exit(EXIT_FAILURE);
        }

        sf_count_t samples_read = 0;

        do {

        } while (samples_read > 0);

        sf_close(ifp);
        sf_close(ofp);
    }

    return 0;
}

/**
 * @file main.c Audio Resampler for WAV-files
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <stdlib.h>
#include <getopt.h>
#include <re.h>
#include <rem.h>


#define BLOCK_SIZE 1024


static int resample(const char *infile, const char *outfile,
		    uint32_t srate_out, int channels_out)
{
	struct aufile *af_in = NULL, *af_out = NULL;
	struct aufile_prm prm_in, prm_out;
	struct auresamp rs;
	size_t sampc_in_total = 0, sampc_out_total = 0;
	double duration_in, duration_out;
	int err;

	err = aufile_open(&af_in, &prm_in, infile, AUFILE_READ);
	if (err) {
		re_fprintf(stderr, "%s: could not open input file (%m)\n",
			   infile, err);
		goto out;
	}

	if (prm_in.fmt != AUFMT_S16LE) {
		err = EINVAL;
		goto out;
	}

	prm_out = prm_in;
	prm_out.srate = srate_out;
	prm_out.channels = channels_out;

	err = aufile_open(&af_out, &prm_out, outfile, AUFILE_WRITE);
	if (err) {
		re_fprintf(stderr, "%s: could not open output file (%m)\n",
			   outfile, err);
		goto out;
	}

	re_printf("%s: %u Hz, %d channels\n",
		  infile, prm_in.srate, prm_in.channels);
	re_printf("%s: %u Hz, %d channels\n",
		  outfile, prm_out.srate, prm_out.channels);

	auresamp_init(&rs);
	err = auresamp_setup(&rs,
			     prm_in.srate, prm_in.channels,
			     prm_out.srate, prm_out.channels);
	if (err)
		goto out;

	for (;;) {
		int16_t sampv_in[BLOCK_SIZE], sampv_out[2048*8];
		size_t sampc_out = ARRAY_SIZE(sampv_out);
		size_t sz_in = sizeof(sampv_in);

		err = aufile_read(af_in, (void *)sampv_in, &sz_in);
		if (err || sz_in == 0)
			break;

		sampc_in_total += (sz_in/2);

		err = auresamp(&rs,
			       sampv_out, &sampc_out,
			       sampv_in, sz_in/2);
		if (err) {
			re_fprintf(stderr, "resampler error: %m\n", err);
			break;
		}

		err = aufile_write(af_out, (void *)sampv_out, sampc_out*2);
		if (err) {
			re_fprintf(stderr, "write to output file error: %m\n",
				   err);
			break;
		}

		sampc_out_total += sampc_out;
	}

 out:
	if (err) {
		re_fprintf(stderr, "resampler error: %m\n", err);
	}

	re_printf("read %u samples, wrote %u samples\n",
		  sampc_in_total, sampc_out_total);

	duration_in  = 1.0 * sampc_in_total / prm_in.srate;
	duration_out = 1.0 * sampc_out_total / prm_out.srate;

	re_printf("duration: input = %f seconds, output = %f seconds\n",
		  duration_in, duration_out);

	mem_deref(af_out);
	mem_deref(af_in);

	return err;
}


static void usage(void)
{
	(void)re_fprintf(stderr,
			 "remsampl -crh  input.wav output.wav\n");
	(void)re_fprintf(stderr, "\t-c <CHANNELS> Output file num channels\n");
	(void)re_fprintf(stderr, "\t-r <SRATE>    Output file sample-rate\n");
	(void)re_fprintf(stderr, "\t-h            Show summary of options\n");
}


int main(int argc, char *argv[])
{
	const char *file_in, *file_out;
	uint32_t srate = 0;
	int channels = 1;
	int err = 0;

	for (;;) {

		const int c = getopt(argc, argv, "c:r:h");
		if (0 > c)
			break;

		switch (c) {

		case 'c':
			channels = atoi(optarg);
			break;

		case 'r':
			srate = atoi(optarg);
			break;

		case '?':
			err = EINVAL;
			/*@fallthrough@*/
		case 'h':
			usage();
			return err;
		}
	}

	if (argc < 3 || argc != (optind + 2)) {
		usage();
		return -EINVAL;
	}

	file_in  = argv[optind++];
	file_out = argv[optind++];

	err = resample(file_in, file_out, srate, channels);
	if (err)
		goto out;

 out:
	tmr_debug();
	mem_debug();

	return err;
}

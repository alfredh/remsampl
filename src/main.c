
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <math.h>
#include <re.h>
#include <rem.h>


static int resample(const char *infile, const char *outfile,
		    uint32_t srate_out)
{
	struct aufile *af_in = NULL, *af_out = NULL;
	struct auresamp *rs = NULL;
	struct aufile_prm prm;
	uint32_t srate_in;
	size_t sampc_in_total = 0;
	size_t sampc_out_total = 0;
	double duration_in, duration_out;
	int err;

	err = aufile_open(&af_in, &prm, infile, AUFILE_READ);
	if (err) {
		re_fprintf(stderr, "%s: could not open input file (%m)\n",
			   infile, err);
		goto out;
	}

	if (prm.channels != 1 || prm.fmt != AUFMT_S16LE) {
		err = EINVAL;
		goto out;
	}

	srate_in = prm.srate;
	prm.srate = srate_out;

	re_printf("input sample-rate is %u Hz\n", srate_in);
	re_printf("output sampler-rate is %u Hz\n", srate_out);

	err = aufile_open(&af_out, &prm, outfile, AUFILE_WRITE);
	if (err) {
		re_fprintf(stderr, "%s: could not open output file (%m)\n",
			   outfile, err);
		goto out;
	}

	err = auresamp_alloc(&rs, 2048,
			     srate_in, prm.channels,
			     srate_out, prm.channels);
	if (err)
		goto out;

	for (;;) {
		int16_t sampv_in[2048], sampv_out[2048];
		size_t sampc_out = ARRAY_SIZE(sampv_out);
		size_t sz_in = sizeof(sampv_in);

		err = aufile_read(af_in, (void *)sampv_in, &sz_in);
		if (err || sz_in == 0)
			break;

		sampc_in_total += (sz_in/2);

		err = auresamp_process(rs,
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
	re_printf("read %u samples, wrote %u samples\n",
		  sampc_in_total, sampc_out_total);

	duration_in  = 1.0 * sampc_in_total / srate_in;
	duration_out = 1.0 *sampc_out_total / srate_out;

	re_printf("duration: input = %f seconds, output = %f seconds\n",
		  duration_in, duration_out);

	mem_deref(rs);
	mem_deref(af_out);
	mem_deref(af_in);

	return err;
}


int main(int argc, char *argv[])
{
	resample("sine-48000.wav", "sine-32000.wav", 32000);


}

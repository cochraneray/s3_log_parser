// S3 Log Parser
//
//
//
#include "../include/s3_lp.h"

int
main(int argc, char *argv[])
{
	char *filename = NULL;	  // default input filename
	char *output_file = NULL; // default output filename
	FILE *ifp = stdin;		  // default file path to stdin
	FILE *ofp = stdout;		  // default file path to stdout
	int err_flag = 0;
	s_context_t context;


	// Initialize ip_hash struct
	{
		context.ip_track.capacity = IP_HASH;
		context.ip_track.count = 0;
		// GTest uses cpp which requires explicit cast
		context.ip_track.ip_hashes = (uint64_t *)calloc(IP_HASH, sizeof(uint64_t));

		context.verbose = 0;
		context.output_filetype_flag = BIN_FILE;
	}
	// Getopt scope - OPTIONS in header file
	{
		int opt = -1;

		while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
			switch (opt) {
			// File name override: defaults to stdout
			case 'f': {
				if (optarg == 0) {
					fprintf(stderr, "-f requires a filename to parse, defaults to stdin");
					err_flag = 1;
				}
				filename = optarg;
				break;
			}
			// output file override: might just default this to slim.bin
			case 'o': {
				if (optarg == 0) {
					fprintf(stderr, "-o requires an output filename, defaults to stdout");
					err_flag = 1;
				}
				output_file = optarg;
				break;
			}
			case 'v': {
				context.verbose = (context.verbose == 0) ? 1 : 0;
				break;
			}
			case 't': {
				char x = *optarg;
				switch (x) {
				case 'c':
					context.output_filetype_flag = CSV_FILE;
					break;
				case 'b':
					context.output_filetype_flag = BIN_FILE;
				default:
					fprintf(stderr, "%c not recognized file type, binary selected as default\n", x);
					context.output_filetype_flag = BIN_FILE;
					break;
				}
				break;
			}
			// help
			case 'h': {
				fprintf(stderr, "USAGE: ./s3_lp -[vh] -f: <filepath> -o: <output_filepath> -t:: [bc]\n"
								"\t-f filepath : override default filepath from stdin\n"
								"\t-o filepath : override default output filepath from stdout\n"
								"\t-v verbose output\n"
								"\t-t file type: (b)in, (c)sv" // pgsql db insert query?
								"\t-h display options\n");
				err_flag = 1;
				break;
			}
			default:
				fprintf(stderr, "USAGE: ./s3_lp -[options] -f <filepath>\n\tUse -h to "
								"get help:\n\n\t./s3_lp -h");
				err_flag = 1;
			} // End of Switch
		} // iEnd of getopt while loop
	} // end of getopt scope

	if (err_flag == 1) {
		free(context.ip_track.ip_hashes);
		exit(EXIT_FAILURE);
	}
	// if filename was passed in, replace FILE*
	if (filename != NULL) {
		ifp = fopen(filename, "r");
		if (ifp == NULL) {
			perror("fopen intake");
			err_flag = 1;
		}
	}

	// if output filename was passed in, replace FILE*
	if (output_file != NULL) {
		ofp = fopen(output_file, "w");
		if (ofp == NULL) {
			perror("fopen output");
			err_flag = 1;
		}
	}

	if (err_flag == 1) {
		free(context.ip_track.ip_hashes);
		exit(EXIT_FAILURE);
	}
	// run log processing
	process_log(ifp, ofp, &context);

	// Free FILE* and struct
	free(context.ip_track.ip_hashes);
	fclose(ifp);
	fclose(ofp);
	exit(EXIT_SUCCESS);
}

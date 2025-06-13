// S3 Log Parser
//
//
//
#include "../include/s3lp.h"

/**
 * S3 Log Processor - Main Entry
 *
 * This program processes log files (AWS S3 bucket access logs) and tracks unique IP addresses.
 * Supports both binary and CSV output formats with configurable input/output sources.
 */

int
main(int argc, char *argv[])
{
	char *filename = NULL;	  // default input filename
	char *output_file = NULL; // default output filename
	FILE *ifp = stdin;		  // default file path to stdin
	FILE *ofp = stdout;		  // default file path to stdout
	int err_flag = 0;
	s_context_t context; // Contextual Flags


	// Initialize Flags, defaults, and IP tracking hash table
	{
		context.ip_track.capacity = IP_HASH;
		context.ip_track.count = 0;
		// GTest uses cpp which requires explicit cast
		context.ip_track.ip_hashes = (uint64_t *)calloc(IP_HASH, sizeof(uint64_t));
		context.verbose = 0;					 // Verbose output disabled
		context.output_filetype_flag = BIN_FILE; // Default Binary File
	}

	// Command-line argument Parsing - OPTIONS defined in header file
	{
		int opt = -1;

		while ((opt = getopt(argc, argv, PARSER_OPTIONS)) != -1) {
			switch (opt) {
			// File name override: defaults to stdout
			// INPUT: -f <filename>
			case 'f': {
				if (optarg == 0) {
					fprintf(stderr, "-f requires a filename to parse, defaults to stdin");
					err_flag = 1;
				}
				filename = optarg;
				break;
			}
			// output file override: might just default this to slim.bin
			// INPUT: -o <filename>
			case 'o': {
				if (optarg == 0) {
					fprintf(stderr, "-o requires an output filename, defaults to stdout");
					err_flag = 1;
				}
				output_file = optarg;
				break;
			}
			// Verbose Toggle
			// INPUT: -v
			case 'v': {
				context.verbose = (context.verbose == 0) ? 1 : 0;
				break;
			}
			// Outuput file type
			// INPUT: -t [c/b]
			// NOTE: EXTRACTION WORKS SPECIFICALLY ON BINARY FILES
			case 't': {
				char x = *optarg;
				switch (x) {
				case 'c':
					context.output_filetype_flag = CSV_FILE;
					break;
				case 'b':
					context.output_filetype_flag = BIN_FILE;
					break;
				default:
					fprintf(stderr, "%c not recognized file type, binary selected as default\n", x);
					context.output_filetype_flag = BIN_FILE;
					break;
				}
				break;
			}
			// Help / Usage information
			// INPUT: -h
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
			// Invalid option handler
			default:
				fprintf(stderr, "USAGE: ./s3_lp -[options] -f <filepath>\n\tUse -h to "
								"get help:\n\n\t./s3_lp -h");
				err_flag = 1;
			} // End of Switch
		} // iEnd of getopt while loop
	} // end of getopt scope

	// Catch arg parsing error
	if (err_flag == 1) {
		free(context.ip_track.ip_hashes);
		exit(EXIT_FAILURE);
	}


	// Open input file if provided (default: stdin)
	if (filename != NULL) {
		ifp = fopen(filename, "r");
		if (ifp == NULL) {
			perror("fopen intake");
			err_flag = 1;
		}
	}

	// Open output file if provided (default: stdout)
	if (output_file != NULL) {
		ofp = fopen(output_file, "w");
		if (ofp == NULL) {
			perror("fopen output");
			err_flag = 1;
		}
	}

	// Exit on file opening error
	if (err_flag == 1) {
		free(context.ip_track.ip_hashes);
		exit(EXIT_FAILURE);
	}

	// Process Input Log
	err_flag = process_log(ifp, ofp, &context);

	if (err_flag != 0) {
		fprintf(stderr, "Error detected: Cleaning up\n\n");
	}
	// Cleanup
	free(context.ip_track.ip_hashes);
	fclose(ifp);
	fclose(ofp);
	exit(EXIT_SUCCESS);
}

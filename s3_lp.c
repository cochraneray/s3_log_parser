// S3 Log Parser
//
//
//
#include "s3_lp.h"

int
main(int argc, char *argv[])
{
	char *filename = NULL;
	char *output_file = NULL;
	FILE *ifp = stdin; // default file path to stdin
	FILE *ofp = stdout;

	int err_flag = -1;

	{
		int opt = -1;

		while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
			switch (opt) {
			case 'f': {
				if (optarg == NULL) {
					fprintf(stderr, "-f requires a filename to parse, defaults to stdin");
					exit(EXIT_FAILURE);
				}
				filename = optarg;
				break;
			}
			case 'o': {
				if (optarg == NULL) {
					fprintf(stderr, "-o requires an output filename, defaults to stdout");
					exit(EXIT_FAILURE);
				}
				output_file = optarg;
				break;
			}
			case 'h': {
				fprintf(stderr, "USAGE: ./s3_lp -[h] -f <filepath>\n"
								"\t-f filepath: override default filepath of stdin\n"
								"\t-h display options\n");
				exit(EXIT_SUCCESS);
			}
			default:
				fprintf(stderr, "USAGE: ./s3_lp -[options] -f <filepath>\n\tUse -h to get help:\n\n\t./s3_lp -h");
				exit(EXIT_SUCCESS);
			}
		}
	}

	if (filename != NULL) {
		ifp = fopen(filename, "r");
		if (ifp == NULL) {
			perror("fopen");
			exit(EXIT_FAILURE);
		}
	}

	process_log(ifp, ofp);


	fclose(ifp);
	fclose(ofp);
	exit(EXIT_SUCCESS);
}

void
process_log(FILE *log, FILE *output)
{
	char log_entry[LOG_SIZE];
	p_log_t *batch_parsed_logs = malloc(BATCH_SIZE * sizeof(p_log_t));
	s_log_t *batch_slim_logs = malloc(BATCH_SIZE * sizeof(s_log_t));

	int count = 0;

	while (fgets(log_entry, sizeof(log_entry), log)) {
		parse_log_entry(log_entry, &batch_parsed_logs[count]);
		extract_log_entry(&batch_parsed_logs[count], &batch_slim_logs[count]);
		++count;

		if (count >= BATCH_SIZE) {
			process_slim_logs(batch_slim_logs, count, output);
		}
	}

	if (count > 0) {
		process_slim_logs(batch_slim_logs, count, output);
	}

	free(batch_parsed_logs);
	free(batch_slim_logs);
}

void
parse_log_entry(char *in_log, p_log_t *logs)
{
}

void
extract_log_entry(p_log_t *in_log, s_log_t *slim_log)
{
}

void
process_slim_logs(s_log_t *slim_log, int num_entrys, FILE *output)
{
}

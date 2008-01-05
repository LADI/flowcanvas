/*
  File autogenerated by gengetopt version 2.21
  generated with the following command:
  gengetopt 

  The developers of gengetopt consider the fixed text that goes in all
  gengetopt output files to be in the public domain:
  we make no copyright claims on it.
*/

/* If we use autoconf.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getopt.h"

#include "cmdline.h"

const char *gengetopt_args_info_purpose = "A modular realtime audio processing system";

const char *gengetopt_args_info_usage = "Usage: ingen [OPTIONS]...";

const char *gengetopt_args_info_description = "This executable can be used to launch any configuration of Ingen.\nIngen can run as a stand-alone server controlled by OSC, or internal to\nanother process.  The GUI can communicate with the engine via either method,\nand many GUIs (or other things) may connect to an engine via OSC.\n";

const char *gengetopt_args_info_help[] = {
  "  -h, --help             Print help and exit",
  "  -V, --version          Print version and exit",
  "  -e, --engine           Run (JACK) engine  (default=off)",
  "  -E, --engine-port=INT  Engine OSC port  (default=`16180')",
  "  -c, --connect=STRING   Connect to existing engine at OSC URI  \n                           (default=`osc.udp://localhost:16180')",
  "  -g, --gui              Launch the GTK graphical interface  (default=off)",
  "  -C, --client-port=INT  Client OSC port",
  "  -l, --load=STRING      Load patch",
  "  -L, --path=STRING      Target path for loaded patch",
  "  -r, --run=STRING       Run script",
  "  -p, --parallelism=INT  Number of concurrent process threads  (default=`1')",
    0
};

static
void clear_given (struct gengetopt_args_info *args_info);
static
void clear_args (struct gengetopt_args_info *args_info);

static int
cmdline_parser_internal (int argc, char * const *argv, struct gengetopt_args_info *args_info,
                        struct cmdline_parser_params *params, const char *additional_error);


static char *
gengetopt_strdup (const char *s);

static
void clear_given (struct gengetopt_args_info *args_info)
{
  args_info->help_given = 0 ;
  args_info->version_given = 0 ;
  args_info->engine_given = 0 ;
  args_info->engine_port_given = 0 ;
  args_info->connect_given = 0 ;
  args_info->gui_given = 0 ;
  args_info->client_port_given = 0 ;
  args_info->load_given = 0 ;
  args_info->path_given = 0 ;
  args_info->run_given = 0 ;
  args_info->parallelism_given = 0 ;
}

static
void clear_args (struct gengetopt_args_info *args_info)
{
  args_info->engine_flag = 0;
  args_info->engine_port_arg = 16180;
  args_info->engine_port_orig = NULL;
  args_info->connect_arg = gengetopt_strdup ("osc.udp://localhost:16180");
  args_info->connect_orig = NULL;
  args_info->gui_flag = 0;
  args_info->client_port_orig = NULL;
  args_info->load_arg = NULL;
  args_info->load_orig = NULL;
  args_info->path_arg = NULL;
  args_info->path_orig = NULL;
  args_info->run_arg = NULL;
  args_info->run_orig = NULL;
  args_info->parallelism_arg = 1;
  args_info->parallelism_orig = NULL;
  
}

static
void init_args_info(struct gengetopt_args_info *args_info)
{
  args_info->help_help = gengetopt_args_info_help[0] ;
  args_info->version_help = gengetopt_args_info_help[1] ;
  args_info->engine_help = gengetopt_args_info_help[2] ;
  args_info->engine_port_help = gengetopt_args_info_help[3] ;
  args_info->connect_help = gengetopt_args_info_help[4] ;
  args_info->gui_help = gengetopt_args_info_help[5] ;
  args_info->client_port_help = gengetopt_args_info_help[6] ;
  args_info->load_help = gengetopt_args_info_help[7] ;
  args_info->path_help = gengetopt_args_info_help[8] ;
  args_info->run_help = gengetopt_args_info_help[9] ;
  args_info->parallelism_help = gengetopt_args_info_help[10] ;
  
}

void
cmdline_parser_print_version (void)
{
  printf ("%s %s\n", CMDLINE_PARSER_PACKAGE, CMDLINE_PARSER_VERSION);
}

void
cmdline_parser_print_help (void)
{
  int i = 0;
  cmdline_parser_print_version ();

  if (strlen(gengetopt_args_info_purpose) > 0)
    printf("\n%s\n", gengetopt_args_info_purpose);

  printf("\n%s\n\n", gengetopt_args_info_usage);

  if (strlen(gengetopt_args_info_description) > 0)
    printf("%s\n", gengetopt_args_info_description);

  while (gengetopt_args_info_help[i])
    printf("%s\n", gengetopt_args_info_help[i++]);
}

void
cmdline_parser_init (struct gengetopt_args_info *args_info)
{
  clear_given (args_info);
  clear_args (args_info);
  init_args_info (args_info);
}

struct cmdline_parser_params *
cmdline_parser_params_init()
{
  struct cmdline_parser_params *params = 
    (struct cmdline_parser_params *)malloc(sizeof(struct cmdline_parser_params));

  if (params)
    { 
      params->override = 0;
      params->initialize = 0;
      params->check_required = 0;
      params->check_ambiguity = 0;
    }
    
  return params;
}

static void
cmdline_parser_release (struct gengetopt_args_info *args_info)
{
  
  if (args_info->engine_port_orig)
    {
      free (args_info->engine_port_orig); /* free previous argument */
      args_info->engine_port_orig = 0;
    }
  if (args_info->connect_arg)
    {
      free (args_info->connect_arg); /* free previous argument */
      args_info->connect_arg = 0;
    }
  if (args_info->connect_orig)
    {
      free (args_info->connect_orig); /* free previous argument */
      args_info->connect_orig = 0;
    }
  if (args_info->client_port_orig)
    {
      free (args_info->client_port_orig); /* free previous argument */
      args_info->client_port_orig = 0;
    }
  if (args_info->load_arg)
    {
      free (args_info->load_arg); /* free previous argument */
      args_info->load_arg = 0;
    }
  if (args_info->load_orig)
    {
      free (args_info->load_orig); /* free previous argument */
      args_info->load_orig = 0;
    }
  if (args_info->path_arg)
    {
      free (args_info->path_arg); /* free previous argument */
      args_info->path_arg = 0;
    }
  if (args_info->path_orig)
    {
      free (args_info->path_orig); /* free previous argument */
      args_info->path_orig = 0;
    }
  if (args_info->run_arg)
    {
      free (args_info->run_arg); /* free previous argument */
      args_info->run_arg = 0;
    }
  if (args_info->run_orig)
    {
      free (args_info->run_orig); /* free previous argument */
      args_info->run_orig = 0;
    }
  if (args_info->parallelism_orig)
    {
      free (args_info->parallelism_orig); /* free previous argument */
      args_info->parallelism_orig = 0;
    }
  
  clear_given (args_info);
}

int
cmdline_parser_file_save(const char *filename, struct gengetopt_args_info *args_info)
{
  FILE *outfile;
  int i = 0;

  outfile = fopen(filename, "w");

  if (!outfile)
    {
      fprintf (stderr, "%s: cannot open file for writing: %s\n", CMDLINE_PARSER_PACKAGE, filename);
      return EXIT_FAILURE;
    }

  if (args_info->help_given) {
    fprintf(outfile, "%s\n", "help");
  }
  if (args_info->version_given) {
    fprintf(outfile, "%s\n", "version");
  }
  if (args_info->engine_given) {
    fprintf(outfile, "%s\n", "engine");
  }
  if (args_info->engine_port_given) {
    if (args_info->engine_port_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "engine-port", args_info->engine_port_orig);
    } else {
      fprintf(outfile, "%s\n", "engine-port");
    }
  }
  if (args_info->connect_given) {
    if (args_info->connect_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "connect", args_info->connect_orig);
    } else {
      fprintf(outfile, "%s\n", "connect");
    }
  }
  if (args_info->gui_given) {
    fprintf(outfile, "%s\n", "gui");
  }
  if (args_info->client_port_given) {
    if (args_info->client_port_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "client-port", args_info->client_port_orig);
    } else {
      fprintf(outfile, "%s\n", "client-port");
    }
  }
  if (args_info->load_given) {
    if (args_info->load_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "load", args_info->load_orig);
    } else {
      fprintf(outfile, "%s\n", "load");
    }
  }
  if (args_info->path_given) {
    if (args_info->path_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "path", args_info->path_orig);
    } else {
      fprintf(outfile, "%s\n", "path");
    }
  }
  if (args_info->run_given) {
    if (args_info->run_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "run", args_info->run_orig);
    } else {
      fprintf(outfile, "%s\n", "run");
    }
  }
  if (args_info->parallelism_given) {
    if (args_info->parallelism_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "parallelism", args_info->parallelism_orig);
    } else {
      fprintf(outfile, "%s\n", "parallelism");
    }
  }
  
  fclose (outfile);

  i = EXIT_SUCCESS;
  return i;
}

void
cmdline_parser_free (struct gengetopt_args_info *args_info)
{
  cmdline_parser_release (args_info);
}


/* gengetopt_strdup() */
/* strdup.c replacement of strdup, which is not standard */
char *
gengetopt_strdup (const char *s)
{
  char *result = NULL;
  if (!s)
    return result;

  result = (char*)malloc(strlen(s) + 1);
  if (result == (char*)0)
    return (char*)0;
  strcpy(result, s);
  return result;
}

int
cmdline_parser (int argc, char * const *argv, struct gengetopt_args_info *args_info)
{
  return cmdline_parser2 (argc, argv, args_info, 0, 1, 1);
}

int
cmdline_parser_ext (int argc, char * const *argv, struct gengetopt_args_info *args_info,
                   struct cmdline_parser_params *params)
{
  int result;
  result = cmdline_parser_internal (argc, argv, args_info, params, NULL);

  if (result == EXIT_FAILURE)
    {
      cmdline_parser_free (args_info);
      exit (EXIT_FAILURE);
    }
  
  return result;
}

int
cmdline_parser2 (int argc, char * const *argv, struct gengetopt_args_info *args_info, int override, int initialize, int check_required)
{
  int result;
  struct cmdline_parser_params params;
  
  params.override = override;
  params.initialize = initialize;
  params.check_required = check_required;
  params.check_ambiguity = 0;

  result = cmdline_parser_internal (argc, argv, args_info, &params, NULL);

  if (result == EXIT_FAILURE)
    {
      cmdline_parser_free (args_info);
      exit (EXIT_FAILURE);
    }
  
  return result;
}

int
cmdline_parser_required (struct gengetopt_args_info *args_info, const char *prog_name)
{
  return EXIT_SUCCESS;
}

int
cmdline_parser_internal (int argc, char * const *argv, struct gengetopt_args_info *args_info,
                        struct cmdline_parser_params *params, const char *additional_error)
{
  int c;	/* Character of the parsed option.  */

  int error = 0;
  struct gengetopt_args_info local_args_info;
  
  int override;
  int initialize;
  int check_required;
  int check_ambiguity;
  
  override = params->override;
  initialize = params->initialize;
  check_required = params->check_required;
  check_ambiguity = params->check_ambiguity;

  if (initialize)
    cmdline_parser_init (args_info);

  cmdline_parser_init (&local_args_info);

  optarg = 0;
  optind = 0;
  opterr = 1;
  optopt = '?';

  while (1)
    {
      int option_index = 0;
      char *stop_char;

      static struct option long_options[] = {
        { "help",	0, NULL, 'h' },
        { "version",	0, NULL, 'V' },
        { "engine",	0, NULL, 'e' },
        { "engine-port",	1, NULL, 'E' },
        { "connect",	1, NULL, 'c' },
        { "gui",	0, NULL, 'g' },
        { "client-port",	1, NULL, 'C' },
        { "load",	1, NULL, 'l' },
        { "path",	1, NULL, 'L' },
        { "run",	1, NULL, 'r' },
        { "parallelism",	1, NULL, 'p' },
        { NULL,	0, NULL, 0 }
      };

      stop_char = 0;
      c = getopt_long (argc, argv, "hVeE:c:gC:l:L:r:p:", long_options, &option_index);

      if (c == -1) break;	/* Exit from `while (1)' loop.  */

      switch (c)
        {
        case 'h':	/* Print help and exit.  */
          cmdline_parser_print_help ();
          cmdline_parser_free (&local_args_info);
          exit (EXIT_SUCCESS);

        case 'V':	/* Print version and exit.  */
          cmdline_parser_print_version ();
          cmdline_parser_free (&local_args_info);
          exit (EXIT_SUCCESS);

        case 'e':	/* Run (JACK) engine.  */
          if (local_args_info.engine_given || (check_ambiguity && args_info->engine_given))
            {
              fprintf (stderr, "%s: `--engine' (`-e') option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
              goto failure;
            }
          if (args_info->engine_given && ! override)
            continue;
          local_args_info.engine_given = 1;
          args_info->engine_given = 1;
          args_info->engine_flag = !(args_info->engine_flag);
          break;

        case 'E':	/* Engine OSC port.  */
          if (local_args_info.engine_port_given || (check_ambiguity && args_info->engine_port_given))
            {
              fprintf (stderr, "%s: `--engine-port' (`-E') option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
              goto failure;
            }
          if (args_info->engine_port_given && ! override)
            continue;
          local_args_info.engine_port_given = 1;
          args_info->engine_port_given = 1;
          args_info->engine_port_arg = strtol (optarg, &stop_char, 0);
          if (!(stop_char && *stop_char == '\0')) {
            fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
            goto failure;
          }
          if (args_info->engine_port_orig)
            free (args_info->engine_port_orig); /* free previous string */
          args_info->engine_port_orig = gengetopt_strdup (optarg);
          break;

        case 'c':	/* Connect to existing engine at OSC URI.  */
          if (local_args_info.connect_given || (check_ambiguity && args_info->connect_given))
            {
              fprintf (stderr, "%s: `--connect' (`-c') option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
              goto failure;
            }
          if (args_info->connect_given && ! override)
            continue;
          local_args_info.connect_given = 1;
          args_info->connect_given = 1;
          if (args_info->connect_arg)
            free (args_info->connect_arg); /* free previous string */
          args_info->connect_arg = gengetopt_strdup (optarg);
          if (args_info->connect_orig)
            free (args_info->connect_orig); /* free previous string */
          args_info->connect_orig = gengetopt_strdup (optarg);
          break;

        case 'g':	/* Launch the GTK graphical interface.  */
          if (local_args_info.gui_given || (check_ambiguity && args_info->gui_given))
            {
              fprintf (stderr, "%s: `--gui' (`-g') option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
              goto failure;
            }
          if (args_info->gui_given && ! override)
            continue;
          local_args_info.gui_given = 1;
          args_info->gui_given = 1;
          args_info->gui_flag = !(args_info->gui_flag);
          break;

        case 'C':	/* Client OSC port.  */
          if (local_args_info.client_port_given || (check_ambiguity && args_info->client_port_given))
            {
              fprintf (stderr, "%s: `--client-port' (`-C') option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
              goto failure;
            }
          if (args_info->client_port_given && ! override)
            continue;
          local_args_info.client_port_given = 1;
          args_info->client_port_given = 1;
          args_info->client_port_arg = strtol (optarg, &stop_char, 0);
          if (!(stop_char && *stop_char == '\0')) {
            fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
            goto failure;
          }
          if (args_info->client_port_orig)
            free (args_info->client_port_orig); /* free previous string */
          args_info->client_port_orig = gengetopt_strdup (optarg);
          break;

        case 'l':	/* Load patch.  */
          if (local_args_info.load_given || (check_ambiguity && args_info->load_given))
            {
              fprintf (stderr, "%s: `--load' (`-l') option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
              goto failure;
            }
          if (args_info->load_given && ! override)
            continue;
          local_args_info.load_given = 1;
          args_info->load_given = 1;
          if (args_info->load_arg)
            free (args_info->load_arg); /* free previous string */
          args_info->load_arg = gengetopt_strdup (optarg);
          if (args_info->load_orig)
            free (args_info->load_orig); /* free previous string */
          args_info->load_orig = gengetopt_strdup (optarg);
          break;

        case 'L':	/* Target path for loaded patch.  */
          if (local_args_info.path_given || (check_ambiguity && args_info->path_given))
            {
              fprintf (stderr, "%s: `--path' (`-L') option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
              goto failure;
            }
          if (args_info->path_given && ! override)
            continue;
          local_args_info.path_given = 1;
          args_info->path_given = 1;
          if (args_info->path_arg)
            free (args_info->path_arg); /* free previous string */
          args_info->path_arg = gengetopt_strdup (optarg);
          if (args_info->path_orig)
            free (args_info->path_orig); /* free previous string */
          args_info->path_orig = gengetopt_strdup (optarg);
          break;

        case 'r':	/* Run script.  */
          if (local_args_info.run_given || (check_ambiguity && args_info->run_given))
            {
              fprintf (stderr, "%s: `--run' (`-r') option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
              goto failure;
            }
          if (args_info->run_given && ! override)
            continue;
          local_args_info.run_given = 1;
          args_info->run_given = 1;
          if (args_info->run_arg)
            free (args_info->run_arg); /* free previous string */
          args_info->run_arg = gengetopt_strdup (optarg);
          if (args_info->run_orig)
            free (args_info->run_orig); /* free previous string */
          args_info->run_orig = gengetopt_strdup (optarg);
          break;

        case 'p':	/* Number of concurrent process threads.  */
          if (local_args_info.parallelism_given || (check_ambiguity && args_info->parallelism_given))
            {
              fprintf (stderr, "%s: `--parallelism' (`-p') option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
              goto failure;
            }
          if (args_info->parallelism_given && ! override)
            continue;
          local_args_info.parallelism_given = 1;
          args_info->parallelism_given = 1;
          args_info->parallelism_arg = strtol (optarg, &stop_char, 0);
          if (!(stop_char && *stop_char == '\0')) {
            fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
            goto failure;
          }
          if (args_info->parallelism_orig)
            free (args_info->parallelism_orig); /* free previous string */
          args_info->parallelism_orig = gengetopt_strdup (optarg);
          break;


        case 0:	/* Long option with no short option */
        case '?':	/* Invalid option.  */
          /* `getopt_long' already printed an error message.  */
          goto failure;

        default:	/* bug: option not considered.  */
          fprintf (stderr, "%s: option unknown: %c%s\n", CMDLINE_PARSER_PACKAGE, c, (additional_error ? additional_error : ""));
          abort ();
        } /* switch */
    } /* while */




  cmdline_parser_release (&local_args_info);

  if ( error )
    return (EXIT_FAILURE);

  return 0;

failure:
  
  cmdline_parser_release (&local_args_info);
  return (EXIT_FAILURE);
}

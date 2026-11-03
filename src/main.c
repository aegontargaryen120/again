#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

typedef enum {
  DIRECTIVE_ERR,
  DIRECTIVE_HELP,
  DIRECTIVE_VERSION,
  DIRECTIVE_RUN,
  DIRECTIVE_BUILD,
} Directive;

typedef struct {
  Directive directive;
  union {
    const char *filename;
    const char *error;
  } data;
} Argument;

Argument makeError(const char *message) {
  return (Argument) {
    .directive = DIRECTIVE_ERR,
    .data.error = message
  };
}

#define VERSION_INFO "again v0.1\nA tool for automating needless tasks, running shell commands, and writing build recipies\nOpen Source Software\n"

void version(void) {
  printf(VERSION_INFO);
}

/* char *strip(char *s) { */
/*   if (s == NULL) { */
/*     return NULL; */
/*   } */

/*   while (isspace((unsigned char)*s)) { */
/*     s++; */
/*   } */

/*   if (*s == '\0') { */
/*     return s; */
/*   } */

/*   char *end = s + strlen(s); */

/*   while (end > s && isspace((unsigned char)end[-1])) { */
/*     end--; */
/*   } */

/*   *end = '\0'; */

/*   return s; */
/* } */

const char *get_flag(const char *flag) {
  if (flag == NULL) {
    return NULL;
  }

  if (strcmp(flag, "-h") == 0 ||
      strcmp(flag, "--help") == 0) {
    return "help";
  }

  if (strcmp(flag, "-v") == 0 ||
      strcmp(flag, "--version") == 0) {
    return "version";
  }

  if (strcmp(flag, "-r") == 0 ||
      strcmp(flag, "--run") == 0) {
    return "run";
  }

  return NULL;
}

void help(const char *message, FILE *stream) {
#define print(...) fprintf(stream, __VA_ARGS__)
  
  print("Usage: again [OPTIONS]\n");
  print("OPTIONS:\n");
  print("     -h, --help          -> Prints this help to the terminal\n");
  print("     -v, --version       -> Prints the version information to the terminal\n");
  print("     -r, --run           -> Runs the next argument as a shell cmd file\n");
  print("\n");

  if (message != NULL && *message != '\0') {
    print("%s\n", message);    
  } 

#undef print
}

Argument parse_args(int argc, char **argv) {
  if (argc < 2) {
    return makeError("No argument provided.");
  }

  for (int i = 1; i < argc; i++) {
    const char *flag = get_flag(argv[i]);

    if (flag == NULL) {
      return makeError("Unknown or malformed flag.");
    }

    if (strcmp(flag, "help") == 0) {
      if (argc != 2) {
	return makeError("Unexpected additional arguments.");
      }

      return (Argument) {
	.directive = DIRECTIVE_HELP
      };
    }

    if (strcmp(flag, "version") == 0) {
      if (argc != 2) {
	return makeError("Unexpected additional arguments.");
      }

      return (Argument) {
	.directive = DIRECTIVE_VERSION
      };
    }

    if (strcmp(flag, "run") == 0) {
      if (i + 1 >= argc) {
	return makeError("--run requires an input file.");
      }

      const char *filename = argv[++i];

      if (i != argc - 1) {
	return makeError("Unexpected additional arguments.");
      }

      return (Argument) {
	.directive = DIRECTIVE_RUN,
	.data.filename = filename
      };
    }
  }

  return makeError("No valid directive provided.");
}

int run_file(const char *filename) {
  pid_t pid = fork();

  if (pid < 0) {
    perror("ERROR: Fork command failed.");
    return EXIT_FAILURE;
  }

  if (pid == 0) {
    execl("/bin/sh", "sh", filename, (char *)NULL);

    perror("ERROR: Execl command failed.");
    _exit(127);
  }

  int status;

  while (waitpid(pid, &status, 0) < 0) {
    if (errno == EINTR) {
      continue;
    }

    perror("ERROR: Waitpid function call failed.");
    return EXIT_FAILURE;
  }

  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }

  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }

  return EXIT_FAILURE;
}
  
int main(int argc, char **argv) {
  Argument arg = parse_args(argc, argv);

  switch (arg.directive) {
  case DIRECTIVE_ERR:
    fprintf(stderr, "%s\n", arg.data.error);
    return 1;

  case DIRECTIVE_HELP:
    help(NULL, stdout);
    return 0;

  case DIRECTIVE_VERSION:
    version();
    return 0;

  case DIRECTIVE_RUN:
    return run_file(arg.data.filename);

  case DIRECTIVE_BUILD:
    fprintf(stderr, "Not implemented.\n");
    return 1;

  default:
    fprintf(stderr, "ERROR: Arg not recognised.\n");
    return 1;
  }
}

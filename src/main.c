#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

typedef enum {
  DIRECTIVE_ERR,
  DIRECTIVE_HELP,
  DIRECTIVE_VERSION,
  DIRECTIVE_RUN,
  DIRECTIVE_BUILD,
  DIRECTIVE_CLEAN,
  DIRECTIVE_INIT,
  DIRECTIVE_TEST,
} Directive;

typedef struct {
  Directive directive;
  union {
    const char *filename;
    const char *error;
    const char *target;
    bool all;
  } data;
} Argument;

Argument makeError(const char *message) {
  return (Argument) {
    .directive = DIRECTIVE_ERR,
    .data.error = message
  };
}

#define VERSION_INFO "again v0.5\nA tool for building CXX projects and running shell commands.\n"

void version(void) {
  printf(VERSION_INFO);
}

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

  if (strcmp(flag, "-b") == 0 ||
      strcmp(flag, "--build") == 0) {
    return "build";
  }

  if (strcmp(flag, "-c") == 0 ||
      strcmp(flag, "--clean") == 0) {
    return "clean";
  }

  if (strcmp(flag, "-i") == 0 ||
      strcmp(flag, "--init") == 0) {
    return "init";
  }

  if (strcmp(flag, "-t") == 0 ||
      strcmp(flag, "--test") == 0) {
    return "test";
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
  print("     -b, --build         -> Builds the entire directory as a CXX project. NOTE: `build` directory must exist\n");
  print("     -c, --clean         -> Cleans out the build folder. NOTE: `build` directory must exist\n");
  print("     -i, --init          -> Initialises a folder for a CXX project\n");
  print("     -t, --test          -> Discovers, compiles, and runs all tests in project `tests` folder.\n");
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
	return makeError("Run requires an input file.");
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

    if (strcmp(flag, "build") == 0) {
      if (i + 1 >= argc) {
	return makeError("Build requires an input file.");
      }

      const char *target = argv[++i];

      if (i != argc - 1) {
	return makeError("Unexpected additional arguments.");
      }

      return (Argument) {
	.directive = DIRECTIVE_BUILD,
	.data.filename = target
      };
    }

    if (strcmp(flag, "clean") == 0) {
      if (argc >= i + 1) {
	if (strcmp(argv[i + 1], "--all") == 0) {
	  return (Argument) {
	    .directive = DIRECTIVE_CLEAN,
	    .data.all = true
	  };
	} else {
	  return (Argument) {
	    .directive = DIRECTIVE_CLEAN,
	    .data.target = argv[++i]
	  };
	}
      }
    }

    if (strcmp(flag, "init") == 0) {
      return (Argument) {
	.directive = DIRECTIVE_INIT
      };
    }

    if (strcmp(flag, "test") == 0) {
      return (Argument) {
	.directive = DIRECTIVE_TEST
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

int build(const char *target) {
  char command[512];

  snprintf(command, sizeof(command), "clang -Wall -Wextra -o build/%s src/*.c", target);

  printf("Building C project...\n");
  printf("-*-Cmd:-*-\t%s\n", command);

  int result = system(command);

  if (result == 0) {
    printf("Build successful!\n");
    return 0;
  } else {
    fprintf(stderr, "ERROR: Compilation failed.\n");
    return 1;
  }

  return 1;
}

int clean(bool all, const char *filename) {
  char command[512];

  if (!all) {
    snprintf(command, sizeof(command), "rm -rf build/%s", filename);
  } else {
    snprintf(command, sizeof(command), "rm -rf build/");
  }

  printf("Cleaning file(s)...\n");
  printf("-*-Cmd:-*-\t%s\n", command);

  int result = system(command);

  if (result == 0) {
    printf("Deletion successful!\n");
    return 0;
  } else {
    fprintf(stderr, "ERROR: Clean failed.\n");
    return 1;
  }

  return 1;
}

int initialize_dir(void) {
  char *command = "mkdir -p src include build";
  int result = system(command);

  if (result == 0) {
    printf("Initial directory creation successful!\n");
  } else {
    fprintf(stderr, "ERROR: Could not create initial directories.\n");
    return 1;
  }

  command = "echo 'int main(void) { return 0; }' >> src/main.c";
  result = system(command);

  if (result == 0) {
    printf("Full initialization successful!\n");
    return 0;
  } else {
    fprintf(stderr, "ERROR: Could not create initial file.\n");
    return 1;
  }

  return 1;
}

int test_dir(void) {
  char *command = "find tests/*";
  int result = system(command);

  if (result == 0) {
    printf("Tests discovered!\n");
  } else {
    fprintf(stderr, "ERROR: No test folder found.\n");
    return 1;
  }

  command = "clang -Wall -Wextra tests/*.c -o test";
  result = system(command);

  if (result == 0) {
    printf("Compiled tests!\n");
  } else {
    fprintf(stderr, "ERROR: Test compilation failed.\n");
    return 1;
  }

  command = "./test";
  result = system(command);

  if (result == 0) {
    printf("Tests successful!\n");
    return 0;
  } else {
    fprintf(stderr, "ERROR: Tests failed.\n");
    return 1;
  }

  return 1;
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
    return build(arg.data.filename);

  case DIRECTIVE_CLEAN:
    if (!arg.data.all) {
      return clean(false, arg.data.target);
    } else {
      return clean(true, NULL);
    }

  case DIRECTIVE_INIT:
    return initialize_dir();

  case DIRECTIVE_TEST:
    return test_dir();

  default:
    fprintf(stderr, "ERROR: Arg not recognised.\n");
    return 1;
  }
}

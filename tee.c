#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#define true (1)
#define false (0)

static int append;
static int ignore_interrupts;
static int tee_files(int, char**);

int main(int argc, char* argv[]) {
	int ok;
	int optc, nopts = 1;
	append = false;
	ignore_interrupts = false;
	while ((optc = getopt(argc, argv, "ai")) != -1) {
		switch (optc) {
		case 'a':
			append = true;
			nopts++;
			break;
		case 'i':
			ignore_interrupts = true;
			nopts++;
			break;
		}
	}
	if (ignore_interrupts) {
		signal(SIGINT, SIG_IGN);
	}
	ok = tee_files(argc - nopts, argv + nopts);
	if (close(STDIN_FILENO) == -1) {
		perror("Problem closing stind");
		ok = false;
	}
	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

int tee_files(int nfiles, char** files) {
	size_t n_outputs = 0;
	int* descriptors;
	char buffer[BUFSIZ];
	ssize_t bytes_read;
	int i, ok = true;
	char* strerror_message, message[BUFSIZ];
	const int mode = append ? O_APPEND|O_WRONLY : O_WRONLY|O_TRUNC;
	descriptors = malloc((nfiles + 1) * sizeof(int));
	descriptors[0] = STDOUT_FILENO;
	n_outputs++;
	for (i = 0; i < nfiles; i++) {
		descriptors[i+1] = open(files[i], mode);
		if (descriptors[i+1] < 0) {
			strerror_message = strerror(errno);
			sprintf(message, "Couldn't open \"%s\" file: %s\n", files[i], strerror_message);
			write(STDERR_FILENO, message, strlen(message));
			descriptors[i+1] = -1;
			ok = false;
		} else {
			n_outputs++;
		}
	}
	while (n_outputs) {
		bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
		if (bytes_read < 0 && errno == EINTR)
			continue;
		if (bytes_read <= 0)
			break;
		for (i = 0; i <= nfiles; i++) {
			if (descriptors[i] >= 0 && write(descriptors[i], buffer, bytes_read) < 0) {
				strerror_message = strerror(errno);
				sprintf(message, "Couldn't write to file: %s\n", strerror_message);
				write(STDERR_FILENO, message, strlen(message));
				ok = false;
				n_outputs--;
			} 
		}
	}
	if (bytes_read < 0) {
		strerror_message = strerror(errno);
		sprintf(message, "Read error: %s\n", strerror_message);
		write(STDERR_FILENO, message, strlen(message));
		ok = false;
	}
	for (i = 1; i <= nfiles; i++)
		if (descriptors[i] > 0 && close(descriptors[i]) != 0) {
			strerror_message = strerror(errno);
			sprintf(message, "Couldn't close file: %s\n", strerror_message); 
			write(STDERR_FILENO, message, strlen(message));
			ok = false;
		}
	free(descriptors);
	return ok;
}

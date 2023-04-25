/* Larissa 08/06/2021 Moysis Moysis */
/* This program rates another program written in c
 * using command line arguments to take the files
 * needed to run the student's program. */

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#define CHECK_SIZE 512

//Struct for every possible subcategory score. 
typedef struct {
	int compileScore;
	int timeScore;
	int outputScore;
	int memoryScore;
	int totalScore;
} score_t;

//Struct for the size and the array to get the arguments ti run the student's program. 
typedef struct {
	char **argumentArr;
	int size;
} arguments_t;

/* Get the files names from command line arguments and check if they are correct. 
 * Return 1 if the command line arguments are not correct. */
void get_arguments (int argc, char *argv[]) {
	if (argc != 6 || argv[5] <= 0) {
		exit (42);
	}
}

/* Action the program does after the timer ends (stops the P2 child that runs the program). */
static void timer_action (int sig) {}

/* Set the time in the timer. Alert for one time only (to kill the student's program if needed). */
void timer_set (struct itimerval *t, int timeout) {
	t->it_value.tv_sec = timeout;
	t->it_value.tv_usec = 0;
	t->it_interval.tv_sec = 0;
	t->it_interval.tv_usec=0;
	setitimer(ITIMER_REAL, t, NULL);
}

/* Resizes the heap memory array with two options. Add and remove.
 * Adds or removes one pointer to/from the array */
int get_heap_size (arguments_t *arguments) {
	int helpSize;
	char **helpPtr;
	
	helpSize = arguments->size + 1;
	
	helpPtr = (char **) realloc(arguments->argumentArr, helpSize * sizeof(char *));
	if (helpPtr != NULL) {
		arguments->argumentArr = helpPtr;
		arguments->size = helpSize;
	}
	else {
		return (-1);
	}
	
	return 0;
}

/* Reads all the bytes the user wants. */
int readall (int fd, void *buf, int readTotal) {
	int readBytes = 0, readNum;

	do {
		readNum = read(fd, &((char *) buf)[readBytes], readTotal - readBytes);
		
		if (readNum == 0) {
			break;
		}
		else if (readNum == -1) {
			perror("Read (76): ");
			return (-1);
		}
		
		readBytes += readNum;
	} while (readBytes < readTotal);
	
	return (readBytes);
}

/* Get the score from the program's compilation. Return -1 for system fail. */
int compile_score (int fd) {
	int readBytes, compScore = 0, result;
	char *strPos, checkArr[CHECK_SIZE + 1] = {'\0'};
	
	do {
		readBytes = readall(fd, checkArr, CHECK_SIZE);
		if (readBytes == 0) {
			break;
		}
		else if  (readBytes < CHECK_SIZE) {
			checkArr[readBytes] = '\0';
		}
		else if (readBytes == -1) {
			return (-1);
		}

		if (strstr(checkArr, " error: ") != NULL) {
			return (-100);
		}
		
		strPos = checkArr;
		while ((strPos = strstr(strPos, " warning: ")) != NULL) {
			strPos ++;
			compScore -= 5;
		}
		
		//Checks if the read 512 bytes stops inside the string warning or error. 
		if (readBytes != 9) {
			result = lseek(fd, -strlen(" warning: ") + 1, SEEK_CUR);
			if (result == -1) {
				return (-1);
			}
		}
   	} while (1);

	return (compScore);
}

/* Get all the arguments to run the program from the .args file. */
int get_file_arguments (arguments_t *arguments, char *argv[]) {
	int fileSize, fd, result, position = 1, readBytes;
	struct stat buf;
	char *heapString, *helpPtr;
	
	fd = open(argv[2], O_RDONLY);
	if (fd < 0) {
		return (-1);
	}
	
	result = fstat(fd, &buf);
	if (result == -1) {
		return (-1);
	}
	
	fileSize = buf.st_size;
	if (fileSize > 0) {
		heapString = calloc(fileSize + 1, sizeof(char));		
		if (heapString == NULL) {
			return (-1);
		}
		
	   	readBytes = readall(fd, heapString, fileSize);
	   	if (readBytes == -1) {
	   		return (-1);
	   	}
	   	else if (readBytes == 0) {
	   		arguments->argumentArr[1] = NULL;
	   		return 0;
	   	}
	   	heapString[fileSize - 1] = '\0';
		
		//Get all the argments from the array and get pointers pointing on every argument. 
		helpPtr = strtok(heapString, " ");
		while (helpPtr != NULL) {
			if (*helpPtr == ' ' || *helpPtr == '\n') {
				helpPtr ++;
			}
			arguments->argumentArr[position] = helpPtr;
			position ++;
			get_heap_size(arguments);
			helpPtr = strtok(NULL, " ");
		}
		arguments->argumentArr[position] = NULL;
	}
	
	return 0;
}

/* Print the final student's scores. */
void print_score (score_t scores) {
	
	scores.totalScore = scores.compileScore + scores.timeScore + scores.outputScore + scores.memoryScore;
	if (scores.totalScore < 0) {
		scores.totalScore = 0;
	}
	
	printf("\nCompilation: %d\n", scores.compileScore);
	printf("\nTimeout: %d\n", scores.timeScore);
	printf("\nOutput: %d\n", scores.outputScore);
	printf("\nMemory access: %d\n", scores.memoryScore);
	printf("\nScore: %d\n", scores.totalScore);
	
}	

// Initialize scores to zero. 
void score_init (score_t *scores) {
	scores->compileScore = 0;
	scores->timeScore = 0;
	scores->outputScore = 0;
	scores->memoryScore = 0;
	scores->totalScore = 0;
}

void get_free (arguments_t *arguments, char *errorName) {
	free(arguments->argumentArr[0]);
	free(arguments->argumentArr[1]);
	free(arguments->argumentArr);
	free(errorName);
}

int main (int argc, char *argv[]) {
	score_t scores;
	arguments_t arguments;
	struct itimerval time = {{0}};
	struct sigaction action = {{0}};
	int counter, pid, fd, status, fileDesc[2], fdIn, pid1, pid2, result, signal; 
	char *errorName, *execName;
	
	get_arguments(argc, argv);
	
	score_init(&scores);
	
	errorName = calloc(strlen(argv[1]) + 3, sizeof(char));
	if (errorName == NULL) {
		return -1;
	}
	execName = calloc(strlen(argv[1]) - 1, sizeof(char));
	if (execName == NULL) {
		return -1;
	}
	
	// Get the executable name. 
	for (counter = 0; counter < strlen(argv[1]) - 2; counter ++) {
		execName[counter] = argv[1][counter];
	}
	execName[counter] = '\0';
		
	// Get the .err file name.
	strcpy(errorName, execName);
	strcat(errorName, ".err");
 	
 	//Compile the program. 
	pid = fork();
	if (pid == 0) {		
		fd = open(errorName, O_RDWR | O_CREAT, 0700);
		if (fd < 0) {
			free(execName);
			free(errorName);
			perror("Open (237): ");
			return -1;
		}
		
		result = dup2(fd, STDERR_FILENO);
		if (result == -1) {
			free(execName);
			free(errorName);
			perror("Dup2 (243): ");
			return -1;
		}
		result = close(fd);
		if (result == -1) {
			free(execName);
			free(errorName);
			perror("Close (248): ");
			return -1;
		}
	
   		result = execlp("gcc", "gcc", "-Wall",  argv[1], "-o", execName, (char *) NULL);
   		if (result == -1) {
   			free(execName);
			free(errorName);
   			perror("Execlp (254): ");
   			return -1;
   		}
	}
	else if (pid == -1) {
		free(execName);
		free(errorName);
		perror("Pid: ");
		return -1;
	}
	
	waitpid(pid, &status, 0);
	if (WIFEXITED(status)) {
		result = WEXITSTATUS(status);
		if (result == -1) {
			free(execName);
			free(errorName);		
			return -1;
		}
	}
	
	fd = open(errorName, O_RDONLY);
	if (fd < 0) {
		free(execName);
		free(errorName);
		perror("Open (275): ");
		return 42;
	}
	
	scores.compileScore = compile_score(fd);

	if (scores.compileScore == -100) {
		// If the compilation is not right. 
		print_score(scores);
		free(execName);
		free(errorName);
		return 0;
	}
	else if (scores.compileScore == -1) {
		free(execName);
		free(errorName);
		return -1;
	}

	// Get the aruments pointer to string array initialized in 1. 
	arguments.argumentArr = (char **) calloc(2, sizeof(char *));
	if (arguments.argumentArr == NULL) {
		free(execName);
		free(errorName);
		return -1;
	}
	arguments.size = 2;
	arguments.argumentArr[0] = execName;
	
	result = get_file_arguments(&arguments, argv);
	if (result == -1) {
		get_free (&arguments, errorName);
		perror("Arguments function: ");
		return (-1);
	}
	
	action.sa_handler = timer_action;
	result = sigaction(SIGALRM, &action, NULL);
	if (result == -1) {
		get_free (&arguments, errorName);
		perror("Sigaction (333): ");
		return (-1);
	}
	
	result = pipe(fileDesc);
	if (result == -1) {
		get_free (&arguments, errorName);
		perror("Pipe (320): ");
		return -1;
	}
	
	pid1 = fork();
	//Run the program. 
	if (pid1 == 0) {
		result = close(fileDesc[0]);
		if (result == -1) {
			get_free (&arguments, errorName);
			perror("Close (332): ");
			return -1;
		}
		
		fdIn = open(argv[3], O_RDONLY);
		if (fdIn < 0) {
			get_free (&arguments, errorName);	
			perror("Open (339): ");
			return -1;
		}
		
		result = dup2(fdIn, STDIN_FILENO);
		if (result == -1) {
			get_free (&arguments, errorName);
			perror("Dup2 (345): ");
			return -1;
		}
	
		result = dup2(fileDesc[1], STDOUT_FILENO);
		if (result == -1) {	
			get_free (&arguments, errorName);
			perror("Dup2 (351): ");
			return -1;
		}

		result = close(fileDesc[1]);
		if (result == -1) {
			get_free (&arguments, errorName);	
			perror("Close (357): ");
			return -1;
		}
		
		result = execv(arguments.argumentArr[0], arguments.argumentArr);
		if (result == -1) {
			get_free (&arguments, errorName);
			perror("Execv (363): ");
			return -1;
		}
	}
	else if (pid1 == -1) {
		get_free (&arguments, errorName);
		perror("Pid1: ");
		return -1;
	}
		
	result = close(fileDesc[1]);
	if (result == -1) {
		get_free (&arguments, errorName);
		perror("Close (374): ");
		return -1;
	}
		
	//Run p4diff to find the output score. 
	pid2 = fork();
	if (pid2 == 0) {
		result = dup2(fileDesc[0], STDIN_FILENO);
		if (result == -1) {
			get_free (&arguments, errorName);
			perror("Dup2 (387): ");
			return -1;
		}

		result = close(fileDesc[0]);
		if (result == -1) {
			get_free (&arguments, errorName);
			perror("Close (393): ");
			return -1;
		}
		
		result = execl("p4diff", "p4diff", argv[4], (char *) NULL);
		if (result == -1) {
			get_free (&arguments, errorName);
			perror("Execl (399): ");
			return -1;
		}
	}
	else if (pid2 == -1) {
		get_free (&arguments, errorName);
		perror("Pid2: ");
		return -1;
	}
	
	result = close(fileDesc[0]);
	if (result == -1) {
		get_free (&arguments, errorName);
		perror("Close (410): ");
		return -1;
	}
	
	timer_set(&time, atoi(argv[5]));
	result = waitpid(pid1, &status, 0);
	if (result == -1 && errno == EINTR) {
		kill(pid1, SIGKILL);
		scores.timeScore= -100;
	}
	
	if (WIFSIGNALED(status)) {
		signal = WTERMSIG(status);
		if (signal == SIGSEGV || signal == SIGABRT || signal == SIGBUS) {
			scores.memoryScore -= 15;
		}
	}
	else if (WIFEXITED(status)) {
		result = WEXITSTATUS(status);
		if (result == -1) {
			get_free (&arguments, errorName);
			return -1;
		}
	}
	
	waitpid(pid2, &status, 0);
	if (WIFEXITED(status)) {
		scores.outputScore = WEXITSTATUS(status);
		if (scores.outputScore == -1) {
			get_free (&arguments, errorName);
			return -1;
		}
	}
	
	print_score(scores);
	get_free (&arguments, errorName);
	
	return 0;
}

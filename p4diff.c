/* Larissa 08/06/2021 Moysis Moysis */
/* This program rates the output of the student's program
 * and returns the % score of the output comparing to the 
 * expected output (get this from a file through command line argument). */
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include <errno.h>
#define BLOCK_BYTES 64

//Use to read every byte rom the file and the STDIN - pipe. 
int readall (int fd, void *buf, int readTotal) {
	int readBytes = 0, readNum;

	do {
		readNum = read(fd, &((char *) buf)[readBytes], readTotal - readBytes);
		
		if (readNum == 0) {
			break;
		}
		else if (readNum == -1) {
			return -1;
		}
		
		readBytes += readNum;
	} while (readBytes < readTotal);
	
	return (readBytes);
}

int main (int argc, char *argv[]) {
	int diffScore, fdOut, readBytes, studentBytes = 0, sameBytes = 0, counter, percent, outSize = 0, readBytes1;
	char studentOut[BLOCK_BYTES] = {'\0'}, compOut[BLOCK_BYTES] = {'\0'};
	
	fdOut = open(argv[1], O_RDONLY);
	if (fdOut < 0) {
		return -1;
	}
	
	do {
		readBytes = readall(fdOut, &compOut, BLOCK_BYTES);
		if (readBytes == -1) {
			return -1;
		}
		
		readBytes1 = readall(STDIN_FILENO, &studentOut, BLOCK_BYTES);
		if (readBytes == 0 && readBytes1 == 0) {
			break;
		}
		else if (readBytes1 == -1) {
			return -1;
		}
		
		//Compares byte to byte the expected output. 
		for (counter = 0; counter < readBytes && counter < readBytes1; counter ++) {
			if (studentOut[counter] == compOut[counter]) {
				sameBytes ++;
			}
		}
		studentBytes += readBytes1;
		outSize += readBytes;
	} while (1);
	
	//Get the final score. 
	percent = sameBytes * 100;
	if (outSize < studentBytes) {
		diffScore = percent / studentBytes;
	}
	else if (outSize == 0 && studentBytes == 0) {
		diffScore = 100;
	}
	else {
		diffScore = percent / outSize;
	}

	return (diffScore);
}

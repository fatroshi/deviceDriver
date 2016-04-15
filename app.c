#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define DEVICE "/dev/elise"

void kernelMsg(){
	printf("+------------------------------------------------------+\n");
	printf("| Kernel message:                                       \n");
	printf("|-----------------                                      \n");
	system("dmesg | grep elise | tail -n 1");
	printf("+------------------------------------------------------+\n");
}

int main(){

	int i; 						// Number of bytes written
	int fd;						// File descriptor
	char command;				// Command 
	char write_buffer[100];
	char read_buffer[100];

	fd = open(DEVICE, O_RDWR);	// Open device file for reading and writing

	// Check if file could be accessed
	if(fd == -1){
		printf("File %s either does not exist or has been locked by another process\n", DEVICE);
		exit(-1);
		kernelMsg();									// Display kernel messages (printk)
	}

	printf("r = Read from device file\nw = Write to device file\nx = quit\n");
		
	int run = 1;
	while(run == 1){
		// Get user input
		printf("Enter command: ");
		scanf(" %c", &command);
		switch(command){
			case 'w':
				printf("Enter data: ");
				scanf(" %[^\n]", write_buffer);					// Store the data in buffer
				write(fd,write_buffer,sizeof(write_buffer));	// 
				kernelMsg();									// Display kernel messages (printk)
				break;
			case 'r':
				read(fd,read_buffer,sizeof(read_buffer));		// 
				kernelMsg();									// Display kernel messages (printk)
				//printf("Device: %s\n", read_buffer);
				break;
			case 'x':
				run = 0;
				break;
			default:
				printf("Wrong command. \n");
				break;
		}
	}
	close(fd);
	kernelMsg();												// Display kernel messages (printk)
	return 0;
}

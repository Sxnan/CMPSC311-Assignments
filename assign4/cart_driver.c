///////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_driver.c
//  Description    : This is the implementation of the standardized IO functions
//                   for used to access the CRUD storage system.
//
//  Author         : Xuannan Su
//  Last Modified  : 10/26/2016
//

// Includes
#include <stdlib.h>
#include <string.h>

// Project Includes
#include <cart_driver.h>
#include <cart_controller.h>
#include <cart_cache.h>
#include <cart_network.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>

typedef enum{
	CLOSE = 0,		//The file is closed
	OPEN  = 1,		//The file is open
	NO_FILE = -1		//The file is not created
} FileStatus;

typedef struct{
	int cartridge;
	int frame;
} FileAddress;

typedef struct{
	char name[128];			//Name of the file
	int descriptor;			//File descriptor
	int length;			//Length of the file
	int position;			//Posisiton pointer of the file
	int num_of_address;		//number of memory frames assigned
	FileStatus file_status;		//File open/closed flag
	FileAddress *file_address;	//A list of the addresses of the memory frame assigned for this file
} FileAllocationTable;

typedef enum{
	CARTALLOC_RANDOM = 0,		
	CARTALLOC_LINEAR  = 1,
	CARTALLOC_BALANCED = 2	
} AllocStrategy;

//Global Data
static enum{
	OFF = 0,
	ON  = 1
} driver_status = OFF;			//Driver open/closed flag

static AllocStrategy alloc_mode = CARTALLOC_RANDOM;		//Memory allocation strategies

static FileAllocationTable *file_alloc_table;		//Table for files

static int num_of_file;		//Number of files in the driver

static int current_cart;		//The current cartridge this driver working on

static char frame_status[CART_MAX_CARTRIDGES][CART_CARTRIDGE_SIZE];		//An 2-D array to tell if the frame is occupied

//Function Prototypes

//Creat the opcode that will pass to the memory controller interface
CartXferRegister creat_cart_opcode(uint64_t ky1, uint64_t ky2, uint64_t ct1, uint64_t fm1);

//Extract the return code from the return value
int extract_cart_opcode(CartXferRegister resp);

//Initilize the file allocation table
int initialize_file_allocation_table();

//Generate a new descriptor
int generate_descriptor();

//Calculate the address index by the given position
int calculate_address_index(int position);

//Calculate the offset in the frame by the given position
int calculate_position_offset(int position);

//Chck if the given address is occupied
int address_occupied(FileAddress file_address);

//Generate the next available address
FileAddress generate_memory_address();

//Grow the file_address list
int grow_file_address_list(FileAllocationTable *file);

//Grow the file_alloc_table
int grow_file_alloc_table(FileAllocationTable **file_alloc_table);

//Load cart with current cart check 
int load_cart(int cart_num);

//
// Implementation

////////////////////////////////////////////////////////////////////////////////////
//
// Function	: creat_cart_opcode
// Description	: Creat the opcode that will pass to the memory controller interface
//
// Input	: ky1 - Key 1 register (8 bits)
//		  ky2 - Key 2 register (8 bits)
//		  ct1 - Cartridge register 1
//		  fm1 - Frame register 1
// Output	: 64 bits opcode

CartXferRegister creat_cart_opcode(uint64_t ky1, uint64_t ky2, uint64_t ct1, uint64_t fm1) {
	CartXferRegister opcode = 0;
	uint64_t tempky1, tempky2, tempct1, tempfm1;
	
	//Shift
	tempky1 = (ky1 & 0xff) << 56;
	tempky2 = (ky2 & 0xff) << 48;
	tempct1 = (ct1 & 0xffff) << 31;
	tempfm1 = (fm1 & 0xffff) << 15;

	//Combine
	opcode = tempky1 | tempky2 | tempct1 | tempfm1;

	//Return
	return opcode;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function	: extract_cart_opcode
// Description	: Extract the return code from response value
//
// Input	: resp - The response value from the op call
// Output	: The return code

int extract_cart_opcode(CartXferRegister resp) {

	uint64_t rt1 = (resp & 0x0000800000000000) >> 47;	//map out the rt1

	return (int)rt1;	//return
}

//////////////////////////////////////////////////////////////////////////////
//
// Function	: initialize_file_allocation_table
// Description	: Initialize the global file allocation table
//
// Input	: none
// Ouput	: 0 if successful

int initialize_file_allocation_table() {
	
	for (int i = 0; i < num_of_file; i++) {
	
		strcpy(file_alloc_table[i].name, "");		//initialize file name
		file_alloc_table[i].descriptor = -1;		//initialize file descriptor
		file_alloc_table[i].length = -1;		//initialize file length
		file_alloc_table[i].position = -1;		//initialize file position
		file_alloc_table[i].num_of_address = 0;		//initialize number of addresses be assigned
		file_alloc_table[i].file_status = NO_FILE;	//initialize file status
		
	}

	//initialize the frame_status table

	for (int i = 0; i < CART_MAX_CARTRIDGES; ++i){
		for (int j = 0; j < CART_CARTRIDGE_SIZE; ++j){
			frame_status[i][j] = 0;
		}
	}

	return 0;

}
////////////////////////////////////////////////////////////////////////////////
//
// Function	: generate_descriptor
// Description	: Generate a new descriptor
//
// Input	: none
// Output	: generated descriptor

int generate_descriptor() {

	static int descriptor = 0;
	descriptor += 1;
	return descriptor;

}

////////////////////////////////////////////////////////////////////////////////
// 
// Function	: calculate_address_index
// Description	: Calculate the address index by the given position
//
// Input	: position - The position of the file
// Output	: The index of the address list

int calculate_address_index(int position) {
	return (position / 1024);
}

///////////////////////////////////////////////////////////////////////////////
//
// Function	: calculate_position_offset
// Description	: Calculate the offset in the frame by the given position
//
// Input	: position - The position of the file
// Output	: The offset of the position in the frame

int calculate_position_offset(int position) {
	return (position % 1024);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function	: address_occupied
// Description	: Check if the given address is occupied
//
// Input	: file_address - The address that need to be checked
// Output	: 0 if it is not occupied, 1 if it is occupied

int address_occupied(FileAddress file_address) {

	if (frame_status[file_address.cartridge][file_address.frame] == 1){
		return(1);
	} else {
		return(0);
	}
		

}

////////////////////////////////////////////////////////////////////////////////
//
// Function	: generate_memory_address
// Description	: Generate the next available address
//
// Input	: none
// Output	: The new memory address 
//		  if no memory avaliable return invalid address {-1, -1}

FileAddress generate_memory_address() {

	FileAddress file_address;
	static int current_cart = 0;
	static int current_frame = 1023;
	static int frame_left = 1024 * 64;

	//Check if the memory is full
	if (frame_left == 0) {
		//assign an invalid address
		file_address.cartridge = -1;
		file_address.frame = -1;

		//Log Message
		logMessage(LOG_ERROR_LEVEL, "Memory allocation fail: Memory is full\n\n");	
		
		return file_address;
	}

	//Check the allocation strategy
	if (alloc_mode == CARTALLOC_RANDOM) {

		//Random Allocation
		//Generate random number for cart (0 -63)
		file_address.cartridge = getRandomValue(0, 63);

		//Generate random number for frame (0 - 1023)
		file_address.frame = getRandomValue(0, 1023);

		//step to the next address if occupied
		while (address_occupied(file_address)) {
			//Step to the next address while the address is occupied
			file_address.frame += 1;
			if (file_address.frame == 1024) {
				file_address.frame = 0;
				file_address.cartridge += 0;			
			}
		}

	} else if (alloc_mode == CARTALLOC_LINEAR) {
		//Linear Allocation
	
		file_address.cartridge = current_cart;
		file_address.frame = current_frame;
		
		//update the next address location
		if (current_frame == 0) {
			current_frame = 1023;
			current_cart = current_cart + 1;
		} else {
			current_frame = current_frame - 1;
		}

	} else if (alloc_mode == CARTALLOC_BALANCED) {
		//Balance Allocation

		//assign the avaliable address	
		file_address.cartridge = current_cart;
		file_address.frame = current_frame;
		
		//update the next address location
		if (current_cart == 63) {
			current_cart = 0;
			current_frame = current_frame -1;
		} else {
			current_cart = current_cart + 1;
		}

	} else {
		//log message
		logMessage(LOG_ERROR_LEVEL, "Memory allocation fail: Invalid allocation strategy\n\n");
		file_address.cartridge = -1;
		file_address.frame = -1;	
	}

	//update number of frame left
	frame_left -= 1;

	//update the frame status
	frame_status[file_address.cartridge][file_address.frame] = 1;
	
	return file_address;
}

//////////////////////////////////////////////////////////////////////////////////
//
// Function	: grow_file_address_list
// Description	: Grow the file_address list
//
// Input	: file - The pointer to the file contains the address list that need to grow
// Output	: 0 if successful, -1 if failure

int grow_file_address_list(FileAllocationTable *file) {
	
	//Check if the file has address
	if (file->num_of_address == 0) {
		//allocate memory for new address
		file->file_address = malloc(sizeof(FileAddress));
	} else {
		//reallocate memory for new address
		file->file_address = realloc(file->file_address, (file->num_of_address + 1) * sizeof(FileAddress));
	}
	
	//assign the new address of the frame
	file->file_address[file->num_of_address] = generate_memory_address();
	//check if the address valid
	if (file->file_address[file->num_of_address].frame == -1) {
		return -1;	
	}
	
	//increase the num_of_address
	file->num_of_address += 1;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
//
// Function	: grow_file_alloc_table
// Description	: Grow the file_alloc_table
//
// Input	: file_alloc_table - The table that need to grow
// Output	: 0 if successful

int grow_file_alloc_table(FileAllocationTable **file_alloc_table) {
	
	//check if there is file in the table
	if (num_of_file == 0) {
		//allocate memory for new file
		*file_alloc_table = malloc(sizeof(FileAllocationTable));
	} else {
		//reallocate memory for new file
		*file_alloc_table = realloc(*file_alloc_table, (num_of_file + 1) * sizeof(FileAllocationTable));
	}

	//increase the num_of_file
	num_of_file += 1;
	
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
//
// Function	: load_cart
// Description	: Load cart with current cart check
//
// Input	: cart_num - The cart number of the cart that need to load
// Output	: 0 if successful, -1 if failure

int load_cart(int cart_num) {

	if (current_cart != cart_num) {
		if (extract_cart_opcode(client_cart_bus_request(creat_cart_opcode(CART_OP_LDCART, 0, cart_num, 0), NULL)) == 1){
			logMessage(LOG_ERROR_LEVEL, "Cart %d Load op fail\n\n", cart_num);
			return(-1);
		}
		current_cart = cart_num;
	}
	
	return 0;

}
////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_poweron
// Description  : Startup up the CART interface, initialize filesystem
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t cart_poweron(void) {
	
	//Check if it is on
	if (driver_status == ON) {
		logMessage(LOG_ERROR_LEVEL, "The driver is already ON\n\n");
		return(-1);
	}
	
	//Initialize the cart interface
	if (extract_cart_opcode(client_cart_bus_request(creat_cart_opcode(CART_OP_INITMS, 0, 0, 0), NULL)) == 1) {
		return(-1);
	}
	
	current_cart = -1;

	//Zero all memory
	for (int i = 0; i < CART_MAX_CARTRIDGES; i++) {
		
		//load cart
		load_cart(i);

		if (extract_cart_opcode(client_cart_bus_request(creat_cart_opcode(CART_OP_BZERO, 0, 0, 0), NULL)) == 1){
				logMessage(LOG_ERROR_LEVEL, "Cart %d Zero op fail\n\n", i);
				return(-1);
		}
	}
			
	//Initialize internal data structure
	initialize_file_allocation_table();
	num_of_file = 0;
	
	// Initialize cache
	init_cart_cache();
	
	//Set drvier status open
	driver_status = ON;
	
	// Return successfully
	return(0);
			
	
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_poweroff
// Description  : Shut down the CART interface, close all files
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t cart_poweroff(void) {

	//Check if the driver is ON
	if (driver_status == OFF) {
		logMessage(LOG_ERROR_LEVEL, "The driver is not open.\n\n");
		return(-1);
	}

	//Execute shutdown opcode
	if (extract_cart_opcode(client_cart_bus_request(creat_cart_opcode(CART_OP_POWOFF,0,0,0), NULL)) == 1) {
		logMessage(LOG_ERROR_LEVEL, "Cart shundown op fail\n\n");
		return(-1);
	}

	//Close all file
	for (int i = 0; i < num_of_file; i++) {
		file_alloc_table[i].file_status = CLOSE;
	}

	//Clean up internal data structure
	for (int i = 0; i < num_of_file; i++)
		free(file_alloc_table[i].file_address);
	free(file_alloc_table);
	num_of_file = 0;
	
	// close cache
	close_cart_cache();

	//Set the driver status OFF
	driver_status = OFF;
	
	// Return successfully
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_open
// Description  : This function opens the file and returns a file handle
//
// Inputs       : path - filename of the file to open
// Outputs      : file handle if successful, -1 if failure

int16_t cart_open(char *path) {
	
	//Check if the driver is ON
	if (driver_status == OFF) {
		//Log error message
		logMessage(LOG_ERROR_LEVEL, "File open fail: The driver is OFF.\n\n");
		//Return failure
		return(-1);
	}
	
	//Check if the file exist
	for (int i = 0; i < num_of_file; i++) {
		if (strcmp(path, file_alloc_table[i].name) == 0) {
			//Check if the file already open
			if (file_alloc_table[i].file_status == OPEN) {
				//Log error message
				logMessage(LOG_ERROR_LEVEL, "File is already opened");
				//Return failure
				return(-1);
			} else {
				//Set the file status to open
				file_alloc_table[i].file_status = OPEN;
				//Set position to 0
				file_alloc_table[i].position = 0;
				//Return descriptor
				return (file_alloc_table[i].descriptor);
			}
		}
	}
	
	//Creat a file
	grow_file_alloc_table(&file_alloc_table);
	int descriptor = generate_descriptor();
	strcpy(file_alloc_table[num_of_file - 1].name, path);			//Set file name
	file_alloc_table[num_of_file - 1].descriptor = descriptor;		//Assign descriptor
	file_alloc_table[num_of_file - 1].length = 0;				//Set length to zero
	file_alloc_table[num_of_file - 1].position = 0;				//Set position to zero
	file_alloc_table[num_of_file - 1].file_status = OPEN;			//Set file_status to open
	file_alloc_table[num_of_file - 1].num_of_address = 0;			//Set num_of_address to zero
	
	grow_file_address_list(&file_alloc_table[num_of_file -1]);
	//Return the file descriptor
	return (descriptor);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_close
// Description  : This function closes the file
//
// Inputs       : fd - the file descriptor
// Outputs      : 0 if successful, -1 if failure

int16_t cart_close(int16_t fd) {
	
	int file_index = - 1;
	
	//Find the file index by the file descriptor
	for (int i = 0; i < num_of_file; i++) {
		if (fd == file_alloc_table[i].descriptor) {
			file_index = i;
			break;
		}
	}
	
	//Check if the descriptor matching or if the file_index is found
	if (file_index == -1) {
		//Log message
		logMessage(LOG_ERROR_LEVEL, "cart_close fail: The descriptor is invalid\n\n");
		//Return failure
		return(-1);
	}
	
	//Check if the file is open
	if (file_alloc_table[file_index].file_status == CLOSE) {
		//Log message
		logMessage(LOG_ERROR_LEVEL, "cart_close fail: The file is not open\n\n");
		//Return failure
		return(-1);
	}

	//Set the file status to CLOSE
	file_alloc_table[file_index].file_status = CLOSE;
	
	// Return successfully
	return (0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_read
// Description  : Reads "count" bytes from the file handle "fh" into the 
//                buffer "buf"
//
// Inputs       : fd - filename of the file to read from
//                buf - pointer to buffer to read into
//                count - number of bytes to read
// Outputs      : bytes read if successful, -1 if failure

int32_t cart_read(int16_t fd, void *buf, int32_t count) {

	int file_index = -1;

	//Find the index by the descriptor
	for (int i = 0; i < num_of_file; i++){
		if (fd == file_alloc_table[i].descriptor) {
			file_index = i;
			break;
		}
	}	
	
	//Check if the desciptor valid or is the file_index is found
	if (file_index == -1) {
		//Log message
		logMessage(LOG_ERROR_LEVEL, "cart read fail: The descriptor is invalid.\n\n ");
		//Return failure
		return(-1);
	}

	//Check if the file open
	if (file_alloc_table[file_index].file_status == CLOSE) {
		//Log message
		logMessage(LOG_ERROR_LEVEL, "cart read fail: The file is not open.\n\n");
		//Return failure
		return (-1);
	}

	
	//Check if the count greater than the number of bytes left in the file
	if (count > (file_alloc_table[file_index].length - file_alloc_table[file_index].position)) {
		//Set the count to the num fo bytes left in the file
		count = file_alloc_table[file_index].length - file_alloc_table[file_index].position;
	}
	
	//Copy from memory to the buffer
	int offset = calculate_position_offset(file_alloc_table[file_index].position);
	int address_index = calculate_address_index(file_alloc_table[file_index].position);
	int cart;
	int frame;
	void *temp;		//temp buffer to store the whole frame bytes
	temp = calloc(1024, sizeof(char));		//allocate memory for temp buffer

	//Check if read in only one frame
	if (count <= CART_FRAME_SIZE - offset) {
		cart = file_alloc_table[file_index].file_address[address_index].cartridge;
		frame = file_alloc_table[file_index].file_address[address_index].frame;
		
		// Check if in the cache
		if (get_cart_cache(cart, frame) != NULL){
			// Get from cache
			memcpy(temp, get_cart_cache(cart, frame), CART_FRAME_SIZE);
		} else {
			//load cart
			load_cart(cart);
		
			//read frame
			if (extract_cart_opcode(client_cart_bus_request(creat_cart_opcode(CART_OP_RDFRME,0,0, frame), temp)) == 1) {
				logMessage(LOG_ERROR_LEVEL, "Cart read op fail\n\n");
				return(-1);
			} 

			// Put into the cache
			put_cart_cache(cart, frame, temp);
		}

		//copy count number of bytes to buffer
		memcpy(buf, (char *)temp + offset, count);
		

	} else {	//read cross frames
		int count_first_frame = CART_FRAME_SIZE - offset;			//number of bytes read from the first frame
		int count_last_frame = (count - count_first_frame) % CART_FRAME_SIZE;	//number of bytes read from the last frame
		int num_of_frame = (count - count_first_frame) / CART_FRAME_SIZE + 2;	//number of frame that read from
		int buff_length = 0;							//the length of the buff
		
		for (int i = 0; i < num_of_frame; i++) {

			cart = file_alloc_table[file_index].file_address[address_index + i].cartridge;				
			frame = file_alloc_table[file_index].file_address[address_index + i].frame;

			//Check if in the cache
			if (get_cart_cache(cart, frame) != NULL){
				// Get from cache
				memcpy(temp, get_cart_cache(cart, frame), CART_FRAME_SIZE);
			} else {
				//load cart	
				load_cart(cart);
	
				//read frame
				if (extract_cart_opcode(client_cart_bus_request(creat_cart_opcode(CART_OP_RDFRME,0,0, frame), temp)) == 1) {
					logMessage(LOG_ERROR_LEVEL, "Cart read op fail\n\n");
					return(-1);
				}
				
				// Put into the cache
				put_cart_cache(cart, frame, temp);
			}

			//Check if it is first frame
			if (i == 0) {
				//read the bytes left in that frame
				
				//copy memory
				memcpy((char *)buf, (char *)temp + offset, count_first_frame);
				buff_length += count_first_frame;

			} else if (i == num_of_frame - 1) {		//if it is last frame
				
				//read the bytes to the boundary
				//copy memory
				memcpy((char *)buf + buff_length, (char *)temp , count_last_frame);
				buff_length += count_last_frame;

			} else {		//if it is middle frame

				//read the whole frame
				
				//copy memory
				memcpy((char *)buf + buff_length, (char *)temp, CART_FRAME_SIZE);
				buff_length += CART_FRAME_SIZE;
			}


					
		}

	}

	//increase the position
	file_alloc_table[file_index].position += count;

	//deallocate
	free(temp);

	// Return successfully
	return (count);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_write
// Description  : Writes "count" bytes to the file handle "fh" from the 
//                buffer  "buf"
//
// Inputs       : fd - filename of the file to write to
//                buf - pointer to buffer to write from
//                count - number of bytes to write
// Outputs      : bytes written if successful, -1 if failure

int32_t cart_write(int16_t fd, void *buf, int32_t count) {

	int file_index = -1;		//Default file index to invalid number -1

	//Find the index by the descriptor
	for (int i = 0; i < num_of_file; i++){
		if (fd == file_alloc_table[i].descriptor) {
			file_index = i;
			break;
		}
	}	
	
	//Check if the desciptor valid
	if (file_index == -1) {
		//Log message
		logMessage(LOG_ERROR_LEVEL, "cart read fail: The descriptor is invalid.\n\n ");
		//Return failure
		return(-1);
	}

	//Check if the file open
	if (file_alloc_table[file_index].file_status == CLOSE) {
		//Log message
		logMessage(LOG_ERROR_LEVEL, "cart read fail: The file is not open.\n\n");
		//Return failure
		return (-1);
	}

	int offset = calculate_position_offset(file_alloc_table[file_index].position);
	int address_index = calculate_address_index(file_alloc_table[file_index].position);
	int cart;
	int frame;
	void *temp;		//temp buffer to process the whole frame bytes
	int length_increament;		//the increament of length of size
	temp = calloc(1024, sizeof(char));		//allocate memory to temp pointer

	//check if write beyond the file length
	if (file_alloc_table[file_index].position + count > file_alloc_table[file_index].length) {
		
		length_increament = file_alloc_table[file_index].position + count - file_alloc_table[file_index].length;

	} else {
	
		length_increament = 0;
	}

	cart = file_alloc_table[file_index].file_address[address_index].cartridge;
	frame = file_alloc_table[file_index].file_address[address_index].frame;
	
	//Check if it write in only one frame
	if (count <= CART_FRAME_SIZE - offset) {
		
		// Check if in the cache
		if (get_cart_cache(cart, frame) != NULL){
			// Get from the cache
			memcpy(temp, get_cart_cache(cart, frame), CART_FRAME_SIZE);
			
			// load cart
			load_cart(cart);
		} else {
			//load cart
			load_cart(cart);
		
			//read frame
			if (extract_cart_opcode(client_cart_bus_request(creat_cart_opcode(CART_OP_RDFRME,0,0, frame), temp)) == 1) {
				logMessage(LOG_ERROR_LEVEL, "Cart read op fail\n\n");
				return(-1);
			} 

			// Put into the cache
			put_cart_cache(cart, frame, temp);
		}

		//copy memory from the buffer
		memcpy((char *)temp + offset, (char *)buf, count);

		// Put into the cache
		put_cart_cache(cart, frame, temp);

		//write to frame
		if (extract_cart_opcode(client_cart_bus_request(creat_cart_opcode(CART_OP_WRFRME,0, 0, frame), temp)) == 1) {
			logMessage(LOG_ERROR_LEVEL, "Cart write fail\n\n", cart);
			return(-1);
		}


	} else {	//if write cross the frame
		
		int count_first_frame = CART_FRAME_SIZE - offset;			//number of bytes write to the first frame
		int count_last_frame = (count - count_first_frame) % CART_FRAME_SIZE;	//number of bytes write to the last frame
		int num_of_frame = (count - count_first_frame) / CART_FRAME_SIZE + 2;	//number of frame that write to
		int num_of_byte_written = 0;

		for (int i = 0; i < num_of_frame; i++) {

			//check if new memory needed
			if (address_index + i == file_alloc_table[file_index].num_of_address - 1) {
				//allocate new address
				grow_file_address_list(&file_alloc_table[file_index]);
			}

			cart = file_alloc_table[file_index].file_address[address_index + i].cartridge;				
			frame = file_alloc_table[file_index].file_address[address_index + i].frame;

			// Check if in the cache
			if (get_cart_cache(cart, frame) != NULL){
				// Get from the cache
				memcpy(temp, get_cart_cache(cart, frame), CART_FRAME_SIZE);

				// load cart
				load_cart(cart);

			} else {
	
				//load cart
				load_cart(cart);
	
				//read frame
				if (extract_cart_opcode(client_cart_bus_request(creat_cart_opcode(CART_OP_RDFRME,0,0, frame), temp)) == 1) {
					logMessage(LOG_ERROR_LEVEL, "Cart read op fail\n\n");
					return(-1);
				}

				// Put into the cache
				put_cart_cache(cart, frame, temp);
			}
			//Check if it is first frame
			if (i == 0) {
		
				//copy memory from the buffer
				memcpy((char *)temp + offset, (char *)buf + num_of_byte_written, count_first_frame);
				num_of_byte_written += count_first_frame;

			} else if (i == num_of_frame - 1) {		//if it is last frame
				
				//copy memory from the buffer
				memcpy((char *)temp, (char *)buf + num_of_byte_written, count_last_frame);
				num_of_byte_written += count_last_frame;

			} else {		//if it is middle frame
	
				//copy memory from the buffer
				memcpy((char *)temp, (char *)buf + num_of_byte_written, CART_FRAME_SIZE);
				num_of_byte_written += CART_FRAME_SIZE;
			}

			//put to the cache
			put_cart_cache(cart, frame, temp);

			//write to frame
			if (extract_cart_opcode(client_cart_bus_request(creat_cart_opcode(CART_OP_WRFRME,0, 0, frame), temp)) == 1) {
				logMessage(LOG_ERROR_LEVEL, "Cart write fail\n\n", cart);
				return(-1);
			}
			
		
		}

	}

	//increase the position
	file_alloc_table[file_index].position += count;

	//increase the length of file
	file_alloc_table[file_index].length += length_increament;

	//deallocate
	free(temp);

	// Return successfully
	return (count);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_seek
// Description  : Seek to specific point in the file
//
// Inputs       : fd - filename of the file to write to
//                loc - offfset of file in relation to beginning of file
// Outputs      : 0 if successful, -1 if failure

int32_t cart_seek(int16_t fd, uint32_t loc) {

	int file_index = -1;

	//Find the index by the descriptor
	for (int i = 0; i < num_of_file; i++){
		if (fd == file_alloc_table[i].descriptor) {
			file_index = i;
			break;
		}
	}	
	
	//Check if the desciptor valid or is the file_index is found
	if (file_index == -1) {
		//Log message
		logMessage(LOG_ERROR_LEVEL, "cart read fail: The descriptor is invalid.\n\n ");
		//Return failure
		return(-1);
	}

	//Check if the file open
	if (file_alloc_table[file_index].file_status == CLOSE) {
		//Log message
		logMessage(LOG_ERROR_LEVEL, "cart read fail: The file is not open.\n\n");
		//Return failure
		return (-1);
	}

	//Check if the loc beyond the length of the file
	if (loc > file_alloc_table[file_index].length) {
		//Log message
		logMessage(LOG_ERROR_LEVEL, "cart_seek fail: beyond the length of the file.\n\n");
		//Return failure
		return (-1);	
	}

	file_alloc_table[file_index].position = loc;

	// Return successfully
	return (0);
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function	: cart_setMode
// Description	: Set the driver's memory allocation mode
//
// Input	: alloc_strategy - The strategy to set
// Output	: 0 if successful

int32_t cart_setMode(AllocStrategy alloc_strategy) {
	
	alloc_mode = alloc_strategy;

	return 0;

}

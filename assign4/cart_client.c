////////////////////////////////////////////////////////////////////////////////
//
//  File          : cart_client.c
//  Description   : This is the client side of the CART communication protocol.
//
//   Author       : ????
//  Last Modified : ????
//

// Include Files
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <gcrypt.h>

// Project Include Files
#include <cart_network.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>

//
//  Global data
int client_socket = -1;
int                cart_network_shutdown = 0;   // Flag indicating shutdown
unsigned char     *cart_network_address = NULL; // Address of CART server
unsigned short     cart_network_port = 0;       // Port of CART serve
unsigned long      CartControllerLLevel = 0; // Controller log level (global)
unsigned long      CartDriverLLevel = 0;     // Driver log level (global)
unsigned long      CartSimulatorLLevel = 0;  // Driver log level (global)
char key[16];		// Key for encryption
int key_generated = 0; 		// Flag indicating if key is generated

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : client_cart_bus_request
// Description  : This the client operation that sends a request to the CART
//                server process.   It will:
//
//                1) if INIT make a connection to the server
//                2) send any request to the server, returning results
//                3) if CLOSE, will close the connection
//
// Inputs       : reg - the request reqisters for the command
//                buf - the block to be read/written from (READ/WRITE)
// Outputs      : the response structure encoded as needed

CartXferRegister client_cart_bus_request(CartXferRegister reg, void *buf) {

	struct sockaddr_in addr;		
	char *cart_ip = CART_DEFAULT_IP;	// server ip
	uint64_t code = htonll64(reg);		// network order command to send
	uint64_t rcode;			// return code
	int ky1 = reg >> 56;		// the opcode in reg
	char *message;			// the message to send to the server
	char *response;			// the response from the server
	char encrypted[1024];		// the encrypted fram
	char decrypted[1024];		// decrypted frame
	gcry_cipher_hd_t hd;		// gcrypt handler

	// Initialize gcrypt
	if (!gcry_check_version("1.6.5")){
		printf("gcrypt version doesn't match\n");
		return -1;
	}

	gcry_control(GCRYCTL_DISABLE_SECMEM, 0);	// disable secured memory
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);	// finish initialization
	
	gcry_cipher_open(&hd, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_ECB, 0);	// open gcrypt handler

	// generate key
	if (!key_generated) {
		getRandomData(key, 16);
		key_generated = 1;
	}

	gcry_cipher_setkey(hd, key, 16);	// set key

	cart_network_port = CART_DEFAULT_PORT;		// set the default port

	addr.sin_family = AF_INET;
	addr.sin_port = htons(cart_network_port);

	// if initial cart establish connection
	if (ky1 == CART_OP_INITMS){
			
		if (inet_aton(cart_ip, &addr.sin_addr) == 0){
			return -1;
		}

		client_socket = socket(PF_INET, SOCK_STREAM, 0);
		if (client_socket == -1){
			logMessage(LOG_ERROR_LEVEL, "Error on socket creation\n");
			return -1;
		}

		if ( connect(client_socket, (const struct sockaddr *)&addr, sizeof(addr)) == -1){
			logMessage(LOG_ERROR_LEVEL, "Error on connect\n");
			return -1;
		}
		cart_network_shutdown = 1;
	
	}
	
	// Check if it is write frame
	if (ky1 == CART_OP_WRFRME){
		// if it is write frame
		message = malloc(1032*sizeof(char));		// Allocata memory for sent message
		memcpy(message, &code, 8);		// Copy command code to the beginning of the message
		gcry_cipher_encrypt(hd, encrypted, 1024, buf, 1024);		// encrypt frame
		memcpy(message+8, encrypted, 1024);		// Follow by the content in the buf
		
		// Sent the message
		if ( write (client_socket, message, 1032) != 1032){
			logMessage(LOG_ERROR_LEVEL, "Error sending command\n");
			return -1;
		}

	}else{
		// not write frame
		message = malloc(8*sizeof(char));	// Allocate memory for sent message
		memcpy(message, &code, 8);		// Copy command code to the message

		// Send the message
		if ( write (client_socket, message, 8) != 8){
			logMessage(LOG_ERROR_LEVEL, "Error sending command\n");
			return -1;
		}

	}
	
	// Check if it is read frame
	if (ky1 == CART_OP_RDFRME){

		// If it is read frame
		response = malloc(1032*sizeof(char));		// Allocate memory to store the response	

		// Recieve the response
		if (read(client_socket, response, 1032) != 1032){
			logMessage(LOG_ERROR_LEVEL, "Error reading return code \n");
			return -1;
		}

		gcry_cipher_decrypt(hd, decrypted, 1024, response+8, 1024);

		memcpy(&rcode, response, 8);		// Get the return code in network order
		memcpy(buf, decrypted, 1024);		// Copy the read content to the buf

	}else {
		
		// not read frame
		response = malloc(8 * sizeof(char));	// Allocate memory to store the reaponse

		// Receive the response	
		if (read(client_socket, response, 8) != 8){
			logMessage(LOG_ERROR_LEVEL, "Error reading return code \n");
			return -1;
		}

		memcpy(&rcode, response, 8);		// Get the return code in network order
	
	}

	rcode = ntohll64(rcode);		// change to host order

	// If is it poweroff
	if (ky1 == CART_OP_POWOFF) {

		// Close the socket
		close(client_socket);
		cart_network_shutdown = 0;		
	}
	
	// deallocate
	free(message);
	free(response);
	
	return rcode;
}

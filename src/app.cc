#include "mongoose.h"

int main(void) 
	{
	struct mg_server *server = mg_create_server(NULL, NULL);
	mg_set_option(server, "document_root", ".");      // Serve current directory
	mg_set_option(server, "listening_port", "8080");  // Open port 8080
	
	for (;;) {
		mg_poll_server(server, 1000);   // Infinite loop, Ctrl-C to stop
		}
	mg_destroy_server(&server);
	
	return 0;
	}

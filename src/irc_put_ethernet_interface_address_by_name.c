#include <ifaddrs.h>
#include <string.h>

/**
 * Put the address of an interface on a null-terminated char buffer with a maximum length.
 * If it does not exist, an empty string will be written to the buffer.
 *
 * @param name 	       The char array with the name of the interface to be searched for.
 * @param buffer       The buffer to be written to.
 * @param buffer_size  The maximum length to write to the buffer.
 *
 * @return             Success indicator, zero if there was an error or the interface was not found. Returns 1 (one) for success.
 */
int irc_put_ethernet_interface_address_by_name(char * name, char * buffer, unsigned int buffer_size);

// Implementation

int irc_put_ethernet_interface_address_by_name(char * name, char * buffer, unsigned int buffer_size) {
	int exists = 0;
	int count = 0;
	struct ifaddrs *addrs, *tmp;
	getifaddrs(&addrs);
	tmp = addrs;
	int i;

	while (tmp) {
		if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET) {
			if (strncmp(name, tmp->ifa_name, buffer_size) == 0) {
				exists = 1;
				for (i=0;i<buffer_size && i < 16;i++) {
					buffer[i] = tmp->ifa_addr->sa_data[i+10];
				}
				if (i < buffer_size-1) {
					if (tmp->ifa_addr->sa_data[i] != '\0') {
						buffer[i] = '\0';
					}
				} else {
					buffer[buffer_size-1] = '\0';
				}
				break;
			}
			count++;
		}
		tmp = tmp->ifa_next;
	}
	freeifaddrs(addrs);
	if (exists == 0 && buffer_size > 0) {
		buffer[0] = '\0';
		return 0;
	}
	return 1;
}
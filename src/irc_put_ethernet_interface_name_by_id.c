#include <ifaddrs.h>

/**
 * Put the name of an interface on a null-terminated char buffer with a maximum length.
 * If it does not exist, an empty string will be written to the buffer.
 *
 * If a given id does not exist it is guaranted that there are no other interfaces with higher ids.
 *
 * @param id 	       The index of the interface from the interface list (starting at zero).
 * @param buffer       The buffer to be written to.
 * @param buffer_size  The maximum length to write to the buffer.
 *
 * @return             Success indicator, zero if there was an error or the interface was not found. Returns 1 (one) for success.
 */
int irc_put_ethernet_interface_name_by_id(int id, char * buffer, unsigned int buffer_size);

// Implementation

int irc_put_ethernet_interface_name_by_id(int id, char * buffer, unsigned int buffer_size) {
	int exists = 0;
	int count = 0;
	struct ifaddrs *addrs, *tmp;
	getifaddrs(&addrs);
	tmp = addrs;
	int i;
	while (tmp) {
		if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET) {
			if (id == count) {
				exists = 1;
				for (i=0;i<buffer_size;i++) {
					buffer[i] = tmp->ifa_name[i];
					if (tmp->ifa_name[i] == '\0' || tmp->ifa_name[i] == '\n' || tmp->ifa_name[i] == '\r') {
						break;
					}
				}
				if (i < buffer_size-1) {
					if (tmp->ifa_name[i] != '\0') {
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
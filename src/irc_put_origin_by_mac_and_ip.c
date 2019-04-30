
int irc_put_origin_by_mac_and_ip(char * mac, char * ip, char * output, unsigned int output_size) {
	unsigned char binary_mac[6+1];
	unsigned char binary_ip[16+1];

	_irc_mac_to_binary(mac, binary_mac);
	_irc_ip_to_binary(ip, binary_ip);

	snprintf(
		output,
		output_size,
		"%02X:%02X:%02X:%02X:%02X:%02X|%d.%d.%d.%d",
		binary_mac[0] & 0xFF,
		binary_mac[1] & 0xFF,
		binary_mac[2] & 0xFF,
		binary_mac[3] & 0xFF,
		binary_mac[4] & 0xFF,
		binary_mac[5] & 0xFF,
		binary_ip[0] & 0xFF,
		binary_ip[1] & 0xFF,
		binary_ip[2] & 0xFF,
		binary_ip[3] & 0xFF
	);
	output[output_size-1] = '\0';

	return 1;
}

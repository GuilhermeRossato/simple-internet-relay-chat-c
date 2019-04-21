/*--------------------------------------------------------*/
/* DHCP Spoofer - Captura pacotes ethernet, encontra DNS  */
/*  e responde solicitacoes de dns como um server.        */
/*--------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

/* Diretorios: net, netinet, linux contem os includes que descrevem */
/* as estruturas de dados do header dos protocolos   	  	        */

#include <net/if.h>  //estrutura ifr
#include <netinet/ether.h> //header ethernet
#include <netinet/in.h> //definicao de protocolos
#include <arpa/inet.h> //funcoes para manipulacao de enderecos IP

#include <netinet/in_systm.h> //tipos de dados

#include <unistd.h> // exec
#include <ifaddrs.h> // Interfaces disponiveis

#define BUFFSIZE 1518
#define DEBUG 1

unsigned char buff1[BUFFSIZE]; // buffer de recepcao

int sockd;

struct ifreq ifr;

typedef struct {
	char target[6];
	char source[6];
	char length[2];
} ethernet_packet_start;

typedef enum {
	Eth_UNKNOWN,
	Eth_RAW_ETHERNET,
	Eth_IPv4,
	Eth_IPv6,
	Eth_ARP,
	Eth_IPX
} ethernet_content_type;

int is_broadcast_mac(char target[6]) {
	return (target[0] & 0xff == 0xff && target[1] & 0xff == 0xff && target[2] & 0xff == 0xff && target[3] & 0xff == 0xff && target[4] & 0xff == 0xff && target[5] & 0xff == 0xff);
}

int is_origin_mac(char origin[6]) {
	return (origin[0] & 0xff == 0 && origin[1] & 0xff == 0 && origin[2] & 0xff == 0 && origin[3] & 0xff == 0 && origin[4] & 0xff == 0 && origin[5] & 0xff == 0);
}

// print bits of a value
//
// usage:
//
// int i = 23;
// printBits(sizeof(i), &i);
void print_bits_of_data(size_t const size, void const * const ptr) {
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=size-1;i>=0;i--)
    {
        for (j=7;j>=0;j--)
        {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}

int on_bootstrap_received(char * bootstrap, int data_length) {
	char transaction_id[4];
	char client_mac_address[6];

	memcpy(transaction_id, bootstrap+4, 4);
	memcpy(client_mac_address, bootstrap+28, 6);

	printf("      [Bootp] Unknown data:\n");
	printf(
		"      (0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x)\n",
		bootstrap[0] & 0xFF,
		bootstrap[1] & 0xFF,
		bootstrap[2] & 0xFF,
		bootstrap[3] & 0xFF,
		bootstrap[4] & 0xFF,
		bootstrap[5] & 0xFF,
		bootstrap[6] & 0xFF,
		bootstrap[7] & 0xFF,
		bootstrap[8] & 0xFF
	);
	return 1;
}

void on_udp_received(char * udp_data, int data_length) {
	int source_port = (udp_data[0] & 0xff << 8) + udp_data[1] & 0xff;
	int target_port = (udp_data[2] & 0xff << 8) + udp_data[3] & 0xff;
	int total_length = (udp_data[4] & 0xff << 8) + udp_data[5] & 0xff;
	printf("    [UDP] Size: %5d - Source Port: %5d - Target Port %5d\n", total_length, source_port, target_port);
	on_bootstrap_received(udp_data + 8, data_length - 8);
}

void on_ipv4_broadcast(char * ipv4_data, int data_length) {
	int ip_version = ipv4_data[0] & 0xf0 >> 2;
	int header_length = (ipv4_data[0] & 0x0f)*4;
	int total_length = (ipv4_data[2] & 0xff << 8) + ipv4_data[3] & 0xff;
	if (ip_version != 4) {
		printf("Erro: Pacote IPv4 nao deveria ter versao %d\n", ip_version);
		printf("Inicio do IPv4: 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x\n", ipv4_data[0] & 0xff, ipv4_data[1] & 0xff, ipv4_data[2] & 0xff, ipv4_data[3] & 0xff, ipv4_data[4] & 0xff, ipv4_data[5] & 0xff);
		exit(1);
	}
	if (header_length == 20) {
		// todos pacotes DHCP tem 20 bytes e portanto nao usam o campo opcoes do ipv4
		printf(
			"  [IPv4] Size: %5d - Header Size: %d - Origin Ip: %d.%d.%d.%d - Target Ip: %d.%d.%d.%d\n",
			total_length,
			header_length,
			ipv4_data[12] & 0xff,
			ipv4_data[13] & 0xff,
			ipv4_data[14] & 0xff,
			ipv4_data[15] & 0xff,
			ipv4_data[16] & 0xff,
			ipv4_data[17] & 0xff,
			ipv4_data[18] & 0xff,
			ipv4_data[19] & 0xff
		);
		//printf("origin: %d.%d.%d.%d\n", );
		//printf("target: %d.%d.%d.%d\n", ipv4_data[16] & 0xff, ipv4_data[17] & 0xff, ipv4_data[18] & 0xff, ipv4_data[19] & 0xff);
		on_udp_received(ipv4_data + 20, data_length - 20);
	}
}

void on_ethernet_package_received(char target[6], char origin[6], ethernet_content_type type, char * ethernet_data, int data_length) {
	if (type == Eth_IPv4) {
		if (is_origin_mac(origin)) {
			printf("A propria maquina enviou um IPv4\n");
		}
		if (is_broadcast_mac(target)) {
			printf(
				"[Ethernet] Size: %5d - Origin MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
				data_length,
				origin[0] & 0xff,
				origin[1] & 0xff,
				origin[2] & 0xff,
				origin[3] & 0xff,
				origin[4] & 0xff,
				origin[5] & 0xff
			);
			on_ipv4_broadcast(ethernet_data + 14, data_length - 14);
			printf("\n");
		}
	}
}

void get_all_interfaces(int max_interface_length, int max_interfaces, char * buffer, int * length) {
	struct ifaddrs *addrs, *tmp;
	getifaddrs(&addrs);
	tmp = addrs;
	*length = 0;
	char * ptr = buffer;
	while (tmp) {
		if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET) {
			memcpy(ptr, tmp->ifa_name, max_interface_length);
			ptr[max_interface_length - 1] = '\0';
			*length = (*length) + 1;
			ptr = ptr + max_interface_length;
			if (*length > max_interfaces) {
				break;
			}
		}
		tmp = tmp->ifa_next;
	}
	freeifaddrs(addrs);
}

int main(int argc,char *argv[]) {
	/* Criacao do socket. Todos os pacotes devem ser construidos a partir do protocolo Ethernet. */
	/* htons: converte um short (2-byte) integer para standard network byte order. */
	if ((sockd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
		printf("Erro na criacao do socket.\n");
		exit(1);
	}
	int i;

	/* Mostra as interfaces para o usuario escolher: */
	char interfaces[16*10];
	memset(interfaces, 16*10-1, '\0');
	int interfaces_length = 0;
	char * interface_selecionada;
	get_all_interfaces(16, 10, interfaces, &interfaces_length);
	if (interfaces_length > 0) {
		printf("As seguintes interfaces estao disponiveis:\n\n");
		for (i=0;i<interfaces_length;i++) {
			if (i*10 >= 16*10) {
				break;
			}
			//interfaces[i*10+8] = '\0';
			printf("[%d] %s\n", i, &interfaces[i*16]);
		}
		printf("\nEscolha a interface: ");
		char c = getchar();
		while (c == 10 || c < '0' || c >= '9' || (c-'0') >= interfaces_length) {
			if (c > 10 && c != ' ' && c != '\n' && c != '\r') {
				printf("\nEscolha um valor valido: ");
			}
			c = getchar();
		}
		interface_selecionada = &interfaces[(c-'0')*16];
	} else {
		printf("Erro: Lista de interfaces nao foi encontrada\n");
		printf("Setado para o default\n");
		interface_selecionada = interfaces;
		memcpy(interfaces, "enp4s0", 8);
	}

	/* Setar a interface em modo promiscuo */
	printf("\nSetando a interface \"%s\" em modo promiscuo\n", interface_selecionada);
	strcpy(ifr.ifr_name, interface_selecionada);
	if (ioctl(sockd, SIOCGIFINDEX, &ifr) < 0) {
		printf("Erro ao setar a interface em modo promiscuo (no ioctl)\n");
		//exit(1);
	}
	ioctl(sockd, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags |= IFF_PROMISC;
	ioctl(sockd, SIOCSIFFLAGS, &ifr);

	/* Aloca variaveis para chamar que lida com ethernet */
	char target[6];
	char origin[6];
	int length;
	int type_or_length;

	/* Leitura dos pacotes */
	printf("Iniciando processo de leitura de pacotes\n");
	while (1) {
   		length = recvfrom(sockd,(char *) &buff1, sizeof(buff1), 0x0, NULL, NULL);
		if (length <= 0) {
			printf("Recebido mensagem sem dados\n");
		} else {
			type_or_length = (buff1[12] << 8) + buff1[13];
			ethernet_content_type type = Eth_UNKNOWN;
			if (type_or_length <= 1500) {
				type = Eth_RAW_ETHERNET;
			} else if (type_or_length == 0x0800) {
				type = Eth_IPv4;
			} else if (type_or_length == 0x0806) {
				type = Eth_ARP;
			} else if (type_or_length == 0x8137) {
				type = Eth_IPX;
				printf("Type: Internet Packet eXchange");
			} else if (type_or_length == 0x86dd) {
				type = Eth_IPv6;
			}
			memcpy((void *) target, buff1, 6);
			memcpy((void *) origin, buff1+6, 6);
			on_ethernet_package_received(target, origin, type, buff1, length);
		}
	}
}

#include <iostream>
#include <string>
#include <cstdint>
#include <unistd.h>

// The extern "C" declaration is not necessary with current master and version 0.6.2 when it's released.
#ifdef __cplusplus
extern "C" {
#include <rtrlib/rtrlib.h>
}
#endif



const int connection_timeout = 20;
enum rtr_mgr_status connection_status = RTR_MGR_CLOSED;

static void connection_status_callback(const struct rtr_mgr_group *group,
				       enum rtr_mgr_status status,
				       const struct rtr_socket *socket,
				       void *data)
{
	connection_status = status;
}


static const std::string RPKI_QUERY_PREFIX = "2001:7fb:fd02::";
static const std::uint8_t RPKI_QUERY_MASK = 48;
static const std::uint16_t  RPKI_QUERY_ASN = 12654;

int main() {
	struct tr_socket tr_tcp;
	struct tr_tcp_config tcp_config = {
		"rpki-validator.realmv6.org",
		"8282"
	};
	struct rtr_socket rtr_tcp;

	tr_tcp_init(&tcp_config, &tr_tcp);
	rtr_tcp.tr_socket = &tr_tcp;

	struct rtr_mgr_group groups[1];

	groups[0].sockets = (struct rtr_socket **) malloc(sizeof(struct rtr_socket *));
	groups[0].sockets_len = 1;
	groups[0].preference = 1;
	groups[0].sockets[0] = &rtr_tcp;

	struct rtr_mgr_config *conf;

	rtr_mgr_init(&conf, groups, 1, 30, 600, 600, NULL, NULL, &connection_status_callback, NULL);

	rtr_mgr_start(conf);

	int sleep_counter = 0;
	// wait 20 sec till at least one group is fully synchronized with the server
	// otherwise EXIT_FAILURE.
	while (!rtr_mgr_conf_in_sync(conf)) {
		sleep_counter++;
		if (connection_status == RTR_MGR_ERROR || sleep_counter > connection_timeout)
			return EXIT_FAILURE;

		sleep(1);
	}

	std::cout << "Validating " << RPKI_QUERY_PREFIX << "/" << RPKI_QUERY_MASK << " " << RPKI_QUERY_ASN << std::endl;

	struct lrtr_ip_addr ip_addr;
	lrtr_ip_str_to_addr(RPKI_QUERY_PREFIX.c_str(), &ip_addr);

	enum pfxv_state result;

	if (rtr_mgr_validate(conf, RPKI_QUERY_ASN, &ip_addr, RPKI_QUERY_MASK, &result) == PFX_ERROR) {
		std::cerr << "Error in validation" << std::endl;
	} else {
		std::string str_result;
		switch (result) {
			case BGP_PFXV_STATE_VALID:
				str_result = "Valid";
				break;
			case BGP_PFXV_STATE_INVALID:
				str_result = "Invalid";
				break;
			case BGP_PFXV_STATE_NOT_FOUND:
				str_result = "Not Found";
				break;
		}

		std::cout << "Validation result: " << str_result << std::endl;
	}


	rtr_mgr_stop(conf);

	free(groups[0].sockets);


};

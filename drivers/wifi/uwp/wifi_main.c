#define SYS_LOG_LEVEL CONFIG_SYS_LOG_WIFI_LEVEL
#define SYS_LOG_DOMAIN "dev/wifi"
#if (SYS_LOG_LEVEL > SYS_LOG_LEVEL_OFF)
#define NET_LOG_ENABLED 1
#endif

#include <logging/sys_log.h>

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_l2.h>
#include <net/net_context.h>
#include <net/net_offload.h>
#include <net/wifi_mgmt.h>
#include <net/ethernet.h>

#include "wifi_main.h"

/* We do not need <socket/include/socket.h>
 * It seems there is a bug in ASF side: if OS is already defining sockaddr
 * and all, ASF will not need to define it. Unfortunately its socket.h does
 * but also defines some NM API functions there (??), so we need to redefine
 * those here.
 */
#define __SOCKET_H__
#define HOSTNAME_MAX_SIZE	(64)
static struct wifi_priv uwp_wifi_priv;
#define DEV_DATA(dev) \
	((struct wifi_priv *)(dev)->driver_data)

typedef enum cp_runing_bit_t{
	CP_RUNNING_BIT = 0,
	CP_WIFI_RUNNING_BIT = 1,
}cp_runing_bit_t;

int wifi_get_mac(u8_t *mac,int idx)
{
	//1 get mac address from flash.
	//2 copy to mac
	// if idx == 0,get sta mac,
	// if idx == 1,get ap mac
	mac[0]=0x00;
	mac[1]=0x0c;
	mac[2]=0x43;
	mac[3]=0x34;
	mac[4]=0x76;
	mac[5]=0x28;

	return 0;
}
#if 0
NMI_API void socketInit(void);
typedef void (*tpfAppSocketCb) (SOCKET sock, u8_t u8Msg, void *pvMsg);
typedef void (*tpfAppResolveCb) (u8_t *pu8DomainName, u32_t u32ServerIP);
NMI_API void registerSocketCallback(tpfAppSocketCb socket_cb,
		tpfAppResolveCb resolve_cb);
NMI_API SOCKET socket(u16_t u16Domain, u8_t u8Type, u8_t u8Flags);
NMI_API s8_t bind(SOCKET sock, struct sockaddr *pstrAddr, u8_t u8AddrLen);
NMI_API s8_t listen(SOCKET sock, u8_t backlog);
NMI_API s8_t accept(SOCKET sock, struct sockaddr *addr, u8_t *addrlen);
NMI_API s8_t connect(SOCKET sock, struct sockaddr *pstrAddr, u8_t u8AddrLen);
NMI_API s16_t recv(SOCKET sock, void *pvRecvBuf,
		u16_t u16BufLen, u32_t u32Timeoutmsec);
NMI_API s16_t send(SOCKET sock, void *pvSendBuffer,
		u16_t u16SendLength, u16_t u16Flags);
NMI_API s16_t sendto(SOCKET sock, void *pvSendBuffer,
		u16_t u16SendLength, u16_t flags,
		struct sockaddr *pstrDestAddr, u8_t u8AddrLen);

enum socket_errors {
	SOCK_ERR_NO_ERROR = 0,
	SOCK_ERR_INVALID_ADDRESS = -1,
	SOCK_ERR_ADDR_ALREADY_IN_USE = -2,
	SOCK_ERR_MAX_TCP_SOCK = -3,
	SOCK_ERR_MAX_UDP_SOCK = -4,
	SOCK_ERR_INVALID_ARG = -6,
	SOCK_ERR_MAX_LISTEN_SOCK = -7,
	SOCK_ERR_INVALID = -9,
	SOCK_ERR_ADDR_IS_REQUIRED = -11,
	SOCK_ERR_CONN_ABORTED = -12,
	SOCK_ERR_TIMEOUT = -13,
	SOCK_ERR_BUFFER_FULL = -14,
};

enum socket_messages {
	SOCKET_MSG_BIND	= 1,
	SOCKET_MSG_LISTEN,
	SOCKET_MSG_DNS_RESOLVE,
	SOCKET_MSG_ACCEPT,
	SOCKET_MSG_CONNECT,
	SOCKET_MSG_RECV,
	SOCKET_MSG_SEND,
	SOCKET_MSG_SENDTO,
	SOCKET_MSG_RECVFROM,
};

typedef struct {
	s8_t	status;
} tstrSocketBindMsg;

typedef struct {
	s8_t	status;
} tstrSocketListenMsg;

typedef struct {
	SOCKET			sock;
	struct sockaddr_in	strAddr;
} tstrSocketAcceptMsg;

typedef struct {
	SOCKET	sock;
	s8_t	s8Error;
} tstrSocketConnectMsg;

typedef struct {
	u8_t			*pu8Buffer;
	s16_t			s16BufferSize;
	u16_t			u16RemainingSize;
	struct sockaddr_in	strRemoteAddr;
} tstrSocketRecvMsg;

#if defined(CONFIG_WIFI_WINC1500_REGION_NORTH_AMERICA)
#define WINC1500_REGION		NORTH_AMERICA
#elif defined(CONFIG_WIFI_WINC1500_REGION_EUROPE)
#define WINC1500_REGION		EUROPE
#elif defined(CONFIG_WIFI_WINC1500_REGION_ASIA)
#define WINC1500_REGION		ASIA
#endif

#define WINC1500_BIND_TIMEOUT 500
#define WINC1500_LISTEN_TIMEOUT 500

NET_BUF_POOL_DEFINE(uwp_tx_pool, CONFIG_WIFI_WINC1500_BUF_CTR,
		CONFIG_WIFI_WINC1500_MAX_PACKET_SIZE, 0, NULL);
NET_BUF_POOL_DEFINE(uwp_rx_pool, CONFIG_WIFI_WINC1500_BUF_CTR,
		CONFIG_WIFI_WINC1500_MAX_PACKET_SIZE, 0, NULL);

K_THREAD_STACK_MEMBER(uwp_stack, CONFIG_WIFI_WINC1500_THREAD_STACK_SIZE);
struct k_thread uwp_thread_data;

struct socket_data {
	struct net_context		*context;
	net_context_connect_cb_t	connect_cb;
	net_tcp_accept_cb_t		accept_cb;
	net_context_send_cb_t		send_cb;
	net_context_recv_cb_t		recv_cb;
	void				*connect_user_data;
	void				*send_user_data;
	void				*recv_user_data;
	void				*accept_user_data;
	struct net_pkt			*rx_pkt;
	struct net_buf			*pkt_buf;
	int				ret_code;
	struct k_sem			wait_sem;
};

#if (SYS_LOG_LEVEL > SYS_LOG_LEVEL_OFF)

static void stack_stats(void)
{
	net_analyze_stack("WINC1500 stack",
			K_THREAD_STACK_BUFFER(uwp_stack),
			K_THREAD_STACK_SIZEOF(uwp_stack));
}

static char *socket_error_string(s8_t err)
{
	switch (err) {
		case SOCK_ERR_NO_ERROR:
			return "Successful socket operation";
		case SOCK_ERR_INVALID_ADDRESS:
			return "Socket address is invalid."
				"The socket operation cannot be completed successfully"
				" without specifying a specific address. "
				"For example: bind is called without specifying"
				" a port number";
		case SOCK_ERR_ADDR_ALREADY_IN_USE:
			return "Socket operation cannot bind on the given address."
				" With socket operations, only one IP address per "
				"socket is permitted. Any attempt for a new socket "
				"to bind with an IP address already bound to another "
				"open socket, will return the following error code. "
				"States that bind operation failed.";
		case SOCK_ERR_MAX_TCP_SOCK:
			return "Exceeded the maximum number of TCP sockets."
				"A maximum number of TCP sockets opened simultaneously"
				" is defined through TCP_SOCK_MAX. It is not permitted"
				" to exceed that number at socket creation."
				" Identifies that @ref socket operation failed.";
		case SOCK_ERR_MAX_UDP_SOCK:
			return "Exceeded the maximum number of UDP sockets."
				"A maximum number of UDP sockets opened simultaneously"
				" is defined through UDP_SOCK_MAX. It is not permitted"
				" to exceed that number at socket creation."
				" Identifies that socket operation failed";
		case SOCK_ERR_INVALID_ARG:
			return "An invalid argument is passed to a function.";
		case SOCK_ERR_MAX_LISTEN_SOCK:
			return "Exceeded the maximum number of TCP passive listening "
				"sockets. Identifies Identifies that listen operation"
				" failed.";
		case SOCK_ERR_INVALID:
			return "The requested socket operation is not valid in the "
				"current socket state. For example: @ref accept is "
				"called on a TCP socket before bind or listen.";
		case SOCK_ERR_ADDR_IS_REQUIRED:
			return "Destination address is required. Failure to provide "
				"the socket address required for the socket operation "
				"to be completed. It is generated as an error to the "
				"sendto function when the address required to send the"
				" data to is not known.";
		case SOCK_ERR_CONN_ABORTED:
			return "The socket is closed by the peer. The local socket is "
				"also closed.";
		case SOCK_ERR_TIMEOUT:
			return "The socket pending operation has  timedout.";
		case SOCK_ERR_BUFFER_FULL:
			return "No buffer space available to be used for the requested"
				" socket operation.";
		default:
			return "Unknown";
	}
}

static char *wifi_cb_msg_2_str(u8_t message_type)
{
	switch (message_type) {
		case M2M_WIFI_RESP_CURRENT_RSSI:
			return "M2M_WIFI_RESP_CURRENT_RSSI";
		case M2M_WIFI_REQ_WPS:
			return "M2M_WIFI_REQ_WPS";
		case M2M_WIFI_RESP_IP_CONFIGURED:
			return "M2M_WIFI_RESP_IP_CONFIGURED";
		case M2M_WIFI_RESP_IP_CONFLICT:
			return "M2M_WIFI_RESP_IP_CONFLICT";
		case M2M_WIFI_RESP_CLIENT_INFO:
			return "M2M_WIFI_RESP_CLIENT_INFO";

		case M2M_WIFI_RESP_GET_SYS_TIME:
			return "M2M_WIFI_RESP_GET_SYS_TIME";
		case M2M_WIFI_REQ_SEND_ETHERNET_PACKET:
			return "M2M_WIFI_REQ_SEND_ETHERNET_PACKET";
		case M2M_WIFI_RESP_ETHERNET_RX_PACKET:
			return "M2M_WIFI_RESP_ETHERNET_RX_PACKET";

		case M2M_WIFI_RESP_CON_STATE_CHANGED:
			return "M2M_WIFI_RESP_CON_STATE_CHANGED";
		case M2M_WIFI_REQ_DHCP_CONF:
			return "Response indicating that IP address was obtained.";
		case M2M_WIFI_RESP_SCAN_DONE:
			return "M2M_WIFI_RESP_SCAN_DONE";
		case M2M_WIFI_RESP_SCAN_RESULT:
			return "M2M_WIFI_RESP_SCAN_RESULT";
		case M2M_WIFI_RESP_PROVISION_INFO:
			return "M2M_WIFI_RESP_PROVISION_INFO";
		default:
			return "UNKNOWN";
	}
}

static char *socket_message_to_string(u8_t message)
{
	switch (message) {
		case SOCKET_MSG_BIND:
			return "Bind socket event.";
		case SOCKET_MSG_LISTEN:
			return "Listen socket event.";
		case SOCKET_MSG_DNS_RESOLVE:
			return "DNS Resolution event.";
		case SOCKET_MSG_ACCEPT:
			return "Accept socket event.";
		case SOCKET_MSG_CONNECT:
			return "Connect socket event.";
		case SOCKET_MSG_RECV:
			return "Receive socket event.";
		case SOCKET_MSG_SEND:
			return "Send socket event.";
		case SOCKET_MSG_SENDTO:
			return "Sendto socket event.";
		case SOCKET_MSG_RECVFROM:
			return "Recvfrom socket event.";
		default:
			return "UNKNOWN.";
	}
}

#endif /* (SYS_LOG_LEVEL > SYS_LOG_LEVEL_OFF) */

/**
 * This function is called when the socket is to be opened.
 */
static int uwp_get(sa_family_t family,
		enum net_sock_type type,
		enum net_ip_protocol ip_proto,
		struct net_context **context)
{
	struct socket_data *sd;

	if (family != AF_INET) {
		SYS_LOG_ERR("Only AF_INET is supported!");
		return -1;
	}

	(*context)->user_data = (void *)(sint32)socket(family, type, 0);
	if ((*context)->user_data < 0) {
		SYS_LOG_ERR("socket error!");
		return -1;
	}

	//sd = &wifi_priv.socket_data[(int)(*context)->user_data];

	k_sem_init(&sd->wait_sem, 0, 1);

	sd->context = *context;

	return 0;
}

/**
 * This function is called when user wants to bind to local IP address.
 */
static int uwp_bind(struct net_context *context,
		const struct sockaddr *addr,
		socklen_t addrlen)
{
	SOCKET socket = (int)context->user_data;
	int ret;

	/* FIXME atmel uwp don't support bind on null port */
	if (net_sin(addr)->sin_port == 0) {
		return 0;
	}

	ret = bind((int)context->user_data, (struct sockaddr *)addr, addrlen);
	if (ret) {
		SYS_LOG_ERR("bind error %d %s!",
				ret, socket_message_to_string(ret));
		return ret;
	}

#if 0
	if (k_sem_take(&wifi_priv.socket_data[socket].wait_sem,
				WINC1500_BIND_TIMEOUT)) {
		SYS_LOG_ERR("bind error timeout expired");
		return -ETIMEDOUT;
	}

	return wifi_priv.socket_data[socket].ret_code;
#endif
	return 0;
}

/**
 * This function is called when user wants to mark the socket
 * to be a listening one.
 */
static int uwp_listen(struct net_context *context, int backlog)
{
	SOCKET socket = (int)context->user_data;
	int ret;

	ret = listen((int)context->user_data, backlog);
	if (ret) {
		SYS_LOG_ERR("listen error %d %s!",
				ret, socket_error_string(ret));
		return ret;
	}

#if 0
	if (k_sem_take(&wifi_priv.socket_data[socket].wait_sem,
				WINC1500_LISTEN_TIMEOUT)) {
		return -ETIMEDOUT;
	}

	return wifi_priv.socket_data[socket].ret_code;
#endif

	return 0;
}

/**
 * This function is called when user wants to create a connection
 * to a peer host.
 */
static int uwp_connect(struct net_context *context,
		const struct sockaddr *addr,
		socklen_t addrlen,
		net_context_connect_cb_t cb,
		s32_t timeout,
		void *user_data)
{
	SOCKET socket = (int)context->user_data;
	int ret;

#if 0
	wifi_priv.socket_data[socket].connect_cb = cb;
	wifi_priv.socket_data[socket].connect_user_data = user_data;
	wifi_priv.socket_data[socket].ret_code = 0;
#endif

	ret = connect(socket, (struct sockaddr *)addr, addrlen);
	if (ret) {
		SYS_LOG_ERR("connect error %d %s!",
				ret, socket_error_string(ret));
		return ret;
	}

#if 0
	if (timeout != 0 &&
			k_sem_take(&wifi_priv.socket_data[socket].wait_sem, timeout)) {
		return -ETIMEDOUT;
	}

	return wifi_priv.socket_data[socket].ret_code;
#endif
	return 0;
}

/**
 * This function is called when user wants to accept a connection
 * being established.
 */
static int uwp_accept(struct net_context *context,
		net_tcp_accept_cb_t cb,
		s32_t timeout,
		void *user_data)
{
	SOCKET socket = (int)context->user_data;
	int ret;

	//wifi_priv.socket_data[socket].accept_cb = cb;

	ret = accept(socket, NULL, 0);
	if (ret) {
		SYS_LOG_ERR("accept error %d %s!",
				ret, socket_error_string(ret));
		return ret;
	}

#if 0
	if (timeout) {
		if (k_sem_take(&wifi_priv.socket_data[socket].wait_sem,
					timeout)) {
			return -ETIMEDOUT;
		}
	} else {
		k_sem_take(&wifi_priv.socket_data[socket].wait_sem,
				K_FOREVER);
	}

	return wifi_priv.socket_data[socket].ret_code;
#endif
	return 0;
}

/**
 * This function is called when user wants to send data to peer host.
 */
static int uwp_send(struct net_pkt *pkt,
		net_context_send_cb_t cb,
		s32_t timeout,
		void *token,
		void *user_data)
{
	struct net_context *context = pkt->context;
	SOCKET socket = (int)context->user_data;
	bool first_frag;
	struct net_buf *frag;
	int ret;

	//wifi_priv.socket_data[socket].send_cb = cb;
	//wifi_priv.socket_data[socket].send_user_data = user_data;

	first_frag = true;

	for (frag = pkt->frags; frag; frag = frag->frags) {
		u8_t *data_ptr;
		u16_t data_len;

		if (first_frag) {
			data_ptr = net_pkt_ll(pkt);
			data_len = net_pkt_ll_reserve(pkt) + frag->len;
			first_frag = false;
		} else {
			data_ptr = frag->data;
			data_len = frag->len;

		}

		ret = send(socket, data_ptr, data_len, 0);
		if (ret) {
			SYS_LOG_ERR("send error %d %s!",
					ret, socket_error_string(ret));
			return ret;
		}
	}

	net_pkt_unref(pkt);

	return 0;
}

/**
 * This function is called when user wants to send data to peer host.
 */
static int uwp_sendto(struct net_pkt *pkt,
		const struct sockaddr *dst_addr,
		socklen_t addrlen,
		net_context_send_cb_t cb,
		s32_t timeout,
		void *token,
		void *user_data)
{
	struct net_context *context = pkt->context;
	SOCKET socket = (int)context->user_data;
	bool first_frag;
	struct net_buf *frag;
	int ret;

	//wifi_priv.socket_data[socket].send_cb = cb;
	//wifi_priv.socket_data[socket].send_user_data = user_data;

	first_frag = true;

	for (frag = pkt->frags; frag; frag = frag->frags) {
		u8_t *data_ptr;
		u16_t data_len;

		if (first_frag) {
			data_ptr = net_pkt_ll(pkt);
			data_len = net_pkt_ll_reserve(pkt) + frag->len;
			first_frag = false;
		} else {
			data_ptr = frag->data;
			data_len = frag->len;
		}

		ret = sendto(socket, data_ptr, data_len, 0,
				(struct sockaddr *)dst_addr, addrlen);
		if (ret) {
			SYS_LOG_ERR("send error %d %s!",
					ret, socket_error_string(ret));
			return ret;
		}
	}

	net_pkt_unref(pkt);

	return 0;
}

/**
 */
static int prepare_pkt(struct socket_data *sock_data)
{
	/* Get the frame from the buffer */
	sock_data->rx_pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
	if (!sock_data->rx_pkt) {
		SYS_LOG_ERR("Could not allocate rx packet");
		return -1;
	}

	/* Reserve a data frag to receive the frame */
	sock_data->pkt_buf = net_pkt_get_frag(sock_data->rx_pkt, K_NO_WAIT);
	if (!sock_data->pkt_buf) {
		SYS_LOG_ERR("Could not allocate data frag");
		net_pkt_unref(sock_data->rx_pkt);
		return -1;
	}

	net_pkt_frag_insert(sock_data->rx_pkt, sock_data->pkt_buf);

	return 0;
}

/**
 * This function is called when user wants to receive data from peer
 * host.
 */
static int uwp_recv(struct net_context *context,
		net_context_recv_cb_t cb,
		s32_t timeout,
		void *user_data)
{
	SOCKET socket = (int) context->user_data;
	int ret;

#if 0
	ret = prepare_pkt(&wifi_priv.socket_data[socket]);
	if (ret) {
		SYS_LOG_ERR("Could not reserve packet buffer");
		return -ENOMEM;
	}

	wifi_priv.socket_data[socket].recv_cb = cb;
	wifi_priv.socket_data[socket].recv_user_data = user_data;

	ret = recv(socket, wifi_priv.socket_data[socket].pkt_buf->data,
			CONFIG_WIFI_WINC1500_MAX_PACKET_SIZE, timeout);
	if (ret) {
		SYS_LOG_ERR("recv error %d %s!",
				ret, socket_error_string(ret));
		return ret;
	}
#endif

	return 0;
}

/**
 * This function is called when user wants to close the socket.
 */
static int uwp_put(struct net_context *context)
{
	return 0;
}

static void handle_wifi_con_state_changed(void *pvMsg)
{
	tstrM2mWifiStateChanged *pstrWifiState =
		(tstrM2mWifiStateChanged *)pvMsg;

	switch (pstrWifiState->u8CurrState) {
		case M2M_WIFI_DISCONNECTED:
			SYS_LOG_DBG("Disconnected (%u)", pstrWifiState->u8ErrCode);

#if 0
			if (wifi_priv.connecting) {
				wifi_mgmt_raise_connect_result_event(wifi_priv.iface,
						pstrWifiState->u8ErrCode ? -EIO : 0);
				wifi_priv.connecting = false;

				break;
			}

			wifi_priv.connected = false;
			wifi_mgmt_raise_disconnect_result_event(wifi_priv.iface, 0);
#endif

			break;
		case M2M_WIFI_CONNECTED:
			SYS_LOG_DBG("Connected (%u)", pstrWifiState->u8ErrCode);

#if 0
			wifi_priv.connected = true;
			wifi_mgmt_raise_connect_result_event(wifi_priv.iface, 0);
#endif

			break;
		case M2M_WIFI_UNDEF:
			/* TODO status undefined*/
			SYS_LOG_DBG("Undefined?");
			break;
	}
}

static void handle_wifi_dhcp_conf(void *pvMsg)
{
	u8_t *pu8IPAddress = (u8_t *)pvMsg;
	struct in_addr addr;
	u8_t i;

	/* Connected and got IP address*/
	SYS_LOG_DBG("Wi-Fi connected, IP is %u.%u.%u.%u",
			pu8IPAddress[0], pu8IPAddress[1],
			pu8IPAddress[2], pu8IPAddress[3]);

	/* TODO at this point the standby mode should be enable
	 * status = WiFi connected IP assigned
	 */
	for (i = 0; i < 4; i++) {
		addr.s4_addr[i] = pu8IPAddress[i];
	}

	/* TODO fill in net mask, gateway and lease time */

	//	net_if_ipv4_addr_add(wifi_priv.iface, &addr, NET_ADDR_DHCP, 0);
}

static void reset_scan_data(void)
{
	//wifi_priv.scan_cb = NULL;
	//wifi_priv.scan_result = 0;
}

static void handle_scan_result(void *pvMsg)
{
	tstrM2mWifiscanResult *pstrScanResult = (tstrM2mWifiscanResult *)pvMsg;
	struct wifi_scan_result result;

#if 0
	if (!wifi_priv.scan_cb) {
		return;
	}

	if (pstrScanResult->u8AuthType == M2M_WIFI_SEC_OPEN) {
		result.security = WIFI_SECURITY_TYPE_NONE;
	} else if (pstrScanResult->u8AuthType == M2M_WIFI_SEC_WPA_PSK) {
		result.security = WIFI_SECURITY_TYPE_PSK;
	} else {
		SYS_LOG_DBG("Security %u not supported",
				pstrScanResult->u8AuthType);
		goto out;
	}

	memcpy(result.ssid, pstrScanResult->au8SSID, WIFI_SSID_MAX_LEN);
	result.ssid_length = strlen(result.ssid);

	result.channel = pstrScanResult->u8ch;
	result.rssi = pstrScanResult->s8rssi;

	wifi_priv.scan_cb(wifi_priv.iface, 0, &result);

	k_yield();

out:
	if (wifi_priv.scan_result < m2m_wifi_get_num_ap_found()) {
		m2m_wifi_req_scan_result(wifi_priv.scan_result);
		wifi_priv.scan_result++;
	} else {
		wifi_priv.scan_cb(wifi_priv.iface, 0, NULL);
		reset_scan_data();
	}
#endif
}

static void handle_scan_done(void *pvMsg)
{
	tstrM2mScanDone	*pstrInfo = (tstrM2mScanDone *)pvMsg;

#if 0
	if (!wifi_priv.scan_cb) {
		return;
	}

	if (pstrInfo->s8ScanState != M2M_SUCCESS) {
		wifi_priv.scan_cb(wifi_priv.iface, -EIO, NULL);
		reset_scan_data();

		SYS_LOG_ERR("Scan failed.");

		return;
	}

	wifi_priv.scan_result = 0;

	if (pstrInfo->u8NumofCh >= 1) {
		SYS_LOG_DBG("Requesting results (%u)",
				m2m_wifi_get_num_ap_found());

		m2m_wifi_req_scan_result(wifi_priv.scan_result);
		wifi_priv.scan_result++;
	} else {
		SYS_LOG_DBG("No AP found");

		wifi_priv.scan_cb(wifi_priv.iface, 0, NULL);
		reset_scan_data();
	}
#endif
}

static void uwp_wifi_cb(u8_t message_type, void *pvMsg)
{
	SYS_LOG_DBG("Msg Type %d %s",
			message_type, wifi_cb_msg_2_str(message_type));

	switch (message_type) {
		case M2M_WIFI_RESP_CON_STATE_CHANGED:
			handle_wifi_con_state_changed(pvMsg);
			break;
		case M2M_WIFI_REQ_DHCP_CONF:
			handle_wifi_dhcp_conf(pvMsg);
			break;
		case M2M_WIFI_RESP_SCAN_RESULT:
			handle_scan_result(pvMsg);
			break;
		case M2M_WIFI_RESP_SCAN_DONE:
			handle_scan_done(pvMsg);
			break;
		default:
			break;
	}

	stack_stats();
}

static void handle_socket_msg_connect(struct socket_data *sd, void *pvMsg)
{
	tstrSocketConnectMsg *strConnMsg = (tstrSocketConnectMsg *)pvMsg;

	SYS_LOG_ERR("CONNECT: socket %d error %d",
			strConnMsg->sock, strConnMsg->s8Error);

	if (sd->connect_cb) {
		sd->connect_cb(sd->context,
				strConnMsg->s8Error,
				sd->connect_user_data);
	}

	sd->ret_code = strConnMsg->s8Error;
}


static bool handle_socket_msg_recv(SOCKET sock,
		struct socket_data *sd, void *pvMsg)
{
	tstrSocketRecvMsg *pstrRx = (tstrSocketRecvMsg *)pvMsg;

	if ((pstrRx->pu8Buffer != NULL) && (pstrRx->s16BufferSize > 0)) {

		net_buf_add(sd->pkt_buf, pstrRx->s16BufferSize);

		net_pkt_set_appdata(sd->rx_pkt, sd->pkt_buf->data);
		net_pkt_set_appdatalen(sd->rx_pkt, pstrRx->s16BufferSize);

		if (sd->recv_cb) {
			sd->recv_cb(sd->context,
					sd->rx_pkt,
					0,
					sd->recv_user_data);
		}
	} else if (pstrRx->pu8Buffer == NULL) {
		if (pstrRx->s16BufferSize == SOCK_ERR_CONN_ABORTED) {
			net_pkt_unref(sd->rx_pkt);
			return false;
		}
	}

	if (prepare_pkt(sd)) {
		SYS_LOG_ERR("Could not reserve packet buffer");
		return false;
	}

	recv(sock, sd->pkt_buf->data,
			CONFIG_WIFI_WINC1500_MAX_PACKET_SIZE, K_NO_WAIT);

	return true;
}

static void handle_socket_msg_bind(struct socket_data *sd, void *pvMsg)
{
	tstrSocketBindMsg *bind_msg = (tstrSocketBindMsg *)pvMsg;

	/* Holding a value of ZERO for a successful bind or otherwise
	 * a negative error code corresponding to the type of error.
	 */

	if (bind_msg->status) {
		SYS_LOG_ERR("BIND: error %d %s",
				bind_msg->status,
				socket_message_to_string(bind_msg->status));
		sd->ret_code = bind_msg->status;
	}
}

static void handle_socket_msg_listen(struct socket_data *sd, void *pvMsg)
{
	tstrSocketListenMsg *listen_msg = (tstrSocketListenMsg *)pvMsg;

	/* Holding a value of ZERO for a successful listen or otherwise
	 * a negative error code corresponding to the type of error.
	 */

	if (listen_msg->status) {
		SYS_LOG_ERR("uwp_socket_cb:LISTEN: error %d %s",
				listen_msg->status,
				socket_message_to_string(listen_msg->status));
		sd->ret_code = listen_msg->status;
	}
}

static void handle_socket_msg_accept(struct socket_data *sd, void *pvMsg)
{
	tstrSocketAcceptMsg *accept_msg = (tstrSocketAcceptMsg *)pvMsg;

	/* On a successful accept operation, the return information is
	 * the socket ID for the accepted connection with the remote peer.
	 * Otherwise a negative error code is returned to indicate failure
	 * of the accept operation.
	 */

	SYS_LOG_DBG("ACCEPT: from %d.%d.%d.%d:%d, new socket is %d",
			accept_msg->strAddr.sin_addr.s4_addr[0],
			accept_msg->strAddr.sin_addr.s4_addr[1],
			accept_msg->strAddr.sin_addr.s4_addr[2],
			accept_msg->strAddr.sin_addr.s4_addr[3],
			ntohs(accept_msg->strAddr.sin_port),
			accept_msg->sock);

	if (accept_msg->sock < 0) {
		SYS_LOG_ERR("ACCEPT: error %d %s",
				accept_msg->sock,
				socket_message_to_string(accept_msg->sock));
		sd->ret_code = accept_msg->sock;
	}

	if (sd->accept_cb) {
		struct socket_data *a_sd;
		int ret;

		a_sd = &wifi_priv.socket_data[accept_msg->sock];

		memcpy(a_sd, sd, sizeof(struct socket_data));

		ret = net_context_get(AF_INET, SOCK_STREAM,
				IPPROTO_TCP, &a_sd->context);
		if (ret < 0) {
			SYS_LOG_ERR("Cannot get new net context for ACCEPT");
		} else {
			a_sd->context->user_data =
				(void *)((int)accept_msg->sock);

			sd->accept_cb(a_sd->context,
					(struct sockaddr *)&accept_msg->strAddr,
					sizeof(struct sockaddr_in),
					(accept_msg->sock > 0) ?
					0 : accept_msg->sock,
					sd->accept_user_data);
		}
	}
}

static void uwp_socket_cb(SOCKET sock, u8_t message, void *pvMsg)
{
	struct socket_data *sd = &wifi_priv.socket_data[sock];

	if (message != 6) {
		SYS_LOG_DBG(": sock %d Msg %d %s",
				sock, message, socket_message_to_string(message));
	}

	sd->ret_code = 0;

	switch (message) {
		case SOCKET_MSG_CONNECT:
			handle_socket_msg_connect(sd, pvMsg);
			k_sem_give(&sd->wait_sem);

			break;
		case SOCKET_MSG_SEND:
			break;
		case SOCKET_MSG_RECV:
			if (!handle_socket_msg_recv(sock, sd, pvMsg)) {
				return;
			}

			break;
		case SOCKET_MSG_BIND:
			handle_socket_msg_bind(sd, pvMsg);
			k_sem_give(&sd->wait_sem);

			break;
		case SOCKET_MSG_LISTEN:
			handle_socket_msg_listen(sd, pvMsg);
			k_sem_give(&sd->wait_sem);

			break;
		case SOCKET_MSG_ACCEPT:
			handle_socket_msg_accept(sd, pvMsg);
			k_sem_give(&sd->wait_sem);

			break;
	}

	stack_stats();
}

static void uwp_thread(void)
{
	printk("%s %d.\n", __func__, __LINE__);
	while (1) {
		while (m2m_wifi_handle_events(NULL) != 0) {
		}

		k_sleep(K_MSEC(1));
	}
}
#endif

static int uwp_mgmt_scan(struct device *dev, scan_result_cb_t cb)
{
	struct wifi_priv *priv=DEV_DATA(dev);

	SYS_LOG_INF("%s %d.\n", __func__, __LINE__);
	if (priv->scan_cb) {
		return -EALREADY;
	}

	if (!priv->opened) {
		wifi_cmd_start_sta(priv);
		priv->opened = true;
	}

	priv->scan_cb = cb;

	if (wifi_cmd_scan(priv)) {
		priv->scan_cb = NULL;
		return -EIO;
	}

	return 0;
}

static int uwp_mgmt_connect(struct device *dev,
		struct wifi_connect_req_params *params)
{
	struct wifi_priv *priv=DEV_DATA(dev);
	SYS_LOG_INF("%s %d.\n", __func__, __LINE__);

	return wifi_cmd_connect(priv, params);
}

static int uwp_mgmt_disconnect(struct device *device)
{
	SYS_LOG_ERR("%s %d.\n", __func__, __LINE__);

	return 0;
}

static void uwp_iface_init(struct net_if *iface)
{
	struct wifi_priv *priv=DEV_DATA(iface->if_dev->dev);

	wifi_cmd_get_cp_info(priv);

	SYS_LOG_WRN("eth_init:net_if_set_link_addr:"
			"MAC Address %02X:%02X:%02X:%02X:%02X:%02X",
			priv->mac[0], priv->mac[1], priv->mac[2],
			priv->mac[3], priv->mac[4], priv->mac[5]);

	net_if_set_link_addr(iface, priv->mac, sizeof(priv->mac),
			NET_LINK_ETHERNET);

	//iface->if_dev->offload = &uwp_offload;

	priv->iface = iface;
}

int wifi_tx_fill_msdu_dscr(struct wifi_priv *priv,
		struct net_pkt *pkt, u8_t type, u8_t offset)
{
	u32_t addr = 0;
	struct tx_msdu_dscr *dscr = NULL;

	net_pkt_set_ll_reserve(pkt,
			sizeof(struct tx_msdu_dscr) + net_pkt_ll_reserve(pkt));

	dscr = (struct tx_msdu_dscr *)net_pkt_ll(pkt);
	memset(dscr, 0x00, sizeof(struct tx_msdu_dscr));
	addr = (u32_t)dscr;
	SPRD_AP_TO_CP_ADDR(addr);
	dscr->next_buf_addr_low = addr;
	dscr->next_buf_addr_high = 0x0;

	dscr->tx_ctrl.checksum_offload = 0;
	dscr->common.type =
		(type == SPRDWL_TYPE_CMD ? SPRDWL_TYPE_CMD : SPRDWL_TYPE_DATA);
	dscr->common.direction_ind = TRANS_FOR_TX_PATH;
	dscr->common.next_buf = 0;

	dscr->common.interface = 0;

	dscr->pkt_len = net_pkt_get_len(pkt);
	dscr->offset = 11;
	/*TODO*/
	dscr->tx_ctrl.sw_rate = (type == SPRDWL_TYPE_DATA_SPECIAL ? 1 : 0);
	dscr->tx_ctrl.wds = 0;
	/*TBD*/ dscr->tx_ctrl.swq_flag = 0;
	/*TBD*/ dscr->tx_ctrl.rsvd = 0;
	/*TBD*/ dscr->tx_ctrl.pcie_mh_readcomp = 1;
	dscr->buffer_info.msdu_tid = 0;
	dscr->buffer_info.mac_data_offset = 0;
	dscr->buffer_info.sta_lut_idx = 0;
	dscr->buffer_info.encrypt_bypass = 0;
	dscr->buffer_info.ap_buf_flag = 1;
	dscr->tx_ctrl.checksum_offload = 0;
	dscr->tx_ctrl.checksum_type = 0;
	dscr->tcp_udp_header_offset = 0;

	return 0;
}

extern void read8_cmd_exe(u32_t addr, u32_t len);
static int uwp_iface_tx(struct net_if *iface, struct net_pkt *pkt)
{
	struct wifi_priv *priv = DEV_DATA(iface->if_dev->dev);
	struct net_buf *frag;
	bool first_frag = true;
	u8_t *data_ptr;
	u16_t data_len;
	u16_t total_len;

	wifi_tx_fill_msdu_dscr(priv, pkt, SPRDWL_TYPE_DATA, 0);

	total_len = net_pkt_ll_reserve(pkt) + net_pkt_get_len(pkt);

	SYS_LOG_WRN("wifi tx data: %d bytes, reserve: %d bytes",
			total_len, net_pkt_ll_reserve(pkt));


	for (frag = pkt->frags; frag; frag = frag->frags) {
		if (first_frag) {
			data_ptr = net_pkt_ll(pkt);
			data_len = net_pkt_ll_reserve(pkt) + frag->len;
			first_frag = false;
		} else {
			data_ptr = frag->data;
			data_len = frag->len;

		}
		read8_cmd_exe((u32_t)data_ptr, data_len);
		SPRD_AP_TO_CP_ADDR(data_ptr);
		wifi_tx_data(data_ptr, data_len);
	}

	net_pkt_unref(pkt);
	return 0;
}

static enum ethernet_hw_caps uwp_iface_get_capabilities(struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T;
}

static const struct net_wifi_mgmt_api uwp_api = {
	.iface_api.init = uwp_iface_init,
	.iface_api.send = uwp_iface_tx,
	.get_capabilities = uwp_iface_get_capabilities,
	.scan		= uwp_mgmt_scan,
	.connect	= uwp_mgmt_connect,
	.disconnect	= uwp_mgmt_disconnect,
};

#define SEC1    1
#define SEC2    2
extern int cp_mcu_init(void);
static int uwp_init(struct device *dev)
{
	int ret;
	struct wifi_priv *priv=DEV_DATA(dev);
//	struct wifi_conf_sec1_t *sec1;
//	struct wifi_conf_sec2_t *sec2;

	priv->connecting = false;
	priv->connected = false;
	priv->opened = false;

	wifi_get_mac(priv->mac, 0);

	ret = cp_mcu_init();
	if (ret) {
		SYS_LOG_ERR("firmware download failed %i.", ret);
		return ret;
	}

	wifi_cmdevt_init();
	wifi_txrx_init(priv);
	wifi_irq_init();

	SYS_LOG_DBG("UWP WIFI driver Initialized");

	return 0;
}

NET_DEVICE_INIT(uwp, CONFIG_WIFI_UWP_NAME,
		uwp_init, &uwp_wifi_priv, NULL,
		CONFIG_WIFI_INIT_PRIORITY,
		&uwp_api,
		ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2),
		1550);
#if 0
NET_DEVICE_OFFLOAD_INIT(uwp, CONFIG_WIFI_UWP_NAME,
		uwp_init, &uwp_wifi_priv, NULL,
		CONFIG_WIFI_INIT_PRIORITY,
		&uwp_api,
		1550);
#endif

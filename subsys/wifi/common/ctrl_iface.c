/*
 * @file
 * @brief Control interface of WiFi manager
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_WIFIMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(wifimgr);

#if defined(CONFIG_WIFIMGR_STA) || defined(CONFIG_WIFIMGR_AP)

#include "ctrl_iface.h"
#include "sm.h"

static struct wifimgr_ctrl_iface *sta_ctrl;
static struct wifimgr_ctrl_iface *ap_ctrl;

int wifimgr_register_connection_notifier(wifi_notifier_fn_t notifier_call)
{
	wifimgr_register_notifier(&sta_ctrl->conn_chain, notifier_call);
}

int wifimgr_unregister_connection_notifier(wifi_notifier_fn_t notifier_call)
{
	wifimgr_unregister_notifier(&sta_ctrl->conn_chain, notifier_call);
}

int wifimgr_register_disconnection_notifier(wifi_notifier_fn_t notifier_call)
{
	wifimgr_register_notifier(&sta_ctrl->disc_chain, notifier_call);
}

int wifimgr_unregister_disconnection_notifier(wifi_notifier_fn_t notifier_call)
{
	wifimgr_unregister_notifier(&sta_ctrl->disc_chain, notifier_call);
}

int wifimgr_register_new_station_notifier(wifi_notifier_fn_t notifier_call)
{
	wifimgr_register_notifier(&ap_ctrl->new_sta_chain, notifier_call);
}

int wifimgr_unregister_new_station_notifier(wifi_notifier_fn_t notifier_call)
{
	wifimgr_unregister_notifier(&ap_ctrl->new_sta_chain, notifier_call);
}

int wifimgr_register_station_leave_notifier(wifi_notifier_fn_t notifier_call)
{
	wifimgr_register_notifier(&ap_ctrl->sta_leave_chain, notifier_call);
}

int wifimgr_unregister_station_leave_notifier(wifi_notifier_fn_t notifier_call)
{
	wifimgr_unregister_notifier(&ap_ctrl->sta_leave_chain, notifier_call);
}

void wifimgr_ctrl_evt_scan_result(void *handle, struct wifi_scan_result *scan_res)
{
	struct wifimgr_ctrl_iface *ctrl = (struct wifimgr_ctrl_iface *)handle;

	/* Notify the external caller */
	if (ctrl->scan_res_cb)
		ctrl->scan_res_cb(scan_res);
}

void wifimgr_ctrl_evt_scan_done(void *handle, char status)
{
	struct wifimgr_ctrl_iface *ctrl = (struct wifimgr_ctrl_iface *)handle;

	ctrl->scan_res_cb = NULL;
	ctrl->evt_status = status;
	if (!status)
		wifimgr_info("scan done!\n");
	else
		wifimgr_info("scan abort!\n");
	fflush(stdout);

	wifimgr_ctrl_iface_wakeup(ctrl);
}

#ifdef CONFIG_WIFIMGR_STA
void wifimgr_ctrl_evt_rtt_response(void *handle, struct wifi_rtt_response *rtt_resp)
{
	struct wifimgr_ctrl_iface *ctrl = (struct wifimgr_ctrl_iface *)handle;

	/* Notify the external caller */
	if (ctrl->rtt_resp_cb)
		ctrl->rtt_resp_cb(rtt_resp);
}

void wifimgr_ctrl_evt_rtt_done(void *handle, char status)
{
	struct wifimgr_ctrl_iface *ctrl = (struct wifimgr_ctrl_iface *)handle;

	ctrl->rtt_resp_cb = NULL;
	ctrl->evt_status = status;
	if (!status)
		wifimgr_info("RTT done!\n");
	else
		wifimgr_info("RTT abort!\n");
	fflush(stdout);

	wifimgr_ctrl_iface_wakeup(ctrl);
}

void wifimgr_ctrl_evt_connect(void *handle, struct wifimgr_notifier_chain *conn_chain, char status)
{
	struct wifimgr_ctrl_iface *ctrl = (struct wifimgr_ctrl_iface *)handle;
	struct wifimgr_notifier *notifier;

	ctrl->evt_status = status;

	/* Notify the passive callback on the connection chain */
	wifimgr_list_for_each_entry(notifier, &conn_chain->list, struct wifimgr_notifier, node) {
		if (notifier->notifier_call) {
			union wifi_notifier_val val;

			val.val_char = status;
			notifier->notifier_call(val);
		}
	}

	wifimgr_ctrl_iface_wakeup(ctrl);
}

void wifimgr_ctrl_evt_disconnect(void *handle, struct wifimgr_notifier_chain *disc_chain, char reason_code)
{
	struct wifimgr_ctrl_iface *ctrl = (struct wifimgr_ctrl_iface *)handle;
	struct wifimgr_notifier *notifier;

	ctrl->evt_status = 0;

	/* Notify the passive callback on the disconnection chain */
	wifimgr_list_for_each_entry(notifier, &disc_chain->list, struct wifimgr_notifier, node) {
		if (notifier->notifier_call) {
			union wifi_notifier_val val;

			val.val_char = reason_code;
			notifier->notifier_call(val);
		}
	}

	wifimgr_ctrl_iface_wakeup(ctrl);
}

void wifimgr_ctrl_evt_timeout(void *handle)
{
	struct wifimgr_ctrl_iface *ctrl = (struct wifimgr_ctrl_iface *)handle;

	ctrl->evt_status = -ETIMEDOUT;
	wifimgr_ctrl_iface_wakeup(ctrl);
}
#endif

#ifdef CONFIG_WIFIMGR_AP
void wifimgr_ctrl_evt_new_station(void *handle, struct wifimgr_notifier_chain *chain, char status, char *mac)
{
	struct wifimgr_ctrl_iface *ctrl = (struct wifimgr_ctrl_iface *)handle;
	struct wifimgr_notifier *notifier;

	if (!status) {
		/* Notify the passive callback on the new station chain */
		wifimgr_list_for_each_entry(notifier, &chain->list, struct wifimgr_notifier, node) {
			if (notifier->notifier_call) {
				union wifi_notifier_val val;

				val.val_ptr = mac;
				notifier->notifier_call(val);
			}
		}
	} else {
		/* Notify the passive callback on the station leave chain */
		wifimgr_list_for_each_entry(notifier, &chain->list, struct wifimgr_notifier, node) {
			if (notifier->notifier_call) {
				union wifi_notifier_val val;

				val.val_ptr = mac;
				notifier->notifier_call(val);
			}
		}
	}

	wifimgr_ctrl_iface_wakeup(ctrl);
}
#endif

int wifimgr_ctrl_iface_set_conf(char *iface_name, struct wifi_config *conf)
{
	struct wifimgr_ctrl_iface *ctrl;
	unsigned int cmd_id;

	if (!iface_name || !conf)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
		ctrl = sta_ctrl;
		cmd_id = WIFIMGR_CMD_SET_STA_CONFIG;
	} else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
		ctrl = ap_ctrl;
		cmd_id = WIFIMGR_CMD_SET_AP_CONFIG;
	} else {
		return -EINVAL;
	}

	/* Check SSID (mandatory) */
	if (strlen(conf->ssid)) {
		if (strlen(conf->ssid) > WIFIMGR_MAX_SSID_LEN) {
			wifimgr_err("Invalid SSID: %s!\n", conf->ssid);
			return -EINVAL;
		}
		wifimgr_info("SSID:\t\t%s\n", conf->ssid);
	}

	/* Check BSSID (optional) */
	if (!is_zero_ether_addr(conf->bssid)) {
		wifimgr_info("BSSID:\t\t" MACSTR "\n", MAC2STR(conf->bssid));
	}

	/* Check Security */
	switch (conf->security) {
	case WIFIMGR_SECURITY_OPEN:
	case WIFIMGR_SECURITY_PSK:
		wifimgr_info("Security:\t%s\n", security2str(conf->security));
	case WIFIMGR_SECURITY_UNKNOWN:
		break;
	default:
		wifimgr_err("invalid security: %d!\n", conf->security);
		return -EINVAL;
	}

	/* Check Passphrase (optional: valid only for WPA/WPA2-PSK) */
	if (strlen(conf->passphrase)) {
		if (strlen(conf->passphrase) > sizeof(conf->passphrase)) {
			wifimgr_err("invalid PSK: %s!\n", conf->passphrase);
			return -EINVAL;
		}
		wifimgr_info("Passphrase:\t%s\n", conf->passphrase);
	}

	/* Check band */
	switch (conf->band) {
	case 2:
	case 5:
		wifimgr_info("Band:\t%u\n", conf->band);
	case 0:
		break;
	default:
		wifimgr_err("invalid band: %u!\n", conf->band);
		return -EINVAL;
	}

	/* Check channel */
	if ((conf->channel > 14 && conf->channel < 34) || (conf->channel > 196)) {
		wifimgr_err("invalid channel: %u!\n", conf->channel);
		return -EINVAL;
	}
	if (conf->channel)
		wifimgr_info("Channel:\t%u\n", conf->channel);

	/* Check channel width */
	switch (conf->ch_width) {
	case 20:
	case 40:
	case 80:
	case 160:
		wifimgr_info("Channel Width:\t%u\n", conf->ch_width);
	case 0:
		break;
	default:
		wifimgr_err("invalid channel width: %u!\n", conf->ch_width);
		return -EINVAL;
	}

	/* Check autorun */
	if (conf->autorun)
		wifimgr_info("----------------\n");
	if (conf->autorun > 0)
		wifimgr_info("Autorun:\t%ds\n", conf->autorun);
	else if (conf->autorun < 0)
		wifimgr_info("Autorun:\toff\n");

	return wifimgr_ctrl_iface_send_cmd(ctrl, cmd_id, conf, sizeof(struct wifi_config));
}

int wifimgr_ctrl_iface_get_conf(char *iface_name, struct wifi_config *conf)
{
	struct wifimgr_ctrl_iface *ctrl;
	unsigned int cmd_id = 0;

	if (!iface_name || !conf)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
		ctrl = sta_ctrl;
		cmd_id = WIFIMGR_CMD_GET_STA_CONFIG;
	} else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
		ctrl = ap_ctrl;
		cmd_id = WIFIMGR_CMD_GET_AP_CONFIG;
	} else {
		return -EINVAL;
	}

	return wifimgr_ctrl_iface_send_cmd(ctrl, cmd_id, conf, sizeof(struct wifi_config));
}

int wifimgr_ctrl_iface_get_capa(char *iface_name, union wifi_capa *capa)
{
	struct wifimgr_ctrl_iface *ctrl;
	unsigned int cmd_id = 0;

	if (!iface_name || !capa)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
		ctrl = sta_ctrl;
		cmd_id = WIFIMGR_CMD_GET_STA_CAPA;
	} else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
		ctrl = ap_ctrl;
		cmd_id = WIFIMGR_CMD_GET_AP_CAPA;
	} else {
		return -EINVAL;
	}

	return wifimgr_ctrl_iface_send_cmd(ctrl, cmd_id, capa, sizeof(union wifi_capa));
}

int wifimgr_ctrl_iface_get_status(char *iface_name, struct wifi_status *sts)
{
	struct wifimgr_ctrl_iface *ctrl;
	unsigned int cmd_id = 0;

	if (!iface_name || !sts)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
		ctrl = sta_ctrl;
		cmd_id = WIFIMGR_CMD_GET_STA_STATUS;
	} else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
		ctrl = ap_ctrl;
		cmd_id = WIFIMGR_CMD_GET_AP_STATUS;
	} else {
		return -EINVAL;
	}

	return wifimgr_ctrl_iface_send_cmd(ctrl, cmd_id, sts, sizeof(struct wifi_status));
}

int wifimgr_ctrl_iface_open(char *iface_name)
{
	struct wifimgr_ctrl_iface *ctrl = sta_ctrl;
	unsigned int cmd_id = 0;

	if (!iface_name)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		cmd_id = WIFIMGR_CMD_OPEN_STA;
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		cmd_id = WIFIMGR_CMD_OPEN_AP;
	else
		return -EINVAL;

	return wifimgr_ctrl_iface_send_cmd(ctrl, cmd_id, NULL, 0);
}

int wifimgr_ctrl_iface_close(char *iface_name)
{
	struct wifimgr_ctrl_iface *ctrl = sta_ctrl;
	unsigned int cmd_id = 0;

	if (!iface_name)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		cmd_id = WIFIMGR_CMD_CLOSE_STA;
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		cmd_id = WIFIMGR_CMD_CLOSE_AP;
	else
		return -EINVAL;

	return wifimgr_ctrl_iface_send_cmd(ctrl, cmd_id, NULL, 0);
}

int wifimgr_ctrl_iface_scan(char *iface_name, scan_res_cb_t scan_res_cb)
{
	struct wifimgr_ctrl_iface *ctrl;
	unsigned int cmd_id = 0;

	if (!iface_name)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
		ctrl = sta_ctrl;
		cmd_id = WIFIMGR_CMD_STA_SCAN;
	} else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
		ctrl = ap_ctrl;
		cmd_id = WIFIMGR_CMD_AP_SCAN;
	} else {
		return -EINVAL;
	}

	ctrl->scan_res_cb = scan_res_cb;

	return wifimgr_ctrl_iface_send_cmd(ctrl, cmd_id, NULL, 0);
}

#ifdef CONFIG_WIFIMGR_STA
int wifimgr_ctrl_iface_rtt_request(struct wifi_rtt_request *rtt_req, rtt_resp_cb_t rtt_resp_cb)
{
	struct wifimgr_ctrl_iface *ctrl = sta_ctrl;
	struct wifi_rtt_peers *peer;
	unsigned char nr_peers;
	int i;

	if (!rtt_req)
		return -EINVAL;

	ctrl = sta_ctrl;

	peer = rtt_req->peers;
	nr_peers = rtt_req->nr_peers;
	for (i = 0; i < nr_peers; i++, peer++) {
		/* Check BSSID */
		if (is_zero_ether_addr(peer->bssid))
			return -EINVAL;

		/* Check band */
		switch (peer->band) {
		case 2:
		case 5:
		case 0:
			break;
		default:
			wifimgr_err("invalid band: %u!\n", peer->band);
			return -EINVAL;
		}

		/* Check channel */
		if (!peer->channel || (peer->channel > 14 && peer->channel < 34) || (peer->channel > 196)) {
			wifimgr_err("invalid channel: %u!\n", peer->channel);
			return -EINVAL;
		}
	}

	ctrl->rtt_resp_cb = rtt_resp_cb;

	return wifimgr_ctrl_iface_send_cmd(ctrl, WIFIMGR_CMD_RTT_REQ, rtt_req, sizeof(struct wifi_rtt_request));
}

int wifimgr_ctrl_iface_connect(void)
{
	struct wifimgr_ctrl_iface *ctrl = sta_ctrl;

	return wifimgr_ctrl_iface_send_cmd(ctrl, WIFIMGR_CMD_CONNECT, NULL, 0);
}

int wifimgr_ctrl_iface_disconnect(void)
{
	struct wifimgr_ctrl_iface *ctrl = sta_ctrl;

	return wifimgr_ctrl_iface_send_cmd(ctrl, WIFIMGR_CMD_DISCONNECT, NULL, 0);
}
#endif

#ifdef CONFIG_WIFIMGR_AP
int wifimgr_ctrl_iface_start_ap(void)
{
	struct wifimgr_ctrl_iface *ctrl = ap_ctrl;

	return wifimgr_ctrl_iface_send_cmd(ctrl, WIFIMGR_CMD_START_AP, NULL, 0);
}

int wifimgr_ctrl_iface_stop_ap(void)
{
	struct wifimgr_ctrl_iface *ctrl = ap_ctrl;

	return wifimgr_ctrl_iface_send_cmd(ctrl, WIFIMGR_CMD_STOP_AP, NULL, 0);
}

int wifimgr_ctrl_iface_del_station(char *mac)
{
	struct wifimgr_ctrl_iface *ctrl = ap_ctrl;
	struct wifimgr_set_mac_acl set_acl;

	if (mac && !is_zero_ether_addr(mac)) {
		memcpy(set_acl.mac, mac, WIFIMGR_ETH_ALEN);
	} else {
		wifimgr_err("invalid MAC address!\n");
		return -EINVAL;
	}

	return wifimgr_ctrl_iface_send_cmd(ctrl, WIFIMGR_CMD_DEL_STA, &set_acl,
					   sizeof(set_acl));
}
int wifimgr_ctrl_iface_set_mac_acl(char subcmd, char *mac)
{
	struct wifimgr_ctrl_iface *ctrl = ap_ctrl;
	struct wifimgr_set_mac_acl set_acl;

	switch (subcmd) {
	case WIFIMGR_SUBCMD_ACL_BLOCK:
	case WIFIMGR_SUBCMD_ACL_UNBLOCK:
	case WIFIMGR_SUBCMD_ACL_BLOCK_ALL:
	case WIFIMGR_SUBCMD_ACL_UNBLOCK_ALL:
		set_acl.subcmd = subcmd;
		break;
	default:
		return -EINVAL;
	}

	if (mac && !is_zero_ether_addr(mac)) {
		memcpy(set_acl.mac, mac, WIFIMGR_ETH_ALEN);
	} else if (!mac) {
		memset(set_acl.mac, 0xff, WIFIMGR_ETH_ALEN);
	} else {
		wifimgr_err("invalid MAC address!\n");
		return -EINVAL;
	}

	return wifimgr_ctrl_iface_send_cmd(ctrl, WIFIMGR_CMD_SET_MAC_ACL, &set_acl,
					   sizeof(set_acl));
}
#endif

int wifimgr_ctrl_iface_send_cmd(struct wifimgr_ctrl_iface *ctrl, unsigned int cmd_id, void *buf, int buf_len)
{
	struct cmd_message msg;
	struct timespec ts;
	int prio;
	int ret;

	msg.cmd_id = cmd_id;
	msg.reply = 0;
	msg.buf_len = buf_len;
	msg.buf = buf;

	/* Send commands */
	ret = mq_send(ctrl->mq, (const char *)&msg, sizeof(msg), 0);
	if (ret == -1) {
		wifimgr_err("failed to send [%s]! errno %d\n",
			    wifimgr_cmd2str(msg.cmd_id), errno);
		ret = -errno;
	} else {
		wifimgr_dbg("send [%s], buf: 0x%08x\n",
			    wifimgr_cmd2str(msg.cmd_id), *(int *)msg.buf);

		/* Receive command replys */
		ret = clock_gettime(CLOCK_MONOTONIC, &ts);
		if (ret)
			wifimgr_err("failed to get clock time! %d\n", ret);
		ts.tv_sec += WIFIMGR_CMD_TIMEOUT;
		ret =
		    mq_timedreceive(ctrl->mq, (char *)&msg, sizeof(msg), &prio, &ts);
		if (ret == -1) {
			wifimgr_err("failed to get command reply! errno %d\n",
				    errno);
			if (errno == ETIME)
				wifimgr_err("[%s] timeout!\n",
					    wifimgr_cmd2str(msg.cmd_id));
			ret = -errno;
		} else {
			wifimgr_dbg("recv [%s] reply: %d\n",
				    wifimgr_cmd2str(msg.cmd_id), msg.reply);
			ret = msg.reply;
			if (ret)
				wifimgr_err("failed to exec [%s]! %d\n",
					    wifimgr_cmd2str(msg.cmd_id), ret);
		}
	}

	return ret;
}

int wifimgr_ctrl_iface_wait_event(char *iface_name)
{
	struct wifimgr_ctrl_iface *ctrl;
	int ret;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		ctrl = sta_ctrl;
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		ctrl = ap_ctrl;
	else
		return -EINVAL;

	ctrl->wait_event = true;
	ret = sem_wait(&ctrl->syncsem);
	if (ret == -1)
		return -errno;
	return ctrl->evt_status;
}

int wifimgr_ctrl_iface_wakeup(struct wifimgr_ctrl_iface *ctrl)
{
	int ret;

	if (!ctrl->wait_event)
		return 0;

	ret = sem_post(&ctrl->syncsem);
	if (ret == -1)
		ret = -errno;
	ctrl->wait_event = false;

	return ret;
}

int wifimgr_init_ctrl_iface(char *iface_name, struct wifimgr_ctrl_iface *ctrl)
{
	struct mq_des *mq;
	struct mq_attr attr;
	int ret;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
		sta_ctrl = ctrl;
		/* Initialize STA notifier chain */
		wifimgr_list_init(&ctrl->conn_chain.list);
		sem_init(&ctrl->conn_chain.exclsem, 0, 1);
		wifimgr_list_init(&ctrl->disc_chain.list);
		sem_init(&ctrl->disc_chain.exclsem, 0, 1);
	} else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
		ap_ctrl = ctrl;
		/* Initialize AP notifier chain */
		wifimgr_list_init(&ctrl->new_sta_chain.list);
		sem_init(&ctrl->new_sta_chain.exclsem, 0, 1);
		wifimgr_list_init(&ctrl->sta_leave_chain.list);
		sem_init(&ctrl->sta_leave_chain.exclsem, 0, 1);
	} else {
		return -EINVAL;
	}

	ret = sem_init(&ctrl->syncsem, 0, 0);
	if (ret == -1)
		ret = -errno;

	attr.mq_maxmsg = WIFIMGR_CMD_MQUEUE_NR;
	attr.mq_msgsize = sizeof(struct cmd_message);
	attr.mq_flags = 0;
	mq = mq_open(WIFIMGR_CMD_MQUEUE, O_RDWR, 0666, &attr);
	if (mq == (mqd_t)-1) {
		wifimgr_err("failed to open command queue %s! errno %d\n",
			    WIFIMGR_CMD_MQUEUE, errno);
		return -errno;
	}
	ctrl->mq = mq;

	return ret;
}

int wifimgr_destroy_ctrl_iface(char *iface_name, struct wifimgr_ctrl_iface *ctrl)
{
	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
		sta_ctrl = NULL;
		/* Deinitialize STA notifier chain */
		wifimgr_list_free(&ctrl->conn_chain.list);
		sem_destroy(&ctrl->conn_chain.exclsem);
		wifimgr_list_free(&ctrl->disc_chain.list);
		sem_destroy(&ctrl->disc_chain.exclsem);
	} else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
		ap_ctrl = NULL;
		/* Deinitialize AP notifier chain */
		wifimgr_list_free(&ctrl->new_sta_chain.list);
		sem_destroy(&ctrl->new_sta_chain.exclsem);
		wifimgr_list_free(&ctrl->sta_leave_chain.list);
		sem_destroy(&ctrl->sta_leave_chain.exclsem);
	} else {
		return -EINVAL;
	}

	if (ctrl->mq && (ctrl->mq != (mqd_t)-1))
		mq_close(ctrl->mq);

	sem_destroy(&ctrl->syncsem);
}
#endif

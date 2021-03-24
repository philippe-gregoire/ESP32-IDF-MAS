/******************************************************************************
# © Copyright IBM Corp. 2021.  All Rights Reserved.
#
# This program and the accompanying materials
# are made available under the terms of the Apache V2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#
# *****************************************************************************
# ESP32Wifi.h
#
# C++ Wrapper for ESP32 wifi
#
# Created on: 24 mars 2021
#
# Author: Philippe Gregoire - IBM France, Hybrid CLoud Build Team Europe
# *****************************************************************************/
#ifndef MAIN_ESP32WIFI_H_
#define MAIN_ESP32WIFI_H_

extern "C" {
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
}

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define IPADDR_LOCAL_DOMAIN ".local"

class ESP32_Wifi {
private:
	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	int s_retry_num = 0;
	const int wifi_max_retry;
	const char* hostname;

	void initialise_mdns();

	static void _event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
	static void _wait_tcp_task(void * that);

protected:
	EventGroupHandle_t s_wifi_event_group;
	virtual void event_handler(esp_event_base_t event_base, int32_t event_id, void* event_data);
	virtual void wait_tcp_task();

public:
	/* Create Wifi Client */
	ESP32_Wifi(const char* ssid, const char* password, const char* hostname, wifi_auth_mode_t authmode=WIFI_AUTH_WPA2_PSK, int wifi_max_retry=50);

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	virtual EventBits_t wait_wifi();

	/* Generate a hostname based on a base prefix and MAC Address */
	static char* generate_hostname(const char* hostname_base);

	static void query_mdns_host(const char * host_name);

	virtual ~ESP32_Wifi();
};

#endif /* MAIN_ESP32WIFI_H_ */

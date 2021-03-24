/******************************************************************************
# © Copyright IBM Corp. 2021.  All Rights Reserved.
#
# This program and the accompanying materials
# are made available under the terms of the Apache V2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#
# *****************************************************************************
# ESP32Wifi.cpp
#
# C++ Wrapper for ESP32 wifi
#
# Created on: 24 mars 2021
#
# Author: Philippe Gregoire - IBM France, Hybrid CLoud Build Team Europe
# *****************************************************************************/
#include "ESP32Wifi.h"

extern "C" {
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "mdns.h"
}

static const char *TAG_WIFI = "ESP32_Wifi";
static const char *TAG_MDNS = "ESP32_MDNs";
static const char *TAG_WAIT = "ESP32_MDNs";

void ESP32_Wifi::initialise_mdns()
{
    //initialize mDNS
    ESP_ERROR_CHECK( mdns_init() );
    //set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK( mdns_hostname_set(hostname) );
    ESP_LOGI(TAG_MDNS, "mdns hostname set to: [%s]", hostname);
    //set default mDNS instance name
    ESP_ERROR_CHECK( mdns_instance_name_set(hostname) );

    //structure with TXT records
    mdns_txt_item_t serviceTxtData[3] = {
        {"board","esp32"},
        {"u","user"},
        {"p","password"}
    };

    //initialize service
    ESP_ERROR_CHECK( mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData, 3) );
    //add another TXT item
    ESP_ERROR_CHECK( mdns_service_txt_item_set("_http", "_tcp", "path", "/foobar") );
    //change TXT item value
    ESP_ERROR_CHECK( mdns_service_txt_item_set("_http", "_tcp", "u", "admin") );
}

void ESP32_Wifi::_wait_tcp_task(void * that) {
	((ESP32_Wifi*)that)->wait_tcp_task();
}

void ESP32_Wifi::wait_tcp_task()
{
	ESP_LOGI(TAG_WAIT, "Waiting for Wifi");
	wait_wifi();

    tcpip_adapter_ip_info_t ip_info;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA,&ip_info);
    const char* pHostname;
    tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_STA, &pHostname);

    ESP_LOGI(TAG_WAIT, "Wifi init OK, host name '%s'",pHostname);

    ESP_LOGI(TAG_WAIT, "Initializing mDNS ");
    initialise_mdns();
//	query_mdns_host(MQTT_BROKER_HOST);
//
//	ESP_LOGI(TAG_WAIT, "Starting MQTT client broker to %s",MQTT_BROKER_HOST);
//	mqtt_app_start(MQTT_BROKER_HOST);
//
//	ESP_LOGI(TAG_WAIT,"Starting OTA. Use curl %d.%d.%d.%d:8032 --data-binary @build/$(PROJECT_NAME).bin", (ip_info.ip.addr)&0xff,(ip_info.ip.addr>>8)&0xff,(ip_info.ip.addr>>16)&0xff,ip_info.ip.addr>>24&0xff);
//    ota_server_start();

    vTaskDelete(NULL);
}

ESP32_Wifi::ESP32_Wifi(const char* ssid, const char* password, const char* hostname, wifi_auth_mode_t authmode, int wifi_max_retry)
: wifi_max_retry(wifi_max_retry), hostname(hostname) {
	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *esp_netif= esp_netif_create_default_wifi_sta();
	esp_netif_set_hostname(esp_netif, hostname);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	// Register handler for various events
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
														ESP_EVENT_ANY_ID,
														&_event_handler,
														this,
														&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
														IP_EVENT_STA_GOT_IP,
														&_event_handler,
														this,
														&instance_got_ip));
    // Start TCP wait task which will wait for Wifi connection on s_wifi_event_group
	xTaskCreate(&_wait_tcp_task, "Wait TCP", 4096, this, 5, NULL);

	wifi_config_t wifi_config;
	memset(&wifi_config,0,sizeof(wifi_config));
	wifi_config.sta.threshold.authmode=authmode;
	wifi_config.sta.pmf_cfg.capable=true;
	wifi_config.sta.pmf_cfg.required=false;

	// Copy SSID and PW making sure we don't overlap...
	strncpy((char *)wifi_config.sta.ssid,ssid,sizeof(wifi_config.sta.ssid)-1);
	wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid)-1]='\0';
	strncpy((char *)wifi_config.sta.password,password,sizeof(wifi_config.sta.password)-1);
	wifi_config.sta.password[sizeof(wifi_config.sta.password)-1]='\0';

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start() );

	ESP_LOGI(TAG_WIFI, "wifi_init_sta finished.");
	EventBits_t bits=wait_wifi();

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */
	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI(TAG_WIFI, "connected to AP SSID %s",wifi_config.sta.ssid);
	} else if (bits & WIFI_FAIL_BIT) {
		ESP_LOGI(TAG_WIFI, "Failed to connect to SSID %s",wifi_config.sta.ssid);
	} else {
		ESP_LOGE(TAG_WIFI, "UNEXPECTED EVENT");
	}
}

EventBits_t ESP32_Wifi::wait_wifi() {
	return xEventGroupWaitBits(s_wifi_event_group,
			WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
			pdFALSE,
			pdFALSE,
			portMAX_DELAY);
}

ESP32_Wifi::~ESP32_Wifi() {
	/* The event will not be processed after unregister */
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
	vEventGroupDelete(s_wifi_event_group);
}

/* Event handler for Wifi notifications */
void ESP32_Wifi::_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	((ESP32_Wifi*)arg)->event_handler(event_base,event_id,event_data);
}

void ESP32_Wifi::event_handler(esp_event_base_t event_base, int32_t event_id, void* event_data) {
   if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < wifi_max_retry) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG_WIFI, "retry to connect to the AP for the %d th time",s_retry_num);
		} else {
			ESP_LOGE(TAG_WIFI, "maximum wifi retry reached: %d",s_retry_num);
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG_WIFI,"connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG_WIFI, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

char* ESP32_Wifi::generate_hostname(const char* hostname_base)
{
    uint8_t mac[6];
    char   *hostname;
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (-1 == asprintf(&hostname, "%s-%02X%02X%02X", hostname_base, mac[3], mac[4], mac[5])) {
        abort();
    }
    return hostname;
}

void ESP32_Wifi::query_mdns_host(const char * host_name)
{
	if(strlen(host_name)>strlen(IPADDR_LOCAL_DOMAIN)) {
		const char *dot_pos=strrchr(host_name,'.');
		if(dot_pos!=NULL) {
			if(strcmp(dot_pos,IPADDR_LOCAL_DOMAIN)==0) {
				// there is a .local domain, make a stack-allocated copy and truncate
				uint host_name_base_len=dot_pos-host_name;
				char *host_name_base=(char *)alloca(host_name_base_len+1);
				memcpy(host_name_base,host_name,host_name_base_len);
				host_name_base[host_name_base_len]='\0';
				host_name=host_name_base;
			}
		}
	}

    ESP_LOGI(TAG_MDNS, "Query A: %s.local", host_name);

    struct esp_ip4_addr addr;
    addr.addr = 0;

    // This uses the hostname without .local
    esp_err_t err = mdns_query_a(host_name, 2000,  &addr);
    if(err){
        if(err == ESP_ERR_NOT_FOUND){
            ESP_LOGW(TAG_MDNS, "%s: Host %s was not found!", esp_err_to_name(err),host_name);
            return;
        }
        ESP_LOGE(TAG_MDNS, "Query Failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG_MDNS, "Query A: %s.local resolved to: " IPSTR, host_name, IP2STR(&addr));
}

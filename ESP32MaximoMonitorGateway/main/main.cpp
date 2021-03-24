/* *****************************************************************************
# © Copyright IBM Corp. 2021.  All Rights Reserved.
#
# This program and the accompanying materials
# are made available under the terms of the Apache V2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#
# *****************************************************************************
#
# Example of using an ESP-32 as Watson IoT gateway
#
# Written by Philippe Gregoire, IBM France, Hybrid CLoud Build Team Europe
# *****************************************************************************
*/
extern "C" {
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "esp_log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "esp_system.h"
extern uint8_t temprature_sens_read();

#include "mqtt_client.h"
}

#include "ESP32SPIFFS.h"
#include "ESP32Wifi.h"

static const char *LOG_TAG="WIOTP";
static const char *LOG_TAG_MQTT = "MQTT";

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

/* Generic MQTT call-back Event handler */
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(LOG_TAG_MQTT, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
            ESP_LOGI(LOG_TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            ESP_LOGI(LOG_TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            ESP_LOGI(LOG_TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            ESP_LOGI(LOG_TAG_MQTT, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(LOG_TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(LOG_TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(LOG_TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(LOG_TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(LOG_TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(LOG_TAG_MQTT, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(LOG_TAG_MQTT, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(LOG_TAG_MQTT, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(LOG_TAG_MQTT, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb((esp_mqtt_event_handle_t)event_data);
}



static esp_mqtt_client_handle_t wiotp_start(const char* wiotp_orgid, const char* wiotp_gw_type, const char* wiotp_gw_id, const char* wiotp_gw_token)
{
    char wiotp_uri[256];
    //snprintf(wiotp_uri,sizeof(wiotp_uri),"mqtt://%s.messaging.internetofthings.ibmcloud.com",wiotp_orgid);

    snprintf(wiotp_uri,sizeof(wiotp_uri),"mqtt://169.62.202.130");
    char wiotp_gw_client_id[256];
    snprintf(wiotp_gw_client_id,sizeof(wiotp_gw_client_id),"g:%s:%s:%s",wiotp_orgid,wiotp_gw_type,wiotp_gw_id);

	// Setting up gateway connection
	// See https://www.ibm.com/support/knowledgecenter/SSQP8H/iot/platform/gateways/mqtt.html
    esp_mqtt_client_config_t mqtt_cfg;
    memset(&mqtt_cfg,0,sizeof(mqtt_cfg));
    mqtt_cfg.uri = wiotp_uri;
    mqtt_cfg.client_id = wiotp_gw_client_id;
	mqtt_cfg.username = "use-token-auth";
	mqtt_cfg.password = wiotp_gw_token;

    ESP_LOGI(LOG_TAG_MQTT,"Connecting to %s with clientid=%s",mqtt_cfg.uri,mqtt_cfg.client_id);
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, client);
    esp_mqtt_client_start(client);

    return client;
}

extern "C" void app_main(void)
{
	// Init SPIFFS
	ESP32_SPIFFS spiffs=ESP32_SPIFFS();

    nvs_flash_init();

    // Init Wifi Station (client)
	ESP32_Wifi wifi=ESP32_Wifi(spiffs.read_string("/secret/wifi_ssid.txt"),
								spiffs.read_string("/secret/wifi_pass.txt"),ESP32_Wifi::generate_hostname("WIOTP"));
    // Wait 5 sec for TCPIP to come up
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    // Init wiotp MQTT Broker
    const char* wiotp_orgid=ESP32_SPIFFS::read_string("/secret/wiotp_orgid.txt",16);
	const char* wiotp_gw_type=ESP32_SPIFFS::read_string("/secret/wiotp_gw_type.txt");
	const char* wiotp_gw_id=ESP32_SPIFFS::read_string("/secret/wiotp_gw_id.txt");
	const char* wiotp_gw_token=ESP32_SPIFFS::read_string("/secret/wiotp_gw_token.txt");
    esp_mqtt_client_handle_t mqttCl=wiotp_start(wiotp_orgid,wiotp_gw_type,wiotp_gw_id,wiotp_gw_token);

    const char* wiotp_dev_type=ESP32_SPIFFS::read_string("/secret/wiotp_dev_type.txt");
	const char* wiotp_dev_id=ESP32_SPIFFS::read_string("/secret/wiotp_dev_id.txt");

	char wiotp_topic[256];
    // iot-2/type/typeId/id/deviceId/evt/eventId/fmt/formatString
    snprintf(wiotp_topic,sizeof(wiotp_topic),"iot-2/type/%s/id/%s/evt/%s/fmt/json",wiotp_dev_type,wiotp_dev_id,"data");

    char wiotp_payload[512];

    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    int level = 0;
    while (true) {
    	uint8_t temp=temprature_sens_read()*rand();
    	ESP_LOGI(LOG_TAG,"HeartBeat %d %d",level,temp);

    	// Publish at QOS 1, no retain
    	snprintf(wiotp_payload,sizeof(wiotp_payload),"{\"d\":{\"temp\":%d}}",temp);
    	esp_mqtt_client_publish(mqttCl, wiotp_topic, wiotp_payload, 0, 1, 0);

        gpio_set_level(GPIO_NUM_4, level);
        level = !level;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/******************************************************************************
# © Copyright IBM Corp. 2021.  All Rights Reserved.
#
# This program and the accompanying materials
# are made available under the terms of the Apache V2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#
# *****************************************************************************
# ESP32SPIFFS.cpp
#
# C++ Wrapper for ESP32 SPIFFS
#
# Created on: 24 mars 2021
#
# Author: Philippe Gregoire - IBM France, Hybrid CLoud Build Team Europe
# *****************************************************************************/
#include "ESP32SPIFFS.h"

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "esp_log.h"

#include "esp_spiffs.h"
}

static const char *LOG_TAG="SPIFFS";

ESP32_SPIFFS::ESP32_SPIFFS(size_t max_files, const char* base_path,const char* partition_label) {
    ESP_LOGI(LOG_TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t conf = {
	 .base_path = base_path,
	 .partition_label = partition_label,
	 .max_files = max_files,
	 .format_if_mount_failed = false
	};

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
	   if (ret == ESP_FAIL) {
		   ESP_LOGE(LOG_TAG, "Failed to mount or format filesystem");
	   } else if (ret == ESP_ERR_NOT_FOUND) {
		   ESP_LOGE(LOG_TAG, "Failed to find SPIFFS partition");
	   } else {
		   ESP_LOGE(LOG_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
	   }
	   return;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK) {
	   ESP_LOGE(LOG_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	} else {
	   ESP_LOGI(LOG_TAG, "Partition size: total: %d, used: %d", total, used);
	}

}

ESP32_SPIFFS::~ESP32_SPIFFS() {
	// TODO Auto-generated destructor stub
}

char* ESP32_SPIFFS::read_string(const char* filename, size_t max_len) {
	struct stat filestat;

	if(stat (filename,&filestat)<0) {
		return NULL;
	}

	if(filestat.st_size>max_len) filestat.st_size=max_len;
	char* filebuf=(char*)malloc(filestat.st_size+1);

	if(!read_string(filename, filebuf, filestat.st_size+1)) {
		// free the buffer and return NULL
		free(filebuf);
		filebuf=NULL;
	}

	return filebuf;
}

bool ESP32_SPIFFS::read_string(const char* filename, char* buf, size_t buflen)
{
    ESP_LOGI(LOG_TAG, "Reading %s %d bytes", filename, buflen);

    // Open for reading hello.txt
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        ESP_LOGE(LOG_TAG, "Failed to open %s",filename);
        return false;
    }

    memset(buf, 0, buflen);
    fread(buf, 1, buflen-1, f);
    fclose(f);

    // Display the read contents from the file
    ESP_LOGV(LOG_TAG, "Read from %s: %s", filename, buf);

    return true;
}

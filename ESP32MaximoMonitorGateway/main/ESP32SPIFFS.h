/******************************************************************************
# © Copyright IBM Corp. 2021.  All Rights Reserved.
#
# This program and the accompanying materials
# are made available under the terms of the Apache V2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#
# *****************************************************************************
# ESP32SPIFFS.h
#
# C++ Wrapper for ESP32 SPIFFS
#
# Created on: 24 mars 2021
#
# Author: Philippe Gregoire - IBM France, Hybrid CLoud Build Team Europe
# *****************************************************************************/
#ifndef MAIN_ESP32SPIFFS_H_
#define MAIN_ESP32SPIFFS_H_

extern "C" {
#include <stddef.h>
}

/**
 * Wrapper for ESP32 SPIFFS.
 * Usage:
 *   Init ESP32_SPIFFS() instance anywhere
 *   Use static helper functions to read files
 */
class ESP32_SPIFFS {
public:
	ESP32_SPIFFS(size_t max_files=5,const char* base_path="",const char* partition_label=NULL);
	virtual ~ESP32_SPIFFS();

	/* Read a string from file, at most buflen-1 chars, always zero-terminated */
	static bool read_string(const char* filename, char* buf, size_t buflen);

	/* Read heap-mallocate and read a string from file. A buffer of appropriate siez is malloced, up to max_len+1 size */
	static char* read_string(const char* filename, size_t max_len=256);
};

#endif /* MAIN_ESP32SPIFFS_H_ */

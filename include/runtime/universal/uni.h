#pragma once

#include "helper.h"
#include <bits/types/FILE.h>

/** 
 * This module contains native function defintiions
 * that are required by the UniAPI.
 */

/**
 * This method takes a string
 * executes the string as a shell command
 * using the system() function of libc. 
 */ 
DECLARE_NATIVE_METHOD(uni_exec_cmd);

/**
 * Returns information about the current platform 
 * on which it is running.
 */
DECLARE_NATIVE_METHOD(uni_get_platform_info);


// File class.
extern Object UNI_FILE_CLASS;

/**
 * Meta data for the file class.
 */
typedef struct {
  FILE *fptr;
} UniFile;

// Initializes the File class.
DECLARE_NATIVE_METHOD(uni_file_class_init);

// Method to open a file. Uses fopen internally.  
DECLARE_NATIVE_METHOD(uni_file_class_method_open);

// Method to open a file. Uses fclose internally.
DECLARE_NATIVE_METHOD(uni_file_class_method_close);

// Method to open a file. Uses
//DECLARE_NATIVE_METHOD(uni_file_class_method_read);

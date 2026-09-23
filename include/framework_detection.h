/**
 * @file framework_detection.h
 * @brief Framework detection and configuration for ESP32 platforms
 * 
 * This header automatically detects whether the code is being compiled
 * for Arduino or ESP-IDF framework and sets appropriate configurations.
 * 
 * @author playmiel
 * @license BSD-3-Clause
 */

#ifndef FRAMEWORK_DETECTION_H
#define FRAMEWORK_DETECTION_H

// Framework detection based on preprocessor definitions
#if defined(ARDUINO) || defined(ESP_ARDUINO_VERSION)
    #define LIBSSH2_ESP_ARDUINO
    #define LIBSSH2_ESP_FRAMEWORK "Arduino"
    
    // Arduino-specific includes
    #ifdef __cplusplus
        #include <Arduino.h>
        #include <WiFi.h>
        // Arduino-specific configurations for C++
        #define LIBSSH2_ESP_LOG(fmt, ...) Serial.printf("[libssh2_esp] " fmt "\n", ##__VA_ARGS__)
        #define LIBSSH2_ESP_ERROR(fmt, ...) Serial.printf("[libssh2_esp ERROR] " fmt "\n", ##__VA_ARGS__)
    #else
        // C-compatible includes for Arduino
        #include <lwip/sockets.h>
        #include <stdio.h>
        // Arduino-specific configurations for C
        #define LIBSSH2_ESP_LOG(fmt, ...) printf("[libssh2_esp] " fmt "\n", ##__VA_ARGS__)
        #define LIBSSH2_ESP_ERROR(fmt, ...) printf("[libssh2_esp ERROR] " fmt "\n", ##__VA_ARGS__)
    #endif
    #include <lwip/sockets.h>
    
#elif defined(ESP_PLATFORM) || defined(ESP_IDF) || defined(IDF_VER)
    #define LIBSSH2_ESP_IDF
    #define LIBSSH2_ESP_FRAMEWORK "ESP-IDF"
    
    // ESP-IDF specific includes
    #include "esp_system.h"
    #include "esp_log.h"
    #include "esp_timer.h"
    #include "lwip/sockets.h"
    #include "lwip/netdb.h"
    #include "lwip/inet.h"
    
    // ESP-IDF specific configurations
    static const char* TAG = "libssh2_esp";
    #define LIBSSH2_ESP_LOG(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
    #define LIBSSH2_ESP_ERROR(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
    
#else
    #error "Unsupported framework. This library requires Arduino or ESP-IDF framework."
#endif

// Common ESP32 configurations
#define LIBSSH2_ESP_VERSION "1.1.0"


// libssh2 configuration flags
#ifndef LIBSSH2_MBEDTLS
#define LIBSSH2_MBEDTLS
#endif

#ifndef HAVE_LIBSSH2_H
#define HAVE_LIBSSH2_H
#endif

#ifndef LIBSSH2_NO_ZLIB
#define LIBSSH2_NO_ZLIB
#endif

// Network configuration
#define LIBSSH2_ESP_SOCKET_TIMEOUT_MS 10000
#define LIBSSH2_ESP_CONNECT_TIMEOUT_MS 30000

// Framework-specific utility macros
#ifdef LIBSSH2_ESP_ARDUINO
    #define LIBSSH2_ESP_DELAY(ms) delay(ms)
    #define LIBSSH2_ESP_MILLIS() millis()
#elif defined(LIBSSH2_ESP_IDF)
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #define LIBSSH2_ESP_DELAY(ms) vTaskDelay(pdMS_TO_TICKS(ms))
    #define LIBSSH2_ESP_MILLIS() (esp_timer_get_time() / 1000)
#endif

#endif // FRAMEWORK_DETECTION_H

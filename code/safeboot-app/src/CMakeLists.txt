idf_component_register(SRCS "main.cpp"
    REQUIRES nvs_flash esp_wifi esp_netif esp_event esp_http_server esp_http_client app_update esp_partition
             esp_timer mbedtls)

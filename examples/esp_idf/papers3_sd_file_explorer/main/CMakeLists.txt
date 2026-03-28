idf_component_register(SRCS "papers3_sd_file_explorer.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES sdmmc fatfs driver jpegdec
                    PRIV_REQUIRES esp_timer)

#include "linux_download.h"


uint8_t compute_checksum(const uint8_t *data, uint32_t size)
{
    uint8_t checksum = 0xEF;

    while (size--) {
        checksum ^= *data++;
    }

    return checksum;
}

FILE *get_file_size(char *path, ssize_t *image_size)
{
	//"./load_bin/project_template.bin"
	FILE *fp = fopen(path, "r");
	if(fp == NULL) {
        printf("fopen fail!");
        return (0);
    }
	fseek(fp, 0L, SEEK_END);
    *image_size = ftell(fp);
    rewind(fp);

    printf("Image size: %lu\n", *image_size);

    return fp;
}


esp_loader_error_t linux_download_to_esp8266(int fd, int addr, char *path)
{
    esp_loader_error_t err;
    //STEP4 flash interaction esp8266 stub loader
#if ENABLE_STUB_LOADER

    int32_t packet_number = 0;
    ssize_t load_bin_size = 0;
    FILE *image = get_file_size(path, &load_bin_size);

    err = esp_loader_flash_start(fd, addr, load_bin_size, sizeof(payload_flash));
    if (err != ESP_LOADER_SUCCESS) {
        printf("Flash start operation failed.\n");
        return (0);
    }
    int send_times = 0;
    while(load_bin_size > 0) {
        memset(payload_flash,0x0,sizeof(payload_flash));
        ssize_t load_to_read = READ_BIN_MIN(load_bin_size, sizeof(payload_flash));
        printf("load_to_read:%lu\n",load_to_read);
        ssize_t read = fread(payload_flash, 1, load_to_read, image);
        printf("read:%lu\n",read);
        if (read != load_to_read) {
            printf("Error occurred while reading file.\n");
            return (0);
        }
        send_times++;
        printf("send_times=%d\n",send_times);
        printf("compute_checksum in main:%lu\n", compute_checksum(payload_flash, load_to_read));
        err = esp_loader_flash_write(fd, payload_flash, load_to_read);
        printf("stub loader err=%d\n",err);
        if (err != ESP_LOADER_SUCCESS) {
            printf("Packet could not be written.\n");
            return (0);
        }
        printf("send_times=%d\n",send_times);
        printf("packet: %d  written: %lu B\n", packet_number++, load_to_read);

        load_bin_size -= load_to_read;
    };

    printf("Flash write done.\n");
    err = esp_loader_flash_verify(fd);
    if (err != ESP_LOADER_SUCCESS) {
        printf("MD5 does not match. err: %d\n", err);
    } else {
        printf("Flash verified success!\n");
    }
    // printf("Flash verified\n");
    fclose(image);

#elif

    //STEP4 flash interaction esp8266 rom loader
    int32_t packet_number = 0;
    ssize_t load_bin_size = 0;
    FILE *image = get_file_size("./load_bin/esp8266/project_template.bin", &load_bin_size);

    err = esp_loader_flash_start(serial_fd, LOAD_APP_ADDRESS_START, load_bin_size, sizeof(payload_flash));
    if (err != ESP_LOADER_SUCCESS) {
        printf("Flash start operation failed.\n");
        return (0);
    } 
    while(load_bin_size > 0) {
        ssize_t load_to_read = READ_BIN_MIN(load_bin_size, sizeof(payload_flash));
        ssize_t read = fread(payload_flash, 1, load_to_read, image);
        if (read != load_to_read) {
            printf("Error occurred while reading file.\n");
            return (0);
        }

        err = esp_loader_flash_write(serial_fd, payload_flash, load_to_read);
        if (err != ESP_LOADER_SUCCESS) {
            printf("Packet could not be written.\n");
            return (0);
        }

        printf("packet: %d  written: %lu B\n", packet_number++, load_to_read);

        load_bin_size -= load_to_read;
        
    };

    // 8266 ROM NOT SUPPORT md5 VERIFY
    err = esp_loader_flash_verify(serial_fd);
    if (err != ESP_LOADER_SUCCESS) {
        printf("MD5 does not match. err: %d\n", err);
        
    }
    printf("Flash verified\n");

    fclose(image);
#endif

    return ESP_LOADER_SUCCESS;
}


void parsing_config_doc_download(int fd, char *config_doc_path)
{
    char count = 0;
    char addr_local = 0;
    FILE* fp;
    char store_para[100][100] = { 0 };
    char store_addr_local[100] = { 0 };

    fp = fopen(config_doc_path, "r");

    while (fscanf(fp, "%s", store_para[count]) != EOF)
    {
        if (strncmp(store_para[count],"0x",2) == 0) {

            // debug
            // printf("-----------%s\n", store_para[count]);
            // printf("-----------%d------------\n", count);
            store_addr_local[addr_local] = count;
            addr_local++;
        }
        count++;
    }

    for (int addr_local_times = 0; addr_local_times < addr_local; addr_local_times++) {

        int nValude = 0;
        //debug
         printf("------addr----%s\n",store_para[store_addr_local[addr_local_times]]);
         printf("------command----%s\n",store_para[store_addr_local[addr_local_times] + 1]);
        
        sscanf(store_para[store_addr_local[addr_local_times]], "%x", &nValude); 
        linux_download_to_esp8266(fd, nValude, store_para[store_addr_local[addr_local_times] + 1]);

        //printf("----%0x----\n",nValude);
        //  printf("------command----%s\n",store_para[store_addr_local[addr_local_times] + 1]);
    }

//debug
#if 0
    {
// ------addr----0x8000
// ------command----partition_table/partition-table.bin
// ------addr----0x9000
// ------command----ota_data_initial.bin
// ------addr----0x8000
// ------command----partition_table/partition-table.bin
// ------addr----0x9000
// ------command----ota_data_initial.bin
// ------addr----0x10000
// ------command----esp-at.bin
// ------addr----0xF0000
// ------command----at_customize.bin
// ------addr----0xF2000
// ------command----customized_partitions/server_cert.bin
// ------addr----0x106000
// ------command----customized_partitions/mqtt_key.bin
// ------addr----0xF4000
// ------command----customized_partitions/server_key.bin
// ------addr----0xF6000
// ------command----customized_partitions/server_ca.bin
// ------addr----0xFC000
// ------command----customized_partitions/client_ca.bin
// ------addr----0xF1000
// ------command----customized_partitions/factory_param.bin
// ------addr----0x104000
// ------command----customized_partitions/mqtt_cert.bin
// ------addr----0x108000
// ------command----customized_partitions/mqtt_ca.bin
// ------addr----0xF8000
// ------command----customized_partitions/client_cert.bin
// ------addr----0xFA000
// ------command----customized_partitions/client_key.bin
// ------addr----0x0
// ------command----bootloader/bootloader.bin

        // sscanf(store_para[store_addr_local[addr_local_times]], "%x", &nValude); 
        printf("linux_download_to_esp8266--%d--%s---\n",0x8000, "partition_table/partition-table.bin");
        linux_download_to_esp8266(fd, 0x8000, "partition_table/partition-table.bin");
        loader_port_delay_ms(21);
        printf("linux_download_to_esp8266--%d--%s---\n",0x9000, "ota_data_initial.bin");
        linux_download_to_esp8266(fd, 0x9000, "ota_data_initial.bin");
        loader_port_delay_ms(21);
        printf("linux_download_to_esp8266--%d--%s---\n",0x0, "bootloader/bootloader.bin");
        linux_download_to_esp8266(fd, 0x0, "bootloader/bootloader.bin");
        sleep(1);
        // printf("linux_download_to_esp8266--%d--%s---\n",0x10000, "esp-at.bin");
        // linux_download_to_esp8266(fd, 0x10000, "esp-at.bin");
        // sleep(1);
        // printf("linux_download_to_esp8266--%d--%s---\n",0xF0000, "esp-at_customize.bin");
        // linux_download_to_esp8266(fd, 0xF0000, "esp-at_customize.bin");
    }
#endif
    fclose(fp);
}
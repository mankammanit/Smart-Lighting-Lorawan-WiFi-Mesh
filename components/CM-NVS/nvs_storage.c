#include "nvs_storage.h"

void save_led(led_status ptr)
{
        // Open
        // printf("\n");
        // printf("Opening Non-Volatile Storage (NVS) handle...\n");
        nvs_handle my_handle;
        esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
        if (err != ESP_OK)
        {
                printf("Error (%d) opening NVS handle!\n", err);
        }
        else
        {
                // printf("Write ratio_state Done\n");
                // Write
                // printf("Updating restart counter in NVS ... ");
                err = nvs_set_blob(my_handle, "led_state", &ptr, sizeof(led_status));
                // printf((err != ESP_OK) ? "Failed!\n" : "ratio_state Done\n");

                // printf("Committing updates in NVS ... ");
                err = nvs_commit(my_handle);
                printf((err != ESP_OK) ? "Failed!\n" : "Write led_state Done\n");

                // Close
                nvs_close(my_handle);
        }
}

bool read_led(led_status *ptr)
{
        // Open
        // printf("\n");
        // printf("Opening Non-Volatile Storage (NVS) handle...\n");
        nvs_handle my_handle;
        esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
        if (err != ESP_OK)
        {
                printf("Error (%d) opening NVS handle!\n", err);
                return false;
        }
        else
        {
                // printf("Read ratio_state Done\n");

                // Read
                // printf("Reading restart counter from NVS ...\n");
                size_t size_read = 0;
                err = nvs_get_blob(my_handle, "led_state", NULL, &size_read);
                if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
                        return err;

                // printf("size :%d\n", size_read);

                err = nvs_get_blob(my_handle, "led_state", ptr, &size_read);
                switch (err)
                {
                case ESP_OK:
                        // printf("Done\n");
                        // printf("Restart counter = %d\n", size_read);
                        break;
                case ESP_ERR_NVS_NOT_FOUND:
                        // printf("The value is not initialized yet!\n");
                        return false;
                        break;
                default:
                        // printf("Error (%d) reading!\n", err);
                        return false;
                        break;
                }
                nvs_close(my_handle);
                return true;
        }
}
void save_string(const char *key, char *str)
{

        // Open
        esp_err_t err;

        printf("\n");
        printf("Opening Non-Volatile Storage (NVS) handle... ");
        nvs_handle my_handle;
        err = nvs_open(key, NVS_READWRITE, &my_handle);
        if (err != ESP_OK) {
                printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        } else {
                printf("Done\n");

                // Write
                printf("Updating restart counter in NVS ... ");

                err = nvs_set_str(my_handle, key, str);
                printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

                printf("Committing updates in NVS ... ");

                err = nvs_commit(my_handle);
                printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

                // Close
                nvs_close(my_handle);
        }

}

bool get_string(const char* key, char* out_value, size_t* length)
{

        // Open
        esp_err_t err;

        printf("\n");
        printf("Opening Non-Volatile Storage (NVS) handle... ");
        nvs_handle my_handle;
        err = nvs_open(key, NVS_READWRITE, &my_handle);
        if (err != ESP_OK) {
                printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        } else {
                printf("Done\n");

                // Read
                printf("Reading restart counter from NVS ... ");
                err = nvs_get_str(my_handle, key, out_value, length);
                switch (err) {
                case ESP_OK:
                        printf("Done\n");
                        return true;
                        break;
                case ESP_ERR_NVS_NOT_FOUND:
                        printf("The value is not initialized yet!\n");
                        return false;
                        break;
                default:
                        printf("Error (%s) reading!\n", esp_err_to_name(err));
                        return false;
                }
                nvs_close(my_handle);
                return true;
        }
        return false;
}

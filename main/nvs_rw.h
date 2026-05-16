#ifndef NVS_RW
#define NVS_RW

#include "my_table.h"
#include "esp_err.h"

struct temp {
    char username[11];
    char password[5];
    uint8_t permission;
};

void save_user_data(struct user *node);
esp_err_t read_stored_blobs(void);
void delete_NVS(void);

#endif

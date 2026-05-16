#include "nvs_flash.h"
#include "esp_log.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "my_table.h"
#include "nvs_rw.h"

/* NVS namespace */
#define STORAGE_NAMESPACE "storage"

extern struct user* root;

static const char* TAG = "NVS_RW";

//struct temp {
//    char username[11];
//    char password[5];
//};

struct temp users[ITEM_CNT];
nvs_handle_t my_handle;

uint8_t copy_linked_list(){
    struct user* temp = root;
    uint8_t i = 0,active = 0;
    while (temp != NULL) {
        strcpy(users[i].username,temp->name);
        strcpy(users[i].password,temp->pswd);
        active = lv_table_has_cell_ctrl(ui_table,i+1,2,LV_TABLE_CELL_CTRL_CUSTOM_1);
        users[i].permission = active;       
        i++;
        if (temp->next != NULL) {
            temp = temp->next;
        }
        else {
            break;
        }
    }
    return i;
}

void save_user_data(struct user *node)
{
    esp_err_t err;

    /* converting linked list references into actual memory */
    uint8_t i = 0;

    copy_linked_list();
    
    //--------------for debug ------------------
    while ( users[i].username[0] != '\0' ) {
        printf("array of structure:\n");
        printf("username:%s\n",users[i].username);
        printf("password:%s\n",users[i].password);
        printf("permission:%d\n\n",users[i].permission);
        i++;
    }
    //-------------------------------------------

    // Open NVS handle
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }


    // Write blob
    ESP_LOGI(TAG, "Saving user data blob...");
    err = nvs_set_blob(my_handle, "user_data", users, sizeof(users));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write test data blob!");
        nvs_close(my_handle);
    }

    // Commit                                                                   
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit data");
    }
    nvs_close(my_handle);
}


esp_err_t read_stored_blobs(void)
{
    esp_err_t err;
    size_t required_size= 0;
    uint8_t i = 0;
    struct user* temp = NULL;

    err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        printf("\nin read_stord blob:\n");
        printf("\nError (%s) reading array size!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        return err;
    }

    /* getting size */
    err = nvs_get_blob(my_handle, "user_data", NULL, &required_size);

    printf("user_data_size:%zu\n",required_size);

    /* getting data from NVS */
    ESP_LOGI(TAG, "Reading user data blob:");
    err = nvs_get_blob(my_handle, "user_data", users, &required_size);

    if (err == ESP_OK) {

        i=0;
        while ( users[i].username[0] != '\0' ) {
            printf("from NVS username:%s\n",users[i].username);
            printf("from NVS password:%s\n\n",users[i].password);
            root = add_user(root,users[i].username,users[i].password);
            printf("node1\n");
            if (users[i].permission == 0) {
                printf("node2\n");
                lv_table_clear_cell_ctrl(ui_table, i+1, 2, LV_TABLE_CELL_CTRL_CUSTOM_1);
                printf("node3\n");
            }
            else {
                lv_table_add_cell_ctrl(ui_table, i+1, 2, LV_TABLE_CELL_CTRL_CUSTOM_1);
            }
            i++;
        }

        temp = root;
        while ( temp != NULL ) {
            printf("add_user: username=%s\npassword%s\n",temp->name,temp->pswd);
            if ( temp->next != NULL ) {
                temp = temp->next;
            }
            else {
                break;
            }
        }
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND) {                           
        printf("+++++*****ESP_ERR_NVS_NOT_FOUND\n");
    }
    else if (err == ESP_ERR_NVS_INVALID_HANDLE) {                           
        printf("+++++*****ESP_ERR_NVS_INVALID_HANDLE\n");
    }
    else if (err == ESP_ERR_NVS_INVALID_NAME) {                           
        printf("+++++*****ESP_ERR_NVS_INVALID_NAME\n");
    }
    else if (err == ESP_ERR_NVS_INVALID_LENGTH) {                           
        printf("+++++*****ESP_ERR_NVS_INVALID_LENGTH\n");
    }
    /* if handle has been closed or is NULL

       if key name doesn't satisfy constraints

       if length is not sufficient to store data

*/
    return 0;
}

void delete_NVS(void){
    nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
    nvs_flash_erase();
    nvs_close(my_handle);
}

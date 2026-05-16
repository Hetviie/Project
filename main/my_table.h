#ifndef TABLE_CREATE
#define TABLE_CREATE

#include "ui.h"
#include "mqtt_client.h"
#define ITEM_CNT 50

/* structure for creating linked list of username and password */
struct user {
    char* name;
    char* pswd;
    struct user* next;
};

extern struct user* root;
extern lv_obj_t *ui_table;

extern uint8_t added_users;
extern esp_mqtt_client_handle_t mqtt_client;

void create_table(lv_obj_t *);
void update_table (void);
void clr_table (lv_obj_t *);

void user_data_publish(struct user*);
void delete_users(struct user* );

#endif

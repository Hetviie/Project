/*
DESCRPTION: CODE FOR FETCHING DATA FROM THINGSBOARD USING HTTP PROTOCOL VIA REST API AND DISPLAYING THE DATA OBTAINED FROM THINGSBOARD ON THE WAVESHARE DISPLAY.
*/

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "lvgl.h"
#include "confg.h"
#include "driver/uart.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "mqtt_client.h"

//Driver
#include "driver/i2c.h"
#include "esp_lcd_touch_gt911.h"

//including the UI
#include "ui/ui.h"
//importing the file
#include "../components/espressif__esp_lcd_touch/display.h"


/* including custom C header files */
#include "my_table.h"
#include "nvs_rw.h"

#define MQTT_BROKER_URI "mqtt://eu.thingsboard.cloud"
#define ACCESS_TOKEN "2gtTJnxmP8zuC87MqsEi"

#define TB_USER "hetvi.shah@embrill.com"
#define TB_PASS "hetvi148"

#define DEVICE_ID "f43721a0-4304-11f1-bfc4-25a208a27468"
#define LOGIN_URL "https://eu.thingsboard.cloud/api/auth/login"
#define FETCH_URL "https://eu.thingsboard.cloud/api/plugins/telemetry/DEVICE/" DEVICE_ID "/values/timeseries?keys=Channel_1,Channel_2,Channel_3,Channel_4,Channel_5,Channel_6"

struct user* add_user ( struct user *, char *, char *);
void mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void mqtt_client_init(void);

int mqtt_connected=0;
int wifi_connected=0;

/* admin credantials */
char *gAdmin_name = "1234567890";
char *gAdmin_pswd = "1234";
/* wifi credentials */
uint8_t gWifi_ssid[32] = "Jenil";
uint8_t gWifi_pass[64] = "12345678";

/* task handler for wifi_task task */
TaskHandle_t gWifi_handle;

//lv_timer_t * gTimer;
//lv_task_t * gFetch_task;

//////////////////////////////////////////////////////HETVI'S CODE///////////////////////////////////////////////////////

//final data will be stored in this structure's members
typedef struct {
    float ch1;
    float ch2;
    float ch3;
    float ch4;
    float ch5;
    float ch6;
}data_t;

data_t data;

char receive[2048];
char jwt_token[2048];

esp_mqtt_client_handle_t mqtt_client;
//Wifi Functions

void wifi_event_handler(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data)
{
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){ //checks for wifi connectivity
       printf("Disconnected\nRetrying...\n");
       wifi_connected=0;
       esp_wifi_connect();//connects to wifi
    }else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){//checks for IP
       printf("Wifi Ready!\n");
       wifi_connected=1;
       if ( mqtt_client == NULL ) {
           mqtt_client_init();//calls function for initilization of mqtt
       }
    }
}

void wifi_init(){

    //initiialize wifi driver
    wifi_init_config_t config=WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&config);

    esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    wifi_config_t wifi_data = {0};
    memcpy(wifi_data.sta.ssid, gWifi_ssid,sizeof(gWifi_ssid)/sizeof(uint8_t));
    memcpy(wifi_data.sta.password, gWifi_pass,sizeof(gWifi_pass)/sizeof(uint8_t));
    printf("ssid=%s\n",wifi_data.sta.ssid);
    printf("pswd=%s\n",wifi_data.sta.password);

    esp_wifi_set_mode(WIFI_MODE_STA); //set station mode
    esp_wifi_set_config(WIFI_IF_STA, &wifi_data);//sets the wifi data configuration

    esp_wifi_start();//starts wifi
    vTaskDelay(2000/portTICK_PERIOD_MS);
    esp_wifi_connect();//connect wifi station to AP

    printf("Connecting...\n");

}


/*---------------HTTP EVENT HANDLER-------------------*/
esp_err_t http_event_handler(esp_http_client_event_t *event){

      //check if any data received
      if (event->event_id == HTTP_EVENT_ON_DATA){
        strncat(receive,(char*)event->data,event->data_len);//data is received in parts so it combines them
      }
      return ESP_OK;
}

/*----------------LOGIN FUNCTION-------------------*/
void login(){

    memset(receive,0,sizeof(receive));//cleans the buffer

    //http configuration
    esp_http_client_config_t config={
        .url=LOGIN_URL,
        .method=HTTP_METHOD_POST,//method to send data
        .event_handler=http_event_handler,
        .crt_bundle_attach=esp_crt_bundle_attach,//certificate for verification
        .buffer_size=4096,
        .buffer_size_tx=2048,
        .timeout_ms=15000,
        .disable_auto_redirect=true,
    };

    esp_http_client_handle_t client=esp_http_client_init(&config);//initialize http

    char login_data[256];

    sprintf(login_data,"{\"username\":\"%s\",\"password\":\"%s\"}",TB_USER,TB_PASS);

    esp_http_client_set_header(client,"Content-Type", "application/json");//set header(headers->additional info about browser or content)
    esp_http_client_set_post_field(client,login_data,strlen(login_data));

    esp_http_client_perform(client);//main function which sends http request , headers , etc.

    printf("Login Response Received!\n");

    //Token Extraction

    char *token_start;

    token_start=strstr(receive,"\"token\":\"");

    if(token_start!=NULL){
        
        char temp_token[1024];
        token_start+=9;
        
        sscanf(token_start,"%1023[^\"]", temp_token);
        snprintf(jwt_token,sizeof(jwt_token), "Bearer %s", temp_token);
        printf("JWT Token Stored!!!\n");
    }
    esp_http_client_cleanup(client);
}


/*---------------FUNCTION TO FETCH DATA------------------*/
void fetch_data(){

    memset(receive,0,sizeof(receive));//cleans the buffer

    //http configuration
    esp_http_client_config_t config={
        .url=FETCH_URL,
        .method=HTTP_METHOD_GET,//method to retrive data
        .event_handler=http_event_handler,
        .crt_bundle_attach=esp_crt_bundle_attach,//certificate for verification
        .buffer_size=4096,
        .buffer_size_tx=2048,
        .timeout_ms=10000,
        .disable_auto_redirect=true,
    };

    esp_http_client_handle_t client=esp_http_client_init(&config);//initialize http
    esp_http_client_set_header(client,"Content-Type", "application/json");//set header(headers->additional info about browser or content)

    esp_http_client_set_header(client,"X-Authorization",jwt_token);//another header

    esp_http_client_perform(client);//main function which sends http request , headers , etc.

    int status=esp_http_client_get_status_code(client);//stores status response

    if(status==401){

        printf("JWT Token Expired!\n");
        esp_http_client_cleanup(client);
        login();
        return;
    }

    sscanf(receive,"{\"Channel_1\":[{\"ts\":%*[^,],\"value\":\"%f\"}],\"Channel_2\":[{\"ts\":%*[^,],\"value\":\"%f\"}],\"Channel_3\":[{\"ts\":%*[^,],\"value\":\"%f\"}],\"Channel_4\":[{\"ts\":%*[^,],\"value\":\"%f\"}],\"Channel_5\":[{\"ts\":%*[^,],\"value\":\"%f\"}],\"Channel_6\":[{\"ts\":%*[^,],\"value\":\"%f\"}]}",&data.ch1,&data.ch2,&data.ch3,&data.ch4,&data.ch5,&data.ch6);

    esp_http_client_cleanup(client);
}

/*-------------------------------------MQTT PUBLISH CODE---------------------------------------------*/

//MQTT Functions


void mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void mqtt_client_init(){

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI ,
        .credentials={
            .username= ACCESS_TOKEN
        },
    };

    mqtt_client=esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

}

void mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){

    if (event_id == MQTT_EVENT_CONNECTED){
       printf("MQTT Connected!\n");
       mqtt_connected=1;
    }else if(event_id == MQTT_EVENT_DISCONNECTED){
       printf("MQTT Disconnected\nReconnecting...\n");
       mqtt_connected=0;
       esp_mqtt_client_reconnect(mqtt_client);
    }
}

//function that publishes user data on thingsboard
void user_data_publish(struct user* root){
    
    while(!mqtt_connected){
        vTaskDelay(100/portTICK_PERIOD_MS);
    }

    if(root==NULL){
        printf("List is Empty!\n");
        return;
    }
    
    struct user* temp=root;

    char users[512]="{";
    int row=1;

    while(temp!=NULL){
        
        bool active=lv_table_has_cell_ctrl(ui_table,row,2,LV_TABLE_CELL_CTRL_CUSTOM_1);
        char data[128];

        sprintf(data,"\"%s\":\"%s#%d\",",temp->name,temp->pswd,active?1:0);
        strcat(users,data);
        temp=temp->next;
        row++;
    }
    
    int len=strlen(users);
    if(users[len-1]==','){
        users[len-1]='\0';
    }
    strcat(users,"}"); 
    esp_mqtt_client_publish(mqtt_client,"v1/devices/me/attributes",users,0,1,0);
    printf("published updated user data!\n");

}

void delete_users(struct user* root){
    
    if(root!=NULL){

        char url[512]="https://eu.thingsboard.cloud/api/plugins/telemetry/DEVICE/0d33bee0-4abf-11f1-bdfb-2fcbd7265065/SHARED_SCOPE?keys=";
        struct user* temp=root;

        while(temp!=NULL){
            
            char temp_buf[128];

            sprintf(temp_buf,"%s,",temp->name);
            strcat(url,temp_buf);
            temp=temp->next;
        }
        
        int len=strlen(url);
        if(url[len-1]==','){
            url[len-1]='\0';
        }

        esp_http_client_config_t config={
            .url=url,
            .method=HTTP_METHOD_DELETE,//method to retrive data
            .event_handler=http_event_handler,
            .crt_bundle_attach=esp_crt_bundle_attach,//certificate for verification
            .buffer_size=4096,
            .buffer_size_tx=2048,
            .timeout_ms=10000,
            .disable_auto_redirect=true,
        };

        esp_http_client_handle_t client=esp_http_client_init(&config);//initialize http
        esp_http_client_set_header(client,"X-Authorization",jwt_token);//another header
         
        esp_http_client_perform(client);//main function which sends http request , headers , etc.
        int status=esp_http_client_get_status_code(client);//stores status response

        if(status==401){

            printf("JWT Token Expired!\n");
            esp_http_client_cleanup(client);
            login();
            return;
        }
        esp_http_client_cleanup(client);
    }
}

//////////////////////////////////////////////////USED JENIL'S CODE LOGIC//////////////////////////////////////////////////////// 
void ui_update(lv_timer_t * timer){
    
    char buf[100];

    sprintf(buf,"%.2f", data.ch1);
    lv_label_set_text ( ui_TmprValue, buf);

    sprintf(buf,"%.2f", data.ch2);
    lv_label_set_text ( ui_hmdtValue1, buf);

    sprintf(buf,"%.2f", data.ch3);
    lv_label_set_text ( ui_hmdtValue2, buf);
    
    sprintf(buf,"%.2f", data.ch4);
    lv_label_set_text ( ui_prsValue1, buf);
    
    sprintf(buf,"%.2f", data.ch5);
    lv_label_set_text ( ui_prsValue2, buf);
    
    sprintf(buf,"%.2f", data.ch6);
    lv_label_set_text ( ui_prsValue3, buf);

}

void http_task(void *pvParam){
    
    while(!mqtt_connected){
        vTaskDelay(100/portTICK_PERIOD_MS);
    }

    login();

    while(1){
        fetch_data();
        vTaskDelay(3000/portTICK_PERIOD_MS);
    }

}

/* wifi task that calls wifi_init */
void wifi_task(void *pvParam) {
    wifi_init();
    vTaskDelay(2000/portTICK_PERIOD_MS);
    vTaskSuspend(NULL);
}

void app_main (void)
{
    //delete_NVS();

    nvs_flash_init();//prepares nvs storage

    esp_netif_init();//enables communication over internet
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();//creates network interface

    wifi_init();/*Added wifi function*/
    vTaskDelay(1000/portTICK_PERIOD_MS);

    display();
    create_table(ui_tablePanel);
    read_stored_blobs();
    


    //root = add_user(root,"user1","1234");
    //root = add_user(root,"user2","1424");
    update_table();

    /* 4096 stack size is necessary as it curropts some data related to wdt */
    //xTaskCreatePinnedToCore(display_task, "display_task", 4096, NULL,5, NULL,1);

    //xTaskCreate(wifi_task, "wifi_task", 4096, NULL,4, &gWifi_handle);
    xTaskCreate(http_task, "http_task", 8192, NULL,3, NULL);
    lv_timer_create(ui_update, 1000,  NULL);
}

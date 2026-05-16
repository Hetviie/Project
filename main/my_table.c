#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "ui/ui.h"
#include "my_table.h"
#include "nvs_rw.h"

uint8_t added_users = 0;

lv_obj_t * ui_table = NULL;

static void draw_event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);

    /*If the cells are drawn...*/
    if(dsc->part == LV_PART_ITEMS) {
        uint32_t row = dsc->id /  lv_table_get_col_cnt(obj);
        uint32_t col = dsc->id - row * lv_table_get_col_cnt(obj);
        bool chk = false;

        char c = *(lv_table_get_cell_value(obj, row, 0));
        if ( row != 0 && col == 2 && c != '\0') {
            chk = lv_table_has_cell_ctrl(obj, row, 2, LV_TABLE_CELL_CTRL_CUSTOM_1);
            lv_draw_rect_dsc_t rect_dsc;
            lv_draw_rect_dsc_init(&rect_dsc);
            rect_dsc.bg_color = chk ? lv_theme_get_color_primary(obj) : lv_palette_lighten(LV_PALETTE_GREY, 2);
            rect_dsc.radius = LV_RADIUS_CIRCLE;

            lv_area_t sw_area;
            sw_area.x1 = dsc->draw_area->x2 - 50;
            sw_area.x2 = sw_area.x1 + 40;
            sw_area.y1 = dsc->draw_area->y1 + lv_area_get_height(dsc->draw_area) / 2 - 10;
            sw_area.y2 = sw_area.y1 + 20;
            lv_draw_rect(dsc->draw_ctx, &rect_dsc, &sw_area);

            rect_dsc.bg_color = lv_color_white();
            if(chk) {
                sw_area.x2 -= 2;
                sw_area.x1 = sw_area.x2 - 16;
            }
            else {
                sw_area.x1 += 2;
                sw_area.x2 = sw_area.x1 + 16;
            }
            sw_area.y1 += 2;
            sw_area.y2 -= 2;
            lv_draw_rect(dsc->draw_ctx, &rect_dsc, &sw_area);
        }

        /*Make the texts in the first cell center aligned*/
        if(row == 0) {
            dsc->label_dsc->align = LV_TEXT_ALIGN_CENTER;
            dsc->rect_dsc->bg_color = lv_color_mix(lv_palette_main(LV_PALETTE_BLUE), dsc->rect_dsc->bg_color, LV_OPA_20);
            dsc->rect_dsc->bg_opa = LV_OPA_COVER;
        }
        /*In the first column align the texts to the right*/
        else if(col == 0) {
            dsc->label_dsc->align = LV_TEXT_ALIGN_RIGHT;
        }

        /*MAke every 2nd row grayish*/
        if((row != 0 && row % 2) == 0) {
            dsc->rect_dsc->bg_color = lv_color_mix(lv_palette_main(LV_PALETTE_GREY), dsc->rect_dsc->bg_color, LV_OPA_10);
            dsc->rect_dsc->bg_opa = LV_OPA_COVER;
        }
    }
}

static void change_event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    uint16_t col = 0;
    uint16_t row = 0;
    lv_table_get_selected_cell(obj, &row, &col);
    bool chk = lv_table_has_cell_ctrl(obj, row, 2, LV_TABLE_CELL_CTRL_CUSTOM_1);
    if ( col == 2 ) {
        if (chk) {
            lv_table_clear_cell_ctrl(obj, row, 2, LV_TABLE_CELL_CTRL_CUSTOM_1);
        }
        else {
            lv_table_add_cell_ctrl(obj, row, 2, LV_TABLE_CELL_CTRL_CUSTOM_1);
        }
        user_data_publish(root);
        save_user_data(root);
    }
}

void update_table (void) {

    /* check condition for empty user list */
    if ( root == NULL ) {
        return;
    }

    struct user* temp = root;
    uint8_t node_index = 0, row = 1;
    char c = *(lv_table_get_cell_value(ui_table, row, 0));

    /* check for empty row */
    while ( c != '\0' ) {
        row++;
        node_index++;
        temp = temp->next;
        c = *(lv_table_get_cell_value(ui_table, row, 0));
    }
    while ( 1 ) {
        
        lv_table_set_cell_value(ui_table, row, 0, temp->name);
        lv_table_set_cell_value(ui_table, row, 1, temp->pswd);

        node_index++;
        row++;
        user_data_publish(root);  

        /* save node into NVS */
        printf("+++++********calling save_user_data\n");
        save_user_data(temp);
        added_users++;

        //temp = temp->next;
        if ( temp->next != NULL ) {
            temp = temp->next;
        }
        else {
            break;
        }
    }
}

/* function to clear all table cell */
void clr_table(lv_obj_t *obj) {
    uint8_t row = 1;
    char c = 0;
    
    while ( (c = *(lv_table_get_cell_value(obj, row, 0))) != '\0' ) {
        lv_table_set_cell_value(obj, row, 0, "");
        lv_table_set_cell_value(obj, row, 1, "");
        lv_table_set_cell_value(obj, row, 2, "");
        row++;
    }
    delete_NVS();
    added_users = 0;
}

void create_table(lv_obj_t *obj)
{
    ui_table = lv_table_create(obj);

    /*Set a smaller height to the table. It'll make it scrollable*/
    lv_obj_set_size(ui_table, LV_SIZE_CONTENT, 200);

    lv_table_set_col_width(ui_table, 0, 150);
    lv_table_set_row_cnt(ui_table, ITEM_CNT); /*Not required but avoids a lot of memory reallocation lv_table_set_set_value*/
    lv_table_set_col_cnt(ui_table, 3);

    /*Don't make the cell pressed, we will draw something different in the event*/
    lv_obj_remove_style(ui_table, NULL, LV_PART_ITEMS | LV_STATE_PRESSED);

    lv_table_set_cell_value(ui_table, 0, 0, "Username");

    lv_table_set_cell_value(ui_table, 0, 1, "Password");

    lv_table_set_cell_value(ui_table, 0, 2, "Permission");

    lv_obj_align(ui_table, LV_ALIGN_CENTER, 0, -20);

    /*Add an event callback to to apply some custom drawing*/
    lv_obj_add_event_cb(ui_table, draw_event_cb, LV_EVENT_DRAW_PART_END, NULL);
    lv_obj_add_event_cb(ui_table, change_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
}

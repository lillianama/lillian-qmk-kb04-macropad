#pragma once

#include <math.h>
#include <stdio.h>
#include "debug.h"
#include "lib/lib8tion/lib8tion.h"
#include "oled_driver.h"
#include "timer.h"
#include "wpm.h"

#define LILLI_WPM_LEVEL_TIMER 100
#define LILLI_WPM_LEVEL_BOX_HEIGHT 50
#define LILLI_WPM_LEVEL_BOX_WIDTH 32
#define LILLI_WPM_LEVEL_BOX_X 0
#define LILLI_WPM_LEVEL_BOX_Y 0
#define LILLI_WPM_LEVEL_BOX_BORDER 1
#define LILLI_WPM_LEVEL_BOX_SPACER 1
#define LILLI_WPM_LEVEL_MAX 110

#define LILLI_WPM_GRAPH_TIMER 250
#define LILLI_WPM_GRAPH_BOX_HEIGHT 50
#define LILLI_WPM_GRAPH_BOX_WIDTH 32
#define LILLI_WPM_GRAPH_BOX_X 0
#define LILLI_WPM_GRAPH_BOX_Y 53
#define LILLI_WPM_GRAPH_BOX_BORDER 1
#define LILLI_WPM_GRAPH_BOX_SPACER 1
#define LILLI_WPM_GRAPH_MAX 110
#define LILLI_WPM_GRAPH_AREA_WIDTH LILLI_WPM_GRAPH_BOX_WIDTH - (LILLI_WPM_GRAPH_BOX_BORDER * 2) - (LILLI_WPM_GRAPH_BOX_SPACER * 2)

struct wpm_buffer {
    uint8_t scaled[LILLI_WPM_GRAPH_AREA_WIDTH];
    int list[LILLI_WPM_GRAPH_AREA_WIDTH];
};
static struct wpm_buffer wpms;
struct lilli_shape {
    int x, y, w, h;
    bool write_off_pixel;
    uint8_t wpm;
    int scaled_wpm;
};

static void lilli_draw(struct lilli_shape shape, bool (*callback)(int x, int y, struct lilli_shape shape)) {
    bool pixel_on;
    for (int x = shape.x; x < shape.x + shape.w; x++) {
        pixel_on = false;
        for (int y = shape.y; y < shape.y + shape.h; y++) {
            pixel_on = (*callback)(x, y, shape);
            if ((shape.write_off_pixel && !pixel_on) || pixel_on)
                oled_write_pixel(x, y, pixel_on);
        }
    }
}

static bool lilli_get_wpm_level_border_pixel(int x, int y, struct lilli_shape shape) {
    if (x == LILLI_WPM_LEVEL_BOX_X || x == LILLI_WPM_LEVEL_BOX_X + LILLI_WPM_LEVEL_BOX_WIDTH - 1)
        return true;
    else if (y == LILLI_WPM_LEVEL_BOX_Y || y == LILLI_WPM_LEVEL_BOX_Y + LILLI_WPM_LEVEL_BOX_HEIGHT - 1)
        return true;
    else
        return false;
}

static bool lilli_get_wpm_graph_border_pixel(int x, int y, struct lilli_shape shape) {
    if (x == LILLI_WPM_GRAPH_BOX_X || x == LILLI_WPM_GRAPH_BOX_X + LILLI_WPM_GRAPH_BOX_WIDTH - 1)
        return true;
    else if (y == LILLI_WPM_GRAPH_BOX_Y || y == LILLI_WPM_GRAPH_BOX_Y + LILLI_WPM_GRAPH_BOX_HEIGHT - 1)
        return true;
    else
        return false;
}

static bool lilli_get_wpm_level_pixel(int x, int y, struct lilli_shape shape) {
    if (y >= shape.scaled_wpm && y % 2 == 0 && x % 2 == 0) return true;
    else return false;
}

static bool lilli_get_wpm_graph_pixel(int x, int y, struct lilli_shape shape) {
    if (y >= wpms.scaled[x-2] && (y % 2 == 0)) return true;
    else return false;
}

static void lilli_render_wpm_level(void) {
    static bool initializing = true;
    static uint32_t timer = 0;
    static uint8_t lwpm = 0;

    if (timer_elapsed32(timer) < LILLI_WPM_LEVEL_TIMER) {
        return;
    } else {
        timer = timer_read32();
        dprint("lilli_render_wpm_level() hi\n");
    }

    if (initializing) {
        initializing = false; //don't try this again :)
        dprintf("lilli_render_wpm_level() initializing\n");

        //draw the border
        struct lilli_shape border_shape = {
            LILLI_WPM_LEVEL_BOX_X, LILLI_WPM_LEVEL_BOX_Y, LILLI_WPM_LEVEL_BOX_WIDTH, LILLI_WPM_LEVEL_BOX_HEIGHT,
            false
        };
        lilli_draw(border_shape, &lilli_get_wpm_level_border_pixel);

        //set inital wpm text
        oled_set_cursor(1, 1);
        oled_write("---", false);
    }

    int wpm = get_current_wpm();

    //stop here if the wpm hasn't changed since last run
    if (wpm == lwpm) return; else lwpm = wpm;

    //draw the graph
    struct lilli_shape level_shape = {
        LILLI_WPM_LEVEL_BOX_X + 2,
        LILLI_WPM_LEVEL_BOX_Y + 2,
        LILLI_WPM_LEVEL_BOX_WIDTH - 4,
        LILLI_WPM_LEVEL_BOX_HEIGHT - 4,
        true,
        wpm
    };

    //scale the wpm value in proportion to a desired maximum wpm and the height of the graph area
    const int y_gt = LILLI_WPM_LEVEL_BOX_Y + 2;
    const int y_gb = LILLI_WPM_LEVEL_BOX_Y + LILLI_WPM_LEVEL_BOX_HEIGHT - 2;
    const int gh = y_gb - y_gt;
    const int s = (wpm * gh) / LILLI_WPM_LEVEL_MAX;
    level_shape.scaled_wpm = gh - s + y_gt;
    lilli_draw(level_shape, &lilli_get_wpm_level_pixel);

    //write the wpm text
    oled_set_cursor(1, 1);
    char wpm_str[] = "---";
    sprintf(wpm_str, "%03u", wpm);
    oled_write(wpm_str, false);
}

static void lilli_render_wpm_graph(void) {
    static bool initializing = true;
    static uint32_t timer = 0;

    if (timer_elapsed32(timer) < LILLI_WPM_GRAPH_TIMER) {
        return;
    } else {
        timer = timer_read32();
        dprint("lilli_render_wpm_graph() hi\n");
    }

    if (initializing) {
        initializing = false; //don't try this again :)
        dprintf("lilli_render_wpm_graph() initializing\n");

        //init the arrays
        for (int i = 0; i < LILLI_WPM_GRAPH_AREA_WIDTH; i++)
            wpms.scaled[i] = LILLI_WPM_GRAPH_BOX_Y + LILLI_WPM_GRAPH_BOX_HEIGHT - 2;
        for (int i = 0; i < LILLI_WPM_GRAPH_AREA_WIDTH; i++)
            wpms.list[i] = 0;

        //draw the graph border
        struct lilli_shape border_shape = {
            LILLI_WPM_GRAPH_BOX_X, LILLI_WPM_GRAPH_BOX_Y,
            LILLI_WPM_GRAPH_BOX_WIDTH, LILLI_WPM_GRAPH_BOX_HEIGHT,
            false
        };
        lilli_draw(border_shape, &lilli_get_wpm_graph_border_pixel);
    }

    int wpm = get_current_wpm();

    //scale the wpm value in proportion to a desired maximum wpm and the height of the graph area
    const int y_gt = LILLI_WPM_GRAPH_BOX_Y + 2;
    const int y_gb = LILLI_WPM_GRAPH_BOX_Y + LILLI_WPM_GRAPH_BOX_HEIGHT - 2;
    const int gh = y_gb - y_gt;
    const int s = floor((wpm * gh) / LILLI_WPM_GRAPH_MAX);
    const uint8_t graph_y = gh - s + y_gt;

    //shift the arrays
    memmove8(&wpms.scaled[0], &wpms.scaled[1], sizeof(wpms.scaled[0]) * (LILLI_WPM_GRAPH_AREA_WIDTH - 1));
    memmove8(&wpms.list[0], &wpms.list[1], sizeof(wpms.list[0]) * (LILLI_WPM_GRAPH_AREA_WIDTH - 1));

    //push latest values
    wpms.scaled[LILLI_WPM_GRAPH_AREA_WIDTH - 1] = graph_y;
    wpms.list[LILLI_WPM_GRAPH_AREA_WIDTH - 1] = wpm;

    //draw the graph
    struct lilli_shape graph_shape = {
        LILLI_WPM_GRAPH_BOX_X + 2,
        LILLI_WPM_GRAPH_BOX_Y + 2,
        LILLI_WPM_GRAPH_BOX_WIDTH - 4,
        LILLI_WPM_GRAPH_BOX_HEIGHT - 4,
        true,
        wpm
    };
    lilli_draw(graph_shape, &lilli_get_wpm_graph_pixel);

    //draw the avg
    int sum = 0, avg = 0;
    for (int i = 0; i < LILLI_WPM_GRAPH_AREA_WIDTH; i++)
        sum += wpms.list[i];
    avg = sum / (LILLI_WPM_GRAPH_AREA_WIDTH + 1);

    oled_set_cursor(1, 13);
    oled_write("avg", false);
    char wpm_str[8];
    sprintf(wpm_str, "%03u", avg);
    oled_set_cursor(1, 14);
    oled_write(wpm_str, false);
}

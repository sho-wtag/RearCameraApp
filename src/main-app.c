/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "main-app.h"

#include "utils/logger.h"
#include "utils/config.h"
#include "utils/ui-utils.h"

#include "view/main-view.h"
#include "view/window.h"

#include <app.h>
#include <efl_extension.h>
#include <Elementary.h>

struct _app_data
{
    window *win;
    Evas_Object *navi;
};

static Evas_Object *_app_navi_add(app_data *app);
static bool _app_create_cb(void *user_data);
static void _app_terminate_cb(void *user_data);
static void _app_navi_back_cb(void *data, Evas_Object *obj, void *event_info);
static void _app_resume_cb(void *user_data);
static void _app_pause_cb(void *user_data);

static app_data *global_ad = NULL;

app_data* app_main_appdata_get()
{
	if (global_ad)
		return global_ad;
	return NULL;
}

app_data *app_create()
{
    return calloc(1, sizeof(app_data));
}

void app_destroy(app_data *app)
{
    free(app);
}

int app_run(app_data *app, int argc, char **argv)
{
    RETVM_IF(!app, -1, "app is NULL");
    ui_app_lifecycle_callback_s cbs =
    {
        .create = _app_create_cb,
        .terminate = _app_terminate_cb,
        .pause = _app_pause_cb,
        .resume = _app_resume_cb,
        .app_control = NULL
    };

    return ui_app_main(argc, argv, &cbs, app);
}

static Evas_Object *_app_navi_add(app_data *app)
{
    RETVM_IF(!app, NULL, "app is NULL");
    Evas_Object *navi = ui_utils_navi_add(window_layout_get(app->win));
   eext_object_event_callback_add(navi, EEXT_CALLBACK_BACK, _app_navi_back_cb, app);
    window_content_set(app->win, navi);
    return navi;
}

static void _app_resume_cb(void *user_data)
{
    RETM_IF(!user_data, "user_data is null");
    app_data *app = user_data;
    Elm_Object_Item *top = elm_naviframe_top_item_get(app->navi);
    Evas_Object *layout = elm_object_item_part_content_get(top, "elm.swallow.content");
    if(layout)
    {
        evas_object_smart_callback_call(layout, EVENT_RESUME, NULL);
    }
}

static void _app_pause_cb(void *user_data)
{
    RETM_IF(!user_data, "user_data is null");
    app_data *app = user_data;
    Elm_Object_Item *top = elm_naviframe_top_item_get(app->navi);
    Evas_Object *layout = elm_object_item_part_content_get(top, "elm.swallow.content");
    if(layout)
    {
        evas_object_smart_callback_call(layout, EVENT_PAUSE, NULL);
    }
}

static bool _app_create_cb(void *user_data)
{
    RETVM_IF(!user_data, false, "user_data is NULL");
    app_data *app = user_data;

    // List View Theme Loading
    Elm_Theme *menu_list_theme = NULL;
    elm_theme_extension_add(menu_list_theme, get_resource_path(SELF_CAMERA_LAYOUT));

    app->win = window_create();
    RETVM_IF(!app->win, false, "window_create() failed");

    app->navi = _app_navi_add(app);
    if(!app->navi)
    {
        ERR("_app_navi_add() failed");
        window_destroy(app->win);
        return false;
    }

    Evas_Object *view = main_view_add(app->navi);
    if(!view)
    {
        ERR("sensor_list_view_create() failed");
    }

    global_ad = app;

    return true;
}

static void _app_terminate_cb(void *user_data)
{
    RETM_IF(!user_data, "user_data is NULL");
    app_data *app = user_data;

    if(app->win)
    {
        window_destroy(app->win);
    }
}

static void _app_navi_back_cb(void *data, Evas_Object *obj, void *event_info)
{
    RETM_IF(!data, "data is NULL");
    app_data *app = data;

    Elm_Object_Item *top = elm_naviframe_top_item_get(app->navi);

    if(top == elm_naviframe_bottom_item_get(app->navi))
    {
        window_lower(app->win);
    }
    else
    {
        elm_naviframe_item_pop(app->navi);
    }
}

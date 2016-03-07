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

#include "view/main-view.h"
#include "view/window.h"

#include "utils/config.h"
#include "utils/logger.h"
#include "utils/ui-utils.h"

#include <camera.h>
#include <dirent.h>
#include <efl_extension.h>
#include <Elementary.h>
#include <wav_player.h>
#include <sensor.h>
#include <main-app.h>
#include <dlog.h>
#include <tizen.h>
#include <curl/curl.h>
#include <string.h>
#include "jsonparser.h"
#include <json-glib.h>
#include <stdio.h>
#include <pthread.h>
#include <locations.h>

struct TotalArrayStruct Array;

#define COUNTER_STR_LEN 3
#define FILE_PREFIX "IMAGE"

#define NUM_OF_ITEMS 22
#define ICON_DIR "/opt/usr/apps/org.tizen.rearcameraapp/res/icon"

static const char *_error = "Error";
static const char *_camera_init_failed = "Camera initialization failed.";
static const char *_app_init_failed = "Image viewing application initialization failed.";
static const char *_app_not_found = "Image viewing application wasn't found.";
static const char *_ok = "OK";
static const char _file_prot_str[]= "file://";

int num_navi_item = 1;

sensor_listener_h magneticListener;

char str[100];
float AccelerationX, AccelerationY, AccelerationZ;
float GyroscopeX, GyroscopeY, GyroscopeZ;
float MagneticX, MagneticY, MagneticZ;
int OriRoll, OriYaw, OriPitch;

float x, y, z;
float current_x[100] = {70, 220, 285, 80, 390, 140, 330, 300, 490, 520};
float current_y[100] = {90, 620, 80, 330, 740, 570, 100, 480, 600, 500};
//Evas_Smart *marker_slw;

char jsonURL[250];
double mylatitude=0;
double mylongitude=0;
int radius=500;

int angleFromAbsoluteNorth;
int minAngle, maxAngle;

//restaurant List arr
int flag_name=0, flag_address=0, flag_lat=0, flag_lng=0, flag_rating=0;
int idx_name=0, idx_address=0, idx_lat=0, idx_lng=0;
char name[100][80], address[100][80];
double lat[100], lng[100];
int rating[100], angle[100], imaginaryAngle[100], distance[100];
//FOV List arr
char FOVname[NUM_RESULTS_IN_FOV+1][80], FOVaddress[NUM_RESULTS_IN_FOV+1][80];
int FOVx[NUM_RESULTS_IN_FOV+1], FOVy[NUM_RESULTS_IN_FOV+1], FOVdistance[NUM_RESULTS_IN_FOV+1];
int FOVnumResults=0;

typedef struct
{
    Evas_Object *navi;
    Elm_Object_Item *navi_item;
    Evas_Object *layout;
    Evas_Object *list_layout;
    Evas_Object *location_list;
    Evas_Object *marker_layout;
    Evas_Object *popup;
    Evas_Object *preview_canvas;

    Evas_Object *geo_marker[100];

    camera_h camera;
    Eina_Bool camera_enabled;

    Ecore_Timer *timer;
    int timer_count;
    int selected_timer_interval;
    location_manager_h l_manager;
} main_view;

typedef struct item_data
{
	int index;
	Elm_Object_Item *item;
} item_data_s;

main_view *global_view;



location_list loc_list_val[10] = {
		{
			"Royel Buffet", "Dhanmondi-15", "4.5"
		},
		{
			"Cookers Inn", "Kazi Nazrul Islam Evenue", "4.2"
		},
		{
			"Pan Pacific Sonargaon", "Kawranbazar", "5.0"
		},
		{
			"KFC", "Dhanmondi-27", "4.8"
		},
		{
			"Regency Hotel", "Airport Road", "5.0"
		},
		{
			"BFC", "Dhanmondi-2", "4.5"
		},
		{
			"Flambe Restaurant", "Banani", "4.8"
		},
		{
			"WFC", "Kathalbagan", "3.9"
		},
		{
			"Seven Hill Restaurant", "Hatirpool", "4.7"
		},
		{
			"Golden Chimney Restaurant", "Hatirpool", "4.6"
		}
	};


static void _main_view_destroy(void *data, Evas *e, Evas_Object *obj, void *event_info);
static Eina_Bool _main_view_init_camera(main_view *view);
static void _main_view_register_cbs(main_view *view);

static void _main_view_back_cb(void *data, Evas_Object *obj, void *event_info);
static void _main_view_pause_cb(void *data, Evas_Object *obj, void *event_info);
static void _main_view_resume_cb(void *data, Evas_Object *obj, void *event_info);

static Eina_Bool _main_view_start_camera_preview (camera_h camera);
static Eina_Bool _main_view_stop_camera_preview (camera_h camera);

static void _main_view_marker_clicked_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source);

static void _ar_view_selected_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source);
static void _list_view_selected_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source);

static void _main_view_show_warning_popup(Evas_Object *navi, const char *caption, const char *text, const char *button_text, void *data);
static void _main_view_popup_close_cb(void *data, Evas_Object *obj, void *event_info);


///////////////////// start GPS //////////////////////
static void position_updated(double latitude, double longitude, double altitude,
        time_t timestamp, void *user_data) {

	main_view *view = (main_view*)user_data;

    double climb, direction, speed;
    double horizontal, vertical;
    location_accuracy_level_e level;

    //mylatitude=latitude;
    //mylongitude=longitude;

    int ret = location_manager_get_location(view->l_manager, &altitude, &latitude,
            &longitude, &climb, &direction, &speed, &level, &horizontal,
            &vertical, &timestamp);

    mylatitude=latitude;
    mylongitude=longitude;

    sprintf(jsonURL,"https://maps.googleapis.com/maps/api/place/nearbysearch/json?types=restaurant&radius=%d&location=%lf,%lf&sensor=true&key=AIzaSyDHR9lSsc7jB8VtYuj6NuaK6QcspfFUnZQ",radius, latitude, longitude);

    dlog_print(DLOG_ERROR, "USR_TAG", "position_updated %s", jsonURL);
}

static void start_location_manager(main_view *view) {
    int ret = location_manager_create(LOCATIONS_METHOD_HYBRID, &view->l_manager);
    location_manager_set_position_updated_cb(view->l_manager, position_updated, 2, (void*)view);
    location_manager_start(view->l_manager);
}


static int
is_change_position_cb(double latitude, double longitude)
{
	if(mylatitude != latitude || mylongitude != longitude)
		return 1;
	else
		return 0;
}
/////////////////////////////////////////////////////////

///////////////////////////////////KHAIRUL
double degree2radian(double deg)
{
  return (deg * pi / 180);
}

double radian2degree(double rad)
{
  return (rad * 180 / pi);
}

double AngleFromCoordinates(double lat1, double lng1, double lat2, double lng2)
{
  double dlng=(lng2-lng1);
  double x,y,brng;
  y=sin(dlng)*cos(lat2);
  x=(cos(lat1)*sin(lat2))-(sin(lat1)*cos(lat2)*cos(dlng));
  brng=atan2(y,x);
  brng=radian2degree(brng);
  brng=(((int)brng)+360)%360;
  brng=360-brng;
  return brng;
}

double DistanceFromCoordinates(double lat1, double lon1, double lat2, double lon2, char unit)
{
    double rlat1 = (pi*lat1)/180;
    double rlat2 = (pi*lat2)/180;
    double theta = lon1 - lon2;
    double rtheta = pi*theta/180;
    double dist = sin(rlat1) * sin(rlat2) + cos(rlat1) * cos(rlat2) * cos(rtheta);
    dist = acos(dist);
    dist = dist*180/pi;
    dist = dist*60*1.1515;

    switch (unit)
    {
        case 'K': //Kilometers -> default
            return dist * 1.609344;
        case 'N': //Nautical Miles
            return dist * 0.8684;
        case 'M': //Miles
            return dist;
    }
    return dist;
}

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  if ((in_max - in_min) > (out_max - out_min)) {
    return (x - in_min) * (out_max - out_min+1) / (in_max - in_min+1) + out_min;
  }
  else
  {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }
}

static void CalculateData()
{
	int i;
	  for(i=0;i<=Array.iLat;i++){
		  //to calculate distance of the resturants
		Array.resturants[i].distance=DistanceFromCoordinates(mylatitude, mylongitude, Array.resturants[i].lat, Array.resturants[i].lng, 'K')*1000;
		//to calculate real angle of the resturants
		Array.resturants[i].angle=AngleFromCoordinates(mylatitude,mylongitude, Array.resturants[i].lat, Array.resturants[i].lng);
		if (Array.resturants[i].angle < CAMERA_FOV){//angle<FOV
			Array.resturants[i].imaginaryAngle=360+Array.resturants[i].angle;
		}
		else if (Array.resturants[i].angle>(360-CAMERA_FOV)){//angle>360-FOV
			Array.resturants[i].imaginaryAngle=Array.resturants[i].angle-360;
		}
		else{
			Array.resturants[i].imaginaryAngle=1000; //it will be outside from our search
		}
		printf("<%d\t%d>\t\t\t%s\n", Array.resturants[i].angle, Array.resturants[i].imaginaryAngle, Array.resturants[i].name);
	  }
}

static void RefreshFOV(int TheAngle)
{
	  //to calculate imaginary angle of the resturants
	  Array.angleFromAbsoluteNorth=TheAngle;
	  //initialize variables
	  int minAngle, maxAngle, FOVidx=0;
	  minAngle=Array.angleFromAbsoluteNorth - (CAMERA_FOV/2);
	  maxAngle=Array.angleFromAbsoluteNorth + (CAMERA_FOV/2);
	  //printf("\n\nangleFromAbsoluteNorth:%d\tminAngle:%d\tmaxAngle:%d\nInside FOV:\nX\tY\tName\n",Array.angleFromAbsoluteNorth,minAngle,maxAngle);
	  //fill up our list with data inside fov
	  int i;
	  for(i=0;i<=Array.iLat;i++){
	    if (FOVidx<(NUM_RESULTS_IN_FOV)){//don't add more results than max limit
	      if ((Array.resturants[i].angle > minAngle) && (Array.resturants[i].angle < maxAngle) ){//real angle is  inside fov
	        Array.FOV[FOVidx].name=Array.resturants[i].name;
	        Array.FOV[FOVidx].address=Array.resturants[i].address;
	        Array.FOV[FOVidx].x=map(Array.resturants[i].angle, minAngle, maxAngle, WINDOW_Xmin, WINDOW_Xmax);
	        Array.FOV[FOVidx].y=map(Array.resturants[i].distance, 0, radius, WINDOW_Ymin, WINDOW_Ymax);
	        FOVidx++;
	      }
	      else if ((Array.resturants[i].imaginaryAngle > minAngle) && (Array.resturants[i].imaginaryAngle < maxAngle) ){//imaginary angle is  inside fov
	        Array.FOV[FOVidx].name=Array.resturants[i].name;
	        Array.FOV[FOVidx].address=Array.resturants[i].address;
	        Array.FOV[FOVidx].x=map(Array.resturants[i].imaginaryAngle,minAngle, maxAngle, WINDOW_Xmin, WINDOW_Xmax);
	        Array.FOV[FOVidx].y=map(Array.resturants[i].distance, 0, radius, WINDOW_Ymin, WINDOW_Ymax);
	        FOVidx++;
	      }
	    }
	  }
	  //Array.resultsInFOV=FOVidx;
		//int i;
		for (i = 0; i < NUM_RESULTS_IN_FOV; i++) {
			if (i<FOVidx){
			//current_x[i] = current_x[i] - (i%4?i+1:i-3)*(20 * (MagneticZ - z));
			evas_object_move(global_view->geo_marker[i], Array.FOV[i].x, i*70);
			//evas_object_move(global_view->geo_marker[i], current_x[i], current_y[i]);

			elm_object_part_text_set(global_view->geo_marker[i], "marker_text", Array.resturants[i].name);
			evas_object_show(global_view->geo_marker[i]);
			}
			else
			{
				//current_x[i] = current_x[i] - (i%4?i+1:i-3)*(20 * (MagneticZ - z));
				evas_object_move(global_view->geo_marker[i], 1500, 1500);
				//evas_object_move(global_view->geo_marker[i], current_x[i], current_y[i]);

				elm_object_part_text_set(global_view->geo_marker[i], "marker_text", Array.resturants[i].name);
				evas_object_show(global_view->geo_marker[i]);
			}
		}

}

//////////////////////////////////////////





static Evas_Object*
create_icon(Evas_Object *parent)
{
	char buf[255];
	Evas_Object *img = elm_image_add(parent);

	snprintf(buf, sizeof(buf), "%s/marker.png", ICON_DIR);
	elm_image_file_set(img, buf, NULL);
	evas_object_color_set(img, 110, 162, 185, 255);
	evas_object_size_hint_align_set(img, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	return img;
}

static void
gl_del_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
	/* FIXME: Unrealized callback can be called after this. */
	/* Accessing item_data_s can be dangerous on unrealized callback. */
	item_data_s *id = data;
	free(id);
}

static char *
gl_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	item_data_s *id = data;
	const Elm_Genlist_Item_Class *itc = elm_genlist_item_item_class_get(id->item);
	char buf[1024];

	dlog_print(DLOG_ERROR, "RearCameraApp", "gl_text_get_cb");
	if (itc->item_style && !strcmp(itc->item_style, "genlist/1con.2title")) {
		dlog_print(DLOG_ERROR, "RearCameraApp", "genlist/1con.2title");
		if (part && !strcmp(part, "title")) {
			snprintf(buf, 1023, "%s", Array.resturants[id->index].name);
			dlog_print(DLOG_ERROR, "RearCameraApp", buf);
			return strdup(buf);
		}
		if (part && !strcmp(part, "subtitle")) {
			snprintf(buf, 1023, "%s", Array.resturants[id->index].address);
			dlog_print(DLOG_ERROR, "RearCameraApp", buf);
			return strdup(buf);
		}
		if (part && !strcmp(part, "rating")) {
			if(Array.resturants[id->index].rating==0.00){
				snprintf(buf, 1023, "Rating: %s", "Not Available");
			}
			else{
				snprintf(buf, 1023, "Rating: %f", Array.resturants[id->index].rating);
			}
			dlog_print(DLOG_ERROR, "RearCameraApp", buf);
			return strdup(buf);
		}
	}

	return NULL;
}

static Evas_Object*
gl_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	item_data_s *id = data;
	const Elm_Genlist_Item_Class *itc = elm_genlist_item_item_class_get(id->item);
	Evas_Object *content = NULL;

	if (itc->item_style && !strcmp(itc->item_style, "genlist/1con.2title")) {
		content = elm_layout_add(obj);
		elm_layout_theme_set(content, "layout", "list/B/type.3", "default");
		Evas_Object *icon = create_icon(content);
		elm_layout_content_set(content, "elm.swallow.content", icon);
	}

	return content;
}

static void
gl_loaded_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
}

static void
gl_realized_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
	/* If you need, you can get item's data. */
	// Elm_Object_Item *it = event_info;
	// item_data_s *id = elm_object_item_data_get(it);
}

static void
gl_selected_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
	Evas_Object *nf = (Evas_Object *)data, *label;
	Elm_Object_Item *it = event_info;
	item_data_s *id = elm_object_item_data_get(it);

	/* Unhighlight item */
	elm_genlist_item_selected_set(id->item, EINA_FALSE);

	/* Write code for item selection*/
}

static Evas_Object *
create_genlist(Evas_Object *parent)
{
	Evas_Object *genlist;
	Elm_Object_Item *it;
	int n_items = Array.iName; // count from the list and change it
	int index;

	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
	itc->item_style = "genlist/1con.2title";
	itc->func.text_get = gl_text_get_cb;
	itc->func.content_get = gl_content_get_cb;
	itc->func.del = gl_del_cb;

	genlist = elm_genlist_add(parent);
	elm_scroller_single_direction_set(genlist, ELM_SCROLLER_SINGLE_DIRECTION_HARD);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);


	for (index = 0; index < n_items; index++) {

		item_data_s *id = calloc(sizeof(item_data_s), 1);

		id->index = index;

		it = elm_genlist_item_append(genlist,							/* genlist object */
										itc,							/* item class */
										id,								/* item class user data */
										NULL,							/* parent item */
										ELM_GENLIST_ITEM_NONE,			/* item type */
										gl_selected_cb,					/* select smart callback */
										parent);						/* smart callback user data */
		id->item = it;
	}
	elm_genlist_item_class_free(itc);
	evas_object_show(genlist);

	return genlist;
}






static void orientation_cb(sensor_h sensor, sensor_event_s *event, void *data)
{
	OriYaw=(int)event->values[0];
	OriRoll=(int)event->values[1];
	OriPitch=(int)event->values[2];
/*
	float tmpMagneticZ=0;
		int thAngle;
		if (MagneticZ > 35){
			tmpMagneticZ=35;
		}
		else if (MagneticZ < -15){
			tmpMagneticZ=-15;
		}
		else {
			tmpMagneticZ = MagneticZ;
		}*/
		 //thAngle=map((tmpMagneticZ+15), 0, (35+15), 360, 0);
		 //minAngle=angleFromAbsoluteNorth - (CAMERA_FOV/2);
		 //maxAngle=angleFromAbsoluteNorth + (CAMERA_FOV/2);
		 //if (thAngle<(Array.oldAngle-5) || thAngle<(Array.oldAngle-5)){
		 RefreshFOV(OriYaw);
}

static int register_orientation_callback(app_data *ad)
{
	int error;
	bool supported;
	sensor_h orientation;
	sensor_listener_h orientationListener;
	error = sensor_is_supported( SENSOR_ORIENTATION, &supported );
	if(error != SENSOR_ERROR_NONE && supported){
		return error;
	}
	error = sensor_get_default_sensor(SENSOR_ORIENTATION, &orientation);
	if(error != SENSOR_ERROR_NONE){
		return error;
	}
	error = sensor_create_listener( orientation, &orientationListener);
	if(error != SENSOR_ERROR_NONE){
		return error;
	}
	error = sensor_listener_set_event_cb( orientation, GYROSCOPE_INTERVAL_MS, orientation_cb, ad);
	if(error != SENSOR_ERROR_NONE) {
		return error;
	}
	error = sensor_listener_start( orientationListener );
	return SENSOR_ERROR_NONE;
}



static void gyroscope_cb(sensor_h sensor, sensor_event_s *event, void *data)
{
	GyroscopeX=(float)event->values[0];
	GyroscopeY=(float)event->values[1];
	GyroscopeZ=(float)event->values[2];


}

static int register_gyroscope_callback(app_data *ad)
{
	int error;
	bool supported;
	sensor_h gyroscope;
	sensor_listener_h gyroscopeListener;
	error = sensor_is_supported( SENSOR_GYROSCOPE, &supported );
	if(error != SENSOR_ERROR_NONE && supported){
		return error;
	}
	error = sensor_get_default_sensor(SENSOR_GYROSCOPE, &gyroscope);
	if(error != SENSOR_ERROR_NONE){
		return error;
	}
	error = sensor_create_listener( gyroscope, &gyroscopeListener);
	if(error != SENSOR_ERROR_NONE){
		return error;
	}
	error = sensor_listener_set_event_cb( gyroscopeListener, GYROSCOPE_INTERVAL_MS, gyroscope_cb, ad);
	if(error != SENSOR_ERROR_NONE) {
		return error;
	}
	error = sensor_listener_start( gyroscopeListener );
	return SENSOR_ERROR_NONE;
}

static void magnetic_cb(sensor_h sensor, sensor_event_s *event, void *data)
{
	MagneticX=(float)event->values[0];
	MagneticY=(float)event->values[1];
	MagneticZ=(float)event->values[2];

	//char gyro_data[256];

	//sprintf(gyro_data, "%f %f %f", MagneticX, MagneticY, MagneticZ);

	//dlog_print(DLOG_ERROR, "RearCameraApp", gyro_data);

	/*
	 * Implement your algorithm to maove marker acording to the change in magnetic field
	 */

	//Evas_Object *marker_slw = elm_object_part_content_get(global_view->layout, "marker_content");
	//elm_object_part_text_set(marker_slw, "marker_text", gyro_data);
	//current_x = current_x - (30 * (MagneticZ - z));

	//evas_object_show(marker_slw);


	// TO move along y axis
	/*current_y = current_y - (30 * (MagneticY - y));
	evas_object_move(marker_slw, current_x, current_y);

	evas_object_show(marker_slw);*/



	/*Fov calucalation by khairul */
	//Z Axis
	//07-09 09:40:36.014 : ERROR / USR_TAG ( 6447 : 6447 ) : Magnetic x = 39.840000,Magnetic y = -33.480000,Magnetic z = 35.520000
	//07-09 09:40:47.404 : ERROR / USR_TAG ( 6447 : 6447 ) : Magnetic x = 30.000000,Magnetic y = -33.540001,Magnetic z = -15.120000
	//X Axis
	//07-09 09:40:40.324 : ERROR / USR_TAG ( 6447 : 6447 ) : Magnetic x = 14.880000,Magnetic y = -36.059998,Magnetic z = 9.360000
	//07-09 09:40:28.364 : ERROR / USR_TAG ( 6447 : 6447 ) : Magnetic x = 64.199997,Magnetic y = -31.500000,Magnetic z = 8.040000
	float tmpMagneticZ=0;
	int thAngle;
	if (MagneticZ > 35){
		tmpMagneticZ=35;
	}
	else if (MagneticZ < -15){
		tmpMagneticZ=-15;
	}
	else {
		tmpMagneticZ = MagneticZ;
	}
	 thAngle=map((tmpMagneticZ+15), 0, (35+15), 360, 0);
	 //minAngle=angleFromAbsoluteNorth - (CAMERA_FOV/2);
	 //maxAngle=angleFromAbsoluteNorth + (CAMERA_FOV/2);
	 //if (thAngle<(Array.oldAngle-5) || thAngle<(Array.oldAngle-5)){
	 RefreshFOV(thAngle);
	 //Array.oldAngle=thAngle;
	 //}


	x = MagneticX;
	y = MagneticY;
	z = MagneticZ;
}

static int register_magnetic_callback(app_data *ad)
{
	int error;
	bool supported;
	sensor_h magnetic;

	error = sensor_is_supported( SENSOR_MAGNETIC, &supported );
	if(error != SENSOR_ERROR_NONE && supported){
		return error;
	}
	error = sensor_get_default_sensor(SENSOR_MAGNETIC, &magnetic);
	if(error != SENSOR_ERROR_NONE){
		return error;
	}
	error = sensor_create_listener( magnetic, &magneticListener);
	if(error != SENSOR_ERROR_NONE){
		return error;
	}
	error = sensor_listener_set_event_cb( magneticListener, MAGNETIC_INTERVAL_MS, magnetic_cb, ad );
	if(error != SENSOR_ERROR_NONE){
		return error;
	}
	error = sensor_listener_start( magneticListener );
	return SENSOR_ERROR_NONE;
}
/*-----Start JSON Parsing----------*/
gchar*
ExtractValue(JsonNode *node)
{
    gchar * value;
    GType valueType=json_node_get_value_type(node);
	switch(valueType)
    {
        case G_TYPE_STRING:
        value = json_node_dup_string(node);
        break;

        case G_TYPE_INTEGER:
        value= malloc(20*sizeof(gchar));
        sprintf (value, "%d", (int)json_node_get_int(node));
        break;

        case G_TYPE_DOUBLE:
        value= malloc(40*sizeof(gchar));
        sprintf (value, "%f", json_node_get_double(node));
        break;

        case G_TYPE_BOOLEAN:
        value= malloc(6*sizeof(gchar));
        if(json_node_get_boolean(node)==TRUE)
        {
        	sprintf (value, "%s", "true");
        }
        else
        {
        	sprintf (value, "%s", "false");
        }
        break;

        default:
        value= malloc(22*sizeof(gchar));
        sprintf (value, "%s", "Value of unknown type");
        break;
    }
	return value;
}

Eina_List*
ParseJsonEntity(JsonNode *root, bool isArrayParsing)
{
    Eina_List *jsonList = NULL;
    if (JSON_NODE_TYPE (root) == JSON_NODE_ARRAY)
    {
         JsonArray* array = json_node_get_array(root);
         guint arraySize = json_array_get_length (array);
         JsonNode *arrayElem;
         guint i;
         for(i=0;i<arraySize;i++)
	     {
            Eina_List *jsonArrElemList=NULL;
        	arrayElem = json_array_get_element(array,i);
        	jsonArrElemList=ParseJsonEntity(arrayElem, true);
        	jsonList= eina_list_merge(jsonList,jsonArrElemList);
        	//add array member (with pair key/value) to the end of the list
	     }
    }
    else if (JSON_NODE_TYPE (root) == JSON_NODE_VALUE)
    {

        jsonList = eina_list_append(jsonList,ExtractValue(root));
     	if(isArrayParsing)
     	{
           jsonList = eina_list_append(jsonList,NULL);
           //add member of list with data=NULL (for arrays without keys as separator)
     	}
    }
    else if(JSON_NODE_TYPE (root) == JSON_NODE_OBJECT)
    {
        JsonObject *object = NULL;

        object = json_node_get_object (root);

        if (object != NULL)
        {
            GList * keysList = NULL;
            GList * valList = NULL;
            guint size;

            size = json_object_get_size (object);

            keysList = json_object_get_members (object);
            valList = json_object_get_values (object);

		    JsonNode *tmpNode;
		    gchar *keyName;
		    int j;
 	        for(j=0;j<size;j++)
	        {
 	 		    if (keysList)
 	 		    {
 	                keyName = malloc( (strlen(keysList->data) +1)*sizeof(gchar) );
 		            sprintf (keyName, "%s", (gchar*)(keysList->data));
 	 	            jsonList = eina_list_append(jsonList,keyName);
 	 		    }
 		        if (valList)
 		        {
 		        	tmpNode=(JsonNode*)(valList->data);
 		        }

 		        Eina_List *l= ParseJsonEntity(tmpNode,false);
 	            jsonList = eina_list_append(jsonList,l);

 	            keysList=g_list_next(keysList);
 		        valList=g_list_next(valList);

 	        }
    	    if (keysList != NULL) g_list_free(keysList);
    	    if (valList != NULL) g_list_free(valList);
        }
    }
    return jsonList;
}

void
printParsedList(Eina_List * jsonlist, int level)
{
    Eina_List *l;
    void *list_data;
    int inc=0;
    EINA_LIST_FOREACH(jsonlist, l, list_data)
    {
    	//dlog_print(DLOG_ERROR, "USR_TAG", "EinaList data = %s", jsonlist);
        if(inc%2){
        	int level_=level+1;
        	if(list_data!=NULL)
        	{
               printParsedList(list_data,level_);
            }
        }
        else
        {
           gchar* spaceOffset=NULL;
      	   spaceOffset = malloc((2*level+1)*sizeof(gchar));
      	   int i;
           for(i=0; i<level;i++){
        	   spaceOffset[2*i]='-';
        	   spaceOffset[2*i+1]='>';
           }
           spaceOffset[2*level]='\0';

           if (flag_name==1){
			   flag_name=0;
			   sprintf(Array.resturants[Array.iName].name,"%s",(gchar*)list_data);
			   Array.iName++;
		   }
		   if (flag_address==1){
			   flag_address=0;
			   sprintf(Array.resturants[Array.iAddress].address,"%s",(gchar*)list_data);
			   Array.iAddress++;
		   }
		   if (flag_lat==1){
			   flag_lat=0;
			   sscanf((gchar*)list_data, "%lf", &Array.resturants[Array.iLat].lat);
			   Array.iLat++;
		   }
		   if (flag_lng==1){
			   flag_lng=0;
			   sscanf((gchar*)list_data, "%lf", &Array.resturants[Array.iLng].lng);
			   Array.iLng++;
		   }
		   if (flag_rating==1){
			   flag_rating=0;
			   sscanf((gchar*)list_data, "%f", &Array.resturants[Array.iName-1].rating);
			   //sprintf(Array.resturants[Array.iName-1].rating,"%s",(gchar*)list_data);
			   Array.iRating++;
		   }

		   if (strcmp((gchar*)list_data, "name")==0){
			   flag_name=1;
			}
			else if (strcmp((gchar*)list_data, "vicinity")==0){
			   flag_address=1;
			}
			else if (strcmp((gchar*)list_data, "lat")==0){
				flag_lat=1;
			}
			else if (strcmp((gchar*)list_data, "lng")==0){
				flag_lng=1;
			}
			else if (strcmp((gchar*)list_data, "rating")==0){
				flag_rating=1;
			}

           //LOGI("%s %s" , spaceOffset, (gchar*)list_data);
           //dlog_print(DLOG_ERROR, "USR_TAG", "EinaList data = %s", list_data);
           if(spaceOffset!=NULL) free(spaceOffset);
           if(list_data!=NULL) free(list_data);
           //for all keys and values added into Eina_List memory should be freed manually
        }
        inc++;
    }
    eina_list_free(jsonlist);
}

void parseArray_cb(void *data)
{
		JsonParser *jsonParser;
		jsonParser = json_parser_new ();
		guint n_nested_objects = G_N_ELEMENTS (data);
		//LOGI("ParseNestedObjects size %d",n_nested_objects);
		int i;
		for (i = 0; i < n_nested_objects/4; i++)
		{
			GError *error = NULL;

			if (!json_parser_load_from_data (jsonParser, data, -1, &error))
			{
				LOGI("ParseNestedObjects Error");
				g_error_free (error);
			}
			else
			{
				int level =1;
				Eina_List *jsonList = NULL;
				jsonList =ParseJsonEntity(json_parser_get_root (jsonParser),false);
				printParsedList(jsonList,level);
			}
		}
		g_object_unref(jsonParser);
		int j;
		//LOGI("NAmes Of REsturants");
		for (j=0; j<(Array.iName);j++){
			//LOGI("%s " , name[j]);
			dlog_print(DLOG_ERROR, "USR_TAG", "Restaurant[%d] :: Name:%s, Lat:%lf, Lng:%lf",j,Array.resturants[j].name, Array.resturants[j].lat, Array.resturants[j].lng);
		}
		CalculateData();
		Array.oldAngle=0;

}
/*----------End JSON Parsing--------*/

/*----------Start CURL--------------*/
struct string {
  char *ptr;
  size_t len;
};

void init_string(struct string *s) {
	  s->len = 0;
	  s->ptr = malloc(s->len+1);
	  if (s->ptr == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	  }
	  s->ptr[0] = '\0';
}

size_t writeCallback(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	  size_t new_len = s->len + size*nmemb;
	  s->ptr = realloc(s->ptr, new_len+1);
	  if (s->ptr == NULL) {
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	  }
	  memcpy(s->ptr+s->len, ptr, size*nmemb);
	  s->ptr[new_len] = '\0';
	  s->len = new_len;

	  return size*nmemb;
}


//static void
//btn_parseArray_clicked_cb()
//{
//	CURL *curl;
//	CURLcode res;
//
////	memset(name,0,sizeof(name));
////	memset(address,0,sizeof(address));
////	memset(lat,0,sizeof(lat));
////	memset(lng,0,sizeof(lng));
//	idx_name=0, idx_address=0, idx_lat=0, idx_lng=0;
//
//	curl_global_init(CURL_GLOBAL_ALL);
//	curl = curl_easy_init();
//
//	struct string s;
//	init_string(&s);
//
//	//curl_easy_setopt(curl, CURLOPT_URL, jsonURL);
//	curl_easy_setopt(curl, CURLOPT_URL, "https://maps.googleapis.com/maps/api/place/nearbysearch/json?types=restaurant&radius=500&location=23.7489959,90.3917064&sensor=true&key=AIzaSyDHR9lSsc7jB8VtYuj6NuaK6QcspfFUnZQ");
//	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
//	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
//
//	res = curl_easy_perform(curl);
//	if(res != CURLE_OK) {
//		dlog_print(DLOG_ERROR, "USR_TAG", "curl_easy_perform Error %d", res);
//	} else {
//		//dlog_print(DLOG_ERROR, "USR_TAG", "curl URL %s", jsonURL);
//		parseArray_cb(s.ptr);
//	}
//
//	  /* cleanup curl stuff */
//	  curl_easy_cleanup(curl);
//	  curl_global_cleanup();
//
//}
/*------End CURL------*/
Evas_Object *main_view_add(Evas_Object *navi)
{
	//curl_easy_setopt(curl, CURLOPT_URL, "https://maps.googleapis.com/maps/api/place/nearbysearch/json?types=restaurant&radius=500&location=23.7489959,90.3917064&sensor=true&key=AIzaSyDHR9lSsc7jB8VtYuj6NuaK6QcspfFUnZQ");
	sprintf(jsonURL,"https://maps.googleapis.com/maps/api/place/nearbysearch/json?types=restaurant&radius=500&location=23.7489959,90.3917064&sensor=true&key=AIzaSyDHR9lSsc7jB8VtYuj6NuaK6QcspfFUnZQ");

	main_view *view = calloc(1, sizeof(main_view));
    RETVM_IF(!view, NULL, "calloc() failed");
    view->navi = navi;

    view->layout = ui_utils_layout_add(view->navi, _main_view_destroy, view);
    if(!view->layout)
    {
        ERR("ui_utils_layout_add() failed");
        free(view);
        return NULL;
    }
    elm_layout_file_set(view->layout, get_resource_path(SELF_CAMERA_LAYOUT), "layout");
    elm_object_signal_emit(view->layout, "mouse,clicked,1", "ar_view_text");

    view->preview_canvas = evas_object_image_filled_add(evas_object_evas_get(view->layout));
	if(!view->preview_canvas)
	{
	   ERR("_main_view_rect_create() failed");
	   evas_object_del(view->layout);
	   return NULL;
	}

	elm_object_part_content_set(view->layout, "elm.swallow.content", view->preview_canvas);


	view->camera_enabled = _main_view_init_camera(view);
	if (!view->camera_enabled)
	{
	   ERR("_main_view_start_preview failed");
	   _main_view_show_warning_popup(navi, _error, _camera_init_failed, _ok, view);
	}

 //  	_main_view_thumbnail_load(view);

    _main_view_register_cbs(view);
   //_list_view_selected_cb();
    //btn_parseArray_clicked_cb();

    //register_magnetic_callback(app_main_appdata_get());
	register_orientation_callback(app_main_appdata_get());
	global_view = view;

	int i;

		for (i = 0; i < NUM_RESULTS_IN_FOV; i++) {
			view->geo_marker[i] = elm_layout_add(view->preview_canvas);
			evas_object_size_hint_weight_set(view->geo_marker[i], EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(view->geo_marker[i], EVAS_HINT_FILL, EVAS_HINT_FILL);
			elm_layout_file_set(view->geo_marker[i], get_resource_path(SELF_CAMERA_LAYOUT), "marker_layout");
			elm_object_content_set(view->preview_canvas, view->geo_marker[i]);
			evas_object_resize(view->geo_marker[i], 50, 70);
			evas_object_move(view->geo_marker[i], current_x[i], current_y[i]);
			evas_object_show(view->geo_marker[i]);
		}


	view->navi_item = elm_naviframe_item_push(view->navi, NULL, NULL, NULL, view->layout, NULL);
	elm_naviframe_item_title_enabled_set(view->navi_item, EINA_FALSE, EINA_FALSE);

	return view->layout;
}

static void _main_view_destroy(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    main_view *view = data;
    _main_view_stop_camera_preview(view->camera);
    camera_destroy(view->camera);

    free(data);
}

static Eina_Bool _main_view_start_camera_preview (camera_h camera)
{
    camera_state_e cur_state = CAMERA_STATE_NONE;
    int res = camera_get_state(camera, &cur_state);
    if(CAMERA_ERROR_NONE == res)
    {
        if(cur_state != CAMERA_STATE_PREVIEW)
        {
            res = camera_start_preview(camera);
            if (CAMERA_ERROR_NONE == res)
            {
                camera_start_focusing(camera,TRUE);
                return EINA_TRUE;
            }
        }
    }
    else
    {
        ERR("Cannot get camera state. Error: %d", res);
    }

    return EINA_FALSE;
}

static Eina_Bool _main_view_stop_camera_preview (camera_h camera)
{
    camera_state_e cur_state = CAMERA_STATE_NONE;
    int res = camera_get_state(camera, &cur_state);
    if(CAMERA_ERROR_NONE == res)
    {
        if(cur_state == CAMERA_STATE_PREVIEW)
        {
            camera_stop_preview(camera);
            return EINA_TRUE;
        }
    }
    else
    {
        ERR("Cannot get camera state. Error: %d", res);
    }

    return EINA_FALSE;
}

static Eina_Bool _main_view_init_camera(main_view *view)
{
    int result = camera_create(CAMERA_DEVICE_CAMERA0, &view->camera);
    if (CAMERA_ERROR_NONE == result)
    {
        if (view->preview_canvas)
        {
            result = camera_set_display(view->camera, CAMERA_DISPLAY_TYPE_EVAS, GET_DISPLAY(view->preview_canvas));
            if (CAMERA_ERROR_NONE == result)
            {
            	camera_attr_set_preview_fps(view->camera,CAMERA_ATTR_FPS_AUTO);
            	camera_attr_set_image_quality(view->camera,100);
                camera_set_display_mode(view->camera, CAMERA_DISPLAY_MODE_FULL);
                camera_set_display_visible(view->camera, true);

                return _main_view_start_camera_preview(view->camera);
            }
        }
    }
    return !result;
}

static void _main_view_register_cbs(main_view *view)
{
    evas_object_smart_callback_add(view->layout, EVENT_BACK, _main_view_back_cb, view);
    evas_object_smart_callback_add(view->layout, EVENT_PAUSE, _main_view_pause_cb, view);
    evas_object_smart_callback_add(view->layout, EVENT_RESUME, _main_view_resume_cb, view);
    //elm_object_signal_callback_add(view->layout, "timer_2_selected", "*",_main_view_timer_2_cb, view);
    //elm_object_signal_callback_add(view->layout, "timer_5_selected", "*",_main_view_timer_5_cb, view);
    //elm_object_signal_callback_add(view->layout, "timer_10_selected", "*",_main_view_timer_10_cb, view);
    // elm_object_signal_callback_add(view->layout, "shutter_button_clicked", "*", _main_view_shutter_button_cb, view);
    //elm_object_signal_callback_add(view->layout, "gallery_button_clicked", "*",_main_view_gallery_button_cb, view);
    elm_object_signal_callback_add(view->layout, "marker_clicked", "*",_main_view_marker_clicked_cb, view);
    //evas_object_smart_callback_add(view->layout,"marker_clicked",_main_view_marker_clicked_cb, view);
   elm_object_signal_callback_add(view->layout, "ar_view_selected_signal", "*", _ar_view_selected_cb, view);
    elm_object_signal_callback_add(view->layout, "list_view_selected_signal", "*", _list_view_selected_cb, view);
}

static void _main_view_back_cb(void *data, Evas_Object *obj, void *event_info)
{
    RETM_IF(!data, "data is NULL");
    //main_view *view = data;


}

static void _main_view_pause_cb(void *data, Evas_Object *obj, void *event_info)
{
    RETM_IF(!data, "data is NULL");
    main_view *view = data;

    _main_view_stop_camera_preview(view->camera);
}

static void _main_view_resume_cb(void *data, Evas_Object *obj, void *event_info)
{
    RETM_IF(!data, "data is NULL");
    main_view *view = data;

    _main_view_start_camera_preview(view->camera);
}

static void _main_view_marker_clicked_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source)
{
   // RETM_IF(!data, "data is NULL");
    //main_view *view = data;

    	CURL *curl;
    	CURLcode res;
    //	main_view *view = source;
    	memset(name,0,sizeof(name));
    	memset(address,0,sizeof(address));
    	memset(lat,0,sizeof(lat));
    	memset(lng,0,sizeof(lng));
    	idx_name=0, idx_address=0, idx_lat=0, idx_lng=0;
    	Array.iAddress=0;
    	Array.iName=0;
    	Array.iLat=0;
    	Array.iLng=0;
    	Array.iRating=0;
    	curl_global_init(CURL_GLOBAL_ALL);
    	curl = curl_easy_init();

    	struct string s;
    	init_string(&s);

    	dlog_print(DLOG_ERROR, "USR_TAG", "checking url %s", jsonURL);
    	curl_easy_setopt(curl, CURLOPT_URL, jsonURL);
    	//curl_easy_setopt(curl, CURLOPT_URL, "https://maps.googleapis.com/maps/api/place/nearbysearch/json?types=restaurant&radius=500&location=23.7489959,90.3917064&sensor=true&key=AIzaSyDHR9lSsc7jB8VtYuj6NuaK6QcspfFUnZQ");
    	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    	res = curl_easy_perform(curl);
    	if(res != CURLE_OK) {
    		dlog_print(DLOG_ERROR, "USR_TAG", "curl_easy_perform Error %d", res);
    	} else {
    		//dlog_print(DLOG_ERROR, "USR_TAG", "curl URL %s", jsonURL);
    		parseArray_cb(s.ptr);
    	}

    	  /* cleanup curl stuff */
    	  curl_easy_cleanup(curl);
    	  curl_global_cleanup();
    /*current_x = 320;
    current_y = 550;

    Evas_Smart *marker_slw = elm_object_part_content_get(view->layout, "marker_content");
	evas_object_move(marker_slw, 320, 550);
	evas_object_show(marker_slw);*/

	/*Evas *e = evas_object_evas_get(view->preview_canvas);
	Evas_Object *marker_slw_2 = evas_object_smart_add(e, marker_slw);

	evas_object_move(marker_slw_2, 520, 550);
	evas_object_show(marker_slw_2);*/



}

static void _ar_view_selected_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source)
{
    RETM_IF(!data, "data is NULL");
    main_view *view = data;


    dlog_print(DLOG_ERROR, "RearCameraApp", "_ar_view_selected_cb");

    if (2 == num_navi_item) {
		view->preview_canvas = evas_object_image_filled_add(evas_object_evas_get(view->layout));
		if(!view->preview_canvas)
		{
		   ERR("_main_view_rect_create() failed");
		   evas_object_del(view->layout);
		   return;
		}

		elm_object_part_content_set(view->layout, "elm.swallow.content", view->preview_canvas);


		view->camera_enabled = _main_view_init_camera(view);
		if (!view->camera_enabled)
		{
		   ERR("_main_view_start_preview failed");
		   _main_view_show_warning_popup(view->navi, _error, _camera_init_failed, _ok, view);
		}
		num_navi_item = 1;

		register_magnetic_callback(app_main_appdata_get());
    }
}

static void _list_view_selected_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source)
{
    RETM_IF(!data, "data is NULL");
    main_view *view = data;


    //btn_parseArray_clicked_cb();
    start_location_manager(view);

    dlog_print(DLOG_ERROR, "RearCameraApp", "_list_view_selected_cb");

    num_navi_item = 2;


    sensor_listener_stop(magneticListener);
    int i;

	for (i = 0; i < 10; i++) {
		evas_object_hide(view->geo_marker[i]);
	}
	camera_stop_preview(view->camera);
	elm_object_content_unset(view->layout);

	view->location_list = create_genlist(view->layout);
	elm_object_part_content_set(view->layout, "elm.swallow.content", view->location_list);
}


static void _main_view_show_warning_popup(Evas_Object *navi, const char *caption, const char *text, const char *button_text, void *data)
{
    RETM_IF(!data, "data is null");
    DBG(" <<< called");
    main_view *view = data;

    Evas_Object *popup = elm_popup_add(navi);
    RETM_IF(!popup, "popup is not created");
    elm_object_part_text_set(popup, "title,text", caption);
    elm_object_text_set(popup, text);
    evas_object_show(popup);

    Evas_Object *button = elm_button_add(popup);
    RETM_IF(!button, "button is not created");
    elm_object_style_set(button, POPUP_BUTTON_STYLE);
    elm_object_text_set(button, button_text);
    elm_object_part_content_set(popup, POPUP_BUTTON_PART, button);
    evas_object_smart_callback_add(button, "clicked", _main_view_popup_close_cb, view);

    eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, _main_view_popup_close_cb, view);

    view->popup = popup;
}

static void _main_view_popup_close_cb(void *data, Evas_Object *obj, void *event_info)
{
    RETM_IF(!data, "data is null");
    DBG(" <<< called");
    main_view *view = data;
    if (view->popup)
    {
        DBG("popup closed");
        evas_object_del(view->popup);
        view->popup = NULL;
    }
}

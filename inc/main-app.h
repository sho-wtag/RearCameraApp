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

#ifndef __MAIN_APP_H__
#define __MAIN_APP_H__

#include <Evas.h>
#include <Ecore_Evas.h>

typedef struct _app_data app_data;

#define pi 3.14159265358979323846
#define CAMERA_FOV 45
#define NUM_RESULTS_IN_FOV 15
#define WINDOW_Xmin 0
#define WINDOW_Xmax 728
#define WINDOW_Ymin 50
#define WINDOW_Ymax 950

#define ACCELEROMETER_INTERVAL_MS 20
#define MAGNETIC_INTERVAL_MS 20
#define GYROSCOPE_INTERVAL_MS 20


typedef struct _location_list
{
	char res_name[256];
	char location[256];
	char rating[256];
} location_list;




typedef struct ResturantStruct {
  char name[100];
  char address[100];
  double lat;
  double lng;
  float rating;
  int distance;
  int angle;
  int imaginaryAngle;
}ResturantStruct;

typedef struct FOVStruct {
  char *name;
  char *address;
  int x;
  int y;
}FOVStruct;


typedef struct TotalArrayStruct {
  struct ResturantStruct resturants[100];
  struct FOVStruct FOV[NUM_RESULTS_IN_FOV+1];
  int iName;
  int iAddress;
  int iLat;
  int iLng;
  int iRating;
  int angleFromAbsoluteNorth;
  int resultsInFOV;
  int oldAngle;
}TotalArrayStruct;




/*
 * @brief Create application instance
 * @return Application instance on success, otherwise NULL
 */
app_data *app_create();

/*
 * @brief Destroy application instance
 * @param[in]   app     Application instance
 */
void app_destroy(app_data *app);

/*
 * @brief Run Tizen application
 * @param[in]   app     Application instance
 * @param[in]   argc    argc paremeter received in main
 * @param[in]   argv    argv parameter received in main
 */
int app_run(app_data *app, int argc, char **argv);


app_data* app_main_appdata_get();

#endif /* __MAIN_APP_H__ */

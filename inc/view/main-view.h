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


#ifndef __MAIN_VIEW_H__
#define __MAIN_VIEW_H__

#include <Evas.h>
#include <Ecore_Evas.h>

/*
 * @brief Adding new view to parent object
 * @param[in]   parent  Parent naviframe
 * @return Main view layout on success, otherwise NULL
 */
Evas_Object *main_view_add(Evas_Object *navi);

#endif /* __MAIN_VIEW_H__ */

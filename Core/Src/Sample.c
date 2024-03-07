/*************************************************************************
* COPYRIGHT - All Rights Reserved.
* 2019 Eastech Flow Controls, Inc.
*
* OWNER:
* Eastech Flow Controls, Inc.
* 4250 S. 76th E. Avenue
* Tulsa, Oklahoma 74145
*
* toll-free: 	(800) 226-3569
* phone: 	(918) 664-1212
* fax: 	(918) 664-8494
* email: 	info@eastechflow.com
* website: 	http://eastechflow.com/
*
* OBLIGATORY: All information contained herein is, and remains
* the property of EASTECH FLOW CONTROLS, INC. The intellectual
* and technical concepts contained herein are proprietary to
* EASTECH FLOW CONTROLS, INC. and its suppliers and may be covered
* by U.S. and Foreign Patents, patents in process, and are protected
* by trade secret or copyright law.
*
* Dissemination, transmission, forwarding or reproduction of this
* content in any way is strictly forbidden.
*************************************************************************/

#include "main.h"
#include "Common.h"
#include "rtc.h"
#include "LTC2943.h"
#include "WiFi_Socket.h"
#include "WiFi.h"
#include "Log.h"
#include "Sample.h"
#include "DeltaQ.h"
#include "Sensor.h"
#include "debug_helper.h"
#include "time.h"
#include "trace.h"
#include "factory.h"
#include "Hydro.h"
#include "quadspi.h"



// zg - these numbers are "level" data to be stored in the log
float sample_data[2980] = { 2, 1.7, 1.91, 1.59, 1.63, 1.71, 1.66, 1.85, 1.77, 2.15, 2.16, 2.39, 2.15, 2.23, 2.04, 1.79, 1.79, 1.79, 1.07, 1.67, 1.67,
    2.21, 2.44, 2.3, 2.13, 1.83, 1.91, 1.76, 1.63, 1.64, 1.8, 1.82, 1.9, 1.88, 2.07, 2.06, 2.19, 2.12, 1.78, 1.78, 1.78, 1.39, 1.25, 1.29, 1.46, 2.04,
    2.64, 2.1, 2.16, 1.98, 1.93, 1.97, 1.91, 1.77, 1.59, 2.19, 2.04, 2.24, 2.19, 2.23, 2.18, 1.99, 1.99, 1.38, 1.43, 1.38, 1.37, 1.47, 1.02, 1.02,
    2.09, 2.07, 2.2, 2.46, 2.41, 2.57, 2.41, 1.92, 1.88, 1.75, 1.78, 1.95, 2.1, 1.92, 2.07, 1.85, 1.94, 1.51, 1.47, 1.39, 1.41, 1.35, 1.39, 1.43,
    1.85, 2.07, 2.49, 2.31, 1.96, 2.1, 2, 2.16, 2.05, 2.29, 2.16, 2.38, 2.5, 2.42, 1.84, 2.12, 1.75, 1.42, 1.49, 1.3, 1.26, 1.36, 1.41, 2.21, 2.21,
    1.93, 2.07, 2.31, 2.36, 1.91, 1.77, 1.69, 1.8, 2.03, 2.18, 2.33, 2.14, 2.23, 2.27, 1.9, 1.72, 1.56, 1.55, 1.39, 1.34, 1.47, 1.47, 1.96, 2.56,
    2.24, 2.13, 1.96, 1.98, 2.17, 1.96, 1.82, 1.89, 1.94, 2.03, 2.24, 1.81, 2.41, 2.3, 2.27, 2.16, 1.66, 1.34, 1.41, 1.24, 1.18, 1.52, 1.57, 2.2,
    1.92, 1.96, 2.26, 2.32, 2.09, 2.01, 2.01, 2.08, 1.85, 2.06, 2.08, 2.29, 2.17, 2.3, 2.13, 1.98, 1.8, 1.64, 1.42, 1.37, 1.42, 1.34, 1.56, 2.09,
    2.26, 2.31, 2.4, 2.78, 2.38, 2.45, 2.21, 2, 2.2, 1.89, 2.37, 2.23, 2.27, 1.92, 2.2, 1.84, 1.63, 1.35, 1.27, 1.29, 1.43, 1.25, 1.57, 1.81, 2.04,
    2.48, 2.6, 2.45, 2.21, 2.01, 2.26, 2.32, 1.98, 2.29, 1.88, 2.13, .83, 1.85, 2.01, 1.82, 1.58, 1.6, 1.44, 1.36, 1.35, 1.41, 1.59, 2.01, 2.17, 2.47,
    2.34, 2.52, 2.19, 2.3, 1.84, 2.09, 2.01, 1.98, 1.89, 2.12, 2.26, 2.1, 1.86, 1.67, 1.6, 1.52, 1.58, 1.13, 1.34, 0, 1.89, 1.82, 2.33, 2.39, 2.65,
    2.32, 2.26, 2.83, 2.23, 2.17, 1.9, 1.98, 2.67, 2.38, 2.41, 1.97, 1.99, 1.66, 1.43, 1.26, 1.21, 1.2, 1.25, 1.37, 1.98, 2.58, 2.01, 1.92, 2.14,
    1.71, 1.8, 1.62, 1.64, 1.69, 1.68, 1.97, 1.9, 2.24, 2.39, 2.2, 2.51, 1.82, 1.62, 2.21, 2.09, 1.76, 1.78, 1.98, 2.48, 2.87, 2.41, 2.3, 2.05, 1.97,
    2.08, 2.06, 1.96, 1.81, 1.89, 2.33, 2.23, 2.56, 2.58, 2.4, 2.1, 1.83, 1.6, 1.58, 1.8, 1.7, 1.88, 4.35, 5.17, 5.62, 2.67, 2.39, 4.93, 5.2, 5.36,
    4.44, 4.52, 3.97, 3.66, 3.6, 3.3, 3.4, 3.27, 3.09, 3.05, 2.75, 2.64, 2.37, 2.36, 2.32, 2.35, 2.44, 2.9, 2.79, 3.14, 2.54, 2.62, 2.77, 2.51, 2.39,
    2.16, 2.33, 2.28, 2.25, 2.46, 2.68, 2.9, 2.51, 2.45, 2.29, 2.06, 1.99, 1.84, 1.87, 1.8, 1.92, 2.42, 2.79, 2.58, 2.16, 2.19, 2.28, 2.31, 2.14,
    2.23, 2.03, 2.15, 2.34, 2.44, 2.47, 2.55, 2.53, 2.5, 1.98, 1.93, 1.89, 1.69, 1.88, 1.73, 1.89, 1.8, 2.2, 2.62, 2.6, 2.69, 2.71, 2.19, 2.48, 2.46,
    2.36, 2.15, 2.52, 2.32, 2.25, 2.28, 2.27, 2.22, 1.78, 2, 1.79, 1.85, 1.85, 1.79, 2.14, 2.94, 3.66, 4.28, 4.43, 4.22, 4.09, 3.92, 3.88, 3.78, 3.63,
    3.56, 4.21, 4.34, 3.99, 3.94, 3.42, 3.45, 3.03, 2.87, 2.65, 2.57, 2.53, 2.59, 2.56, 3.12, 3.27, 2.71, 2.64, 2.56, 2.66, 2.36, 2.37, 2.63, 2.68,
    2.86, 3.16, 3.03, 3.2, 3.25, 3.23, 3.14, 2.57, 2.68, 2.45, 2.45, 2.94, 3.45, 3.6, 4.19, 3.73, 3.95, 3.74, 4.51, 4.19, 3.97, 3.66, 3.69, 3.55,
    3.44, 3.44, 3.34, 3.4, 3.51, 3.06, 2.95, 2.93, 2.66, 2.55, 2.47, 2.53, 2.53, 2.59, 3.07, 3.11, 2.78, 2.75, 2.78, 2.79, 2.52, 2.47, 2.55, 2.5,
    2.58, 2.44, 2.64, 2.72, 2.89, 2.97, 2.58, 2.43, 2.3, 1.93, 2, 1.99, 2.1, 2.18, 2.91, 2.84, 2.66, 2.58, 2.72, 2.44, 2.54, 2.34, 2.29, 2.46, 2.29,
    2.53, 2.47, 2.49, 2.5, 2.73, 2.57, 2.38, 2.15, 2.09, 1.93, 1.93, 1.99, 2.05, 2.48, 2.85, 2.53, 2.64, 2.53, 2.6, 2.35, 2.31, 2.41, 2.29, 2.39,
    2.19, 2.35, 2.54, 2.77, 2.55, 2.45, 2.3, 2.13, 1.9, 1.84, 1.95, 1.81, 1.92, 1.8, 2.44, 2.59, 2.89, 2.64, 2.73, 2.52, 2.39, 2.47, 2.74, 2.48, 2.83,
    2.36, 2.4, 2.43, 2.51, 2.22, 2.26, 1.94, 1.98, 1.83, 1.75, 1.74, 1.92, 1.9, 2.18, 2.74, 2.69, 2.6, 2.24, 2.41, 2.38, 2.45, 2.26, 2.55, 2.6, 2.64,
    2.58, 2.43, 2.51, 2.54, 2.03, 1.92, 1.83, 1.63, 1.67, 1.7, 2.02, 2.38, 3.13, 2.76, 2.88, 2.83, 2.63, 2.4, 2.42, 2.35, 2.42, 2.59, 2.53, 2.69,
    2.65, 2.7, 2.38, 2.54, 2.38, 2.09, 2.01, 1.98, 1.94, 1.89, 1.98, 2.65, 2.82, 2.34, 2.58, 2.45, 1.98, 2.65, 3.07, 3.11, 2.78, 2.75, 2.78, 2.79,
    2.52, 2.47, 2.55, 2.5, 2.58, 2.44, 2.64, 2.72, 2.89, 2.97, 2.58, 2.43, 2.3, 1.93, 2, 1.99, 2.1, 2.18, 2.91, 2.84, 2.66, 2.58, 2.72, 2.44, 2.54,
    2.34, 2.29, 2.46, 1.89, 1.69, 1.88, 1.73, 1.89, 1.8, 2.2, 2.62, 2.6, 2.69, 2.71, 2.19, 2.48, 2.46, 2.36, 2.15, 2.52, 2.32, 2.25, 2.28, 2.27, 2.22,
    1.78, 2, 1.79, 1.85, 1.85, 1.79, 2.14, 2.94, 3.66, 4.28, 4.43, 4.22, 4.09, 3.92, 3.88, 3.78, 3.63, 3.56, 4.21, 4.34, 3.99, 3.94, 3.42, 3.45, 3.03,
    2.87, 2.93, 2.66, 2.55, 2.47, 2.53, 2.53, 2.59, 3.07, 3.11, 2.78, 2.75, 2, 1.7, 1.91, 1.59, 1.63, 1.71, 1.66, 1.85, 1.77, 2.15, 2.16, 2.39, 2.15,
    2.23, 2.04, 1.79, 1.79, 1.79, 1.07, 1.67, 1.67, 2.21, 2.44, 2.3, 2.13, 1.83, 1.91, 1.76, 1.63, 1.64, 1.8, 1.82, 1.9, 1.88, 2.07, 2.06, 2.19, 2.12,
    1.78, 1.78, 1.78, 1.39, 1.25, 1.29, 1.46, 2.04, 2.64, 2.1, 2.16, 1.98, 1.93, 1.97, 1.91, 1.77, 1.59, 2.19, 2.04, 2.24, 2.19, 2.23, 2.18, 1.99,
    1.99, 1.38, 1.43, 1.38, 1.37, 1.47, 1.02, 1.02, 2.09, 2.07, 2.2, 2.46, 2.41, 2.57, 2.41, 1.92, 1.88, 1.75, 1.78, 1.95, 2.1, 1.92, 2.07, 1.85,
    1.94, 1.51, 1.47, 1.39, 1.41, 1.35, 1.39, 1.43, 1.85, 2.07, 2.49, 2.31, 1.96, 2.1, 2, 2.16, 2.05, 2.29, 2.16, 2.38, 2.5, 2.42, 1.84, 2.12, 1.75,
    1.42, 1.49, 1.3, 1.26, 1.36, 1.41, 2.21, 2.21, 1.93, 2.07, 2.31, 2.36, 1.91, 1.77, 1.69, 1.8, 2.03, 2.18, 2.33, 2.14, 2.23, 2.27, 1.9, 1.72, 1.56,
    1.55, 1.39, 1.34, 1.47, 1.47, 1.96, 2.56, 2.24, 2.13, 1.96, 1.98, 2.17, 1.96, 1.82, 1.89, 1.94, 2.03, 2.24, 1.81, 2.41, 2.3, 2.27, 2.16, 1.66,
    1.34, 1.41, 1.24, 1.18, 1.52, 1.57, 2.2, 1.92, 1.96, 2.26, 2.32, 2.09, 2.01, 2.01, 2.08, 1.85, 2.06, 2.08, 2.29, 2.17, 2.3, 2.13, 1.98, 1.8, 1.64,
    1.42, 1.37, 1.42, 1.34, 1.56, 2.09, 2.26, 2.31, 2.4, 2.78, 2.38, 2.45, 2.21, 2, 2.2, 1.89, 2.37, 2.23, 2.27, 1.92, 2.2, 1.84, 1.63, 1.35, 1.27,
    1.29, 1.43, 1.25, 1.57, 1.81, 2.04, 2.48, 2.6, 2.45, 2.21, 2.01, 2.26, 2.32, 1.98, 2.29, 1.88, 2.13, .83, 1.85, 2.01, 1.82, 1.58, 1.6, 1.44, 1.36,
    1.35, 1.41, 1.59, 2.01, 2.17, 2.47, 2.34, 2.52, 2.19, 2.3, 1.84, 2.09, 2.01, 1.98, 1.89, 2.12, 2.26, 2.1, 1.86, 1.67, 1.6, 1.52, 1.58, 1.13, 1.34,
    0, 1.89, 1.82, 2.33, 2.39, 2.65, 2.32, 2.26, 2.83, 2.23, 2.17, 1.9, 1.98, 2.67, 2.38, 2.41, 1.97, 1.99, 1.66, 1.43, 1.26, 1.21, 1.2, 1.25, 1.37,
    1.98, 2.58, 2.01, 1.92, 2.14, 1.71, 1.8, 1.62, 1.64, 1.69, 1.68, 1.97, 1.9, 2.24, 2.39, 2.2, 2.51, 1.82, 1.62, 2.21, 2.09, 1.76, 1.78, 1.98, 2.48,
    2.87, 2.41, 2.3, 2.05, 1.97, 2.08, 2.06, 1.96, 1.81, 1.89, 2.33, 2.23, 2.56, 2.58, 2.4, 2.1, 1.83, 1.6, 1.58, 1.8, 1.7, 1.88, 4.35, 5.17, 5.62,
    2.67, 2.39, 4.93, 5.2, 5.36, 4.44, 4.52, 3.97, 3.66, 3.6, 3.3, 3.4, 3.27, 3.09, 3.05, 2.75, 2.64, 2.37, 2.36, 2.32, 2.35, 2.44, 2.9, 2.79, 3.14,
    2.54, 2.62, 2.77, 2.51, 2.39, 2.16, 2.33, 2.28, 2.25, 2.46, 2.68, 2.9, 2.51, 2.45, 2.29, 2.06, 1.99, 1.84, 1.87, 1.8, 1.92, 2.42, 2.79, 2.58,
    2.16, 2.19, 2.28, 2.31, 2.14, 2.23, 2.03, 2.15, 2.34, 2.44, 2.47, 2.55, 2.53, 2.5, 1.98, 1.93, 1.89, 1.69, 1.88, 1.73, 1.89, 1.8, 2.2, 2.62, 2.6,
    2.69, 2.71, 2.19, 2.48, 2.46, 2.36, 2.15, 2.52, 2.32, 2.25, 2.28, 2.27, 2.22, 1.78, 2, 1.79, 1.85, 1.85, 1.79, 2.14, 2.94, 3.66, 4.28, 4.43, 4.22,
    4.09, 3.92, 3.88, 3.78, 3.63, 3.56, 4.21, 4.34, 3.99, 3.94, 3.42, 3.45, 3.03, 2.87, 2.65, 2.57, 2.53, 2.59, 2.56, 3.12, 3.27, 2.71, 2.64, 2.56,
    2.66, 2.36, 2.37, 2.63, 2.68, 2.86, 3.16, 3.03, 3.2, 3.25, 3.23, 3.14, 2.57, 2.68, 2.45, 2.45, 2.94, 3.45, 3.6, 4.19, 3.73, 3.95, 3.74, 4.51,
    4.19, 3.97, 3.66, 3.69, 3.55, 3.44, 3.44, 3.34, 3.4, 3.51, 3.06, 2.95, 2.93, 2.66, 2.55, 2.47, 2.53, 2.53, 2.59, 3.07, 3.11, 2.78, 2.75, 2.78,
    2.79, 2.52, 2.47, 2.55, 2.5, 2.58, 2.44, 2.64, 2.72, 2.89, 2.97, 2.58, 2.43, 2.3, 1.93, 2, 1.99, 2.1, 2.18, 2.91, 2.84, 2.66, 2.58, 2.72, 2.44,
    2.54, 2.34, 2.29, 2.46, 2.29, 2.53, 2.47, 2.49, 2.5, 2.73, 2.57, 2.38, 2.15, 2.09, 1.93, 1.93, 1.99, 2.05, 2.48, 2.85, 2.53, 2.64, 2.53, 2.6,
    2.35, 2.31, 2.41, 2.29, 2.39, 2.19, 2.35, 2.54, 2.77, 2.55, 2.45, 2.3, 2.13, 1.9, 1.84, 1.95, 1.81, 1.92, 1.8, 2.44, 2.59, 2.89, 2.64, 2.73, 2.52,
    2.39, 2.47, 2.74, 2.48, 2.83, 2.36, 2.4, 2.43, 2.51, 2.22, 2.26, 1.94, 1.98, 1.83, 1.75, 1.74, 1.92, 1.9, 2.18, 2.74, 2.69, 2.6, 2.24, 2.41, 2.38,
    2.45, 2.26, 2.55, 2.6, 2.64, 2.58, 2.43, 2.51, 2.54, 2.03, 1.92, 1.83, 1.63, 1.67, 1.7, 2.02, 2.38, 3.13, 2.76, 2.88, 2.83, 2.63, 2.4, 2.42, 2.35,
    2.42, 2.59, 2.53, 2.69, 2.65, 2.7, 2.38, 2.54, 2.38, 2.09, 2.01, 1.98, 1.94, 1.89, 1.98, 2.65, 2.82, 2.34, 2.58, 2.45, 1.98, 2.65, 3.07, 3.11,
    2.78, 2.75, 2.78, 2.79, 2.52, 2.47, 2.55, 2.5, 2.58, 2.44, 2.64, 2.72, 2.89, 2.97, 2.58, 2.43, 2.3, 1.93, 2, 1.99, 2.1, 2.18, 2.91, 2.84, 2.66,
    2.58, 2.72, 2.44, 2.54, 2.34, 2.29, 2.46, 1.89, 1.69, 1.88, 1.73, 1.89, 1.8, 2.2, 2.62, 2.6, 2.69, 2.71, 2.19, 2.48, 2.46, 2.36, 2.15, 2.52, 2.32,
    2.25, 2.28, 2.27, 2.22, 1.78, 2, 1.79, 1.85, 1.85, 1.79, 2.14, 2.94, 3.66, 4.28, 4.43, 4.22, 4.09, 3.92, 3.88, 3.78, 3.63, 3.56, 4.21, 4.34, 3.99,
    3.94, 3.42, 3.45, 3.03, 2.87, 2.93, 2.66, 2.55, 2.47, 2.53, 2.53, 2.59, 3.07, 3.11, 2.78, 2.75, 2, 1.7, 1.91, 1.59, 1.63, 1.71, 1.66, 1.85, 1.77,
    2.15, 2.16, 2.39, 2.15, 2.23, 2.04, 1.79, 1.79, 1.79, 1.07, 1.67, 1.67, 2.21, 2.44, 2.3, 2.13, 1.83, 1.91, 1.76, 1.63, 1.64, 1.8, 1.82, 1.9, 1.88,
    2.07, 2.06, 2.19, 2.12, 1.78, 1.78, 1.78, 1.39, 1.25, 1.29, 1.46, 2.04, 2.64, 2.1, 2.16, 1.98, 1.93, 1.97, 1.91, 1.77, 1.59, 2.19, 2.04, 2.24,
    2.19, 2.23, 2.18, 1.99, 1.99, 1.38, 1.43, 1.38, 1.37, 1.47, 1.02, 1.02, 2.09, 2.07, 2.2, 2.46, 2.41, 2.57, 2.41, 1.92, 1.88, 1.75, 1.78, 1.95,
    2.1, 1.92, 2.07, 1.85, 1.94, 1.51, 1.47, 1.39, 1.41, 1.35, 1.39, 1.43, 1.85, 2.07, 2.49, 2.31, 1.96, 2.1, 2, 2.16, 2.05, 2.29, 2.16, 2.38, 2.5,
    2.42, 1.84, 2.12, 1.75, 1.42, 1.49, 1.3, 1.26, 1.36, 1.41, 2.21, 2.21, 1.93, 2.07, 2.31, 2.36, 1.91, 1.77, 1.69, 1.8, 2.03, 2.18, 2.33, 2.14,
    2.23, 2.27, 1.9, 1.72, 1.56, 1.55, 1.39, 1.34, 1.47, 1.47, 1.96, 2.56, 2.24, 2.13, 1.96, 1.98, 2.17, 1.96, 1.82, 1.89, 1.94, 2.03, 2.24, 1.81,
    2.41, 2.3, 2.27, 2.16, 1.66, 1.34, 1.41, 1.24, 1.18, 1.52, 1.57, 2.2, 1.92, 1.96, 2.26, 2.32, 2.09, 2.01, 2.01, 2.08, 1.85, 2.06, 2.08, 2.29,
    2.17, 2.3, 2.13, 1.98, 1.8, 1.64, 1.42, 1.37, 1.42, 1.34, 1.56, 2.09, 2.26, 2.31, 2.4, 2.78, 2.38, 2.45, 2.21, 2, 2.2, 1.89, 2.37, 2.23, 2.27,
    1.92, 2.2, 1.84, 1.63, 1.35, 1.27, 1.29, 1.43, 1.25, 1.57, 1.81, 2.04, 2.48, 2.6, 2.45, 2.21, 2.01, 2.26, 2.32, 1.98, 2.29, 1.88, 2.13, .83, 1.85,
    2.01, 1.82, 1.58, 1.6, 1.44, 1.36, 1.35, 1.41, 1.59, 2.01, 2.17, 2.47, 2.34, 2.52, 2.19, 2.3, 1.84, 2.09, 2.01, 1.98, 1.89, 2.12, 2.26, 2.1, 1.86,
    1.67, 1.6, 1.52, 1.58, 1.13, 1.34, 0, 1.89, 1.82, 2.33, 2.39, 2.65, 2.32, 2.26, 2.83, 2.23, 2.17, 1.9, 1.98, 2.67, 2.38, 2.41, 1.97, 1.99, 1.66,
    1.43, 1.26, 1.21, 1.2, 1.25, 1.37, 1.98, 2.58, 2.01, 1.92, 2.14, 1.71, 1.8, 1.62, 1.64, 1.69, 1.68, 1.97, 1.9, 2.24, 2.39, 2.2, 2.51, 1.82, 1.62,
    2.21, 2.09, 1.76, 1.78, 1.98, 2.48, 2.87, 2.41, 2.3, 2.05, 1.97, 2.08, 2.06, 1.96, 1.81, 1.89, 2.33, 2.23, 2.56, 2.58, 2.4, 2.1, 1.83, 1.6, 1.58,
    1.8, 1.7, 1.88, 4.35, 5.17, 5.62, 2.67, 2.39, 4.93, 5.2, 5.36, 4.44, 4.52, 3.97, 3.66, 3.6, 3.3, 3.4, 3.27, 3.09, 3.05, 2.75, 2.64, 2.37, 2.36,
    2.32, 2.35, 2.44, 2.9, 2.79, 3.14, 2.54, 2.62, 2.77, 2.51, 2.39, 2.16, 2.33, 2.28, 2.25, 2.46, 2.68, 2.9, 2.51, 2.45, 2.29, 2.06, 1.99, 1.84,
    1.87, 1.8, 1.92, 2.42, 2.79, 2.58, 2.16, 2.19, 2.28, 2.31, 2.14, 2.23, 2.03, 2.15, 2.34, 2.44, 2.47, 2.55, 2.53, 2.5, 1.98, 1.93, 1.89, 1.69,
    1.88, 1.73, 1.89, 1.8, 2.2, 2.62, 2.6, 2.69, 2.71, 2.19, 2.48, 2.46, 2.36, 2.15, 2.52, 2.32, 2.25, 2.28, 2.27, 2.22, 1.78, 2, 1.79, 1.85, 1.85,
    1.79, 2.14, 2.94, 3.66, 4.28, 4.43, 4.22, 4.09, 3.92, 3.88, 3.78, 3.63, 3.56, 4.21, 4.34, 3.99, 3.94, 3.42, 3.45, 3.03, 2.87, 2.65, 2.57, 2.53,
    2.59, 2.56, 3.12, 3.27, 2.71, 2.64, 2.56, 2.66, 2.36, 2.37, 2.63, 2.68, 2.86, 3.16, 3.03, 3.2, 3.25, 3.23, 3.14, 2.57, 2.68, 2.45, 2.45, 2.94,
    3.45, 3.6, 4.19, 3.73, 3.95, 3.74, 4.51, 4.19, 3.97, 3.66, 3.69, 3.55, 3.44, 3.44, 3.34, 3.4, 3.51, 3.06, 2.95, 2.93, 2.66, 2.55, 2.47, 2.53,
    2.53, 2.59, 3.07, 3.11, 2.78, 2.75, 2.78, 2.79, 2.52, 2.47, 2.55, 2.5, 2.58, 2.44, 2.64, 2.72, 2.89, 2.97, 2.58, 2.43, 2.3, 1.93, 2, 1.99, 2.1,
    2.18, 2.91, 2.84, 2.66, 2.58, 2.72, 2.44, 2.54, 2.34, 2.29, 2.46, 2.29, 2.53, 2.47, 2.49, 2.5, 2.73, 2.57, 2.38, 2.15, 2.09, 1.93, 1.93, 1.99,
    2.05, 2.48, 2.85, 2.53, 2.64, 2.53, 2.6, 2.35, 2.31, 2.41, 2.29, 2.39, 2.19, 2.35, 2.54, 2.77, 2.55, 2.45, 2.3, 2.13, 1.9, 1.84, 1.95, 1.81, 1.92,
    1.8, 2.44, 2.59, 2.89, 2.64, 2.73, 2.52, 2.39, 2.47, 2.74, 2.48, 2.83, 2.36, 2.4, 2.43, 2.51, 2.22, 2.26, 1.94, 1.98, 1.83, 1.75, 1.74, 1.92, 1.9,
    2.18, 2.74, 2.69, 2.6, 2.24, 2.41, 2.38, 2.45, 2.26, 2.55, 2.6, 2.64, 2.58, 2.43, 2.51, 2.54, 2.03, 1.92, 1.83, 1.63, 1.67, 1.7, 2.02, 2.38, 3.13,
    2.76, 2.88, 2.83, 2.63, 2.4, 2.42, 2.35, 2.42, 2.59, 2.53, 2.69, 2.65, 2.7, 2.38, 2.54, 2.38, 2.09, 2.01, 1.98, 1.94, 1.89, 1.98, 2.65, 2.82,
    2.34, 2.58, 2.45, 1.98, 2.65, 3.07, 3.11, 2.78, 2.75, 2.78, 2.79, 2.52, 2.47, 2.55, 2.5, 2.58, 2.44, 2.64, 2.72, 2.89, 2.97, 2.58, 2.43, 2.3,
    1.93, 2, 1.99, 2.1, 2.18, 2.91, 2.84, 2.66, 2.58, 2.72, 2.44, 2.54, 2.34, 2.29, 2.46, 1.89, 1.69, 1.88, 1.73, 1.89, 1.8, 2.2, 2.62, 2.6, 2.69,
    2.71, 2.19, 2.48, 2.46, 2.36, 2.15, 2.52, 2.32, 2.25, 2.28, 2.27, 2.22, 1.78, 2, 1.79, 1.85, 1.85, 1.79, 2.14, 2.94, 3.66, 4.28, 4.43, 4.22, 4.09,
    3.92, 3.88, 3.78, 3.63, 3.56, 4.21, 4.34, 3.99, 3.94, 3.42, 3.45, 3.03, 2.87, 2.93, 2.66, 2.55, 2.47, 2.53, 2.53, 2.59, 3.07, 3.11, 2.78, 2.75, 2,
    1.7, 1.91, 1.59, 1.63, 1.71, 1.66, 1.85, 1.77, 2.15, 2.16, 2.39, 2.15, 2.23, 2.04, 1.79, 1.79, 1.79, 1.07, 1.67, 1.67, 2.21, 2.44, 2.3, 2.13,
    1.83, 1.91, 1.76, 1.63, 1.64, 1.8, 1.82, 1.9, 1.88, 2.07, 2.06, 2.19, 2.12, 1.78, 1.78, 1.78, 1.39, 1.25, 1.29, 1.46, 2.04, 2.64, 2.1, 2.16, 1.98,
    1.93, 1.97, 1.91, 1.77, 1.59, 2.19, 2.04, 2.24, 2.19, 2.23, 2.18, 1.99, 1.99, 1.38, 1.43, 1.38, 1.37, 1.47, 1.02, 1.02, 2.09, 2.07, 2.2, 2.46,
    2.41, 2.57, 2.41, 1.92, 1.88, 1.75, 1.78, 1.95, 2.1, 1.92, 2.07, 1.85, 1.94, 1.51, 1.47, 1.39, 1.41, 1.35, 1.39, 1.43, 1.85, 2.07, 2.49, 2.31,
    1.96, 2.1, 2, 2.16, 2.05, 2.29, 2.16, 2.38, 2.5, 2.42, 1.84, 2.12, 1.75, 1.42, 1.49, 1.3, 1.26, 1.36, 1.41, 2.21, 2.21, 1.93, 2.07, 2.31, 2.36,
    1.91, 1.77, 1.69, 1.8, 2.03, 2.18, 2.33, 2.14, 2.23, 2.27, 1.9, 1.72, 1.56, 1.55, 1.39, 1.34, 1.47, 1.47, 1.96, 2.56, 2.24, 2.13, 1.96, 1.98,
    2.17, 1.96, 1.82, 1.89, 1.94, 2.03, 2.24, 1.81, 2.41, 2.3, 2.27, 2.16, 1.66, 1.34, 1.41, 1.24, 1.18, 1.52, 1.57, 2.2, 1.92, 1.96, 2.26, 2.32,
    2.09, 2.01, 2.01, 2.08, 1.85, 2.06, 2.08, 2.29, 2.17, 2.3, 2.13, 1.98, 1.8, 1.64, 1.42, 1.37, 1.42, 1.34, 1.56, 2.09, 2.26, 2.31, 2.4, 2.78, 2.38,
    2.45, 2.21, 2, 2.2, 1.89, 2.37, 2.23, 2.27, 1.92, 2.2, 1.84, 1.63, 1.35, 1.27, 1.29, 1.43, 1.25, 1.57, 1.81, 2.04, 2.48, 2.6, 2.45, 2.21, 2.01,
    2.26, 2.32, 1.98, 2.29, 1.88, 2.13, .83, 1.85, 2.01, 1.82, 1.58, 1.6, 1.44, 1.36, 1.35, 1.41, 1.59, 2.01, 2.17, 2.47, 2.34, 2.52, 2.19, 2.3, 1.84,
    2.09, 2.01, 1.98, 1.89, 2.12, 2.26, 2.1, 1.86, 1.67, 1.6, 1.52, 1.58, 1.13, 1.34, 0, 1.89, 1.82, 2.33, 2.39, 2.65, 2.32, 2.26, 2.83, 2.23, 2.17,
    1.9, 1.98, 2.67, 2.38, 2.41, 1.97, 1.99, 1.66, 1.43, 1.26, 1.21, 1.2, 1.25, 1.37, 1.98, 2.58, 2.01, 1.92, 2.14, 1.71, 1.8, 1.62, 1.64, 1.69, 1.68,
    1.97, 1.9, 2.24, 2.39, 2.2, 2.51, 1.82, 1.62, 2.21, 2.09, 1.76, 1.78, 1.98, 2.48, 2.87, 2.41, 2.3, 2.05, 1.97, 2.08, 2.06, 1.96, 1.81, 1.89, 2.33,
    2.23, 2.56, 2.58, 2.4, 2.1, 1.83, 1.6, 1.58, 1.8, 1.7, 1.88, 4.35, 5.17, 5.62, 2.67, 2.39, 4.93, 5.2, 5.36, 4.44, 4.52, 3.97, 3.66, 3.6, 3.3, 3.4,
    3.27, 3.09, 3.05, 2.75, 2.64, 2.37, 2.36, 2.32, 2.35, 2.44, 2.9, 2.79, 3.14, 2.54, 2.62, 2.77, 2.51, 2.39, 2.16, 2.33, 2.28, 2.25, 2.46, 2.68,
    2.9, 2.51, 2.45, 2.29, 2.06, 1.99, 1.84, 1.87, 1.8, 1.92, 2.42, 2.79, 2.58, 2.16, 2.19, 2.28, 2.31, 2.14, 2.23, 2.03, 2.15, 2.34, 2.44, 2.47,
    2.55, 2.53, 2.5, 1.98, 1.93, 1.89, 1.69, 1.88, 1.73, 1.89, 1.8, 2.2, 2.62, 2.6, 2.69, 2.71, 2.19, 2.48, 2.46, 2.36, 2.15, 2.52, 2.32, 2.25, 2.28,
    2.27, 2.22, 1.78, 2, 1.79, 1.85, 1.85, 1.79, 2.14, 2.94, 3.66, 4.28, 4.43, 4.22, 4.09, 3.92, 3.88, 3.78, 3.63, 3.56, 4.21, 4.34, 3.99, 3.94, 3.42,
    3.45, 3.03, 2.87, 2.65, 2.57, 2.53, 2.59, 2.56, 3.12, 3.27, 2.71, 2.64, 2.56, 2.66, 2.36, 2.37, 2.63, 2.68, 2.86, 3.16, 3.03, 3.2, 3.25, 3.23,
    3.14, 2.57, 2.68, 2.45, 2.45, 2.94, 3.45, 3.6, 4.19, 3.73, 3.95, 3.74, 4.51, 4.19, 3.97, 3.66, 3.69, 3.55, 3.44, 3.44, 3.34, 3.4, 3.51, 3.06,
    2.95, 2.93, 2.66, 2.55, 2.47, 2.53, 2.53, 2.59, 3.07, 3.11, 2.78, 2.75, 2.78, 2.79, 2.52, 2.47, 2.55, 2.5, 2.58, 2.44, 2.64, 2.72, 2.89, 2.97,
    2.58, 2.43, 2.3, 1.93, 2, 1.99, 2.1, 2.18, 2.91, 2.84, 2.66, 2.58, 2.72, 2.44, 2.54, 2.34, 2.29, 2.46, 2.29, 2.53, 2.47, 2.49, 2.5, 2.73, 2.57,
    2.38, 2.15, 2.09, 1.93, 1.93, 1.99, 2.05, 2.48, 2.85, 2.53, 2.64, 2.53, 2.6, 2.35, 2.31, 2.41, 2.29, 2.39, 2.19, 2.35, 2.54, 2.77, 2.55, 2.45,
    2.3, 2.13, 1.9, 1.84, 1.95, 1.81, 1.92, 1.8, 2.44, 2.59, 2.89, 2.64, 2.73, 2.52, 2.39, 2.47, 2.74, 2.48, 2.83, 2.36, 2.4, 2.43, 2.51, 2.22, 2.26,
    1.94, 1.98, 1.83, 1.75, 1.74, 1.92, 1.9, 2.18, 2.74, 2.69, 2.6, 2.24, 2.41, 2.38, 2.45, 2.26, 2.55, 2.6, 2.64, 2.58, 2.43, 2.51, 2.54, 2.03, 1.92,
    1.83, 1.63, 1.67, 1.7, 2.02, 2.38, 3.13, 2.76, 2.88, 2.83, 2.63, 2.4, 2.42, 2.35, 2.42, 2.59, 2.53, 2.69, 2.65, 2.7, 2.38, 2.54, 2.38, 2.09, 2.01,
    1.98, 1.94, 1.89, 1.98, 2.65, 2.82, 2.34, 2.58, 2.45, 1.98, 2.65, 3.07, 3.11, 2.78, 2.75, 2.78, 2.79, 2.52, 2.47, 2.55, 2.5, 2.58, 2.44, 2.64,
    2.72, 2.89, 2.97, 2.58, 2.43, 2.3, 1.93, 2, 1.99, 2.1, 2.18, 2.91, 2.84, 2.66, 2.58, 2.72, 2.44, 2.54, 2.34, 2.29, 2.46, 1.89, 1.69, 1.88, 1.73,
    1.89, 1.8, 2.2, 2.62, 2.6, 2.69, 2.71, 2.19, 2.48, 2.46, 2.36, 2.15, 2.52, 2.32, 2.25, 2.28, 2.27, 2.22, 1.78, 2, 1.79, 1.85, 1.85, 1.79, 2.14,
    2.94, 3.66, 4.28, 4.43, 4.22, 4.09, 3.92, 3.88, 3.78, 3.63, 3.56, 4.21, 4.34, 3.99, 3.94, 3.42, 3.45, 3.03, 2.87, 2.93, 2.66, 2.55, 2.47, 2.53,
    2.53, 2.59, 3.07, 3.11, 2.78, 2.75 };

// For SmartFlume testing
static float vertical_mount[SAMPLE_DATA_TYPE_MAX] = {
		30.0,  // iTracker demo
		16.0,  // 4 + 12
		18.0,  // 6 + 12
		20.0,  // 8 + 12
		22.0,  // 10 + 12
		24.0   // 12 + 12
};

static float pipe_id[SAMPLE_DATA_TYPE_MAX] = {
		8.0,  // iTracker demo
		4.0,  // 4" flume
		6.0,  // 6" flume
		8.0,  // 8" flume
		10.0,  // 10" flume
		12.0   // 12" flume
};



static float get_distance(int DataType, int i)
{
	float result;
	switch (DataType)
	{
	default:
	case SAMPLE_DATA_TYPE_PIPE_8:    result = MainConfig.vertical_mount - sample_data[i]; break;
	case SAMPLE_DATA_TYPE_FLUME_4:   result = MainConfig.vertical_mount - (i % 4); break;
	case SAMPLE_DATA_TYPE_FLUME_6:   result = MainConfig.vertical_mount - (i % 6); break;
	case SAMPLE_DATA_TYPE_FLUME_8:   result = MainConfig.vertical_mount - (i % 8); break;
	case SAMPLE_DATA_TYPE_FLUME_10:  result = MainConfig.vertical_mount - (i % 10); break;
	case SAMPLE_DATA_TYPE_FLUME_12:  result = MainConfig.vertical_mount - (i % 12); break;
	}
	return result;
}




// To fill memory:  LOG_TOP - LOG_BOTTOM + 1 =  0xFFFFFF - 0x200000 + 1 = 0xDFFFFF + 1 = 0xE00000 = 14,680,064 bytes
// Each record is 32 bytes long, so 14,680,064 / 32 = 458,752 records.
// If using sample data set:  458,752  / 2880 = 159 cycles.
// Each record takes a minimum of 20 ms so 2880 readings is about 1 minute.
// To fill memory would take a minimum of 2.65 hours.
//
// Effectively puts the iTracker into a clean DEMO mode.
// Create 30 days of samples at a 15 minute interval.
void loadSampleData(uint8_t HowManyCycles, int DataType)
{
	struct tm tmpdate;
	int totalReadings = 2880; // 30 days, 4*24 readings per day (every 15 mins)
#ifdef ENABLE_ELAPSED_TIME
	uint32_t elapsed_start;
	uint32_t elapsed_stop;
	uint32_t elapsed_time;

	elapsed_start = rtc_get_time();
#endif

	int prev_pattern;

	prev_pattern = led_set_pattern(LED_PATTERN_FAST_YEL);


	// Get the date 30 days ago.
	uint32_t startDt = rtc_get_time() - ((uint32_t)HowManyCycles * SEC_30DAYS);
	sec_to_dt(&startDt, &tmpdate);
	// Shift to midnight
	tmpdate.tm_hour = 0;
	tmpdate.tm_min  = 0;
	tmpdate.tm_sec  = 0;
	startDt = convertDateToTime(&tmpdate);
	// End shift to midnight
	LastLogHour = tmpdate.tm_hour;
	LastLogDay = tmpdate.tm_mday;

	// Setup DEMO configuration settings.
	MainConfig.vertical_mount = vertical_mount[DataType]; // 30.0;
	MainConfig.pipe_id        = pipe_id[DataType];        //  8.0;
	MainConfig.log_interval   = 900; // 15 Mins
	MainConfig.population     = 1118;

	// Clear all logs.
	log_start(startDt);

	// Reset the DeltaQ
	//Delta_Q_Reset();

	// Save settings.
	Save_MainConfig();

	// Default temp and gain for test.
	//CurrentLog.temperature = 76.5;
	//CurrentLog.gain = 185;

	// zg - timing measurements for 2880 sample points per cycle:
	//    With erasing each sector: ~163 s
	//    With no erase:              ~2 s
	// So, for a 1 second toggle update on the LED:
	// 1 s * (2880 / 163 s) = 17.7 records.
	//
	// Note: a bulk erase takes 48 seconds, so there is a lot of
	// overhead when doing an erase for each sector...

	for (uint8_t cycles = 0; cycles < HowManyCycles; cycles++)
	{
		// Create the 30 days of readings/log entries.
		for (int i = 0; i < totalReadings; i++)
		{
			// compute distance from sample level data
			//LastGoodMeasure = MainConfig.vertical_mount - sample_data[i];
			LastGoodMeasure = get_distance(DataType, i);

			// Create the entry.
			log_write(startDt, 76.5, LastGoodMeasure, 185, MainConfig.vertical_mount, LOG_DO_NOT_WRITE_LOG_TRACK);

			// Next reading.
			startDt += 900; // 15 mins.

			led_heartbeat();

		}
	}
	BSP_Log_Track_Write(1);  // update battery accumulator value when requested

#if 0
	// For testing, write a CSV file
	if (DataType > SAMPLE_DATA_TYPE_PIPE_8)
	{
	  extern void save_log_to_sd_card_as_csv(void);
	  save_log_to_sd_card_as_csv();
	}
#endif


	led_set_pattern(prev_pattern);


#ifdef ENABLE_ELAPSED_TIME
	elapsed_stop = rtc_get_time();
	elapsed_time = elapsed_stop - elapsed_start;
#endif
}

// for debugging, step through data one record at a time
void viewData(void)
{

	log_t log_rec;
	uint32_t addr;
	uint32_t expected_log_number = 0;
#ifdef LOG_DOES_NOT_WRAP
	addr = LOG_BOTTOM;
#else
	addr = LogTrack_first_logrec_addr;
#endif

	if (addr == 0)
	{
		return;
	}

	while (addr)
	{

		log_GetRecord(&log_rec, addr, LOG_FORWARD);

		if (expected_log_number == 0)
		{
			expected_log_number = 1;
		}

		// are log numbers sequential?
		if (log_rec.log_num != (expected_log_number))
		{
			while (1);
		}

		expected_log_number++;


		addr += LOG_SIZE;
		if (addr >= LogTrack.next_logrec_addr)
		{
			return;
		}
	}

}




//#define ENABLE_CSV_TRACE


TCHAR * parse_csv_text(char *pLine, char *pText, int Text_MaxLen)
{
	int text_len_remaining;

#ifdef ENABLE_CSV_TRACE  // for development, show how the line is consumed, piece by piece
    {
    	char msg[120];
    	sprintf(msg, "%s", pLine);
    	trace_AppendMsg(msg);
    }
#endif

	text_len_remaining = Text_MaxLen - 1;

	// Parse text
	while ((*pLine) && (*pLine != ',') && (*pLine != '\n'))
	{
        led_heartbeat();
		if ((text_len_remaining) && (*pLine != '\r'))  // skip CR
		{
		  *pText = *pLine;
		  pText++;
		  text_len_remaining--;
		}
		pLine++;
	}
	*pText = '\0';  // null-terminate label
	if (*pLine == ',') pLine++;  // skip the comma, leave the LF if any

	return pLine;  // note: this may point to a '\n' which is considered to be the end-of-a-line and is not skipped here ?? or should it be consumed??
}


static TCHAR * parse_label_value_pair(char *pLine, char *pLabel, int Label_MaxLen, char *pValue, int Value_MaxLen)
{
	TCHAR * next_pos;

	next_pos = pLine;

	next_pos = parse_csv_text(next_pos, pLabel, Label_MaxLen);
	next_pos = parse_csv_text(next_pos, pValue, Value_MaxLen);

	return next_pos;  // return however far we got when parsing
}

//Site ID,Site 001,,,,
static int parse_header_line_01(TCHAR * pLine)
{
	char label[32];
	char value[32];

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));

    if (strcmp(label,"Site ID") != 0) return 0;  // unexpected label

    // null terminate the value at the maximum len allowed
    value[sizeof(MainConfig.site_name) - 1] = '\0';
    strcpy(MainConfig.site_name, value);

    return 1;
}


//Location,Site 001,,,,
static int parse_header_line_02(TCHAR * pLine)
{
	char label[32];
	char value[32];

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));

    if (strcmp(label,"Location") != 0) return 0;  // unexpected label

#if 0 // this is a duplicate of line 01
    // null terminate the value at the maximum len allowed
    value[sizeof(MainConfig.site_name) - 1] = '\0';
    strcpy(MainConfig.site_name, value);
#endif

    return 1;
}
//Population,66,,,,
static int parse_header_line_03(TCHAR * pLine)
{
	char label[32];
	char value[32];

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));

    if (strcmp(label,"Population") != 0) return 0;  // unexpected label

    MainConfig.population =  atoi(value);

    return 1;
}

//Pipe ID,8,Vertical Mount,23,Damping,10
static int parse_header_line_04(TCHAR * pLine)
{
	char label[32];
	char value[32];

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));
    if (strcmp(label,"Pipe ID") != 0) return 0;  // unexpected label

    MainConfig.pipe_id =  atof(value);

    if (pLine == NULL) return 0;

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));
    if (strcmp(label,"Vertical Mount") != 0) return 0;  // unexpected label

    MainConfig.vertical_mount =  atof(value);  // This depends on units, which is read later

    if (pLine == NULL) return 0;
    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));
    if (strcmp(label,"Damping") != 0) return 0;  // unexpected label

    MainConfig.damping =  atoi(value);

    return 1;
}


//Download Date,6/27/2016 15:06,Last Download Date,6/24/2016 10:25,,
static int parse_header_line_05(TCHAR * pLine)
{
	char label[32];
	char value[32];

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));
    if (strcmp(label,"Download Date") != 0) return 0;  // unexpected label

    // nothing to store for Download Date

    // Parse Last Download Date
    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));
    if (strcmp(label,"Last Download Date") != 0) return 0;  // unexpected label

    // Save in LogTrack
    LogTrack.last_download_datetime = parse_timestamp(value);

    return 1;
}

//FW Ver,1.3.4,MAC,00:00:00:00:00:00,,
static int parse_header_line_06(TCHAR * pLine)
{
	// don't care about any data on this line, skip it and move on
    return 1;
}


//Battery Percent,76.29,Battery Current,12207.27,Estimated Battery Life,34.11 days
static int parse_header_line_07(TCHAR * pLine)
{
	// don't care about any data on this line, skip it and move on
    return 1;
}

//Units,English,Log Interval,5 minutes,Logged Length,1509
static int parse_header_line_08(TCHAR * pLine)
{
	char label[32];
	char value[32];

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));
    if (strcmp(label,"Units") != 0) return 0;  // unexpected label

    if (strcmp(value,"English")==0)
    {
      MainConfig.units = UNITS_ENGLISH;
    }
    else
    {
        //MainConfig.units = UNITS_METRIC;
    	// cannot import other units, too many conversion problems, just stop
        AppendStatusMessage("Units must be English for CSV import.");

        return 0;
    }

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));
    if (strcmp(label,"Log Interval") != 0) return 0;  // unexpected label

#if 0
	strChkNum(value);
	int tmp_int_val = atoi(value);
	if ((tmp_int_val == 1) || (tmp_int_val == 5) || (tmp_int_val == 10) || (tmp_int_val == 15) || (tmp_int_val == 30) || (tmp_int_val == 60))
	{
		MainConfig.log_interval = tmp_int_val * 60;
	}
#endif

    MainConfig.log_interval = atoi(value) * 60;

    // Don't care about Logged Length

    return 1;
}

//Record #,DateTime,Level,Distance,Gain,Temp
static int parse_header_line_09(TCHAR * pLine)
{
	char label[32];

	pLine = parse_csv_text(pLine, label, sizeof(label));
    if (strcmp(label,"Record #") != 0) return 0;  // unexpected label

	pLine = parse_csv_text(pLine, label, sizeof(label));
    if (strcmp(label,"DateTime") != 0) return 0;  // unexpected label

	pLine = parse_csv_text(pLine, label, sizeof(label));
    if (strcmp(label,"Level") != 0) return 0;  // unexpected label

	pLine = parse_csv_text(pLine, label, sizeof(label));
    if (strcmp(label,"Distance") != 0) return 0;  // unexpected label

	pLine = parse_csv_text(pLine, label, sizeof(label));
    if (strcmp(label,"Gain") != 0) return 0;  // unexpected label

	pLine = parse_csv_text(pLine, label, sizeof(label));
    if (strcmp(label,"Temp") != 0) return 0;  // unexpected label

    return 1;

}



//The header is 8 lines long and looks like this:
//
//Site ID,Site 001,,,,
//Location,Site 001,,,,
//Population,66,,,,
//Pipe ID,8,Vertical Mount,23,Damping,10
//Download Date,6/27/2016 15:06,Last Download Date,6/24/2016 10:25,,
//FW Ver,1.3.4,MAC,00:00:00:00:00:00,,
//Battery Percent,76.29,Battery Current,12207.27,Estimated Battery Life,34.11 days
//Units,English,Log Interval,5 minutes,Logged Length,1509
//
static int import_header(FIL * pSDFile)
{

	TCHAR * pLine;
	char rtext[100]; /* File read buffer */


	// read a line terminated with '\n' up to max size.  Strip '\r'. Return a null-terminated string usually ending with '\n' unless EOF is reached.
	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_header_line_01(pLine)) return 0;

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_header_line_02(pLine)) return 0;

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_header_line_03(pLine)) return 0;

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_header_line_04(pLine)) return 0;

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_header_line_05(pLine)) return 0;

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_header_line_06(pLine)) return 0;

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_header_line_07(pLine)) return 0;

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_header_line_08(pLine)) return 0;

	return 1;
}




// The data section looks like this:
//Record #,DateTime,Level,Distance,Gain,Temp
//1,8/27/2014 13:15,2.86129584,20.13870416,50,72
//2,8/27/2014 13:30,2.96151696,20.03848304,50,72
//3,8/27/2014 13:45,2.76765984,20.23234016,50,72
//4,8/27/2014 14:00,3.17145744,19.82854256,50,72
static int import_data(FIL * pSDFile)  // how about expected_length ??
{
	TCHAR * pLine;
	char rtext[100]; /* File read buffer */
	char value[32];
	//int rec_no;
	uint32_t timestamp;
	float level;
	float distance;
	float vertical_mount;
	int gain;
	float temperature;
	int init_log = 1;
	int cnt = 0;


	// Verify the headings

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_header_line_09(pLine)) return 0;

	// Parse the remaining data lines
	pLine = f_gets(rtext, sizeof(rtext), pSDFile);
	while (pLine)
	{
        led_heartbeat();

		// For development
    	//trace_AppendMsg(pLine);

		// Record # (not stored in log)
		pLine = parse_csv_text(pLine, value, sizeof(value));
		//rec_no = atoi(value);

		// DateTime
		pLine = parse_csv_text(pLine, value, sizeof(value));
		timestamp = parse_timestamp(value);

		if (init_log)
		{

			// Use the first timestamp to initialize the log
			// Clear all logs.
			log_start(timestamp);

			// Reset the DeltaQ
			//Delta_Q_Reset();

			init_log = 0;
		}

		// Level (not stored in log) but needed to calculate vertical_mount
		pLine = parse_csv_text(pLine, value, sizeof(value));
		level = atof(value);

		// Distance
		pLine = parse_csv_text(pLine, value, sizeof(value));
		distance = atof(value);

		//level = vertical_mount - distance;
		vertical_mount = level + distance;


		// Gain
		pLine = parse_csv_text(pLine, value, sizeof(value));
		gain = atoi(value);

		// Temperature
		pLine = parse_csv_text(pLine, value, sizeof(value));
        temperature = atof(value);

        // Create a Log entry - set globals

        CurrentLog.temperature = temperature;
        //Actual_Gain = 255 - gain;
        Actual_Gain = gain;
        LastGoodMeasure = distance;  // note, level is not stored in the log
		MainConfig.vertical_mount = vertical_mount;

		// Create the entry.
		log_write(timestamp, temperature, distance, gain, vertical_mount, LOG_DO_NOT_WRITE_LOG_TRACK);

		cnt++;
		if ((cnt % 17) == 0) STATUS_LED_TOGGLE;  // this produces a roughly 1 s pulse pattern


        // Go to next line
    	pLine = f_gets(rtext, sizeof(rtext), pSDFile);
	}

	BSP_Log_Track_Write(1);  // update battery accumulator value when requested

	return 1;
}

uint8_t Import_CSV_File(const TCHAR * Filename)  // "Site001.csv"
{
	FIL SDFile;
	FRESULT sd_res;
	int result;


    // Open the CSV file
	sd_res = f_open(&SDFile, Filename, FA_READ);
    if (sd_res != FR_OK) return 0;

    // Import the header
    result = import_header(&SDFile);
    if (result)
    {
    	// Import the data
    	result = import_data(&SDFile);
    }

#ifdef ENABLE_CSV_TRACE
	trace_SaveBuffer();
#endif

    f_close(&SDFile);  // close file in any case

	//f_rename("oldname.txt", "newname.txt");

    // Perform Hydrograph Decomposition and report results to SD card
    if (result) Hydro_ProcessImportedData(Filename);

    return result;
}

#if 0 // Disable Factory Config File

//BOARD,0,                    //0=iTrackerGen4, 1=iTrackerGen5
static int parse_config_line_01(TCHAR * pLine)
{
	char label[32];
	char value[32];
	int setting;
	char msg[80];


    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));

    if (strcmp(label,"BOARD") != 0) return 0;  // unexpected label

    setting = atoi(value);
	if ((setting < 0) || (setting > 1)) setting = 0;

	SetBoard(setting);

	sprintf(msg, "Set BOARD to %d", setting);
	AppendStatusMessage(msg);

    return 1;
}

//CELL_MOD,1,                 //0=NONE, 1=TELIT, 2=XBEE3, 3=LAIRD
static int parse_config_line_02(TCHAR * pLine)
{
	char label[32];
	char value[32];
	int setting;
	char msg[80];

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));

    if (strcmp(label,"CELL_MOD") != 0) return 0;  // unexpected label


    setting = atoi(value);
	if ((setting < 0) || (setting > 3)) setting = 0;

	SetCellModule(setting);

	sprintf(msg, "Set CELL_MOD to %d", setting);
	AppendStatusMessage(msg);

    return 1;
}

//CELL_UART,1,                //1=UART1
static int parse_config_line_03(TCHAR * pLine)
{
	char label[32];
	char value[32];
	int setting;
	char msg[80];

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));

    if (strcmp(label,"CELL_UART") != 0) return 0;  // unexpected label

    setting = atoi(value);
	if ((setting < 1) || (setting > 1)) setting = 1;

	SetCellUart(setting);

	sprintf(msg, "Set CELL_UART to %d", setting);
	AppendStatusMessage(msg);

    return 1;
}

//WIFI_MOD,1,                 //0=NONE, 1=ZENTRI, 2=ESP32_WIFI, 3=ESP32_BT, 4=LAIRD_BT
static int parse_config_line_04(TCHAR * pLine)
{
	char label[32];
	char value[32];
	int setting;
	char msg[80];

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));

    if (strcmp(label,"WIFI_MOD") != 0) return 0;  // unexpected label

    setting = atoi(value);
	if ((setting < 0) || (setting > 4)) setting = 0;

	SetWiFiModule(setting);

	sprintf(msg, "Set WIFI_MOD to %d", setting);
	AppendStatusMessage(msg);

    return 1;
}

//WIFI_UART,2,                //2=UART2
static int parse_config_line_05(TCHAR * pLine)
{
	char label[32];
	char value[32];
	int setting;
	char msg[80];

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));

    if (strcmp(label,"WIFI_UART") != 0) return 0;  // unexpected label

    setting = atoi(value);
	if ((setting < 2) || (setting > 2)) setting = 2;

	SetWiFiUart(setting);

	sprintf(msg, "Set WIFI_UART to %d", setting);
	AppendStatusMessage(msg);

    return 1;
}

#if 0

static int parse_config_line_06(TCHAR * pLine)
{
	char label[32];
	char value[32];

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));

    if (strcmp(label,"CELL_APN") != 0) return 0;  // unexpected label

    // null terminate the value at the maximum len allowed
    //value[sizeof(MainConfig.site_name) - 1] = '\0';
    //strcpy(MainConfig.site_name, value);

    return 1;
}


static int parse_config_line_07(TCHAR * pLine)
{
	char label[32];
	char value[32];

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));

    if (strcmp(label,"CELL_IP") != 0) return 0;  // unexpected label

    // null terminate the value at the maximum len allowed
    //value[sizeof(MainConfig.site_name) - 1] = '\0';
    //strcpy(MainConfig.site_name, value);

    return 1;
}


static int parse_config_line_08(TCHAR * pLine)
{
	char label[32];
	char value[32];

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));

    if (strcmp(label,"WIFI_MOD") != 0) return 0;  // unexpected label

    // null terminate the value at the maximum len allowed
    //value[sizeof(MainConfig.site_name) - 1] = '\0';
    //strcpy(MainConfig.site_name, value);

    return 1;
}


static int parse_config_line_09(TCHAR * pLine)
{
	char label[32];
	char value[32];

    pLine = parse_label_value_pair(pLine, label, sizeof(label), value, sizeof(value));

    if (strcmp(label,"WIFI_UART") != 0) return 0;  // unexpected label

    // null terminate the value at the maximum len allowed
    //value[sizeof(MainConfig.site_name) - 1] = '\0';
    //strcpy(MainConfig.site_name, value);

    return 1;
}
#endif


// Maybe you cannot mux WiFi and Cell ??? How to handle async events?  E.G. make a phone call from BT or WiFil ?
// Better to have a small module with Zentri footprint that off-boards the UART2 signals the same way that Gen5 does ?
// Maybe could do WiFi then make a single phone call, then return to WiFi.  But not much feedback...
// Can Gen4 with no Zentri MUX UART1 on the daughterboard between WiFi and Cell ?

//The hardware config file looks like this
//

//CELL_MOD,1,                 //0=NONE, 1=TELIT, 2=XBEE3, 3=LAIRD
//WIFI_MOD,1,                 //0=NONE, 1=ZENTRI, 2=ESP32_WIFI, 3=ESP32_BT, 4=LAIRD_BT   CAN INFER FROM Gen4 or Gen5


// RESERVED FOR FUTURE USE...
//BOARD,0,                    //0=iTrackerGen4, 1=iTrackerGen5
//WIFI_UART,2,                //2=UART2


//CELL_UART,1,                //1=UART1
//CELL_CTX,                  List: 1,3
//CELL_APN,                  hologram
//CELL_IP,                   52.24.219.107
//SerialNo,SN001,,,,         Serial number: 123456789  up to nine characters
//UART1_MUX0,ESP32_WIFI,,,,  List of : NONE, ESP32_WIFI, ESP32_BT, LAIRD_CELL, LAIRD_BT  sets CELL_AUX line to low
//UART1_MUX1,LAIRD_CELL,,,,  List of : NONE, ESP32_WIFI, ESP32_BT, LAIRD_CELL, LAIRD_BT  sets CELL_AUX line to high
//UART2_MUX0,ESP32_WIFI,,,,  List of : NONE, ESP32_WIFI, ESP32_BT, LAIRD_CELL, LAIRD_BT  sets WIFI_AUX line to low
//UART2_MUX1,LAIRD_CELL,,,,  List of : NONE, ESP32_WIFI, ESP32_BT, LAIRD_CELL, LAIRD_BT  sets WIFI_AUX line to high


//SetCellModule(i);
//SetCellUart(i+1);
//SetWiFiModule(i+2);
//SetWiFiUart(i+3);


static int import_factory_config(FIL * pSDFile)
{

	TCHAR * pLine;
	char rtext[100]; /* File read buffer */


	// read a line terminated with '\n' up to max size.  Strip '\r'. Return a null-terminated string usually ending with '\n' unless EOF is reached.
	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_config_line_01(pLine)) return 0;

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_config_line_02(pLine)) return 0;

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_config_line_03(pLine)) return 0;

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_config_line_04(pLine)) return 0;

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_config_line_05(pLine)) return 0;
#if 0
	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_config_line_06(pLine)) return 0;

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_config_line_07(pLine)) return 0;

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_config_line_08(pLine)) return 0;

	pLine = f_gets(rtext, sizeof(rtext), pSDFile);	if (pLine == NULL) return 0;
	if (!parse_config_line_09(pLine)) return 0;
#endif

	return 1;
}


void PerformFactoryConfig(void)
{
	FIL SDFile;
	FRESULT sd_res;
	int result;

    // Open the CSV file
	sd_res = f_open(&SDFile, FACTORY_CONFIG_NAME, FA_READ);  // "FactoryConfig.txt"
    if (sd_res != FR_OK) return;

	AppendStatusMessage("Performing factory configuration from SD card.");

    if (NoHardwareBitsSet())  SetHardwareBitsFlag();

    // Import factory config items
    result = import_factory_config(&SDFile);

    f_close(&SDFile);  // close file in any case

    // Update Flash in case any values changed
    Save_MainConfig();

    UNUSED(result);

}


#endif // Disable Factory Config File



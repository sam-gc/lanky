/* Lanky -- Scripting Language and Virtual Machine
 * Copyright (C) 2014  Sam Olsen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef UNITS_H
#define UNITS_H

// Unit types:
// distance, time, mass
#define UN_TYPE_COUNT 4

#define UN_DISTANCE 1
#define UN_TIME 2
#define UN_MASS 4
#define UN_ANGLE 8

typedef enum {
    UNM_DIST = 0,
    UNM_TIME,
    UNM_MASS,
    UNM_ANGLE
} un_family;

typedef enum {
    UN_TOP_UNITS,
    UN_BOTTOM_UNITS
} un_unit_location;

typedef enum {
    UN_km,
    UN_m,
    UN_micron,
    UN_mm,
    UN_cm,
    UN_dm,
    UN_ang,
    UN_in,
    UN_ft,
    UN_yd,
    UN_mi,
    UN_pc,
    UN_au,
    UN_lyr,
    UN_nm
} un_distance_type;

typedef enum {
    UN_hr,
    UN_s,
    UN_min,
    UN_day,
    UN_wk,
    UN_mo,
    UN_yr,
    UN_century
} un_time_type;

typedef enum {
    UN_kg,
    UN_g,
    UN_micg,
    UN_mg,
    UN_Mg,
    UN_oz,
    UN_lb,
    UN_ton
} un_mass_type;

typedef enum {
    UN_deg,
    UN_rad,
    UN_amin,
    UN_asec
} un_angle_type;

typedef struct {
    int type;
    int pow;
} un_unit_unit;

typedef struct {
    un_unit_unit top[UN_TYPE_COUNT];
    un_unit_unit bot[UN_TYPE_COUNT];

    int tm;
    int bm;

    double val;
} un_unit;

void un_setup();
void un_clean();
un_unit un_create_basic(double val);
void un_set_units(un_unit_location loc, un_unit *u, int mask, ...);
un_unit un_create(double val, char *fmt);
void un_format_type(un_unit a, char *buf);
void un_stringify(un_unit a, char *buf);
un_unit un_add(un_unit a, un_unit b);
un_unit un_sub(un_unit a, un_unit b);
un_unit un_mult(un_unit a, un_unit b);
un_unit un_div(un_unit a, un_unit b);

#endif

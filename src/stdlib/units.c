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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "units.h"

#define TOKENPASTE(x, y) x ## y

#define UN_TRANSLATE(f, n, x, y) if(strcmp(f, #n) == 0) { *oor |= x; *idx = y; return TOKENPASTE(UN_, n); }
#define UN_RETRANSLATE(i, t) if( TOKENPASTE(UN_, t) == i ) return #t

/*
static double un_t_cvt[][3] = {
    { 1.0, 60.0, 3600.0 },
    { 0.0166666666666667, 1.0, 60},
    { 0.0002777777777778, 0.0166666666666667, 1.0}
};*/

static double un_t_cvt[][8] = {
    { 1.0, 3600.0, 60.0, 0.04167, 0.005952, 0.001369, 0.0001141, 1.141e-6, },
    { 0.000277777777777778, 1.0, 0.0166666666666667, 1.1575e-5, 1.65333333333333e-6, 3.80277777777778e-7, 3.16944444444444e-8, 3.16944444444444e-10, },
    { 0.0166666666666667, 60.0, 1.0, 0.0006945, 9.92e-5, 2.28166666666667e-5, 1.90166666666667e-6, 1.90166666666667e-8, },
    { 23.9980801535877, 86393.0885529158, 1439.88480921526, 1.0, 0.142836573074154, 0.0328533717302616, 0.00273818094552436, 2.73818094552436e-5, },
    { 168.010752688172, 604838.709677419, 10080.6451612903, 7.00100806451613, 1.0, 0.230006720430108, 0.0191700268817204, 0.000191700268817204, },
    { 730.460189919649, 2629656.68371074, 43827.611395179, 30.4382761139518, 4.34769905040175, 1.0, 0.083345507669832, 0.00083345507669832, },
    { 8764.24189307625, 31551270.8150745, 525854.513584575, 365.205959684487, 52.1647677475898, 11.9982471516214, 1.0, 0.01, },
    { 876424.189307625, 3155127081.50745, 52585451.3584575, 36520.5959684487, 5216.47677475898, 1199.82471516214, 100.0, 1.0, }
};

double un_m_cvt[][15] = {
    { 1.0, 1000.0, 1000000000.0, 1000000.0, 100000.0, 10000.0, 10000000000000.0, 39370.07874016, 3280.83989501, 1093.61329834, 0.62137119, 3.24077929e-14, 6.68458712e-9, 1.05702341e-13, 1000000000000.0, },
    { 0.001, 1.0, 1000000.0, 1000.0, 100.0, 10.0, 10000000000.0, 39.37007874016, 3.28083989501, 1.09361329834, 0.00062137119, 3.24077929e-17, 6.68458712e-12, 1.05702341e-16, 1000000000.0, },
    { 1.0e-9, 1.0e-6, 1.0, 0.001, 0.0001, 1.0e-5, 10000.0, 3.937007874016e-5, 3.28083989501e-6, 1.09361329834e-6, 6.2137119e-10, 3.24077929e-23, 6.68458712e-18, 1.05702341e-22, 1000.0, },
    { 1.0e-6, 0.001, 1000.0, 1.0, 0.1, 0.01, 10000000.0, 0.03937007874016, 0.00328083989501, 0.00109361329834, 6.2137119e-7, 3.24077929e-20, 6.68458712e-15, 1.05702341e-19, 1000000.0, },
    { 1.0e-5, 0.01, 10000.0, 10.0, 1.0, 0.1, 100000000.0, 0.3937007874016, 0.0328083989501, 0.0109361329834, 6.2137119e-6, 3.24077929e-19, 6.68458712e-14, 1.05702341e-18, 10000000.0, },
    { 0.0001, 0.1, 100000.0, 100.0, 10.0, 1.0, 1000000000.0, 3.937007874016, 0.328083989501, 0.109361329834, 6.2137119e-5, 3.24077929e-18, 6.68458712e-13, 1.05702341e-17, 100000000.0, },
    { 1.0e-13, 1.0e-10, 0.0001, 1.0e-7, 1.0e-8, 1.0e-9, 1.0, 3.937007874016e-9, 3.28083989501e-10, 1.09361329834e-10, 6.2137119e-14, 3.24077929e-27, 6.68458712e-22, 1.05702341e-26, 0.1, },
    { 2.53999999999984e-5, 0.0253999999999984, 25399.9999999984, 25.3999999999984, 2.53999999999984, 0.253999999999984, 253999999.999984, 1.0, 0.0833333333332487, 0.0277777777778342, 1.5782828225999e-5, 8.23157939659947e-19, 1.69788512847989e-13, 2.68483946139983e-18, 25399999.9999984, },
    { 0.00030480000000029, 0.30480000000029, 304800.00000029, 304.80000000029, 30.480000000029, 3.0480000000029, 3048000000.0029, 12.0000000000122, 1.0, 0.333333333334349, 0.00018939393871218, 9.8778952759294e-18, 2.03746215417794e-12, 3.22180735368307e-17, 304800000.00029, },
    { 0.000914399999998083, 0.914399999998083, 914399.999998083, 914.399999998083, 91.4399999998083, 9.14399999998083, 9143999999.98083, 35.9999999999268, 2.99999999999086, 1.0, 0.000568181816134809, 2.96336858276979e-17, 6.11238646251519e-12, 9.66542206101974e-17, 914399999.998083, },
    { 1.60934400579467, 1609.34400579467, 1609344005.79467, 1609344.00579467, 160934.400579467, 16093.4400579467, 16093440057946.7, 63360.0002281406, 5280.00001900635, 1760.00000634082, 1.0, 5.215528724465e-14, 1.07578002127842e-8, 1.70111428886814e-13, 1609344005794.67, },
    { 30856775809623.2, 3.08567758096232e+16, 3.08567758096232e+22, 3.08567758096232e+19, 3.08567758096232e+18, 3.08567758096232e+17, 3.08567758096232e+26, 1.21483369329233e+18, 1.01236141107591e+17, 3.37453803693e+16, 19173511504388.8, 1.0, 206264.806141735, 3.26163343878935, 3.08567758096232e+25, },
    { 149597870.750767, 149597870750.767, 1.49597870750767e+17, 149597870750767.0, 14959787075076.7, 1495978707507.67, 1.49597870750767e+21, 5889679950817.96, 490806662567.665, 163602220856.387, 92955806.9698701, 4.84813681357182e-6, 1.0, 1.58128451469715e-5, 1.49597870750767e+20, },
    { 9460528409678.27, 9.46052840967827e+15, 9.46052840967827e+21, 9.46052840967827e+18, 9.46052840967827e+17, 9.46052840967827e+16, 9.46052840967827e+25, 3.72461748412554e+17, 3.1038479034348e+16, 1.03461596781475e+16, 5878499795950.59, 0.30659484542542, 63239.7263557294, 1.0, 9.46052840967827e+24, },
    { 1.0e-12, 1.0e-9, 0.001, 1.0e-6, 1.0e-7, 1.0e-8, 10.0, 3.937007874016e-8, 3.28083989501e-9, 1.09361329834e-9, 6.2137119e-13, 3.24077929e-26, 6.68458712e-21, 1.05702341e-25, 1.0, }
};

double un_k_cvt[][8] = {
    { 1.0, 1000.0, 1000000000.0, 1000000.0, 0.001, 35.27396195, 2.20462262, 0.00110231, },
    { 0.001, 1.0, 1000000.0, 1000.0, 1.0e-6, 0.03527396195, 0.00220462262, 1.10231e-6, },
    { 1.0e-9, 1.0e-6, 1.0, 0.001, 1.0e-12, 3.527396195e-8, 2.20462262e-9, 1.10231e-12, },
    { 1.0e-6, 0.001, 1000.0, 1.0, 1.0e-9, 3.527396195e-5, 2.20462262e-6, 1.10231e-9, },
    { 1000.0, 1000000.0, 1000000000000.0, 1000000000.0, 1.0, 35273.96195, 2204.62262, 1.10231, },
    { 0.0283495231246628, 28.3495231246628, 28349523.1246628, 28349.5231246628, 2.83495231246628e-5, 1.0, 0.0624999999468446, 3.1249962835547e-5, },
    { 0.453592370380378, 453.592370380378, 453592370.380378, 453592.370380378, 0.000453592370380378, 16.0000000136078, 1.0, 0.000499999405793995, },
    { 907.185818871279, 907185.818871279, 907185818871.279, 907185818.871279, 0.907185818871279, 32000.0380564451, 2000.00237682685, 1.0, }
};

double un_a_cvt[][4] = {
    { 1.0, 0.017453293, 60.0, 3600.0, },
    { 57.2957779371492, 1.0, 3437.74667622895, 206264.800573737, },
    { 0.0166666666666667, 0.000290888216666667, 1.0, 60.0, },
    { 0.000277777777777778, 4.84813694444444e-6, 0.0166666666666667, 1.0, }
};

static int masks[] = {UN_DISTANCE, UN_TIME, UN_MASS, UN_ANGLE};
static double **matrices[UN_TYPE_COUNT];

/*
void un_cvt_distance(un_unit *a, un_unit *b)
{
    int aidx = un_idx_distance(a->topmask);
    int bidx = un_idx_distance(b->topmask);

    if(aidx == bidx)
        return;

    double nv = b->val / un_m_cvt[aidx][bidx];

    b->val = nv;
    b->topmask = a->topmask;
}*/

double **un_alloc_matrix(int m, int n, double *cpy)
{
    double *vals = malloc(m * n * sizeof(double));
    memcpy(vals, cpy, m * n * sizeof(double));
    double **rows = malloc(n * sizeof(double *));
    int i;
    for(i = 0; i < n; i++)
        rows[i] = vals + i * m;

    return rows;
}

void un_setup()
{
    double **mt = un_alloc_matrix(15, 15, (double *)un_m_cvt);
    double **tt = un_alloc_matrix(8, 8, (double *)un_t_cvt);
    double **kt = un_alloc_matrix(8, 8, (double *)un_k_cvt);
    double **at = un_alloc_matrix(4, 4, (double *)un_a_cvt);
    matrices[0] = mt;
    matrices[1] = tt;
    matrices[2] = kt;
    matrices[3] = at;
}

void un_clean()
{
    int i;
    for(i = 0; i < UN_TYPE_COUNT; i++)
    {
        free(*matrices[i]);
        free(matrices[i]);
    }
}

int un_get_type_from_string(char *fmt, int *oor, int *idx)
{
    UN_TRANSLATE(fmt, m, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, micron, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, mm, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, cm, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, dm, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, km, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, ang, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, in, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, ft, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, yd, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, mi, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, nm, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, pc, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, au, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, lyr, UN_DISTANCE, UNM_DIST);
    UN_TRANSLATE(fmt, s, UN_TIME, UNM_TIME);
    UN_TRANSLATE(fmt, min, UN_TIME, UNM_TIME);
    UN_TRANSLATE(fmt, hr, UN_TIME, UNM_TIME);
    UN_TRANSLATE(fmt, day, UN_TIME, UNM_TIME);
    UN_TRANSLATE(fmt, wk, UN_TIME, UNM_TIME);
    UN_TRANSLATE(fmt, mo, UN_TIME, UNM_TIME);
    UN_TRANSLATE(fmt, yr, UN_TIME, UNM_TIME);
    UN_TRANSLATE(fmt, century, UN_TIME, UNM_TIME);
    UN_TRANSLATE(fmt, kg, UN_MASS, UNM_MASS); 
    UN_TRANSLATE(fmt, g, UN_MASS, UNM_MASS); 
    UN_TRANSLATE(fmt, micg, UN_MASS, UNM_MASS); 
    UN_TRANSLATE(fmt, mg, UN_MASS, UNM_MASS); 
    UN_TRANSLATE(fmt, Mg, UN_MASS, UNM_MASS); 
    UN_TRANSLATE(fmt, oz, UN_MASS, UNM_MASS); 
    UN_TRANSLATE(fmt, lb, UN_MASS, UNM_MASS); 
    UN_TRANSLATE(fmt, ton, UN_MASS, UNM_MASS); 
    UN_TRANSLATE(fmt, deg, UN_ANGLE, UNM_ANGLE);
    UN_TRANSLATE(fmt, rad, UN_ANGLE, UNM_ANGLE);
    UN_TRANSLATE(fmt, amin, UN_ANGLE, UNM_ANGLE);
    UN_TRANSLATE(fmt, asec, UN_ANGLE, UNM_ANGLE);

    return -1;
}

char *un_get_string_from_type(int fmt, int type)
{
    switch(type)
    {
        case UN_DISTANCE:
            UN_RETRANSLATE(fmt, m);
            UN_RETRANSLATE(fmt, micron);
            UN_RETRANSLATE(fmt, mm);
            UN_RETRANSLATE(fmt, cm);
            UN_RETRANSLATE(fmt, dm);
            UN_RETRANSLATE(fmt, km);
            UN_RETRANSLATE(fmt, ang);
            UN_RETRANSLATE(fmt, in);
            UN_RETRANSLATE(fmt, ft);
            UN_RETRANSLATE(fmt, yd);
            UN_RETRANSLATE(fmt, mi);
            UN_RETRANSLATE(fmt, au);
            UN_RETRANSLATE(fmt, pc);
            UN_RETRANSLATE(fmt, lyr);
            UN_RETRANSLATE(fmt, nm);
        case UN_TIME:
            UN_RETRANSLATE(fmt, s);
            UN_RETRANSLATE(fmt, min);
            UN_RETRANSLATE(fmt, hr);
            UN_RETRANSLATE(fmt, day);
            UN_RETRANSLATE(fmt, wk);
            UN_RETRANSLATE(fmt, mo);
            UN_RETRANSLATE(fmt, yr);
            UN_RETRANSLATE(fmt, century);
        case UN_MASS:
            UN_RETRANSLATE(fmt, kg);
            UN_RETRANSLATE(fmt, g);
            UN_RETRANSLATE(fmt, micg);
            UN_RETRANSLATE(fmt, mg);
            UN_RETRANSLATE(fmt, Mg);
            UN_RETRANSLATE(fmt, oz);
            UN_RETRANSLATE(fmt, lb);
            UN_RETRANSLATE(fmt, ton);
        case UN_ANGLE:
            UN_RETRANSLATE(fmt, deg);
            UN_RETRANSLATE(fmt, rad);
            UN_RETRANSLATE(fmt, asec);
            UN_RETRANSLATE(fmt, amin);
    }

    return "na";
}

un_unit un_create(double val, char *fmt)
{
    un_unit u = un_create_basic(val);
    char top = 1;
    char np = 0;
    int topmask, botmask;
    topmask = botmask = 0;
    char buf[strlen(fmt) + 1];
    strcpy(buf, "");

    int i;
    for(i = 0; i < strlen(fmt) + 1; i++)
    {
        char c = fmt[i];

        if(c == ' ')
            continue;

        if(c == '*' || c == '\0' || c == '/')
        {
            char *t;
            char tb[100];
            t = tb;
            int p = 1;
            if(np)
            {
                t = strtok(buf, "^");
                p = atoi(strtok(NULL, "^"));
            }
            else
                sscanf(buf, "%s", t);

            int *mask = top ? &topmask : &botmask;
            int idx;
            int num = un_get_type_from_string(t, mask, &idx); 

            un_unit_unit uu = { num, p };

            if(top)
                u.top[idx] = uu;
            else
                u.bot[idx] = uu;

            np = 0;
            strcpy(buf, "");

            if(c == '/')
                top = 0;
            continue;
        }

        if(c == '^')
            np = 1;

        char cc[2];
        sprintf(cc, "%c", c);
        strcat(buf, cc);
    }

    u.tm = topmask;
    u.bm = botmask;
    return u;
}

un_unit un_create_basic(double val)
{
    un_unit un;
    un.tm = 0;
    un.bm = 0;
    
    memset(un.top, 0, UN_TYPE_COUNT * sizeof(un_unit_unit));
    memset(un.bot, 0, UN_TYPE_COUNT * sizeof(un_unit_unit));

    un.val = val;
    return un;
}

/*
void un_set_units(un_unit_location loc, un_unit *u, int mask, ...)
{
    va_list args;
    va_start(args, mask);

    un_unit_unit *w = (loc == UN_TOP_UNITS ? u->top : u->bot);

    if(loc == UN_TOP_UNITS)
        u->tm = mask;
    else
        u->bm = mask;

    char dist = mask & UN_DISTANCE ? 1 : 0;
    char time = mask & UN_TIME ? 1 : 0;
    char mass = mask & UN_MASS ? 1 : 0;   
    if(dist)
    {
        int type = va_arg(args, int);
        int pow = va_arg(args, int);   

        un_unit_unit t;
        t.pow = pow;
        t.type = type;

        w[UNM_DIST] = t;
    }

    if(time)
    {
        int type = va_arg(args, int);
        int pow = va_arg(args, int);

        un_unit_unit t = {type, pow};
        w[UNM_TIME] = t;
    }

    if(mass)
    {
        int type = va_arg(args, int);
        int pow = va_arg(args, int);

        un_unit_unit t = {type, pow};
        w[UNM_MASS] = t;
    }

    va_end(args);
}*/

void un_append_uu(char *buf, un_unit_unit u, int type)
{
    if(u.pow == 1)
        strcat(buf, un_get_string_from_type(u.type, type));
    else
    {
        char *tp = un_get_string_from_type(u.type, type);
        char tmp[100];
        sprintf(tmp, "%s^%d", tp, u.pow);
        strcat(buf, tmp);
    }
}

void un_format_type(un_unit a, char *buf)
{
    strcpy(buf, "");

    int i;
    for(i = 0; i < (a.bm ? 2 : 1); i++)
    {
        int mask = a.tm;
        un_unit_unit *w = a.top;
        if(i == 1)
        {
            strcat(buf, "/");
            mask = a.bm;
            w = a.bot;
        }

        if(mask & UN_DISTANCE)
        {
            un_append_uu(buf, w[UNM_DIST], UN_DISTANCE);
            strcat(buf, "*");
        }

        if(mask & UN_TIME)
        {

            un_append_uu(buf, w[UNM_TIME], UN_TIME);
            strcat(buf, "*");
        }

        if(mask & UN_MASS)
        {
            un_append_uu(buf, w[UNM_MASS], UN_MASS);
            strcat(buf, "*");
        }

        if(mask & UN_ANGLE)
        {
            un_append_uu(buf, w[UNM_ANGLE], UN_ANGLE);
            strcat(buf, "*");
        }

        buf[strlen(buf) - 1] = '\0';
    }
}

void un_stringify(un_unit a, char *buf)
{
    char type[100];
    un_format_type(a, type);
    if((a.val < 0.0001 && a.val > -0.0001) && a.val)
        sprintf(buf, "%e%s", a.val, type);
    else
        sprintf(buf, "%lf%s", a.val, type);
}

/*un_unit un_create(double val, long topmask, long botmask)
{
    un_unit u = {topmask, botmask, val};

    return u;
}*/

void un_convert(un_unit *b, un_unit a)
{
    int i;
    for(i = 0; i < UN_TYPE_COUNT; i++)
    {
        int mask = masks[i];
        if(mask & a.tm)
        {
            un_unit_unit au = a.top[i];

            char top;
            un_unit_unit bu;
            if(mask & b->tm)
            {
                top = 1;
                bu = b->top[i];
            }
            else
            {
                top = 0;
                bu = b->bot[i];
            }
            /*
            if(au.pow != bu.pow)
                return;
            */

            if(au.type != bu.type)
            {
                double val = pow(matrices[i][au.type][bu.type], au.pow);
                if(top)
                    b->val /= val;
                else
                    b->val *= val;
                (top ? b->top : b->bot)[i] = au;
            }
        }

        /*
        if(mask & a.bm)
        {
            un_unit_unit au = a.bot[i];
            un_unit_unit bu = b->bot[i];

            if(au.pow != bu.pow)
                return;
            
            if(au.type != bu.type)
            {
                b->val /= pow(matrices[i][au.type][bu.type], au.pow);
                b->bot[i] = au;
            }
        }
        */
    }
}

un_unit un_flatten(un_unit a)
{
    un_unit b = un_create_basic(a.val);
    
    int i;
    for(i = 0; i < UN_TYPE_COUNT; i++)
    {
        int mask = masks[i];
        if(mask & a.tm)
        {
            b.top[i] = a.top[i];
            b.tm |= mask;
        }
        else if(mask & a.bm)
        {
            b.top[i] = a.bot[i];
            b.tm |= mask;
        }
    }

    return b;
}

un_unit un_arith(un_unit a, un_unit b, char add)
{
    //if(un_distance_mask | a.topmask != un_distance_mask && un_time_mask | a.topmask != un_time_mask)
    //    return un_create(0, 0, 

    un_unit ret = un_create_basic(0);
    if(a.tm != b.tm || a.bm != b.bm)
        return ret;

    un_convert(&b, un_flatten(a));

    int i;
    for(i = 0; i < UN_TYPE_COUNT; i++)
    {
        ret.top[i] = a.top[i];
        ret.bot[i] = a.bot[i];
    }

    ret.tm = a.tm;
    ret.bm = a.bm;

    ret.val = add ? a.val + b.val : a.val - b.val;

    return ret;
}

un_unit un_mult(un_unit a, un_unit b)
{
    un_unit ret = un_create_basic(0);

    un_unit base = un_create_basic(0);
    int i;
    for(i = 0; i < UN_TYPE_COUNT; i++)
    {
        int mask = masks[i];

        if(mask & (a.tm & b.tm))
        {
            base.tm |= mask;
            un_unit_unit u = {a.top[i].type, b.top[i].pow};
            base.top[i] = u;
        }

        if(mask & (a.bm & b.bm))
        {
            base.bm |= mask;
            un_unit_unit u = {a.bot[i].type, b.bot[i].pow};
            base.bot[i] = u;
        }

        if(mask & (a.tm & b.bm))
        {
            base.bm |= mask;
            un_unit_unit u = {a.top[i].type, b.bot[i].pow};
            base.bot[i] = u;
        }

        if(mask & (a.bm & b.tm))
        {
            base.tm |= mask;
            un_unit_unit u = {a.bot[i].type, b.top[i].pow};
            base.top[i] = u;
        }
    }

    un_convert(&b, un_flatten(base));
    //printf("%lf\n", b.val);

    for(i = 0; i < UN_TYPE_COUNT; i++)
    {
        int mask = masks[i];
        int count = 0;
        int type = -1;
        if(mask & a.tm)
        {
            count += a.top[i].pow;
            type = a.top[i].type;
        }
        if(mask & b.tm)
        {
            count += b.top[i].pow;
            type = b.top[i].type;
        }
        if(mask & a.bm)
        {
            count -= a.bot[i].pow;
            type = a.bot[i].type;
        }
        if(mask & b.bm)
        {
            count -= b.bot[i].pow;
            type = b.bot[i].type;
        }

        if(type < 0)
            continue;

        if(count > 0)
        {
            un_unit_unit u = {type, count};
            ret.top[i] = u;
            ret.tm |= mask;
        }
        else if(count < 0)
        {
            un_unit_unit u = {type, abs(count)};
            ret.bot[i] = u;
            ret.bm |= mask;
        }
    }
    
    ret.val = a.val * b.val;
    return ret;
}

un_unit un_div(un_unit a, un_unit b)
{
    un_unit swapped = un_create_basic(1 / b.val);

    int i;
    for(i = 0; i < UN_TYPE_COUNT; i++)
    {
        swapped.top[i] = b.bot[i];
        swapped.bot[i] = b.top[i];
    }

    swapped.tm = b.bm;
    swapped.bm = b.tm;

    return un_mult(a, swapped);
}

un_unit un_add(un_unit a, un_unit b)
{
    return un_arith(a, b, 1);
}

un_unit un_sub(un_unit a, un_unit b)
{
    return un_arith(a, b, 0);
}


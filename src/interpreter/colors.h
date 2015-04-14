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

#ifdef USE_COLOR
#define BLACK		   "\033[0;30m"
#define BLUE	       "\033[0;34m"
#define GREEN	       "\033[0;32m"
#define CYAN	       "\033[0;36m"
#define RED		       "\033[0;31m"
#define PURPLE         "\033[0;35m"
#define YELLOW         "\033[0;33m"
#define LIGHT_GREY     "\033[0;37m"
#define DARK_GREY      "\033[1;30m"
#define LIGHT_BLUE     "\033[1;34m"
#define LIGHT_GREEN    "\033[1;32m"
#define LIGHT_CYAN     "\033[1;36m"
#define LIGHT_RED      "\033[1;31m"
#define LIGHT_PURPLE   "\033[1;35m"
#define LIGHT_YELLOW   "\033[1;33m"
#define WHITE          "\033[1;37m"
#define DEFAULT		   "\033[0m"
#else
#define BLACK
#define BLUE
#define GREEN	
#define CYAN
#define RED	
#define PURPLE 
#define YELLOW 
#define LIGHT_GREY 
#define DARK_GREY 
#define LIGHT_BLUE 
#define LIGHT_GREEN  
#define LIGHT_CYAN   
#define LIGHT_RED    
#define LIGHT_PURPLE 
#define LIGHT_YELLOW 
#define WHITE        
#define DEFAULT		
#endif

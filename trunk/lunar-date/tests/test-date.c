/* vi: set sw=4 ts=4: */
/*
 * test.c
 *
 * This file is part of liblunar.
 *
 * Copyright (C) 2007 - yetist <yetist@gmail.com>.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 * */

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif
#include <date.h>
#include <glib/gi18n.h>

static void display (LunarDate *date)
{
	g_printf("\n");
	g_printf("format:%%(YEAR)年%%(MONTH)月%%(DAY)日%%(HOUR)时->%s\n", lunar_date_strftime(date, "%(YEAR)年%(MONTH)月%(DAY)日%(HOUR)时"));
	g_printf("format:%%(year)年%%(month)月%%(day)日%%(hour)时->%s\n", lunar_date_strftime(date, "%(year)年%(month)月%(day)日%(hour)时"));
	g_printf("format:%%(NIAN)年%%(YUE)月%%(RI)日%%(SHI)时->%s\n", lunar_date_strftime(date, "%(NIAN)年%(YUE)月%(RI)日%(SHI)时"));
	g_printf("format:%%(nian)年%%(yue)月%%(ri)日%%(shi)时->%s\n", lunar_date_strftime(date, "%(nian)年%(yue)月%(ri)日%(shi)时"));
	g_printf("format:%%(Y60)年%%(M60)月%%(D60)日%%(H60)时->%s\n", lunar_date_strftime(date, "%(Y60)年%(M60)月%(D60)日%(H60)时"));
	g_printf("format:%%(Y8)年%%(M8)月%%(D8)日%%(H8)时->%s\n", lunar_date_strftime(date, "%(Y8)年%(M8)月%(D8)日%(H8)时"));
	g_printf("format:%%(shengxiao)->%s\n", lunar_date_strftime(date, "%(shengxiao)"));
	g_printf("format:%%(jieri)->%s\n", lunar_date_strftime(date, "%(jieri)"));
}

void set_lunar_date(LunarDate *date, GDateYear year, GDateMonth month, GDateDay day , guint8 hour, gboolean isleap)
{
	GError *error = NULL;

	g_printf("Test Date: <%u-%u-%u %u>\n", year, month, day, hour);
	lunar_date_set_lunar_date(date, year, month, day , hour, isleap, &error);
	if (error != NULL)
	{
		g_printf("Error: %s \n", error->message);
		g_error_free(error);
		return;
	}
	display(date);

	lunar_date_set_solar_date(date, year, month, day , hour, &error);
	if (error != NULL)
	{
		g_printf("Error: %s \n", error->message);
		g_error_free(error);
		return;
	}
	display(date);
	if (error != NULL)
		g_error_free(error);
}

/*
gchar* lunar_date_strftime (LunarDate *date, const char *format);
 * %(YEAR)年%(MONTH)月%(DAY)日		公历：大写
 * %(year)年%(month)月%(day)日		公历：小写
 * %(NIAN)年%(YUE)月%(RI)日%(SHI)时 阴历：大写
 * %(nian)年%(yue)月%(ri)日%(shi)时 阴历：小写
 *
 * %(Y60)年%(M60)月%(D60)日%(H60)时 干支:大写
 * %(y60)年%(m60)月%(d60)日%(h60)时 干支:小写 not use
 * %(Y8)年%(M8)月%(D8)日%(H8)时		八字:大写
 * %(y8)年%(m8)月%(d8)日%(h8)时		八字:小写 not use
 * %(shengxiao)%(jieri)				生肖和节日
 */

void random_test(void)
{
	GRand *rand = NULL;
	LunarDate *date;

	date = lunar_date_new();
	rand = g_rand_new ();

	GDateYear year = g_rand_int_range(rand, 1900, 2049);
	GDateMonth month = g_rand_int_range(rand, 1, 12);
	GDateDay day = g_rand_int_range(rand, 1, 31);
	gboolean isleap = g_rand_int_range(rand, 0, 1);
	guint8 hour = g_rand_int_range(rand, 0, 23);

	set_lunar_date(date, year, month, day , hour, isleap);

	g_rand_free(rand);
	lunar_date_free(date);
}

void test(gchar* argv[])
{
	GError *error = NULL;
	GRand *rand = NULL;
	LunarDate *date;

	date = lunar_date_new();
	rand = g_rand_new ();

	date = lunar_date_new();

	GDateYear year = atoi(argv[1]);
	GDateMonth month =	atoi(argv[2]);
	GDateDay day = atoi(argv[3]);

	gboolean isleap = g_rand_int_range(rand, 0, 1);
	guint8 hour = g_rand_int_range(rand, 0, 23);

	set_lunar_date(date, year, month, day, hour, isleap);

	g_rand_free(rand);
	lunar_date_free(date);
}

int main (int argc, char* argv[])
{
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, LUNAR_DATE_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	g_type_init();
	if (argc == 4)
	{
		test(argv);
	}
	else if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
	{
		g_printf("usage: test-date [year month day]\n");
		return 0;
	}
	else 
	{
		int i = 10;
		while (i-- >=0)
		{
			g_printf("--------------------------------------\n");
			random_test();
			sleep(2);
		}
	}
	return 0;
}

/* vi: set sw=4 ts=4: */
/*
 * gcl-calendar.c: This file is part of ____
 *
 * Copyright (C) 2007 yetist <yetist@gmail.com>
 *
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

#if HAVE_CONFIG_H
    #include <config.h>
#endif
#include <sys/time.h>
#ifdef HAVE__NL_TIME_FIRST_WEEKDAY
#include <langinfo.h>
#endif
#include <time.h>

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "gcl-calendar.h"
#include "gcl-date.h"

#define I_
#define P_

/***************************************************************************/
/* The following date routines are taken from the lib_date package. 
 * They have been minimally edited to avoid conflict with types defined
 * in win32 headers.
 */

static const guint month_length[2][13] =
{
  { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
  { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static const guint days_in_months[2][14] =
{
  { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
  { 0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

static glong  calc_days(guint year, guint mm, guint dd);
static guint  day_of_week(guint year, guint mm, guint dd);
static glong  dates_difference(guint year1, guint mm1, guint dd1,
			       guint year2, guint mm2, guint dd2);
static guint  weeks_in_year(guint year);

static gboolean 
leap (guint year)
{
  return((((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0));
}

static guint 
day_of_week (guint year, guint mm, guint dd)
{
  glong  days;
  
  days = calc_days(year, mm, dd);
  if (days > 0L)
    {
      days--;
      days %= 7L;
      days++;
    }
  return( (guint) days );
}

static guint weeks_in_year(guint year)
{
  return(52 + ((day_of_week(year,1,1)==4) || (day_of_week(year,12,31)==4)));
}

static gboolean 
check_date(guint year, guint mm, guint dd)
{
  if (year < 1) return FALSE;
  if ((mm < 1) || (mm > 12)) return FALSE;
  if ((dd < 1) || (dd > month_length[leap(year)][mm])) return FALSE;
  return TRUE;
}

static guint 
week_number(guint year, guint mm, guint dd)
{
  guint first;
  
  first = day_of_week(year,1,1) - 1;
  return( (guint) ( (dates_difference(year,1,1, year,mm,dd) + first) / 7L ) +
	  (first < 4) );
}

static glong 
year_to_days(guint year)
{
  return( year * 365L + (year / 4) - (year / 100) + (year / 400) );
}


static glong 
calc_days(guint year, guint mm, guint dd)
{
  gboolean lp;
  
  if (year < 1) return(0L);
  if ((mm < 1) || (mm > 12)) return(0L);
  if ((dd < 1) || (dd > month_length[(lp = leap(year))][mm])) return(0L);
  return( year_to_days(--year) + days_in_months[lp][mm] + dd );
}

static gboolean 
week_of_year(guint *week, guint *year, guint mm, guint dd)
{
  if (check_date(*year,mm,dd))
    {
      *week = week_number(*year,mm,dd);
      if (*week == 0) 
	*week = weeks_in_year(--(*year));
      else if (*week > weeks_in_year(*year))
	{
	  *week = 1;
	  (*year)++;
	}
      return TRUE;
    }
  return FALSE;
}

static glong 
dates_difference(guint year1, guint mm1, guint dd1,
		 guint year2, guint mm2, guint dd2)
{
  return( calc_days(year2, mm2, dd2) - calc_days(year1, mm1, dd1) );
}

/*** END OF lib_date routines ********************************************/

/* Spacing around day/week headers and main area, inside those windows */
#define CALENDAR_MARGIN		 0
/* Spacing around day/week headers and main area, outside those windows */
#define INNER_BORDER		 4
/* Separation between day headers and main area */
#define CALENDAR_YSEP		 4
/* Separation between week headers and main area */
#define CALENDAR_XSEP		 4

#define DAY_XSEP		 0 /* not really good for small calendar */
#define DAY_YSEP		 0 /* not really good for small calendar */

#define SCROLL_DELAY_FACTOR      5

/* Color usage */
#define HEADER_FG_COLOR(widget)		 (& (widget)->style->fg[GTK_WIDGET_STATE (widget)])
#define HEADER_BG_COLOR(widget)		 (& (widget)->style->bg[GTK_WIDGET_STATE (widget)])
#define SELECTED_BG_COLOR(widget)	 (& (widget)->style->base[GTK_WIDGET_HAS_FOCUS (widget) ? GTK_STATE_SELECTED : GTK_STATE_ACTIVE])
#define SELECTED_FG_COLOR(widget)	 (& (widget)->style->text[GTK_WIDGET_HAS_FOCUS (widget) ? GTK_STATE_SELECTED : GTK_STATE_ACTIVE])
#define NORMAL_DAY_COLOR(widget)	 (& (widget)->style->fg[GTK_WIDGET_STATE (widget)])
#define PREV_MONTH_COLOR(widget)	 (& (widget)->style->mid[GTK_WIDGET_STATE (widget)])
#define NEXT_MONTH_COLOR(widget)	 (& (widget)->style->mid[GTK_WIDGET_STATE (widget)])
#define MARKED_COLOR(widget)		 (& (widget)->style->fg[GTK_WIDGET_STATE (widget)])
#define BACKGROUND_COLOR(widget)	 (& (widget)->style->base[GTK_WIDGET_STATE (widget)])
#define HIGHLIGHT_BACK_COLOR(widget)	 (& (widget)->style->mid[GTK_WIDGET_STATE (widget)])

#define GTK_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB


enum {
  ARROW_YEAR_LEFT,
  ARROW_YEAR_RIGHT,
  ARROW_MONTH_LEFT,
  ARROW_MONTH_RIGHT
};

enum {
  MONTH_PREV,
  MONTH_CURRENT,
  MONTH_NEXT
};

enum {
  MONTH_CHANGED_SIGNAL,
  DAY_SELECTED_SIGNAL,
  DAY_SELECTED_DOUBLE_CLICK_SIGNAL,
  PREV_MONTH_SIGNAL,
  NEXT_MONTH_SIGNAL,
  PREV_YEAR_SIGNAL,
  NEXT_YEAR_SIGNAL,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_YEAR,
  PROP_MONTH,
  PROP_DAY,
  PROP_SHOW_HEADING,
  PROP_SHOW_DAY_NAMES,
  PROP_NO_MONTH_CHANGE,
  PROP_SHOW_WEEK_NUMBERS,
  PROP_SHOW_LUNAR,
  PROP_LAST
};

static guint gcl_calendar_signals[LAST_SIGNAL] = { 0 };

struct _GclCalendarPrivate
{
  GdkWindow *header_win;
  GdkWindow *day_name_win;
  GdkWindow *main_win;
  GdkWindow *week_win;
  GdkWindow *arrow_win[4];
  GCLDate   *date;

  guint header_h;
  guint day_name_h;
  guint main_h;

  guint	     arrow_state[4];
  guint	     arrow_width;
  guint	     max_month_width;
  guint	     max_year_width;
  gchar*     lunar_day[6][7];
  gchar*     lunar_year;
  
  guint day_width;
  guint week_width;

  guint min_day_width;
  guint max_day_char_width;
  guint max_day_char_ascent;
  guint max_day_char_descent;
  guint max_label_char_ascent;
  guint max_label_char_descent;
  guint max_week_char_width;
  
  /* flags */
  guint year_before : 1;

  guint need_timer  : 1;

  guint in_drag : 1;
  guint drag_highlight : 1;

  guint32 timer;
  gint click_child;

  gint week_start;

  gint drag_start_x;
  gint drag_start_y;
};

#define GCL_CALENDAR_GET_PRIVATE(widget)  (GCL_CALENDAR (widget)->priv)

static void gcl_calendar_finalize     (GObject      *calendar);
static void gcl_calendar_destroy      (GtkObject    *calendar);
static void gcl_calendar_set_property (GObject      *object,
				       guint         prop_id,
				       const GValue *value,
				       GParamSpec   *pspec);
static void gcl_calendar_get_property (GObject      *object,
				       guint         prop_id,
				       GValue       *value,
				       GParamSpec   *pspec);

static void     gcl_calendar_realize        (GtkWidget        *widget);
static void     gcl_calendar_unrealize      (GtkWidget        *widget);
static void     gcl_calendar_size_request   (GtkWidget        *widget,
					     GtkRequisition   *requisition);
static void     gcl_calendar_size_allocate  (GtkWidget        *widget,
					     GtkAllocation    *allocation);
static gboolean gcl_calendar_expose         (GtkWidget        *widget,
					     GdkEventExpose   *event);
static gboolean gcl_calendar_button_press   (GtkWidget        *widget,
					     GdkEventButton   *event);
static gboolean gcl_calendar_button_release (GtkWidget        *widget,
					     GdkEventButton   *event);
static gboolean gcl_calendar_motion_notify  (GtkWidget        *widget,
					     GdkEventMotion   *event);
static gboolean gcl_calendar_enter_notify   (GtkWidget        *widget,
					     GdkEventCrossing *event);
static gboolean gcl_calendar_leave_notify   (GtkWidget        *widget,
					     GdkEventCrossing *event);
static gboolean gcl_calendar_scroll         (GtkWidget        *widget,
					     GdkEventScroll   *event);
static gboolean gcl_calendar_key_press      (GtkWidget        *widget,
					     GdkEventKey      *event);
static gboolean gcl_calendar_focus_out      (GtkWidget        *widget,
					     GdkEventFocus    *event);
static void     gcl_calendar_grab_notify    (GtkWidget        *widget,
					     gboolean          was_grabbed);
static void     gcl_calendar_state_changed  (GtkWidget        *widget,
					     GtkStateType      previous_state);
static void     gcl_calendar_style_set      (GtkWidget        *widget,
					     GtkStyle         *previous_style);

static void     gcl_calendar_drag_data_get      (GtkWidget        *widget,
						 GdkDragContext   *context,
						 GtkSelectionData *selection_data,
						 guint             info,
						 guint             time);
static void     gcl_calendar_drag_data_received (GtkWidget        *widget,
						 GdkDragContext   *context,
						 gint              x,
						 gint              y,
						 GtkSelectionData *selection_data,
						 guint             info,
						 guint             time);
static gboolean gcl_calendar_drag_motion        (GtkWidget        *widget,
						 GdkDragContext   *context,
						 gint              x,
						 gint              y,
						 guint             time);
static void     gcl_calendar_drag_leave         (GtkWidget        *widget,
						 GdkDragContext   *context,
						 guint             time);
static gboolean gcl_calendar_drag_drop          (GtkWidget        *widget,
						 GdkDragContext   *context,
						 gint              x,
						 gint              y,
						 guint             time);

static gchar* gcl_calendar_lunar_day_str (GclCalendar *calendar, gint year, gint month, gint day);
static gchar* gcl_calendar_lunar_year_str (GclCalendar *calendar, gint year, gint month, gint day);

static void calendar_start_spinning (GclCalendar *calendar,
				     gint         click_child);
static void calendar_stop_spinning  (GclCalendar *calendar);

static void calendar_invalidate_day     (GclCalendar *widget,
					 gint       row,
					 gint       col);
static void calendar_invalidate_day_num (GclCalendar *widget,
					 gint       day);
static void calendar_invalidate_arrow   (GclCalendar *widget,
					 guint      arrow);

static void calendar_compute_days      (GclCalendar *calendar);
     
static char    *default_abbreviated_dayname[7];
static char    *default_monthname[12];

G_DEFINE_TYPE (GclCalendar, gcl_calendar, GTK_TYPE_WIDGET)

static void
gcl_calendar_class_init (GclCalendarClass *class)
{
  GObjectClass   *gobject_class;
  GtkObjectClass   *object_class;
  GtkWidgetClass *widget_class;

  gobject_class = (GObjectClass*)  class;
  object_class = (GtkObjectClass*)  class;
  widget_class = (GtkWidgetClass*) class;
  
  gobject_class->set_property = gcl_calendar_set_property;
  gobject_class->get_property = gcl_calendar_get_property;
  gobject_class->finalize = gcl_calendar_finalize;

  object_class->destroy = gcl_calendar_destroy;

  widget_class->realize = gcl_calendar_realize;
  widget_class->unrealize = gcl_calendar_unrealize;
  widget_class->expose_event = gcl_calendar_expose;
  widget_class->size_request = gcl_calendar_size_request;
  widget_class->size_allocate = gcl_calendar_size_allocate;
  widget_class->button_press_event = gcl_calendar_button_press;
  widget_class->button_release_event = gcl_calendar_button_release;
  widget_class->motion_notify_event = gcl_calendar_motion_notify;
  widget_class->enter_notify_event = gcl_calendar_enter_notify;
  widget_class->leave_notify_event = gcl_calendar_leave_notify;
  widget_class->key_press_event = gcl_calendar_key_press;
  widget_class->scroll_event = gcl_calendar_scroll;
  widget_class->style_set = gcl_calendar_style_set;
  widget_class->state_changed = gcl_calendar_state_changed;
  widget_class->grab_notify = gcl_calendar_grab_notify;
  widget_class->focus_out_event = gcl_calendar_focus_out;

  widget_class->drag_data_get = gcl_calendar_drag_data_get;
  widget_class->drag_motion = gcl_calendar_drag_motion;
  widget_class->drag_leave = gcl_calendar_drag_leave;
  widget_class->drag_drop = gcl_calendar_drag_drop;
  widget_class->drag_data_received = gcl_calendar_drag_data_received;
  
  g_object_class_install_property (gobject_class,
                                   PROP_YEAR,
                                   g_param_spec_int ("year",
						     P_("Year"),
						     P_("The selected year"),
						     0, G_MAXINT, 0,
						     GTK_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_MONTH,
                                   g_param_spec_int ("month",
						     P_("Month"),
						     P_("The selected month (as a number between 0 and 11)"),
						     0, 11, 0,
						     GTK_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_DAY,
                                   g_param_spec_int ("day",
						     P_("Day"),
						     P_("The selected day (as a number between 1 and 31, or 0 to unselect the currently selected day)"),
						     0, 31, 0,
						     GTK_PARAM_READWRITE));

/**
 * GclCalendar:show-heading:
 *
 * Determines whether a heading is displayed.
 *
 * Since: 2.4
 */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_HEADING,
                                   g_param_spec_boolean ("show-heading",
							 P_("Show Heading"),
							 P_("If TRUE, a heading is displayed"),
							 TRUE,
							 GTK_PARAM_READWRITE));

/**
 * GclCalendar:show-day-names:
 *
 * Determines whether day names are displayed.
 *
 * Since: 2.4
 */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_DAY_NAMES,
                                   g_param_spec_boolean ("show-day-names",
							 P_("Show Day Names"),
							 P_("If TRUE, day names are displayed"),
							 TRUE,
							 GTK_PARAM_READWRITE));
/**
 * GclCalendar:no-month-change:
 *
 * Determines whether the selected month can be changed.
 *
 * Since: 2.4
 */
  g_object_class_install_property (gobject_class,
                                   PROP_NO_MONTH_CHANGE,
                                   g_param_spec_boolean ("no-month-change",
							 P_("No Month Change"),
							 P_("If TRUE, the selected month cannot be changed"),
							 FALSE,
							 GTK_PARAM_READWRITE));

/**
 * GclCalendar:show-week-numbers:
 *
 * Determines whether week numbers are displayed.
 *
 * Since: 2.4
 */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_WEEK_NUMBERS,
                                   g_param_spec_boolean ("show-week-numbers",
							 P_("Show Week Numbers"),
							 P_("If TRUE, week numbers are displayed"),
							 FALSE,
							 GTK_PARAM_READWRITE));
/**
 * GclCalendar:show-lunar-date:
 *
 * Determines whether chinese lunar date are displayed.
 */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_LUNAR,
                                   g_param_spec_boolean ("show-lunar-date",
							 P_("Show Chinese Lunar Date"),
							 P_("If TRUE, Chinese Lunar Date are displayed"),
							 FALSE,
							 GTK_PARAM_READWRITE));

  gcl_calendar_signals[MONTH_CHANGED_SIGNAL] =
    g_signal_new (I_("month_changed"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GclCalendarClass, month_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  gcl_calendar_signals[DAY_SELECTED_SIGNAL] =
    g_signal_new (I_("day_selected"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GclCalendarClass, day_selected),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  gcl_calendar_signals[DAY_SELECTED_DOUBLE_CLICK_SIGNAL] =
    g_signal_new (I_("day_selected_double_click"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GclCalendarClass, day_selected_double_click),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  gcl_calendar_signals[PREV_MONTH_SIGNAL] =
    g_signal_new (I_("prev_month"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GclCalendarClass, prev_month),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  gcl_calendar_signals[NEXT_MONTH_SIGNAL] =
    g_signal_new (I_("next_month"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GclCalendarClass, next_month),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  gcl_calendar_signals[PREV_YEAR_SIGNAL] =
    g_signal_new (I_("prev_year"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GclCalendarClass, prev_year),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  gcl_calendar_signals[NEXT_YEAR_SIGNAL] =
    g_signal_new (I_("next_year"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GclCalendarClass, next_year),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  
  g_type_class_add_private (gobject_class, sizeof (GclCalendarPrivate));
}

static void
gcl_calendar_init (GclCalendar *calendar)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  time_t secs;
  struct tm *tm;
  gint i;
  char buffer[255];
  time_t tmp_time;
  GclCalendarPrivate *priv;
  gchar *year_before;
#ifdef HAVE__NL_TIME_FIRST_WEEKDAY
  gchar *langinfo;
  gint week_1stday = 0;
  gint first_weekday = 1;
  guint week_origin;
#else
  gchar *week_start;
#endif

  priv = calendar->priv = G_TYPE_INSTANCE_GET_PRIVATE (calendar,
						       GCL_TYPE_CALENDAR,
						       GclCalendarPrivate);

  GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_FOCUS);
  
  if (!default_abbreviated_dayname[0])
    for (i=0; i<7; i++)
      {
          tmp_time= (i+3)*86400;
          strftime ( buffer, sizeof (buffer), "%a", gmtime (&tmp_time));
          default_abbreviated_dayname[i] = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
      }
  
  if (!default_monthname[0])
    for (i=0; i<12; i++)
      {
	tmp_time=i*2764800;
	strftime ( buffer, sizeof (buffer), "%B", gmtime (&tmp_time));
	default_monthname[i] = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
      }
  
  /* Set defaults */
  secs = time (NULL);
  tm = localtime (&secs);
  calendar->month = tm->tm_mon;
  calendar->year  = 1900 + tm->tm_year;

  for (i=0;i<31;i++)
    calendar->marked_date[i] = FALSE;
  calendar->num_marked_dates = 0;
  calendar->selected_day = tm->tm_mday;
  
  calendar->display_flags = ( GCL_CALENDAR_SHOW_HEADING | 
			      GCL_CALENDAR_SHOW_DAY_NAMES );
  
  calendar->highlight_row = -1;
  calendar->highlight_col = -1;
  
  calendar->focus_row = -1;
  calendar->focus_col = -1;

  priv->max_year_width = 0;
  priv->max_month_width = 0;
  priv->max_day_char_width = 0;
  priv->max_week_char_width = 0;

  priv->max_day_char_ascent = 0;
  priv->max_day_char_descent = 0;
  priv->max_label_char_ascent = 0;
  priv->max_label_char_descent = 0;

  priv->arrow_width = 10;

  priv->need_timer = 0;
  priv->timer = 0;
  priv->click_child = -1;

  priv->in_drag = 0;
  priv->drag_highlight = 0;

  gtk_drag_dest_set (widget, 0, NULL, 0, GDK_ACTION_COPY);
  gtk_drag_dest_add_text_targets (widget);

  priv->year_before = 0;
  priv->date = gcl_date_new();

  /* Translate to calendar:YM if you want years to be displayed
   * before months; otherwise translate to calendar:MY.
   * Do *not* translate it to anything else, if it
   * it isn't calendar:YM or calendar:MY it will not work.
   *
   * Note that this flipping is in top of the text direction flipping,
   * so if you have a default text direction of RTL and YM, then
   * the year will appear on the right.
   */
  year_before = _("calendar:MY");
  if (strcmp (year_before, "calendar:YM") == 0)
    priv->year_before = 1;
  else if (strcmp (year_before, "calendar:MY") != 0)
    g_warning ("Whoever translated calendar:MY did so wrongly.\n");

#ifdef HAVE__NL_TIME_FIRST_WEEKDAY
  langinfo = nl_langinfo (_NL_TIME_FIRST_WEEKDAY);
  first_weekday = langinfo[0];
  langinfo = nl_langinfo (_NL_TIME_WEEK_1STDAY);
  week_origin = GPOINTER_TO_INT (langinfo);
  if (week_origin == 19971130) /* Sunday */
    week_1stday = 0;
  else if (week_origin == 19971201) /* Monday */
    week_1stday = 1;
  else
    g_warning ("Unknown value of _NL_TIME_WEEK_1STDAY.\n");

  priv->week_start = (week_1stday + first_weekday - 1) % 7;
#else
  /* Translate to calendar:week_start:0 if you want Sunday to be the
   * first day of the week to calendar:week_start:1 if you want Monday
   * to be the first day of the week, and so on.
   */  
  week_start = _("calendar:week_start:0");

  if (strncmp (week_start, "calendar:week_start:", 20) == 0)
    priv->week_start = *(week_start + 20) - '0';
  else 
    priv->week_start = -1;
  
  if (priv->week_start < 0 || priv->week_start > 6)
    {
      g_warning ("Whoever translated calendar:week_start:0 did so wrongly.\n");
      priv->week_start = 0;
    }
#endif

  calendar_compute_days (calendar);
}

/****************************************
 *          Utility Functions           *
 ****************************************/

static void
calendar_set_month_next (GclCalendar *calendar)
{
  gint month_len;
  
  g_return_if_fail (GTK_IS_WIDGET (calendar));
  
  if (calendar->display_flags & GCL_CALENDAR_NO_MONTH_CHANGE)
    return;
  
  
  if (calendar->month == 11)
    {
      calendar->month = 0;
      calendar->year++;
    } 
  else 
    calendar->month++;
  
  calendar_compute_days (calendar);
  g_signal_emit (calendar,
		 gcl_calendar_signals[NEXT_MONTH_SIGNAL],
		 0);
  g_signal_emit (calendar,
		 gcl_calendar_signals[MONTH_CHANGED_SIGNAL],
		 0);
  
  month_len = month_length[leap (calendar->year)][calendar->month + 1];
  
  if (month_len < calendar->selected_day)
    {
      calendar->selected_day = 0;
      gcl_calendar_select_day (calendar, month_len);
    }
  else
    gcl_calendar_select_day (calendar, calendar->selected_day);

  gtk_widget_queue_draw (GTK_WIDGET (calendar));
}

static void
calendar_set_year_prev (GclCalendar *calendar)
{
  gint month_len;
  
  g_return_if_fail (GTK_IS_WIDGET (calendar));
  
  calendar->year--;
  calendar_compute_days (calendar);
  g_signal_emit (calendar,
		 gcl_calendar_signals[PREV_YEAR_SIGNAL],
		 0);
  g_signal_emit (calendar,
		 gcl_calendar_signals[MONTH_CHANGED_SIGNAL],
		 0);
  
  month_len = month_length[leap (calendar->year)][calendar->month + 1];
  
  if (month_len < calendar->selected_day)
    {
      calendar->selected_day = 0;
      gcl_calendar_select_day (calendar, month_len);
    }
  else
    gcl_calendar_select_day (calendar, calendar->selected_day);
  
  gtk_widget_queue_draw (GTK_WIDGET (calendar));
}

static void
calendar_set_year_next (GclCalendar *calendar)
{
  gint month_len;
  
  g_return_if_fail (GTK_IS_WIDGET (calendar));
  
  calendar->year++;
  calendar_compute_days (calendar);
  g_signal_emit (calendar,
		 gcl_calendar_signals[NEXT_YEAR_SIGNAL],
		 0);
  g_signal_emit (calendar,
		 gcl_calendar_signals[MONTH_CHANGED_SIGNAL],
		 0);
  
  month_len = month_length[leap (calendar->year)][calendar->month + 1];
  
  if (month_len < calendar->selected_day)
    {
      calendar->selected_day = 0;
      gcl_calendar_select_day (calendar, month_len);
    }
  else
    gcl_calendar_select_day (calendar, calendar->selected_day);
  
  gtk_widget_queue_draw (GTK_WIDGET (calendar));
}

static void
calendar_compute_days (GclCalendar *calendar)
{
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (GTK_WIDGET (calendar));
  gint month;
  gint year;
  gint ndays_in_month;
  gint ndays_in_prev_month;
  gint first_day;
  gint row;
  gint col;
  gint day;

  //yetist
  guint tmp_year=0;
  guint tmp_month;

  g_return_if_fail (GCL_IS_CALENDAR (calendar));

  year = calendar->year;
  month = calendar->month + 1;
  
//  if (calendar->display_flags & GCL_CALENDAR_SHOW_LUNAR)
//  {
      priv->lunar_year=gcl_calendar_lunar_year_str(calendar, year, month, calendar->selected_day);
//  }
  ndays_in_month = month_length[leap (year)][month];
  
  first_day = day_of_week (year, month, 1);
  first_day = (first_day + 7 - priv->week_start) % 7;
  
  /* Compute days of previous month */
  if (month > 1)
    ndays_in_prev_month = month_length[leap (year)][month-1];
  else
    ndays_in_prev_month = month_length[leap (year)][12];
  day = ndays_in_prev_month - first_day + 1;
  
  row = 0;
  if (first_day > 0)
    {
      for (col = 0; col < first_day; col++)
	{
	  calendar->day[row][col] = day;
          /* get lunar day of the prev month */
          //if (calendar->display_flags & GCL_CALENDAR_SHOW_LUNAR)
          //{
              if (month == 1)
              {
                  tmp_year = year-1;
                  tmp_month = 12;
              }else{
                  tmp_year = year;
                  tmp_month = month-1;
              }
              priv->lunar_day[row][col]=gcl_calendar_lunar_day_str(calendar, tmp_year, tmp_month, day);
          //}
	  calendar->day_month[row][col] = MONTH_PREV;
	  day++;
	}
    }
  
  /* Compute days of current month */
  col = first_day;
  for (day = 1; day <= ndays_in_month; day++)
    {
      calendar->day[row][col] = day;
      /* get lunar day of the current month */
      //if (calendar->display_flags & GCL_CALENDAR_SHOW_LUNAR)
      //{
          priv->lunar_day[row][col]=gcl_calendar_lunar_day_str(calendar, year, month, day);
      //}
      calendar->day_month[row][col] = MONTH_CURRENT;
      
      col++;
      if (col == 7)
	{
	  row++;
	  col = 0;
	}
    }
  
  /* Compute days of next month */
  day = 1;
  for (; row <= 5; row++)
    {
      for (; col <= 6; col++)
	{
	  calendar->day[row][col] = day;
          /* get lunar day of the next month */
          //if (calendar->display_flags & GCL_CALENDAR_SHOW_LUNAR)
          //{
              if (month == 12)
              {
                  tmp_year = year+1;
                  tmp_month =1;
              }else{
                  tmp_year = year;
                  tmp_month = month+1;
              }
              priv->lunar_day[row][col]=gcl_calendar_lunar_day_str(calendar, tmp_year, tmp_month, day);
          //}
	  calendar->day_month[row][col] = MONTH_NEXT;
	  day++;
	}
      col = 0;
    }
}

static void
calendar_select_and_focus_day (GclCalendar *calendar,
			       guint        day)
{
  gint old_focus_row = calendar->focus_row;
  gint old_focus_col = calendar->focus_col;
  gint row;
  gint col;
  
  for (row = 0; row < 6; row ++)
    for (col = 0; col < 7; col++)
      {
	if (calendar->day_month[row][col] == MONTH_CURRENT 
	    && calendar->day[row][col] == day)
	  {
	    calendar->focus_row = row;
	    calendar->focus_col = col;
	  }
      }

  if (old_focus_row != -1 && old_focus_col != -1)
    calendar_invalidate_day (calendar, old_focus_row, old_focus_col);
  
  gcl_calendar_select_day (calendar, day);
}


/****************************************
 *     Layout computation utilities     *
 ****************************************/

static gint
calendar_row_height (GclCalendar *calendar)
{
  return (GCL_CALENDAR_GET_PRIVATE (calendar)->main_h - CALENDAR_MARGIN
	  - ((calendar->display_flags & GCL_CALENDAR_SHOW_DAY_NAMES)
	     ? CALENDAR_YSEP : CALENDAR_MARGIN)) / 6;
}


/* calendar_left_x_for_column: returns the x coordinate
 * for the left of the column */
static gint
calendar_left_x_for_column (GclCalendar *calendar,
			    gint	 column)
{
  gint width;
  gint x_left;
  
  if (gtk_widget_get_direction (GTK_WIDGET (calendar)) == GTK_TEXT_DIR_RTL)
    column = 6 - column;

  width = GCL_CALENDAR_GET_PRIVATE (calendar)->day_width;
  if (calendar->display_flags & GCL_CALENDAR_SHOW_WEEK_NUMBERS)
    x_left = CALENDAR_XSEP + (width + DAY_XSEP) * column;
  else
    x_left = CALENDAR_MARGIN + (width + DAY_XSEP) * column;
  
  return x_left;
}

/* column_from_x: returns the column 0-6 that the
 * x pixel of the xwindow is in */
static gint
calendar_column_from_x (GclCalendar *calendar,
			gint	     event_x)
{
  gint c, column;
  gint x_left, x_right;
  
  column = -1;
  
  for (c = 0; c < 7; c++)
    {
      x_left = calendar_left_x_for_column (calendar, c);
      x_right = x_left + GCL_CALENDAR_GET_PRIVATE (calendar)->day_width;
      
      if (event_x >= x_left && event_x < x_right)
	{
	  column = c;
	  break;
	}
    }
  
  return column;
}

/* calendar_top_y_for_row: returns the y coordinate
 * for the top of the row */
static gint
calendar_top_y_for_row (GclCalendar *calendar,
			gint	     row)
{
  
  return (GCL_CALENDAR_GET_PRIVATE (calendar)->main_h 
	  - (CALENDAR_MARGIN + (6 - row)
	     * calendar_row_height (calendar)));
}

/* row_from_y: returns the row 0-5 that the
 * y pixel of the xwindow is in */
static gint
calendar_row_from_y (GclCalendar *calendar,
		     gint	  event_y)
{
  gint r, row;
  gint height;
  gint y_top, y_bottom;
  
  height = calendar_row_height (calendar);
  row = -1;
  
  for (r = 0; r < 6; r++)
    {
      y_top = calendar_top_y_for_row (calendar, r);
      y_bottom = y_top + height;
      
      if (event_y >= y_top && event_y < y_bottom)
	{
	  row = r;
	  break;
	}
    }
  
  return row;
}

static void
calendar_arrow_rectangle (GclCalendar  *calendar,
			  guint	        arrow,
			  GdkRectangle *rect)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);
  gboolean year_left;

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR) 
    year_left = priv->year_before;
  else
    year_left = !priv->year_before;
    
  rect->y = 3;
  rect->width = priv->arrow_width;
  rect->height = priv->header_h - 7;
  
  switch (arrow)
    {
    case ARROW_MONTH_LEFT:
      if (year_left) 
	rect->x = (widget->allocation.width - 2 * widget->style->xthickness
		   - (3 + 2*priv->arrow_width 
		      + priv->max_month_width));
      else
	rect->x = 3;
      break;
    case ARROW_MONTH_RIGHT:
      if (year_left) 
	rect->x = (widget->allocation.width - 2 * widget->style->xthickness 
		   - 3 - priv->arrow_width);
      else
	rect->x = (priv->arrow_width 
		   + priv->max_month_width);
      break;
    case ARROW_YEAR_LEFT:
      if (year_left) 
	rect->x = 3;
      else
	rect->x = (widget->allocation.width - 2 * widget->style->xthickness
		   - (3 + 2*priv->arrow_width 
		      + priv->max_year_width));
      break;
    case ARROW_YEAR_RIGHT:
      if (year_left) 
	rect->x = (priv->arrow_width 
		   + priv->max_year_width);
      else
	rect->x = (widget->allocation.width - 2 * widget->style->xthickness 
		   - 3 - priv->arrow_width);
      break;
    }
}

static void
calendar_day_rectangle (GclCalendar  *calendar,
			gint          row,
			gint          col,
			GdkRectangle *rect)
{
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);

  rect->x = calendar_left_x_for_column (calendar, col);
  rect->y = calendar_top_y_for_row (calendar, row);
  rect->height = calendar_row_height (calendar);
  rect->width = priv->day_width;
}

static void
calendar_set_month_prev (GclCalendar *calendar)
{
  gint month_len;
  
  if (calendar->display_flags & GCL_CALENDAR_NO_MONTH_CHANGE)
    return;
  
  if (calendar->month == 0)
    {
      calendar->month = 11;
      calendar->year--;
    } 
  else 
    calendar->month--;
  
  month_len = month_length[leap (calendar->year)][calendar->month + 1];
  
  calendar_compute_days (calendar);
  
  g_signal_emit (calendar,
		 gcl_calendar_signals[PREV_MONTH_SIGNAL],
		 0);
  g_signal_emit (calendar,
		 gcl_calendar_signals[MONTH_CHANGED_SIGNAL],
		 0);
  
  if (month_len < calendar->selected_day)
    {
      calendar->selected_day = 0;
      gcl_calendar_select_day (calendar, month_len);
    }
  else
    {
      if (calendar->selected_day < 0)
	calendar->selected_day = calendar->selected_day + 1 + month_length[leap (calendar->year)][calendar->month + 1];
      gcl_calendar_select_day (calendar, calendar->selected_day);
    }

  gtk_widget_queue_draw (GTK_WIDGET (calendar));
}


/****************************************
 *           Basic object methods       *
 ****************************************/

static void
gcl_calendar_finalize (GObject *object)
{
  (* G_OBJECT_CLASS (gcl_calendar_parent_class)->finalize) (object);
}

static void
gcl_calendar_destroy (GtkObject *object)
{
  calendar_stop_spinning (GCL_CALENDAR (object));
  
  GTK_OBJECT_CLASS (gcl_calendar_parent_class)->destroy (object);
}


static void
calendar_set_display_option (GclCalendar              *calendar,
			     GclCalendarDisplayOptions flag,
			     gboolean                  setting)
{
  GclCalendarDisplayOptions flags;
  if (setting) 
    flags = calendar->display_flags | flag;
  else
    flags = calendar->display_flags & ~flag; 
  gcl_calendar_display_options (calendar, flags);
}

static gboolean
calendar_get_display_option (GclCalendar              *calendar,
			     GclCalendarDisplayOptions flag)
{
  return (calendar->display_flags & flag) != 0;
}

static void 
gcl_calendar_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  GclCalendar *calendar;

  calendar = GCL_CALENDAR (object);

  switch (prop_id) 
    {
    case PROP_YEAR:
      gcl_calendar_select_month (calendar,
				 calendar->month,
				 g_value_get_int (value));
      break;
    case PROP_MONTH:
      gcl_calendar_select_month (calendar,
				 g_value_get_int (value),
				 calendar->year);
      break;
    case PROP_DAY:
      gcl_calendar_select_day (calendar,
			       g_value_get_int (value));
      break;
    case PROP_SHOW_HEADING:
      calendar_set_display_option (calendar,
				   GCL_CALENDAR_SHOW_HEADING,
				   g_value_get_boolean (value));
      break;
    case PROP_SHOW_DAY_NAMES:
      calendar_set_display_option (calendar,
				   GCL_CALENDAR_SHOW_DAY_NAMES,
				   g_value_get_boolean (value));
      break;
    case PROP_NO_MONTH_CHANGE:
      calendar_set_display_option (calendar,
				   GCL_CALENDAR_NO_MONTH_CHANGE,
				   g_value_get_boolean (value));
      break;
    case PROP_SHOW_WEEK_NUMBERS:
      calendar_set_display_option (calendar,
				   GCL_CALENDAR_SHOW_WEEK_NUMBERS,
				   g_value_get_boolean (value));
      break;
    case PROP_SHOW_LUNAR:
      calendar_set_display_option (calendar,
				   GCL_CALENDAR_SHOW_LUNAR,
				   g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
gcl_calendar_get_property (GObject      *object,
			   guint         prop_id,
			   GValue       *value,
			   GParamSpec   *pspec)
{
  GclCalendar *calendar;

  calendar = GCL_CALENDAR (object);

  switch (prop_id) 
    {
    case PROP_YEAR:
      g_value_set_int (value, calendar->year);
      break;
    case PROP_MONTH:
      g_value_set_int (value, calendar->month);
      break;
    case PROP_DAY:
      g_value_set_int (value, calendar->selected_day);
      break;
    case PROP_SHOW_HEADING:
      g_value_set_boolean (value, calendar_get_display_option (calendar,
							       GCL_CALENDAR_SHOW_HEADING));
      break;
    case PROP_SHOW_DAY_NAMES:
      g_value_set_boolean (value, calendar_get_display_option (calendar,
							       GCL_CALENDAR_SHOW_DAY_NAMES));
      break;
    case PROP_NO_MONTH_CHANGE:
      g_value_set_boolean (value, calendar_get_display_option (calendar,
							       GCL_CALENDAR_NO_MONTH_CHANGE));
      break;
    case PROP_SHOW_WEEK_NUMBERS:
      g_value_set_boolean (value, calendar_get_display_option (calendar,
							       GCL_CALENDAR_SHOW_WEEK_NUMBERS));
      break;
    case PROP_SHOW_LUNAR:
      g_value_set_boolean (value, calendar_get_display_option (calendar,
							       GCL_CALENDAR_SHOW_LUNAR));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


/****************************************
 *             Realization              *
 ****************************************/

static void
calendar_realize_arrows (GclCalendar *calendar)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);
  GdkWindowAttr attributes;
  gint attributes_mask;
  gint i;
  
  /* Arrow windows ------------------------------------- */
  if (! (calendar->display_flags & GCL_CALENDAR_NO_MONTH_CHANGE)
      && (calendar->display_flags & GCL_CALENDAR_SHOW_HEADING))
    {
      attributes.wclass = GDK_INPUT_OUTPUT;
      attributes.window_type = GDK_WINDOW_CHILD;
      attributes.visual = gtk_widget_get_visual (widget);
      attributes.colormap = gtk_widget_get_colormap (widget);
      attributes.event_mask = (gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK
			       | GDK_BUTTON_PRESS_MASK	| GDK_BUTTON_RELEASE_MASK
			       | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
      attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
      for (i = 0; i < 4; i++)
	{
	  GdkRectangle rect;
	  calendar_arrow_rectangle (calendar, i, &rect);
	  
	  attributes.x = rect.x;
	  attributes.y = rect.y;
	  attributes.width = rect.width;
	  attributes.height = rect.height;
	  priv->arrow_win[i] = gdk_window_new (priv->header_win,
					       &attributes, 
					       attributes_mask);
	  if (GTK_WIDGET_IS_SENSITIVE (widget))
	    priv->arrow_state[i] = GTK_STATE_NORMAL;
	  else 
	    priv->arrow_state[i] = GTK_STATE_INSENSITIVE;
	  gdk_window_set_background (priv->arrow_win[i],
				     HEADER_BG_COLOR (GTK_WIDGET (calendar)));
	  gdk_window_show (priv->arrow_win[i]);
	  gdk_window_set_user_data (priv->arrow_win[i], widget);
	}
    }
  else
    {
      for (i = 0; i < 4; i++)
	priv->arrow_win[i] = NULL;
    }
}

static void
calendar_realize_header (GclCalendar *calendar)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);
  GdkWindowAttr attributes;
  gint attributes_mask;
  
  /* Header window ------------------------------------- */
  if (calendar->display_flags & GCL_CALENDAR_SHOW_HEADING)
    {
      attributes.wclass = GDK_INPUT_OUTPUT;
      attributes.window_type = GDK_WINDOW_CHILD;
      attributes.visual = gtk_widget_get_visual (widget);
      attributes.colormap = gtk_widget_get_colormap (widget);
      attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
      attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
      attributes.x = widget->style->xthickness;
      attributes.y = widget->style->ythickness;
      attributes.width = widget->allocation.width - 2 * attributes.x;
      attributes.height = priv->header_h - 2 * attributes.y;
      priv->header_win = gdk_window_new (widget->window,
					 &attributes, attributes_mask);
      
      gdk_window_set_background (priv->header_win,
				 HEADER_BG_COLOR (GTK_WIDGET (calendar)));
      gdk_window_show (priv->header_win);
      gdk_window_set_user_data (priv->header_win, widget);
      
    }
  else
    {
      priv->header_win = NULL;
    }
  calendar_realize_arrows (calendar);
}

static void
calendar_realize_day_names (GclCalendar *calendar)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);
  GdkWindowAttr attributes;
  gint attributes_mask;
  
  /* Day names	window --------------------------------- */
  if ( calendar->display_flags & GCL_CALENDAR_SHOW_DAY_NAMES)
    {
      attributes.wclass = GDK_INPUT_OUTPUT;
      attributes.window_type = GDK_WINDOW_CHILD;
      attributes.visual = gtk_widget_get_visual (widget);
      attributes.colormap = gtk_widget_get_colormap (widget);
      attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
      attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
      attributes.x = (widget->style->xthickness + INNER_BORDER);
      attributes.y = priv->header_h + (widget->style->ythickness 
					   + INNER_BORDER);
      attributes.width = (widget->allocation.width 
			  - (widget->style->xthickness + INNER_BORDER) 
			  * 2);
      attributes.height = priv->day_name_h;
      priv->day_name_win = gdk_window_new (widget->window,
					   &attributes, 
					   attributes_mask);
      gdk_window_set_background (priv->day_name_win, 
				 BACKGROUND_COLOR ( GTK_WIDGET ( calendar)));
      gdk_window_show (priv->day_name_win);
      gdk_window_set_user_data (priv->day_name_win, widget);
    }
  else
    {
      priv->day_name_win = NULL;
    }
}

static void
calendar_realize_week_numbers (GclCalendar *calendar)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);
  GdkWindowAttr attributes;
  gint attributes_mask;
  
  /* Week number window -------------------------------- */
  if (calendar->display_flags & GCL_CALENDAR_SHOW_WEEK_NUMBERS)
    {
      attributes.wclass = GDK_INPUT_OUTPUT;
      attributes.window_type = GDK_WINDOW_CHILD;
      attributes.visual = gtk_widget_get_visual (widget);
      attributes.colormap = gtk_widget_get_colormap (widget);
      attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
      
      attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
      attributes.x = widget->style->xthickness + INNER_BORDER;
      attributes.y = (priv->header_h + priv->day_name_h 
		      + (widget->style->ythickness + INNER_BORDER));
      attributes.width = priv->week_width;
      attributes.height = priv->main_h;
      priv->week_win = gdk_window_new (widget->window,
				       &attributes, attributes_mask);
      gdk_window_set_background (priv->week_win,  
				 BACKGROUND_COLOR (GTK_WIDGET (calendar)));
      gdk_window_show (priv->week_win);
      gdk_window_set_user_data (priv->week_win, widget);
    } 
  else
    {
      priv->week_win = NULL;
    }
}

static void
gcl_calendar_realize (GtkWidget *widget)
{
  GclCalendar *calendar = GCL_CALENDAR (widget);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);
  GdkWindowAttr attributes;
  gint attributes_mask;

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask =  (gtk_widget_get_events (widget) 
			    | GDK_EXPOSURE_MASK |GDK_KEY_PRESS_MASK | GDK_SCROLL_MASK);
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window,
				   &attributes, attributes_mask);
  
  widget->style = gtk_style_attach (widget->style, widget->window);
  
  /* Header window ------------------------------------- */
  calendar_realize_header (calendar);
  /* Day names	window --------------------------------- */
  calendar_realize_day_names (calendar);
  /* Week number window -------------------------------- */
  calendar_realize_week_numbers (calendar);
  /* Main Window --------------------------------------	 */
  attributes.event_mask =  (gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK
			    | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
			    | GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK);
  
  attributes.x = priv->week_width + (widget->style->ythickness + INNER_BORDER);
  attributes.y = (priv->header_h + priv->day_name_h 
		  + (widget->style->ythickness + INNER_BORDER));
  attributes.width = (widget->allocation.width - attributes.x 
		      - (widget->style->xthickness + INNER_BORDER));
  attributes.height = priv->main_h;
  priv->main_win = gdk_window_new (widget->window,
				   &attributes, attributes_mask);
  gdk_window_set_background (priv->main_win, 
			     BACKGROUND_COLOR ( GTK_WIDGET ( calendar)));
  gdk_window_show (priv->main_win);
  gdk_window_set_user_data (priv->main_win, widget);
  gdk_window_set_background (widget->window, BACKGROUND_COLOR (widget));
  gdk_window_show (widget->window);
  gdk_window_set_user_data (widget->window, widget);
}

static void
gcl_calendar_unrealize (GtkWidget *widget)
{
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);
  gint i;
  
  if (priv->header_win)
    {
      for (i = 0; i < 4; i++)
	{
	  if (priv->arrow_win[i])
	    {
	      gdk_window_set_user_data (priv->arrow_win[i], NULL);
	      gdk_window_destroy (priv->arrow_win[i]);
	      priv->arrow_win[i] = NULL;
	    }
	}
      gdk_window_set_user_data (priv->header_win, NULL);
      gdk_window_destroy (priv->header_win);
      priv->header_win = NULL;
    }
  
  if (priv->week_win)
    {
      gdk_window_set_user_data (priv->week_win, NULL);
      gdk_window_destroy (priv->week_win);
      priv->week_win = NULL;      
    }
  
  if (priv->main_win)
    {
      gdk_window_set_user_data (priv->main_win, NULL);
      gdk_window_destroy (priv->main_win);
      priv->main_win = NULL;      
    }
  if (priv->day_name_win)
    {
      gdk_window_set_user_data (priv->day_name_win, NULL);
      gdk_window_destroy (priv->day_name_win);
      priv->day_name_win = NULL;      
    }
  
  if (GTK_WIDGET_CLASS (gcl_calendar_parent_class)->unrealize)
    (* GTK_WIDGET_CLASS (gcl_calendar_parent_class)->unrealize) (widget);
}


/****************************************
 *       Size Request and Allocate      *
 ****************************************/

static void
gcl_calendar_size_request (GtkWidget	  *widget,
			   GtkRequisition *requisition)
{
  GclCalendar *calendar = GCL_CALENDAR (widget);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);
  PangoLayout *layout;
  PangoRectangle logical_rect;

  gint height;
  gint i;
  gint calendar_margin = CALENDAR_MARGIN;
  gint header_width, main_width;
  gint max_header_height = 0;
  gint focus_width;
  gint focus_padding;
  
  gtk_widget_style_get (GTK_WIDGET (widget),
			"focus-line-width", &focus_width,
			"focus-padding", &focus_padding,
			NULL);

  layout = gtk_widget_create_pango_layout (widget, NULL);
  
  /*
   * Calculate the requisition	width for the widget.
   */
  
  /* Header width */
  
  if (calendar->display_flags & GCL_CALENDAR_SHOW_HEADING)
    {
      priv->max_month_width = 0;
      for (i = 0; i < 12; i++)
	{
	  pango_layout_set_text (layout, default_monthname[i], -1);
	  pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
	  priv->max_month_width = MAX (priv->max_month_width,
					       logical_rect.width + 8);
	  max_header_height = MAX (max_header_height, logical_rect.height); 
	}

      priv->max_year_width = 0;
      /* Translators:  This is a text measurement template.
       * Translate it to the widest year text. 
       * 
       * Don't include the prefix "year measurement template|" 
       * in the translation.
       *
       * If you don't understand this, leave it as "2000"
       */
      //compute year width
      if (calendar->display_flags & GCL_CALENDAR_SHOW_LUNAR)
      {
          GString*    tmp_year_format = g_string_new("2000");
          g_string_append (tmp_year_format, gcl_calendar_lunar_year_str (calendar, 1957, 11, 20));
          pango_layout_set_text (layout, Q_(tmp_year_format->str), -1);
          g_string_free (tmp_year_format, TRUE);
      }
      else
          pango_layout_set_text (layout, Q_("2000"), -1);	  
      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
      priv->max_year_width = MAX (priv->max_year_width,
				  logical_rect.width + 8);
      max_header_height = MAX (max_header_height, logical_rect.height); 
    } 
  else 
    {
      priv->max_month_width = 0;
      priv->max_year_width = 0;
    }
  
  if (calendar->display_flags & GCL_CALENDAR_NO_MONTH_CHANGE)
    header_width = (priv->max_month_width 
		    + priv->max_year_width
		    + 3 * 3);
  else
    header_width = (priv->max_month_width 
		    + priv->max_year_width
		    + 4 * priv->arrow_width + 3 * 3);

  /* Mainwindow labels width */
  
  priv->max_day_char_width = 0;
  priv->max_day_char_ascent = 0;
  priv->max_day_char_descent = 0;
  priv->min_day_width = 1;

  for (i = 0; i < 9; i++)
    {
      gchar buffer[32];
      /* comput day width */
      if (calendar->display_flags & GCL_CALENDAR_SHOW_LUNAR)                 
          g_snprintf (buffer, sizeof (buffer), "%s", _("Shi2Er2Yue4"));
      else
          g_snprintf (buffer, sizeof (buffer), Q_("%d"), i * 11);
      pango_layout_set_text (layout, buffer, -1);	  
      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
      priv->min_day_width = MAX (priv->min_day_width,
					 logical_rect.width);

      priv->max_day_char_ascent = MAX (priv->max_day_char_ascent,
					       PANGO_ASCENT (logical_rect));
      priv->max_day_char_descent = MAX (priv->max_day_char_descent, 
						PANGO_DESCENT (logical_rect));
    }
  /* We add one to max_day_char_width to be able to make the marked day "bold" */
  priv->max_day_char_width = priv->min_day_width / 2 + 1;
  
  priv->max_label_char_ascent = 0;
  priv->max_label_char_descent = 0;
  if (calendar->display_flags & GCL_CALENDAR_SHOW_DAY_NAMES)
    for (i = 0; i < 7; i++)
      {
	pango_layout_set_text (layout, default_abbreviated_dayname[i], -1);
	pango_layout_line_get_pixel_extents (pango_layout_get_lines (layout)->data, NULL, &logical_rect);

	priv->min_day_width = MAX (priv->min_day_width, logical_rect.width);
	priv->max_label_char_ascent = MAX (priv->max_label_char_ascent,
						   PANGO_ASCENT (logical_rect));
	priv->max_label_char_descent = MAX (priv->max_label_char_descent, 
						    PANGO_DESCENT (logical_rect));
      }
  
  priv->max_week_char_width = 0;
  if (calendar->display_flags & GCL_CALENDAR_SHOW_WEEK_NUMBERS)
    for (i = 0; i < 9; i++)
      {
	gchar buffer[32];
        /* compute week width */
        if (calendar->display_flags & GCL_CALENDAR_SHOW_LUNAR)
            g_snprintf (buffer, sizeof (buffer), _("%dZhou1"), i * 11);
        else
            g_snprintf (buffer, sizeof (buffer), "%d", i * 11);
	pango_layout_set_text (layout, buffer, -1);	  
	pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
	priv->max_week_char_width = MAX (priv->max_week_char_width,
					   logical_rect.width / 2);
      }
  
  main_width = (7 * (priv->min_day_width + (focus_padding + focus_width) * 2) + (DAY_XSEP * 6) + CALENDAR_MARGIN * 2
		+ (priv->max_week_char_width
		   ? priv->max_week_char_width * 2 + (focus_padding + focus_width) * 2 + CALENDAR_XSEP * 2
		   : 0));
  
  
  requisition->width = MAX (header_width, main_width + INNER_BORDER * 2) + widget->style->xthickness * 2;
  
  /*
   * Calculate the requisition height for the widget.
   */
  
  if (calendar->display_flags & GCL_CALENDAR_SHOW_HEADING)
    {
      priv->header_h = (max_header_height + CALENDAR_YSEP * 2);
    }
  else
    {
      priv->header_h = 0;
    }
  
  if (calendar->display_flags & GCL_CALENDAR_SHOW_DAY_NAMES)
    {
      priv->day_name_h = (priv->max_label_char_ascent
				  + priv->max_label_char_descent
				  + 2 * (focus_padding + focus_width) + calendar_margin);
      calendar_margin = CALENDAR_YSEP;
    } 
  else
    {
      priv->day_name_h = 0;
    }

  //by yetist:compute main_eight, priv->max_day_char_descent *2 two rows
  if (calendar->display_flags & GCL_CALENDAR_SHOW_LUNAR)                 
  {
      priv->main_h = (CALENDAR_MARGIN + calendar_margin
              + 6 * (priv->max_day_char_ascent
                  + priv->max_day_char_descent*2
                  + 2 * (focus_padding + focus_width))
              + DAY_YSEP * 5);
  }
  else
  {
      priv->main_h = (CALENDAR_MARGIN + calendar_margin
              + 6 * (priv->max_day_char_ascent
                  + priv->max_day_char_descent
                  + 2 * (focus_padding + focus_width))
              + DAY_YSEP * 5);

  }
  
  height = (priv->header_h + priv->day_name_h 
	    + priv->main_h);
  
  requisition->height = height + (widget->style->ythickness + INNER_BORDER) * 2;

  g_object_unref (layout);
}

static void
gcl_calendar_size_allocate (GtkWidget	  *widget,
			    GtkAllocation *allocation)
{
  GclCalendar *calendar = GCL_CALENDAR (widget);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);
  gint xthickness = widget->style->xthickness;
  gint ythickness = widget->style->xthickness;
  guint i;
  
  widget->allocation = *allocation;
    
  if (calendar->display_flags & GCL_CALENDAR_SHOW_WEEK_NUMBERS)
    {
      priv->day_width = (priv->min_day_width
			 * ((allocation->width - (xthickness + INNER_BORDER) * 2
			     - (CALENDAR_MARGIN * 2) -  (DAY_XSEP * 6) - CALENDAR_XSEP * 2))
			 / (7 * priv->min_day_width + priv->max_week_char_width * 2));
      priv->week_width = ((allocation->width - (xthickness + INNER_BORDER) * 2
			   - (CALENDAR_MARGIN * 2) - (DAY_XSEP * 6) - CALENDAR_XSEP * 2 )
			  - priv->day_width * 7 + CALENDAR_MARGIN + CALENDAR_XSEP);
    } 
  else 
    {
      priv->day_width = (allocation->width
			 - (xthickness + INNER_BORDER) * 2
			 - (CALENDAR_MARGIN * 2)
			 - (DAY_XSEP * 6))/7;
      priv->week_width = 0;
    }
  
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);
      if (priv->header_win)
	gdk_window_move_resize (priv->header_win,
				xthickness, ythickness,
				allocation->width - 2 * xthickness, priv->header_h);

      for (i = 0 ; i < 4 ; i++)
	{
	  if (priv->arrow_win[i])
	    {
	      GdkRectangle rect;
	      calendar_arrow_rectangle (calendar, i, &rect);
	  
	      gdk_window_move_resize (priv->arrow_win[i],
				      rect.x, rect.y, rect.width, rect.height);
	    }
	}
      
      if (priv->day_name_win)
	gdk_window_move_resize (priv->day_name_win,
				xthickness + INNER_BORDER,
				priv->header_h + (widget->style->ythickness + INNER_BORDER),
				allocation->width - (xthickness + INNER_BORDER) * 2,
				priv->day_name_h);
      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR) 
	{
	  if (priv->week_win)
	    gdk_window_move_resize (priv->week_win,
				    (xthickness + INNER_BORDER),
				    priv->header_h + priv->day_name_h
				    + (widget->style->ythickness + INNER_BORDER),
				    priv->week_width,
				    priv->main_h);
	  gdk_window_move_resize (priv->main_win,
				  priv->week_width + (xthickness + INNER_BORDER),
				  priv->header_h + priv->day_name_h
				  + (widget->style->ythickness + INNER_BORDER),
				  allocation->width 
				  - priv->week_width 
				  - (xthickness + INNER_BORDER) * 2,
				  priv->main_h);
	}
      else 
	{
	  gdk_window_move_resize (priv->main_win,
				  (xthickness + INNER_BORDER),
				  priv->header_h + priv->day_name_h
				  + (widget->style->ythickness + INNER_BORDER),
				  allocation->width 
				  - priv->week_width 
				  - (xthickness + INNER_BORDER) * 2,
				  priv->main_h);
	  if (priv->week_win)
	    gdk_window_move_resize (priv->week_win,
				    allocation->width 
				    - priv->week_width 
				    - (xthickness + INNER_BORDER),
				    priv->header_h + priv->day_name_h
				    + (widget->style->ythickness + INNER_BORDER),
				    priv->week_width,
				    priv->main_h);
	}
    }
}


/****************************************
 *              Repainting              *
 ****************************************/

static void
calendar_paint_header (GclCalendar *calendar)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);
  cairo_t *cr;
  char buffer[255];
  int x, y;
  gint header_width;
  gint max_month_width;
  gint max_year_width;
  PangoLayout *layout;
  PangoRectangle logical_rect;
  gboolean year_left;
  time_t tmp_time;
  struct tm *tm;
  gchar *str;

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR) 
    year_left = priv->year_before;
  else
    year_left = !priv->year_before;

  cr = gdk_cairo_create (priv->header_win);
  
  header_width = widget->allocation.width - 2 * widget->style->xthickness;
  
  max_month_width = priv->max_month_width;
  max_year_width = priv->max_year_width;
  
  gtk_paint_shadow (widget->style, priv->header_win,
		    GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		    NULL, widget, "calendar",
		    0, 0, header_width, priv->header_h);

  tmp_time = 1;  /* Jan 1 1970, 00:00:01 UTC */
  tm = gmtime (&tmp_time);
  tm->tm_year = calendar->year - 1900;

  /* Translators: This dictates how the year is displayed in
   * gtkcalendar widget.  See strftime() manual for the format.
   * Use only ASCII in the translation.
   *
   * Also look for the msgid "year measurement template|2000".  
   * Translate that entry to a year with the widest output of this
   * msgid. 
   * 
   * Don't include the prefix "calendar year format|" in the 
   * translation. "%Y" is appropriate for most locales.
   */
  strftime (buffer, sizeof (buffer), Q_("%Y"), tm);
  if (calendar->display_flags & GCL_CALENDAR_SHOW_LUNAR)                 
      g_sprintf(buffer, "%s%s", buffer, priv->lunar_year);
  str = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
  layout = gtk_widget_create_pango_layout (widget, str);
  g_free (str);
  
  pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
  
  /* Draw title */
  y = (priv->header_h - logical_rect.height) / 2;
  
  /* Draw year and its arrows */
  
  if (calendar->display_flags & GCL_CALENDAR_NO_MONTH_CHANGE)
    if (year_left)
      x = 3 + (max_year_width - logical_rect.width)/2;
    else
      x = header_width - (3 + max_year_width
			  - (max_year_width - logical_rect.width)/2);
  else
    if (year_left)
      x = 3 + priv->arrow_width + (max_year_width - logical_rect.width)/2;
    else
      x = header_width - (3 + priv->arrow_width + max_year_width
			  - (max_year_width - logical_rect.width)/2);
  

  gdk_cairo_set_source_color (cr, HEADER_FG_COLOR (GTK_WIDGET (calendar)));
  cairo_move_to (cr, x, y);
  pango_cairo_show_layout (cr, layout);
  
  /* Draw month */
  g_snprintf (buffer, sizeof (buffer), "%s", default_monthname[calendar->month]);
  pango_layout_set_text (layout, buffer, -1);
  pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

  if (calendar->display_flags & GCL_CALENDAR_NO_MONTH_CHANGE)
    if (year_left)
      x = header_width - (3 + max_month_width
			  - (max_month_width - logical_rect.width)/2);      
    else
    x = 3 + (max_month_width - logical_rect.width) / 2;
  else
    if (year_left)
      x = header_width - (3 + priv->arrow_width + max_month_width
			  - (max_month_width - logical_rect.width)/2);
    else
    x = 3 + priv->arrow_width + (max_month_width - logical_rect.width)/2;

  cairo_move_to (cr, x, y);
  pango_cairo_show_layout (cr, layout);

  g_object_unref (layout);
  cairo_destroy (cr);
}

static void
calendar_paint_day_names (GclCalendar *calendar)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);
  cairo_t *cr;
  char buffer[255];
  int day,i;
  int day_width, cal_width;
  int day_wid_sep;
  PangoLayout *layout;
  PangoRectangle logical_rect;
  gint focus_padding;
  gint focus_width;
  
  cr = gdk_cairo_create (priv->day_name_win);
  
  gtk_widget_style_get (GTK_WIDGET (widget),
			"focus-line-width", &focus_width,
			"focus-padding", &focus_padding,
			NULL);
  
  day_width = priv->day_width;
  cal_width = widget->allocation.width;
  day_wid_sep = day_width + DAY_XSEP;
  
  /*
   * Draw rectangles as inverted background for the labels.
   */

  gdk_cairo_set_source_color (cr, SELECTED_BG_COLOR (widget));
  cairo_rectangle (cr,
		   CALENDAR_MARGIN, CALENDAR_MARGIN,
		   cal_width-CALENDAR_MARGIN * 2,
		   priv->day_name_h - CALENDAR_MARGIN);
  cairo_fill (cr);
  
  if (calendar->display_flags & GCL_CALENDAR_SHOW_WEEK_NUMBERS)
    {
      cairo_rectangle (cr, 
		       CALENDAR_MARGIN,
		       priv->day_name_h - CALENDAR_YSEP,
		       priv->week_width - CALENDAR_YSEP - CALENDAR_MARGIN,
		       CALENDAR_YSEP);
      cairo_fill (cr);
    }
  
  /*
   * Write the labels
   */

  layout = gtk_widget_create_pango_layout (widget, NULL);

  gdk_cairo_set_source_color (cr, SELECTED_FG_COLOR (widget));
  for (i = 0; i < 7; i++)
    {
      if (gtk_widget_get_direction (GTK_WIDGET (calendar)) == GTK_TEXT_DIR_RTL)
	day = 6 - i;
      else
	day = i;
      day = (day + priv->week_start) % 7;
      g_snprintf (buffer, sizeof (buffer), "%s", default_abbreviated_dayname[day]);

      pango_layout_set_text (layout, buffer, -1);
      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

      cairo_move_to (cr, 
		     (CALENDAR_MARGIN +
		      + (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR ?
			 (priv->week_width + (priv->week_width ? CALENDAR_XSEP : 0))
			 : 0)
		      + day_wid_sep * i
		      + (day_width - logical_rect.width)/2),
		     CALENDAR_MARGIN + focus_width + focus_padding + logical_rect.y);
      pango_cairo_show_layout (cr, layout);
    }
  
  g_object_unref (layout);
  cairo_destroy (cr);
}

static void
calendar_paint_week_numbers (GclCalendar *calendar)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);
  cairo_t *cr;
  gint row, week = 0, year;
  gint x_loc;
  char buffer[32];
  gint y_loc, day_height;
  PangoLayout *layout;
  PangoRectangle logical_rect;
  gint focus_padding;
  gint focus_width;
  
  cr = gdk_cairo_create (priv->week_win);
  
  gtk_widget_style_get (GTK_WIDGET (widget),
			"focus-line-width", &focus_width,
			"focus-padding", &focus_padding,
			NULL);
  
  /*
   * Draw a rectangle as inverted background for the labels.
   */

  gdk_cairo_set_source_color (cr, SELECTED_BG_COLOR (widget));
  if (priv->day_name_win)
    cairo_rectangle (cr, 
		     CALENDAR_MARGIN,
		     0,
		     priv->week_width - CALENDAR_MARGIN,
		     priv->main_h - CALENDAR_MARGIN);
  else
    cairo_rectangle (cr,
		     CALENDAR_MARGIN,
		     CALENDAR_MARGIN,
		     priv->week_width - CALENDAR_MARGIN,
		     priv->main_h - 2 * CALENDAR_MARGIN);
  cairo_fill (cr);
  
  /*
   * Write the labels
   */
  
  layout = gtk_widget_create_pango_layout (widget, NULL);
  
  gdk_cairo_set_source_color (cr, SELECTED_FG_COLOR (widget));
  day_height = calendar_row_height (calendar);
  for (row = 0; row < 6; row++)
    {
      gboolean result;
      
      year = calendar->year;
      if (calendar->day[row][6] < 15 && row > 3 && calendar->month == 11)
	year++;

      result = week_of_year (&week, &year,		
			     ((calendar->day[row][6] < 15 && row > 3 ? 1 : 0)
			      + calendar->month) % 12 + 1, calendar->day[row][6]);
      g_return_if_fail (result);

      /* Translators: this defines whether the week numbers should use
       * localized digits or the ones used in English (0123...).
       *
       * Translate to "%Id" if you want to use localized digits, or
       * translate to "%d" otherwise.  Don't include the
       * "calendar:week:digits|" part in the translation.
       *
       * Note that translating this doesn't guarantee that you get localized
       * digits.  That needs support from your system and locale definition
       * too.
       */
      /* show week */
      if (calendar->display_flags & GCL_CALENDAR_SHOW_LUNAR)                 
          g_snprintf (buffer, sizeof (buffer), _("%dZhou1"), week);
      else
          g_snprintf (buffer, sizeof (buffer), "%d", week);
      pango_layout_set_text (layout, buffer, -1);
      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

      y_loc = calendar_top_y_for_row (calendar, row) + (day_height - logical_rect.height) / 2;

      x_loc = (priv->week_width
	       - logical_rect.width
	       - CALENDAR_XSEP - focus_padding - focus_width);

      cairo_move_to (cr, x_loc, y_loc);
      pango_cairo_show_layout (cr, layout);
    }
  
  g_object_unref (layout);
  cairo_destroy (cr);
}

static void
calendar_invalidate_day_num (GclCalendar *calendar,
			     gint         day)
{
  gint r, c, row, col;
  
  row = -1;
  col = -1;
  for (r = 0; r < 6; r++)
    for (c = 0; c < 7; c++)
      if (calendar->day_month[r][c] == MONTH_CURRENT &&
	  calendar->day[r][c] == day)
	{
	  row = r;
	  col = c;
	}
  
  g_return_if_fail (row != -1);
  g_return_if_fail (col != -1);
  
  calendar_invalidate_day (calendar, row, col);
}

static void
calendar_invalidate_day (GclCalendar *calendar,
			 gint         row,
			 gint         col)
{
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);

  if (priv->main_win)
    {
      GdkRectangle day_rect;
      
      calendar_day_rectangle (calendar, row, col, &day_rect);
      gdk_window_invalidate_rect (priv->main_win, &day_rect, FALSE);
    }
}

static void
calendar_paint_day (GclCalendar *calendar,
		    gint	     row,
		    gint	     col)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);
  cairo_t *cr;
  GdkColor *text_color;
  gchar buffer[32];
  gint day;
  gint x_loc, y_loc;
  GdkRectangle day_rect;
  PangoLayout *layout;
  PangoRectangle logical_rect;
  gchar* lunar_day;
  
  g_return_if_fail (row < 6);
  g_return_if_fail (col < 7);

  cr = gdk_cairo_create (priv->main_win);

  day = calendar->day[row][col];
  //yetist get lunar_day from array for show
  if (calendar->display_flags & GCL_CALENDAR_SHOW_LUNAR)                 
  {
      lunar_day = priv->lunar_day[row][col];
  }

  calendar_day_rectangle (calendar, row, col, &day_rect);
  
  if (calendar->day_month[row][col] == MONTH_PREV)
    {
      text_color = PREV_MONTH_COLOR (widget);
    } 
  else if (calendar->day_month[row][col] == MONTH_NEXT)
    {
      text_color =  NEXT_MONTH_COLOR (widget);
    } 
  else 
    {
#if 0      
      if (calendar->highlight_row == row && calendar->highlight_col == col)
	{
	  cairo_set_source_color (cr, HIGHLIGHT_BG_COLOR (widget));
	  gdk_cairo_rectangle (cr, &day_rect);
	  cairo_fill (cr);
	}
#endif     
      if (calendar->selected_day == day)
	{
	  gdk_cairo_set_source_color (cr, SELECTED_BG_COLOR (widget));
	  gdk_cairo_rectangle (cr, &day_rect);
	  cairo_fill (cr);
	}
      if (calendar->selected_day == day)
	text_color = SELECTED_FG_COLOR (widget);
      else if (calendar->marked_date[day-1])
	text_color = MARKED_COLOR (widget);
      else
	text_color = NORMAL_DAY_COLOR (widget);
    }

  /* Translators: this defines whether the day numbers should use
   * localized digits or the ones used in English (0123...).
   *
   * Translate to "%Id" if you want to use localized digits, or
   * translate to "%d" otherwise.  Don't include the "calendar:day:digits|"
   * part in the translation.
   *
   * Note that translating this doesn't guarantee that you get localized
   * digits.  That needs support from your system and locale definition
   * too.
   */
  // by yetist show days
  if (calendar->display_flags & GCL_CALENDAR_SHOW_LUNAR)                 
  {
      gint len;
      len = strlen(lunar_day) % 2 == 0? strlen(lunar_day)/2: strlen(lunar_day)/2+1;
      g_snprintf (buffer, sizeof (buffer), "%*d\n%s", len ,day, lunar_day);
  }
  else
      g_snprintf (buffer, sizeof (buffer), "%d", day);
  layout = gtk_widget_create_pango_layout (widget, buffer);
  pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
  
  x_loc = day_rect.x + day_rect.width / 2 + priv->max_day_char_width;
  x_loc -= logical_rect.width;
  y_loc = day_rect.y + (day_rect.height - logical_rect.height) / 2;
  
  gdk_cairo_set_source_color (cr, text_color);
  cairo_move_to (cr, x_loc, y_loc);
  pango_cairo_show_layout (cr, layout);
    
  if (calendar->marked_date[day-1]
      && calendar->day_month[row][col] == MONTH_CURRENT)
    {
      cairo_move_to (cr, x_loc - 1, y_loc);
      pango_cairo_show_layout (cr, layout);
    }

  if (GTK_WIDGET_HAS_FOCUS (calendar) 
      && calendar->focus_row == row && calendar->focus_col == col)
    {
      GtkStateType state;

      if (calendar->selected_day == day)
	state = GTK_WIDGET_HAS_FOCUS (widget) ? GTK_STATE_SELECTED : GTK_STATE_ACTIVE;
      else
	state = GTK_STATE_NORMAL;
      
      gtk_paint_focus (widget->style, 
		       priv->main_win,
	               state,
#if 0
		       (calendar->selected_day == day) 
		          ? GTK_STATE_SELECTED : GTK_STATE_NORMAL, 
#endif
		       NULL, widget, "calendar-day",
		       day_rect.x,     day_rect.y, 
		       day_rect.width, day_rect.height);
    }

  g_object_unref (layout);
  cairo_destroy (cr);
}

static void
calendar_paint_main (GclCalendar *calendar)
{
  gint row, col;
  
  for (col = 0; col < 7; col++)
    for (row = 0; row < 6; row++)
      calendar_paint_day (calendar, row, col);
}

static void
calendar_invalidate_header (GclCalendar *calendar)
{
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);
  GdkWindow *window;
  
  window = priv->header_win;
  if (window)
    gdk_window_invalidate_rect (window, NULL, FALSE);
}

static void
calendar_invalidate_arrow (GclCalendar *calendar,
			   guint        arrow)
{
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);
  GdkWindow *window;
  
  window = priv->arrow_win[arrow];
  if (window)
    gdk_window_invalidate_rect (window, NULL, FALSE);
}

static void
calendar_paint_arrow (GclCalendar *calendar,
		      guint	       arrow)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);
  GdkWindow *window;
  
  window = priv->arrow_win[arrow];
  if (window)
    {
      cairo_t *cr = gdk_cairo_create (window);
      gint width, height;
      gint state;
	
      state = priv->arrow_state[arrow];

      gdk_cairo_set_source_color (cr, &widget->style->bg[state]);
      cairo_paint (cr);
      cairo_destroy (cr);
      
      gdk_drawable_get_size (window, &width, &height);
      if (arrow == ARROW_MONTH_LEFT || arrow == ARROW_YEAR_LEFT)
	gtk_paint_arrow (widget->style, window, state, 
			 GTK_SHADOW_OUT, NULL, widget, "calendar",
			 GTK_ARROW_LEFT, TRUE, 
			 width/2 - 3, height/2 - 4, 8, 8);
      else 
	gtk_paint_arrow (widget->style, window, state, 
			 GTK_SHADOW_OUT, NULL, widget, "calendar",
			 GTK_ARROW_RIGHT, TRUE, 
			 width/2 - 2, height/2 - 4, 8, 8);
    }
}

static gboolean
gcl_calendar_expose (GtkWidget	    *widget,
		     GdkEventExpose *event)
{
  GclCalendar *calendar = GCL_CALENDAR (widget);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);
  int i;

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      if (event->window == priv->main_win)
	calendar_paint_main (calendar);
      
      if (event->window == priv->header_win)
	calendar_paint_header (calendar);

      for (i = 0; i < 4; i++)
	if (event->window == priv->arrow_win[i])
	  calendar_paint_arrow (calendar, i);
      
      if (event->window == priv->day_name_win)
	calendar_paint_day_names (calendar);
      
      if (event->window == priv->week_win)
	calendar_paint_week_numbers (calendar);
      if (event->window == widget->window)
	{
	  gtk_paint_shadow (widget->style, widget->window, GTK_WIDGET_STATE (widget),
			    GTK_SHADOW_IN, NULL, widget, "calendar",
			    0, 0, widget->allocation.width, widget->allocation.height);
	}
    }
  
  return FALSE;
}


/****************************************
 *           Mouse handling             *
 ****************************************/

static void
calendar_arrow_action (GclCalendar *calendar,
		       guint        arrow)
{
  switch (arrow)
    {
    case ARROW_YEAR_LEFT:
      calendar_set_year_prev (calendar);
      break;
    case ARROW_YEAR_RIGHT:
      calendar_set_year_next (calendar);
      break;
    case ARROW_MONTH_LEFT:
      calendar_set_month_prev (calendar);
      break;
    case ARROW_MONTH_RIGHT:
      calendar_set_month_next (calendar);
      break;
    default:;
      /* do nothing */
    }
}

static gboolean
calendar_timer (gpointer data)
{
  GclCalendar *calendar = data;
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);
  gboolean retval = FALSE;
  
  GDK_THREADS_ENTER ();

  if (priv->timer)
    {
      calendar_arrow_action (calendar, priv->click_child);

      if (priv->need_timer)
	{
          GtkSettings *settings;
          guint        timeout;

          settings = gtk_widget_get_settings (GTK_WIDGET (calendar));
          g_object_get (settings, "gtk-timeout-repeat", &timeout, NULL);

	  priv->need_timer = FALSE;
	  priv->timer = g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE,
					    timeout * SCROLL_DELAY_FACTOR,
					    (GSourceFunc) calendar_timer,
					    (gpointer) calendar, NULL);
	}
      else 
	retval = TRUE;
    }

  GDK_THREADS_LEAVE ();

  return retval;
}

static void
calendar_start_spinning (GclCalendar *calendar,
			 gint         click_child)
{
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);

  priv->click_child = click_child;
  
  if (!priv->timer)
    {
      GtkSettings *settings;
      guint        timeout;

      settings = gtk_widget_get_settings (GTK_WIDGET (calendar));
      g_object_get (settings, "gtk-timeout-initial", &timeout, NULL);

      priv->need_timer = TRUE;
      priv->timer = g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE,
					timeout,
					(GSourceFunc) calendar_timer,
					(gpointer) calendar, NULL);
    }
}

static void
calendar_stop_spinning (GclCalendar *calendar)
{
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);

  if (priv->timer)
    {
      g_source_remove (priv->timer);
      priv->timer = 0;
      priv->need_timer = FALSE;
    }
}

static void
calendar_main_button_press (GclCalendar	   *calendar,
			    GdkEventButton *event)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);
  gint x, y;
  gint row, col;
  gint day_month;
  gint day;

  gint month;
  
  x = (gint) (event->x);
  y = (gint) (event->y);
  
  row = calendar_row_from_y (calendar, y);
  col = calendar_column_from_x (calendar, x);

  /* If row or column isn't found, just return. */
  if (row == -1 || col == -1)
    return;
  
  day_month = calendar->day_month[row][col];

  if (event->type == GDK_BUTTON_PRESS)
    {
      day = calendar->day[row][col];
      
      if (day_month == MONTH_PREV)
	calendar_set_month_prev (calendar);
      else if (day_month == MONTH_NEXT)
	calendar_set_month_next (calendar);
      else
        month=calendar->month;

      
      if (!GTK_WIDGET_HAS_FOCUS (widget))
	gtk_widget_grab_focus (widget);

	  
      if (event->button == 1) 
	{
	  priv->in_drag = 1;
	  priv->drag_start_x = x;
	  priv->drag_start_y = y;
	}

      calendar_select_and_focus_day (calendar, day);
    }
  else if (event->type == GDK_2BUTTON_PRESS)
    {
      priv->in_drag = 0;
      if (day_month == MONTH_CURRENT)
	g_signal_emit (calendar,
		       gcl_calendar_signals[DAY_SELECTED_DOUBLE_CLICK_SIGNAL],
		       0);
    }
}

static gboolean
gcl_calendar_button_press (GtkWidget	  *widget,
			   GdkEventButton *event)
{
  GclCalendar *calendar = GCL_CALENDAR (widget);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);
  gint arrow = -1;
  
  if (event->window == priv->main_win)
    calendar_main_button_press (calendar, event);

  if (!GTK_WIDGET_HAS_FOCUS (widget))
    gtk_widget_grab_focus (widget);

  for (arrow = ARROW_YEAR_LEFT; arrow <= ARROW_MONTH_RIGHT; arrow++)
    {
      if (event->window == priv->arrow_win[arrow])
	{
	  
	  /* only call the action on single click, not double */
	  if (event->type == GDK_BUTTON_PRESS)
	    {
	      if (event->button == 1)
		calendar_start_spinning (calendar, arrow);

	      calendar_arrow_action (calendar, arrow);	      
	    }

	  return TRUE;
	}
    }

  return FALSE;
}

static gboolean
gcl_calendar_button_release (GtkWidget	  *widget,
			     GdkEventButton *event)
{
  GclCalendar *calendar = GCL_CALENDAR (widget);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);

  if (event->button == 1) 
    {
      calendar_stop_spinning (calendar);

      if (priv->in_drag)
	priv->in_drag = 0;
    }

  return TRUE;
}

static gboolean
gcl_calendar_motion_notify (GtkWidget	   *widget,
			    GdkEventMotion *event)
{
  GclCalendar *calendar = GCL_CALENDAR (widget);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);
  gint event_x, event_y;
  gint row, col;
  gint old_row, old_col;
  
  event_x = (gint) (event->x);
  event_y = (gint) (event->y);
  
  if (event->window == priv->main_win)
    {
      
      if (priv->in_drag) 
	{
	  if (gtk_drag_check_threshold (widget,
					priv->drag_start_x, priv->drag_start_y,
					event->x, event->y))
	    {
	      GdkDragContext *context;
	      GtkTargetList *target_list = gtk_target_list_new (NULL, 0);
	      gtk_target_list_add_text_targets (target_list, 0);
	      context = gtk_drag_begin (widget, target_list, GDK_ACTION_COPY,
					1, (GdkEvent *)event);

	  
	      priv->in_drag = 0;
	      
	      gtk_target_list_unref (target_list);
	      gtk_drag_set_icon_default (context);
	    }
	}
      else 
	{
	  row = calendar_row_from_y (calendar, event_y);
	  col = calendar_column_from_x (calendar, event_x);
	  
	  if (row != calendar->highlight_row || calendar->highlight_col != col)
	    {
	      old_row = calendar->highlight_row;
	      old_col = calendar->highlight_col;
	      if (old_row > -1 && old_col > -1)
		{
		  calendar->highlight_row = -1;
		  calendar->highlight_col = -1;
		  calendar_invalidate_day (calendar, old_row, old_col);
		}
	      
	      calendar->highlight_row = row;
	      calendar->highlight_col = col;
	      
	      if (row > -1 && col > -1)
		calendar_invalidate_day (calendar, row, col);
	    }
	}
    }
  return TRUE;
}

static gboolean
gcl_calendar_enter_notify (GtkWidget	    *widget,
			   GdkEventCrossing *event)
{
  GclCalendar *calendar = GCL_CALENDAR (widget);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);
  
  if (event->window == priv->arrow_win[ARROW_MONTH_LEFT])
    {
      priv->arrow_state[ARROW_MONTH_LEFT] = GTK_STATE_PRELIGHT;
      calendar_invalidate_arrow (calendar, ARROW_MONTH_LEFT);
    }
  
  if (event->window == priv->arrow_win[ARROW_MONTH_RIGHT])
    {
      priv->arrow_state[ARROW_MONTH_RIGHT] = GTK_STATE_PRELIGHT;
      calendar_invalidate_arrow (calendar, ARROW_MONTH_RIGHT);
    }
  
  if (event->window == priv->arrow_win[ARROW_YEAR_LEFT])
    {
      priv->arrow_state[ARROW_YEAR_LEFT] = GTK_STATE_PRELIGHT;
      calendar_invalidate_arrow (calendar, ARROW_YEAR_LEFT);
    }
  
  if (event->window == priv->arrow_win[ARROW_YEAR_RIGHT])
    {
      priv->arrow_state[ARROW_YEAR_RIGHT] = GTK_STATE_PRELIGHT;
      calendar_invalidate_arrow (calendar, ARROW_YEAR_RIGHT);
    }
  
  return TRUE;
}

static gboolean
gcl_calendar_leave_notify (GtkWidget	    *widget,
			   GdkEventCrossing *event)
{
  GclCalendar *calendar = GCL_CALENDAR (widget);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);
  gint row;
  gint col;
  
  if (event->window == priv->main_win)
    {
      row = calendar->highlight_row;
      col = calendar->highlight_col;
      calendar->highlight_row = -1;
      calendar->highlight_col = -1;
      if (row > -1 && col > -1)
	calendar_invalidate_day (calendar, row, col);
    }
  
  if (event->window == priv->arrow_win[ARROW_MONTH_LEFT])
    {
      priv->arrow_state[ARROW_MONTH_LEFT] = GTK_STATE_NORMAL;
      calendar_invalidate_arrow (calendar, ARROW_MONTH_LEFT);
    }
  
  if (event->window == priv->arrow_win[ARROW_MONTH_RIGHT])
    {
      priv->arrow_state[ARROW_MONTH_RIGHT] = GTK_STATE_NORMAL;
      calendar_invalidate_arrow (calendar, ARROW_MONTH_RIGHT);
    }
  
  if (event->window == priv->arrow_win[ARROW_YEAR_LEFT])
    {
      priv->arrow_state[ARROW_YEAR_LEFT] = GTK_STATE_NORMAL;
      calendar_invalidate_arrow (calendar, ARROW_YEAR_LEFT);
    }
  
  if (event->window == priv->arrow_win[ARROW_YEAR_RIGHT])
    {
      priv->arrow_state[ARROW_YEAR_RIGHT] = GTK_STATE_NORMAL;
      calendar_invalidate_arrow (calendar, ARROW_YEAR_RIGHT);
    }
  
  return TRUE;
}

static gboolean
gcl_calendar_scroll (GtkWidget      *widget,
		     GdkEventScroll *event)
{
  GclCalendar *calendar = GCL_CALENDAR (widget);

  if (event->direction == GDK_SCROLL_UP) 
    {
      if (!GTK_WIDGET_HAS_FOCUS (widget))
	gtk_widget_grab_focus (widget);
      calendar_set_month_prev (calendar);
    }
  else if (event->direction == GDK_SCROLL_DOWN) 
    {
      if (!GTK_WIDGET_HAS_FOCUS (widget))
	gtk_widget_grab_focus (widget);
      calendar_set_month_next (calendar);
    }
  else
    return FALSE;

  return TRUE;
}


/****************************************
 *             Key handling              *
 ****************************************/

static void 
move_focus (GclCalendar *calendar, 
	    gint         direction)
{
  GtkTextDirection text_dir = gtk_widget_get_direction (GTK_WIDGET (calendar));

  if ((text_dir == GTK_TEXT_DIR_LTR && direction == -1) ||
      (text_dir == GTK_TEXT_DIR_RTL && direction == 1)) 
    {
      if (calendar->focus_col > 0)
	  calendar->focus_col--;
      else if (calendar->focus_row > 0)
	{
	  calendar->focus_col = 6;
	  calendar->focus_row--;
	}
    }
  else 
    {
      if (calendar->focus_col < 6)
	calendar->focus_col++;
      else if (calendar->focus_row < 5)
	{
	  calendar->focus_col = 0;
	  calendar->focus_row++;
	}
    }
}

static gboolean
gcl_calendar_key_press (GtkWidget   *widget,
			GdkEventKey *event)
{
  GclCalendar *calendar;
  gint return_val;
  gint old_focus_row;
  gint old_focus_col;
  gint row, col, day;
  
  calendar = GCL_CALENDAR (widget);
  return_val = FALSE;
  
  old_focus_row = calendar->focus_row;
  old_focus_col = calendar->focus_col;

  switch (event->keyval)
    {
    case GDK_KP_Left:
    case GDK_Left:
      return_val = TRUE;
      if (event->state & GDK_CONTROL_MASK)
	calendar_set_month_prev (calendar);
      else
	{
	  move_focus (calendar, -1);
	  calendar_invalidate_day (calendar, old_focus_row, old_focus_col);
	  calendar_invalidate_day (calendar, calendar->focus_row,
				   calendar->focus_col);
	}
      break;
    case GDK_KP_Right:
    case GDK_Right:
      return_val = TRUE;
      if (event->state & GDK_CONTROL_MASK)
	calendar_set_month_next (calendar);
      else
	{
	  move_focus (calendar, 1);
	  calendar_invalidate_day (calendar, old_focus_row, old_focus_col);
	  calendar_invalidate_day (calendar, calendar->focus_row,
				   calendar->focus_col);
	}
      break;
    case GDK_KP_Up:
    case GDK_Up:
      return_val = TRUE;
      if (event->state & GDK_CONTROL_MASK)
	calendar_set_year_prev (calendar);
      else
	{
	  if (calendar->focus_row > 0)
	    calendar->focus_row--;
	  calendar_invalidate_day (calendar, old_focus_row, old_focus_col);
	  calendar_invalidate_day (calendar, calendar->focus_row,
				   calendar->focus_col);
	}
      break;
    case GDK_KP_Down:
    case GDK_Down:
      return_val = TRUE;
      if (event->state & GDK_CONTROL_MASK)
	calendar_set_year_next (calendar);
      else
	{
	  if (calendar->focus_row < 5)
	    calendar->focus_row++;
	  calendar_invalidate_day (calendar, old_focus_row, old_focus_col);
	  calendar_invalidate_day (calendar, calendar->focus_row,
				   calendar->focus_col);
	}
      break;
    case GDK_KP_Space:
    case GDK_space:
      row = calendar->focus_row;
      col = calendar->focus_col;
      
      if (row > -1 && col > -1)
	{
	  return_val = TRUE;

          day = calendar->day[row][col];
	  if (calendar->day_month[row][col] == MONTH_PREV)
	    calendar_set_month_prev (calendar);
	  else if (calendar->day_month[row][col] == MONTH_NEXT)
	    calendar_set_month_next (calendar);

	  calendar_select_and_focus_day (calendar, day);
	}
    }	
  
  return return_val;
}


/****************************************
 *           Misc widget methods        *
 ****************************************/

static void
calendar_set_background (GtkWidget *widget)
{
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);
  gint i;
  
  if (GTK_WIDGET_REALIZED (widget))
    {
      for (i = 0; i < 4; i++)
	{
	  if (priv->arrow_win[i])
	    gdk_window_set_background (priv->arrow_win[i], 
				       HEADER_BG_COLOR (widget));
	}
      if (priv->header_win)
	gdk_window_set_background (priv->header_win, 
				   HEADER_BG_COLOR (widget));
      if (priv->day_name_win)
	gdk_window_set_background (priv->day_name_win, 
				   BACKGROUND_COLOR (widget));
      if (priv->week_win)
	gdk_window_set_background (priv->week_win,
				   BACKGROUND_COLOR (widget));
      if (priv->main_win)
	gdk_window_set_background (priv->main_win,
				   BACKGROUND_COLOR (widget));
      if (widget->window)
	gdk_window_set_background (widget->window,
				   BACKGROUND_COLOR (widget)); 
    }
}

static void
gcl_calendar_style_set (GtkWidget *widget,
			GtkStyle  *previous_style)
{
  if (previous_style && GTK_WIDGET_REALIZED (widget))
    calendar_set_background (widget);
}

static void
gcl_calendar_state_changed (GtkWidget	   *widget,
			    GtkStateType    previous_state)
{
  GclCalendar *calendar = GCL_CALENDAR (widget);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);
  int i;
  
  if (!GTK_WIDGET_IS_SENSITIVE (widget))
    {
      priv->in_drag = 0;
      calendar_stop_spinning (calendar);    
    }

  for (i = 0; i < 4; i++)
    if (GTK_WIDGET_IS_SENSITIVE (widget))
      priv->arrow_state[i] = GTK_STATE_NORMAL;
    else 
      priv->arrow_state[i] = GTK_STATE_INSENSITIVE;
  
  calendar_set_background (widget);
}

static void
gcl_calendar_grab_notify (GtkWidget *widget,
			  gboolean   was_grabbed)
{
  if (!was_grabbed)
    calendar_stop_spinning (GCL_CALENDAR (widget));
}

static gboolean
gcl_calendar_focus_out (GtkWidget     *widget,
			GdkEventFocus *event)
{
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);

  gtk_widget_queue_draw (widget);

  calendar_stop_spinning (GCL_CALENDAR (widget));
  
  priv->in_drag = 0; 

  return FALSE;
}


/****************************************
 *          Drag and Drop               *
 ****************************************/

static void
gcl_calendar_drag_data_get (GtkWidget        *widget,
			    GdkDragContext   *context,
			    GtkSelectionData *selection_data,
			    guint             info,
			    guint             time)
{
  GclCalendar *calendar = GCL_CALENDAR (widget);
  GDate *date;
  gchar str[128];
  gsize len;

  date = g_date_new_dmy (calendar->selected_day, calendar->month + 1, calendar->year);
  len = g_date_strftime (str, 127, "%x", date);
  gtk_selection_data_set_text (selection_data, str, len);
  
  g_free (date);
}

/* Get/set whether drag_motion requested the drag data and
 * drag_data_received should thus not actually insert the data,
 * since the data doesn't result from a drop.
 */
static void
set_status_pending (GdkDragContext *context,
                    GdkDragAction   suggested_action)
{
  g_object_set_data (G_OBJECT (context),
                     I_("gtk-calendar-status-pending"),
                     GINT_TO_POINTER (suggested_action));
}

static GdkDragAction
get_status_pending (GdkDragContext *context)
{
  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (context),
                                             "gtk-calendar-status-pending"));
}

static void
gcl_calendar_drag_leave (GtkWidget      *widget,
			 GdkDragContext *context,
			 guint           time)
{
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);

  priv->drag_highlight = 0;
  gtk_drag_unhighlight (widget);
  
}

static gboolean
gcl_calendar_drag_motion (GtkWidget      *widget,
			  GdkDragContext *context,
			  gint            x,
			  gint            y,
			  guint           time)
{
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (widget);
  GdkAtom target;
  
  if (!priv->drag_highlight) 
    {
      priv->drag_highlight = 1;
      gtk_drag_highlight (widget);
    }
  
  target = gtk_drag_dest_find_target (widget, context, NULL);
  if (target == GDK_NONE || context->suggested_action == 0)
    gdk_drag_status (context, 0, time);
  else
    {
      set_status_pending (context, context->suggested_action);
      gtk_drag_get_data (widget, context, target, time);
    }
  
  return TRUE;
}

static gboolean
gcl_calendar_drag_drop (GtkWidget      *widget,
			GdkDragContext *context,
			gint            x,
			gint            y,
			guint           time)
{
  GdkAtom target;

  target = gtk_drag_dest_find_target (widget, context, NULL);  
  if (target != GDK_NONE)
    {
      gtk_drag_get_data (widget, context, 
			 target, 
			 time);
      return TRUE;
    }

  return FALSE;
}

static void
gcl_calendar_drag_data_received (GtkWidget        *widget,
				 GdkDragContext   *context,
				 gint              x,
				 gint              y,
				 GtkSelectionData *selection_data,
				 guint             info,
				 guint             time)
{
  GclCalendar *calendar = GCL_CALENDAR (widget);
  guint day, month, year;
  gchar *str;
  GDate *date;
  GdkDragAction suggested_action;

  suggested_action = get_status_pending (context);

  if (suggested_action) 
    {
      set_status_pending (context, 0);
     
      /* We are getting this data due to a request in drag_motion,
       * rather than due to a request in drag_drop, so we are just
       * supposed to call drag_status, not actually paste in the
       * data.
       */
      str = gtk_selection_data_get_text (selection_data);
      if (str) 
	{
	  date = g_date_new ();
	  g_date_set_parse (date, str);
	  if (!g_date_valid (date)) 
	      suggested_action = 0;
	  g_date_free (date);
	  g_free (str);
	}
      else
	suggested_action = 0;

      gdk_drag_status (context, suggested_action, time);

      return;
    }

  date = g_date_new ();
  str = gtk_selection_data_get_text (selection_data);
  if (str) 
    {
      g_date_set_parse (date, str);
      g_free (str);
    }
  
  if (!g_date_valid (date)) 
    {
      g_warning ("Received invalid date data\n");
      g_date_free (date);	
      gtk_drag_finish (context, FALSE, FALSE, time);
      return;
    }

  day = g_date_get_day (date);
  month = g_date_get_month (date);
  year = g_date_get_year (date);
  g_date_free (date);	

  gtk_drag_finish (context, TRUE, FALSE, time);

  
  g_object_freeze_notify (G_OBJECT (calendar));
  if (!(calendar->display_flags & GCL_CALENDAR_NO_MONTH_CHANGE)
      && (calendar->display_flags & GCL_CALENDAR_SHOW_HEADING))
    gcl_calendar_select_month (calendar, month - 1, year);
  gcl_calendar_select_day (calendar, day);
  g_object_thaw_notify (G_OBJECT (calendar));  
}


/****************************************
 *              Public API              *
 ****************************************/

/**
 * gcl_calendar_new:
 * 
 * Creates a new calendar, with the current date being selected. 
 * 
 * Return value: a newly #GclCalendar widget
 **/
GtkWidget*
gcl_calendar_new (void)
{
  return g_object_new (GCL_TYPE_CALENDAR, NULL);
}

/**
 * gcl_calendar_display_options:
 * @calendar: a #GclCalendar.
 * @flags: the display options to set.
 *
 * Sets display options (whether to display the heading and the month headings).
 * 
 * Deprecated: 2.4: Use gcl_calendar_set_display_options() instead
 **/
void
gcl_calendar_display_options (GclCalendar	       *calendar,
			      GclCalendarDisplayOptions flags)
{
  gcl_calendar_set_display_options (calendar, flags);
}

/**
 * gcl_calendar_get_display_options:
 * @calendar: a #GclCalendar
 * 
 * Returns the current display options of @calendar. 
 * 
 * Return value: the display options.
 *
 * Since: 2.4
 **/
GclCalendarDisplayOptions 
gcl_calendar_get_display_options (GclCalendar         *calendar)
{
  g_return_val_if_fail (GCL_IS_CALENDAR (calendar), 0);

  return calendar->display_flags;
}

/**
 * gcl_calendar_set_display_options:
 * @calendar: a #GclCalendar
 * @flags: the display options to set
 * 
 * Sets display options (whether to display the heading and the month  
 * headings).
 *
 * Since: 2.4
 **/
void
gcl_calendar_set_display_options (GclCalendar	       *calendar,
				  GclCalendarDisplayOptions flags)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);
  gint resize = 0;
  gint i;
  GclCalendarDisplayOptions old_flags;
  
  g_return_if_fail (GCL_IS_CALENDAR (calendar));
  
  old_flags = calendar->display_flags;
  
  if (GTK_WIDGET_REALIZED (widget))
    {
      if ((flags ^ calendar->display_flags) & GCL_CALENDAR_NO_MONTH_CHANGE)
	{
	  resize ++;
	  if (! (flags & GCL_CALENDAR_NO_MONTH_CHANGE)
	      && (priv->header_win))
	    {
	      calendar->display_flags &= ~GCL_CALENDAR_NO_MONTH_CHANGE;
	      calendar_realize_arrows (calendar);
	    }
	  else
	    {
	      for (i = 0; i < 4; i++)
		{
		  if (priv->arrow_win[i])
		    {
		      gdk_window_set_user_data (priv->arrow_win[i], 
						NULL);
		      gdk_window_destroy (priv->arrow_win[i]);
		      priv->arrow_win[i] = NULL;
		    }
		}
	    }
	}
      
      if ((flags ^ calendar->display_flags) & GCL_CALENDAR_SHOW_HEADING)
	{
	  resize++;
	  
	  if (flags & GCL_CALENDAR_SHOW_HEADING)
	    {
	      calendar->display_flags |= GCL_CALENDAR_SHOW_HEADING;
	      calendar_realize_header (calendar);
	    }
	  else
	    {
	      for (i = 0; i < 4; i++)
		{
		  if (priv->arrow_win[i])
		    {
		      gdk_window_set_user_data (priv->arrow_win[i], 
						NULL);
		      gdk_window_destroy (priv->arrow_win[i]);
		      priv->arrow_win[i] = NULL;
		    }
		}
	      gdk_window_set_user_data (priv->header_win, NULL);
	      gdk_window_destroy (priv->header_win);
	      priv->header_win = NULL;
	    }
	}
      
      
      if ((flags ^ calendar->display_flags) & GCL_CALENDAR_SHOW_DAY_NAMES)
	{
	  resize++;
	  
	  if (flags & GCL_CALENDAR_SHOW_DAY_NAMES)
	    {
	      calendar->display_flags |= GCL_CALENDAR_SHOW_DAY_NAMES;
	      calendar_realize_day_names (calendar);
	    }
	  else
	    {
	      gdk_window_set_user_data (priv->day_name_win, NULL);
	      gdk_window_destroy (priv->day_name_win);
	      priv->day_name_win = NULL;
	    }
	}
      
      if ((flags ^ calendar->display_flags) & GCL_CALENDAR_SHOW_WEEK_NUMBERS)
	{
	  resize++;
	  
	  if (flags & GCL_CALENDAR_SHOW_WEEK_NUMBERS)
	    {
	      calendar->display_flags |= GCL_CALENDAR_SHOW_WEEK_NUMBERS;
	      calendar_realize_week_numbers (calendar);
	    }
	  else
	    {
	      gdk_window_set_user_data (priv->week_win, NULL);
	      gdk_window_destroy (priv->week_win);
	      priv->week_win = NULL;
	    }
	}

      if ((flags ^ calendar->display_flags) & GCL_CALENDAR_SHOW_LUNAR)
	{
	  resize++;
	  
	  if (flags & GCL_CALENDAR_SHOW_LUNAR)
	    {
	      calendar->display_flags |= GCL_CALENDAR_SHOW_LUNAR;
	    }
	}
      if ((flags ^ calendar->display_flags) & GCL_CALENDAR_WEEK_START_MONDAY)
	g_warning ("GCL_CALENDAR_WEEK_START_MONDAY is ignored; the first day of the week is determined from the locale");
      
      calendar->display_flags = flags;
      if (resize)
	gtk_widget_queue_resize (GTK_WIDGET (calendar));
      
    } 
  else
    calendar->display_flags = flags;
  
  g_object_freeze_notify (G_OBJECT (calendar));
  if ((old_flags ^ calendar->display_flags) & GCL_CALENDAR_SHOW_HEADING)
    g_object_notify (G_OBJECT (calendar), "show-heading");
  if ((old_flags ^ calendar->display_flags) & GCL_CALENDAR_SHOW_DAY_NAMES)
    g_object_notify (G_OBJECT (calendar), "show-day-names");
  if ((old_flags ^ calendar->display_flags) & GCL_CALENDAR_NO_MONTH_CHANGE)
    g_object_notify (G_OBJECT (calendar), "no-month-change");
  if ((old_flags ^ calendar->display_flags) & GCL_CALENDAR_SHOW_WEEK_NUMBERS)
    g_object_notify (G_OBJECT (calendar), "show-week-numbers");
  if ((old_flags ^ calendar->display_flags) & GCL_CALENDAR_SHOW_LUNAR)
    g_object_notify (G_OBJECT (calendar), "show-lunar-date");
  g_object_thaw_notify (G_OBJECT (calendar));
}

/**
 * gcl_calendar_select_month:
 * @calendar: a #GclCalendar
 * @month: a month number between 0 and 11.
 * @year: the year the month is in.
 * 
 * Shifts the calendar to a different month.
 * 
 * Return value: %TRUE, always
 **/
gboolean
gcl_calendar_select_month (GclCalendar *calendar,
			   guint	month,
			   guint	year)
{
  g_return_val_if_fail (GCL_IS_CALENDAR (calendar), FALSE);
  g_return_val_if_fail (month <= 11, FALSE);
  
  calendar->month = month;
  calendar->year  = year;
  
  calendar_compute_days (calendar);
  
  gtk_widget_queue_draw (GTK_WIDGET (calendar));

  g_object_freeze_notify (G_OBJECT (calendar));
  g_object_notify (G_OBJECT (calendar), "month");
  g_object_notify (G_OBJECT (calendar), "year");
  g_object_thaw_notify (G_OBJECT (calendar));

  g_signal_emit (calendar,
		 gcl_calendar_signals[MONTH_CHANGED_SIGNAL],
		 0);
  return TRUE;
}

/**
 * gcl_calendar_select_day:
 * @calendar: a #GclCalendar.
 * @day: the day number between 1 and 31, or 0 to unselect 
 *   the currently selected day.
 * 
 * Selects a day from the current month.
 **/
void
gcl_calendar_select_day (GclCalendar *calendar,
			 guint	      day)
{
  g_return_if_fail (GCL_IS_CALENDAR (calendar));
  g_return_if_fail (day <= 31);
  
  /* Deselect the old day */
  if (calendar->selected_day > 0)
    {
      gint selected_day;
      
      selected_day = calendar->selected_day;
      calendar->selected_day = 0;
      if (GTK_WIDGET_DRAWABLE (GTK_WIDGET (calendar)))
	calendar_invalidate_day_num (calendar, selected_day);
    }
  
  calendar->selected_day = day;
  //yetist get the content of the select day
  //char year_tmp[40];
  gint year, month;
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);

  year = calendar->year;
  month = calendar->month + 1;
  if (calendar->display_flags & GCL_CALENDAR_SHOW_LUNAR)                 
  {
      priv->lunar_year=gcl_calendar_lunar_year_str(calendar, year, month, day);
  }
  calendar_invalidate_header(calendar);
  
  /* Select the new day */
  if (day != 0)
    {
      if (GTK_WIDGET_DRAWABLE (GTK_WIDGET (calendar)))
	calendar_invalidate_day_num (calendar, day);
    }
  
  g_object_notify (G_OBJECT (calendar), "day");

  g_signal_emit (calendar,
		 gcl_calendar_signals[DAY_SELECTED_SIGNAL],
		 0);
}

/**
 * gcl_calendar_clear_marks:
 * @calendar: a #GclCalendar
 * 
 * Remove all visual markers.
 **/
void
gcl_calendar_clear_marks (GclCalendar *calendar)
{
  guint day;
  
  g_return_if_fail (GCL_IS_CALENDAR (calendar));
  
  for (day = 0; day < 31; day++)
    {
      calendar->marked_date[day] = FALSE;
    }

  calendar->num_marked_dates = 0;

  gtk_widget_queue_draw (GTK_WIDGET (calendar));
}

/**
 * gcl_calendar_mark_day:
 * @calendar: a #GclCalendar 
 * @day: the day number to mark between 1 and 31.
 * 
 * Places a visual marker on a particular day.
 * 
 * Return value: %TRUE, always
 **/
gboolean
gcl_calendar_mark_day (GclCalendar *calendar,
		       guint	    day)
{
  g_return_val_if_fail (GCL_IS_CALENDAR (calendar), FALSE);
  
  if (day >= 1 && day <= 31 && calendar->marked_date[day-1] == FALSE)
    {
      calendar->marked_date[day - 1] = TRUE;
      calendar->num_marked_dates++;
      calendar_invalidate_day_num (calendar, day);
    }
  
  return TRUE;
}

/**
 * gcl_calendar_unmark_day:
 * @calendar: a #GclCalendar.
 * @day: the day number to unmark between 1 and 31.
 * 
 * Removes the visual marker from a particular day.
 * 
 * Return value: %TRUE, always
 **/
gboolean
gcl_calendar_unmark_day (GclCalendar *calendar,
			 guint	      day)
{
  g_return_val_if_fail (GCL_IS_CALENDAR (calendar), FALSE);
  
  if (day >= 1 && day <= 31 && calendar->marked_date[day-1] == TRUE)
    {
      calendar->marked_date[day - 1] = FALSE;
      calendar->num_marked_dates--;
      calendar_invalidate_day_num (calendar, day);
    }
  
  return TRUE;
}

/**
 * gcl_calendar_get_date:
 * @calendar: a #GclCalendar
 * @year: location to store the year number, or %NULL
 * @month: location to store the month number (between 0 and 11), or %NULL
 * @day: location to store the day number (between 1 and 31), or %NULL
 * 
 * Obtains the selected date from a #GclCalendar.
 **/
void
gcl_calendar_get_date (GclCalendar *calendar,
		       guint	   *year,
		       guint	   *month,
		       guint	   *day)
{
  g_return_if_fail (GCL_IS_CALENDAR (calendar));
  
  if (year)
    *year = calendar->year;
  
  if (month)
    *month = calendar->month;
  
  if (day)
    *day = calendar->selected_day;
}

/**
 * gcl_calendar_freeze:
 * @calendar: a #GclCalendar
 * 
 * Does nothing. Previously locked the display of the calendar until
 * it was thawed with gcl_calendar_thaw().
 *
 * Deprecated: 2.8: 
 **/
void
gcl_calendar_freeze (GclCalendar *calendar)
{
  g_return_if_fail (GCL_IS_CALENDAR (calendar));
}

/**
 * gcl_calendar_thaw:
 * @calendar: a #GclCalendar
 * 
 * Does nothing. Previously defrosted a calendar; all the changes made
 * since the last gcl_calendar_freeze() were displayed.
 *
 * Deprecated: 2.8: 
 **/
void
gcl_calendar_thaw (GclCalendar *calendar)
{
  g_return_if_fail (GCL_IS_CALENDAR (calendar));
}

gchar* gcl_calendar_lunar_day_str (GclCalendar *calendar, gint year, gint month, gint day)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);

  gcl_date_set_solar_date(priv->date, year, month, day, 0, NULL);

  char* buf;

  if (strlen(buf = gcl_date_strftime(priv->date, "%(jieri)")) > 0)
  {
      return buf;
  }
  if (strcmp(gcl_date_strftime(priv->date, "%(ri)"), "1") == 0)
      return gcl_date_strftime(priv->date, "%(YUE)");
  else
      return gcl_date_strftime(priv->date, "%(RI)");
}

gchar* gcl_calendar_lunar_year_str (GclCalendar *calendar, gint year, gint month, gint day)
{
  GtkWidget *widget = GTK_WIDGET (calendar);
  GclCalendarPrivate *priv = GCL_CALENDAR_GET_PRIVATE (calendar);

  gcl_date_set_solar_date(priv->date, year, month, day, 0, NULL);

  return gcl_date_strftime(priv->date, _("Nian %(shengxiao) %(Y60)%(M60)%(D60)"));
}

#define __GCL_CALENDAR_C__

/*
vi:ts=4:nowrap:ai:expandtab
*/
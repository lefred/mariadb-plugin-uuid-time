/* Copyright (c) 2019,2024,2025,2036 MariaDB Corporation
   Copyright (c) 2026 lefred (Frédéric Descamps)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1335  USA */

#define MYSQL_SERVER
#include "mariadb.h"
#include "item_uuidtime.h"

#include "m_ctype.h"

#include <cstdio>
#include <ctime>
#include <my_sys.h>
#include <my_time.h>
#include <mysqld_error.h>
#include <string>

#include "common.h"
#include "uuid_utils.h"

String *Item_func_uuid_to_timestamp::val_str(String *str)
{
  String in_tmp;
  String *uuid_arg= args[0]->val_str(&in_tmp);
  if (!uuid_arg)
  {
    null_value= true;
    return nullptr;
  }

  std::string in_str(uuid_arg->ptr(), uuid_arg->length());
  std::string out_str= uuid_to_ts(in_str);

  if (out_str.empty())
  {
    null_value= true;
    return nullptr;
  }

  // copy() allocates, copies, sets length, and NUL-terminates
  if (str->copy(out_str.c_str(), static_cast<uint>(out_str.size()),
                collation.collation))
  {
    null_value= true;
    return nullptr;
  }

  null_value= false;
  return str;
}

String *Item_func_uuid_to_timestamp_long::val_str(String *str)
{
  // Implement the conversion from UUID to DETAILED TIMESTAMP
  String in_tmp;
  String *uuid_arg= args[0]->val_str(&in_tmp);
  if (!uuid_arg)
  {
    null_value= true;
    return nullptr;
  }

  std::string in_str(uuid_arg->ptr(), uuid_arg->length());
  std::string out_str= uuid_to_ts(in_str, TS_LONG);

  if (out_str.empty())
  {
    null_value= true;
    return nullptr;
  }

  if (str->copy(out_str.c_str(), static_cast<uint>(out_str.size()),
                collation.collation))
  {
    null_value= true;
    return nullptr;
  }

  null_value= false;
  return str;
}

longlong Item_func_uuid_to_unixtime::val_int()
{
  // Implement the conversion from UUID to UNIXTIME
  String in_tmp;
  String *uuid_arg= args[0]->val_str(&in_tmp);
  if (!uuid_arg)
  {
    null_value= true;
    return 0;
  }

  std::string in_str(uuid_arg->ptr(), uuid_arg->length());
  uint64_t out= 0;
  if (!uuid_to_unixtime(in_str, &out))
  {
    null_value= true;
    return 0;
  }

  null_value= false;
  return out;
}

static bool uuid_age_bounds_ms(Item **args, const char *func_name,
                               uint64_t *timestamp_ms, uint64_t *now_ms)
{
  String in_tmp;
  String *uuid_arg= args[0]->val_str(&in_tmp);
  if (!uuid_arg)
  {
    return false;
  }

  std::string in_str(uuid_arg->ptr(), uuid_arg->length());
  if (!uuid_to_unixms(in_str, timestamp_ms, func_name))
  {
    return false;
  }

  *now_ms= my_hrtime().val / 1000;
  return true;
}

double Item_func_uuid_age::val_real()
{
  uint64_t timestamp_ms= 0;
  uint64_t now_ms= 0;
  if (!uuid_age_bounds_ms(args, "uuid_age", &timestamp_ms, &now_ms))
  {
    null_value= true;
    return 0.0;
  }

  null_value= false;
  return (static_cast<double>(now_ms) - static_cast<double>(timestamp_ms)) /
         1000.0;
}

struct Utc_point
{
  uint year;
  uint month;
  uint day;
  uint hour;
  uint minute;
  uint second;
  uint millisecond;
};

static Utc_point utc_point_from_ms(uint64_t unix_ms)
{
  time_t seconds= static_cast<time_t>(unix_ms / 1000);
  struct tm tm_value;
#ifdef _WIN32
  gmtime_s(&tm_value, &seconds);
#else
  gmtime_r(&seconds, &tm_value);
#endif

  Utc_point point;
  point.year= static_cast<uint>(tm_value.tm_year + 1900);
  point.month= static_cast<uint>(tm_value.tm_mon + 1);
  point.day= static_cast<uint>(tm_value.tm_mday);
  point.hour= static_cast<uint>(tm_value.tm_hour);
  point.minute= static_cast<uint>(tm_value.tm_min);
  point.second= static_cast<uint>(tm_value.tm_sec);
  point.millisecond= static_cast<uint>(unix_ms % 1000);
  return point;
}

static uint64_t utc_point_to_ms(const Utc_point &point)
{
  static const long epoch_daynr= calc_daynr(1970, 1, 1);
  long daynr= calc_daynr(point.year, point.month, point.day);
  uint64_t seconds= static_cast<uint64_t>(daynr - epoch_daynr) * 86400ULL +
                    point.hour * 3600ULL + point.minute * 60ULL +
                    point.second;
  return seconds * 1000ULL + point.millisecond;
}

static Utc_point add_months_clamped(const Utc_point &point, longlong months)
{
  longlong month_index= static_cast<longlong>(point.year) * 12 +
                        static_cast<longlong>(point.month) - 1 + months;
  Utc_point result= point;
  result.year= static_cast<uint>(month_index / 12);
  result.month= static_cast<uint>(month_index % 12) + 1;

  uint last_day= calc_days_in_month(result.year, result.month);
  if (result.day > last_day)
    result.day= last_day;

  return result;
}

static std::string format_uuid_age_long(uint64_t timestamp_ms, uint64_t now_ms)
{
  static const uint64_t milliseconds_per_second= 1000;
  static const uint64_t milliseconds_per_minute= milliseconds_per_second * 60;
  static const uint64_t milliseconds_per_hour= milliseconds_per_minute * 60;
  static const uint64_t milliseconds_per_day= milliseconds_per_hour * 24;

  bool negative= timestamp_ms > now_ms;
  uint64_t start_ms= negative ? now_ms : timestamp_ms;
  uint64_t end_ms= negative ? timestamp_ms : now_ms;

  Utc_point start= utc_point_from_ms(start_ms);
  Utc_point end= utc_point_from_ms(end_ms);

  longlong years= static_cast<longlong>(end.year) -
                  static_cast<longlong>(start.year);
  while (years > 0 &&
         utc_point_to_ms(add_months_clamped(start, years * 12)) > end_ms)
  {
    years--;
  }

  Utc_point cursor= add_months_clamped(start, years * 12);
  longlong months= (static_cast<longlong>(end.year) -
                    static_cast<longlong>(cursor.year)) * 12 +
                   static_cast<longlong>(end.month) -
                   static_cast<longlong>(cursor.month);
  while (months > 0 &&
         utc_point_to_ms(add_months_clamped(cursor, months)) > end_ms)
  {
    months--;
  }

  cursor= add_months_clamped(cursor, months);
  uint64_t cursor_ms= utc_point_to_ms(cursor);
  uint64_t remainder= end_ms - cursor_ms;

  uint64_t days= remainder / milliseconds_per_day;
  remainder%= milliseconds_per_day;
  uint64_t hours= remainder / milliseconds_per_hour;
  remainder%= milliseconds_per_hour;
  uint64_t minutes= remainder / milliseconds_per_minute;
  remainder%= milliseconds_per_minute;
  uint64_t seconds= remainder / milliseconds_per_second;
  uint64_t milliseconds= remainder % milliseconds_per_second;

  char buffer[96];
  my_snprintf(buffer, sizeof(buffer), "%s%lldy %lldmo %llud %lluh %llum "
                                    "%llu.%03llus",
              negative ? "-" : "", years, months,
              static_cast<ulonglong>(days), static_cast<ulonglong>(hours),
              static_cast<ulonglong>(minutes),
              static_cast<ulonglong>(seconds),
              static_cast<ulonglong>(milliseconds));
  return std::string(buffer);
}

String *Item_func_uuid_age_long::val_str(String *str)
{
  uint64_t timestamp_ms= 0;
  uint64_t now_ms= 0;
  if (!uuid_age_bounds_ms(args, "uuid_age_long", &timestamp_ms, &now_ms))
  {
    null_value= true;
    return nullptr;
  }

  std::string out_str= format_uuid_age_long(timestamp_ms, now_ms);
  if (str->copy(out_str.c_str(), static_cast<uint>(out_str.size()),
                collation.collation))
  {
    null_value= true;
    return nullptr;
  }

  null_value= false;
  return str;
}

# mariadb-plugin-uuid-time

MariaDB function plugin that extracts timestamps from UUID values when the
timestamp is encoded in the UUID. It supports UUID version 1 and UUID version 7.

The plugin provides three SQL functions:

| Function | Description |
| --- | --- |
| `uuid_to_timestamp(uuid)` | Returns the embedded timestamp as `YYYY-MM-DD HH:MM:SS.mmm` |
| `uuid_to_timestamp_long(uuid)` | Returns the embedded timestamp in the server process locale/timezone format |
| `uuid_to_unixtime(uuid)` | Returns the embedded timestamp as Unix time in seconds |

Valid UUIDs that do not contain a timestamp, such as UUIDv4 values, return
`NULL`. Invalid UUID strings raise an error.

## Build

This plugin is intended to be built as part of a MariaDB source tree. Place the
plugin directory under the MariaDB plugin directory and build MariaDB with the
plugin enabled.

The CMake target builds the module as `uuid_time`.

## Installation

Install the plugin module in MariaDB:

```sql
INSTALL SONAME 'uuid_time';
```

Verify that the three function plugins are loaded:

```sql
SELECT plugin_name, plugin_type, plugin_library, plugin_description,
       plugin_author
FROM information_schema.PLUGINS
WHERE plugin_type = 'FUNCTION'
  AND plugin_library = 'uuid_time.so'
ORDER BY plugin_name;
```

Expected functions:

```text
+------------------------+-------------+----------------+-----------------------------------+---------------+
| plugin_name            | plugin_type | plugin_library | plugin_description                | plugin_author |
+------------------------+-------------+----------------+-----------------------------------+---------------+
| uuid_to_timestamp      | FUNCTION    | uuid_time.so   | Function UUID_TO_TIMESTAMP()      | lefred        |
| uuid_to_timestamp_long | FUNCTION    | uuid_time.so   | Function UUID_TO_TIMESTAMP_LONG() | lefred        |
| uuid_to_unixtime       | FUNCTION    | uuid_time.so   | Function UUID_TO_UNIXTIME()       | lefred        |
+------------------------+-------------+----------------+-----------------------------------+---------------+
```

Uninstall the plugin module:

```sql
UNINSTALL SONAME 'uuid_time';
```

## Examples

The timestamp string functions use the server process timezone. Exact displayed
hours can differ by environment, but the Unix timestamp represents the same
instant.

### `uuid_to_timestamp()`

Convert a UUIDv1 value:

```sql
SELECT uuid_to_timestamp('a00c9994-6a13-11f1-b6b2-5e1b9081e705');
```

Example result:

```text
+----------------------------------------------------------------+
| uuid_to_timestamp('a00c9994-6a13-11f1-b6b2-5e1b9081e705')      |
+----------------------------------------------------------------+
|  2026-06-17 08:13:14.729                                       |
+----------------------------------------------------------------+
```

Convert a UUIDv7 value:

```sql
SELECT uuid_to_timestamp('019ed437-74b1-7f54-814d-b96799236b76');
```

Example result:

```text
+----------------------------------------------------------------+
| uuid_to_timestamp('019ed437-74b1-7f54-814d-b96799236b76')      |
+----------------------------------------------------------------+
| 2026-06-17 08:14:24.689                                        |
+----------------------------------------------------------------+
```

Use generated UUIDs:

```sql
SELECT uuid_to_timestamp(UUID()) AS uuid_v1_timestamp,
       uuid_to_timestamp(UUID_v7()) AS uuid_v7_timestamp;
```

### `uuid_to_timestamp_long()`

Return a longer, locale-dependent timestamp:

```sql
SELECT uuid_to_timestamp_long('a00c9994-6a13-11f1-b6b2-5e1b9081e705');
```

Example result:

```text
+----------------------------------------------------------------+
| uuid_to_timestamp_long('a00c9994-6a13-11f1-b6b2-5e1b9081e705') |
+----------------------------------------------------------------+
| Wed Jun 17 08:13:14 2026 CEST                                  |
+----------------------------------------------------------------+
```

For UUIDv7:

```sql
SELECT uuid_to_timestamp_long('019ed437-74b1-7f54-814d-b96799236b76');
```

Example result:

```text
+----------------------------------------------------------------+
| uuid_to_timestamp_long('019ed437-74b1-7f54-814d-b96799236b76') |
+----------------------------------------------------------------+
| Wed Jun 17 08:14:24 2026 CEST                                  |
+----------------------------------------------------------------+
```

### `uuid_to_unixtime()`

Return Unix time in seconds:

```sql
SELECT uuid_to_unixtime('896e34e0-2d0c-11ea-8000-000000000000') AS v1,
       uuid_to_unixtime('018bcfe5-687b-7000-8000-000000000000') AS v7;
```

Result:

```text
+------------+------------+
| v1         | v7         |
+------------+------------+
| 1577934245 | 1700000000 |
+------------+------------+
```

Combine with `FROM_UNIXTIME()`:

```sql
SELECT FROM_UNIXTIME(
         uuid_to_unixtime('018bcfe5-687b-7000-8000-000000000000')
       ) AS uuid_v7_datetime;
```

## NULL and Error Behavior

`NULL` input returns `NULL`:

```sql
SELECT uuid_to_timestamp(NULL),
       uuid_to_timestamp_long(NULL),
       uuid_to_unixtime(NULL);
```

Valid UUIDs without an embedded timestamp return `NULL`:

```sql
SELECT uuid_to_timestamp('f47ac10b-58cc-4372-a567-0e02b2c3d479') AS ts,
       uuid_to_timestamp_long('f47ac10b-58cc-4372-a567-0e02b2c3d479') AS ts_long,
       uuid_to_unixtime('f47ac10b-58cc-4372-a567-0e02b2c3d479') AS unix_time;
```

Result:

```text
+------+---------+-----------+
| ts   | ts_long | unix_time |
+------+---------+-----------+
| NULL | NULL    | NULL      |
+------+---------+-----------+
```

Invalid UUID text raises an error:

```sql
SELECT uuid_to_timestamp('not-a-uuid');
```

Result:

```text
ERROR 1105 (HY000): uuid_to_timestamp: not a valid UUID
```

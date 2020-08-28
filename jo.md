---
title: JO(1) User Manuals
---

NAME
====

jo - JSON output from a shell

SYNOPSIS
========

jo \[-p\] \[-a\] \[-B\] \[-e\] \[-n\] \[-v\] \[-V\] \[-d keydelim\]
\[--\] \[ \[-s\|-n\|-b\] word ...\]

DESCRIPTION
===========

*jo* creates a JSON string on *stdout* from *word*s given it as
arguments or read from *stdin*. Without option `-a` it generates an
object whereby each *word* is a `key=value` (or `key@value`) pair with
*key* being the JSON object element and *value* its value. *jo* attempts
to guess the type of *value* in order to create number (using
*strtod(3)*), string, or null values in JSON.

A missing or empty *value* normally results in an element whose value is
`null`. If `-n` is specified, this element is not created.

*jo* normally treats *key* as a literal string value. If the `-d` option
is specified, *key* will be interpreted as an *object path*, whose
individual components are separated by the first character of
*keydelim*.

*jo* normally treats *value* as a literal string value, unless it begins
with one of the following characters:

  value   action
  ------- ---------------------------------------------------------------------
  @file   substitute the contents of *file* as-is
  %file   substitute the contents of *file* in base64-encoded form
  :file   interpret the contents of *file* as JSON, and substitute the result

Escape the special character with a backslash to prevent this
interpretation.

*jo* treats `key@value` specifically as boolean JSON elements: if the
value begins with `T`, `t`, or the numeric value is greater than zero,
the result is `true`, else `false`.

*jo* creates an array instead of an object when `-a` is specified.

When the `:=` operator is used in a *word*, the name to the right of
`:=` is a file containing JSON which is parsed and assigned to the key
left of the operator. The file may be specified as `-` to read from
*jo*'s standard input.

TYPE COERCION
=============

*jo*'s type guesses can be overridden on a per-word basis by prefixing
*word* with `-s` for *string*, `-n` for *number*, or `-b` for *boolean*.
The list of *word*s *must* be prefixed with `--`, to indicate to *jo*
that there are no more global options.

Type coercion works as follows:

  word           -s                 -n          -b          default
  -------------- ------------------ ----------- ----------- ------------------
  a=             "a":\"\"           "a":0       "a":false   "a":null
  a=string       "a":"string"       "a":6       "a":true    "a":"string"
  a=\"quoted\"   "a":"\"quoted\""   "a":8       "a":true    "a":"\"quoted\""
  a=12345        "a":"12345"        "a":12345   "a":true    "a":12345
  a=true         "a":"true"         "a":1       "a":true    "a":true
  a=false        "a":"false"        "a":0       "a":false   "a":false
  a=null         "a":\"\"           "a":0       "a":false   "a":null

Coercing a non-number string to number outputs the *length* of the
string.

Coercing a non-boolean string to boolean outputs `false` if the string
is empty, `true` otherwise.

Type coercion only applies to `key=value` words, and individual words in
a `-a` array. Coercing other words has no effect.

EXAMPLES
========

Create an object. Note how the incorrectly-formatted float value becomes
a string:

    $ jo tst=1457081292 lat=12.3456 cc=FR badfloat=3.14159.26 name="JP Mens" nada= coffee@T
    {"tst":1457081292,"lat":12.3456,"cc":"FR","badfloat":"3.14159.26","name":"JP Mens","nada":null,"coffee":true}

Pretty-print an array with a list of files in the current directory:

    $ jo -p -a *
    [
     "Makefile",
     "README.md",
     "jo.1",
     "jo.c",
     "jo.pandoc",
     "json.c",
     "json.h"
    ]

Create objects within objects; this works because if the first character
of value is an open brace or a bracket we attempt to decode the
remainder as JSON. Beware spaces in strings ...

    $ jo -p name=JP object=$(jo fruit=Orange hungry@0 point=$(jo x=10 y=20 list=$(jo -a 1 2 3 4 5)) number=17) sunday@0
    {
     "name": "JP",
     "object": {
      "fruit": "Orange",
      "hungry": false,
      "point": {
       "x": 10,
       "y": 20,
       "list": [
        1,
        2,
        3,
        4,
        5
       ]
      },
      "number": 17
     },
     "sunday": false
    }

Booleans as strings or as boolean (pay particular attention to *switch*;
the `-B` option disables the default detection of the "`true`",
"`false`", and "`null`" strings):

    $ jo switch=true morning@0
    {"switch":true,"morning":false}

    $ jo -B switch=true morning@0
    {"switch":"true","morning":false}

Elements (objects and arrays) can be nested. The following example nests
an array called *point* and an object named *geo*:

    $ jo -p name=Jane point[]=1 point[]=2 geo[lat]=10 geo[lon]=20
    {
       "name": "Jane",
       "point": [
          1,
          2
       ],
       "geo": {
          "lat": 10,
          "lon": 20
       }
    }

The same example, using object paths:

    $ jo -p -d. name=Jane point[]=1 point[]=2 geo.lat=10 geo.lon=20
    {
       "name": "Jane",
       "point": [
          1,
          2
       ],
       "geo": {
          "lat": 10,
          "lon": 20
       }
    }

Without `-d`, a different object is generated:

    $ jo -p name=Jane point[]=1 point[]=2 geo.lat=10 geo.lon=20
    {
       "name": "Jane",
       "point": [
          1,
          2
       ],
       "geo.lat": 10,
       "geo.lon": 20
    }

Create empty objects or arrays, intentionally or potentially:

    $ jo < /dev/null
    {}

    $ MY_ARRAY=(a=1 b=2)
    $ jo -a "${MY_ARRAY[@]}" < /dev/null
    ["a=1","b=2"]

Type coercion:

    $ jo -p -- -s a=true b=true -s c=123 d=123 -b e="1" -b f="true" -n g="This is a test" -b h="This is a test"
    {
       "a": "true",
       "b": true,
       "c": "123",
       "d": 123,
       "e": true,
       "f": true,
       "g": 14,
       "h": true
    }

    $ jo -a -- -s 123 -n "This is a test" -b C_Rocks 456
    ["123",14,true,456]

Read element values from files: a value which starts with `@` is read in
plain whereas if it begins with a `%` it will be base64-encoded and if
it starts with `:` the contents are interpreted as JSON:

    $ jo program=jo authors=@AUTHORS
    {"program":"jo","authors":"Jan-Piet Mens <jpmens@gmail.com>"}

    $ jo filename=AUTHORS content=%AUTHORS
    {"filename":"AUTHORS","content":"SmFuLVBpZXQgTWVucyA8anBtZW5zQGdtYWlsLmNvbT4K"}

    $ jo nested:=nested.json
    {"nested":{"field1":123,"field2":"abc"}}

These characters can be escaped to avoid interpretation:

    $ jo name="JP Mens" twitter='\@jpmens'
    {"name":"JP Mens","twitter":"@jpmens"}

    $ jo char=" " URIescape=\\%20
    {"char":" ","URIescape":"%20"}

    $ jo action="split window" vimcmd="\:split"
    {"action":"split window","vimcmd":":split"}

Read element values from a file in order to overcome ARG_MAX limits
during object assignment:

    $ ls | jo -a > child.json
    $ jo files:=child.json
    {"files":["AUTHORS","COPYING","ChangeLog" ....

    $ ls *.c | jo -a > source.json; ls *.h | jo -a > headers.json
    $ jo -a :source.json :headers.json
    [["base64.c","jo.c","json.c"],["base64.h","json.h"]]

OPTIONS
=======

*jo* understands the following global options.

-a
:   Interpret the list of *words* as array values and produce an array
    instead of an object.

-B
:   By default *jo* interprets the strings "`true`" and "`false`" as
    boolean elements `true` and `false` respectively, and "`null`" as
    `null`. Disable with this option.

-e
:   Ignore empty stdin (i.e. don't produce a diagnostic error when
    *stdin* is empty)

-n
:   Do not add keys with empty values.

-p
:   Pretty-print the JSON string on output instead of the terse one-line
    output it prints by default.

-v
:   Show version and exit.

-V
:   Show version as a JSON object and exit.

BUGS
====

Probably.

If a value given to *jo* expands to empty in the shell, then *jo*
produces a `null` in object mode, and might appear to hang in array
mode; it is not hanging, rather it's reading *stdin*. This is not a bug.

Numeric values are converted to numbers which can produce undesired
results. If you quote a numeric value, *jo* will make it a string.
Compare the following:

    $ jo a=1.0
    {"a":1}
    $ jo a=\"1.0\"
    {"a":"1.0"}

Omitting a closing bracket on a nested element causes a diagnostic
message to print, but the output contains garbage anyway. This was
designed thusly.

RETURN CODES
============

*jo* exits with a code 0 on success and non-zero on failure after
indicating what caused the failure.

AVAILABILITY
============

<http://github.com/jpmens/jo>

CREDITS
=======

-   This program uses `json.[ch]`, by Joseph A. Adams.

SEE ALSO
========

-   <https://stedolan.github.io/jq/>
-   <https://github.com/micha/jsawk>
-   <https://github.com/jtopjian/jsed>
-   strtod(3)

AUTHOR
======

Jan-Piet Mens <http://jpmens.net>

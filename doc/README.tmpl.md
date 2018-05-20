## Template system

This is meant to be compatible with perls HTML::Template module.

The following files can be used to use this implementation in other
programs:

* `buffer.c`
* `buffer.h`
* `template.c`
* `template.h`

## Variable Setup

See the header file `template.h` for functions aiding in setting up
a `struct tmpl_data` structure to be used as an argument to the
`tmpl_parse()`  and `tmpl_parse_file()` functions.

## Tag Description

### Variables

Usage: `<TMPL_VAR name="identifier">`

Replaced by the variable value from the current `struct tmpl_data` context.

### Loops

Usage: `<TMPL_LOOP name="loopname">...</TMPL_LOOP>`

Implementation: The filter parser stores the position of the loop begin/end
and calls a new instance of itself for each `struct tmpl_data` in the
`struct tmpl_loop`. If no such data exists or the loop name was not found
the parser continues after the `</TMPL_LOOP>` tag.

It is possible to build complex data structures with nested loops.

### Includes

Usage: `<TMPL_INCL name="filename">` or `<TMPL_INCLUDE name="filename">`

Includes the "filename" as if it was part of the template to the output.

Implementation: A new parser instance is generated with the files content
and the current `struct tmpl_data` object is passed as the data structure.

### Conditions

#### TMPL_IF

Usage: `<TMPL_IF name="identifier">...</TMPL_IF>`

If the "identifier" references a loop the condition evaluates to `true` if
the loop is not empty. If a variable is referenced the variable will evaluate
to `false` if one of the following conditions apply:

- Variable does not exist

- Variable is empty

- Variable contains the string '0' (zero)

Implementation: The filter parser takes the `</TMPL_IF>` tag and
passes the range between those tags to a new instance of the parser with
the current `struct tmpl_data`. In case of an `<TMPL_ELSE>` belonging to
the conditional block either the part before the `<TMPL_ELSE>` or after the
`<TMPL_ELSE>` is parsed by passing the current `struct tmpl_data` object to
a new parser instance.

#### TMPL_UNLESS

Usage: `<TMPL_UNLESS name="identifier">...</TMPL_UNLESS>`

Same as `TMPL_IF` but logically negated.

#### TMPL_ELSE

Usage: `<TMPL_IF name="identifier">...<TMPL_ELSE>...</TMPL_IF>`

Works the same way with `TMPL_UNLESS`.

## Notes

Unless the perl HTML::Template module the name attribute is required exacactly
in the form given above to simplify parsing.

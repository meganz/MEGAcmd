### attr
Lists/updates node attributes.

Usage: `attr remotepath [--force-non-officialficial] [-s attribute value|-d attribute [--print-only-value]`

<pre>
Options:
 -s     attribute value         Sets an attribute to a value
 -d     attribute               Removes the attribute
 --print-only-value     When listing attributes, print only the values, not the attribute names.
 --force-non-official   Forces using the custom attribute version for officially recognized attributes.
                        Note that custom attributes are internally stored with a `_` prefix.
                        Use this option to show, modify or delete a custom attribute with the same name as one official.
</pre>


### codepage
Switches the codepage used to decide which characters show on-screen.

Usage: `codepage [N [M]]`
<pre>
MEGAcmd supports unicode or specific code pages.  For european countries you may need
to select a suitable codepage or secondary codepage for the character set you use.
Of course a font containing the glyphs you need must have been selected for the terminal first.
Options:
 (no option)	Outputs the selected code page and secondary codepage (if configured).
 N	Sets the main codepage to N. 65001 is Unicode.
 M	Sets the secondary codepage to M, which is used if the primary can't translate a character.

Note: this command is only available on some versions of Windows
</pre>

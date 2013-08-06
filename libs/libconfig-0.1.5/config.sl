% -*- slang -*-

%  This file provides a mode for editing my configuration files.
%
%  Written by Abraham vd Merwe <abz@blio.com>
%  Last updated: 6 July 2002

%
% TODO:
%
%   - fix config_newline_indent_hook() and re-enable newline_indent hook again
%

variable config = "config";

static define looking_at_whitespace ()
{
   return (looking_at (" ") or looking_at ("\t") or looking_at ("\n"));
}

static define pad_to_column (n)
{
   variable i;
   bol ();
   for (i = 0; i <= n; i++) insert_char (' ');
}

define config_newline_indent_hook ()
{
   push_spot ();
   !if (looking_at_whitespace ()) go_left_1 ();
   while (looking_at_whitespace () and not bobp ()) go_left_1 ();
   if (what_char () == '{')
	 {
		% find the position of the start of the keyword
		go_left_1 ();
		while ((looking_at ("=") or looking_at_whitespace ()) and not bobp ()) go_left_1 ();
		while (re_looking_at ("[0-9a-zA-Z_]") and not bobp ()) go_left_1 ();
		go_right_1 ();
		variable col = what_column ();

		% go back to where we came from and remove all the whitespace after and before the '{' and also the '{' itself
		pop_spot ();
		push_mark ();
		!if (looking_at_whitespace ()) go_left_1 ();
		while (looking_at_whitespace () and not bobp ()) go_left_1 ();
		del_region ();
		if (looking_at_whitespace ()) del ();

		% press ENTER and insert the '{'
		newline ();
		pad_to_column (col);
		insert_char ('{');

		% press ENTER and indent
		newline ();
		pad_to_column (col + 3);
	 }
   else
	 {
		pop_spot ();
		newline ();
	 }
}

!if (keymap_p (config)) make_keymap (config);
definekey ("config_insert_newline","\n",config);

% Now create and initialize a syntax table.
create_syntax_table (config);

define_syntax ("#","",'%',config);				% comments
define_syntax ("{", "}",'(',config);			% brackets
define_syntax ('"','"',config);					% strings
define_syntax ('\\','\\',config);				% escape character
define_syntax ("0-9a-zA-Z_",'w',config);		% identifiers
define_syntax ("0-9a-fA-FxX",'0',config);		% numbers
define_syntax (",",',',config);					% delimiters
define_syntax ("-=",'+',config);	% operators

set_syntax_flags (config,0x04);

% reserved words
() = define_keywords_n (config,"true",4,0);
() = define_keywords_n (config,"false",5,0);

define config_mode ()
{
   set_mode (config,4);
   use_keymap (config);
   use_syntax_table (config);
%   set_buffer_hook ("indent_hook","config_indent_hook");
%   set_buffer_hook ("newline_indent_hook","config_newline_indent_hook");
}


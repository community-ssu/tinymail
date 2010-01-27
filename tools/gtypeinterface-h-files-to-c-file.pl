#! /usr/bin/perl -w

#
# Usage: ./gtypeinterface-h-files-to-c-file.pl MyTypeName hfiles
# For example: 
# ./gtypeinterface-h-files-to-c-file.pl MyMimePartComponent ../libtinymailui/tny-mime-part-view.h ../libtinymailui/tny-mime-part-saver.h
#

use strict;

my $typename = shift @ARGV;

die "usage: $0 TypeName iface1.h iface2.h ... > newiface.c\n"
	unless defined $typename;

my @interfaces = ();

sub max     { $_[0] > $_[1] ? $_[0] : $_[1]; }

sub uncamel_low
{
	my $line = shift;

	my $out = lcfirst $line;
	$out =~ s/([a-z0-9_])([A-Z])/$1."_".lcfirst($2)/ge;

	return $out;
}

sub uncamel_file
{
	my $line = shift;

	my $out = lcfirst $line;
	$out =~ s/([a-z0-9_])([A-Z])/$1."-".lcfirst($2)/ge;

	return $out;
}

sub uncamel_up
{
	my $line = shift;

	my $out = $line;
	$out =~ s/([a-z0-9_])([A-Z])/$1\_$2/g;

	return uc $out;
}

sub unpointer
{
	my $line = shift;

	$line =~ s/\s*\*//g;

	return $line;
}


#
# Slurp everything in as one big line.  Since C code can have arbitrary
# formatting, we can't just parse by lines.
#
$/ = undef;
my $intext = join "", <>;

#
# strip comments.  note that this is straight from the perlop manpage, and
# doesn't deal with all the nasty special cases you can get in C comments...
#
$intext =~ s{/\*    # open
             .*?    # minimal internal match
             \*/    # close
             }{}gsx;

#
# Now look for all the structure definitions...
#
while ($intext =~ m/\bstruct            # struct keword.
                    \s+                 # separator.
                    (\w+)               # struct tag name.
                    \s*                 # maybe whitespace.
                    {                   # open structure definition block.
                        ([^}]*)         # body; NOTE, we're not allowing
                                        #       nested blocks with this.
                    }                   # close structure definition block.
                    \s*                 # maybe whitespace.
                   /xsg)
{
        my $iface = $1;
        my $body = $2;

        # Remove any decorators from the structure name:
        $iface =~ s/^_//;       # leading underscore
        $iface =~ s/Iface$//;   # trailing "Iface"

        my @methods;

        # Handle the body one statement at a time.
        foreach my $statement (split /;/, $body) {
                $statement =~ y{\n}{ };
                $statement =~ s/^\s*//;
                $statement =~ s/\s*$//;

                if ($statement =~ /^
                     (.*)               # any leading stuff, return spec
                     \(                 # open paren for funcptr member
                       \s*
                       \*               # an asterisk
                       \s*
                       (\w+)            # the symbol name
                       \s*
                     \)                 # close paren for member name
                     \s*
                     \(                 # open paren for parameter list
                        (.*)            # parameter specs
                     \)                 # close paren for parameter list
                    $/xs) {
                        my $retspec = $1;
                        my $func_name = $2;
                        my $params = $3;

                        # Another version without any _func suffixes, as they
                        # tend to be a bit redundant.  ;-)
                        (my $clean_sym = $func_name) =~ s/_func$//;

                        # Clean up the return type specifier.
                        $retspec =~ s/\s*$//;

                        # And the parameter spec.
                        $params =~ s/^\s*//;
                        $params =~ s/\s*$//;

                        push @methods, {
                                func_full_name => $func_name,
                                func_name      => $clean_sym,
                                return_type    => $retspec,
                                params         => $params,
                        };
                }
        }

        push @interfaces, {
                name => $iface,
                methods => \@methods,
        };
}

warn "Found ".scalar(@interfaces)." interfaces\n";
use Data::Dumper;
warn Dumper(\@interfaces);



print ("/* Your copyright here */\n\n");
print ("#include <config.h>\n");
print ("#include <glib.h>\n");
print ("#include <glib/gi18n-lib.h>\n\n");
print ("#include <".uncamel_file ($typename).".h>\n");
print ("#include \"".uncamel_file ($typename)."-priv.h\"\n\n");
print ("static GObjectClass *parent_class = NULL;\n\n");

foreach my $iface (@interfaces)
{
    foreach my $method (@{ $iface->{methods} })
    {
	print "static ".$method->{return_type}."\n";
	print uncamel_low($typename)."_".$method->{func_name}." ";
	print "(".$method->{params}.")";
	print "\n{\n";
	print "\treturn ".uncamel_low(unpointer($method->{return_type}))."_new ()\n"
	        if $method->{return_type} ne "void";
	print "}\n\n";
    }
}

print "static void\n";
print uncamel_low($typename)."_finalize (GObject *object)\n";
print "{\n\tparent_class->finalize (object);\n}\n";

print "static void\n";
print uncamel_low($typename)."_instance_init (GTypeInstance *instance, gpointer g_class)\n";
print "{\n}\n";

foreach my $iface (@interfaces)
{
	print ("\nstatic void\n");
	print (uncamel_low ($iface->{name})."_init (".$iface->{name}."Iface *klass)\n{\n");


	foreach my $method (@{ $iface->{methods} })
  	{
		print ("\tklass->".$method->{func_full_name}." = ");
		print (uncamel_low($typename)."_");
		print ($method->{func_name}.";\n");
	}

	print ("}\n\n");

}

print ("static void\n");
print (uncamel_low ($typename)."_class_init (".$typename."Class *klass)\n{\n");
print ("\tGObjectClass *object_class;\n\n");
print ("\tparent_class = g_type_class_peek_parent (klass);\n");
print ("\tobject_class = (GObjectClass*) klass;\n");
print ("\tobject_class->finalize = ".uncamel_low ($typename)."_finalize;\n");
print ("}\n");

print ("GType\n");
print (uncamel_low ($typename)."_get_type (void)\n{\n");
print ("\tstatic GType type = 0;\n");
print ("\tif (G_UNLIKELY(type == 0))\n\t{\n");
print ("\t\tstatic const GTypeInfo info = \n\t\t{\n");
print ("\t\t\tsizeof (".$typename."Class),\n");
print ("\t\t\tNULL,   /* base_init */\n");
print ("\t\t\tNULL,   /* base_finalize */\n");
print ("\t\t\t(GClassInitFunc) ".uncamel_low ($typename)."_class_init,   /* class_init */\n");
print ("\t\t\tNULL,   /* class_finalize */\n");
print ("\t\t\tNULL,   /* class_data */\n");
print ("\t\t\tsizeof (".$typename."),\n");
print ("\t\t\t0,      /* n_preallocs */\n");
print ("\t\t\t".uncamel_low ($typename)."_instance_init,    /* instance_init */\n");
print ("\t\t\tNULL\n\t\t};\n\n\n");

foreach my $iface (@interfaces)
{
        my $iface_low = uncamel_low ($iface->{name});
	print ("\t\tstatic const GInterfaceInfo ".$iface_low."_info = \n\t\t{\n");
	print ("\t\t\t(GInterfaceInitFunc) ".$iface_low."_init, /* interface_init */\n");
	print ("\t\t\tNULL,         /* interface_finalize */\n");
	print ("\t\t\tNULL          /* interface_data */\n\t\t};\n\n");
}


print "\t\ttype = g_type_register_static (G_TYPE_OBJECT,\n";
print "\t\t\t\"".$typename."\",\n";
print "\t\t\t&info, 0);\n\n";

foreach my $iface (@interfaces)
{
        my $iface_up = uncamel_up ($iface->{name});
        my $iface_low = uncamel_low ($iface->{name});
	print "\t\tFIX THIS (ADD _TYPE):\n\t\tg_type_add_interface_static (type, $iface_up,\n";
	print "\t\t\t&".$iface_low."_info);\n\n";
}
print "\t}\n\treturn type;\n}\n";


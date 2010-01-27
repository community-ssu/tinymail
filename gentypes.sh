
srcdir=$1
echo "#include <glib.h>" > docs/devel/reference/libtinymail.types

find $srcdir -maxdepth 2 -type f -iname '*.h' -printf '#include <%f>\n' | grep -v "\-priv" | grep -v "\-test" | grep -v platform_include.h | grep -v camel.h | grep -v config.h | grep -v tny-demoui-summary-view.h | grep -v olpc | grep -v gnome | grep -v maemo | grep -v gpe | grep -v acap | grep -v tny-gtk-enums.h | grep -v tny-enums.h | grep -v check | grep -v webkit | grep -v html | grep -v async-worker | grep -v mail-noti >> docs/devel/reference/libtinymail.types

echo >> docs/devel/reference/libtinymail.types

find $srcdir -maxdepth 2 -type f -iname "*.h" -exec grep get_type {} \; | grep -v _tny_simple_list_iterator | grep -v tny_demoui_summary_view | grep -v define | grep -v tny_maemo | grep -v tny_gpe | grep -v tny_gnome | grep -v tny_olpc | grep -v _camel_get_type | grep -v tny_test_stream_get_type |  grep -v tny_gtk_enums_get_type | grep -v tny_enums_get_type |  grep -v tny_acap_account_store_get_type | grep -v webkit | grep -v html | grep -v async_worker | grep -v mail_noti | cut -d " " -f 2- | cut -d "(" -f 1 | sed s/\ //g >> docs/devel/reference/libtinymail.types


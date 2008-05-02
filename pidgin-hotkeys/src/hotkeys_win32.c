#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkwin32.h>
#include "hotkeys.h"

static HWND dummy_win = NULL;
static FilterFunc filter = NULL;

static LRESULT CALLBACK WndProc(HWND hwnd,
				UINT uMsg,
				WPARAM wParam,
				LPARAM lParam)
{
    if (uMsg == WM_HOTKEY && filter)
	filter(GINT_TO_POINTER(lParam));

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static HWND get_dummy_win()
{
    if (!dummy_win)
    {
	WNDCLASSEX wcex;
	static LPCTSTR classname = "GaimHotkeys";

	memset(&wcex, 0, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.lpszClassName = classname;
	RegisterClassEx(&wcex);

	dummy_win = CreateWindow(classname, "", 0, 0, 0, 0, 0, GetDesktopWindow(),
				 NULL, GetModuleHandle(NULL), 0);
    }

    return dummy_win;
}

gint real_event_filter(gpointer event_data,
		       HotkeyEntry* hotkeys,
		       guint num_hotkeys)
{
    gint i;
    
    for (i = 0; i < num_hotkeys; i++)
    {
        if (hotkeys[i].code == HIWORD(event_data) &&
            hotkeys[i].mod == (LOWORD(event_data) & (MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN)))
            break;
    }

    return i;
}

gchar* keycode_to_string(KeyCode keycode,
			 GtkWidget* widget)
{
    guint *keyvals;
    gint n;
    gboolean retval;
    gchar *keystr = NULL;
    GdkDisplay* display;

    display = widget ? gtk_widget_get_display(widget) : gdk_display_get_default();
    if (!display)
        return FALSE;

    retval = gdk_keymap_get_entries_for_keycode(
	gdk_keymap_get_for_display(display),
	keycode, NULL, &keyvals, &n);
    if (!retval || !n)
	return NULL;

    keystr = gdk_keyval_name(keyvals[0]);
    g_free(keyvals);

    return keystr;
}

gboolean parse_keystr(const gchar* keystr,
		      GtkWidget* widget,
		      HotkeyEntry* key)
{
    guint keyval;
    GdkKeymapKey *keys;
    gint n;
    GdkDisplay* display;

    if (!keystr || strlen(keystr) < 1)
        return FALSE;
  
    keyval = gdk_keyval_from_name(keystr);
    if (keyval == GDK_VoidSymbol)
	    return FALSE;

    display = widget ? gtk_widget_get_display(widget) : gdk_display_get_default();
    if (!display)
        return FALSE;

    gdk_keymap_get_entries_for_keyval(
	gdk_keymap_get_for_display(display),
	keyval, &keys, &n);
    if (n < 1)
	return FALSE;
    key->code = keys[0].keycode;

    return TRUE;
}

static gchar *make_hotkey_atom_name(HotkeyEntry* key)
{
    return g_strdup_printf("GAIM_HOTKEY_%d_%d", key->mod, key->code);
}

static ATOM make_hotkey_atom(HotkeyEntry* key)
{
    gchar *atom_name;
    ATOM atom;

    atom_name = make_hotkey_atom_name(key);
    atom = GlobalAddAtom(atom_name);
    g_free(atom_name);
    
    return atom;
}

static ATOM find_hotkey_atom(HotkeyEntry* key)
{
    gchar *atom_name;
    ATOM atom;

    atom_name = make_hotkey_atom_name(key);
    atom = GlobalFindAtom(atom_name);
    g_free(atom_name);
    
    return atom;
}

gboolean grab_key(GdkDisplay* gdisplay,
		  GdkWindow* groot,
		  HotkeyEntry* key)
{
    ATOM atom;

    if (!key->code || !key->mod)
        return FALSE;
  
    if (!(atom = make_hotkey_atom(key)))
	return FALSE;

    return RegisterHotKey(get_dummy_win(), atom, key->mod, key->code);
}

gboolean free_key(GdkDisplay* gdisplay,
		  GdkWindow* groot,
		  HotkeyEntry* key)
{
    ATOM atom;
    gboolean retval;

    if (!key->code || !key->mod)
        return FALSE;
  
    if (!(atom = find_hotkey_atom(key)))
	return FALSE;
    
    retval = UnregisterHotKey(get_dummy_win(), atom);
    GlobalDeleteAtom(atom);

    return retval;
}

void _free_keys(GdkDisplay* gdisplay,
		GdkWindow* groot,
		HotkeyEntry* hotkeys,
		guint num_hotkeys)
{
    guint i;

    for (i = 0; i < num_hotkeys; ++i)
	free_key(gdisplay, groot, hotkeys + i);
}

void hacky_active_window(GtkWidget *window)
{
}

gboolean
setup_filter(GdkWindow *root, FilterFunc filter_func)
{
    filter = filter_func;

    return TRUE;
}

void release_filter(GdkWindow *root, FilterFunc filter_func)
{
    HWND hwnd = get_dummy_win();

    if (hwnd)
    {
	DestroyWindow(hwnd);
	hwnd = NULL;
    }
    filter = NULL;
}

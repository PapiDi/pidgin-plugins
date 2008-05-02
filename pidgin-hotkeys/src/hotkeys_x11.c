#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "hotkeys.h"
#include <string.h>

/* private functions */

gint
real_event_filter(gpointer event_data,
		  HotkeyEntry* hotkeys,
		  guint num_hotkeys)
{
    XKeyEvent *keyevent = (XKeyEvent*)event_data;
    gint i;

    if (((XEvent*)keyevent)->type != KeyPress)
        return -1;

    for (i = 0; i < num_hotkeys; i++)
    {
        if (hotkeys[i].code == keyevent->keycode &&
            hotkeys[i].mod == (keyevent->state & (Mod1Mask | ControlMask | ShiftMask | Mod4Mask)))
            break;
    }

    return i;
}

gchar*
keycode_to_string(KeyCode keycode, GtkWidget* widget)
{
    gchar *keyname;
    KeySym sym;
    Display *display;

    keyname = NULL;
    display = widget ? GDK_DISPLAY_XDISPLAY(gtk_widget_get_display(widget)) : GDK_DISPLAY();
    sym = XKeycodeToKeysym(display, keycode, 0);
    if (sym != NoSymbol)
        keyname = XKeysymToString(sym);

    return keyname;
}

gboolean
parse_keystr(const gchar* keystr, GtkWidget* widget, HotkeyEntry* key)
{
    Display* display;
    KeySym sym;

    display = widget ? GDK_DISPLAY_XDISPLAY(gtk_widget_get_display(widget)) : GDK_DISPLAY();
    if (!display)
        return FALSE;

    if (strlen(keystr) < 1)
        return FALSE;
  
    sym = XStringToKeysym(keystr);
    if (sym == NoSymbol)
	    return FALSE;

    key->code = XKeysymToKeycode(display, sym);
    if (!key->code)
	    return FALSE;

    return TRUE;
}

gboolean
grab_key(GdkDisplay* gdisplay, GdkWindow* groot, HotkeyEntry* key)
{
    KeyCode code;
    gint mod;
    Display* display;
    Window root;

    display = GDK_DISPLAY_XDISPLAY(gdisplay);
    root = GDK_WINDOW_XID(groot);
    code = key->code;
    mod = key->mod;

    if (!code || !mod)
        return FALSE;
  
    gdk_error_trap_push();

    XGrabKey(display, code, mod, root, True,
	     GrabModeAsync, GrabModeAsync);
    XGrabKey(display, code, mod | Mod2Mask, root, True,
	     GrabModeAsync, GrabModeAsync);
    XGrabKey(display, code, mod | Mod5Mask, root, True,
	     GrabModeAsync, GrabModeAsync);
    XGrabKey(display, code, mod | LockMask, root, True,
	     GrabModeAsync, GrabModeAsync);
    XGrabKey(display, code, mod | Mod2Mask | LockMask, root, True,
	     GrabModeAsync, GrabModeAsync);
    XGrabKey(display, code, mod | Mod5Mask | LockMask, root, True,
	     GrabModeAsync, GrabModeAsync);
    XGrabKey(display, code, mod | Mod2Mask | Mod5Mask, root, True,
	     GrabModeAsync, GrabModeAsync);
    XGrabKey(display, code, mod | Mod2Mask | Mod5Mask | LockMask, root, True,
	     GrabModeAsync, GrabModeAsync);

    gdk_flush();

    if (gdk_error_trap_pop())
        return FALSE;

    return TRUE;
}

gboolean
free_key(GdkDisplay* gdisplay, GdkWindow* groot, HotkeyEntry* key)
{
    gint code, modifiers;
    Display* display;
    Window root;

    display = GDK_DISPLAY_XDISPLAY(gdisplay);
    root = GDK_WINDOW_XID(groot);

    code = key->code;
    modifiers = key->mod;

    XUngrabKey(display, code, modifiers, root);
    XUngrabKey(display, code, modifiers | Mod2Mask, root);
    XUngrabKey(display, code, modifiers | Mod5Mask, root);
    XUngrabKey(display, code, modifiers | LockMask, root);
    XUngrabKey(display, code, modifiers | Mod2Mask | LockMask, root);
    XUngrabKey(display, code, modifiers | Mod5Mask | LockMask, root);
    XUngrabKey(display, code, modifiers | Mod2Mask | Mod5Mask, root);
    XUngrabKey(display, code, modifiers | Mod2Mask | Mod5Mask | LockMask, root);

    return TRUE;
}

void
_free_keys(GdkDisplay* gdisplay, GdkWindow* groot, HotkeyEntry* hotkeys, guint num_hotkeys)
{
    XUngrabKey(GDK_DISPLAY_XDISPLAY(gdisplay), AnyKey, AnyModifier, GDK_WINDOW_XID(groot));
}

void
hacky_active_window(GtkWidget *window)
{
    GdkScreen *screen;
    GdkWindow *root;
    GdkDisplay *display;
    Display *xdisplay;
    Window xroot;
    XEvent xev;
    static Atom _net_active_window = None;

    screen = gtk_widget_get_screen(window);
    root = gdk_screen_get_root_window(screen);
    display = gdk_screen_get_display(screen);

    xdisplay = GDK_DISPLAY_XDISPLAY(display);
    xroot = GDK_WINDOW_XWINDOW(root);

    if (_net_active_window == None)
	_net_active_window = XInternAtom(xdisplay,
					 "_NET_ACTIVE_WINDOW",
					 False);

    xev.xclient.type = ClientMessage;
    xev.xclient.serial = 0;
    xev.xclient.send_event = True;
    xev.xclient.window = GDK_WINDOW_XWINDOW(window->window);
    xev.xclient.message_type = _net_active_window;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1; /* requestor type; we're an app, I guess */
    xev.xclient.data.l[1] = CurrentTime;
    xev.xclient.data.l[2] = None; /* "currently active window", supposedly */
    xev.xclient.data.l[3] = 0;
    xev.xclient.data.l[4] = 0;

    XSendEvent(xdisplay,
	       xroot, False,
	       SubstructureRedirectMask | SubstructureNotifyMask,
	       &xev);
}

static GdkFilterReturn 
gdk_filter(GdkXEvent *xevent,
	   GdkEvent *event,
	   gpointer data)
{
    if (data && ((FilterFunc)data)(xevent))
	return GDK_FILTER_REMOVE;

    return GDK_FILTER_CONTINUE;
}

gboolean
setup_filter(GdkWindow *root, FilterFunc filter_func)
{
    gdk_window_add_filter(root,
                          gdk_filter,
                          filter_func);

    return TRUE;
}

void release_filter(GdkWindow *root, FilterFunc filter_func)
{
    gdk_window_remove_filter(root,
			     gdk_filter,
			     filter_func);
}


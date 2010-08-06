#include <gtk/gtk.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

/*********************************
 * TOFIX: now I used 3 state to show 
 * status, set_mode() has to be changed 
 */

#define INIT_MODE 0
#define WRITTING_MODE 2
#define PAUSE_MODE 3

#define SETTING_MODE 1
#define WRITTING_MODE 2

#define false FALSE
#define true TRUE

#define STATUS_STRING "xzpeter@gmail.com"
#define COUNT_SCRIPT "/home/peter/codes/gtk/gre/count_word.pl"

/* GtkWidget is the storage type for widgets */
GtkWidget *window;
GtkWidget *view, *vbox1, *hbox2, *label_min, *label_sec, 
					*entry_min, *entry_sec, *start_button, *quit_button;
GtkWidget *entry_topic, *label_topic, *hbox3, *stop_button, *hbox_status, 
					*label_rest_time, *label_status, *debug_button;
GtkEntryBuffer *min_buf, *sec_buf, *topic_buf;
GtkTextBuffer *buffer;
PangoFontDescription *font;

static int time_left;

/* update time_left onto screen */
void update_time(void)
{
	char buf[256];
	if (label_rest_time == NULL)
		return;
	if (time_left > 3600) { /* there is a hour */
		int hour = time_left / 3600;
		int min = (time_left % 3600) / 60;
		int sec = (time_left % 60);
		snprintf(buf, 256, "%d:%02d:%02d", hour, min, sec);
	} else {
		snprintf(buf, 256, "%02d:%02d", time_left/60, time_left%60);
	}
	/* update buf */
	gtk_label_set_text(GTK_LABEL(label_rest_time), buf);
}

// /* enable/disable widget */
// void switch_widget(int writable)
// {
// 	if (writable) {
// 	}
// }

void set_mode(int type)
{
	int min, sec;
	switch (type) {
		case SETTING_MODE:
			/* active/unactive some widgets */
			gtk_widget_set_sensitive(entry_min, 1);
			gtk_widget_set_sensitive(entry_sec, 1);
			gtk_widget_set_sensitive(start_button, 1);
			gtk_widget_set_sensitive(stop_button, 0);
			gtk_widget_set_sensitive(view, 0);
			gtk_widget_set_sensitive(label_rest_time, 0);
			/* clear left_time */
			time_left = 0;
			update_time();
			break;
		case WRITTING_MODE:
			gtk_widget_set_sensitive(entry_min, 0);
			gtk_widget_set_sensitive(entry_sec, 0);
			gtk_widget_set_sensitive(start_button, 0);
			gtk_widget_set_sensitive(stop_button, 1);
			gtk_widget_set_sensitive(view, 1);
			gtk_widget_set_sensitive(label_rest_time, 1);
			/* update time_left */
			min = atoi(gtk_entry_get_text(GTK_ENTRY(entry_min)));
			sec = atoi(gtk_entry_get_text(GTK_ENTRY(entry_sec)));
			g_message("min=%d, sec=%d.", min, sec);
			time_left = min*60+sec;
			update_time();
			/* trigger alarm */
			alarm(1);
			break;
		default:
			g_message("recved error type in %s.", __func__);
			break;
	}
}

static void callback( GtkWidget *widget,
		gpointer   data )
{
	g_message ("Hello again - %s was pressed", (gchar *) data);
}

/* callback of quit signal */
static void quit_handler( GtkWidget *widget,
		gpointer   data )
{
	gtk_main_quit();
}

int save_file(char *str)
{
	/* save article under the current folder */
	int fd, count;
	time_t the_time;
	struct tm *ptm;
	char file_name[256], str_buf[256];
	char *s;

	/* file name format */
	the_time = time(NULL);
	ptm = localtime(&the_time);
	snprintf(file_name, 256, "article_%02d.%02d.%02d_%02d.%02d.%02d.txt", 
			ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, 
			ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

	/* open file */
	fd = open(file_name, O_WRONLY | O_CREAT, 0644);
	if (fd == -1) {
		g_message ("failed to open: errno=%d, errstr=%s", errno, strerror(errno));
		return -1;
	}
	s = "# Topic:\n#\t";
	write(fd, s, strlen(s));
	s = gtk_entry_get_text(GTK_ENTRY(entry_topic));
	write(fd, s, strlen(s));
	s = "\n# Article:\n\n";
	write(fd, s, strlen(s));
	write(fd, str, strlen(str));
	close(fd);
	g_message("save file successfully to %s.", file_name);

	/* if -x count_word.pl, count the words. */
	if (access(COUNT_SCRIPT, X_OK)) {
		g_message("cannot access to count_word.pl, skip counting.");
		return 0;
	}
	/* do the count */
	snprintf(str_buf, 256, COUNT_SCRIPT" %s", file_name);
	s = system(str_buf);
	g_message("word count updated to txt file.");
	return 0;
}

static void save_handler(GtkWidget *widget, gpointer data)
{
// 	gtk_label_set_text(GTK_LABEL(label_rest_time), "xx:xx");
	const GtkTextIter start, end;
	char *str;

	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buffer), &start);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(buffer), &end);
	str = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buffer), &start, &end, TRUE);

	save_file(str);

	return;
}

static void start_handler(GtkWidget *widget, gpointer data)
{
	gint result;
	GtkWindow *window = (GtkWindow *)data;
	GtkWidget *dialog;
	dialog = gtk_dialog_new_with_buttons ("My dialog",
			window,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			NULL);

	/* count time to alarm */
	set_mode(WRITTING_MODE);
// 	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), "", 0);
	gtk_window_set_focus (GTK_WINDOW(window), GTK_WIDGET(view));
	return;

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	switch (result) {
		default:
			break;
	}
	gtk_widget_destroy(dialog);
}

void *alarm_handler(int signal)
{
	if (time_left-- == 0) {
		set_mode(SETTING_MODE);
	} else {
		update_time();
		alarm(1);
		/* still working */
	}
	return NULL;
}

static void stop_handler(GtkWidget *widget, gpointer data)
{
	set_mode(SETTING_MODE);
}

/* another callback */
static gboolean delete_event( GtkWidget *widget,
		GdkEvent  *event,
		gpointer   data )
{
	gtk_main_quit ();
	return FALSE;
}

int main( int   argc,
		char *argv[] )
{
	/* This is called in all GTK applications. Arguments are parsed
	 * from the command line and are returned to the application. */
	gtk_init (&argc, &argv);

	/* Create a new window */
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	/* This is a new call, which just sets the title of our
	 * new window to "Hello Buttons!" */
	gtk_window_set_title (GTK_WINDOW (window), "FIGHTING FOR GRE!");

	/* Here we just set a handler for delete_event that immediately
	 * exits GTK. */
	g_signal_connect (G_OBJECT (window), "delete_event",
			G_CALLBACK (delete_event), NULL);

	/* Sets the border width of the window. */
	gtk_container_set_border_width (GTK_CONTAINER (window), 10);

	/* create boxes */
	vbox1 = gtk_vbox_new(FALSE, 5);
	hbox2 = gtk_hbox_new(FALSE, 5);
	hbox3 = gtk_hbox_new(FALSE, 0);
	hbox_status = gtk_hbox_new(FALSE, 20);
// 	gtk_widget_set_size_request(hbox2, 600, 30);
// 	gtk_widget_set_size_request(hbox3, 600, 30);

	/* first row */
	/* labels and buttons */
	label_min = gtk_label_new("Minute(s):");
	label_sec = gtk_label_new("Second(s):");
	entry_min = gtk_entry_new();
	entry_sec = gtk_entry_new();
	gtk_widget_set_size_request(entry_min, 50, -1);
	gtk_widget_set_size_request(entry_sec, 50, -1);
// 	gtk_widget_set_size_request(label_min, 80, -1);
// 	gtk_widget_set_size_request(label_sec, 80, -1);
	min_buf = gtk_entry_get_buffer(GTK_ENTRY(entry_min));
	sec_buf = gtk_entry_get_buffer(GTK_ENTRY(entry_sec));
	gtk_entry_buffer_set_text(min_buf, "10", -1);
	gtk_entry_buffer_set_text(sec_buf, "0", -1);
	start_button = gtk_button_new_with_label("Start timer");
	g_signal_connect(start_button, "clicked", start_handler, (gpointer)window);
	stop_button = gtk_button_new_with_label("Stop timer");
	g_signal_connect(stop_button, "clicked", stop_handler, (gpointer)window);
	debug_button = gtk_button_new_with_label("Save Article");
	g_signal_connect(debug_button, "clicked", save_handler, (gpointer)window);
	quit_button = gtk_button_new_with_label("Quit");
	g_signal_connect(quit_button, "clicked", quit_handler, NULL);
	gtk_box_pack_start(GTK_BOX(hbox2), label_min, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), entry_min, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), label_sec, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), entry_sec, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), start_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), stop_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), debug_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), quit_button, TRUE, TRUE, 0);

	/* row 2 */
	/* topics */
	label_topic = gtk_label_new("Topic:");
	entry_topic = gtk_entry_new();
// 	gtk_widget_set_size_request(label_topic, 80, -1);
	gtk_widget_set_size_request(entry_topic, 500, -1);
	topic_buf = gtk_entry_get_buffer(GTK_ENTRY(entry_topic));
// 	gtk_entry_buffer_set_text(topic_buf, "Please input your topic here.", -1);
	gtk_box_pack_start(GTK_BOX(hbox3), label_topic, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox3), entry_topic, TRUE, TRUE, 0);

	/* row data input */
	/* create multiple entry */
	view = gtk_text_view_new();
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
//  	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), "Input your article here.", -1);
	gtk_widget_set_size_request(view, 800, 500);
	gtk_text_view_set_wrap_mode(view, GTK_WRAP_WORD);

	/* row status */
	label_rest_time = gtk_label_new("00:00");
	label_status = gtk_label_new(STATUS_STRING);
	gtk_box_pack_start(GTK_BOX(hbox_status), label_rest_time, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hbox_status), label_status, false, false, 0);

	/* do the layout */
	gtk_container_add(GTK_CONTAINER(window), vbox1);
	gtk_container_add(GTK_CONTAINER(vbox1), hbox2);
	gtk_container_add(GTK_CONTAINER(vbox1), hbox3);
	gtk_container_add(GTK_CONTAINER(vbox1), view);
	gtk_container_add(GTK_CONTAINER(vbox1), hbox_status);

	/* disable some widgets at the very beginning */
	set_mode(SETTING_MODE);
	/* connect alarm signal */
	signal(SIGALRM, alarm_handler);

	/* set focus and view fonts */
	gtk_window_set_focus (GTK_WINDOW(window), GTK_WIDGET(start_button));
// 	font = pango_font_description_new();
// 	pango_font_description_set_weight(font, PANGO_WEIGHT_MEDIUM);
// 	pango_font_description_set_size(font, 100);
// 	gtk_widget_modify_font(GTK_WIDGET(view), font);

	gtk_widget_show_all (window);

	/* Rest in gtk_main and wait for the fun to begin! */
	gtk_main ();

	return 0;
}

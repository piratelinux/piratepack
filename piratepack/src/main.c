#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <gtk/gtk.h>

typedef struct _Data Data;
struct _Data
{
  GtkWindow * window;

  GtkHButtonBox * hbuttonbox;
  
  GtkProgressBar * progress;

  GtkLabel * message;

  GtkButton * button_enable;
  GtkButton * button_disable;
  GtkButton * button_update;
  GtkButton * button_reenable;
  GtkButton * button_yes;
  GtkButton * button_no;

  gchar * action;
  gchar * curpath;
  gchar * file;
  gchar * targetname;
  gchar * homedir;

  gchar * processpath;
  gchar * basedir;
  gchar * maindir;
  gchar * versionpath;

  gchar ** argv;
  gint argc;
 
  gint timeout_id;
};

static void cb_execute_install(GtkButton *button, Data *data);
static void cb_execute_reinstall(GtkButton *button, Data *data);
static void cb_execute_remove(GtkButton *button, Data *data);
static void cb_execute_update(GtkButton *button, Data *data);

gchar* substring(const gchar* str, size_t begin, size_t len)
{
  if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len))
    return 0;

  return strndup(str + begin, len);
}

gchar* exec(gchar* cmd, gint size) {
  FILE* pipe = popen(cmd, "r");
  if (!pipe) return "ERROR";
  gchar * result = g_malloc(size);
  strcpy(result,"");
  gchar * retch = fgets(result,size,pipe);
  gchar * resultmov = result;
  int totsize = size;
  while((retch != 0) && (!feof(pipe))) {
    int reslen = strlen(resultmov);
    if (reslen == (size-1)) {
      totsize = totsize+size;
      result = g_realloc(result,totsize);
    }
    resultmov = resultmov + reslen;
    retch = fgets(resultmov,size,pipe);
  }
  pclose(pipe);
  return result;
}

gchar * get_readlink(gchar * procpath) {

    gint size = 100;
    gchar * buffer = g_malloc(size);

    do {
      int nchars = readlink(procpath, buffer, size);
      if (nchars < 0) {
	g_free(buffer);
	return(0);
      }
      if (nchars < size) {
	gchar * index = buffer + nchars;
	*index = '\0';
	return buffer;
      }
      size = size*2;
      buffer = g_realloc(buffer,size);
    } while (1);
}

gchar * get_dirname(gchar * path) {

  gchar * index = path + strlen(path) - 1;
  int len = strlen(path);
  while (*index != '/') {
    index = index - 1;
    len = len - 1;
  }
  index = index - 1;
  len = len - 1;
  return substring(path,0,len);

}

gint getargi(gchar * target, int argc, char ** argv) {
  int i = 1;
  while (i < argc) {
    if (strcmp(target,argv[i]) == 0) {
      return i;
    }
    i++;
  }
  return -1;
}

int numDigits(int num)
{
  int dig = 0;
  if (num < 0) dig = 1; // remove this line if '-' counts as a digit
  while (num) {
    num /= 10;
    dig++;
  }
  return dig;
}

static void
release_locks(gchar * str, gchar * pid_str, gchar * homedir, gchar * prefix) {

  int ret = 0;

  strcpy(str,homedir);
  strcat(str,"/.piratepack/");
  strcat(str,prefix);
  strcat(str,".lock-");
  strcat(str,pid_str);
  ret = remove(str);

  strcpy(str,homedir);
  strcat(str,"/.piratepack/");
  strcat(str,prefix);
  strcat(str,".lock");
  FILE * f = fopen (str , "r");

  if (f == 0) {

    fprintf(stderr,"Error opening file");

  }

  else {

    gint size = 20;
    gchar * line = g_malloc(20);
    strcpy(line,"");

    while (!feof(f)) {

      if(fgets(line, size, f) != 0) {

	gchar * endchar = line + strlen(line) - 1;
	
	if (*endchar == '\n') {

	  *endchar = '\0';
	  break;

	}
	
      }

    }
    
    if (strcmp(line,pid_str) == 0) {

      ret = remove(str);

    }

    g_free(line);
    fclose(f);

  }

}

int refresh_tor(gchar * maindir,gchar * homedir) {

  int ret;
  gchar * str2 = g_malloc(strlen(maindir)+strlen(homedir)+100);
  gchar * pids = 0;
  gchar * rest = 0;
  gchar * tok = 0;
  gchar * ptr = 0;
  
  strcpy(str2,maindir);
  strcat(str2,"/bin/polipo -c ");
  strcat(str2,homedir);
  strcat(str2,"/.polipo_tor >> /dev/null 2>> /dev/null &");
  ret = system(str2);

  pids = exec("pidof tor",100);
  rest = 0;
  tok = 0;
  ptr = pids;
  
  if (!(tok = strtok_r(ptr, " \n", &rest))) {
    
    ret = system("kill $(pidof vidalia) >> /dev/null 2>> /dev/null");
    
    strcpy(str2,maindir);
    strcat(str2,"/bin/tor-instance >> /dev/null 2>> /dev/null &");
    ret = system(str2);
  }
  
  else {
    
    if (pids != 0) {
      g_free(pids);
    }
    pids = exec("pidof vidalia",100);
    ptr = pids;
    
    if (!(tok = strtok_r(ptr, " \n", &rest))) {
      
      ret = system("kill $(pidof tor) >> /dev/null 2>> /dev/null");
      
      strcpy(str2,maindir);
      strcat(str2,"/bin/tor-instance >> /dev/null 2>> /dev/null &");
      ret = system(str2);
    }
  }
  if (pids != 0) {
    g_free(pids);
  }
  g_free(str2);
  return(0);
}

int refresh_theme(gchar * maindir, const gchar * homedir, gchar * option) {

  int ret = 0;
  gchar * str = g_malloc(strlen(maindir)+strlen(homedir)+100);

  strcpy(str,homedir);
  strcat(str,"/.piratepack/theme/.installed");
  if (g_file_test(str,G_FILE_TEST_IS_REGULAR)) {
    g_free(str);
    return(0);
  }

  strcpy(str,homedir);
  strcat(str,"/.piratepack/theme");
  while (!g_file_test(str,G_FILE_TEST_IS_DIR)) {
    sleep(1);
  }

  strcpy(str,maindir);
  strcat(str,"/share/theme");
  ret = chdir(str);

  strcpy (str,"./install_theme.sh");
  ret = system(str);

  g_free(str);
  return(0);
}
 
static void
cb_child_watch( GPid  pid,
                gint  status,
                Data *data )
{
 
    /* Close pid */
    g_spawn_close_pid( pid );
}
 
static gboolean
cb_out_watch( GIOChannel   *channel,
              GIOCondition  cond,
              Data         *data )
{
    gchar *string;
    gsize  size;
 
    if( cond == G_IO_HUP )
    {
        g_io_channel_unref( channel );
        return( FALSE );
    }
 
    g_io_channel_read_line( channel, &string, &size, 0, 0 );

    //printf("out_watch:%s\n",string);

    g_free( string );
 
    return( TRUE );
}
 
static gboolean
cb_err_watch( GIOChannel   *channel,
              GIOCondition  cond,
              Data         *data )
{
    gchar *string;
    gsize  size;
 
    if( cond == G_IO_HUP ) {
        g_io_channel_unref( channel );
        return( FALSE );
    }
 
    g_io_channel_read_line( channel, &string, &size, 0, 0 );
    gtk_label_set_label( data->message, string);
    g_free( string );

    if (strcmp(data->action,"install")==0) {
      if (g_strcmp0("Enabled\n",gtk_label_get_label(data->message)) == 0) {
	gtk_container_remove((GtkContainer *)data->hbuttonbox,(GtkWidget *)data->button_enable);
	data->button_disable = (GtkButton *)gtk_button_new_with_label("Disable");
	gtk_widget_set_size_request((GtkWidget *)data->button_disable,120,-1);
	g_signal_connect(G_OBJECT(data->button_disable), "clicked", G_CALLBACK(cb_execute_remove), data);
	gtk_container_add((GtkContainer *)data->hbuttonbox,(GtkWidget *)data->button_disable);
	gtk_widget_show((GtkWidget *)data->button_disable);
	/* Remove timeout callback */
	g_source_remove( data->timeout_id );
	gtk_progress_bar_set_fraction((GtkProgressBar *)data->progress, 0.0);
	g_free(data->action);
      }
    }
    else if (strcmp(data->action,"update")==0) {
      if (g_strcmp0("Updated\n",gtk_label_get_label(data->message)) == 0) {
	gtk_widget_set_sensitive((GtkWidget *)data->button_disable,TRUE);
	/* Remove timeout callback */
	g_source_remove( data->timeout_id );
	gtk_progress_bar_set_fraction((GtkProgressBar *)data->progress, 0.0);
	g_free(data->action);
      }
    }
    else if (strcmp(data->action,"remove")==0) {
      if (g_strcmp0("Disabled\n",gtk_label_get_label((GtkLabel *)data->message)) == 0) {
        gtk_container_remove((GtkContainer *)data->hbuttonbox,(GtkWidget *)data->button_disable);
        data->button_enable = (GtkButton *)gtk_button_new_with_label("Enable");
        gtk_widget_set_size_request((GtkWidget *)data->button_enable,120,-1);
        g_signal_connect( G_OBJECT( data->button_enable ), "clicked", G_CALLBACK(cb_execute_install), data);
        gtk_container_add((GtkContainer *)data->hbuttonbox,(GtkWidget *)data->button_enable);
        gtk_widget_show((GtkWidget *)data->button_enable);
	/* Remove timeout callback */
	g_source_remove( data->timeout_id );
	gtk_progress_bar_set_fraction((GtkProgressBar *)data->progress, 0.0);
	g_free(data->action);
      }
    }
    else if (strcmp(data->action,"install_pirate_file")==0) {
      if ( (g_strcmp0("Installed\n",gtk_label_get_label(data->message)) == 0) || (g_strcmp0("File not authentic\n",gtk_label_get_label(data->message)) == 0) || (g_strcmp0("Error\n",gtk_label_get_label(data->message)) == 0) ) {
        gtk_container_remove((GtkContainer *)data->hbuttonbox,(GtkWidget *)data->button_yes);
        gtk_container_remove((GtkContainer *)data->hbuttonbox,(GtkWidget *)data->button_no);
        data->button_yes = (GtkButton *)gtk_button_new_with_label("Exit");
        gtk_widget_set_size_request((GtkWidget *)data->button_yes,120,-1);
        g_signal_connect(G_OBJECT(data->button_yes), "clicked", G_CALLBACK(gtk_main_quit),0);
        gtk_container_add((GtkContainer *)data->hbuttonbox,(GtkWidget *)data->button_yes);
        gtk_widget_show((GtkWidget *)data->button_yes);
	/* Remove timeout callback */
	g_source_remove( data->timeout_id );
	gtk_progress_bar_set_fraction((GtkProgressBar *)data->progress, 0.0);
	g_free(data->action);
      }
    }
    return( TRUE );
}
 
static gboolean
cb_timeout( Data *data )
{
    /* Bounce progress bar */
    gtk_progress_bar_pulse( data->progress );
 
    return( TRUE );
}
 
static void
cb_execute( GtkButton *button,
            Data      *data)
{

    GPid        pid;
    gchar    **argv;
    gint argc = -1;

    if (strcmp(data->action,"install") == 0) {
      argc = 3;
      argv = g_malloc(sizeof(gchar*) * 4);
      argv[0] = strdup(data->processpath);
      argv[1] = strdup("--install");
      argv[2] = strdup("--async");
      argv[3] = 0;
      gtk_widget_set_sensitive((GtkWidget *)data->button_enable,FALSE);
    }
    else if (strcmp(data->action,"reinstall") == 0) {
      argc = 3;
      argv = g_malloc(sizeof(gchar*) * 4);
      argv[0] = strdup(data->processpath);
      argv[1] = strdup("--reinstall");
      argv[2] = strdup("--async");
      argv[3] = 0;
    }
    else if (strcmp(data->action,"update") == 0) {
      argc = 3;
      argv = g_malloc(sizeof(gchar*) * 4);
      argv[0] = strdup(data->processpath);
      argv[1] = strdup("--update");
      argv[2] = strdup("--async");
      argv[3] = 0;
      gtk_widget_set_sensitive((GtkWidget *)data->button_disable,FALSE);
    }
    else if (strcmp(data->action,"remove") == 0) {
      argc = 3;
      argv = g_malloc(sizeof(gchar*) * 4);
      argv[0] = strdup(data->processpath);
      argv[1] = strdup( "--remove");
      argv[2] = strdup("--async");
      argv[3] = 0;
      gtk_widget_set_sensitive((GtkWidget *)data->button_disable,FALSE);
    }
    else if (strcmp(data->action,"install_pirate_file") == 0) {
      argc = 6;
      argv = g_malloc(sizeof(gchar*) * 7);
      argv[0] = strdup(data->processpath);
      argv[1] = strdup("--install-pirate-file");
      argv[2] = strdup(data->curpath);
      argv[3] = strdup(data->file);
      argv[4] = strdup(data->targetname);
      argv[5] = strdup("--async");
      argv[6] = 0;
      gtk_widget_set_sensitive((GtkWidget *)data->button_yes,FALSE);
      gtk_widget_set_sensitive((GtkWidget *)data->button_no,FALSE);
    }
    else if (strcmp(data->action,"refresh_tor") == 0) {
      argc = 3;
      argv = g_malloc(sizeof(gchar*) * 4);
      argv[0] = strdup(data->processpath);
      argv[1] = strdup("--refresh-tor");
      argv[2] = strdup("--async");
      argv[3] = 0;
    }
    else if (strcmp(data->action,"refresh_theme") == 0) {
      argc = 2;
      argv = g_malloc(sizeof(gchar*) * 3);
      argv[0] = strdup(data->processpath);
      argv[1] = strdup("--refresh-theme");
      argv[2] = 0;
    }
    else if (strcmp(data->action,"start_version") == 0) {
      argc = data->argc;
      argv = g_malloc(sizeof(gchar*) * (data->argc + 1));
      argv[0] = strdup(data->versionpath);
      int i;
      for (i=1; i < (data->argc); i++) {
	argv[i] = strdup((data->argv)[i]);
      }
      argv[(data->argc - 1)] = 0;
    }

    gint in, out, err;
    GIOChannel *in_ch, *out_ch, *err_ch;
    gboolean ret;
 
    /* Spawn child process */
    ret = g_spawn_async_with_pipes(0, argv, 0, G_SPAWN_DO_NOT_REAP_CHILD, 0, 0, &pid, &in, &out, &err, 0);
    if( ! ret ) {
      g_error( "SPAWN FAILED" );
      return;
    }

    /* Add watch function to catch termination of the process. This function
     * will clean any remnants of process. */
    g_child_watch_add( pid, (GChildWatchFunc)cb_child_watch, data );
 
    /* Create channels that will be used to read data from pipes. */
#ifdef G_OS_WIN32
    in_ch = g_io_channel_win32_new_fd( in );
    out_ch = g_io_channel_win32_new_fd( out );
    err_ch = g_io_channel_win32_new_fd( err );
#else
    in_ch = g_io_channel_unix_new( in );
    out_ch = g_io_channel_unix_new( out );
    err_ch = g_io_channel_unix_new( err );
#endif
 
    /* Add watches to channels */
    g_io_add_watch( out_ch, G_IO_IN | G_IO_HUP, (GIOFunc)cb_out_watch, data );
    g_io_add_watch( err_ch, G_IO_IN | G_IO_HUP, (GIOFunc)cb_err_watch, data );

    /* Install timeout function that will move the progress bar */
    data->timeout_id = g_timeout_add( 100, (GSourceFunc)cb_timeout, data );

    if ((strcmp(data->action,"refresh_theme")!=0)&&(strcmp(data->action,"refresh_tor")!=0)) {
      int pid_len = numDigits(pid);
      gchar * pid_str = g_malloc(pid_len+1);
      sprintf(pid_str,"%d",pid);
      gchar * str = g_malloc(strlen(data->homedir)+100);
      strcpy(str,data->homedir);
      strcat(str,"/.piratepack/.lock-");
      strcat(str,pid_str);
      FILE * f;
      f = fopen (str,"w");
      if (f != 0) {
	fclose(f);
      }
      g_free(str);
      g_free(pid_str);
      
      gsize * bytes_written = 0;
      GError * error = 0;
      g_io_channel_write_chars(in_ch, "ready\n", -1, bytes_written, &error);
      g_io_channel_flush(in_ch, &error);
      
      g_free(bytes_written);
      g_free(error);
    }

    g_io_channel_unref(in_ch);
    int i=0;
    while (i < argc) {
      g_free(argv[i]);
      i++;
    }
    g_free(argv);
}

static void cb_execute_install( GtkButton *button,
				Data      *data)
{
 
  data->action = strdup("install");
  cb_execute(button,data);

}

static void cb_execute_reinstall( GtkButton *button,
                                Data      *data)
{

  data->action = strdup("reinstall");
  cb_execute(button,data);

}

static void cb_execute_update( GtkButton *button,
                                Data      *data)
{

  data->action = strdup("update");
  cb_execute(button,data);

}

static void cb_execute_remove( GtkButton *button,
				Data      *data)
{
  
  data->action = strdup("remove");
  cb_execute(button,data);

}


static void cb_execute_install_pirate_file ( GtkButton *button,
                                Data      *data)
{

  data->action = strdup("install_pirate_file");
  cb_execute(button,data);

}

static void cb_execute_refresh_tor ( GtkButton *button,
					     Data      *data)
{

  data->action = strdup("refresh_tor");
  cb_execute(button,data);

}

static void cb_execute_refresh_theme ( GtkButton *button,
				     Data      *data)
{

  data->action = strdup("refresh_theme");
  cb_execute(button,data);

}

static void cb_execute_start_version ( GtkButton *button, Data *data)
{

  data->action = strdup("start_version");
  cb_execute(button,data);

}


int
install_pack(int argc, char **argv, Data * data)
{

  int ret;

  gchar * processpath = data->processpath;
  gchar * basedir = data->basedir;
  gchar * maindir = data->maindir;

  gchar * curpath = g_get_current_dir();
  gchar * homedir = data->homedir;

  gchar * str = g_malloc(2*strlen(homedir)+strlen(curpath)+strlen(basedir)+strlen(maindir)+strlen(processpath)+300);

  //strcpy(str,"Enabling");
  //fprintf( stderr, "%s\n", str );

  gchar * logpipe = g_malloc(2*strlen(homedir)+200);
  
  ret = chdir(homedir);

  if (!g_file_test(".piratepack",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir .piratepack >> /dev/null 2>> /dev/null");
  }

  ret = chdir(".piratepack");

  if (!g_file_test("logs",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir logs >> /dev/null 2>> /dev/null");
  }

  ret = chdir(homedir);

  strcpy (logpipe,">> /dev/null 2>> ");
  strcat (logpipe,homedir);
  strcat (logpipe,"/.piratepack/logs/piratepack_install.log");

  strcpy(str,"echo \"[$(date)]\" >> ");
  strcat(str,homedir);
  strcat(str,"/.piratepack/logs/piratepack_install.log");
  ret = system(str);

  strcpy (str,"chmod u+rwx .piratepack ");
  strcat (str,logpipe);
  ret = system(str);

  ret = chdir(".piratepack");

  if (g_file_test(".installed",G_FILE_TEST_IS_REGULAR)) {
    strcpy (str,"chmod -R u+rw .installed ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf .installed ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy (str,"touch .installed ");
  strcat (str,logpipe);
  ret = system(str);

  gchar * processpathsub = substring(processpath,strlen(basedir)+1,strlen(processpath)-strlen(basedir)-1);
  strcpy (str,"echo Path: ");
  strcat (str,processpathsub);
  strcat (str," >> .installed 2>> logs/piratepack_install.log");
  ret = system(str); 
  g_free(processpathsub);

  //install firefox-mods

  //strcpy(str,"firefox-mods");
  //fprintf( stderr, "%s\n", str );

  if (g_file_test("firefox-mods",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod -R u+rw firefox-mods ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf firefox-mods ");
    strcat (str,logpipe);
    ret = system(str);
  }
  
  strcpy (str,"mkdir firefox-mods ");
  strcat (str,logpipe);
  ret = system(str);
 
  strcpy(str,maindir);
  strcat(str,"/share/firefox-mods");

  ret = chdir(str);

  strcpy (str,"./install_firefox-mods.sh ");
  strcat (str,logpipe);
  ret = system(str);

  strcpy(str,maindir);
  strcat(str,"/bin/polipo >> /dev/null 2>> /dev/null &");
  ret = system(str);

  ret = chdir(homedir);
  ret = chdir(".piratepack");

  ret = system("echo firefox-mods >> .installed 2>> logs/piratepack_install.log");

  //install tor-browser

  //strcpy(str,"tor-browser");
  //fprintf( stderr, "%s\n", str );

  if (g_file_test("tor-browser",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod -R u+rw tor-browser ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf tor-browser ");
    strcat (str,logpipe);
    ret = system(str);
  }
  
  strcpy (str,"mkdir tor-browser ");
  strcat (str,logpipe);
  ret = system(str);

  strcpy(str,maindir);
  strcat(str,"/share/tor-browser");

  ret = chdir(str);
  
  strcpy (str,"./install_tor-browser.sh ");
  strcat (str,logpipe);
  ret = system(str);

  ret = chdir(homedir);
  ret = chdir(".piratepack");

  ret = system("echo tor-browser >> .installed 2>> logs/piratepack_install.log");

  //install i2p-browser

  if (g_file_test("i2p-browser",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod -R u+rw i2p-browser ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf i2p-browser ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy (str,"mkdir i2p-browser ");
  strcat (str,logpipe);
  ret = system(str);

  strcpy(str,maindir);
  strcat(str,"/share/i2p-browser");

  ret = chdir(str);

  strcpy (str,"./install_i2p-browser.sh ");
  strcat (str,logpipe);
  ret = system(str);

  ret = chdir(homedir);
  ret = chdir(".piratepack");

  ret = system("echo i2p-browser >> .installed 2>> logs/piratepack_install.log");

  //install file-manager

  //strcpy(str,"file-manager");
  //fprintf( stderr, "%s\n", str );

  if (g_file_test("file-manager",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod -R u+rw file-manager ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf file-manager ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy (str,"mkdir file-manager ");
  strcat (str,logpipe);
  ret = system(str);

  strcpy(str,maindir);
  strcat(str,"/share/file-manager");

  ret = chdir(str);

  strcpy (str,"./install_file-manager.sh ");
  strcat (str,logpipe);
  ret = system(str);

  ret = chdir(homedir);
  ret = chdir(".piratepack");

  ret = system("echo file-manager >> .installed 2>> logs/piratepack_install.log");

  //install ppcavpn

  //strcpy(str,"ppcavpn");
  //fprintf( stderr, "%s\n", str );

  if (g_file_test("ppcavpn",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod -R u+rw ppcavpn ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf ppcavpn ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy (str,"mkdir ppcavpn ");
  strcat (str,logpipe);
  ret = system(str);

  strcpy(str,maindir);
  strcat(str,"/share/ppcavpn");

  ret = chdir(str);

  strcpy (str,"./install_ppcavpn.sh ");
  strcat (str,logpipe);
  ret = system(str);

  ret = chdir(homedir);
  ret = chdir(".piratepack");

  ret = system("echo ppcavpn >> .installed 2>> logs/piratepack_install.log");

  //install bitcoin

  //strcpy(str,"bitcoin");
  //fprintf( stderr, "%s\n", str );

  if (g_file_test("bitcoin",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod -R u+rw bitcoin ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf bitcoin ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy (str,"mkdir bitcoin ");
  strcat (str,logpipe);
  ret = system(str);

  strcpy(str,maindir);
  strcat(str,"/share/bitcoin");

  ret = chdir(str);

  strcpy (str,"./install_bitcoin.sh ");
  strcat (str,logpipe);
  ret = system(str);

  ret = chdir(homedir);
  ret = chdir(".piratepack");

  ret = system("echo bitcoin >> .installed 2>> logs/piratepack_install.log");

  //install theme

  //strcpy(str,"theme");
  //fprintf( stderr, "%s\n", str );

  if (g_file_test("theme",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod -R u+rw theme ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf theme ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy (str,"mkdir theme ");
  strcat (str,logpipe);
  ret = system(str);

  //cb_execute_refresh_theme(0,data);

  ret = chdir(homedir);
  ret = chdir(".piratepack");

  ret = system("echo theme >> .installed 2>> logs/piratepack_install.log");

  strcpy(str,maindir);
  strcat(str,"/share");
  ret = chdir(str);

  strcpy(str,"./addpath.sh '");
  strcat(str,basedir);
  strcat(str,"' '");
  strcat(str,maindir);
  strcat(str,"' ");
  strcat(str,logpipe);
  ret = system(str);

  //complete installation

  ret = chdir(homedir);

  if (!g_file_test(".piratepack",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir .piratepack >> /dev/null 2>> /dev/null");
  }

  ret = chdir(".piratepack");

  if (!g_file_test("logs",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir logs >> /dev/null 2>> /dev/null");
  }

  ret = chdir(homedir);

  if (g_file_test(".piratepack/.removed",G_FILE_TEST_IS_REGULAR)) {
    strcpy (str,"chmod u+rw .piratepack/.removed ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm .piratepack/.removed ");
    strcat (str,logpipe);
    ret = system(str);
  }

  if (g_file_test(".piratepack/.disable",G_FILE_TEST_IS_REGULAR)) {
    strcpy (str,"chmod u+rw .piratepack/.disable ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm .piratepack/.disable ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy(str,"Enabled");
  fprintf( stderr, "%s\n", str );
  fflush(stderr);

  g_free( curpath );
  g_free( logpipe );
  g_free( str );

  return( 0 );
}

int
reinstall_pack(int argc, char **argv, Data * data)
{

  int ret;

  gchar * processpath=data->processpath;
  gchar * basedir=data->basedir;
  gchar * maindir=data->maindir;

  gchar * curpath = g_get_current_dir();
  gchar * homedir = data->homedir;

  gchar * str = g_malloc(2*strlen(homedir)+strlen(curpath)+strlen(processpath)+strlen(basedir)+strlen(maindir)+200);

  //start setup                                                                                                                                                                                                                            
  //strcpy(str,"Updating");
  //fprintf( stderr, "%s\n", str );

  gchar * logpipe = g_malloc(2*strlen(homedir)+200);
  
  ret = chdir(homedir);

  if (!g_file_test(".piratepack",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir .piratepack >> /dev/null 2>> /dev/null");
  }

  ret = chdir(".piratepack");

  if (!g_file_test("logs",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir logs >> /dev/null 2>> /dev/null");
  }

  ret = chdir(homedir);

  strcpy (logpipe,">> /dev/null 2>> ");
  strcat (logpipe,homedir);
  strcat (logpipe,"/.piratepack/logs/piratepack_remove.log");

  strcpy(str,"echo \"[$(date)]\" >> ");
  strcat(str,homedir);
  strcat(str,"/.piratepack/logs/piratepack_remove.log");
  ret = system(str);

  if (g_file_test(".piratepack",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod u+r .piratepack ");
    strcat (str,logpipe);
    ret = system(str);

    ret = chdir(".piratepack");

    gchar line[200];
    FILE *fp;
    fp = fopen(".installed", "r");
    if(!fp) return 1;

    while((!feof(fp)) && (fgets(line,200,fp) != 0)) {

      gint len = strlen(line)-1;
      if(line[len] == '\n') 
	line[len] = 0;
      
      gchar * linesub = substring(line,0,8);
      if ((strlen(line)>=8) && (strcmp(linesub,"Version:")==0)) {
	g_free(linesub);
	continue;
      }
      if (linesub != 0) {
	g_free(linesub);
      }

      if (g_file_test(line,G_FILE_TEST_IS_DIR)) {

        //strcpy (str,line);
	//fprintf( stderr, "%s\n", str );
	
	strcpy(str,maindir);
        strcat(str,"/share/");
        strcat(str,line);
        ret = chdir(str);

        strcpy (str,"./remove_");
        strcat (str,line);
        strcat (str,".sh ");
        strcat (str,logpipe);
        ret = system(str);

	ret = chdir(homedir);
        ret = chdir(".piratepack");

	if (g_file_test(line,G_FILE_TEST_IS_DIR)) {
	
	  strcpy (str,"chmod u+rw ");
	  strcat (str,line);
	  strcat (str," ");
	  strcat (str,logpipe);
	  ret = system(str);

	  strcpy (str,"rm -rf ");
	  strcat (str,line);
	  strcat (str," ");
	  strcat (str,logpipe);
	  ret = system(str);
	}
      }
    }
    fclose(fp);
    ret = chdir(homedir);
  }

  //complete removal

  strcpy(str,maindir);
  strcat(str,"/share");
  ret = chdir(str);

  strcpy(str,"./removepath.sh '");
  strcat(str,basedir);
  strcat(str,"' '");
  strcat(str,maindir);
  strcat(str,"' ");
  strcat(str,logpipe);
  ret = system(str);

  ret = chdir(homedir);

  if (!g_file_test(".piratepack",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir .piratepack >> /dev/null 2>> /dev/null");
  }

  ret = chdir(".piratepack");

  if (!g_file_test("logs",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir logs >> /dev/null 2>> /dev/null");
  }

  ret = chdir(homedir);

  strcpy (str,"touch .piratepack/.removed ");
  strcat (str,logpipe);
  ret = system(str);

  if (g_file_test(".piratepack/.installed",G_FILE_TEST_IS_REGULAR)) {
    strcpy (str,"chmod u+rw .piratepack/.installed ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm .piratepack/.installed ");
    strcat (str,logpipe);
    ret = system(str);
  }

  //Start Install

  ret = chdir(homedir);

  if (!g_file_test(".piratepack",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir .piratepack >> /dev/null 2>> /dev/null");
  }

  ret = chdir(".piratepack");

  if (!g_file_test("logs",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir logs >> /dev/null 2>> /dev/null");
  }

  ret = chdir(homedir);

  strcpy (logpipe,">> /dev/null 2>> ");
  strcat (logpipe,homedir);
  strcat (logpipe,"/.piratepack/logs/piratepack_install.log");

  strcpy(str,"echo \"[$(date)]\" >> ");
  strcat(str,homedir);
  strcat(str,"/.piratepack/logs/piratepack_install.log");
  ret = system(str);

  strcpy (str,"chmod u+r .piratepack ");
  strcat (str,logpipe);
  ret = system(str);

  ret = chdir(".piratepack");

  if (g_file_test(".installed",G_FILE_TEST_IS_REGULAR)) {
    strcpy (str,"chmod -R u+rw .installed ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf .installed ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy (str,"touch .installed ");
  strcat (str,logpipe);
  ret = system(str);

  gchar * processpathsub = substring(processpath,strlen(basedir)+1,strlen(processpath)-strlen(basedir)-1);
  strcpy (str,"echo Path: ");
  strcat (str,processpathsub);
  strcat (str," >> .installed 2>> logs/piratepack_install.log");
  ret = system(str); 
  g_free(processpathsub);

  //install firefox-mods

  //strcpy(str,"firefox-mods");
  //fprintf( stderr, "%s\n", str );

  if (g_file_test("firefox-mods",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod -R u+rw firefox-mods ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf firefox-mods ");
    strcat (str,logpipe);
    ret = system(str);
  }
  
  strcpy (str,"mkdir firefox-mods ");
  strcat (str,logpipe);
  ret = system(str);
 
  strcpy(str,maindir);
  strcat(str,"/share/firefox-mods");

  ret = chdir(str);

  strcpy (str,"./install_firefox-mods.sh ");
  strcat (str,logpipe);
  ret = system(str);

  strcpy(str,maindir);
  strcat(str,"/bin/polipo >> /dev/null 2>> /dev/null &");
  ret = system(str);

  ret = chdir(homedir);
  ret = chdir(".piratepack");

  ret = system("echo firefox-mods >> .installed 2>> logs/piratepack_install.log");

  //install tor-browser

  //strcpy(str,"tor-browser");
  //fprintf( stderr, "%s\n", str );

  if (g_file_test("tor-browser",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod -R u+rw tor-browser ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf tor-browser ");
    strcat (str,logpipe);
    ret = system(str);
  }
  
  strcpy (str,"mkdir tor-browser ");
  strcat (str,logpipe);
  ret = system(str);

  strcpy(str,maindir);
  strcat(str,"/share/tor-browser");

  ret = chdir(str);
  
  strcpy (str,"./install_tor-browser.sh ");
  strcat (str,logpipe);
  ret = system(str);

  ret = chdir(homedir);
  ret = chdir(".piratepack");

  ret = system("echo tor-browser >> .installed 2>> logs/piratepack_install.log");

  //install i2p-browser

  if (g_file_test("i2p-browser",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod -R u+rw i2p-browser ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf i2p-browser ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy (str,"mkdir i2p-browser ");
  strcat (str,logpipe);
  ret = system(str);

  strcpy(str,maindir);
  strcat(str,"/share/i2p-browser");

  ret = chdir(str);

  strcpy (str,"./install_i2p-browser.sh ");
  strcat (str,logpipe);
  ret = system(str);

  ret = chdir(homedir);
  ret = chdir(".piratepack");

  ret = system("echo i2p-browser >> .installed 2>> logs/piratepack_install.log");

  //install file-manager

  //strcpy(str,"file-manager");
  //fprintf( stderr, "%s\n", str );

  if (g_file_test("file-manager",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod -R u+rw file-manager ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf file-manager ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy (str,"mkdir file-manager ");
  strcat (str,logpipe);
  ret = system(str);

  strcpy(str,maindir);
  strcat(str,"/share/file-manager");

  ret = chdir(str);

  strcpy (str,"./install_file-manager.sh ");
  strcat (str,logpipe);
  ret = system(str);

  ret = chdir(homedir);
  ret = chdir(".piratepack");

  ret = system("echo file-manager >> .installed 2>> logs/piratepack_install.log");

  //install ppcavpn

  //strcpy(str,"ppcavpn");
  //fprintf( stderr, "%s\n", str );

  if (g_file_test("ppcavpn",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod -R u+rw ppcavpn ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf ppcavpn ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy (str,"mkdir ppcavpn ");
  strcat (str,logpipe);
  ret = system(str);

  strcpy(str,maindir);
  strcat(str,"/share/ppcavpn");

  ret = chdir(str);

  strcpy (str,"./install_ppcavpn.sh ");
  strcat (str,logpipe);
  ret = system(str);

  ret = chdir(homedir);
  ret = chdir(".piratepack");

  ret = system("echo ppcavpn >> .installed 2>> logs/piratepack_install.log");

//install bitcoin

  //strcpy(str,"bitcoin");
  //fprintf( stderr, "%s\n", str );

  if (g_file_test("bitcoin",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod -R u+rw bitcoin ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf bitcoin ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy (str,"mkdir bitcoin ");
  strcat (str,logpipe);
  ret = system(str);

  strcpy(str,maindir);
  strcat(str,"/share/bitcoin");

  ret = chdir(str);

  strcpy (str,"./install_bitcoin.sh ");
  strcat (str,logpipe);
  ret = system(str);

  ret = chdir(homedir);
  ret = chdir(".piratepack");

  ret = system("echo bitcoin >> .installed 2>> logs/piratepack_install.log");

  //install theme

  //strcpy(str,"theme");
  //fprintf( stderr, "%s\n", str );

  if (g_file_test("theme",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod -R u+rw theme ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm -rf theme ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy (str,"mkdir theme ");
  strcat (str,logpipe);
  ret = system(str);

  //cb_execute_refresh_theme(0,data);

  ret = chdir(homedir);
  ret = chdir(".piratepack");

  ret = system("echo theme >> .installed 2>> logs/piratepack_install.log");

  strcpy(str,maindir);
  strcat(str,"/share");
  ret = chdir(str);

  strcpy(str,"./addpath.sh '");
  strcat(str,basedir);
  strcat(str,"' '");
  strcat(str,maindir);
  strcat(str,"' ");
  strcat(str,logpipe);
  ret = system(str);
  
  //complete installation

  ret = chdir(homedir);

  if (!g_file_test(".piratepack",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir .piratepack >> /dev/null 2>> /dev/null");
  }

  ret = chdir(".piratepack");

  if (!g_file_test("logs",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir logs >> /dev/null 2>> /dev/null");
  }

  ret = chdir(homedir);

  if (g_file_test(".piratepack/.removed",G_FILE_TEST_IS_REGULAR)) {
    strcpy (str,"chmod u+rw .piratepack/.removed ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm .piratepack/.removed ");
    strcat (str,logpipe);
    ret = system(str);
  }

  if (g_file_test(".piratepack/.disable",G_FILE_TEST_IS_REGULAR)) {
    strcpy (str,"chmod u+rw .piratepack/.disable ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm .piratepack/.disable ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy(str,"Updated");
  fprintf( stderr, "%s\n", str );
  fflush(stderr);

  g_free( curpath );
  g_free( logpipe );
  g_free( str );

  return( 0 );
}

int update_pack (int argc, char ** argv, Data * data) {

  //For now, just reinstall
  return reinstall_pack(argc,argv,data);

}

int remove_pack(int argc, char **argv, Data * data) {

  int ret;

  gchar * basedir=data->basedir;
  gchar * maindir=data->maindir;

  gchar *curpath = g_get_current_dir();
  gchar *homedir = data->homedir;
  gchar *str = g_malloc(2*strlen(homedir)+strlen(curpath)+strlen(basedir)+strlen(maindir)+300);

  //start removal

  //strcpy(str,"Disabling");
  //fprintf( stderr, "%s\n", str );

  gchar *logpipe = g_malloc(2*strlen(homedir)+200);

  ret = chdir(homedir);

  if (!g_file_test(".piratepack",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir .piratepack >> /dev/null 2>> /dev/null");
  }

  ret = chdir(".piratepack");

  if (!g_file_test("logs",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir logs >> /dev/null 2>> /dev/null");
  }

  ret = chdir(homedir);

  strcpy (logpipe,">> /dev/null 2>> ");
  strcat (logpipe,homedir);
  strcat (logpipe,"/.piratepack/logs/piratepack_remove.log");

  strcpy(str,"echo \"[$(date)]\" >> ");
  strcat(str,homedir);
  strcat(str,"/.piratepack/logs/piratepack_remove.log");
  ret = system(str);

  if (g_file_test(".piratepack",G_FILE_TEST_IS_DIR)) {

    strcpy (str,"chmod u+r .piratepack ");
    strcat (str,logpipe);
    ret = system(str);

    ret = chdir(".piratepack");

    gchar line[200];
    FILE *fp;
    fp = fopen(".installed", "r");
    if(!fp) return 1;

    while( (!feof(fp)) && (fgets(line,200,fp) != 0) ) {

      gint len = strlen(line)-1;
      if(line[len] == '\n') 
	line[len] = 0;

      gchar * linesub = substring(line,0,8);
      if ((strlen(line)>=8) && (strcmp(linesub,"Version:")==0)) {
	g_free(linesub);
	continue;
      }
      if (linesub != 0) {
	g_free(linesub);
      }

      if (g_file_test(line,G_FILE_TEST_IS_DIR)) {
	
	//strcpy (str,line);
	//fprintf( stderr, "%s\n", str );
	
	strcpy(str,maindir);
	strcat(str,"/share/");
	strcat(str,line);
	ret = chdir(str);

	strcpy (str,"./remove_");
	strcat (str,line);
	strcat (str,".sh ");
	strcat (str,logpipe);
	ret = system(str);

	ret = chdir(homedir);
	ret = chdir(".piratepack");
	
	if (g_file_test(line,G_FILE_TEST_IS_DIR)) {
	  
	  strcpy (str,"chmod u+rw ");
	  strcat (str,line);
	  strcat (str," ");
	  strcat (str,logpipe);
	  ret = system(str);
	  
	  strcpy (str,"rm -rf ");
	  strcat (str,line);
	  strcat (str," ");
	  strcat (str,logpipe);
	  ret = system(str);
	}
      }
    }
    fclose(fp);
    ret = chdir(homedir);
  }
  
  //complete removal
  
  strcpy(str,maindir);
  strcat(str,"/share");
  ret = chdir(str);

  strcpy(str,"./removepath.sh '");
  strcat(str,basedir);
  strcat(str,"' '");
  strcat(str,maindir);
  strcat(str,"' ");
  strcat(str,logpipe);
  ret = system(str);

  ret = chdir(homedir);

  if (!g_file_test(".piratepack",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir .piratepack >> /dev/null 2>> /dev/null");
  }

  ret = chdir(".piratepack");

  if (!g_file_test("logs",G_FILE_TEST_IS_DIR)) {
    ret = system("mkdir logs >> /dev/null 2>> /dev/null");
  }

  ret = chdir(homedir);

  strcpy (str,"touch .piratepack/.removed ");
  strcat (str,logpipe);
  ret = system(str);

  if (g_file_test(".piratepack/.installed",G_FILE_TEST_IS_REGULAR)) {
    strcpy (str,"chmod u+rw .piratepack/.installed ");
    strcat (str,logpipe);
    ret = system(str);

    strcpy (str,"rm .piratepack/.installed ");
    strcat (str,logpipe);
    ret = system(str);
  }

  strcpy(str,"Disabled");
  fprintf( stderr, "%s\n", str );
  fflush(stderr);

  g_free( curpath );
  g_free( logpipe );
  g_free( str );

  return( 0 );
}

int open_pirate_file(int argc, char **argv, Data * data) {

  int ret;

  gchar * maindir = data->maindir;

  gchar * file = strdup(argv[1]);
  gchar * curpath = g_get_current_dir();
  gchar * homedir = data->homedir;
  gchar * str = g_malloc(2*strlen(homedir)+2*strlen(file)+strlen(maindir)+200);
  gchar * logpipe = g_malloc(2*strlen(homedir)+200);

  ret = chdir(homedir);

  if (!g_file_test(".piratepack/.installed",G_FILE_TEST_IS_REGULAR)) {
    return ( 0 );
  }

  strcpy (logpipe,">> /dev/null 2>> ");
  strcat (logpipe,homedir);
  strcat (logpipe,"/.piratepack/logs/piratepack_open.log");

  ret = chdir(".piratepack");

  strcpy(str,"echo \"[$(date)]\" >> ");
  strcat(str,homedir);
  strcat(str,"/.piratepack/logs/piratepack_open.log");
  ret = system(str);

  if (!g_file_test("tmp",G_FILE_TEST_IS_DIR)) {
    strcpy(str,"mkdir tmp ");
    strcat(str,logpipe);
    ret = system(str);

    strcpy(str,"chmod u+rwx tmp ");
    strcat(str,logpipe);
    ret = system(str);
  }

  strcpy(str,"rm -rf tmp/* ");
  strcat(str,logpipe);
  ret = system(str);

  ret = chdir(curpath);

  strcpy(str,"cp ");
  strcat(str,file);
  strcat(str," ~/.piratepack/tmp ");
  strcat(str,logpipe);
  ret = system(str);

  ret = chdir(homedir);
  ret = chdir(".piratepack/tmp");

  strcpy(str, maindir);
  strcat(str,"/share/file-manager/get_file_info.sh $(find *.pirate)");
  gchar * result = exec(str,200);

  if (strlen(result) < 4) {
    strcpy(result,"err:Error");
  }

  ret = chdir("../");

  if (g_file_test("tmp",G_FILE_TEST_IS_DIR)) {
    strcpy(str,"chmod u+rwx tmp ");
    strcat(str,logpipe);
    ret = system(str);

    strcpy(str,"rm -rf tmp/* ");
    strcat(str,logpipe);
    ret = system(str);
  }
  
  GtkWindow * window;
  GtkTable * table1, * table2;
  GtkButton * button_enable, * button_yes, * button_no;
  GtkProgressBar * progress;
  GtkLabel * message;
  GtkImage * logo;
  GtkHButtonBox * hbuttonbox;

  gtk_init( &argc, &argv );

  data->file = file;
  g_free(data->curpath);
  data->curpath = curpath;
  
  window = (GtkWindow *)gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size( GTK_WINDOW( window ), 500, 400 );
  gtk_window_set_title( GTK_WINDOW( window ), "Pirate Pack");
  strcpy(str,maindir);
  strcat(str,"/share/graphics/logo.png");
  gtk_window_set_icon_from_file( GTK_WINDOW( window ), str, 0);
  g_signal_connect( G_OBJECT( window ), "destroy", G_CALLBACK( gtk_main_quit ), 0 );
  table1 = (GtkTable *)gtk_table_new( 4, 3, FALSE );
  gtk_container_add( GTK_CONTAINER( window ), (GtkWidget *)table1 );
  table2 = (GtkTable *)gtk_table_new( 3, 3, FALSE );
  gtk_table_attach( GTK_TABLE( table1 ), (GtkWidget *)table2, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0 );
  strcpy(str,maindir);
  strcat(str,"/share/graphics/logo.png");
  logo = (GtkImage *)gtk_image_new_from_file(str);
  gtk_table_attach( GTK_TABLE( table2 ), (GtkWidget *)logo, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5 );
  gchar *targetname = substring(result,4,strlen(result)-5);
  data->targetname = targetname;
  
  gint error = 0;
  
  gchar * resultsub = substring(result,0,4);
  if ((strlen(result)>=4) && (strcmp(resultsub,"out:")==0)) {
    strcpy (str,"Install ");
    if (strcmp(targetname,"ppcavpn")==0) {
      strcat (str,"PPCA VPN");
    }
    strcat (str,"?");
  }
  else if ((strlen(result)>=4)&&(strcmp(resultsub,"err:")==0)) {
    gchar * resultsub2 = substring(result,4,strlen(result)-4);
    strcpy (str,resultsub2);
    error = 1;
    g_free(resultsub2);
  }
  else {
    strcpy (str,"Error");
    error = 1;
  }
  if (resultsub != 0) {
    g_free(resultsub);
  }

  message = (GtkLabel *)gtk_label_new(str);
  gtk_label_set_justify(message,GTK_JUSTIFY_CENTER);
  gtk_widget_set_size_request((GtkWidget *)message,-1,80);
  gtk_misc_set_alignment((GtkMisc *)message,0.5,1);
  gtk_table_attach( GTK_TABLE( table1 ), (GtkWidget *)message, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5 );
  progress = (GtkProgressBar *)gtk_progress_bar_new();
  gtk_table_attach( GTK_TABLE( table1 ),(GtkWidget *)progress, 1, 2, 2, 3, GTK_FILL, GTK_SHRINK | GTK_FILL, 5, 0 );
  strcpy (str,homedir);
  strcat (str,"/.piratepack/.installed");
  hbuttonbox = (GtkHButtonBox *)gtk_hbutton_box_new();
  
  if (error == 1) {
    button_enable = (GtkButton *)gtk_button_new_with_label( "Exit" );
    gtk_widget_set_size_request((GtkWidget *)button_enable,120,-1);
    g_signal_connect( G_OBJECT( button_enable ), "clicked",
		      G_CALLBACK( gtk_main_quit ), data );
    gtk_container_add((GtkContainer *)hbuttonbox,(GtkWidget *)button_enable);
    data->button_enable = GTK_BUTTON( button_enable );
  }
  
  else if (g_file_test(str,G_FILE_TEST_IS_REGULAR)) {
    button_yes = (GtkButton *)gtk_button_new_with_label( "Yes" );
    gtk_widget_set_size_request((GtkWidget *)button_yes,120,-1);
    g_signal_connect( G_OBJECT( button_yes ), "clicked",
		      G_CALLBACK( cb_execute_install_pirate_file ), data );
    button_no = (GtkButton *)gtk_button_new_with_label( "No" );
    gtk_widget_set_size_request((GtkWidget *)button_no,120,-1);
    g_signal_connect( G_OBJECT( button_no ), "clicked",
		      G_CALLBACK( gtk_main_quit ), data );
    gtk_container_add((GtkContainer *)hbuttonbox,(GtkWidget *)button_yes);
    gtk_container_add((GtkContainer *)hbuttonbox,(GtkWidget *)button_no);
    data->button_yes = button_yes;
    data->button_no = button_no;
  }
  else {
    gtk_label_set_label(message, "Enable Pirate Pack to install this file");
    button_enable = (GtkButton *)gtk_button_new_with_label( "Enable" );
    gtk_widget_set_size_request((GtkWidget *)button_enable,120,-1);
    g_signal_connect( G_OBJECT( button_enable ), "clicked", G_CALLBACK( cb_execute_install ), data );
    gtk_container_add((GtkContainer *)hbuttonbox,(GtkWidget *)button_enable);
    data->button_enable = button_enable;
  }
  
  gtk_table_attach( GTK_TABLE( table1 ), (GtkWidget *)hbuttonbox, 1, 2, 3, 4, 0, 0, 5, 5 );
  
  data->message = message;
  data->progress = progress;
  data->hbuttonbox = hbuttonbox;
  data->window = window;
  
  g_free(logpipe);
  g_free(str);
  g_free(result);
  
  gtk_widget_show_all((GtkWidget *)window);
  
  gtk_main();
  
  return( 0 );
  
}

int install_pirate_file(int argc, char **argv, Data * data) {

  if (argc < 5) {
    return( 0 );
  }

  int ret;

  gchar * maindir = data->maindir;

  gchar *callingdir = argv[2];
  gchar *file = argv[3];
  gchar *targetname = strdup(argv[4]);

  gchar *curpath = g_get_current_dir();
  g_free(data->curpath);
  data->curpath = curpath;

  gchar *homedir = data->homedir;

  gchar *str = g_malloc(2*strlen(homedir)+2*strlen(file)+2*strlen(targetname)+strlen(maindir)+300);

  gchar *logpipe = g_malloc(2*strlen(homedir)+200);

  ret = chdir(homedir);

  if (!g_file_test(".piratepack/.installed",G_FILE_TEST_IS_REGULAR)) {
    return ( 0 );
  }

  strcpy (logpipe,">> /dev/null 2>> ");
  strcat (logpipe,homedir);
  strcat (logpipe,"/.piratepack/logs/piratepack_open.log");

  ret = chdir(".piratepack");

  strcpy(str,"echo \"[$(date)]\" >> ");
  strcat(str,homedir);
  strcat(str,"/.piratepack/logs/piratepack_open.log");
  ret = system(str);

  if (!g_file_test("tmp",G_FILE_TEST_IS_DIR)) {
    strcpy(str,"mkdir tmp ");
    strcat(str,logpipe);
    ret = system(str);

    strcpy(str,"chmod u+rwx tmp ");
    strcat(str,logpipe);
    ret = system(str);
  }

  strcpy(str,"rm -rf tmp/* ");
  strcat(str,logpipe);
  ret = system(str);

  ret = chdir(callingdir);

  strcpy(str,"cp ");
  strcat(str,file);
  strcat(str," ~/.piratepack/tmp ");
  strcat(str,logpipe);
  ret = system(str);

  ret = chdir(homedir);
  ret = chdir(".piratepack/tmp");

  strcpy(str,maindir);
  strcat(str,"/share/file-manager/verify_file.sh $(find *.pirate)");
  strcat(str," ");
  strcat(str, targetname);  
  gchar * result = exec(str,200);

  if (strlen(result)>=4) {
    gchar * resultsub = substring(result,0,4);
    if (strcmp(resultsub,"out:")==0) {
      
      strcpy(str,maindir);
      strcat(str,"/share/file-manager");
      ret = chdir(str);
      
      strcpy(str,"./install_file.sh ");
      strcat(str, targetname);
      strcat(str, " ");
      strcat(str, homedir);
      strcat(str, "/.piratepack/tmp");
      strcat(str, " ");
      strcat(str,logpipe);
      ret = system(str);
      
      //end setup                                                                                                                                                                                                                        
      strcpy(str,"Installed");
      fprintf( stderr, "%s\n", str );
    }
    else if (strcmp(resultsub,"err:")==0) {
      strcpy(str,"File not authentic");
      fprintf( stderr, "%s\n", str );
    }
    g_free(resultsub);
  }
  else {
    strcpy(str,"Error");
    fprintf( stderr, "%s\n", str );
  }
  fflush(stderr);

  strcpy(str,homedir);
  strcat(str,"/.piratepack");
  ret = chdir(str);

  strcpy(str,"rm -rf tmp/* ");
  strcat(str,logpipe);
  ret = system(str);

  g_free(logpipe);
  g_free(str);
  g_free(result);

  return( 0 );
}

int gui_status(int argc, char ** argv, Data * data) {

  GtkWindow * window;
  GtkTable * table1, * table2;
  GtkButton * button, * button_disable, * button_enable, * button_update;
  GtkProgressBar * progress;
  GtkLabel * message;
  GtkImage * logo;
  GtkHButtonBox * hbuttonbox;

  gtk_init( &argc, &argv );

  gchar * maindir = data->maindir;

  gchar * homedir = data->homedir;

  gchar * curpath = g_get_current_dir();
  g_free(data->curpath);
  data->curpath = curpath;

  gchar * str = g_malloc(strlen(homedir) + strlen(curpath) + strlen(maindir) + 300);

  window = (GtkWindow *)gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size( GTK_WINDOW( window ), 500, 400 );
  gtk_window_set_title( GTK_WINDOW( window ), "Pirate Pack");

  strcpy(str,maindir);
  strcat(str,"/share/graphics/logo.png");

  gtk_window_set_icon_from_file( GTK_WINDOW( window ), str, 0);
  
  g_signal_connect( G_OBJECT( window ), "destroy", G_CALLBACK( gtk_main_quit ), 0 );
  
  table1 = (GtkTable *)gtk_table_new( 4, 3, FALSE );
  gtk_container_add((GtkContainer *)window, (GtkWidget *)table1);
  
  table2 = (GtkTable *)gtk_table_new( 3, 3, FALSE );
  gtk_table_attach( GTK_TABLE( table1 ), (GtkWidget *)table2, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0 );
  logo = (GtkImage *)gtk_image_new_from_file(str);
  gtk_table_attach( GTK_TABLE( table2 ), (GtkWidget *)logo, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5 );
  message = (GtkLabel *)gtk_label_new("The Pirate Pack enhances your digital freedom\nLearn more at piratelinux.org");
  gtk_label_set_justify((GtkLabel *)message,GTK_JUSTIFY_CENTER);
  gtk_widget_set_size_request((GtkWidget *)message,-1,80);
  gtk_misc_set_alignment((GtkMisc *)message,0.5,1);
  gtk_table_attach( GTK_TABLE( table1 ), (GtkWidget *)message, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5 );
  progress = (GtkProgressBar *)gtk_progress_bar_new();
  gtk_table_attach( GTK_TABLE( table1 ),(GtkWidget *)progress, 1, 2, 2, 3, GTK_FILL, GTK_SHRINK | GTK_FILL, 5, 0 );
  strcpy (str,homedir);
  strcat (str,"/.piratepack/.installed");
  hbuttonbox = (GtkHButtonBox *)gtk_hbutton_box_new();
  
  if (g_file_test(str,G_FILE_TEST_IS_REGULAR)) {
    button_disable = (GtkButton *)gtk_button_new_with_label( "Disable" );
    gtk_widget_set_size_request((GtkWidget *)button_disable,120,-1);
    g_signal_connect( G_OBJECT( button_disable ), "clicked", G_CALLBACK( cb_execute_remove ), data );
    gtk_container_add((GtkContainer *)hbuttonbox,(GtkWidget *)button_disable);
    data->button_disable = button_disable;
  }
  else {
    button_enable = (GtkButton *)gtk_button_new_with_label( "Enable" );
    gtk_widget_set_size_request((GtkWidget *)button_enable,120,-1);
    g_signal_connect( G_OBJECT( button_enable ), "clicked", G_CALLBACK( cb_execute_install ), data );
    gtk_container_add((GtkContainer *)hbuttonbox,(GtkWidget *)button_enable);
    data->button_enable = button_enable;
  }
  
  gtk_table_attach( GTK_TABLE( table1 ), (GtkWidget *)hbuttonbox, 1, 2, 3, 4, 0, 0, 5, 5 );
  data->message = message;
  data->progress = progress;
  data->hbuttonbox = hbuttonbox;
  data->window = window;
  
  g_free(str);

  gtk_widget_show_all((GtkWidget *)window);

  gtk_main();
}
 
int
main( int argc, char ** argv ) {

  int ret;
  gchar * curpath = g_get_current_dir();

  gchar * procpath = g_malloc(32);
  gint pid = getpid();
  sprintf(procpath, "/proc/%d/exe", pid);

  gchar * processpath = (gchar *) get_readlink(procpath);

  g_free(procpath);

  if ((processpath == 0) || (strlen(processpath) == 0)) {
    g_free(curpath);
    return(0);
  }
  
  gchar * processpathdir = get_dirname(processpath);

  ret = chdir(processpathdir);
  ret = chdir("..");
  
  gchar * maindir = g_get_current_dir();

  ret = chdir("..");

  gchar * basedir = g_get_current_dir();
  
  gchar * homedir = strdup(getenv("HOME"));
  
  gchar * str = g_malloc(strlen(homedir)+strlen(processpath)+350);

  if (getargi("--async",argc,argv) != -1) {
    gchar * strin = g_malloc(100);
    while ((strin == 0) || (strcmp(strin,"ready") != 0)) {
      ret = scanf("%s",strin);
    }
    g_free(strin);
  }

  gchar refreshtor = 0;
  if (argc == 2) {
    if (strcmp(argv[1],"--refresh-tor")==0) {
      refreshtor = '1';
    }
  }
  gchar refreshtheme = 0;
  gchar * themeoption = "none";
  if (argc > 1) {
    if (strcmp(argv[1],"--refresh-theme")==0) {
      refreshtheme = '1';
      if (argc == 3) {
	themeoption = argv[2];
      }
    }
  }

  strcpy(str,maindir);
  if (refreshtor == '1') {
    strcat(str,"/.refreshtor.lock");
  }
  else if (refreshtheme == '1') {
    strcat(str,"/.refreshtheme.lock");
  }
  else {
    strcat(str,"/.lock");
  }

  if (g_file_test(str,G_FILE_TEST_IS_REGULAR)) {
    g_free(processpath);
    g_free(processpathdir);
    g_free(basedir);
    g_free(maindir);
    g_free(curpath);
    g_free(str);
    return(0);
  }

  int pid_len = numDigits(pid);

  gchar * pid_str = g_malloc(pid_len+1);
  sprintf(pid_str,"%d",pid);

  gchar locked = 1;

  strcpy(str,homedir);
  strcat(str,"/.piratepack");

  if (!g_file_test(str,G_FILE_TEST_IS_DIR)) {

    if (g_file_test(str,G_FILE_TEST_EXISTS)) {
      
      ret = remove(str);

    }
    
    ret = mkdir(str, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  }

  if (refreshtor == '1') {
    strcat(str,"/.refreshtor.lock");
  }
  else if (refreshtheme == '1') {
    strcat(str,"/.refreshtheme.lock");
  }
  else {
    strcat(str,"/.lock");
  }
  int pfd = open(str, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (pfd == -1) {

    strcat(str,"-");
    strcat(str,pid_str);

    if (g_file_test(str,G_FILE_TEST_IS_REGULAR)) {

      locked = 0;

    }

    else {

      strcpy(str,homedir);
      if (refreshtor == '1') {
	strcat(str,"/.piratepack/.refreshtor.lock");
      }
      else if (refreshtheme == '1') {
        strcat(str,"/.piratepack/.refreshtheme.lock");
      }
      else {
	strcat(str,"/.piratepack/.lock");
      }
      struct stat st;
      ret = stat (str, &st);
      time_t locktime = st.st_mtime;
      time_t curtime = time(0);

      //check time of file and make sure its more than a second before current time
      if ((curtime - locktime) < 1) {
	sleep(1);
      }

      FILE * f = fopen (str , "r");
      gint size = 20;
      gchar * line = g_malloc(20);
      strcpy(line,"");
      
      if (f == 0) {

	fprintf(stderr,"Error opening file\n");

      }

      else {

	while (!feof(f)) {

	  if(fgets(line, size, f) != 0) {

	    gchar * endchar = line + strlen(line) - 1;

	    if (*endchar == '\n') {

	      *endchar = '\0';
	      break;

	    }

	  }

	}

	fclose(f);

      }

      gchar * pids = exec("pidof piratepack",100);
      gchar * rest;
      gchar * tok;
      gchar * ptr = pids;
      gchar match = 0;

      while(tok = strtok_r(ptr, " \n", &rest)) {

	if (strcmp(tok,line) == 0) {

	  match = 1;
	  break;

	}

	ptr = rest;

      }

      g_free(pids);
      g_free(line);

      if (match == 0) {

	ret = stat (str, &st);
	time_t locktime2 = st.st_mtime;
	
	if (locktime2 == locktime) {

	  gchar * cmd = g_malloc(strlen(str)+50);
	  strcpy(cmd,"rm -f ");
	  strcat(cmd,str);
	  strcat(cmd,"* >> /dev/null 2>> /dev/null");
	  ret = system(cmd);
	  g_free(cmd);
	  int pfd = open(str, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	  if (pfd != -1) {

	    locked = 0;
	    FILE * f = fopen(str,"w");

	    if (f != 0) {

	      fputs(pid_str,f);
	      fputs("\n",f);
	      fclose (f);

	    }

	    strcat(str,"-");
	    strcat(str,pid_str);
	    f = fopen(str,"w");

	    if (f != 0) {

	      fclose (f);

	    }

	  }

	}

      }

    }

  }                                                       

  else {

    locked = 0;
    FILE * f = fopen(str,"w");

    if (f != 0) {

      fputs(pid_str,f);
      fputs("\n",f);
      fclose (f);

    }

    strcat(str,"-");
    strcat(str,pid_str);
    f = fopen(str,"w");

    if (f != 0) {

      fclose (f);

    }

  }

  if (locked == 1) {

    printf("locked\n");
    g_free(processpath);
    g_free(processpathdir);
    g_free(basedir);
    g_free(maindir);
    g_free(curpath);
    g_free(str);
    return(0);

  }

  Data * data = g_malloc(sizeof(Data));
  data->processpath = processpath;
  data->basedir = basedir;
  data->maindir = maindir;
  data->curpath = curpath;
  data->homedir = homedir;
  data->file = 0;
  data->targetname = 0;
  data->action = 0;
  data->argv = argv;
  data->argc = argc;

  strcpy (str,homedir);
  strcat (str,"/.piratepack/.installed");

  if (refreshtor == '1') {
    if (g_file_test(str,G_FILE_TEST_IS_REGULAR)) {
      ret = refresh_tor(maindir,homedir);
    }
  }
  else if (refreshtheme == '1') {
    if (g_file_test(str,G_FILE_TEST_IS_REGULAR)) {
      ret = refresh_theme(maindir,homedir,themeoption);
    }
  }

  //If locally installed
  else if (g_file_test(str,G_FILE_TEST_IS_REGULAR)) {

    if (argc > 1) {
      if (strcmp(argv[1],"--remove")==0) {
	gchar * str2 = g_malloc(strlen(homedir)+100);
	strcpy (str2,"touch ");
	strcat (str2,homedir);
	strcat (str2,"/.piratepack/.disable");
	ret = system (str2);
	g_free(str2);
      }
    }
    
    //check what version was installed for the user
    gint size = 100;
    int vpsize = size+strlen(basedir)+10;
    gchar * versionpath = g_malloc(vpsize);
    strcpy(versionpath,basedir);
    strcat(versionpath,"/");
    FILE * file;
    gchar * line = g_malloc(size);
    file = fopen (str , "r");
    if (file == 0) {
      perror("Error opening file");
    }
    else {
      while (!feof(file)) {
	if(fgets(line, size, file) != 0) {
	  gchar * linesub = substring(line,0,6);
	  if ((strlen(line)>=6) && (strcmp(linesub,"Path: ")==0)) {
	      g_free(linesub);
	      linesub = substring(line,6,strlen(line)-6);
	      strcat(versionpath,linesub);
	      gchar* index = line + strlen(line) - 1;
	      if(*index != '\n') {
		while(!feof(file)) {
		  if(fgets(line, size, file) != 0) {
		    index = line + strlen(line) - 1;
		    if(*index != '\n') {
		      vpsize = vpsize + size;
		      versionpath = (gchar *) g_realloc (versionpath, vpsize);
		      strcat(versionpath,line);
		    }	  
		  }
		}
	      }
	      index = versionpath + strlen(versionpath) - 1;
	      if (*index == '\n') {
		*index = '\0';
	      }
	      g_free(linesub);
	      break;
	  }
	  if (linesub != 0) {
	    g_free(linesub);
	  }
	}
      }
    }
    fclose (file);
    g_free(line);

    data->versionpath = versionpath;

    //if versionpath differs from processpath
    if(strcmp(versionpath,processpath) != 0) {
      if (argc > 1) {
	if (strcmp(argv[1],"--refresh") == 0) {
          strcpy (str,homedir);
          strcat (str,"/.piratepack/.disable");
          if (g_file_test(str,G_FILE_TEST_IS_REGULAR)) {
            ret = remove_pack(argc,argv,data);
          }
	  else {
	    ret = update_pack(argc,argv,data);
	  }
        }
	else if (strcmp(argv[1],"--update") == 0) {
	  ret = update_pack(argc,argv,data);
	}
      }
      else if (g_file_test(versionpath,G_FILE_TEST_IS_REGULAR)) {
	gint cmdlen = strlen(versionpath);
	gint i=1;
	while (i < argc) {
	  cmdlen = cmdlen + strlen(argv[i]);
	  i=i+1;
	}
	cmdlen = cmdlen + 10;
	gchar * cmd = g_malloc(cmdlen);
	strcpy(cmd,versionpath);
	i=1;
	while (i < argc) {
	  strcat(cmd," ");
	  strcat(cmd,argv[i]);
	  i=i+1;
	}
	strcat(cmd," &");
	
	ret = system(cmd);
	g_free(cmd);
      }
      else {
	strcpy (str,homedir);
	strcat (str,"/.piratepack/.disable");
	if (g_file_test(str,G_FILE_TEST_IS_REGULAR)) {
	  ret = remove_pack(argc,argv,data);
	}
	else {
	  ret = update_pack(argc,argv,data);
	  main(argc,argv);
	}
      }
    }
    else { //versionpath same as processpath
      if (argc > 1) {
	gchar * argvsub = substring(argv[1],strlen(argv[1])-7,7);
	if (strcmp(argv[1],"--refresh")==0) {
	  strcpy (str,homedir);
	  strcat (str,"/.piratepack/.disable");
	  if (g_file_test(str,G_FILE_TEST_IS_REGULAR)) {
	    ret = remove_pack(argc,argv,data);
	  }
	  else {
	    strcpy(str,maindir);
	    strcat(str,"/bin/polipo >> /dev/null 2>> /dev/null &");
	    ret = system(str);
	  }
	}
	else if (strcmp(argv[1],"--reinstall")==0) {
	  ret = reinstall_pack(argc,argv,data);
	}
	else if (strcmp(argv[1],"--update")==0) {
	  ret = reinstall_pack(argc,argv,data);
	}
	else if (strcmp(argv[1],"--remove")==0) {
	  ret = remove_pack(argc,argv,data);
	}
	else if ((strlen(argv[1]) >= 7) && (strcmp(argvsub,".pirate")==0)) {
	  ret = chdir(curpath);
	  ret = open_pirate_file(argc,argv,data);
	}
	else if (strcmp(argv[1],"--install-pirate-file")==0) {
	  ret = install_pirate_file(argc,argv,data);
	}
	if (argvsub != 0) {
	  g_free(argvsub);
	}
      }
      else {
	ret = gui_status(argc,argv,data);
      }
    }
    g_free(data->versionpath);
  }
  
  //if not locally installed
  else {

    if (refreshtor == '1') {
      strcpy (str,homedir);
      strcat (str,"/.piratepack/.disable");
      if (!g_file_test(str,G_FILE_TEST_IS_REGULAR)) {
	ret = refresh_tor(maindir,homedir);
      }
    }
    else if (refreshtheme == '1') {
      strcpy (str,homedir);
      strcat (str,"/.piratepack/.disable");
      if (!g_file_test(str,G_FILE_TEST_IS_REGULAR)) {
        ret = refresh_theme(maindir,homedir,themeoption);
      }
    }

    if (argc > 1) {
      if (strcmp(argv[1],"--install")==0) {
	ret = install_pack(argc,argv,data);
      }
      else if (strcmp(argv[1],"--refresh")==0) {
	
	strcpy (str,homedir);
	strcat (str,"/.piratepack/.disable");

	if (!g_file_test(str,G_FILE_TEST_IS_REGULAR)) {
	  ret = install_pack(argc,argv,data);
	}

      }
    }
    else {
      ret = gui_status(argc,argv,data);
    }
  }

  gchar * lockprefix = 0;
  if (refreshtor == '1') {
    lockprefix = ".refreshtor";
  }
  else if (refreshtheme == '1') {
    lockprefix = ".refreshtheme";
  }
  else {
    lockprefix = "";
  }
  release_locks(str,pid_str,homedir,lockprefix);

  g_free(str);
  g_free(pid_str);
  g_free(processpathdir);

  g_free(data->processpath);
  g_free(data->basedir);
  g_free(data->maindir);
  g_free(data->curpath);
  g_free(data->homedir);
  g_free(data->file);
  g_free(data->targetname);
  g_free(data);

  return(0);

}

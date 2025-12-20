#include "str.h"
#include "webview/webview.h"
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define STR_IMPLEMENTATION
#include "str.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

// global state
webview_t window;
str url = {0};

// respond to eval
void print_response(bool success, str res) {
  if(res.len) {
    fprintf(stdout, "%c{\"success\": %s, \"data\": "STR_FMT"}%c\n",
            2, success ? "true" : "false",
            STR_ARG(res), 3);
  } else {
    fprintf(stdout, "%c{\"success\": %s}%c\n", 2, success ? "true" : "false", 3);
  }
}

void respond(const char *id, const char *result, void *user) {
  // result is JSONified array of [1 or 0,result]
  // we assume result is valid JSON and don't parse it
  str res = str_drop(str_constant((char*)result), 1);
  bool success = res.data[0] == '1';
  res = str_drop(res, 2); // skip "1," or "0,"
  res.len--; // skip the end ']' char
  print_response(success, res);
  webview_return(window, id, 0, "");
}

void console_log(const char *id, const char *result, void *user) {
  // logs are received as: ["level",{args}]
  str log = str_drop(str_constant((char*)result), 2); // skip ["
  str level, data;
  str_splitat(log, ",", &level, &data);
  level.len--; // don't include last '"'
  data.len--; // don't include last ']'
  fprintf(stdout, "%c{\"log\": \""STR_FMT"\", data: "STR_FMT"}%c\n",
          2, STR_ARG(level), STR_ARG(data), 3);
  webview_return(window, id, 0, "");
}

// commands
#define DO_COMMANDS(X)                                                         \
  X(go, "go <url>", "navigate to given URL")                                   \
  X(reload, "reload", "reload current URL")                                    \
  X(eval, "eval <js>", "evaluate given JS code")                               \
  X(click, "click <selector>", "query the given selector and click it")        \
  X(html, "html <selector>", "get the outer HTML of the given selector")       \
  X(console, "console",                                                        \
    "instrument console log, must be redone after navigation")                 \
  X(wsize, "wsize <width> <height>", "set browser window size")                \
  X(help, "help", "get some help")


struct sz {
  int w;
  int h;
};

void wsize_set(webview_t w, void *size) {
  struct sz *s = (struct sz*)size;
  webview_set_size(w, s->w, s->h, WEBVIEW_HINT_NONE);
}

void wsize(str args) {
  str w, h;
  if(str_splitat(args, " ", &w, &h)) {
    struct sz s = (struct sz){.w = str_to_long(w), .h = str_to_long(h)};
    if(!s.w || !s.h) goto fail;
    printf("w: %d, h: %d\n", s.w, s.h);
    webview_dispatch(window, wsize_set, &s);
    print_response(true, (str){0});
    return;
  }
 fail:
    print_response(false, str_constant("\"unparseable window size\""));

}

void console(str args) {
  webview_eval(window,
               "console.log = (function() { _drewl_log('log',arguments) });"
               "console.info = (function() { _drewl_log('info',arguments) });"
               "console.debug = (function() { _drewl_log('debug',arguments) });"
               "console.warn = (function() { _drewl_log('warn',arguments) });");
}

    
void go(str args) {
  str_free(&url);
  url = str_dup(args);
  char _url[2048];
  snprintf(_url, 2048, STR_FMT, STR_ARG(args));
  webview_navigate(window, _url);
}

void reload(str _args) { webview_eval(window, "window.location.reload()"); }

void _eval(str prefix, str item, str suffix) {
  char js[4096];
  snprintf(js, 4096, "try{_drewl_respond(1,"STR_FMT STR_FMT STR_FMT")}catch(e){_drewl_respond(0,e.toString())}", STR_ARG(prefix), STR_ARG(item), STR_ARG(suffix));
  webview_eval(window, js);
}

void eval(str args) {
  _eval((str){0}, args, (str){0});
}

void click(str args) {
  _eval(str_constant("document.querySelector(`"),
        args,
        str_constant("`).click()"));
}

void html(str args) {
  _eval(str_constant("document.querySelector(`"),
        args,
        str_constant("`).outerHTML"));
}

#define HELP_SIZE 8192
void help(str args) {
  char help[HELP_SIZE];
  int left = HELP_SIZE;
  left -= snprintf(help, left, "{\"commands\":[");
#define HELP(_fn, cmd, descr) left -= snprintf(&help[HELP_SIZE-left], left, "%s{\"usage\":       \"%s\",\n   \"description\": \"%s\"}", (first ? "\n  " : ",\n  "), cmd, descr); first = false;
  bool first = true;
  DO_COMMANDS(HELP);
#undef HELP
  left -= snprintf(&help[HELP_SIZE-left], left, "\n]}");
  print_response(true, (str){.data=help, .len=HELP_SIZE-left});
}

#define STRINGIFY_(x) #x
#define STRINGIFY(x)  STRINGIFY_(x)

#define DISPATCH(x, _h, _d)                                                    \
  if (str_eq_constant(cmd, STRINGIFY(x))) {                                    \
    disp = true;                                                               \
    x(args);                                                                   \
  }

void quit(webview_t w, void *_arg) {
  window = 0;
  webview_destroy(w);
}

#define BUF_SIZE 4096
char in_buf[BUF_SIZE]; // max command that can be read
void *drewl(void *input) {
  FILE *in = (FILE*)input;
  char *ln;
  while((ln = fgets(in_buf, BUF_SIZE, in))) {
    str line = (str){.len=strlen(ln),.data=ln};
    line = str_trim(line);
    str cmd, args;
    bool disp = false;
    if(!str_splitat(line, " \t", &cmd, &args)) {
      cmd = line;
      args = (str){0};
    }
    DO_COMMANDS(DISPATCH);
    if(!disp && (str_eq_constant(cmd, "quit") || str_eq_constant(cmd, "exit"))) {
      goto exit;
    }
    if(!disp) {
      print_response(false, str_constant("\"unrecognized command, hint: help\""));
    }
  }
 exit:
  webview_dispatch(window, quit, NULL);
  while(window) usleep(1000); // wait for webview to be destroyed
  exit(0);
  return NULL;
}

int main(void) {
  pthread_t cli;
  pthread_create(&cli, NULL, drewl, stdin);
  window = webview_create(0, NULL);
  webview_set_title(window, "drewl");
  webview_set_size(window, 480, 320, WEBVIEW_HINT_NONE);
  webview_set_html(window, "No URL set, use \"go\" command to navigate.");
  webview_bind(window, "_drewl_respond", respond, NULL);
  webview_bind(window, "_drewl_log", console_log, NULL);
  webview_run(window);
  return 0;
}

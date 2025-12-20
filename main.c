#include "str.h"
#include "webview/webview.h"
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>

#define STR_IMPLEMENTATION
#include "str.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

// global state
webview_t window;
str url = {0};

// respond to eval
void respond(const char *id, const char *result, void *user) {
  // result is JSONified array of [1 or 0,result]
  // we assume result is valid JSON and don't parse it
  str res = str_drop(str_constant((char*)result), 1);
  bool success = res.data[0] == '1';
  res = str_drop(res, 2); // skip "1," or "0,"
  res.len--; // skip the end ']' char
  fprintf(stdout, "{\"success\": %s, \"data\": "STR_FMT"}\n",
          success ? "true" : "false",
          STR_ARG(res));
}

// commands
#define DO_COMMANDS(X)                                                         \
  X(go)                                                                        \
  X(reload)                                                                    \
  X(eval)                                                                      \
  X(click)

    
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
 
#define STRINGIFY_(x) #x
#define STRINGIFY(x)  STRINGIFY_(x)

#define DISPATCH(x)                                                            \
  if (str_eq_constant(cmd, STRINGIFY(x))) {                                    \
    disp = true;                                                               \
    x(args);                                                                   \
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
      printf("Bye!\n");
      return NULL;
    }
    if(!disp) {
      printf("Unrecognized: "STR_FMT"\n", STR_ARG(line));
    }
  }
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
  webview_run(window);
  webview_destroy(window);
  return 0;
}

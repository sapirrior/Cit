#ifndef UI_H
#define UI_H

// GitHub Primer Design System Colors (Dark Mode)
#define COLOR_FG_DEFAULT   "\033[38;2;201;209;217m"
#define COLOR_FG_MUTED     "\033[38;2;139;148;158m"
#define COLOR_FG_SUCCESS   "\033[38;2;63;185;80m"
#define COLOR_FG_DANGER    "\033[38;2;248;81;73m"
#define COLOR_FG_ATTENTION "\033[38;2;210;153;34m"
#define COLOR_FG_ACCENT    "\033[38;2;88;166;255m"
#define COLOR_RESET        "\033[0m"
#define COLOR_BOLD         "\033[1m"

void ui_init();
void ui_info(const char *fmt, ...);
void ui_success(const char *fmt, ...);
void ui_error(const char *fmt, ...);
void ui_warn(const char *fmt, ...);
void ui_header(const char *fmt, ...);

#endif // UI_H

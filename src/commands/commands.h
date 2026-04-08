#ifndef COMMANDS_H
#define COMMANDS_H

int cmd_init(int argc, char *argv[]);
int cmd_add(int argc, char *argv[]);
int cmd_commit(int argc, char *argv[]);
int cmd_status(int argc, char *argv[]);
int cmd_log(int argc, char *argv[]);
int cmd_branch(int argc, char *argv[]);
int cmd_config(int argc, char *argv[]);
int cmd_checkout(int argc, char *argv[]);
int cmd_diff(int argc, char *argv[]);
int cmd_show(int argc, char *argv[]);
int cmd_reset(int argc, char *argv[]);
int cmd_clone(int argc, char *argv[]);

#endif // COMMANDS_H

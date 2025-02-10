#ifndef LPASS_RLG2_H
#define LPASS_RLG2_H

#ifdef LIGBIT

#include <git2.h>

typedef enum {
    GIT_ACTION_INSERT,
    GIT_ACTION_GENERATE,
    GIT_ACTION_EDIT,
    GIT_ACTION_DELETE,
    GIT_ACTION_MOVE
} git_action_t;

int lg2_open_repo(const char *path);
int lg2_close_repo();
int lg2_simple_action(git_action_t action, int is_overwrite, const char *path, const char *new_path);

#endif /* LIGBIT */

#endif /* LPASS_RLG2_H */

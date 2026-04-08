#ifndef REFS_H
#define REFS_H

char *get_current_branch();
int update_ref(const char *ref_path, const char *sha256);

#endif // REFS_H

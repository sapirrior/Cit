#include "utils.h"
#include <string.h>
#include <ctype.h>

int is_valid_email(const char *email) {
    if (!email || !*email) return 0;

    // Rule: Cannot start with a digit
    if (isdigit((unsigned char)email[0])) return 0;

    // Rule: Must start with an alphanumeric character
    if (!isalnum((unsigned char)email[0])) return 0;

    const char *at = strchr(email, '@');
    if (!at) return 0; // Must have '@'
    if (strchr(at + 1, '@')) return 0; // Must have ONLY one '@'

    // Local part (before @)
    if (at == email) return 0; // Cannot be empty

    for (const char *p = email; p < at; p++) {
        if (!isalnum((unsigned char)*p) && *p != '.' && *p != '_' && *p != '-' && *p != '+') {
            return 0; // Invalid character in local part
        }
    }

    // Domain part (after @)
    const char *domain = at + 1;
    if (!*domain) return 0; // Domain cannot be empty

    const char *dot = strrchr(domain, '.');
    if (!dot || dot == domain || !*(dot + 1)) return 0; // Must have a '.' and it can't be first or last

    // TLD (after last dot) should be at least 2 chars
    if (strlen(dot + 1) < 2) return 0;

    for (const char *p = domain; *p; p++) {
        if (!isalnum((unsigned char)*p) && *p != '.' && *p != '-') {
            return 0; // Invalid character in domain
        }
    }

    return 1;
}

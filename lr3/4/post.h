#ifndef POST_H
#define POST_H

#include "mystring.h"

#define IS_DIG(c) ((c) >= '0' && (c) <= '9')

typedef unsigned long long ull;

typedef struct
{
    String city, street, building, index;
    int house, apartment;
} Address;

typedef struct
{
    Address to;
    double weight;
    String id, created, deadline;
    ull created_key, deadline_key;
    bool delivered;
} Mail;

typedef struct
{
    const Address *office;
    Mail *mails;
    size_t size, cap;
} Post;

void free_address(Address *a);
void free_mail(Mail *m);
void free_post(Post *p);
status_t parse_time(const char *s, ull *key);
status_t now_key(ull *key);
status_t find_index(const Post *p, const String *id, size_t *idx, bool *found);
status_t post_add(Post *p, const Mail *m);
status_t post_remove(Post *p, size_t idx);
status_t post_select(const Post *p, ull now, bool delivered, const Mail ***out, size_t *count);

#endif
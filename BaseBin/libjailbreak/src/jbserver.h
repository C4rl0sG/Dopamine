#ifndef JBSERVER_H
#define JBSERVER_H

#include <stdbool.h>
#include <stdint.h>
#include <xpc/xpc.h>
#include <xpc_private.h>
#include "jbserver_domains.h"

typedef enum {
    JBS_TYPE_BOOL,
	JBS_TYPE_UINT64,
	JBS_TYPE_STRING,
	JBS_TYPE_DATA,
    JBS_TYPE_ARRAY,
	JBS_TYPE_DICTIONARY,
	JBS_TYPE_CALLER_TOKEN,
    JBS_TYPE_XPC_GENERIC,
} jbserver_type;

typedef struct s_jbserver_arg
{
    const char *name;
    jbserver_type type;
	bool out;
} jbserver_arg;

struct jbserver_action {
    void *handler;
    jbserver_arg *args;
};

struct jbserver_domain {
    bool (*permissionHandler)(audit_token_t);
    struct jbserver_action actions[];  // Flexible array member moved to the end
};

struct jbserver_impl {
    uint64_t maxDomain;
    struct jbserver_domain **domains;
};

extern struct jbserver_impl gGlobalServer;

int jbserver_received_xpc_message(struct jbserver_impl *server, xpc_object_t xmsg);

#define JBSERVER_MACH_MAGIC 0x444F50414D494E45
#define JBSERVER_MACH_CHECKIN_STAGE1 0

struct jbserver_mach_msg {
    mach_msg_header_t hdr;
    uint64_t magic;
    uint64_t action;
};

struct jbserver_mach_msg_reply {
    struct jbserver_mach_msg msg;
    uint64_t status;
};

struct jbserver_mach_msg_checkin_stage1 {
    struct jbserver_mach_msg base;
};

struct jbserver_mach_msg_checkin_stage1_reply {
    struct jbserver_mach_msg_reply base;
    char sbx_tokens[2000];
};

extern int (*jbserver_mach_msg_handler)(audit_token_t *auditToken, struct jbserver_mach_msg *jbsMachMsg);
int jbserver_received_mach_message(audit_token_t *auditToken, struct jbserver_mach_msg *jbsMachMsg);

#endif

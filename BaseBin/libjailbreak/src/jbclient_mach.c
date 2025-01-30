#include "jbclient_mach.h"
#include <dispatch/dispatch.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <pthread.h>
#include <mach-o/dyld.h>
#include <dlfcn.h>

mach_port_t jbclient_mach_get_launchd_port(void)
{
	mach_port_t launchdPort = MACH_PORT_NULL;
	task_get_bootstrap_port(task_self_trap(), &launchdPort);
	return launchdPort;
}

kern_return_t jbclient_mach_send_msg(struct jbserver_mach_msg *msg, struct jbserver_mach_msg_reply *reply)
{
	static mach_port_t launchdPort = MACH_PORT_NULL;
	mach_port_t replyPort = mig_get_reply_port();
	
	if (!replyPort)
		return KERN_FAILURE;
	
	if (!launchdPort)
		launchdPort = jbclient_mach_get_launchd_port();
	
	if (!launchdPort)
		return KERN_FAILURE;

	msg->magic = JBSERVER_MACH_MAGIC;
	
	msg->hdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	// size already set
	msg->hdr.msgh_remote_port  = launchdPort;
	msg->hdr.msgh_local_port   = replyPort;
	msg->hdr.msgh_voucher_port = 0;
	msg->hdr.msgh_id           = 0x40000000;
	
	kern_return_t kr = mach_msg(&msg->hdr, MACH_SEND_MSG, msg->hdr.msgh_size, 0, 0, 0, 0);
	if (kr != KERN_SUCCESS)
		return kr;
	
	kr = mach_msg(&reply->msg.hdr, MACH_RCV_MSG, 0, reply->msg.hdr.msgh_size, replyPort, 0, 0);
	if (kr != KERN_SUCCESS)
		return kr;
	
	// Get rid of any rights we might have received
	mach_msg_destroy(&reply->msg.hdr);
	return KERN_SUCCESS;
}

int jbclient_mach_process_checkin_stage1(char *sandboxExtensionsOut)
{
	struct jbserver_mach_msg_checkin_stage1 msg;
	msg.base.hdr.msgh_size = sizeof(msg);
	msg.base.action = JBSERVER_MACH_CHECKIN_STAGE1;

	size_t replySize = sizeof(struct jbserver_mach_msg_checkin_stage1_reply) + MAX_TRAILER_SIZE;

	uint8_t replyU[replySize];
	bzero(replyU, replySize);
	struct jbserver_mach_msg_checkin_stage1_reply *reply = (struct jbserver_mach_msg_checkin_stage1_reply *)&replyU;
	reply->base.msg.hdr.msgh_size = replySize;

	kern_return_t kr = jbclient_mach_send_msg((struct jbserver_mach_msg *)&msg, (struct jbserver_mach_msg_reply *)reply);
	if (kr == KERN_SUCCESS) {
		reply->sbx_tokens[1999] = '\0';
		strcpy(sandboxExtensionsOut, reply->sbx_tokens);
	}
	return kr;
}
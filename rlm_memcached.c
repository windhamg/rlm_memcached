/*
 * rlm_memcached.c
 *
 * Version:	$Id$
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Copyright 2000,2006  The FreeRADIUS server project
 * Copyright 2000  your name info@simonecaruso.com
 */

#include <freeradius/ident.h>
#include <freeradius/radiusd.h>
#include <freeradius/modules.h>
#include <libmemcached/memcached.h>

RCSID("$Id$")

typedef struct rlm_memcached_t {
	char *server_addr;
	int otp_mode;
	memcached_st *st;
} rlm_memcached_t;


/* EXAMPLE CONFIG */

static const CONF_PARSER module_config[] = {
  { "address",  PW_TYPE_STRING_PTR, offsetof(rlm_memcached_t, server_addr), NULL,  "127.0.0.1"},
  { "otp" , PW_TYPE_BOOLEAN, offsetof(rlm_memcached_t, otp_mode), NULL, "no"},
  { NULL, -1, 0, NULL, NULL }
};

static int memcached_instantiate(CONF_SECTION *conf, void **instance)
{
	rlm_memcached_t *config = (rlm_memcached_t *)rad_malloc(sizeof(rlm_memcached_t));
	memcached_return rc;
	memcached_server_st *server_list = NULL;
	size_t len;
	void *ret;

	if (cf_section_parse(conf, config, module_config) < 0) {
		free(config);
		return -1;
	}

 	config->st = memcached_create(NULL);

	if(!config->st){
		radlog(L_ERR, "rlm_memcached: Error in memcached configuration %s", memcached_last_error_message(config->st));
		return 1;
	}
	 
	
	server_list = memcached_server_list_append(server_list, config->server_addr, 11211, &rc);
	memcached_server_push(config->st, server_list);
	
	memcached_server_list_free(server_list);


//	ret = memcached_get(config->st, "simone", strlen("simone"), &len, 0, &rc);


	*instance = config;

	return 0;
}

/*
 *	Find the named user in this modules database.  Create the set
 *	of attribute-value pairs to check and reply with for this user
 *	from the database. The authentication code only needs to check
 *	the password, the rest is done here.
 */
static int memcached_authorize(void *instance, REQUEST *request)
{
	VALUE_PAIR *state;
	VALUE_PAIR	user, *vp;
	memcached_return rc;
	rlm_memcached_t *conf = instance;
	size_t result_len,len;
	uint32_t retflags;
	time_t exptime;
	char *result;
	char *timestamp;
	
	user.name = request->username ? request->username->vp_strvalue : "NONE";
	len = strlen(user.name);
	result = memcached_get(conf->st, user.name, len, &result_len, &retflags, &rc);

	if(!result){
		DEBUG("User not found: %s", memcached_last_error_message(conf->st));
		return RLM_MODULE_NOOP;
	}
	if (conf->otp_mode && ((timestamp = strstr(result, "\t")) != NULL) {
		exptime = (time_t)atol(++timestamp);
		if (exptime <= time()) {
			DEBUG("OTP expired");
			return RLM_MODULE_OK;
		}
	}
	DEBUG("User found: %s", result);
	vp = pairmake("Cleartext-Password", result, T_OP_SET);
	pairmove(&request->config_items, &vp);
	pairfree(&vp);
	pairdelete(&request->reply->vps, PW_FALL_THROUGH);

	/* delete OTP creds from memcache if in OTP mode */
	if (conf->otp_mode) {
		result = memcached_delete(conf->st, user.name, len, (time_t)0);
		if(!result) {
			DEBUG("User not found on OTP delete: %s", memcached_last_error_message(conf->st));
		}
	}

	return RLM_MODULE_OK;
}

/*
 *	Authenti
 cate the user with the given password.
 */
static int memcached_authenticate(void *instance, REQUEST *request)
{
	char *p ;

	/* quiet the compiler */
	instance = instance;
	request = request;

	DEBUG("EXAMPLE: AUTHENTICATE!");
	p = request->password->vp_octets;
/*
	do{
		
		if(strncmp(p, mc->password, request->password->length) == 0){
			DEBUG("TROVATA");		
			break;
		}
		mc = mc->next;
	}while(mc->next);
*/

	if(rad_digest_cmp(request->config_items->vp_strvalue,
				request->password->vp_strvalue,
				request->config_items->length) == 0){
		DEBUG ("AUTH OK");
		return RLM_MODULE_OK;
	}

	//DEBUG("EXAMPLE passwd: %s", p);

	

	return RLM_MODULE_FAIL;
}

/*
 *	Massage the request before recording it or proxying it
 */
	static int memcached_preacct(void *instance, REQUEST *request)
{
	/* quiet the compiler */
	instance = instance;
	request = request;

	return RLM_MODULE_OK;
}

/*
 *	Write accounting information to this modules database.
 */
static int memcached_accounting(void *instance, REQUEST *request)
{
	/* quiet the compiler */
	instance = instance;
	request = request;

	return RLM_MODULE_OK;
}

static int memcached_checksimul(void *instance, REQUEST *request)
{
  instance = instance;

  request->simul_count=0;

  return RLM_MODULE_OK;
}


/*
 *	Only free memory we allocated.  The strings allocated via
 *	cf_section_parse() do not need to be freed.
 */
static int memcached_detach(void *instance)
{
	free(instance);
	return 0;
}

/*
 *	The module name should be the only globally exported symbol.
 *	That is, everything else should be 'static'.
 *
 *	If the module needs to temporarily modify it's instantiation
 *	data, the type should be changed to RLM_TYPE_THREAD_UNSAFE.
 *	The server will then take care of ensuring that the module
 *	is single-threaded.
 */
module_t rlm_memcached = {
	RLM_MODULE_INIT,
	"memcached",
	RLM_TYPE_THREAD_SAFE,		/* type */
	memcached_instantiate,		/* instantiation */
	memcached_detach,			/* detach */
	{
		memcached_authenticate,	/* authentication */
		memcached_authorize,	/* authorization */
		NULL,	/* preaccounting */
		NULL,	/* accounting */
		NULL,	/* checksimul */
		NULL,			/* pre-proxy */
		NULL,			/* post-proxy */
		NULL			/* post-auth */
	},
};

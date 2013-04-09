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

RCSID("$Id$")

typedef struct myconf {
	char *user;
	char *password;
	struct myconf *next;
} myconf;


/* EXAMPLE CONFIG */
/*
static const CONF_PARSER module_config[] = {
  { "integer", PW_TYPE_INTEGER,    offsetof(rlm_example_t,value), NULL,   "1" },
  { "boolean", PW_TYPE_BOOLEAN,    offsetof(rlm_example_t,boolean), NULL, "no"},
  { "string",  PW_TYPE_STRING_PTR, offsetof(rlm_example_t,string), NULL,  NULL},
  { "ipaddr",  PW_TYPE_IPADDR,     offsetof(rlm_example_t,ipaddr), NULL,  "*" },

  { NULL, -1, 0, NULL, NULL }
};

*/
static int memcached_instantiate(CONF_SECTION *conf, void **instance)
{

	myconf *root, *p ;
	root = rad_malloc(sizeof(*p));
	if (!root) {
		return -1;
	}
	p = root;
	p->user = "pippo";
	p->password  = "pippo";
	p->next = rad_malloc(sizeof(*p));
	p = p->next;
	p->next = NULL;
	p->user = "simone";
	p->password = "simone";

	DEBUG("EXAMPLE: INSTANZIATO!");

	*instance = root;

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
	VALUE_PAIR *reply;
	VALUE_PAIR my_pl,*vp;
	VALUE_PAIR	*namepair;

	myconf *conf = instance;

	int found = 0;
	DEBUG("EXAMPLE: authorized sec");


	

	namepair = request->username;
	my_pl.name = namepair ? (char *) namepair->vp_strvalue : "NONE";
	
	do{
		if(strcmp(conf->user, my_pl.name) == 0){
			found=1;
			break;
		}
		conf= conf->next;
	}while(conf);
	
	if(!found){
		DEBUG("NO USER FOUND");
		return RLM_MODULE_FAIL;
	}
	DEBUG("USERNMANE: %s", my_pl.name);

	vp = pairmake("Cleartext-Password", conf->password, T_OP_SET);
	if (!vp)
		return RLM_MODULE_FAIL;

	pairmove(&request->config_items, &vp);
	pairfree(&vp);

	pairdelete(&request->reply->vps, PW_FALL_THROUGH);
	
	//pairadd(&request->config_items, pairmake("Auth-Type", "PAP", T_OP_EQ));

	return RLM_MODULE_OK;
}

/*
 *	Authenticate the user with the given password.
 */
static int memcached_authenticate(void *instance, REQUEST *request)
{
	char *p ;
	myconf *mc = instance;
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

	DEBUG("EXAMPLE passwd: %s", p);

	

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

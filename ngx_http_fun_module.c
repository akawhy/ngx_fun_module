#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ddebug.h"

typedef struct {
    ngx_str_t   varname;
    ngx_str_t   varvalue;
} ngx_http_fun_loc_conf_t;

static ngx_int_t ngx_http_fun_init(ngx_conf_t *cf);
static void *ngx_http_fun_create_loc_conf(ngx_conf_t *cf);

static ngx_int_t ngx_http_fun_handler(ngx_http_request_t *r);

static ngx_command_t ngx_http_fun_commands[] = {
    {
        ngx_string("fun"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_fun_loc_conf_t, varname),
        NULL
    },
    {
        ngx_string("fun_value"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_fun_loc_conf_t, varvalue),
        NULL
    },
    ngx_null_command
};

static ngx_http_module_t ngx_http_fun_module_ctx = {
    NULL,                              /* preconfiguration */
    ngx_http_fun_init,                 /* postconfiguration */

    NULL,                              /* create main configuration */
    NULL,                              /* init main configuration */

    NULL,                              /* create server configuration */
    NULL,                              /* merge server configuration */

    ngx_http_fun_create_loc_conf,      /* create location configuration */
    NULL                               /* merge location configuration */
};

ngx_module_t  ngx_http_fun_module = {
    NGX_MODULE_V1,
    &ngx_http_fun_module_ctx,                      /* module context */
    ngx_http_fun_commands,                         /* module directives */
    NGX_HTTP_MODULE,                                /* module type */
    NULL,                                           /* init master */
    NULL,                                           /* init module */
    NULL,                                           /* init process */
    NULL,                                           /* init thread */
    NULL,                                           /* exit thread */
    NULL,                                           /* exit process */
    NULL,                                           /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_int_t
ngx_http_fun_handler(ngx_http_request_t *r)
{
    ngx_http_variable_t          *v;
    ngx_http_variable_value_t    *vv;
    ngx_str_t                    *varname, *val;
    u_char                       *lowcase;
    ngx_http_fun_loc_conf_t      *flcf;
    ngx_http_core_main_conf_t    *cmcf;
    ngx_uint_t                    key;

    flcf = ngx_http_get_module_loc_conf(r, ngx_http_fun_module);
    if (flcf->varname.len == 0 || flcf->varvalue.len == 0) {
        return NGX_DECLINED;
    }

    varname = &flcf->varname;
    val     = &flcf->varvalue;

    /* get variable */
    lowcase = ngx_pcalloc(r->pool, varname->len + 1);
    if (lowcase == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "out of memory");
        return NGX_DECLINED;
    }

    key = ngx_hash_strlow(lowcase, varname->data, varname->len);
    lowcase[varname->len] = 0;

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

    v = ngx_hash_find(&cmcf->variables_hash, key, lowcase, varname->len);

    if (v == NULL) {
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                "\"%s\" variable not found", lowcase);
        return NGX_DECLINED;
    }

    if (!(v->flags & NGX_HTTP_VAR_CHANGEABLE)) {
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                "\"%s\" variable not changable", lowcase);
        return NGX_DECLINED;
    }

    /* set variable */
    if (v->set_handler) {

        vv = ngx_palloc(r->pool, sizeof(ngx_http_variable_value_t));
        if (vv == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                    "out of memory");
            return NGX_DECLINED;
        }

        vv->valid = 1;
        vv->not_found = 0;
        vv->no_cacheable = 0;

        vv->data = val->data;
        vv->len  = val->len;

        v->set_handler(r, vv, v->data);

        return NGX_DECLINED;
    }

    if (v->flags & NGX_HTTP_VAR_INDEXED) {
        vv = &r->variables[v->index];

        vv->valid = 1;
        vv->not_found = 0;
        vv->no_cacheable = 0;

        vv->data = val->data;
        vv->len  = val->len;
   
        return NGX_DECLINED;
    }

    return NGX_DECLINED;
}
 
static ngx_int_t
ngx_http_fun_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt               *h;  

    ngx_http_core_main_conf_t         *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_PREACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }    

    *h = ngx_http_fun_handler;

    return NGX_OK;
}


static void *
ngx_http_fun_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_fun_loc_conf_t *flcf =  NULL;

    flcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_fun_loc_conf_t));

    if (flcf == NULL) {
        return NULL;
    }

    return flcf;
}

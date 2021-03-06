#ifndef __THA_DB__
#define __THA_DB__

#include "common.h"
#include "memory.h"

extern int I_AM_ACTIVE_CP;

/* primitive THA sync enable Data structures*/

#define CHECKPOINT_DISABLE	0
#define CHECKPOINT_ENABLE	(!CHECKPOINT_DISABLE)

void 
load_checkpoint (char *CHP_FILE_NAME);

/* THA Struct db */
typedef struct _tha_struct_db_rec_t tha_struct_db_rec_t;
typedef struct _tha_object_db_rec_t tha_object_db_rec_t;
typedef struct _tha_struct_db_t tha_struct_db_t;
typedef struct _tha_object_db_t tha_object_db_t;

#define STATIC_OBJ	1
#define DYN_OBJ		2

typedef struct _tha_field_info_{
	char fname[MAX_FIELD_NAME_SIZE];
	unsigned int size;
	unsigned int offset;
	DATA_TYPE_t dtype;
	char nested_str_name[MAX_STRUCTURE_NAME_SIZE]; // valid if dtype = OBJ_STRUCT
	tha_struct_db_rec_t *nested_struct_ptr;
} tha_field_info_t;

tha_field_info_t*
tha_get_struct_field_info_by_index(tha_object_db_rec_t *obj_rec, int field_index);


//tha_struct_db_rec_t
struct _tha_struct_db_rec_t{
	tha_struct_db_rec_t *next;
	tha_struct_db_rec_t *prev;
	char struct_name[MAX_STRUCTURE_NAME_SIZE]; // key
	unsigned int ds_size;
	unsigned int n_fields;
	tha_field_info_t *fields;
	char late_binding_flag;
};

struct _tha_struct_db_t{
	tha_struct_db_rec_t *head;
	unsigned int count;
};



#define HA_FIELD_OFFSET(st, name)       ((int)&((st *)0)->name)
#define HA_FIELD_SIZE(st, name)         sizeof (((st *)0)->name)


#define HA_FIELD_INFO(st, fname, dtype, nested_str_name)				         \
	{#fname, HA_FIELD_SIZE(st, fname), HA_FIELD_OFFSET(st, fname), dtype, #nested_str_name, NULL}

typedef void (*_fn_ptr_db)(void **dest, int fn_id);

typedef struct _tha_handle_t{
	tha_struct_db_t *struct_db;
	tha_object_db_t *object_db;
	_fn_ptr_db fn_ptr_db;
	char standby_ip[16];
} tha_handle_t;

extern tha_handle_t *handle;

tha_handle_t* init_tha(char *standby_ip, _fn_ptr_db fn_ptr_db);

int
ha_reg_struct(tha_handle_t *handle, tha_struct_db_rec_t *rec);

int
ha_unreg_struct(tha_handle_t *handle, tha_struct_db_rec_t *rec);

void
dump_tha_struct_db(tha_handle_t *handle);

void
dump_tha_object(tha_object_db_rec_t *obj_rec);

void
tha_clean_application_state(tha_handle_t *handle);

void
send_tha_clean_application_state_order(tha_handle_t *handle);

int
bind_field_nested_structure(tha_handle_t *handle, tha_field_info_t *fields, int n_fields);

#define HA_REG_STRUCT(handle, st_name, _fields)					\
do{										\
	tha_struct_db_rec_t *rec = calloc(1, sizeof(tha_struct_db_rec_t));	\
	strncpy(rec->struct_name, #st_name, MAX_STRUCTURE_NAME_SIZE);		\
	rec->ds_size = sizeof(st_name);						\
	rec->n_fields = sizeof(_fields)/sizeof(tha_field_info_t);		\
	rec->fields = _fields;							\
	if(ha_reg_struct(handle, rec) == FAILURE){				\
		free(rec);							\
	}									\
	int rc = bind_field_nested_structure(handle, _fields, rec->n_fields);   \
	if(rc == FAILURE)							\
		rec->late_binding_flag = 1;					\
}while(0);

void
dump_struct_db_rec(tha_struct_db_rec_t *rec);


/* THA object db*/

struct _tha_object_db_rec_t{
	tha_object_db_rec_t *next;
	tha_object_db_rec_t *prev;
	short dirty_bit; /* is set object is changed and needs THA sync*/
	void *ptr;
	char obj_id[MAX_STRUCTURE_NAME_SIZE];
	char obj_name[MAX_STRUCTURE_NAME_SIZE];
	unsigned int units;
	int object_type;
	unsigned int *tha_enable_offset;
	tha_struct_db_rec_t *struct_rec;
};

typedef struct _tha_object_db_t{
	tha_object_db_rec_t *head;
	unsigned int count;
} tha_object_db_t;


int
ha_reg_object(tha_handle_t *handle, tha_object_db_rec_t *rec);

int
ha_unreg_object(tha_handle_t *handle, tha_object_db_rec_t *rec);

void
dump_tha_object_db(tha_handle_t *handle);

void
dump_object_db_rec(tha_object_db_rec_t *rec);


/* THA Operarions */

#define ha_calloc(handle, out_ptr, structure,  units)					\
do{											\
	if(I_AM_ACTIVE_CP == 0)								\
		break;									\
	tha_struct_db_rec_t *struct_rec = tha_struct_db_look_up(handle, #structure);	\
	if(struct_rec){									\
		out_ptr = (structure *)calloc(sizeof(structure), units); 		\
	}										\
	tha_add_object(handle, out_ptr, #structure, NULL, units, DYN_OBJ);		\
}while(0);


/* to be used for base array pointers Or simple pointers to the objects*/
#define THA_ADD_GLOBAL_OBJECT_BY_REF(handle, obj_id, ref_type)					\
do{											\
	int units = sizeof(obj_id)/sizeof(ref_type);					\
	memset(obj_id, 0, sizeof(ref_type) * units);					\
	tha_add_object(handle, obj_id, #ref_type, #obj_id, units, STATIC_OBJ);		\
}while(0);										

#if 1
/* to be used for object variables*/
#define THA_ADD_GLOBAL_OBJECT_BY_VAL(handle, obj_id, structure)				\
do{											\
	int units = sizeof(obj_id)/sizeof(structure);					\
	memset(&obj_id, 0, sizeof(structure));						\
	tha_add_object(handle, &obj_id, #structure, #obj_id, units, STATIC_OBJ);	\
}while(0);										
#endif

#define GET_OBJECT_PTR_AT_INDEX(object_rec, object_index)	\
	((char *)(object_rec->ptr) + (object_index * object_rec->struct_rec->ds_size))

#define FREE_OBJECT_HASH_ARRAY(object_rec)	\
	free(object_rec->tha_enable_offset);	\
	object_rec->tha_enable_offset = NULL;


int
tha_add_object(tha_handle_t *handle, void *ptr, char *str_name, char *obj_id, int units, int obj_type);

void
tha_perform_structure_late_binding(tha_handle_t *handle);

void
free_object_in_object_record(tha_object_db_rec_t *obj_rec);

/* LookUp functions*/

/* struct db look up function*/

tha_struct_db_rec_t* tha_struct_db_look_up(tha_handle_t *handle, char *struct_name);

/* Object DB lookup function*/

tha_object_db_rec_t* tha_object_db_lookup_by_name(tha_handle_t *handle, char *object_name);
tha_object_db_rec_t* tha_object_db_lookup_by_Addr(tha_handle_t *handle, void *addr);
tha_object_db_rec_t *tha_object_db_lookup_by_objid(tha_handle_t *handle,  char *obj_id);

/* work for both static and dynamic objects*/
unsigned int *get_struct_hash_array_fn(tha_object_db_rec_t *obj_rec, int *out_size, int obj_index);

int
tha_remove_object_db_rec(tha_handle_t *handle, tha_object_db_rec_t *object_rec);

/* user space API*/
int
ha_sync(tha_handle_t *handle);

void
ha_free(void *obj_ptr);

typedef void (*cotrol_fn)();
void tha_acquire_standby_state(cotrol_fn fn);

void tha_acquire_active_state(cotrol_fn fn);


#endif

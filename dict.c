#include"dict.h"

#include<stdlib.h>
#include<string.h>
#include<assert.h>

static const int init_bucket_count = 8;

typedef struct dict_internal_struct
{
	hash_func hash_func;
	compare_func compare_func;
	alloc_copy_key alloc_copy_key;
}dict_internal_t, *dict_internal_p;

static uint32_t crc32_notable(uint32_t seed, const void *data, size_t len)
{
	int i;
	const uint8_t *ptr = data;
	uint32_t crc = seed ^ 0xffffffff;
	while(len--)
	{
		uint32_t val = (crc ^ *ptr++) & 0xff;
		for (i = 0; i < 8; i++)
		{
			val = (val >> 1) ^ (val & 1 ? 0xEDB88320 : 0);
		}
		crc = val ^ crc >> 8;
	}
	return crc ^ 0xffffffff;
}

static hash_t def_hash_func(const void *key)
{
	size_t a = 114514;
	size_t b = 1919810;
	hash_t hash = 0;
	const char *str = key;
	size_t l = strlen(str);
	size_t loopc = l / sizeof l;
	const size_t *p = key;

	while (loopc--)
	{
		hash *= a;
		hash += (*p++);
		a *= b;
		l -= sizeof l;
	}

	if (l)
	{
		loopc = 0;
		memcpy(&loopc, p, l);
		hash *= a;
		hash += loopc;
	}

	return hash;
}

static void def_on_delete_key(void *key)
{
	free(key);
}

static void def_on_delete_value(void *value)
{
	free(value);
}

static int def_compare_func(const void *key1, const void *key2)
{
	return strcmp(key1, key2);
}

static void *def_alloc_copy_key(const void *key)
{
	size_t length = strlen(key);
	void *str = malloc(length + 1);
	if (!str) return NULL;

	memcpy(str, key, length + 1);
	return str;
}

dict_p dict_create()
{
	dict_p r = NULL;
	dict_internal_p i;
	size_t bucket_size;

	r = malloc(sizeof *r + sizeof (dict_internal_t));
	if (!r) return r;
	memset(r, 0, sizeof * r);
	r->internal = &r[1];
	i = r->internal;
	i->hash_func = def_hash_func;
	i->compare_func = def_compare_func;
	i->alloc_copy_key = def_alloc_copy_key;
	r->on_delete_key = def_on_delete_key;
	r->on_delete_value = def_on_delete_value;

	r->bucket_bit_count = init_bucket_count;
	bucket_size = (size_t)1 << r->bucket_bit_count;
	r->buckets = malloc(bucket_size * sizeof r->buckets[0]);
	if (!r->buckets) goto FailExit;
	memset(r->buckets, 0, bucket_size * sizeof r->buckets[0]);
	return r;
FailExit:
	dict_delete(r);
	return NULL;
}

dict_status dict_set_hash_func(dict_p dict, hash_func hash_func)
{
	dict_internal_p i;

	if (!dict) return ds_invalidarguments;
	if (!hash_func) hash_func = def_hash_func;
	if (dict->data_count) return ds_invalidcall;

	i = dict->internal;
	i->hash_func = hash_func;
	return ds_ok;
}

hash_func dict_get_hash_func(dict_p dict)
{
	dict_internal_p i;
	if (!dict) return NULL;

	i = dict->internal;
	return i->hash_func;
}

dict_status dict_set_compare_func(dict_p dict, compare_func compare_func)
{
	dict_internal_p i;

	if (!dict) return ds_invalidarguments;
	if (!compare_func) compare_func = def_compare_func;
	if (dict->data_count) return ds_invalidcall;

	i = dict->internal;
	i->compare_func = compare_func;
	return ds_ok;
}

compare_func dict_get_compare_func(dict_p dict)
{
	dict_internal_p i;
	if (!dict) return NULL;

	i = dict->internal;
	return i->compare_func;
}

dict_status dict_set_alloc_copy_key_func
(
	dict_p dict,
	alloc_copy_key alloc_copy_key
)
{
	dict_internal_p i;
	if (!dict) return ds_invalidarguments;
	if (!alloc_copy_key) alloc_copy_key = def_alloc_copy_key;

	i = dict->internal;
	i->alloc_copy_key = alloc_copy_key;
	return ds_ok;
}

alloc_copy_key dict_get_alloc_copy_key_func(dict_p dict)
{
	dict_internal_p i;
	if (!dict) return NULL;

	i = dict->internal;
	return i->alloc_copy_key;
}

dict_status dict_set_on_delete_key(dict_p dict, on_delete_key on_delete_key)
{
	if (!dict) return ds_invalidarguments;
	if (!on_delete_key) on_delete_key = def_on_delete_key;

	dict->on_delete_key = on_delete_key;
	return ds_ok;
}

on_delete_key dict_get_on_delete_key(dict_p dict)
{
	return dict->on_delete_key;
}

dict_status dict_set_on_delete_value(dict_p dict, on_delete_value on_delete_value)
{
	if (!dict) return ds_invalidarguments;
	if (!on_delete_value) on_delete_value = def_on_delete_value;

	dict->on_delete_value = on_delete_value;
	return ds_ok;
}

on_delete_value dict_get_on_delete_value(dict_p dict)
{
	return dict->on_delete_value;
}

static void free_buckets(dict_bucket_p buckets, int bucket_bit_count)
{
	dict_bucket_p bucket;
	size_t i;
	size_t count = (size_t)1 << bucket_bit_count;
	for (i = 0; i < count; i++)
	{
		dict_bucket_p next;
		bucket = &buckets[i];
		next = bucket->next;
		while (next)
		{
			free(bucket);
			bucket = next;
			next = bucket->next;
		}
	}
	free(buckets);
}

static dict_status add_to_buckets(dict_bucket_p buckets, int bucket_bit_count, compare_func cp, hash_t hash, void *key, void *value, size_t *collision_counter)
{
	size_t bucket_size = (size_t)1 << bucket_bit_count;
	size_t hashed_index = hash & (bucket_size - 1);
	dict_bucket_p bucket = &buckets[hashed_index];

	// 检查是撞哈希还是key已经存在
	if (bucket->key)
	{
		while(1)
		{
			// key完全相同，则重复
			if (!cp(bucket->key, key)) return ds_alreadyexists;

			// key不同，意味着哈希碰撞，需要遍历链表至尾部
			collision_counter[0]++;
			if (bucket->next)
			{
				bucket = bucket->next;
				continue;
			}
			else
			{
				bucket->next = malloc(sizeof * bucket);
				bucket = bucket->next;
				if (!bucket) return ds_nomemory;
				goto CopyValue;
			}
		}
	}
CopyValue:
	bucket->hash = hash;
	bucket->next = NULL;
	bucket->key = key;
	bucket->value = value;
	return ds_ok;
}

dict_status dict_resize(dict_p dict, int new_bit_count)
{
	size_t i;
	dict_internal_p in = dict->internal;
	int old_bit_count = dict->bucket_bit_count;
	size_t old_bucket_size = (size_t)1 << old_bit_count;
	size_t new_bucket_size = (size_t)1 << new_bit_count;
	dict_status retval = ds_ok;
	dict_bucket_p old_buckets = dict->buckets;
	dict_bucket_p new_buckets = malloc(new_bucket_size * sizeof new_buckets[0]);
	size_t new_coll = 0;
	if (!new_buckets) return ds_nomemory;
	memset(new_buckets, 0, new_bucket_size * sizeof new_buckets[0]);

	// 遍历旧的字典桶，取出数据重新放入新字典桶。
	for (i = 0; i < old_bucket_size; i++)
	{
		dict_bucket_p next;
		dict_bucket_p bucket = &dict->buckets[i];

		// 以key值是否为NULL判断桶是否装有数据
		if (bucket->key)
		{
			retval = add_to_buckets(new_buckets, new_bit_count, in->compare_func, bucket->hash, bucket->key, bucket->value, &new_coll);
			if (retval != ds_ok) goto FailExit;

			// 遍历链表
			next = bucket->next;
			while (next)
			{
				bucket = next;
				retval = add_to_buckets(new_buckets, new_bit_count, in->compare_func, bucket->hash, bucket->key, bucket->value, &new_coll);
				if (retval != ds_ok) goto FailExit;
				next = bucket->next;
			}
		}
	}

	free_buckets(old_buckets, old_bit_count);
	dict->buckets = new_buckets;
	dict->bucket_bit_count = new_bit_count;
	dict->hash_collision_count = new_coll;
	return ds_ok;
FailExit:
	free_buckets(new_buckets, new_bit_count);
	return retval;
}

dict_status dict_insert
(
	dict_p dict,
	const void *key,
	void *value
)
{
	hash_t hash;
	dict_internal_p in;
	dict_status retval = ds_ok;
	size_t bucket_size;
	void *new_alloc_key = NULL;

	if (!dict || !key) return ds_invalidarguments;
	in = dict->internal;
	new_alloc_key = in->alloc_copy_key(key);
	if (!new_alloc_key) return ds_nomemory;

	bucket_size = (size_t)1 << dict->bucket_bit_count;
	if (dict->data_count >= bucket_size)
	{
		int new_bit_count = dict->bucket_bit_count;
		while (((size_t)1 << new_bit_count) < dict->data_count) new_bit_count ++;
		retval = dict_resize(dict, new_bit_count);
		if (retval != ds_ok) return retval;
		bucket_size = (size_t)1 << dict->bucket_bit_count;
	}

	hash = in->hash_func(key);
	retval = add_to_buckets(dict->buckets, dict->bucket_bit_count, in->compare_func, hash, new_alloc_key, value, &dict->hash_collision_count);
	if (retval == ds_ok)
	{
		dict->data_count++;
	}
	else
	{
		dict->on_delete_key(new_alloc_key);
	}
	return retval;
}

dict_status dict_assign
(
	dict_p dict,
	void *key,
	void *value
)
{
	dict_bucket_p found = dict_search_bucket(dict, key);
	if (!found) return dict_insert(dict, key, value);
	else
	{
		dict->on_delete_value(found->value);
		found->value = value;
		return ds_ok;
	}
}

dict_bucket_p dict_search_bucket(dict_p dict, const void *key)
{
	hash_t hash;
	size_t bucket_size;
	size_t hashed_index;
	dict_internal_p in;
	dict_bucket_p bucket;

	if (!dict || !key) return NULL;

	in = dict->internal;
	hash = in->hash_func(key);
	bucket_size = (size_t)1 << dict->bucket_bit_count;
	hashed_index = hash & (bucket_size - 1);
	bucket = &dict->buckets[hashed_index];

	do
	{
		if (!bucket->key) return NULL;
		if (!in->compare_func(key, bucket->key)) return bucket;
		bucket = bucket->next;
	} while (bucket);
	return NULL;
}

void *dict_search(dict_p dict, const void *key)
{
	dict_bucket_p found = dict_search_bucket(dict, key);
	if (!found) return NULL;
	else return found->value;
}

dict_status dict_remove(dict_p dict, const void *key)
{
	hash_t hash;
	size_t bucket_size;
	size_t hashed_index;
	dict_internal_p in;
	dict_bucket_p bucket;

	if (!dict || !key) return ds_invalidarguments;

	in = dict->internal;
	hash = in->hash_func(key);
	bucket_size = (size_t)1 << dict->bucket_bit_count;
	hashed_index = hash & (bucket_size - 1);
	bucket = &dict->buckets[hashed_index];

	if (!bucket->key) return ds_keynotfound;
	if (!in->compare_func(key, bucket->key))
	{
		dict->on_delete_key(bucket->key);
		dict->on_delete_value(bucket->value);
		if (bucket->next)
		{
			*bucket = *bucket->next;
		}
		else
		{
			bucket->hash = 0;
			bucket->key = NULL;
			bucket->value = NULL;
		}
		dict->data_count--;
		return ds_ok;
	}
	else
	{
		dict_bucket_p next = bucket->next;
		while (next)
		{
			if (!in->compare_func(key, next->key))
			{
				dict->on_delete_key(next->key);
				dict->on_delete_value(next->value);
				*bucket = *next;
				free(next);
				dict->data_count--;
				return ds_ok;
			}
			bucket = next;
			next = bucket->next;
		}
		return ds_keynotfound;
	}
}

void dict_delete(dict_p dict)
{
	size_t i;
	size_t bucket_size;
	dict_internal_p in;
	dict_bucket_p bucket, next;

	if (!dict) return;

	bucket_size = (size_t)1 << dict->bucket_bit_count;
	in = dict->internal;
	for (i = 0; i < bucket_size; i++)
	{
		bucket = &dict->buckets[i];
		if (bucket->key)
		{
			dict->on_delete_key(bucket->key);
			dict->on_delete_value(bucket->value);
			if (bucket->next)
			{
				next = bucket->next;
				while (next)
				{
					bucket = next;
					dict->on_delete_key(bucket->key);
					dict->on_delete_value(bucket->value);
					next = bucket->next;
					free(bucket);
				}
			}
		}
	}
	free(dict->buckets);
	free(dict);
}

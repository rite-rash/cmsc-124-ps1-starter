/*
 * dt_map.c: Associative arrays for Unit 5, Section E.
 *
 * An array does not store its indices. This map stores its keys.
 * An array calculates a position with one subtraction.
 * The map calculates a hash and then compares keys in one bucket.
 *
 * Hashing turns the key into a bucket number. Compare all keys in that bucket
 * because two keys can select it. Use a linked list for each bucket. Start the
 * unsigned accumulator at 14695981039346656037ULL. For each unsigned byte,
 * exclusive-or the byte into it and multiply by 1099511628211ULL.
 *
 * A separate list stores insertion order for stable output. dt_map_key_at
 * reads this list. Updating a key preserves its position. Removing and
 * reinserting a key moves it to the end.
 */

#include "dt.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_BUCKET_NUMBER 16
#define DEFAULT_ORDER_SIZE 1024

typedef struct dt_entry {
    char            *key;   //copy of the key from the caller
    dt_value         value; //the value stored for this key
    struct dt_entry *next;  //next entry in the bucket chain
} dt_entry;

struct dt_map {
    dt_entry **buckets;
    size_t     nbuckets;
    char     **order;      //insertion-order array
    size_t     lenkeys;
};

/* Function to hash a key (64-bit FNV-1a) */
static size_t hash(const char *key, size_t nbuckets)
{
    unsigned long long h = 14695981039346656037ULL;
    const unsigned char *p = (const unsigned char *)key;

    while (*p != '\0') {
        //xor the byte in first then multiply by the fnv prime
        h = h ^ (unsigned long long)*p;
        h = h * 1099511628211ULL;
        p++;
    }

    //mod so it fits in the buckets
    size_t result = (size_t)(h % nbuckets);
    return result;
}

/*
 * dt_map_new builds an empty map. It returns NULL after an allocation failure.
 */
dt_map *dt_map_new(void)
{
    /* TODO: Return an allocated empty map. Return NULL after an allocation failure.
       dt_map_new()  -> a map whose dt_map_len is 0
       cases/normal/map_basics.case */

    //allocate memory for the map
    dt_map *m = malloc(sizeof(dt_map));
    if (m == NULL) {
        return NULL;
    }

    //for the buckets
    m->nbuckets = DEFAULT_BUCKET_NUMBER;
    m->buckets = calloc(m->nbuckets, sizeof(dt_entry *));
    if (m->buckets == NULL) {
        free(m);
        return NULL;
    }

    //for the order array
    m->order = malloc(DEFAULT_ORDER_SIZE * sizeof(char *));
    if (m->order == NULL) {
        free(m->buckets);
        free(m);
        return NULL;
    }

    m->lenkeys = 0;
    return m;
}

/*
 * dt_map_free releases each entry, copied key, order array, and map.
 * It accepts NULL. The environment owns the values.
 */
void dt_map_free(dt_map *m)
{
    /* TODO: Release each entry, copied key, order array, and map.
       Preserve the values. The environment owns them.
       a map holding a string value  -> the nodes and keys go, the string stays
       dt_map_free(NULL)             -> returns, having done nothing
       cases/cleanup/map_churn.case */
    if (m == NULL) {
        return;
    }

    size_t i = 0;
    while (i < m->nbuckets) {
        dt_entry *e = m->buckets[i];
        while (e != NULL) {
            //save next before we free e
            dt_entry *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
        i++;
    }

    free(m->order);
    free(m->buckets);
    free(m);
}

/*
 * dt_map_len returns the number of keys in constant time.
 */
size_t dt_map_len(const dt_map *m)
{
    /* TODO: Return the current key count.
       Replacing a value does not change this count.
       after put alpha, beta, gamma:  dt_map_len(m) -> 3
       after put beta again:          dt_map_len(m) -> 3, still
       after del alpha:               dt_map_len(m) -> 2
       cases/normal/map_basics.case */
    return m->lenkeys;
}

/*
 * dt_map_put binds v to key.
 * An existing key keeps its insertion position. A new key becomes the last key.
 * Copy each new key because the caller owns the source buffer.
 * Return DT_ERR_CAPACITY after an allocation failure.
 */
dt_status dt_map_put(dt_map *m, const char *key, dt_value v)
{
    /* TODO: Replace the value for an existing key.
       Add a new entry for a new key. Copy each new key.
       Hash the key. Select its bucket. Search the bucket chain.
       Add a new entry to the chain and insertion list.
       put "beta" -> 2 on an empty map    -> DT_OK, "beta" is last in order
       put "beta" -> 22 on that map       -> DT_OK, same position, new value
       an allocation failure              -> DT_ERR_CAPACITY, map unchanged
       cases/normal/map_basics.case */
    size_t b = hash(key, m->nbuckets);

    //look for the key in the bucket
    dt_entry *e = m->buckets[b];
    while (e != NULL) {
        if (strcmp(e->key, key) == 0) {
            //found it so just change the value
            e->value = v;
            return DT_OK;
        }
        e = e->next;
    }

    //key is new so check if there is room
    if (m->lenkeys == DEFAULT_ORDER_SIZE) {
        return DT_ERR_CAPACITY;
    }

    dt_entry *newentry = malloc(sizeof(dt_entry));
    if (newentry == NULL) {
        return DT_ERR_CAPACITY;
    }

    size_t len = strlen(key) + 1;
    newentry->key = malloc(len);
    if (newentry->key == NULL) {
        free(newentry);
        return DT_ERR_CAPACITY;
    }

    strcpy(newentry->key, key);
    newentry->value = v;

    //put it at the front of the bucket
    newentry->next = m->buckets[b];
    m->buckets[b] = newentry;

    //add to the order array
    m->order[m->lenkeys] = newentry->key;
    m->lenkeys = m->lenkeys + 1;

    return DT_OK;
}

/*
 * dt_map_get writes the value for key to *out.
 * It returns DT_ERR_KEY and does not change *out when the key is absent.
 * An absent key differs from a nil value.
 */
dt_status dt_map_get(const dt_map *m, const char *key, dt_value *out)
{
    /* TODO: Return DT_ERR_KEY when the key is absent.
       Preserve *out after this error. A nil value can be present.
       after put "beta" -> 22:
         dt_map_get(m, "beta", &out)   -> DT_OK, *out is the integer 22
         dt_map_get(m, "ghost", &out)  -> DT_ERR_KEY, *out untouched
       cases/normal/map_basics.case, cases/boundary/map_missing_key.case */
    size_t b = hash(key, m->nbuckets);

    dt_entry *e = m->buckets[b];
    while (e != NULL) {
        if (strcmp(e->key, key) == 0) {
            *out = e->value;
            return DT_OK;
        }
        e = e->next;
    }

    return DT_ERR_KEY;
}

/*
 * dt_map_remove removes key from its bucket and insertion position.
 * It releases the copied key. It returns DT_ERR_KEY when the key is absent.
 */
dt_status dt_map_remove(dt_map *m, const char *key)
{
    /* TODO: Remove the entry from its bucket and insertion position.
       Release the copied key. Return DT_ERR_KEY when the key is absent.
       a map holding alpha, beta, gamma:
         dt_map_remove(m, "alpha")  -> DT_OK, order is now beta, gamma
         dt_map_remove(m, "ghost")  -> DT_ERR_KEY, nothing changes
       reinserting "alpha" appends it after "gamma"
       cases/normal/map_basics.case, cases/boundary/map_remove_missing_key.case */
    size_t b = hash(key, m->nbuckets);

    //prev is the one before e, NULL if e is first
    dt_entry *prev = NULL;
    dt_entry *e = m->buckets[b];

    while (e != NULL) {
        if (strcmp(e->key, key) == 0) {
            //unlink from the bucket chain
            if (prev == NULL) {
                m->buckets[b] = e->next;
            } else {
                prev->next = e->next;
            }

            //find it in the order array and shift everything left
            size_t i = 0;
            while (i < m->lenkeys) {
                //compare pointers because order uses the same key
                if (m->order[i] == e->key) {
                    size_t j = i;
                    while (j < m->lenkeys - 1) {
                        m->order[j] = m->order[j + 1];
                        j++;
                    }
                    break;
                }
                i++;
            }

            m->lenkeys = m->lenkeys - 1;
            free(e->key);
            free(e);
            return DT_OK;
        }

        prev = e;
        e = e->next;
    }

    return DT_ERR_KEY;
}

/*
 * dt_map_key_at writes the key at insertion position index to *out.
 * It returns DT_ERR_RANGE and does not change *out for an invalid index.
 */
dt_status dt_map_key_at(const dt_map *m, size_t index, const char **out)
{
    /* TODO: Write the key at the specified insertion position to *out.
       Return DT_ERR_RANGE for an invalid position. Preserve *out after this error.
       The printer uses this order.
       a map holding alpha, beta, gamma:
         dt_map_key_at(m, 0, &out)  -> DT_OK, *out = "alpha"
         dt_map_key_at(m, 3, &out)  -> DT_ERR_RANGE, *out untouched
       cases/normal/map_basics.case */
    if (index >= m->lenkeys) {
        return DT_ERR_RANGE;
    }

    *out = m->order[index];
    return DT_OK;
}
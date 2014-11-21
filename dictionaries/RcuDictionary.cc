#define _LGPL_SOURCE // this line is just for testing
#define USING_URCU_LIB

#include "urcu.h"
#include "urcu/rculfhash.h"

#define RCU_MB

template<typename T> class Dictionary {
    cds_lfht *internalDictionary;

    Dictionary(Dictionary const &);	// prevent copying
    void operator =(Dictionary const &);

    struct dictNode {
        unsigned int key;
        T *value;
        cds_lfht_node node;
    };

    public:
    Dictionary() {
        internalDictionary = cds_lfht_new(1024, 2, 0, CDS_LFHT_AUTO_RESIZE | CDS_LFHT_ACCOUNTING, NULL);
    }

    void put(unsigned int key, T *v) {
        unsigned long hash = jhash(key); //&value, sizeof(value), seed);
    	dictNode *mp = new dictNode();
    	cds_lfht_node_init(&mp->node);
    	mp->value = v;
    	rcu_read_lock();
    	cds_lfht_add_replace(internalDictionary, hash, match, &key, &mp->node);
    	rcu_read_unlock();
    }

    T *tryGet(unsigned int key) {
        dictNode *mp;
	    cds_lfht_iter iter;
	    T *v = NULL;   
	
    	unsigned long hash = jhash(key); //&value, sizeof(value), seed);
    	rcu_read_lock();
    	cds_lfht_lookup(internalDictionary, hash, match, &key, &iter);
    	cds_lfht_node *ht_node = cds_lfht_iter_get_node(&iter);
    	if (ht_node) {
		    mp = caa_container_of(ht_node, struct dictNode, node);
    		v = mp->value;
	    }
    	rcu_read_unlock();
	    return v;
    }

    private:
    inline unsigned long jhash(unsigned int key) {
        return key;
    }

    static inline int match(struct cds_lfht_node *node, const void *key) {
        struct dictNode *mp = caa_container_of(node, struct dictNode, node);
	    const unsigned int *_key = key;
	    return * _key == mp->key;
    }
};

// Local Variables: //
// compile-command: "g++ -Wall -O3 -DNDEBUG -fno-reorder-functions -DPIN -DDictionary=RcuDictionary Harness.cc -lpthread -lm -lrcu -lrcu-cds -fpermissive" //
// We use fpermissive because of the convertion from void * to unsigned int * type
// End: //

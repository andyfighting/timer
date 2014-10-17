#ifndef _KLIST_H_
#define _KLIST_H_

typedef long long s64;
typedef int       s32;
typedef short     s16;
typedef char      s8;

typedef unsigned long long u64;
typedef unsigned int       u32;
typedef unsigned short     u16;
typedef unsigned char      u8;


#define TRUE      1
#define FALSE     0

#define FAILURE   0
#define SUCCESS   1

#define LOCK_PREFIX  "lock; "

#define mb(...) __sync_synchronize (...)

static inline int atomic_read(int * v)
{
	return (*(volatile int *)(v));
}

static inline void atomic_set(int * v, int i)
{
	*v = i;
}

static inline void atomic_add(int i, int *v)
{
	asm volatile(LOCK_PREFIX "addl %1,%0"
		     : "+m" (*v)
		     : "ir" (i));
}

static inline void atomic_sub(int i, int * v)
{
	asm volatile(LOCK_PREFIX "subl %1,%0"
		     : "+m" (*v)
		     : "ir" (i));
}

/**
 * atomic_sub_and_test - subtract value from variable and test result
 *
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static inline unsigned char atomic_sub_and_test(int i, int *v)
{
	unsigned char c;

	asm volatile(LOCK_PREFIX "subl %2,%0; sete %1"
		     : "+m" (*v), "=qm" (c)
		     : "ir" (i) : "memory");
	return c;
}

/**
 * atomic_inc - increment atomic variable
 *
 * Atomically increments @v by 1.
 */
static inline void atomic_inc(int *v)
{
	asm volatile(LOCK_PREFIX "incl %0"
		     : "+m" (*v));
}

/**
 * atomic_dec - decrement atomic variable
 *
 * Atomically decrements @v by 1.
 */
static inline void atomic_dec(int *v)
{
	asm volatile(LOCK_PREFIX "decl %0"
		     : "+m" (*v));
}

/**
 * atomic_dec_and_test - decrement and test
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */
static inline int atomic_dec_and_test(int *v)
{
	unsigned char c;

	asm volatile(LOCK_PREFIX "decl %0; sete %1"
		     : "+m" (*v), "=qm" (c)
		     : : "memory");
	return c != 0;
}

/**
 * atomic_inc_and_test - increment and test
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
static inline int atomic_inc_and_test(int *v)
{
	unsigned char c;

	asm volatile(LOCK_PREFIX "incl %0; sete %1"
		     : "+m" (*v), "=qm" (c)
		     : : "memory");
	return c != 0;
}

/**
 * atomic_add_negative - add and test if negative
 * @i: integer value to add
 *
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */
static inline int atomic_add_negative(int i, int *v)
{
	unsigned char c;

	asm volatile(LOCK_PREFIX "addl %2,%0; sets %1"
		     : "+m" (*v), "=qm" (c)
		     : "ir" (i) : "memory");
	return c;
}

/**
 * atomic_add_fetch - add integer and return
 * @i: integer value to add
 *
 * Atomically adds @i to @v and returns @i + @v
 */
static inline int atomic_add_fetch(int i, int *v)
{
	return __sync_add_and_fetch(v,i);
}

/**
 * atomic_sub_fetch - subtract integer and return
 * @i: integer value to subtract
 *
 * Atomically subtracts @i from @v and returns @v - @i
 */
static inline int atomic_sub_fetch(int i, int *v)
{
	return atomic_add_fetch(-i, v);
}

#define atomic_inc_fetch(v)  (atomic_add_fetch(1, v))
#define atomic_dec_fetch(v)  (atomic_sub_fetch(1, v))

/**
 * atomic_fetch_add - return and add integer
 * @i: integer value to add
 *
 * Atomically returns @v and adds @i to @v
 */
static inline int atomic_fetch_add(int i, int *v)
{
	return __sync_fetch_and_add(v,i);
}

/**
 * atomic_fetch_sub - subtract integer and return
 * @i: integer value to subtract
 *
 * Atomically returns @v and subtracts @i from @v
 */
static inline int atomic_fetch_sub(int i, int *v)
{
	return atomic_fetch_add(-i, v);
}

#define atomic_fetch_inc(v)  (atomic_fetch_add(1, v))
#define atomic_fetch_dec(v)  (atomic_fetch_sub(1, v))


static inline int atomic_cmpxchg(int *v, int old, int new)
{
	return __sync_val_compare_and_swap(v,old, new);
}

static inline int atomic_xchg(int *v, int new)
{
	return __sync_lock_test_and_set(v, new);
}

//cas.h
static inline unsigned char CAS_PTR(void ** mem,void * oldval,void * newval)
{
#if 0
	return __sync_bool_compare_and_swap(mem,oldval,newval);
#else
	unsigned char result = 0;
	//asm volatile ("lock; cmpxchgl %3, %0; setz %1" : "=m"(mem), "=q" (result) : "m" (*mem), "r" (newval), "a" (oldval) : "memory");
	asm volatile ("lock; cmpxchgl %2, %1;setz %0"
		: "=q" (result), "=m" (*mem)
		: "r" (newval), "m" (*mem), "a" (oldval));
	return result;
#endif
}


static inline unsigned char CAS_32(s32 *mem,s32 oldval,s32 newval)
{
#if 0
	return __sync_bool_compare_and_swap(mem,oldval,newval);
#else
	unsigned char result = 0;
	//asm volatile ("lock; cmpxchgl %3, %0; setz %1" : "=m"(mem), "=q" (result) : "m" (*mem), "r" (newval), "a" (oldval) : "memory");
	//cmpxchgl
	asm volatile ("lock; cmpxchgl %2, %1;setz %0"
		: "=q" (result), "=m" (*mem)
		: "r" (newval), "m" (*mem), "a" (oldval));
	return result;
#endif
}

static inline unsigned char CAS_16(s16 *mem,s16 oldval,s16 newval)
{
#if 0
	return __sync_bool_compare_and_swap(mem,oldval,newval);
#else
	unsigned char result;
	//asm volatile ("lock; cmpxchgl %3, %0; setz %1" : "=m"(mem), "=q" (result) : "m" (*mem), "r" (newval), "a" (oldval) : "memory");
	asm volatile ("lock; cmpxchg %2, %1;setz %0"
		: "=q" (result), "=m" (*mem)
		: "r" (newval), "m" (*mem), "a" (oldval));
	return result;
#endif
}

static inline unsigned char CAS_8(s8 *mem,s8 oldval,s8 newval)
{
#if 0
	return __sync_bool_compare_and_swap(mem,oldval,newval);
#else
	unsigned char result;
	//asm volatile ("lock; cmpxchgl %3, %0; setz %1" : "=m"(mem), "=q" (result) : "m" (*mem), "r" (newval), "a" (oldval) : "memory");
	asm volatile ("lock; cmpxchgb %2, %1;setz %0"
		: "=q" (result), "=m" (*mem)
		: "r" (newval), "m" (*mem), "a" (oldval));
	return result;
#endif
}

//cas_stack.h

struct cas_stack
{
	struct cas_stack * next;
};

static inline void INIT_CAS_STACK(struct cas_stack *stack)
{
	stack->next = NULL;
}

/**
 * caslist_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void cas_stack_push(struct cas_stack *new_node, struct cas_stack *stack_head)
{
	do
	{
		new_node->next = stack_head->next;
	}while (!CAS_PTR((void **)&stack_head->next,new_node->next,new_node));
	return;
}

static inline struct cas_stack * cas_stack_pop(struct cas_stack *stack_head)
{
	struct cas_stack * pop;
	do
	{
		pop = stack_head->next;
	}while (!CAS_PTR((void **)&stack_head->next,pop,pop->next));
	return pop;
}

#define cas_stack_pop_entry(pos,stack_head, member) \
{\
	struct cas_stack * pop;\
	pop = cas_stack_pop(stack_head);\
	if (pop)\
	{\
		pos = container_of(pop,typeof(*pos),member);\
	}\
	else\
	{\
		pos = NULL;\
	}\
}

//////////////////////////////////////////
struct list_head 
{
        struct list_head *next, *prev;
};

#define resoffsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ( { \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - resoffsetof(type,member) ); } )

static inline void prefetch(const void *x) {;}
static inline void prefetchw(const void *x) {;}

#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)



#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
        struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
        (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/*
* Insert a new entry between two known consecutive entries.
*
* This is only for internal list manipulation where we know
* the prev/next entries already!
*/
static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next)
{
        next->prev = new;
        new->next = next;
        new->prev = prev;
        prev->next = new;
}

/**
* list_add - add a new entry
* @new: new entry to be added
* @head: list head to add it after
*
* Insert a new entry after the specified head.
* This is good for implementing stacks.
*/
static inline void list_add(struct list_head *new, struct list_head *head)
{
        __list_add(new, head, head->next);
}

/**
* list_add_tail - add a new entry
* @new: new entry to be added
* @head: list head to add it before
*
* Insert a new entry before the specified head.
* This is useful for implementing queues.
*/
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
        __list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head * prev, struct list_head * next)
{
        next->prev = prev;
        prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
        __list_del(entry->prev, entry->next);
        entry->next = LIST_POISON1;
        entry->prev = LIST_POISON2;
}

static inline void list_del_init(struct list_head *entry)
{
        __list_del(entry->prev, entry->next);
        INIT_LIST_HEAD(entry);
}

static inline void list_move(struct list_head *list, struct list_head *head)
{
        __list_del(list->prev, list->next);
        list_add(list, head);
}

static inline void list_move_tail(struct list_head *list,
                                  struct list_head *head)
{
        __list_del(list->prev, list->next);
        list_add_tail(list, head);
}

static inline int list_empty(const struct list_head *head)
{
        return head->next == head;
}

static inline int list_empty_careful(const struct list_head *head)
{
        struct list_head *next = head->next;
        return (next == head) && (next == head->prev);
}

static inline void __list_splice(struct list_head *list,
                                 struct list_head *head)
{
        struct list_head *first = list->next;
        struct list_head *last = list->prev;
        struct list_head *at = head->next;

        first->prev = head;
        head->next = first;

        last->next = at;
        at->prev = last;
}

/**
* list_splice - join two lists
* @list: the new list to add.
* @head: the place to add it in the first list.
*/
static inline void list_splice(struct list_head *list, struct list_head *head)
{
        if (!list_empty(list))
                __list_splice(list, head);
}

/**
* list_splice_init - join two lists and reinitialise the emptied list.
* @list: the new list to add.
* @head: the place to add it in the first list.
*
* The list at @list is reinitialised
*/
static inline void list_splice_init(struct list_head *list,
                                    struct list_head *head)
{
        if (!list_empty(list)) {
                __list_splice(list, head);
                INIT_LIST_HEAD(list);
        }
}

#define list_entry(ptr, type, member) container_of(ptr, type, member)


#define list_for_each(pos, head) \
        for (pos = (head)->next; prefetch(pos->next), pos != (head); \
                pos = pos->next)

#define __list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_prev(pos, head) \
        for (pos = (head)->prev; prefetch(pos->prev), pos != (head); \
                pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
        for (pos = (head)->next, n = pos->next; pos != (head); \
                pos = n, n = pos->next)

#define list_for_each_entry(pos, head, member)                                \
        for (pos = list_entry((head)->next, typeof(*pos), member);        \
             prefetch(pos->member.next), &pos->member != (head);         \
             pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)                        \
        for (pos = list_entry((head)->prev, typeof(*pos), member);        \
             prefetch(pos->member.prev), &pos->member != (head);         \
             pos = list_entry(pos->member.prev, typeof(*pos), member))

#define list_prepare_entry(pos, head, member) \
        ((pos) ? : list_entry(head, typeof(*pos), member))

#define list_for_each_entry_continue(pos, head, member)                 \
        for (pos = list_entry(pos->member.next, typeof(*pos), member);        \
             prefetch(pos->member.next), &pos->member != (head);        \
             pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)                        \
        for (pos = list_entry((head)->next, typeof(*pos), member),        \
                n = list_entry(pos->member.next, typeof(*pos), member);        \
             &pos->member != (head);                                         \
             pos = n, n = list_entry(n->member.next, typeof(*n), member))

//HASH LIST
struct hlist_head {
        struct hlist_node *first;
};

struct hlist_node {
        struct hlist_node *next, **pprev;
};

#define HLIST_HEAD_INIT { .first = NULL }
#define HLIST_HEAD(name) struct hlist_head name = {  .first = NULL }
#define INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)
#define INIT_HLIST_NODE(ptr) ((ptr)->next = NULL, (ptr)->pprev = NULL)

static inline int hlist_unhashed(const struct hlist_node *h)
{
        return !h->pprev;
}

static inline int hlist_empty(const struct hlist_head *h)
{
        return !h->first;
}

static inline void __hlist_del(struct hlist_node *n)
{
        struct hlist_node *next = n->next;
        struct hlist_node **pprev = n->pprev;
        *pprev = next;
        if (next)
                next->pprev = pprev;
}

static inline void hlist_del(struct hlist_node *n)
{
        __hlist_del(n);
        n->next = LIST_POISON1;
        n->pprev = LIST_POISON2;
}

static inline void hlist_del_init(struct hlist_node *n)
{
        if (n->pprev)  {
                __hlist_del(n);
                INIT_HLIST_NODE(n);
        }
}

static inline void hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
        struct hlist_node *first = h->first;
        n->next = first;
        if (first)
                first->pprev = &n->next;
        h->first = n;
        n->pprev = &h->first;
}


/* next must be != NULL */
static inline void hlist_add_before(struct hlist_node *n,
                                        struct hlist_node *next)
{
        n->pprev = next->pprev;
        n->next = next;
        next->pprev = &n->next;
        *(n->pprev) = n;
}

static inline void hlist_add_after(struct hlist_node *n,
                                        struct hlist_node *next)
{
        next->next = n->next;
        n->next = next;
        next->pprev = &n->next;

        if(next->next)
                next->next->pprev  = &next->next;
}

#define hlist_entry(ptr, type, member) container_of(ptr,type,member)

#define hlist_for_each(pos, head) \
        for (pos = (head)->first; pos && ({ prefetch(pos->next); 1; }); \
             pos = pos->next)

#define hlist_for_each_safe(pos, n, head) \
        for (pos = (head)->first; pos && ({ n = pos->next; 1; }); \
             pos = n)

#define hlist_for_each_entry(tpos, pos, head, member)                         \
        for (pos = (head)->first;                                         \
             pos && ({ prefetch(pos->next); 1;}) &&                         \
                ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
             pos = pos->next)

#define hlist_for_each_entry_continue(tpos, pos, member)                 \
        for (pos = (pos)->next;                                                 \
             pos && ({ prefetch(pos->next); 1;}) &&                         \
                ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
             pos = pos->next)

#define hlist_for_each_entry_from(tpos, pos, member)                         \
        for (; pos && ({ prefetch(pos->next); 1;}) &&                         \
                ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
             pos = pos->next)

#define hlist_for_each_entry_safe(tpos, pos, n, head, member)                  \
        for (pos = (head)->first;                                         \
             pos && ({ n = pos->next; 1; }) &&                                  \
                ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
             pos = n)

//spin_lock.h
#define SPIN_LOCK_LOCKED    (1)
#define SPIN_LOCK_UNLOCKED  (0)
		
typedef struct st_spinlock_t
{
	s32 spin_lock;
}spinlock_t;
		
static inline void spin_unlock(spinlock_t * lock)
{
	while (!CAS_32(&lock->spin_lock,SPIN_LOCK_LOCKED,SPIN_LOCK_UNLOCKED));
}
		
static inline void spin_lock(spinlock_t * lock)
{
	while (CAS_32(&lock->spin_lock,SPIN_LOCK_UNLOCKED,SPIN_LOCK_LOCKED));
}
		
static inline void init_spinlock(spinlock_t * lock)
{
	lock->spin_lock = SPIN_LOCK_UNLOCKED;
}
		
#define SPIN_LOCK_INIT {0}
		
//spin_rwlock.h
typedef struct st_spin_rwlock_t
{
	s32 	   read_ref;
	spinlock_t write_lock;
}spin_rwlock_t;
		
static inline void spin_unlock_r(spin_rwlock_t * lock)
{
	atomic_dec(&lock->read_ref);
}
		
static inline void spin_lock_r(spin_rwlock_t * lock)
{
	spin_lock(&lock->write_lock);
	atomic_inc(&lock->read_ref);
	spin_unlock(&lock->write_lock);
}
		
static inline void spin_unlock_w(spin_rwlock_t * lock)
{
	spin_unlock(&lock->write_lock);
}
		
static inline void spin_lock_w(spin_rwlock_t * lock)
{
	spin_lock(&lock->write_lock);
	while (lock->read_ref);
}
		
		
static inline void init_spin_rwlock(spin_rwlock_t * lock)
{
	lock->read_ref	 = 0;
	init_spinlock(&lock->write_lock);
}
			 
#endif			 

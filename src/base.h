#ifndef INCLUDED_BASE_H
#define INCLUDED_BASE_H

/* semantics */
#define OPTIONAL

#define STATIC_ASSERT(cond) { extern int __static_assert[(cond) ? 1 : -1]; }
#define ARRAY_SIZEOF(arr) (sizeof(arr) / sizeof(arr[0]))

#endif

/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU Affero General Public License in all respects
 * for all of the code used other than OpenSSL.
 */

#ifndef UTIL_MEM_H
#define UTIL_MEM_H

/**
 * General purpose <b>memory allocation</b> methods.
 *
 * @author http://www.tildeslash.com/
 * @see http://www.mmonit.com/
 * @file
 */

/**
 * Allocate <code>n</code> bytes.
 * @param n number of bytes to allocate
 * @return A pointer to the newly allocated memory
 * @exception MemoryException if allocation failed
 * @exception AssertException if <code>n <= 0</code>
 * @see AssertException.h, MemoryException.h
 * @hideinitializer
 */
#define ALLOC(n) Mem_alloc((n), __func__, __FILE__, __LINE__)

/**
 * Allocate <code>c</code> objects of size <code>n</code> each.
 * Same as calling ALLOC(c * n) except this function also clear
 * the memory region before it is returned.
 * @param c number of objects to allocate
 * @param n object size in bytes
 * @return A pointer to the newly allocated memory
 * @exception MemoryException if allocation failed
 * @exception AssertException if <code>c or n <= 0</code>
 * @see AssertException.h, MemoryException.h
 * @hideinitializer
 */
#define CALLOC(c, n) Mem_calloc((c), (n), __func__, __FILE__, __LINE__)

/**
 * Allocate object <code>p</code> and clear the memory region
 * before the allocated object is returned.
 * @param p Object to allocate
 * @exception MemoryException if allocation failed
 * @see AssertException.h, MemoryException.h
 * @hideinitializer
 */
#define NEW(p) ((p) = CALLOC(1, (long)sizeof *(p)))

/**
 * Deallocates <code>p</code>
 * @param p Object to deallocate
 * @hideinitializer
 */
#define FREE(p) ((void)(Mem_free((p), __func__, __FILE__, __LINE__), (p) = 0))

/**
 * Reallocate <code>p</code> with size <code>n</code>.
 * @param p pointer to reallocate
 * @param n new object size in bytes
 * @exception MemoryException if allocation failed
 * @exception AssertException if <code>n <= 0</code>
 * @see AssertException.h, MemoryException.h
 * @hideinitializer
 */
#define RESIZE(p, n) ((p) = Mem_resize((p), (n), __func__, __FILE__, __LINE__))

/**
 * Allocate and return <code>size</code> bytes of memory. If
 * allocation failed this throws a MemoryException
 * @param size The number of bytes to allocate
 * @param func the callee
 * @param file location of caller
 * @param line location of caller
 * @return a pointer to the allocated memory
 * @exception MemoryException if allocation failed
 * @exception AssertException if <code>n <= 0</code>
 * @see AssertException.h, MemoryException.h
 */
void *Mem_alloc(long size, const char *func, const char *file, int line);

/**
 * Allocate and return memory for <code>count</code> objects, each of
 * <code>size</code> bytes. The returned memory is cleared. If allocation
 * failed this method throws a MemoryException
 * @param count The number of objects to allocate
 * @param size The size of each object to allocate
 * @param func the callee
 * @param file location of caller
 * @param line location of caller
 * @return a pointer to the allocated memory
 * @exception MemoryException if allocation failed
 * @exception AssertException if <code>c or n <= 0</code>
 * @see AssertException.h, MemoryException.h
 */
void *Mem_calloc(long count, long size, const char *func, const char *file, int line);

/**
 * Deallocate the memory pointed to by <code>p</code>
 * @param p The memory to deallocate
 * @param func the callee
 * @param file location of caller
 * @param line location of caller
 */
void Mem_free(void *p, const char *func, const char *file, int line);

/**
 * Resize the allocation pointed to by <code>p</code> by <code>size</code>
 * bytes and return the changed allocation. If allocation failed this
 * method throws a MemoryException
 * @param p A pointer to the allocation to change
 * @param size The new size of <code>p</code>
 * @param func the callee
 * @param file location of caller
 * @param line location of caller
 * @return a pointer to the changed memory
 * @exception MemoryException if allocation failed
 * @exception AssertException if <code>n <= 0</code>
 * @see AssertException.h, MemoryException.h
 */
void *Mem_resize(void *p, long size, const char *func, const char *file, int line);

#endif

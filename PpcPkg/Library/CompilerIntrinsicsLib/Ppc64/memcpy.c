/******************************************************************************
 * Copyright (c) 2004, 2008 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

// XXX Replace this with something a bit more optimal...

void * memcpy(void *dest, const void *src, unsigned long n)
{
	char *cdest;
	const char *csrc = src;

	cdest = dest;
	while (n-- > 0) {
		*cdest++ = *csrc++;
	}

	return dest;
}

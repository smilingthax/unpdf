#ifndef _GFXFILES_H
#define _GFXFILES_H

// return 0 on success
// if >filename==NULL stdin resp. stdout is used
// read will allocate memory if *buf==NULL or free and allocate if (*width+7)/8*(*height) too small
int read_pbm(const char *filename,unsigned char **buf,int *width,int *height);
int write_pbm(const char *filename,unsigned char *buf,int width,int height,int plain);

#endif
